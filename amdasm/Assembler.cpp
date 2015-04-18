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

static inline const char* skipSpacesToEnd(const char* string, const char* end)
{
    while (string!=end && *string == ' ') string++;
    return string;
}

// extract sybol name or argument name or other identifier
static inline const std::string extractSymName(size_t size, const char* string)
{
    AsmSymbolEntry* entry = nullptr;
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


AsmSourceFilter::~AsmSourceFilter()
{ }

void AsmSourceFilter::translatePos(size_t colNo, uint64_t& outLineNo, size_t& outColNo) const
{
    auto found = std::lower_bound(colTranslations.begin(), colTranslations.end(),
         LineTrans({ colNo-1, 0 }),
         [](const LineTrans& t1, const LineTrans& t2)
         { return t1.position < t2.position; });
    
    outLineNo = found->lineNo;
    outColNo = colNo-found->position;
}

/*
 * AsmInputFilter
 */

static const size_t AsmParserLineMaxSize = 300;

AsmInputFilter::AsmInputFilter(std::istream& is) :
        stream(is), mode(LineMode::NORMAL)
{
    buffer.reserve(AsmParserLineMaxSize);
}

const char* AsmInputFilter::readLine(Assembler& assembler, size_t& lineSize)
{
    colTranslations.clear();
    bool endOfLine = false;
    size_t lineStart = pos;
    size_t joinStart = pos;
    size_t destPos = pos;
    bool slash = false;
    size_t backslash = false;
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
                                asterisk = false;
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
                            if (destPos-lineStart == colTranslations.back().position)
                                colTranslations.pop_back();
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
                        if (destPos-lineStart == colTranslations.back().position)
                            colTranslations.pop_back();
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
                            if (destPos-lineStart == colTranslations.back().position)
                                colTranslations.pop_back();
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
                size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 32);
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
                    size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 32);
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
            size_t size = 1+itocstrCStyle<size_t>(macro->lineNo, numBuf+1, 32);
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
                    size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 32);
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
                                      numBuf+1, 32);
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
                size_t size = 1+itocstrCStyle<size_t>(curMacro->lineNo, numBuf+1, 32);
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
        size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 32);
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
            size_t size = 1+itocstrCStyle<size_t>(curFile->lineNo, numBuf+1, 32);
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
    size_t size = 1+itocstrCStyle<size_t>(lineNo, numBuf+1, 32);
    os.write(numBuf, size);
    numBuf[0] = ':';
    size = itocstrCStyle<size_t>(colNo, numBuf+size, 32);
    numBuf[size++] = ':';
    numBuf[size++] = ' ';
    os.write(numBuf, size);
}

/*
 * expressions
 */

AsmExpression::AsmExpression(const AsmSourcePos& inPos, size_t inSymOccursNum,
          size_t inOpsNum, const AsmExprOp* inOps, size_t inOpPosNum,
          const LineCol* inOpPos, size_t inArgsNum, const AsmExprArg* inArgs)
        : sourcePos(inPos), symOccursNum(inSymOccursNum)
{
    ops.reset(new AsmExprOp[inOpsNum]);
    args.reset(new AsmExprArg[inArgsNum]);
    divsAndModsPos.reset(new LineCol[inOpPosNum]);
    std::copy(inOps, inOps+inOpsNum, ops.get());
    std::copy(inArgs, inArgs+inArgsNum, args.get());
    std::copy(inOpPos, inOpPos+inOpPosNum, divsAndModsPos.get());
}

