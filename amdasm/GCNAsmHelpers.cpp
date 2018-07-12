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
#include <cstdio>
#include <cstring>
#include <string>
#include <memory>
#include <utility>
#include <algorithm>
#include <CLRX/utils/GPUId.h>
#include "GCNAsmInternals.h"

using namespace CLRX;

// simple routine to parse byte value (0-255)
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

// print error 'range expected', return true if printed, false if not
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

static inline cxbyte getRegVarAlign(const AsmRegVar* regVar, cxuint regsNum, Flags flags)
{
    if ((flags & INSTROP_UNALIGNED) == 0 && regVar->type==REGTYPE_SGPR)
        return regsNum==2 ? 2 : regsNum>=3 ? 4 : 1;
    return 1;
}

bool GCNAsmUtils::parseRegVarRange(Assembler& asmr, const char*& linePtr,
                 RegRange& regPair, GPUArchMask arch, cxuint regsNum, AsmRegField regField,
                 Flags flags, bool required)
{
    const char* oldLinePtr = linePtr;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* regVarPlace = linePtr;
    const char *regTypeName = (flags&INSTROP_VREGS) ? "vector" : "scalar";
    
    const CString name = extractScopedSymName(linePtr, end, false);
    bool regVarFound = false;
    //AsmSection& section = asmr.sections[asmr.currentSection];
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    const AsmRegVar* regVar;
    if (!name.empty())
        regVarFound = asmr.getRegVar(name, regVar);
    if (regVarFound)
    {
        // if regvar found
        cxuint rstart = 0;
        cxuint rend = regVar->size;
        if (((flags & INSTROP_VREGS)!=0 && regVar->type==REGTYPE_VGPR) ||
            ((flags & INSTROP_SREGS)!=0 && regVar->type==REGTYPE_SGPR))
        {
            skipSpacesToEnd(linePtr, end);
            if (linePtr != end && *linePtr == '[')
            {
                // if we have range '[first:last]'
                uint64_t value1, value2;
                skipCharAndSpacesToEnd(linePtr, end);
                // parse first register index
                if (!getAbsoluteValueArg(asmr, value1, linePtr, true))
                    return false;
                skipSpacesToEnd(linePtr, end);
                if (linePtr == end || (*linePtr!=':' && *linePtr!=']'))
                    // error
                    ASM_FAIL_BY_ERROR(regVarPlace, "Unterminated register range")
                if (linePtr!=end && *linePtr==':')
                {
                    skipCharAndSpacesToEnd(linePtr, end);
                    // parse second register index
                    if (!getAbsoluteValueArg(asmr, value2, linePtr, true))
                        return false;
                }
                else // we assume [first] -> [first:first]
                    value2 = value1;
                skipSpacesToEnd(linePtr, end);
                if (linePtr == end || *linePtr != ']')
                    // error
                    ASM_FAIL_BY_ERROR(regVarPlace, "Unterminated register range")
                ++linePtr;
                if (value2 < value1)
                    // error (illegal register range)
                    ASM_FAIL_BY_ERROR(regVarPlace, "Illegal register range")
                if (value2 >= rend || value1 >= rend)
                    ASM_FAIL_BY_ERROR(regVarPlace, "Regvar range out of range")
                rend = value2+1;
                rstart = value1;
            }
            
            if (regsNum!=0 && regsNum != rend-rstart)
            {
                printXRegistersRequired(asmr, regVarPlace, regTypeName, regsNum);
                return false;
            }
            
            if (regField!=ASMFIELD_NONE)
            {
                cxbyte align = getRegVarAlign(regVar, regsNum, flags);
                // set reg var usage for current position and instruction field
                gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), regVar,
                    uint16_t(rstart), uint16_t(rend), regField,
                    cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                    ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), align });
            }
            regPair = { rstart, rend, regVar };
            return true;
        }
    }
    if (printRegisterRangeExpected(asmr, regVarPlace, regTypeName, regsNum, required))
        return false;
    regPair = { 0, 0 };
    linePtr = oldLinePtr; // revert current line pointer
    return true;
}

static inline bool isGCNConstLiteral(uint16_t rstart, GPUArchMask arch)
{
    if ((rstart >= 128 && rstart <= 208) || (rstart >= 240 && rstart <= 247))
        return true;
    return ((arch & ARCH_GCN_1_2_4) != 0 && rstart == 248);
}

static inline bool isGCNVReg(uint16_t rstart, uint16_t rend, const AsmRegVar* regVar)
{ return (regVar==nullptr && rstart >= 256 && rend >= 256) ||
            (regVar!=nullptr && regVar->type == REGTYPE_VGPR); }

static inline bool isGCNSReg(uint16_t rstart, uint16_t rend, const AsmRegVar* regVar)
{ return (regVar==nullptr && rstart < 128 && rend < 128) ||
        (regVar!=nullptr && regVar->type == REGTYPE_SGPR); }

static inline bool isGCNSSource(uint16_t rstart, uint16_t rend, const AsmRegVar* regVar)
{ return (regVar==nullptr && rstart >= 128 && rstart<255); }

bool GCNAsmUtils::parseSymRegRange(Assembler& asmr, const char*& linePtr,
            RegRange& regPair, GPUArchMask arch, cxuint regsNum, AsmRegField regField,
            Flags flags, bool required)
{
    const char* oldLinePtr = linePtr;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* regRangePlace = linePtr;
    
    AsmSymbolEntry* symEntry = nullptr;
    if (linePtr!=end && *linePtr=='@')
        skipCharAndSpacesToEnd(linePtr, end);
    
    const char *regTypeName = (flags&INSTROP_VREGS) ? "vector" : "scalar";
    const cxuint maxSGPRsNum = getGPUMaxRegsNumByArchMask(arch, REGTYPE_SGPR);
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    if (asmr.parseSymbol(linePtr, symEntry, false, true)==
        Assembler::ParseState::PARSED && symEntry!=nullptr &&
        symEntry->second.regRange)
    {
        // set up regrange
        const AsmRegVar* regVar = symEntry->second.regVar;
        cxuint rstart = symEntry->second.value&UINT_MAX;
        cxuint rend = symEntry->second.value>>32;
        /* parse register range if:
         * vector/scalar register enabled and is vector/scalar register or
         * other scalar register, (ignore VCCZ, EXECZ, SCC if no SSOURCE enabled) */
        if (((flags & INSTROP_VREGS)!=0 && isGCNVReg(rstart, rend, regVar)) ||
            ((flags & INSTROP_SREGS)!=0 && isGCNSReg(rstart, rend, regVar)) ||
            (((flags&INSTROP_SSOURCE)!=0) && isGCNSSource(rstart, rend, regVar)))
        {
            skipSpacesToEnd(linePtr, end);
            const bool isConstLit = (regVar==nullptr && isGCNConstLiteral(rstart, arch));
            
            if (linePtr != end && *linePtr == '[')
            {
                uint64_t value1, value2;
                // parse first register index
                skipCharAndSpacesToEnd(linePtr, end);
                if (!getAbsoluteValueArg(asmr, value1, linePtr, true))
                    return false;
                skipSpacesToEnd(linePtr, end);
                if (linePtr == end || (*linePtr!=':' && *linePtr!=']'))
                    // error
                    ASM_FAIL_BY_ERROR(regRangePlace, "Unterminated register range")
                if (linePtr!=end && *linePtr==':')
                {
                    // parse last register index
                    skipCharAndSpacesToEnd(linePtr, end);
                    if (!getAbsoluteValueArg(asmr, value2, linePtr, true))
                        return false;
                }
                else  // we assume [first] -> [first:first]
                    value2 = value1;
                skipSpacesToEnd(linePtr, end);
                if (linePtr == end || *linePtr != ']')
                    // error
                    ASM_FAIL_BY_ERROR(regRangePlace, "Unterminated register range")
                ++linePtr;
                if (value2 < value1)
                    // error (illegal register range)
                    ASM_FAIL_BY_ERROR(regRangePlace, "Illegal register range")
                if (value2 >= rend-rstart || value1 >= rend-rstart)
                    ASM_FAIL_BY_ERROR(regRangePlace, "Register range out of range")
                    
                if (isConstLit && (value1!=0 || (value2!=0 && value2+1!=regsNum)))
                {
                    ASM_FAIL_BY_ERROR(linePtr, "Register range for const literals must be"
                            "[0] or [0:regsNum-1]");
                    return false;
                }
                
                rend = rstart + value2+1;
                rstart += value1;
            }
            
            if (!isConstLit)
            {
                if (regsNum!=0 && regsNum != rend-rstart)
                {
                    printXRegistersRequired(asmr, regRangePlace, regTypeName, regsNum);
                    return false;
                }
                /// check aligned for scalar registers but not regvars
                if (regVar==nullptr && rstart<maxSGPRsNum)
                {
                    if ((flags & INSTROP_UNALIGNED) == 0)
                    {
                        if ((rend-rstart==2 && (rstart&1)!=0) ||
                            (rend-rstart>2 && (rstart&3)!=0))
                            ASM_FAIL_BY_ERROR(regRangePlace,
                                        "Unaligned scalar register range")
                    }
                    else if ((flags & INSTROP_UNALIGNED) == INSTROP_SGPR_UNALIGNED)
                        if ((rstart & 0xfc) != ((rend-1) & 0xfc))
                            // unaligned, but some restrictions:
                            // two regs can be in single 4-dword register line
                            ASM_FAIL_BY_ERROR(regRangePlace,
                                    "Scalar register range cross two register lines")
                }
                
                // set reg var usage for current position and instruction field
                if (regField != ASMFIELD_NONE)
                {
                    cxbyte align = 0;
                    if (regVar != nullptr)
                        align = getRegVarAlign(regVar, regsNum, flags);
                    
                    gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), regVar,
                        uint16_t(rstart), uint16_t(rend), regField,
                        cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                        ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), align });
                }
                regPair = { rstart, rend, regVar };
            }
            else
                regPair = { rstart, 0, nullptr };
            
            return true;
        }
    }
    if (printRegisterRangeExpected(asmr, regRangePlace, regTypeName, regsNum, required))
        return false;
    regPair = { 0, 0 }; // no range
    linePtr = oldLinePtr; // revert current line pointer
    return true;
}

