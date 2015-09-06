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
//#include <iostream>
#include <cstdio>
#include <vector>
#include <cstring>
#include <algorithm>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"
#include "GCNInternals.h"

using namespace CLRX;

static std::once_flag clrxGCNAssemblerOnceFlag;
static Array<GCNAsmInstruction> gcnInstrSortedTable;

static void initializeGCNAssembler()
{
    size_t tableSize = 0;
    while (gcnInstrsTable[tableSize].mnemonic!=nullptr)
        tableSize++;
    gcnInstrSortedTable.resize(tableSize);
    for (cxuint i = 0; i < tableSize; i++)
    {
        const GCNInstruction& insn = gcnInstrsTable[i];
        gcnInstrSortedTable[i] = {insn.mnemonic, insn.encoding, GCNENC_NONE, insn.mode,
                    insn.code, UINT16_MAX, insn.archMask};
    }
    
    std::sort(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
            [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
            {   // compare mnemonic and if mnemonic
                int r = ::strcmp(instr1.mnemonic, instr2.mnemonic);
                return (r < 0) || (r==0 && instr1.encoding1 < instr2.encoding1) ||
                            (r == 0 && instr1.encoding1 == instr2.encoding1 &&
                             instr1.archMask < instr2.archMask);
            });
    
    cxuint j = 0;
    /* join VOP3A instr with VOP2/VOPC/VOP1 instr together to faster encoding. */
    for (cxuint i = 0; i < tableSize; i++)
    {   
        GCNAsmInstruction insn = gcnInstrSortedTable[i];
        if ((insn.encoding1 == GCNENC_VOP3A || insn.encoding1 == GCNENC_VOP3B))
        {   // check duplicates
            cxuint k = j-1;
            while (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                    (gcnInstrSortedTable[k].archMask & insn.archMask)!=insn.archMask) k--;
            
            if (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                (gcnInstrSortedTable[k].archMask & insn.archMask)==insn.archMask)
            {   // we found duplicate, we apply
                if (gcnInstrSortedTable[k].code2==UINT16_MAX)
                {   // if second slot for opcode is not filled
                    gcnInstrSortedTable[k].code2 = insn.code1;
                    gcnInstrSortedTable[k].encoding2 = insn.encoding1;
                    gcnInstrSortedTable[k].archMask &= insn.archMask;
                }
                else
                {   // if filled we create new entry
                    gcnInstrSortedTable[j] = gcnInstrSortedTable[k];
                    gcnInstrSortedTable[j].archMask &= insn.archMask;
                    gcnInstrSortedTable[j].encoding2 = insn.encoding1;
                    gcnInstrSortedTable[j++].code2 = insn.code1;
                }
            }
            else // not found
                gcnInstrSortedTable[j++] = insn;
        }
        else // normal instruction
            gcnInstrSortedTable[j++] = insn;
    }
    gcnInstrSortedTable.resize(j); // final size
    /* for (const GCNAsmInstruction& instr: gcnInstrSortedTable)
        std::cout << "{ " << instr.mnemonic << ", " << cxuint(instr.encoding1) << ", " <<
                cxuint(instr.encoding2) <<
                std::hex << ", 0x" << instr.mode << ", 0x" << instr.code1 << ", 0x" <<
                instr.code2 << std::dec << ", " << instr.archMask << " }" << std::endl;*/
}

GCNAssembler::GCNAssembler(Assembler& assembler): ISAAssembler(assembler),
        regs({0, 0}), curArchMask(1U<<cxuint(
                    getGPUArchitectureFromDeviceType(assembler.getDeviceType())))
{
    std::call_once(clrxGCNAssemblerOnceFlag, initializeGCNAssembler);
}

GCNAssembler::~GCNAssembler()
{ }

static cxbyte cstrtobyte(const char*& str, const char* end)
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

bool GCNAsmUtils::parseVRegRange(Assembler& asmr, const char*& linePtr, RegPair& regPair,
                    cxuint regsNum, bool required)
{
    const char* oldLinePtr = linePtr;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* vgprRangePlace = linePtr;
    if (linePtr == end || toLower(*linePtr) != 'v' || ++linePtr == end)
    {
        if (printRegisterRangeExpected(asmr, vgprRangePlace, "vector", regsNum, required))
            return false;
        regPair = { 0, 0 }; // no range
        linePtr = oldLinePtr; // revert current line pointer
        return true;
    }
    
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
        cxuint value1, value2;
        skipSpacesToEnd(linePtr, end);
        value1 = cstrtobyte(linePtr, end);
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || (*linePtr!=':' && *linePtr!=']'))
        {   // error
            asmr.printError(vgprRangePlace, "Unterminated vector register range");
            return false;
        }
        if (linePtr!=end && *linePtr==':')
        {
            ++linePtr;
            skipSpacesToEnd(linePtr, end);
            value2 = cstrtobyte(linePtr, end);
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

bool GCNAsmUtils::parseSRegRange(Assembler& asmr, const char*& linePtr, RegPair& regPair,
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
    if (toLower(*linePtr) != 's') // if
    {
        const char* oldLinePtr = linePtr;
        char regName[20];
        if (!getNameArg(asmr, 20, regName, linePtr, "register name", required, false))
            return false;
        toLowerString(regName);
        
        size_t loHiRegSuffix = 0;
        cxuint loHiReg = 0;
        if (regName[0] == 'v' && regName[1] == 'c' && regName[2] == 'c')
        {   /// vcc
            loHiRegSuffix = 3;
            loHiReg = 106;
        }
        else if (regName[0] == 'e' && regName[1] == 'x' && regName[2] == 'e' &&
            regName[3] == 'c')
        {   /* exec* */
            loHiRegSuffix = 4;
            loHiReg = 126;
        }
        else if (regName[0]=='t')
        {   /* tma,tba, ttmpX */
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
            else if (regName[1] == 't' && regName[2] == 'm' && regName[3] == 'p')
            {
                ttmpReg = true;
                linePtr = oldLinePtr;
            }
        }
        else if (regName[0] == 'm' && regName[1] == '0' && regName[2] == 0)
        {
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
                    regPair = std::make_pair(loHiReg, loHiReg+1);
                else if (regName[loHiRegSuffix+1] == 'h' &&
                    regName[loHiRegSuffix+2] == 'i' && regName[loHiRegSuffix+3] == 0)
                    regPair = std::make_pair(loHiReg+1, loHiReg+2);
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
        else if (!ttmpReg)
        {   // otherwise
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
                asmr.printError(sgprRangePlace, "Scalar register number of out range");
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
        if (regsNum!=0 && regsNum != 1)
        {
            printXRegistersRequired(asmr, linePtr, "scalar", regsNum);
            return false;
        }
        if (!ttmpReg)
            regPair =  { value, value+1 };
        else
            regPair = { 112+value, 112+value+1 };
        return true;
    }
    else if (*linePtr == '[')
    {   // many registers
        ++linePtr;
        cxuint value1, value2;
        skipSpacesToEnd(linePtr, end);
        value1 = cstrtobyte(linePtr, end);
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
            ++linePtr;
            skipSpacesToEnd(linePtr, end);
            value2 = cstrtobyte(linePtr, end);
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

template<typename T>
bool GCNAsmUtils::parseImm(Assembler& asmr, const char*& linePtr, T& outValue,
            std::unique_ptr<AsmExpression>& outTargetExpr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* exprPlace = linePtr;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse( asmr, linePtr));
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
        asmr.printWarningForRange(sizeof(T)<<3, value, asmr.getSourcePos(exprPlace));
        outValue = value;
        return true;
    }
    else
    {   // return output expression with symbols to resolve
        outTargetExpr = std::move(expr);
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
            std::unique_ptr<AsmExpression>& outTargetExpr, Flags instropMask)
{
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
    return parseImm<uint32_t>(asmr, linePtr, value, outTargetExpr);
}

bool GCNAsmUtils::parseOperand(Assembler& asmr, const char*& linePtr, GCNOperand& operand,
             std::unique_ptr<AsmExpression>& outTargetExpr, uint16_t arch,
             cxuint regsNum, Flags instrOpMask)
{
    outTargetExpr.reset();
    
    if (instrOpMask == INSTROP_SREGS)
        return parseSRegRange(asmr, linePtr, operand.pair, arch, regsNum);
    else if (instrOpMask == INSTROP_VREGS)
        return parseVRegRange(asmr, linePtr, operand.pair, regsNum);
    
    const char* end = asmr.line+asmr.lineSize;
    if (instrOpMask & INSTROP_VOP3MODS)
    {
        operand.vop3Mods = 0;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr=='@') // treat this operand as expression
            return parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                             instrOpMask & ~INSTROP_VOP3MODS);
        const char* negPlace = linePtr;
        if (linePtr!=end && *linePtr=='-')
        {
            operand.vop3Mods |= VOPOPFLAG_NEG;
            skipCharAndSpacesToEnd(linePtr, end);
        }
        if (linePtr+3 <= end && toLower(linePtr[0])=='a' &&
            toLower(linePtr[1])=='b' && toLower(linePtr[2])=='s')
        {
            linePtr += 3;
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr=='(')
            {
                operand.vop3Mods |= VOPOPFLAG_ABS;
                ++linePtr;
            }
            else
            {
                asmr.printError(linePtr, "Expected '(' after abs");
                return false;
            }
        }
        
        bool good;
        if (operand.vop3Mods != VOPOPFLAG_NEG)
            good = parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                                     instrOpMask & ~INSTROP_VOP3MODS);
        else //
        {
            linePtr = negPlace;
            good = parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                             (instrOpMask & ~INSTROP_VOP3MODS) | INSTROP_PARSEWITHNEG);
        }
        
        if (operand.vop3Mods & VOPOPFLAG_ABS)
        {
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr==')')
                linePtr++;
            else
            {
                asmr.printError(linePtr, "Unterminated abs() modifier call");
                return false;
            }
        }
        return good;
    }
    skipSpacesToEnd(linePtr, end);
    const char* negPlace = linePtr;
    if (instrOpMask & INSTROP_VOP3NEG)
        operand.vop3Mods = 0;
    if (instrOpMask & (INSTROP_PARSEWITHNEG|INSTROP_VOP3NEG))
    {
        if (linePtr!=end && *linePtr=='-')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            operand.vop3Mods |= VOPOPFLAG_NEG;
        }
    }
    
    // otherwise
    if (instrOpMask & INSTROP_SREGS)
    {
        if (!parseSRegRange(asmr, linePtr, operand.pair, arch, regsNum, false))
            return false;
        if (operand.pair.first!=0 || operand.pair.second!=0)
            return true;
    }
    if (instrOpMask & INSTROP_VREGS)
    {
        if (!parseVRegRange(asmr, linePtr, operand.pair, regsNum, false))
            return false;
        if (operand.pair.first!=0 || operand.pair.second!=0)
            return true;
    }
    
    skipSpacesToEnd(linePtr, end);
    
    if ((instrOpMask & INSTROP_SSOURCE)!=0)
    {
        char regName[20];
        const char* regNamePlace = linePtr;
        if (getNameArg(asmr, 20, regName, linePtr, "register name", false, false))
        {
            toLowerString(regName);
            operand.pair = {0, 0};
            if (::strcmp(regName, "vccz") == 0)
            {
                operand.pair = { 251, 252 };
                return true;
            }
            else if (::strcmp(regName, "execz") == 0)
            {
                operand.pair = { 252, 253 };
                return true;
            }
            else if (::strcmp(regName, "scc") == 0)
            {
                operand.pair = { 253, 254 };
                return true;
            }
            else if ((::strcmp(regName, "lds")==0 || ::strcmp(regName, "lds_direct")==0))
            {
                if ((instrOpMask&INSTROP_LDS)==0)
                {
                    asmr.printError(regNamePlace, "LDS_Direct in this place is illegal");
                    return false;
                }
                operand.pair = { 254, 255 };
                return true;
            }
            if (operand.pair.first!=0 || operand.pair.second!=0)
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
        operand.vop3Mods = 0; // zeroing operand modifiers
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
                            operand.pair = { 128, 0 };
                            return true;
                        case 0x3800: // 0.5
                            operand.pair = { 240, 0 };
                            return true;
                        case 0xb800: // -0.5
                            operand.pair = { 241, 0 };
                            return true;
                        case 0x3c00: // 1.0
                            operand.pair = { 242, 0 };
                            return true;
                        case 0xbc00: // -1.0
                            operand.pair = { 243, 0 };
                            return true;
                        case 0x4000: // 2.0
                            operand.pair = { 244, 0 };
                            return true;
                        case 0xc000: // -2.0
                            operand.pair = { 245, 0 };
                            return true;
                        case 0x4400: // 4.0
                            operand.pair = { 246, 0 };
                            return true;
                        case 0xc400: // -4.0
                            operand.pair = { 247, 0 };
                            return true;
                        case 0x3118: // 1/(2*PI)
                            if (arch&ARCH_RX3X0)
                            {
                                operand.pair = { 248, 0 };
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
                            operand.pair = { 128, 0 };
                            return true;
                        case 0x3f000000: // 0.5
                            operand.pair = { 240, 0 };
                            return true;
                        case 0xbf000000: // -0.5
                            operand.pair = { 241, 0 };
                            return true;
                        case 0x3f800000: // 1.0
                            operand.pair = { 242, 0 };
                            return true;
                        case 0xbf800000: // -1.0
                            operand.pair = { 243, 0 };
                            return true;
                        case 0x40000000: // 2.0
                            operand.pair = { 244, 0 };
                            return true;
                        case 0xc0000000: // -2.0
                            operand.pair = { 245, 0 };
                            return true;
                        case 0x40800000: // 4.0
                            operand.pair = { 246, 0 };
                            return true;
                        case 0xc0800000: // -4.0
                            operand.pair = { 247, 0 };
                            return true;
                        case 0x3e22f983: // 1/(2*PI)
                            if (arch&ARCH_RX3X0)
                            {
                                operand.pair = { 248, 0 };
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
                    asmr.printError(regNamePlace,
                            "Only one literal constant can be used in instruction");
                    return false;
                }
                outTargetExpr = std::move(expr);
                operand.pair = { 255, 0 };
                return true;
            }
            
            if (value <= 64)
            {
                operand.pair = { 128+value, 0 };
                return true;
            }
            else if (int64_t(value) >= -16 && int64_t(value) < 0)
            {
                operand.pair = { 192-value, 0 };
                return true;
            }
        }
        
        if ((instrOpMask & INSTROP_ONLYINLINECONSTS)!=0)
        {   // error
            asmr.printError(regNamePlace,
                        "Only one literal constant can be used in instruction");
            return false;
        }
        
        // not in range
        asmr.printWarningForRange(32, value, asmr.getSourcePos(regNamePlace));
        operand = { { 255, 0 }, uint32_t(value), operand.vop3Mods };
        return true;
    }
    
    // check otherwise
    asmr.printError(linePtr, "Unrecognized operand");
    return false;
}

