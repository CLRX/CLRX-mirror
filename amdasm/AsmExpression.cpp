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
#include <vector>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>

using namespace CLRX;

static inline const char* skipSpacesToEnd(const char* string, const char* end)
{
    while (string!=end && *string == ' ') string++;
    return string;
}

/*
 * expressions
 */

AsmExpression::AsmExpression(const AsmSourcePos& _pos, size_t _symOccursNum,
          size_t _opsNum, const AsmExprOp* _ops, size_t _opPosNum,
          const LineCol* _opPos, size_t _argsNum, const AsmExprArg* _args)
        : sourcePos(_pos), symOccursNum(_symOccursNum), ops(_ops, _ops+_opsNum)
{
    args.reset(new AsmExprArg[_argsNum]);
    messagePositions.reset(new LineCol[_opPosNum]);
    std::copy(_args, _args+_argsNum, args.get());
    std::copy(_opPos, _opPos+_opPosNum, messagePositions.get());
}

AsmExpression::AsmExpression(const AsmSourcePos& _pos, size_t _symOccursNum,
            size_t _opsNum, size_t _opPosNum, size_t _argsNum)
        : sourcePos(_pos), symOccursNum(_symOccursNum), ops(_opsNum)
{
    args.reset(new AsmExprArg[_argsNum]);
    messagePositions.reset(new LineCol[_opPosNum]);
}

bool AsmExpression::evaluate(Assembler& assembler, uint64_t& value) const
{
    if (symOccursNum != 0)
        throw Exception("Expression can't be evaluated if symbols still are unresolved!");
    
    std::stack<uint64_t> stack;
    
    size_t argPos = 0;
    size_t opPos = 0;
    bool failed = false;
    size_t messagePosIndex = 0;
    
    while (opPos < ops.size())
    {
        const AsmExprOp op = ops[opPos++];
        if (op == AsmExprOp::ARG_VALUE)
        {
            stack.push(args[argPos++].value);
            continue;
        }
        value = stack.top();
        stack.pop();
        if (isUnaryOp(op))
        {
            switch (op)
            {
                case AsmExprOp::NEGATE:
                    value = -value;
                    break;
                case AsmExprOp::BIT_NOT:
                    value = ~value;
                    break;
                case AsmExprOp::LOGICAL_NOT:
                    value = !value;
                    break;
                default:
                    break;
            }
        }
        else if (isBinaryOp(op))
        {
            uint64_t value2 = stack.top();
            stack.pop();
            switch (op)
            {
                case AsmExprOp::ADDITION:
                    value = value2 + value;
                    break;
                case AsmExprOp::SUBTRACT:
                    value = value2 - value;
                    break;
                case AsmExprOp::MULTIPLY:
                    value = value2 * value;
                    break;
                case AsmExprOp::DIVISION:
                    if (value != 0)
                        value = value2 / value;
                    else // error
                    {
                        assembler.printError(getSourcePos(messagePosIndex),
                               "Division by zero");
                        failed = true;
                        value = 0;
                    }
                    messagePosIndex++;
                    break;
                case AsmExprOp::SIGNED_DIVISION:
                    if (value != 0)
                        value = int64_t(value2) / int64_t(value);
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
                        value = value2 % value;
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
                        value = int64_t(value2) % int64_t(value);
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
                    value = value2 & value;
                    break;
                case AsmExprOp::BIT_OR:
                    value = value2 | value;
                    break;
                case AsmExprOp::BIT_XOR:
                    value = value2 ^ value;
                    break;
                case AsmExprOp::BIT_ORNOT:
                    value = value2 | ~value;
                    break;
                case AsmExprOp::SHIFT_LEFT:
                    if (value < 64)
                        value = value2 << value;
                    else
                    {
                        assembler.printWarning(getSourcePos(messagePosIndex),
                               "Shift count out of range (between 0 and 63)");
                        value = 0;
                    }
                    messagePosIndex++;
                    break;
                case AsmExprOp::SHIFT_RIGHT:
                    if (value < 64)
                        value = value2 >> value;
                    else
                    {
                        assembler.printWarning(getSourcePos(messagePosIndex),
                               "Shift count out of range (between 0 and 63)");
                        value = 0;
                    }
                    messagePosIndex++;
                    break;
                case AsmExprOp::SIGNED_SHIFT_RIGHT:
                    if (value < 64)
                        value = int64_t(value2) >> value;
                    else
                    {
                        assembler.printWarning(getSourcePos(messagePosIndex),
                               "Shift count out of range (between 0 and 63)");
                        value = (value2>=(1ULL<<63)) ? UINT64_MAX : 0;
                    }
                    messagePosIndex++;
                    break;
                case AsmExprOp::LOGICAL_AND:
                    value = value2 && value;
                    break;
                case AsmExprOp::LOGICAL_OR:
                    value = value2 || value;
                    break;
                case AsmExprOp::EQUAL:
                    value = (value2 == value) ? UINT64_MAX : 0;
                    break;
                case AsmExprOp::NOT_EQUAL:
                    value = (value2 != value) ? UINT64_MAX : 0;
                    break;
                case AsmExprOp::LESS:
                    value = (int64_t(value2) < int64_t(value))? UINT64_MAX: 0;
                    break;
                case AsmExprOp::LESS_EQ:
                    value = (int64_t(value2) <= int64_t(value)) ? UINT64_MAX : 0;
                    break;
                case AsmExprOp::GREATER:
                    value = (int64_t(value2) > int64_t(value)) ? UINT64_MAX : 0;
                    break;
                case AsmExprOp::GREATER_EQ:
                    value = (int64_t(value2) >= int64_t(value)) ? UINT64_MAX : 0;
                    break;
                case AsmExprOp::BELOW:
                    value = (value2 < value)? UINT64_MAX: 0;
                    break;
                case AsmExprOp::BELOW_EQ:
                    value = (value2 <= value) ? UINT64_MAX : 0;
                    break;
                case AsmExprOp::ABOVE:
                    value = (value2 > value) ? UINT64_MAX : 0;
                    break;
                case AsmExprOp::ABOVE_EQ:
                    value = (value2 >= value) ? UINT64_MAX : 0;
                    break;
                default:
                    break;
            }
            
        }
        else if (op == AsmExprOp::CHOICE)
        {
            const uint64_t value2 = stack.top();
            stack.pop();
            const uint64_t value3 = stack.top();
            stack.pop();
            value = value3 ? value2 : value;
        }
        stack.push(value);
    }
    
    if (!stack.empty())
        value = stack.top();
    return !failed;
}