bool GCNAsmUtils::parseVRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                    cxuint regsNum, AsmRegField regField, bool required, Flags flags)
{
    const char* oldLinePtr = linePtr;
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* vgprRangePlace = linePtr;
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    bool isRange = false;
    try /* for handling parse exception */
    {
        if (linePtr!=end && toLower(*linePtr) == 'v' && linePtr+1 != end)
        {
            if (isDigit(linePtr[1]))
            {
                const char* oldPlace = linePtr;
                linePtr++;
                // if single register
                cxuint value = cstrtobyte(linePtr, end);
                // check whether is register name: same v0, v1, but not v43xxx
                if (linePtr==end || (!isAlpha(*linePtr) && *linePtr!='_' &&
                            *linePtr!='$' && *linePtr!='.'))
                {
                    if (regsNum!=0 && regsNum != 1)
                    {
                        printXRegistersRequired(asmr, vgprRangePlace, "vector", regsNum);
                        return false;
                    }
                    regPair = { 256+value, 256+value+1 };
                    
                    // set reg var usage for current position and instruction field
                    if (regField != ASMFIELD_NONE)
                        gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), nullptr,
                            regPair.start, regPair.end, regField,
                            cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                            ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), 0 });
                    
                    return true;
                }
                else // if not register name
                    linePtr = oldPlace;
            }
            else if (linePtr[1]=='[')
                isRange = true;
        }
    } catch(const ParseException& ex)
    {
        asmr.printError(linePtr, ex.what());
        return false;
    }
    // if is not range: try to parse regvar or symbol with register range
    if (!isRange)
    {
        linePtr = oldLinePtr;
        if (!parseRegVarRange(asmr, linePtr, regPair, 0, regsNum, regField,
                    INSTROP_VREGS | (flags&INSTROP_ACCESS_MASK), false))
            return false;
        if (!regPair && (flags&INSTROP_SYMREGRANGE) != 0)
        {
            // try to parse regrange symbol
            linePtr = oldLinePtr;
            return parseSymRegRange(asmr, linePtr, regPair, 0, regsNum,
                        regField, INSTROP_VREGS|(flags&INSTROP_ACCESS_MASK), required);
        }
        else if (regPair)
            return true;
        if (printRegisterRangeExpected(asmr, vgprRangePlace, "vector", regsNum, required))
            return false;
        regPair = { 0, 0 }; // no range
        linePtr = oldLinePtr; // revert current line pointer
        return true;
    }
    linePtr++;
    
    try /* for handling parse exception */
    {
        // many registers
        uint64_t value1, value2;
        skipCharAndSpacesToEnd(linePtr, end);
        // parse first register index
        if (!getAbsoluteValueArg(asmr, value1, linePtr, true))
            return false;
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || (*linePtr!=':' && *linePtr!=']'))
            // error
            ASM_FAIL_BY_ERROR(vgprRangePlace, "Unterminated vector register range")
        if (linePtr!=end && *linePtr==':')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            // parse last register index
            if (!getAbsoluteValueArg(asmr, value2, linePtr, true))
                return false;
        }
        else
            value2 = value1;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
            // error
            ASM_FAIL_BY_ERROR(vgprRangePlace, "Unterminated vector register range")
        ++linePtr;
        
        if (value2 < value1)
            // error (illegal register range)
            ASM_FAIL_BY_ERROR(vgprRangePlace, "Illegal vector register range")
        if (value1 >= 256 || value2 >= 256)
            ASM_FAIL_BY_ERROR(vgprRangePlace, "Some vector register number out of range")
        
        if (regsNum!=0 && regsNum != value2-value1+1)
        {
            printXRegistersRequired(asmr, vgprRangePlace, "vector", regsNum);
            return false;
        }
        regPair = { 256+value1, 256+value2+1 };
        
        // set reg var usage for current position and instruction field
        if (regField != ASMFIELD_NONE)
            gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), nullptr,
                regPair.start, regPair.end, regField,
                cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), 0 });
        return true;
    } catch(const ParseException& ex)
    {
        asmr.printError(linePtr, ex.what());
        return false;
    }
}

