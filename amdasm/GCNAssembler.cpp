/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <memory>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <CLRX/amdasm/Assembler.h>
#include "GCNAsmInternals.h"

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
    std::unique_ptr<uint16_t[]> oldArchMasks(new uint16_t[tableSize]);
    /* join VOP3A instr with VOP2/VOPC/VOP1 instr together to faster encoding. */
    for (cxuint i = 0; i < tableSize; i++)
    {
        GCNAsmInstruction insn = gcnInstrSortedTable[i];
        if (insn.encoding == GCNENC_VOP3A || insn.encoding == GCNENC_VOP3B)
        {   // check duplicates
            cxuint k = j-1;
            while (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                    (oldArchMasks[k] & insn.archMask)!=insn.archMask) k--;
            
            if (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                (oldArchMasks[k] & insn.archMask)==insn.archMask)
            {   // we found duplicate, we apply
                if (gcnInstrSortedTable[k].code2==UINT16_MAX)
                {   // if second slot for opcode is not filled
                    gcnInstrSortedTable[k].code2 = insn.code1;
                    gcnInstrSortedTable[k].archMask = oldArchMasks[k] & insn.archMask;
                }
                else
                {   // if filled we create new entry
                    oldArchMasks[j] = gcnInstrSortedTable[j].archMask;
                    gcnInstrSortedTable[j] = gcnInstrSortedTable[k];
                    gcnInstrSortedTable[j].archMask = oldArchMasks[k] & insn.archMask;
                    gcnInstrSortedTable[j++].code2 = insn.code1;
                }
            }
            else // not found
            {
                oldArchMasks[j] = insn.archMask;
                gcnInstrSortedTable[j++] = insn;
            }
        }
        else if (insn.encoding == GCNENC_VINTRP)
        {   // check duplicates
            cxuint k = j-1;
            oldArchMasks[j] = insn.archMask;
            gcnInstrSortedTable[j++] = insn;
            while (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                    gcnInstrSortedTable[k].encoding!=GCNENC_VOP3A) k--;
            if (::strcmp(gcnInstrSortedTable[k].mnemonic, insn.mnemonic)==0 &&
                gcnInstrSortedTable[k].encoding==GCNENC_VOP3A)
                // we found VINTRP duplicate, set up second code (VINTRP)
                gcnInstrSortedTable[k].code2 = insn.code1;
        }
        else // normal instruction
        {
            oldArchMasks[j] = insn.archMask;
            gcnInstrSortedTable[j++] = insn;
        }
    }
    gcnInstrSortedTable.resize(j); // final size
    /*for (const GCNAsmInstruction& instr: gcnInstrSortedTable)
        std::cout << "{ " << instr.mnemonic << ", " << cxuint(instr.encoding) <<
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

namespace CLRX
{

static const uint32_t constImmFloatLiterals[9] = 
{
    0x3f000000, 0xbf000000, 0x3f800000, 0xbf800000,
    0x40000000, 0xc0000000, 0x40800000, 0xc0800000, 0x3e22f983
};

static void tryPromoteConstImmToLiteral(GCNOperand& src0Op, uint16_t arch)
{
    if (src0Op.range.start>=128 && src0Op.range.start<=208)
    {
        src0Op.value = src0Op.range.start<193? src0Op.range.start-128 :
                192-src0Op.range.start;
        src0Op.range.start = 255;
    }
    else if ((src0Op.range.start>=240 && src0Op.range.start<248) ||
            ((arch&ARCH_RX3X0)!=0 && src0Op.range.start==248))
    {
        src0Op.value = constImmFloatLiterals[src0Op.range.start-240];
        src0Op.range.start = 255;
    }
}

void GCNAsmUtils::parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    bool good = true;
    RegRange dstReg(0, 0);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
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
    
    if (gcnEncSize==GCNEncSize::BIT64)
    {   // try to promote constant immediate to literal
        tryPromoteConstImmToLiteral(src0Op, arch);
        tryPromoteConstImmToLiteral(src1Op, arch);
    }
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
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
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
    if (dstReg)
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
    if (src0Op.range)
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    if (src1Op.range)
        updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
}

void GCNAsmUtils::parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
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
    
    if (gcnEncSize==GCNEncSize::BIT64)
        // try to promote constant immediate to literal
        tryPromoteConstImmToLiteral(src0Op, arch);
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
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
    // set expression targets
    if (src0Expr!=nullptr)
        src0Expr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words), 
            reinterpret_cast<cxbyte*>(words + wordsNum));
    // prevent freeing expressions
    src0Expr.release();
    
    if (dstReg)
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
    if (src0Op.range)
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
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
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
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
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
    
    uint32_t words[2];
    SLEV(words[0], 0xb0000000U | imm16 | (uint32_t(dstReg.start)<<16) |
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
    if (dstReg && (gcnInsn.mode&GCN_MASK1)!=GCN_DST_SRC && 
                (gcnInsn.mode & GCN_IMM_DST)==0)
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
}

void GCNAsmUtils::parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
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
    
    if (gcnEncSize==GCNEncSize::BIT64)
    {   // try to promote constant immediate to literal
        tryPromoteConstImmToLiteral(src0Op, arch);
        tryPromoteConstImmToLiteral(src1Op, arch);
    }
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
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
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
    
    updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
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
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT64)
    {
        asmr.printError(instrPlace, "Only 32-bit size for SOPP encoding");
        return;
    }
    
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
                name[0] = 0;
                good &= getNameArgS(asmr, 20, name, linePtr, "function name", true);
                toLowerString(name);
                
                cxuint bitPos = 0, bitMask = UINT_MAX;
                bool goodCnt = true;
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
                    goodCnt = good = false;
                }
                
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end || *linePtr!='(')
                {
                    if (goodCnt) // only if cnt has been parsed (do not duplicate errors)
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
                if (linePtr[0] == '&')
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
                    if (gsopIndex==2 && gsopNameIndex==0)
                    {   /* 'emit-cut' handling */
                        if (linePtr+4<end && ::strncmp(linePtr, "-cut", 4)==0 &&
                            (linePtr==end || (!isAlnum(*linePtr) && *linePtr!='_' &&
                            *linePtr!='$' && *linePtr!='.')))
                        {
                            linePtr+=4;
                            gsopIndex++;
                        }
                    }
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
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT32)
    {
        asmr.printError(instrPlace, "Only 64-bit size for SMRD encoding");
        return;
    }
    
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
    updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    updateRegFlags(gcnRegs.regFlags, sbaseReg.start, arch);
    if (soffsetReg)
        updateRegFlags(gcnRegs.regFlags, soffsetReg.start, arch);
}

void GCNAsmUtils::parseSMEMEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT32)
    {
        asmr.printError(instrPlace, "Only 64-bit size for SMRD encoding");
        return;
    }
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
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dataReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dataReg.start, arch);
    }
    updateRegFlags(gcnRegs.regFlags, sbaseReg.start, arch);
    if (soffsetReg)
        updateRegFlags(gcnRegs.regFlags, soffsetReg.start, arch);
}

