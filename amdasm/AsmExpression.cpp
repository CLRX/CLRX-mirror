/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
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
#include <iterator>
#include <vector>
#include <stack>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

/*
 * expressions
 */

// bitmask: if bit is enabled then operator can give message (error or warning)
static const uint64_t operatorWithMessage = 
        (1ULL<<int(AsmExprOp::DIVISION)) | (1ULL<<int(AsmExprOp::SIGNED_DIVISION)) |
        (1ULL<<int(AsmExprOp::MODULO)) | (1ULL<<int(AsmExprOp::SIGNED_MODULO)) |
        (1ULL<<int(AsmExprOp::SHIFT_LEFT)) | (1ULL<<int(AsmExprOp::SHIFT_RIGHT)) |
        (1ULL<<int(AsmExprOp::SIGNED_SHIFT_RIGHT));

AsmExpression::AsmExpression() : symOccursNum(0), relativeSymOccurs(false),
            baseExpr(false)
{ }

// set symbol occurrences, operators and arguments, line positions for messages
void AsmExpression::setParams(size_t _symOccursNum,
          bool _relativeSymOccurs, size_t _opsNum, const AsmExprOp* _ops, size_t _opPosNum,
          const LineCol* _opPos, size_t _argsNum, const AsmExprArg* _args, bool _baseExpr)
{
    symOccursNum = _symOccursNum;
    relativeSymOccurs = _relativeSymOccurs;
    baseExpr = _baseExpr;
    args.reset(new AsmExprArg[_argsNum]);
    ops.assign(_ops, _ops+_opsNum);
    messagePositions.reset(new LineCol[_opPosNum]);
    std::copy(_args, _args+_argsNum, args.get());
    std::copy(_opPos, _opPos+_opPosNum, messagePositions.get());
}

AsmExpression::AsmExpression(const AsmSourcePos& _pos, size_t _symOccursNum,
          bool _relSymOccurs, size_t _opsNum, const AsmExprOp* _ops, size_t _opPosNum,
          const LineCol* _opPos, size_t _argsNum, const AsmExprArg* _args,
          bool _baseExpr)
        : sourcePos(_pos), symOccursNum(_symOccursNum), relativeSymOccurs(_relSymOccurs),
          baseExpr(_baseExpr), ops(_ops, _ops+_opsNum)
{
    args.reset(new AsmExprArg[_argsNum]);
    messagePositions.reset(new LineCol[_opPosNum]);
    std::copy(_args, _args+_argsNum, args.get());
    std::copy(_opPos, _opPos+_opPosNum, messagePositions.get());
}

AsmExpression::AsmExpression(const AsmSourcePos& _pos, size_t _symOccursNum,
            bool _relSymOccurs, size_t _opsNum, size_t _opPosNum, size_t _argsNum,
            bool _baseExpr)
        : sourcePos(_pos), symOccursNum(_symOccursNum), relativeSymOccurs(_relSymOccurs),
          baseExpr(_baseExpr), ops(_opsNum)
{
    args.reset(new AsmExprArg[_argsNum]);
    messagePositions.reset(new LineCol[_opPosNum]);
}

AsmExpression::~AsmExpression()
{
    if (!baseExpr)
    {
        // delete all occurrences in expression at that place
        for (size_t i = 0, j = 0; i < ops.size(); i++)
            if (ops[i] == AsmExprOp::ARG_SYMBOL)
            {
                args[j].symbol->second.removeOccurrenceInExpr(this, j, i);
                j++;
            }
            else if (ops[i]==AsmExprOp::ARG_VALUE)
                j++;
    }
}

// helper for handling errors

#define ASMX_FAILED_BY_ERROR(PLACE, STRING) \
    { \
        assembler.printError(PLACE, STRING); \
        failed = true; \
    }

#define ASMX_NOTGOOD_BY_ERROR(PLACE, STRING) \
    { \
        assembler.printError(PLACE, STRING); \
        good = false; \
    }

// expression with relative symbols
struct RelMultiply
{
    // multiplication of section
    uint64_t multiply;
    AsmSectionId sectionId;
};

template<typename T1, typename T2>
static bool checkRelativesEqualityInt(T1& relatives, const T2& relatives2)
{
    size_t requals = 0;
    if (relatives2.size() != relatives.size())
        return false;
    else
    {
        // check whether relatives as same in two operands
        for (const RelMultiply& r: relatives2)
            for (RelMultiply& r2: relatives)
                if (r.multiply == r2.multiply &&
                        r.sectionId == r2.sectionId)
                {
                    r2.sectionId = ASMSECT_ABS; // ignore in next iter
                    requals++;
                    break;
                }
        if (requals != relatives.size())
            return false;
    }
    return true;
}

// check relatives equality
static bool checkRelativesEquality(Assembler& assembler,
            std::vector<RelMultiply>& relatives,
            const Array<RelMultiply>& relatives2, bool withSectionDiffs,
            bool sectDiffsPrepared, bool& tryLater)
{
    if (!withSectionDiffs || sectDiffsPrepared)
        // standard mode
        return checkRelativesEqualityInt(relatives, relatives2);
    else
    {
        const std::vector<AsmSection>& sections = assembler.getSections();
        // compare with ignoring sections with relspaces
        std::vector<RelMultiply> orels1;
        std::vector<RelMultiply> orels2;
        std::copy_if(relatives.begin(), relatives.end(), std::back_inserter(orels1),
                     [&sections](const RelMultiply& r)
                     { return sections[r.sectionId].relSpace == UINT_MAX; });
        std::copy_if(relatives2.begin(), relatives2.end(), std::back_inserter(orels2),
                     [&sections](const RelMultiply& r)
                     { return sections[r.sectionId].relSpace == UINT_MAX; });
        // now compare
        bool equal = checkRelativesEqualityInt(orels1, orels2);
        if (equal && (orels1.size()!=relatives.size() || orels2.size()!=relatives2.size()))
            // try later, if relatives has section with relspaces
            // and if other relatives are equal
            tryLater = true;
        return equal;
    }
}

// check whether sections is in resolvable section diffs
static bool checkSectionDiffs(size_t n, const RelMultiply* relMultiplies,
                    const std::vector<AsmSection>& sections, bool withSectionDiffs,
                    bool sectDiffsPreppared, bool& tryLater)
{
    if (n == 0)
        return true;
    
    if (!withSectionDiffs || sectDiffsPreppared)
        return false;
    
    for (size_t i = 0; i < n; i++)
        if (sections[relMultiplies[i].sectionId].relSpace == UINT_MAX)
            return false; // if found section not in anything relocation space
    tryLater = true;
    return true;
}

#define CHKSREL(rel) checkSectionDiffs(rel.size(), rel.data(), sections, \
                withSectionDiffs, sectDiffsPrepared, tryLater)

