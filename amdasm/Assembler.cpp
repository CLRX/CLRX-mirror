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

using namespace CLRX;

static inline const char* skipSpacesToEnd(const char* string, const char* end)
{
    while (string!=end && *string == ' ') string++;
    return string;
}

// extract sybol name or argument name or other identifier
static inline const std::string extractSymName(const char* startString, const char* end,
           bool localLabelSymName)
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
        else if (localLabelSymName && *string >= '0' && *string <= '9') // local label
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

static inline const std::string extractLabelName(const char* startString, const char* end)
{
    if (startString != end && *startString >= '0' && *startString <= '9')
    {
        const char* string = startString;
        while (string != end && *string >= '0' && *string <= '9') string++;
        return std::string(startString, string);
    }
    return extractSymName(startString, end, false);
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
    stream->exceptions(std::ios::badbit);
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
    stream->exceptions(std::ios::badbit);
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

void AsmSymbol::clearOccurrencesInExpr()
{
    /* iteration with index and occurrencesInExprs.size() is required for checking size
     * after expression deletion that removes occurrences in exprs in this symbol */
    for (size_t i = 0; i < occurrencesInExprs.size();)
    {
        auto& occur = occurrencesInExprs[i];
        if (occur.expression!=nullptr)
        {
            if (!occur.expression->unrefSymOccursNum())
            {   // delete and keep in this place to delete next element
                // because expression removes this occurrence
                delete occur.expression;
                occur.expression = nullptr;
            }
            else i++;
        }
        else i++;
    }
    occurrencesInExprs.clear();
}

/*
 * Assembler
 */

Assembler::Assembler(const std::string& filename, std::istream& input, cxuint _flags,
        AsmFormat _format, GPUDeviceType _deviceType, std::ostream& msgStream,
        std::ostream& _printStream)
        : format(_format), deviceType(_deviceType), _64bit(false), isaAssembler(nullptr),
          symbolMap({std::make_pair(".", AsmSymbol(0, uint64_t(0)))}),
          flags(_flags), macroCount(0), inclusionLevel(0), macroSubstLevel(0),
          lineSize(0), line(nullptr), lineNo(0), messageStream(msgStream),
          printStream(_printStream), outFormatInitialized(false),
          inGlobal(_format != AsmFormat::RAWCODE),
          inAmdConfig(false), currentKernel(0), currentSection(0),
          // get value reference from first symbol: '.'
          currentOutPos(symbolMap.begin()->second.value)
{
    good = true;
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
    
    /// remove expressions before symbol map deletion
    for (auto& entry: symbolMap)
        entry.second.clearOccurrencesInExpr();
    
    for (AsmSection* section: sections)
        delete section;
    
    for (AsmRepeat* repeat: repeats)
        delete repeat;
}

bool Assembler::parseString(std::string& strarray, const char* string,
            const char*& outend)
{
    const char* end = line+lineSize;
    outend = string;
    strarray.clear();
    if (outend == end || *outend != '"')
    {
        printError(string, "Expected string");
        return false;
    }
    outend++;
    
    while (outend != end && *outend != '"')
    {
        if (*outend == '\\')
        {   // escape
            outend++;
            uint16_t value;
            if (outend == end)
            {
                printError(string, "Terminated character of string");
                return false;
            }
            if (*outend == 'x')
            {   // hex
                outend++;
                if (outend == end)
                {
                    printError(string, "Terminated character of string");
                    return false;
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
                        return false;
                    }
                    value = (value<<4) + digit;
                }
            }
            else if (*outend >= '0' &&  *outend <= '7')
            {   // octal
                value = 0;
                for (cxuint i = 0; outend != end && i < 3; i++, outend++)
                {
                    if (*outend < '0' || *outend > '7')
                    {
                        printError(string, "Expected octal character code");
                        return false;
                    }
                    value = (value<<3) + uint64_t(*outend-'0');
                    if (value > 255)
                    {
                        printError(string, "Octal code out of range");
                        return false;
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
            strarray.push_back(value);
        }
        else // regular character
            strarray.push_back(*outend++);
    }
    if (outend == end)
    {
        printError(string, "Terminated string");
        return false;
    }
    outend++;
    return true;
}

bool Assembler::parseLiteral(uint64_t& value, const char* string, const char*& outend)
{
    outend = string;
    const char* end = line+lineSize;
    if (outend != end && *outend == '\'')
    {
        outend++;
        if (outend == end)
        {
            printError(string, "Terminated character literal");
            return false;
        }
        if (*outend == '\'')
        {
            printError(string, "Empty character literal");
            return false;
        }
        
        if (*outend != '\\')
        {
            value = *outend++;
            if (outend == end || *outend != '\'')
            {
                printError(string, "Missing ''' at end of literal");
                return false;
            }
            outend++;
            return true;
        }
        else // escapes
        {
            outend++;
            if (outend == end)
            {
                printError(string, "Terminated character literal");
                return false;
            }
            if (*outend == 'x')
            {   // hex
                outend++;
                if (outend == end)
                {
                    printError(string, "Terminated character literal");
                    return false;
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
                        return false;
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
                        return false;
                    }
                    value = (value<<3) + uint64_t(*outend-'0');
                    if (value > 255)
                    {
                        printError(string, "Octal code out of range");
                        return false;
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
                return false;
            }
            outend++;
            return true;
        }
    }
    try
    { value = cstrtovCStyle<uint64_t>(string, line+lineSize, outend); }
    catch(const ParseException& ex)
    {
        printError(string, ex.what());
        return false;
    }
    return true;
}

bool Assembler::parseSymbol(const char* string, AsmSymbolEntry*& entry, bool localLabel)
{
    const std::string symName = extractSymName(string, line+lineSize, localLabel);
    if (symName.empty())
    {
        entry = nullptr;
        return true;
    }
    
    bool good = true;
    std::pair<AsmSymbolMap::iterator, bool> res =
            symbolMap.insert({ symName, AsmSymbol()});
    if (symName.front() >= '0' && symName.front() <= '9' && symName.back() == 'b' &&
        !res.first->second.isDefined)
    {
        std::string error = "Undefined previous local label '";
        error.insert(error.end(), symName.begin(), symName.end()-1);
        error += "'";
        printError(string, error.c_str());
        good = false;
    }
    AsmSymbolMap::iterator it = res.first;
    AsmSymbol& sym = it->second;
    sym.occurrences.push_back(getSourcePos(string));
    entry = &*it;
    return good;
}

bool Assembler::setSymbol(AsmSymbolEntry& symEntry, uint64_t value, cxuint sectionId)
{
    symEntry.second.value = value;
    symEntry.second.sectionId = sectionId;
    symEntry.second.isDefined = true;
    bool good = true;
    // resolve value of pending symbols
    std::stack<std::pair<AsmSymbolEntry*, size_t> > symbolStack;
    symbolStack.push(std::make_pair(&symEntry, 0));
    symEntry.second.resolving = true;
    
    uint64_t prevOutPos = currentOutPos;
    
    try
    {
    while (!symbolStack.empty())
    {
        std::pair<AsmSymbolEntry*, size_t>& entry = symbolStack.top();
        if (entry.second < entry.first->second.occurrencesInExprs.size())
        {   // 
            AsmExprSymbolOccurence& occurrence =
                    entry.first->second.occurrencesInExprs[entry.second];
            AsmExpression* expr = occurrence.expression;
            if (isAbsoluteSymbol(entry.first->second))
                expr->substituteOccurrence(occurrence, entry.first->second.value);
            else
                expr->substituteOccurrence(occurrence, entry.first);
            entry.second++;
            
            if (!expr->unrefSymOccursNum())
            {   // expresion has been fully resolved
                uint64_t value;
                cxuint sectionId;
                const AsmExprTarget& target = expr->getTarget();
                currentOutPos = target.offset;
                if (!expr->evaluate(*this, value, sectionId))
                {   // if failed
                    delete occurrence.expression; // delete expression
                    occurrence.expression = nullptr; // clear expression
                    continue;
                }
                
                switch(target.type)
                {
                    case ASMXTGT_SYMBOL:
                    {    // resolve symbol
                        AsmSymbolEntry& curSymEntry = *target.symbol;
                        if (!curSymEntry.second.resolving)
                        {
                            curSymEntry.second.value = value;
                            curSymEntry.second.sectionId = sectionId;
                            curSymEntry.second.isDefined = true;
                            symbolStack.push(std::make_pair(&curSymEntry, 0));
                            curSymEntry.second.resolving = true;
                        }
                        // otherwise we ignore circular dependencies
                        break;
                    }
                    case ASMXTGT_DATA8:
                        if (sectionId != ASMSECT_ABS)
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                        else
                        {
                            printWarningForRange(8, value, expr->getSourcePos());
                            sections[target.sectionId]->content[target.offset] =
                                    cxbyte(value);
                        }
                        break;
                    case ASMXTGT_DATA16:
                        if (sectionId != ASMSECT_ABS)
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                        else
                        {
                            printWarningForRange(16, value, expr->getSourcePos());
                            SULEV(*reinterpret_cast<uint16_t*>(sections[target.sectionId]
                                    ->content.data() + target.offset), uint16_t(value));
                        }
                        break;
                    case ASMXTGT_DATA32:
                        if (sectionId != ASMSECT_ABS)
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                        else
                        {
                            printWarningForRange(32, value, expr->getSourcePos());
                            SULEV(*reinterpret_cast<uint32_t*>(sections[target.sectionId]
                                    ->content.data() + target.offset), uint32_t(value));
                        }
                        break;
                    case ASMXTGT_DATA64:
                        if (sectionId != ASMSECT_ABS)
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                        else
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
            entry.first->second.resolving = false;
            entry.first->second.occurrencesInExprs.clear();
            entry.first->second.occurrences.clear();
            symbolStack.pop();
        }
    }
    }
    catch(...)
    {
        currentOutPos = prevOutPos;
        throw;
    }
    currentOutPos = prevOutPos;
    return good;
}

bool Assembler::assignSymbol(const std::string& symbolName, const char* stringAtSymbol,
             const char* string, bool reassign)
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
    if (!res.second && (res.first->second.onceDefined || !reassign))
    {   // found and can be only once defined
        std::string msg = "Symbol '";
        msg += symbolName;
        msg += "' is already defined";
        printError(stringAtSymbol, msg.c_str());
        return false;
    }
    AsmSymbolEntry& symEntry = *res.first;
    
    if (expr->getSymOccursNum()==0)
    {   // can evalute, assign now
        uint64_t value;
        cxuint sectionId;
        if (!expr->evaluate(*this, value, sectionId))
            return false;
        setSymbol(symEntry, value, sectionId);
    }
    else // set expression
    {
        expr->setTarget(AsmExprTarget::symbolTarget(&symEntry));
        symEntry.second.expression = expr.release();
        symEntry.second.isDefined = false;
        symEntry.second.onceDefined = !reassign;
    }
    return true;
}

bool Assembler::isAbsoluteSymbol(const AsmSymbol& symbol) const
{
    if (symbol.sectionId == ASMSECT_ABS)
        return true;
    // otherwise check section
    if (sections.empty())
        return false; // fallback
    const AsmSection& section = *sections[symbol.sectionId];
    return section.type!=AsmSectionType::AMD_KERNEL_CODE &&
            section.type!=AsmSectionType::GALLIUM_CODE &&
            section.type!=AsmSectionType::RAWCODE_CODE;
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
    good = false;
    pos.print(messageStream);
    messageStream.write("Error: ", 7);
    messageStream.write(message, ::strlen(message));
    messageStream.put('\n');
}

void Assembler::printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos)
{
    if (int64_t(value) > (1LL<<bits) || int64_t(value) < -(1LL<<(bits-1)))
    {
        std::string warning = "Value ";
        char buf[32];
        itocstrCStyle(value, buf, 32, 16);
        warning += buf;
        warning += " truncated to ";
        itocstrCStyle(value&((1ULL<<bits)-1), buf, 32, 16);
        warning += buf;
        printWarning(pos, warning.c_str());
    }
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
    else if (format == AsmFormat::RAWCODE && rawCode == nullptr)
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
    "32bit", "64bit", "abort", "align",
    "arch", "ascii", "asciz", "balign",
    "balignl", "balignw", "byte", "catalyst",
    "config", "data", "double", "else",
    "elseif", "elseifb", "elseifc", "elseifdef",
    "elseifeq", "elseifeqs", "elseifge", "elseifgt",
    "elseifle", "elseiflt", "elseifnb", "elseifnc",
    "elseifndef", "elseifne", "elseifnes", "elseifnotdef",
    "end", "endfunc", "endif", "endm",
    "endr", "equ", "equiv", "eqv",
    "err", "error", "exitm", "extern",
    "fail", "file", "fill", "float",
    "format", "func", "gallium", "global",
    "gpu", "half", "hword", "if",
    "ifb", "ifc", "ifdef", "ifeq",
    "ifeqs", "ifge", "ifgt", "ifle",
    "iflt", "ifnb", "ifnc", "ifndef",
    "ifne", "ifnes", "ifnotdef", "incbin",
    "include", "int", "kernel", "line",
    "ln", "loc", "local", "long",
    "macro", "octa", "offset", "org",
    "p2align", "print", "purgem", "quad",
    "rawcode", "rept", "section", "set",
    "short", "single", "size", "skip",
    "space", "string", "string16", "string32",
    "string64", "struct", "text", "title",
    "warning", "word"
};

enum
{
    ASMOP_32BIT = 0, ASMOP_64BIT, ASMOP_ABORT, ASMOP_ALIGN,
    ASMOP_ARCH, ASMOP_ASCII, ASMOP_ASCIZ, ASMOP_BALIGN,
    ASMOP_BALIGNL, ASMOP_BALIGNW, ASMOP_BYTE, ASMOP_CATALYST,
    ASMOP_CONFIG, ASMOP_DATA, ASMOP_DOUBLE, ASMOP_ELSE,
    ASMOP_ELSEIF, ASMOP_ELSEIFB, ASMOP_ELSEIFC, ASMOP_ELSEIFDEF,
    ASMOP_ELSEIFEQ, ASMOP_ELSEIFEQS, ASMOP_ELSEIFGE, ASMOP_ELSEIFGT,
    ASMOP_ELSEIFLE, ASMOP_ELSEIFLT, ASMOP_ELSEIFNB, ASMOP_ELSEIFNC,
    ASMOP_ELSEIFNDEF, ASMOP_ELSEIFNE, ASMOP_ELSEIFNES, ASMOP_ELSEIFNOTDEF,
    ASMOP_END, ASMOP_ENDFUNC, ASMOP_ENDIF, ASMOP_ENDM,
    ASMOP_ENDR, ASMOP_EQU, ASMOP_EQUIV, ASMOP_EQV,
    ASMOP_ERR, ASMOP_ERROR, ASMOP_EXITM, ASMOP_EXTERN,
    ASMOP_FAIL, ASMOP_FILE, ASMOP_FILL, ASMOP_FLOAT,
    ASMOP_FORMAT, ASMOP_FUNC, ASMOP_GALLIUM, ASMOP_GLOBAL,
    ASMOP_GPU, ASMOP_HALF, ASMOP_HWORD, ASMOP_IF,
    ASMOP_IFB, ASMOP_IFC, ASMOP_IFDEF, ASMOP_IFEQ,
    ASMOP_IFEQS, ASMOP_IFGE, ASMOP_IFGT, ASMOP_IFLE,
    ASMOP_IFLT, ASMOP_IFNB, ASMOP_IFNC, ASMOP_IFNDEF,
    ASMOP_IFNE, ASMOP_IFNES, ASMOP_IFNOTDEF, ASMOP_INCBIN,
    ASMOP_INCLUDE, ASMOP_INT, ASMOP_KERNEL, ASMOP_LINE,
    ASMOP_LN, ASMOP_LOC, ASMOP_LOCAL, ASMOP_LONG,
    ASMOP_MACRO, ASMOP_OCTA, ASMOP_OFFSET, ASMOP_ORG,
    ASMOP_P2ALIGN, ASMOP_PRINT, ASMOP_PURGEM, ASMOP_QUAD,
    ASMOP_RAWCODE, ASMOP_REPT, ASMOP_SECTION, ASMOP_SET,
    ASMOP_SHORT, ASMOP_SINGLE, ASMOP_SIZE, ASMOP_SKIP,
    ASMOP_SPACE, ASMOP_STRING, ASMOP_STRING16, ASMOP_STRING32,
    ASMOP_STRING64, ASMOP_STRUCT, ASMOP_TEXT, ASMOP_TITLE,
    ASMOP_WARNING, ASMOP_WORD
};

namespace CLRX
{

struct CLRX_INTERNAL AsmPseudoOps
{
    /* parsing helpers */
    // get absolute value arg resolved at this time
    static bool getAbsoluteValueArg(Assembler& asmr, uint64_t& value, const char*& string);
    // get name (not symbol name)
    static bool getNameArg(Assembler& asmr, std::string& outStr, const char*& string,
               const char* objName);
    // skip comma
    static bool skipComma(Assembler& asmr, bool& haveComma, const char*& string);
    
    /*
     * pseudo-ops logic
     */
    static void setBitness(Assembler& asmr, const char*& string, bool _64Bit);
    static void setOutFormat(Assembler& asmr, const char*& string);
    static void goToKernel(Assembler& asmr, const char*& string);
    
    static void includeFile(Assembler& asmr, const char* pseudoStr, const char*& string);
    
    static void doFail(Assembler& asmr, const char* pseudoStr, const char*& string);
    static void printError(Assembler& asmr, const char* pseudoStr, const char*& string);
    static void printWarning(Assembler& asmr, const char* pseudoStr, const char*& string);
    
    template<typename T>
    static void putIntegers(Assembler& asmr, const char*& string);
    
    template<typename UIntType>
    static void putFloats(Assembler& asmr, const char*& string);
    
    static void putStrings(Assembler& asmr, const char*& string, bool addZero = false);
    template<typename T>
    static void putStringsToInts(Assembler& asmr, const char*& string);
    
    static void putUInt128s(Assembler& asmr, const char*& string);
    
    static void setSymbol(Assembler& asmr, const char*& string, bool reassign = true);
};

bool AsmPseudoOps::getAbsoluteValueArg(Assembler& asmr, uint64_t& value,
                      const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* exprStr = string;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, string, string));
    if (expr == nullptr)
        return false;
    if (expr->getSymOccursNum() != 0)
    {   // not resolved at this time
        asmr.printError(exprStr, "Expression has unresolved symbols!");
        return false;
    }
    cxuint sectionId; // for getting
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    else if (sectionId != ASMSECT_ABS)
    {   // if not absolute value
        asmr.printError(exprStr, "Expression must be absolute!");
        return false;
    }
    return true;
}

bool AsmPseudoOps::getNameArg(Assembler& asmr, std::string& outStr, const char*& string,
            const char* objName)
{
    const char* end = asmr.line + asmr.lineSize;
    outStr.clear();
    string = skipSpacesToEnd(string, end);
    if (string == end)
    {
        std::string error("Expected");
        error += objName;
        asmr.printError(string, error.c_str());
        return false;
    }
    const char* nameStr = string;
    if ((*string >= 'a' && *string <= 'z') || (*string >= 'A' && *string <= 'Z') ||
            *string == '_')
    {
        string++;
        while (string != end && ((*string >= 'a' && *string <= 'z') ||
           (*string >= 'A' && *string <= 'Z') || (*string >= '0' && *string <= '9') ||
           *string == '_')) string++;
    }
    else
    {
        asmr.printError(string, "Some garbages at name place");
        return false;
    }
    outStr.assign(nameStr, string);
    return true;
}

inline bool AsmPseudoOps::skipComma(Assembler& asmr, bool& haveComma, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string == end)
    {
        haveComma = false;
        return true;
    }
    if (*string != ',')
    {
        asmr.printError(string, "Expected ',' before argument");
        return false;
    }
    string++;
    haveComma = true;
    return true;
}

void AsmPseudoOps::setBitness(Assembler& asmr, const char*& string, bool _64Bit)
{
    if (asmr.outFormatInitialized)
        asmr.printError(string, "Bitness has already been defined");
    if (asmr.format != AsmFormat::CATALYST)
        asmr.printWarning(string, "Bitness ignored for other formats than AMD Catalyst");
    else
        asmr._64bit = (_64Bit);
}

void AsmPseudoOps::setOutFormat(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    std::string formatName;
    if (!getNameArg(asmr, formatName, string, "output format type"))
        return;
    
    toLowerString(formatName);
    if (formatName == "catalyst" || formatName == "amd")
        asmr.format = AsmFormat::CATALYST;
    else if (formatName == "gallium")
        asmr.format = AsmFormat::GALLIUM;
    else if (formatName == "raw")
        asmr.format = AsmFormat::RAWCODE;
    else
        asmr.printError(string, "Unknown output format type");
    if (asmr.outFormatInitialized)
        asmr.printError(string, "Output format type has already been defined");
}

void AsmPseudoOps::goToKernel(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.format == AsmFormat::CATALYST || asmr.format == AsmFormat::GALLIUM)
    {
        string = skipSpacesToEnd(string, end);
        std::string kernelName;
        if (!getNameArg(asmr, kernelName, string, "kernel name"))
            return;
        if (asmr.format == AsmFormat::CATALYST)
        {
            asmr.kernelMap.insert(std::make_pair(kernelName,
                            asmr.amdOutput->kernels.size()));
            asmr.amdOutput->addEmptyKernel(kernelName.c_str());
        }
        else
        {
            asmr.kernelMap.insert(std::make_pair(kernelName,
                        asmr.galliumOutput->kernels.size()));
            //galliumOutput->addEmptyKernel(kernelName.c_str());
        }
    }
    else if (asmr.format == AsmFormat::RAWCODE)
        asmr.printError(string, "Raw code can have only one unnamed kernel");
}

void AsmPseudoOps::includeFile(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    std::string filename;
    const char* nameStr = string;
    if (asmr.parseString(filename, string, string))
    {
        bool failedOpen = false;
        filesystemPath(filename);
        try
        {
            std::unique_ptr<AsmInputFilter> newInputFilter(new AsmStreamInputFilter(
                    asmr.getSourcePos(pseudoStr), filename));
            asmr.asmInputFilters.push(newInputFilter.release());
            asmr.currentInputFilter = asmr.asmInputFilters.top(); 
            return;
        }
        catch(const Exception& ex)
        { failedOpen = true; }
        
        for (const std::string& incDir: asmr.includeDirs)
        {
            failedOpen = false;
            std::string inDirFilename;
            try
            {
                inDirFilename = joinPaths(incDir, filename);
                std::unique_ptr<AsmInputFilter> newInputFilter(new AsmStreamInputFilter(
                        asmr.getSourcePos(pseudoStr), inDirFilename));
                asmr.asmInputFilters.push(newInputFilter.release());
                asmr.currentInputFilter = asmr.asmInputFilters.top();
                break;
            }
            catch(const Exception& ex)
            { failedOpen = true; }
        }
        if (failedOpen)
        {
            std::string error("Include file '");
            error += filename;
            error += "' not found or unavailable in any directory";
            asmr.printError(nameStr, error.c_str());
        }
    }
}

void AsmPseudoOps::doFail(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t value = 0;
    if (!getAbsoluteValueArg(asmr, value, string))
        return;
    char buf[50];
    ::memcpy(buf, ".fail ", 6);
    const size_t pos = 6+itocstrCStyle(int64_t(value), buf+6, 50-6);
    ::memcpy(buf+pos, " encountered", 13);
    if (int64_t(value) >= 500)
        asmr.printWarning(pseudoStr, buf);
    else
        asmr.printError(pseudoStr, buf);
}

void AsmPseudoOps::printError(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, string, string))
            return; // error
        asmr.printError(pseudoStr, outStr.c_str());
    }
    else
        asmr.printError(pseudoStr, ".error encountered");
}

