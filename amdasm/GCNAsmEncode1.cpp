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
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/GCNDefs.h>
#include "GCNAsmInternals.h"

namespace CLRX
{

static const uint32_t constImmFloatLiterals[9] = 
{
    0x3f000000, 0xbf000000, 0x3f800000, 0xbf800000,
    0x40000000, 0xc0000000, 0x40800000, 0xc0800000, 0x3e22f983
};

// used while converting 32-bit SOPx encoding to 64-bit SOPx encoding
static void tryPromoteConstImmToLiteral(GCNOperand& src0Op, GPUArchMask arch)
{
    if (!src0Op.range.isRegVar() && src0Op.range.start>=128 && src0Op.range.start<=208)
    {
        // convert integer const immediates
        src0Op.value = src0Op.range.start<193? src0Op.range.start-128 :
                192-src0Op.range.start;
        src0Op.range.start = 255;
    }
    else if (!src0Op.range.isRegVar() &&
            ((src0Op.range.start>=240 && src0Op.range.start<248) ||
             ((arch&ARCH_GCN_1_2_4)!=0 && src0Op.range.start==248)))
    {
        // floating point immediates to literal
        src0Op.value = constImmFloatLiterals[src0Op.range.start-240];
        src0Op.range.start = 255;
    }
}

// check whether reg range can be equal (regvar and registers)
static inline bool regRangeCanEqual(const RegRange& r1, const RegRange& r2)
{
    return r1.regVar==r2.regVar && r1.start==r2.start;
}

bool GCNAsmUtils::parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    bool good = true;
    RegRange dstReg(0, 0);
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
    {
        // parse SDST (SGPR)
        gcnAsm->setCurrentRVU(0);
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_SDST, true,
                   INSTROP_SYMREGRANGE|INSTROP_WRITE);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
    }
    
    std::unique_ptr<AsmExpression> src0Expr, src1Expr;
    // parse SRC0 (can be SGPR or scalar source)
    GCNOperand src0Op{};
    gcnAsm->setCurrentRVU(1);
    good &= parseOperand(asmr, linePtr, src0Op, &src0Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
                         INSTROP_READ, GCNFIELD_SSRC0);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    GCNOperand src1Op{};
    // parse SRC1 (can be SGPR or scalar source)
    gcnAsm->setCurrentRVU(2);
    good &= parseOperand(asmr, linePtr, src1Op, &src1Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
             (src0Op.range.isVal(255) ? INSTROP_ONLYINLINECONSTS : 0)|INSTROP_READ,
             GCNFIELD_SSRC1);
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    if (gcnEncSize==GCNEncSize::BIT64)
    {
        // try to promote constant immediate to literal
        tryPromoteConstImmToLiteral(src0Op, arch);
        tryPromoteConstImmToLiteral(src1Op, arch);
    }
    // put data
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0x80000000U | (uint32_t(gcnInsn.code1)<<23) | src0Op.range.bstart() |
            (src1Op.range.bstart()<<8) | uint32_t(dstReg.bstart())<<16);
    if (src0Op.range.isVal(255) || src1Op.range.isVal(255))
    {
        // put literal value
        if (src0Expr==nullptr && src1Expr==nullptr)
            SLEV(words[1], src0Op.range.isVal(255) ? src0Op.value : src1Op.value);
        else    // zero if unresolved value
            SLEV(words[1], 0);
        wordsNum++;
    }
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
    // set expression targets to resolving later
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
    // update SGPR counting and VCC usage (regflags)
    if (dstReg && !dstReg.isRegVar())
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
    if (src0Op.range && !src0Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    if (src1Op.range && !src1Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
    return true;
}

bool GCNAsmUtils::parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    bool good = true;
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    RegRange dstReg(0, 0);
    
    if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
    {
        // parse SDST (SGPR)
        gcnAsm->setCurrentRVU(0);
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_SDST, true,
                       INSTROP_SYMREGRANGE|INSTROP_WRITE);
        if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
            if (!skipRequiredComma(asmr, linePtr))
                return false;
    }
    
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0Expr;
    if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
    {
        // parse SRC0 (can be SGPR or source scalar, constant or literal)
        gcnAsm->setCurrentRVU(1);
        good &= parseOperand(asmr, linePtr, src0Op, &src0Expr, arch,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
                         INSTROP_READ, GCNFIELD_SSRC0);
    }
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    if (gcnEncSize==GCNEncSize::BIT64)
        // try to promote constant immediate to literal
        tryPromoteConstImmToLiteral(src0Op, arch);
    cxuint wordsNum = 1;
    uint32_t words[2];
    // put instruction word
    SLEV(words[0], 0xbe800000U | (uint32_t(gcnInsn.code1)<<8) | src0Op.range.bstart() |
            uint32_t(dstReg.bstart())<<16);
    if (src0Op.range.start==255)
    {
        // put literal
        if (src0Expr==nullptr)
            SLEV(words[1], src0Op.value);
        else    // zero if unresolved value
            SLEV(words[1], 0);
        wordsNum++;
    }
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
    // set expression targets
    if (src0Expr!=nullptr)
        src0Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words + wordsNum));
    // prevent freeing expressions
    src0Expr.release();
    // update SGPR counting and VCC usage (regflags)
    if (dstReg && !dstReg.isRegVar())
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
    if (src0Op.range && !src0Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    return true;
}

// hwreg names sorted by names
static const std::pair<const char*, cxuint> hwregNamesMap[] =
{
    { "gpr_alloc", 5 },
    { "hw_id", 4 },
    { "ib_dbg0", 12 },
    { "ib_dbg1", 13 },
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

// update SGPR counting and VCC usage (regflags) for GCN 1.4 (VEGA)
static const std::pair<const char*, cxuint> hwregNamesGCN14Map[] =
{
    { "flush_ib", 14 },
    { "gpr_alloc", 5 },
    { "hw_id", 4 },
    { "ib_dbg0", 12 },
    { "ib_dbg1", 13 },
    { "ib_sts", 7 },
    { "inst_dw0", 10 },
    { "inst_dw1", 11 },
    { "lds_alloc", 6 },
    { "mode", 1 },
    { "pc_hi", 9 },
    { "pc_lo", 8 },
    { "sh_mem_bases", 15 },
    { "sq_shader_tba_hi", 17 },
    { "sq_shader_tba_lo", 16 },
    { "sq_shader_tma_hi", 19 },
    { "sq_shader_tma_lo", 18 },
    { "status", 2 },
    { "trapsts", 3 }
};

static const size_t hwregNamesGCN14MapSize = sizeof(hwregNamesGCN14Map) /
            sizeof(std::pair<const char*, uint16_t>);

bool GCNAsmUtils::parseSOPKEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    RegRange dstReg(0, 0);
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    
    gcnAsm->setCurrentRVU(0);
    bool doWrite = (gcnInsn.mode&GCN_MASK1) != GCN_DST_SRC &&
            ((gcnInsn.mode&GCN_MASK1) != GCN_IMM_REL);
    if ((gcnInsn.mode & GCN_IMM_DST) == 0)
    {
        // parse SDST (SGPR)
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_SDST, true,
                   INSTROP_SYMREGRANGE|
                   (doWrite ? INSTROP_WRITE : INSTROP_READ));
        if (!skipRequiredComma(asmr, linePtr))
            return false;
    }
    
    uint16_t imm16 = 0;
    std::unique_ptr<AsmExpression> imm16Expr;
    
    if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL)
    {
        // parse relative address
        uint64_t value = 0;
        if (!getJumpValueArg(asmr, value, imm16Expr, linePtr))
            return false;
        if (imm16Expr==nullptr)
        {
            // if resolved at this time
            int64_t offset = (int64_t(value)-int64_t(output.size())-4);
            if (offset & 3)
                ASM_NOTGOOD_BY_ERROR(linePtr, "Jump is not aligned to word!")
            offset >>= 2;
            if (offset > INT16_MAX || offset < INT16_MIN)
                ASM_NOTGOOD_BY_ERROR(linePtr, "Jump out of range")
            imm16 = offset;
            // add codeflow entry
            if (good)
            {
                asmr.sections[asmr.currentSection].addCodeFlowEntry({ 
                    size_t(asmr.currentOutPos), size_t(value),
                    (isGCN14 && gcnInsn.code1==21) ? AsmCodeFlowType::CALL :
                            AsmCodeFlowType::CJUMP });
            }
        }
    }
    else if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_SREG)
    {
        // parse hwreg: hwreg(HWREG, bitstart, bitsize)
        skipSpacesToEnd(linePtr, end);
        char name[20];
        const char* funcNamePlace = linePtr;
        if (!getNameArg(asmr, 20, name, linePtr, "function name", true))
            return false;
        toLowerString(name);
        skipSpacesToEnd(linePtr, end);
        // if not hwreg
        if (::strcmp(name, "hwreg")!=0 || linePtr==end || *linePtr!='(')
            ASM_FAIL_BY_ERROR(funcNamePlace, "Expected hwreg function")
        ++linePtr;
        skipSpacesToEnd(linePtr, end);
        cxuint hwregId = 0;
        if (linePtr == end || *linePtr!='@')
        {
            // parse hwreg by name
            const char* hwregNamePlace = linePtr;
            // choose hwReg names map
            const size_t regMapSize = isGCN14 ? hwregNamesGCN14MapSize : hwregNamesMapSize;
            const std::pair<const char*, cxuint>* regMap = isGCN14 ?
                        hwregNamesGCN14Map : hwregNamesMap;
            good &= getEnumeration(asmr, linePtr, "HWRegister",
                        regMapSize, regMap, hwregId, "hwreg_");
            if (good && (arch & ARCH_GCN_1_2_4) == 0 && hwregId == 13)
                // if ib_dgb1 in not GCN 1.2
                ASM_NOTGOOD_BY_ERROR(hwregNamePlace, "Unknown HWRegister")
        }
        else
        {
            // parametrization (if preceded by '@')
            linePtr++;
            good &= parseImm(asmr, linePtr, hwregId, nullptr, 6, WS_UNSIGNED);
        }
        
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        uint64_t arg2Value = 0;
        skipSpacesToEnd(linePtr, end);
        const char* funcArg2Place = linePtr;
        
        // second argument of hwreg
        if (getAbsoluteValueArg(asmr, arg2Value, linePtr, true))
        {
            if (arg2Value >= 32)
                asmr.printWarning(funcArg2Place, "Second argument out of range (0-31)");
        }
        else
            good = false;
        
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        uint64_t arg3Value = 0;
        skipSpacesToEnd(linePtr, end);
        const char* funcArg3Place = linePtr;
        
        // second argument of hwreg
        if (getAbsoluteValueArg(asmr, arg3Value, linePtr, true))
        {
            if (arg3Value >= 33 || arg3Value < 1)
                asmr.printWarning(funcArg3Place, "Third argument out of range (1-32)");
        }
        else
            good = false;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!=')')
            ASM_FAIL_BY_ERROR(linePtr, "Unterminated hwreg function")
        ++linePtr;
        imm16 = hwregId | (arg2Value<<6) | ((arg3Value-1)<<11);
    }
    else // otherwise we parse expression
        good &= parseImm(asmr, linePtr, imm16, &imm16Expr);
    
    uint32_t imm32 = 0;
    std::unique_ptr<AsmExpression> imm32Expr;
    if (gcnInsn.mode & GCN_IMM_DST)
    {
        // parse SDST as immediate or next source
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        if (gcnInsn.mode & GCN_SOPK_CONST)
            good &= parseImm(asmr, linePtr, imm32, &imm32Expr);
        else
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_SDST, true,
                   INSTROP_SYMREGRANGE|INSTROP_READ); // new field!
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    const cxuint wordsNum = (gcnInsn.mode & GCN_SOPK_CONST) ? 2 : 1;
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
    
    // put data (instruction words)
    uint32_t words[2];
    SLEV(words[0], 0xb0000000U | imm16 | (uint32_t(dstReg.bstart())<<16) |
                uint32_t(gcnInsn.code1)<<23);
    if (wordsNum==2)
        SLEV(words[1], imm32);
    
    // setting expresion targets (for immediates)
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
    // update SGPR counting and VCC usage (regflags)
    if (dstReg && !dstReg.isRegVar() && doWrite && (gcnInsn.mode & GCN_IMM_DST)==0)
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
    if (dstReg && !dstReg.isRegVar())
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    return true;
}

