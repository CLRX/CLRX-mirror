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

static const uint32_t spacesCharBitMask = 0x3c00; // "\t\n\v\f\r"

AsmParser::AsmParser(std::istream& is)
        : stream(is), pos(0), lastNoSpaceEndPos(0), input(200), insideString(false),
          lineNo(1), asmLineNo(1), colNo(1)
{ }

void AsmParser::readInput()
{
    if (input.size() >= 400)
    {
        input.clear();
        input.resize(200);
    }
    stream.read(input.data(), input.size());
    const size_t readed = stream.gcount();
    if (readed < input.size())
        input.resize(readed);
}

void AsmParser::keepAndReadInput()
{
    std::copy_backward(input.begin()+pos, input.end(), input.begin());
    const size_t kept = input.size()-pos;
    stream.read(input.data() + kept, pos);
    pos = kept;
    const size_t readed = stream.gcount();
    if (kept + readed < input.size())
        input.resize(kept + readed);
}

bool AsmParser::skipSpacesAndComments()
{
    if (insideString) return true;
    lastNoSpaceEndPos = 0;
    bool inLineComment = false;
    bool inLongComment = false;
    while (true)
    {
        if (inLineComment)
        {
            while (pos < input.size() && input[pos] != '\n' && input[pos] != '\\')
            { colNo++; pos++; }
            if (pos < input.size())
            {
                if (input[pos] == '\n')
                {
                    inLineComment = false;
                    colNo = 1;
                    lineNo++;
                    asmLineNo++;
                    pos++;
                }
                else // backslash
                {
                    colNo++;
                    pos++;
                    if (pos >= input.size())
                    {   // read and keep old
                        pos--;
                        keepAndReadInput();
                        pos++;
                        if (pos == input.size())
                            return false;
                    }
                    if (input[pos] == '\n')
                    {   // increment only lineNo (not joinedLineNo)
                        pos++;
                        colNo = 1;
                        lineNo++;
                    }
                }
            }
            else
            {
                readInput();
                if (input.empty())
                    return false; // no more word or other text
            }
        }
        else if (inLongComment)
        {
            while (pos < input.size() && input[pos] != '*' && input[pos] != '\n' &&
                         input[pos] != '\\')
            { colNo++; pos++; }
            if (pos < input.size())
            {
                if (input[pos] == '*')
                {
                    if (pos+1 < input.size())
                        pos++;
                    else
                    { // read next
                        readInput();
                        if (input.empty())
                            throw ParseException(lineNo, colNo,
                                     "Unterminated long comment");
                    }
                    if (input[pos] == '/')
                    {
                        inLongComment = false;
                        colNo++;
                        pos++;
                    }
                }
                else if (input[pos] == '\n')
                {
                    pos++;
                    colNo = 1;
                    lineNo++;
                    asmLineNo++;
                }
                else // backslash
                {
                    colNo++;
                    pos++;
                    if (pos >= input.size())
                    {   // read and keep old
                        pos--;
                        keepAndReadInput();
                        pos++;
                        if (pos == input.size())
                            return false;
                    }
                    if (input[pos] == '\n')
                    {   // increment only lineNo (not joinedLineNo)
                        pos++;
                        colNo = 1;
                        lineNo++;
                    }
                }
            }
        }
        else
        {
            while (pos < input.size() && input[pos] >= 0 && input[pos] <= 32 &&
                (input[pos] == 32 || (spacesCharBitMask & (1U<<input[pos])) != 0) &&
                input[pos+1] != '\\')
            { colNo++; pos++; }
            if (pos < input.size())
            {
                if (input[pos] == '\n')
                {
                    pos++;
                    colNo = 1;
                    lineNo++;
                    asmLineNo++;
                }
                else if (input[pos] == '\\')
                {
                    colNo++;
                    pos++;
                    if (pos >= input.size())
                    {   // read and keep old
                        pos--;
                        keepAndReadInput();
                        pos++;
                        if (pos == input.size())
                            return false;
                    }
                    if (input[pos] != '\n')
                        return true;
                    else
                    {   // increment only lineNo (not joinedLineNo)
                        pos++;
                        colNo = 1;
                        lineNo++;
                    }
                }
                else // found no content that is not space or comment
                    return true;
            }
            else if (pos == input.size())
            {
                readInput();
                if (input.empty())
                    return false;
            }
        }
    }
    return false;
}

const char* AsmParser::getWord(size_t& size)
{
    size_t oldPos = pos;
    if (lastNoSpaceEndPos > pos)
    {
        pos = lastNoSpaceEndPos;
        size = pos - oldPos;
        return input.data() + oldPos;
    }
    while (true)
    {
        while (pos < input.size() && (input[pos] < 0 || input[pos] > 32 ||
            (spacesCharBitMask & (1U<<input[pos])) == 0))
        { colNo++; pos++; }
        if (pos == input.size())
        {
            if (oldPos != 0)
            {
                pos = oldPos;
                keepAndReadInput();
                oldPos = 0;
                if (pos == input.size())
                    break;
            }
            else
            {   // resize same input and read data
                input.resize(input.size() + (input.size()>>1));
                stream.read(input.data() + pos, input.size() - pos);
                const size_t readed = stream.gcount();
                if (pos + readed < input.size())
                    input.resize(pos + readed);
                if (readed == 0)
                    break;
            }
        }
        else break;
    }
    size = pos - oldPos;
    lastNoSpaceEndPos = pos;
    if (pos == oldPos)
        return nullptr;
    return input.data() + oldPos;
}

const char* AsmParser::getStringLiteral(size_t& size)
{
    size_t oldPos = pos;
    if (lastNoSpaceEndPos > pos)
    {
        pos = lastNoSpaceEndPos;
        size = pos - oldPos;
        return input.data() + oldPos;
    }
    if (input[pos] != '\"')
        throw ParseException(lineNo, colNo, "Expected string literal!");
    pos++;
    while (true)
    {
        while (pos < input.size() && (input[pos] != '\"' && input[pos] != '\n' &&
            input[pos] != '\\'))
        { colNo++; pos++; }
        if (pos == input.size())
        {
            if (oldPos != 0)
            {
                pos = oldPos;
                keepAndReadInput();
                oldPos = 0;
                if (pos == input.size())
                    break;
            }
            else
            {   // resize same input and read data
                input.resize(input.size() + (input.size()>>1));
                stream.read(input.data() + pos, input.size() - pos);
                const size_t readed = stream.gcount();
                if (pos + readed < input.size())
                    input.resize(pos + readed);
                if (readed == 0)
                    break;
            }
        }
        else if (input[pos] == '\n') // newline
        {
            lineNo++;
            colNo = 1;
        }
        else break; // end of string
    }
    pos++; // end of string
    size = pos - oldPos;
    lastNoSpaceEndPos = pos;
    if (pos == oldPos)
        return nullptr;
    return input.data() + oldPos;
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
        AsmParser parser(inputStream);
        // first line
        parser.skipSpacesAndComments();
        size_t lineNo = parser.getLineNo();
        size_t joinedLineNo = parser.getAsmLineNo();
        size_t wordSize = 0;
        const char* word = parser.getWord(wordSize);
        
    }
    catch(...)
    {
        inputStream.exceptions(oldExceptions);
        throw;
    }
    inputStream.exceptions(oldExceptions);
}