void AsmPseudoOps::printWarning(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, string, string))
            return; // error
        asmr.printWarning(pseudoStr, outStr.c_str());
    }
    else
        asmr.printWarning(pseudoStr, ".warning encountered");
}

template<typename T>
void AsmPseudoOps::putIntegers(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    while (true)
    {
        const char* literalStr = string;
        std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, string, string));
        if (expr)
        {
            if (expr->getSymOccursNum()==0)
            {   // put directly to section
                uint64_t value;
                cxuint sectionId;
                if (expr->evaluate(asmr, value, sectionId))
                {
                    if (sectionId == ASMSECT_ABS)
                    {
                        if (sizeof(T) < 8)
                            asmr.printWarningForRange(sizeof(T)<<3, value,
                                         asmr.getSourcePos(literalStr));
                        T out;
                        SLEV(out, value);
                        asmr.putData(sizeof(T), reinterpret_cast<const cxbyte*>(&out));
                    }
                    else
                        asmr.printError(literalStr, "Expression must be absolute!");
                }
            }
            else // expression
            {
                expr->setTarget(AsmExprTarget::dataTarget<T>(
                                asmr.currentSection, asmr.currentOutPos));
                expr.release();
                asmr.reserveData(sizeof(T));
            }
        }
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

template<typename T> inline
T asmcstrtofCStyleLEV(const char* str, const char* inend, const char*& outend);