bool GCNAsmUtils::parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    bool good = true;
    std::unique_ptr<AsmExpression> src0Expr, src1Expr;
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    GCNOperand src0Op{};
    
    // parse SRC0 (can be SGPR, source scalar, literal or constant
    gcnAsm->setCurrentRVU(0);
    good &= parseOperand(asmr, linePtr, src0Op, &src0Expr, arch,
             (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
                     INSTROP_READ, GCNFIELD_SSRC0);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    GCNOperand src1Op{};
    if ((gcnInsn.mode & GCN_SRC1_IMM) == 0)
    {
        // parse SRC1 (can be SGPR, source scalar, literal or constant
        gcnAsm->setCurrentRVU(1);
        good &= parseOperand(asmr, linePtr, src1Op, &src1Expr, arch,
                 (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, INSTROP_SSOURCE|INSTROP_SREGS|
                 (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0)|INSTROP_READ,
                         GCNFIELD_SSRC1);
    }
    else // immediate
        good &= parseImm(asmr, linePtr, src1Op.range.start, &src1Expr, 8);
    
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    if (gcnEncSize==GCNEncSize::BIT64)
    {
        // try to promote constant immediate to literal
        tryPromoteConstImmToLiteral(src0Op, arch);
        tryPromoteConstImmToLiteral(src1Op, arch);
    }
    // put data
    cxuint wordsNum = 1;
    uint32_t words[2];
    SLEV(words[0], 0xbf000000U | (uint32_t(gcnInsn.code1)<<16) | src0Op.range.bstart() |
            (src1Op.range.bstart()<<8));
    if (src0Op.range.start==255 ||
        ((gcnInsn.mode & GCN_SRC1_IMM)==0 && src1Op.range.start==255))
    {
        // put literal
        if (src0Expr==nullptr && src1Expr==nullptr)
            SLEV(words[1], src0Op.range.isVal(255) ? src0Op.value : src1Op.value);
        else    // zero if unresolved value
            SLEV(words[1], 0);
        wordsNum++;
    }
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
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
    // update SGPR counting and VCC usage (regflags)
    if (src0Op.range && !src0Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    if (src1Op.range && !src1Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
    return true;
}

// message names sorted by name
static const std::pair<const char*, uint16_t> sendMessageNamesMap[] =
{
    { "gs", 2 },
    { "gs_done", 3 },
    { "interrupt", 1 },
    { "savewave", 4 },
    { "sysmsg", 15 },
    { "system", 15 }
};

static const size_t sendMessageNamesMapSize = sizeof(sendMessageNamesMap) /
            sizeof(std::pair<const char*, uint16_t>);

// message names sorted by name for GCN1.4 (VEGA)
static const std::pair<const char*, uint16_t> sendMessageNamesGCN14Map[] =
{
    { "early_prim_dealloc", 8 },
    { "get_doorbell", 10 },
    { "gs", 2 },
    { "gs_alloc_req", 9 },
    { "gs_done", 3 },
    { "halt_waves", 6 },
    { "interrupt", 1 },
    { "ordered_ps_done", 7 },
    { "savewave", 4 },
    { "stall_wave_gen", 5 },
    { "sysmsg", 15 },
    { "system", 15 }
};

static const size_t sendMessageNamesGCN14MapSize = sizeof(sendMessageNamesGCN14Map) /
            sizeof(std::pair<const char*, uint16_t>);

static const char* sendMsgGSOPTable[] =
{ "nop", "cut", "emit", "emit_cut" };

static const size_t sendMsgGSOPTableSize = sizeof(sendMsgGSOPTable) / sizeof(const char*);

bool GCNAsmUtils::parseSOPPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    bool good = true;
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    if (gcnEncSize==GCNEncSize::BIT64)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 32-bit size for SOPP encoding")
    
    uint16_t imm16 = 0;
    std::unique_ptr<AsmExpression> imm16Expr;
    switch (gcnInsn.mode&GCN_MASK1)
    {
        case GCN_IMM_REL:
        {
            // parse relative address
            uint64_t value = 0;
            if (!getJumpValueArg(asmr, value, imm16Expr, linePtr))
                return false;
            if (imm16Expr==nullptr)
            {
                int64_t offset = (int64_t(value)-int64_t(output.size())-4);
                if (offset & 3)
                    ASM_NOTGOOD_BY_ERROR(linePtr, "Jump is not aligned to word!")
                offset >>= 2;
                if (offset > INT16_MAX || offset < INT16_MIN)
                    ASM_NOTGOOD_BY_ERROR(linePtr, "Jump out of range")
                imm16 = offset;
                Assembler& asmr = gcnAsm->assembler;
                // add codeflow entry
                if (good)
                    asmr.sections[asmr.currentSection].addCodeFlowEntry({ 
                        size_t(asmr.currentOutPos), size_t(value),
                        gcnInsn.code1==2 ? AsmCodeFlowType::JUMP :
                            AsmCodeFlowType::CJUMP });
            }
            break;
        }
        case GCN_IMM_LOCKS:
        {
            /* parse locks for s_waitcnt */
            char name[20];
            bool haveLgkmCnt = false;
            bool haveExpCnt = false;
            bool haveVMCnt = false;
            imm16 = isGCN14 ? 0xcf7f : 0xf7f;
            while (true)
            {
                skipSpacesToEnd(linePtr, end);
                const char* funcNamePlace = linePtr;
                name[0] = 0;
                // get function name
                good &= getNameArgS(asmr, 20, name, linePtr, "function name", true);
                toLowerString(name);
                
                cxuint bitPos = 0, bitMask = UINT_MAX;
                bool goodCnt = true;
                bool doVMCnt = false;
                // select bitfield for lock
                if (::strcmp(name, "vmcnt")==0)
                {
                    if (haveVMCnt)
                        asmr.printWarning(funcNamePlace, "vmcnt was already defined");
                    bitPos = 0;
                    bitMask = isGCN14 ? 63 : 15;
                    doVMCnt = haveVMCnt = true;
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
                    ASM_NOTGOOD_BY_ERROR1(goodCnt = good, funcNamePlace,
                                    "Expected vmcnt, lgkmcnt or expcnt")
                
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end || *linePtr!='(')
                {
                    if (goodCnt) // only if cnt has been parsed (do not duplicate errors)
                        asmr.printError(funcNamePlace, "Expected vmcnt, lgkmcnt or expcnt");
                    return false;
                }
                skipCharAndSpacesToEnd(linePtr, end);
                const char* argPlace = linePtr;
                uint64_t value;
                // parse value for lock
                if (getAbsoluteValueArg(asmr, value, linePtr, true))
                {
                    if (value > bitMask)
                        asmr.printWarning(argPlace, "Value out of range");
                    if (!isGCN14 || !doVMCnt)
                        imm16 = (imm16 & ~(bitMask<<bitPos)) | ((value&bitMask)<<bitPos);
                    else // vmcnt for GFX9
                        imm16 = (imm16 & 0x3ff0) | ((value&15) | ((value&0x30)<<10));
                }
                else
                    good = false;
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end || *linePtr!=')')
                    ASM_FAIL_BY_ERROR(linePtr, "Unterminated function")
                // ampersand
                skipCharAndSpacesToEnd(linePtr, end);
                if (linePtr==end)
                    break;
                if (linePtr[0] == '&')
                    ++linePtr;
            }
            
            if (gcnInsn.code1==12)
            {
                // S_WAICTNT
                uint16_t lgkmCnt = (imm16>>8) & 15;
                if ((arch & ARCH_HD7X00) != 0)
                    lgkmCnt = std::min(uint16_t(7), lgkmCnt);
                const uint16_t expCnt = (imm16>>4) & 7;
                const uint16_t vmCnt = ((imm16) & 15) + (isGCN14 ? ((imm16>>10)&0x30) : 0);
                gcnAsm->waitInstr = { output.size(), { vmCnt, lgkmCnt, expCnt } };
                gcnAsm->hasWaitInstr = true;
            }
            break;
        }
        case GCN_IMM_MSGS:
        {
            char name[25];
            const char* funcNamePlace = linePtr;
            if (!getNameArg(asmr, 25, name, linePtr, "function name", true))
                return false;
            toLowerString(name);
            skipSpacesToEnd(linePtr, end);
            if (::strcmp(name, "sendmsg")!=0 || linePtr==end || *linePtr!='(')
                ASM_FAIL_BY_ERROR(funcNamePlace, "Expected sendmsg function")
            skipCharAndSpacesToEnd(linePtr, end);
            
            const char* funcArg1Place = linePtr;
            size_t sendMessage = 0;
            if (linePtr == end || *linePtr != '@')
            {
                // parse message name
                if (getNameArg(asmr, 25, name, linePtr, "message name", true))
                {
                    toLowerString(name);
                    const size_t msgNameIndex = (::strncmp(name, "msg_", 4) == 0) ? 4 : 0;
                    // choose message name table
                    auto msgMap = isGCN14 ? sendMessageNamesGCN14Map :
                            sendMessageNamesMap;
                    const size_t msgMapSize = isGCN14 ?
                            sendMessageNamesGCN14MapSize : sendMessageNamesMapSize;
                    // find message name
                    size_t index = binaryMapFind(msgMap, msgMap + msgMapSize,
                            name+msgNameIndex, CStringLess()) - msgMap;
                    if (index != msgMapSize &&
                        // save_wave only for GCN1.2
                        (msgMap[index].second!=4 || (arch&ARCH_GCN_1_2_4)!=0))
                        sendMessage = msgMap[index].second;
                    else
                        ASM_NOTGOOD_BY_ERROR(funcArg1Place, "Unknown message")
                }
                else
                    good = false;
            }
            else
            {
                // parametrization
                linePtr++;
                good &= parseImm(asmr, linePtr, sendMessage, nullptr, 4, WS_UNSIGNED);
            }
            
            cxuint gsopIndex = 0;
            cxuint streamId = 0;
            if (sendMessage == 2 || sendMessage == 3)
            {
                if (!skipRequiredComma(asmr, linePtr))
                    return false;
                skipSpacesToEnd(linePtr, end);
                
                // parse GSOP parameter
                if (linePtr == end || *linePtr != '@')
                {
                    const char* funcArg2Place = linePtr;
                    if (getNameArg(asmr, 20, name, linePtr, "GSOP", true))
                    {
                        toLowerString(name);
                        // handle gs_op prefix
                        const size_t gsopNameIndex = (::strncmp(name, "gs_op_", 6) == 0)
                                    ? 6 : 0;
                        for (gsopIndex = 0; gsopIndex < 4; gsopIndex++)
                            if (::strcmp(name+gsopNameIndex,
                                        sendMsgGSOPTable[gsopIndex])==0)
                                break;
                        if (gsopIndex==2 && gsopNameIndex==0)
                        {
                            /* 'emit-cut' handling */
                            if (linePtr+4<=end && ::strncasecmp(linePtr, "-cut", 4)==0 &&
                                (linePtr==end || (!isAlnum(*linePtr) && *linePtr!='_' &&
                                *linePtr!='$' && *linePtr!='.')))
                            {
                                linePtr+=4;
                                gsopIndex++;
                            }
                        }
                        if (gsopIndex == sendMsgGSOPTableSize)
                        {
                            // not found
                            gsopIndex = 0;
                            ASM_NOTGOOD_BY_ERROR(funcArg2Place, "Unknown GSOP")
                        }
                    }
                    else
                        good = false;
                }
                else
                {
                    // parametrization
                    linePtr++;
                    good &= parseImm(asmr, linePtr, gsopIndex, nullptr, 3, WS_UNSIGNED);
                }
                
                if (gsopIndex!=0)
                {
                    if (!skipRequiredComma(asmr, linePtr))
                        return false;
                    
                    uint64_t value;
                    skipSpacesToEnd(linePtr, end);
                    const char* func3ArgPlace = linePtr;
                    // parse STREAMID (third argument of sendmsg)
                    good &= getAbsoluteValueArg(asmr, value, linePtr, true);
                    if (value > 3)
                        asmr.printWarning(func3ArgPlace,
                                  "StreamId (3rd argument) out of range");
                    streamId = value&3;
                }
            }
            skipSpacesToEnd(linePtr, end);
            if (linePtr==end || *linePtr!=')')
                ASM_FAIL_BY_ERROR(linePtr, "Unterminated sendmsg function")
            ++linePtr;
            imm16 = sendMessage | (gsopIndex<<4) | (streamId<<8);
            
            gcnAsm->delayedOps[0] = { output.size(), nullptr, uint16_t(0), uint16_t(0),
                    1, GCNDELOP_SENDMSG, GCNDELOP_NONE, cxbyte(0) };
            break;
        }
        case GCN_IMM_NONE:
            // if s_endpgm or s_endpgm_saved then add 'end' to code flow entries
            if (gcnInsn.code1 == 1 || gcnInsn.code1 == 27)
                asmr.sections[asmr.currentSection].addCodeFlowEntry({ 
                    size_t(asmr.currentOutPos+4), size_t(0), AsmCodeFlowType::END });
            break;
        default:
            good &= parseImm(asmr, linePtr, imm16, &imm16Expr);
    }
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    // put data (instruction word)
    uint32_t word;
    SLEV(word, 0xbf800000U | imm16 | (uint32_t(gcnInsn.code1)<<16));
    
    if (imm16Expr!=nullptr)
        imm16Expr->setTarget(AsmExprTarget(((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL) ?
                GCNTGT_SOPJMP : GCNTGT_SOPKSIMM16, asmr.currentSection, output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word), 
            reinterpret_cast<cxbyte*>(&word)+4);
    /// prevent freeing expression
    imm16Expr.release();
    return true;
}

bool GCNAsmUtils::parseSMRDEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT64)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 32-bit size for SMRD encoding")
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    RegRange dstReg(0, 0);
    RegRange sbaseReg(0, 0);
    RegRange soffsetReg(0, 0);
    cxbyte soffsetVal = 0;
    std::unique_ptr<AsmExpression> soffsetExpr;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        // parse SDST (SGPR)
        gcnAsm->setCurrentRVU(0);
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_SMRD_SDST, true,
                       INSTROP_SYMREGRANGE|INSTROP_WRITE);
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        // parse SDST (SGPR's (1-16 registers))
        gcnAsm->setCurrentRVU(0);
        good &= parseSRegRange(asmr, linePtr, dstReg, arch, dregsNum,
                   GCNFIELD_SMRD_SDST, true, INSTROP_SYMREGRANGE|INSTROP_WRITE);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        
        // parse SBASE (2 or 4 registers)
        gcnAsm->setCurrentRVU(1);
        good &= parseSRegRange(asmr, linePtr, sbaseReg, arch,
                   (gcnInsn.mode&GCN_SBASE4)?4:2, GCNFIELD_SMRD_SBASE, true,
                   INSTROP_SYMREGRANGE|INSTROP_READ);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!='@')
        {
            // parse SOFFSET (SGPR)
            gcnAsm->setCurrentRVU(2);
            good &= parseSRegRange(asmr, linePtr, soffsetReg, arch, 1,
                       GCNFIELD_SMRD_SOFFSET, false, INSTROP_SYMREGRANGE|INSTROP_READ);
        }
        else // '@' prefix (treat next as expression)
            skipCharAndSpacesToEnd(linePtr, end);
        
        if (!soffsetReg)
        {
            // parse immediate
            soffsetReg.start = 255; // indicate an immediate
            good &= parseImm(asmr, linePtr, soffsetVal, &soffsetExpr, 0, WS_UNSIGNED);
        }
    }
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    if (mode1 != GCN_ARG_NONE)
        gcnAsm->delayedOps[0] = AsmDelayedOp { output.size(),
                    gcnAsm->instrRVUs[0].regVar, gcnAsm->instrRVUs[0].rstart,
                    gcnAsm->instrRVUs[0].rend,
                    cxbyte(gcnAsm->instrRVUs[0].rend - gcnAsm->instrRVUs[0].rstart),
                    GCNDELOP_SMEMOP, GCNDELOP_NONE, gcnAsm->instrRVUs[0].rwFlags };
    
    if (soffsetExpr!=nullptr)
        soffsetExpr->setTarget(AsmExprTarget(GCNTGT_SMRDOFFSET, asmr.currentSection,
                       output.size()));
    
    // put data (instruction word)
    uint32_t word;
    SLEV(word, 0xc0000000U | (uint32_t(gcnInsn.code1)<<22) |
            (uint32_t(dstReg.bstart())<<15) |
            ((sbaseReg.bstart()&~1U)<<8) | ((soffsetReg.isVal(255)) ? 0x100 : 0) |
            ((soffsetReg.isVal(255)) ? soffsetVal : soffsetReg.bstart()));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word), 
            reinterpret_cast<cxbyte*>(&word)+4);
    /// prevent freeing expression
    soffsetExpr.release();
    
    // update SGPR counting and VCC usage (regflags)
    if (dstReg && !dstReg.isRegVar())
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
    if (!sbaseReg.isRegVar())
        updateRegFlags(gcnRegs.regFlags, sbaseReg.start, arch);
    if (!soffsetReg.isRegVar())
        updateRegFlags(gcnRegs.regFlags, soffsetReg.start, arch);
    return true;
}

