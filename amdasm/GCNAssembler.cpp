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
        sgprsNum(0), vgprsNum(0), curArchMask(
                    1U<<cxuint(getGPUArchitectureFromDeviceType(assembler.getDeviceType())))
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

bool GCNAsmUtils::parseVRegRange(Assembler& asmr, const char*& linePtr, RegPair& regPair,
                    cxuint regsNum, bool required)
{
    const char* oldLinePtr = linePtr;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* vgprRangePlace = linePtr;
    if (linePtr == end)
    {
        if (required)
        {
            asmr.printError(vgprRangePlace, "VRegister range is required");
            return false;
        }
        regPair = { 0, 0 };
        linePtr = oldLinePtr;
        return true;
    }
    if (toLower(*linePtr) != 'v') // if
    {
        if (required)
        {
            asmr.printError(vgprRangePlace, "VRegister range is required");
            return false;
        }
        regPair = { 0, 0 };
        linePtr = oldLinePtr;
        return true;
    }
    if (++linePtr == end)
    {
        if (required)
        {
            asmr.printError(vgprRangePlace, "VRegister range is required");
            return false;
        }
        regPair = { 0, 0 };
        linePtr = oldLinePtr;
        return true;
    }
    
    try /* for handling parse exception */
    {
    if (isDigit(*vgprRangePlace))
    {   // if single register
        cxuint value = cstrtobyte(linePtr, end);
        if (value >= 256)
        {
            asmr.printError(vgprRangePlace, "VRegister number of out range (0-255)");
            return false;
        }
        if (regsNum!=0 && regsNum != 1)
        {
            char buf[64];
            snprintf(buf, 64, "Required %u VRegisters", regsNum);
            asmr.printError(vgprRangePlace, buf);
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
            asmr.printError(vgprRangePlace, "Unterminated VRegister range");
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
        
        if (value2 < value1 || value2 >= 256 || value1 >= 256)
        {   // error (illegal register range)
            asmr.printError(vgprRangePlace, "Illegal VRegister range");
            return false;
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
        {   // error
            asmr.printError(vgprRangePlace, "Unterminated VRegister range");
            return false;
        }
        ++linePtr;
        
        if (regsNum!=0 && regsNum != value2-value1+1)
        {
            char buf[64];
            snprintf(buf, 64, "Required %u VRegisters", regsNum);
            asmr.printError(vgprRangePlace, buf);
            return false;
        }
        regPair = { 256+value1, 256+value2+1 };
        return true;
    }
    } catch(const ParseException& ex)
    { asmr.printError(linePtr, ex.what()); }
    
    if (required)
    {
        asmr.printError(vgprRangePlace, "Required VRegisters range");
        return false;
    }
    regPair = { 0, 0 };
    linePtr = oldLinePtr;
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
        if (required)
        {
            asmr.printError(sgprRangePlace, "SRegister range is required");
            return false;
        }
        regPair = { 0, 0 };
        linePtr = oldLinePtr;
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
                char buf[64];
                snprintf(buf, 64, "Required %u SRegisters", regsNum);
                asmr.printError(sgprRangePlace, buf);
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
                    char buf[64];
                    snprintf(buf, 64, "Required %u XRegisters", regsNum);
                    asmr.printError(sgprRangePlace, buf);
                    return false;
                }
                return true;
            }
            else if (regName[loHiRegSuffix] == 0)
            {
                if (regsNum!=0 && regsNum != 2)
                {
                    char buf[64];
                    snprintf(buf, 64, "Required %u XRegisters", regsNum);
                    asmr.printError(sgprRangePlace, buf);
                    return false;
                }
                regPair = { loHiReg, loHiReg+2 };
                return true;
            }
            else
            {   // this is not this register
                linePtr = oldLinePtr;
                regPair = { 0, 0 };
                return true;
            }
        }
        else if (!ttmpReg)
        {   // otherwise
            if (required)
            {
                asmr.printError(sgprRangePlace, "SRegister range is required");
                return false;
            }
            regPair = { 0, 0 };
            linePtr = oldLinePtr;
            return true;
        }
    }
    linePtr += (ttmpReg)?4:1; // skip
    if (linePtr == end)
    {
        if (required)
        {
            asmr.printError(sgprRangePlace, "SRegister range is required");
            return false;
        }
        regPair = { 0, 0 };
        linePtr = oldLinePtr;
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
                asmr.printError(sgprRangePlace, "SRegister number of out range");
                return false;
            }
        }
        else
        {
            if (value >= 12)
            {
                asmr.printError(sgprRangePlace, "TTMPRegister number of out range (0-11)");
                return false;
            }
        }
        if (regsNum!=0 && regsNum != 1)
        {
            char buf[64];
            snprintf(buf, 64, "Required %u SRegisters", regsNum);
            asmr.printError(sgprRangePlace, buf);
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
            asmr.printError(sgprRangePlace, "Unterminated SRegister range");
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
        
        if (!ttmpReg)
        {
            if (value2 < value1 || value1 >= maxSGPRsNum || value2 >= maxSGPRsNum)
            {   // error (illegal register range)
                asmr.printError(sgprRangePlace, "Illegal SRegister range");
                return false;
            }
        }
        else
        {
            if (value2 < value1 || value1 >= 12 || value2 >= 12)
            {   // error (illegal register range)
                asmr.printError(sgprRangePlace, "Illegal TTMPRegister range");
                return false;
            }
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
        {   // error
            asmr.printError(sgprRangePlace, (!ttmpReg) ? "Unterminated SRegister range" :
                        "Unterminated TTMPRegister range");
            return false;
        }
        ++linePtr;
        
        if (regsNum!=0 && regsNum != value2-value1+1)
        {
            char buf[64];
            snprintf(buf, 64, "Required %u Registers", regsNum);
            asmr.printError(sgprRangePlace, buf);
            return false;
        }
        /// check alignment
        if (!ttmpReg)
            if ((value2-value1==1 && (value1&1)!=0) ||
                (value2-value1>1 && (value1&3)!=0))
            {
                asmr.printError(sgprRangePlace, "Unaligned SRegister range");
                return false;
            }
        if (!ttmpReg)
            regPair = { value1, uint16_t(value2)+1 };
        else
            regPair = { 112+value1, 112+uint16_t(value2)+1 };
        return true;
    }
    } catch(const ParseException& ex)
    { asmr.printError(linePtr, ex.what()); }
    
    if (required)
    {
        asmr.printError(sgprRangePlace, "Required SRegisters range");
        return false;
    }
    regPair = { 0, 0 };
    linePtr = oldLinePtr;
    return true;
}