template<> inline
uint16_t asmcstrtofCStyleLEV<uint16_t>(const char* str, const char* inend,
                   const char*& outend)
{ return LEV(cstrtohCStyle(str, inend, outend)); }

template<> inline
uint32_t asmcstrtofCStyleLEV<uint32_t>(const char* str, const char* inend,
                   const char*& outend)
{
    union {
        float f;
        uint32_t u;
    } value;
    value.f = cstrtovCStyle<float>(str, inend, outend);
    return LEV(value.u);
}

template<> inline
uint64_t asmcstrtofCStyleLEV<uint64_t>(const char* str, const char* inend,
                   const char*& outend)
{
    union {
        double f;
        uint64_t u;
    } value;
    value.f = cstrtovCStyle<double>(str, inend, outend);
    return LEV(value.u);
}

template<typename UIntType>
void AsmPseudoOps::putFloats(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    while (true)
    {
        UIntType out = 0;
        if (string != end && *string != ',')
        {
            try
            { out = asmcstrtofCStyleLEV<UIntType>(string, end, string); }
            catch(const ParseException& ex)
            { asmr.printError(string, ex.what()); }
        }
        
        asmr.putData(sizeof(UIntType), reinterpret_cast<const cxbyte*>(&out));
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

void AsmPseudoOps::putUInt128s(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    while (true)
    {
        UInt128 value = { 0, 0 };
        if (string != end && *string != ',')
        {
            try
            { value = cstrtou128CStyle(string, end, string); }
            catch(const ParseException& ex)
            { asmr.printError(string, ex.what()); }
        }
        UInt128 out;
        SLEV(out.lo, value.lo);
        SLEV(out.hi, value.hi);
        asmr.putData(16, reinterpret_cast<const cxbyte*>(&out));
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

void AsmPseudoOps::putStrings(Assembler& asmr, const char*& string, bool addZero)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    while (string != end)
    {
        std::string outStr;
        if (*string != ',')
        {
            if (asmr.parseString(outStr, string, string))
                asmr.putData(outStr.size()+(addZero), (const cxbyte*)outStr.c_str());
        }
        
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

template<typename T>
void AsmPseudoOps::putStringsToInts(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    while (string != end)
    {
        std::string outStr;
        if (*string != ',')
        {
            if (asmr.parseString(outStr, string, string))
            {
                const uint64_t oldOffset = asmr.currentOutPos;
                const size_t strSize = outStr.size()+1;
                asmr.reserveData(sizeof(T)*(strSize));
                T* outData = reinterpret_cast<T*>(
                        asmr.sections[asmr.currentSection]->content.data()+oldOffset);
                for (size_t i = 0; i < strSize; i++)
                    SULEV(outData[i], T(outStr[i])&T(0xff));
            }
        }
        
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

void AsmPseudoOps::setSymbol(Assembler& asmr, const char*& string, bool reassign)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string == end)
    {
        asmr.printError(string, "Expected symbol name and expression");
        return;
    }
    const char* strAtSymName = string;
    std::string symName = extractSymName(string, end, false);
    if (symName.empty())
    {
        asmr.printError(string, "Expected symbol name and expression");
        return;
    }
    string += symName.size();
    bool haveComma;
    if (!skipComma(asmr, haveComma, string))
        return;
    if (!haveComma)
    {
        asmr.printError(string, "Expected symbol name and expression");
        return;
    }
    asmr.assignSymbol(symName, strAtSymName, string, reassign);
    string = end;
}

};

bool Assembler::assemble()
{
    good = true;
    while (readLine())
    {
        /* parse line */
        const char* string = line;
        const char* end = line+lineSize;
        string = skipSpacesToEnd(string, end);
        if (string == end)
            continue; // only empty line
        
        const char* firstNameString = string;
        std::string firstName = extractLabelName(string, end);
        string += firstName.size();
        
        string = skipSpacesToEnd(string, end);
        
        bool doNextLine = false;
        while (!firstName.empty() && string != end && *string == ':')
        {   // labels
            string++;
            string = skipSpacesToEnd(string, end);
            initializeOutputFormat();
            if (firstName.front() >= '0' && firstName.front() <= '9')
            {   // handle local labels
                if (sections.empty())
                {
                    printError(firstNameString,
                               "Local label can't be defined outside section");
                    doNextLine = true;
                    break;
                }
                if (inAmdConfig)
                {
                    printError(firstNameString,
                               "Local label can't defined in AMD config place");
                    doNextLine = true;
                    break;
                }
                std::pair<AsmSymbolMap::iterator, bool> prevLRes =
                        symbolMap.insert({ firstName+"b", AsmSymbol() });
                std::pair<AsmSymbolMap::iterator, bool> nextLRes =
                        symbolMap.insert({ firstName+"f", AsmSymbol() });
                assert(setSymbol(*nextLRes.first, currentOutPos, currentSection));
                prevLRes.first->second.clearOccurrencesInExpr();
                prevLRes.first->second.occurrences.clear();
                prevLRes.first->second.value = nextLRes.first->second.value;
                prevLRes.first->second.isDefined = true;
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
                        std::string msg = "Symbol '";
                        msg += firstName;
                        msg += "' is already defined";
                        printError(firstNameString, msg.c_str());
                        doNextLine = true;
                        break;
                    }
                }
                if (sections.empty())
                {
                    printError(firstNameString,
                               "Label can't be defined outside section");
                    doNextLine = true;
                    break;
                }
                if (inAmdConfig)
                {
                    printError(firstNameString,
                               "Label can't defined in AMD config place");
                    doNextLine = true;
                    break;
                }
                
                setSymbol(*res.first, currentOutPos, currentSection);
                res.first->second.onceDefined = true;
                res.first->second.sectionId = currentSection;
            }
            // new label or statement
            firstNameString = string;
            firstName = extractLabelName(string, end);
            string += firstName.size();
        }
        if (doNextLine)
            continue;
        
        string = skipSpacesToEnd(string, end);
        if (string != end && *string == '=' &&
            // not for local labels
            (firstName.front() < '0' || firstName.front() > '9'))
        {   // assignment
            string = skipSpacesToEnd(string+1, line+lineSize);
            if (string == end)
            {
                printError(string, "Expected assignment expression");
                continue;
            }
            assignSymbol(firstName, firstNameString, string);
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
                    AsmPseudoOps::setBitness(*this, string, pseudoOp == ASMOP_64BIT);
                    break;
                case ASMOP_ABORT:
                    printError(firstNameString, "Aborted!");
                    return good;
                    break;
                case ASMOP_ALIGN:
                    break;
                case ASMOP_ARCH:
                    break;
                case ASMOP_ASCII:
                    AsmPseudoOps::putStrings(*this, string);
                    break;
                case ASMOP_ASCIZ:
                    AsmPseudoOps::putStrings(*this, string, true);
                    break;
                case ASMOP_BALIGN:
                    break;
                case ASMOP_BALIGNL:
                    break;
                case ASMOP_BALIGNW:
                    break;
                case ASMOP_BYTE:
                    AsmPseudoOps::putIntegers<cxbyte>(*this, string);
                    break;
                case ASMOP_RAWCODE:
                case ASMOP_CATALYST:
                case ASMOP_GALLIUM:
                    if (outFormatInitialized)
                    {
                        printError(string, "Output format type has already been defined");
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
                            printError(string,
                                   "Configuration in global layout is illegal");
                        else
                            inAmdConfig = true; // inside Amd Config
                    }
                    else
                        printError(string,
                           "Configuration section only for AMD Catalyst binaries");
                    break;
                case ASMOP_DATA:
                    break;
                case ASMOP_DOUBLE:
                    AsmPseudoOps::putFloats<uint64_t>(*this, string);
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
                case ASMOP_SET:
                    AsmPseudoOps::setSymbol(*this, string);
                    break;
                case ASMOP_EQUIV:
                    AsmPseudoOps::setSymbol(*this, string, false);
                    break;
                case ASMOP_EQV: // FIXME: fix eqv (must be evaluated for each use)
                    AsmPseudoOps::setSymbol(*this, string, false);
                    break;
                case ASMOP_ERR:
                    printError(firstNameString, ".err encountered");
                    break;
                case ASMOP_ERROR:
                    AsmPseudoOps::printError(*this, firstNameString, string);
                    break;
                case ASMOP_EXITM:
                    break;
                case ASMOP_EXTERN:
                    break;
                case ASMOP_FAIL:
                    AsmPseudoOps::doFail(*this, firstNameString, string);
                    break;
                case ASMOP_FILE:
                    break;
                case ASMOP_FILL:
                    break;
                case ASMOP_FLOAT:
                    AsmPseudoOps::putFloats<uint32_t>(*this, string);
                    break;
                case ASMOP_FORMAT:
                    AsmPseudoOps::setOutFormat(*this, string);
                    break;
                case ASMOP_FUNC:
                    break;
                case ASMOP_GLOBAL:
                    break;
                case ASMOP_GPU:
                    break;
                case ASMOP_HALF:
                    AsmPseudoOps::putFloats<uint16_t>(*this, string);
                    break;
                case ASMOP_HWORD:
                    AsmPseudoOps::putIntegers<uint16_t>(*this, string);
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
                    AsmPseudoOps::includeFile(*this, firstNameString, string);
                    break;
                case ASMOP_INT:
                case ASMOP_LONG:
                    AsmPseudoOps::putIntegers<uint32_t>(*this, string);
                    break;
                case ASMOP_KERNEL:
                    AsmPseudoOps::goToKernel(*this, string);
                    break;
                case ASMOP_LINE:
                case ASMOP_LN:
                    break;
                case ASMOP_LOC:
                    break;
                case ASMOP_LOCAL:
                    break;
                case ASMOP_MACRO:
                    break;
                case ASMOP_OCTA:
                    AsmPseudoOps::putUInt128s(*this, string);
                    break;
                case ASMOP_OFFSET:
                    break;
                case ASMOP_ORG:
                    break;
                case ASMOP_P2ALIGN:
                    break;
                case ASMOP_PRINT:
                {
                    std::string outStr;
                    if (parseString(outStr, string, string))
                    {
                        std::cout.write(outStr.c_str(), outStr.size());
                        std::cout.put('\n');
                    }
                    break;
                }
                case ASMOP_PURGEM:
                    break;
                case ASMOP_QUAD:
                    AsmPseudoOps::putIntegers<uint64_t>(*this, string);
                    break;
                case ASMOP_REPT:
                    break;
                case ASMOP_SECTION:
                    break;
                case ASMOP_SHORT:
                    AsmPseudoOps::putIntegers<uint16_t>(*this, string);
                    break;
                case ASMOP_SINGLE:
                    AsmPseudoOps::putFloats<uint32_t>(*this, string);
                    break;
                case ASMOP_SIZE:
                    break;
                case ASMOP_SKIP:
                    break;
                case ASMOP_SPACE:
                    break;
                case ASMOP_STRING:
                    AsmPseudoOps::putStrings(*this, string, true);
                    break;
                case ASMOP_STRING16:
                    AsmPseudoOps::putStringsToInts<uint16_t>(*this, string);
                    break;
                case ASMOP_STRING32:
                    AsmPseudoOps::putStringsToInts<uint32_t>(*this, string);
                    break;
                case ASMOP_STRING64:
                    AsmPseudoOps::putStringsToInts<uint64_t>(*this, string);
                    break;
                case ASMOP_STRUCT:
                    break;
                case ASMOP_TEXT:
                    break;
                case ASMOP_TITLE:
                    break;
                case ASMOP_WARNING:
                    AsmPseudoOps::printWarning(*this, firstNameString, string);
                    break;
                case ASMOP_WORD:
                    AsmPseudoOps::putIntegers<uint32_t>(*this, string);
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
        if (string != end) // garbages
            printError(string, "Garbages at end of line with pseudo-op");
    }
    return good;
}
