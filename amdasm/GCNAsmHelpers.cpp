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
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "GCNAsmInternals.h"

using namespace CLRX;

cxbyte CLRX::cstrtobyte(const char*& str, const char* end)
{
    uint16_t value = 0;
    if (str==end || !isDigit(*str))
        throw ParseException("Missing number");
    for (;str!=end && isDigit(*str); str++)
    {
        value = value*10 + *str-'0';
        if (value >= 256)
            throw ParseException("Number is too big");
    }
    return value;
}

namespace CLRX
{

bool GCNAsmUtils::printRegisterRangeExpected(Assembler& asmr, const char* linePtr,
               const char* regPoolName, cxuint regsNum, bool required)
{
    if (!required)
        return false;
    char buf[60];
    if (regsNum!=0)
        snprintf(buf, 50, "Expected %u %s register%s", regsNum, regPoolName,
                 (regsNum==1)?"":"s");
    else
        snprintf(buf, 50, "Expected %s registers", regPoolName);
    asmr.printError(linePtr, buf);
    return true;
}

void GCNAsmUtils::printXRegistersRequired(Assembler& asmr, const char* linePtr,
              const char* regPoolName, cxuint regsNum)
{
    char buf[60];
    snprintf(buf, 60, "Required %u %s register%s", regsNum, regPoolName,
             (regsNum==1)?"":"s");
    asmr.printError(linePtr, buf);
}

bool GCNAsmUtils::parseVRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                    cxuint regsNum, bool required)
{
    const char* oldLinePtr = linePtr;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* vgprRangePlace = linePtr;
    if (linePtr == end || toLower(*linePtr) != 'v' || linePtr+1 == end ||
        (!isDigit(linePtr[1]) && linePtr[1]!='['))
    {
        if (linePtr!=end)
        {   // check whether is name of symbol with register
            AsmSymbolEntry* symEntry = nullptr;
            if (*linePtr=='@')
                skipCharAndSpacesToEnd(linePtr, end);
            if (asmr.parseSymbol(linePtr, symEntry, false, true)==
                Assembler::ParseState::PARSED && symEntry!=nullptr &&
                symEntry->second.regRange)
            { // set up regrange
                cxuint rstart = symEntry->second.value&UINT_MAX;
                cxuint rend = symEntry->second.value>>32;
                if (rstart >= 256 && rend >= 256)
                {
                    if (regsNum!=0 && regsNum != rend-rstart)
                    {
                        printXRegistersRequired(asmr, vgprRangePlace, "vector", regsNum);
                        return false;
                    }
                    regPair = { rstart, rend };
                    return true;
                }
            }
        }
        if (printRegisterRangeExpected(asmr, vgprRangePlace, "vector", regsNum, required))
            return false;
        regPair = { 0, 0 }; // no range
        linePtr = oldLinePtr; // revert current line pointer
        return true;
    }
    linePtr++;
    
    try /* for handling parse exception */
    {
    if (isDigit(*linePtr))
    {   // if single register
        cxuint value = cstrtobyte(linePtr, end);
        if (regsNum!=0 && regsNum != 1)
        {
            printXRegistersRequired(asmr, vgprRangePlace, "vector", regsNum);
            return false;
        }
        regPair = { 256+value, 256+value+1 };
        return true;
    }
    else if (*linePtr == '[')
    {   // many registers
        ++linePtr;
        uint64_t value1, value2;
        skipSpacesToEnd(linePtr, end);
        if (!getAbsoluteValueArg(asmr, value1, linePtr, true))
            return false;
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || (*linePtr!=':' && *linePtr!=']'))
        {   // error
            asmr.printError(vgprRangePlace, "Unterminated vector register range");
            return false;
        }
        if (linePtr!=end && *linePtr==':')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            if (!getAbsoluteValueArg(asmr, value2, linePtr, true))
                return false;
        }
        else
            value2 = value1;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
        {   // error
            asmr.printError(vgprRangePlace, "Unterminated vector register range");
            return false;
        }
        ++linePtr;
        
        if (value2 < value1)
        {   // error (illegal register range)
            asmr.printError(vgprRangePlace, "Illegal vector register range");
            return false;
        }
        if (value1 >= 256 || value2 >= 256)
        {
            asmr.printError(vgprRangePlace, "Some vector register number out of range");
            return false;
        }
        
        if (regsNum!=0 && regsNum != value2-value1+1)
        {
            printXRegistersRequired(asmr, vgprRangePlace, "vector", regsNum);
            return false;
        }
        regPair = { 256+value1, 256+value2+1 };
        return true;
    }
    } catch(const ParseException& ex)
    {
        asmr.printError(linePtr, ex.what());
        return false;
    }
    
    if (printRegisterRangeExpected(asmr, vgprRangePlace, "vector", regsNum, required))
        return false;
    regPair = { 0, 0 }; // no range
    linePtr = oldLinePtr; // revert current line pointer
    return true;
}

