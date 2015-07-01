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
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

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
         LineTrans({ ssize_t(position), 0 }),
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
    :  managed(false), stream(&is), mode(LineMode::NORMAL), stmtPos(0)
{
    source = RefPtr<const AsmSource>(new AsmFile(filename));
    stream->exceptions(std::ios::badbit);
    buffer.reserve(AsmParserLineMaxSize);
}

AsmStreamInputFilter::AsmStreamInputFilter(const AsmSourcePos& pos,
           const std::string& filename)
try : managed(true), stream(nullptr), mode(LineMode::NORMAL), stmtPos(0)
{
    if (!pos.macro)
        source = RefPtr<const AsmSource>(new AsmFile(pos.source, pos.lineNo,
                         pos.colNo, filename));
    else // if inside macro
        source = RefPtr<const AsmSource>(new AsmFile(
            RefPtr<const AsmSource>(new AsmMacroSource(pos.macro, pos.source)),
                 pos.lineNo, pos.colNo, filename));
    
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
        const std::string& filename) : managed(false), stream(&is), mode(LineMode::NORMAL),
        stmtPos(0)
{
    if (!pos.macro)
        source = RefPtr<const AsmSource>(new AsmFile(pos.source, pos.lineNo,
                             pos.colNo, filename));
    else // if inside macro
        source = RefPtr<const AsmSource>(new AsmFile(
            RefPtr<const AsmSource>(new AsmMacroSource(pos.macro, pos.source)),
                 pos.lineNo, pos.colNo, filename));
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
    size_t joinStart = pos; // join Start - physical line start
    size_t destPos = pos;
    size_t backslash = false;
    bool prevAsterisk = false;
    bool asterisk = false;
    colTranslations.push_back({ssize_t(-stmtPos), lineNo});
    while (!endOfLine)
    {
        switch(mode)
        {
            case LineMode::NORMAL:
            {
                if (pos < buffer.size() && !isSpace(buffer[pos]) && buffer[pos] != ';')
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
                        
                    } while (pos < buffer.size() && !isSpace(buffer[pos]) &&
                                buffer[pos] != ';');
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
                            if (ssize_t(destPos-lineStart) ==
                                colTranslations.back().position)
                                colTranslations.pop_back();
                            colTranslations.push_back(
                                {ssize_t(destPos-lineStart), lineNo});
                        }
                        stmtPos = 0;
                        pos++;
                        joinStart = pos;
                        backslash = false;
                        break;
                    }
                    else if (buffer[pos] == ';' && mode == LineMode::NORMAL)
                    {   /* treat statement as separate line */
                        endOfLine = true;
                        pos++;
                        stmtPos += pos-joinStart;
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
                        if (ssize_t(destPos-lineStart) == colTranslations.back().position)
                            colTranslations.pop_back();
                        colTranslations.push_back({ssize_t(destPos-lineStart), lineNo});
                    }
                    else
                        mode = LineMode::NORMAL;
                    pos++;
                    joinStart = pos;
                    backslash = false;
                    stmtPos = 0;
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
                            if (ssize_t(destPos-lineStart) ==
                                colTranslations.back().position)
                                colTranslations.pop_back();
                            colTranslations.push_back(
                                {ssize_t(destPos-lineStart), lineNo});
                        }
                        pos++;
                        joinStart = pos;
                        backslash = false;
                        stmtPos = 0;
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
                        {
                            destPos--; // ignore last backslash
                            colTranslations.push_back(
                                {ssize_t(destPos-lineStart), lineNo});
                        }
                        else
                            assembler.printWarning({lineNo, pos-joinStart+stmtPos+1},
                                        "Unterminated string: newline inserted");
                        pos++;
                        joinStart = pos;
                        stmtPos = 0;
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
                    assembler.printError({lineNo, pos-joinStart+stmtPos+1},
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
                   pos.source, pos.lineNo, pos.colNo));
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
    colTranslations.push_back({ curColTrans->position, curColTrans->lineNo});
    size_t colTransThreshold = (curColTrans+1 != colTransEnd) ?
            curColTrans[1].position : SIZE_MAX;
    
    while (pos < contentSize && content[pos] != '\n')
    {
        if (content[pos] != '\\')
        {
            if (pos >= colTransThreshold)
            {
                curColTrans++;
                colTranslations.push_back({ssize_t(destPos + pos-toCopyPos),
                            curColTrans->lineNo});
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
                colTranslations.push_back({ssize_t(destPos + pos-toCopyPos),
                            curColTrans->lineNo});
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
        auto sourceRept = source.staticCast<const AsmRepeatSource>();
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
    const AsmSourcePos* thisPos = this;
    bool exprFirstDepth = true;
    char numBuf[32];
    while (thisPos->exprSourcePos!=nullptr)
    {
        AsmSourcePos sourcePosToPrint = *(thisPos->exprSourcePos);
        sourcePosToPrint.exprSourcePos = nullptr;
        printIndent(os, indentLevel);
        if (sourcePosToPrint.source->type == AsmSourceType::FILE)
        {
            RefPtr<const AsmFile> file = sourcePosToPrint.source.
                        staticCast<const AsmFile>();
            if (!file->parent)
            {
                os.write((exprFirstDepth) ? "Expression evaluation from " :
                        "                      from ", 27);
                os.write(file->file.c_str(), file->file.size());
                numBuf[0] = ':';
                size_t size = 1+itocstrCStyle(sourcePosToPrint.lineNo, numBuf+1, 31);
                os.write(numBuf, size);
                if (colNo != 0)
                {
                    numBuf[0] = ':';
                    size = 1+itocstrCStyle(sourcePosToPrint.colNo, numBuf+1, 29);
                    numBuf[size++] = ':';
                    os.write(numBuf, size);
                }
                os.put('\n');
                exprFirstDepth = false;
                thisPos = thisPos->exprSourcePos;
                continue;
            }
        }
        exprFirstDepth = true;
        os.write("Expression evaluation from\n", 27);
        sourcePosToPrint.print(os, indentLevel+1);
        os.put('\n');
        thisPos = thisPos->exprSourcePos;
    }
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
                    curMacro->source, curMacro->lineNo, curMacro->colNo };
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
                size_t size = 1+itocstrCStyle(curMacro->lineNo, numBuf+1, 30);
                os.write(numBuf, size);
                size = itocstrCStyle(curMacro->colNo, numBuf, 29);
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
                curMacro->lineNo, curMacro->colNo };
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
                    numBuf[size++] = ':';
                    os.write(numBuf, size);
                    size = itocstrCStyle(curFile->colNo, numBuf, 30);
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
                        curFile->lineNo, curFile->colNo };
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
            size = 1+itocstrCStyle(colNo, numBuf+1, 31);
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

