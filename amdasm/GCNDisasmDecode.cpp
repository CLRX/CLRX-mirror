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
#include <algorithm>
#include <iostream>
#include <cstring>
#include <mutex>
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/MemAccess.h>
#include "GCNInternals.h"
#include "GCNDisasmInternals.h"

using namespace CLRX;

// put chars to buffer (helper)
static inline void putChars(char*& buf, const char* input, size_t size)
{
    ::memcpy(buf, input, size);
    buf += size;
}

static inline void putCommaSpace(char*& bufPtr)
{
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
}

static const char* gcnOperandFloatTable[] =
{
    "0.5", "-0.5", "1.0", "-1.0", "2.0", "-2.0", "4.0", "-4.0"
};

union CLRX_INTERNAL FloatUnion
{
    float f;
    uint32_t u;
};

// simple helpers for writing values: bytes, regranges

static inline void putByteToBuf(cxuint op, char*& bufPtr)
{
    cxuint val = op;
    if (val >= 100U)
    {
        cxuint digit2 = val/100U;
        *bufPtr++ = digit2+'0';
        val -= digit2*100U;
    }
    {
        const cxuint digit1 = val/10U;
        if (digit1 != 0 || op >= 100U)
            *bufPtr++ = digit1 + '0';
        *bufPtr++ = val-digit1*10U + '0';
    }
}

static inline void putHexByteToBuf(cxuint op, char*& bufPtr)
{
    const cxuint digit0 = op&0xf;
    const cxuint digit1 = (op&0xf0)>>4;
    *bufPtr++ = '0';
    *bufPtr++ = 'x';
    if (digit1 != 0)
        *bufPtr++ = (digit1<=9)?'0'+digit1:'a'+digit1-10;
    *bufPtr++ = (digit0<=9)?'0'+digit0:'a'+digit0-10;
}

// print regranges
static void regRanges(cxuint op, cxuint vregNum, char*& bufPtr)
{
    if (vregNum!=1)
        *bufPtr++ = '[';
    putByteToBuf(op, bufPtr);
    if (vregNum!=1)
    {
        *bufPtr++ = ':';
        op += vregNum-1;
        if (op > 255)
            op -= 256; // fix for VREGS
        putByteToBuf(op, bufPtr);
        *bufPtr++ = ']';
    }
}

static void decodeGCNVRegOperand(cxuint op, cxuint vregNum, char*& bufPtr)
{
    *bufPtr++ = 'v';
    return regRanges(op, vregNum, bufPtr);
}

/* parameters: dasm - disassembler, codePos - code position for relocation,
 * relocIter, optional - if literal is optional (can be replaced by inline constant) */
void GCNDisasmUtils::printLiteral(GCNDisassembler& dasm, size_t codePos,
          RelocIter& relocIter, uint32_t literal, FloatLitType floatLit, bool optional)
{
    // if with relocation, just write
    if (dasm.writeRelocation(dasm.startOffset + (codePos<<2)-4, relocIter))
        return;
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(50);
    char* bufPtr = bufStart;
    if (optional && (int32_t(literal)<=64 && int32_t(literal)>=-16)) // use lit(...)
    {
        // use lit() to force correct encoding (avoid constants)
        putChars(bufPtr, "lit(", 4);
        bufPtr += itocstrCStyle<int32_t>(literal, bufPtr, 11, 10);
        *bufPtr++ = ')';
    }
    else
        bufPtr += itocstrCStyle(literal, bufPtr, 11, 16);
    if (floatLit != FLTLIT_NONE)
    {
        // print float point (FP16 or FP32) literal in comment
        FloatUnion fu;
        fu.u = literal;
        putChars(bufPtr, " /* ", 4);
        if (floatLit == FLTLIT_F32)
            bufPtr += ftocstrCStyle(fu.f, bufPtr, 20);
        else
            bufPtr += htocstrCStyle(fu.u, bufPtr, 20);
        *bufPtr++ = (floatLit == FLTLIT_F32)?'f':'h';
        putChars(bufPtr, " */", 3);
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeGCNOperandNoLit(GCNDisassembler& dasm, cxuint op,
           cxuint regNum, char*& bufPtr, GPUArchMask arch, FloatLitType floatLit)
{
    const bool isGCN12 = ((arch&ARCH_GCN_1_2_4)!=0);
    const bool isGCN14 = ((arch&ARCH_GCN_1_4)!=0);
    const cxuint maxSgprsNum = getGPUMaxRegsNumByArchMask(arch, REGTYPE_SGPR);
    if ((op < maxSgprsNum) || (op >= 256 && op < 512))
    {
        // vector
        if (op >= 256)
        {
            *bufPtr++ = 'v';
            op -= 256;
        }
        // scalar
        else
            *bufPtr++ = 's';
        regRanges(op, regNum, bufPtr);
        return;
    }
    
    const cxuint op2 = op&~1U;
    if (op2 == 106 || (!isGCN14 && (op2 == 108 || op2 == 110)) || op2 == 126 ||
        (op2 == 104 && (arch&ARCH_RX2X0)!=0) ||
        ((op2 == 102 || op2 == 104) && isGCN12))
    {
        // if not SGPR, but other scalar registers
        switch(op2)
        {
            case 102:
                putChars(bufPtr, "flat_scratch", 12);
                break;
            case 104:
                if (isGCN12) // GCN1.2
                    putChars(bufPtr, "xnack_mask", 10);
                else // GCN1.1
                    putChars(bufPtr, "flat_scratch", 12);
                break;
            case 106:
                // vcc
                putChars(bufPtr, "vcc", 3);
                break;
            case 108:
                // tba
                putChars(bufPtr, "tba", 3);
                break;
            case 110:
                // tma
                putChars(bufPtr, "tma", 3);
                break;
            case 126:
                putChars(bufPtr, "exec", 4);
                break;
        }
        // if full 64-bit register
        if (regNum >= 2)
        {
            if (op&1) // unaligned!!
                putChars(bufPtr, "_u!", 3);
            if (regNum > 2)
                putChars(bufPtr, "&ill!", 5);
            return;
        }
        // suffix _lo or _hi (like vcc_lo or vcc_hi)
        *bufPtr++ = '_';
        if ((op&1) == 0)
        { *bufPtr++ = 'l'; *bufPtr++ = 'o'; }
        else
        { *bufPtr++ = 'h'; *bufPtr++ = 'i'; }
        return;
    }
    
    if (op == 255) // if literal
    {
        putChars(bufPtr, "0x0", 3); // zero 
        return;
    }
    
    cxuint ttmpStart = (isGCN14 ? 108 : 112);
    if (op >= ttmpStart && op < 124)
    {
        // print ttmp register
        putChars(bufPtr, "ttmp", 4);
        regRanges(op-ttmpStart, regNum, bufPtr);
        return;
    }
    if (op == 124)
    {
        // M0 register
        *bufPtr++ = 'm';
        *bufPtr++ = '0';
        if (regNum > 1)
            putChars(bufPtr, "&ill!", 5);
        return;
    }
    
    if (op >= 128 && op <= 192)
    {
        // nonnegative integer constant
        op -= 128;
        const cxuint digit1 = op/10U;
        if (digit1 != 0)
            *bufPtr++ = digit1 + '0';
        *bufPtr++ = op-digit1*10U + '0';
        return;
    }
    if (op > 192 && op <= 208)
    {
        // negative integer constant
        *bufPtr++ = '-';
        if (op >= 202)
        {
            *bufPtr++ = '1';
            *bufPtr++ = op-202+'0';
        }
        else
            *bufPtr++ = op-192+'0';
        return;
    }
    
    Flags disasmFlags = dasm.disassembler.getFlags();
    if (op >= 240 && op < 248)
    {
        const char* inOp = gcnOperandFloatTable[op-240];
        putChars(bufPtr, inOp, ::strlen(inOp));
        if ((disasmFlags & DISASM_BUGGYFPLIT)!=0 && floatLit==FLTLIT_F16)
            *bufPtr++ = 's';    /* old rules of assembler */
        return;
    }
    
    if (isGCN14)
        // special registers in GCN1.4 (VEGA)
        switch(op)
        {
            case 0xeb:
                putChars(bufPtr, "shared_base", 11);
                return;
            case 0xec:
                putChars(bufPtr, "shared_limit", 12);
                return;
            case 0xed:
                putChars(bufPtr, "private_base", 12);
                return;
            case 0xee:
                putChars(bufPtr, "private_limit", 13);
                return;
            case 0xef:
                putChars(bufPtr, "pops_exiting_wave_id", 20);
                return;
        }
    
    switch(op)
    {
        case 248:
            if (isGCN12)
            {
                // 1/(2*PI)
                putChars(bufPtr, "0.15915494", 10);
                if ((disasmFlags & DISASM_BUGGYFPLIT)!=0 && floatLit==FLTLIT_F16)
                    *bufPtr++ = 's';    /* old rules of assembler */
                return;
            }
            break;
        case 251:
            putChars(bufPtr, "vccz", 4);
            return;
        case 252:
            putChars(bufPtr, "execz", 5);
            return;
        case 253:
            // scc
            putChars(bufPtr, "scc", 3);
            return;
        case 254:
            // lds
            putChars(bufPtr, "lds", 3);
            return;
    }
    
    // reserved value (illegal)
    putChars(bufPtr, "ill_", 4);
    *bufPtr++ = '0'+op/100U;
    *bufPtr++ = '0'+(op/10U)%10U;
    *bufPtr++ = '0'+op%10U;
}

char* GCNDisasmUtils::decodeGCNOperand(GCNDisassembler& dasm, size_t codePos,
              RelocIter& relocIter, cxuint op, cxuint regNum, GPUArchMask arch,
              uint32_t literal, FloatLitType floatLit)
{
    FastOutputBuffer& output = dasm.output;
    if (op == 255)
    {
        // if literal
        printLiteral(dasm, codePos, relocIter, literal, floatLit, true);
        return output.reserve(100);
    }
    char* bufStart = output.reserve(50);
    char* bufPtr = bufStart;
    decodeGCNOperandNoLit(dasm, op, regNum, bufPtr, arch, floatLit);
    output.forward(bufPtr-bufStart);
    return output.reserve(100);
}

// table of values sendmsg
static const char* sendMsgCodeMessageTable[16] =
{
    "@0",
    "interrupt",
    "gs",
    "gs_done",
    "@4", "@5", "@6", "@7", "@8", "@9", "@10", "@11", "@12", "@13", "@14", "system"
};

// table of values sendmsg (GCN 1.4 VEGA)
static const char* sendMsgCodeMessageTableVEGA[16] =
{
    "@0",
    "interrupt",
    "gs",
    "gs_done",
    "savewave",
    "stall_wave_gen",
    "halt_waves",
    "ordered_ps_done",
    "early_prim_dealloc",
    "gs_alloc_req",
    "get_doorbell",
    "@11", "@12", "@13", "@14", "system"
};


static const char* sendGsOpMessageTable[4] =
{ "nop", "cut", "emit", "emit-cut" };

/* encoding routines */

// add N spaces
static void addSpaces(char*& bufPtr, cxuint spacesToAdd)
{
    for (cxuint k = spacesToAdd; k>0; k--)
        *bufPtr++ = ' ';
}

void GCNDisasmUtils::decodeSOPCEncoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(90);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    output.forward(bufPtr-bufStart);
    
    // form: INSTR SRC0, SRC1
    bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, insnCode&0xff,
                     (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal);
    putCommaSpace(bufPtr);
    if ((gcnInsn.mode & GCN_SRC1_IMM) != 0)
    {
        // if immediate in SRC1
        putHexByteToBuf((insnCode>>8)&0xff, bufPtr);
        output.forward(bufPtr-bufStart);
    }
    else
    {
        output.forward(bufPtr-bufStart);
        decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>8)&0xff,
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, arch, literal);
    }
}

