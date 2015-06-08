/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <string>
#include <cassert>
#include <fstream>
#include <vector>
#include <stack>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/amdasm/Assembler.h>

#define ASM_OUTPUT_DUMP 1

using namespace CLRX;

static inline const char* skipSpacesToEnd(const char* string, const char* end)
{
    while (string!=end && *string == ' ') string++;
    return string;
}

// extract sybol name or argument name or other identifier
static inline const std::string extractSymName(const char* startString, const char* end,
           bool localLabel)
{
    const char* string = startString;
    if (string != end)
    {
        if((*string >= 'a' && *string <= 'z') || (*string >= 'A' && *string <= 'Z') ||
            *string == '_' || *string == '.' || *string == '$')
            for (string++; string != end && ((*string >= '0' && *string <= '9') ||
                (*string >= 'a' && *string <= 'z') ||
                (*string >= 'A' && *string <= 'Z') || *string == '_' ||
                 *string == '.' || *string == '$') ; string++);
        else if (localLabel && *string >= '0' && *string <= '9') // local label
        {
            for (string++; string!=end && (*string >= '0' && *string <= '9'); string++);
            // check whether is local forward or backward label
            string = (string != end && (*string == 'f' || *string == 'b')) ?
                    string+1 : startString;
            // if not part of binary number or illegal bin number
            if (startString != string && (string!=end &&
                ((*string >= '0' && *string <= '9') ||
                (*string >= 'a' && *string <= 'z') ||
                (*string >= 'A' && *string <= 'Z'))))
                string = startString;
        }
    }
    if (startString == string) // not parsed
        return std::string();
    
    return std::string(startString, string);
}

ISAAssembler::~ISAAssembler()
{ }

AsmSource::~AsmSource()
{ }

AsmFile::~AsmFile()
{ }

AsmMacroSource::~AsmMacroSource()
{ }

AsmRepeatSource::~AsmRepeatSource()
{ }

/* Asm Macro */
AsmMacro::AsmMacro(const AsmSourcePos& _pos, const Array<AsmMacroArg>& _args)
        : contentLineNo(0), pos(_pos), args(_args)
{ }

void AsmMacro::addLine(RefPtr<const AsmSource> source,
           const std::vector<LineTrans>& colTrans, size_t lineSize, const char* line)
{
    content.insert(content.end(), line, line+lineSize);
    if (lineSize > 0 && line[lineSize-1] != '\n')
        content.push_back('\n');
    colTranslations.insert(colTranslations.end(), colTrans.begin(), colTrans.end());
    if (sourceTranslations.empty() || sourceTranslations.back().source != source)
        sourceTranslations.push_back({contentLineNo, source});
    contentLineNo++;
}

/* Asm Repeat */
AsmRepeat::AsmRepeat(const AsmSourcePos& _pos, uint64_t _repeatsNum)
        : contentLineNo(0), pos(_pos), repeatsNum(_repeatsNum)
{ }

void AsmRepeat::addLine(RefPtr<const AsmMacroSubst> macro, RefPtr<const AsmSource> source,
            const std::vector<LineTrans>& colTrans, size_t lineSize, const char* line)
{
    content.insert(content.end(), line, line+lineSize);
    if (lineSize > 0 && line[lineSize-1] != '\n')
        content.push_back('\n');
    colTranslations.insert(colTranslations.end(), colTrans.begin(), colTrans.end());
    if (sourceTranslations.empty() || sourceTranslations.back().source != source ||
        sourceTranslations.back().macro != macro)
        sourceTranslations.push_back({contentLineNo, macro, source});
    contentLineNo++;
}

/* AsmInputFilter */

AsmInputFilter::~AsmInputFilter()
{ }

LineCol AsmInputFilter::translatePos(size_t position) const
{
    auto found = std::lower_bound(colTranslations.rbegin(), colTranslations.rend(),
         LineTrans({ position, 0 }),
         [](const LineTrans& t1, const LineTrans& t2)
         { return t1.position > t2.position; });
    return { found->lineNo, position-found->position+1 };
}

/*
 * AsmStreamInputFilter
 */

static const size_t AsmParserLineMaxSize = 300;

AsmStreamInputFilter::AsmStreamInputFilter(const std::string& filename)
try : managed(true), stream(nullptr), mode(LineMode::NORMAL)
{
    source = RefPtr<const AsmSource>(new AsmFile(filename));
    stream = new std::ifstream(filename.c_str(), std::ios::binary);
    if (!*stream)
        throw Exception("Can't open include file");
    stream->exceptions(std::ios::badbit);
    buffer.reserve(AsmParserLineMaxSize);
}
catch(...)
{
    delete stream;
}

AsmStreamInputFilter::AsmStreamInputFilter(std::istream& is, const std::string& filename)
    :  managed(false), stream(&is), mode(LineMode::NORMAL)
{
    source = RefPtr<const AsmSource>(new AsmFile(filename));
    buffer.reserve(AsmParserLineMaxSize);
}

AsmStreamInputFilter::AsmStreamInputFilter(const AsmSourcePos& pos,
           const std::string& filename)
try : managed(true), stream(nullptr), mode(LineMode::NORMAL)
{
    if (!pos.macro)
        source = RefPtr<const AsmSource>(new AsmFile(pos.source, pos.lineNo, filename));
    else // if inside macro
        source = RefPtr<const AsmSource>(new AsmFile(
            RefPtr<const AsmSource>(new AsmMacroSource(pos.macro, pos.source)),
                 pos.lineNo, filename));
    
    stream = new std::ifstream(filename.c_str(), std::ios::binary);
    if (!*stream)
        throw Exception("Can't open include file");
    stream->exceptions(std::ios::badbit);
    buffer.reserve(AsmParserLineMaxSize);
}
catch(...)
{
    delete stream;
}

AsmStreamInputFilter::AsmStreamInputFilter(const AsmSourcePos& pos, std::istream& is,
        const std::string& filename) : managed(false), stream(&is), mode(LineMode::NORMAL)
{
    if (!pos.macro)
        source = RefPtr<const AsmSource>(new AsmFile(pos.source, pos.lineNo, filename));
    else // if inside macro
        source = RefPtr<const AsmSource>(new AsmFile(
            RefPtr<const AsmSource>(new AsmMacroSource(pos.macro, pos.source)),
                 pos.lineNo, filename));
    buffer.reserve(AsmParserLineMaxSize);
}

AsmStreamInputFilter::~AsmStreamInputFilter()
{
    if (managed)
        delete stream;
}