bool GCNAsmUtils::parseSRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                    uint16_t arch, cxuint regsNum, bool required)
{
    const char* oldLinePtr = linePtr;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* sgprRangePlace = linePtr;
    if (linePtr == end)
    {
        if (printRegisterRangeExpected(asmr, sgprRangePlace, "scalar", regsNum, required))
            return false;
        regPair = { 0, 0 };
        linePtr = oldLinePtr; // revert current line pointer
        return true;
    }
    
    bool ttmpReg = false;
    try
    {
    if (linePtr+4 < end && toLower(linePtr[0]) == 't' &&
        toLower(linePtr[1]) == 't' && toLower(linePtr[2]) == 'm' &&
        toLower(linePtr[3]) == 'p' && (isDigit(linePtr[4]) || linePtr[4]=='['))
        ttmpReg = true; // we have ttmp registers
    else if (toLower(*linePtr) != 's' || linePtr+1==end ||
        (!isDigit(linePtr[1]) && linePtr[1]!='[')) // if not sgprs
    {
        const char* oldLinePtr = linePtr;
        char regName[20];
        if (linePtr==end || *linePtr != '@')
        {   // if not '@'
            if (!getNameArg(asmr, 20, regName, linePtr, "register name", required, true))
                return false;
        }
        else // otherwise reset regName
            regName[0] = 0;
        toLowerString(regName);
        
        size_t loHiRegSuffix = 0;
        cxuint loHiReg = 0;
        if (regName[0] == 'v' && regName[1] == 'c' && regName[2] == 'c')
        {   /// vcc
            loHiRegSuffix = 3;
            loHiReg = 106;
        }
        else if (::memcmp(regName, "exec", 4)==0)
        {   /* exec* */
            loHiRegSuffix = 4;
            loHiReg = 126;
        }
        else if (regName[0]=='t')
        {   /* tma,tba */
            if (regName[1] == 'b' && regName[2] == 'a')
            {
                loHiRegSuffix = 3;
                loHiReg = 108;
            }
            else if (regName[1] == 'm' && regName[2] == 'a')
            {
                loHiRegSuffix = 3;
                loHiReg = 110;
            }
        }
        else if (regName[0] == 'm' && regName[1] == '0' && regName[2] == 0)
        {   /* M0 */
            if (regsNum!=0 && regsNum!=1 && regsNum!=2)
            {
                printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
                return false;
            }
            regPair = { 124, 125 };
            return true;
        }
        else if (arch&ARCH_GCN_1_1_2)
        {
            if (::memcmp(regName, "flat_scratch", 12)==0)
            {   // flat
                loHiRegSuffix = 12;
                loHiReg = (arch&ARCH_RX3X0)?102:104;
            }
            else if ((arch&ARCH_RX3X0)!=0 && ::memcmp(regName, "xnack_mask", 10)==0)
            {   // xnack
                loHiRegSuffix = 10;
                loHiReg = 104;
            }
        }
        
        if (loHiRegSuffix != 0) // handle 64-bit registers
        {
            if (regName[loHiRegSuffix] == '_')
            {
                if (regName[loHiRegSuffix+1] == 'l' && regName[loHiRegSuffix+2] == 'o' &&
                    regName[loHiRegSuffix+3] == 0)
                    regPair = { loHiReg, loHiReg+1 };
                else if (regName[loHiRegSuffix+1] == 'h' &&
                    regName[loHiRegSuffix+2] == 'i' && regName[loHiRegSuffix+3] == 0)
                    regPair = { loHiReg+1, loHiReg+2 };
                if (regsNum!=0 && regsNum != 1)
                {
                    printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
                    return false;
                }
                return true;
            }
            else if (regName[loHiRegSuffix] == 0)
            {
                if (regsNum!=0 && regsNum != 2)
                {
                    printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
                    return false;
                }
                regPair = { loHiReg, loHiReg+2 };
                return true;
            }
            else
            {   // this is not this register
                if (printRegisterRangeExpected(asmr, sgprRangePlace, "scalar",
                            regsNum, required))
                    return false;
                linePtr = oldLinePtr;
                regPair = { 0, 0 };
                return true;
            }
        }
        else
        {   // otherwise
            // check whether is name of symbol with register
            AsmSymbolEntry* symEntry = nullptr;
            linePtr = oldLinePtr;
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr=='@')
                skipCharAndSpacesToEnd(linePtr, end);
            if (asmr.parseSymbol(linePtr, symEntry, false, true)==
                Assembler::ParseState::PARSED && symEntry!=nullptr &&
                symEntry->second.regRange)
            { // set up regrange
                cxuint rstart = symEntry->second.value&UINT_MAX;
                cxuint rend = symEntry->second.value>>32;
                if (rstart < 256 && rend <= 256)
                {
                    if (regsNum!=0 && regsNum != rend-rstart)
                    {
                        printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
                        return false;
                    }
                    regPair = { rstart, rend };
                    return true;
                }
            }
            if (printRegisterRangeExpected(asmr, sgprRangePlace, "scalar",
                            regsNum, required))
                return false;
            regPair = { 0, 0 };
            linePtr = oldLinePtr; // revert current line pointer
            return true;
        }
    }
    linePtr += (ttmpReg)?4:1; // skip
    if (linePtr == end)
    {
        if (printRegisterRangeExpected(asmr, sgprRangePlace, "scalar", regsNum, required))
            return false;
        regPair = { 0, 0 };
        linePtr = oldLinePtr; // revert current line pointer
        return true;
    }
    
    const cxuint maxSGPRsNum = (arch&ARCH_RX3X0) ? 102 : 104;
    if (isDigit(*linePtr))
    {   // if single register
        cxuint value = cstrtobyte(linePtr, end);
        if (!ttmpReg)
        {
            if (value >= maxSGPRsNum)
            {
                asmr.printError(sgprRangePlace, "Scalar register number out of range");
                return false;
            }
        }
        else
        {
            if (value >= 12)
            {
                asmr.printError(sgprRangePlace, "TTMPRegister number out of range (0-11)");
                return false;
            }
        }
        if (regsNum!=0 && regsNum!=1)
        {
            printXRegistersRequired(asmr, linePtr, "scalar", regsNum);
            return false;
        }
        if (!ttmpReg)
            regPair = { value, value+1 };
        else
            regPair = { 112+value, 112+value+1 };
        return true;
    }
    else if (*linePtr == '[')
    {   // many registers
        ++linePtr;
        uint64_t value1, value2;
        skipSpacesToEnd(linePtr, end);
        if (!getAbsoluteValueArg(asmr, value1, linePtr, true))
            return false;
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || (*linePtr!=':' && *linePtr!=']'))
        {   // error
            asmr.printError(sgprRangePlace, (!ttmpReg) ?
                        "Unterminated scalar register range" :
                        "Unterminated TTMPRegister range");
            return false;
        }
        if (linePtr!=end && *linePtr==':')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            if (!getAbsoluteValueArg(asmr, value2, linePtr, true))
                return false;
        }
        else
            value2 = value1;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
        {   // error
            asmr.printError(sgprRangePlace, (!ttmpReg) ?
                        "Unterminated scalar register range" :
                        "Unterminated TTMPRegister range");
            return false;
        }
        ++linePtr;
        
        if (!ttmpReg)
        {
            if (value2 < value1)
            {   // error (illegal register range)
                asmr.printError(sgprRangePlace, "Illegal scalar register range");
                return false;
            }
            if (value1 >= maxSGPRsNum || value2 >= maxSGPRsNum)
            {
                asmr.printError(sgprRangePlace,
                            "Some scalar register number out of range");
                return false;
            }
        }
        else
        {
            if (value2 < value1)
            {   // error (illegal register range)
                asmr.printError(sgprRangePlace, "Illegal TTMPRegister range");
                return false;
            }
            if (value1 >= 12 || value2 >= 12)
            {
                asmr.printError(sgprRangePlace,
                                "Some TTMPRegister number out of range (0-11)");
                return false;
            }
        }
        
        if (regsNum!=0 && regsNum != value2-value1+1)
        {
            printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
            return false;
        }
        /// check alignment
        if (!ttmpReg)
            if ((value2-value1==1 && (value1&1)!=0) || (value2-value1>1 && (value1&3)!=0))
            {
                asmr.printError(sgprRangePlace, "Unaligned scalar register range");
                return false;
            }
        if (!ttmpReg)
            regPair = { value1, uint16_t(value2)+1 };
        else
            regPair = { 112+value1, 112+uint16_t(value2)+1 };
        return true;
    }
    } catch(const ParseException& ex)
    {
        asmr.printError(linePtr, ex.what());
        return false;
    }
    
    if (printRegisterRangeExpected(asmr, sgprRangePlace, "scalar", regsNum, required))
        return false;
    regPair = { 0, 0 };
    linePtr = oldLinePtr; // revert current line pointer
    return true;
}