/// about label writer - label is workaround for class hermetization
void GCNDisasmUtils::decodeSOPPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
         GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t literal, size_t pos)
{
    const bool isGCN14 = ((arch&ARCH_GCN_1_4)!=0);
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(70);
    char* bufPtr = bufStart;
    const cxuint imm16 = insnCode&0xffff;
    switch(gcnInsn.mode&GCN_MASK1)
    {
        case GCN_IMM_REL:
        {
            // print relative address as label
            const size_t branchPos = dasm.startOffset + ((pos + int16_t(imm16))<<2);
            addSpaces(bufPtr, spacesToAdd);
            output.forward(bufPtr-bufStart);
            dasm.writeLocation(branchPos);
            return;
        }
        case GCN_IMM_LOCKS:
        {
            bool prevLock = false;
            addSpaces(bufPtr, spacesToAdd);
            const bool isf7f = (!isGCN14 && imm16==0xf7f) ||
                    (isGCN14 && imm16==0xcf7f);
            // print vmcnt only if not highest value and if 0x[c]f7f value
            if ((!isGCN14 && (imm16&15) != 15) ||
                (isGCN14 && (imm16&0xc00f) != 0xc00f) || isf7f)
            {
                const cxuint lockCnt = isGCN14 ?
                        ((imm16>>10)&0x30) + (imm16&15) : imm16&15;
                putChars(bufPtr, "vmcnt(", 6);
                // print value of lockCnt
                const cxuint digit2 = lockCnt/10U;
                if (lockCnt >= 10)
                    *bufPtr++ = '0'+digit2;
                *bufPtr++= '0' + lockCnt-digit2*10U;
                *bufPtr++ = ')';
                prevLock = true;
            }
            // print only if expcnt have not highest value (7)
            if (((imm16>>4)&7) != 7 || isf7f)
            {
                if (prevLock)
                    // print & before previous lock: vmcnt()
                    putChars(bufPtr, " & ", 3);
                putChars(bufPtr, "expcnt(", 7);
                // print value
                *bufPtr++ = '0' + ((imm16>>4)&7);
                *bufPtr++ = ')';
                prevLock = true;
            }
            // print only if lgkmcnt have not highest value (15)
            if (((imm16>>8)&15) != 15 || isf7f)
            {
                /* LGKMCNT bits is 4 (5????) */
                const cxuint lockCnt = (imm16>>8)&15;
                if (prevLock)
                    // print & before previous lock: vmcnt()
                    putChars(bufPtr, " & ", 3);
                putChars(bufPtr, "lgkmcnt(", 8);
                const cxuint digit2 = lockCnt/10U;
                if (lockCnt >= 10)
                    *bufPtr++ = '0'+digit2;
                *bufPtr++= '0' + lockCnt-digit2*10U;
                *bufPtr++ = ')';
                prevLock = true;
            }
            if ((!isGCN14 && (imm16&0xf080) != 0) || (isGCN14 && (imm16&0x3080) != 0))
            {
                /* additional info about imm16 */
                if (prevLock)
                    putChars(bufPtr, " :", 2);
                bufPtr += itocstrCStyle(imm16, bufPtr, 11, 16);
            }
            break;
        }
        case GCN_IMM_MSGS:
        {
            cxuint illMask = 0xfff0;
            addSpaces(bufPtr, spacesToAdd);
            
            // print sendmsg
            putChars(bufPtr, "sendmsg(", 8);
            const cxuint msgType = imm16&15;
            const char* msgName = (isGCN14 ? sendMsgCodeMessageTableVEGA[msgType] :
                    sendMsgCodeMessageTable[msgType]);
            cxuint minUnknownMsgType = isGCN14 ? 11 : 4;
            if ((arch & ARCH_RX3X0) != 0 && msgType == 4)
            {
                msgName = "savewave"; // 4 - savewave
                minUnknownMsgType = 5;
            }
            // put message name to buffer
            while (*msgName != 0)
                *bufPtr++ = *msgName++;
            
            // if also some arguments supplied (gsops)
            if ((msgType&14) == 2 || (msgType >= minUnknownMsgType && msgType <= 14) ||
                (imm16&0x3f0) != 0) // gs ops
            {
                putCommaSpace(bufPtr);
                illMask = 0xfcc0; // set new illegal mask of bits
                const cxuint gsopId = (imm16>>4)&3;
                const char* gsopName = sendGsOpMessageTable[gsopId];
                // put gsop name to buffer
                while (*gsopName != 0)
                    *bufPtr++ = *gsopName++;
                if (gsopId!=0 || ((imm16>>8)&3)!=0)
                {
                    putCommaSpace(bufPtr);
                    // print gsop value
                    *bufPtr++ = '0' + ((imm16>>8)&3);
                }
            }
            *bufPtr++ = ')';
            // if some bits is not zero (illegal value)
            if ((imm16&illMask) != 0)
            {
                *bufPtr++ = ' ';
                *bufPtr++ = ':';
                bufPtr += itocstrCStyle(imm16, bufPtr, 11, 16);
            }
            break;
        }
        case GCN_IMM_NONE:
            if (imm16 != 0)
            {
                addSpaces(bufPtr, spacesToAdd);
                bufPtr += itocstrCStyle(imm16, bufPtr, 11, 16);
            }
            break;
        default:
            addSpaces(bufPtr, spacesToAdd);
            bufPtr += itocstrCStyle(imm16, bufPtr, 11, 16);
            break;
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeSOP1Encoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(80);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    bool isDst = (gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE;
    // print destination if instruction have it
    if (isDst)
    {
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>16)&0x7f,
                         (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
    }
    
    if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
    {
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
            // put ',' if destination and source
            putCommaSpace(bufPtr);
        // put SRC1
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, insnCode&0xff,
                         (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal);
    }
    else if ((insnCode&0xff) != 0)
    {
        // print value, if some are not used, but values is not default
        putChars(bufPtr," ssrc=", 6);
        bufPtr += itocstrCStyle((insnCode&0xff), bufPtr, 6, 16);
    }
    // print value, if some are not used, but values is not default
    if (!isDst && ((insnCode>>16)&0x7f) != 0)
    {
        putChars(bufPtr, " sdst=", 6);
        bufPtr += itocstrCStyle(((insnCode>>16)&0x7f), bufPtr, 6, 16);
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeSOP2Encoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(90);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
    {
        // print destination
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>16)&0x7f,
                         (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
        putCommaSpace(bufPtr);
    }
    // print SRC0
    output.forward(bufPtr-bufStart);
    bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, insnCode&0xff,
                     (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal);
    putCommaSpace(bufPtr);
    // print SRC1
    output.forward(bufPtr-bufStart);
    bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>8)&0xff,
                 (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, arch, literal);
    
    // print value, if some are not used, but values is not default
    if ((gcnInsn.mode & GCN_MASK1) == GCN_DST_NONE && ((insnCode>>16)&0x7f) != 0)
    {
        putChars(bufPtr, " sdst=", 6);
        bufPtr += itocstrCStyle(((insnCode>>16)&0x7f), bufPtr, 6, 16);
    }
    output.forward(bufPtr-bufStart);
}

// table hwreg names
static const char* hwregNames[20] =
{
    "@0", "mode", "status", "trapsts",
    "hw_id", "gpr_alloc", "lds_alloc", "ib_sts",
    "pc_lo", "pc_hi", "inst_dw0", "inst_dw1",
    "ib_dbg0", "ib_dbg1", "flush_ib", "sh_mem_bases",
    "sq_shader_tba_lo", "sq_shader_tba_hi",
    "sq_shader_tma_lo", "sq_shader_tma_hi"
};

/// about label writer - label is workaround for class hermetization
void GCNDisasmUtils::decodeSOPKEncoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(90);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    if ((gcnInsn.mode & GCN_IMM_DST) == 0)
    {
        // if normal destination
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>16)&0x7f,
                         (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
        putCommaSpace(bufPtr);
    }
    const cxuint imm16 = insnCode&0xffff;
    if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL)
    {
        // print relative address (as label)
        const size_t branchPos = dasm.startOffset + ((codePos + int16_t(imm16))<<2);
        output.forward(bufPtr-bufStart);
        dasm.writeLocation(branchPos);
        bufPtr = bufStart = output.reserve(60);
    }
    else if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_SREG)
    {
        // print hwreg()
        putChars(bufPtr, "hwreg(", 6);
        const cxuint hwregId = imm16&0x3f;
        cxuint hwregNamesNum = 13 + ((arch&ARCH_GCN_1_2_4)!=0);
        if ((arch&ARCH_GCN_1_4) != 0)
            hwregNamesNum = 20;
        if (hwregId < hwregNamesNum)
            putChars(bufPtr, hwregNames[hwregId], ::strlen(hwregNames[hwregId]));
        else
        {
            // parametrized hwreg: hwreg(@0
            const cxuint digit2 = hwregId/10U;
            *bufPtr++ = '@';
            *bufPtr++ = '0' + digit2;
            *bufPtr++ = '0' + hwregId - digit2*10U;
        }
        putCommaSpace(bufPtr);
        // start bit
        putByteToBuf((imm16>>6)&31, bufPtr);
        putCommaSpace(bufPtr);
        // size in bits
        putByteToBuf(((imm16>>11)&31)+1, bufPtr);
        *bufPtr++ = ')';
    }
    else
        bufPtr += itocstrCStyle(imm16, bufPtr, 11, 16);
    
    if (gcnInsn.mode & GCN_IMM_DST)
    {
        putCommaSpace(bufPtr);
        // print value, if some are not used, but values is not default
        if (gcnInsn.mode & GCN_SOPK_CONST)
        {
            // for S_SETREG_IMM32_B32
            bufPtr += itocstrCStyle(literal, bufPtr, 11, 16);
            if (((insnCode>>16)&0x7f) != 0)
            {
                putChars(bufPtr, " sdst=", 6);
                bufPtr += itocstrCStyle((insnCode>>16)&0x7f, bufPtr, 6, 16);
            }
        }
        else
        {
            // for s_setreg_b32, print destination as source
            output.forward(bufPtr-bufStart);
            bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter,
                     (insnCode>>16)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
        }
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeSMRDEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(100);
    char* bufPtr = bufStart;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    
    bool spacesAdded = false;
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        // print only destination
        addSpaces(bufPtr, spacesToAdd);
        decodeGCNOperandNoLit(dasm, (insnCode>>15)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1,
                         bufPtr, arch);
        useDst = true;
        spacesAdded = true;
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        addSpaces(bufPtr, spacesToAdd);
        // print destination (1,2,4,8 or 16 registers)
        decodeGCNOperandNoLit(dasm, (insnCode>>15)&0x7f, dregsNum, bufPtr, arch);
        putCommaSpace(bufPtr);
        // print SBASE (base address registers) (address or resource)
        decodeGCNOperandNoLit(dasm, (insnCode>>8)&0x7e, (gcnInsn.mode&GCN_SBASE4)?4:2,
                          bufPtr, arch);
        putCommaSpace(bufPtr);
        if (insnCode&0x100) // immediate value
            bufPtr += itocstrCStyle(insnCode&0xff, bufPtr, 11, 16);
        else // S register
            decodeGCNOperandNoLit(dasm, insnCode&0xff, 1, bufPtr, arch);
        // set what is printed
        useDst = true;
        useOthers = true;
        spacesAdded = true;
    }
    
    // print value, if some are not used, but values is not default
    if (!useDst && (insnCode & 0x3f8000U) != 0)
    {
        addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " sdst=", 6);
        bufPtr += itocstrCStyle((insnCode>>15)&0x7f, bufPtr, 6, 16);
    }
    if (!useOthers && (insnCode & 0x7e00U)!=0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " sbase=", 7);
        bufPtr += itocstrCStyle((insnCode>>9)&0x3f, bufPtr, 6, 16);
    }
    if (!useOthers && (insnCode & 0xffU)!=0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " offset=", 8);
        bufPtr += itocstrCStyle(insnCode&0xff, bufPtr, 6, 16);
    }
    if (!useOthers && (insnCode & 0x100U)!=0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " imm=1", 6);
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeSMEMEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
         GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(120);
    char* bufPtr = bufStart;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    bool spacesAdded = false;
    const bool isGCN14 = ((arch&ARCH_GCN_1_4) != 0);
    bool printOffset = false;
    
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        // print only destination
        addSpaces(bufPtr, spacesToAdd);
        decodeGCNOperandNoLit(dasm, (insnCode>>6)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1,
                         bufPtr, arch);
        useDst = true;
        spacesAdded = true;
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        addSpaces(bufPtr, spacesToAdd);
        if (!(mode1 & GCN_SMEM_NOSDATA)) {
            if (mode1 & GCN_SMEM_SDATA_IMM)
                // print immediate value
                putHexByteToBuf((insnCode>>6)&0x7f, bufPtr);
            else
                // print destination (1,2,4,8 or 16 registers)
                decodeGCNOperandNoLit(dasm, (insnCode>>6)&0x7f, dregsNum, bufPtr , arch);
            putCommaSpace(bufPtr);
            useDst = true;
        }
        // print SBASE (base address registers) (address or resource)
        decodeGCNOperandNoLit(dasm, (insnCode<<1)&0x7e, (gcnInsn.mode&GCN_SBASE4)?4:2,
                          bufPtr, arch);
        putCommaSpace(bufPtr);
        if (insnCode&0x20000) // immediate value
        {
            if (isGCN14 && (insnCode & 0x4000) != 0)
            {
                // last 8-bit in second dword
                decodeGCNOperandNoLit(dasm, (insnCode2>>25), 1, bufPtr , arch);
                printOffset = true;
            }
            else
            {
                // print SOFFSET
                uint32_t immMask =  isGCN14 ? 0x1fffff : 0xfffff;
                bufPtr += itocstrCStyle(insnCode2 & immMask, bufPtr, 11, 16);
            }
        }
        else // SOFFSET register
        {
            if (isGCN14 && (insnCode & 0x4000) != 0)
                decodeGCNOperandNoLit(dasm, insnCode2>>25, 1, bufPtr, arch);
            else
                decodeGCNOperandNoLit(dasm, insnCode2&0xff, 1, bufPtr, arch);
        }
        useOthers = true;
        spacesAdded = true;
    }
    
    if ((insnCode & 0x10000) != 0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        // print GLC modifier
        putChars(bufPtr, " glc", 4);
    }
    
    if (isGCN14 && (insnCode & 0x8000) != 0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        // print NV modifier
        putChars(bufPtr, " nv", 3);
    }
    
    if (printOffset)
    {
        // GCN 1.4 extra OFFSET
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " offset:", 8);
        bufPtr += itocstrCStyle(insnCode2 & 0x1fffff, bufPtr, 11, 16);
    }
    
    // print value, if some are not used, but values is not default
    if (!useDst && (insnCode & 0x1fc0U) != 0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " sdata=", 7);
        bufPtr += itocstrCStyle((insnCode>>6)&0x7f, bufPtr, 6, 16);
    }
    if (!useOthers && (insnCode & 0x3fU)!=0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " sbase=", 7);
        bufPtr += itocstrCStyle(insnCode&0x3f, bufPtr, 6, 16);
    }
    if (!useOthers && insnCode2!=0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " offset=", 8);
        bufPtr += itocstrCStyle(insnCode2, bufPtr, 12, 16);
    }
    if (!useOthers && (insnCode & 0x20000U)!=0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " imm=1", 6);
    }
    
    output.forward(bufPtr-bufStart);
}