bool AsmExpression::evaluate(Assembler& assembler, uint64_t& value) const
{
    struct ExprStackEntry
    {
        AsmExprOp op;
        cxbyte filledArgs;
        uint64_t value;
    };
    
    if (symOccursNum != 0)
        throw Exception("Expression can't be evaluated if symbols still are unresolved!");
    
    size_t argPos = 0;
    size_t opPos = 0;
    value = 0;
    std::stack<ExprStackEntry> stack;
    size_t modAndDivsPos = 0;
    
    bool failed = false;
    do {
        AsmExprOp op = ops[opPos++];
        if (op != AsmExprOp::ARG_VALUE)
        {    // push to stack
            stack.push({ op, 0, 0 });
            continue;
        }
        bool evaluated;
        value = args[argPos++].value;
        do {
            ExprStackEntry& entry = stack.top();
            op = entry.op;
            evaluated = false;
            if (entry.filledArgs == 0)
            {
                if (op == AsmExprOp::NEGATE || op == AsmExprOp::BIT_NOT ||
                    op == AsmExprOp::LOGICAL_NOT)
                {   // unary operator
                    if (op == AsmExprOp::NEGATE)
                        value = -value;
                    else if (op == AsmExprOp::BIT_NOT)
                        value = ~value;
                    else if (op == AsmExprOp::LOGICAL_NOT)
                        value = !value;
                    
                    evaluated = true;
                    stack.pop();
                }
                else
                {   // binary operator (first operand)
                    entry.filledArgs++;
                    entry.value = value;
                }
            }
            else if (entry.filledArgs == 1)
            {   // binary operators
                evaluated = true;
                switch(op)
                {
                    case AsmExprOp::ADDITION:
                        value = entry.value + value;
                        break;
                    case AsmExprOp::SUBTRACT:
                        value = entry.value - value;
                        break;
                    case AsmExprOp::MULTIPLY:
                        value = entry.value * value;
                        break;
                    case AsmExprOp::DIVISION:
                        if (value != 0)
                            value = entry.value / value;
                        else // error
                        {
                            assembler.printError(divsAndModsPos[modAndDivsPos],
                                   "Divide by zero");
                            failed = true;
                            value = 0;
                        }
                        modAndDivsPos++;
                        break;
                    case AsmExprOp::SIGNED_DIVISION:
                        if (value != 0)
                            value = int64_t(entry.value) / int64_t(value);
                        else // error
                        {
                            assembler.printError(divsAndModsPos[modAndDivsPos],
                                   "Division by zero");
                            failed = true;
                            value = 0;
                        }
                        modAndDivsPos++;
                        break;
                    case AsmExprOp::MODULO:
                        if (value != 0)
                            value = entry.value % value;
                        else // error
                        {
                            assembler.printError(divsAndModsPos[modAndDivsPos],
                                   "Division by zero");
                            failed = true;
                            value = 0;
                        }
                        modAndDivsPos++;
                        break;
                    case AsmExprOp::SIGNED_MODULO:
                        if (value != 0)
                            value = int64_t(entry.value) % int64_t(value);
                        else // error
                        {
                            assembler.printError(divsAndModsPos[modAndDivsPos],
                                   "Division by zero");
                            failed = true;
                            value = 0;
                        }
                        modAndDivsPos++;
                        break;
                    case AsmExprOp::BIT_AND:
                        value = entry.value & value;
                        break;
                    case AsmExprOp::BIT_OR:
                        value = entry.value | value;
                        break;
                    case AsmExprOp::BIT_XOR:
                        value = entry.value ^ value;
                        break;
                    case AsmExprOp::SHIFT_LEFT:
                        if (value < 64)
                            value = entry.value << value;
                        else
                        {
                            assembler.printWarning(divsAndModsPos[modAndDivsPos],
                                   "shift count out of range (between 0 and 63)");
                            value = 0;
                        }
                        modAndDivsPos++;
                        break;
                    case AsmExprOp::SHIFT_RIGHT:
                        if (value < 64)
                            value = entry.value >> value;
                        else
                        {
                            assembler.printWarning(divsAndModsPos[modAndDivsPos],
                                   "shift count out of range (between 0 and 63)");
                            value = 0;
                        }
                        modAndDivsPos++;
                        break;
                    case AsmExprOp::SIGNED_SHIFT_RIGHT:
                        if (value < 64)
                            value = int64_t(entry.value) >> value;
                        else
                        {
                            assembler.printWarning(divsAndModsPos[modAndDivsPos],
                                   "shift count out of range (between 0 and 63)");
                            value = 0;
                        }
                        modAndDivsPos++;
                        break;
                    case AsmExprOp::LOGICAL_AND:
                        value = entry.value && value;
                        break;
                    case AsmExprOp::LOGICAL_OR:
                        value = entry.value || value;
                        break;
                    case AsmExprOp::CHOICE:
                        if (entry.value)
                        {   // choose second argument, ignore last argument
                            entry.value = value;
                            entry.filledArgs = 2;
                        }
                        else    // choose last argument
                            entry.filledArgs = 3;
                        evaluated = false;
                        break;
                    case AsmExprOp::EQUAL:
                        value = (entry.value == value) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::NOT_EQUAL:
                        value = (entry.value != value) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::LESS:
                        value = (entry.value < value)? UINT64_MAX: 0;
                        break;
                    case AsmExprOp::LESS_EQ:
                        value = (entry.value <= value) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::GREATER:
                        value = (entry.value > value) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::GREATER_EQ:
                        value = (entry.value >= value) ? UINT64_MAX : 0;
                        break;
                    default:
                        break;
                }
                if (evaluated)
                    stack.pop();
            }
            else if (entry.filledArgs == 2)
            {   // choice second
                value = entry.value;
                evaluated = true;
                stack.pop();
            }
            else if (entry.filledArgs == 3) // choice last choice
            {
                evaluated = true;
                stack.pop();
            }
            
        } while (evaluated && !stack.empty());
        
    } while (!stack.empty());
    
    return !failed;
}

