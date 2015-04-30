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

/*static inline const char* skipSpacesToEnd(const char* string, const char* end)
{
    while (string!=end && *string == ' ') string++;
    return string;
}*/

// extract sybol name or argument name or other identifier
static inline const std::string extractSymName(size_t size, const char* string)
{
    size_t p = 0;
    if (0 < size && ((string[0] >= 'a' && string[0] <= 'z') ||
        (string[0] >= 'A' && string[0] <= 'Z') || string[0] == '_' || string[0] == '.' ||
         string[0] == '$'))
        for (p = 1; p < size && ((string[p] >= '0' && string[p] <= '9') ||
            (string[p] >= 'a' && string[p] <= 'z') ||
            (string[p] >= 'A' && string[p] <= 'Z') || string[p] == '_' ||
             string[p] == '.' || string[p] == '$') ; p++);
    if (p == 0) // not parsed
        return std::string();
    
    return std::string(string, string + p);
}

ISAAssembler::~ISAAssembler()
{ }

AsmMacro::AsmMacro(const AsmSourcePos& inPos, uint64_t inContentLineNo,
             const Array<AsmMacroArg>& inArgs, const std::string& inContent)
        : pos(inPos), contentLineNo(inContentLineNo), args(inArgs), content(inContent)
{ }


AsmInputFilter::~AsmInputFilter()
{ }

LineCol AsmInputFilter::translatePos(size_t position) const
{
    auto found = std::lower_bound(colTranslations.begin(), colTranslations.end(),
         LineTrans({ position, 0 }),
         [](const LineTrans& t1, const LineTrans& t2)
         { return t1.position < t2.position; });
    if (found != colTranslations.end())
        return { found->lineNo, position-found->position+1 };
    else // not found!!!
        return { 1, position+1 };
}

/*
 * AsmInputFilter
 */

static const size_t AsmParserLineMaxSize = 300;

AsmStreamInputFilter::AsmStreamInputFilter(std::istream& is) :
        stream(is), mode(LineMode::NORMAL)
{
    buffer.reserve(AsmParserLineMaxSize);
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
                if (pos < buffer.size() && buffer[pos] != '\n' &&
                    buffer[pos] != ' ' && buffer[pos] != '\t' && buffer[pos] != '\v' &&
                    buffer[pos] != '\f' && buffer[pos] != '\r')
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
                            (buffer[pos] == ' ' || buffer[pos] == '\t' ||
                            buffer[pos] == '\v' || buffer[pos] == '\f' ||
                            buffer[pos] == '\r'));
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
            
            stream.read(buffer.data()+pos, buffer.size()-pos);
            const size_t readed = stream.gcount();
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

AsmMacroInputFilter::AsmMacroInputFilter(const AsmMacro& inMacro,
        const Array<std::pair<std::string, std::string> >& inArgMap)
        : macro(inMacro), argMap(inArgMap), exit(false)
{
    curColTrans = macro.colTranslations.data();
    buffer.reserve(300);
    lineNo = macro.contentLineNo;
}

const char* AsmMacroInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    buffer.clear();
    
    const LineTrans* colTransEnd = macro.colTranslations.data()+macro.colTranslations.size();
    const size_t contentSize = macro.content.size();
    if (exit || pos == contentSize)
    {   // early exit from macro
        lineSize = 0;
        return nullptr;
    }
    
    const char* content = macro.content.c_str();
    
    colTranslations.clear();
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
                                contentSize-pos, content+pos);
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
    return buffer.data();
}

/*
 * source pos
 */