// temporary structure to store operand modifiers and operand SRC0
struct CLRX_INTERNAL VOPExtraWordOut
{
    uint16_t src0;
    bool sextSrc0;
    bool negSrc0;
    bool absSrc0;
    bool sextSrc1;
    bool negSrc1;
    bool absSrc1;
    bool scalarSrc1;
};

// SDWA SEL field value names
static const char* sdwaSelChoicesTbl[] =
{
    "byte0", "byte1", "byte2", "byte3", "word0", "word1", nullptr, "invalid"
};

// SDWA UNUSED field value names
static const char* sdwaDstUnusedTbl[] =
{
    nullptr, "sext", "preserve", "invalid"
};

/* returns mask of abs,neg,sext for src0 and src1 argument and src0 register */
static inline VOPExtraWordOut decodeVOPSDWAFlags(uint32_t insnCode2, GPUArchMask arch)
{
    const bool isGCN14 = (arch & ARCH_GCN_1_4)!=0;
    return { uint16_t((insnCode2&0xff) +
        ((!isGCN14 || (insnCode2 & (1U<<23))==0) ? 256 : 0)),
        (insnCode2&(1U<<19))!=0, (insnCode2&(1U<<20))!=0, (insnCode2&(1U<<21))!=0,
        (insnCode2&(1U<<27))!=0, (insnCode2&(1U<<28))!=0, (insnCode2&(1U<<29))!=0,
        isGCN14 && ((insnCode2&(1U<<31))!=0) };
}

// decode and print VOP SDWA encoding
static void decodeVOPSDWA(FastOutputBuffer& output, GPUArchMask arch, uint32_t insnCode2,
          bool src0Used, bool src1Used, bool vopc = false)
{
    char* bufStart = output.reserve(100);
    char* bufPtr = bufStart;
    const bool isGCN14 = ((arch&ARCH_GCN_1_4) != 0);
    cxuint dstSel = 6;
    cxuint dstUnused = 0;
    if (!isGCN14 || !vopc)
    {
        // not VEGA or not VOPC
        dstSel = (insnCode2>>8)&7;
        dstUnused = (insnCode2>>11)&3;
    }
    const cxuint src0Sel = (insnCode2>>16)&7;
    const cxuint src1Sel = (insnCode2>>24)&7;
    
    if (!isGCN14 || !vopc)
    {
        // not VEGA or not VOPC
        if (isGCN14 && (insnCode2 & 0xc000U) != 0)
        {
            cxuint omod = (insnCode2>>14)&3;
            const char* omodStr = (omod==3)?" div:2":(omod==2)?" mul:4":" mul:2";
            putChars(bufPtr, omodStr, 6);
        }
        if (insnCode2 & 0x2000)
            putChars(bufPtr, " clamp", 6);
        
        // print dst_sel:XXXX
        if (dstSel != 6)
        {
            putChars(bufPtr, " dst_sel:", 9);
            putChars(bufPtr, sdwaSelChoicesTbl[dstSel],
                    ::strlen(sdwaSelChoicesTbl[dstSel]));
        }
        // print dst_unused:XXX
        if (dstUnused!=0)
        {
            putChars(bufPtr, " dst_unused:", 12);
            putChars(bufPtr, sdwaDstUnusedTbl[dstUnused],
                    ::strlen(sdwaDstUnusedTbl[dstUnused]));
        }
    }
    // print src0_sel and src1_sel if used
    if (src0Sel!=6 && src0Used)
    {
        putChars(bufPtr, " src0_sel:", 10);
        putChars(bufPtr, sdwaSelChoicesTbl[src0Sel],
                 ::strlen(sdwaSelChoicesTbl[src0Sel]));
    }
    if (src1Sel!=6 && src1Used)
    {
        putChars(bufPtr, " src1_sel:", 10);
        putChars(bufPtr, sdwaSelChoicesTbl[src1Sel],
                 ::strlen(sdwaSelChoicesTbl[src1Sel]));
    }
    // unused but fields
    if (!src0Used)
    {
        if ((insnCode2&(1U<<19))!=0)
            putChars(bufPtr, " sext0", 6);
        if ((insnCode2&(1U<<20))!=0)
            putChars(bufPtr, " neg0", 5);
        if ((insnCode2&(1U<<21))!=0)
            putChars(bufPtr, " abs0", 5);
    }
    if (!src1Used)
    {
        if ((insnCode2&(1U<<27))!=0)
            putChars(bufPtr, " sext1", 6);
        if ((insnCode2&(1U<<28))!=0)
            putChars(bufPtr, " neg1", 5);
        if ((insnCode2&(1U<<29))!=0)
            putChars(bufPtr, " abs1", 5);
    }
    
    // add SDWA encoding specifier at end if needed (to avoid ambiguity)
    if (((isGCN14 && vopc) || (dstSel==6 && dstUnused==0)) &&
        src0Sel==6 && (insnCode2&(1U<<19))==0 && src1Sel==6 && (insnCode2&(1U<<27))==0)
        putChars(bufPtr, " sdwa", 5);
    
    output.forward(bufPtr-bufStart);
}