bool GCNAsmUtils::parseSRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                    GPUArchMask arch, cxuint regsNum, AsmRegField regField,
                    bool required, Flags flags)
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
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    bool isRange = false;
    bool ttmpReg = false;
    bool singleSorTtmp = false;
    try
    {
    const char* oldPlace = linePtr;
    if (linePtr+4 < end && toLower(linePtr[0]) == 't' &&
        toLower(linePtr[1]) == 't' && toLower(linePtr[2]) == 'm' &&
        toLower(linePtr[3]) == 'p')
    {
        // we have ttmp registers
        singleSorTtmp = ttmpReg = true;
        linePtr += 4;
    }
    else if (linePtr!=end && toLower(linePtr[0]) == 's' && linePtr+1 != end)
    {
        singleSorTtmp = true;
        linePtr++;
    }
    
    /* parse single SGPR */
    const bool isGCN14 = (arch & ARCH_GCN_1_4) != 0;
    const cxuint ttmpSize = isGCN14 ? 16 : 12;
    const cxuint ttmpStart = isGCN14 ? 108 : 112;
        
    const cxuint maxSGPRsNum = getGPUMaxRegsNumByArchMask(arch, REGTYPE_SGPR);
    if (singleSorTtmp)
    {
        if (isDigit(*linePtr))
        {
            // if single register
            cxuint value = cstrtobyte(linePtr, end);
            // check wether is register name: s0, s4, but not s63v, s0.XX
            if (linePtr==end || (!isAlpha(*linePtr) && *linePtr!='_' &&
                    *linePtr!='$' && *linePtr!='.'))
            {
                if (!ttmpReg)
                {
                    // if scalar register
                    if (value >= maxSGPRsNum)
                        ASM_FAIL_BY_ERROR(sgprRangePlace,
                                        "Scalar register number out of range")
                }
                else
                {
                    // ttmp register
                    if (value >= ttmpSize)
                        ASM_FAIL_BY_ERROR(sgprRangePlace,
                            isGCN14 ? "TTMPRegister number out of range (0-15)" :
                            "TTMPRegister number out of range (0-11)")
                }
                if (regsNum!=0 && regsNum!=1)
                {
                    printXRegistersRequired(asmr, linePtr, "scalar", regsNum);
                    return false;
                }
                if (!ttmpReg)
                {
                    regPair = { value, value+1 };
                    // set reg var usage for current position and instruction field
                    if (regField != ASMFIELD_NONE)
                        gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), nullptr,
                            regPair.start, regPair.end, regField,
                            cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                            ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), 0 });
                }
                else
                    regPair = { ttmpStart+value, ttmpStart+value+1 };
                return true;
            }
            else // not register name
                linePtr = oldPlace;
        }
        else if (*linePtr=='[')
            isRange = true;
        else
            linePtr = oldPlace;
    }
    
    bool doubleReg = false; // register that have two 32-bit regs
    bool specialSGPRReg = false;
    if (!isRange) // if not sgprs
    {
        const char* oldLinePtr = linePtr;
        char regName[20];
        if (linePtr==end || *linePtr != '@')
        {
            // if not '@'
            if (!getNameArg(asmr, 20, regName, linePtr, "register name", required, true))
                return false;
        }
        else // otherwise reset regName
            regName[0] = 0;
        toLowerString(regName);
        
        size_t loHiRegSuffix = 0;
        cxuint loHiReg = 0;
        if (regName[0] == 'v' && regName[1] == 'c' && regName[2] == 'c')
        {
            /// vcc
            loHiRegSuffix = 3;
            loHiReg = 106;
            specialSGPRReg = true;
        }
        else if (::strncmp(regName, "exec", 4)==0)
        {
            /* exec* */
            loHiRegSuffix = 4;
            loHiReg = 126;
        }
        else if ((arch & ARCH_GCN_1_4) == 0 && regName[0]=='t')
        {
            /* tma,tba */
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
            /* M0 */
            if (regsNum!=0 && regsNum!=1 && regsNum!=2)
            {
                printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
                return false;
            }
            regPair = { 124, 125 };
            return true;
        }
        else if (arch&ARCH_GCN_1_1_2_4)
        {
            if (::strncmp(regName, "flat_scratch", 12)==0)
            {
                // flat
                loHiRegSuffix = 12;
                loHiReg = (arch&ARCH_GCN_1_2_4)?102:104;
                specialSGPRReg = true;
            }
            else if ((arch&ARCH_GCN_1_2_4)!=0 && ::strncmp(regName, "xnack_mask", 10)==0)
            {
                // xnack
                loHiRegSuffix = 10;
                loHiReg = 104;
                specialSGPRReg = true;
            }
        }
        
        bool trySymReg = false;
        if (loHiRegSuffix != 0) // handle 64-bit registers
        {
            if (regName[loHiRegSuffix] == '_')
            {
                bool isLoHiName = false;
                // if suffix _lo
                if (regName[loHiRegSuffix+1] == 'l' && regName[loHiRegSuffix+2] == 'o' &&
                    regName[loHiRegSuffix+3] == 0)
                {
                    regPair = { loHiReg, loHiReg+1 };
                    isLoHiName = true;
                }
                // if suffxi _hi
                else if (regName[loHiRegSuffix+1] == 'h' &&
                    regName[loHiRegSuffix+2] == 'i' && regName[loHiRegSuffix+3] == 0)
                {
                    regPair = { loHiReg+1, loHiReg+2 };
                    isLoHiName = true;
                }
                if (isLoHiName)
                {
                    if (regsNum!=0 && regsNum != 1)
                    {
                        printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
                        return false;
                    }
                    
                    // set reg var usage for current position and instruction field
                    if (regField != ASMFIELD_NONE && specialSGPRReg)
                        gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), nullptr,
                            regPair.start, regPair.end, regField,
                            cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                            ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), 0 });
                    return true;
                }
                else
                    trySymReg = true;
            }
            else if (regName[loHiRegSuffix] == 0)
            {
                // full 64-bit register
                regPair = { loHiReg, loHiReg+2 };
                
                if (linePtr == end || *linePtr!='[')
                {
                    if (regsNum!=0 && regsNum != 2)
                    {
                        printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
                        return false;
                    }
                    // set reg var usage for current position and instruction field
                    if (regField != ASMFIELD_NONE && specialSGPRReg)
                        gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), nullptr,
                            regPair.start, regPair.end, regField,
                            cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                            ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), 0 });
                    return true;
                }
                else
                {
                    // if regrange []
                    isRange = true;
                    doubleReg = true;
                }
            }
            else // this is not this register
                trySymReg = true;
        }
        else
            trySymReg = true;
        
        if (trySymReg)
        {
            // otherwise, we try to parse regvar or symreg
            linePtr = oldLinePtr;
            if (!parseRegVarRange(asmr, linePtr, regPair, 0, regsNum, regField,
                    INSTROP_SREGS|(flags&(INSTROP_ACCESS_MASK|INSTROP_UNALIGNED)), false))
                return false;
            if (!regPair && (flags&INSTROP_SYMREGRANGE) != 0)
            {
                linePtr = oldLinePtr;
                return parseSymRegRange(asmr, linePtr, regPair, arch, regsNum,
                        regField, INSTROP_SREGS |
                        (flags & (INSTROP_ACCESS_MASK|INSTROP_UNALIGNED)), required);
            }
            else if (regPair)
                return true;
            if (printRegisterRangeExpected(asmr, sgprRangePlace, "scalar",
                            regsNum, required))
                return false;
            regPair = { 0, 0 };
            linePtr = oldLinePtr; // revert current line pointer
            return true;
        }
    }
    
    {
        // many registers
        uint64_t value1, value2;
        skipCharAndSpacesToEnd(linePtr, end);
        // parse first register index
        if (!getAbsoluteValueArg(asmr, value1, linePtr, true))
            return false;
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || (*linePtr!=':' && *linePtr!=']'))
            // error
            ASM_FAIL_BY_ERROR(sgprRangePlace, (!ttmpReg) ?
                        "Unterminated scalar register range" :
                        "Unterminated TTMPRegister range")
        if (linePtr!=end && *linePtr==':')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            // parse last register index
            if (!getAbsoluteValueArg(asmr, value2, linePtr, true))
                return false;
        }
        else
            value2 = value1;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr == end || *linePtr != ']')
        {
            // error
            const char* errMessage = (doubleReg ? "Unterminated double register range" :
                    (ttmpReg ? "Unterminated TTMPRegister range" :
                            "Unterminated scalar register range"));
            ASM_FAIL_BY_ERROR(sgprRangePlace, errMessage)
        }
        ++linePtr;
        
        if (!ttmpReg && !doubleReg)
        {
            // is scalar register
            if (value2 < value1)
                // error (illegal register range)
                ASM_FAIL_BY_ERROR(sgprRangePlace, "Illegal scalar register range")
            if (value1 >= maxSGPRsNum || value2 >= maxSGPRsNum)
                ASM_FAIL_BY_ERROR(sgprRangePlace,
                            "Some scalar register number out of range")
        }
        else if (!doubleReg)
        {
            // is TTMP register
            if (value2 < value1)
                // error (illegal register range)
                ASM_FAIL_BY_ERROR(sgprRangePlace, "Illegal TTMPRegister range")
            if (value1 >= ttmpSize || value2 >= ttmpSize)
                ASM_FAIL_BY_ERROR(sgprRangePlace,
                            isGCN14 ? "Some TTMPRegister number out of range (0-15)" :
                            "Some TTMPRegister number out of range (0-11)")
        }
        else
        {
            // double reg - VCC, flat_scratch, xnack_mask
            if (value2 < value1)
                // error (illegal register range)
                ASM_FAIL_BY_ERROR(sgprRangePlace, "Illegal doublereg range")
            if (value1 >= 2 || value2 >= 2)
                ASM_FAIL_BY_ERROR(sgprRangePlace,
                            "Some doublereg number out of range (0-1)")
        }
        
        if (regsNum!=0 && regsNum != value2-value1+1)
        {
            printXRegistersRequired(asmr, sgprRangePlace, "scalar", regsNum);
            return false;
        }
        /// check alignment
        if (!ttmpReg && !doubleReg)
        {
            if ((flags & INSTROP_UNALIGNED)==0)
            {
                if ((value2-value1==1 && (value1&1)!=0) ||
                    (value2-value1>1 && (value1&3)!=0))
                    ASM_FAIL_BY_ERROR(sgprRangePlace, "Unaligned scalar register range")
            }
            else  if ((flags & INSTROP_UNALIGNED)==INSTROP_SGPR_UNALIGNED)
                if ((value1 & 0xfc) != ((value2) & 0xfc))
                   // unaligned, but some restrictions
                    // two regs can be in single 4-dword register line
                    ASM_FAIL_BY_ERROR(sgprRangePlace,
                            "Scalar register range cross two register lines")
            regPair = { value1, uint16_t(value2)+1 };
            
            // set reg var usage for current position and instruction field
            if (regField != ASMFIELD_NONE)
                gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), nullptr,
                    regPair.start, regPair.end, regField,
                    cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                    ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), 0 });
        }
        else if (!doubleReg)
            regPair = { ttmpStart+value1, ttmpStart+uint16_t(value2)+1 };
        else
        {
            regPair = { regPair.start+value1, regPair.start+uint16_t(value2)+1 };
            // set reg var usage for current position and instruction field
            if (regField != ASMFIELD_NONE && specialSGPRReg)
                gcnAsm->setRegVarUsage({ size_t(asmr.currentOutPos), nullptr,
                    regPair.start, regPair.end, regField,
                    cxbyte(((flags & INSTROP_READ)!=0 ? ASMRVU_READ: 0) |
                    ((flags & INSTROP_WRITE)!=0 ? ASMRVU_WRITE : 0)), 0 });
        }
        return true;
    }
    } catch(const ParseException& ex)
    {
        asmr.printError(linePtr, ex.what());
        return false;
    }
}

// internal routine to parse immediate (with specified number of bits and signess)
// if expression is not resolved, then returns expresion to outTargetExpr
bool GCNAsmUtils::parseImmInt(Assembler& asmr, const char*& linePtr, uint32_t& outValue,
            std::unique_ptr<AsmExpression>* outTargetExpr, cxuint bits, cxbyte signess)
{
    const char* end = asmr.line+asmr.lineSize;
    if (outTargetExpr!=nullptr)
        outTargetExpr->reset();
    skipSpacesToEnd(linePtr, end);
    
    uint64_t value;
    const char* exprPlace = linePtr;
    // if fast expression
    if (AsmExpression::fastExprEvaluate(asmr, linePtr, value))
    {
        if (bits != UINT_MAX && bits < 64)
        {
            asmr.printWarningForRange(bits, value,
                            asmr.getSourcePos(exprPlace), signess);
            outValue = value & ((1ULL<<bits)-1ULL);
        }
        else // just copy
            outValue = value;
        return true;
    }
    
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr));
    if (expr==nullptr) // error
        return false;
    if (expr->isEmpty())
        ASM_FAIL_BY_ERROR(exprPlace, "Expected expression")
    if (expr->getSymOccursNum()==0)
    {
        // resolved now
        cxuint sectionId; // for getting
        if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
            return false;
        else if (sectionId != ASMSECT_ABS)
            // if not absolute value
            ASM_FAIL_BY_ERROR(exprPlace, "Expression must be absolute!")
        if (bits != UINT_MAX && bits < 64)
        {
            asmr.printWarningForRange(bits, value,
                            asmr.getSourcePos(exprPlace), signess);
            outValue = value & ((1ULL<<bits)-1ULL);
        }
        else // just copy
            outValue = value;
        return true;
    }
    else
    {
        // return output expression with symbols to resolve
        if (outTargetExpr!=nullptr)
            *outTargetExpr = std::move(expr);
        else
            ASM_FAIL_BY_ERROR(exprPlace, "Unresolved expression is illegal in this place")
        return true;
    }
}

enum FloatLitType
{
    FLTT_F16,
    FLTT_F32,
    FLTT_F64
};

// determinr float literal type from suffix
static FloatLitType getFloatLitType(const char* str, const char* end,
                        FloatLitType defaultFPType)
{
    if (str==end)
        return defaultFPType; // end of string and no replacing suffix
    else if (toLower(*str)=='l')
        return FLTT_F64;
    else if (toLower(*str)=='s')
        return FLTT_F32;
    else if (toLower(*str)=='h')
        return FLTT_F16;
    else
        return defaultFPType;
}