bool GCNAsmUtils::parseSMEMEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT32)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 64-bit size for SMEM encoding")
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    RegRange dataReg(0, 0);
    RegRange sbaseReg(0, 0);
    RegRange soffsetReg(0, 0);
    uint32_t soffsetVal = 0;
    std::unique_ptr<AsmExpression> soffsetExpr;
    std::unique_ptr<AsmExpression> simm7Expr;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    const bool isGCN14 = (arch & ARCH_GCN_1_4) != 0;
    
    const char* soffsetPlace = nullptr;
    AsmSourcePos soffsetPos;
    
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        // parse SDST (SGPR)
        gcnAsm->setCurrentRVU(0);
        good &= parseSRegRange(asmr, linePtr, dataReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_SMRD_SDST, true,
                       INSTROP_SYMREGRANGE|INSTROP_WRITE);
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        // parse SDST (SGPR's (1-16 registers))
        gcnAsm->setCurrentRVU(0);
        if ((mode1 & GCN_SMEM_SDATA_IMM)==0)
            good &= parseSRegRange(asmr, linePtr, dataReg, arch, dregsNum,
                    GCNFIELD_SMRD_SDST, true, INSTROP_SYMREGRANGE|
                    ((gcnInsn.mode & GCN_MLOAD) != 0 ? INSTROP_WRITE : INSTROP_READ));
        else
            good &= parseImm(asmr, linePtr, dataReg.start, &simm7Expr, 7);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        
        // parse SBASE (2 or 4 SGPR's)
        gcnAsm->setCurrentRVU(1);
        good &= parseSRegRange(asmr, linePtr, sbaseReg, arch,
                   (gcnInsn.mode&GCN_SBASE4)?4:2, GCNFIELD_SMRD_SBASE, true,
                   INSTROP_SYMREGRANGE|INSTROP_READ);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!='@')
        {
            // parse SOFFSET (SGPR)
            gcnAsm->setCurrentRVU(2);
            const char* soffsetPlace = linePtr;
            good &= parseSRegRange(asmr, linePtr, soffsetReg, arch, 1,
                       GCNFIELD_SMRD_SOFFSET, false, INSTROP_SYMREGRANGE|INSTROP_READ);
            if (good && !isGCN14 && (gcnInsn.mode & GCN_MLOAD) == 0 && soffsetReg &&
                    !soffsetReg.isVal(124))
                // if no M0 register
                ASM_NOTGOOD_BY_ERROR(soffsetPlace,
                        "Store/Atomic SMEM instructions accepts only M0 register")
        }
        else // '@' prefix (treat next as expression)
            skipCharAndSpacesToEnd(linePtr, end);
        
        if (!soffsetReg)
        {
            // parse immediate
            soffsetReg.start = 255; // indicate an immediate
            skipSpacesToEnd(linePtr, end);
            soffsetPlace = linePtr;
            soffsetPos = asmr.getSourcePos(soffsetPlace);
            good &= parseImm(asmr, linePtr, soffsetVal, &soffsetExpr,
                        isGCN14 ? 21 : 20, isGCN14 ? WS_BOTH : WS_UNSIGNED);
        }
    }
    bool haveGlc = false;
    bool haveNv = false;
    bool haveOffset = false;
    // parse modifiers
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
                good &= parseModEnable(asmr, linePtr, haveGlc, "glc modifier");
            else if (isGCN14 && ::strcmp(name, "nv")==0)
                good &= parseModEnable(asmr, linePtr, haveNv, "nv modifier");
            else if (isGCN14 && ::strcmp(name, "offset")==0)
            {
                // parse offset and it parameter: offset:XXX
                if (parseModImm(asmr, linePtr, soffsetVal, &soffsetExpr, "offset",
                        21, WS_BOTH))
                {
                    if (haveOffset)
                        asmr.printWarning(modPlace, "Offset is already defined");
                    haveOffset = true;
                    if (soffsetReg.isVal(255))
                        // illegal second offset
                        ASM_NOTGOOD_BY_ERROR(modPlace, "Illegal second offset");
                }
                else
                    good = false;
            }
            else
                ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown SMEM modifier")
        }
        else
            good = false;
    }
    /// if errors
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    // set expression target for offsets and immediates
    if (soffsetExpr!=nullptr)
        soffsetExpr->setTarget(AsmExprTarget(isGCN14 ?
                    GCNTGT_SMEMOFFSETVEGA : GCNTGT_SMEMOFFSET,
                    asmr.currentSection, output.size()));
    if (simm7Expr!=nullptr)
        simm7Expr->setTarget(AsmExprTarget(GCNTGT_SMEMIMM, asmr.currentSection,
                       output.size()));
    
    bool dataToRead = false;
    bool dataToWrite = false;
    if (dataReg)
    {
        dataToWrite = ((gcnInsn.mode&GCN_MLOAD) != 0 ||
                ((gcnInsn.mode&GCN_MATOMIC)!=0 && haveGlc));
        dataToRead = (gcnInsn.mode&GCN_MLOAD)==0 || (gcnInsn.mode&GCN_MATOMIC)!=0;
    }
    
    gcnAsm->instrRVUs[0].rwFlags = (dataToRead ? ASMRVU_READ : 0) |
            (dataToWrite ? ASMRVU_WRITE : 0);
    // check fcmpswap
    if ((gcnInsn.mode & GCN_MHALFWRITE) != 0 && dataToWrite &&
            gcnAsm->instrRVUs[0].regField != ASMFIELD_NONE)
    {
        // fix access
        AsmRegVarUsage& rvu = gcnAsm->instrRVUs[0];
        uint16_t size = rvu.rend-rvu.rstart;
        rvu.rend = rvu.rstart + (size>>1);
        AsmRegVarUsage& nextRvu = gcnAsm->instrRVUs[3];
        nextRvu = rvu;
        nextRvu.regField = GCNFIELD_SMRD_SDSTH;
        nextRvu.rstart += (size>>1);
        nextRvu.rend = rvu.rstart + size;
        nextRvu.rwFlags = ASMRVU_READ;
        nextRvu.align = 0;
    }
    
    if (mode1 != GCN_ARG_NONE)
    {
        gcnAsm->delayedOps[0] = AsmDelayedOp { output.size(),
                    gcnAsm->instrRVUs[0].regVar, gcnAsm->instrRVUs[0].rstart,
                    gcnAsm->instrRVUs[0].rend,
                    cxbyte(gcnAsm->instrRVUs[0].rend - gcnAsm->instrRVUs[0].rstart),
                    GCNDELOP_SMEMOP, GCNDELOP_NONE, gcnAsm->instrRVUs[0].rwFlags };
        if (gcnAsm->instrRVUs[3].regField != ASMFIELD_NONE)
            gcnAsm->delayedOps[1] = AsmDelayedOp { output.size(),
                    gcnAsm->instrRVUs[3].regVar, gcnAsm->instrRVUs[3].rstart,
                    gcnAsm->instrRVUs[3].rend,
                    cxbyte(gcnAsm->instrRVUs[0].rend - gcnAsm->instrRVUs[0].rstart),
                    GCNDELOP_SMEMOP, GCNDELOP_NONE, gcnAsm->instrRVUs[3].rwFlags };
    }
    
    // put data (2 instruction words)
    uint32_t words[2];
    SLEV(words[0], 0xc0000000U | (uint32_t(gcnInsn.code1)<<18) | (dataReg.bstart()<<6) |
            (sbaseReg.bstart()>>1) |
            // enable IMM if soffset is immediate or haveOffset with SGPR
            ((soffsetReg.isVal(255) || haveOffset) ? 0x20000 : 0) |
            (haveGlc ? 0x10000 : 0) | (haveNv ? 0x8000 : 0) | (haveOffset ? 0x4000 : 0));
    SLEV(words[1], (
            // store IMM OFFSET if offset: or IMM offset instead SGPR
            ((soffsetReg.isVal(255) || haveOffset) ? soffsetVal : soffsetReg.bstart())) |
            // store SGPR in SOFFSET if have offset and have SGPR offset
                ((haveOffset && !soffsetReg.isVal(255)) ? (soffsetReg.bstart()<<25) : 0));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words+2));
    /// prevent freeing expression
    soffsetExpr.release();
    simm7Expr.release();
    
    // update SGPR counting and VCC usage (regflags)
    if (!dataReg.isRegVar() && dataToWrite)
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dataReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dataReg.start, arch);
    }
    if (!sbaseReg.isRegVar())
        updateRegFlags(gcnRegs.regFlags, sbaseReg.start, arch);
    if (!soffsetReg.isRegVar())
        updateRegFlags(gcnRegs.regFlags, soffsetReg.start, arch);
    return true;
}