const char* AsmStreamInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    colTranslations.clear();
    bool endOfLine = false;
    size_t lineStart = pos;
    size_t joinStart = pos;
    size_t destPos = pos;
    size_t backslash = false;
    bool prevAsterisk = false;
    bool asterisk = false;
    colTranslations.push_back({0, lineNo});
    while (!endOfLine)
    {
        switch(mode)
        {
            case LineMode::NORMAL:
            {
                if (pos < buffer.size() && !isSpace(buffer[pos]))
                {   // putting regular string (no spaces)
                    do {
                        backslash = (buffer[pos] == '\\');
                        if (buffer[pos] == '*' &&
                            destPos > 0 && buffer[destPos-1] == '/') // longComment
                        {
                            prevAsterisk = false;
                            asterisk = false;
                            buffer[destPos-1] = ' ';
                            buffer[destPos++] = ' ';
                            mode = LineMode::LONG_COMMENT;
                            pos++;
                            break;
                        }
                        if (buffer[pos] == '#') // line comment
                        {
                            buffer[destPos++] = ' ';
                            mode = LineMode::LINE_COMMENT;
                            pos++;
                            break;
                        }
                        
                        const char old = buffer[pos];
                        buffer[destPos++] = buffer[pos++];
                        
                        if (old == '"') // if string opened
                        {
                            mode = LineMode::STRING;
                            break;
                        }
                        else if (old == '\'') // if string opened
                        {
                            mode = LineMode::LSTRING;
                            break;
                        }
                        
                    } while (pos < buffer.size() && !isSpace(buffer[pos]));
                }
                if (pos < buffer.size())
                {
                    if (buffer[pos] == '\n')
                    {
                        lineNo++;
                        endOfLine = (!backslash);
                        if (backslash) 
                        {
                            destPos--;
                            if (destPos-lineStart == colTranslations.back().position)
                                colTranslations.pop_back();
                            colTranslations.push_back({destPos-lineStart, lineNo});
                        }
                        pos++;
                        joinStart = pos;
                        backslash = false;
                        break;
                    }
                    else if (mode == LineMode::NORMAL)
                    {   /* spaces */
                        backslash = false;
                        do {
                            buffer[destPos++] = ' ';
                            pos++;
                        } while (pos < buffer.size() && buffer[pos] != '\n' &&
                            isSpace(buffer[pos]));
                    }
                }
                break;
            }
            case LineMode::LINE_COMMENT:
            {
                while (pos < buffer.size() && buffer[pos] != '\n')
                {
                    backslash = (buffer[pos] == '\\');
                    pos++;
                    buffer[destPos++] = ' ';
                }
                if (pos < buffer.size())
                {
                    lineNo++;
                    endOfLine = (!backslash);
                    if (backslash)
                    {
                        destPos--;
                        if (destPos-lineStart == colTranslations.back().position)
                            colTranslations.pop_back();
                        colTranslations.push_back({destPos-lineStart, lineNo});
                    }
                    else
                        mode = LineMode::NORMAL;
                    pos++;
                    joinStart = pos;
                    backslash = false;
                }
                break;
            }
            case LineMode::LONG_COMMENT:
            {
                while (pos < buffer.size() && buffer[pos] != '\n' &&
                    (!asterisk || buffer[pos] != '/'))
                {
                    backslash = (buffer[pos] == '\\');
                    prevAsterisk = asterisk;
                    asterisk = (buffer[pos] == '*');
                    pos++;
                    buffer[destPos++] = ' ';
                }
                if (pos < buffer.size())
                {
                    if ((asterisk && buffer[pos] == '/'))
                    {
                        pos++;
                        buffer[destPos++] = ' ';
                        mode = LineMode::NORMAL;
                    }
                    else // newline
                    {
                        lineNo++;
                        endOfLine = (!backslash);
                        if (backslash) 
                        {
                            asterisk = prevAsterisk;
                            prevAsterisk = false;
                            destPos--;
                            if (destPos-lineStart == colTranslations.back().position)
                                colTranslations.pop_back();
                            colTranslations.push_back({destPos-lineStart, lineNo});
                        }
                        pos++;
                        joinStart = pos;
                        backslash = false;
                    }
                }
                break;
            }
            case LineMode::STRING:
            case LineMode::LSTRING:
            {
                const char quoteChar = (mode == LineMode::STRING)?'"':'\'';
                while (pos < buffer.size() && buffer[pos] != '\n' &&
                    ((backslash&1) || buffer[pos] != quoteChar))
                {
                    if (buffer[pos] == '\\')
                        backslash++;
                    else
                        backslash = 0;
                    buffer[destPos++] = buffer[pos];
                    pos++;
                }
                if (pos < buffer.size())
                {
                    if ((backslash&1)==0 && buffer[pos] == quoteChar)
                    {
                        pos++;
                        mode = LineMode::NORMAL;
                        buffer[destPos++] = quoteChar;
                    }
                    else
                    {
                        lineNo++;
                        endOfLine = ((backslash&1)==0);
                        if (backslash&1)
                            colTranslations.push_back({destPos-lineStart, lineNo});
                        else
                            assembler.printWarning({lineNo, pos-joinStart+1},
                                        "Unterminated string: newline inserted");
                        pos++;
                        joinStart = pos;
                    }
                    backslash = false;
                }
                break;
            }
            default:
                break;
        }
        
        if (endOfLine)
            break;
        
        if (pos >= buffer.size())
        {   /* get from buffer */
            if (lineStart != 0)
            {
                std::copy_backward(buffer.begin()+lineStart, buffer.begin()+pos,
                       buffer.begin() + pos-lineStart);
                destPos -= lineStart;
                joinStart -= pos-destPos;
                pos = destPos;
                lineStart = 0;
            }
            if (pos == buffer.size())
                buffer.resize(std::max(AsmParserLineMaxSize, (pos>>1)+pos));
            
            stream->read(buffer.data()+pos, buffer.size()-pos);
            const size_t readed = stream->gcount();
            buffer.resize(pos+readed);
            if (readed == 0)
            {   // end of file. check comments
                if (mode == LineMode::LONG_COMMENT && lineStart!=pos)
                    assembler.printError({lineNo, pos-joinStart+1},
                           "Unterminated multi-line comment");
                if (destPos-lineStart == 0)
                {
                    lineSize = 0;
                    return nullptr;
                }
                break;
            }
        }
    }
    lineSize = destPos-lineStart;
    return buffer.data()+lineStart;
}

AsmMacroInputFilter::AsmMacroInputFilter(const AsmMacro& _macro, const AsmSourcePos& pos,
        const Array<std::pair<std::string, std::string> >& _argMap)
        : macro(_macro), argMap(_argMap), contentLineNo(0), sourceTransIndex(0)
{
    source = macro.getSourceTrans(0).source;
    macroSubst = RefPtr<const AsmMacroSubst>(new AsmMacroSubst(pos.macro,
                   pos.source, pos.lineNo));
    curColTrans = macro.getColTranslations().data();
    buffer.reserve(300);
    lineNo = curColTrans[0].lineNo;
}