static inline bool isXRegRange(RegPair pair, cxuint regsNum = 1)
{   // second==0 - we assume that first is inline constant, otherwise we check range
    return (pair.second==0) || cxuint(pair.second-pair.first)==regsNum;
}

static inline void updateVGPRsNum(cxuint& vgprsNum, cxuint vgpr)
{
    vgprsNum = std::min(std::max(vgprsNum, vgpr+1), 256U);
}

static inline void updateSGPRsNum(cxuint& sgprsNum, cxuint sgpr, uint16_t arch)
{
    cxuint maxSGPRsNum = arch&ARCH_RX3X0 ? 102U: 104U;
    if (sgpr < maxSGPRsNum) /* sgpr=-1 does not change sgprsNum */
        sgprsNum = std::min(std::max(sgprsNum, sgpr+1), arch&ARCH_RX3X0 ? 100U: 102U);
}

void GCNAsmUtils::parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    RegPair dstReg(0, 1);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_REG_S1_JMP)
    {
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        if (!skipRequiredComma(asmr, linePtr))
            return;
    }
    
    std::unique_ptr<AsmExpression> src0Expr, src1Expr;
    GCNOperand src0Op;
    good &= parseOperand(asmr, linePtr, src0Op, src0Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    GCNOperand src1Op;
    good &= parseOperand(asmr, linePtr, src1Op, src1Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
             (src0Op.pair.first==255 ? INSTROP_ONLYINLINECONSTS : 0));
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // put data
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0x80000000U | (uint32_t(gcnInsn.code1)<<23) | src0Op.pair.first |
            (src1Op.pair.first<<8) | uint32_t(dstReg.first)<<16);
    if (src0Op.pair.first==255 || src1Op.pair.first==255)
    {
        if (src0Expr==nullptr && src1Expr==nullptr)
            SLEV(words[1], src0Op.pair.first==255 ? src0Op.value : src1Op.value);
        else    // zero if unresolved value
            SLEV(words[1], 0);
        wordsNum++;
    }
    // set expression targets
    if (src0Expr!=nullptr)
        src0Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    else if (src1Expr!=nullptr)
        src1Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words + wordsNum));
    // prevent freeing expressions
    src0Expr.release();
    src1Expr.release();
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.second-1, arch);
}