// choose between 64-bit immediate (FP64) and 32-bit immediate
static Flags correctOpType(uint32_t regsNum, Flags typeMask)
{
    return (regsNum==2 && (typeMask==INSTROP_FLOAT || typeMask==INSTROP_INT)) ?
        INSTROP_V64BIT : typeMask;
}

static void encodeVOPWords(uint32_t vop0Word, cxbyte modifiers,
        const VOPExtraModifiers& extraMods, const GCNOperand& src0Op,
        const GCNOperand& src1Op, uint32_t immValue, GCNInsnMode mode1,
        // dstMod - (16bits lower value, 16bit - use dstMod instead std encoding
        uint32_t inDstMod, cxuint& wordsNum, uint32_t* words)
{
    // VOP2 encoding
    cxuint src0out = src0Op.range.bstart();
    if (extraMods.needSDWA)
        src0out = 0xf9;
    else if (extraMods.needDPP)
        src0out = 0xfa;
    SLEV(words[0], vop0Word | uint32_t(src0out));
    if (extraMods.needSDWA)
    {
        const uint32_t dstMod = (inDstMod & 0x10000) ? (inDstMod&0xff00) :
                    ((uint32_t(extraMods.dstSel)<<8) |
                    (uint32_t(extraMods.dstUnused)<<11) |
                    ((modifiers & VOP3_CLAMP) ? 0x2000 : 0) |
                    (uint32_t(modifiers & 3) << 14));
        // if SDWA encoding
        SLEV(words[wordsNum++], dstMod | (src0Op.range.bstart()&0xff) |
                (uint32_t(extraMods.src0Sel)<<16) |
                ((src0Op.vopMods&VOPOP_SEXT) ? (1U<<19) : 0) |
                ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0) |
                (uint32_t(extraMods.src1Sel)<<24) |
                ((src1Op.vopMods&VOPOP_SEXT) ? (1U<<27) : 0) |
                ((src1Op.vopMods&VOPOP_NEG) ? (1U<<28) : 0) |
                ((src1Op.vopMods&VOPOP_ABS) ? (1U<<29) : 0) |
                (src0Op.range.isNonVGPR() ? (1U<<23) : 0) |
                (src1Op.range.isNonVGPR() ? (1U<<31) : 0));
    }
    else if (extraMods.needDPP)
        // DPP encoding
        SLEV(words[wordsNum++], (src0Op.range.bstart()&0xff) | (extraMods.dppCtrl<<8) |
                ((modifiers&VOP3_BOUNDCTRL) ? (1U<<19) : 0) |
                ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0) |
                ((src1Op.vopMods&VOPOP_NEG) ? (1U<<22) : 0) |
                ((src1Op.vopMods&VOPOP_ABS) ? (1U<<23) : 0) |
                (uint32_t(extraMods.bankMask)<<24) |
                (uint32_t(extraMods.rowMask)<<28));
    else if (src0Op.range.isVal(255)) // otherwise we check for immediate/literal value
        SLEV(words[wordsNum++], src0Op.value);
    else if (src1Op.range.isVal(255))
        // literal from SRC1
        SLEV(words[wordsNum++], src1Op.value);
    else if (mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM)
        SLEV(words[wordsNum++], immValue);
}

static void encodeVOP3Words(bool isGCN12, const GCNAsmInstruction& gcnInsn,
        cxbyte modifiers, VOPOpModifiers opMods, bool haveDstCC,
        const RegRange& dstReg, const RegRange& dstCCReg, const RegRange& srcCCReg,
        const GCNOperand& src0Op, const GCNOperand& src1Op,
        cxuint& wordsNum, uint32_t* words)
{
    // VOP3 encoding
    uint32_t code = (isGCN12) ?
            (uint32_t(gcnInsn.code2)<<16) | ((modifiers&VOP3_CLAMP) ? 0x8000 : 0) :
            (uint32_t(gcnInsn.code2)<<17) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0);
    if (haveDstCC) // if VOP3B
        SLEV(words[0], 0xd0000000U | code |
            (dstReg.bstart()&0xff) | (uint32_t(dstCCReg.bstart())<<8));
    else // if VOP3A
        SLEV(words[0], 0xd0000000U | code | (dstReg.bstart()&0xff) |
            ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0) |
            ((src1Op.vopMods & VOPOP_ABS) ? 0x200 : 0) |
            ((opMods.opselMod&15) << 11));
    // second dword
    SLEV(words[1], src0Op.range.bstart() | (uint32_t(src1Op.range.bstart())<<9) |
        (uint32_t(srcCCReg.bstart())<<18) | (uint32_t(modifiers & 3) << 27) |
        ((src0Op.vopMods & VOPOP_NEG) ? (1U<<29) : 0) |
        ((src1Op.vopMods & VOPOP_NEG) ? (1U<<30) : 0));
    wordsNum++;
}

