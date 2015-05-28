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

AsmStreamInputFilter::AsmStreamInputFilter(const std::string& filename)
try
        : managed(true), stream(nullptr), mode(LineMode::NORMAL)
{
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

AsmStreamInputFilter::AsmStreamInputFilter(std::istream& is)
        : managed(false), stream(&is), mode(LineMode::NORMAL)
{
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

AsmMacroInputFilter::AsmMacroInputFilter(const AsmMacro& inMacro,
        const Array<std::pair<std::string, std::string> >& inArgMap)
        : macro(inMacro), argMap(inArgMap)
{
    curColTrans = macro.colTranslations.data();
    buffer.reserve(300);
    lineNo = macro.contentLineNo;
}

const char* AsmMacroInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    buffer.clear();
    
    const LineTrans* colTransEnd = macro.colTranslations.data()+
            macro.colTranslations.size();
    const size_t contentSize = macro.content.size();
    if (pos == contentSize)
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

void AsmSymbol::removeOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex)
{ 
    auto it = std::remove(occurrencesInExprs.begin(), occurrencesInExprs.end(),
            AsmExprSymbolOccurence{expr, argIndex, opIndex});
    occurrencesInExprs.resize(it-occurrencesInExprs.begin());
}

/*
 * Assembler
 */

Assembler::Assembler(const std::string& filename, std::istream& input, cxuint flags,
        std::ostream& msgStream)
        : format(AsmFormat::CATALYST), deviceType(GPUDeviceType::CAPE_VERDE),
          isaAssembler(nullptr), symbolMap({std::make_pair(".", AsmSymbol(0, uint64_t(0)))}),
          flags(0), macroCount(0), inclusionLevel(0), macroSubstLevel(0),
          topFile(RefPtr<const AsmFile>(new AsmFile(filename))), 
          lineSize(0), line(nullptr), lineNo(0), messageStream(msgStream),
          inGlobal(true), inAmdConfig(false), currentKernel(0), currentSection(0),
          // get value reference from first symbol: '.'
          currentOutPos(symbolMap.begin()->second.value)
{
    amdOutput = nullptr;
    input.exceptions(std::ios::badbit);
    currentInputFilter = new AsmStreamInputFilter(input);
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
    
    for (auto& entry: symbolMap)    // free pending expressions
        if (!entry.second.isDefined)
            for (auto& occur: entry.second.occurrencesInExprs)
                if (occur.expression!=nullptr)
                {
                    if (--occur.expression->symOccursNum==0)
                        delete occur.expression;
                }
}

void Assembler::includeFile(const std::string& filename)
{
    asmInputFilters.push(new AsmStreamInputFilter(filename));
}

void Assembler::applyMacro(const std::string& macroName, AsmMacroArgMap argMap)
{
    auto it = macroMap.find(macroName);
    if (it == macroMap.end())
        throw Exception("Macro doesn't exists");
    asmInputFilters.push(new AsmMacroInputFilter(it->second, argMap));
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
    
    while (!symbolStack.empty())
    {
        std::pair<AsmSymbol*, size_t>& entry = symbolStack.top();
        if (entry.second < entry.first->occurrencesInExprs.size())
        {   // 
            AsmExprSymbolOccurence& occurrence =
                    entry.first->occurrencesInExprs[entry.second];
            AsmExpression* expr = occurrence.expression;
            expr->ops[occurrence.opIndex] = AsmExprOp::ARG_VALUE;
            expr->args[occurrence.argIndex].value = entry.first->value;
            
            if (--(expr->symOccursNum) == 0)
            {   // expresion has been fully resolved
                uint64_t value;
                if (!expr->evaluate(*this, value))
                {   // if failed
                    occurrence.expression = nullptr; // clear expression
                    good = false;
                    continue;
                }
                const AsmExprTarget& target = expr->target;
                switch(expr->target.type)
                {
                    case ASMXTGT_SYMBOL:
                    {    // resolve symbol
                        AsmSymbolEntry& curSymEntry = *target.symbol;
                        curSymEntry.second.value = value;
                        curSymEntry.second.isDefined = true;
                        symbolStack.push(std::make_pair(&curSymEntry.second, 0));
                        break;
                    }
                    case ASMXTGT_DATA8:
                        sections[target.sectionId].content[target.offset] =
                                cxbyte(value);
                        break;
                    case ASMXTGT_DATA16:
                        SULEV(*reinterpret_cast<uint16_t*>(sections[target.sectionId]
                                .content.data() + target.offset), uint16_t(value));
                        break;
                    case ASMXTGT_DATA32:
                        SULEV(*reinterpret_cast<uint32_t*>(sections[target.sectionId]
                                .content.data() + target.offset), uint32_t(value));
                        break;
                    case ASMXTGT_DATA64:
                        SULEV(*reinterpret_cast<uint64_t*>(sections[target.sectionId]
                                .content.data() + target.offset), uint64_t(value));
                        break;
                    default: // ISA assembler resolves this dependency
                        isaAssembler->resolveCode(sections[target.sectionId]
                                .content.data() + target.offset, target.type, value);
                        break;
                }
                delete occurrence.expression; // delete expression
            }
            occurrence.expression = nullptr; // clear expression
        }
        else // pop
        {
            entry.first->occurrencesInExprs.clear();
            symbolStack.pop();
        }
    }
    return good;
}

void Assembler::printWarning(const AsmSourcePos& pos, const char* message)
{
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

void Assembler::readLine()
{
    line = currentInputFilter->readLine(*this, lineSize);
}

static const char* pseudoOpNamesTbl[] =
{
    "32bit",
    "64bit",
    "abort",
    "align",
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

void Assembler::assemble()
{
    while (!asmInputFilters.empty())
    {
        lineNo = currentInputFilter->getLineNo();
        line = currentInputFilter->readLine(*this, lineSize);
        while (line == nullptr)
        {   // no line
            delete asmInputFilters.top();
            asmInputFilters.pop();
            if (asmInputFilters.empty())
                break;
            currentInputFilter = asmInputFilters.top();
            lineNo = currentInputFilter->getLineNo();
            line = currentInputFilter->readLine(*this, lineSize);
        }
        
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
            (firstName.front() < '0' || firstName.front() > '9' ))
        {   // assignment
            string = skipSpacesToEnd(string+1, line+lineSize);
            if (string == end)
            {
                printError(string, "Expected assignment expression");
                continue;
            }
            std::unique_ptr<AsmExpression> expr(AsmExpression::parse(*this, string, string));
            string = skipSpacesToEnd(string+1, line+lineSize);
            
            if (!expr) // no expression, errors
                continue;
            if (string != end)
            {
                printError(string, "Garbages at end of expression");
                continue;
            }
            
            std::pair<AsmSymbolMap::iterator, bool> res =
                    symbolMap.insert({ firstName, AsmSymbol()});
            if (!res.second && res.first->second.onceDefined)
            {   // found and can be only once defined
                std::string msg = "Label '";
                msg += firstName;
                msg += "' is already defined";
                printError(firstNameString, msg.c_str());
                continue;
            }
            AsmSymbolEntry& symEntry = *res.first;
            
            if (expr->symOccursNum==0)
            {   // can evalute, assign now
                uint64_t value;
                if (!expr->evaluate(*this, value))
                    continue;
                setSymbol(symEntry, value);
            }
            else // set expression
            {
                symEntry.second.expression = expr.release();
                symEntry.second.isDefined = false; // undefine symbol
            }
            continue;
        }
        else if (string != end && *string == ':')
        {   // labels
            string = skipSpacesToEnd(string+1, line+lineSize);
            if (string != end)
            {
                printError(string, "Expected assignment expression");
                continue;
            }
            if (firstName.front() >= '0' && firstName.front() <= '9')
            {   // handle local labels
                std::pair<AsmSymbolMap::iterator, bool> prevLRes =
                        symbolMap.insert({ firstName+"b", AsmSymbol() });
                std::pair<AsmSymbolMap::iterator, bool> nextLRes =
                        symbolMap.insert({ firstName+"f", AsmSymbol() });
                setSymbol(*nextLRes.first, currentOutPos);
                prevLRes.first->second.value = nextLRes.first->second.value;
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
                        continue;
                    }
                    setSymbol(*res.first, currentOutPos);
                    res.first->second.onceDefined = true;
                }
            }
            continue;
        }
        // check for pseudo-op
        const size_t pseudoOp = binaryFind(pseudoOpNamesTbl, pseudoOpNamesTbl +
                sizeof(pseudoOpNamesTbl)/sizeof(char*), firstName.c_str())-pseudoOpNamesTbl;
                
        switch(pseudoOp)
        {
            case ASMOP_32BIT:
                break;
            case ASMOP_64BIT:
                break;
            case ASMOP_ABORT:
                break;
            case ASMOP_ALIGN:
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
            case ASMOP_CATALYST:
                break;
            case ASMOP_CONFIG:
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
                break;
            case ASMOP_FUNC:
                break;
            case ASMOP_GALLIUM:
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
                break;
            case ASMOP_KERNEL:
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
                break;
        }
    }
}