static Flags correctOpType(uint32_t regsNum, Flags typeMask)
{
    return (regsNum==2 && (typeMask==INSTROP_FLOAT || typeMask==INSTROP_INT)) ?
        INSTROP_V64BIT : typeMask;
}

void GCNAsmUtils::parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
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
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, true,
                       INSTROP_SYMREGRANGE|INSTROP_UNALIGNED);
    else // if VGPRS as destination
        good &= parseVRegRange(asmr, linePtr, dstReg, (gcnInsn.mode&GCN_REG_DST_64)?2:1);
    
    const bool haveDstCC = mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC;
    const bool haveSrcCC = mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC;
    if (haveDstCC) /* VOP3b */
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseSRegRange(asmr, linePtr, dstCCReg, arch, 2, true,
                               INSTROP_SYMREGRANGE|INSTROP_UNALIGNED);
    }
    
    GCNOperand src0Op{}, src1Op{};
    std::unique_ptr<AsmExpression> src0OpExpr, src1OpExpr;
    const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
            (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
    
    const Flags vopOpModFlags = ((haveDstCC && !isGCN12) ?
                    INSTROP_VOP3NEG : INSTROP_VOP3MODS);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
    good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch, regsNum,
            correctOpType(regsNum, literalConstsFlags) | vopOpModFlags |
            INSTROP_UNALIGNED|INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS);
    
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
    regsNum = (gcnInsn.mode&GCN_REG_SRC1_64)?2:1;
    good &= parseOperand(asmr, linePtr, src1Op, &src1OpExpr, arch, regsNum,
            correctOpType(regsNum, literalConstsFlags) | vopOpModFlags |
            (!sgprRegInSrc1 ? INSTROP_VREGS : 0)|INSTROP_SSOURCE|INSTROP_SREGS|
            INSTROP_UNALIGNED | (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0));
    
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
        good &= parseSRegRange(asmr, linePtr, srcCCReg, arch, 2, true,
                               INSTROP_SYMREGRANGE|INSTROP_UNALIGNED);
    }
    
    // modifiers
    cxbyte modifiers = 0;
    VOPExtraModifiers extraMods{};
    good &= parseVOPModifiers(asmr, linePtr, modifiers, (isGCN12) ? &extraMods : nullptr,
                              !haveDstCC || isGCN12);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    bool vop3 = /* src1=sgprs and not (DS1_SGPR|src1_SGPR) */
        ((src1Op.range.start<256) ^ sgprRegInSrc1) ||
        (!isGCN12 && (src0Op.vopMods!=0 || src1Op.vopMods!=0)) ||
        (modifiers&~(VOP3_BOUNDCTRL|(extraMods.needSDWA?VOP3_CLAMP:0)))!=0 ||
        /* srcCC!=VCC or dstCC!=VCC */
        (haveDstCC && dstCCReg.start!=106) || (haveSrcCC && srcCCReg.start!=106) ||
        (gcnEncSize==GCNEncSize::BIT64);
    
    if ((src0Op.range.start==255 || src1Op.range.start==255) &&
        (src0Op.range.start<108 || src0Op.range.start==124 ||
         src1Op.range.start<108 || src1Op.range.start==124))
    {
        asmr.printError(instrPlace, "Literal with SGPR or M0 is illegal");
        return;
    }
    
    cxuint sgprsReaded = 0;
    if (src0Op.range.start<108)
        sgprsReaded++;
    if (src1Op.range.start<108 && src0Op.range.start!=src1Op.range.start)
        sgprsReaded++;
    if (haveSrcCC && src1Op.range.start!=srcCCReg.start &&
                src0Op.range.start!=srcCCReg.start)
        sgprsReaded++;
    
    if (sgprsReaded >= 2)
    {   /* include VCCs (???) */
        asmr.printError(instrPlace, "More than one SGPR to read in instruction");
        return;
    }
    const bool needImm = (src0Op.range.start==255 || src1Op.range.start==255 ||
             mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM);
    
    bool sextFlags = ((src0Op.vopMods|src1Op.vopMods) & VOPOP_SEXT);
    if (isGCN12 && (extraMods.needSDWA || extraMods.needDPP || sextFlags ||
                gcnVOPEnc!=GCNVOPEnc::NORMAL))
    {   /* if VOP_SDWA or VOP_DPP is required */
        if (!checkGCNVOPExtraModifers(asmr, needImm, sextFlags, vop3, gcnVOPEnc, src0Op,
                    extraMods, instrPlace))
            return;
    }
    else if (isGCN12 && ((src0Op.vopMods|src1Op.vopMods) & ~VOPOP_SEXT)!=0 && !sextFlags)
        // if all pass we check we promote VOP3 if only operand modifiers expect sext()
        vop3 = true;
    
    if (isGCN12 && vop3 && haveDstCC && ((src0Op.vopMods|src1Op.vopMods) & VOPOP_ABS) != 0)
    {
        asmr.printError(instrPlace, "Abs modifier is illegal for VOP3B encoding");
        return;
    }
    if (vop3 && needImm)
    {
        asmr.printError(instrPlace, "Literal in VOP3 encoding is illegal");
        return;
    }
    
    if (!checkGCNVOPEncoding(asmr, instrPlace, gcnVOPEnc, &extraMods))
        return;
    
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
        cxuint src0out = src0Op.range.start;
        if (extraMods.needSDWA)
            src0out = 0xf9;
        else if (extraMods.needDPP)
            src0out = 0xfa;
        SLEV(words[0], (uint32_t(gcnInsn.code1)<<25) | src0out |
            (uint32_t(src1Op.range.start&0xff)<<9) | (uint32_t(dstReg.start&0xff)<<17));
        if (extraMods.needSDWA)
            SLEV(words[wordsNum++], (src0Op.range.start&0xff) |
                    (uint32_t(extraMods.dstSel)<<8) |
                    (uint32_t(extraMods.dstUnused)<<11) |
                    ((modifiers & VOP3_CLAMP) ? 0x2000 : 0) |
                    (uint32_t(extraMods.src0Sel)<<16) |
                    ((src0Op.vopMods&VOPOP_SEXT) ? (1U<<19) : 0) |
                    ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                    ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0) |
                    (uint32_t(extraMods.src1Sel)<<24) |
                    ((src1Op.vopMods&VOPOP_SEXT) ? (1U<<27) : 0) |
                    ((src1Op.vopMods&VOPOP_NEG) ? (1U<<28) : 0) |
                    ((src1Op.vopMods&VOPOP_ABS) ? (1U<<29) : 0));
        else if (extraMods.needDPP)
            SLEV(words[wordsNum++], (src0Op.range.start&0xff) | (extraMods.dppCtrl<<8) | 
                    ((modifiers&VOP3_BOUNDCTRL) ? (1U<<19) : 0) |
                    ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                    ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0) |
                    ((src1Op.vopMods&VOPOP_NEG) ? (1U<<22) : 0) |
                    ((src1Op.vopMods&VOPOP_ABS) ? (1U<<23) : 0) |
                    (uint32_t(extraMods.bankMask)<<24) |
                    (uint32_t(extraMods.rowMask)<<28));
        else if (src0Op.range.start==255) // otherwise we check for immediate/literal value
            SLEV(words[wordsNum++], src0Op.value);
        else if (src1Op.range.start==255)
            SLEV(words[wordsNum++], src1Op.value);
        else if (mode1 == GCN_ARG1_IMM || mode1 == GCN_ARG2_IMM)
            SLEV(words[wordsNum++], immValue);
    }
    else
    {   // VOP3 encoding
        uint32_t code = (isGCN12) ?
                (uint32_t(gcnInsn.code2)<<16) | ((modifiers&VOP3_CLAMP) ? 0x8000 : 0) :
                (uint32_t(gcnInsn.code2)<<17) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0);
        if (haveDstCC) // if VOP3B
            SLEV(words[0], 0xd0000000U | code |
                (dstReg.start&0xff) | (uint32_t(dstCCReg.start)<<8));
        else // if VOP3A
            SLEV(words[0], 0xd0000000U | code | (dstReg.start&0xff) |
                ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0) |
                ((src1Op.vopMods & VOPOP_ABS) ? 0x200 : 0));
        SLEV(words[1], src0Op.range.start | (uint32_t(src1Op.range.start)<<9) |
            (uint32_t(srcCCReg.start)<<18) | ((modifiers & 3) << 27) |
            ((src0Op.vopMods & VOPOP_NEG) ? (1U<<29) : 0) |
            ((src1Op.vopMods & VOPOP_NEG) ? (1U<<30) : 0));
        wordsNum++;
    }
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
    
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
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    }
    if (src0Op.range)
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    if (src1Op.range)
        updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
    if (dstCCReg)
    {
        updateSGPRsNum(gcnRegs.sgprsNum, dstCCReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, dstCCReg.start, arch);
    }
    if (srcCCReg)
        updateRegFlags(gcnRegs.regFlags, srcCCReg.start, arch);
}