bool GCNAsmUtils::parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    const GCNInsnMode mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_GCN_1_2_4)!=0;
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    RegRange dstReg(0, 0);
    RegRange dstCCReg(0, 0);
    RegRange srcCCReg(0, 0);
    gcnAsm->setCurrentRVU(0);
    if (mode1 == GCN_DS1_SGPR)
        // if SGPRS as destination
        good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_VOP_SDST, true,
                       INSTROP_SYMREGRANGE|INSTROP_SGPR_UNALIGNED|INSTROP_WRITE);
    else
    {
         // if VGPRS as destination
        bool v_mac = ::strncmp(gcnInsn.mnemonic, "v_mac_", 6)==0 ||
                // for V_FMAC_F32 (VEGA20)
                ::strncmp(gcnInsn.mnemonic, "v_fmac_", 7)==0;
        good &= parseVRegRange(asmr, linePtr, dstReg, (gcnInsn.mode&GCN_REG_DST_64)?2:1,
                        GCNFIELD_VOP_VDST, true, INSTROP_SYMREGRANGE|INSTROP_WRITE|
                              (v_mac?INSTROP_READ:0));
    }
    
    const bool haveDstCC = mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC;
    const bool haveSrcCC = mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC;
    if (haveDstCC) /* VOP3b */
    {
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        // parse SDST (in place VCC) (2 SGPR's)
        gcnAsm->setCurrentRVU(1);
        good &= parseSRegRange(asmr, linePtr, dstCCReg, arch, 2, GCNFIELD_VOP3_SDST1, true,
                               INSTROP_SYMREGRANGE|INSTROP_SGPR_UNALIGNED|INSTROP_WRITE);
    }
    
    GCNOperand src0Op{}, src1Op{};
    std::unique_ptr<AsmExpression> src0OpExpr, src1OpExpr;
    const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
            (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
    
    const Flags vopOpModFlags = ((haveDstCC && !isGCN12) ?
                    INSTROP_VOP3NEG : INSTROP_VOP3MODS);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
    // parse SRC0 (can be VGPR, SGPR, scalar source, LDS, literal or constant)
    gcnAsm->setCurrentRVU(2);
    good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch, regsNum,
            correctOpType(regsNum, literalConstsFlags) | vopOpModFlags |
            INSTROP_SGPR_UNALIGNED|INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|
            INSTROP_READ, GCNFIELD_VOP_SRC0);
    
    uint32_t immValue = 0;
    std::unique_ptr<AsmExpression> immExpr;
    if (mode1 == GCN_ARG1_IMM)
    {
        // for V_MADMK_FXxx instruction
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        good &= parseLiteralImm(asmr, linePtr, immValue, &immExpr, literalConstsFlags);
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    bool sgprRegInSrc1 = mode1 == GCN_DS1_SGPR || mode1 == GCN_SRC1_SGPR;
    skipSpacesToEnd(linePtr, end);
    regsNum = (gcnInsn.mode&GCN_REG_SRC1_64)?2:1;
    gcnAsm->setCurrentRVU(3);
    // parse SRC1 (can be VGPR, SGPR, scalar source, LDS, literal or constant)
    //  except when SGPR for SRC1 when instructions accepts SGPR in this place
    good &= parseOperand(asmr, linePtr, src1Op, &src1OpExpr, arch, regsNum,
            correctOpType(regsNum, literalConstsFlags) | vopOpModFlags |
            (!sgprRegInSrc1 ? INSTROP_VREGS : 0)|INSTROP_SSOURCE|INSTROP_SREGS|
            INSTROP_SGPR_UNALIGNED |
            (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0)|
            INSTROP_READ, (!sgprRegInSrc1) ? GCNFIELD_VOP_VSRC1 : GCNFIELD_VOP_SSRC1);
    
    if (mode1 == GCN_ARG2_IMM)
    {
        // for V_MADAK_Fxx instruction
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        good &= parseLiteralImm(asmr, linePtr, immValue, &immExpr, literalConstsFlags);
    }
    else if (haveSrcCC)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        gcnAsm->setCurrentRVU(4);
        // parse SSRC (VCC) (2 SGPR's)
        good &= parseSRegRange(asmr, linePtr, srcCCReg, arch, 2, GCNFIELD_VOP3_SSRC, true,
                       INSTROP_SYMREGRANGE|INSTROP_UNALIGNED|INSTROP_READ);
    }
    
    // modifiers
    cxbyte modifiers = 0;
    VOPExtraModifiers extraMods{};
    VOPOpModifiers opMods{};
    good &= parseVOPModifiers(asmr, linePtr, arch, modifiers, opMods, 3,
                    (isGCN12) ? &extraMods : nullptr,
                    ((!haveDstCC || isGCN12) ? PARSEVOP_WITHCLAMP : 0)|PARSEVOP_WITHSEXT|
                    ((isGCN14 && !haveDstCC) ? PARSEVOP_WITHOPSEL : 0));
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    // apply VOP modifiers (abs,neg,sext) to operands from modifiers
    if (src0Op)
        src0Op.vopMods |= ((opMods.absMod&1) ? VOPOP_ABS : 0) |
                ((opMods.negMod&1) ? VOPOP_NEG : 0) |
                ((opMods.sextMod&1) ? VOPOP_SEXT : 0);
    if (src1Op)
        src1Op.vopMods |= ((opMods.absMod&2) ? VOPOP_ABS : 0) |
                ((opMods.negMod&2) ? VOPOP_NEG : 0) |
                ((opMods.sextMod&2) ? VOPOP_SEXT : 0);
    
    extraMods.needSDWA |= ((src0Op.vopMods | src1Op.vopMods) & VOPOP_SEXT) != 0;
    // determine whether VOP3 encoding is needed
    bool vop3 = /* src1=sgprs and not (DS1_SGPR|src1_SGPR) */
        //((src1Op.range.start<256) ^ sgprRegInSrc1) ||
        ((!isGCN14 || !extraMods.needSDWA) &&
                (src1Op.range.isNonVGPR() ^ sgprRegInSrc1)) ||
        (!isGCN12 && (src0Op.vopMods!=0 || src1Op.vopMods!=0)) ||
        (modifiers&~(VOP3_BOUNDCTRL|(extraMods.needSDWA?VOP3_CLAMP:0)|
            /* exclude OMOD if RXVEGA and SDWA used */
            ((isGCN14 && extraMods.needSDWA) ? 3 : 0)))!=0 ||
        /* srcCC!=VCC or dstCC!=VCC */
        //(haveDstCC && dstCCReg.start!=106) || (haveSrcCC && srcCCReg.start!=106) ||
        (haveDstCC && !dstCCReg.isVal(106)) || (haveSrcCC && !srcCCReg.isVal(106)) ||
        ((opMods.opselMod & 15) != 0) || (gcnEncSize==GCNEncSize::BIT64);
    
    if ((src0Op.range.isVal(255) || src1Op.range.isVal(255)) &&
        (src0Op.range.isSGPR() || src0Op.range.isVal(124) ||
         src1Op.range.isSGPR() || src1Op.range.isVal(124)))
        ASM_FAIL_BY_ERROR(instrPlace, "Literal with SGPR or M0 is illegal")
    
    AsmRegVarUsage* rvus = gcnAsm->instrRVUs;
    if (vop3) // modify fields in reg usage
    {
        if (rvus[0].regField != ASMFIELD_NONE)
            rvus[0].regField = (rvus[0].regField==GCNFIELD_VOP_VDST) ? GCNFIELD_VOP3_VDST :
                            GCNFIELD_VOP3_SDST0;
        if (rvus[2].regField != ASMFIELD_NONE)
            rvus[2].regField = GCNFIELD_VOP3_SRC0;
        if (rvus[3].regField != ASMFIELD_NONE)
            rvus[3].regField = GCNFIELD_VOP3_SRC1;
    }
    else
    {
        // vop2
        if (rvus[1].regField != ASMFIELD_NONE)
            rvus[1].regField = GCNFIELD_VOP_VCC_SDST1;
        if (rvus[4].regField != ASMFIELD_NONE)
            rvus[4].regField = GCNFIELD_VOP_VCC_SSRC;
    }
    
    // count number SGPR operands readed by instruction
    cxuint sgprsReaded = 0;
    if (src0Op.range.isSGPR())
        sgprsReaded++;
    if (src1Op.range.isSGPR() && !regRangeCanEqual(src0Op.range, src1Op.range))
        sgprsReaded++;
    if (haveSrcCC)
        // check for third operand (SSRC)
        if (!regRangeCanEqual(src0Op.range, srcCCReg) &&
            !regRangeCanEqual(src1Op.range, srcCCReg))
            sgprsReaded++;
    
    if (sgprsReaded >= 2)
        /* include VCCs (???) */
        ASM_FAIL_BY_ERROR(instrPlace, "More than one SGPR to read in instruction")
    
    const bool needImm = (src0Op.range.start==255 || src1Op.range.start==255 ||
             mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM);
    
    bool sextFlags = ((src0Op.vopMods|src1Op.vopMods) & VOPOP_SEXT);
    if (isGCN12 && (extraMods.needSDWA || extraMods.needDPP || sextFlags ||
                gcnVOPEnc!=GCNVOPEnc::NORMAL))
    {
        /* if VOP_SDWA or VOP_DPP is required */
        if (!checkGCNVOPExtraModifers(asmr, arch, needImm, sextFlags, vop3,
                    gcnVOPEnc, src0Op, extraMods, instrPlace))
            return false;
        if (gcnAsm->instrRVUs[2].regField != ASMFIELD_NONE)
            gcnAsm->instrRVUs[2].regField = GCNFIELD_DPPSDWA_SRC0;
        
        if (extraMods.needSDWA && isGCN14)
        {
            // fix for extra type operand from SDWA
            if (rvus[2].regField != ASMFIELD_NONE && src0Op.range.isNonVGPR())
                rvus[2].regField = GCNFIELD_DPPSDWA_SSRC0;
            if (rvus[3].regField != ASMFIELD_NONE && src1Op.range.isNonVGPR())
                rvus[3].regField = GCNFIELD_VOP_SSRC1;
        }
    }
    else if (isGCN12 && ((src0Op.vopMods|src1Op.vopMods) & ~VOPOP_SEXT)!=0 && !sextFlags)
        // if all pass we check we promote VOP3 if only operand modifiers expect sext()
        vop3 = true;
    
    if (isGCN12 && vop3 && haveDstCC && ((src0Op.vopMods|src1Op.vopMods) & VOPOP_ABS) != 0)
        ASM_FAIL_BY_ERROR(instrPlace, "Abs modifier is illegal for VOP3B encoding")
    if (vop3 && needImm)
        ASM_FAIL_BY_ERROR(instrPlace, "Literal in VOP3 encoding is illegal")
    
    if (!checkGCNVOPEncoding(asmr, instrPlace, gcnVOPEnc, &extraMods))
        return false;
    
    // set target expressions if needed
    if (src0OpExpr!=nullptr)
        src0OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    if (src1OpExpr!=nullptr)
        src1OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    if (immExpr!=nullptr)
        immExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    // put data (instruction words)
    cxuint wordsNum = 1;
    uint32_t words[2];
    if (!vop3)
        // VOP2 encoding
        encodeVOPWords((uint32_t(gcnInsn.code1)<<25) |
                (uint32_t(src1Op.range.bstart()&0xff)<<9) |
                (uint32_t(dstReg.bstart()&0xff)<<17),
                modifiers, extraMods, src0Op, src1Op, immValue, mode1,
                0, wordsNum, words);
    else
        // VOP3 encoding
        encodeVOP3Words(isGCN12, gcnInsn, modifiers, opMods, haveDstCC,
                dstReg, dstCCReg, srcCCReg, src0Op, src1Op, wordsNum, words);
    
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    src1OpExpr.release();
    immExpr.release();
    // update register pool (VGPR and SGPR counting)
    if (!dstReg.isRegVar())
    {
        if (dstReg.start>=256)
            updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
        else // sgprs
        {
            updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
            updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
        }
    }
    // for SRC operands
    if (src0Op.range && !src0Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    if (src1Op.range && !src1Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
    if (dstCCReg && !dstCCReg.isRegVar())
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstCCReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstCCReg.start, arch);
    }
    if (srcCCReg && !srcCCReg.isRegVar())
        updateRegFlags(gcnRegs.regFlags, srcCCReg.start, arch);
    return true;
}

bool GCNAsmUtils::parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    const GCNInsnMode mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_GCN_1_2_4)!=0;
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    RegRange dstReg(0, 0);
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0OpExpr;
    cxbyte modifiers = 0;
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        gcnAsm->setCurrentRVU(0);
        if (mode1 == GCN_DST_SGPR)
            // if SGPRS as destination
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                           (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_VOP_SDST, true,
                           INSTROP_SYMREGRANGE|INSTROP_SGPR_UNALIGNED|INSTROP_WRITE);
        else // if VGPRS as destination
            good &= parseVRegRange(asmr, linePtr, dstReg,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_VOP_VDST, true,
                                  INSTROP_SYMREGRANGE|INSTROP_WRITE);
        
        const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
        
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        gcnAsm->setCurrentRVU(1);
        // parse SRC0 (can be VGPR, SGPR, source scalar, literal or cosntant, LDS
        good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_SGPR_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|
                    INSTROP_VOP3MODS|INSTROP_READ, GCNFIELD_VOP_SRC0);
    }
    // modifiers
    VOPExtraModifiers extraMods{};
    VOPOpModifiers opMods{};
    good &= parseVOPModifiers(asmr, linePtr, arch, modifiers, opMods,
                  (mode1!=GCN_VOP_ARG_NONE) ? 2 : 0, (isGCN12)?&extraMods:nullptr,
                  PARSEVOP_WITHCLAMP|PARSEVOP_WITHSEXT|
                  (isGCN14 ? PARSEVOP_WITHOPSEL : 0), (mode1!=GCN_VOP_ARG_NONE) ? 2 : 0);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    if (src0Op)
        src0Op.vopMods |= ((opMods.absMod&1) ? VOPOP_ABS : 0) |
                ((opMods.negMod&1) ? VOPOP_NEG : 0) |
                ((opMods.sextMod&1) ? VOPOP_SEXT : 0);
    
    extraMods.needSDWA |= ((src0Op.vopMods) & VOPOP_SEXT) != 0;
    // check whether VOP3 encoding is needed
    bool vop3 = ((!isGCN12 && src0Op.vopMods!=0) ||
            (modifiers&~(VOP3_BOUNDCTRL|(extraMods.needSDWA?VOP3_CLAMP:0)|
            /* exclude OMOD if RXVEGA and SDWA used */
            ((isGCN14 && extraMods.needSDWA) ? 3 : 0)))!=0) ||
            ((opMods.opselMod & 15) != 0) || (gcnEncSize==GCNEncSize::BIT64);
    if (vop3) // modify fields in reg usage
    {
        AsmRegVarUsage* rvus = gcnAsm->instrRVUs;
        if (rvus[0].regField != ASMFIELD_NONE)
            rvus[0].regField = (rvus[0].regField==GCNFIELD_VOP_VDST) ?
                        GCNFIELD_VOP3_VDST : GCNFIELD_VOP3_SDST0;
        if (rvus[1].regField != ASMFIELD_NONE)
            rvus[1].regField = GCNFIELD_VOP3_SRC0;
    }
    
    bool sextFlags = (src0Op.vopMods & VOPOP_SEXT);
    bool needImm = (src0Op && src0Op.range.isVal(255));
    if (isGCN12 && (extraMods.needSDWA || extraMods.needDPP || sextFlags ||
                gcnVOPEnc!=GCNVOPEnc::NORMAL))
    {
        /* if VOP_SDWA or VOP_DPP is required */
        if (!checkGCNVOPExtraModifers(asmr, arch, needImm, sextFlags, vop3,
                    gcnVOPEnc, src0Op, extraMods, instrPlace))
            return false;
        if (gcnAsm->instrRVUs[1].regField != ASMFIELD_NONE)
            gcnAsm->instrRVUs[1].regField = GCNFIELD_DPPSDWA_SRC0;
        if (extraMods.needSDWA && isGCN14)
        {
            // fix for extra type operand from SDWA
            AsmRegVarUsage* rvus = gcnAsm->instrRVUs;
            if (rvus[1].regField != ASMFIELD_NONE && src0Op.range.isNonVGPR())
                rvus[1].regField = GCNFIELD_DPPSDWA_SSRC0;
        }
    }
    else if (isGCN12 && (src0Op.vopMods & ~VOPOP_SEXT)!=0 && !sextFlags)
        // if all pass we check we promote VOP3 if only operand modifiers expect sext()
        vop3 = true;
    
    if (vop3 && src0Op.range.isVal(255))
        ASM_FAIL_BY_ERROR(instrPlace, "Literal in VOP3 encoding is illegal")
    
    if (!checkGCNVOPEncoding(asmr, instrPlace, gcnVOPEnc, &extraMods))
        return false;
    
    if (src0OpExpr!=nullptr)
        src0OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    // put data (instruction word)
    cxuint wordsNum = 1;
    uint32_t words[2];
    if (!vop3)
        // VOP1 encoding
        encodeVOPWords(0x7e000000U | (uint32_t(gcnInsn.code1)<<9) |
                (uint32_t(dstReg.bstart()&0xff)<<17),
                modifiers, extraMods, src0Op, GCNOperand{ { 256, 257 } }, 0, mode1,
                0, wordsNum, words);
    else
        // VOP3 encoding
        encodeVOP3Words(isGCN12, gcnInsn, modifiers, opMods, false,
                dstReg, RegRange{}, RegRange{}, src0Op, GCNOperand{}, wordsNum, words);
    
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    // update register pool (VGPR and SGPR counting)
    if (dstReg && !dstReg.isRegVar())
    {
        if (dstReg.start>=256)
            updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
        else // sgprs
        {
            updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
            updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
        }
    }
    if (src0Op.range && !src0Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    return true;
}