const char* AsmMacroInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    buffer.clear();
    colTranslations.clear();
    const std::vector<LineTrans>& macroColTrans = macro.getColTranslations();
    const LineTrans* colTransEnd = macroColTrans.data()+ macroColTrans.size();
    const size_t contentSize = macro.getContent().size();
    if (pos == contentSize)
    {
        lineSize = 0;
        return nullptr;
    }
    
    const char* content = macro.getContent().data();
    
    size_t destPos = 0;
    size_t toCopyPos = pos;
    colTranslations.push_back({0, curColTrans->lineNo});
    size_t colTransThreshold = (curColTrans+1 != colTransEnd) ?
            curColTrans[1].position : SIZE_MAX;
    
    while (pos < contentSize && content[pos] != '\n')
    {
        if (content[pos] != '\\')
        {
            if (pos >= colTransThreshold)
            {
                curColTrans++;
                colTranslations.push_back({destPos + pos-toCopyPos, curColTrans->lineNo});
                colTransThreshold = (curColTrans+1 != colTransEnd) ?
                        curColTrans[1].position : SIZE_MAX;
            }
            pos++;
        }
        else
        {   // backslash
            if (pos >= colTransThreshold)
            {
                curColTrans++;
                colTranslations.push_back({destPos + pos-toCopyPos, curColTrans->lineNo});
                colTransThreshold = (curColTrans+1 != colTransEnd) ?
                        curColTrans[1].position : SIZE_MAX;
            }
            // copy chars to buffer
            if (pos > toCopyPos)
            {
                buffer.resize(destPos + pos-toCopyPos);
                std::copy(content + toCopyPos, content + pos, buffer.begin() + destPos);
                destPos += pos-toCopyPos;
            }
            pos++;
            bool skipColTransBetweenMacroArg = true;
            if (pos < contentSize)
            {
                if (content[pos] == '(' && pos+1 < contentSize && content[pos+1]==')')
                    pos += 2;   // skip this separator
                else
                { // extract argName
                    //ile (content[pos] >= '0'
                    const std::string symName = extractSymName(
                                content+pos, content+contentSize, false);
                    auto it = binaryMapFind(argMap.begin(), argMap.end(), symName);
                    if (it != argMap.end())
                    {   // if found
                        buffer.resize(destPos + it->second.size());
                        std::copy(it->second.begin(), it->second.end(),
                                  buffer.begin()+destPos);
                        destPos += it->second.size();
                        pos += symName.size();
                    }
                    else if (content[pos] == '@')
                    {
                        char numBuf[32];
                        const size_t numLen = itocstrCStyle(assembler.macroCount,
                                numBuf, 32);
                        pos++;
                        buffer.resize(destPos + numLen);
                        std::copy(numBuf, numBuf+numLen, buffer.begin()+destPos);
                        destPos += numLen;
                    }
                    else
                    {
                        buffer.push_back('\\');
                        destPos++;
                        skipColTransBetweenMacroArg = false;
                    }
                }
            }
            toCopyPos = pos;
            // skip colTrans between macroarg or separator
            if (skipColTransBetweenMacroArg)
            {
                while (pos >= colTransThreshold)
                {
                    curColTrans++;
                    colTransThreshold = (curColTrans+1 != colTransEnd) ?
                            curColTrans[1].position : SIZE_MAX;
                }
            }
        }
    }
    if (pos > toCopyPos)
    {
        buffer.resize(destPos + pos-toCopyPos);
        std::copy(content + toCopyPos, content + pos, buffer.begin() + destPos);
        destPos += pos-toCopyPos;
    }
    if (pos < contentSize)
    {
        if (curColTrans+1 != colTransEnd)
            curColTrans++;
        pos++; // skip newline
    }
    lineSize = buffer.size();
    lineNo = curColTrans->lineNo;
    if (sourceTransIndex+1 < macro.getSourceTransSize())
    {
        const AsmMacro::SourceTrans& fpos = macro.getSourceTrans(sourceTransIndex+1);
        if (fpos.lineNo == contentLineNo)
        {
            source = fpos.source;
            sourceTransIndex++;
        }
    }
    contentLineNo++;
    return buffer.data();
}

/*
 * AsmRepeatInputFilter
 */

AsmRepeatInputFilter::AsmRepeatInputFilter(const AsmRepeat& _repeat) :
          repeat(_repeat), repeatCount(0), contentLineNo(0), sourceTransIndex(0)
{
    source = RefPtr<const AsmSource>(new AsmRepeatSource(
                _repeat.getSourceTrans(0).source, 0, repeat.getRepeatsNum()));
    macroSubst = _repeat.getSourceTrans(0).macro;
    curColTrans = repeat.getColTranslations().data();
    lineNo = curColTrans[0].lineNo;
}

const char* AsmRepeatInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    colTranslations.clear();
    const std::vector<LineTrans>& repeatColTrans = repeat.getColTranslations();
    const LineTrans* colTransEnd = repeatColTrans.data()+ repeatColTrans.size();
    const size_t contentSize = repeat.getContent().size();
    if (pos == contentSize)
    {
        repeatCount++;
        if (repeatCount == repeat.getRepeatsNum())
        {
            lineSize = 0;
            return nullptr;
        }
        sourceTransIndex = 0;
        curColTrans = repeat.getColTranslations().data();
        lineNo = curColTrans[0].lineNo;
        pos = 0;
        contentLineNo = 0;
        source = RefPtr<const AsmSource>(new AsmRepeatSource(
            repeat.getSourceTrans(0).source, repeatCount, repeat.getRepeatsNum()));
    }
    const char* content = repeat.getContent().data();
    size_t oldPos = pos;
    while (pos < contentSize && content[pos] != '\n')
        pos++;
    
    lineSize = pos - oldPos; // set new linesize
    if (pos < contentSize)
        pos++; // skip newline
    
    const LineTrans* oldCurColTrans = curColTrans;
    curColTrans++;
    while (curColTrans != colTransEnd && curColTrans->position != 0)
        curColTrans++;
    colTranslations.assign(oldCurColTrans, curColTrans);
    
    lineNo = (curColTrans != colTransEnd) ? curColTrans->lineNo : repeatColTrans[0].lineNo;
    if (sourceTransIndex+1 < repeat.getSourceTransSize())
    {
        const AsmRepeat::SourceTrans& fpos = repeat.getSourceTrans(sourceTransIndex+1);
        if (fpos.lineNo == contentLineNo)
        {
            macroSubst = fpos.macro;
            sourceTransIndex++;
            source = RefPtr<const AsmSource>(new AsmRepeatSource(
                fpos.source, repeatCount, repeat.getRepeatsNum()));
        }
    }
    contentLineNo++;
    return content + oldPos;
}

/*
 * source pos
 */

static void printIndent(std::ostream& os, cxuint indentLevel)
{
    for (; indentLevel != 0; indentLevel--)
        os.write("    ", 4);
}

static RefPtr<const AsmSource> printAsmRepeats(std::ostream& os,
           RefPtr<const AsmSource> source, cxuint indentLevel)
{
    bool firstDepth = true;
    while (source->type == AsmSourceType::REPT)
    {
        RefPtr<const AsmRepeatSource> sourceRept = source.staticCast<const AsmRepeatSource>();
        printIndent(os, indentLevel);
        os.write((firstDepth)?"In repetition ":"              ", 14);
        char numBuf[64];
        size_t size = itocstrCStyle(sourceRept->repeatCount+1, numBuf, 32);
        numBuf[size++] = '/';
        size += itocstrCStyle(sourceRept->repeatsNum, numBuf+size, 32-size);
        numBuf[size++] = ':';
        numBuf[size++] = '\n';
        os.write(numBuf, size);
        source = sourceRept->source;
        firstDepth = false;
    }
    return source;
}