void GCNAsmUtils::parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    RegPair dstReg(0, 1);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
    {
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
            if (!skipRequiredComma(asmr, linePtr))
                return;
    }
    
    GCNOperand src0Op = { std::make_pair(0,1) };
    std::unique_ptr<AsmExpression> src0Expr;
    if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
        good &= parseOperand(asmr, linePtr, src0Op, src0Expr, arch,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS);
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0xbe800000U | (uint32_t(gcnInsn.code1)<<8) | src0Op.pair.first |
            uint32_t(dstReg.first)<<16);
    if (src0Op.pair.first==255)
    {
        if (src0Expr==nullptr)
            SLEV(words[1], src0Op.value);
        else    // zero if unresolved value
            SLEV(words[1], 0);
        wordsNum++;
    }
    // set expression targets
    if (src0Expr!=nullptr)
        src0Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words + wordsNum));
    // prevent freeing expressions
    src0Expr.release();
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.second-1, arch);
}

static const std::pair<const char*, uint16_t> hwregNamesMap[] =
{
    { "gpr_alloc", 5 },
    { "hw_id", 4 },
    { "ib_dbg0", 12 },
    { "ib_sts", 7 },
    { "inst_dw0", 10 },
    { "inst_dw1", 11 },
    { "lds_alloc", 6 },
    { "mode", 1 },
    { "pc_hi", 9 },
    { "pc_lo", 8 },
    { "status", 2 },
    { "trapsts", 3 }
};