void AsmSourcePos::print(std::ostream& os) const
{
    char numBuf[32];
    if (macro)
    {
        RefPtr<const AsmMacroSubst> curMacro = macro;
        if (curMacro)
        {
            RefPtr<const AsmMacroSubst> parentMacro = curMacro->parent;
            
            RefPtr<const AsmFile> curFile = curMacro->file;
            if (curFile->parent)
            {
                os.write("In macro substituted from\n", 26);
                RefPtr<const AsmFile> parentFile = curFile->parent;
                os.write("    In file included from ", 26);
                if (!parentFile->file.empty())
                    os.write(parentFile->file.c_str(), parentFile->file.size());
                else // stdin
                    os.write("<stdin>", 7);
                
                numBuf[0] = ':';
                size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 29);
                curFile = parentFile;
                numBuf[size++] = (curFile->parent) ? ',' : ':';
                numBuf[size++] = '\n';
                os.write(numBuf, size);
                
                while (curFile->parent)
                {
                    parentFile = curFile->parent;
                    os.write("                          ", 26);
                    if (!parentFile->file.empty())
                        os.write(parentFile->file.c_str(), parentFile->file.size());
                    else // stdin
                        os.write("<stdin>", 7);
                    
                    numBuf[0] = ':';
                    size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 29);
                    curFile = curFile->parent;
                    numBuf[size++] = (curFile->parent) ? ',' : ':';
                    numBuf[size++] = '\n';
                    os.write(numBuf, size);
                }
                os.write("    ", 4);
            }
            else
                os.write("In macro substituted from ", 26);
            // leaf
            curFile = curMacro->file;
            if (!curFile->file.empty())
                os.write(curFile->file.c_str(), curFile->file.size());
            else // stdin
                os.write("<stdin>", 7);
            numBuf[0] = ':';
            size_t size = 1+itocstrCStyle<size_t>(macro->lineNo, numBuf+1, 29);
            numBuf[size++] = (parentMacro) ? ';' : ':';
            numBuf[size++] = '\n';
            os.write(numBuf, size);
            
            curMacro = parentMacro;
            
            while(curMacro)
            {
                parentMacro = curMacro->parent;
                //os.write("In macro substituted from\n", 26);
                
                curFile = curMacro->file;
                if (curFile->parent)
                {
                    RefPtr<const AsmFile> parentFile = curFile->parent;
                    os.write("    In file included from ", 26);
                    if (!parentFile->file.empty())
                        os.write(parentFile->file.c_str(), parentFile->file.size());
                    else // stdin
                        os.write("<stdin>", 7);
                    
                    numBuf[0] = ':';
                    size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 29);
                    curFile = parentFile;
                    numBuf[size++] = (curFile->parent) ? ',' : ':';
                    numBuf[size++] = '\n';
                    os.write(numBuf, size);
                    
                    while (curFile->parent)
                    {
                        parentFile = curFile->parent;
                        os.write("                          ", 26);
                        if (!parentFile->file.empty())
                            os.write(parentFile->file.c_str(), parentFile->file.size());
                        else // stdin
                            os.write("<stdin>", 7);
                        
                        numBuf[0] = ':';
                        size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo,
                                      numBuf+1, 29);
                        curFile = curFile->parent;
                        numBuf[size++] = (curFile->parent) ? ',' : ':';
                        numBuf[size++] = '\n';
                        os.write(numBuf, size);
                    }
                    os.write("    ", 4);
                }
                else
                    os.write("                          ", 26);
                // leaf
                curFile = curMacro->file;
                if (!curFile->file.empty())
                    os.write(curFile->file.c_str(), curFile->file.size());
                else // stdin
                    os.write("<stdin>", 7);
                numBuf[0] = ':';
                size_t size = 1+itocstrCStyle<size_t>(curMacro->lineNo, numBuf+1, 29);
                numBuf[size++] = (parentMacro) ? ';' : ':';
                numBuf[size++] = '\n';
                os.write(numBuf, size);
                
                curMacro = parentMacro;
            }
        }
    }
    RefPtr<const AsmFile> curFile = file;
    if (curFile->parent)
    {
        RefPtr<const AsmFile> parentFile = curFile->parent;
        os.write("In file included from ", 22);
        if (!parentFile->file.empty())
            os.write(parentFile->file.c_str(), parentFile->file.size());
        else // stdin
            os.write("<stdin>", 7);
        
        numBuf[0] = ':';
        size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 29);
        curFile = parentFile;
        numBuf[size++] = (curFile->parent) ? ',' : ':';
        numBuf[size++] = '\n';
        os.write(numBuf, size);
        
        while (curFile->parent)
        {
            parentFile = curFile->parent;
            os.write("                      ", 22);
            if (!parentFile->file.empty())
                os.write(parentFile->file.c_str(), parentFile->file.size());
            else // stdin
                os.write("<stdin>", 7);
            
            numBuf[0] = ':';
            size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 29);
            curFile = curFile->parent;
            numBuf[size++] = (curFile->parent) ? ',' : ':';
            numBuf[size++] = '\n';
            os.write(numBuf, size);
        }
    }
    // leaf
    if (!file->file.empty())
        os.write(file->file.c_str(), file->file.size());
    else // stdin
        os.write("<stdin>", 7);
    numBuf[0] = ':';
    size_t size = 1+itocstrCStyle<size_t>(lineNo, numBuf+1, 31);
    os.write(numBuf, size);
    numBuf[0] = ':';
    size = 1+itocstrCStyle<size_t>(colNo, numBuf+1, 29);
    numBuf[size++] = ':';
    numBuf[size++] = ' ';
    os.write(numBuf, size);
}

/*
 * Assembler
 */

Assembler::Assembler(const std::string& filename, std::istream& input, cxuint flags,
        std::ostream& msgStream)
        : macroCount(0), inputStream(&input), inclusionLevel(0), macroSubstLevel(0),
          topFile(RefPtr<const AsmFile>(new AsmFile(filename))), 
          lineSize(0), line(nullptr), lineNo(0), messageStream(msgStream)
{
    input.exceptions(std::ios::badbit);
    currentInputFilter = new AsmStreamInputFilter(input);
    asmInputFilters.push(currentInputFilter);
}