bool GCNAsmUtils::parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    const GCNInsnMode mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_GCN_1_2_4)!=0;
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    RegRange dstReg(0, 0);
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0OpExpr;
    GCNOperand src1Op{};
    std::unique_ptr<AsmExpression> src1OpExpr;
    cxbyte modifiers = 0;
    
    // parse SDST (2 SGPR's)
    gcnAsm->setCurrentRVU(0);
    good &= parseSRegRange(asmr, linePtr, dstReg, arch, 2, GCNFIELD_VOP3_SDST0, true,
                           INSTROP_SYMREGRANGE|INSTROP_SGPR_UNALIGNED|INSTROP_WRITE);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
    cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
    gcnAsm->setCurrentRVU(1);
    // parse SRC0 (can VGPR, SGPR, scalar source,literal or constant)
    good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_SGPR_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|
                    INSTROP_VOP3MODS|INSTROP_READ, GCNFIELD_VOP_SRC0);
    
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    regsNum = (gcnInsn.mode&GCN_REG_SRC1_64)?2:1;
    gcnAsm->setCurrentRVU(2);
    // parse SRC1 (can VGPR, SGPR, scalar source,literal or constant)
    good &= parseOperand(asmr, linePtr, src1Op, &src1OpExpr, arch, regsNum,
                correctOpType(regsNum, literalConstsFlags) | INSTROP_VOP3MODS|
                INSTROP_SGPR_UNALIGNED|INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|
                INSTROP_READ| (src0Op.range.isVal(255) ? INSTROP_ONLYINLINECONSTS : 0),
                GCNFIELD_VOP_VSRC1);
    // modifiers
    VOPExtraModifiers extraMods{};
    VOPOpModifiers opMods{};
    good &= parseVOPModifiers(asmr, linePtr, arch, modifiers, opMods, 3,
                (isGCN12)?&extraMods:nullptr, (isGCN14 ? PARSEVOP_NODSTMODS : 0)|
                PARSEVOP_WITHCLAMP|PARSEVOP_WITHSEXT|(isGCN14 ? PARSEVOP_WITHOPSEL : 0));
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    // set VOP operand modifiers to src operands
    if (src0Op)
        src0Op.vopMods |= ((opMods.absMod&1) ? VOPOP_ABS : 0) |
                ((opMods.negMod&1) ? VOPOP_NEG : 0) |
                ((opMods.sextMod&1) ? VOPOP_SEXT : 0);
    if (src1Op)
        src1Op.vopMods |= ((opMods.absMod&2) ? VOPOP_ABS : 0) |
                ((opMods.negMod&2) ? VOPOP_NEG : 0) |
                ((opMods.sextMod&2) ? VOPOP_SEXT : 0);
    
    // determine whether SDWA is needed or VOP3 encoding needed
    extraMods.needSDWA |= ((src0Op.vopMods | src1Op.vopMods) & VOPOP_SEXT) != 0;
    bool vop3 = //(dstReg.start!=106) || (src1Op.range.start<256) ||
        ((!isGCN14 || !extraMods.needSDWA) && !dstReg.isVal(106)) ||
        ((!isGCN14 || !extraMods.needSDWA) && src1Op.range.isNonVGPR()) ||
        (!isGCN12 && (src0Op.vopMods!=0 || src1Op.vopMods!=0)) ||
        (modifiers&~(VOP3_BOUNDCTRL|(extraMods.needSDWA?VOP3_CLAMP:0)|
            /* exclude OMOD if RXVEGA and SDWA used */
            ((isGCN14 && extraMods.needSDWA) ? 3 : 0)))!=0 ||
        ((opMods.opselMod & 15) != 0) || (gcnEncSize==GCNEncSize::BIT64);
    
    if ((src0Op.range.isVal(255) || src1Op.range.isVal(255)) &&
        (src0Op.range.isSGPR() || src0Op.range.isVal(124) ||
         src1Op.range.isSGPR() || src1Op.range.isVal(124)))
        ASM_FAIL_BY_ERROR(instrPlace, "Literal with SGPR or M0 is illegal")
    if (src0Op.range.isSGPR() && src1Op.range.isSGPR() &&
        !regRangeCanEqual(src0Op.range, src1Op.range))
        /* include VCCs (???) */
        ASM_FAIL_BY_ERROR(instrPlace, "More than one SGPR to read in instruction")
    
    AsmRegVarUsage* rvus = gcnAsm->instrRVUs;
    if (vop3)
    {
        // modify fields in reg usage
        if (rvus[1].regField != ASMFIELD_NONE)
            rvus[1].regField = GCNFIELD_VOP3_SRC0;
        if (rvus[2].regField != ASMFIELD_NONE)
            rvus[2].regField = GCNFIELD_VOP3_SRC1;
    }
    else if (rvus[0].regField != ASMFIELD_NONE)
        rvus[0].regField = GCNFIELD_VOP_VCC_SDST0;
    
    const bool needImm = src0Op.range.start==255 || src1Op.range.start==255;
    
    bool sextFlags = ((src0Op.vopMods|src1Op.vopMods) & VOPOP_SEXT);
    if (isGCN12 && (extraMods.needSDWA || extraMods.needDPP || sextFlags ||
                gcnVOPEnc!=GCNVOPEnc::NORMAL))
    {
        /* if VOP_SDWA or VOP_DPP is required */
        if (!checkGCNVOPExtraModifers(asmr, arch, needImm, sextFlags, vop3,
                    gcnVOPEnc, src0Op, extraMods, instrPlace))
            return false;
        if (gcnAsm->instrRVUs[1].regField != ASMFIELD_NONE)
            gcnAsm->instrRVUs[1].regField = GCNFIELD_DPPSDWA_SRC0;
        
        if (extraMods.needSDWA && isGCN14)
        {
            // fix for extra type operand from SDWA
            AsmRegVarUsage* rvus = gcnAsm->instrRVUs;
            if (rvus[0].regField != ASMFIELD_NONE)
                rvus[0].regField = GCNFIELD_SDWAB_SDST;
            if (rvus[1].regField != ASMFIELD_NONE && src0Op.range.isNonVGPR())
                rvus[1].regField = GCNFIELD_DPPSDWA_SSRC0;
            if (rvus[2].regField != ASMFIELD_NONE && src1Op.range.isNonVGPR())
                rvus[2].regField = GCNFIELD_VOP_SSRC1;
        }
    }
    else if (isGCN12 && ((src0Op.vopMods|src1Op.vopMods) & ~VOPOP_SEXT)!=0 && !sextFlags)
        // if all pass we check we promote VOP3 if only operand modifiers expect sext()
        vop3 = true;
    
    if (vop3 && (src0Op.range.isVal(255) || src1Op.range.isVal(255)))
        ASM_FAIL_BY_ERROR(instrPlace, "Literal in VOP3 encoding is illegal")
    
    if (!checkGCNVOPEncoding(asmr, instrPlace, gcnVOPEnc, &extraMods))
        return false;
    
    if (isGCN14 && extraMods.needSDWA && ((modifiers & VOP3_CLAMP)!=0 || (modifiers&3)!=0))
        ASM_FAIL_BY_ERROR(instrPlace, "Modifiers CLAMP and OMOD is illegal in SDWAB")
    
    if (src0OpExpr!=nullptr)
        src0OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    if (src1OpExpr!=nullptr)
        src1OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    // put data (instruction words)
    cxuint wordsNum = 1;
    uint32_t words[2];
    if (!vop3)
    {
        // VOPC encoding
        const uint32_t dstMods = (isGCN14 ? 0x10000 : 0) |
                ((isGCN14 && !dstReg.isVal(106)) ? ((dstReg.bstart()|0x80)<<8) : 0);
        
        encodeVOPWords(0x7c000000U | (uint32_t(gcnInsn.code1)<<17) |
                (uint32_t(src1Op.range.bstart()&0xff)<<9),
                modifiers, extraMods, src0Op, src1Op, 0, 0,
                dstMods, wordsNum, words);
    }
    else
        // VOP3 encoding
        encodeVOP3Words(isGCN12, gcnInsn, modifiers, opMods, false,
                dstReg, RegRange{}, RegRange{}, src0Op, src1Op, wordsNum, words);
    
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    src1OpExpr.release();
    // update register pool (VGPR and SGPR counting)
    if (dstReg && !dstReg.isRegVar())
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
    if (src0Op.range && !src0Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    if (src1Op.range && !src1Op.range.isRegVar())
        updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
    return true;
}