/* check whether string is exclusively floating point value
 * (only floating point, and neither integer and nor symbol) */
static bool isOnlyFloat(const char* str, const char* end, FloatLitType defaultFPType,
                        FloatLitType& outFPType)
{
    if (str == end)
        return false;
    if (*str=='-' || *str=='+')
        str++; // skip '-' or '+'
    if (str+2 < end && *str=='0' && (str[1]=='X' || str[1]=='x'))
    {
        // hexadecimal
        str += 2;
        const char* beforeComma = str;
        while (str!=end && isXDigit(*str)) str++;
        const char* point = str;
        if (str == end || *str!='.')
        {
            if (beforeComma-point!=0 && str!=end && (*str=='p' || *str=='P'))
            {
                // handle part P[+|-]exp
                str++;
                if (str!=end && (*str=='-' || *str=='+'))
                    str++;
                const char* expPlace = str;
                while (str!=end && isDigit(*str)) str++;
                if (str-expPlace!=0)
                {
                    outFPType = getFloatLitType(str, end, defaultFPType);
                    return true; // if 'XXXp[+|-]XXX'
                }
            }
            return false; // no '.'
        }
        // if XXX.XXX
        str++;
        while (str!=end && isXDigit(*str)) str++;
        const char* afterComma = str;
        
        if (point-beforeComma!=0 || afterComma-(point+1)!=0)
        {
            if (beforeComma-point!=0 && str!=end && (*str=='p' || *str=='P'))
            {
                // handle part P[+|-]exp
                str++;
                if (str!=end && (*str=='-' || *str=='+'))
                    str++;
                while (str!=end && isDigit(*str)) str++;
            }
            outFPType = getFloatLitType(str, end, defaultFPType);
            return true;
        }
    }
    else
    {
        // decimal
        const char* beforeComma = str;
        while (str!=end && isDigit(*str)) str++;
        const char* point = str;
        if (str == end || *str!='.')
        {
            if (beforeComma-point!=0 && str!=end && (*str=='e' || *str=='E'))
            {
                // handle part E[+|-]exp
                str++;
                if (str!=end && (*str=='-' || *str=='+'))
                    str++;
                const char* expPlace = str;
                while (str!=end && isDigit(*str)) str++;
                if (str-expPlace!=0)
                {
                    outFPType = getFloatLitType(str, end, defaultFPType);
                    return true; // if 'XXXe[+|-]XXX'
                }
            }
            return false; // no '.'
        }
        // if XXX.XXX
        str++;
        while (str!=end && isDigit(*str)) str++;
        const char* afterComma = str;
        
        if (point-beforeComma!=0 || afterComma-(point+1)!=0)
        {
            if (beforeComma-point!=0 && str!=end && (*str=='e' || *str=='E'))
            {
                // handle part E[+|-]exp
                str++;
                if (str!=end && (*str=='-' || *str=='+'))
                    str++;
                while (str!=end && isDigit(*str)) str++;
            }
            outFPType = getFloatLitType(str, end, defaultFPType);
            return true;
        }
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
    FloatLitType fpType;
    FloatLitType defaultFpType = (instropMask&INSTROP_TYPE_MASK)!=INSTROP_F16 ?
            ((instropMask&INSTROP_TYPE_MASK)==INSTROP_V64BIT ?
            FLTT_F64 : FLTT_F32) : FLTT_F16;
    
    // before parsing, we check that is floating point and integer value
    if (isOnlyFloat(linePtr, end, defaultFpType, fpType))
    {
        // try to parse floating point FP16, FP32 or high 32-bit part of FP64
        try
        {
        if (fpType==FLTT_F16)
        {
            value = cstrtohCStyle(linePtr, end, linePtr);
            if (linePtr!=end && toLower(*linePtr)=='h')
                linePtr++;
        }
        else if (fpType==FLTT_F32)
        {
            union FloatUnion { uint32_t i; float f; };
            FloatUnion v;
            v.f = cstrtovCStyle<float>(linePtr, end, linePtr);
            if (linePtr!=end && toLower(*linePtr)=='s')
                linePtr++;
            value = v.i;
        }
        else
        {
            /* 64-bit (high 32-bits) */
            uint32_t v = cstrtofXCStyle(linePtr, end, linePtr, 11, 20);
            if (linePtr!=end && toLower(*linePtr)=='l')
                linePtr++;
            value = v;
        }
        }
        catch(const ParseException& ex)
        {
            // error
            asmr.printError(linePtr, ex.what());
            return false;
        }
        return true;
    }
    return parseImm(asmr, linePtr, value, outTargetExpr);
}

// source register names (EXECZ,SCC,VCCZ, ...) for GCN1.0/1.1/1.2
static const std::pair<const char*, uint16_t> ssourceNamesTbl[] =
{
    { "execz", 252 },
    { "scc", 253 },
    { "src_execz", 252 },
    { "src_scc", 253 },
    { "src_vccz", 251 },
    { "vccz", 251 }
};

static const size_t ssourceNamesTblSize = sizeof(ssourceNamesTbl) /
        sizeof(std::pair<const char*, uint16_t>);

// source register names (EXECZ,SCC,VCCZ, ...) for GCN1.4
static const std::pair<const char*, uint16_t> ssourceNamesGCN14Tbl[] =
{
    { "execz", 252 },
    { "pops_exiting_wave_id", 0xef },
    { "private_base", 0xed },
    { "private_limit", 0xee },
    { "scc", 253 },
    { "shared_base", 0xeb },
    { "shared_limit", 0xec },
    { "src_execz", 252 },
    { "src_pops_exiting_wave_id", 0xef },
    { "src_private_base", 0xed },
    { "src_private_limit", 0xee },
    { "src_scc", 253 },
    { "src_shared_base", 0xeb },
    { "src_shared_limit", 0xec },
    { "src_vccz", 251 },
    { "vccz", 251 }
};

static const size_t ssourceNamesGCN14TblSize = sizeof(ssourceNamesGCN14Tbl) /
        sizeof(std::pair<const char*, uint16_t>);

// main routine to parse operand
bool GCNAsmUtils::parseOperand(Assembler& asmr, const char*& linePtr, GCNOperand& operand,
             std::unique_ptr<AsmExpression>* outTargetExpr, GPUArchMask arch,
             cxuint regsNum, Flags instrOpMask, AsmRegField regField)
{
    if (outTargetExpr!=nullptr)
        outTargetExpr->reset();
    
    if (asmr.buggyFPLit && (instrOpMask&INSTROP_TYPE_MASK)==INSTROP_V64BIT)
        // buggy fplit does not accept 64-bit values (high 32-bits)
        instrOpMask = (instrOpMask&~INSTROP_TYPE_MASK) | INSTROP_INT;
    
    const Flags optionFlags = (instrOpMask & (INSTROP_UNALIGNED|INSTROP_ACCESS_MASK));
    // fast path to parse only VGPR or SGPR
    if ((instrOpMask&~INSTROP_UNALIGNED) == INSTROP_SREGS)
        return parseSRegRange(asmr, linePtr, operand.range, arch, regsNum, regField, true,
                              INSTROP_SYMREGRANGE | optionFlags);
    else if ((instrOpMask&~INSTROP_UNALIGNED) == INSTROP_VREGS)
        return parseVRegRange(asmr, linePtr, operand.range, regsNum, regField, true,
                              INSTROP_SYMREGRANGE | optionFlags);
    
    // otherwise we must include modifiers and other registers or literals/constants
    const char* end = asmr.line+asmr.lineSize;
    if (instrOpMask & INSTROP_VOP3MODS)
    {
        operand.vopMods = 0;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr=='@') // treat this operand as expression
            return parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                             instrOpMask & ~INSTROP_VOP3MODS, regField);
        
        if ((arch & ARCH_GCN_1_2_4)!=0 &&
            (instrOpMask & (INSTROP_NOSEXT|INSTROP_VOP3P))==0 &&
            linePtr+4 <= end && strncasecmp(linePtr, "sext", 4)==0)
        {
            /* sext */
            linePtr += 4;
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr=='(')
            {
                operand.vopMods |= VOPOP_SEXT;
                ++linePtr;
            }
            else
                ASM_FAIL_BY_ERROR(linePtr, "Expected '(' after sext")
        }
        
        const char* negPlace = linePtr;
        // parse negation
        if (linePtr!=end && *linePtr=='-')
        {
            operand.vopMods |= VOPOP_NEG;
            skipCharAndSpacesToEnd(linePtr, end);
        }
        // parse abs modifier
        bool llvmAbs = false;
        if (linePtr+3 <= end && (instrOpMask & INSTROP_VOP3P)==0 &&
            toLower(linePtr[0])=='a' &&
            toLower(linePtr[1])=='b' && toLower(linePtr[2])=='s')
        {
            // if 'abs' operand modifier
            linePtr += 3;
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr=='(')
            {
                operand.vopMods |= VOPOP_ABS;
                ++linePtr;
            }
            else
                ASM_FAIL_BY_ERROR(linePtr, "Expected '(' after abs")
        }
        else if (linePtr<=end && linePtr[0]=='|')
        {
            // LLVM like syntax for abs modifier
            linePtr++;
            skipSpacesToEnd(linePtr, end);
            operand.vopMods |= VOPOP_ABS;
            llvmAbs = true;
        }
        
        bool good;
        // now we parse operand except VOP modifiers
        if ((operand.vopMods&(VOPOP_NEG|VOPOP_ABS)) != VOPOP_NEG)
            good = parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                                     instrOpMask & ~INSTROP_VOP3MODS, regField);
        else //
        {
            // parse with negation if neg (it can be literal with negation)
            linePtr = negPlace;
            good = parseOperand(asmr, linePtr, operand, outTargetExpr, arch, regsNum,
                     (instrOpMask & ~INSTROP_VOP3MODS) | INSTROP_PARSEWITHNEG, regField);
        }
        
        // checking closing of VOP modifiers
        if (operand.vopMods & VOPOP_ABS)
        {
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && ((*linePtr==')' && !llvmAbs) ||
                        (*linePtr=='|' && llvmAbs)))
                linePtr++;
            else
                ASM_FAIL_BY_ERROR(linePtr, "Unterminated abs() modifier")
        }
        if (operand.vopMods & VOPOP_SEXT)
        {
            skipSpacesToEnd(linePtr, end);
            if (linePtr!=end && *linePtr==')')
                linePtr++;
            else
                ASM_FAIL_BY_ERROR(linePtr, "Unterminated sext() modifier")
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
    
    // otherwise, we try parse scalar register
    if (instrOpMask & INSTROP_SREGS)
    {
        if (!parseSRegRange(asmr, linePtr, operand.range, arch, regsNum, regField,
                        false, optionFlags))
            return false;
        if (operand)
            return true;
    }
    // otherwise try parse vector register
    if (instrOpMask & INSTROP_VREGS)
    {
        if (!parseVRegRange(asmr, linePtr, operand.range, regsNum, regField,
                        false, optionFlags))
            return false;
        if (operand)
            return true;
    }
    // if still is not this, try parse symbol regrange
    if (instrOpMask & (INSTROP_SREGS|INSTROP_VREGS))
    {
        if (!parseSymRegRange(asmr, linePtr, operand.range, arch, regsNum, regField,
                  (instrOpMask&((INSTROP_SREGS|INSTROP_VREGS|INSTROP_SSOURCE))) |
                    optionFlags, false))
            return false;
        if (operand)
            return true;
    }
    
    skipSpacesToEnd(linePtr, end);
    
    if ((instrOpMask & INSTROP_SSOURCE)!=0)
    {
        // try parse scalar source (vcc, scc, literals)
        char regName[25];
        const char* regNamePlace = linePtr;
        if (getNameArg(asmr, 25, regName, linePtr, "register name", false, true))
        {
            toLowerString(regName);
            operand.range = {0, 0};
            
            auto regNameTblEnd = (arch & ARCH_GCN_1_4) ?
                        ssourceNamesGCN14Tbl + ssourceNamesGCN14TblSize :
                        ssourceNamesTbl + ssourceNamesTblSize;
            auto regNameIt = binaryMapFind(
                    (arch & ARCH_GCN_1_4) ? ssourceNamesGCN14Tbl : ssourceNamesTbl,
                    regNameTblEnd, regName, CStringLess());
            
            // if found in table
            if (regNameIt != regNameTblEnd)
            {
                operand.range = { regNameIt->second, regNameIt->second+1 };
                return true;
            }
            // if lds or src_lds_direct, lds_direct
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
            ASM_FAIL_BY_ERROR(linePtr, "Expected instruction operand")
        const char* exprPlace = linePtr;
        
        uint64_t value;
        operand.vopMods = 0; // zeroing operand modifiers
        FloatLitType fpType;
        
        bool exprToResolve = false;
        bool encodeAsLiteral = false;
        // if 'lit(' in this place
        if (linePtr+4<end && toLower(linePtr[0])=='l' && toLower(linePtr[1])=='i' &&
            toLower(linePtr[2])=='t' && (isSpace(linePtr[3]) || linePtr[3]=='('))
        {
            // lit() - force encoding as literal
            linePtr+=3;
            const char* oldLinePtr = linePtr;
            skipSpacesToEnd(linePtr, end);
            // space between lit and (
            if (linePtr!=end && *linePtr=='(')
            {
                encodeAsLiteral = true;
                linePtr++;
                skipSpacesToEnd(linePtr, end);
                negPlace = linePtr; // later treaten as next part of operand
            }
            else // back to expression start
                linePtr = oldLinePtr;
        }
        
        FloatLitType defaultFPType = (instrOpMask & INSTROP_TYPE_MASK)!=INSTROP_F16?
                    ((instrOpMask & INSTROP_TYPE_MASK)!=INSTROP_V64BIT?
                        FLTT_F32:FLTT_F64):FLTT_F16;
        if (!forceExpression && isOnlyFloat(negPlace, end, defaultFPType, fpType))
        {
            // if only floating point value
            /* if floating point literal can be processed */
            linePtr = negPlace;
            try
            {
                if (fpType==FLTT_F16)
                {
                    // parse FP16
                    value = cstrtohCStyle(linePtr, end, linePtr);
                    // skip suffix if needed
                    if (linePtr!=end && toLower(*linePtr)=='h')
                        linePtr++;
                    
                    if (!encodeAsLiteral)
                    {
                        if (asmr.buggyFPLit && value == 0)
                        {
                            // old buggy behaviour (to 0.1.2 version)
                            operand.range = { 128, 0 };
                            return true;
                        }
                    }
                }
                else if (fpType==FLTT_F32) /* otherwise, FLOAT */
                {
                    union FloatUnion { uint32_t i; float f; };
                    FloatUnion v;
                    v.f = cstrtovCStyle<float>(linePtr, end, linePtr);
                    // skip suffix if needed
                    if (linePtr!=end && toLower(*linePtr)=='s')
                        linePtr++;
                    value = v.i;
                    /// simplify to float constant immediate (-0.5, 0.5, 1.0, 2.0,...)
                    /// constant immediates converted only to single floating points
                    if (!encodeAsLiteral && asmr.buggyFPLit)
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
                                if (arch&ARCH_GCN_1_2_4)
                                {
                                    operand.range = { 248, 0 };
                                    return true;
                                }
                        }
                }
                else
                {
                    uint32_t v = cstrtofXCStyle(linePtr, end, linePtr, 11, 20);
                    // skip suffix if needed
                    if (linePtr!=end && toLower(*linePtr)=='l')
                        linePtr++;
                    value = v;
                }
                
                /// simplify to float constant immediate (-0.5, 0.5, 1.0, 2.0,...)
                /// constant immediates converted only to single floating points
                /// new behaviour
                if (!asmr.buggyFPLit && !encodeAsLiteral && fpType==defaultFPType)
                {
                    if (defaultFPType==FLTT_F16)
                        // simplify FP16 to constant immediate
                        switch (value)
                        {
                            case 0x0:
                                operand.range = { 128, 0 };
                                return true;
                            case 0x3800: // 0.5
                                operand.range = { 240, 0 };
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
                                if (arch&ARCH_GCN_1_2_4)
                                {
                                    operand.range = { 248, 0 };
                                    return true;
                                }
                        }
                    else if (defaultFPType==FLTT_F32)
                        // simplify FP32 to constant immediate
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
                                if (arch&ARCH_GCN_1_2_4)
                                {
                                    operand.range = { 248, 0 };
                                    return true;
                                }
                        }
                    else /* FP64 */
                        // simplify FP64 (only high part) to constant immediate
                        switch (value)
                        {
                            case 0x0:
                                operand.range = { 128, 0 };
                                return true;
                            case 0x3fe00000: // 0.5
                                operand.range = { 240, 0 };
                                return true;
                            case 0xbfe00000: // -0.5
                                operand.range = { 241, 0 };
                                return true;
                            case 0x3ff00000: // 1.0
                                operand.range = { 242, 0 };
                                return true;
                            case 0xbff00000: // -1.0
                                operand.range = { 243, 0 };
                                return true;
                            case 0x40000000: // 2.0
                                operand.range = { 244, 0 };
                                return true;
                            case 0xc0000000: // -2.0
                                operand.range = { 245, 0 };
                                return true;
                            case 0x40100000: // 4.0
                                operand.range = { 246, 0 };
                                return true;
                            case 0xc0100000: // -4.0
                                operand.range = { 247, 0 };
                                return true;
                            case 0x3fc45f30: // 1/(2*PI)
                                if (arch&ARCH_GCN_1_2_4)
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
        {
            if (!AsmExpression::fastExprEvaluate(asmr, linePtr, value))
            {
            // if expression
            std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr));
            if (expr==nullptr) // error
                return false;
            if (expr->isEmpty())
                ASM_FAIL_BY_ERROR(exprPlace, "Expected expression")
            
            bool tryLater = false;
            if (expr->getSymOccursNum()==0)
            {
                // resolved now
                cxuint sectionId; // for getting
                AsmTryStatus evalStatus = expr->tryEvaluate(asmr, value, sectionId,
                                        asmr.withSectionDiffs());
                
                if (evalStatus == AsmTryStatus::FAILED) // failed evaluation!
                    return false;
                else if (evalStatus == AsmTryStatus::TRY_LATER)
                {
                    if (outTargetExpr!=nullptr)
                        asmr.unevalExpressions.push_back(expr.get());
                    tryLater = true;
                }
                else if (sectionId != ASMSECT_ABS)
                    // if not absolute value
                    ASM_FAIL_BY_ERROR(exprPlace, "Expression must be absolute!")
            }
            else
                tryLater = true;
            
            if (tryLater)
            {
                // return output expression with symbols to resolve
                if ((instrOpMask & INSTROP_ONLYINLINECONSTS)!=0)
                {
                    // error
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
                exprToResolve = true;
            }
            }
            
            if (!encodeAsLiteral && !exprToResolve)
            {
                // if literal can be a constant immediate
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
        }
        if (encodeAsLiteral)
        {
            /* finish lit function */
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end || *linePtr!=')')
                ASM_FAIL_BY_ERROR(linePtr, "Expected ')' after expression at 'lit'")
            else // skip end of lit
                linePtr++;
        }
        if (exprToResolve) // finish if expression to resolve
            return true;
        
        if ((instrOpMask & INSTROP_ONLYINLINECONSTS)!=0)
        {
            // error
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

// used while parsing op_sel or op_sel_hi, neg_lo, neg_hi
bool GCNAsmUtils::parseImmWithBoolArray(Assembler& asmr, const char*& linePtr,
            uint32_t& value, cxuint bits, cxbyte signess)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end || *linePtr != '[')
        return parseImm(asmr, linePtr, value, nullptr, bits, signess);
    // array of boolean values
    linePtr++;
    bool good = true;
    uint32_t inVal = 0;
    for (cxuint i = 0; i < bits; i++)
    {
        uint32_t v = 0;
        // parse boolean immediate (0 or 1)
        good &= parseImm(asmr, linePtr, v, nullptr, 1, WS_UNSIGNED);
        inVal |= v<<i;
        skipSpacesToEnd(linePtr, end);
        if (i+1 < bits)
        {
            // next bool will be, try parse ','
            if (linePtr==end || *linePtr!=',')
            {
                ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ',' before bit value")
                break;
            }
            else
                ++linePtr;
        }
        else
        {
            // end of array, try parse ']'
            if (linePtr == end || *linePtr!=']')
                ASM_NOTGOOD_BY_ERROR(linePtr, "Unterminated bit array")
            else
                ++linePtr;
        }
    }
    if (good)
        value = inVal;
    return good;
}

bool GCNAsmUtils::parseSingleOMODCLAMP(Assembler& asmr, const char*& linePtr,
                    const char* modPlace, const char* mod, GPUArchMask arch,
                    cxbyte& mods, VOPOpModifiers& opMods, cxuint modOperands,
                    cxuint flags, bool& haveAbs, bool& haveNeg,
                    bool& alreadyModDefined, bool& good)
{
    const char* end = asmr.line+asmr.lineSize;
    const bool vop3p = (flags & PARSEVOP_VOP3P)!=0;
    if (!vop3p && ::strcmp(mod, "mul")==0)
    {
        // if 'mul:xx'
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
                ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown VOP3 mul:X modifier")
        }
        else
            ASM_NOTGOOD_BY_ERROR(linePtr,
                        "Expected ':' before multiplier number")
    }
    else if (!vop3p && ::strcmp(mod, "div")==0)
    {
        // if 'div:2'
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
                ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown VOP3 div:X modifier")
        }
        else
            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before divider number")
    }
    else if (!vop3p && ::strcmp(mod, "omod")==0)
    {
        // if omod (parametrization of div or mul)
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr==':')
        {
            linePtr++;
            cxbyte omod = 0;
            if (parseImm(asmr, linePtr, omod, nullptr, 2, WS_UNSIGNED))
                mods = (mods & ~3) | omod;
            else
                good = false;
        }
        else
            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before omod")
    }
    else if (::strcmp(mod, "clamp")==0) // clamp
    {
        bool clamp = false;
        good &= parseModEnable(asmr, linePtr, clamp, "clamp modifier");
        if (flags & PARSEVOP_WITHCLAMP)
            mods = (mods & ~VOP3_CLAMP) | (clamp ? VOP3_CLAMP : 0);
        else
            ASM_NOTGOOD_BY_ERROR(modPlace, "Modifier CLAMP in VOP3B is illegal")
    }
    else if (!vop3p && modOperands>1 && ::strcmp(mod, "abs")==0)
    {
        // abs modifiers for source operands (bit per operand in array)
        uint32_t absVal = 0;
        if (linePtr!=end && *linePtr==':')
        {
            linePtr++;
            if (parseImmWithBoolArray(asmr, linePtr, absVal, modOperands-1,
                        WS_UNSIGNED))
            {
                opMods.absMod = absVal;
                if (haveAbs)
                    asmr.printWarning(modPlace, "Abs is already defined");
                haveAbs = true;
            }
        }
        else
            good = false;
    }
    else if (modOperands>1 && (::strcmp(mod, "neg")==0 ||
            (vop3p && ::strcmp(mod, "neg_lo")==0)))
    {
        // neg or neg_lo modifiers for source operands 
        // (bit per operand in array)
        uint32_t negVal = 0;
        if (linePtr!=end && *linePtr==':')
        {
            linePtr++;
            if (parseImmWithBoolArray(asmr, linePtr, negVal, modOperands-1,
                            WS_UNSIGNED))
            {
                opMods.negMod = (opMods.negMod&0xf0) | negVal;
                if (haveNeg)
                    asmr.printWarning(modPlace, "Neg is already defined");
                haveNeg = true;
            }
        }
        else
            good = false;
    }
    else // other modifier
        return false;
    return true;
}

