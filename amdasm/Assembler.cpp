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
#include <ostream>
#include <vector>
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

ISAAssembler::~ISAAssembler()
{ }

/*
 * AsmLayoutParser
 */

static const size_t AsmParserLineMaxSize = 300;

AsmInputFilter::AsmInputFilter(Assembler& inAssembler, std::istream& is) :
        assembler(inAssembler), stream(is), mode(LineMode::NORMAL),
        lineStart(0), pos(0), lineNo(1)
{
    buffer.reserve(AsmParserLineMaxSize);
}

const char* AsmInputFilter::readLine(size_t& lineSize)
{
    colTranslations.clear();
    bool endOfLine = false;
    lineStart = pos;
    size_t joinStart = pos;
    size_t destPos = pos;
    bool slash = false;
    bool backslash = false;
    bool asterisk = false;
    colTranslations.push_back({0, lineNo});
    while (!endOfLine)
    {
        switch(mode)
        {
            case LineMode::NORMAL:
            {
                if (pos < buffer.size() && buffer[pos] != '\n' &&
                    buffer[pos] != ' ' && buffer[pos] != '\t' && buffer[pos] != '\v' &&
                    buffer[pos] != '\f' && buffer[pos] != '\r')
                {   // putting regular string (no spaces)
                    do {
                        backslash = (buffer[pos] == '\\');
                        if (slash)
                        {
                            if (buffer[pos] == '*') // longComment
                            {
                                buffer[destPos-1] = ' ';
                                buffer[destPos++] = ' ';
                                mode = LineMode::LONG_COMMENT;
                                pos++;
                                break;
                            }
                            else if (buffer[pos] == '/') // line comment
                            {
                                buffer[destPos-1] = ' ';
                                buffer[destPos++] = ' ';
                                mode = LineMode::LINE_COMMENT;
                                pos++;
                                break;
                            }
                            slash = false;
                        }
                        else
                            slash = (buffer[pos] == '/');
                        
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
                        
                    } while (pos < buffer.size() && buffer[pos] != '\n' &&
                        (buffer[pos] != ' ' && buffer[pos] != '\t' &&
                        buffer[pos] != '\v' && buffer[pos] != '\f' &&
                        buffer[pos] != '\r'));
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
                            colTranslations.push_back({destPos-lineStart, lineNo});
                        }
                        pos++;
                        joinStart = pos;
                        break;
                    }
                    else if (mode == LineMode::NORMAL)
                    {   /* spaces */
                        backslash = false;
                        slash = false;
                        do {
                            buffer[destPos++] = ' ';
                            pos++;
                        } while (pos < buffer.size() && buffer[pos] != '\n' &&
                            (buffer[pos] == ' ' || buffer[pos] == '\t' ||
                            buffer[pos] == '\v' || buffer[pos] == '\f' ||
                            buffer[pos] == '\r'));
                    }
                }
                break;
            }
            case LineMode::LINE_COMMENT:
            {
                slash = false;
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
                        colTranslations.push_back({destPos-lineStart, lineNo});
                    }
                    else
                        mode = LineMode::NORMAL;
                    pos++;
                    joinStart = pos;
                }
                break;
            }
            case LineMode::LONG_COMMENT:
            {
                slash = false;
                while (pos < buffer.size() && buffer[pos] != '\n' &&
                    (!asterisk || buffer[pos] != '/'))
                {
                    backslash = (buffer[pos] == '\\');
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
                            destPos--;
                            colTranslations.push_back({destPos-lineStart, lineNo});
                        }
                        pos++;
                        joinStart = pos;
                    }
                }
                break;
            }
            case LineMode::STRING:
            case LineMode::LSTRING:
            {
                slash = false;
                const char quoteChar = (mode == LineMode::STRING)?'"':'\'';
                while (pos < buffer.size() && buffer[pos] != '\n' &&
                    (backslash || buffer[pos] != quoteChar))
                {
                    backslash = (buffer[pos] == '\\');
                    buffer[destPos++] = buffer[pos];
                    pos++;
                }
                if (pos < buffer.size())
                {
                    if (!backslash && buffer[pos] == quoteChar)
                    {
                        pos++;
                        mode = LineMode::NORMAL;
                        buffer[destPos++] = quoteChar;
                    }
                    else
                    {
                        lineNo++;
                        endOfLine = (!backslash);
                        if (backslash)
                            colTranslations.push_back({destPos-lineStart, lineNo});
                        else
                            assembler.addWarning(lineNo, pos-joinStart+1,
                                         "Unterminated string: newline inserted");
                        pos++;
                        joinStart = pos;
                    }
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
                pos = destPos;
                lineStart = 0;
            }
            if (pos == buffer.size())
                buffer.resize(std::max(AsmParserLineMaxSize, (pos>>1)+pos));
            
            stream.read(buffer.data()+pos, buffer.size()-pos);
            const size_t readed = stream.gcount();
            buffer.resize(pos+readed);
            if (readed == 0)
            {   // end of file. check comments
                if (mode == LineMode::LONG_COMMENT && lineStart!=pos)
                    assembler.addError(lineNo, pos-joinStart+1,
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

void AsmInputFilter::translatePos(size_t colNo, size_t& outLineNo, size_t& outColNo) const
{
    auto found = std::lower_bound(colTranslations.begin(), colTranslations.end(),
         LineTrans({ colNo-1, 0 }),
         [](const LineTrans& t1, const LineTrans& t2)
         { return t1.position < t2.position; });
    
    outLineNo = found->lineNo;
    outColNo = colNo-found->position;
}

/*
 * Assembler
 */

Assembler::Assembler(std::istream& input, cxuint flags)
{
}

Assembler::~Assembler()
{
}

void Assembler::addWarning(size_t lineNo, size_t colNo, const std::string& message)
{
}

void Assembler::addError(size_t lineNo, size_t colNo, const std::string& message)
{
}

void Assembler::addIncludeDir(const std::string& includeDir)
{
    includeDirs.push_back(std::string(includeDir));
}

void Assembler::addInitialDefSym(const std::string& symName, uint64_t value)
{
    defSyms.push_back({symName, value});
}

AsmExpression* Assembler::parseExpression(size_t lineNo, size_t colNo, size_t stringSize,
             const char* string, uint64_t& outValue) const
{
    return nullptr;
}

void Assembler::assemble(size_t inputSize, const char* inputString,
             std::ostream& msgStream)
{
    ArrayIStream is(inputSize, inputString);
    assemble(is, msgStream);
}

void Assembler::assemble(std::istream& inputStream, std::ostream& msgStream)
{
    const std::ios::iostate oldExceptions = inputStream.exceptions();
    inputStream.exceptions(std::ios::badbit);
    
    try
    {
        /*AsmParser parser(inputStream);
        // first line
        parser.skipSpacesAndComments();
        size_t lineNo = parser.getLineNo();
        size_t wordSize = 0;
        const char* word = parser.getWord(wordSize);*/
        
    }
    catch(...)
    {
        inputStream.exceptions(oldExceptions);
        throw;
    }
    inputStream.exceptions(oldExceptions);
}