void AsmSourcePos::print(std::ostream& os, cxuint indentLevel) const
{
    if (indentLevel == 10)
    {
        printIndent(os, indentLevel);
        os.write("Can't print all tree trace due to too big depth level\n", 53);
        return;
    }
    char numBuf[32];
    /* print macro tree */
    RefPtr<const AsmMacroSubst> curMacro = macro;
    RefPtr<const AsmMacroSubst> parentMacro;
    while(curMacro)
    {
        const bool firstDepth = (curMacro == macro);
        parentMacro = curMacro->parent;
        
        if (curMacro->source->type != AsmSourceType::MACRO)
        {   /* if file */
            RefPtr<const AsmFile> curFile = curMacro->source.staticCast<const AsmFile>();
            if (curMacro->source->type == AsmSourceType::REPT || curFile->parent)
            {
                if (firstDepth)
                {
                    printIndent(os, indentLevel);
                    os.write("In macro substituted from\n", 26);
                }
                AsmSourcePos nextLevelPos = { RefPtr<const AsmMacroSubst>(),
                    curMacro->source, curMacro->lineNo, 0 };
                nextLevelPos.print(os, indentLevel+1);
                os.write((parentMacro) ? ";\n" : ":\n", 2);
            }
            else
            {
                printIndent(os, indentLevel);
                os.write((firstDepth) ? "In macro substituted from " :
                        "                     from ", 26);
                // leaf
                curFile = curMacro->source.staticCast<const AsmFile>();
                if (!curFile->file.empty())
                    os.write(curFile->file.c_str(), curFile->file.size());
                else // stdin
                    os.write("<stdin>", 7);
                numBuf[0] = ':';
                size_t size = 1+itocstrCStyle(curMacro->lineNo, numBuf+1, 29);
                numBuf[size++] = (parentMacro) ? ';' : ':';
                numBuf[size++] = '\n';
                os.write(numBuf, size);
            }
        }
        else
        {   // if macro
            printIndent(os, indentLevel);
            os.write("In macro substituted from macro content:\n", 41);
            RefPtr<const AsmMacroSource> curMacroPos =
                    curMacro->source.staticCast<const AsmMacroSource>();
            AsmSourcePos macroPos = { curMacroPos->macro, curMacroPos->source,
                curMacro->lineNo, 0 };
            macroPos.print(os, indentLevel+1);
            os.write((parentMacro) ? ";\n" : ":\n", 2);
        }
        
        curMacro = parentMacro;
    }
    /* print source tree */
    RefPtr<const AsmSource> curSource = source;
    while (curSource->type == AsmSourceType::REPT)
        curSource = curSource.staticCast<const AsmRepeatSource>()->source;
    
    if (curSource->type != AsmSourceType::MACRO)
    {   // if file
        RefPtr<const AsmFile> curFile = curSource.staticCast<const AsmFile>();
        if (curFile->parent)
        {
            RefPtr<const AsmSource> parentSource;
            bool firstDepth = true;
            while (curFile->parent)
            {
                parentSource = curFile->parent;
                
                parentSource = printAsmRepeats(os, parentSource, indentLevel);
                if (!firstDepth)
                    firstDepth = (curFile->parent != parentSource); // if repeats
                
                printIndent(os, indentLevel);
                if (parentSource->type != AsmSourceType::MACRO)
                {
                    RefPtr<const AsmFile> parentFile =
                            parentSource.staticCast<const AsmFile>();
                    os.write(firstDepth ? "In file included from " :
                            "                 from ", 22);
                    if (!parentFile->file.empty())
                        os.write(parentFile->file.c_str(), parentFile->file.size());
                    else // stdin
                        os.write("<stdin>", 7);
                    
                    numBuf[0] = ':';
                    size_t size = 1+itocstrCStyle(curFile->lineNo, numBuf+1, 29);
                    curFile = parentFile.staticCast<const AsmFile>();
                    numBuf[size++] = curFile->parent ? ',' : ':';
                    numBuf[size++] = '\n';
                    os.write(numBuf, size);
                    firstDepth = false;
                }
                else
                {   /* if macro */
                    os.write("In file included from macro content:\n", 37);
                    RefPtr<const AsmMacroSource> curMacroPos =
                            parentSource.staticCast<const AsmMacroSource>();
                    AsmSourcePos macroPos = { curMacroPos->macro, curMacroPos->source,
                        curFile->lineNo, 0 };
                    macroPos.print(os, indentLevel+1);
                    os.write(":\n", 2);
                    break;
                }
            }
        }
        // leaf
        printAsmRepeats(os, source, indentLevel);
        printIndent(os, indentLevel);
        if (!curSource.staticCast<const AsmFile>()->file.empty())
            os.write(curSource.staticCast<const AsmFile>()->file.c_str(),
                     curSource.staticCast<const AsmFile>()->file.size());
        else // stdin
            os.write("<stdin>", 7);
        numBuf[0] = ':';
        size_t size = 1+itocstrCStyle(lineNo, numBuf+1, 31);
        os.write(numBuf, size);
        if (colNo != 0)
        {
            numBuf[0] = ':';
            size = 1+itocstrCStyle(colNo, numBuf+1, 29);
            numBuf[size++] = ':';
            numBuf[size++] = ' ';
            os.write(numBuf, size);
        }
    }
    else
    {   // if macro
        printAsmRepeats(os, source, indentLevel);
        printIndent(os, indentLevel);
        os.write("In macro content:\n", 18);
        RefPtr<const AsmMacroSource> curMacroPos =
                curSource.staticCast<const AsmMacroSource>();
        AsmSourcePos macroPos = { curMacroPos->macro, curMacroPos->source, lineNo, colNo };
        macroPos.print(os, indentLevel+1);
    }
}

void AsmSymbol::removeOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex)
{
    auto it = std::remove(occurrencesInExprs.begin(), occurrencesInExprs.end(),
            AsmExprSymbolOccurence{expr, argIndex, opIndex});
    occurrencesInExprs.resize(it-occurrencesInExprs.begin());
}

/*
 * Assembler
 */

Assembler::Assembler(const std::string& filename, std::istream& input, cxuint _flags,
        AsmFormat _format, GPUDeviceType _deviceType, std::ostream& msgStream)
        : format(_format), deviceType(_deviceType), _64bit(false), isaAssembler(nullptr),
          symbolMap({std::make_pair(".", AsmSymbol(0, uint64_t(0)))}),
          flags(_flags), macroCount(0), inclusionLevel(0), macroSubstLevel(0),
          lineSize(0), line(nullptr), lineNo(0), messageStream(msgStream),
          outFormatInitialized(false),
          inGlobal(_format != AsmFormat::RAWCODE),
          inAmdConfig(false), currentKernel(0), currentSection(0),
          // get value reference from first symbol: '.'
          currentOutPos(symbolMap.begin()->second.value)
{
    amdOutput = nullptr;
    input.exceptions(std::ios::badbit);
    currentInputFilter = new AsmStreamInputFilter(input, filename);
    asmInputFilters.push(currentInputFilter);
}

Assembler::~Assembler()
{
    if (amdOutput != nullptr)
    {
        if (format == AsmFormat::GALLIUM)
            delete galliumOutput;
        else if (format == AsmFormat::CATALYST)
            delete amdOutput;
    }
    if (isaAssembler != nullptr)
        delete isaAssembler;
    while (!asmInputFilters.empty())
    {
        delete asmInputFilters.top();
        asmInputFilters.pop();
    }
    
    for (AsmSection* section: sections)
        delete section;
    
    for (AsmRepeat* repeat: repeats)
        delete repeat;
    
    for (auto& entry: symbolMap)    // free pending expressions
        if (!entry.second.isDefined)
            for (auto& occur: entry.second.occurrencesInExprs)
                if (occur.expression!=nullptr)
                {
                    if (!occur.expression->unrefSymOccursNum())
                        delete occur.expression;
                }
}

void Assembler::includeFile(const std::string& filename)
{
}

void Assembler::applyMacro(const std::string& macroName, AsmMacroArgMap argMap)
{
}

void Assembler::exitFromMacro()
{
}

