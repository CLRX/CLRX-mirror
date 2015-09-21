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
        gcnInstrSortedTable[i] = {insn.mnemonic, insn.encoding, insn.mode,
                    insn.code, UINT16_MAX, insn.archMask};
    }
    
    std::sort(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
            [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
            {   // compare mnemonic and if mnemonic
                int r = ::strcmp(instr1.mnemonic, instr2.mnemonic);
                return (r < 0) || (r==0 && instr1.encoding < instr2.encoding) ||
                            (r == 0 && instr1.encoding == instr2.encoding &&
                             instr1.archMask < instr2.archMask);
            });
    
    cxuint j = 0;
    /* join VOP3A instr with VOP2/VOPC/VOP1 instr together to faster encoding. */
    for (cxuint i = 0; i < tableSize; i++)
    {   
        GCNAsmInstruction insn = gcnInstrSortedTable[i];
        if ((insn.encoding == GCNENC_VOP3A || insn.encoding == GCNENC_VOP3B))
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
                    //gcnInstrSortedTable[k].encoding2 = insn.encoding1;
                    gcnInstrSortedTable[k].archMask &= insn.archMask;
                }
                else
                {   // if filled we create new entry
                    gcnInstrSortedTable[j] = gcnInstrSortedTable[k];
                    gcnInstrSortedTable[j].archMask &= insn.archMask;
                    //gcnInstrSortedTable[j].encoding2 = insn.encoding1;
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

bool GCNAsmUtils::parseVRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
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
            skipCharAndSpacesToEnd(linePtr, end);
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
    if (linePtr+4 <= end && toLower(linePtr[0]) == 't' &&
        toLower(linePtr[1]) == 't' && toLower(linePtr[2]) == 'm' &&
        toLower(linePtr[3]) == 'p')
        ttmpReg = true; // we have ttmp registers
    else if (toLower(*linePtr) != 's') // if not sgprs
    {
        const char* oldLinePtr = linePtr;
        char regName[20];
        if (!getNameArg(asmr, 20, regName, linePtr, "register name", required, true))
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
            skipCharAndSpacesToEnd(linePtr, end);
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
            std::unique_ptr<AsmExpression>* outTargetExpr, cxuint bits, cxbyte signess)
{
    const char* end = asmr.line+asmr.lineSize;
    if (outTargetExpr!=nullptr)
        outTargetExpr->reset();
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
        if (bits == 0)
            bits = sizeof(T)<<3;
        asmr.printWarningForRange(bits, value, asmr.getSourcePos(exprPlace), signess);
        outValue = value & ((1ULL<<bits)-1ULL);
        return true;
    }
    else
    {   // return output expression with symbols to resolve
        if (outTargetExpr!=nullptr)
            *outTargetExpr = std::move(expr);
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

template<typename T>
bool GCNAsmUtils::parseModImm(Assembler& asmr, const char*& linePtr, T& value,
        std::unique_ptr<AsmExpression>* outTargetExpr, const char* modName, cxuint bits,
        cxbyte signess)
{
    if (outTargetExpr!=nullptr)
        outTargetExpr->reset();
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr!=end && *linePtr==':')
    {
        skipCharAndSpacesToEnd(linePtr, end);
        return parseImm(asmr, linePtr, value, outTargetExpr, bits, signess);
    }
    asmr.printError(linePtr, (std::string("Expected ':' before ")+modName).c_str());
    return false;
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
        operand.vop3Mods = 0;
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
                operand.vop3Mods |= VOPOPFLAG_SEXT;
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
        if ((operand.vop3Mods&(VOPOPFLAG_NEG|VOPOPFLAG_ABS)) != VOPOPFLAG_NEG)
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
                asmr.printError(linePtr, "Unterminated abs() modifier");
                return false;
            }
        }
        if (operand.vop3Mods & VOPOPFLAG_SEXT)
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
        operand.vop3Mods = 0; // clear modifier (VOP3NEG)
        /// PARSEWITHNEG used to continuing operand parsing with modifiers
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
                (::strcmp(regName, "lds")==0 || ::strcmp(regName, "lds_direct")==0))
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
            else
                asmr.printError(regNamePlace,
                        "Only one literal can be used in instruction");
            return false;
        }
        
        // not in range
        asmr.printWarningForRange(32, value, asmr.getSourcePos(regNamePlace));
        operand = { { 255, 0 }, uint32_t(value), operand.vop3Mods };
        return true;
    }
    
    // check otherwise
    asmr.printError(linePtr, "Unknown operand");
    return false;
}

static inline bool isXRegRange(RegRange pair, cxuint regsNum = 1)
{   // second==0 - we assume that first is inline constant, otherwise we check range
    return (pair.end==0) || cxuint(pair.end-pair.start)==regsNum;
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
    bool good = true;
    RegRange dstReg(0, 0);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_REG_S1_JMP)
    {
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        if (!skipRequiredComma(asmr, linePtr))
            return;
    }
    
    std::unique_ptr<AsmExpression> src0Expr, src1Expr;
    GCNOperand src0Op{};
    good &= parseOperand(asmr, linePtr, src0Op, &src0Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    GCNOperand src1Op{};
    good &= parseOperand(asmr, linePtr, src1Op, &src1Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
             (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0));
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // put data
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0x80000000U | (uint32_t(gcnInsn.code1)<<23) | src0Op.range.start |
            (src1Op.range.start<<8) | uint32_t(dstReg.start)<<16);
    if (src0Op.range.start==255 || src1Op.range.start==255)
    {
        if (src0Expr==nullptr && src1Expr==nullptr)
            SLEV(words[1], src0Op.range.start==255 ? src0Op.value : src1Op.value);
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
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
}

void GCNAsmUtils::parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    bool good = true;
    RegRange dstReg(0, 0);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
    {
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
            if (!skipRequiredComma(asmr, linePtr))
                return;
    }
    
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0Expr;
    if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
        good &= parseOperand(asmr, linePtr, src0Op, &src0Expr, arch,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS);
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0xbe800000U | (uint32_t(gcnInsn.code1)<<8) | src0Op.range.start |
            uint32_t(dstReg.start)<<16);
    if (src0Op.range.start==255)
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
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
}

static const std::pair<const char*, cxuint> hwregNamesMap[] =
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
    bool good = true;
    RegRange dstReg(0, 0);
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
        cxuint hwregId = 0;
        good &= getEnumeration(asmr, linePtr, "HWRegister",
                      hwregNamesMapSize, hwregNamesMap, hwregId, "hwreg_");
        
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
            asmr.printError(linePtr, "Unterminated hwreg function");
            return;
        }
        ++linePtr;
        imm16 = hwregId | (arg2Value<<6) | ((arg3Value-1)<<11);
    }
    else // otherwise we parse expression
        good &= parseImm(asmr, linePtr, imm16, &imm16Expr);
    
    uint32_t imm32 = 0;
    std::unique_ptr<AsmExpression> imm32Expr;
    if (gcnInsn.mode & GCN_IMM_DST)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        if (gcnInsn.mode & GCN_SOPK_CONST)
            good &= parseImm(asmr, linePtr, imm32, &imm32Expr);
        else
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const cxuint wordsNum = (gcnInsn.mode & GCN_SOPK_CONST) ? 2 : 1;
    uint32_t words[2];
    SLEV(words[0], 0xb0000000U | imm16 | uint32_t(dstReg.start)<<16 |
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
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
}