bool GCNAsmUtils::parseImm16(Assembler& asmr, const char*& linePtr, uint16_t& value16,
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
        asmr.printWarningForRange(16, value, asmr.getSourcePos(exprPlace));
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

bool GCNAsmUtils::parseOperand(Assembler& asmr, const char*& linePtr, GCNOperand& operand,
             std::unique_ptr<AsmExpression>& outTargetExpr, uint16_t arch,
             cxuint regsNum, Flags instrOpMask)
{
    outTargetExpr.reset();
    
    if (instrOpMask == INSTROP_SREGS)
        return parseSRegRange(asmr, linePtr, operand.pair, arch, regsNum);
    else if (instrOpMask == INSTROP_VREGS)
        return parseVRegRange(asmr, linePtr, operand.pair, regsNum);
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
    
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    if ((instrOpMask & INSTROP_SSOURCE)!=0)
    {   char regName[20];
        const char* regNamePlace = linePtr;
        if (getNameArg(asmr, 20, regName, linePtr, "register name", false, false))
        {
            toLowerString(regName);
            operand.pair = {0, 0};
            if (::memcmp(regName, "vccz", 5) == 0)
            {
                operand = { { 251, 252 } };
                return true;
            }
            else if (::memcmp(regName, "execz", 6) == 0)
            {
                operand = { { 252, 253 } };
                return true;
            }
            else if (::memcmp(regName, "scc", 4) == 0)
            {
                operand = { { 253, 254 } };
                return true;
            }
            if (operand.pair.first!=0 || operand.pair.second!=0)
            {
                if (regsNum!=0 && regsNum!=1 && regsNum!=2)
                {
                    char buf[64];
                    snprintf(buf, 64, "Required %u SRegisters", regsNum);
                    asmr.printError(regNamePlace, buf);
                    return false;
                }
                return true;
            }
            /* check expression, back to before regName */
            linePtr = regNamePlace;
        }
        // treat argument as expression
        bool forceExpression = false;
        if (linePtr!=end && *linePtr=='@')
        {
            forceExpression = true;
            linePtr++;
        }
        skipSpacesToEnd(linePtr, end);
        const char* exprPlace = linePtr;
        
        uint64_t value;
        if (!forceExpression && isOnlyFloat(linePtr, end))
        {   // if only floating point value
            /* if floating point literal can be processed */
            try
            {
                if ((instrOpMask & INSTROP_TYPE_MASK) == INSTROP_F16)
                {
                    value = cstrtohCStyle(linePtr, end, linePtr);
                    switch (value)
                    {
                        case 0x0:
                            operand = { { 128, 0 } };
                            return true;
                        case 0x3800: // 0.5
                            operand = { { 240, 0 } };
                            return true;
                        case 0xb800: // -0.5
                            operand = { { 241, 0 } };
                            return true;
                        case 0x3c00: // 1.0
                            operand = { { 242, 0 } };
                            return true;
                        case 0xbc00: // -1.0
                            operand = { { 243, 0 } };
                            return true;
                        case 0x4000: // 2.0
                            operand = { { 244, 0 } };
                            return true;
                        case 0xc000: // -2.0
                            operand = { { 245, 0 } };
                            return true;
                        case 0x4400: // 4.0
                            operand = { { 246, 0 } };
                            return true;
                        case 0xc400: // -4.0
                            operand = { { 247, 0 } };
                            return true;
                        case 0x3118: // 1/(2*PI)
                            if (arch&ARCH_RX3X0)
                            {
                                operand = { { 248, 0 } };
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
                            operand = { { 128, 0 } };
                            return true;
                        case 0x3f000000: // 0.5
                            operand = { { 240, 0 } };
                            return true;
                        case 0xbf000000: // -0.5
                            operand = { { 241, 0 } };
                            return true;
                        case 0x3f800000: // 1.0
                            operand = { { 242, 0 } };
                            return true;
                        case 0xbf800000: // -1.0
                            operand = { { 243, 0 } };
                            return true;
                        case 0x40000000: // 2.0
                            operand = { { 244, 0 } };
                            return true;
                        case 0xc0000000: // -2.0
                            operand = { { 245, 0 } };
                            return true;
                        case 0x40800000: // 4.0
                            operand = { { 246, 0 } };
                            return true;
                        case 0xc0800000: // -4.0
                            operand = { { 247, 0 } };
                            return true;
                        case 0x3e22f983: // 1/(2*PI)
                            if (arch&ARCH_RX3X0)
                            {
                                operand = { { 248, 0 } };
                                return true;
                            }
                    }
                }
                
            }
            catch(const ParseException& ex)
            {
                asmr.printError(regNamePlace, ex.what());
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
                            "Literal constant is illegal in this place");
                    return false;
                }
                outTargetExpr = std::move(expr);
                operand = { { 255, 0 } };
                return true;
            }
            
            if (value <= 64)
            {
                operand = { { 128+value, 0 } };
                return true;
            }
            else if (int64_t(value) >= -16 && int64_t(value) < 0)
            {
                operand = { { 192-value, 0 } };
                return true;
            }
        }
        
        if ((instrOpMask & INSTROP_ONLYINLINECONSTS)!=0)
        {   // error
            asmr.printError(regNamePlace, "Literal constant is illegal in this place");
            return false;
        }
        
        // not in range
        asmr.printWarningForRange(32, value, asmr.getSourcePos(regNamePlace));
        operand = { { 255, 0 }, uint32_t(value) };
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

void GCNAsmUtils::parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
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
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
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
}

void GCNAsmUtils::parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
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
    {
        good &= parseOperand(asmr, linePtr, src0Op, src0Expr, arch,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS);
        if (!skipRequiredComma(asmr, linePtr))
            return;
    }
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0xbe800000U | (uint32_t(gcnInsn.code1)<<16) | src0Op.pair.first |
            uint32_t(dstReg.first)<<16);
    // set expression targets
    if (src0Expr!=nullptr)
        src0Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words + wordsNum));
    // prevent freeing expressions
    src0Expr.release();
}