// sorted list of VOP SDWA DST_SEL names
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
            
/* main routine to parse VOP modifiers: basic modifiers stored in mods parameter,
 * modifier specific for VOP_SDWA and VOP_DPP stored in extraMods structure
 * withSDWAOperands - specify number of operand for that modifier will be parsed */
bool GCNAsmUtils::parseVOPModifiers(Assembler& asmr, const char*& linePtr,
                GPUArchMask arch, cxbyte& mods, VOPOpModifiers& opMods, cxuint modOperands,
                VOPExtraModifiers* extraMods, cxuint flags, cxuint withSDWAOperands)
{
    const char* end = asmr.line+asmr.lineSize;
    //bool haveSDWAMod = false, haveDPPMod = false;
    bool haveDstSel = false, haveSrc0Sel = false, haveSrc1Sel = false;
    bool haveDstUnused = false;
    bool haveBankMask = false, haveRowMask = false;
    bool haveBoundCtrl = false, haveDppCtrl = false;
    bool haveNeg = false, haveAbs = false;
    bool haveSext = false, haveOpsel = false;
    bool haveNegHi = false, haveOpselHi = false;
    bool haveDPP = false, haveSDWA = false;
    
    // set default VOP extra modifiers (SDWA/DPP)
    if (extraMods!=nullptr)
        *extraMods = { 6, 0, cxbyte((withSDWAOperands>=2)?6:0),
                    cxbyte((withSDWAOperands>=3)?6:0),
                    15, 15, 0xe4 /* TODO: why not 0xe4? */, false, false };
    
    skipSpacesToEnd(linePtr, end);
    const char* modsPlace = linePtr;
    bool good = true;
    mods = 0;
    const bool vop3p = (flags & PARSEVOP_VOP3P)!=0;
    
    // main loop
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
                // check what is VOP modifier
                bool alreadyModDefined = false;
                if (parseSingleOMODCLAMP(asmr, linePtr, modPlace, mod, arch, mods,
                        opMods, modOperands, flags, haveAbs, haveNeg,
                        alreadyModDefined, good))
                {   // do nothing
                }
                else if (modOperands>1 && vop3p && ::strcmp(mod, "neg_hi")==0)
                {
                    // neg_hi modifiers for source operands (bit per operand in array)
                    uint32_t negVal = 0;
                    if (linePtr!=end && *linePtr==':')
                    {
                        linePtr++;
                        if (parseImmWithBoolArray(asmr, linePtr, negVal, modOperands-1,
                                        WS_UNSIGNED))
                        {
                            opMods.negMod = (opMods.negMod&15) | (negVal<<4);
                            if (haveNegHi)
                                asmr.printWarning(modPlace, "Neg_hi is already defined");
                            haveNegHi = true;
                        }
                    }
                    else
                        good = false;
                }
                else if (!vop3p &&(arch & ARCH_GCN_1_2_4) &&
                        (flags & PARSEVOP_WITHSEXT)!=0 &&
                         modOperands>1 && ::strcmp(mod, "sext")==0)
                {
                    // neg_hi modifiers for source operands (bit per operand in array)
                    uint32_t sextVal = 0;
                    if (linePtr!=end && *linePtr==':')
                    {
                        linePtr++;
                        if (parseImmWithBoolArray(asmr, linePtr, sextVal, modOperands-1,
                                    WS_UNSIGNED))
                        {
                            opMods.sextMod = sextVal;
                            if (haveSext)
                                asmr.printWarning(modPlace, "Sext is already defined");
                            haveSext = true;
                        }
                    }
                    else
                        good = false;
                }
                else if ((flags & PARSEVOP_WITHOPSEL) != 0 && modOperands>1 &&
                            ::strcmp(mod, "op_sel")==0)
                {
                    // op_sel (bit per operand in array)
                    uint32_t opselVal = 0;
                    if (linePtr!=end && *linePtr==':')
                    {
                        linePtr++;
                        // for VOP3P encoding is one less bit in array
                        if (parseImmWithBoolArray(asmr, linePtr, opselVal,
                                (vop3p ? modOperands-1 : modOperands),
                                    WS_UNSIGNED))
                        {
                            if (!vop3p && modOperands<4)
                                /* move last bit to dest (fourth bit)
                                 * if less than 4 operands */
                                opselVal = (opselVal & ((1U<<(modOperands-1))-1)) |
                                        ((opselVal & (1U<<(modOperands-1))) ? 8 : 0);
                            opMods.opselMod = (opMods.opselMod&0xf0) | opselVal;
                            if (haveOpsel)
                                asmr.printWarning(modPlace, "Opsel is already defined");
                            haveOpsel = true;
                        }
                    }
                    else
                        good = false;
                }
                else if (vop3p && (flags & PARSEVOP_WITHOPSEL) != 0 && modOperands>1 &&
                            ::strcmp(mod, "op_sel_hi")==0)
                {
                    // op_sel_hi (bit per operand in array)
                    uint32_t opselVal = 0;
                    if (linePtr!=end && *linePtr==':')
                    {
                        linePtr++;
                        // for VOP3P encoding is one less bit in array
                        if (parseImmWithBoolArray(asmr, linePtr, opselVal, modOperands-1,
                                    WS_UNSIGNED))
                        {
                            opMods.opselMod = (opMods.opselMod&15) | (opselVal<<4);
                            if (haveOpselHi)
                                asmr.printWarning(modPlace, "Opsel_hi is already defined");
                            haveOpselHi = true;
                        }
                    }
                    else
                        good = false;
                }
                else if (::strcmp(mod, "vop3")==0)
                {
                    bool vop3 = false;
                    good &= parseModEnable(asmr, linePtr, vop3, "vop3 modifier");
                    mods = (mods & ~VOP3_VOP3) | (vop3 ? VOP3_VOP3 : 0);
                }
                else if (extraMods!=nullptr)
                {
                    /* parse specific modofier from VOP_SDWA or VOP_DPP encoding */
                    if (withSDWAOperands>=1 && (flags&PARSEVOP_NODSTMODS)==0 &&
                            ::strcmp(mod, "dst_sel")==0)
                    {
                        // dstsel
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint dstSel = 0;
                            if (linePtr == end || *linePtr!='@')
                            {
                                // parse name of dst_sel
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
                                /* parametrize (if in form '@value') */
                                linePtr++;
                                if (parseImm(asmr, linePtr, dstSel, nullptr,
                                                 3, WS_UNSIGNED))
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
                        }
                        else
                            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before dst_sel")
                    }
                    else if (withSDWAOperands>=1 && (flags&PARSEVOP_NODSTMODS)==0 &&
                        (::strcmp(mod, "dst_unused")==0 || ::strcmp(mod, "dst_un")==0))
                    {
                        /* dst_unused modifer */
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            skipCharAndSpacesToEnd(linePtr, end);
                            cxbyte unused = 0;
                            if (linePtr == end || *linePtr!='@')
                            {
                                // parse name of dst_unused
                                char name[20];
                                const char* enumPlace = linePtr;
                                if (getNameArg(asmr, 20, name, linePtr, "dst_unused"))
                                {
                                    toLowerString(name);
                                    size_t namePos =
                                            (::strncmp(name, "unused_", 7)==0) ?7 : 0;
                                    if (::strcmp(name+namePos, "sext")==0)
                                        unused = 1;
                                    else if (::strcmp(name+namePos, "preserve")==0)
                                        unused = 2;
                                    else if (::strcmp(name+namePos, "pad")!=0)
                                        ASM_NOTGOOD_BY_ERROR(enumPlace,
                                                    "Unknown dst_unused")
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
                                /* '@' in dst_unused: parametrization */
                                linePtr++;
                                if (parseImm(asmr, linePtr, unused, nullptr,
                                                 2, WS_UNSIGNED))
                                {
                                    extraMods->dstUnused = unused;
                                    if (haveDstUnused)
                                        asmr.printWarning(modPlace,
                                                        "Dst_unused is already defined");
                                    haveDstUnused = true;
                                }
                                else
                                    good = false;
                            }
                        }
                        else
                            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before dst_unused")
                    }
                    else if (withSDWAOperands>=2 && ::strcmp(mod, "src0_sel")==0)
                    {
                        /* src0_sel modifier */
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint src0Sel = 0;
                            if (linePtr == end || *linePtr!='@')
                            {
                                // parse source selection (name)
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
                                /* parametrize (if in form '@value') */
                                linePtr++;
                                if (parseImm(asmr, linePtr, src0Sel, nullptr,
                                                 3, WS_UNSIGNED))
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
                        }
                        else
                            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before src0_sel")
                    }
                    else if (withSDWAOperands>=3 && ::strcmp(mod, "src1_sel")==0)
                    {
                        // src1_sel modifier
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            linePtr++;
                            cxuint src1Sel = 0;
                            if (linePtr == end || *linePtr!='@')
                            {
                                // parse source selection (name)
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
                                /* parametrize by '@' */
                                linePtr++;
                                if (parseImm(asmr, linePtr, src1Sel, nullptr,
                                                 3, WS_UNSIGNED))
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
                        }
                        else
                            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before src1_sel")
                    }
                    else if (::strcmp(mod, "quad_perm")==0)
                    {
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            // parse quad_perm array
                            bool goodMod = true;
                            skipCharAndSpacesToEnd(linePtr, end);
                            if (linePtr==end || *linePtr!='[')
                            {
                                ASM_NOTGOOD_BY_ERROR1(goodMod = good, linePtr,
                                        "Expected '[' before quad_perm list")
                                continue;
                            }
                            cxbyte quadPerm = 0;
                            linePtr++;
                            // parse four 2-bit values
                            for (cxuint k = 0; k < 4; k++)
                            {
                                skipSpacesToEnd(linePtr, end);
                                cxbyte qpv = 0;
                                good &= parseImm(asmr, linePtr, qpv, nullptr,
                                        2, WS_UNSIGNED);
                                quadPerm |= qpv<<(k<<1);
                                skipSpacesToEnd(linePtr, end);
                                if (k!=3)
                                {
                                    // skip ',' before next value
                                    if (linePtr==end || *linePtr!=',')
                                    {
                                        ASM_NOTGOOD_BY_ERROR1(goodMod = good, linePtr,
                                            "Expected ',' before quad_perm component")
                                        break;
                                    }
                                    else
                                        ++linePtr;
                                }
                                else if (linePtr==end || *linePtr!=']')
                                {
                                    // unterminated quad_perm
                                    asmr.printError(linePtr, "Unterminated quad_perm");
                                    goodMod = good = false;
                                }
                                else
                                    ++linePtr;
                            }
                            if (goodMod)
                            {
                                // set up quad perm
                                extraMods->dppCtrl = quadPerm;
                                if (haveDppCtrl)
                                    asmr.printWarning(modPlace,
                                              "DppCtrl is already defined");
                                haveDppCtrl = true;
                            }
                        }
                        else
                            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before quad_perm")
                    }
                    else if (::strcmp(mod, "bank_mask")==0)
                    {
                        // parse bank_mask with 4-bit value
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
                            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before bank_mask")
                    }
                    else if (::strcmp(mod, "row_mask")==0)
                    {
                        // parse row_mask with 4-bit value
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
                            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before row_mask")
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
                                // accept '0' and '1' as enabled (old and new syntax)
                                bool boundCtrl = false;
                                linePtr++;
                                good &= parseModEnable(asmr, linePtr, boundCtrl,
                                        "bound_ctrl modifier");
                                mods = (mods & ~VOP3_BOUNDCTRL) |
                                        (boundCtrl ? VOP3_BOUNDCTRL : 0);
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
                        {
                            // bound_ctrl is defined
                            if (haveBoundCtrl)
                                asmr.printWarning(modPlace, "BoundCtrl is already defined");
                            haveBoundCtrl = true;
                            extraMods->needDPP = true;
                        }
                    }
                    else if (mod[0]=='r' && mod[1]=='o' && mod[2]=='w' && mod[3]=='_' &&
                            (::strcmp(mod+4, "shl")==0 || ::strcmp(mod+4, "shr")==0 ||
                                ::strcmp(mod+4, "ror")==0))
                    {
                        // row_XXX (shl, shr, ror) modifier (shift is in 1-15)
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
                                    ASM_NOTGOOD_BY_ERROR(shiftPlace,
                                            "Illegal zero shift for row_XXX shift")
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
                            ASM_NOTGOOD_BY_ERROR(linePtr, (std::string(
                                        "Expected ':' before ")+mod).c_str())
                    }
                    else if (memcmp(mod, "wave_", 5)==0 &&
                        (::strcmp(mod+5, "shl")==0 || ::strcmp(mod+5, "shr")==0 ||
                            ::strcmp(mod+5, "rol")==0 || ::strcmp(mod+5, "ror")==0))
                    {
                        // wave_XXX (shl,shr,rol,ror)
                        bool modGood = true;
                        skipSpacesToEnd(linePtr, end);
                        if (linePtr!=end && *linePtr==':')
                        {
                            // accept only 1 at parameter
                            skipCharAndSpacesToEnd(linePtr, end);
                            if (linePtr!=end && *linePtr=='1')
                                ++linePtr;
                            else
                                ASM_NOTGOOD_BY_ERROR1(modGood = good, linePtr,
                                            "Value must be '1'")
                        }
                        if (mod[5]=='s')
                            extraMods->dppCtrl = 0x100 | ((mod[7]=='l') ? 0x30 : 0x38);
                        else if (mod[5]=='r')
                            extraMods->dppCtrl = 0x100 | ((mod[7]=='l') ? 0x34 : 0x3c);
                        if (modGood)
                        {
                            // dpp_ctrl is defined
                            if (haveDppCtrl)
                                asmr.printWarning(modPlace, "DppCtrl is already defined");
                            haveDppCtrl = true;
                        }
                    }
                    else if (::strcmp(mod, "row_mirror")==0 ||
                        ::strcmp(mod, "row_half_mirror")==0 ||
                        ::strcmp(mod, "row_hmirror")==0)
                    {
                        // row_mirror, row_half_mirror
                        extraMods->dppCtrl = (mod[4]=='h') ? 0x141 : 0x140;
                        if (haveDppCtrl)
                            asmr.printWarning(modPlace, "DppCtrl is already defined");
                        haveDppCtrl = true;
                    }
                    else if (::strncmp(mod, "row_bcast", 9)==0 && (
                        (mod[9]=='1' && mod[10]=='5' && mod[11]==0) ||
                        (mod[9]=='3' && mod[10]=='1' && mod[11]==0) || mod[9]==0))
                    {
                        // row_bcast15 and row_bast31 modifier
                        bool modGood = true;
                        if (mod[9] =='1') // if row_bcast15
                            extraMods->dppCtrl = 0x142;
                        else if (mod[9] =='3') // if row_bcast31
                            extraMods->dppCtrl = 0x143;
                        else
                        {
                            // get number
                            skipSpacesToEnd(linePtr, end);
                            if (linePtr!=end && *linePtr==':')
                            {
                                skipCharAndSpacesToEnd(linePtr, end);
                                const char* numPlace = linePtr;
                                cxbyte value = cstrtobyte(linePtr, end);
                                // parse row_bcast:15 or row_bcast:31
                                if (value == 31)
                                    extraMods->dppCtrl = 0x143;
                                else if (value == 15)
                                    extraMods->dppCtrl = 0x142;
                                else
                                    ASM_NOTGOOD_BY_ERROR1(modGood = good, numPlace,
                                            "Thread to broadcast must be 15 or 31")
                            }
                            else
                                ASM_NOTGOOD_BY_ERROR1(modGood = good, linePtr,
                                            "Expected ':' before row_bcast")
                        }
                        if (modGood)
                        {
                            if (haveDppCtrl)
                                asmr.printWarning(modPlace, "DppCtrl is already defined");
                            haveDppCtrl = true;
                        }
                    }
                    else if (::strcmp(mod, "sdwa")==0)
                    {
                        // SDWA - force SDWA encoding
                        bool sdwa = false;
                        good &= parseModEnable(asmr, linePtr, sdwa, "vop3 modifier");
                        haveSDWA = sdwa;
                    }
                    else if (::strcmp(mod, "dpp")==0)
                    {
                        // DPP - force DPP encoding
                        bool dpp = false;
                        good &= parseModEnable(asmr, linePtr, dpp, "vop3 modifier");
                        haveDPP = dpp;
                    }
                    else    /// unknown modifier
                        ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown VOP modifier")
                }
                else /// unknown modifier
                    ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown VOP modifier")
                
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
    
    // determine what encoding will be needed to encode instruction
    const bool vopSDWA = (haveDstSel || haveDstUnused || haveSrc0Sel || haveSrc1Sel ||
        opMods.sextMod!=0 || haveSDWA);
    const bool vopDPP = (haveDppCtrl || haveBoundCtrl || haveBankMask || haveRowMask ||
            haveDPP);
    const bool isGCN14 = (arch & ARCH_GCN_1_4) != 0;
    // mul/div modifier does not apply to vop3 if RXVEGA (this case will be checked later)
    const bool vop3 = (mods & ((isGCN14 ? 0 : 3)|VOP3_VOP3))!=0 ||
                ((opMods.opselMod&15)!=0);
    if (extraMods!=nullptr)
    {
        extraMods->needSDWA = vopSDWA;
        extraMods->needDPP = vopDPP;
    }
    if ((int(vop3)+vopSDWA+vopDPP)>1 ||
                // RXVEGA: mul/div modifier are accepted in VOP_SDWA but not for VOP_DPP
                (isGCN14 && (mods & 3)!=0 && vopDPP) ||
                ((mods&VOP3_CLAMP)!=0 && vopDPP))
        ASM_FAIL_BY_ERROR(modsPlace, "Mixing modifiers from different encodings is illegal")
    return good;
}