void GCNAsmUtils::parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    bool good = true;
    std::unique_ptr<AsmExpression> src0Expr, src1Expr;
    GCNOperand src0Op{};
    good &= parseOperand(asmr, linePtr, src0Op, &src0Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    GCNOperand src1Op{};
    if ((gcnInsn.mode & GCN_SRC1_IMM) == 0)
        good &= parseOperand(asmr, linePtr, src1Op, &src1Expr, arch,
                 (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
                 (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0));
    else // immediate
        good &= parseImm(asmr, linePtr, src1Op.range.start, &src1Expr, 8);
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    // put data
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0xbf000000U | (uint32_t(gcnInsn.code1)<<16) | src0Op.range.start |
            (src1Op.range.start<<8));
    if (src0Op.range.start==255 ||
        ((gcnInsn.mode & GCN_SRC1_IMM)==0 && src1Op.range.start==255))
    {
        if (src0Expr==nullptr && src1Expr==nullptr)
            SLEV(words[1], src0Op.range.start==255 ? src0Op.value : src1Op.value);
        else    // zero if unresolved value
            SLEV(words[1], 0);
        wordsNum++;
    }
    // set expression targets
    if (src0Expr!=nullptr)
        src0Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    else if (src1Expr!=nullptr)
        src1Expr->setTarget(AsmExprTarget(
            ((gcnInsn.mode&GCN_SRC1_IMM)) ? GCNTGT_SOPCIMM8 : GCNTGT_LITIMM,
            asmr.currentSection, output.size()));
    
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
                skipSpacesToEnd(linePtr, end);
                const char* funcNamePlace = linePtr;
                good &= getNameArgS(asmr, 20, name, linePtr, "function name", true);
                toLowerString(name);
                
                cxuint bitPos = 0, bitMask = UINT_MAX;
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
                    good = false;
                }
                
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end || *linePtr!='(')
                {
                    asmr.printError(funcNamePlace, "Expected vmcnt, lgkmcnt or expcnt");
                    return;
                }
                skipCharAndSpacesToEnd(linePtr, end);
                const char* argPlace = linePtr;
                uint64_t value;
                if (getAbsoluteValueArg(asmr, value, linePtr, true))
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
                    asmr.printError(linePtr, "Unterminated function");
                    return;
                }
                // ampersand
                skipCharAndSpacesToEnd(linePtr, end);
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
            skipCharAndSpacesToEnd(linePtr, end);
            
            const char* funcArg1Place = linePtr;
            size_t sendMessage = 0;
            if (getNameArg(asmr, 20, name, linePtr, "message name", true))
            {
                toLowerString(name);
                const size_t msgNameIndex = (::strncmp(name, "msg_", 4) == 0) ? 4 : 0;
                size_t index = binaryMapFind(sendMessageNamesMap,
                         sendMessageNamesMap + sendMessageNamesMapSize,
                         name+msgNameIndex, CStringLess()) - sendMessageNamesMap;
                if (index != sendMessageNamesMapSize)
                    sendMessage = sendMessageNamesMap[index].second;
                else
                {
                    asmr.printError(funcArg1Place, "Unknown message");
                    good = false;
                }
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
                        asmr.printError(funcArg2Place, "Unknown GSOP");
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
                asmr.printError(linePtr, "Unterminated sendmsg function");
                return;
            }
            ++linePtr;
            imm16 = sendMessage | (gsopIndex<<4) | (streamId<<8);
            break;
        }
        case GCN_IMM_NONE:
            break;
        default:
            good &= parseImm(asmr, linePtr, imm16, &imm16Expr);
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
    bool good = true;
    RegRange dstReg(0, 0);
    RegRange sbaseReg(0, 0);
    RegRange soffsetReg(0, 0);
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
        else // '@' prefix
            skipCharAndSpacesToEnd(linePtr, end);
        
        if (!soffsetReg)
        {   // parse immediate
            soffsetReg.start = 255; // indicate an immediate
            good &= parseImm(asmr, linePtr, soffsetVal, &soffsetExpr, 0, WS_UNSIGNED);
        }
    }
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (soffsetExpr!=nullptr)
        soffsetExpr->setTarget(AsmExprTarget(GCNTGT_SMRDOFFSET, asmr.currentSection,
                       output.size()));
    
    uint32_t word;
    SLEV(word, 0xc0000000U | (uint32_t(gcnInsn.code1)<<22) | (uint32_t(dstReg.start)<<15) |
            ((sbaseReg.start<<8)&~1) | ((soffsetReg.start==255) ? 0x100 : 0) |
            ((soffsetReg.start==255) ? soffsetVal : soffsetReg.start));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word), 
            reinterpret_cast<cxbyte*>(&word)+4);
    /// prevent freeing expression
    soffsetExpr.release();
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
}

void GCNAsmUtils::parseSMEMEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    RegRange dataReg(0, 0);
    RegRange sbaseReg(0, 0);
    RegRange soffsetReg(0, 0);
    uint32_t soffsetVal = 0;
    std::unique_ptr<AsmExpression> soffsetExpr;
    std::unique_ptr<AsmExpression> simm7Expr;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    if (mode1 == GCN_SMRD_ONLYDST)
        good &= parseSRegRange(asmr, linePtr, dataReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        if ((mode1 & GCN_SMEM_SDATA_IMM)==0)
            good &= parseSRegRange(asmr, linePtr, dataReg, arch, dregsNum);
        else
            good &= parseImm(asmr, linePtr, dataReg.start, &simm7Expr, 7);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        
        good &= parseSRegRange(asmr, linePtr, sbaseReg, arch,
                   (gcnInsn.mode&GCN_SBASE4)?4:2);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!='@')
            good &= parseSRegRange(asmr, linePtr, soffsetReg, arch, 1, false);
        else // '@' prefix
            skipCharAndSpacesToEnd(linePtr, end);
        
        if (!soffsetReg)
        {   // parse immediate
            soffsetReg.start = 255; // indicate an immediate
            good &= parseImm(asmr, linePtr, soffsetVal, &soffsetExpr, 20, WS_UNSIGNED);
        }
    }
    bool haveGlc = false;
    // modifiers
    while (linePtr != end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end)
            break;
        const char* modPlace = linePtr;
        char name[10];
        if (getNameArgS(asmr, 10, name, linePtr, "modifier"))
        {
            toLowerString(name);
            if (::strcmp(name, "glc")==0)
                haveGlc = true;
            else
            {
                asmr.printError(modPlace, "Unknown SMEM modifier");
                good = false;
            }
        }
        else
            good = false;
    }
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (soffsetExpr!=nullptr)
        soffsetExpr->setTarget(AsmExprTarget(GCNTGT_SMEMOFFSET, asmr.currentSection,
                       output.size()));
    if (simm7Expr!=nullptr)
        simm7Expr->setTarget(AsmExprTarget(GCNTGT_SMEMIMM, asmr.currentSection,
                       output.size()));
    
    uint32_t words[2];
    SLEV(words[0], 0xc0000000U | (uint32_t(gcnInsn.code1)<<18) | (dataReg.start<<6) |
            (sbaseReg.start>>1) | ((soffsetReg.start==255) ? 0x20000 : 0) |
            (haveGlc ? 0x10000 : 0));
    SLEV(words[1], ((soffsetReg.start==255) ? soffsetVal : soffsetReg.start));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words+2));
    /// prevent freeing expression
    soffsetExpr.release();
    simm7Expr.release();
    if (gcnInsn.mode & GCN_MLOAD)
        updateSGPRsNum(gcnRegs.sgprsNum, dataReg.end-1, arch);
}

static const std::pair<const char*, cxuint> vopSDWADSTSelNamesMap[] =
{
    { "b0", 0 },
    { "b1", 1 },
    { "b2", 2 },
    { "b3", 3 },
    { "byte_0", 0 },
    { "byte_1", 1 },
    { "byte_2", 2 },
    { "byte_3", 3 },
    { "dword", 6 },
    { "w0", 4 },
    { "w1", 5 },
    { "word_0", 4 },
    { "word_1", 5 }
};