void GCNAsmUtils::parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_RX3X0)!=0;
    
    RegRange dstReg(0, 0);
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0OpExpr;
    cxbyte modifiers = 0;
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        if (mode1 == GCN_DST_SGPR) // if SGPRS as destination
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                           (gcnInsn.mode&GCN_REG_DST_64)?2:1, true,
                           INSTROP_SYMREGRANGE|INSTROP_UNALIGNED);
        else // if VGPRS as destination
            good &= parseVRegRange(asmr, linePtr, dstReg,
                           (gcnInsn.mode&GCN_REG_DST_64)?2:1);
        
        const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
        
        if (!skipRequiredComma(asmr, linePtr))
            return;
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|
                    INSTROP_VOP3MODS);
    }
    // modifiers
    VOPExtraModifiers extraMods{};
    good &= parseVOPModifiers(asmr, linePtr, modifiers, (isGCN12)?&extraMods:nullptr,
                  true, (mode1!=GCN_VOP_ARG_NONE) ? 2 : 0);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    bool vop3 = ((!isGCN12 && src0Op.vopMods!=0) ||
            (modifiers&~(VOP3_BOUNDCTRL|(extraMods.needSDWA?VOP3_CLAMP:0)))!=0) ||
            (gcnEncSize==GCNEncSize::BIT64);
    
    bool sextFlags = (src0Op.vopMods & VOPOP_SEXT);
    bool needImm = (src0Op && src0Op.range.start==255);
    if (isGCN12 && (extraMods.needSDWA || extraMods.needDPP || sextFlags ||
                gcnVOPEnc!=GCNVOPEnc::NORMAL))
    {   /* if VOP_SDWA or VOP_DPP is required */
        if (!checkGCNVOPExtraModifers(asmr, needImm, sextFlags, vop3, gcnVOPEnc, src0Op,
                    extraMods, instrPlace))
            return;
    }
    else if (isGCN12 && (src0Op.vopMods & ~VOPOP_SEXT)!=0 && !sextFlags)
        // if all pass we check we promote VOP3 if only operand modifiers expect sext()
        vop3 = true;
    
    if (vop3 && src0Op.range.start==255)
    {
        asmr.printError(instrPlace, "Literal in VOP3 encoding is illegal");
        return;
    }
    
    if (!checkGCNVOPEncoding(asmr, instrPlace, gcnVOPEnc, &extraMods))
        return;
    
    if (src0OpExpr!=nullptr)
        src0OpExpr->setTarget(AsmExprTarget(GCNTGT_LITIMM, asmr.currentSection,
                      output.size()));
    
    cxuint wordsNum = 1;
    uint32_t words[2];
    if (!vop3)
    {   // VOP1 encoding
        cxuint src0out = src0Op.range.start;
        if (extraMods.needSDWA)
            src0out = 0xf9;
        else if (extraMods.needDPP)
            src0out = 0xfa;
        SLEV(words[0], 0x7e000000U | (uint32_t(gcnInsn.code1)<<9) | uint32_t(src0out) |
                (uint32_t(dstReg.start&0xff)<<17));
        if (extraMods.needSDWA)
            SLEV(words[wordsNum++], (src0Op.range.start&0xff) |
                    (uint32_t(extraMods.dstSel)<<8) |
                    (uint32_t(extraMods.dstUnused)<<11) |
                    ((modifiers & VOP3_CLAMP) ? 0x2000 : 0) |
                    (uint32_t(extraMods.src0Sel)<<16) |
                    (uint32_t(extraMods.src1Sel)<<24) |
                    ((src0Op.vopMods&VOPOP_SEXT) ? (1U<<19) : 0) |
                    ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                    ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0));
        else if (extraMods.needDPP)
            SLEV(words[wordsNum++], (src0Op.range.start&0xff) | (extraMods.dppCtrl<<8) | 
                    ((modifiers&VOP3_BOUNDCTRL) ? (1U<<19) : 0) |
                    ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                    ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0) |
                    (uint32_t(extraMods.bankMask)<<24) |
                    (uint32_t(extraMods.rowMask)<<28));
        else if (src0Op.range.start==255)
            SLEV(words[wordsNum++], src0Op.value);
    }
    else
    {   // VOP3 encoding
        uint32_t code = (isGCN12) ?
                (uint32_t(gcnInsn.code2)<<16) | ((modifiers&VOP3_CLAMP) ? 0x8000 : 0) :
                (uint32_t(gcnInsn.code2)<<17) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0);
        SLEV(words[0], 0xd0000000U | code | (dstReg.start&0xff) |
            ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0));
        SLEV(words[1], src0Op.range.start | ((modifiers & 3) << 27) |
            ((src0Op.vopMods & VOPOP_NEG) ? (1U<<29) : 0));
        wordsNum++;
    }
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    // update register pool
    if (dstReg)
    {
        if (dstReg.start>=256)
            updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
        else // sgprs
        {
            updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
            updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
        }
    }
    if (src0Op.range)
        updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
}