static const size_t hwregNamesMapSize = sizeof(hwregNamesMap) /
            sizeof(std::pair<const char*, uint16_t>);

void GCNAsmUtils::parseSOPKEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    RegPair dstReg(0, 1);
    if ((gcnInsn.mode & GCN_IMM_DST) == 0)
    {
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        if (!skipRequiredComma(asmr, linePtr))
            return;
    }
    
    uint16_t imm16 = 0;
    std::unique_ptr<AsmExpression> imm16Expr;
    
    if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL)
    {
        uint64_t value = 0;
        if (!getJumpValueArg(asmr, value, imm16Expr, linePtr))
            return;
        if (imm16Expr==nullptr)
        {
            int64_t offset = (int64_t(value)-int64_t(output.size())-4);
            if (offset & 3)
            {
                asmr.printError(linePtr, "Jump is not aligned to word!");
                good = false;
            }
            offset >>= 2;
            if (offset > INT16_MAX || offset < INT16_MIN)
            {
                asmr.printError(linePtr, "Jump out of range");
                good = false;
            }
            imm16 = offset;
        }
    }
    else if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_SREG)
    {
        skipSpacesToEnd(linePtr, end);
        char name[20];
        const char* funcNamePlace = linePtr;
        if (!getNameArg(asmr, 20, name, linePtr, "function name", true))
            return;
        toLowerString(name);
        skipSpacesToEnd(linePtr, end);
        if (::strcmp(name, "hwreg")!=0 || linePtr==end || *linePtr!='(')
        {
            asmr.printError(funcNamePlace, "Expected hwreg function");
            return;
        }
        ++linePtr;
        skipSpacesToEnd(linePtr, end);
        const char* funcArg1Place = linePtr;
        size_t hwregId = 0;
        if (getNameArg(asmr, 20, name, linePtr, "HWRegister name", true))
        {
            toLowerString(name);
            const size_t hwregNameIndex = (::strncmp(name, "hwreg_", 6) == 0) ? 6 : 0;
            size_t index = binaryMapFind(hwregNamesMap, hwregNamesMap + hwregNamesMapSize,
                  name+hwregNameIndex, CStringLess()) - hwregNamesMap;
            if (index != hwregNamesMapSize)
                hwregId = hwregNamesMap[index].second;
            else
            {
                asmr.printError(funcArg1Place, "Unrecognized HWRegister");
                good = false;
            }
        }
        else
            good = false;
        
        if (!skipRequiredComma(asmr, linePtr))
            return;
        uint64_t arg2Value = 0;
        skipSpacesToEnd(linePtr, end);
        const char* funcArg2Place = linePtr;
        if (getAbsoluteValueArg(asmr, arg2Value, linePtr, true))
        {
            if (arg2Value >= 32)
                asmr.printWarning(funcArg2Place, "Second argument out of range (0-31)");
        }
        else
            good = false;
        
        if (!skipRequiredComma(asmr, linePtr))
            return;
        uint64_t arg3Value = 0;
        skipSpacesToEnd(linePtr, end);
        const char* funcArg3Place = linePtr;
        if (getAbsoluteValueArg(asmr, arg3Value, linePtr, true))
        {
            if (arg3Value >= 33 || arg3Value < 1)
                asmr.printWarning(funcArg3Place, "Third argument out of range (1-32)");
        }
        else
            good = false;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!=')')
        {
            asmr.printError(linePtr, "Unterminated hwreg function call");
            return;
        }
        ++linePtr;
        imm16 = hwregId | (arg2Value<<6) | ((arg3Value-1)<<11);
    }
    else // otherwise we parse expression
        good &= parseImm<uint16_t>(asmr, linePtr, imm16, imm16Expr);
    
    uint32_t imm32 = 0;
    std::unique_ptr<AsmExpression> imm32Expr;
    if (gcnInsn.mode & GCN_IMM_DST)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        if (gcnInsn.mode & GCN_SOPK_CONST)
            good &= parseImm<uint32_t>(asmr, linePtr, imm32, imm32Expr);
        else
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const cxuint wordsNum = (gcnInsn.mode & GCN_SOPK_CONST) ? 2 : 1;
    uint32_t words[2];
    SLEV(words[0], 0xb0000000U | imm16 | uint32_t(dstReg.first)<<16 |
                uint32_t(gcnInsn.code1)<<23);
    if (wordsNum==2)
        SLEV(words[1], imm32);
    
    if (imm32Expr!=nullptr)
        imm32Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                           output.size()));
    if (imm16Expr!=nullptr)
        imm16Expr->setTarget(AsmExprTarget(((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL) ?
                GCNTGT_SOPJMP : GCNTGT_SOPKSIMM16, asmr.currentSection, output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    imm32Expr.release();
    imm16Expr.release();
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.second-1, arch);
}

void GCNAsmUtils::parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    std::unique_ptr<AsmExpression> src0Expr, src1Expr;
    GCNOperand src0Op;
    good &= parseOperand(asmr, linePtr, src0Op, src0Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    GCNOperand src1Op;
    good &= parseOperand(asmr, linePtr, src1Op, src1Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
             (src0Op.pair.first==255 ? INSTROP_ONLYINLINECONSTS : 0));
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // put data
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0xbf000000U | (uint32_t(gcnInsn.code1)<<16) | src0Op.pair.first |
            (src1Op.pair.first<<8));
    if (src0Op.pair.first==255 || src1Op.pair.first==255)
    {
        if (src0Expr==nullptr && src1Expr==nullptr)
            SLEV(words[1], src0Op.pair.first==255 ? src0Op.value : src1Op.value);
        else    // zero if unresolved value
            SLEV(words[1], 0);
        wordsNum++;
    }
    // set expression targets
    if (src0Expr!=nullptr)
        src0Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    else if (src1Expr!=nullptr)
        src1Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words + wordsNum));
    // prevent freeing expressions
    src0Expr.release();
    src1Expr.release();
}