bool GCNAsmUtils::parseVOPModifiers(Assembler& asmr, const char*& linePtr, cxbyte& mods,
                VOPExtraModifiers* extraMods, bool withClamp, bool withVOPSDWA_DPP)
{
    const char* end = asmr.line+asmr.lineSize;
    //bool haveSDWAMod = false, haveDPPMod = false;
    bool haveDstSel = false, haveSrc0Sel = false, haveSrc1Sel = false;
    bool haveBankMask = false, haveRowMask = false;
    bool haveBoundCtrl = false, haveDppCtrl = false;
    
    bool good = true;
    mods = 0;
    while (linePtr != end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end)
            break;
        char mod[6];
        const char* modPlace = linePtr;
        if (getNameArgS(asmr, 6, mod, linePtr, "VOP modifier"))
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
                    if (::strcmp(mod, "dst_sel")==0)
                    {   // dstsel
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint dstSel = 0;
                            if (getEnumeration(asmr, linePtr, "dst_sel", 6,
                                        vopSDWADSTSelNamesMap, dstSel))
                            {
                                extraMods->dstSelUnused =
                                        (extraMods->dstSelUnused&~7) | dstSel;
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
                    else if (::strcmp(mod, "dst_unused")==0 || ::strcmp(mod, "dst_un")==0)
                    {
                        //cxbyte dstUn;
                    }
                    else if (::strcmp(mod, "src0_sel")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint src0Sel = 0;
                            if (getEnumeration(asmr, linePtr, "src0_sel", 6,
                                        vopSDWADSTSelNamesMap, src0Sel))
                            {
                                extraMods->dstSelUnused = (extraMods->src01Sel&~7) |
                                            src0Sel;
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
                    else if (::strcmp(mod, "src1_sel")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint src1Sel = 0;
                            if (getEnumeration(asmr, linePtr, "src1_sel", 6,
                                        vopSDWADSTSelNamesMap, src1Sel))
                            {
                                extraMods->dstSelUnused = (extraMods->src01Sel&~0x70) |
                                            (src1Sel<<4);
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
                            asmr.printError(linePtr, "Expected ':' before src0_sel");
                            good = false;
                        }
                    }
                    else if (::strcmp(mod, "quad_perm")==0 ||
                        (mod[0]=='q' && mod[1]=='p' && mod[2]==0))
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            bool goodMod = true;
                            skipCharAndSpacesToEnd(linePtr, end);
                            if (linePtr==end || *linePtr!=':')
                            {
                                asmr.printError(linePtr,
                                        "Expected '[' before quad_perm list");
                                goodMod = good = false;
                                continue;
                            }
                            cxbyte quadPerm = 0;
                            skipCharAndSpacesToEnd(linePtr, end);
                            for (cxuint k = 0; k < 4; k++)
                            {
                                cxbyte qpMask = (3<<(k<<1));
                                try
                                {
                                    cxbyte qpv = cstrtobyte(linePtr, end);
                                    if (qpv<4)
                                        quadPerm |= qpv&qpMask;
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
                                    }
                                }
                                else if (linePtr==end || *linePtr!=']')
                                {   // unterminated quad_perm
                                    asmr.printError(linePtr, "Unterminated quad_perm");
                                    goodMod = good = false;
                                }
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
                    else if (::strcmp(mod, "bank_mask")==0 ||
                        (mod[0]=='b' && mod[1]=='m' && mod[2]==0))
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxbyte bankMask = 0;
                            if (parseImm(asmr, linePtr, bankMask, nullptr, 4, WS_UNSIGNED))
                            {
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
                    else if (::strcmp(mod, "row_mask")==0 ||
                        (mod[0]=='r' && mod[1]=='m' && mod[2]==0))
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxbyte rowMask = 0;
                            if (parseImm(asmr, linePtr, rowMask, nullptr, 4, WS_UNSIGNED))
                            {
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
                    else if (::strcmp(mod, "bound_ctrl")==0 ||
                        (mod[0]=='b' && mod[1]=='c' && mod[2]==0))
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
                        }
                    }
                    else if (mod[0]=='r' && mod[0]=='o' && mod[0]=='w' && mod[0]=='_' &&
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
                            if (linePtr==end || *linePtr!='1')
                            {
                                asmr.printError(linePtr, "Value must be '1'");
                                modGood = good = false;
                            }
                        }
                        if (mod[5]=='s')
                            extraMods->dppCtrl = 0x100 | ((mod[7]=='l') ? 0x30 : 0x34);
                        else if (mod[5]=='r')
                            extraMods->dppCtrl = 0x100 | ((mod[7]=='l') ? 0x38 : 0x3c);
                        if (modGood)
                        {   // dpp_ctrl is defined
                            if (haveDppCtrl)
                                asmr.printWarning(modPlace, "DppCtrl is already defined");
                            haveDppCtrl = true;
                        }
                    }
                    else if (::strcmp(mod, "row_mirror")==0 ||
                        ::strcmp(mod, "row_half_mirror")==0)
                    {
                    }
                    else if (::strcmp(mod, "row_bcast15")==0 ||
                        ::strcmp(mod, "row_bcast31")==0 || ::strcmp(mod, "row_bcast")==0)
                    {
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
    return good;
}

void GCNAsmUtils::parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_RX3X0)!=0;
    
    RegRange dstReg(0, 0);
    RegRange dstCCReg(0, 0);
    RegRange srcCCReg(0, 0);
    if (mode1 == GCN_DS1_SGPR) // if SGPRS as destination
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    else // if VGPRS as destination
        good &= parseVRegRange(asmr, linePtr, dstReg, (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    
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
    
    const Flags vopOpModFlags = (((haveDstCC || haveSrcCC) && !isGCN12) ?
                    INSTROP_VOP3NEG : INSTROP_VOP3MODS);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch,
            (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, literalConstsFlags | vopOpModFlags |
            INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS);
    
    uint32_t immValue = 0;
    std::unique_ptr<AsmExpression> immExpr;
    if (mode1 == GCN_ARG1_IMM)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseLiteralImm(asmr, linePtr, immValue, &immExpr, literalConstsFlags);
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    bool sgprRegInSrc1 = mode1 == GCN_DS1_SGPR || mode1 == GCN_SRC1_SGPR;
    skipSpacesToEnd(linePtr, end);
    good &= parseOperand(asmr, linePtr, src1Op, &src1OpExpr, arch,
            (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, literalConstsFlags | vopOpModFlags |
            (!sgprRegInSrc1 ? INSTROP_VREGS : 0)|INSTROP_SSOURCE|INSTROP_SREGS|
            (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0));
    
    if (mode1 == GCN_ARG2_IMM)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseLiteralImm(asmr, linePtr, immValue, &immExpr, literalConstsFlags);
    }
    else if (haveSrcCC)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseSRegRange(asmr, linePtr, srcCCReg, arch, 2);
    }
    
    // modifiers
    cxbyte modifiers = 0;
    good &= parseVOPModifiers(asmr, linePtr, modifiers, nullptr,
                              !(haveDstCC || haveSrcCC));
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const bool vop3 = /* src1=sgprs and not (DS1_SGPR|src1_SGPR) */
        ((src1Op.range.start<256) ^ sgprRegInSrc1) ||
        src0Op.vop3Mods!=0 || src1Op.vop3Mods!=0 || modifiers!=0 ||
        /* srcCC!=VCC or dstCC!=VCC */
        (haveDstCC && dstCCReg.start!=106) || (haveSrcCC && srcCCReg.start!=106);
    
    cxuint maxSgprsNum = (isGCN12)?102:104;
    
    if ((src0Op.range.start==255 || src1Op.range.start==255) &&
        (src0Op.range.start<maxSgprsNum || src0Op.range.start==124 ||
         src1Op.range.start<maxSgprsNum || src1Op.range.start==124))
    {
        asmr.printError(instrPlace, "Literal with SGPR or M0 is illegal");
        return;
    }
    if (src0Op.range.start<maxSgprsNum && src1Op.range.start<maxSgprsNum &&
        src0Op.range.start!=src1Op.range.start)
    {   /* include VCCs (???) */
        asmr.printError(instrPlace, "More than one SGPR to read in instruction");
        return;
    }
    
    if (vop3 && (src0Op.range.start==255 || src1Op.range.start==255 ||
            mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM))
    {
        asmr.printError(instrPlace, "Literal in VOP3 encoding is illegal");
        return;
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
        SLEV(words[0], (uint32_t(gcnInsn.code1)<<25) | uint32_t(src0Op.range.start) |
            (uint32_t(src1Op.range.start&0xff)<<9) | (uint32_t(dstReg.start&0xff)<<17));
        if (src0Op.range.start==255)
            SLEV(words[wordsNum++], src0Op.value);
        else if (src1Op.range.start==255)
            SLEV(words[wordsNum++], src1Op.value);
        else if (mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM)
            SLEV(words[wordsNum++], immValue);
    }
    else
    {   // VOP3 encoding
        if (haveDstCC || haveSrcCC)
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code2)<<17) |
                (dstReg.start&0xff) | (uint32_t(dstCCReg.start)<<8));
        else
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code2)<<17) |
                (dstReg.start&0xff) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0) |
                ((src0Op.vop3Mods & VOPOPFLAG_ABS) ? 0x100 : 0) |
                ((src1Op.vop3Mods & VOPOPFLAG_ABS) ? 0x200 : 0));
        SLEV(words[1], src0Op.range.start | (uint32_t(src1Op.range.start)<<9) |
            (uint32_t(srcCCReg.start)<<18) | ((modifiers & 3) << 27) |
            ((src0Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<29) : 0) |
            ((src1Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<30) : 0));
        wordsNum++;
    }
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    src1OpExpr.release();
    immExpr.release();
    // update register pool
    if (dstReg.start>=256)
        updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
    else // sgprs
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
    updateSGPRsNum(gcnRegs.sgprsNum, dstCCReg.end-1, arch);
}