void GCNAsmUtils::parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_RX3X0)!=0;
    
    RegRange dstReg(0, 0);
    GCNOperand src0Op{};
    std::unique_ptr<AsmExpression> src0OpExpr;
    GCNOperand src1Op{};
    std::unique_ptr<AsmExpression> src1OpExpr;
    cxbyte modifiers = 0;
    
    good &= parseSRegRange(asmr, linePtr, dstReg, arch, 2, true,
                           INSTROP_SYMREGRANGE|INSTROP_UNALIGNED);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    const Flags literalConstsFlags = (mode2==GCN_FLOATLIT) ? INSTROP_FLOAT :
                (mode2==GCN_F16LIT) ? INSTROP_F16 : INSTROP_INT;
    cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
    good &= parseOperand(asmr, linePtr, src0Op, &src0OpExpr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|
                    INSTROP_VOP3MODS);
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    regsNum = (gcnInsn.mode&GCN_REG_SRC1_64)?2:1;
    good &= parseOperand(asmr, linePtr, src1Op, &src1OpExpr, arch, regsNum,
                correctOpType(regsNum, literalConstsFlags) | INSTROP_VOP3MODS|
                INSTROP_UNALIGNED|INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|
                (src0Op.range.start==255 ? INSTROP_ONLYINLINECONSTS : 0));
    // modifiers
    VOPExtraModifiers extraMods{};
    good &= parseVOPModifiers(asmr, linePtr, modifiers, (isGCN12)?&extraMods:nullptr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    bool vop3 = (dstReg.start!=106) || (src1Op.range.start<256) ||
        (!isGCN12 && (src0Op.vopMods!=0 || src1Op.vopMods!=0)) ||
        (modifiers&~(VOP3_BOUNDCTRL|(extraMods.needSDWA?VOP3_CLAMP:0)))!=0 ||
        (gcnEncSize==GCNEncSize::BIT64);
    
    if ((src0Op.range.start==255 || src1Op.range.start==255) &&
        (src0Op.range.start<108 || src0Op.range.start==124 ||
         src1Op.range.start<108|| src1Op.range.start==124))
    {
        asmr.printError(instrPlace, "Literal with SGPR or M0 is illegal");
        return;
    }
    if (src0Op.range.start<108 && src1Op.range.start<108 &&
        src0Op.range.start!=src1Op.range.start)
    {   /* include VCCs (???) */
        asmr.printError(instrPlace, "More than one SGPR to read in instruction");
        return;
    }
    
    const bool needImm = src0Op.range.start==255 || src1Op.range.start==255;
    
    bool sextFlags = ((src0Op.vopMods|src1Op.vopMods) & VOPOP_SEXT);
    if (isGCN12 && (extraMods.needSDWA || extraMods.needDPP || sextFlags ||
                gcnVOPEnc!=GCNVOPEnc::NORMAL))
    {   /* if VOP_SDWA or VOP_DPP is required */
        if (!checkGCNVOPExtraModifers(asmr, needImm, sextFlags, vop3, gcnVOPEnc, src0Op,
                    extraMods, instrPlace))
            return;
    }
    else if (isGCN12 && ((src0Op.vopMods|src1Op.vopMods) & ~VOPOP_SEXT)!=0 && !sextFlags)
        // if all pass we check we promote VOP3 if only operand modifiers expect sext()
        vop3 = true;
    
    if (vop3 && (src0Op.range.start==255 || src1Op.range.start==255))
    {
        asmr.printError(instrPlace, "Literal in VOP3 encoding is illegal");
        return;
    }
    
    if (!checkGCNVOPEncoding(asmr, instrPlace, gcnVOPEnc, &extraMods))
        return;
    
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
        cxuint src0out = src0Op.range.start;
        if (extraMods.needSDWA)
            src0out = 0xf9;
        else if (extraMods.needDPP)
            src0out = 0xfa;
        SLEV(words[0], 0x7c000000U | (uint32_t(gcnInsn.code1)<<17) | src0out |
                (uint32_t(src1Op.range.start&0xff)<<9));
        if (extraMods.needSDWA)
            SLEV(words[wordsNum++], (src0Op.range.start&0xff) |
                    (uint32_t(extraMods.dstSel)<<8) |
                    (uint32_t(extraMods.dstUnused)<<11) |
                    ((modifiers & VOP3_CLAMP) ? 0x2000 : 0) |
                    (uint32_t(extraMods.src0Sel)<<16) |
                    ((src0Op.vopMods&VOPOP_SEXT) ? (1U<<19) : 0) |
                    ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                    ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0) |
                    (uint32_t(extraMods.src1Sel)<<24) |
                    ((src1Op.vopMods&VOPOP_SEXT) ? (1U<<27) : 0) |
                    ((src1Op.vopMods&VOPOP_NEG) ? (1U<<28) : 0) |
                    ((src1Op.vopMods&VOPOP_ABS) ? (1U<<29) : 0));
        else if (extraMods.needDPP)
            SLEV(words[wordsNum++], (src0Op.range.start&0xff) | (extraMods.dppCtrl<<8) | 
                    ((modifiers&VOP3_BOUNDCTRL) ? (1U<<19) : 0) |
                    ((src0Op.vopMods&VOPOP_NEG) ? (1U<<20) : 0) |
                    ((src0Op.vopMods&VOPOP_ABS) ? (1U<<21) : 0) |
                    ((src1Op.vopMods&VOPOP_NEG) ? (1U<<22) : 0) |
                    ((src1Op.vopMods&VOPOP_ABS) ? (1U<<23) : 0) |
                    (uint32_t(extraMods.bankMask)<<24) |
                    (uint32_t(extraMods.rowMask)<<28));
        else if (src0Op.range.start==255)
            SLEV(words[wordsNum++], src0Op.value);
        else if (src1Op.range.start==255)
            SLEV(words[wordsNum++], src1Op.value);
    }
    else
    {   // VOP3 encoding
        uint32_t code = (isGCN12) ?
                (uint32_t(gcnInsn.code2)<<16) | ((modifiers&VOP3_CLAMP) ? 0x8000 : 0) :
                (uint32_t(gcnInsn.code2)<<17) | ((modifiers&VOP3_CLAMP) ? 0x800 : 0);
        SLEV(words[0], 0xd0000000U | code | (dstReg.start&0xff) |
                ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0) |
                ((src1Op.vopMods & VOPOP_ABS) ? 0x200 : 0));
        SLEV(words[1], src0Op.range.start | (uint32_t(src1Op.range.start)<<9) |
            ((modifiers & 3) << 27) | ((src0Op.vopMods & VOPOP_NEG) ? (1U<<29) : 0) |
            ((src1Op.vopMods & VOPOP_NEG) ? (1U<<30) : 0));
        wordsNum++;
    }
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    /// prevent freeing expression
    src0OpExpr.release();
    src1OpExpr.release();
    // update register pool
    updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
    updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
    updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
    updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
}