static const char* vintrpParamsTbl[] =
{ "p10", "p20", "p0" };

// parse interpolation (P0,P10,P20) parameter for VINTRP instructions
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
            ASM_FAIL_BY_ERROR(p0Place, "Unknown VINTRP parameter")
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
        ASM_NOTGOOD_BY_ERROR1(goodAttr = good, attrPlace, "Expected 'attr' keyword")
    char buf[5];
    if (goodAttr)
    {
        std::transform(linePtr, linePtr+4, buf, toLower); // to lowercase
        // parse attr:
        if (::strncmp(buf, "attr", 4)!=0)
        {
            // skip spaces (to next element)
            while (linePtr!=end && *linePtr!=' ') linePtr++;
            ASM_NOTGOOD_BY_ERROR1(goodAttr = good, attrPlace, "Expected 'attr' keyword")
        }
        else
            linePtr+=4;
    }
    
    cxbyte attrVal = 0;
    if (goodAttr)
    {
        // parse only attribute value if no error before
        const char* attrNumPlace = linePtr;
        try
        { attrVal = cstrtobyte(linePtr, end); }
        catch(const ParseException& ex)
        {
            asmr.printError(linePtr, ex.what());
            goodAttr = good = false;
        }
        if (attrVal >= 64)
            ASM_NOTGOOD_BY_ERROR1(goodAttr = good, attrNumPlace,
                        "Attribute number out of range (0-63)")
    }
    if (goodAttr)
    {
        // parse again if no error before
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!='.')
            ASM_NOTGOOD_BY_ERROR1(goodAttr = good, linePtr,
                            "Expected '.' after attribute number")
        else
            ++linePtr;
    }
    if (goodAttr)
    {
        // parse again if no error before
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end)
            ASM_NOTGOOD_BY_ERROR1(goodAttr = good, linePtr, "Expected attribute component")
    }
    char attrCmpName = 0;
    if (goodAttr)
    {
        // parse only attribute component if no error before
        attrCmpName = toLower(*linePtr);
        if (attrCmpName!='x' && attrCmpName!='y' && attrCmpName!='z' && attrCmpName!='w')
            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected attribute component")
        linePtr++;
    }
    
    attr = (attrVal<<2) | ((attrCmpName=='x') ? 0 : (attrCmpName=='y') ? 1 :
            (attrCmpName=='z') ? 2 : 3);
    return good;
}