void GCNAsmUtils::parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs)
{
    bool good = true;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    
    RegRange dstReg(0, 0);
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0OpExpr;
    cxbyte modifiers = 0;
    
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        if (mode1 == GCN_DST_SGPR) // if SGPRS as destination
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                           (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        else // if VGPRS as destination
            good &= parseVRegRange(asmr, linePtr, dstReg,
                           (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        
        const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
        
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch,
                    (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, literalConstsFlags|INSTROP_VREGS|
                    INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|INSTROP_VOP3MODS);
    }
    // modifiers
    good &= parseVOPModifiers(asmr, linePtr, modifiers, nullptr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if ((src0Op.vop3Mods!=0 || modifiers!=0) && src0Op.range.start==255)
    {
        asmr.printError(instrPlace, "Literal in VOP3 encoding is illegal");
        return;
    }
    
    if (src0OpExpr!=nullptr)
        src0OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    cxuint wordsNum = 1;
    uint32_t words[2];
    if (!(src0Op.vop3Mods!=0 || modifiers!=0))
    {   // VOP1 encoding
        SLEV(words[0], 0x7e000000U | (uint32_t(gcnInsn.code1)<<9) |
            uint32_t(src0Op.range.start) | (uint32_t(dstReg.start&0xff)<<17));
        if (src0Op.range.start==255)
            SLEV(words[wordsNum++], src0Op.value);
    }
    else
    {   // VOP3 encoding
        SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code2)<<17) |
            (dstReg.start&0xff) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0) |
            ((src0Op.vop3Mods & VOPOPFLAG_ABS) ? 0x100 : 0));
        SLEV(words[1], src0Op.range.start | ((modifiers & 3) << 27) |
            ((src0Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<29) : 0));
        wordsNum++;
    }
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    // update register pool
    if (dstReg.start>=256)
        updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
    else // sgprs
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
}

void GCNAsmUtils::parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs)
{
    bool good = true;
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    
    RegRange dstReg(0, 0);
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0OpExpr;
    GCNOperand src1Op{};
    std::unique_ptr<AsmExpression> src1OpExpr;
    cxbyte modifiers = 0;
    
    good &= parseSRegRange(asmr, linePtr, dstReg, arch, 2);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
    
    good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch,
                    (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, literalConstsFlags|INSTROP_VREGS|
                    INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|INSTROP_VOP3MODS);
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    good &= parseOperand(asmr, linePtr, src1Op, &src1OpExpr, arch,
                (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, literalConstsFlags | INSTROP_VOP3MODS|
                INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|
                (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0));
    // modifiers
    good &= parseVOPModifiers(asmr, linePtr, modifiers, nullptr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const bool vop3 = (dstReg.start!=106) || (src1Op.range.start<256) ||
        src0Op.vop3Mods!=0 || src1Op.vop3Mods!=0 || modifiers!=0;
    
    cxuint maxSgprsNum = (arch&ARCH_RX3X0)?102:104;
    if ((src0Op.range.start==255 || src1Op.range.start==255) &&
        (src0Op.range.start<maxSgprsNum || src0Op.range.start==124 ||
         src1Op.range.start<maxSgprsNum || src1Op.range.start==124))
    {
        asmr.printError(instrPlace, "Literal with SGPR or M0 is illegal");
        return;
    }
    if (src0Op.range.start<maxSgprsNum && src1Op.range.start<maxSgprsNum &&
        src0Op.range.start!=src1Op.range.start)
    {   /* include VCCs (???) */
        asmr.printError(instrPlace, "More than one SGPR to read in instruction");
        return;
    }
    
    if (vop3 && (src0Op.range.start==255 || src1Op.range.start==255))
    {
        asmr.printError(instrPlace, "Literal in VOP3 encoding is illegal");
        return;
    }
    
    if (src0OpExpr!=nullptr)
        src0OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    if (src1OpExpr!=nullptr)
        src1OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    cxuint wordsNum = 1;
    uint32_t words[2];
    if (!vop3)
    {   // VOPC encoding
        SLEV(words[0], 0x7c000000U | (uint32_t(gcnInsn.code1)<<17) | src0Op.range.start |
                (uint32_t(src1Op.range.start&0xff)<<9));
        if (src0Op.range.start==255)
            SLEV(words[wordsNum++], src0Op.value);
        else if (src1Op.range.start==255)
            SLEV(words[wordsNum++], src1Op.value);
    }
    else
    {   // VOP3 encoding
        SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code2)<<17) |
            (dstReg.start&0xff) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0) |
            ((src0Op.vop3Mods & VOPOPFLAG_ABS) ? 0x100 : 0) |
            ((src1Op.vop3Mods & VOPOPFLAG_ABS) ? 0x200 : 0));
        SLEV(words[1], src0Op.range.start | (uint32_t(src1Op.range.start)<<9) |
            ((modifiers & 3) << 27) | ((src0Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<29) : 0) |
            ((src1Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<30) : 0));
        wordsNum++;
    }
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    src1OpExpr.release();
    // update register pool
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
}

void GCNAsmUtils::parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs)
{
    bool good = true;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    
    RegRange dstReg(0, 0);
    RegRange sdstReg(0, 0);
    GCNOperand src0Op{};
    GCNOperand src1Op{};
    GCNOperand src2Op{};
    
    cxbyte modifiers = 0;
    const Flags vop3Mods = (gcnInsn.encoding == GCNENC_VOP3B) ?
            INSTROP_VOP3NEG : INSTROP_VOP3MODS;
    
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        good &= parseVRegRange(asmr, linePtr, dstReg, (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        
        if (gcnInsn.encoding == GCNENC_VOP3B &&
            (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC || mode1 == GCN_DST_VCC_VSRC2 ||
             mode1 == GCN_S0EQS12)) /* VOP3b */
        {
            good &= parseSRegRange(asmr, linePtr, sdstReg, arch, 2);
            if (!skipRequiredComma(asmr, linePtr))
                return;
        }
        const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
        
        good &= parseOperand(asmr, linePtr, src0Op, nullptr, arch,
                    (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, literalConstsFlags|INSTROP_VREGS|
                    INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|vop3Mods|
                    INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR);
        
        if (mode1 != GCN_SRC12_NONE)
        {
            if (!skipRequiredComma(asmr, linePtr))
                return;
            good &= parseOperand(asmr, linePtr, src1Op, nullptr, arch,
                    (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, literalConstsFlags|INSTROP_VREGS|
                    INSTROP_SSOURCE|INSTROP_SREGS|vop3Mods|
                    INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR);
         
            if (mode1 != GCN_SRC2_NONE && mode1 != GCN_DST_VCC)
            {
                if (!skipRequiredComma(asmr, linePtr))
                    return;
                good &= parseOperand(asmr, linePtr, src2Op, nullptr, arch,
                        (gcnInsn.mode&GCN_REG_SRC2_64)?2:1, literalConstsFlags|
                        INSTROP_VREGS| INSTROP_SSOURCE|INSTROP_SREGS|vop3Mods|
                        INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR);
            }
        }
    }
    // modifiers
    good &= parseVOPModifiers(asmr, linePtr, modifiers, nullptr,
                              gcnInsn.encoding!=GCNENC_VOP3B);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    cxuint maxSgprsNum = (arch&ARCH_RX3X0)?102:104;
    cxuint numSgprToRead = 0;
    if (src0Op.range.start<maxSgprsNum)
        numSgprToRead++;
    if (src1Op && src1Op.range.start<maxSgprsNum && src0Op.range.start!=src1Op.range.start)
        numSgprToRead++;
    if (src2Op && src2Op.range.start<maxSgprsNum &&
            src0Op.range.start!=src2Op.range.start &&
            src1Op.range.start!=src2Op.range.start)
        numSgprToRead++;
    
    if (numSgprToRead>=2)
    {
        asmr.printError(instrPlace, "More than one SGPR to read in instruction");
        return;
    }
    if (mode1 == GCN_S0EQS12 && (src0Op.range.start!=src1Op.range.start &&
        src0Op.range.start!=src2Op.range.start))
    {
        asmr.printError(instrPlace,
                    "First source must be equal to second or third source");
        return;
    }
    
    uint32_t words[2];
    if (gcnInsn.encoding == GCNENC_VOP3B)
        SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<17) |
            (dstReg.start&0xff) | (uint32_t(sdstReg.start)<<8));
    else // VOP3A
        SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<17) |
            (dstReg.start&0xff) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0) |
            ((src0Op.vop3Mods & VOPOPFLAG_ABS) ? 0x100 : 0) |
            ((src1Op.vop3Mods & VOPOPFLAG_ABS) ? 0x200 : 0) |
            ((src2Op.vop3Mods & VOPOPFLAG_ABS) ? 0x400 : 0));
    SLEV(words[1], src0Op.range.start | (uint32_t(src1Op.range.start)<<9) |
        (uint32_t(src2Op.range.start)<<18) | ((modifiers & 3) << 27) |
        ((src0Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<29) : 0) |
        ((src1Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<30) : 0) |
        ((src2Op.vop3Mods & VOPOPFLAG_NEG) ? (1U<<31) : 0));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    // update register pool
    if (dstReg.start>=256)
        updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
    else // sgprs
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
    updateSGPRsNum(gcnRegs.sgprsNum, sdstReg.end-1, arch);
}