uint64_t Assembler::parseLiteral(const char* string, const char*& outend)
{
    uint64_t value;
    outend = string;
    const char* end = line+lineSize;
    if (outend != end && *outend == '\'')
    {
        outend++;
        if (outend == end)
        {
            printError(string, "Terminated character literal");
            throw ParseException("Terminated character literal");
        }
        if (*outend == '\'')
        {
            printError(string, "Empty character literal");
            throw ParseException("Empty character literal");
        }
        
        if (*outend != '\\')
        {
            value = *outend++;
            if (outend == end || *outend != '\'')
            {
                printError(string, "Missing ''' at end of literal");
                throw ParseException("Missing ''' at end of literal");
            }
            outend++;
            return value;
        }
        else // escapes
        {
            outend++;
            if (outend == end)
            {
                printError(string, "Terminated character literal");
                throw ParseException("Terminated character literal");
            }
            if (*outend == 'x')
            {   // hex
                outend++;
                if (outend == end)
                {
                    printError(string, "Terminated character literal");
                    throw ParseException("Terminated character literal");
                }
                value = 0;
                for (cxuint i = 0; outend != end && i < 2; i++, outend++)
                {
                    cxuint digit;
                    if (*outend >= '0' && *outend <= '9')
                        digit = *outend-'0';
                    else if (*outend >= 'a' && *outend <= 'f')
                        digit = *outend-'a'+10;
                    else if (*outend >= 'A' && *outend <= 'F')
                        digit = *outend-'A'+10;
                    else if (*outend == '\'' && i != 0)
                        break;
                    else
                    {
                        printError(string, "Expected hexadecimal character code");
                        throw ParseException("Expected hexadecimal character code");
                    }
                    value = (value<<4) + digit;
                }
            }
            else if (*outend >= '0' &&  *outend <= '7')
            {   // octal
                value = 0;
                for (cxuint i = 0; outend != end && i < 3 && *outend != '\''; i++, outend++)
                {
                    if (*outend < '0' || *outend > '7')
                    {
                        printError(string, "Expected octal character code");
                        throw ParseException("Expected octal character code");
                    }
                    value = (value<<3) + uint64_t(*outend-'0');
                    if (value > 255)
                    {
                        printError(string, "Octal code out of range");
                        throw ParseException("Octal code out of range");
                    }
                }
            }
            else
            {   // normal escapes
                const char c = *outend++;
                switch (c)
                {
                    case 'a':
                        value = '\a';
                        break;
                    case 'b':
                        value = '\b';
                        break;
                    case 'r':
                        value = '\r';
                        break;
                    case 'n':
                        value = '\n';
                        break;
                    case 'f':
                        value = '\f';
                        break;
                    case 'v':
                        value = '\v';
                        break;
                    case 't':
                        value = '\t';
                        break;
                    case '\\':
                        value = '\\';
                        break;
                    case '\'':
                        value = '\'';
                        break;
                    case '\"':
                        value = '\"';
                        break;
                    default:
                        value = c;
                }
            }
            if (outend == end || *outend != '\'')
            {
                printError(string, "Missing ''' at end of literal");
                throw ParseException("Missing ''' at end of literal");
            }
            outend++;
            return value;
        }
    }
    try
    { value = cstrtovCStyle<uint64_t>(string, line+lineSize, outend); }
    catch(const ParseException& ex)
    {
        printError(string, ex.what());
        throw;
    }
    return value;
}

AsmSymbolEntry* Assembler::parseSymbol(const char* string, bool localLabel)
{
    AsmSymbolEntry* entry = nullptr;
    const std::string symName = extractSymName(string, line+lineSize, localLabel);
    if (symName.empty())
        return nullptr;
    std::pair<AsmSymbolMap::iterator, bool> res =
            symbolMap.insert({ symName, AsmSymbol()});
    if (!res.second)
    {   // if found
        AsmSymbolMap::iterator it = res.first;
        AsmSymbol& sym = it->second;
        sym.occurrences.push_back(getSourcePos(string-line));
        entry = &*it;
    }
    else // if new symbol has been put
        entry = &*res.first;
    return entry;
}

bool Assembler::setSymbol(AsmSymbolEntry& symEntry, uint64_t value)
{
    symEntry.second.value = value;
    symEntry.second.isDefined = true;
    bool good = true;
    // resolve value of pending symbols
    std::stack<std::pair<AsmSymbol*, size_t> > symbolStack;
    symbolStack.push(std::make_pair(&symEntry.second, 0));
    symEntry.second.resolving = true;
    
    while (!symbolStack.empty())
    {
        std::pair<AsmSymbol*, size_t>& entry = symbolStack.top();
        if (entry.second < entry.first->occurrencesInExprs.size())
        {   // 
            AsmExprSymbolOccurence& occurrence =
                    entry.first->occurrencesInExprs[entry.second];
            AsmExpression* expr = occurrence.expression;
            expr->substituteOccurrence(occurrence, entry.first->value);
            entry.second++;
            
            if (!expr->unrefSymOccursNum())
            {   // expresion has been fully resolved
                uint64_t value;
                if (!expr->evaluate(*this, value))
                {   // if failed
                    delete occurrence.expression; // delete expression
                    occurrence.expression = nullptr; // clear expression
                    good = false;
                    continue;
                }
                const AsmExprTarget& target = expr->getTarget();
                switch(target.type)
                {
                    case ASMXTGT_SYMBOL:
                    {    // resolve symbol
                        AsmSymbolEntry& curSymEntry = *target.symbol;
                        if (!curSymEntry.second.resolving)
                        {
                            curSymEntry.second.value = value;
                            curSymEntry.second.isDefined = true;
                            symbolStack.push(std::make_pair(&curSymEntry.second, 0));
                            curSymEntry.second.resolving = true;
                        }
                        // otherwise we ignore circular dependencies
                        break;
                    }
                    case ASMXTGT_DATA8:
                        sections[target.sectionId]->content[target.offset] =
                                cxbyte(value);
                        break;
                    case ASMXTGT_DATA16:
                        SULEV(*reinterpret_cast<uint16_t*>(sections[target.sectionId]
                                ->content.data() + target.offset), uint16_t(value));
                        break;
                    case ASMXTGT_DATA32:
                        SULEV(*reinterpret_cast<uint32_t*>(sections[target.sectionId]
                                ->content.data() + target.offset), uint32_t(value));
                        break;
                    case ASMXTGT_DATA64:
                        SULEV(*reinterpret_cast<uint64_t*>(sections[target.sectionId]
                                ->content.data() + target.offset), uint64_t(value));
                        break;
                    default: // ISA assembler resolves this dependency
                        isaAssembler->resolveCode(sections[target.sectionId]
                                ->content.data() + target.offset, target.type, value);
                        break;
                }
                delete occurrence.expression; // delete expression
            }
            occurrence.expression = nullptr; // clear expression
        }
        else // pop
        {
            entry.first->resolving = false;
            entry.first->occurrencesInExprs.clear();
            symbolStack.pop();
        }
    }
    return good;
}

bool Assembler::assignSymbol(const std::string& symbolName, const char* stringAtSymbol,
             const char* string)
{
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(*this, string, string));
    string = skipSpacesToEnd(string, line+lineSize);
    
    if (!expr) // no expression, errors
        return false;
    if (string != line+lineSize)
    {
        printError(string, "Garbages at end of expression");
        return false;
    }
    
    std::pair<AsmSymbolMap::iterator, bool> res =
            symbolMap.insert({ symbolName, AsmSymbol() });
    if (!res.second && res.first->second.onceDefined)
    {   // found and can be only once defined
        std::string msg = "Label '";
        msg += symbolName;
        msg += "' is already defined";
        printError(stringAtSymbol, msg.c_str());
        return false;
    }
    AsmSymbolEntry& symEntry = *res.first;
    
    if (expr->getSymOccursNum()==0)
    {   // can evalute, assign now
        uint64_t value;
        if (!expr->evaluate(*this, value))
            return false;
        setSymbol(symEntry, value);
    }
    else // set expression
    {
        expr->setTarget(AsmExprTarget::symbolTarget(&symEntry));
        symEntry.second.expression = expr.release();
        symEntry.second.isDefined = false;
    }
    return true;
}

void Assembler::printWarning(const AsmSourcePos& pos, const char* message)
{
    if ((flags & ASM_WARNINGS) == 0)
        return; // do nothing
    pos.print(messageStream);
    messageStream.write("Warning: ", 9);
    messageStream.write(message, ::strlen(message));
    messageStream.put('\n');
}

void Assembler::printError(const AsmSourcePos& pos, const char* message)
{
    pos.print(messageStream);
    messageStream.write("Error: ", 7);
    messageStream.write(message, ::strlen(message));
    messageStream.put('\n');
}