static const cxbyte asmOpPrioritiesTbl[] =
{   /* higher value, higher priority */
    7, // ARG_VALUE
    7, // ARG_SYMBOL
    6, // NEGATE
    6, // BIT_NOT
    6, // LOGICAL_NOT
    6, // PLUS
    3, // ADDITION
    3, // SUBTRACT
    5, // MULTIPLY
    5, // DIVISION
    5, // SIGNED_DIVISION
    5, // MODULO
    5, // SIGNED_MODULO
    4, // BIT_AND
    4, // BIT_OR
    4, // BIT_XOR
    4, // BIT_ORNOT
    5, // SHIFT_LEFT
    5, // SHIFT_RIGHT
    5, // SIGNED_SHIFT_RIGHT
    1, // LOGICAL_AND
    1, // LOGICAL_OR
    2, // EQUAL
    2, // NOT_EQUAL
    2, // LESS
    2, // LESS_EQ
    2, // GREATER
    2, // GREATER_EQ
    2, // BELOW
    2, // BELOW_EQ
    2, // ABOVE
    2, // ABOVE_EQ
    0, // CHOICE
    0 // CHOICE_END
};

AsmExpression* AsmExpression::parse(Assembler& assembler, size_t linePos, size_t& outLinePos)
{
    const char* outend;
    AsmExpression* expr = parse(assembler, assembler.line+linePos, outend);
    outLinePos = outend-(assembler.line+linePos);
    return expr;
}   