bool GCNAsmUtils::parseImmInt(Assembler& asmr, const char*& linePtr, uint32_t& outValue,
            std::unique_ptr<AsmExpression>* outTargetExpr, cxuint bits, cxbyte signess)
{
    const char* end = asmr.line+asmr.lineSize;
    if (outTargetExpr!=nullptr)
        outTargetExpr->reset();
    skipSpacesToEnd(linePtr, end);
    const char* exprPlace = linePtr;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr));
    if (expr==nullptr) // error
        return false;
    if (expr->isEmpty())
    {
        asmr.printError(exprPlace, "Expected expression");
        return false;
    }
    if (expr->getSymOccursNum()==0)
    {   // resolved now
        cxuint sectionId; // for getting
        uint64_t value;
        if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
            return false;
        else if (sectionId != ASMSECT_ABS)
        {   // if not absolute value
            asmr.printError(exprPlace, "Expression must be absolute!");
            return false;
        }
        asmr.printWarningForRange(bits, value, asmr.getSourcePos(exprPlace), signess);
        outValue = value & ((1ULL<<bits)-1ULL);
        return true;
    }
    else
    {   // return output expression with symbols to resolve
        if (outTargetExpr!=nullptr)
            *outTargetExpr = std::move(expr);
        else
        {
            asmr.printError(exprPlace, "Unresolved expression is illegal in this place");
            return false;
        }
        return true;
    }
}

/* check whether string is exclusively floating point value
 * (only floating point, and neither integer and nor symbol) */
static bool isOnlyFloat(const char* str, const char* end)
{
    if (str == end)
        return false;
    if (*str=='-' || *str=='+')
        str++; // skip '-' or '+'
    if (str+2 < end && *str=='0' && (str[1]=='X' || str[1]=='x'))
    {   // hexadecimal
        str += 2;
        const char* beforeComma = str;
        while (str!=end && isXDigit(*str)) str++;
        const char* point = str;
        if (str == end || *str!='.')
        {
            if (beforeComma-point!=0 && str!=end && (*str=='p' || *str=='P'))
            {
                str++;
                if (str!=end && (*str=='-' || *str=='+'))
                    str++;
                const char* expPlace = str;
                while (str!=end && isDigit(*str)) str++;
                if (str-expPlace!=0)
                    return true; // if 'XXXp[+|-]XXX'
            }
            return false; // no '.'
        }
        str++;
        while (str!=end && isXDigit(*str)) str++;
        const char* afterComma = str;
        
        if (point-beforeComma!=0 || afterComma-(point+1)!=0)
            return true;
    }
    else
    {   // decimal
        const char* beforeComma = str;
        while (str!=end && isDigit(*str)) str++;
        const char* point = str;
        if (str == end || *str!='.')
        {
            if (beforeComma-point!=0 && str!=end && (*str=='e' || *str=='E'))
            {
                str++;
                if (str!=end && (*str=='-' || *str=='+'))
                    str++;
                const char* expPlace = str;
                while (str!=end && isDigit(*str)) str++;
                if (str-expPlace!=0)
                    return true; // if 'XXXe[+|-]XXX'
            }
            return false; // no '.'
        }
        str++;
        while (str!=end && isDigit(*str)) str++;
        const char* afterComma = str;
        
        if (point-beforeComma!=0 || afterComma-(point+1)!=0)
            return true;
    }
    return false;
}

bool GCNAsmUtils::parseLiteralImm(Assembler& asmr, const char*& linePtr, uint32_t& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, Flags instropMask)
{
    if (outTargetExpr!=nullptr)
        outTargetExpr->reset();
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (isOnlyFloat(linePtr, end))
    {
        try
        {
        if ((instropMask&INSTROP_TYPE_MASK) == INSTROP_F16)
            value = cstrtohCStyle(linePtr, end, linePtr);
        else
        {
            union FloatUnion { uint32_t i; float f; };
            FloatUnion v;
            v.f = cstrtovCStyle<float>(linePtr, end, linePtr);
            value = v.i;
        }
        }
        catch(const ParseException& ex)
        {   // error
            asmr.printError(linePtr, ex.what());
            return false;
        }
        return true;
    }
    return parseImm(asmr, linePtr, value, outTargetExpr);
}