void Assembler::addIncludeDir(const std::string& includeDir)
{
    includeDirs.push_back(std::string(includeDir));
}

void Assembler::addInitialDefSym(const std::string& symName, uint64_t value)
{
    defSyms.push_back({symName, value});
}

bool Assembler::readLine()
{
    // input filters
    lineNo = currentInputFilter->getLineNo();
    line = currentInputFilter->readLine(*this, lineSize);
    while (line == nullptr)
    {   // no line
        if (asmInputFilters.size() > 1)
        {
            delete asmInputFilters.top();
            asmInputFilters.pop();
        }
        else
            return false;
        currentInputFilter = asmInputFilters.top();
        lineNo = currentInputFilter->getLineNo();
        line = currentInputFilter->readLine(*this, lineSize);
    }
    return true;
}

void Assembler::initializeOutputFormat()
{
    if (outFormatInitialized)
        return;
    outFormatInitialized = true;
    if (format == AsmFormat::CATALYST && amdOutput == nullptr)
    {   // if not created
        amdOutput = new AmdInput();
        amdOutput->is64Bit = _64bit;
        amdOutput->deviceType = deviceType;
        amdOutput->globalData = nullptr;
        amdOutput->globalDataSize = 0;
        amdOutput->sourceCode = nullptr;
        amdOutput->sourceCodeSize = 0;
        amdOutput->llvmir = nullptr;
        amdOutput->llvmirSize = 0;
        /* sections */
        sections.push_back(new AsmSection{ 0, AsmSectionType::AMD_GLOBAL_DATA });
    }
    else if (format == AsmFormat::GALLIUM && galliumOutput == nullptr)
    {
        galliumOutput = new GalliumInput();
        galliumOutput->code = nullptr;
        galliumOutput->codeSize = 0;
        galliumOutput->disassembly = nullptr;
        galliumOutput->disassemblySize = 0;
        galliumOutput->commentSize = 0;
        galliumOutput->comment = nullptr;
        sections.push_back(new AsmSection{ 0, AsmSectionType::GALLIUM_CODE});
    }
    else if (format == AsmFormat::GALLIUM && rawCode == nullptr)
    {
        sections.push_back(new AsmSection{ 0, AsmSectionType::RAWCODE_CODE});
        rawCode = sections[0];
    }
    else // FAIL
        abort();
    // define counter symbol
    symbolMap.find(".")->second.sectionId = 0;
}

static const char* pseudoOpNamesTbl[] =
{
    "32bit",
    "64bit",
    "abort",
    "align",
    "arch",
    "ascii",
    "asciz",
    "balign",
    "balignl",
    "balignw",
    "byte",
    "catalyst",
    "config",
    "data",
    "double",
    "else",
    "elseif",
    "elseifb",
    "elseifc",
    "elseifdef",
    "elseifeq",
    "elseifeqs",
    "elseifge",
    "elseifgt",
    "elseifle",
    "elseiflt",
    "elseifnb",
    "elseifnc",
    "elseifndef",
    "elseifne",
    "elseifnes",
    "elseifnotdef",
    "end",
    "endfunc",
    "endif",
    "endm",
    "endr",
    "equ",
    "equiv",
    "eqv",
    "err",
    "error",
    "exitm",
    "extern",
    "fail",
    "file",
    "fill",
    "float",
    "format",
    "func",
    "gallium",
    "global",
    "gpu",
    "half",
    "hword",
    "if",
    "ifb",
    "ifc",
    "ifdef",
    "ifeq",
    "ifeqs",
    "ifge",
    "ifgt",
    "ifle",
    "iflt",
    "ifnb",
    "ifnc",
    "ifndef",
    "ifne",
    "ifnes",
    "ifnotdef",
    "incbin",
    "include",
    "int",
    "kernel",
    "line",
    "ln",
    "loc",
    "local",
    "long",
    "macro",
    "octa",
    "offset",
    "org",
    "p2align",
    "print",
    "purgem",
    "quad",
    "rawcode",
    "rept",
    "section",
    "set",
    "short",
    "single",
    "size",
    "skip",
    "space",
    "string",
    "string16",
    "string32",
    "struct",
    "text",
    "title",
    "warning",
    "word"
};

enum
{
    ASMOP_32BIT = 0,
    ASMOP_64BIT,
    ASMOP_ABORT,
    ASMOP_ALIGN,
    ASMOP_ARCH,
    ASMOP_ASCII,
    ASMOP_ASCIZ,
    ASMOP_BALIGN,
    ASMOP_BALIGNL,
    ASMOP_BALIGNW,
    ASMOP_BYTE,
    ASMOP_CATALYST,
    ASMOP_CONFIG,
    ASMOP_DATA,
    ASMOP_DOUBLE,
    ASMOP_ELSE,
    ASMOP_ELSEIF,
    ASMOP_ELSEIFB,
    ASMOP_ELSEIFC,
    ASMOP_ELSEIFDEF,
    ASMOP_ELSEIFEQ,
    ASMOP_ELSEIFEQS,
    ASMOP_ELSEIFGE,
    ASMOP_ELSEIFGT,
    ASMOP_ELSEIFLE,
    ASMOP_ELSEIFLT,
    ASMOP_ELSEIFNB,
    ASMOP_ELSEIFNC,
    ASMOP_ELSEIFNDEF,
    ASMOP_ELSEIFNE,
    ASMOP_ELSEIFNES,
    ASMOP_ELSEIFNOTDEF,
    ASMOP_END,
    ASMOP_ENDFUNC,
    ASMOP_ENDIF,
    ASMOP_ENDM,
    ASMOP_ENDR,
    ASMOP_EQU,
    ASMOP_EQUIV,
    ASMOP_EQV,
    ASMOP_ERR,
    ASMOP_ERROR,
    ASMOP_EXITM,
    ASMOP_EXTERN,
    ASMOP_FAIL,
    ASMOP_FILE,
    ASMOP_FILL,
    ASMOP_FLOAT,
    ASMOP_FORMAT,
    ASMOP_FUNC,
    ASMOP_GALLIUM,
    ASMOP_GLOBAL,
    ASMOP_GPU,
    ASMOP_HALF,
    ASMOP_HWORD,
    ASMOP_IF,
    ASMOP_IFB,
    ASMOP_IFC,
    ASMOP_IFDEF,
    ASMOP_IFEQ,
    ASMOP_IFEQS,
    ASMOP_IFGE,
    ASMOP_IFGT,
    ASMOP_IFLE,
    ASMOP_IFLT,
    ASMOP_IFNB,
    ASMOP_IFNC,
    ASMOP_IFNDEF,
    ASMOP_IFNE,
    ASMOP_IFNES,
    ASMOP_IFNOTDEF,
    ASMOP_INCBIN,
    ASMOP_INCLUDE,
    ASMOP_INT,
    ASMOP_KERNEL,
    ASMOP_LINE,
    ASMOP_LN,
    ASMOP_LOC,
    ASMOP_LOCAL,
    ASMOP_LONG,
    ASMOP_MACRO,
    ASMOP_OCTA,
    ASMOP_OFFSET,
    ASMOP_ORG,
    ASMOP_P2ALIGN,
    ASMOP_PRINT,
    ASMOP_PURGEM,
    ASMOP_QUAD,
    ASMOP_RAWCODE,
    ASMOP_REPT,
    ASMOP_SECTION,
    ASMOP_SET,
    ASMOP_SHORT,
    ASMOP_SINGLE,
    ASMOP_SIZE,
    ASMOP_SKIP,
    ASMOP_SPACE,
    ASMOP_STRING,
    ASMOP_STRING16,
    ASMOP_STRING32,
    ASMOP_STRUCT,
    ASMOP_TEXT,
    ASMOP_TITLE,
    ASMOP_WARNING,
    ASMOP_WORD
};