void AsmSymbol::removeOccurrenceInExpr(AsmExpression* expr, size_t argIndex,
               size_t opIndex)
{
    auto it = std::remove(occurrencesInExprs.begin(), occurrencesInExprs.end(),
            AsmExprSymbolOccurrence{expr, argIndex, opIndex});
    occurrencesInExprs.resize(it-occurrencesInExprs.begin());
}

void AsmSymbol::clearOccurrencesInExpr()
{
    /* iteration with index and occurrencesInExprs.size() is required for checking size
     * after expression deletion that removes occurrences in exprs in this symbol */
    for (size_t i = 0; i < occurrencesInExprs.size(); i++)
    {
        auto& occur = occurrencesInExprs[i];
        if (occur.expression!=nullptr && !occur.expression->unrefSymOccursNum())
        {   // delete and move back iteration by number of removed elements
            const size_t oldSize = occurrencesInExprs.size();
            AsmExpression* occurExpr = occur.expression;
            occur.expression = nullptr;
            delete occurExpr;
            // subtract number of removed elements from counter
            i -= oldSize-occurrencesInExprs.size(); 
        }
    }
    occurrencesInExprs.clear();
}

/*
 * Assembler
 */

Assembler::Assembler(const std::string& filename, std::istream& input, cxuint _flags,
        BinaryFormat _format, GPUDeviceType _deviceType, std::ostream& msgStream,
        std::ostream& _printStream)
        : format(_format), deviceType(_deviceType), _64bit(false), isaAssembler(nullptr),
          symbolMap({std::make_pair(".", AsmSymbol(0, uint64_t(0)))}),
          flags(_flags), macroCount(0), inclusionLevel(0), macroSubstLevel(0),
          lineSize(0), line(nullptr), lineNo(0), endOfAssembly(false),
          messageStream(msgStream), printStream(_printStream), outFormatInitialized(false),
          inGlobal(_format != BinaryFormat::RAWCODE),
          inAmdConfig(false), currentKernel(0),
          // value reference and section reference from first symbol: '.'
          currentSection(symbolMap.begin()->second.sectionId),
          currentOutPos(symbolMap.begin()->second.value)
{
    lineAlreadyRead = false;
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
        if (format == BinaryFormat::GALLIUM)
            delete galliumOutput;
        else if (format == BinaryFormat::AMD)
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
    /// remove expressions before symbol snapshots
    for (auto& entry: symbolSnapshots)
        entry->second.clearOccurrencesInExpr();
    
    for (auto& entry: symbolSnapshots)
        delete entry;
    
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
                printError(string, "Unterminated character of string");
                return false;
            }
            if (*outend == 'x')
            {   // hex
                outend++;
                if (outend == end)
                {
                    printError(string, "Unterminated character of string");
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
            else if (isODigit(*outend))
            {   // octal
                value = 0;
                for (cxuint i = 0; outend != end && i < 3; i++, outend++)
                {
                    if (!isODigit(*outend))
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
        printError(string, "Unterminated string");
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
            printError(string, "Unterminated character literal");
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
                printError(string, "Unterminated character literal");
                return false;
            }
            if (*outend == 'x')
            {   // hex
                outend++;
                if (outend == end)
                {
                    printError(string, "Unterminated character literal");
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
            else if (isODigit(*outend))
            {   // octal
                value = 0;
                for (cxuint i = 0; outend != end && i < 3 && *outend != '\''; i++, outend++)
                {
                    if (!isODigit(*outend))
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

Assembler::ParseState Assembler::parseSymbol(const char* string, const char*& outend,
                AsmSymbolEntry*& entry, bool localLabel, bool dontCreateSymbol)
{
    const std::string symName = extractSymName(string, line+lineSize, localLabel);
    outend = string;
    if (symName.empty())
    {   // this is not symbol or a missing symbol
        while (outend != line+lineSize && !isSpace(*outend) && *outend != ',' &&
            *outend != ':' && *outend != ';') outend++;
        entry = nullptr;
        return Assembler::ParseState::MISSING;
    }
    outend = string + symName.size();
    if (symName == ".") // any usage of '.' causes format initialization
        initializeOutputFormat();
    
    Assembler::ParseState state = Assembler::ParseState::PARSED;
    bool symIsDefined;
    if (!dontCreateSymbol)
    {   // create symbol if not found
        std::pair<AsmSymbolMap::iterator, bool> res =
                symbolMap.insert({ symName, AsmSymbol()});
        entry = &*res.first;
        symIsDefined = res.first->second.isDefined;
    }
    else
    {   // only find symbol and set isDefined and entry
        AsmSymbolMap::iterator it = symbolMap.find(symName);
        entry = (it != symbolMap.end()) ? &*it : nullptr;
        symIsDefined = (it != symbolMap.end() && it->second.isDefined);
    }
    if (isDigit(symName.front()) && symName.back() == 'b' && !symIsDefined)
    {   // failed at finding
        std::string error = "Undefined previous local label '";
        error.append(symName.begin(), symName.end()-1);
        error += "'";
        printError(string, error.c_str());
        state = Assembler::ParseState::FAILED;
    }
    
    return state;
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
    
    while (!symbolStack.empty())
    {
        std::pair<AsmSymbolEntry*, size_t>& entry = symbolStack.top();
        if (entry.second < entry.first->second.occurrencesInExprs.size())
        {
            AsmExprSymbolOccurrence& occurrence =
                    entry.first->second.occurrencesInExprs[entry.second];
            AsmExpression* expr = occurrence.expression;
            expr->substituteOccurrence(occurrence, entry.first->second.value,
                       (!isAbsoluteSymbol(entry.first->second)) ?
                       entry.first->second.sectionId : ASMSECT_ABS);
            entry.second++;
            
            if (!expr->unrefSymOccursNum())
            {   // expresion has been fully resolved
                uint64_t value;
                cxuint sectionId;
                const AsmExprTarget& target = expr->getTarget();
                if (!expr->evaluate(*this, value, sectionId))
                {   // if failed
                    delete occurrence.expression; // delete expression
                    good = false;
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
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                        {
                            printWarningForRange(8, value, expr->getSourcePos());
                            sections[target.sectionId]->content[target.offset] =
                                    cxbyte(value);
                        }
                        break;
                    case ASMXTGT_DATA16:
                        if (sectionId != ASMSECT_ABS)
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                        {
                            printWarningForRange(16, value, expr->getSourcePos());
                            SULEV(*reinterpret_cast<uint16_t*>(sections[target.sectionId]
                                    ->content.data() + target.offset), uint16_t(value));
                        }
                        break;
                    case ASMXTGT_DATA32:
                        if (sectionId != ASMSECT_ABS)
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                        {
                            printWarningForRange(32, value, expr->getSourcePos());
                            SULEV(*reinterpret_cast<uint32_t*>(sections[target.sectionId]
                                    ->content.data() + target.offset), uint32_t(value));
                        }
                        break;
                    case ASMXTGT_DATA64:
                        if (sectionId != ASMSECT_ABS)
                        {
                            printError(expr->getSourcePos(),
                                   "Relative value is illegal in data expressions");
                            good = false;
                        }
                        else
                            SULEV(*reinterpret_cast<uint64_t*>(sections[target.sectionId]
                                    ->content.data() + target.offset), uint64_t(value));
                        break;
                    default: // ISA assembler resolves this dependency
                        if (!isaAssembler->resolveCode(sections[target.sectionId]
                                ->content.data() + target.offset, target.type, value))
                            good = false;
                        break;
                }
                delete occurrence.expression; // delete expression
            }
            else // otherwise we only clear occurrence expression
                occurrence.expression = nullptr; // clear expression
        }
        else // pop
        {
            entry.first->second.resolving = false;
            entry.first->second.occurrencesInExprs.clear();
            if (entry.first->second.snapshot && --(entry.first->second.refCount) == 0)
            {
                symbolSnapshots.erase(entry.first);
                delete entry.first; // delete this symbol snapshot
            }
            symbolStack.pop();
        }
    }
    return good;
}

bool Assembler::assignSymbol(const std::string& symbolName, const char* stringAtSymbol,
             const char* string, bool reassign, bool baseExpr)
{
    const char* exprStr = string;
    // make base expr if baseExpr=true and symbolName is not output counter
    bool makeBaseExpr = (baseExpr && symbolName != ".");
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(
                    *this, string, string, makeBaseExpr));
    string = skipSpacesToEnd(string, line+lineSize);
    if (!expr) // no expression, errors
        return false;
    
    if (string != line+lineSize)
    {
        printError(string, "Garbages at end of expression");
        return false;
    }
    if (expr->isEmpty()) // empty expression, we treat as error
    {
        printError(exprStr, "Expected assignment expression");
        return false;
    }
    
    if (symbolName == ".")
    {   // assigning '.'
        uint64_t value;
        cxuint sectionId;
        if (!expr->evaluate(*this, value, sectionId))
            return false;
        if (expr->getSymOccursNum()==0)
            return assignOutputCounter(stringAtSymbol, value, sectionId);
        else
        {
            printError(stringAtSymbol, "Symbol '.' requires a resolved expression");
            return false;
        }
    }
    
    std::pair<AsmSymbolMap::iterator, bool> res =
            symbolMap.insert({ symbolName, AsmSymbol() });
    if (!res.second && ((res.first->second.onceDefined || !reassign) &&
        (res.first->second.isDefined || res.first->second.expression!=nullptr)))
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
        symEntry.second.onceDefined = !reassign;
    }
    else // set expression
    {
        expr->setTarget(AsmExprTarget::symbolTarget(&symEntry));
        symEntry.second.expression = expr.release();
        symEntry.second.isDefined = false;
        symEntry.second.onceDefined = !reassign;
        symEntry.second.base = baseExpr;
        if (baseExpr && !symEntry.second.occurrencesInExprs.empty())
        {
            AsmSymbolEntry* tempSymEntry;
            if (!AsmExpression::makeSymbolSnapshot(*this, symEntry, tempSymEntry,
                    &symEntry.second.occurrencesInExprs[0].expression->getSourcePos()))
                return false;
            //std::unique_ptr<AsmSymbolEntry> tempSymEntry(tempSymEntryPtr);
            tempSymEntry->second.occurrencesInExprs =
                        symEntry.second.occurrencesInExprs;
            // clear occurrences after copy
            symEntry.second.occurrencesInExprs.clear();
            if (tempSymEntry->second.isDefined) // set symbol chain
                setSymbol(*tempSymEntry, tempSymEntry->second.value,
                          tempSymEntry->second.sectionId);
        }
    }
    return true;
}

bool Assembler::assignOutputCounter(const char* symbolStr, uint64_t value,
            cxuint sectionId, cxbyte fillValue)
{
    initializeOutputFormat();
    if (currentSection != sectionId && sectionId != ASMSECT_ABS)
    {
        printError(symbolStr, "Illegal section change for symbol '.'");
        return false;
    }
    if (int64_t(currentOutPos) > int64_t(value))
    {
        printError(symbolStr, "Attempt to move backwards");
        return false;
    }
    if (value-currentOutPos!=0)
        reserveData(value-currentOutPos, fillValue);
    currentOutPos = value;
    return true;
}

bool Assembler::skipSymbol(const char* string, const char*& outend)
{
    const char* end = line+lineSize;
    string = skipSpacesToEnd(string, end);
    const char* start = string;
    if (string != end)
    {
        if(isAlpha(*string) || *string == '_' || *string == '.' || *string == '$')
            for (string++; string != end && (isAlnum(*string) || *string == '_' ||
                 *string == '.' || *string == '$') ; string++);
    }
    outend = string;
    if (start == string)
    {   // this is not symbol name
        while (outend != end && !isSpace(*outend) && *outend != ',' &&
            *outend != ':' && *outend != ';') outend++;
        printError(start, "Expected symbol name");
        return false;
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
    messageStream.write(": Warning: ", 11);
    messageStream.write(message, ::strlen(message));
    messageStream.put('\n');
}

void Assembler::printError(const AsmSourcePos& pos, const char* message)
{
    good = false;
    pos.print(messageStream);
    messageStream.write(": Error: ", 9);
    messageStream.write(message, ::strlen(message));
    messageStream.put('\n');
}

void Assembler::printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos)
{
    if (bits < 64 && (int64_t(value) > (1LL<<bits) || int64_t(value) < -(1LL<<(bits-1))))
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

bool Assembler::pushClause(const AsmSourcePos& sourcePos,
             AsmClauseType clauseType, bool satisfied, bool& included)
{
    if (clauseType == AsmClauseType::MACRO || clauseType == AsmClauseType::IF ||
        clauseType == AsmClauseType::REPEAT)
    {   // add new clause
        clauses.push({ clauseType, sourcePos, satisfied, { } });
        included = satisfied;
        return true;
    }
    if (clauses.empty())
    {   // no clauses
        if (clauseType == AsmClauseType::ELSEIF)
            printError(sourcePos, "No '.if' before '.elseif");
        else // else
            printError(sourcePos, "No '.if' before '.else'");
        return false;
    }
    AsmClause& clause = clauses.top();
    switch(clause.type)
    {
        case AsmClauseType::ELSE:
            if (clauseType == AsmClauseType::ELSEIF)
                printError(sourcePos, "'.elseif' after '.else'");
            else // else
                printError(sourcePos, "Duplicate of '.else'");
            printError(clause.pos, "here is previous '.else'"); 
            printError(clause.prevIfPos, "here is begin of conditional clause"); 
            return false;
        case AsmClauseType::MACRO:
            if (clauseType == AsmClauseType::ELSEIF)
                printError(sourcePos,
                       "No '.if' before '.elseif' and inside macro");
            else // else
                printError(sourcePos,
                       "No '.if' before '.else' and inside macro");
            return false;
        case AsmClauseType::REPEAT:
            if (clauseType == AsmClauseType::ELSEIF)
                printError(sourcePos,
                       "No '.if' before '.elseif' and inside repetition");
            else // else
                printError(sourcePos,
                       "No '.if' before '.else' and inside repetition");
            return false;
        default:
            break;
    }
    included = satisfied && !clause.condSatisfied;
    clause.condSatisfied |= included;
    if (clause.type == AsmClauseType::IF)
        clause.prevIfPos = clause.pos;
    clause.type = clauseType;
    clause.pos = sourcePos;
    return true;
}

bool Assembler::popClause(const AsmSourcePos& sourcePos, AsmClauseType clauseType)
{
    if (clauses.empty())
    {   // no clauses
        if (clauseType == AsmClauseType::IF)
            printError(sourcePos, "No conditional before '.endif' pseudo-op");
        else if (clauseType == AsmClauseType::MACRO) // macro
            printError(sourcePos, "No '.macro' pseudo-op before '.endm' pseudo-op");
        else if (clauseType == AsmClauseType::REPEAT) // repeat
            printError(sourcePos, "No '.rept' pseudo-op before '.endr' pseudo-op");
        return false;
    }
    AsmClause& clause = clauses.top();
    switch(clause.type)
    {
        case AsmClauseType::IF:
        case AsmClauseType::ELSE:
        case AsmClauseType::ELSEIF:
            if (clauseType == AsmClauseType::MACRO)
            {
                printError(sourcePos, "Ending macro across conditionals");
                return false;
            }
            else if (clauseType == AsmClauseType::REPEAT)
            {
                printError(sourcePos, "Ending repetition across conditionals");
                return false;
            }
            break;
        case AsmClauseType::MACRO:
            if (clauseType == AsmClauseType::REPEAT)
            {
                printError(sourcePos, "Ending repetition across macro");
                return false;
            }
            if (clauseType == AsmClauseType::IF)
            {
                printError(sourcePos, "Ending conditional across macro");
                return false;
            }
            break;
        case AsmClauseType::REPEAT:
            if (clauseType == AsmClauseType::MACRO)
            {
                printError(sourcePos, "Ending macro across repetition");
                return false;
            }
            if (clauseType == AsmClauseType::IF)
            {
                printError(sourcePos, "Ending conditional across repetition");
                return false;
            }
            break;
        default:
            break;
    }
    clauses.pop();
    return true;
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
    if (format == BinaryFormat::AMD && amdOutput == nullptr)
    {   // if not created
        amdOutput = new AmdInput{};
        amdOutput->is64Bit = _64bit;
        amdOutput->deviceType = deviceType;
        /* sections */
        sections.push_back(new AsmSection{ 0, AsmSectionType::AMD_GLOBAL_DATA });
    }
    else if (format == BinaryFormat::GALLIUM && galliumOutput == nullptr)
    {
        galliumOutput = new GalliumInput{};
        galliumOutput->code = nullptr;
        galliumOutput->codeSize = 0;
        galliumOutput->disassembly = nullptr;
        galliumOutput->disassemblySize = 0;
        galliumOutput->commentSize = 0;
        galliumOutput->comment = nullptr;
        sections.push_back(new AsmSection{ 0, AsmSectionType::GALLIUM_CODE});
    }
    else if (format == BinaryFormat::RAWCODE && rawCode == nullptr)
    {
        sections.push_back(new AsmSection{ 0, AsmSectionType::RAWCODE_CODE});
        rawCode = sections[0];
    }
    else // FAIL
        abort();
    // define counter symbol
    symbolMap.find(".")->second.sectionId = 0;
}

bool Assembler::assemble()
{
    good = true;
    while (!endOfAssembly)
    {
        if (!lineAlreadyRead)
        {   // read line
            if (!readLine())
                break;
        }
        else
        {   // already line is read
            lineAlreadyRead = false;
            if (line == nullptr)
                break; // end of stream
        }
        
        const char* string = line; // string points to place of line
        const char* end = line+lineSize;
        string = skipSpacesToEnd(string, end);
        if (string == end)
            continue; // only empty line
        
        // statement start (except labels). in this time can point to labels
        const char* stmtStartStr = string;
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
                    printError(stmtStartStr,
                               "Local label can't be defined outside section");
                    doNextLine = true;
                    break;
                }
                if (inAmdConfig)
                {
                    printError(stmtStartStr,
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
                    if (res.first->second.onceDefined && (res.first->second.isDefined ||
                        res.first->second.expression!=nullptr))
                    {   // if label
                        std::string msg = "Symbol '";
                        msg += firstName;
                        msg += "' is already defined";
                        printError(stmtStartStr, msg.c_str());
                        doNextLine = true;
                        break;
                    }
                }
                if (sections.empty())
                {
                    printError(stmtStartStr,
                               "Label can't be defined outside section");
                    doNextLine = true;
                    break;
                }
                if (inAmdConfig)
                {
                    printError(stmtStartStr,
                               "Label can't defined in AMD config place");
                    doNextLine = true;
                    break;
                }
                
                setSymbol(*res.first, currentOutPos, currentSection);
                res.first->second.onceDefined = true;
                res.first->second.sectionId = currentSection;
            }
            // new label or statement
            stmtStartStr = string;
            firstName = extractLabelName(string, end);
            string += firstName.size();
        }
        if (doNextLine)
            continue;
        
        /* now stmtStartStr - points to first string of statement
         * (labels has been skipped) */
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
            assignSymbol(firstName, stmtStartStr, string);
            continue;
        }
        // make firstname as lowercase
        toLowerString(firstName);
        
        if (firstName.size() >= 2 && firstName[0] == '.') // check for pseudo-op
        {
            parsePseudoOps(firstName, stmtStartStr, string);
        }
        else if (firstName.size() >= 1 && isDigit(firstName[0]))
            printError(stmtStartStr, "Illegal number at statement begin");
        else
        {   // try to parse processor instruction or macro substitution
        }
    }
    /* check clauses and print errors */
    while (!clauses.empty())
    {
        const AsmClause& clause = clauses.top();
        switch(clause.type)
        {
            case AsmClauseType::IF:
                printError(clause.pos, "Unterminated 'if'");
                break;
            case AsmClauseType::ELSEIF:
                printError(clause.pos, "Unterminated '.else'");
                break;
            case AsmClauseType::ELSE:
                printError(clause.pos, "Unterminated '.else'");
                break;
            case AsmClauseType::MACRO:
                printError(clause.pos, "Unterminated macro definition");
                break;
            case AsmClauseType::REPEAT:
                printError(clause.pos, "Unterminated repetition");
            default:
                break;
        }
        clauses.pop();
    }
    return good;
}