static const char* vintrpParamsTbl[] =
{ "p10", "p20", "p0" };

void GCNAsmUtils::parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    RegRange dstReg(0, 0);
    RegRange srcReg(0, 0);
    
    good &= parseVRegRange(asmr, linePtr, dstReg, 1);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    if ((gcnInsn.mode & GCN_MASK1) == GCN_P0_P10_P20)
    {   // vintrp parameter
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
                srcReg = { p0Code, p0Code+1 };
            else
            {
                asmr.printError(p0Place, "Unknown VINTRP parameter");
                good = false;
            }
        }
        else
            good = false;
    }
    else // regular vector register
        good &= parseVRegRange(asmr, linePtr, srcReg, 1);
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
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
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const cxbyte attrChan = (attrCmpName=='x') ? 0 : (attrCmpName=='y') ? 1 :
            (attrCmpName=='z') ? 2 : 3;
    
    /* */
    uint32_t word;
    SLEV(word, 0xc8000000U | (srcReg.start&0xff) | (uint32_t(attrChan&3)<<8) |
            (uint32_t(attrVal&0x3f)<<10) | (uint32_t(gcnInsn.code1)<<16) |
            (uint32_t(dstReg.start&0xff)<<18));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word),
            reinterpret_cast<cxbyte*>(&word)+4);
    
    updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
}

void GCNAsmUtils::parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    RegRange dstReg(0, 0);
    RegRange addrReg(0, 0);
    RegRange data0Reg(0, 0), data1Reg(0, 0);
    
    bool beforeData = false;
    bool vdstUsed = false;
    
    if ((gcnInsn.mode & GCN_ADDR_SRC) != 0 || (gcnInsn.mode & GCN_ONLYDST) != 0)
    {   /* vdst is dst */
        cxuint regsNum = (gcnInsn.mode&GCN_REG_DST_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        good &= parseVRegRange(asmr, linePtr, dstReg, regsNum);
        vdstUsed = beforeData = true;
    }
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0)
    {
        if (vdstUsed)
            if (!skipRequiredComma(asmr, linePtr))
                return;
        good &= parseVRegRange(asmr, linePtr, addrReg, 1);
        beforeData = true;
    }
    
    const uint16_t srcMode = (gcnInsn.mode & GCN_SRCS_MASK);
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 &&
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) != 0 && srcMode != GCN_NOSRC)
    {   /* two vdata */
        if (beforeData)
            if (!skipRequiredComma(asmr, linePtr))
                return;
        
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        good &= parseVRegRange(asmr, linePtr, data0Reg, regsNum);
        if (srcMode == GCN_2SRCS)
        {
            if (!skipRequiredComma(asmr, linePtr))
                return;
            good &= parseVRegRange(asmr, linePtr, data1Reg,
                       (gcnInsn.mode&GCN_REG_SRC1_64)?2:1);
        }
    }
    
    bool haveGds = false;
    std::unique_ptr<AsmExpression> offsetExpr, offset2Expr;
    char name[10];
    uint16_t offset = 0;
    cxbyte offset1 = 0, offset2 = 0;
    bool haveOffset = false, haveOffset2 = false;
    while (linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, "DS modifier"))
        {
            good = false;
            continue;
        }
        toLowerString(name);
        if (::strcmp(name, "gds")==0)
            haveGds = true;
        else if ((gcnInsn.mode & GCN_2OFFSETS) == 0) /* single offset */
        {
            if (::strcmp(name, "offset") == 0)
            {
                if (parseModImm(asmr, linePtr, offset, &offsetExpr, "offset", WS_UNSIGNED))
                {
                    if (haveOffset)
                        asmr.printWarning(modPlace, "Offset is already defined");
                    haveOffset = true;
                }
                else
                    good = false;
            }
            else
            {
                asmr.printError(modPlace, "Expected 'offset'");
                good = false;
            }
        }
        else
        {   // two offsets (offset0, offset1)
            if (::memcmp(name, "offset", 6)==0 &&
                (name[6]=='0' || name[6]=='1') && name[7]==0)
            {
                skipSpacesToEnd(linePtr, end);
                if (linePtr!=end && *linePtr==':')
                {
                    skipCharAndSpacesToEnd(linePtr, end);
                    if (name[6]=='0')
                    {   /* offset0 */
                        if (parseImm(asmr, linePtr, offset1, &offsetExpr, 0, WS_UNSIGNED))
                        {
                            if (haveOffset)
                                asmr.printWarning(modPlace, "Offset0 is already defined");
                            haveOffset = true;
                        }
                        else
                            good = false;
                    }
                    else
                    {   /* offset1 */
                        if (parseImm(asmr, linePtr, offset2, &offset2Expr, 0, WS_UNSIGNED))
                        {
                            if (haveOffset2)
                                asmr.printWarning(modPlace, "Offset1 is already defined");
                            haveOffset2 = true;
                        }
                        else
                            good = false;
                    }
                }
                else
                {
                    asmr.printError(linePtr, "Expected ':' before offset");
                    good = false;
                }
            }
            else
            {
                asmr.printError(modPlace,
                                "Expected 'offset', 'offset0' or 'offset1'");
                good = false;
            }
        }
    }
    
    if ((gcnInsn.mode & GCN_2OFFSETS) != 0)
        offset = offset1 | (offset2<<8);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if ((gcnInsn.mode&GCN_ONLYGDS) != 0 && !haveGds)
    {
        asmr.printError(instrPlace, "Instruction requires GDS modifier");
        return;
    }
    
    if (offsetExpr!=nullptr)
        offsetExpr->setTarget(AsmExprTarget((gcnInsn.mode & GCN_2OFFSETS) ?
                    GCNTGT_DSOFFSET8_0 : GCNTGT_DSOFFSET16, asmr.currentSection,
                    output.size()));
    if (offset2Expr!=nullptr)
        offset2Expr->setTarget(AsmExprTarget(GCNTGT_DSOFFSET8_1, asmr.currentSection,
                    output.size()));
    
    uint32_t words[2];
    SLEV(words[0], 0xd8000000U | uint32_t(offset) | (haveGds ? 0x20000U : 0U) |
            (uint32_t(gcnInsn.code1)<<18));
    SLEV(words[1], (addrReg.start&0xff) | (uint32_t(data0Reg.start&0xff)<<8) |
            (uint32_t(data1Reg.start&0xff)<<16) | (uint32_t(dstReg.start&0xff)<<24));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    offsetExpr.release();
    offset2Expr.release();
    // update register pool
    updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
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

static const std::pair<const char*, uint16_t> mtbufDFMTNamesMap[] =
{
    { "10_10_10_2", 8 },
    { "10_11_11", 6 },
    { "11_11_10", 7 },
    { "16", 2 },
    { "16_16", 5 },
    { "16_16_16_16", 12 },
    { "2_10_10_10", 9 },
    { "32", 4 },
    { "32_32", 11 },
    { "32_32_32", 13 },
    { "32_32_32_32", 14 },
    { "8", 1 },
    { "8_8", 3 },
    { "8_8_8_8", 10 }
};