void GCNAsmUtils::parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t mode2 = (gcnInsn.mode & GCN_MASK2);
    const bool isGCN12 = (arch & ARCH_RX3X0)!=0;
    if (gcnVOPEnc!=GCNVOPEnc::NORMAL)
    {
        asmr.printError(instrPlace, "DPP and SDWA encoding is illegal for VOP3");
        return;
    }
    
    RegRange dstReg(0, 0);
    RegRange sdstReg(0, 0);
    GCNOperand src0Op{};
    GCNOperand src1Op{};
    GCNOperand src2Op{};
    
    const bool is128Ops = (gcnInsn.mode & 0xf000) == GCN_VOP3_DS2_128;
    bool modHigh = false;
    cxbyte modifiers = 0;
    const Flags vop3Mods = (gcnInsn.encoding == GCNENC_VOP3B) ?
            INSTROP_VOP3NEG : INSTROP_VOP3MODS;
    
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        if ((gcnInsn.mode&GCN_VOP3_DST_SGPR)==0)
            good &= parseVRegRange(asmr, linePtr, dstReg,
                       (is128Ops) ? 4 : ((gcnInsn.mode&GCN_REG_DST_64)?2:1));
        else // SGPRS as dest
            good &= parseSRegRange(asmr, linePtr, dstReg, arch,
                       (gcnInsn.mode&GCN_REG_DST_64)?2:1, true,
                       INSTROP_SYMREGRANGE|INSTROP_UNALIGNED);
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
        
        cxuint regsNum;
        if (mode2 != GCN_VOP3_VINTRP)
        {
            regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
            good &= parseOperand(asmr, linePtr, src0Op, nullptr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|INSTROP_LDS|vop3Mods|
                    INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR);
        }
        
        if (mode2 == GCN_VOP3_VINTRP)
        {
            if (mode1 != GCN_P0_P10_P20)
                good &= parseVRegRange(asmr, linePtr, src1Op.range, 1);
            else /* P0_P10_P20 */
                good &= parseVINTRP0P10P20(asmr, linePtr, src1Op.range);
            
            if (!skipRequiredComma(asmr, linePtr))
                return;
            
            cxbyte attr;
            good &= parseVINTRPAttr(asmr, linePtr, attr);
            attr = ((attr&3)<<6) | ((attr&0xfc)>>2);
            src0Op.range = { attr, uint16_t(attr+1) };
            
            if ((gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2)
            {
                if (!skipRequiredComma(asmr, linePtr))
                    return;
                good &= parseOperand(asmr, linePtr, src2Op, nullptr, arch,
                    (gcnInsn.mode&GCN_REG_SRC2_64)?2:1, INSTROP_UNALIGNED|INSTROP_VREGS|
                    INSTROP_SREGS);
            }
            // high and vop3
            const char* end = asmr.line+asmr.lineSize;
            while (true)
            {
                skipSpacesToEnd(linePtr, end);
                if (linePtr==end)
                    break;
                char modName[10];
                const char* modPlace = linePtr;
                if (!getNameArgS(asmr, 10, modName, linePtr, "VINTRP modifier"))
                    continue;
                if (::strcmp(modName, "high")==0)
                    modHigh = true;
                else if (::strcmp(modName, "vop3")==0)
                    modifiers |= VOP3_VOP3;
                else
                {
                    asmr.printError(modPlace, "Unknown VINTRP modifier");
                    good = false;
                }
            }
            if (modHigh)
            {
                src0Op.range.start+=0x100;
                src0Op.range.end+=0x100;
            }
        }
        else if (mode1 != GCN_SRC12_NONE)
        {
            if (!skipRequiredComma(asmr, linePtr))
                return;
            regsNum = (gcnInsn.mode&GCN_REG_SRC1_64)?2:1;
            good &= parseOperand(asmr, linePtr, src1Op, nullptr, arch, regsNum,
                    correctOpType(regsNum, literalConstsFlags)|INSTROP_VREGS|
                    INSTROP_UNALIGNED|INSTROP_SSOURCE|INSTROP_SREGS|vop3Mods|
                    INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR);
         
            if (mode1 != GCN_SRC2_NONE && mode1 != GCN_DST_VCC)
            {
                if (!skipRequiredComma(asmr, linePtr))
                    return;
                regsNum = (gcnInsn.mode&GCN_REG_SRC2_64)?2:1;
                good &= parseOperand(asmr, linePtr, src2Op, nullptr, arch,
                        is128Ops ? 4 : regsNum,
                        correctOpType(regsNum, literalConstsFlags)|INSTROP_UNALIGNED|
                        INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_SREGS|
                        vop3Mods|INSTROP_ONLYINLINECONSTS|INSTROP_NOLITERALERROR);
            }
        }
    }
    // modifiers
    if (mode2 != GCN_VOP3_VINTRP)
        good &= parseVOPModifiers(asmr, linePtr, modifiers, nullptr,
                              isGCN12 || gcnInsn.encoding!=GCNENC_VOP3B);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (mode2 != GCN_VOP3_VINTRP)
    {
        cxuint numSgprToRead = 0;
        if (src0Op.range.start<108)
            numSgprToRead++;
        if (src1Op && src1Op.range.start<108 &&
                    src0Op.range.start!=src1Op.range.start)
            numSgprToRead++;
        if (src2Op && src2Op.range.start<108 &&
                src0Op.range.start!=src2Op.range.start &&
                src1Op.range.start!=src2Op.range.start)
            numSgprToRead++;
        
        if (numSgprToRead>=2)
        {
            asmr.printError(instrPlace, "More than one SGPR to read in instruction");
            return;
        }
    }
    
    uint32_t words[2];
    cxuint wordsNum = 2;
    if (gcnInsn.encoding == GCNENC_VOP3B)
    {
        if (!isGCN12)
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<17) |
                (dstReg.start&0xff) | (uint32_t(sdstReg.start)<<8));
        else
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<16) |
                (dstReg.start&0xff) | (uint32_t(sdstReg.start)<<8) |
                ((modifiers&VOP3_CLAMP) ? 0x8000 : 0));
    }
    else // VOP3A
    {
        if (!isGCN12)
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<17) |
                (dstReg.start&0xff) | ((modifiers&VOP3_CLAMP) ? 0x800: 0) |
                ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0) |
                ((src1Op.vopMods & VOPOP_ABS) ? 0x200 : 0) |
                ((src2Op.vopMods & VOPOP_ABS) ? 0x400 : 0));
        else if (mode2 != GCN_VOP3_VINTRP || mode1 == GCN_NEW_OPCODE ||
            (gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2 ||
            (modifiers & VOP3_VOP3)!=0 || (src0Op.range.start&0x100)!=0/* high */)
            SLEV(words[0], 0xd0000000U | (uint32_t(gcnInsn.code1)<<16) |
                (dstReg.start&0xff) | ((modifiers&VOP3_CLAMP) ? 0x8000: 0) |
                ((src0Op.vopMods & VOPOP_ABS) ? 0x100 : 0) |
                ((src1Op.vopMods & VOPOP_ABS) ? 0x200 : 0) |
                ((src2Op.vopMods & VOPOP_ABS) ? 0x400 : 0));
        else // VINTRP
        {
            SLEV(words[0], 0xd4000000U | (src1Op.range.start&0xff) |
                (uint32_t(src0Op.range.start>>6)<<8) |
                (uint32_t(src0Op.range.start&63)<<10) |
                (uint32_t(gcnInsn.code2)<<16) | (uint32_t(dstReg.start&0xff)<<18));
            wordsNum--;
        }
    }
    if (wordsNum==2)
        SLEV(words[1], src0Op.range.start | (uint32_t(src1Op.range.start)<<9) |
                (uint32_t(src2Op.range.start)<<18) | ((modifiers & 3) << 27) |
                ((src0Op.vopMods & VOPOP_NEG) ? (1U<<29) : 0) |
                ((src1Op.vopMods & VOPOP_NEG) ? (1U<<30) : 0) |
                ((src2Op.vopMods & VOPOP_NEG) ? (1U<<31) : 0));
    
    if (!checkGCNEncodingSize(asmr, instrPlace, gcnEncSize, wordsNum))
        return;
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + wordsNum));
    // update register pool
    if (dstReg)
    {
        if (dstReg.start>=256)
            updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
        else // sgprs
        {
            updateSGPRsNum(gcnRegs.sgprsNum, dstReg.end-1, arch);
            updateRegFlags(gcnRegs.regFlags, dstReg.start, arch);
        }
    }
    if (sdstReg)
    {
        updateSGPRsNum(gcnRegs.sgprsNum, sdstReg.end-1, arch);
        updateRegFlags(gcnRegs.regFlags, sdstReg.start, arch);
    }
    if (mode2 != GCN_VOP3_VINTRP)
    {
        if (src0Op.range && src0Op.range.start < 256)
            updateRegFlags(gcnRegs.regFlags, src0Op.range.start, arch);
        if (src1Op.range && src1Op.range.start < 256)
            updateRegFlags(gcnRegs.regFlags, src1Op.range.start, arch);
    }
    if (src2Op.range && src2Op.range.start < 256)
        updateRegFlags(gcnRegs.regFlags, src2Op.range.start, arch);
}