/// DPP CTRL value table (only 
static const char* dppCtrl130Tbl[] =
{
    " wave_shl", nullptr, nullptr, nullptr,
    " wave_rol", nullptr, nullptr, nullptr,
    " wave_shr", nullptr, nullptr, nullptr,
    " wave_ror", nullptr, nullptr, nullptr,
    " row_mirror", " row_half_mirror", " row_bcast15", " row_bcast31"
};

/* returns mask of abs,neg,sext for src0 and src1 argument and src0 register */
static inline VOPExtraWordOut decodeVOPDPPFlags(uint32_t insnCode2)
{
    return { uint16_t((insnCode2&0xff)+256),
        false, (insnCode2&(1U<<20))!=0, (insnCode2&(1U<<21))!=0,
        false, (insnCode2&(1U<<22))!=0, (insnCode2&(1U<<23))!=0, false };
}

static void decodeVOPDPP(FastOutputBuffer& output, uint32_t insnCode2,
        bool src0Used, bool src1Used)
{
    char* bufStart = output.reserve(110);
    char* bufPtr = bufStart;
    const cxuint dppCtrl = (insnCode2>>8)&0x1ff;
    
    if (dppCtrl<256)
    {
        // print quadperm: quad_perm[A:B:C:D] A,B,C,D - 2-bit values
        putChars(bufPtr, " quad_perm:[", 12);
        *bufPtr++ = '0' + (dppCtrl&3);
        *bufPtr++ = ',';
        *bufPtr++ = '0' + ((dppCtrl>>2)&3);
        *bufPtr++ = ',';
        *bufPtr++ = '0' + ((dppCtrl>>4)&3);
        *bufPtr++ = ',';
        *bufPtr++ = '0' + ((dppCtrl>>6)&3);
        *bufPtr++ = ']';
    }
    else if ((dppCtrl >= 0x101 && dppCtrl <= 0x12f) && ((dppCtrl&0xf) != 0))
    {
        // row_shl, row_shr or row_ror
        if ((dppCtrl&0xf0) == 0)
            putChars(bufPtr, " row_shl:", 9);
        else if ((dppCtrl&0xf0) == 16)
            putChars(bufPtr, " row_shr:", 9);
        else
            putChars(bufPtr, " row_ror:", 9);
        // print shift
        putByteToBuf(dppCtrl&0xf, bufPtr);
    }
    else if (dppCtrl >= 0x130 && dppCtrl <= 0x143 && dppCtrl130Tbl[dppCtrl-0x130]!=nullptr)
        // print other dpp modifier
        putChars(bufPtr, dppCtrl130Tbl[dppCtrl-0x130],
                 ::strlen(dppCtrl130Tbl[dppCtrl-0x130]));
    // otherwise print value of dppctrl (illegal value)
    else if (dppCtrl != 0x100)
    {
        putChars(bufPtr, " dppctrl:", 9);
        bufPtr += itocstrCStyle(dppCtrl, bufPtr, 10, 16);
    }
    
    if (insnCode2 & (0x80000U)) // bound ctrl
        putChars(bufPtr, " bound_ctrl", 11);
    
    // print bank_mask and row_mask
    putChars(bufPtr, " bank_mask:", 11);
    putByteToBuf((insnCode2>>24)&0xf, bufPtr);
    putChars(bufPtr, " row_mask:", 10);
    putByteToBuf((insnCode2>>28)&0xf, bufPtr);
    // unused but fields
    if (!src0Used)
    {
        if ((insnCode2&(1U<<20))!=0)
            putChars(bufPtr, " neg0", 5);
        if ((insnCode2&(1U<<21))!=0)
            putChars(bufPtr, " abs0", 5);
    }
    if (!src1Used)
    {
        if ((insnCode2&(1U<<22))!=0)
            putChars(bufPtr, " neg1", 5);
        if ((insnCode2&(1U<<23))!=0)
            putChars(bufPtr, " abs1", 5);
    }
    
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeVOPCEncoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    const bool isGCN12 = ((arch&ARCH_GCN_1_2_4)!=0);
    char* bufStart = output.reserve(120);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    
    const cxuint src0Field = (insnCode&0x1ff);
    // extra flags are zeroed by default
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    if ((arch & ARCH_GCN_1_4) != 0 && src0Field==0xf9 && (literal & 0x8000) != 0)
    {
        // SDWAB replacement of SDST
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter,
                            (literal>>8)&0x7f, 2, arch);
        putCommaSpace(bufPtr);
    }
    else // just vcc
        putChars(bufPtr, "vcc, ", 5);
    
    if (isGCN12)
    {
        // return VOP SDWA/DPP flags for operands
        if (src0Field == 0xf9)
            extraFlags = decodeVOPSDWAFlags(literal, arch);
        else if (src0Field == 0xfa)
            extraFlags = decodeVOPDPPFlags(literal);
        else
            extraFlags.src0 = src0Field;
    }
    else
        extraFlags.src0 = src0Field;
    
    // apply sext(), negation and abs() if applied
    if (extraFlags.sextSrc0)
        putChars(bufPtr, "sext(", 5);
    if (extraFlags.negSrc0)
        *bufPtr++ = '-';
    if (extraFlags.absSrc0)
        putChars(bufPtr, "abs(", 4);
    
    output.forward(bufPtr-bufStart);
    // print SRC0
    bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter, extraFlags.src0,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal, displayFloatLits);
    // closing for abs and sext
    if (extraFlags.absSrc0)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc0)
        *bufPtr++ = ')';
    putCommaSpace(bufPtr);
    
    // apply sext(), negation and abs() if applied
    if (extraFlags.sextSrc1)
        putChars(bufPtr, "sext(", 5);
    
    if (extraFlags.negSrc1)
        *bufPtr++ = '-';
    if (extraFlags.absSrc1)
        putChars(bufPtr, "abs(", 4);
    
    // print SRC1
    decodeGCNOperandNoLit(dasm, ((insnCode>>9)&0xff) + (extraFlags.scalarSrc1 ? 0 : 256),
                (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, bufPtr, arch);
    // closing for abs and sext
    if (extraFlags.absSrc1)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc1)
        *bufPtr++ = ')';
    
    output.forward(bufPtr-bufStart);
    if (isGCN12)
    {
        // print extra SDWA/DPP modifiers
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, arch, literal, true, true, true);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, true, true);
    }
}

void GCNDisasmUtils::decodeVOP1Encoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    const bool isGCN12 = ((arch&ARCH_GCN_1_2_4)!=0);
    char* bufStart = output.reserve(130);
    char* bufPtr = bufStart;
    
    const cxuint src0Field = (insnCode&0x1ff);
    // extra flags are zeroed by default
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    
    if (isGCN12)
    {
        // return extra flags from SDWA/DPP encoding
        if (src0Field == 0xf9)
            extraFlags = decodeVOPSDWAFlags(literal, arch);
        else if (src0Field == 0xfa)
            extraFlags = decodeVOPDPPFlags(literal);
        else
            extraFlags.src0 = src0Field;
    }
    else
        extraFlags.src0 = src0Field;
    
    bool argsUsed = true;
    if ((gcnInsn.mode & GCN_MASK1) != GCN_VOP_ARG_NONE)
    {
        addSpaces(bufPtr, spacesToAdd);
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_SGPR)
            // print DST as SGPR
            decodeGCNVRegOperand(((insnCode>>17)&0xff),
                     (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr);
        else
            // print DST as normal VGPR
            decodeGCNOperandNoLit(dasm, ((insnCode>>17)&0xff),
                          (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr, arch);
        putCommaSpace(bufPtr);
        // apply sext, negation and abs if supplied
        if (extraFlags.sextSrc0)
            putChars(bufPtr, "sext(", 5);
        
        if (extraFlags.negSrc0)
            *bufPtr++ = '-';
        if (extraFlags.absSrc0)
            putChars(bufPtr, "abs(", 4);
        output.forward(bufPtr-bufStart);
        // print SRC0
        bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter, extraFlags.src0,
                     (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal, displayFloatLits);
        // closing sext and abs
        if (extraFlags.absSrc0)
            *bufPtr++ = ')';
        if (extraFlags.sextSrc0)
            *bufPtr++ = ')';
    }
    else if ((insnCode & 0x1fe01ffU) != 0)
    {
        // unused, but set fields
        addSpaces(bufPtr, spacesToAdd);
        if ((insnCode & 0x1fe0000U) != 0)
        {
            putChars(bufPtr, "vdst=", 5);
            bufPtr += itocstrCStyle((insnCode>>17)&0xff, bufPtr, 6, 16);
            if ((insnCode & 0x1ff) != 0)
                *bufPtr++ = ' ';
        }
        if ((insnCode & 0x1ff) != 0)
        {
            putChars(bufPtr, "src0=", 5);
            bufPtr += itocstrCStyle(insnCode&0x1ff, bufPtr, 6, 16);
        }
        argsUsed = false;
    }
    output.forward(bufPtr-bufStart);
    if (isGCN12)
    {
        // print extra SDWA/DPP modifiers
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, arch, literal, argsUsed, false);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, argsUsed, false);
    }
}