static const std::pair<const char*, uint16_t> sendMessageNamesMap[] =
{
    { "gs", 2 },
    { "gs_done", 3 },
    { "interrupt", 1 },
    { "sysmsg", 15 },
    { "system", 15 }
};

static const size_t sendMessageNamesMapSize = sizeof(sendMessageNamesMap) /
            sizeof(std::pair<const char*, uint16_t>);

static const char* sendMsgGSOPTable[] =
{ "nop", "cut", "emit", "emit_cut" };

void GCNAsmUtils::parseSOPPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    uint16_t imm16 = 0;
    std::unique_ptr<AsmExpression> imm16Expr;
    switch (gcnInsn.mode&GCN_MASK1)
    {
        case GCN_IMM_REL:
        {
            uint64_t value = 0;
            if (!getJumpValueArg(asmr, value, imm16Expr, linePtr))
                return;
            if (imm16Expr==nullptr)
            {
                int64_t offset = (int64_t(value)-int64_t(output.size())-4);
                if (offset & 3)
                {
                    asmr.printError(linePtr, "Jump is not aligned to word!");
                    good = false;
                }
                offset >>= 2;
                if (offset > INT16_MAX || offset < INT16_MIN)
                {
                    asmr.printError(linePtr, "Jump out of range");
                    good = false;
                }
                imm16 = offset;
            }
            break;
        }
        case GCN_IMM_LOCKS:
        {   /* parse locks for s_waitcnt */
            char name[20];
            bool haveLgkmCnt = false;
            bool haveExpCnt = false;
            bool haveVMCnt = false;
            imm16 = 0xf7f;
            while (true)
            {
                const char* funcNamePlace = linePtr;
                if (!getNameArg(asmr, 20, name, linePtr, "function name", true))
                    return;
                toLowerString(name);
                
                cxuint bitPos = 0, bitMask = 0;
                if (::strcmp(name, "vmcnt")==0)
                {
                    if (haveVMCnt)
                        asmr.printWarning(funcNamePlace, "vmcnt was already defined");
                    bitPos = 0;
                    bitMask = 15;
                    haveVMCnt = true;
                }
                else if (::strcmp(name, "lgkmcnt")==0)
                {
                    if (haveLgkmCnt)
                        asmr.printWarning(funcNamePlace, "lgkmcnt was already defined");
                    bitPos = 8;
                    bitMask = 15;
                    haveLgkmCnt = true;
                }
                else if (::strcmp(name, "expcnt")==0)
                {
                    if (haveExpCnt)
                        asmr.printWarning(funcNamePlace, "expcnt was already defined");
                    bitPos = 4;
                    bitMask = 7;
                    haveExpCnt = true;
                }
                else
                {
                    asmr.printError(funcNamePlace, "Expected vmcnt, lgkmcnt or expcnt");
                    return;
                }
                
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end || *linePtr!='(')
                {
                    asmr.printError(funcNamePlace, "Expected vmcnt, lgkmcnt or expcnt");
                    return;
                }
                ++linePtr;
                skipSpacesToEnd(linePtr, end);
                const char* argPlace = linePtr;
                uint64_t value;
                if (getAbsoluteValueArg(asmr, value, linePtr))
                {
                    if (value > bitMask)
                        asmr.printWarning(argPlace, "Value out of range");
                    imm16 = (imm16 & ~(bitMask<<bitPos)) | ((value&bitMask)<<bitPos);
                }
                else
                    good = false;
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end || *linePtr!=')')
                {
                    asmr.printError(linePtr, "Unterminated function call");
                    return;
                }
                ++linePtr;
                
                // ampersand
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end)
                    break;
                if (linePtr[0] != '&')
                {
                    asmr.printError(linePtr, "Expected '&' before lock function");
                    return;
                }
                ++linePtr;
            }
            break;
        }
        case GCN_IMM_MSGS:
        {
            char name[20];
            const char* funcNamePlace = linePtr;
            if (!getNameArg(asmr, 20, name, linePtr, "function name", true))
                return;
            toLowerString(name);
            skipSpacesToEnd(linePtr, end);
            if (::strcmp(name, "sendmsg")!=0 || linePtr==end || *linePtr!='(')
            {
                asmr.printError(funcNamePlace, "Expected sendmsg function");
                return;
            }
            ++linePtr;
            skipSpacesToEnd(linePtr, end);
            
            const char* funcArg1Place = linePtr;
            size_t sendMessage = 0;
            if (getNameArg(asmr, 20, name, linePtr, "message name", true))
            {
                toLowerString(name);
                const size_t msgNameIndex = (::strncmp(name, "msg_", 4) == 0) ? 4 : 0;
                size_t index = binaryMapFind(sendMessageNamesMap,
                         sendMessageNamesMap + sendMessageNamesMapSize,
                         name+msgNameIndex, CStringLess()) - sendMessageNamesMap;
                if (index == sendMessageNamesMapSize)
                {
                    asmr.printError(funcArg1Place, "Unrecognized message");
                    good = false;
                }
                sendMessage = sendMessageNamesMap[index].second;
            }
            else
                good = false;
            
            cxuint gsopIndex = 0;
            cxuint streamId = 0;
            if (sendMessage == 2 || sendMessage == 3)
            {
                if (!skipRequiredComma(asmr, linePtr))
                    return;
                skipSpacesToEnd(linePtr, end);
                const char* funcArg2Place = linePtr;
                if (getNameArg(asmr, 20, name, linePtr, "GSOP", true))
                {
                    toLowerString(name);
                    const size_t gsopNameIndex = (::strncmp(name, "gs_op_", 6) == 0)
                                ? 6 : 0;
                    for (gsopIndex = 0; gsopIndex < 4; gsopIndex++)
                        if (::strcmp(name+gsopNameIndex, sendMsgGSOPTable[gsopIndex])==0)
                            break;
                    if (gsopIndex == 4)
                    {   // not found
                        gsopIndex = 0;
                        asmr.printError(funcArg2Place, "Unrecognized GSOP");
                        good = false;
                    }
                }
                else
                    good = false;
                
                if (gsopIndex!=0)
                {
                    if (!skipRequiredComma(asmr, linePtr))
                        return;
                    
                    uint64_t value;
                    skipSpacesToEnd(linePtr, end);
                    const char* func3ArgPlace = linePtr;
                    good &= getAbsoluteValueArg(asmr, value, linePtr, true);
                    if (value > 3)
                        asmr.printWarning(func3ArgPlace,
                                  "StreamId (3rd argument) out of range");
                    streamId = value&3;
                }
            }
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end || *linePtr!=')')
            {
                asmr.printError(linePtr, "Unterminated sendmsg function call");
                return;
            }
            ++linePtr;
            imm16 = sendMessage | (gsopIndex<<4) | (streamId<<8);
            break;
        }
        case GCN_IMM_NONE:
            break;
        default:
            good &= parseImm<uint16_t>(asmr, linePtr, imm16, imm16Expr);
            break;
    }
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    uint32_t word;
    SLEV(word, 0xbf800000U | imm16 | (uint32_t(gcnInsn.code1)<<16));
    
    if (imm16Expr!=nullptr)
        imm16Expr->setTarget(AsmExprTarget(((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL) ?
                GCNTGT_SOPJMP : GCNTGT_SOPKSIMM16, asmr.currentSection, output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word), 
            reinterpret_cast<cxbyte*>(&word)+4);
    /// prevent freeing expression
    imm16Expr.release();
}