AsmTryStatus AsmExpression::tryEvaluate(Assembler& assembler, size_t opStart, size_t opEnd,
            uint64_t& outValue, AsmSectionId& outSectionId, bool withSectionDiffs) const
{
    if (symOccursNum != 0)
        throw AsmException("Expression can't be evaluated if "
                    "symbols still are unresolved!");
    
    bool failed = false;
    bool tryLater = false;
    uint64_t value = 0; // by default is zero
    AsmSectionId sectionId = 0;
    if (!relativeSymOccurs)
    {
        // all value is absolute
        std::stack<uint64_t> stack;
        
        size_t argPos = 0;
        size_t opPos = 0;
        size_t messagePosIndex = 0;
        
        // move messagePosIndex and argument position to opStart position
        for (opPos = 0; opPos < opStart; opPos++)
        {
            if (ops[opPos]==AsmExprOp::ARG_VALUE)
                argPos++;
            if ((operatorWithMessage & (1ULL<<cxuint(ops[opPos])))!=0)
                messagePosIndex++;
        }
        
        while (opPos < opEnd)
        {
            const AsmExprOp op = ops[opPos++];
            if (op == AsmExprOp::ARG_VALUE)
            {
                // push argument to stack
                stack.push(args[argPos++].value);
                continue;
            }
            value = stack.top();
            stack.pop();
            if (isUnaryOp(op))
            {
                // unary operator (-,~,!)
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
                // get first argument (second in stack)
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
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SIGNED_DIVISION:
                        if (value != 0)
                            value = int64_t(value2) / int64_t(value);
                        else // error
                        {
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::MODULO:
                        if (value != 0)
                            value = value2 % value;
                        else // error
                        {
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SIGNED_MODULO:
                        if (value != 0)
                            value = int64_t(value2) % int64_t(value);
                        else // error
                        {
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
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
                // get second and first (second and third in stack)
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
        sectionId = ASMSECT_ABS;
    }
    else
    {
        const bool sectDiffsPrepared = assembler.sectionDiffsPrepared;
        
        // structure that contains relatives info: offset value and mult. of sections
        struct ValueAndMultiplies
        {
            uint64_t value;
            Array<RelMultiply> relatives;
            
            ValueAndMultiplies(uint64_t _value = 0) : value(_value)
            { }
            ValueAndMultiplies(uint64_t _value, AsmSectionId _sectionId) : value(_value),
                relatives({{1,_sectionId}})
            { }
        };
        
        std::stack<ValueAndMultiplies> stack;
        size_t argPos = 0;
        size_t opPos = 0;
        size_t messagePosIndex = 0;
        std::vector<RelMultiply> relatives;
        
        // move messagePosIndex and argument position to opStart position
        for (opPos = 0; opPos < opStart; opPos++)
        {
            if (ops[opPos]==AsmExprOp::ARG_VALUE)
                argPos++;
            if ((operatorWithMessage & (1ULL<<cxuint(ops[opPos])))!=0)
                messagePosIndex++;
        }
        
        const std::vector<Array<AsmSectionId> >& relSpacesSects =
                    assembler.relSpacesSections;
        const std::vector<AsmSection>& sections = assembler.sections;
        
        while (opPos < opEnd)
        {
            const AsmExprOp op = ops[opPos++];
            if (op == AsmExprOp::ARG_VALUE)
            {
                if (args[argPos].relValue.sectionId != ASMSECT_ABS)
                {
                    uint64_t ovalue = args[argPos].relValue.value;
                    AsmSectionId osectId = args[argPos].relValue.sectionId;
                    if (sectDiffsPrepared && sections[osectId].relSpace!=UINT_MAX)
                    {
                        // resolve section in relspace
                        AsmSectionId rsectId =
                                relSpacesSects[sections[osectId].relSpace][0];
                        ovalue += sections[osectId].relAddress -
                                sections[rsectId].relAddress;
                        osectId = rsectId;
                    }
                    stack.push(ValueAndMultiplies(ovalue, osectId));
                }
                else
                    stack.push(args[argPos].value);
                argPos++;
                continue;
            }
            value = stack.top().value;
            relatives.assign(stack.top().relatives.begin(), stack.top().relatives.end());
            stack.pop();
            
            // handle unary operators
            if (isUnaryOp(op))
            {
                switch (op)
                {
                    case AsmExprOp::NEGATE:
                        for (RelMultiply& r: relatives)
                            r.multiply = -r.multiply;
                        value = -value;
                        break;
                    case AsmExprOp::BIT_NOT:
                        for (RelMultiply& r: relatives)
                            r.multiply = -r.multiply;
                        value = ~value;
                        break;
                    case AsmExprOp::LOGICAL_NOT:
                        if (!relatives.empty())
                            ASMX_FAILED_BY_ERROR(sourcePos,
                                 "Logical negation is not allowed to relative values")
                        value = !value;
                        break;
                    default:
                        break;
                }
            }
            else if (isBinaryOp(op))
            {
                uint64_t value2 = stack.top().value;
                const Array<RelMultiply> relatives2 = stack.top().relatives;
                stack.pop();
                switch (op)
                {
                    case AsmExprOp::ADDITION:
                    case AsmExprOp::SUBTRACT:
                    {
                        if (op == AsmExprOp::SUBTRACT)
                        {
                            for (RelMultiply& r: relatives)
                                r.multiply = -r.multiply; // negate before subtraction
                            value = -value;
                        }
                        for (const RelMultiply& r2: relatives2)
                        {
                            bool rfound = false;
                            for (RelMultiply& r: relatives)
                                if (r.sectionId == r2.sectionId)
                                {
                                    r.multiply += r2.multiply;
                                    rfound = true;
                                }
                           if (!rfound)
                               relatives.push_back(r2);
                        }
                        // remove zeroes from relatives
                        relatives.resize(std::remove_if(relatives.begin(), relatives.end(),
                           [](const RelMultiply& r) { return r.multiply==0; }) -
                           relatives.begin());
                        value = value2 + value;
                        break;
                    }
                    case AsmExprOp::MULTIPLY:
                        // we do not multiply with relatives in two operands
                        if (!CHKSREL(relatives) && !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos,
                                 "Multiplication is not allowed for two relative values")
                        if (relatives2.empty())
                        {
                            // multiply relatives
                            if (value2 != 0)
                                for (RelMultiply& r: relatives)
                                    r.multiply *= value2;
                            else
                                relatives.clear();
                        }
                        else
                        {
                            // multiply relatives2
                            if (value != 0)
                            {
                                relatives.assign(relatives2.begin(), relatives2.end());
                                for (RelMultiply& r: relatives)
                                    r.multiply *= value;
                            }
                        }
                        value = value2 * value;
                        break;
                    case AsmExprOp::DIVISION:
                        if (!CHKSREL(relatives) || !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos,
                                 "Division is not allowed for any relative value")
                        if (value != 0)
                            value = value2 / value;
                        else
                        {
                            // error
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SIGNED_DIVISION:
                        if (!CHKSREL(relatives)|| !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos,
                                 "Signed division is not allowed for any relative value")
                        if (value != 0)
                            value = int64_t(value2) / int64_t(value);
                        else
                        {
                            // error
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::MODULO:
                        if (!CHKSREL(relatives) || !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos,
                                 "Modulo is not allowed for any relative value")
                        if (value != 0)
                            value = value2 % value;
                        else
                        {
                            // error
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SIGNED_MODULO:
                        if (!CHKSREL(relatives) || !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos,
                                 "Signed Modulo is not allowed for any relative value")
                        if (value != 0)
                            value = int64_t(value2) % int64_t(value);
                        else
                        {
                            // error
                            ASMX_FAILED_BY_ERROR(getSourcePos(messagePosIndex),
                                   "Division by zero")
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::BIT_AND:
                    {
                        bool norel1 = CHKSREL(relatives);
                        bool norel2 = CHKSREL(relatives2);
                        if ((norel1 && value==0) || (norel2 && value2==0))
                        {
                            relatives.clear();
                            value = 0;
                        }
                        else if (norel1 && value==UINT64_MAX)
                        {
                            relatives.assign(relatives2.begin(), relatives2.end());
                            value = value2;
                        }
                        else if (norel2 && value2==UINT64_MAX)
                        { } // keep old value
                        else if (!norel1 || !norel2)
                            ASMX_FAILED_BY_ERROR(sourcePos, "Binary AND is not allowed "
                                    "for any relative value except special cases")
                        else
                            value = value2 & value;
                        break;
                    }
                    case AsmExprOp::BIT_OR:
                    {
                        bool norel1 = CHKSREL(relatives);
                        bool norel2 = CHKSREL(relatives2);
                        if ((norel1 && value==UINT64_MAX) || (norel2 && value2==UINT64_MAX))
                        {
                            relatives.clear();
                            value = UINT64_MAX;
                        }
                        else if (norel1 && value==0)
                        {
                            relatives.assign(relatives2.begin(), relatives2.end());
                            value = value2;
                        }
                        else if (norel2 && value2==0)
                        { } // keep old value
                        else if (!norel1 || !norel2)
                            ASMX_FAILED_BY_ERROR(sourcePos, "Binary OR is not allowed "
                                    "for any relative value except special cases")
                        else
                            value = value2 | value;
                        break;
                    }
                    case AsmExprOp::BIT_XOR:
                    {
                        bool norel1 = CHKSREL(relatives);
                        bool norel2 = CHKSREL(relatives2);
                        if (norel1 && value==0)
                        {
                            relatives.assign(relatives2.begin(), relatives2.end());
                            value = value2;
                        }
                        else if (norel2 && value2==0)
                        { } // keep old value
                        else if (!norel1 || !norel2)
                            ASMX_FAILED_BY_ERROR(sourcePos, "Binary XOR is not allowed "
                                    "for any relative value except special cases")
                        else
                            value = value2 ^ value;
                        break;
                    }
                    case AsmExprOp::BIT_ORNOT:
                    {
                        bool norel1 = CHKSREL(relatives);
                        bool norel2 = CHKSREL(relatives2);
                        if ((norel1 && value==0) || (norel2 && value2==UINT64_MAX))
                        {
                            relatives.clear();
                            value = UINT64_MAX;
                        }
                        else if (norel1 && value==UINT64_MAX)
                        {
                            relatives.assign(relatives2.begin(), relatives2.end());
                            value = value2;
                        }
                        else if (norel2 && value2 == 0)
                        {
                            // 0 | ~v
                            for (RelMultiply& r: relatives)
                                r.multiply = -r.multiply;
                            value = ~value;
                        }
                        else if (!norel1 || !norel2)
                            ASMX_FAILED_BY_ERROR(sourcePos, "Binary ORNOT is not allowed "
                                    "for any relative value except special cases")
                        else
                            value = value2 | ~value;
                        break;
                    }
                    case AsmExprOp::SHIFT_LEFT:
                        if (!CHKSREL(relatives))
                            ASMX_FAILED_BY_ERROR(sourcePos, "Shift left is not allowed "
                                    "for any for relative second value")
                        else if (value < 64)
                        {
                            relatives.assign(relatives2.begin(), relatives2.end());
                            for (RelMultiply& r: relatives)
                                r.multiply <<= value;
                            value = value2 << value;
                        }
                        else
                        {
                            assembler.printWarning(getSourcePos(messagePosIndex),
                                   "Shift count out of range (between 0 and 63)");
                            value = 0;
                        }
                        messagePosIndex++;
                        break;
                    case AsmExprOp::SHIFT_RIGHT:
                        if (!CHKSREL(relatives) || !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos,
                                 "Shift right is not allowed for any relative value")
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
                        if (!CHKSREL(relatives) || !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos, "Signed shift right is not "
                                    "allowed for any relative value")
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
                    {
                        bool norel1 = CHKSREL(relatives);
                        bool norel2 = CHKSREL(relatives2);
                        if ((norel1 && value==0) || (norel2 && value2==0))
                        {
                            relatives.clear();
                            value = 0;
                        }
                        else if (!norel1 || !norel2)
                            ASMX_FAILED_BY_ERROR(sourcePos, "Logical AND is not allowed "
                                    "for any relative value except special cases")
                        else
                            value = value2 && value;
                        break;
                    }
                    case AsmExprOp::LOGICAL_OR:
                    {
                        bool norel1 = CHKSREL(relatives);
                        bool norel2 = CHKSREL(relatives2);
                        if ((norel1 && value!=0) || (norel2 && value2!=0))
                        {
                            relatives.clear();
                            value = 1;
                        }
                        else if (!CHKSREL(relatives) || !CHKSREL(relatives2))
                            ASMX_FAILED_BY_ERROR(sourcePos, "Logical OR is not allowed "
                                    "for any relative value except special cases")
                        else
                            value = value2 || value;
                        break;
                    }
                    case AsmExprOp::EQUAL:
                    case AsmExprOp::NOT_EQUAL:
                    case AsmExprOp::LESS:
                    case AsmExprOp::LESS_EQ:
                    case AsmExprOp::GREATER:
                    case AsmExprOp::GREATER_EQ:
                    case AsmExprOp::BELOW:
                    case AsmExprOp::BELOW_EQ:
                    case AsmExprOp::ABOVE:
                    case AsmExprOp::ABOVE_EQ:
                    {
                        if (!checkRelativesEquality(assembler, relatives, relatives2,
                                    withSectionDiffs, sectDiffsPrepared, tryLater))
                            ASMX_FAILED_BY_ERROR(sourcePos, "For comparisons "
                                        "two values must have this same relatives!")
                        relatives.clear();
                        switch(op)
                        {
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
                                value = (int64_t(value2) <= int64_t(value)) ?
                                        UINT64_MAX : 0;
                                break;
                            case AsmExprOp::GREATER:
                                value = (int64_t(value2) > int64_t(value)) ? UINT64_MAX : 0;
                                break;
                            case AsmExprOp::GREATER_EQ:
                                value = (int64_t(value2) >= int64_t(value)) ?
                                        UINT64_MAX : 0;
                                break;
                            case AsmExprOp::BELOW:
                                value = (value2 < value)? UINT64_MAX: 0;
                                break;
                            case AsmExprOp::BELOW_EQ:
                                value = (value2 <= value)? UINT64_MAX: 0;
                                break;
                            case AsmExprOp::ABOVE:
                                value = (value2 > value)? UINT64_MAX: 0;
                                break;
                            case AsmExprOp::ABOVE_EQ:
                                value = (value2 >= value)? UINT64_MAX: 0;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            else if (op == AsmExprOp::CHOICE)
            {
                const uint64_t value2 = stack.top().value;
                const Array<RelMultiply> relatives2 = stack.top().relatives;
                stack.pop();
                const uint64_t value3 = stack.top().value;
                const Array<RelMultiply> relatives3 = stack.top().relatives;
                stack.pop();
                if (!CHKSREL(relatives3))
                    ASMX_FAILED_BY_ERROR(sourcePos,
                         "Choice is not allowed for first relative value")
                if (value3)
                    relatives.assign(relatives2.begin(), relatives2.end());
                value = value3 ? value2 : value;
            }
            
            ValueAndMultiplies relOut(value);
            relOut.relatives.assign(relatives.begin(), relatives.end());
            stack.push(relOut);
        }
        
        if (!stack.empty())
        {
            value = stack.top().value;
            relatives.assign(stack.top().relatives.begin(), stack.top().relatives.end());
        }
        
        if (relatives.empty())
            sectionId = ASMSECT_ABS;
        else if (relatives.size() == 1 && relatives.front().multiply == 1)
            sectionId = relatives.front().sectionId;
        else
        {
            bool moreThanOneRelSpace = false;
            cxuint relSpace = UINT_MAX;
            if (withSectionDiffs && !sectDiffsPrepared)
                // resolve whether is one relspace or not
                for (const RelMultiply r: relatives)
                {
                    if (sections[r.sectionId].relSpace != UINT_MAX)
                    {
                        if (relSpace != UINT_MAX &&
                            relSpace != sections[r.sectionId].relSpace)
                        {
                            moreThanOneRelSpace = true;
                            break;
                        }
                        relSpace = sections[r.sectionId].relSpace;
                    }
                    else // treat section without relspace as separate relspace
                    {
                        moreThanOneRelSpace = true;
                        break;
                    }
                }
            else
                moreThanOneRelSpace = true;
            
            if (moreThanOneRelSpace)
                ASMX_FAILED_BY_ERROR(sourcePos,
                        "Only one relative=1 (section) can be result of expression")
            else // not sure, should be evaluated after section diffs resolving
                tryLater = true;
        }
        
        if (sectDiffsPrepared && sectionId != ASMSECT_ABS &&
            sections[sectionId].relSpace != UINT_MAX)
        {   // find proper section
            const Array<AsmSectionId>& rlSections  = relSpacesSects[
                                sections[sectionId].relSpace];
            uint64_t valAddr = sections[sectionId].relAddress + value;
            auto it = std::lower_bound(rlSections.begin(), rlSections.end(), ASMSECT_ABS,
                [&sections,valAddr](AsmSectionId a, AsmSectionId b)
                {
                    uint64_t relAddr1 = a!=ASMSECT_ABS ? sections[a].relAddress : valAddr;
                    uint64_t relAddr2 = b!=ASMSECT_ABS ? sections[b].relAddress : valAddr;
                    return relAddr1 < relAddr2;
                });
            // if section address higher than current address
            if ((it == rlSections.end() || sections[*it].relAddress != valAddr) &&
                    it != rlSections.begin())
                --it;
            
            AsmSectionId newSectionId = (it != rlSections.end()) ? *it : rlSections.back();
            value += sections[sectionId].relAddress - sections[newSectionId].relAddress;
            sectionId = newSectionId;
        }
    }
    if (tryLater)
        return AsmTryStatus::TRY_LATER;
    if (!failed)
    {
        // write results only if no errors
        outSectionId = sectionId;
        outValue = value;
    }
    return failed ? AsmTryStatus::FAILED : AsmTryStatus::SUCCESS;
}

static const cxbyte asmOpPrioritiesTbl[] =
{
    /* higher value, higher priority */
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

// method used also by AsmExprParse test co create expression from string
AsmExpression* AsmExpression::parse(Assembler& assembler, size_t& linePos,
                bool makeBase, bool dontResolveSymbolsLater)
{
    const char* outend = assembler.line+linePos;
    AsmExpression* expr = parse(assembler, outend, makeBase, dontResolveSymbolsLater);
    linePos = outend-(assembler.line+linePos);
    return expr;
}   

struct CLRX_INTERNAL SymbolSnapshotHash: std::hash<CString>
{
    size_t operator()(const AsmSymbolEntry* e1) const
    { return static_cast<const std::hash<CString>&>(*this)(e1->first); }
};

struct CLRX_INTERNAL SymbolSnapshotEqual
{
    bool operator()(const AsmSymbolEntry* e1, const AsmSymbolEntry* e2) const
    { return e1->first == e2->first; }
};

/// internal class of temporary symbol (snapshot of symbol) map 
class CLRX_INTERNAL CLRX::AsmExpression::TempSymbolSnapshotMap: 
        public std::unordered_set<AsmSymbolEntry*, SymbolSnapshotHash, SymbolSnapshotEqual>
{ };

AsmExpression* AsmExpression::createForSnapshot(const AsmSourcePos* exprSourcePos) const
{
    std::unique_ptr<AsmExpression> expr(new AsmExpression);
    size_t argsNum = 0;
    size_t msgPosNum = 0;
    for (AsmExprOp op: ops)
        if (AsmExpression::isArg(op))
            argsNum++;
        else if (operatorWithMessage & (1ULL<<int(op)))
            msgPosNum++;
    expr->sourcePos = sourcePos;
    expr->sourcePos.exprSourcePos = exprSourcePos;
    expr->ops = ops;
    expr->args.reset(new AsmExprArg[argsNum]);
    std::copy(args.get(), args.get()+argsNum, expr->args.get());
    expr->messagePositions.reset(new LineCol[msgPosNum]);
    std::copy(messagePositions.get(), messagePositions.get()+msgPosNum,
              expr->messagePositions.get());
    return expr.release();
}

/* create symbol entry for temporary snapshot expression. this is copy of
 * original symbol entry that can holds snapshot expression instead of normal expression */
static inline AsmSymbolEntry* createSymbolEntryForSnapshot(const AsmSymbolEntry& symEntry,
            const AsmSourcePos* exprSourcePos)
{
    std::unique_ptr<AsmExpression> expr(
            symEntry.second.expression->createForSnapshot(exprSourcePos));
    
    std::unique_ptr<AsmSymbolEntry> newSymEntry(new AsmSymbolEntry{
            symEntry.first, AsmSymbol(expr.get(), false, false)});
    
    expr->setTarget(AsmExprTarget::symbolTarget(newSymEntry.get()));
    expr.release();
    newSymEntry->second.base = true;
    return newSymEntry.release();
}

// main routine to create symbol snapshot with snapshot expressions
bool AsmExpression::makeSymbolSnapshot(Assembler& assembler,
           TempSymbolSnapshotMap* snapshotMap, const AsmSymbolEntry& symEntry,
           AsmSymbolEntry*& outSymEntry, const AsmSourcePos* topParentSourcePos)
{
    struct StackEntry
    {
        AsmSymbolEntry* entry;
        size_t opIndex;
        size_t argIndex;
        
        explicit StackEntry(AsmSymbolEntry* _entry)
            : entry(_entry), opIndex(0), argIndex(0)
        { }
        
        AsmSymbolEntry* releaseEntry()
        {
            AsmSymbolEntry* out = entry;
            entry = nullptr;
            return out;
        }
    };
    std::stack<StackEntry> stack;
    
    outSymEntry = nullptr;
    {
        std::unique_ptr<AsmSymbolEntry> newSymEntry(
                    createSymbolEntryForSnapshot(symEntry, topParentSourcePos));
        auto res = snapshotMap->insert(newSymEntry.get());
        if (!res.second)
        {
            // do nothing (symbol snapshot already made)
            outSymEntry = *res.first;
            outSymEntry->second.refCount++;
            return true;
        }
        stack.push(StackEntry(newSymEntry.release()));
    }
    
    bool good = true;
    
    while (!stack.empty())
    {
        StackEntry& se = stack.top();
        size_t opIndex = se.opIndex;
        size_t argIndex = se.argIndex;
        AsmExpression* expr = se.entry->second.expression;
        const size_t opsSize = expr->ops.size();
        
        AsmExprArg* args = expr->args.get();
        AsmExprOp* ops = expr->ops.data();
        if (opIndex < opsSize)
        {
            for (; opIndex < opsSize; opIndex++)
                if (ops[opIndex] == AsmExprOp::ARG_SYMBOL)
                {
                    // check this symbol
                    AsmSymbolEntry* nextSymEntry = args[argIndex].symbol;
                    if (nextSymEntry->second.base)
                    {
                        // new base expression (set by using .'eqv')
                        std::unique_ptr<AsmSymbolEntry> newSymEntry(
                                    createSymbolEntryForSnapshot(*nextSymEntry,
                                     &(expr->sourcePos)));
                        auto res = snapshotMap->insert(newSymEntry.get());
                        if (!res.second)
                        {
                            // replace this symEntry by symbol from tempSymbolMap
                            nextSymEntry = *res.first;
                            args[argIndex].symbol = nextSymEntry;
                            nextSymEntry->second.refCount++;
                        }
                        else
                        {
                            // new symEntry to stack
                            stack.push(StackEntry(newSymEntry.release()));
                            se.argIndex = argIndex;
                            se.opIndex = opIndex;
                            break;
                        }
                    }
                    
                    if (nextSymEntry->second.hasValue)
                    {
                        // put value to argument
                        if (nextSymEntry->second.regRange)
                            ASMX_NOTGOOD_BY_ERROR(expr->getSourcePos(),
                                                 "Expression have register symbol")
                        ops[opIndex] = AsmExprOp::ARG_VALUE;
                        args[argIndex].relValue.value = nextSymEntry->second.value;
                        if (!assembler.isAbsoluteSymbol(nextSymEntry->second))
                        {
                            expr->relativeSymOccurs = true;
                            args[argIndex].relValue.sectionId = 
                                nextSymEntry->second.sectionId;
                        }
                        else
                            args[argIndex].relValue.sectionId = ASMSECT_ABS;
                    }
                    else // if not defined
                    {
                        args[argIndex].symbol->second.addOccurrenceInExpr(
                                        expr, argIndex, opIndex);
                        expr->symOccursNum++;
                    }
                    
                    argIndex++;
                }
                else if (ops[opIndex]==AsmExprOp::ARG_VALUE)
                    argIndex++;
        }
        if (opIndex == opsSize)
        {
            // check if expression is evaluatable
            AsmSymbolEntry* thisSymEntry = se.releaseEntry();
            if (expr->symOccursNum == 0) // no symbols, we try to evaluate
            {
                // evaluate and remove obsolete expression
                if (!expr->evaluate(assembler, thisSymEntry->second.value,
                            thisSymEntry->second.sectionId))
                    good = false;
                thisSymEntry->second.hasValue = true;
                delete thisSymEntry->second.expression;
                thisSymEntry->second.expression = nullptr;
            }
            thisSymEntry->second.base = false;
            thisSymEntry->second.snapshot = true;
            stack.pop();
            if (!stack.empty())
            {
                // put to place in parent expression
                StackEntry& parentStackEntry = stack.top();
                AsmExpression* parentExpr = parentStackEntry.entry->second.expression;
                parentExpr->args[parentStackEntry.argIndex].symbol = thisSymEntry;
                parentExpr->ops[parentStackEntry.opIndex] = AsmExprOp::ARG_SYMBOL;
            }
            else
            {
                // last we return it
                outSymEntry = thisSymEntry;
                break;
            }
        }
    }
    return good;
}

// routine that call main makeSymbolSnapshot with error handling
bool AsmExpression::makeSymbolSnapshot(Assembler& assembler,
           const AsmSymbolEntry& symEntry, AsmSymbolEntry*& outSymEntry,
           const AsmSourcePos* parentExprSourcePos)
{
    TempSymbolSnapshotMap symbolSnapshots;
    bool good = true;
    try
    {
        good = makeSymbolSnapshot(assembler, &symbolSnapshots, symEntry, outSymEntry,
                    parentExprSourcePos);
        if (good)
            for (AsmSymbolEntry* symEntry: symbolSnapshots)
                // delete evaluated symbol entries (except output symbol entry)
                if (outSymEntry != symEntry && symEntry->second.hasValue)
                {
                    delete symEntry->second.expression;
                    symEntry->second.expression = nullptr;
                    delete symEntry;
                }
                else
                    assembler.symbolSnapshots.insert(symEntry);
        else // if failed
            for (AsmSymbolEntry* symEntry: symbolSnapshots)
            {
                assembler.symbolSnapshots.erase(symEntry);
                delete symEntry->second.expression;
                symEntry->second.expression = nullptr;
                delete symEntry;
            }
    }
    catch(...)
    {
        for (AsmSymbolEntry* symEntry: symbolSnapshots)
        {
            assembler.symbolSnapshots.erase(symEntry);
            delete symEntry->second.expression;
            symEntry->second.expression = nullptr;
            delete symEntry;
        }
        throw;
    }   
    return good;
}

AsmExpression* AsmExpression::createExprToEvaluate(Assembler& assembler) const
{
    TempSymbolSnapshotMap symbolSnapshots;
    
    try
    {
    size_t argsNum = 0;
    size_t msgPosNum = 0;
    for (AsmExprOp op: ops)
        if (AsmExpression::isArg(op))
            argsNum++;
    else if (operatorWithMessage & (1ULL<<int(op)))
        msgPosNum++;
    std::unique_ptr<AsmExpression> newExpr(new AsmExpression(
            sourcePos, symOccursNum, relativeSymOccurs, ops.size(), ops.data(),
            msgPosNum, messagePositions.get(), argsNum, args.get(), false));
    argsNum = 0;
    bool good = true;
    // try to resolve symbols
    for (AsmExprOp& op: newExpr->ops)
        if (AsmExpression::isArg(op))
        {
            if (op == AsmExprOp::ARG_SYMBOL) // if
            {
                AsmExprArg& arg = newExpr->args[argsNum];
                AsmSymbolEntry* symEntry = arg.symbol;
                if (symEntry!=nullptr && symEntry->second.base)
                    // create symbol snapshot if symbol is base
                    // only if for regular expression (not base
                    good &= newExpr->makeSymbolSnapshot(assembler, &symbolSnapshots,
                                *symEntry, symEntry, &sourcePos);
                if (symEntry==nullptr || !symEntry->second.hasValue)
                {
                    // no symbol not found
                    std::string errorMsg("Expression have unresolved symbol '");
                    errorMsg += symEntry->first.c_str();
                    errorMsg += '\'';
                    ASMX_NOTGOOD_BY_ERROR(sourcePos, errorMsg.c_str())
                }
                else
                {
                    if (symEntry->second.hasValue)
                    {
                        // resolve only if have symbol have value,
                        // but do not that if expression will be base for snapshot
                        if (!assembler.isAbsoluteSymbol(symEntry->second))
                        {
                            newExpr->relativeSymOccurs = true;
                            arg.relValue.sectionId = symEntry->second.sectionId;
                        }
                        arg.relValue.value = symEntry->second.value;
                        newExpr->symOccursNum--;
                        op = AsmExprOp::ARG_VALUE;
                    }
                }
            }
            argsNum++;
        }
    
    if (!good)
        return nullptr;
    // add expression into symbol occurrences in expressions
    for (AsmSymbolEntry* symEntry: symbolSnapshots)
    {
        if (!symEntry->second.hasValue)
            assembler.symbolSnapshots.insert(symEntry);
        else
        {
            // delete symbol snapshoft if have new value
            delete symEntry->second.expression;
            symEntry->second.expression = nullptr;
            delete symEntry;
        }
    }
    symbolSnapshots.clear();
    return newExpr.release();
    }
    catch(...)
    {
        // delete symbol snapshots if exception occurred
        for (AsmSymbolEntry* symEntry: symbolSnapshots)
        {
            // remove from assembler symbolSnapshots
            assembler.symbolSnapshots.erase(symEntry);
            // remove this snapshot
            delete symEntry->second.expression;
            symEntry->second.expression = nullptr;
            delete symEntry;
        }
        throw;
    }
}

AsmExpression* AsmExpression::parse(Assembler& assembler, const char*& linePtr,
            bool makeBase, bool dontResolveSymbolsLater)
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
    
    TempSymbolSnapshotMap symbolSnapshots;
    
    try
    {
    const char* startString = linePtr;
    const char* end = assembler.line + assembler.lineSize;
    size_t parenthesisCount = 0;
    size_t symOccursNum = 0;
    bool good = true;
    bool relativeSymOccurs = false;
    
    // indicate what is expected at current place
    enum ExpectedToken
    {
        XT_FIRST = 0,   // operator or argument
        XT_OP = 1,  // expected operator
        XT_ARG = 2  // expected argument
    };
    ExpectedToken expectedToken = XT_FIRST;
    std::unique_ptr<AsmExpression> expr(new AsmExpression);
    expr->sourcePos = assembler.getSourcePos(startString);
    
    while (linePtr != end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end) break;
        
        LineCol lineCol = { 0, 0 };
        AsmExprOp op = AsmExprOp::NONE;
        bool expectedPrimaryExpr = false;
        //const size_t oldParenthesisCount = parenthesisCount;
        bool doExit = false;
        
        const char* beforeToken = linePtr;
        /* parse expression operator or value/symbol */
        switch(*linePtr)
        {
            case '(':
                if (expectedToken == XT_OP)
                    ASMX_NOTGOOD_BY_ERROR(linePtr, "Expected operator")
                else
                {
                    expectedToken = XT_FIRST;
                    parenthesisCount++;
                }
                linePtr++;
                break;
            case ')':
                if (expectedToken != XT_OP)
                    ASMX_NOTGOOD_BY_ERROR(linePtr, "Expected operator or value or symbol")
                else
                {
                    if (parenthesisCount==0)
                    { doExit = true; break; }
                    parenthesisCount--;
                }
                linePtr++;
                break;
            case '+':
                op = (expectedToken == XT_OP) ? AsmExprOp::ADDITION : AsmExprOp::PLUS;
                linePtr++;
                break;
            case '-':
                op = (expectedToken == XT_OP) ? AsmExprOp::SUBTRACT : AsmExprOp::NEGATE;
                linePtr++;
                break;
            case '*':
                op = AsmExprOp::MULTIPLY;
                linePtr++;
                break;
            case '/':
                lineCol = assembler.translatePos(linePtr);
                if (linePtr+1 != end && linePtr[1] == '/')
                {
                    op = AsmExprOp::DIVISION;
                    linePtr++;
                }
                else // standard GNU as signed division
                    op = AsmExprOp::SIGNED_DIVISION;
                linePtr++;
                break;
            case '%':
                lineCol = assembler.translatePos(linePtr);
                if (linePtr+1 != end && linePtr[1] == '%')
                {
                    op = AsmExprOp::MODULO;
                    linePtr++;
                }
                else // standard GNU as signed modulo
                    op = AsmExprOp::SIGNED_MODULO;
                linePtr++;
                break;
            case '&':
                if (linePtr+1 != end && linePtr[1] == '&')
                {
                    op = AsmExprOp::LOGICAL_AND;
                    linePtr++;
                }
                else // standard bitwise AND
                    op = AsmExprOp::BIT_AND;
                linePtr++;
                break;
            case '|':
                if (linePtr+1 != end && linePtr[1] == '|')
                {
                    op = AsmExprOp::LOGICAL_OR;
                    linePtr++;
                }
                else // standard bitwise OR
                    op = AsmExprOp::BIT_OR;
                linePtr++;
                break;
            case '!':
                if (expectedToken == XT_OP) // ORNOT or logical not
                {
                    if (linePtr+1 != end && linePtr[1] == '=')
                    {
                        // we have != (inequality operator)
                        op = AsmExprOp::NOT_EQUAL;
                        linePtr++;
                    }
                    else
                        op = AsmExprOp::BIT_ORNOT;
                }
                else
                    op = AsmExprOp::LOGICAL_NOT;
                linePtr++;
                break;
            case '~':
                if (expectedToken != XT_OP)
                    op = AsmExprOp::BIT_NOT;
                else
                    ASMX_NOTGOOD_BY_ERROR(linePtr,
                        "Expected non-unary operator, '(', or end of expression")
                linePtr++;
                break;
            case '^':
                op = AsmExprOp::BIT_XOR;
                linePtr++;
                break;
            case '<':
                if (linePtr+1 != end && linePtr[1] == '<')
                {
                    lineCol = assembler.translatePos(linePtr);
                    op = AsmExprOp::SHIFT_LEFT;
                    linePtr++;
                }
                else if (linePtr+1 != end && linePtr[1] == '>')
                {
                    op = AsmExprOp::NOT_EQUAL;
                    linePtr++;
                }
                else if (linePtr+1 != end && linePtr[1] == '=')
                {
                    // less or equal than (signed and unsigned)
                    if (linePtr+2 != end && linePtr[2] == '@')
                    {
                        op = AsmExprOp::BELOW_EQ;
                        linePtr++;
                    }
                    else
                        op = AsmExprOp::LESS_EQ;
                    linePtr++;
                }
                // less than (signed and unsigned)
                else if (linePtr+1 != end && linePtr[1] == '@')
                {
                    op = AsmExprOp::BELOW;
                    linePtr++;
                }
                else
                    op = AsmExprOp::LESS;
                linePtr++;
                break;
            case '>':
                if (linePtr+1 != end && linePtr[1] == '>')
                {
                    lineCol = assembler.translatePos(linePtr);
                    if (linePtr+2 != end && linePtr[2] == '>')
                    {
                        op = AsmExprOp::SIGNED_SHIFT_RIGHT;
                        linePtr++;
                    }
                    else
                        op = AsmExprOp::SHIFT_RIGHT;
                    linePtr++;
                }
                else if (linePtr+1 != end && linePtr[1] == '=')
                {
                    // greater or equal than (signed and unsigned)
                    if (linePtr+2 != end && linePtr[2] == '@')
                    {
                        op = AsmExprOp::ABOVE_EQ;
                        linePtr++;
                    }
                    else
                        op = AsmExprOp::GREATER_EQ;
                    linePtr++;
                }
                // greater than (signed and unsigned)
                else if (linePtr+1 != end && linePtr[1] == '@')
                {
                    op = AsmExprOp::ABOVE;
                    linePtr++;
                }
                else
                    op = AsmExprOp::GREATER;
                linePtr++;
                break;
            case '=':
                if (linePtr+1 != end && linePtr[1] == '=')
                {
                    op = AsmExprOp::EQUAL;
                    linePtr++;
                }
                else
                    expectedPrimaryExpr = true;
                linePtr++;
                break;
            case '?':
                lineCol = assembler.translatePos(linePtr);
                op = AsmExprOp::CHOICE_START;
                linePtr++;
                break;
            case ':':
                if (linePtr+1==end || linePtr[1]!=':')
                {
                    op = AsmExprOp::CHOICE;
                    linePtr++;
                    break;
                }
                // otherwise try parse symbol
            default: // parse symbol or value
                if (expectedToken != XT_OP)
                {
                    ExpectedToken oldExpectedToken = expectedToken;
                    expectedToken = XT_OP;
                    AsmSymbolEntry* symEntry;
                    
                    const char* symEndStr = linePtr;
                    Assembler::ParseState parseState = assembler.parseSymbol(symEndStr,
                                     symEntry, true, dontResolveSymbolsLater);
                    if (symEntry!=nullptr && symEntry->second.regRange)
                    {
                        ASMX_NOTGOOD_BY_ERROR(linePtr, "Expression have register symbol")
                        continue;
                    }
                    
                    if (parseState == Assembler::ParseState::FAILED) good = false;
                    AsmExprArg arg;
                    arg.relValue.sectionId = ASMSECT_ABS;
                    if (parseState != Assembler::ParseState::MISSING)
                    {
                        if (symEntry!=nullptr && symEntry->second.base && !makeBase)
                            // create symbol snapshot if symbol is base
                            // only if for regular expression (not base
                            good &= makeSymbolSnapshot(assembler, &symbolSnapshots,
                                      *symEntry, symEntry, &(expr->sourcePos));
                        if (symEntry==nullptr ||
                            (!symEntry->second.hasValue && dontResolveSymbolsLater))
                        {
                            // no symbol not found
                            std::string errorMsg("Expression have unresolved symbol '");
                            errorMsg.append(linePtr, symEndStr);
                            errorMsg += '\'';
                            ASMX_NOTGOOD_BY_ERROR(linePtr, errorMsg.c_str())
                        }
                        else
                        {
                            if (symEntry->second.hasValue && !makeBase)
                            {
                                // resolve only if have symbol have value,
                                // but do not that if expression will be base for snapshot
                                if (!assembler.isAbsoluteSymbol(symEntry->second))
                                {
                                    relativeSymOccurs = true;
                                    arg.relValue.sectionId = symEntry->second.sectionId;
                                }
                                arg.relValue.value = symEntry->second.value;
                                args.push_back(arg);
                                ops.push_back(AsmExprOp::ARG_VALUE);
                            }
                            else
                            {
                                /* add symbol */
                                symOccursNum++;
                                arg.symbol = symEntry;
                                args.push_back(arg);
                                ops.push_back(AsmExprOp::ARG_SYMBOL);
                            }
                        }
                        linePtr = symEndStr;
                    }
                    else if (parenthesisCount != 0 || (*linePtr >= '0' &&
                            *linePtr <= '9') || *linePtr == '\'')
                    {
                        // other we try to parse number
                        const char* oldStr = linePtr;
                        if (!assembler.parseLiteral(arg.value, linePtr))
                        {
                            arg.value = 0;
                            if (linePtr != end && oldStr == linePtr)
                                // skip one character when end is in this same place
                                // (avoids infinity loops)
                                linePtr++;
                            good = false;
                        }
                        args.push_back(arg);
                        ops.push_back(AsmExprOp::ARG_VALUE);
                    }
                    else
                    {
                        // otherwise we finish parsing
                        expectedToken = oldExpectedToken;
                        doExit = true; break;
                    }
                }
                else
                {
                    // otherwise we exit if no left parenthesis
                    if (parenthesisCount == 0)
                    { doExit = true; break; }
                    else
                    {
                        linePtr++;
                        ASMX_NOTGOOD_BY_ERROR(linePtr, "Garbages at end of expression")
                    }
                }
        }
        
        if (op != AsmExprOp::NONE && !isUnaryOp(op) && expectedToken != XT_OP)
        {
            expectedPrimaryExpr = true;
            op = AsmExprOp::NONE;
        }
        // when no argument before binary/ternary operator
        if (expectedPrimaryExpr)
        {
            ASMX_NOTGOOD_BY_ERROR(beforeToken,
                     "Expected primary expression before operator")
            continue;
        }
        
        // determine what is next expected
        if (op != AsmExprOp::NONE && !isUnaryOp(op))
            expectedToken = (expectedToken == XT_OP) ? XT_ARG : XT_OP;
        
        //afterParenthesis = (oldParenthesisCount < parenthesisCount);
        const size_t lineColPos = (lineCol.lineNo!=0) ? messagePositions.size() : SIZE_MAX;
        if (lineCol.lineNo!=0)
            messagePositions.push_back(lineCol);
        
        if (op != AsmExprOp::NONE)
        {
            // if operator
            const bool unaryOp = isUnaryOp(op);
            const cxuint priority = (parenthesisCount<<3) +
                        asmOpPrioritiesTbl[cxuint(op)];
            
            if (op == AsmExprOp::CHOICE)
            {
                /* second part of choice */
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
                if (stack.empty())
                {
                    linePtr--; // go back to ':'
                    expectedToken = XT_OP;
                    doExit = true;
                    break;
                }
                else if (stack.top().op != AsmExprOp::CHOICE_START ||
                        stack.top().priority != priority)
                {
                    // not found
                    ASMX_NOTGOOD_BY_ERROR(beforeToken, "Missing '?' before ':'")
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
                    {
                        // unfinished choice
                        stack.pop();
                        ASMX_NOTGOOD_BY_ERROR(messagePositions[entry.lineColPos],
                                 "Missing ':' for '?'")
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
    if (parenthesisCount != 0) // print error
        ASMX_NOTGOOD_BY_ERROR(linePtr, "Missing ')'")
    if (expectedToken != XT_OP)
    {
        if (!ops.empty() || !stack.empty())
            ASMX_NOTGOOD_BY_ERROR(linePtr, "Unterminated expression")
    }
    else
    {
        while (!stack.empty())
        {
            const ConExprOpEntry& entry = stack.top();
            if (entry.op == AsmExprOp::CHOICE_START)
            {
                // unfinished choice
                ASMX_NOTGOOD_BY_ERROR(messagePositions[entry.lineColPos],
                         "Missing ':' for '?'")
                break;
            }
            if (entry.op != AsmExprOp::PLUS)
                ops.push_back(entry.op);
            if (entry.lineColPos != SIZE_MAX && entry.op != AsmExprOp::CHOICE_START)
                outMsgPositions.push_back(messagePositions[entry.lineColPos]);
            stack.pop();
        }
    }
    
    if (good)
    {
        const size_t argsNum = args.size();
        // if good, we set symbol occurrences, operators, arguments ...
        expr->setParams(symOccursNum, relativeSymOccurs,
                  ops.size(), ops.data(), outMsgPositions.size(), outMsgPositions.data(),
                  argsNum, args.data(), makeBase);
        if (!makeBase)
        {
            // add expression into symbol occurrences in expressions
            // only for non-base expressions
            for (size_t i = 0, j = 0; j < argsNum; i++)
                if (ops[i] == AsmExprOp::ARG_SYMBOL)
                {
                    args[j].symbol->second.addOccurrenceInExpr(expr.get(), j, i);
                    j++;
                }
                else if (ops[i]==AsmExprOp::ARG_VALUE)
                    j++;
        }
        for (AsmSymbolEntry* symEntry: symbolSnapshots)
        {
            if (!symEntry->second.hasValue)
                assembler.symbolSnapshots.insert(symEntry);
            else
            {
                // delete symbol snapshoft if have new value
                delete symEntry->second.expression;
                symEntry->second.expression = nullptr;
                delete symEntry;
            }
        }
        symbolSnapshots.clear();
        return expr.release();
    }
    else
    {
        // delete symbol snapshots if error
        for (AsmSymbolEntry* symEntry: symbolSnapshots)
        {
            delete symEntry->second.expression;
            symEntry->second.expression = nullptr;
            delete symEntry;
        }
        return nullptr;
    }
    }
    catch(...)
    {
        // delete symbol snapshots if exception occurred
        for (AsmSymbolEntry* symEntry: symbolSnapshots)
        {
            // remove from assembler symbolSnapshots
            assembler.symbolSnapshots.erase(symEntry);
            // remove this snapshot
            delete symEntry->second.expression;
            symEntry->second.expression = nullptr;
            delete symEntry;
        }
        throw;
    }
}

// go to top in expression from specified operator index (can be used to divide expression)
size_t AsmExpression::toTop(size_t opIndex) const
{
    std::stack<cxbyte> stack;
    while (true)
    {
        AsmExprOp op = ops[opIndex];
        if (op==AsmExprOp::ARG_VALUE)
        {
            while (!stack.empty() && --stack.top()==0)
                stack.pop();
            if (stack.empty())
                break; // is end
        }
        else // operators
            stack.push(isUnaryOp(op) ? 1 : (isBinaryOp(op) ? 2 : 3));
        opIndex--;
    }
    return opIndex;
}

static const uint32_t acceptedCharsAfterFastExpr =
    (1U<<('!'-32)) | (1U<<('%'-32)) | (1U<<('&'-32)) | (1U<<('*'-32)) | (1U<<('!'-32)) |
        (1U<<('+'-32)) | (1U<<('-'-32)) | (1U<<('/'-32)) | (1U<<('<'-32)) |
        (1U<<('='-32)) | (1U<<('>'-32)) | (1U<<('?'-32));

bool AsmExpression::fastExprEvaluate(Assembler& assembler, const char*& linePtr,
                        uint64_t& value)
{
    const char* end = assembler.line + assembler.lineSize;
    uint64_t sum = 0;
    bool addition = true;
    const char* tmpLinePtr = linePtr;
    skipSpacesToEnd(tmpLinePtr, end);
    
    // main loop
    while (true)
    {
        // loop for chain of unary '+' and '-'
        while (tmpLinePtr != end && (*tmpLinePtr=='+' || *tmpLinePtr=='-'))
        {
            if (*tmpLinePtr=='-')
                addition = !addition;
            skipCharAndSpacesToEnd(tmpLinePtr, end);
        }
        uint64_t tmp = 0;
        if (tmpLinePtr == end || (!isDigit(*tmpLinePtr) && *tmpLinePtr!='\''))
            return false;
        if (!assembler.parseLiteralNoError(tmp, tmpLinePtr))
            return false;
        if (tmpLinePtr!=end && (*tmpLinePtr=='f' || *tmpLinePtr=='b'))
        {   // if local label
            const char* t = tmpLinePtr-1;
            while (t!=linePtr-1 && isDigit(*t)) t--;
            if (t==linePtr-1 || isSpace(*t)) // if local label
                return false;
        }
        // add or subtract
        sum += addition ? tmp : -tmp;
        // skip to next '+' or '-'
        skipSpacesToEnd(tmpLinePtr, end);
        if (tmpLinePtr==end || (*tmpLinePtr!='+' && *tmpLinePtr!='-'))
            break; // end
        // otherwise we continue
        addition = (*tmpLinePtr=='+');
        skipCharAndSpacesToEnd(tmpLinePtr, end);
    }
    if (tmpLinePtr==end ||
        // check whether is not other operator
        (*tmpLinePtr!='~' && *tmpLinePtr!='^' && *tmpLinePtr!='|' &&
        (*tmpLinePtr<32 || *tmpLinePtr>=64 ||
        // check whether is not other operator (by checking rest of characters)
        ((1U<<(*tmpLinePtr-32)) & acceptedCharsAfterFastExpr)==0)))
    {
        value = sum;
        linePtr = tmpLinePtr;
        return true;
    }
    return false;
}

bool AsmExpression::fastExprEvaluate(Assembler& assembler, size_t& linePos,
                        uint64_t& value)
{
    const char* linePtr = assembler.line + linePos;
    bool res = fastExprEvaluate(assembler, linePtr, value);
    linePos = linePtr - assembler.line;
    return res;
}