void GCNDisasmUtils::decodeVOP2Encoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    const bool isGCN12 = ((arch&ARCH_GCN_1_2_4)!=0);
    char* bufStart = output.reserve(150);
    char* bufPtr = bufStart;
    
    addSpaces(bufPtr, spacesToAdd);
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    
    const cxuint src0Field = (insnCode&0x1ff);
    // extra flags are zeroed by default
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    
    if (isGCN12)
    {
        // return extra flags from SDWA/DPP encoding
        if (src0Field == 0xf9)
            extraFlags = decodeVOPSDWAFlags(literal, arch);
        else if (src0Field == 0xfa)
            extraFlags = decodeVOPDPPFlags(literal);
        else
            extraFlags.src0 = src0Field;
    }
    else
        extraFlags.src0 = src0Field;
    
    if (mode1 != GCN_DS1_SGPR)
        // print DST as SGPR
        decodeGCNVRegOperand(((insnCode>>17)&0xff),
                     (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr);
    else
        decodeGCNOperandNoLit(dasm, ((insnCode>>17)&0xff),
                 (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr, arch);
    
    // add VCC if V_ADD_XXX or other instruction VCC as DST
    if (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC)
        putChars(bufPtr, ", vcc", 5);
    putCommaSpace(bufPtr);
    // apply sext, negation and abs if supplied
    if (extraFlags.sextSrc0)
        putChars(bufPtr, "sext(", 5);
    if (extraFlags.negSrc0)
        *bufPtr++ = '-';
    if (extraFlags.absSrc0)
        putChars(bufPtr, "abs(", 4);
    output.forward(bufPtr-bufStart);
    // print SRC0
    bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter, extraFlags.src0,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal, displayFloatLits);
    // closing sext and abs
    if (extraFlags.absSrc0)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc0)
        *bufPtr++ = ')';
    if (mode1 == GCN_ARG1_IMM)
    {
        // extra immediate (like V_MADMK_F32)
        putCommaSpace(bufPtr);
        output.forward(bufPtr-bufStart);
        printLiteral(dasm, codePos, relocIter, literal, displayFloatLits, false);
        bufStart = bufPtr = output.reserve(100);
    }
    putCommaSpace(bufPtr);
    // apply sext, negation and abs if supplied
    if (extraFlags.sextSrc1)
        putChars(bufPtr, "sext(", 5);
    
    if (extraFlags.negSrc1)
        *bufPtr++ = '-';
    if (extraFlags.absSrc1)
        putChars(bufPtr, "abs(", 4);
    // print SRC0
    if (mode1 == GCN_DS1_SGPR || mode1 == GCN_SRC1_SGPR)
    {
        output.forward(bufPtr-bufStart);
        bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter,
                 ((insnCode>>9)&0xff), (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, arch);
    }
    else
        decodeGCNOperandNoLit(dasm, ((insnCode>>9)&0xff) +
                (extraFlags.scalarSrc1 ? 0 : 256),
                (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, bufPtr, arch);
    // closing sext and abs
    if (extraFlags.absSrc1)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc1)
        *bufPtr++ = ')';
    if (mode1 == GCN_ARG2_IMM)
    {
        // extra immediate (like V_MADAK_F32)
        putCommaSpace(bufPtr);
        output.forward(bufPtr-bufStart);
        printLiteral(dasm, codePos, relocIter, literal, displayFloatLits, false);
    }
    else if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
    {
        // VCC like in V_CNDMASK_B32 or V_SUBB_B32
        putChars(bufPtr, ", vcc", 5);
        output.forward(bufPtr-bufStart);
    }
    else
        output.forward(bufPtr-bufStart);
    if (isGCN12)
    {
        // print extra SDWA/DPP modifiers
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, arch, literal, true, true);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, true, true);
    }
}

// VINTRP param names
static const char* vintrpParamsTbl[] =
{ "p10", "p20", "p0" };

static void decodeVINTRPParam(uint16_t p, char*& bufPtr)
{
    if (p >= 3)
    {
        putChars(bufPtr, "invalid_", 8);
        bufPtr += itocstrCStyle(p, bufPtr, 8);
    }
    else
        putChars(bufPtr, vintrpParamsTbl[p], ::strlen(vintrpParamsTbl[p]));
}