Assembler::~Assembler()
{
    while (!asmInputFilters.empty())
    {
        delete asmInputFilters.top();
        asmInputFilters.pop();
    }
}

uint64_t Assembler::parseLiteral(size_t linePos, size_t& outLinePos)
{
    uint64_t value;
    const char* end;
    outLinePos = linePos;
    if (outLinePos != lineSize && line[outLinePos] == '\'')
    {
        outLinePos++;
        if (outLinePos == lineSize)
        {
            printError(line+linePos, "Terminated character literal");
            throw ParseException("Terminated character literal");
        }
        if (line[outLinePos] == '\'')
        {
            printError(line+linePos, "Empty character literal");
            throw ParseException("Empty character literal");
        }
        
        if (line[outLinePos] != '\\')
        {
            value = line[outLinePos++];
            if (outLinePos == lineSize || line[outLinePos] != '\'')
            {
                printError(line+linePos, "Missing ''' at end of literal");
                throw ParseException("Missing ''' at end of literal");
            }
            outLinePos = outLinePos+1;
            return value;
        }
        else // escapes
        {
            outLinePos++;
            if (outLinePos == lineSize)
            {
                printError(line+linePos, "Terminated character literal");
                throw ParseException("Terminated character literal");
            }
            if (line[outLinePos] == 'x')
            {   // hex
                outLinePos++;
                if (outLinePos == lineSize)
                {
                    printError(line+linePos, "Terminated character literal");
                    throw ParseException("Terminated character literal");
                }
                value = 0;
                for (cxuint i = 0; outLinePos != lineSize && i < 2; i++, outLinePos++)
                {
                    cxuint digit;
                    if (line[outLinePos] >= '0' && line[outLinePos] <= '9')
                        digit = line[outLinePos]-'0';
                    else if (line[outLinePos] >= 'a' && line[outLinePos] <= 'f')
                        digit = line[outLinePos]-'a'+10;
                    else if (line[outLinePos] >= 'A' && line[outLinePos] <= 'F')
                        digit = line[outLinePos]-'A'+10;
                    else if (line[outLinePos] == '\'' && i != 0)
                        break;
                    else
                    {
                        printError(line+linePos, "Expected hexadecimal character code");
                        throw ParseException("Expected hexadecimal character code");
                    }
                    value = (value<<4) + digit;
                }
            }
            else if (line[outLinePos] >= '0' &&  line[outLinePos] <= '7')
            {   // octal
                value = 0;
                for (cxuint i = 0; outLinePos != lineSize && i < 3 &&
                            line[outLinePos] != '\''; i++, outLinePos++)
                {
                    if (line[outLinePos] < '0' || line[outLinePos] > '7')
                    {
                        printError(line+linePos, "Expected octal character code");
                        throw ParseException("Expected octal character code");
                    }
                    value = (value<<3) + uint64_t(line[outLinePos]-'0');
                    if (value > 255)
                    {
                        printError(line+linePos, "Octal code out of range");
                        throw ParseException("Octal code out of range");
                    }
                }
            }
            else
            {   // normal escapes
                const char c = line[outLinePos++];
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
            if (outLinePos == lineSize || line[outLinePos] != '\'')
            {
                printError(line+linePos, "Missing ''' at end of literal");
                throw ParseException("Missing ''' at end of literal");
            }
            outLinePos++;
            return value;
        }
    }
    try
    { value = cstrtovCStyle<uint64_t>(line+linePos, line+lineSize, end); }
    catch(const ParseException& ex)
    {
        outLinePos = end-line;
        printError(line+linePos, ex.what());
        throw;
    }
    outLinePos = end-line;
    return value;
}

AsmSymbolEntry* Assembler::parseSymbol(size_t linePos)
{
    AsmSymbolEntry* entry = nullptr;
    const std::string symName = extractSymName(lineSize-linePos, line+linePos);
    if (symName.empty())
        return nullptr;
    std::pair<AsmSymbolMap::iterator, bool> res = symbolMap.insert({ symName, AsmSymbol()});
    if (!res.second)
    {   // if found
        AsmSymbolMap::iterator it = res.first;
        AsmSymbol& sym = it->second;
        sym.occurrences.push_back(getSourcePos(linePos));
        entry = &*it;
    }
    else // if new symbol has been put
        entry = &*res.first;
    return entry;
}

void Assembler::printWarning(const AsmSourcePos& pos, const std::string& message)
{
    pos.print(messageStream);
    messageStream.write("Warning: ", 9);
    messageStream.write(message.c_str(), message.size());
    messageStream.put('\n');
}

void Assembler::printError(const AsmSourcePos& pos, const std::string& message)
{
    pos.print(messageStream);
    messageStream.write("Error: ", 7);
    messageStream.write(message.c_str(), message.size());
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

void Assembler::readLine()
{
    line = currentInputFilter->readLine(*this, lineSize);
}

void Assembler::assemble()
{
}