bool GCNAsmUtils::parseOperand(Assembler& asmr, const char*& linePtr, GCNOperand& operand,
             std::unique_ptr<AsmExpression>* outTargetExpr, uint16_t arch,
             cxuint regsNum, Flags instrOpMask)
{
    if (outTargetExpr!=nullptr)
        outTargetExpr->reset();
    
    if (instrOpMask == INSTROP_SREGS)
        return parseSRegRange(asmr, linePtr, operand.range, arch, regsNum);
    else if (instrOpMask == INSTROP_VREGS)
        return parseVRegRange(asmr, linePtr, operand.range, regsNum);
    
    const char* end = asmr.line+asmr.lineSize;
    if (instrOpMask & INSTROP_VOP3MODS)
    {
        operand.vopMods = 0;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr=='@') // treat this operand as expression
            return parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                             instrOpMask & ~INSTROP_VOP3MODS);
        
        if ((arch & ARCH_RX3X0) && linePtr+4 <= end && toLower(linePtr[0])=='s' &&
            toLower(linePtr[1])=='e' && toLower(linePtr[2])=='x' &&
            toLower(linePtr[3])=='t')
        {   /* sext */
            linePtr += 4;
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr=='(')
            {
                operand.vopMods |= VOPOP_SEXT;
                ++linePtr;
            }
            else
            {
                asmr.printError(linePtr, "Expected '(' after sext");
                return false;
            }
        }
        
        const char* negPlace = linePtr;
        if (linePtr!=end && *linePtr=='-')
        {
            operand.vopMods |= VOPOP_NEG;
            skipCharAndSpacesToEnd(linePtr, end);
        }
        if (linePtr+3 <= end && toLower(linePtr[0])=='a' &&
            toLower(linePtr[1])=='b' && toLower(linePtr[2])=='s')
        {
            linePtr += 3;
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr=='(')
            {
                operand.vopMods |= VOPOP_ABS;
                ++linePtr;
            }
            else
            {
                asmr.printError(linePtr, "Expected '(' after abs");
                return false;
            }
        }
        
        bool good;
        if ((operand.vopMods&(VOPOP_NEG|VOPOP_ABS)) != VOPOP_NEG)
            good = parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                                     instrOpMask & ~INSTROP_VOP3MODS);
        else //
        {
            linePtr = negPlace;
            good = parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                             (instrOpMask & ~INSTROP_VOP3MODS) | INSTROP_PARSEWITHNEG);
        }
        
        if (operand.vopMods & VOPOP_ABS)
        {
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr==')')
                linePtr++;
            else
            {
                asmr.printError(linePtr, "Unterminated abs() modifier");
                return false;
            }
        }
        if (operand.vopMods & VOPOP_SEXT)
        {
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr==')')
                linePtr++;
            else
            {
                asmr.printError(linePtr, "Unterminated sext() modifier");
                return false;
            }
        }
        return good;
    }
    skipSpacesToEnd(linePtr, end);
    const char* negPlace = linePtr;
    if (instrOpMask & INSTROP_VOP3NEG)
        operand.vopMods = 0; // clear modifier (VOP3NEG)
        /// PARSEWITHNEG used to continuing operand parsing with modifiers
    if (instrOpMask & (INSTROP_PARSEWITHNEG|INSTROP_VOP3NEG))
    {
        if (linePtr!=end && *linePtr=='-')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            operand.vopMods |= VOPOP_NEG;
        }
    }
    
    // otherwise
    if (instrOpMask & INSTROP_SREGS)
    {
        if (!parseSRegRange(asmr, linePtr, operand.range, arch, regsNum, false))
            return false;
        if (operand)
            return true;
    }
    if (instrOpMask & INSTROP_VREGS)
    {
        if (!parseVRegRange(asmr, linePtr, operand.range, regsNum, false))
            return false;
        if (operand)
            return true;
    }
    
    skipSpacesToEnd(linePtr, end);
    
    if ((instrOpMask & INSTROP_SSOURCE)!=0)
    {
        char regName[20];
        const char* regNamePlace = linePtr;
        if (getNameArg(asmr, 20, regName, linePtr, "register name", false, true))
        {
            toLowerString(regName);
            operand.range = {0, 0};
            if (::strcmp(regName, "vccz") == 0)
            {
                operand.range = { 251, 252 };
                return true;
            }
            else if (::strcmp(regName, "execz") == 0)
            {
                operand.range = { 252, 253 };
                return true;
            }
            else if (::strcmp(regName, "scc") == 0)
            {
                operand.range = { 253, 254 };
                return true;
            }
            else if ((instrOpMask&INSTROP_LDS)!=0 &&
                (::strcmp(regName, "lds")==0 || ::strcmp(regName, "lds_direct")==0 ||
                    ::strcmp(regName, "src_lds_direct")==0))
            {
                operand.range = { 254, 255 };
                return true;
            }
            if (operand)
            {
                if (regsNum!=0 && regsNum!=1 && regsNum!=2)
                {
                    printXRegistersRequired(asmr, regNamePlace, "scalar", regsNum);
                    return false;
                }
                return true;
            }
            /* check expression, back to before regName */
            linePtr = negPlace;
        }
        // treat argument as expression
        bool forceExpression = false;
        if (linePtr!=end && *linePtr=='@')
        {
            forceExpression = true;
            skipCharAndSpacesToEnd(linePtr, end);
        }
        if (linePtr==end || *linePtr==',')
        {
            asmr.printError(linePtr, "Expected instruction operand");
            return false;
        }
        const char* exprPlace = linePtr;
        
        uint64_t value;
        operand.vopMods = 0; // zeroing operand modifiers
        if (!forceExpression && isOnlyFloat(negPlace, end))
        {   // if only floating point value
            /* if floating point literal can be processed */
            linePtr = negPlace;
            try
            {
                if ((instrOpMask & INSTROP_TYPE_MASK) == INSTROP_F16)
                {
                    value = cstrtohCStyle(linePtr, end, linePtr);
                    switch (value)
                    {
                        case 0x0:
                            operand.range = { 128, 0 };
                            return true;
                        case 0x3800: // 0.5
                            operand.range  = { 240, 0 };
                            return true;
                        case 0xb800: // -0.5
                            operand.range = { 241, 0 };
                            return true;
                        case 0x3c00: // 1.0
                            operand.range = { 242, 0 };
                            return true;
                        case 0xbc00: // -1.0
                            operand.range = { 243, 0 };
                            return true;
                        case 0x4000: // 2.0
                            operand.range = { 244, 0 };
                            return true;
                        case 0xc000: // -2.0
                            operand.range = { 245, 0 };
                            return true;
                        case 0x4400: // 4.0
                            operand.range = { 246, 0 };
                            return true;
                        case 0xc400: // -4.0
                            operand.range = { 247, 0 };
                            return true;
                        case 0x3118: // 1/(2*PI)
                            if (arch&ARCH_RX3X0)
                            {
                                operand.range = { 248, 0 };
                                return true;
                            }
                    }
                }
                else /* otherwise, FLOAT */
                {
                    union FloatUnion { uint32_t i; float f; };
                    FloatUnion v;
                    v.f = cstrtovCStyle<float>(linePtr, end, linePtr);
                    value = v.i;
                    
                    switch (value)
                    {
                        case 0x0:
                            operand.range = { 128, 0 };
                            return true;
                        case 0x3f000000: // 0.5
                            operand.range = { 240, 0 };
                            return true;
                        case 0xbf000000: // -0.5
                            operand.range = { 241, 0 };
                            return true;
                        case 0x3f800000: // 1.0
                            operand.range = { 242, 0 };
                            return true;
                        case 0xbf800000: // -1.0
                            operand.range = { 243, 0 };
                            return true;
                        case 0x40000000: // 2.0
                            operand.range = { 244, 0 };
                            return true;
                        case 0xc0000000: // -2.0
                            operand.range = { 245, 0 };
                            return true;
                        case 0x40800000: // 4.0
                            operand.range = { 246, 0 };
                            return true;
                        case 0xc0800000: // -4.0
                            operand.range = { 247, 0 };
                            return true;
                        case 0x3e22f983: // 1/(2*PI)
                            if (arch&ARCH_RX3X0)
                            {
                                operand.range = { 248, 0 };
                                return true;
                            }
                    }
                }
            }
            catch(const ParseException& ex)
            {
                asmr.printError(linePtr, ex.what());
                return false;
            }
        }
        else
        {   // if expression
            std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr));
            if (expr==nullptr) // error
                return false;
            if (expr->isEmpty())
            {
                asmr.printError(exprPlace, "Expected expression");
                return false;
            }
            if (expr->getSymOccursNum()==0)
            {   // resolved now
                cxuint sectionId; // for getting
                if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
                    return false;
                else if (sectionId != ASMSECT_ABS)
                {   // if not absolute value
                    asmr.printError(exprPlace, "Expression must be absolute!");
                    return false;
                }
            }
            else
            {   // return output expression with symbols to resolve
                if ((instrOpMask & INSTROP_ONLYINLINECONSTS)!=0)
                {   // error
                    if ((instrOpMask & INSTROP_NOLITERALERROR)!=0)
                        asmr.printError(regNamePlace, "Literal in VOP3 is illegal");
                    else if ((instrOpMask & INSTROP_NOLITERALERRORMUBUF)!=0)
                        asmr.printError(regNamePlace, "Literal in MUBUF is illegal");
                    else
                        asmr.printError(regNamePlace,
                                "Only one literal can be used in instruction");
                    return false;
                }
                if (outTargetExpr!=nullptr)
                    *outTargetExpr = std::move(expr);
                operand.range = { 255, 0 };
                return true;
            }
            
            if (value <= 64)
            {
                operand.range = { 128+value, 0 };
                return true;
            }
            else if (int64_t(value) >= -16 && int64_t(value) < 0)
            {
                operand.range = { 192-value, 0 };
                return true;
            }
        }
        
        if ((instrOpMask & INSTROP_ONLYINLINECONSTS)!=0)
        {   // error
            if ((instrOpMask & INSTROP_NOLITERALERROR)!=0)
                asmr.printError(regNamePlace, "Literal in VOP3 is illegal");
            else if ((instrOpMask & INSTROP_NOLITERALERRORMUBUF)!=0)
                asmr.printError(regNamePlace, "Literal in MUBUF is illegal");
            else
                asmr.printError(regNamePlace,
                        "Only one literal can be used in instruction");
            return false;
        }
        
        // not in range
        asmr.printWarningForRange(32, value, asmr.getSourcePos(regNamePlace));
        operand = { { 255, 0 }, uint32_t(value), operand.vopMods };
        return true;
    }
    
    // check otherwise
    asmr.printError(linePtr, "Unknown operand");
    return false;
}