bool GCNAsmUtils::parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    const GCNInsnMode mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_GCN_1_2_4)!=0;
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    const bool vop3p = (gcnInsn.mode & GCN_VOP3_VOP3P) != 0;
    if (gcnVOPEnc!=GCNVOPEnc::NORMAL)
        ASM_FAIL_BY_ERROR(instrPlace, "DPP and SDWA encoding is illegal for VOP3")
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    RegRange dstReg(0, 0);
    RegRange sdstReg(0, 0);
    GCNOperand src0Op{};
    GCNOperand src1Op{};
    GCNOperand src2Op{};
    
    const bool is128Ops = (gcnInsn.mode & 0x7000) == GCN_VOP3_DS2_128;
    bool modHigh = false;
    cxbyte modifiers = 0;
    const Flags vop3Mods = ((gcnInsn.encoding == GCNENC_VOP3B) ?
            INSTROP_VOP3NEG : INSTROP_VOP3MODS | INSTROP_NOSEXT) |
            (vop3p ? INSTROP_VOP3P : 0);
    
    // by default OPSEL_HI is [1,1,1] in vop3p instructions
    VOPOpModifiers opMods{ 0, 0, 0, cxbyte(vop3p ? 7<<4 : 0) };
    cxuint operands = 1;
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        gcnAsm->setCurrentRVU(0);
        // parse DST (
        if ((gcnInsn.mode&GCN_VOP3_DST_SGPR)==0)
            good &= parseVRegRange(asmr, linePtr, dstReg,
                       (is128Ops) ? 4 : ((gcnInsn.mode&GCN_REG_DST_64)?2:1),
                       GCNFIELD_VOP3_VDST, true, INSTROP_SYMREGRANGE|INSTROP_WRITE);
        else // SGPRS as dest
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, GCNFIELD_VOP3_SDST0, true,
                       INSTROP_SYMREGRANGE|INSTROP_SGPR_UNALIGNED|INSTROP_WRITE);
        if (!skipRequiredComma(asmr, linePtr))
            return false;
        
        if (gcnInsn.encoding == GCNENC_VOP3B &&
            (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC || mode1 == GCN_DST_VCC_VSRC2 ||
             mode1 == GCN_S0EQS12)) /* VOP3b */
        {
            // SDST (VCC) (2 SGPR's)
            gcnAsm->setCurrentRVU(1);
            good &= parseSRegRange(asmr, linePtr, sdstReg, arch, 2, GCNFIELD_VOP3_SDST1,
                       true, INSTROP_SYMREGRANGE|INSTROP_WRITE|INSTROP_SGPR_UNALIGNED);
            if (!skipRequiredComma(asmr, linePtr))
                return false;
        }
        const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
        
        cxuint regsNum;
        if (mode2 != GCN_VOP3_VINTRP)
        {
            // parse SRC0 (can be VGPR, SGPR, scalar source, constant, LDS)
            gcnAsm->setCurrentRVU(2);
            regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
            good &= parseOperand(asmr, linePtr, src0Op, nullptr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_SGPR_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|
                    vop3Mods|INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR|INSTROP_READ,
                    GCNFIELD_VOP3_SRC0);
            operands++;
        }
        
        if (mode2 == GCN_VOP3_VINTRP)
        {
            // if VINTRP instruction
            gcnAsm->setCurrentRVU(3);
            if (mode1 != GCN_P0_P10_P20)
            {
                good &= parseOperand(asmr, linePtr, src1Op, nullptr, arch, 1,
                        INSTROP_VREGS|vop3Mods|INSTROP_READ, GCNFIELD_VOP3_SRC1);
            }
            else /* P0_P10_P20 */
                good &= parseVINTRP0P10P20(asmr, linePtr, src1Op.range);
            
            if (!skipRequiredComma(asmr, linePtr))
                return false;
            
            cxbyte attr;
            good &= parseVINTRPAttr(asmr, linePtr, attr);
            attr = ((attr&3)<<6) | ((attr&0xfc)>>2);
            src0Op.range = { attr, uint16_t(attr+1) };
            
            if ((gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2)
            {
                if (!skipRequiredComma(asmr, linePtr))
                    return false;
                // parse SRC2 (VGPR, SGPR)
                gcnAsm->setCurrentRVU(4);
                good &= parseOperand(asmr, linePtr, src2Op, nullptr, arch,
                    (gcnInsn.mode&GCN_REG_SRC2_64)?2:1, vop3Mods|INSTROP_SGPR_UNALIGNED|
                    INSTROP_VREGS|INSTROP_SREGS|INSTROP_READ, GCNFIELD_VOP3_SRC2);
            }
            // high and vop3
            const char* end = asmr.line+asmr.lineSize;
            bool haveOpsel = false;
            bool haveNeg = false, haveAbs = false;
            // own parse VINTRP modifiers with some VOP3 modifiers
            while (true)
            {
                bool alreadyModDefined = false;
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end)
                    break;
                char modName[10];
                const char* modPlace = linePtr;
                if (!getNameArgS(asmr, 10, modName, linePtr, "VINTRP modifier"))
                    continue;
                if (::strcmp(modName, "high")==0)
                    good &= parseModEnable(asmr, linePtr, modHigh, "high modifier");
                else if (::strcmp(modName, "vop3")==0)
                {
                    bool vop3Mod = false;
                    good &= parseModEnable(asmr, linePtr, vop3Mod, "vop3 modifier");
                    modifiers = (modifiers & ~VOP3_VOP3) | (vop3Mod ? VOP3_VOP3 : 0);
                }
                else if (parseSingleOMODCLAMP(asmr, linePtr, modPlace, modName, arch,
                        modifiers, opMods, 
                        (gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2 ? 4 : 3, PARSEVOP_WITHCLAMP, haveAbs, haveNeg,
                        alreadyModDefined, good))
                {   // do nothing
                }
                else if (::strcmp(modName, "op_sel")==0)
                {
                    // op_sel with boolean array
                    uint32_t opselVal = 0;
                    if (linePtr!=end && *linePtr==':')
                    {
                        linePtr++;
                        if (parseImmWithBoolArray(asmr, linePtr, opselVal, 4, WS_UNSIGNED))
                        {
                            opMods.opselMod = opselVal;
                            if (haveOpsel)
                                asmr.printWarning(modPlace, "Opsel is already defined");
                            haveOpsel = true;
                            opMods.opselMod = opselVal;
                        }
                    }
                    else
                        good = false;
                }
                else
                    ASM_NOTGOOD_BY_ERROR(modPlace, "Unknown VINTRP modifier")
            }
            if (modHigh)
            {
                src0Op.range.start+=0x100;
                src0Op.range.end+=0x100;
            }
        }
        else if (mode1 != GCN_SRC12_NONE)
        {
            // if encoding have two or three source operands
            if (!skipRequiredComma(asmr, linePtr))
                return false;
            regsNum = (gcnInsn.mode&GCN_REG_SRC1_64)?2:1;
            // parse SRC1 (can be VGPR, SGPR, source scalar, constant)
            gcnAsm->setCurrentRVU(3);
            good &= parseOperand(asmr, linePtr, src1Op, nullptr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_SGPR_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|vop3Mods|
                    INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR|INSTROP_READ,
                    GCNFIELD_VOP3_SRC1);
            operands++;
            
            if (mode1 != GCN_SRC2_NONE && mode1 != GCN_DST_VCC)
            {
                if (!skipRequiredComma(asmr, linePtr))
                    return false;
                regsNum = (gcnInsn.mode&GCN_REG_SRC2_64)?2:1;
                // parse SRC2 (can be VGPR, SGPR, source scalar, constant)
                gcnAsm->setCurrentRVU(4);
                good &= parseOperand(asmr, linePtr, src2Op, nullptr, arch,
                        is128Ops ? 4 : regsNum,
                        correctOpType(regsNum, literalConstsFlags)|INSTROP_SGPR_UNALIGNED|
                        INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_READ|
                        vop3Mods|INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR,
                        GCNFIELD_VOP3_SRC2);
                operands++;
            }
        }
    }
    // modifiers
    if (mode2 != GCN_VOP3_VINTRP)
        good &= parseVOPModifiers(asmr, linePtr, arch, modifiers, opMods, operands,
                    nullptr, ((isGCN12 || gcnInsn.encoding!=GCNENC_VOP3B) ?
                            PARSEVOP_WITHCLAMP : 0) |
                    ((isGCN14 && gcnInsn.encoding!=GCNENC_VOP3B) ?
                            PARSEVOP_WITHOPSEL : 0) | (vop3p ? PARSEVOP_VOP3P : 0), 3);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    // apply VOP modifiers (abs,neg,sext) to operands from modifiers
    if (src0Op)
        src0Op.vopMods |= ((opMods.absMod&1) ? VOPOP_ABS : 0) |
                ((opMods.negMod&1) ? VOPOP_NEG : 0) |
                ((opMods.sextMod&1) ? VOPOP_SEXT : 0);
    if (src1Op)
        src1Op.vopMods |= ((opMods.absMod&2) ? VOPOP_ABS : 0) |
                ((opMods.negMod&2) ? VOPOP_NEG : 0) |
                ((opMods.sextMod&2) ? VOPOP_SEXT : 0);
    if (src2Op)
        src2Op.vopMods |= ((opMods.absMod&4) ? VOPOP_ABS : 0) |
                ((opMods.negMod&4) ? VOPOP_NEG : 0);
    
    if (mode2 != GCN_VOP3_VINTRP)
    {
        // count SGPR operands readed by instruction
        cxuint numSgprToRead = 0;
        //if (src0Op.range.start<108)
        if (src0Op.range.isSGPR())
            numSgprToRead++;
        //if (src1Op && src1Op.range.start<108 &&
        if (src1Op && src1Op.range.isSGPR() &&
                    !regRangeCanEqual(src0Op.range, src1Op.range))
            numSgprToRead++;
        //if (src2Op && src2Op.range.start<108 &&
        if (src2Op && src2Op.range.isSGPR())
        {
            if (!regRangeCanEqual(src0Op.range, src2Op.range) &&
                !regRangeCanEqual(src1Op.range, src2Op.range))
                numSgprToRead++;
        }
        
        if (numSgprToRead>=2)
            ASM_FAIL_BY_ERROR(instrPlace, "More than one SGPR to read in instruction")
    }
    
    // put data (instruction words)
    uint32_t words[2];
    cxuint wordsNum = 2;
    if (gcnInsn.encoding == GCNENC_VOP3B)
    {
        // VOP3B encoding
        if (!isGCN12)
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<17) |
                (dstReg.bstart()&0xff) | (uint32_t(sdstReg.bstart())<<8));
        else
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<16) |
                (dstReg.bstart()&0xff) | (uint32_t(sdstReg.bstart())<<8) |
                ((modifiers&VOP3_CLAMP) ? 0x8000 : 0));
    }
    else
    {
        // VOP3A
        if (!isGCN12)
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<17) |
                (dstReg.bstart()&0xff) | ((modifiers&VOP3_CLAMP) ? 0x800: 0) |
                ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0) |
                ((src1Op.vopMods & VOPOP_ABS) ? 0x200 : 0) |
                ((src2Op.vopMods & VOPOP_ABS) ? 0x400 : 0));
        else if (mode2 != GCN_VOP3_VINTRP || mode1 == GCN_NEW_OPCODE ||
            (gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2 ||
            (modifiers & VOP3_VOP3)!=0 || (src0Op.range.bstart()&0x100)!=0/* high */ ||
            (modifiers & (VOP3_CLAMP|3)) != 0 || opMods.opselMod != 0 ||
            src1Op.vopMods!=0 || src2Op.vopMods!=0)
            // new VOP3 for GCN 1.2
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<16) |
                (dstReg.bstart()&0xff) | ((modifiers&VOP3_CLAMP) ? 0x8000: 0) |
                (vop3p ? (uint32_t(opMods.negMod>>4) << 8) /* VOP3P NEG_HI */ :
                    ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0) |
                    ((src1Op.vopMods & VOPOP_ABS) ? 0x200 : 0) |
                    ((src2Op.vopMods & VOPOP_ABS) ? 0x400 : 0)) |
                (((opMods.opselMod & 64) !=0) ? 0x4000 : 0) |
                ((opMods.opselMod&15) << 11));
        else // VINTRP
        {
            SLEV(words[0], 0xd4000000U | (src1Op.range.bstart()&0xff) |
                (uint32_t(src0Op.range.bstart()>>6)<<8) |
                (uint32_t(src0Op.range.bstart()&63)<<10) |
                (uint32_t(gcnInsn.code2)<<16) | (uint32_t(dstReg.bstart()&0xff)<<18));
            // VOP3 VINTRP have only one word instruction
            wordsNum--;
        }
    }
    if (wordsNum==2)
        // second instruction's word 
        SLEV(words[1], src0Op.range.bstart() | (uint32_t(src1Op.range.bstart())<<9) |
                (uint32_t(src2Op.range.bstart())<<18) |
                (vop3p ? ((uint32_t(opMods.opselMod>>4)&3)<<27) :
                 (uint32_t(modifiers & 3) << 27)) |
                /* in VOP3P is also NEG_LO */
                ((src0Op.vopMods & VOPOP_NEG) ? (1U<<29) : 0) |
                ((src1Op.vopMods & VOPOP_NEG) ? (1U<<30) : 0) |
                ((src2Op.vopMods & VOPOP_NEG) ? (1U<<31) : 0));
    
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return false;
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    
    // update register pool (VGPR and SGPR counting)
    if (dstReg && !dstReg.isRegVar())
    {
        if (dstReg.start>=256)
            updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
        else // sgprs
        {
            updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
            updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
        }
    }
    if (sdstReg && !sdstReg.isRegVar())
    {
        updateSGPRsNum(gcnRegs.sgprsNum, sdstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, sdstReg.start, arch);
    }
    if (mode2 != GCN_VOP3_VINTRP)
    {
        // count for SSRC0 and SSRC1 for VOP3A/B encoding (not VINTRP) ???
        if (src0Op.range && !src0Op.range.isRegVar() && src0Op.range.start < 256)
            updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
        if (src1Op.range && !src1Op.range.isRegVar() && src1Op.range.start < 256)
            updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
    }
    if (src2Op.range && !src2Op.range.isRegVar() && src2Op.range.start < 256)
        updateRegFlags(gcnRegs.regFlags, src2Op.range.start, arch);
    return true;
}