void GCNAsmUtils::parseSMRDEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    RegPair dstReg(0, 1);
    RegPair sbaseReg(0, 1);
    RegPair soffsetReg(0, 0);
    cxbyte soffsetVal = 0;
    std::unique_ptr<AsmExpression> soffsetExpr;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    if (mode1 == GCN_SMRD_ONLYDST)
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        good &= parseSRegRange(asmr, linePtr, dstReg, arch, dregsNum);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        
        good &= parseSRegRange(asmr, linePtr, sbaseReg, arch,
                   (gcnInsn.mode&GCN_SBASE4)?4:2);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!='@')
            good &= parseSRegRange(asmr, linePtr, soffsetReg, arch, 1, false);
        else
        {   // '@' prefix
            ++linePtr;
            skipSpacesToEnd(linePtr, end);
        }
        if (soffsetReg.first==0 && soffsetReg.second==0)
        {   // parse immediate
            soffsetReg.first = 255; // indicate an immediate
            good &= parseImm<cxbyte>(asmr, linePtr, soffsetVal, soffsetExpr);
        }
    }
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (soffsetExpr!=nullptr)
        soffsetExpr->setTarget(AsmExprTarget(GCNTGT_SMRDOFFSET, asmr.currentSection,
                       output.size()));
    
    uint32_t word;
    SLEV(word, 0xc0000000U | (uint32_t(gcnInsn.code1)<<22) | (dstReg.first<<15) |
            ((sbaseReg.first<<8)&~1) | ((soffsetReg.first==255) ? 0x100 : 0) |
            ((soffsetReg.first==255) ? soffsetVal : soffsetReg.first));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word), 
            reinterpret_cast<cxbyte*>(&word)+4);
    /// prevent freeing expression
    soffsetExpr.release();
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.second-1, arch);
}

enum:  cxbyte {
    VOP3_MUL2 = 1,
    VOP3_MUL4 = 2,
    VOP3_DIV2 = 3,
    VOP3_CLAMP = 16,
    VOP3_VOP3 = 32
};