static const cxbyte asmOpPrioritiesTbl[] =
{
    0, // ARG_VALUE
    0, // ARG_SYMBOL
    3, // ADDITION
    3, // SUBTRACT
    1, // NEGATE
    2, // MULTIPLY
    2, // DIVISION
    2, // SIGNED_DIVISION
    2, // MODULO
    2, // SIGNED_MODULO
    7, // BIT_AND
    9, // BIT_OR
    8, // BIT_XOR
    1, // BIT_NOT
    4, // SHIFT_LEFT
    4, // SHIFT_RIGHT
    4, // SIGNED_SHIFT_RIGHT
    10, // LOGICAL_AND
    11, // LOGICAL_OR
    1, // LOGICAL_NOT
    12, // CHOICE
    6, // EQUAL
    6, // NOT_EQUAL
    5, // LESS
    5, // LESS_EQ
    5, // GREATER
    5  // GREATER_EQ
};

AsmExpression* AsmExpression::parseExpression(Assembler& assembler,
                  size_t size, const char* string, size_t& endPos)
{
    struct ConExprEntry
    {
        size_t parethesisCount;
        cxbyte priority;
        AsmExprOp op;
    };
    std::vector<std::pair<size_t, AsmExprOp> > exprOps;
    std::vector<AsmExprArg> exprArgs;
    std::stack<AsmExprOp> opStack;
    
    const char* end = string + size;
    size_t parenthesisCount = 0;
    cxuint priority = 0;
    AsmExprOp curOp = AsmExprOp::NONE;
    bool emptyParen = false;
    
    while (string != end)
    {
        string = skipSpacesToEnd(string, end);
        if (string == end) break;
        
        if (*string == '(')
        {
            if (curOp == AsmExprOp::ARG_SYMBOL && curOp == AsmExprOp::ARG_VALUE)
            {   // error
                /*assembler.addError(assembler.cur,
                                   "Divide by zero");*/
            }
        }
        else if (*string == ')')
        {
        }
    }
    return nullptr;
}

/*
 * Assembler
 */

Assembler::Assembler(std::istream& input, cxuint flags, std::ostream& msgStream)
        : macroCount(0), inclusionLevel(0), macroSubstLevel(0),
          topFile(RefPtr<const AsmFile>(new AsmFile(""))), messageStream(msgStream)
{
    
}

Assembler::~Assembler()
{
}

AsmSymbolEntry* Assembler::parseSymbol(LineCol lineCol, size_t size, const char* string)
{
    AsmSymbolEntry* entry = nullptr;
    const std::string symName = extractSymName(size, string);
    std::pair<AsmSymbolMap::iterator, bool> res = symbolMap.insert({ symName, AsmSymbol()});
    if (!res.second)
    {   // if found
        AsmSymbolMap::iterator it = res.first;
        AsmSymbol& sym = it->second;
        sym.occurrences.push_back({ topFile, topMacroSubst, lineCol.lineNo, lineCol.colNo });
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

AsmExpression* Assembler::parseExpression(LineCol lineCol, size_t stringSize,
             const char* string, size_t& outValue) const
{
    return nullptr;
}

void Assembler::assemble(size_t inputSize, const char* inputString)
{
    ArrayIStream is(inputSize, inputString);
    assemble(is);
}

void Assembler::assemble(std::istream& inputStream)
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