static const std::pair<const char*, cxuint> mtbufNFMTNamesMap[] =
{
    { "float", 7 },
    { "sint", 5 },
    { "snorm", 1 },
    { "snorm_ogl", 6 },
    { "sscaled", 3 },
    { "uint", 4 },
    { "unorm", 0 },
    { "uscaled", 2 }
};

void GCNAsmUtils::parseMUBUFEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    RegRange vaddrReg(0, 0);
    RegRange vdataReg(0, 0);
    RegRange soffsetReg(0, 0);
    RegRange srsrcReg(0, 0);
    
    skipSpacesToEnd(linePtr, end);
    const char* vdataPlace = linePtr;
    const char* vaddrPlace = nullptr;
    if (mode1 != GCN_ARG_NONE)
    {
        if (mode1 != GCN_MUBUF_NOVAD)
        {
            good &= parseVRegRange(asmr, linePtr, vdataReg, 0);
            if (!skipRequiredComma(asmr, linePtr))
                return;
            
            skipSpacesToEnd(linePtr, end);
            vaddrPlace = linePtr;
            good &= parseVRegRange(asmr, linePtr, vaddrReg, 0);
            if (!skipRequiredComma(asmr, linePtr))
                return;
        }
        good &= parseSRegRange(asmr, linePtr, srsrcReg, arch, 4);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseSRegRange(asmr, linePtr, soffsetReg, arch, 1);
    }
    
    bool haveOffset = false, haveFormat = false;
    cxuint dfmt = 1, nfmt = 0;
    cxuint offset = 0;
    std::unique_ptr<AsmExpression> offsetExpr;
    bool haveAddr64 = false, haveTfe = false, haveSlc = false, haveLds = false;
    bool haveGlc = false, haveOffen = false, haveIdxen = false;
    const char* modName = (gcnInsn.encoding==GCNENC_MTBUF) ?
            "MTBUF modifier" : "MUBUF modifier";
    
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        char name[10];
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, modName))
        {
            good = false;
            continue;
        }
        toLowerString(name);
        
        if (name[0] == 'o')
        {   // offen, offset
            if (::strcmp(name+1, "ffen")==0)
                haveOffen = true;
            else if (::strcmp(name+1, "ffset")==0)
            {   // parse offset
                if (parseModImm(asmr, linePtr, offset, &offsetExpr, "offset",
                                12, WS_UNSIGNED))
                {
                    if (haveOffset)
                        asmr.printWarning(modPlace, "Offset is already defined");
                    haveOffset = true;
                }
                else
                    good = false;
            }
            else
            {
                asmr.printError(modPlace, (gcnInsn.encoding==GCNENC_MUBUF) ? 
                    "Unknown MUBUF modifier" : "Unknown MTBUF modifier");
            }
        }
        else if (gcnInsn.encoding==GCNENC_MTBUF && ::strcmp(name, "format")==0)
        {   // parse format
            bool modGood = true;
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end || *linePtr!=':')
            {
                asmr.printError(linePtr, "Expected ':' before format");
                good = false;
                continue;
            }
            skipCharAndSpacesToEnd(linePtr, end);
            if (linePtr==end || *linePtr!='[')
            {
                asmr.printError(modPlace, "Expected '[' before format");
                modGood = good = false;
            }
            if (modGood)
            {
                skipCharAndSpacesToEnd(linePtr, end);
                const char* fmtPlace = linePtr;
                char fmtName[30];
                bool haveNFMT = false;
                if (getMUBUFFmtNameArg(asmr, 30, fmtName, linePtr, "data/number format"))
                {
                    toLowerString(fmtName);
                    size_t dfmtNameIndex = (::strncmp(fmtName,
                                 "buf_data_format_", 16)==0) ? 16 : 0;
                    size_t dfmtIdx = binaryMapFind(mtbufDFMTNamesMap, mtbufDFMTNamesMap+14,
                                fmtName+dfmtNameIndex, CStringLess())-mtbufDFMTNamesMap;
                    if (dfmtIdx != 14)
                        dfmt = mtbufDFMTNamesMap[dfmtIdx].second;
                    else
                    {   // nfmt
                        size_t nfmtNameIndex = (::strncmp(fmtName,
                                 "buf_num_format_", 15)==0) ? 15 : 0;
                        size_t nfmtIdx = binaryMapFind(mtbufNFMTNamesMap,
                               mtbufNFMTNamesMap+8, fmtName+nfmtNameIndex,
                               CStringLess())-mtbufNFMTNamesMap;
                        if (nfmtIdx!=8)
                        {
                            nfmt = mtbufNFMTNamesMap[nfmtIdx].second;
                            haveNFMT = true;
                        }
                        else
                        {
                            asmr.printError(fmtPlace, "Unknown data/number format");
                            modGood = good = false;
                        }
                    }
                }
                else
                    modGood = good = false;
                
                skipSpacesToEnd(linePtr, end);
                if (!haveNFMT && linePtr!=end && *linePtr==',')
                {
                    skipCharAndSpacesToEnd(linePtr, end);
                    fmtPlace = linePtr;
                    good &= getEnumeration(asmr, linePtr, "number format",
                              8, mtbufNFMTNamesMap, nfmt, "buf_num_format_");
                }
                skipSpacesToEnd(linePtr, end);
                if (linePtr!=end && *linePtr==']')
                    linePtr++;
                else
                {
                    asmr.printError(linePtr, "Unterminated format modifier");
                    good = false;
                }
                if (modGood)
                {
                    if (haveFormat)
                        asmr.printWarning(modPlace, "Format is already defined");
                    haveFormat = true;
                }
            }
        }
        else if (::strcmp(name, "addr64")==0)
            haveAddr64 = true;
        else if (::strcmp(name, "tfe")==0)
            haveTfe = true;
        else if (::strcmp(name, "glc")==0)
            haveGlc = true;
        else if (::strcmp(name, "slc")==0)
            haveSlc = true;
        else if (gcnInsn.encoding==GCNENC_MUBUF && ::strcmp(name, "lds")==0)
            haveLds = true;
        else if (::strcmp(name, "idxen")==0)
            haveIdxen = true;
        else
        {
            asmr.printError(modPlace, (gcnInsn.encoding==GCNENC_MUBUF) ? 
                    "Unknown MUBUF modifier" : "Unknown MTBUF modifier");
            good = false;
        }
    }
    
    /* checking addr range and vdata range */
    if (vdataReg)
    {
        cxuint dregsNum = (((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1) + (haveTfe);
        if (!isXRegRange(vdataReg, dregsNum))
        {
            char errorMsg[40];
            snprintf(errorMsg, 40, "Required %u vector register%s", dregsNum,
                     (dregsNum>1) ? "s" : "");
            asmr.printError(vdataPlace, errorMsg);
            good = false;
        }
    }
    if (vaddrReg)
    {
        const cxuint vaddrSize = ((haveOffen&&haveIdxen) || haveAddr64) ? 2 : 1;
        if (!isXRegRange(vaddrReg, vaddrSize))
        {
            asmr.printError(vaddrPlace, (vaddrSize==2) ? "Required 2 vector registers" : 
                        "Required 1 vector register");
            good = false;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    /* checking modifiers conditions */
    if (haveAddr64 && (haveOffen || haveIdxen))
    {
        asmr.printError(instrPlace, "Idxen and offen must be zero in 64-bit address mode");
        return;
    }
    if (haveOffen && offset!=0)
        asmr.printWarning(instrPlace, "Offset will be ignored for enabled offen");
    
    if (offsetExpr!=nullptr)
        offsetExpr->setTarget(AsmExprTarget(GCNTGT_MXBUFOFFSET, asmr.currentSection,
                    output.size()));
    
    uint32_t words[2];
    if (gcnInsn.encoding==GCNENC_MUBUF)
        SLEV(words[0], 0xe0000000U | offset | (haveOffen ? 0x1000U : 0U) |
                (haveIdxen ? 0x2000U : 0U) | (haveGlc ? 0x4000U : 0U) |
                (haveAddr64 ? 0x8000U : 0U) | (haveLds ? 0x10000U : 0U) |
                (uint32_t(gcnInsn.code1)<<18));
    else // MTBUF
        SLEV(words[0], 0xe8000000U | offset | (haveOffen ? 0x1000U : 0U) |
                (haveIdxen ? 0x2000U : 0U) | (haveGlc ? 0x4000U : 0U) |
                (haveAddr64 ? 0x8000U : 0U) | (uint32_t(gcnInsn.code1)<<16) |
                (uint32_t(dfmt)<<19) | (uint32_t(nfmt)<<23));
    
    SLEV(words[1], (vaddrReg.start&0xff) | (uint32_t(vdataReg.start&0xff)<<8) |
            (uint32_t(srsrcReg.start>>2)<<16) | (haveSlc ? (1U<<22) : 0) |
            (haveTfe ? (1U<<23) : 0) | (uint32_t(soffsetReg.start)<<24));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    offsetExpr.release();
    // update register pool (instr loads or save old value) */
    if ((gcnInsn.mode&GCN_MLOAD) != 0 || ((gcnInsn.mode&GCN_MATOMIC)!=0 && haveGlc))
        updateVGPRsNum(gcnRegs.vgprsNum, vdataReg.end-257);
}

void GCNAsmUtils::parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    RegRange vaddrReg(0, 0);
    RegRange vdataReg(0, 0);
    RegRange ssampReg(0, 0);
    RegRange srsrcReg(0, 0);
    
    skipSpacesToEnd(linePtr, end);
    const char* vdataPlace = linePtr;
    good &= parseVRegRange(asmr, linePtr, vdataReg, 0);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    skipSpacesToEnd(linePtr, end);
    good &= parseVRegRange(asmr, linePtr, vaddrReg, 4);
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    const char* srsrcPlace = linePtr;
    good &= parseSRegRange(asmr, linePtr, srsrcReg, arch, 0);
    
    if ((gcnInsn.mode & GCN_MASK2) == GCN_MIMG_SAMPLE)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseSRegRange(asmr, linePtr, ssampReg, arch, 4);
    }
    
    bool haveTfe = false, haveSlc = false, haveGlc = false;
    bool haveDa = false, haveR128 = false, haveLwe = false, haveUnorm = false;
    bool haveDMask = false;
    cxbyte dmask = 0x1;
    /* modifiers and modifiers */
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        char name[10];
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, "MIMG modifier"))
        {
            good = false;
            continue;
        }
        toLowerString(name);
        
        if (name[0] == 'd')
        {
            if (name[1]=='a' && name[2]==0)
                haveDa = true;
            else if (::strcmp(name+1, "mask")==0)
            {
                // parse offset
                skipSpacesToEnd(linePtr, end);
                if (linePtr!=end && *linePtr==':')
                {   /* parse offset immediate */
                    skipCharAndSpacesToEnd(linePtr, end);
                    const char* valuePlace = linePtr;
                    uint64_t value;
                    if (getAbsoluteValueArg(asmr, value, linePtr, true))
                    {
                        if (haveDMask)
                            asmr.printWarning(modPlace, "Dmask is already defined");
                        haveDMask = true;
                        if (value>0xf)
                            asmr.printWarning(valuePlace, "Dmask out of range (0-15)");
                        dmask = value&0xf;
                        if (dmask == 0)
                        {
                            asmr.printError(valuePlace, "Zero in dmask is illegal");
                            good = false;
                        }
                    }
                    else
                        good = false;
                }
                else
                {
                    asmr.printError(linePtr, "Expected ':' before dmask");
                    good = false;
                }
            }
            else
            {
                asmr.printError(modPlace, "Unknown MIMG modifier");
                good = false;
            }
        }
        else if (name[0] < 's')
        {
            if (::strcmp(name, "glc")==0)
                haveGlc = true;
            else if (::strcmp(name, "lwe")==0)
                haveLwe = true;
            else if (::strcmp(name, "r128")==0)
                haveR128 = true;
            else
            {
                asmr.printError(modPlace, "Unknown MIMG modifier");
                good = false;
            }
        }
        else if (::strcmp(name, "tfe")==0)
            haveTfe = true;
        else if (::strcmp(name, "slc")==0)
            haveSlc = true;
        else if (::strcmp(name, "unorm")==0)
            haveUnorm = true;
        else
        {
            asmr.printError(modPlace, "Unknown MIMG modifier");
            good = false;
        }
    }
    
    if (dmask!=0)
    {
        cxuint dregsNum = ((dmask & 1)?1:0) + ((dmask & 2)?1:0) + ((dmask & 4)?1:0) +
                ((dmask & 8)?1:0) + (haveTfe);
        if (!isXRegRange(vdataReg, dregsNum))
        {
            char errorMsg[40];
            snprintf(errorMsg, 40, "Required %u vector register%s", dregsNum,
                     (dregsNum>1) ? "s" : "");
            asmr.printError(vdataPlace, errorMsg);
            good = false;
        }
    }
    if (!isXRegRange(srsrcReg, (haveR128)?4:8))
    {
        asmr.printError(srsrcPlace, (haveR128) ? "Required 4 scalar registers" :
                    "Required 8 scalar registers");
        good = false;
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    /* checking modifiers conditions */
    if (!haveUnorm && ((gcnInsn.mode&GCN_MLOAD) == 0 || (gcnInsn.mode&GCN_MATOMIC)!=0))
    {   // unorm is not set for this instruction
        asmr.printError(instrPlace, "Unorm is not set for store or atomic instruction");
        return;
    }
    
    uint32_t words[2];
    SLEV(words[0], 0xf0000000U | (uint32_t(dmask&0xf)<<8) | (haveUnorm ? 0x1000U : 0) |
        (haveGlc ? 0x2000U : 0) | (haveDa ? 0x4000U : 0) | (haveR128 ? 0x8000U : 0) |
        (haveTfe ? 0x10000U : 0) | (haveLwe ? 0x20000U : 0) |
        (uint32_t(gcnInsn.code1)<<18) | (haveSlc ? (1U<<25) : 0));
    SLEV(words[1], (vaddrReg.start&0xff) | (uint32_t(vdataReg.start&0xff)<<8) |
            (uint32_t(srsrcReg.start>>2)<<16) | (uint32_t(ssampReg.start>>2)<<21));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    // update register pool (instr loads or save old value) */
    if ((gcnInsn.mode&GCN_MLOAD) != 0 || ((gcnInsn.mode&GCN_MATOMIC)!=0 && haveGlc))
        updateVGPRsNum(gcnRegs.vgprsNum, vdataReg.end-257);
}