void GCNAsmUtils::parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc)
{
    bool good = true;
    RegRange dstReg(0, 0);
    RegRange srcReg(0, 0);
    if (gcnEncSize==GCNEncSize::BIT64)
    {
        asmr.printError(instrPlace, "Only 32-bit size for VINTRP encoding");
        return;
    }
    if (gcnVOPEnc!=GCNVOPEnc::NORMAL)
    {
        asmr.printError(instrPlace, "DPP and SDWA encoding is illegal for VOP3");
        return;
    }
    
    good &= parseVRegRange(asmr, linePtr, dstReg, 1);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    if ((gcnInsn.mode & GCN_MASK1) == GCN_P0_P10_P20)
        good &= parseVINTRP0P10P20(asmr, linePtr, srcReg);
    else // regular vector register
        good &= parseVRegRange(asmr, linePtr, srcReg, 1);
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    
    cxbyte attrVal;
    good &= parseVINTRPAttr(asmr, linePtr, attrVal);
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    /* */
    uint32_t word;
    SLEV(word, 0xc8000000U | (srcReg.start&0xff) | (uint32_t(attrVal&0xff)<<8) |
            (uint32_t(gcnInsn.code1)<<16) | (uint32_t(dstReg.start&0xff)<<18));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(&word),
            reinterpret_cast<cxbyte*>(&word)+4);
    
    updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
}