/* special version of getNameArg for MUBUF format name.
 * this version accepts digits at first character of format name */
bool GCNAsmUtils::getMUBUFFmtNameArg(Assembler& asmr, size_t maxOutStrSize, char* outStr,
               const char*& linePtr, const char* objName)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        ASM_FAIL_BY_ERROR(linePtr, (std::string("Expected ")+objName).c_str())
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
        ASM_FAIL_BY_ERROR(linePtr, (std::string(objName)+" is too long").c_str())
    const size_t outStrSize = std::min(maxOutStrSize-1, size_t(linePtr-nameStr));
    std::copy(nameStr, nameStr+outStrSize, outStr);
    outStr[outStrSize] = 0; // null-char
    return true;
}

bool GCNAsmUtils::checkGCNEncodingSize(Assembler& asmr, const char* insnPtr,
                     GCNEncSize gcnEncSize, uint32_t wordsNum)
{
    if (gcnEncSize==GCNEncSize::BIT32 && wordsNum!=1)
        ASM_FAIL_BY_ERROR(insnPtr, "32-bit encoding specified when 64-bit encoding")
    if (gcnEncSize==GCNEncSize::BIT64 && wordsNum!=2)
        ASM_FAIL_BY_ERROR(insnPtr, "64-bit encoding specified when 32-bit encoding")
    return true;
}