bool GCNAsmUtils::parseVOP3Modifiers(Assembler& asmr, const char*& linePtr, cxbyte& mods,
                bool withClamp)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    mods = 0;
    while (linePtr != end)
    {
        char mod[6];
        const char* modPlace = linePtr;
        if (getNameArg(asmr, 6, mod, linePtr, "modifier"))
        {
            toLowerString(mod);
            try
            {
                bool alreadyModDefined = false;
                if (::strcmp(mod, "mul")==0)
                {
                    if (linePtr!=end && *linePtr==':')
                    {
                        ++linePtr;
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
                            asmr.printError(modPlace, "Unrecognized VOP3 mul:X modifier");
                            good = false;
                        }
                    }
                    else
                    {
                        asmr.printError(modPlace, "Expected ':' before multiplier number");
                        good = false;
                    }
                }
                else if (::strcmp(mod, "div")==0)
                {
                    if (linePtr!=end && *linePtr==':')
                    {
                        ++linePtr;
                        cxbyte count = cstrtobyte(linePtr, end);
                        if (count==2)
                        {
                            alreadyModDefined = mods&3;
                            mods = (mods&~3) | VOP3_DIV2;
                        }
                        else
                        {
                            asmr.printError(modPlace, "Unrecognized VOP3 div:X modifier");
                            good = false;
                        }
                    }
                    else
                    {
                        asmr.printError(modPlace, "Expected ':' before divider number");
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
                else
                {   /// unknown modifier
                    asmr.printError(modPlace, "Unrecognized VOP3 modifier");
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
        
        skipSpacesToEnd(linePtr, end);
    }
    return good;
}

void GCNAsmUtils::parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* argsPlace = linePtr;
    
    bool good = true;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    RegPair dstReg(0, 1);
    RegPair dstCCReg(0, 0);
    RegPair srcCCReg(0, 0);
    if (mode1 == GCN_DS1_SGPR) // if SGPRS as destination
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    else // if VGPRS as destination
        good &= parseVRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    
    const bool haveDstCC = mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC;
    const bool haveSrcCC = mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC;
    if (haveDstCC) /* VOP3b */
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseSRegRange(asmr, linePtr, dstCCReg, arch, 2);
    }
    
    GCNOperand src0Op{}, src1Op{};
    std::unique_ptr<AsmExpression> src0OpExpr, src1OpExpr;
    const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
            (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    good &= parseOperand(asmr, linePtr, src0Op, src0OpExpr, arch,
                (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, literalConstsFlags |
                INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|
                ((haveDstCC || haveSrcCC) ? INSTROP_VOP3NEG : INSTROP_VOP3MODS));
    
    uint32_t immValue = 0;
    std::unique_ptr<AsmExpression> immExpr;
    if (mode1 == GCN_ARG1_IMM)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseLiteralImm(asmr, linePtr, immValue, immExpr, literalConstsFlags);
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    good &= parseOperand(asmr, linePtr, src1Op, src1OpExpr, arch,
                (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, literalConstsFlags |
                INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|
                ((haveDstCC || haveSrcCC) ? INSTROP_VOP3NEG : INSTROP_VOP3MODS) |
                (src0Op.pair.first==255 ? INSTROP_ONLYINLINECONSTS : 0));
    
    if (mode1 == GCN_ARG2_IMM)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseLiteralImm(asmr, linePtr, immValue, immExpr, literalConstsFlags);
    }
    else if (haveSrcCC)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseSRegRange(asmr, linePtr, srcCCReg, arch, 2);
    }
    
    // modifiers
    cxbyte modifiers = 0;
    good &= parseVOP3Modifiers(asmr, linePtr, modifiers, !(haveDstCC || haveSrcCC));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const bool vop3 = (modifiers&VOP3_VOP3)!=0|| (src1Op.pair.first<256) ||
        src0Op.vop3Mods!=0 || src1Op.vop3Mods!=0 || modifiers!=0 ||
        /* srcCC!=VCC or dstCC!=VCC */
        (haveDstCC && dstCCReg.first!=106) || (haveSrcCC && srcCCReg.first!=106);
    
    cxuint maxSgprsNum = (arch&ARCH_RX3X0)?102:104;
    
    if ((src0Op.pair.first==255 || src1Op.pair.first==255) &&
        (src0Op.pair.first<maxSgprsNum || src0Op.pair.first==124 ||
         src1Op.pair.first<maxSgprsNum || src1Op.pair.first==124))
    {
        asmr.printError(argsPlace, "Literal constant with SGPR or M0 is illegal");
        return;
    }
    if (src0Op.pair.first<maxSgprsNum && src1Op.pair.first<maxSgprsNum &&
        src0Op.pair.first!=src1Op.pair.first)
    {   /* include VCCs (???) */
        asmr.printError(argsPlace, "More than one SGPR to read in instruction");
        return;
    }
    
    if (vop3)
    {   // use VOP3 encoding
        if (src0Op.pair.first==255 || src1Op.pair.first==255 ||
            mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM)
        {
            asmr.printError(argsPlace, "Literal in VOP3 encoding is illegal");
            return;
        }
    }
    
    if (src0OpExpr!=nullptr)
        src0OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    if (src1OpExpr!=nullptr)
        src1OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    if (immExpr!=nullptr)
        immExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    cxuint wordsNum = 1;
    uint32_t words[2];
    if (!vop3)
    {   // VOP2 encoding
        SLEV(words[0], (uint32_t(gcnInsn.code1)<<25) | uint32_t(src0Op.pair.first) |
            (uint32_t(src1Op.pair.first&0xff)<<9) | (uint32_t(dstReg.first&0xff)<<17));
        if (src0Op.pair.first==255)
            SLEV(words[wordsNum++], src0Op.value);
        else if (src1Op.pair.first==255)
            SLEV(words[wordsNum++], src1Op.value);
        else if (mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM)
            SLEV(words[wordsNum++], immValue);
    }
    else
    {   // VOP3 encoding
        if (haveDstCC || haveSrcCC)
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code2)<<17) |
                (dstReg.first&0xff) | (uint32_t(dstCCReg.first)<<8));
        else
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code2)<<17) |
                (dstReg.first&0xff) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0) |
                ((src0Op.vop3Mods & VOPOPFLAG_ABS) ? 0x100 : 0) |
                ((src1Op.vop3Mods & VOPOPFLAG_ABS) ? 0x200 : 0));
        SLEV(words[1], src0Op.pair.first | (uint32_t(src1Op.pair.first)<<9) |
            (uint32_t(srcCCReg.first)<<18) | ((modifiers & 3) << 27) |
            ((src0Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<29) : 0) |
            ((src1Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<30) : 0));
        wordsNum++;
    }
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words) + (wordsNum<<2));
    /// prevent freeing expression
    src0OpExpr.release();
    src1OpExpr.release();
    immExpr.release();
    // update register pool
    if (dstReg.first>=256)
        updateVGPRsNum(gcnRegs.vgprsNum, dstReg.second-257);
    else // sgprs
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.second-1, arch);
    updateSGPRsNum(gcnRegs.sgprsNum, dstCCReg.second-1, arch);
}