void GCNAsmUtils::parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT32)
    {
        asmr.printError(instrPlace, "Only 64-bit size for DS encoding");
        return;
    }
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
        if ((gcnInsn.mode&GCN_DS_128) != 0 || (gcnInsn.mode&GCN_DST128) != 0)
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
    if ((arch & ARCH_RX3X0)==0)
        SLEV(words[0], 0xd8000000U | uint32_t(offset) | (haveGds ? 0x20000U : 0U) |
                (uint32_t(gcnInsn.code1)<<18));
    else
        SLEV(words[0], 0xd8000000U | uint32_t(offset) | (haveGds ? 0x10000U : 0U) |
                (uint32_t(gcnInsn.code1)<<17));
    SLEV(words[1], (addrReg.start&0xff) | (uint32_t(data0Reg.start&0xff)<<8) |
            (uint32_t(data1Reg.start&0xff)<<16) | (uint32_t(dstReg.start&0xff)<<24));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    offsetExpr.release();
    offset2Expr.release();
    // update register pool
    if (dstReg)
        updateVGPRsNum(gcnRegs.vgprsNum, dstReg.end-257);
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
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    bool good = true;
    if (gcnEncSize==GCNEncSize::BIT32)
    {
        asmr.printError(instrPlace, "Only 64-bit size for MUBUF/MTBUF encoding");
        return;
    }
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    RegRange vaddrReg(0, 0);
    RegRange vdataReg(0, 0);
    GCNOperand soffsetOp{};
    RegRange srsrcReg(0, 0);
    const bool isGCN12 = (arch & ARCH_RX3X0)!=0;
    
    skipSpacesToEnd(linePtr, end);
    const char* vdataPlace = linePtr;
    const char* vaddrPlace = nullptr;
    bool parsedVaddr = false;
    if (mode1 != GCN_ARG_NONE)
    {
        if (mode1 != GCN_MUBUF_NOVAD)
        {
            good &= parseVRegRange(asmr, linePtr, vdataReg, 0);
            if (!skipRequiredComma(asmr, linePtr))
                return;
            
            skipSpacesToEnd(linePtr, end);
            vaddrPlace = linePtr;
            if (!parseVRegRange(asmr, linePtr, vaddrReg, 0, false))
                good = false;
            if (vaddrReg) // only if vaddr is
            {
                parsedVaddr = true;
                if (!skipRequiredComma(asmr, linePtr))
                    return;
            }
            else
            {// if not, default is v0
                if (linePtr+3<=end && ::strncasecmp(linePtr, "off", 3)==0 &&
                    (isSpace(linePtr[3]) || linePtr[3]==','))
                {
                    linePtr+=3;
                    if (!skipRequiredComma(asmr, linePtr))
                        return;
                }
                vaddrReg = {256, 257};
            }
        }
        good &= parseSRegRange(asmr, linePtr, srsrcReg, arch, 4);
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseOperand(asmr, linePtr, soffsetOp, nullptr, arch, 1,
                 INSTROP_SREGS|INSTROP_SSOURCE|INSTROP_ONLYINLINECONSTS|
                 INSTROP_NOLITERALERRORMUBUF);
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
        else if (!isGCN12 && ::strcmp(name, "addr64")==0)
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
        if (!parsedVaddr && (haveIdxen || haveOffen || haveAddr64))
        {   // no vaddr in instruction
            asmr.printError(vaddrPlace, "VADDR is required if idxen, offen "
                    "or addr64 is enabled");
            good = false;
        }
        else
        {
            const cxuint vaddrSize = ((haveOffen&&haveIdxen) || haveAddr64) ? 2 : 1;
            if (!isXRegRange(vaddrReg, vaddrSize))
            {
                asmr.printError(vaddrPlace, (vaddrSize==2) ? "Required 2 vector registers" : 
                            "Required 1 vector register");
                good = false;
            }
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
    if (haveTfe && haveLds)
    {
        asmr.printError(instrPlace, "Both LDS and TFE is illegal");
        return;
    }
    
    if (offsetExpr!=nullptr)
        offsetExpr->setTarget(AsmExprTarget(GCNTGT_MXBUFOFFSET, asmr.currentSection,
                    output.size()));
    
    uint32_t words[2];
    if (gcnInsn.encoding==GCNENC_MUBUF)
        SLEV(words[0], 0xe0000000U | offset | (haveOffen ? 0x1000U : 0U) |
                (haveIdxen ? 0x2000U : 0U) | (haveGlc ? 0x4000U : 0U) |
                ((haveAddr64 && !isGCN12) ? 0x8000U : 0U) | (haveLds ? 0x10000U : 0U) |
                ((haveSlc && isGCN12) ? 0x20000U : 0) | (uint32_t(gcnInsn.code1)<<18));
    else // MTBUF
    {
        uint32_t code = (isGCN12) ? (uint32_t(gcnInsn.code1)<<15) :
                (uint32_t(gcnInsn.code1)<<16);
        SLEV(words[0], 0xe8000000U | offset | (haveOffen ? 0x1000U : 0U) |
                (haveIdxen ? 0x2000U : 0U) | (haveGlc ? 0x4000U : 0U) |
                ((haveAddr64 && !isGCN12) ? 0x8000U : 0U) | code |
                (uint32_t(dfmt)<<19) | (uint32_t(nfmt)<<23));
    }
    
    SLEV(words[1], (vaddrReg.start&0xff) | (uint32_t(vdataReg.start&0xff)<<8) |
            (uint32_t(srsrcReg.start>>2)<<16) |
            ((haveSlc && (!isGCN12 || gcnInsn.encoding==GCNENC_MTBUF)) ? (1U<<22) : 0) |
            (haveTfe ? (1U<<23) : 0) | (uint32_t(soffsetOp.range.start)<<24));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    offsetExpr.release();
    // update register pool (instr loads or save old value) */
    if (vdataReg && ((gcnInsn.mode&GCN_MLOAD) != 0 ||
                ((gcnInsn.mode&GCN_MATOMIC)!=0 && haveGlc)))
        updateVGPRsNum(gcnRegs.vgprsNum, vdataReg.end-257);
    if (soffsetOp.range)
        updateRegFlags(gcnRegs.regFlags, soffsetOp.range.start, arch);
}

void GCNAsmUtils::parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    if (gcnEncSize==GCNEncSize::BIT32)
    {
        asmr.printError(instrPlace, "Only 64-bit size for MIMG encoding");
        return;
    }
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
    const char* vaddrPlace = linePtr;
    good &= parseVRegRange(asmr, linePtr, vaddrReg, 0);
    cxuint geRegRequired = (gcnInsn.mode&GCN_MIMG_VA_MASK)+1;
    cxuint vaddrRegsNum = vaddrReg.end-vaddrReg.start;
    cxuint vaddrMaxExtraRegs = (gcnInsn.mode&GCN_MIMG_VADERIV) ? 7 : 3;
    if (vaddrRegsNum < geRegRequired || vaddrRegsNum > geRegRequired+vaddrMaxExtraRegs)
    {
        char buf[60];
        snprintf(buf, 60, "Required (%u-%u) vector registers", geRegRequired,
                 geRegRequired+vaddrMaxExtraRegs);
        asmr.printError(vaddrPlace, buf);
        good = false;
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    const char* srsrcPlace = linePtr;
    good &= parseSRegRange(asmr, linePtr, srsrcReg, arch, 0);
    
    if ((gcnInsn.mode & GCN_MIMG_SAMPLE) != 0)
    {
        if (!skipRequiredComma(asmr, linePtr))
            return;
        good &= parseSRegRange(asmr, linePtr, ssampReg, arch, 4);
    }
    
    bool haveTfe = false, haveSlc = false, haveGlc = false;
    bool haveDa = false, haveR128 = false, haveLwe = false, haveUnorm = false;
    bool haveDMask = false, haveD16 = false;
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
            else if ((arch & ARCH_RX3X0)!=0 && name[1]=='1' && name[2]=='6' && name[3]==0)
                haveD16 = true;
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
    
    cxuint dregsNum = 4;
    if ((gcnInsn.mode & GCN_MIMG_VDATA4) == 0)
        dregsNum = ((dmask & 1)?1:0) + ((dmask & 2)?1:0) + ((dmask & 4)?1:0) +
                ((dmask & 8)?1:0) + (haveTfe);
    if (dregsNum!=0 && !isXRegRange(vdataReg, dregsNum))
    {
        char errorMsg[40];
        snprintf(errorMsg, 40, "Required %u vector register%s", dregsNum,
                 (dregsNum>1) ? "s" : "");
        asmr.printError(vdataPlace, errorMsg);
        good = false;
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
            (uint32_t(srsrcReg.start>>2)<<16) | (uint32_t(ssampReg.start>>2)<<21) |
            (haveD16 ? (1U<<31) : 0));
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
    
    // update register pool (instr loads or save old value) */
    if (vdataReg && ((gcnInsn.mode&GCN_MLOAD) != 0 ||
                ((gcnInsn.mode&GCN_MATOMIC)!=0 && haveGlc)))
        updateVGPRsNum(gcnRegs.vgprsNum, vdataReg.end-257);
}

void GCNAsmUtils::parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    if (gcnEncSize==GCNEncSize::BIT32)
    {
        asmr.printError(instrPlace, "Only 64-bit size for EXP encoding");
        return;
    }
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
    SLEV(words[0], ((arch&ARCH_RX3X0) ? 0xc4000000 : 0xf8000000U) | enMask |
            (uint32_t(target)<<4) | (haveCompr ? 0x400 : 0) | (haveDone ? 0x800 : 0) |
            (haveVM ? 0x1000U : 0));
    SLEV(words[1], uint32_t(vsrcsReg[0].start&0xff) |
            (uint32_t(vsrcsReg[1].start&0xff)<<8) |
            (uint32_t(vsrcsReg[2].start&0xff)<<16) |
            (uint32_t(vsrcsReg[3].start&0xff)<<24));
    
    output.insert(output.end(), reinterpret_cast<cxbyte*>(words),
            reinterpret_cast<cxbyte*>(words + 2));
}