void GCNAsmUtils::parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    cxbyte enMask = 0xf;
    cxbyte target = 0;
    RegRange vsrcsReg[4];
    const char* vsrcPlaces[4];
    
    char name[20];
    skipSpacesToEnd(linePtr, end);
    const char* targetPlace = linePtr;
    
    try
    {
    if (getNameArg(asmr, 20, name, linePtr, "target"))
    {
        size_t nameSize = linePtr-targetPlace;
        const char* nameStart = name;
        toLowerString(name);
        if (name[0]=='m' && name[1]=='r' && name[2]=='t')
        {   // mrt
            if (name[3]!='z' || name[4]!=0)
            {
                nameStart+=3;
                target = cstrtobyte(nameStart, name+nameSize);
                if (target>=8)
                {
                    asmr.printError(targetPlace, "MRT number out of range (0-7)");
                    good = false;
                }
            }
            else
                target = 8; // mrtz
        }
        else if (name[0]=='p' && name[1]=='o' && name[2]=='s')
        {
            nameStart+=3;
            cxbyte posNum = cstrtobyte(nameStart, name+nameSize);
            if (posNum>=4)
            {
                asmr.printError(targetPlace, "Pos number out of range (0-3)");
                good = false;
            }
            else
                target = posNum+12;
        }
        else if (strcmp(name, "null")==0)
            target = 9;
        else if (memcmp(name, "param", 5)==0)
        {
            nameStart+=5;
            cxbyte posNum = cstrtobyte(nameStart, name+nameSize);
            if (posNum>=32)
            {
                asmr.printError(targetPlace, "Param number out of range (0-31)");
                good = false;
            }
            else
                target = posNum+32;
        }
        else
        {
            asmr.printError(targetPlace, "Unknown EXP target");
            good = false;
        }
    }
    else
        good = false;
    }
    catch (const ParseException& ex)
    {   // number parsing error
        asmr.printError(targetPlace, ex.what());
        good = false;
    }
    
    /* registers */
    for (cxuint i = 0; i < 4; i++)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        skipSpacesToEnd(linePtr, end);
        vsrcPlaces[i] = linePtr;
        if (linePtr+2>=end || toLower(linePtr[0])!='o' || toLower(linePtr[1])!='f' ||
            toLower(linePtr[2])!='f' || (linePtr+3!=end && isAlnum(linePtr[3])))
            good &= parseVRegRange(asmr, linePtr, vsrcsReg[i], 1);
        else
        {   // if vsrcX is off
            enMask &= ~(1U<<i);
            vsrcsReg[i] = { 0, 0 };
            linePtr += 3;
        }
    }
    
    /* EXP modifiers */
    bool haveVM = false, haveCompr = false, haveDone = false;
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, "EXP modifier"))
        {
            good = false;
            continue;
        }
        toLowerString(name);
        if (name[0]=='v' && name[1]=='m' && name[2]==0)
            haveVM = true;
        else if (::strcmp(name, "done")==0)
            haveDone = true;
        else if (::strcmp(name, "compr")==0)
            haveCompr = true;
        else
        {
            asmr.printError(modPlace, "Unknown EXP modifier");
            good = false;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (haveCompr)
    {
        if (vsrcsReg[0].start!=vsrcsReg[1].start && (enMask&3)==3)
        {   // error (vsrc1!=vsrc0)
            asmr.printError(vsrcPlaces[1], "VSRC1 must be equal to VSRC0 in compr mode");
            return;
        }
        if (vsrcsReg[2].start!=vsrcsReg[3].start && (enMask&12)==12)
        {   // error (vsrc3!=vsrc2)
            asmr.printError(vsrcPlaces[3], "VSRC3 must be equal to VSRC2 in compr mode");
            return;
        }
        vsrcsReg[1] = vsrcsReg[2];
        vsrcsReg[2] = vsrcsReg[3] = { 0, 0 };
    }
    
    uint32_t words[2];
    SLEV(words[0], 0xf8000000U | enMask | (uint32_t(target)<<4) |
            (haveCompr ? 0x400 : 0) | (haveDone ? 0x800 : 0) | (haveVM ? 0x1000U : 0));
    SLEV(words[1], uint32_t(vsrcsReg[0].start&0xff) |
            (uint32_t(vsrcsReg[1].start&0xff)<<8) |
            (uint32_t(vsrcsReg[2].start&0xff)<<16) |
            (uint32_t(vsrcsReg[3].start&0xff)<<24));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
}