void GCNDisasmUtils::decodeVOP3Encoding(GCNDisassembler& dasm, cxuint spacesToAdd,
         GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t insnCode2, FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(170);
    char* bufPtr = bufStart;
    const bool isGCN12 = ((arch&ARCH_GCN_1_2_4)!=0);
    const bool isGCN14 = ((arch&ARCH_GCN_1_4)!=0);
    const cxuint opcode = (isGCN12) ? ((insnCode>>16)&0x3ff) : ((insnCode>>17)&0x1ff);
    
    const cxuint vdst = insnCode&0xff;
    const cxuint vsrc0 = insnCode2&0x1ff;
    const cxuint vsrc1 = (insnCode2>>9)&0x1ff;
    const cxuint vsrc2 = (insnCode2>>18)&0x1ff;
    const cxuint sdst = (insnCode>>8)&0x7f;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t vop3Mode = (gcnInsn.mode&GCN_VOP3_MASK2);
    
    bool vdstUsed = false;
    bool vsrc0Used = false;
    bool vsrc1Used = false;
    bool vsrc2Used = false;
    bool vsrc2CC = false;
    cxuint absFlags = 0;
    cxuint negFlags = 0;
    
    if (gcnInsn.encoding == GCNENC_VOP3A && vop3Mode != GCN_VOP3_VOP3P)
        absFlags = (insnCode>>8)&7;
    // get negation flags from insnCode (VOP3P)
    if (vop3Mode != GCN_VOP3_VOP3P)
        negFlags = (insnCode2>>29)&7;
    
    const bool is128Ops = (gcnInsn.mode&0x7000)==GCN_VOP3_DS2_128;
    
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        addSpaces(bufPtr, spacesToAdd);
        
        if (opcode < 256 || (gcnInsn.mode&GCN_VOP3_DST_SGPR)!=0)
            /* if compares (print DST as SDST) */
            decodeGCNOperandNoLit(dasm, vdst, ((gcnInsn.mode&GCN_VOP3_DST_SGPR)==0)?2:1,
                              bufPtr, arch);
        else /* regular instruction */
            // for V_MQSAD_U32 SRC2 is 128-bit
            decodeGCNVRegOperand(vdst, (is128Ops) ? 4 : 
                                 ((gcnInsn.mode&GCN_REG_DST_64)?2:1), bufPtr);
        
        if (gcnInsn.encoding == GCNENC_VOP3B &&
            (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC || mode1 == GCN_DST_VCC_VSRC2 ||
             mode1 == GCN_S0EQS12)) /* VOP3b */
        {
            // print SDST operand (VOP3B)
            putCommaSpace(bufPtr);
            decodeGCNOperandNoLit(dasm, ((insnCode>>8)&0x7f), 2, bufPtr, arch);
        }
        putCommaSpace(bufPtr);
        if (vop3Mode != GCN_VOP3_VINTRP)
        {
            // print negation or abs if supplied
            if (negFlags & 1)
                *bufPtr++ = '-';
            if (absFlags & 1)
                putChars(bufPtr, "abs(", 4);
            // print VSRC0
            decodeGCNOperandNoLit(dasm, vsrc0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                                   bufPtr, arch, displayFloatLits);
            // closing abs
            if (absFlags & 1)
                *bufPtr++ = ')';
            vsrc0Used = true;
        }
        
        if (vop3Mode == GCN_VOP3_VINTRP)
        {
            // print negation or abs if supplied
            if (negFlags & 2)
                *bufPtr++ = '-';
            if (absFlags & 2)
                putChars(bufPtr, "abs(", 4);
            if (mode1 == GCN_P0_P10_P20)
                // VINTRP param
                decodeVINTRPParam(vsrc1, bufPtr);
            else
                // print VSRC1
                decodeGCNOperandNoLit(dasm, vsrc1, 1, bufPtr, arch, displayFloatLits);
            if (absFlags & 2)
                *bufPtr++ = ')';
            // print VINTRP attr
            putChars(bufPtr, ", attr", 6);
            const cxuint attr = vsrc0&63;
            putByteToBuf(attr, bufPtr);
            *bufPtr++ = '.';
            *bufPtr++ = "xyzw"[((vsrc0>>6)&3)]; // attrchannel
            
            if ((gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2)
            {
                putCommaSpace(bufPtr);
                // print abs and negation for VSRC2
                if (negFlags & 4)
                    *bufPtr++ = '-';
                if (absFlags & 4)
                    putChars(bufPtr, "abs(", 4);
                // print VSRC2
                decodeGCNOperandNoLit(dasm, vsrc2, 1, bufPtr, arch, displayFloatLits);
                if (absFlags & 4)
                    *bufPtr++ = ')';
                vsrc2Used = true;
            }
            
            if (vsrc0 & 0x100)
                putChars(bufPtr, " high", 5);
            vsrc0Used = true;
            vsrc1Used = true;
        }
        else if (mode1 != GCN_SRC12_NONE)
        {
            putCommaSpace(bufPtr);
            // print abs and negation for VSRC1
            if (negFlags & 2)
                *bufPtr++ = '-';
            if (absFlags & 2)
                putChars(bufPtr, "abs(", 4);
            // print VSRC1
            decodeGCNOperandNoLit(dasm, vsrc1, (gcnInsn.mode&GCN_REG_SRC1_64)?2:1,
                      bufPtr, arch, displayFloatLits);
            if (absFlags & 2)
                *bufPtr++ = ')';
            /* GCN_DST_VCC - only sdst is used, no vsrc2 */
            if (mode1 != GCN_SRC2_NONE && mode1 != GCN_DST_VCC && opcode >= 256)
            {
                putCommaSpace(bufPtr);
                
                if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
                {
                    decodeGCNOperandNoLit(dasm, vsrc2, 2, bufPtr, arch);
                    vsrc2CC = true;
                }
                else
                {
                    // print abs and negation for VSRC2
                    if (negFlags & 4)
                        *bufPtr++ = '-';
                    if (absFlags & 4)
                        putChars(bufPtr, "abs(", 4);
                    // for V_MQSAD_U32 SRC2 is 128-bit
                    // print VSRC2
                    decodeGCNOperandNoLit(dasm, vsrc2, is128Ops ? 4 :
                                (gcnInsn.mode&GCN_REG_SRC2_64)?2:1,
                                 bufPtr, arch, displayFloatLits);
                    if (absFlags & 4)
                        *bufPtr++ = ')';
                }
                vsrc2Used = true;
            }
            vsrc1Used = true;
        }
        
        vdstUsed = true;
    }
    else
        addSpaces(bufPtr, spacesToAdd-1);
    
    uint32_t opselBaseMask = 0;
    uint32_t opselMask = 1;
    uint32_t opsel = 0;
    if (isGCN14 && gcnInsn.encoding != GCNENC_VOP3B)
    {
        opsel = (insnCode >> 11) & 15;
        // print OPSEL
        const bool opsel2Bit = (vop3Mode!=GCN_VOP3_VOP3P && vsrc1Used) ||
            (vop3Mode==GCN_VOP3_VOP3P && vsrc2Used);
        const bool opsel3Bit = (vsrc2Used && vop3Mode!=GCN_VOP3_VOP3P);
        if (vop3Mode!=GCN_VOP3_VOP3P)
        {
            opselMask |= (opsel2Bit?2:0) | (opsel3Bit?4:0) | 8;
            opselBaseMask = (15U<<11);
        }
        else // VOP3P
        {
            opselMask |= (opsel2Bit?6:2);
            opselBaseMask = (7U<<11);
        }
        
        if ((insnCode & (opselMask<<11)) != 0 &&
            ((insnCode & opselBaseMask) & ~(opselMask<<11)) == 0)
        {
            putChars(bufPtr, " op_sel:[", 9);
            *bufPtr++ = (insnCode&0x800) ? '1' : '0';
            *bufPtr++ = ',';
            if (vop3Mode==GCN_VOP3_VOP3P || vsrc1Used)
                *bufPtr++ = (insnCode&0x1000) ? '1' : '0';
            else
                // last bit 14-bit dest (for one operand instr)
                *bufPtr++ = (insnCode&0x4000) ? '1' : '0';
            // print next opsel if next operand is present
            // for VOP3P: VSRC2, non VOP3P - VSRC1
            if (vop3Mode!=GCN_VOP3_VOP3P && vsrc1Used)
            {
                *bufPtr++ = ',';
                if (vsrc2Used)
                    *bufPtr++ = (insnCode&0x2000) ? '1' : '0';
                else
                    // last bit 14-bit dest (no third source operand)
                    *bufPtr++ = (insnCode&0x4000) ? '1' : '0';
            }
            else if (vop3Mode==GCN_VOP3_VOP3P && vsrc2Used)
            {
                *bufPtr++ = ',';
                *bufPtr++ = (insnCode&0x2000) ? '1' : '0';
            }
            // only for VOP3P and VSRC2
            if (vsrc2Used && vop3Mode!=GCN_VOP3_VOP3P)
            {
                *bufPtr++ = ',';
                *bufPtr++ = (insnCode&0x4000) ? '1' : '0';
            }
            *bufPtr++ = ']';
        }
    }
    
    // fi VOP3P encoding
    if (vop3Mode==GCN_VOP3_VOP3P)
    {
        // print OP_SEL_HI modifier
        cxuint opselHi = ((insnCode2 >> 27) & 3) | (vsrc2Used ? ((insnCode>>12)&4) : 0);
        if (opselHi != 3+(vsrc2Used?4:0))
        {
            putChars(bufPtr, " op_sel_hi:[", 12);
            *bufPtr++ = (insnCode2 & (1U<<27)) ? '1' : '0';
            *bufPtr++ = ',';
            *bufPtr++ = (insnCode2 & (1U<<28)) ? '1' : '0';
            if (vsrc2Used)
            {
                *bufPtr++ = ',';
                *bufPtr++ = (insnCode& 0x4000) ? '1' : '0';
            }
            *bufPtr++ = ']';
        }
        // print NEG_LO modifier
        if ((insnCode2&((3+(vsrc2Used?4:0))<<29)) != 0)
        {
            putChars(bufPtr, " neg_lo:[", 9);
            *bufPtr++ = (insnCode2 & (1U<<29)) ? '1' : '0';
            *bufPtr++ = ',';
            *bufPtr++ = (insnCode2 & (1U<<30)) ? '1' : '0';
            if (vsrc2Used)
            {
                *bufPtr++ = ',';
                *bufPtr++ = (insnCode2 & (1U<<31)) ? '1' : '0';
            }
            *bufPtr++ = ']';
        }
        // print NEG_HI modifier
        if ((insnCode & ((3+(vsrc2Used?4:0))<<8)) != 0)
        {
            putChars(bufPtr, " neg_hi:[", 9);
            *bufPtr++ = (insnCode & (1U<<8)) ? '1' : '0';
            *bufPtr++ = ',';
            *bufPtr++ = (insnCode & (1U<<9)) ? '1' : '0';
            if (vsrc2Used)
            {
                *bufPtr++ = ',';
                *bufPtr++ = (insnCode & (1U<<10)) ? '1' : '0';
            }
            *bufPtr++ = ']';
        }
    }
    
    const cxuint omod = (insnCode2>>27)&3;
    if (vop3Mode != GCN_VOP3_VOP3P && omod != 0)
    {
        const char* omodStr = (omod==3)?" div:2":(omod==2)?" mul:4":" mul:2";
        putChars(bufPtr, omodStr, 6);
    }
    
    const bool clamp = (!isGCN12 && gcnInsn.encoding == GCNENC_VOP3A &&
            (insnCode&0x800) != 0) || (isGCN12 && (insnCode&0x8000) != 0);
    if (clamp)
        putChars(bufPtr, " clamp", 6);
    
    /* print unused values of parts if not values are not default */
    if (!vdstUsed && vdst != 0)
    {
        putChars(bufPtr, " dst=", 5);
        bufPtr += itocstrCStyle(vdst, bufPtr, 6, 16);
    }
    
    if (!vsrc0Used)
    {
        if (vsrc0 != 0)
        {
            putChars(bufPtr, " src0=", 6);
            bufPtr += itocstrCStyle(vsrc0, bufPtr, 6, 16);
        }
        if (absFlags & 1)
            putChars(bufPtr, " abs0", 5);
        if ((insnCode2 & (1U<<29)) != 0)
            putChars(bufPtr, " neg0", 5);
    }
    if (!vsrc1Used)
    {
        if (vsrc1 != 0)
        {
            putChars(bufPtr, " vsrc1=", 7);
            bufPtr += itocstrCStyle(vsrc1, bufPtr, 6, 16);
        }
        if (absFlags & 2)
            putChars(bufPtr, " abs1", 5);
        if ((insnCode2 & (1U<<30)) != 0)
            putChars(bufPtr, " neg1", 5);
    }
    if (!vsrc2Used)
    {
        if (vsrc2 != 0)
        {
            putChars(bufPtr, " vsrc2=", 7);
            bufPtr += itocstrCStyle(vsrc2, bufPtr, 6, 16);
        }
        if (absFlags & 4)
            putChars(bufPtr, " abs2", 5);
        if ((insnCode2 & (1U<<31)) != 0)
            putChars(bufPtr, " neg2", 5);
    }
    
    // unused op_sel field
    if (isGCN14 && ((insnCode & opselBaseMask) & ~(opselMask<<11)) != 0 &&
        gcnInsn.encoding != GCNENC_VOP3B)
    {
        putChars(bufPtr, " op_sel=", 8);
        bufPtr += itocstrCStyle((insnCode>>11)&15, bufPtr, 6, 16);
    }
    
    const cxuint usedMask = 7 & ~(vsrc2CC?4:0);
    /* check whether instruction is this same like VOP2/VOP1/VOPC */
    bool isVOP1Word = false; // if can be write in single VOP dword
    if (vop3Mode == GCN_VOP3_VINTRP)
    {
        if (mode1 != GCN_NEW_OPCODE) /* check clamp and abs flags */
            isVOP1Word = !((insnCode&(7<<8)) != 0 || (insnCode2&(7<<29)) != 0 ||
                    clamp || omod!=0 || ((vsrc1 < 256 && mode1!=GCN_P0_P10_P20) ||
                    (mode1==GCN_P0_P10_P20 && vsrc1 >= 256)) ||
                    vsrc0 >= 256 || vsrc2 != 0);
    }
    else if (mode1 != GCN_ARG1_IMM && mode1 != GCN_ARG2_IMM)
    {
        const bool reqForVOP1Word = omod==0 && ((insnCode2&(usedMask<<29)) == 0);
        if (gcnInsn.encoding != GCNENC_VOP3B)
        {
            /* for VOPC */
            if (opcode < 256 && vdst == 106 /* vcc */ && vsrc1 >= 256 && vsrc2 == 0)
                isVOP1Word = true;
            /* for VOP1 */
            else if (vop3Mode == GCN_VOP3_VOP1 && vsrc1 == 0 && vsrc2 == 0)
                isVOP1Word = true;
            /* for VOP2 */
            else if (vop3Mode == GCN_VOP3_VOP2 && ((!vsrc1Used && vsrc1 == 0) || 
                /* distinguish for v_read/writelane and other vop2 encoded as vop3 */
                (((gcnInsn.mode&GCN_VOP3_SRC1_SGPR)==0) && vsrc1 >= 256) ||
                (((gcnInsn.mode&GCN_VOP3_SRC1_SGPR)!=0) && vsrc1 < 256)) &&
                ((mode1 != GCN_SRC2_VCC && vsrc2 == 0) ||
                (vsrc2 == 106 && mode1 == GCN_SRC2_VCC)))
                isVOP1Word = true;
            /* check clamp and abs and neg flags */
            if ((insnCode&(usedMask<<8)) != 0 || (insnCode2&(usedMask<<29)) != 0 || clamp)
                isVOP1Word = false;
        }
        /* for VOP2 encoded as VOP3b (v_addc....) */
        else if (gcnInsn.encoding == GCNENC_VOP3B && vop3Mode == GCN_VOP3_VOP2 &&
                vsrc1 >= 256 && sdst == 106 /* vcc */ &&
                ((vsrc2 == 106 && mode1 == GCN_DS2_VCC) || 
                    (vsrc2 == 0 && mode1 != GCN_DS2_VCC))) /* VOP3b */
            isVOP1Word = true;
        
        if (isVOP1Word && !reqForVOP1Word)
            isVOP1Word = false;
        if (opsel != 0)
            isVOP1Word = false;
    }
    else // force for v_madmk_f32 and v_madak_f32
        isVOP1Word = true;
    
    if (isVOP1Word) // add vop3 for distinguishing encoding
        putChars(bufPtr, " vop3", 5);
    
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeVINTRPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
          GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(90);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    // print DST operand
    decodeGCNVRegOperand((insnCode>>18)&0xff, 1, bufPtr);
    putCommaSpace(bufPtr);
    if ((gcnInsn.mode & GCN_MASK1) == GCN_P0_P10_P20)
        // print VINTRP param
        decodeVINTRPParam(insnCode&0xff, bufPtr);
    else
        // or VSRC0 operand
        decodeGCNVRegOperand(insnCode&0xff, 1, bufPtr);
    putChars(bufPtr, ", attr", 6);
    const cxuint attr = (insnCode>>10)&63;
    putByteToBuf(attr, bufPtr);
    *bufPtr++ = '.';
    *bufPtr++ = "xyzw"[((insnCode>>8)&3)]; // attrchannel
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeDSEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
          GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
          uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(105);
    char* bufPtr = bufStart;
    const bool isGCN12 = ((arch&ARCH_GCN_1_2_4)!=0);
    addSpaces(bufPtr, spacesToAdd);
    bool vdstUsed = false;
    bool vaddrUsed = false;
    bool vdata0Used = false;
    bool vdata1Used = false;
    const cxuint vaddr = insnCode2&0xff;
    const cxuint vdata0 = (insnCode2>>8)&0xff;
    const cxuint vdata1 = (insnCode2>>16)&0xff;
    const cxuint vdst = insnCode2>>24;
    
    if (((gcnInsn.mode & GCN_ADDR_SRC) != 0 || (gcnInsn.mode & GCN_ONLYDST) != 0) &&
            (gcnInsn.mode & GCN_ONLY_SRC) == 0)
    {
        /* vdst is dst */
        cxuint regsNum = (gcnInsn.mode&GCN_REG_DST_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0 || (gcnInsn.mode&GCN_DST128) != 0)
            regsNum = 4;
        // print VDST
        decodeGCNVRegOperand(vdst, regsNum, bufPtr);
        vdstUsed = true;
    }
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 && (gcnInsn.mode & GCN_ONLY_SRC) == 0)
    {
        /// print VADDR
        if (vdstUsed)
            putCommaSpace(bufPtr);
        decodeGCNVRegOperand(vaddr, 1, bufPtr);
        vaddrUsed = true;
    }
    
    const uint16_t srcMode = (gcnInsn.mode & GCN_SRCS_MASK);
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 &&
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) != 0 && srcMode != GCN_NOSRC)
    {
        /* print two vdata */
        if (vaddrUsed || vdstUsed)
            // comma after previous argument (VDST, VADDR)
            putCommaSpace(bufPtr);
        // determine number of register for VDATA0
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        // print VDATA0
        decodeGCNVRegOperand(vdata0, regsNum, bufPtr);
        vdata0Used = true;
        if (srcMode == GCN_2SRCS)
        {
            putCommaSpace(bufPtr);
            // print VDATA1
            decodeGCNVRegOperand(vdata1, (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, bufPtr);
            vdata1Used = true;
        }
    }
    
    const cxuint offset = (insnCode&0xffff);
    // printing offsets (one 16-bit or two 8-bit)
    if (offset != 0)
    {
        if ((gcnInsn.mode & GCN_2OFFSETS) == 0) /* single offset */
        {
            putChars(bufPtr, " offset:", 8);
            bufPtr += itocstrCStyle(offset, bufPtr, 7, 10);
        }
        else
        {
            // if two 8-bit offsets (if supplied)
            if ((offset&0xff) != 0)
            {
                putChars(bufPtr, " offset0:", 9);
                putByteToBuf(offset&0xff, bufPtr);
            }
            if ((offset&0xff00) != 0)
            {
                putChars(bufPtr, " offset1:", 9);
                putByteToBuf((offset>>8)&0xff, bufPtr);
            }
        }
    }
    
    if ((!isGCN12 && (insnCode&0x20000)!=0) || (isGCN12 && (insnCode&0x10000)!=0))
        putChars(bufPtr, " gds", 4);
    
    // print value, if some are not used, but values is not default
    if (!vaddrUsed && vaddr != 0)
    {
        putChars(bufPtr, " vaddr=", 7);
        bufPtr += itocstrCStyle(vaddr, bufPtr, 6, 16);
    }
    if (!vdata0Used && vdata0 != 0)
    {
        putChars(bufPtr, " vdata0=", 8);
        bufPtr += itocstrCStyle(vdata0, bufPtr, 6, 16);
    }
    if (!vdata1Used && vdata1 != 0)
    {
        putChars(bufPtr, " vdata1=", 8);
        bufPtr += itocstrCStyle(vdata1, bufPtr, 6, 16);
    }
    if (!vdstUsed && vdst != 0)
    {
        putChars(bufPtr, " vdst=", 6);
        bufPtr += itocstrCStyle(vdst, bufPtr, 6, 16);
    }
    output.forward(bufPtr-bufStart);
}