static const std::pair<const char*, cxuint> vopSDWADSTSelNamesMap[] =
{
    { "b0", 0 },
    { "b1", 1 },
    { "b2", 2 },
    { "b3", 3 },
    { "byte0", 0 },
    { "byte1", 1 },
    { "byte2", 2 },
    { "byte3", 3 },
    { "byte_0", 0 },
    { "byte_1", 1 },
    { "byte_2", 2 },
    { "byte_3", 3 },
    { "dword", 6 },
    { "w0", 4 },
    { "w1", 5 },
    { "word0", 4 },
    { "word1", 5 },
    { "word_0", 4 },
    { "word_1", 5 }
};

static const size_t vopSDWADSTSelNamesNum = sizeof(vopSDWADSTSelNamesMap)/
            sizeof(std::pair<const char*, cxuint>);

bool GCNAsmUtils::parseVOPModifiers(Assembler& asmr, const char*& linePtr, cxbyte& mods,
                VOPExtraModifiers* extraMods, bool withClamp, cxuint withSDWAOperands)
{
    const char* end = asmr.line+asmr.lineSize;
    //bool haveSDWAMod = false, haveDPPMod = false;
    bool haveDstSel = false, haveSrc0Sel = false, haveSrc1Sel = false;
    bool haveDstUnused = false;
    bool haveBankMask = false, haveRowMask = false;
    bool haveBoundCtrl = false, haveDppCtrl = false;
    
    if (extraMods!=nullptr)
        *extraMods = { 6, 0, (withSDWAOperands>=2)?6U:0U, (withSDWAOperands>=3)?6U:0U,
                    15, 15, 0xe4 /* TODO: why not 0xe4? */, false, false };
    
    skipSpacesToEnd(linePtr, end);
    const char* modsPlace = linePtr;
    bool good = true;
    mods = 0;
    while (linePtr != end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end)
            break;
        char mod[20];
        const char* modPlace = linePtr;
        if (getNameArgS(asmr, 20, mod, linePtr, "VOP modifier"))
        {
            toLowerString(mod);
            try
            {
                bool alreadyModDefined = false;
                if (::strcmp(mod, "mul")==0)
                {
                    skipSpacesToEnd(linePtr, end);
                    if (linePtr!=end && *linePtr==':')
                    {
                        skipCharAndSpacesToEnd(linePtr, end);
                        cxbyte count = cstrtobyte(linePtr, end);
                        if (count==2)
                        {
                            alreadyModDefined = mods&3;
                            mods = (mods&~3) | VOP3_MUL2;
                        }
                        else if (count==4)
                        {
                            alreadyModDefined = mods&3;
                            mods = (mods&~3) | VOP3_MUL4;
                        }
                        else
                        {
                            asmr.printError(modPlace, "Unknown VOP3 mul:X modifier");
                            good = false;
                        }
                    }
                    else
                    {
                        asmr.printError(linePtr, "Expected ':' before multiplier number");
                        good = false;
                    }
                }
                else if (::strcmp(mod, "div")==0)
                {
                    skipSpacesToEnd(linePtr, end);
                    if (linePtr!=end && *linePtr==':')
                    {
                        skipCharAndSpacesToEnd(linePtr, end);
                        cxbyte count = cstrtobyte(linePtr, end);
                        if (count==2)
                        {
                            alreadyModDefined = mods&3;
                            mods = (mods&~3) | VOP3_DIV2;
                        }
                        else
                        {
                            asmr.printError(modPlace, "Unknown VOP3 div:X modifier");
                            good = false;
                        }
                    }
                    else
                    {
                        asmr.printError(linePtr, "Expected ':' before divider number");
                        good = false;
                    }
                }
                else if (::strcmp(mod, "clamp")==0) // clamp
                {
                    if (withClamp)
                        mods |= VOP3_CLAMP;
                    else
                    {
                        asmr.printError(modPlace, "Modifier CLAMP in VOP3B is illegal");
                        good = false;
                    }
                }
                else if (::strcmp(mod, "vop3")==0)
                    mods |= VOP3_VOP3;
                else if (extraMods!=nullptr)
                {   /* VOP_SDWA or VOP_DPP */
                    if (withSDWAOperands>=1 && ::strcmp(mod, "dst_sel")==0)
                    {   // dstsel
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint dstSel = 0;
                            if (getEnumeration(asmr, linePtr, "dst_sel",
                                        vopSDWADSTSelNamesNum,
                                        vopSDWADSTSelNamesMap, dstSel))
                            {
                                extraMods->dstSel = dstSel;
                                if (haveDstSel)
                                    asmr.printWarning(modPlace,
                                              "Dst_sel is already defined");
                                haveDstSel = true;
                            }
                            else
                                good = false;
                        }
                        else
                        {
                            asmr.printError(linePtr, "Expected ':' before dst_sel");
                            good = false;
                        }
                    }
                    else if (withSDWAOperands>=1 &&
                        (::strcmp(mod, "dst_unused")==0 || ::strcmp(mod, "dst_un")==0))
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            skipCharAndSpacesToEnd(linePtr, end);
                            char name[20];
                            const char* enumPlace = linePtr;
                            if (getNameArg(asmr, 20, name, linePtr, "dst_unused"))
                            {
                                toLowerString(name);
                                size_t namePos = (::strncmp(name, "unused_", 7)==0) ? 7 : 0;
                                cxbyte unused = 0;
                                if (::strcmp(name+namePos, "sext")==0)
                                    unused = 1;
                                else if (::strcmp(name+namePos, "preserve")==0)
                                    unused = 2;
                                else if (::strcmp(name+namePos, "pad")!=0)
                                {
                                    asmr.printError(enumPlace, "Unknown dst_unused");
                                    good = false;
                                }
                                extraMods->dstUnused = unused;
                                if (haveDstUnused)
                                    asmr.printWarning(modPlace,
                                                      "Dst_unused is already defined");
                                haveDstUnused = true;
                            }
                            else
                                good = false;
                        }
                        else
                        {
                            asmr.printError(linePtr, "Expected ':' before dst_unused");
                            good = false;
                        }
                    }
                    else if (withSDWAOperands>=2 && ::strcmp(mod, "src0_sel")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint src0Sel = 0;
                            if (getEnumeration(asmr, linePtr, "src0_sel",
                                        vopSDWADSTSelNamesNum,
                                        vopSDWADSTSelNamesMap, src0Sel))
                            {
                                extraMods->src0Sel = src0Sel;
                                if (haveSrc0Sel)
                                    asmr.printWarning(modPlace,
                                                      "Src0_sel is already defined");
                                haveSrc0Sel = true;
                            }
                            else
                                good = false;
                        }
                        else
                        {
                            asmr.printError(linePtr, "Expected ':' before src0_sel");
                            good = false;
                        }
                    }
                    else if (withSDWAOperands>=3 && ::strcmp(mod, "src1_sel")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint src1Sel = 0;
                            if (getEnumeration(asmr, linePtr, "src1_sel",
                                        vopSDWADSTSelNamesNum,
                                        vopSDWADSTSelNamesMap, src1Sel))
                            {
                                extraMods->src1Sel = src1Sel;
                                if (haveSrc1Sel)
                                    asmr.printWarning(modPlace,
                                                      "Src1_sel is already defined");
                                haveSrc1Sel = true;
                            }
                            else
                                good = false;
                        }
                        else
                        {
                            asmr.printError(linePtr, "Expected ':' before src1_sel");
                            good = false;
                        }
                    }
                    else if (::strcmp(mod, "quad_perm")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            bool goodMod = true;
                            skipCharAndSpacesToEnd(linePtr, end);
                            if (linePtr==end || *linePtr!='[')
                            {
                                asmr.printError(linePtr,
                                        "Expected '[' before quad_perm list");
                                goodMod = good = false;
                                continue;
                            }
                            cxbyte quadPerm = 0;
                            linePtr++;
                            for (cxuint k = 0; k < 4; k++)
                            {
                                skipSpacesToEnd(linePtr, end);
                                try
                                {
                                    cxbyte qpv = cstrtobyte(linePtr, end);
                                    if (qpv<4)
                                        quadPerm |= qpv<<(k<<1);
                                    else
                                    {
                                        asmr.printError(linePtr,
                                            "quad_perm component out of range (0-3)");
                                        goodMod = good = false;
                                    }
                                }
                                catch(const ParseException& ex)
                                {
                                    asmr.printError(linePtr, ex.what());
                                    goodMod = good = false;
                                }
                                skipSpacesToEnd(linePtr, end);
                                if (k!=3)
                                {
                                    if (linePtr==end || *linePtr!=',')
                                    {
                                        asmr.printError(linePtr,
                                            "Expected ',' before quad_perm component");
                                        goodMod = good = false;
                                        break;
                                    }
                                    else
                                        ++linePtr;
                                }
                                else if (linePtr==end || *linePtr!=']')
                                {   // unterminated quad_perm
                                    asmr.printError(linePtr, "Unterminated quad_perm");
                                    goodMod = good = false;
                                }
                                else
                                    ++linePtr;
                            }
                            if (goodMod)
                            {
                                extraMods->dppCtrl = quadPerm;
                                if (haveDppCtrl)
                                    asmr.printWarning(modPlace,
                                              "DppCtrl is already defined");
                                haveDppCtrl = true;
                            }
                        }
                        else
                        {
                            asmr.printError(linePtr, "Expected ':' before quad_perm");
                            good = false;
                        }
                    }
                    else if (::strcmp(mod, "bank_mask")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxbyte bankMask = 0;
                            if (parseImm(asmr, linePtr, bankMask, nullptr, 4, WS_UNSIGNED))
                            {
                                extraMods->bankMask = bankMask;
                                if (haveBankMask)
                                    asmr.printWarning(modPlace,
                                              "Bank_mask is already defined");
                                haveBankMask = true;
                            }
                            else
                                good = false;
                        }
                        else
                        {
                            asmr.printError(linePtr, "Expected ':' before bank_mask");
                            good = false;
                        }
                    }
                    else if (::strcmp(mod, "row_mask")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxbyte rowMask = 0;
                            if (parseImm(asmr, linePtr, rowMask, nullptr, 4, WS_UNSIGNED))
                            {
                                extraMods->rowMask = rowMask;
                                if (haveRowMask)
                                    asmr.printWarning(modPlace,
                                              "Row_mask is already defined");
                                haveRowMask = true;
                            }
                            else
                                good = false;
                        }
                        else
                        {
                            asmr.printError(linePtr, "Expected ':' before row_mask");
                            good = false;
                        }
                    }
                    else if (::strcmp(mod, "bound_ctrl")==0)
                    {
                        bool modGood = true;
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            skipCharAndSpacesToEnd(linePtr, end);
                            if (linePtr!=end && (*linePtr=='0' || *linePtr=='1'))
                            {
                                if (*linePtr=='1')
                                    mods |= VOP3_BOUNDCTRL;
                                else // disable
                                    mods &= ~VOP3_BOUNDCTRL;
                                linePtr++;
                            }
                            else
                            {
                                asmr.printError(linePtr, "Value must be '0' or '1'");
                                modGood = good = false;
                            }
                        }
                        else // just enable boundctrl
                            mods |= VOP3_BOUNDCTRL;
                        if (modGood)
                        {   // bound_ctrl is defined
                            if (haveBoundCtrl)
                                asmr.printWarning(modPlace, "BoundCtrl is already defined");
                            haveBoundCtrl = true;
                            extraMods->needDPP = true;
                        }
                    }
                    else if (mod[0]=='r' && mod[1]=='o' && mod[2]=='w' && mod[3]=='_' &&
                            (::strcmp(mod+4, "shl")==0 || ::strcmp(mod+4, "shr")==0 ||
                                ::strcmp(mod+4, "ror")==0))
                    {   //
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            skipCharAndSpacesToEnd(linePtr, end);
                            const char* shiftPlace = linePtr;
                            cxbyte shift = 0;
                            if (parseImm(asmr, linePtr, shift , nullptr, 4, WS_UNSIGNED))
                            {
                                if (shift == 0)
                                {
                                    asmr.printError(shiftPlace,
                                            "Illegal zero shift for row_XXX shift");
                                    good = false;
                                    continue;
                                }
                                if (haveDppCtrl)
                                    asmr.printWarning(modPlace,
                                              "DppCtrl is already defined");
                                haveDppCtrl = true;
                                /* retrieve dppCtrl code from mod name:
                                 * shl - 0, shr - 0x10, ror - 0x20 */
                                extraMods->dppCtrl = 0x100U | ((mod[4]=='r') ? 0x20 :
                                    (mod[4]=='s' && mod[6]=='r') ? 0x10 : 0) | shift;
                            }
                            else
                                good = false;
                        }
                        else
                        {
                            asmr.printError(linePtr, (std::string(
                                        "Expected ':' before ")+mod).c_str());
                            good = false;
                        }
                    }
                    else if (memcmp(mod, "wave_", 5)==0 &&
                        (::strcmp(mod+5, "shl")==0 || ::strcmp(mod+5, "shr")==0 ||
                            ::strcmp(mod+5, "rol")==0 || ::strcmp(mod+5, "ror")==0))
                    {
                        bool modGood = true;
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            skipCharAndSpacesToEnd(linePtr, end);
                            if (linePtr!=end && *linePtr=='1')
                                ++linePtr;
                            else
                            {
                                asmr.printError(linePtr, "Value must be '1'");
                                modGood = good = false;
                            }
                        }
                        if (mod[5]=='s')
                            extraMods->dppCtrl = 0x100 | ((mod[7]=='l') ? 0x30 : 0x38);
                        else if (mod[5]=='r')
                            extraMods->dppCtrl = 0x100 | ((mod[7]=='l') ? 0x34 : 0x3c);
                        if (modGood)
                        {   // dpp_ctrl is defined
                            if (haveDppCtrl)
                                asmr.printWarning(modPlace, "DppCtrl is already defined");
                            haveDppCtrl = true;
                        }
                    }
                    else if (::strcmp(mod, "row_mirror")==0 ||
                        ::strcmp(mod, "row_half_mirror")==0 ||
                        ::strcmp(mod, "row_hmirror")==0)
                    {
                        extraMods->dppCtrl = (mod[4]=='h') ? 0x141 : 0x140;
                        if (haveDppCtrl)
                            asmr.printWarning(modPlace, "DppCtrl is already defined");
                        haveDppCtrl = true;
                    }
                    else if (::memcmp(mod, "row_bcast", 9)==0 && (
                        (mod[9]=='1' && mod[10]=='5' && mod[11]==0) ||
                        (mod[9]=='3' && mod[10]=='1' && mod[11]==0) || mod[9]==0))
                    {
                        bool modGood = true;
                        if (mod[9] =='1')
                            extraMods->dppCtrl = 0x142;
                        else if (mod[9] =='3')
                            extraMods->dppCtrl = 0x143;
                        else
                        { // get number
                            skipSpacesToEnd(linePtr, end);
                            if (linePtr!=end && *linePtr==':')
                            {
                                skipCharAndSpacesToEnd(linePtr, end);
                                const char* numPlace = linePtr;
                                cxbyte value = cstrtobyte(linePtr, end);
                                if (value == 31)
                                    extraMods->dppCtrl = 0x143;
                                else if (value == 15)
                                    extraMods->dppCtrl = 0x142;
                                else
                                {
                                    asmr.printError(numPlace, "Thread to broadcast must be"
                                                " 15 or 31");
                                    modGood = good = false;
                                }
                            }
                            else
                            {
                                asmr.printError(linePtr, "Expected ':' before row_bcast");
                                modGood = good = false;
                            }
                        }
                        if (modGood)
                        {
                            if (haveDppCtrl)
                                asmr.printWarning(modPlace, "DppCtrl is already defined");
                            haveDppCtrl = true;
                        }
                    }
                    else
                    {   /// unknown modifier
                        asmr.printError(modPlace, "Unknown VOP modifier");
                        good = false;
                    }
                }
                else
                {   /// unknown modifier
                    asmr.printError(modPlace, "Unknown VOP modifier");
                    good = false;
                }
                
                if (alreadyModDefined)
                    asmr.printWarning(modPlace, "OMOD is already defined");
            }
            catch(const ParseException& ex)
            {
                asmr.printError(linePtr, ex.what());
                good = false;
            }
        }
        else
            good = false;
    }
    bool vopSDWA = (haveDstSel || haveDstUnused || haveSrc0Sel || haveSrc1Sel);
    bool vopDPP = (haveDppCtrl || haveBoundCtrl || haveBankMask || haveRowMask);
    bool vop3 = (mods & (3|VOP3_VOP3))!=0;
    if (extraMods!=nullptr)
    {
        extraMods->needSDWA = vopSDWA;
        extraMods->needDPP = vopDPP;
    }
        
    if ((int(vop3)+vopSDWA+vopDPP)>1 || ((mods&VOP3_CLAMP)!=0 && vopDPP))
    {
        asmr.printError(modsPlace, "Mixing modifiers from different encodings is illegal");
        return false;
    }
    return good;
}