void GCNAsmUtils::parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                  GCNAssembler::Regs& gcnRegs)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    RegRange vaddrReg(0, 0);
    RegRange vdstReg(0, 0);
    RegRange vdataReg(0, 0);
    
    skipSpacesToEnd(linePtr, end);
    const char* vdstPlace = nullptr;
    
    const cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
    if ((gcnInsn.mode & GCN_FLAT_ADST) == 0)
    {
        vdstPlace = linePtr;
        good &= parseVRegRange(asmr, linePtr, vdstReg, 0);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseVRegRange(asmr, linePtr, vaddrReg, 2);
    }
    else
    {
        good &= parseVRegRange(asmr, linePtr, vaddrReg, 2);
        if ((gcnInsn.mode & GCN_FLAT_NODST) == 0)
        {
            if (!skipRequiredComma(asmr, linePtr))
                return;
            skipSpacesToEnd(linePtr, end);
            vdstPlace = linePtr;
            good &= parseVRegRange(asmr, linePtr, vdstReg, 0);
        }
    }
    
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0) /* print data */
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseVRegRange(asmr, linePtr, vdataReg, dregsNum);
    }
    
    bool haveTfe = false, haveSlc = false, haveGlc = false;
    while(linePtr!=end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            break;
        char name[10];
        const char* modPlace = linePtr;
        if (!getNameArgS(asmr, 10, name, linePtr, "FLAT modifier"))
        {
            good = false;
            continue;
        }
        if (::strcmp(name, "tfe") == 0)
            haveTfe = true;
        else if (::strcmp(name, "glc") == 0)
            haveGlc = true;
        else if (::strcmp(name, "slc") == 0)
            haveSlc = true;
        else
        {
            asmr.printError(modPlace, "Unknown FLAT modifier");
            good = false;
        }
    }
    /* check register ranges */
    
    if (vdstReg)
    {
        const cxuint dstRegsNum = (haveTfe) ? dregsNum+1:dregsNum; // include tfe 
        if (!isXRegRange(vdstReg, dstRegsNum))
        {
            char errorMsg[40];
            snprintf(errorMsg, 40, "Required %u vector register%s", dstRegsNum,
                     (dstRegsNum>1) ? "s" : "");
            asmr.printError(vdstPlace, errorMsg);
            good = false;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    uint32_t words[2];
    SLEV(words[0], 0xdc000000U | (haveGlc ? 0x10000 : 0) | (haveSlc ? 0x20000: 0) |
            (uint32_t(gcnInsn.code1)<<18));
    SLEV(words[1], (vaddrReg.start&0xff) | (uint32_t(vdataReg.start&0xff)<<8) |
            (haveTfe ? (1U<<23) : 0) | (uint32_t(vdstReg.start&0xff)<<24));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    // update register pool
    updateVGPRsNum(gcnRegs.vgprsNum, vdstReg.end-257);
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
        printError(mnemPlace, "Unknown instruction");
        return;
    }
    
    /* decode instruction line */
    switch(it->encoding)
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
            if (curArchMask & ARCH_RX3X0)
                GCNAsmUtils::parseSMEMEncoding(assembler, *it, linePtr,
                                       curArchMask, output, regs);
            else
                GCNAsmUtils::parseSMRDEncoding(assembler, *it, linePtr,
                                       curArchMask, output, regs);
            break;
        case GCNENC_VOPC:
            GCNAsmUtils::parseVOPCEncoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VOP1:
            GCNAsmUtils::parseVOP1Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VOP2:
            GCNAsmUtils::parseVOP2Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VOP3A:
        case GCNENC_VOP3B:
            GCNAsmUtils::parseVOP3Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_VINTRP:
            GCNAsmUtils::parseVINTRPEncoding(assembler, *it, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_DS:
            GCNAsmUtils::parseDSEncoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_MUBUF:
        case GCNENC_MTBUF:
            GCNAsmUtils::parseMUBUFEncoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs);
            break;
        case GCNENC_MIMG:
            GCNAsmUtils::parseMIMGEncoding(assembler, *it, mnemPlace, linePtr,
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
            printWarningForRange(8, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_DSOFFSET16:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, "Relative value is illegal in offset expressions");
                return false;
            }
            SULEV(*reinterpret_cast<uint16_t*>(sectionData+offset), value);
            printWarningForRange(16, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_DSOFFSET8_0:
        case GCNTGT_DSOFFSET8_1:
        case GCNTGT_SOPCIMM8:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, (targetType != GCNTGT_SOPCIMM8) ?
                        "Relative value is illegal in offset expressions" :
                        "Relative value is illegal in immediate expressions");
                return false;
            }
            if (targetType==GCNTGT_DSOFFSET8_0)
                sectionData[offset] = value;
            else
                sectionData[offset+1] = value;
            printWarningForRange(8, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_MXBUFOFFSET:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, "Relative value is illegal in offset expressions");
                return false;
            }
            sectionData[offset] = value&0xff;
            sectionData[offset+1] = (sectionData[offset+1]&0xf0) | ((value>>8)&0xf);
            printWarningForRange(12, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_SMEMOFFSET:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, "Relative value is illegal in offset expressions");
                return false;
            }
            SULEV(*reinterpret_cast<uint32_t*>(sectionData+offset+4), value&0xfffffU);
            printWarningForRange(20, value, sourcePos, WS_UNSIGNED);
            return true;
        case GCNTGT_SMEMIMM:
            if (sectionId != ASMSECT_ABS)
            {
                printError(sourcePos, "Relative value is illegal in immediate expressions");
                return false;
            }
            sectionData[offset] = (sectionData[offset]&0x3f) | ((value<<6)&0xff);
            sectionData[offset+1] = (sectionData[offset+1]&0xe0) | ((value>>2)&0x1f);
            printWarningForRange(7, value, sourcePos, WS_UNSIGNED);
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