void GCNAsmUtils::parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                  const char* instrPlace, const char* linePtr, uint16_t arch,
                  std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                  GCNEncSize gcnEncSize)
{
    const char* end = asmr.line+asmr.lineSize;
    if (gcnEncSize==GCNEncSize::BIT32)
    {
        asmr.printError(instrPlace, "Only 64-bit size for FLAT encoding");
        return;
    }
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
        cxuint dstRegsNum = ((gcnInsn.mode & GCN_CMPSWAP)!=0) ? (dregsNum>>1) : dregsNum;
        dstRegsNum = (haveTfe) ? dstRegsNum+1:dstRegsNum; // include tfe 
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
    if (vdstReg)
        updateVGPRsNum(gcnRegs.vgprsNum, vdstReg.end-257);
}

};

void GCNAssembler::assemble(const CString& inMnemonic, const char* mnemPlace,
            const char* linePtr, const char* lineEnd, std::vector<cxbyte>& output)
{
    CString mnemonic;
    size_t inMnemLen = inMnemonic.size();
    GCNEncSize gcnEncSize = GCNEncSize::UNKNOWN;
    GCNVOPEnc vopEnc = GCNVOPEnc::NORMAL;
    if (inMnemLen>4 && ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e64")==0)
    {
        gcnEncSize = GCNEncSize::BIT64;
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    }
    else if (inMnemLen>4 && ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e32")==0)
    {
        gcnEncSize = GCNEncSize::BIT32;
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    }
    else if (inMnemLen>6 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_dpp")==0)
    {
        vopEnc = GCNVOPEnc::DPP;
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    }
    else if (inMnemLen>7 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-5, "_sdwa")==0)
    {
        vopEnc = GCNVOPEnc::SDWA;
        mnemonic = inMnemonic.substr(0, inMnemLen-5);
    }
    else
        mnemonic = inMnemonic;
    
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
            GCNAsmUtils::parseSOPCEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOPP:
            GCNAsmUtils::parseSOPPEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOP1:
            GCNAsmUtils::parseSOP1Encoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOP2:
            GCNAsmUtils::parseSOP2Encoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SOPK:
            GCNAsmUtils::parseSOPKEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_SMRD:
            if (curArchMask & ARCH_RX3X0)
                GCNAsmUtils::parseSMEMEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            else
                GCNAsmUtils::parseSMRDEncoding(assembler, *it, mnemPlace, linePtr,
                               curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_VOPC:
            GCNAsmUtils::parseVOPCEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VOP1:
            GCNAsmUtils::parseVOP1Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VOP2:
            GCNAsmUtils::parseVOP2Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VOP3A:
        case GCNENC_VOP3B:
            GCNAsmUtils::parseVOP3Encoding(assembler, *it, mnemPlace, linePtr,
                                   curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_VINTRP:
            GCNAsmUtils::parseVINTRPEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize, vopEnc);
            break;
        case GCNENC_DS:
            GCNAsmUtils::parseDSEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_MUBUF:
        case GCNENC_MTBUF:
            GCNAsmUtils::parseMUBUFEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_MIMG:
            GCNAsmUtils::parseMIMGEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_EXP:
            GCNAsmUtils::parseEXPEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
            break;
        case GCNENC_FLAT:
            GCNAsmUtils::parseFLATEncoding(assembler, *it, mnemPlace, linePtr,
                           curArchMask, output, regs, gcnEncSize);
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

bool GCNAssembler::checkMnemonic(const CString& inMnemonic) const
{
    CString mnemonic;
    size_t inMnemLen = inMnemonic.size();
    if (inMnemLen>4 &&
        (::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e64")==0 ||
            ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_e32")==0))
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    else if (inMnemLen>6 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-4, "_dpp")==0)
        mnemonic = inMnemonic.substr(0, inMnemLen-4);
    else if (inMnemLen>7 && toLower(inMnemonic[0])=='v' && inMnemonic[1]=='_' &&
        ::strcasecmp(inMnemonic.c_str()+inMnemLen-5, "_sdwa")==0)
        mnemonic = inMnemonic.substr(0, inMnemLen-5);
    else
        mnemonic = inMnemonic;
    
    return std::binary_search(gcnInstrSortedTable.begin(), gcnInstrSortedTable.end(),
               GCNAsmInstruction{mnemonic.c_str()},
               [](const GCNAsmInstruction& instr1, const GCNAsmInstruction& instr2)
               { return ::strcmp(instr1.mnemonic, instr2.mnemonic)<0; });
}

void GCNAssembler::setAllocatedRegisters(const cxuint* inRegs, Flags inRegFlags)
{
    if (inRegs==nullptr)
        regs.sgprsNum = regs.vgprsNum = 0;
    else // if not null, just copy
        std::copy(inRegs, inRegs+2, regTable);
    regs.regFlags = inRegFlags;
}

const cxuint* GCNAssembler::getAllocatedRegisters(size_t& regTypesNum,
              Flags& outRegFlags) const
{
    regTypesNum = 2;
    outRegFlags = regs.regFlags;
    return regTable;
}

void GCNAssembler::fillAlignment(size_t size, cxbyte* output)
{
    uint32_t value = LEV(0xbf800000U); // fill with s_nop's
    if ((size&3)!=0)
    {   // first, we fill zeros
        const size_t toAlign4 = 4-(size&3);
        ::memset(output, 0, toAlign4);
        output += toAlign4;
    }
    std::fill((uint32_t*)output, ((uint32_t*)output) + (size>>2), value);
}

bool GCNAssembler::parseRegisterRange(const char*& linePtr, cxuint& regStart,
          cxuint& regEnd, const AsmRegVar*& regVar)
{
    GCNOperand operand;
    if (!GCNAsmUtils::parseOperand(assembler, linePtr, operand, nullptr, curArchMask, 0,
                INSTROP_SREGS|INSTROP_VREGS|INSTROP_SSOURCE|INSTROP_UNALIGNED))
        return false;
    regStart = operand.range.start;
    regEnd = operand.range.end;
    return true;
}

bool GCNAssembler::relocationIsFit(cxuint bits, AsmExprTargetType tgtType)
{
    if (bits==32)
        return tgtType==GCNTGT_SOPJMP || tgtType==GCNTGT_LITIMM;
    return false;
}

bool GCNAssembler::parseRegisterType(const char*& linePtr, const char* end, cxuint& type)
{
    skipSpacesToEnd(linePtr, end);
    if (linePtr!=end)
    {
        const char c = toLower(*linePtr);
        if (c=='v' || c=='s')
        {
            type = c=='v' ? REGTYPE_VGPR : REGTYPE_SGPR;
            linePtr++;
            return true;
        }
        return false;
    }
    return false;
}
