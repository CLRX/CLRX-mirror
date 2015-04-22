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

AsmExpression::AsmExpression(const AsmSourcePos& inPos, size_t inSymOccursNum,
            size_t inOpsNum, size_t inOpPosNum, size_t inArgsNum)
        : sourcePos(inPos), symOccursNum(inSymOccursNum)
{
    ops.reset(new AsmExprOp[inOpsNum]);
    args.reset(new AsmExprArg[inArgsNum]);
    messagePositions.reset(new LineCol[inOpPosNum]);
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
    2, // ABOVE_EQ
    0  // CHOICE_END
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
        cxuint priority;
        cxuint lineColPos;
        ConExprArgType arg1Type;
        ConExprArgType arg2Type;
        ConExprArgType arg3Type;
        AsmExprOp op;
        ConExprArg arg1;
        ConExprArg arg2;
        ConExprArg arg3;
        cxbyte visitedArgs;
        
        ConExprNode() : parent(SIZE_MAX), priority(0), lineColPos(UINT_MAX),
              arg1Type(CXARG_NONE), arg2Type(CXARG_NONE), arg3Type(CXARG_NONE),
              op(AsmExprOp::NONE), visitedArgs(0)
        { }
    };
    std::vector<ConExprNode> exprTree(0);
    std::vector<LineCol> messagePositions;
    std::vector<LineCol> errorPositions;
    
    const char* string = assembler.line;
    const char* end = assembler.line + assembler.lineSize;
    size_t parenthesisCount = 0;
    bool afterParenthesis = false;
    size_t root = SIZE_MAX;
    size_t symOccursNum = 0;
    size_t argsNum = 0;
    
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
            (!beforeOp && curNode->arg2Type == CXARG_NONE) ||
            (!beforeOp && curNode->arg3Type == CXARG_NONE &&
             curNode->op == AsmExprOp::CHOICE));
        
        LineCol lineCol = { 0, 0 };
        AsmExprOp op = AsmExprOp::NONE;
        bool expectedPrimaryExpr = false;
        const size_t oldParenthesisCount = parenthesisCount;
        bool doExit = false;
        
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
                if (parenthesisCount==0)
                { doExit = true; break; }
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
            case '?':
                if (!beforeArg && beforeOp)
                {
                    errorPositions.push_back(assembler.translatePos(string));
                    op = AsmExprOp::CHOICE_START;
                }
                else
                    expectedPrimaryExpr = true;
                break;
            case ':':
                if (!beforeArg && beforeOp)
                    op = AsmExprOp::CHOICE;
                else
                    expectedPrimaryExpr = true;
                break;
            default: // parse symbol or value
                if (beforeArg)
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
                            symOccursNum++;
                            arg.arg.symbol = symEntry;
                            argType = CXARG_SYMBOL;
                        }
                        string += symEntry->first.size();
                    }
                    else if (parenthesisCount != 0 || *string < '0' || *string > '9')
                    {   // other we try to parse number
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
                    else // otherwise we finish parsing
                    { doExit = true; break; }
                    
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
                    argsNum++;
                }
                else
                {
                    if (parenthesisCount == 0)
                    { doExit = true; break; }
                    else
                    {
                        assembler.printError(string, "Junks at end of expression");
                        throw ParseException("Junks at end of expression");
                    }
                }
        }
        if (doExit) // exit from parsing
            break;
        
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
            
            if (op == AsmExprOp::CHOICE_START)
            {   // choice
                // first part
                if (exprTree.size() == 1 && exprTree[0].op == AsmExprOp::NONE)
                {   /* first value */
                    exprTree[0].op = op;
                    exprTree[0].lineColPos = lineColPos;
                    exprTree[0].priority = (parenthesisCount<<3) +
                            asmOpPrioritiesTbl[cxuint(op)];
                }
                else
                {   // value or node
                    const size_t nextNodeIndex = exprTree.size();
                    exprTree.push_back(ConExprNode());
                    ConExprNode& nextNode = exprTree.back();
                    nextNode.op = op;
                    nextNode.priority = (parenthesisCount<<3) +
                                asmOpPrioritiesTbl[cxuint(op)];
                    nextNode.lineColPos = lineColPos;
                    size_t leftNodeIndex = curNodeIndex;
                    if (nextNode.priority < exprTree[leftNodeIndex].priority)
                    {   /* if left side has higher priority than current */
                        while (exprTree[leftNodeIndex].parent != SIZE_MAX &&
                               nextNode.priority <
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
                        if (curNode->arg3Type != CXARG_NONE)
                        {   // binary op
                            nextNode.arg1Type = curNode->arg3Type;
                            nextNode.arg1 = curNode->arg3;
                            curNode->arg3Type = CXARG_NODE;
                            curNode->arg3.node = nextNodeIndex;
                        }
                        else if (curNode->arg2Type != CXARG_NONE)
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
            else if (op == AsmExprOp::CHOICE)
            {   /* second part */
                size_t nodeIndex = curNodeIndex;
                const size_t priority = (parenthesisCount<<3) +
                        asmOpPrioritiesTbl[cxuint(op)];
                while (nodeIndex != SIZE_MAX && priority < exprTree[nodeIndex].priority)
                    nodeIndex = exprTree[nodeIndex].parent;
                if (nodeIndex == SIZE_MAX || exprTree[nodeIndex].priority != priority ||
                    exprTree[nodeIndex].op != AsmExprOp::CHOICE)
                {
                    assembler.printError(string, "Missing '?' before ':' or too many ':'");
                    throw ParseException("Missing '?' before ':'");
                }
                ConExprNode& thisNode = exprTree[nodeIndex];
                thisNode.op = AsmExprOp::CHOICE;
                curNodeIndex = nodeIndex;
            }
            else if (binaryOp)
            {
                if (exprTree.size() == 1 && exprTree[0].op == AsmExprOp::NONE)
                {   /* first value */
                    exprTree[0].op = op;
                    exprTree[0].lineColPos = lineColPos;
                    exprTree[0].priority = (parenthesisCount<<3) +
                            asmOpPrioritiesTbl[cxuint(op)];
                    root = 0;
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
                        if (nextNode.parent == SIZE_MAX)
                            root = nextNodeIndex;
                    }
                    else
                    {   /* if left side has lower priority than current */
                        nextNode.parent = leftNodeIndex;
                        if (curNode->arg3Type != CXARG_NONE)
                        {   // binary op
                            nextNode.arg1Type = curNode->arg3Type;
                            nextNode.arg1 = curNode->arg3;
                            curNode->arg3Type = CXARG_NODE;
                            curNode->arg3.node = nextNodeIndex;
                        }
                        else if (curNode->arg2Type != CXARG_NONE)
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
                    else /* choice operator */
                    {
                        curNode->arg3Type = CXARG_NODE;
                        curNode->arg3.node = nextNodeIndex;
                    }
                    if (nextNode.parent == SIZE_MAX)
                        root = nextNodeIndex;
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
    
    /* convert into Expression */
    size_t opsNum;
    if (exprTree.size()==1 && exprTree[0].op==AsmExprOp::NONE)
        opsNum = exprTree.size()+argsNum;
    else
        opsNum = argsNum;
    
    std::unique_ptr<AsmExpression> expr(new AsmExpression(assembler.getSourcePos(string),
              symOccursNum, opsNum, messagePositions.size(), argsNum));
    
    {
        AsmExprOp* opPtr = expr->ops.get();
        AsmExprArg* argPtr = expr->args.get();
        LineCol* msgPosPtr = expr->messagePositions.get();
        
        ConExprNode* node = &exprTree[root];
        while (node != nullptr)
        {
            if (node->visitedArgs==0)
            {
                if (node->op == AsmExprOp::CHOICE_START)
                {   // error
                }
                if (node->lineColPos != UINT_MAX)
                    *msgPosPtr++ = messagePositions[node->lineColPos];
                *opPtr++ = node->op;
            }
            
            if (node->visitedArgs == 0)
            {
                node->visitedArgs++;
                if (node->arg1Type == CXARG_NODE)
                {
                    if (exprTree[node->arg1.node].visitedArgs==0)
                    {
                        node = &exprTree[node->arg1.node];
                        continue;
                    }
                }
                else if (node->arg1Type != CXARG_NONE)
                {
                    *opPtr++ = (node->arg1Type == CXARG_VALUE) ?
                            AsmExprOp::ARG_VALUE : AsmExprOp::ARG_SYMBOL;
                    *argPtr++ = node->arg1.arg;
                }
            }
            if (node->visitedArgs == 1)
            {
                node->visitedArgs++;
                if (node->arg2Type == CXARG_NODE)
                {
                    if (exprTree[node->arg2.node].visitedArgs==0)
                    {
                        node = &exprTree[node->arg2.node];
                        continue;
                    }
                }
                else if (node->arg2Type != CXARG_NONE)
                {
                    *opPtr++ = (node->arg2Type == CXARG_VALUE) ?
                            AsmExprOp::ARG_VALUE : AsmExprOp::ARG_SYMBOL;
                    *argPtr++ = node->arg2.arg;
                }
            }
            if (node->visitedArgs == 2)
            {
                node->visitedArgs++;
                if (node->arg3Type == CXARG_NODE)
                {
                    if (exprTree[node->arg3.node].visitedArgs==0)
                    {
                        node = &exprTree[node->arg3.node];
                        continue;
                    }
                }
                else if (node->arg3Type != CXARG_NONE)
                {
                    *opPtr++ = (node->arg3Type == CXARG_VALUE) ?
                            AsmExprOp::ARG_VALUE : AsmExprOp::ARG_SYMBOL;
                    *argPtr++ = node->arg3.arg;
                }
            }
            
            if (node->visitedArgs == 3) // go back
                node = (node->parent != SIZE_MAX) ? &exprTree[node->parent] : nullptr;
        }
    }
    return expr.release();
}