// print DATA FORMAT name table
static const char* mtbufDFMTTable[] =
{
    "invalid", "8", "16", "8_8", "32", "16_16", "10_11_11", "11_11_10",
    "10_10_10_2", "2_10_10_10", "8_8_8_8", "32_32", "16_16_16_16", "32_32_32",
    "32_32_32_32", "reserved"
};

// print NUMBER FORMAT name table
static const char* mtbufNFMTTable[] =
{
    "unorm", "snorm", "uscaled", "sscaled",
    "uint", "sint", "snorm_ogl", "float"
};

void GCNDisasmUtils::decodeMUBUFEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
          GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
          uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(170);
    char* bufPtr = bufStart;
    const bool isGCN12 = ((arch&ARCH_GCN_1_2_4)!=0);
    const bool isGCN14 = ((arch&ARCH_GCN_1_4)!=0);
    const cxuint vaddr = insnCode2&0xff;
    const cxuint vdata = (insnCode2>>8)&0xff;
    const cxuint srsrc = (insnCode2>>16)&0x1f;
    const cxuint soffset = insnCode2>>24;
    const GCNInsnMode mode1 = (gcnInsn.mode & GCN_MASK1);
    if (mode1 != GCN_ARG_NONE)
    {
        addSpaces(bufPtr, spacesToAdd);
        if (mode1 != GCN_MUBUF_NOVAD)
        {
            // determine number of regs in VDATA
            cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
            if ((gcnInsn.mode & GCN_MUBUF_D16)!=0 && isGCN14)
                // 16-bit values packed into half of number of registers
                dregsNum = (dregsNum+1)>>1;
            if (insnCode2 & 0x800000U)
                dregsNum++; // tfe
            // print VDATA
            decodeGCNVRegOperand(vdata, dregsNum, bufPtr);
            putCommaSpace(bufPtr);
            // determine number of vaddr registers
            /* for addr32 - idxen+offen or 1, for addr64 - 2 (idxen and offen is illegal) */
            const cxuint aregsNum = ((insnCode & 0x3000U)==0x3000U ||
                    /* addr64 only for older GCN than 1.2 */
                    (!isGCN12 && (insnCode & 0x8000U)))? 2 : 1;
            // print VADDR
            decodeGCNVRegOperand(vaddr, aregsNum, bufPtr);
            putCommaSpace(bufPtr);
        }
        // print SRSRC
        decodeGCNOperandNoLit(dasm, srsrc<<2, 4, bufPtr, arch);
        putCommaSpace(bufPtr);
        // print SOFFSET
        decodeGCNOperandNoLit(dasm, soffset, 1, bufPtr, arch);
    }
    else
        addSpaces(bufPtr, spacesToAdd-1);
    
    // print modifiers: (offen and idxen, glc)
    if (insnCode & 0x1000U)
        putChars(bufPtr, " offen", 6);
    if (insnCode & 0x2000U)
        putChars(bufPtr, " idxen", 6);
    const cxuint offset = insnCode&0xfff;
    if (offset != 0)
    {
        putChars(bufPtr, " offset:", 8);
        bufPtr += itocstrCStyle(offset, bufPtr, 7, 10);
    }
    if (insnCode & 0x4000U)
        putChars(bufPtr, " glc", 4);
    
    // print SLD if supplied
    if (((!isGCN12 || gcnInsn.encoding==GCNENC_MTBUF) && (insnCode2 & 0x400000U)!=0) ||
        ((isGCN12 && gcnInsn.encoding!=GCNENC_MTBUF) && (insnCode & 0x20000)!=0))
        putChars(bufPtr, " slc", 4);
    
    if (!isGCN12 && (insnCode & 0x8000U)!=0)
        putChars(bufPtr, " addr64", 7);
    if (gcnInsn.encoding!=GCNENC_MTBUF && (insnCode & 0x10000U) != 0)
        putChars(bufPtr, " lds", 4);
    if (insnCode2 & 0x800000U)
        putChars(bufPtr, " tfe", 4);
    // routine to decode MTBUF format (include default values)
    if (gcnInsn.encoding==GCNENC_MTBUF)
    {
        const cxuint dfmt = (insnCode>>19)&15;
        const cxuint nfmt = (insnCode>>23)&7;
        if (dfmt!=1 || nfmt!=0)
        {
            // in shortened form: format:[DFMT, NFMT]
            putChars(bufPtr, " format:[", 9);
            if (dfmt!=1)
            {
                // print DATA_FORMAT if not default
                const char* dfmtStr = mtbufDFMTTable[dfmt];
                putChars(bufPtr, dfmtStr, ::strlen(dfmtStr));
            }
            if (dfmt!=1 && nfmt!=0)
                *bufPtr++ = ',';
            if (nfmt!=0)
            {
                // print NUMBER_FORMAT if not default
                const char* nfmtStr = mtbufNFMTTable[nfmt];
                putChars(bufPtr, nfmtStr, ::strlen(nfmtStr));
            }
            *bufPtr++ = ']';
        }
    }
    // print value, if some are not used, but values is not default
    if (mode1 == GCN_ARG_NONE || mode1 == GCN_MUBUF_NOVAD)
    {
        if (vaddr != 0)
        {
            putChars(bufPtr, " vaddr=", 7);
            bufPtr += itocstrCStyle(vaddr, bufPtr, 6, 16);
        }
        if (vdata != 0)
        {
            putChars(bufPtr, " vdata=", 7);
            bufPtr += itocstrCStyle(vdata, bufPtr, 6, 16);
        }
    }
    // also SRSRC and SOFFSET if no argument in instruction
    if (mode1 == GCN_ARG_NONE)
    {
        if (srsrc != 0)
        {
            putChars(bufPtr, " srsrc=", 7);
            bufPtr += itocstrCStyle(srsrc, bufPtr, 6, 16);
        }
        if (soffset != 0)
        {
            putChars(bufPtr, " soffset=", 9);
            bufPtr += itocstrCStyle(soffset, bufPtr, 6, 16);
        }
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeMIMGEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
         GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t insnCode2)
{
    const bool isGCN14 = ((arch&ARCH_GCN_1_4)!=0);
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(170);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    
    const cxuint dmask = (insnCode>>8)&15;
    cxuint dregsNum = 4;
    // determine register number for VDATA
    if ((gcnInsn.mode & GCN_MIMG_VDATA4) == 0)
        dregsNum = ((dmask & 1)?1:0) + ((dmask & 2)?1:0) + ((dmask & 4)?1:0) +
                ((dmask & 8)?1:0);
    
    dregsNum = (dregsNum == 0) ? 1 : dregsNum;
    if (insnCode & 0x10000)
        dregsNum++; // tfe
    
    // print VDATA
    decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, bufPtr);
    putCommaSpace(bufPtr);
    // print VADDR
    decodeGCNVRegOperand(insnCode2&0xff,
                 std::max(4, (gcnInsn.mode&GCN_MIMG_VA_MASK)+1), bufPtr);
    putCommaSpace(bufPtr);
    // print SRSRC
    decodeGCNOperandNoLit(dasm, ((insnCode2>>14)&0x7c),
                (((insnCode & 0x8000)!=0) && !isGCN14) ? 4: 8, bufPtr, arch);
    
    const cxuint ssamp = (insnCode2>>21)&0x1f;
    if ((gcnInsn.mode & GCN_MIMG_SAMPLE) != 0)
    {
        putCommaSpace(bufPtr);
        // print SSAMP if supplied
        decodeGCNOperandNoLit(dasm, ssamp<<2, 4, bufPtr, arch);
    }
    if (dmask != 1)
    {
        putChars(bufPtr, " dmask:", 7);
        // print value dmask (0-15)
        if (dmask >= 10)
        {
            *bufPtr++ = '1';
            *bufPtr++ = '0' + dmask - 10;
        }
        else
            *bufPtr++ = '0' + dmask;
    }
    
    // print other modifiers (unorm, glc, slc, ...)
    if (insnCode & 0x1000)
        putChars(bufPtr, " unorm", 6);
    if (insnCode & 0x2000)
        putChars(bufPtr, " glc", 4);
    if (insnCode & 0x2000000)
        putChars(bufPtr, " slc", 4);
    if (insnCode & 0x8000)
    {
        if (!isGCN14)
            putChars(bufPtr, " r128", 5);
        else
            putChars(bufPtr, " a16", 4);
    }
    if (insnCode & 0x10000)
        putChars(bufPtr, " tfe", 4);
    if (insnCode & 0x20000)
        putChars(bufPtr, " lwe", 4);
    if (insnCode & 0x4000)
        putChars(bufPtr, " da", 3);
    
    if ((arch & ARCH_GCN_1_2_4)!=0 && (insnCode2 & (1U<<31)) != 0)
        putChars(bufPtr, " d16", 4);
    
    // print value, if some are not used, but values is not default
    if ((gcnInsn.mode & GCN_MIMG_SAMPLE) == 0 && ssamp != 0)
    {
        putChars(bufPtr, " ssamp=", 7);
        bufPtr += itocstrCStyle(ssamp, bufPtr, 6, 16);
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeEXPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
            GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(100);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    /* export target */
    const cxuint target = (insnCode>>4)&63;
    if (target >= 32)
    {
        // print paramXX
        putChars(bufPtr, "param", 5);
        const cxuint tpar = target-32;
        const cxuint digit2 = tpar/10;
        if (digit2 != 0)
            *bufPtr++ = '0' + digit2;
        *bufPtr++ = '0' + tpar - 10*digit2;
    }
    else if (target >= 12 && target <= 15)
    {
        // print posX
        putChars(bufPtr, "pos0", 4);
        bufPtr[-1] = '0' + target-12;
    }
    else if (target < 8)
    {
        // print mrtX
        putChars(bufPtr, "mrt0", 4);
        bufPtr[-1] = '0' + target;
    }
    else if (target == 8)
        putChars(bufPtr, "mrtz", 4);
    else if (target == 9)
        putChars(bufPtr, "null", 4);
    else
    {
        /* reserved */
        putChars(bufPtr, "ill_", 4);
        const cxuint digit2 = target/10U;
        *bufPtr++ = '0' + digit2;
        *bufPtr++ = '0' + target - 10U*digit2;
    }
    
    /* print vdata registers */
    cxuint vsrcsUsed = 0;
    for (cxuint i = 0; i < 4; i++)
    {
        putCommaSpace(bufPtr);
        if (insnCode & (1U<<i))
        {
            if ((insnCode&0x400)==0)
            {
                decodeGCNVRegOperand((insnCode2>>(i<<3))&0xff, 1, bufPtr);
                vsrcsUsed |= 1<<i;
            }
            else
            {
                // if compr=1
                decodeGCNVRegOperand(((i>=2)?(insnCode2>>8):insnCode2)&0xff, 1, bufPtr);
                vsrcsUsed |= 1U<<(i>>1);
            }
        }
        else
            putChars(bufPtr, "off", 3);
    }
    
    // other modifiers
    if (insnCode&0x800)
        putChars(bufPtr, " done", 5);
    if (insnCode&0x400)
        putChars(bufPtr, " compr", 6);
    if (insnCode&0x1000)
        putChars(bufPtr, " vm", 3);
    
    // print value, if some are not used, but values is not default
    for (cxuint i = 0; i < 4; i++)
    {
        const cxuint val = (insnCode2>>(i<<3))&0xff;
        if ((vsrcsUsed&(1U<<i))==0 && val!=0)
        {
            putChars(bufPtr, " vsrc0=", 7);
            bufPtr[-2] += i; // number
            bufPtr += itocstrCStyle(val, bufPtr, 6, 16);
        }
    }
    output.forward(bufPtr-bufStart);
}

