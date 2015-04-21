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

LineCol AsmSourceFilter::translatePos(size_t position) const
{
    auto found = std::lower_bound(colTranslations.begin(), colTranslations.end(),
         LineTrans({ position, 0 }),
         [](const LineTrans& t1, const LineTrans& t2)
         { return t1.position < t2.position; });
    
    return { found->lineNo, position-found->position+1 };
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
                            slash = false;
                        }
                        else
                            slash = (buffer[pos] == '/');
                        if (buffer[pos] == '#') // line comment
                        {
                            buffer[destPos-1] = ' ';
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
 * expressions
 */

AsmExpression::AsmExpression(const AsmSourcePos& inPos, size_t inSymOccursNum,
          size_t inOpsNum, const AsmExprOp* inOps, size_t inOpPosNum,
          const LineCol* inOpPos, size_t inArgsNum, const AsmExprArg* inArgs)
        : sourcePos(inPos), symOccursNum(inSymOccursNum)
{
    ops.reset(new AsmExprOp[inOpsNum]);
    args.reset(new AsmExprArg[inArgsNum]);
    messagePositions.reset(new LineCol[inOpPosNum]);
    std::copy(inOps, inOps+inOpsNum, ops.get());
    std::copy(inArgs, inArgs+inArgsNum, args.get());
    std::copy(inOpPos, inOpPos+inOpPosNum, messagePositions.get());
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
    size_t messagePosIndex = 0;
    
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
                            assembler.printError(getSourcePos(messagePosIndex),
                                   "Divide by zero");
                            failed = true;
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SIGNED_DIVISION:
                        if (value != 0)
                            value = int64_t(entry.value) / int64_t(value);
                        else // error
                        {
                            assembler.printError(getSourcePos(messagePosIndex),
                                   "Division by zero");
                            failed = true;
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::MODULO:
                        if (value != 0)
                            value = entry.value % value;
                        else // error
                        {
                            assembler.printError(getSourcePos(messagePosIndex),
                                   "Division by zero");
                            failed = true;
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SIGNED_MODULO:
                        if (value != 0)
                            value = int64_t(entry.value) % int64_t(value);
                        else // error
                        {
                            assembler.printError(getSourcePos(messagePosIndex),
                                   "Division by zero");
                            failed = true;
                            value = 0;
                        }
                        messagePosIndex++;
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
                    case AsmExprOp::BIT_ORNOT:
                        value = entry.value | ~value;
                        break;
                    case AsmExprOp::SHIFT_LEFT:
                        if (value < 64)
                            value = entry.value << value;
                        else
                        {
                            assembler.printWarning(getSourcePos(messagePosIndex),
                                   "shift count out of range (between 0 and 63)");
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SHIFT_RIGHT:
                        if (value < 64)
                            value = entry.value >> value;
                        else
                        {
                            assembler.printWarning(getSourcePos(messagePosIndex),
                                   "shift count out of range (between 0 and 63)");
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SIGNED_SHIFT_RIGHT:
                        if (value < 64)
                            value = int64_t(entry.value) >> value;
                        else
                        {
                            assembler.printWarning(getSourcePos(messagePosIndex),
                                   "shift count out of range (between 0 and 63)");
                            value = (entry.value>=(1ULL<<63)) ? UINT64_MAX : 0;
                        }
                        messagePosIndex++;
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
                        value = (int64_t(entry.value) < int64_t(value))? UINT64_MAX: 0;
                        break;
                    case AsmExprOp::LESS_EQ:
                        value = (int64_t(entry.value) <= int64_t(value)) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::GREATER:
                        value = (int64_t(entry.value) > int64_t(value)) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::GREATER_EQ:
                        value = (int64_t(entry.value) >= int64_t(value)) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::BELOW:
                        value = (entry.value < value)? UINT64_MAX: 0;
                        break;
                    case AsmExprOp::BELOW_EQ:
                        value = (entry.value <= value) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::ABOVE:
                        value = (entry.value > value) ? UINT64_MAX : 0;
                        break;
                    case AsmExprOp::ABOVE_EQ:
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
{   /* higher value, higher priority */
    6, // ARG_VALUE
    6, // ARG_SYMBOL
    2, // ADDITION
    2, // SUBTRACT
    5, // NEGATE
    4, // MULTIPLY
    4, // DIVISION
    4, // SIGNED_DIVISION
    4, // MODULO
    4, // SIGNED_MODULO
    3, // BIT_AND
    3, // BIT_OR
    3, // BIT_XOR
    3, // BIT_ORNOT
    5, // BIT_NOT
    4, // SHIFT_LEFT
    4, // SHIFT_RIGHT
    4, // SIGNED_SHIFT_RIGHT
    1, // LOGICAL_AND
    1, // LOGICAL_OR
    1, // LOGICAL_NOT
    0, // CHOICE
    2, // EQUAL
    2, // NOT_EQUAL
    2, // LESS
    2, // LESS_EQ
    2, // GREATER
    2, // GREATER_EQ
    2, // BELOW
    2, // BELOW_EQ
    2, // ABOVE
    2  // ABOVE_EQ
};

AsmExpression* AsmExpression::parseExpression(Assembler& assembler, size_t linePos)
{
    union ConExprArg
    {
        AsmExprArg arg;
        size_t node;
    };
    enum ConExprArgType: cxbyte
    {
        CXARG_NONE = 0,
        CXARG_VALUE = 1,
        CXARG_SYMBOL = 2,
        CXARG_NODE = 3
    };
    struct ConExprNode
    {
        size_t parent;
        size_t priority;
        cxuint lineColPos;
        struct {
            ConExprArgType arg1Type;
            ConExprArgType arg2Type;
        };
        AsmExprOp op;
        ConExprArg arg1;
        ConExprArg arg2;
        bool filled;
        
        ConExprNode() : parent(SIZE_MAX), priority(0), lineColPos(UINT_MAX),
              arg1Type(CXARG_NONE), arg2Type(CXARG_NONE), op(AsmExprOp::NONE)
        { }
    };
    std::vector<ConExprNode> exprTree(0);
    std::vector<LineCol> messagePositions;
    
    const char* string = assembler.line;
    const char* end = assembler.line + assembler.lineSize;
    size_t parenthesisCount = 0;
    bool afterParenthesis = false;
    cxuint priority = 0;
    bool emptyParen = false;
    
    size_t curNodeIndex = SIZE_MAX;
    
    while (string != end)
    {
        ConExprNode* curNode = nullptr;
        if (curNodeIndex != SIZE_MAX)
            curNode = &exprTree[curNodeIndex];
        string = skipSpacesToEnd(string, end);
        if (string == end) break;
        
        const bool beforeOp = !afterParenthesis &&
                    (curNode == nullptr || curNode->op == AsmExprOp::NONE);
        const bool beforeArg = curNode == nullptr ||
            (curNode->arg1Type == CXARG_NONE ||
            (!beforeOp && curNode->arg2Type == CXARG_NONE));
        
        const bool binaryOp = (curNode->op != AsmExprOp::BIT_NOT &&
                    curNode->op != AsmExprOp::LOGICAL_NOT &&
                    curNode->op != AsmExprOp::NEGATE);
        
        LineCol lineCol = { 0, 0 };
        AsmExprOp op = AsmExprOp::NONE;
        bool expectedPrimaryExpr = false;
        const size_t oldParenthesisCount = parenthesisCount;
        switch(*string)
        {
            case '(':
                if (!beforeArg)
                {
                    assembler.printError(string, "Expected operator");
                    throw ParseException("Expected operator");
                }
                parenthesisCount++;
                string++;
                break;
            case ')':
                if (!beforeOp && beforeArg)
                {
                    assembler.printError(string, "Expected operator or value or symbol");
                    throw ParseException("Expected operator or value or symbol");
                }
                if (parenthesisCount == 0)
                {
                    assembler.printError(string, "Too many ')'");
                    throw ParseException("Too many ')'");
                }
                parenthesisCount--;
                string++;
                break;
            case '+':
                if (!beforeArg && beforeOp) // addition
                {
                    op = AsmExprOp::ADDITION;
                    string++;
                }
                break;
            case '-':
                if (!beforeArg && beforeOp) // subtract
                {
                    op = AsmExprOp::SUBTRACT;
                    string++;
                }
                else if (beforeArg) // minus
                {
                    op = AsmExprOp::NEGATE;
                    string++;
                }
                break;
            case '*':
                if (!beforeArg && beforeOp) // multiply
                {
                    op = AsmExprOp::MULTIPLY;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '/':
                if (!beforeArg && beforeOp) // division
                {
                    lineCol = assembler.translatePos(string);
                    if (string+1 != end && string[1] == '/')
                    {
                        op = AsmExprOp::DIVISION;
                        string++;
                    }
                    else // standard GNU as signed division
                        op = AsmExprOp::SIGNED_DIVISION;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '%':
                if (!beforeArg && beforeOp) // modulo
                {
                    lineCol = assembler.translatePos(string);
                    if (string+1 != end && string[1] == '%')
                    {
                        op = AsmExprOp::MODULO;
                        string++;
                    }
                    else // standard GNU as signed division
                        op = AsmExprOp::SIGNED_MODULO;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '&':
                if (!beforeArg && beforeOp) // AND
                {
                    if (string+1 != end && string[1] == '&')
                    {
                        op = AsmExprOp::LOGICAL_AND;
                        string++;
                    }
                    else // standard GNU as signed division
                        op = AsmExprOp::BIT_AND;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '|':
                if (!beforeArg && beforeOp) // OR
                {
                    if (string+1 != end && string[1] == '|')
                    {
                        op = AsmExprOp::LOGICAL_OR;
                        string++;
                    }
                    else // standard GNU as signed division
                        op = AsmExprOp::BIT_OR;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '!':
                if (!beforeArg && beforeOp) // ORNOT or logical not
                {
                    if (string+1 != end && string[1] == '=')
                    {
                        op = AsmExprOp::NOT_EQUAL;
                        string++;
                    }
                    else
                        op = AsmExprOp::BIT_ORNOT;
                    string++;
                }
                else if (beforeArg)
                {
                    op = AsmExprOp::LOGICAL_NOT;
                    string++;
                }
                break;
            case '~':
                if (beforeArg)
                {
                    op = AsmExprOp::BIT_NOT;
                    string++;
                }
                else
                {
                    assembler.printError(string,
                        "Expected non-unary operator, '(', or end of expression");
                    throw ParseException(
                        "Expected non-unary operator, '(', or end of expression");
                }
                break;
            case '^':
                if (!beforeArg && beforeOp) // bit xor
                {
                    op = AsmExprOp::BIT_XOR;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '<':
                if (!beforeArg && beforeOp) // ORNOT or logical not
                {
                    if (string+1 != end && string[1] == '<')
                    {
                        lineCol = assembler.translatePos(string);
                        op = AsmExprOp::SHIFT_LEFT;
                        string++;
                    }
                    else if (string+1 != end && string[1] == '>')
                    {
                        op = AsmExprOp::NOT_EQUAL;
                        string++;
                    }
                    else if (string+1 != end && string[1] == '=')
                    {
                        if (string+2 != end && string[2] == '@')
                        {
                            op = AsmExprOp::BELOW_EQ;
                            string++;
                        }
                        else
                            op = AsmExprOp::LESS_EQ;
                        string++;
                    }
                    else if (string+1 != end && string[1] == '@')
                    {
                        op = AsmExprOp::BELOW;
                        string++;
                    }
                    else
                        op = AsmExprOp::LESS;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '>':
                if (!beforeArg && beforeOp) // ORNOT or logical not
                {
                    if (string+1 != end && string[1] == '>')
                    {
                        lineCol = assembler.translatePos(string);
                        if (string+2 != end && string[2] == '>')
                        {
                            op = AsmExprOp::SIGNED_SHIFT_RIGHT;
                            string++;
                        }
                        else
                            op = AsmExprOp::SHIFT_RIGHT;
                        string++;
                    }
                    else if (string+1 != end && string[1] == '=')
                    {
                        if (string+2 != end && string[2] == '@')
                        {
                            op = AsmExprOp::ABOVE_EQ;
                            string++;
                        }
                        else
                            op = AsmExprOp::GREATER_EQ;
                    }
                    else if (string+1 != end && string[1] == '@')
                    {
                        op = AsmExprOp::ABOVE;
                        string++;
                    }
                    else
                        op = AsmExprOp::GREATER;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case '=':
                if (string+1 != end && string[1] == '=')
                {
                    if (!beforeArg && beforeOp)
                    {
                        op = AsmExprOp::EQUAL;
                        string += 2;
                    }
                    else
                        expectedPrimaryExpr = true;
                }
                else
                    ; // error
                break;
            default: // parse symbol or value
                {
                    AsmSymbolEntry* symEntry = assembler.parseSymbol(string-assembler.line);
                    ConExprArgType argType = CXARG_NONE;
                    ConExprArg arg;
                    if (symEntry != nullptr)
                    {
                        if (symEntry->second.isDefined)
                        {
                            arg.arg.value = symEntry->second.value;
                            argType = CXARG_VALUE;
                        }
                        else
                        {
                            arg.arg.symbol = symEntry;
                            argType = CXARG_SYMBOL;
                        }
                        string += symEntry->first.size();
                    }
                    else // other we try to parse number
                    {
                        try
                        {
                            const char* newStrPos;
                            arg.arg.value = cstrtovCStyle<uint64_t>(
                                    string, end, newStrPos);
                            argType = CXARG_VALUE;
                            string = newStrPos;
                        }
                        catch(const ParseException& ex)
                        {
                            assembler.printError(string, ex.what());
                            throw;
                        }
                    }
                    
                    /* put argument */
                    if (curNode != nullptr)
                    {
                        if (curNode->arg1Type == CXARG_NONE)
                        {
                            curNode->arg1Type = argType;
                            curNode->arg1 = arg;
                        }
                        else
                        {
                            curNode->arg2Type = argType;
                            curNode->arg2 = arg;
                        }
                    }
                    else
                    {   // first node
                        curNodeIndex = exprTree.size();
                        exprTree.push_back(ConExprNode());
                        curNode = &exprTree.back();
                        curNode->arg1Type = argType;
                        curNode->arg1 = arg;
                        curNode->op = AsmExprOp::NONE;
                    }
                }
                break;
        }
        afterParenthesis = (oldParenthesisCount < oldParenthesisCount);
        const cxuint lineColPos = (lineCol.lineNo!=0) ? messagePositions.size() : UINT_MAX;
        if (lineCol.lineNo!=0)
            messagePositions.push_back(lineCol);
        
        if (expectedPrimaryExpr)
        {
            assembler.printError(string, "Expected primary expression before operator");
            throw ParseException("Expected primary expression before operator");
        }
        if (op != AsmExprOp::NONE)
        {   // if operator
            const bool binaryOp = (op != AsmExprOp::BIT_NOT &&
                            op != AsmExprOp::LOGICAL_NOT &&
                            op != AsmExprOp::NEGATE);
            
            if (binaryOp)
            {
                if (exprTree.size() == 1 && exprTree[0].op == AsmExprOp::NONE)
                {   /* first value */
                    exprTree[0].op = op;
                    exprTree[0].lineColPos = lineColPos;
                    exprTree[0].priority = (parenthesisCount<<3) +
                            asmOpPrioritiesTbl[cxuint(op)];
                }
                else
                {   /* next operators, add node */
                    const size_t nextNodeIndex = exprTree.size();
                    exprTree.push_back(ConExprNode());
                    ConExprNode& nextNode = exprTree.back();
                    nextNode.op = op;
                    nextNode.priority = (parenthesisCount<<3) +
                                asmOpPrioritiesTbl[cxuint(op)];
                    nextNode.lineColPos = lineColPos;
                    size_t leftNodeIndex = curNodeIndex;
                    if (nextNode.priority <= exprTree[leftNodeIndex].priority)
                    {   /* if left side has higher priority than current */
                        while (exprTree[leftNodeIndex].parent != SIZE_MAX &&
                               nextNode.priority <= 
                                   exprTree[exprTree[leftNodeIndex].parent].priority)
                            leftNodeIndex = exprTree[leftNodeIndex].parent;
                        
                        nextNode.arg1.node = leftNodeIndex;
                        nextNode.arg1Type = CXARG_NODE;
                        nextNode.parent = exprTree[leftNodeIndex].parent;
                        exprTree[leftNodeIndex].parent = nextNodeIndex;
                    }
                    else
                    {   /* if left side has lower priority than current */
                        nextNode.parent = leftNodeIndex;
                        if (curNode->arg2Type != CXARG_NONE)
                        {   // binary op
                            nextNode.arg1Type = curNode->arg2Type;
                            nextNode.arg1 = curNode->arg2;
                            curNode->arg2Type = CXARG_NODE;
                            curNode->arg2.node = nextNodeIndex;
                        }
                        else
                        {   // unary op
                            nextNode.arg1Type = curNode->arg1Type;
                            nextNode.arg1 = curNode->arg1;
                            curNode->arg1Type = CXARG_NODE;
                            curNode->arg1.node = nextNodeIndex;
                        }
                    }
                    curNodeIndex = nextNodeIndex;
                }
            }
            else
            {   // if unary operator
                const size_t nextNodeIndex = exprTree.size();
                exprTree.push_back(ConExprNode());
                ConExprNode& nextNode = exprTree.back();
                nextNode.op = op;
                nextNode.priority = (parenthesisCount<<3) + asmOpPrioritiesTbl[cxuint(op)];
                nextNode.lineColPos = lineColPos;
                
                if (curNodeIndex != SIZE_MAX)
                {
                    nextNode.parent = curNodeIndex;
                    if (curNode->arg1Type == CXARG_NONE)
                    {
                        curNode->arg1Type = CXARG_NODE;
                        curNode->arg1.node = nextNodeIndex;
                    }
                    else if (curNode->arg2Type == CXARG_NONE)
                    {
                        curNode->arg2Type = CXARG_NODE;
                        curNode->arg2.node = nextNodeIndex;
                    }
                }
                curNodeIndex = nextNodeIndex;
            }
        }
    }
    if (parenthesisCount != 0)
    {   // print error
        assembler.printError(string, "Missing ')'");
        throw ParseException("Parenthesis not closed");
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

AsmSymbolEntry* Assembler::parseSymbol(size_t linePos)
{
    AsmSymbolEntry* entry = nullptr;
    const std::string symName = extractSymName(lineSize-linePos, line+linePos);
    std::pair<AsmSymbolMap::iterator, bool> res = symbolMap.insert({ symName, AsmSymbol()});
    if (!res.second)
    {   // if found
        AsmSymbolMap::iterator it = res.first;
        AsmSymbol& sym = it->second;
        const LineCol curPos = currentInputFilter->translatePos(linePos);
        sym.occurrences.push_back({ topFile, topMacroSubst, curPos.lineNo, curPos.colNo });
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