static const char* vintrpParamsTbl[] =
{ "p10", "p20", "p0" };

bool GCNAsmUtils::parseVINTRP0P10P20(Assembler& asmr, const char*& linePtr, RegRange& reg)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* p0Place = linePtr;
    char pxName[5];
    if (getNameArg(asmr, 5, pxName, linePtr, "VINTRP parameter"))
    {
        cxuint p0Code = 0;
        toLowerString(pxName);
        for (p0Code = 0; p0Code < 3; p0Code++)
            if (::strcmp(vintrpParamsTbl[p0Code], pxName)==0)
                break;
        if (p0Code < 3) // as srcReg
            reg = { p0Code, p0Code+1 };
        else
        {
            asmr.printError(p0Place, "Unknown VINTRP parameter");
            return false;
        }
        return true;
    }
    return false;
}

bool GCNAsmUtils::parseVINTRPAttr(Assembler& asmr, const char*& linePtr, cxbyte& attr)
{
    bool good = true;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    bool goodAttr = true;
    const char* attrPlace = linePtr;
    if (linePtr+4 > end)
    {
        asmr.printError(attrPlace, "Expected 'attr' keyword");
        goodAttr = good = false;
    }
    char buf[5];
    if (goodAttr)
    {
        std::transform(linePtr, linePtr+4, buf, toLower);
        if (::memcmp(buf, "attr", 4)!=0)
        {
            while (linePtr!=end && *linePtr!=' ') linePtr++;
            asmr.printError(attrPlace, "Expected 'attr' keyword");
            goodAttr = good = false;
        }
        else
            linePtr+=4;
    }
    
    cxbyte attrVal = 0;
    if (goodAttr)
    {
        const char* attrNumPlace = linePtr;
        try
        { attrVal = cstrtobyte(linePtr, end); }
        catch(const ParseException& ex)
        {
            asmr.printError(linePtr, ex.what());
            goodAttr = good = false;
        }
        if (attrVal >= 64)
        {
            asmr.printError(attrNumPlace, "Attribute number out of range (0-63)");
            goodAttr = good = false;
        }
    }
    if (goodAttr)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!='.')
        {
            asmr.printError(linePtr, "Expected '.' after attribute number");
            goodAttr = good = false;
        }
        else
            ++linePtr;
    }
    if (goodAttr)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
        {
            asmr.printError(linePtr, "Expected attribute component");
            goodAttr = good = false;
        }
    }
    char attrCmpName = 0;
    if (goodAttr)
    {
        attrCmpName = toLower(*linePtr);
        if (attrCmpName!='x' && attrCmpName!='y' && attrCmpName!='z' && attrCmpName!='w')
        {
            asmr.printError(linePtr, "Expected attribute component");
            good = false;
        }
        linePtr++;
    }
    
    attr = (attrVal<<2) | ((attrCmpName=='x') ? 0 : (attrCmpName=='y') ? 1 :
            (attrCmpName=='z') ? 2 : 3);
    return good;
}

bool GCNAsmUtils::getMUBUFFmtNameArg(Assembler& asmr, size_t maxOutStrSize, char* outStr,
               const char*& linePtr, const char* objName)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
    {
        asmr.printError(linePtr, (std::string("Expected ")+objName).c_str());
        return false;
    }
    const char* nameStr = linePtr;
    if (isAlnum(*linePtr) || *linePtr == '_' || *linePtr == '.')
    {
        linePtr++;
        while (linePtr != end && (isAlnum(*linePtr) ||
            *linePtr == '_' || *linePtr == '.')) linePtr++;
    }
    else
    {
        asmr.printError(linePtr, (std::string("Some garbages at ")+objName+
                " place").c_str());
        while (linePtr != end && !isSpace(*linePtr)) linePtr++;
        return false;
    }
    if (maxOutStrSize-1 < size_t(linePtr-nameStr))
    {
        asmr.printError(linePtr, (std::string(objName)+" is too long").c_str());
        return false;
    }
    const size_t outStrSize = std::min(maxOutStrSize-1, size_t(linePtr-nameStr));
    std::copy(nameStr, nameStr+outStrSize, outStr);
    outStr[outStrSize] = 0; // null-char
    return true;
}

};