// routine to print FLAT address including 'off' for GCN 1.4
void GCNDisasmUtils::printFLATAddr(cxuint flatMode, char*& bufPtr, uint32_t insnCode2)
{
    const cxuint vaddr = insnCode2&0xff;
    if (flatMode == 0)
        decodeGCNVRegOperand(vaddr, 2 , bufPtr); // addr
    else if (flatMode == GCN_FLAT_GLOBAL)
        decodeGCNVRegOperand(vaddr,
                // if off in SADDR, then single VGPR offset
                ((insnCode2>>16)&0x7f) == 0x7f ? 2 : 1, bufPtr); // addr
    else if (flatMode == GCN_FLAT_SCRATCH)
    {
        if (((insnCode2>>16)&0x7f) == 0x7f)
            decodeGCNVRegOperand(vaddr, 1, bufPtr); // addr
        else // no vaddr
            putChars(bufPtr, "off", 3);
    }
}

void GCNDisasmUtils::decodeFLATEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
            GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2)
{
    const bool isGCN14 = ((arch&ARCH_GCN_1_4)!=0);
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(150);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    bool vdstUsed = false;
    bool vdataUsed = false;
    bool saddrUsed = false;
    const cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
    /// cmpswap store only to half of number of data registers
    cxuint dstRegsNum = ((gcnInsn.mode & GCN_CMPSWAP)!=0) ? (dregsNum>>1) :  dregsNum;
    const cxuint flatMode = gcnInsn.mode & GCN_FLAT_MODEMASK;
    // add tfe extra register if needed
    dstRegsNum = (!isGCN14 && (insnCode2 & 0x800000U)) ? dstRegsNum+1 : dstRegsNum;
    
    bool printAddr = false;
    if ((gcnInsn.mode & GCN_FLAT_ADST) == 0)
    {
        vdstUsed = true;
        // print VDST
        decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, bufPtr);
        putCommaSpace(bufPtr);
        printFLATAddr(flatMode, bufPtr, insnCode2);
        printAddr = true;
    }
    else
    {
        /* two vregs, because 64-bitness stored in PTR32 mode (at runtime) */
        printFLATAddr(flatMode, bufPtr, insnCode2);
        printAddr = true;
        if ((gcnInsn.mode & GCN_FLAT_NODST) == 0)
        {
            vdstUsed = true;
            putCommaSpace(bufPtr);
            // print VDST
            decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, bufPtr);
        }
    }
    
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0) /* print data */
    {
        vdataUsed = true;
        putCommaSpace(bufPtr);
        // print DATA field
        decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, bufPtr);
    }
    
    if (flatMode != 0 && printAddr)
    {
        // if GLOBAL_ or SCRATCH_
        putCommaSpace(bufPtr);
        cxuint saddr = (insnCode2>>16)&0x7f;
        if ((saddr&0x7f) != 0x7f)
            // print SADDR (GCN 1.4)
            decodeGCNOperandNoLit(dasm, saddr, flatMode == GCN_FLAT_SCRATCH ? 1 : 2,
                        bufPtr, arch, FLTLIT_NONE);
        else // off
            putChars(bufPtr, "off", 3);
        saddrUsed = true;
    }
    
    // get inst_offset, with sign if FLAT_SCRATCH, FLAT_GLOBAL
    const cxint instOffset = (flatMode != 0 && (insnCode&0x1000) != 0) ?
                -4096+(insnCode&0xfff) : insnCode&0xfff;
    if (isGCN14 && instOffset != 0)
    {
        putChars(bufPtr, " inst_offset:", 13);
        bufPtr += itocstrCStyle(instOffset, bufPtr, 7, 10);
    }
    
    // print other modifers
    if (isGCN14 && (insnCode & 0x2000U))
        putChars(bufPtr, " lds", 4);
    if (insnCode & 0x10000U)
        putChars(bufPtr, " glc", 4);
    if (insnCode & 0x20000U)
        putChars(bufPtr, " slc", 4);
    if (insnCode2 & 0x800000U)
    {
        if (!isGCN14)
            putChars(bufPtr, " tfe", 4);
        else
            // if GCN 1.4 this bit is NV
            putChars(bufPtr, " nv", 3);
    }
    
    // print value, if some are not used, but values is not default
    if (!vdataUsed && ((insnCode2>>8)&0xff) != 0)
    {
        putChars(bufPtr, " vdata=", 7);
        bufPtr += itocstrCStyle((insnCode2>>8)&0xff, bufPtr, 6, 16);
    }
    if (!vdstUsed && (insnCode2>>24) != 0)
    {
        putChars(bufPtr, " vdst=", 6);
        bufPtr += itocstrCStyle(insnCode2>>24, bufPtr, 6, 16);
    }
    if (flatMode != 0 && !saddrUsed && ((insnCode>>16)&0xff) != 0)
    {
        putChars(bufPtr, " saddr=", 7);
        bufPtr += itocstrCStyle((insnCode2>>16)&0xff, bufPtr, 6, 16);
    }
    output.forward(bufPtr-bufStart);
}