static const std::pair<const char*, uint16_t> hwregNamesMap[] =
{
    { "0", 0 },
    { "mode", 1 },
    { "status", 2 },
    { "trapsts", 3 },
    { "hw_id", 4 },
    { "gpr_alloc", 5 },
    { "lds_alloc", 6 },
    { "ib_sts", 7 },
    { "pc_lo", 8 },
    { "pc_hi", 9 },
    { "inst_dw0", 10 },
    { "inst_dw1", 11 },
    { "ib_dbg0", 12 }
};

void GCNAsmUtils::parseSOPKEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    bool good = true;
    RegPair dstReg(0, 1);
    if ((gcnInsn.mode & GCN_MASK1) == GCN_IMM_DST)
    {
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        if (!skipRequiredComma(asmr, linePtr))
            return;
    }
    
    uint16_t imm16;
    if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL)
    {
        uint64_t value;
        if (!getJumpValueArg(asmr, value, linePtr))
            return;
        value = (value-output.size()-4)>>2;
        if (value > INT16_MAX || value < INT16_MIN)
        {
            asmr.printError(linePtr, "Jump out of range");
            good = false;
        }
        imm16 =value;
    }
    else if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_SREG)
    {
    }
    else
    {
    }
}

void GCNAsmUtils::parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
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
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
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