bool Assembler::assemble()
{
    bool good = true;
    while (readLine())
    {
        /* parse line */
        const char* string = line;
        const char* end = line+lineSize;
        string = skipSpacesToEnd(string, end);
        if (string == end)
            continue; // only empty line
        
        const char* firstNameString = string;
        std::string firstName = extractSymName(string, end, false);
        if (firstName.empty())
        {
            const char* startString = string;
            // try to parse local label
            while (string != end && *string >= '0' && *string <= '9') string++;
            if (string == startString)
            {   // error
                good = false;
                printError(string, "Syntax error");
                continue;
            }
            firstName.assign(startString, string);
        }
        else
            string += firstName.size(); // skip this name
        string = skipSpacesToEnd(string, end);
        if (string != end && *string == '=' &&
            // not for local labels
            (firstName.front() < '0' || firstName.front() > '9'))
        {   // assignment
            string = skipSpacesToEnd(string+1, line+lineSize);
            if (string == end)
            {
                good = false;
                printError(string, "Expected assignment expression");
                continue;
            }
            if (!assignSymbol(firstName, firstNameString, string))
                good = false;
            continue;
        }
        else if (string != end && *string == ':')
        {   // labels
            initializeOutputFormat();
            string = skipSpacesToEnd(string+1, line+lineSize);
            if (string != end)
            {
                good = false;
                printError(string, "Expected assignment expression");
                continue;
            }
            if (firstName.front() >= '0' && firstName.front() <= '9')
            {   // handle local labels
                if (sections.empty())
                {
                    printError(firstNameString,
                               "Local label can't be defined outside section");
                    good = false;
                    continue;
                }
                if (inAmdConfig)
                {
                    printError(firstNameString,
                               "Local label can't defined in AMD config place");
                    good = false;
                    continue;
                }
                std::pair<AsmSymbolMap::iterator, bool> prevLRes =
                        symbolMap.insert({ firstName+"b", AsmSymbol() });
                std::pair<AsmSymbolMap::iterator, bool> nextLRes =
                        symbolMap.insert({ firstName+"f", AsmSymbol() });
                assert(setSymbol(*nextLRes.first, currentOutPos));
                prevLRes.first->second.value = nextLRes.first->second.value;
                prevLRes.first->second.sectionId = currentSection;
                nextLRes.first->second.isDefined = false;
            }
            else
            {   // regular labels
                std::pair<AsmSymbolMap::iterator, bool> res = 
                        symbolMap.insert({ firstName, AsmSymbol() });
                if (!res.second)
                {   // found
                    if (res.first->second.onceDefined)
                    {   // if label
                        std::string msg = "Label '";
                        msg += firstName;
                        msg += "' is already defined";
                        printError(firstNameString, msg.c_str());
                        good = false;
                        continue;
                    }
                }
                if (sections.empty())
                {
                    printError(firstNameString,
                               "Label can't be defined outside section");
                    good = false;
                    continue;
                }
                if (inAmdConfig)
                {
                    printError(firstNameString,
                               "Label can't defined in AMD config place");
                    good = false;
                    continue;
                }
                
                if (!setSymbol(*res.first, currentOutPos))
                {   // if symbol not set
                    good = false;
                    continue;
                }
                res.first->second.onceDefined = true;
                res.first->second.sectionId = currentSection;
            }
            continue;
        }
        // make firstname as lowercase
        toLowerString(firstName);
        
        if (firstName.size() > 2 && firstName[0] == '.')
        {   // check for pseudo-op
            const size_t pseudoOp = binaryFind(pseudoOpNamesTbl, pseudoOpNamesTbl +
                    sizeof(pseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - pseudoOpNamesTbl;
            
            switch(pseudoOp)
            {
                case ASMOP_32BIT:
                case ASMOP_64BIT:
                    if (outFormatInitialized)
                    {
                        printError(string, "Bitness has already been defined");
                        good = false;
                    }
                    if (format != AsmFormat::CATALYST)
                        printWarning(string,
                             "Bitness ignored for other formats than AMD Catalyst");
                    else
                        _64bit = (pseudoOp == ASMOP_64BIT);
                    break;
                case ASMOP_ABORT:
                    break;
                case ASMOP_ALIGN:
                    break;
                case ASMOP_ARCH:
                    break;
                case ASMOP_ASCII:
                    break;
                case ASMOP_ASCIZ:
                    break;
                case ASMOP_BALIGN:
                    break;
                case ASMOP_BALIGNL:
                    break;
                case ASMOP_BALIGNW:
                    break;
                case ASMOP_BYTE:
                    break;
                case ASMOP_RAWCODE:
                case ASMOP_CATALYST:
                case ASMOP_GALLIUM:
                    if (outFormatInitialized)
                    {
                        printError(string, "Output format type has already been defined");
                        good = false;
                    }
                    format = (pseudoOp == ASMOP_GALLIUM) ? AsmFormat::GALLIUM :
                        (pseudoOp == ASMOP_CATALYST) ? AsmFormat::CATALYST :
                        AsmFormat::RAWCODE;
                    break;
                case ASMOP_CONFIG:
                    if (format == AsmFormat::CATALYST)
                    {
                        initializeOutputFormat();
                        if (inGlobal)
                        {
                            printError(string,
                                   "Configuration in global layout is illegal");
                            good = false;
                        }
                        else
                            inAmdConfig = true; // inside Amd Config
                    }
                    else
                    {
                        printError(string,
                           "Configuration section only for AMD Catalyst binaries");
                        good = false;
                    }
                    break;
                case ASMOP_DATA:
                    break;
                case ASMOP_DOUBLE:
                    break;
                case ASMOP_ELSE:
                    break;
                case ASMOP_ELSEIF:
                    break;
                case ASMOP_ELSEIFB:
                    break;
                case ASMOP_ELSEIFC:
                    break;
                case ASMOP_ELSEIFDEF:
                    break;
                case ASMOP_ELSEIFEQ:
                    break;
                case ASMOP_ELSEIFEQS:
                    break;
                case ASMOP_ELSEIFGE:
                    break;
                case ASMOP_ELSEIFGT:
                    break;
                case ASMOP_ELSEIFLE:
                    break;
                case ASMOP_ELSEIFLT:
                    break;
                case ASMOP_ELSEIFNB:
                    break;
                case ASMOP_ELSEIFNC:
                    break;
                case ASMOP_ELSEIFNDEF:
                    break;
                case ASMOP_ELSEIFNE:
                    break;
                case ASMOP_ELSEIFNES:
                    break;
                case ASMOP_ELSEIFNOTDEF:
                    break;
                case ASMOP_END:
                    break;
                case ASMOP_ENDFUNC:
                    break;
                case ASMOP_ENDIF:
                    break;
                case ASMOP_ENDM:
                    break;
                case ASMOP_ENDR:
                    break;
                case ASMOP_EQU:
                    break;
                case ASMOP_EQUIV:
                case ASMOP_EQV:
                    break;
                case ASMOP_ERR:
                    break;
                case ASMOP_ERROR:
                    break;
                case ASMOP_EXITM:
                    break;
                case ASMOP_EXTERN:
                    break;
                case ASMOP_FAIL:
                    break;
                case ASMOP_FILE:
                    break;
                case ASMOP_FILL:
                    break;
                case ASMOP_FLOAT:
                    break;
                case ASMOP_FORMAT:
                {
                    string = skipSpacesToEnd(string, end);
                    if (string == end)
                    {
                        printError(string, "Expected output format type");
                        good = false;
                    }
                    std::string formatName = extractSymName(string, end, false);
                    toLowerString(formatName);
                    if (formatName == "catalyst" || formatName == "amd")
                        format = AsmFormat::CATALYST;
                    else if (formatName == "gallium")
                        format = AsmFormat::GALLIUM;
                    else if (formatName == "raw")
                        format = AsmFormat::RAWCODE;
                    else
                    {
                        printError(string, "Unknown output format type");
                        good = false;
                    }
                    if (outFormatInitialized)
                    {
                        printError(string, "Output format type has already been defined");
                        good = false;
                    }
                    break;
                }
                case ASMOP_FUNC:
                    break;
                case ASMOP_GLOBAL:
                    break;
                case ASMOP_GPU:
                    break;
                case ASMOP_HALF:
                    break;
                case ASMOP_HWORD:
                    break;
                case ASMOP_IF:
                    break;
                case ASMOP_IFB:
                    break;
                case ASMOP_IFC:
                    break;
                case ASMOP_IFDEF:
                    break;
                case ASMOP_IFEQ:
                    break;
                case ASMOP_IFEQS:
                    break;
                case ASMOP_IFGE:
                    break;
                case ASMOP_IFGT:
                    break;
                case ASMOP_IFLE:
                    break;
                case ASMOP_IFLT:
                    break;
                case ASMOP_IFNB:
                    break;
                case ASMOP_IFNC:
                    break;
                case ASMOP_IFNDEF:
                    break;
                case ASMOP_IFNE:
                    break;
                case ASMOP_IFNES:
                    break;
                case ASMOP_IFNOTDEF:
                    break;
                case ASMOP_INCBIN:
                    break;
                case ASMOP_INCLUDE:
                    break;
                case ASMOP_INT:
                {
                    string = skipSpacesToEnd(string, end);
                    while (string != end)
                    {
                        std::unique_ptr<AsmExpression> expr(AsmExpression::parse(
                                    *this, string, string));
                        if (expr)
                        {
                            if (expr->getSymOccursNum()==0)
                            {   // put directly to section
                                uint64_t value;
                                if (expr->evaluate(*this, value))
                                {
                                    uint32_t out;
                                    SLEV(out, value);
                                    putData(4, reinterpret_cast<const cxbyte*>(&out));
                                }
                            }
                            else // expression
                            {
                                expr->setTarget(AsmExprTarget::data32Target(
                                                currentSection, currentOutPos));
                                expr.release();
                                reserveData(4);
                            }
                        }
                        else // fail
                            good = false;
                        string = skipSpacesToEnd(string, end); // spaces before ','
                        if (string == end)
                            break;
                        if (*string != ',')
                        {
                            printError(string, "Expected ',' before next value");
                            good = false;
                        }
                        else
                            string = skipSpacesToEnd(string+1, end);
                    }
                    break;
                }
                case ASMOP_KERNEL:
                    if (format == AsmFormat::CATALYST || format == AsmFormat::GALLIUM)
                    {
                        string = skipSpacesToEnd(string, end);
                        if (string == end)
                        {
                            printError(string, "Expected kernel name");
                            good = false;
                            break;
                        }
                        std::string kernelName = extractSymName(string, end, false);
                        if (kernelName.empty())
                        {
                            printError(string, "This is not kernel name");
                            good = false;
                            break;
                        }
                        if (format == AsmFormat::CATALYST)
                        {
                            kernelMap.insert(std::make_pair(kernelName,
                                            amdOutput->kernels.size()));
                            amdOutput->addEmptyKernel(kernelName.c_str());
                        }
                        else
                        {
                            kernelMap.insert(std::make_pair(kernelName,
                                        galliumOutput->kernels.size()));
                            //galliumOutput->addEmptyKernel(kernelName.c_str());
                        }
                    }
                    else if (format == AsmFormat::RAWCODE)
                    {
                        printError(string, "Raw code can have only one unnamed kernel");
                        good = false;
                    }
                    break;
                case ASMOP_LINE:
                case ASMOP_LN:
                    break;
                case ASMOP_LOC:
                    break;
                case ASMOP_LOCAL:
                    break;
                case ASMOP_LONG:
                    break;
                case ASMOP_MACRO:
                    break;
                case ASMOP_OCTA:
                    break;
                case ASMOP_OFFSET:
                    break;
                case ASMOP_ORG:
                    break;
                case ASMOP_P2ALIGN:
                    break;
                case ASMOP_PRINT:
                    break;
                case ASMOP_PURGEM:
                    break;
                case ASMOP_QUAD:
                    break;
                case ASMOP_REPT:
                    break;
                case ASMOP_SECTION:
                    break;
                case ASMOP_SET:
                    break;
                case ASMOP_SHORT:
                    break;
                case ASMOP_SINGLE:
                    break;
                case ASMOP_SIZE:
                    break;
                case ASMOP_SKIP:
                    break;
                case ASMOP_SPACE:
                    break;
                case ASMOP_STRING:
                    break;
                case ASMOP_STRING16:
                    break;
                case ASMOP_STRING32:
                    break;
                case ASMOP_STRUCT:
                    break;
                case ASMOP_TEXT:
                    break;
                case ASMOP_TITLE:
                    break;
                case ASMOP_WARNING:
                    break;
                case ASMOP_WORD:
                    break;
                default:
                    // macro substitution
                    break;
            }
        }
        else
        {   // try to parse processor instruction or macro substitution
        }
        
        // check garbages at line and print error
        string = skipSpacesToEnd(string, end);
        if (string != end)
        {   // garbages
            printError(string, "Garbages at end of line with pseud-op");
            good = false;
        }
    }
#ifdef ASM_OUTPUT_DUMP
    {
        std::vector<const AsmSymbolEntry*> symEntries;
        for (const AsmSymbolEntry& symEntry: symbolMap)
            symEntries.push_back(&symEntry);
        std::sort(symEntries.begin(), symEntries.end(),
                  [](const AsmSymbolEntry* s1, const AsmSymbolEntry* s2)
                  { return s1->first < s2->first; });
        
        for (const AsmSymbolEntry* symEntryPtr: symEntries)
        {
            const AsmSymbolEntry& symEntry = *symEntryPtr;
            if (symEntry.second.isDefined)
                std::cerr << symEntry.first << " = " << symEntry.second.value <<
                        "(0x" << std::hex << symEntry.second.value << ")" << std::dec;
            else
                std::cerr << symEntry.first << " undefined";
            if (symEntry.second.onceDefined)
                std::cerr << " onceDefined";
            if (symEntry.second.sectionId == ASMSECT_ABS)
                std::cerr << ", sect=ABS";
            else if (sections.size() < symEntry.second.sectionId)
                std::cerr << ", secttype=" <<
                        cxuint(sections[symEntry.second.sectionId]->type);
            else
                std::cerr << ", sect=" << symEntry.second.sectionId;
            std::cerr << std::endl;
        }
        
        /* print sections */
        for (const AsmSection* sectionPtr: sections)
        {
            const AsmSection& section = *sectionPtr;
            std::cerr << "SectionType: " << cxuint(section.type) << std::endl;
            for (size_t i = 0; i < section.content.size(); i+=8)
            {
                char buf[64];
                size_t bufsz = itocstrCStyle(i, buf, 64, 16, 16, false);
                std::cerr.write(buf, bufsz);
                std::cerr.put(' ');
                for (size_t j = i; j < i+8 && j < section.content.size(); j++)
                {
                    size_t bufsz = itocstrCStyle(section.content[j], buf, 64, 16, 2, false);
                    std::cerr.write(buf, bufsz);
                }
                std::cerr << std::endl;
            }
        }
    }
#endif
    return good;
}