void GCNAsmUtils::parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseMXBUFEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

void GCNAsmUtils::parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
}

};

void GCNAssembler::assemble(const CString& mnemonic, const char* mnemPlace,
            const char* linePtr, const char* lineEnd, std::vector<cxbyte>& output)
{
    auto it = binaryFind(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
               GCNAsmInstruction{mnemonic.c_str()},
               [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
               { return ::strcmp(instr1.mnemonic, instr2.mnemonic)<0; });
    
    // find matched entry
    if (it != gcnInstrSortedTable.end() && (it->archMask & curArchMask)==0)
        // if not match current arch mask
        for (++it ;it != gcnInstrSortedTable.end() &&
               ::strcmp(it->mnemonic, mnemonic.c_str())==0 &&
               (it->archMask & curArchMask)==0; ++it);

    if (it == gcnInstrSortedTable.end() || ::strcmp(it->mnemonic, mnemonic.c_str())!=0)
    {   // unrecognized mnemonic
        printError(mnemPlace, "Unrecognized instruction");
        return;
    }
    
    /* decode instruction line */
    switch(it->encoding1)
    {
        case GCNENC_SOPC:
            GCNAsmUtils::parseSOPCEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_SOPP:
            GCNAsmUtils::parseSOPPEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_SOP1:
            GCNAsmUtils::parseSOP1Encoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_SOP2:
            GCNAsmUtils::parseSOP2Encoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_SOPK:
            GCNAsmUtils::parseSOPKEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_SMRD:
            GCNAsmUtils::parseSMRDEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VOPC:
            GCNAsmUtils::parseVOPCEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VOP1:
            GCNAsmUtils::parseVOP1Encoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VOP2:
            GCNAsmUtils::parseVOP2Encoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VOP3A:
        case GCNENC_VOP3B:
            GCNAsmUtils::parseVOP3Encoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VINTRP:
            GCNAsmUtils::parseVINTRPEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_DS:
            GCNAsmUtils::parseDSEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_MUBUF:
        case GCNENC_MTBUF:
            GCNAsmUtils::parseMXBUFEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_MIMG:
            GCNAsmUtils::parseMIMGEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_EXP:
            GCNAsmUtils::parseEXPEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_FLAT:
            GCNAsmUtils::parseFLATEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        default:
            break;
    }
}

bool GCNAssembler::resolveCode(const AsmSourcePos& sourcePos, cxuint targetSectionId,
             cxbyte* sectionData, size_t offset, AsmExprTargetType targetType,
             cxuint sectionId, uint64_t value)
{
    switch(targetType)
    {
        case GCNTGT_LITIMM:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, "Relative value is illegal in literal expressions");
                return false;
            }
            SULEV(*reinterpret_cast<uint32_t*>(sectionData+offset+4), value);
            printWarningForRange(32, value, sourcePos);
            return true;
        case GCNTGT_SOPKSIMM16:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, "Relative value is illegal in immediate expressions");
                return false;
            }
            SULEV(*reinterpret_cast<uint16_t*>(sectionData+offset), value);
            printWarningForRange(16, value, sourcePos);
            return true;
        case GCNTGT_SOPJMP:
        {
            if (sectionId != targetSectionId)
            {   // if jump outside current section (.text)
                printError(sourcePos, "Jump over current section!");
                return false;
            }
            int64_t outOffset = (int64_t(value)-int64_t(offset)-4);
            if (outOffset & 3)
            {
                printError(sourcePos, "Jump is not aligned to word!");
                return false;
            }
            outOffset >>= 2;
            if (outOffset > INT16_MAX || outOffset < INT16_MIN)
            {
                printError(sourcePos, "Jump out of range!");
                return false;
            }
            SULEV(*reinterpret_cast<uint16_t*>(sectionData+offset), outOffset);
            return true;
        }
        case GCNTGT_SMRDOFFSET:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, "Relative value is illegal in offset expressions");
                return false;
            }
            sectionData[offset] = value;
            printWarningForRange(8, value, sourcePos);
            return true;
        default:
            return false;
    }
}

bool GCNAssembler::checkMnemonic(const CString& mnemonic) const
{
    return std::binary_search(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
               GCNAsmInstruction{mnemonic.c_str()},
               [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
               { return ::strcmp(instr1.mnemonic, instr2.mnemonic)<0; });
}

const cxuint* GCNAssembler::getAllocatedRegisters(size_t& regTypesNum) const
{
    regTypesNum = 2;
    return regTable;
}