bool GCNAsmUtils::checkGCNVOPEncoding(Assembler& asmr, const char* insnPtr,
                     GCNVOPEnc vopEnc, const VOPExtraModifiers* modifiers)
{
    if (vopEnc==GCNVOPEnc::DPP && !modifiers->needDPP)
        ASM_FAIL_BY_ERROR(insnPtr, "DPP encoding specified when DPP not present")
    if (vopEnc==GCNVOPEnc::SDWA && !modifiers->needSDWA)
        ASM_FAIL_BY_ERROR(insnPtr, "DPP encoding specified when DPP not present")
    return true;
}

bool GCNAsmUtils::checkGCNVOPExtraModifers(Assembler& asmr, GPUArchMask arch, bool needImm,
                 bool sextFlags, bool vop3, GCNVOPEnc gcnVOPEnc, const GCNOperand& src0Op,
                 VOPExtraModifiers& extraMods, const char* instrPlace)
{
    if (needImm)
        ASM_FAIL_BY_ERROR(instrPlace, "Literal with SDWA or DPP word is illegal")
    if ((arch & ARCH_GCN_1_4)==0 && !src0Op.range.isVGPR())
        ASM_FAIL_BY_ERROR(instrPlace, "SRC0 must be a vector register with "
                    "SDWA or DPP word")
    if ((arch & ARCH_GCN_1_4)!=0 && extraMods.needDPP && !src0Op.range.isVGPR())
        ASM_FAIL_BY_ERROR(instrPlace, "SRC0 must be a vector register with DPP word")
    if (vop3)
        // if VOP3 and (VOP_DPP or VOP_SDWA)
        ASM_FAIL_BY_ERROR(instrPlace, "Mixing VOP3 with SDWA or WORD is illegal")
    if (sextFlags && extraMods.needDPP)
        ASM_FAIL_BY_ERROR(instrPlace, "SEXT modifiers is unavailable for DPP word")
    if (!extraMods.needSDWA && !extraMods.needDPP)
    {
        if (gcnVOPEnc!=GCNVOPEnc::DPP)
            extraMods.needSDWA = true; // by default we choose SDWA word
        else
            extraMods.needDPP = true;
    }
    return true;
}

};