bool GCNAsmUtils::parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    RegRange dstReg(0, 0);
    RegRange srcReg(0, 0);
    if (gcnEncSize==GCNEncSize::BIT64)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 32-bit size for VINTRP encoding")
    if (gcnVOPEnc!=GCNVOPEnc::NORMAL)
        ASM_FAIL_BY_ERROR(instrPlace, "DPP and SDWA encoding is illegal for VOP3")
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    // parse DST (VGPR)
    gcnAsm->setCurrentRVU(0);
    good &= parseVRegRange(asmr, linePtr, dstReg, 1, GCNFIELD_VINTRP_VDST, true,
                        INSTROP_SYMREGRANGE|INSTROP_WRITE);
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    if ((gcnInsn.mode & GCN_MASK1) == GCN_P0_P10_P20)
        good &= parseVINTRP0P10P20(asmr, linePtr, srcReg);
    else
    {
        // regular vector register
        gcnAsm->setCurrentRVU(1);
        good &= parseVRegRange(asmr, linePtr, srcReg, 1, GCNFIELD_VINTRP_VSRC0, true,
                        INSTROP_SYMREGRANGE|INSTROP_READ);
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return false;
    
    cxbyte attrVal;
    good &= parseVINTRPAttr(asmr, linePtr, attrVal);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    /* put data (instruction word */
    uint32_t word;
    SLEV(word, 0xc8000000U | (srcReg.bstart()&0xff) | (uint32_t(attrVal&0xff)<<8) |
            (uint32_t(gcnInsn.code1)<<16) | (uint32_t(dstReg.bstart()&0xff)<<18));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word),
            reinterpret_cast<cxbyte*>(&word)+4);
    // update register pool (VGPR counting)
    if (!dstReg.isRegVar())
        updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
    return true;
}

bool GCNAsmUtils::parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, GPUArchMask arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT32)
        ASM_FAIL_BY_ERROR(instrPlace, "Only 64-bit size for DS encoding")
    RegRange dstReg(0, 0);
    RegRange addrReg(0, 0);
    RegRange data0Reg(0, 0), data1Reg(0, 0);
    
    bool beforeData = false;
    bool vdstUsed = false;
    cxuint delayRVU = UINT_MAX;
    cxuint destDelayRVU = UINT_MAX;
    bool secondDelay = false;
    
    GCNAssembler* gcnAsm = static_cast<GCNAssembler*>(asmr.isaAssembler);
    
    if (((gcnInsn.mode & GCN_ADDR_SRC) != 0 || (gcnInsn.mode & GCN_ONLYDST) != 0) &&
            (gcnInsn.mode & GCN_ONLY_SRC) == 0)
    {
        /* vdst is dst */
        cxuint regsNum = (gcnInsn.mode&GCN_REG_DST_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0 || (gcnInsn.mode&GCN_DST128) != 0)
            regsNum = 4;
        gcnAsm->setCurrentRVU(0);
        good &= parseVRegRange(asmr, linePtr, dstReg, regsNum, GCNFIELD_DS_VDST, true,
                    INSTROP_SYMREGRANGE|INSTROP_WRITE);
        vdstUsed = beforeData = true;
        destDelayRVU = 0;
    }
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 && (gcnInsn.mode & GCN_ONLY_SRC) == 0)
    {
        // parse ADDR as first (VGPR)
        if (vdstUsed)
            if (!skipRequiredComma(asmr, linePtr))
                return false;
        gcnAsm->setCurrentRVU(1);
        good &= parseVRegRange(asmr, linePtr, addrReg, 1, GCNFIELD_DS_ADDR, true,
                    INSTROP_SYMREGRANGE|INSTROP_READ);
        beforeData = true;
    }
    
    const uint16_t srcMode = (gcnInsn.mode & GCN_SRCS_MASK);
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 &&
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) != 0 && srcMode != GCN_NOSRC)
    {
        /* two vdata */
        if (beforeData)
            if (!skipRequiredComma(asmr, linePtr))
                return false;
        
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        // parse VDATA0 (VGPR)
        gcnAsm->setCurrentRVU(2);
        good &= parseVRegRange(asmr, linePtr, data0Reg, regsNum, GCNFIELD_DS_DATA0, true,
                    INSTROP_SYMREGRANGE|INSTROP_READ);
        if (srcMode == GCN_2SRCS)
        {
            // instruction have second source
            if (!skipRequiredComma(asmr, linePtr))
                return false;
            // parse VDATA0 (VGPR)
            gcnAsm->setCurrentRVU(3);
            good &= parseVRegRange(asmr, linePtr, data1Reg,
                       (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, GCNFIELD_DS_DATA1, true,
                               INSTROP_SYMREGRANGE|INSTROP_READ);
            secondDelay = true;
        }
        delayRVU = 2;
    }
    
    bool haveGds = false;
    std::unique_ptr<AsmExpression> offsetExpr, offset2Expr;
    char name[10];
    uint16_t offset = 0;
    cxbyte offset1 = 0, offset2 = 0;
    bool haveOffset = false, haveOffset2 = false;
    // parse DS modifiers
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
            good &= parseModEnable(asmr, linePtr, haveGds, "gds modifier");
        else if ((gcnInsn.mode & GCN_2OFFSETS) == 0) /* single offset */
        {
            // single offset
            if (::strcmp(name, "offset") == 0)
            {
                if (parseModImm(asmr, linePtr, offset, &offsetExpr, "offset",
                            0, WS_UNSIGNED))
                {
                    // detect multiple occurrences
                    if (haveOffset)
                        asmr.printWarning(modPlace, "Offset is already defined");
                    haveOffset = true;
                }
                else
                    good = false;
            }
            else
                ASM_NOTGOOD_BY_ERROR(modPlace, "Expected 'offset'")
        }
        else
        {
            // two offsets (offset0, offset1)
            if (::memcmp(name, "offset", 6)==0 &&
                (name[6]=='0' || name[6]=='1') && name[7]==0)
            {
                skipSpacesToEnd(linePtr, end);
                if (linePtr!=end && *linePtr==':')
                {
                    skipCharAndSpacesToEnd(linePtr, end);
                    if (name[6]=='0')
                    {
                        /* offset0 */
                        if (parseImm(asmr, linePtr, offset1, &offsetExpr, 0, WS_UNSIGNED))
                        {
                            // detect multiple occurrences
                            if (haveOffset)
                                asmr.printWarning(modPlace, "Offset0 is already defined");
                            haveOffset = true;
                        }
                        else
                            good = false;
                    }
                    else
                    {
                        /* offset1 */
                        if (parseImm(asmr, linePtr, offset2, &offset2Expr, 0, WS_UNSIGNED))
                        {
                            // detect multiple occurrences
                            if (haveOffset2)
                                asmr.printWarning(modPlace, "Offset1 is already defined");
                            haveOffset2 = true;
                        }
                        else
                            good = false;
                    }
                }
                else
                    ASM_NOTGOOD_BY_ERROR(linePtr, "Expected ':' before offset")
            }
            else
                ASM_NOTGOOD_BY_ERROR(modPlace,
                                "Expected 'offset', 'offset0' or 'offset1'")
        }
    }
    
    if ((gcnInsn.mode & GCN_2OFFSETS) != 0)
        offset = offset1 | (offset2<<8);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return false;
    
    // register Delayed results
    if (destDelayRVU != UINT_MAX)
        gcnAsm->delayedOps[0] = { output.size(), gcnAsm->instrRVUs[destDelayRVU].regVar,
                    gcnAsm->instrRVUs[destDelayRVU].rstart,
                    gcnAsm->instrRVUs[destDelayRVU].rend,
                    1, haveGds ? GCNDELOP_GDSOP : GCNDELOP_LDSOP, GCNDELOP_NONE,
                    gcnAsm->instrRVUs[destDelayRVU].rwFlags };
    
    if (delayRVU != UINT_MAX)
    {
        gcnAsm->delayedOps[1] = { output.size(), gcnAsm->instrRVUs[delayRVU].regVar,
                    gcnAsm->instrRVUs[delayRVU].rstart, gcnAsm->instrRVUs[delayRVU].rend,
                    1, haveGds ? GCNDELOP_GDSOP : GCNDELOP_LDSOP,
                    haveGds ? GCNDELOP_EXPORT : GCNDELOP_NONE,
                    gcnAsm->instrRVUs[delayRVU].rwFlags,
                    cxbyte(haveGds ? ASMRVU_READ : 0) };
        if (secondDelay)
            gcnAsm->delayedOps[2] = { output.size(),
                    gcnAsm->instrRVUs[delayRVU+1].regVar,
                    gcnAsm->instrRVUs[delayRVU+1].rstart,
                    gcnAsm->instrRVUs[delayRVU+1].rend,
                    1, haveGds ? GCNDELOP_GDSOP : GCNDELOP_LDSOP,
                    haveGds ? GCNDELOP_EXPORT : GCNDELOP_NONE,
                    gcnAsm->instrRVUs[delayRVU+1].rwFlags,
                    cxbyte(haveGds ? ASMRVU_READ : 0) };
    }
    if ((gcnInsn.mode & GCN_SRC_ADDR2) != 0)
        // register for DS_*_SRC2_* instructions
        gcnAsm->delayedOps[0] = { output.size(), nullptr, uint16_t(0), uint16_t(0),
                1, haveGds ? GCNDELOP_GDSOP : GCNDELOP_LDSOP,
                haveGds ? GCNDELOP_EXPORT : GCNDELOP_NONE, cxbyte(0) };
    
    if ((gcnInsn.mode&GCN_ONLYGDS) != 0 && !haveGds)
        ASM_FAIL_BY_ERROR(instrPlace, "Instruction requires GDS modifier")
    
    // set target expressions for offsets (if needed)
    if (offsetExpr!=nullptr)
        offsetExpr->setTarget(AsmExprTarget((gcnInsn.mode & GCN_2OFFSETS) ?
                    GCNTGT_DSOFFSET8_0 : GCNTGT_DSOFFSET16, asmr.currentSection,
                    output.size()));
    if (offset2Expr!=nullptr)
        offset2Expr->setTarget(AsmExprTarget(GCNTGT_DSOFFSET8_1, asmr.currentSection,
                    output.size()));
    // put data (two instruction words)
    uint32_t words[2];
    if ((arch & ARCH_GCN_1_2_4)==0)
        SLEV(words[0], 0xd8000000U | uint32_t(offset) | (haveGds ? 0x20000U : 0U) |
                (uint32_t(gcnInsn.code1)<<18));
    else
        SLEV(words[0], 0xd8000000U | uint32_t(offset) | (haveGds ? 0x10000U : 0U) |
                (uint32_t(gcnInsn.code1)<<17));
    SLEV(words[1], (addrReg.bstart()&0xff) | (uint32_t(data0Reg.bstart()&0xff)<<8) |
            (uint32_t(data1Reg.bstart()&0xff)<<16) | (uint32_t(dstReg.bstart()&0xff)<<24));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    offsetExpr.release();
    offset2Expr.release();
    // update register pool (VGPR counting)
    if (dstReg && !dstReg.isRegVar())
        updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
    return true;
}

};