AsmExpression* AsmExpression::parse(Assembler& assembler, const char* string,
            const char*& outend)
{
    struct ConExprOpEntry
    {
        AsmExprOp op;
        cxuint priority;
        size_t lineColPos;
    };

    std::stack<ConExprOpEntry> stack;
    std::vector<AsmExprOp> ops;
    std::vector<AsmExprArg> args;
    std::vector<LineCol> messagePositions;
    std::vector<LineCol> outMsgPositions;
    
    const char* end = assembler.line + assembler.lineSize;
    size_t parenthesisCount = 0;
    size_t symOccursNum = 0;
    bool good = true;
    
    enum ExpectedToken
    {
        XT_FIRST = 0,
        XT_OP = 1,
        XT_ARG = 2
    };
    ExpectedToken expectedToken = XT_FIRST;
    
    while (string != end)
    {
        string = skipSpacesToEnd(string, end);
        if (string == end) break;
        
        LineCol lineCol = { 0, 0 };
        AsmExprOp op = AsmExprOp::NONE;
        bool expectedPrimaryExpr = false;
        //const size_t oldParenthesisCount = parenthesisCount;
        bool doExit = false;
        
        const char* beforeToken = string;
        switch(*string)
        {
            case '(':
                if (expectedToken == XT_OP)
                {
                    assembler.printError(string, "Expected operator");
                    good = false;
                }
                else
                {
                    expectedToken = XT_FIRST;
                    parenthesisCount++;
                }
                string++;
                break;
            case ')':
                if (expectedToken != XT_OP)
                {
                    assembler.printError(string, "Expected operator or value or symbol");
                    good = false;
                }
                else
                {
                    if (parenthesisCount==0)
                    { doExit = true; break; }
                    parenthesisCount--;
                }
                string++;
                break;
            case '+':
                op = (expectedToken == XT_OP) ? AsmExprOp::ADDITION : AsmExprOp::PLUS;
                string++;
                break;
            case '-':
                op = (expectedToken == XT_OP) ? AsmExprOp::SUBTRACT : AsmExprOp::NEGATE;
                string++;
                break;
            case '*':
                op = AsmExprOp::MULTIPLY;
                string++;
                break;
            case '/':
                lineCol = assembler.translatePos(string);
                if (string+1 != end && string[1] == '/')
                {
                    op = AsmExprOp::DIVISION;
                    string++;
                }
                else // standard GNU as signed division
                    op = AsmExprOp::SIGNED_DIVISION;
                string++;
                break;
            case '%':
                lineCol = assembler.translatePos(string);
                if (string+1 != end && string[1] == '%')
                {
                    op = AsmExprOp::MODULO;
                    string++;
                }
                else // standard GNU as signed division
                    op = AsmExprOp::SIGNED_MODULO;
                string++;
                break;
            case '&':
                if (string+1 != end && string[1] == '&')
                {
                    op = AsmExprOp::LOGICAL_AND;
                    string++;
                }
                else // standard GNU as signed division
                    op = AsmExprOp::BIT_AND;
                string++;
                break;
            case '|':
                if (string+1 != end && string[1] == '|')
                {
                    op = AsmExprOp::LOGICAL_OR;
                    string++;
                }
                else // standard GNU as signed division
                    op = AsmExprOp::BIT_OR;
                string++;
                break;
            case '!':
                if (expectedToken == XT_OP) // ORNOT or logical not
                {
                    if (string+1 != end && string[1] == '=')
                    {
                        op = AsmExprOp::NOT_EQUAL;
                        string++;
                    }
                    else
                        op = AsmExprOp::BIT_ORNOT;
                }
                else
                    op = AsmExprOp::LOGICAL_NOT;
                string++;
                break;
            case '~':
                if (expectedToken != XT_OP)
                    op = AsmExprOp::BIT_NOT;
                else
                {
                    assembler.printError(string,
                        "Expected non-unary operator, '(', or end of expression");
                    good = false;
                }
                string++;
                break;
            case '^':
                op = AsmExprOp::BIT_XOR;
                string++;
                break;
            case '<':
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
                break;
            case '>':
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
                    string++;
                }
                else if (string+1 != end && string[1] == '@')
                {
                    op = AsmExprOp::ABOVE;
                    string++;
                }
                else
                    op = AsmExprOp::GREATER;
                string++;
                break;
            case '=':
                if (string+1 != end && string[1] == '=')
                {
                    op = AsmExprOp::EQUAL;
                    string++;
                }
                else
                    expectedPrimaryExpr = true;
                string++;
                break;
            case '?':
                lineCol = assembler.translatePos(string);
                op = AsmExprOp::CHOICE_START;
                string++;
                break;
            case ':':
                op = AsmExprOp::CHOICE;
                string++;
                break;
            default: // parse symbol or value
                if (expectedToken != XT_OP)
                {
                    ExpectedToken oldExpectedToken = expectedToken;
                    expectedToken = XT_OP;
                    std::pair<AsmSymbolEntry*,bool> out = assembler.parseSymbol(string);
                    AsmSymbolEntry* symEntry = out.first;
                    if (!out.second) good = false;
                    AsmExprArg arg;
                    if (symEntry != nullptr)
                    {
                        if (symEntry->second.isDefined)
                        {
                            arg.value = symEntry->second.value;
                            args.push_back(arg);
                            ops.push_back(AsmExprOp::ARG_VALUE);
                        }
                        else
                        {
                            symOccursNum++;
                            arg.symbol = symEntry;
                            args.push_back(arg);
                            ops.push_back(AsmExprOp::ARG_SYMBOL);
                        }
                        string += symEntry->first.size();
                    }
                    else if (parenthesisCount != 0 || (*string >= '0' && *string <= '9') ||
                             *string == '\'')
                    {   // other we try to parse number
                        const char* oldStr = string;
                        try
                        { arg.value = assembler.parseLiteral(string, string); }
                        catch(const ParseException& ex)
                        {
                            arg.value = 0;
                            if (string != end && oldStr == string)
                                // skip one character when end is in this same place
                                // (avoids infinity loops)
                                string++;
                            good = false;
                        }
                        args.push_back(arg);
                        ops.push_back(AsmExprOp::ARG_VALUE);
                    }
                    else // otherwise we finish parsing
                    {
                        expectedToken = oldExpectedToken;
                        doExit = true; break;
                    }
                }
                else
                {
                    if (parenthesisCount == 0)
                    { doExit = true; break; }
                    else
                    {
                        string++;
                        assembler.printError(string, "Garbages at end of expression");
                        good = false;
                    }
                }
        }
        
        if (op != AsmExprOp::NONE && !isUnaryOp(op) && expectedToken != XT_OP)
        {
            expectedPrimaryExpr = true;
            op = AsmExprOp::NONE;
        }
        if (expectedPrimaryExpr)
        {
            assembler.printError(beforeToken,
                     "Expected primary expression before operator");
            good = false;
            continue;
        }
        
        if (op != AsmExprOp::NONE && !isUnaryOp(op))
            expectedToken = (expectedToken == XT_OP) ? XT_ARG : XT_OP;
        
        //afterParenthesis = (oldParenthesisCount < parenthesisCount);
        const size_t lineColPos = (lineCol.lineNo!=0) ? messagePositions.size() : SIZE_MAX;
        if (lineCol.lineNo!=0)
            messagePositions.push_back(lineCol);
        
        if (op != AsmExprOp::NONE)
        {   // if operator
            const bool unaryOp = isUnaryOp(op);
            const cxuint priority = (parenthesisCount<<3) +
                        asmOpPrioritiesTbl[cxuint(op)];
            
            if (op == AsmExprOp::CHOICE)
            {   /* second part of choice */
                while (!stack.empty())
                {
                    const ConExprOpEntry& entry = stack.top();
                    if (priority > entry.priority || (priority == entry.priority &&
                        entry.op == AsmExprOp::CHOICE_START))
                        break;
                    if (entry.op != AsmExprOp::PLUS)
                        ops.push_back(entry.op);
                    if (entry.lineColPos != SIZE_MAX && entry.op != AsmExprOp::CHOICE_START)
                        outMsgPositions.push_back(messagePositions[entry.lineColPos]);
                    stack.pop();
                }
                if (stack.empty() || stack.top().op != AsmExprOp::CHOICE_START ||
                        stack.top().priority != priority)
                {   // not found
                    assembler.printError(beforeToken, "Missing '?' before ':'");
                    good = false;
                    continue; // do noy change stack and them entries
                }
                ConExprOpEntry& entry = stack.top();
                entry.op = AsmExprOp::CHOICE;
                entry.lineColPos = SIZE_MAX;
            }
            else
            {
                while (!stack.empty())
                {
                    const ConExprOpEntry& entry = stack.top();
                    if (priority +
                        /* because ?: is computed from right to left we adds 1 to priority
                         * to force put new node higher than left node */
                        (op == AsmExprOp::CHOICE_START) + unaryOp > entry.priority)
                        break;
                    if (entry.op == AsmExprOp::CHOICE_START)
                    {   // unfinished choice
                        assembler.printError(messagePositions[entry.lineColPos],
                                 "Missing ':' for '?'");
                        stack.pop();
                        good = false;
                        break;
                    }
                    if (entry.op != AsmExprOp::PLUS)
                        ops.push_back(entry.op);
                    if (entry.lineColPos != SIZE_MAX && entry.op != AsmExprOp::CHOICE_START)
                        outMsgPositions.push_back(messagePositions[entry.lineColPos]);
                    stack.pop();
                }
                stack.push({ op, priority, lineColPos });
            }
        }
        
        if (doExit) // exit from parsing
            break;
    }
    if (parenthesisCount != 0)
    {   // print error
        assembler.printError(string, "Missing ')'");
        good = false;
    }
    if (expectedToken != XT_OP)
    {
        if (ops.empty() && stack.empty())
        {
            ops.push_back(AsmExprOp::ARG_VALUE);
            args.push_back({0});
        }
        else
        {
            assembler.printError(string, "Missing primary expression");
            good = false;
        }
    }
    else
    {
        while (!stack.empty())
        {
            const ConExprOpEntry& entry = stack.top();
            if (entry.op == AsmExprOp::CHOICE_START)
            {   // unfinished choice
                assembler.printError(messagePositions[entry.lineColPos],
                         "Missing ':' for '?'");
                good = false;
                break;
            }
            if (entry.op != AsmExprOp::PLUS)
                ops.push_back(entry.op);
            if (entry.lineColPos != SIZE_MAX && entry.op != AsmExprOp::CHOICE_START)
                outMsgPositions.push_back(messagePositions[entry.lineColPos]);
            stack.pop();
        }
    }
    outend = string;
    
    if (good)
    {
        std::unique_ptr<AsmExpression> expr(new AsmExpression(
                  assembler.getSourcePos(string), symOccursNum, ops.size(), ops.data(),
                  outMsgPositions.size(), outMsgPositions.data(),
                  args.size(), args.data()));
        try
        {
            for (size_t i = 0, j = 0; i < ops.size(); i++)
                if (ops[i] == AsmExprOp::ARG_SYMBOL)
                {
                    args[j].symbol->second.addOccurrenceInExpr(expr.get(), j, i);
                    j++;
                }
                else if (ops[i] == AsmExprOp::ARG_VALUE)
                    j++;
        }
        catch(...)
        {   // fallback! remove all occurrences
            for (size_t i = 0, j = 0; i < ops.size(); i++)
                if (ops[i] == AsmExprOp::ARG_SYMBOL)
                {
                    args[j].symbol->second.removeOccurrenceInExpr(expr.get(), j, i);
                    j++;
                }
                else if (ops[i] == AsmExprOp::ARG_VALUE)
                    j++;
            throw;
        }
        return expr.release();
    }
    else
        return nullptr;
}