void GCNAsmUtils::parseSOPPEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseSMRDEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseMXBUFEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
{
}

void GCNAsmUtils::parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& insn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output)
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
            GCNAsmUtils::parseSOPCEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_SOPP:
            GCNAsmUtils::parseSOPPEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_SOP1:
            GCNAsmUtils::parseSOP1Encoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_SOP2:
            GCNAsmUtils::parseSOP2Encoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_SOPK:
            GCNAsmUtils::parseSOPKEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_SMRD:
            GCNAsmUtils::parseSMRDEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_VOPC:
            GCNAsmUtils::parseVOPCEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_VOP1:
            GCNAsmUtils::parseVOP1Encoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_VOP2:
            GCNAsmUtils::parseVOP2Encoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_VOP3A:
        case GCNENC_VOP3B:
            GCNAsmUtils::parseVOP3Encoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_VINTRP:
            GCNAsmUtils::parseVINTRPEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_DS:
            GCNAsmUtils::parseDSEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_MUBUF:
        case GCNENC_MTBUF:
            GCNAsmUtils::parseMXBUFEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_MIMG:
            GCNAsmUtils::parseMIMGEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_EXP:
            GCNAsmUtils::parseEXPEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        case GCNENC_FLAT:
            GCNAsmUtils::parseFLATEncoding(assembler, *it, linePtr, curArchMask, output);
            break;
        default:
            break;
    }
}

bool GCNAssembler::resolveCode(cxbyte* location, AsmExprTargetType targetType,
                   uint64_t value)
{
    switch(targetType)
    {
        case GCNTGT_LITIMM:
            SULEV(*reinterpret_cast<uint32_t*>(location+4), value);
            return true;
    }
    return false;
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
