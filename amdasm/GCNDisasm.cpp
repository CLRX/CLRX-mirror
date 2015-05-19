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
#include <algorithm>
#include <cstring>
#include <mutex>
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/MemAccess.h>
#include "AsmInternals.h"

using namespace CLRX;

static std::once_flag clrxGCNDisasmOnceFlag; 
static std::unique_ptr<GCNInstruction[]> gcnInstrTableByCode = nullptr;

struct GCNEncodingSpace
{
    cxuint offset;
    cxuint instrsNum;
};

static const char* gcnEncodingNames[GCNENC_MAXVAL+1] =
{
    "NONE", "SOPC", "SOPP", "SOP1", "SOP2", "SOPK", "SMRD", "VOPC", "VOP1", "VOP2",
    "VOP3A", "VOP3B", "VINTRP", "DS", "MUBUF", "MTBUF", "MIMG", "EXP", "FLAT"
};

static const GCNEncodingSpace gcnInstrTableByCodeSpaces[2*(GCNENC_MAXVAL+1)+2] =
{
    { 0, 0 },
    { 0, 0x80 }, /* GCNENC_SOPC, opcode = (7bit)<<16 */
    { 0x0080, 0x80 }, /* GCNENC_SOPP, opcode = (7bit)<<16 */
    { 0x0100, 0x100 }, /* GCNENC_SOP1, opcode = (8bit)<<8 */
    { 0x0200, 0x80 }, /* GCNENC_SOP2, opcode = (7bit)<<23 */
    { 0x0280, 0x20 }, /* GCNENC_SOPK, opcode = (5bit)<<23 */
    { 0x02a0, 0x40 }, /* GCNENC_SMRD, opcode = (6bit)<<22 */
    { 0x02e0, 0x100 }, /* GCNENC_VOPC, opcode = (8bit)<<27 */
    { 0x03e0, 0x100 }, /* GCNENC_VOP1, opcode = (8bit)<<9 */
    { 0x04e0, 0x40 }, /* GCNENC_VOP2, opcode = (6bit)<<25 */
    { 0x0520, 0x200 }, /* GCNENC_VOP3A, opcode = (9bit)<<17 */
    { 0x0520, 0x200 }, /* GCNENC_VOP3B, opcode = (9bit)<<17 */
    { 0x0720, 0x4 }, /* GCNENC_VINTRP, opcode = (2bit)<<16 */
    { 0x0724, 0x100 }, /* GCNENC_DS, opcode = (8bit)<<18 */
    { 0x0824, 0x80 }, /* GCNENC_MUBUF, opcode = (7bit)<<18 */
    { 0x08a4, 0x8 }, /* GCNENC_MTBUF, opcode = (3bit)<<16 */
    { 0x08ac, 0x80 }, /* GCNENC_MIMG, opcode = (7bit)<<18 */
    { 0x092c, 0x1 }, /* GCNENC_EXP, opcode = none */
    { 0x092d, 0x100 }, /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
    { 0x0a2d, 0x200 }, /* GCNENC_VOP3A, opcode = (9bit)<<17 (GCN1.1) */
    { 0x0a2d, 0x200 },  /* GCNENC_VOP3B, opcode = (9bit)<<17 (GCN1.1) */
    { 0x0c2d, 0x0 },
    { 0x0c2d, 0x80 }, /* GCNENC_SOPC, opcode = (7bit)<<16 (GCN1.2) */
    { 0x0cad, 0x80 }, /* GCNENC_SOPP, opcode = (7bit)<<16 (GCN1.2) */
    { 0x0d2d, 0x100 }, /* GCNENC_SOP1, opcode = (8bit)<<8 (GCN1.2) */
    { 0x0e2d, 0x80 }, /* GCNENC_SOP2, opcode = (7bit)<<23 (GCN1.2) */
    { 0x0ead, 0x20 }, /* GCNENC_SOPK, opcode = (5bit)<<23 (GCN1.2) */
    { 0x0ecd, 0x100 }, /* GCNENC_SMEM, opcode = (8bit)<<18 (GCN1.2) */
    { 0x0fcd, 0x100 }, /* GCNENC_VOPC, opcode = (8bit)<<27 (GCN1.2) */
    { 0x10cd, 0x100 }, /* GCNENC_VOP1, opcode = (8bit)<<9 (GCN1.2) */
    { 0x11cd, 0x40 }, /* GCNENC_VOP2, opcode = (6bit)<<25 (GCN1.2) */
    { 0x120d, 0x400 }, /* GCNENC_VOP3A, opcode = (10bit)<<16 (GCN1.2) */
    { 0x120d, 0x400 }, /* GCNENC_VOP3B, opcode = (10bit)<<16 (GCN1.2) */
    { 0x160d, 0x4 }, /* GCNENC_VINTRP, opcode = (2bit)<<16 (GCN1.2) */
    { 0x1611, 0x100 }, /* GCNENC_DS, opcode = (8bit)<<18 (GCN1.2) */
    { 0x1711, 0x80 }, /* GCNENC_MUBUF, opcode = (7bit)<<18 (GCN1.2) */
    { 0x1791, 0x10 }, /* GCNENC_MTBUF, opcode = (4bit)<<16 (GCN1.2) */
    { 0x17a1, 0x80 }, /* GCNENC_MIMG, opcode = (7bit)<<18 (GCN1.2) */
    { 0x1821, 0x1 }, /* GCNENC_EXP, opcode = none (GCN1.2) */
};

static const size_t gcnInstrTableByCodeLength = 0x1822;

static void initializeGCNDisassembler()
{
    gcnInstrTableByCode.reset(new GCNInstruction[gcnInstrTableByCodeLength]);
    for (cxuint i = 0; i < gcnInstrTableByCodeLength; i++)
    {
        gcnInstrTableByCode[i].mnemonic = nullptr;
        gcnInstrTableByCode[i].mode = GCN_STDMODE;
        // except VOP3 decoding routines ignores encoding (we can set None for encoding)
        gcnInstrTableByCode[i].encoding = GCNENC_NONE;
    }
    
    for (cxuint i = 0; gcnInstrsTable[i].mnemonic != nullptr; i++)
    {
        const GCNInstruction& instr = gcnInstrsTable[i];
        const GCNEncodingSpace& encSpace = gcnInstrTableByCodeSpaces[instr.encoding];
        if ((instr.archMask & ARCH_GCN_1_0_1) != 0)
        {
            if (gcnInstrTableByCode[encSpace.offset + instr.code].mnemonic == nullptr)
                gcnInstrTableByCode[encSpace.offset + instr.code] = instr;
            else /* otherwise we for GCN1.1 */
            {
                const GCNEncodingSpace& encSpace2 =
                        gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+1];
                gcnInstrTableByCode[encSpace2.offset + instr.code] = instr;
            }
        }
        if ((instr.archMask & ARCH_RX3X0) != 0)
        {
            const GCNEncodingSpace& encSpace3 = gcnInstrTableByCodeSpaces[
                        GCNENC_MAXVAL+3+instr.encoding];
            gcnInstrTableByCode[encSpace3.offset + instr.code] = instr;
        }
    }
}

GCNDisassembler::GCNDisassembler(Disassembler& disassembler)
        : ISADisassembler(disassembler), instrOutOfCode(false)
{
    std::call_once(clrxGCNDisasmOnceFlag, initializeGCNDisassembler);
}

GCNDisassembler::~GCNDisassembler()
{ }

static const bool gcnSize11Table[16] =
{
    false, // GCNENC_SMRD, // 0000
    false, // GCNENC_SMRD, // 0001
    false, // GCNENC_VINTRP, // 0010
    false, // GCNENC_NONE, // 0011 - illegal
    true,  // GCNENC_VOP3A, // 0100
    false, // GCNENC_NONE, // 0101 - illegal
    true,  // GCNENC_DS,   // 0110
    true,  // GCNENC_FLAT, // 0111
    true,  // GCNENC_MUBUF, // 1000
    false, // GCNENC_NONE,  // 1001 - illegal
    true,  // GCNENC_MTBUF, // 1010
    false, // GCNENC_NONE,  // 1011 - illegal
    true,  // GCNENC_MIMG,  // 1100
    false, // GCNENC_NONE,  // 1101 - illegal
    true,  // GCNENC_EXP,   // 1110
    false, // GCNENC_NONE   // 1111 - illegal
};

static const bool gcnSize12Table[16] =
{
    true,  // GCNENC_SMEM, // 0000
    true,  // GCNENC_EXP, // 0001
    false, // GCNENC_NONE, // 0010 - illegal
    false, // GCNENC_NONE, // 0011 - illegal
    true,  // GCNENC_VOP3A, // 0100
    false, // GCNENC_VINTRP, // 0101
    true,  // GCNENC_DS,   // 0110
    true,  // GCNENC_FLAT, // 0111
    true,  // GCNENC_MUBUF, // 1000
    false, // GCNENC_NONE,  // 1001 - illegal
    true,  // GCNENC_MTBUF, // 1010
    false, // GCNENC_NONE,  // 1011 - illegal
    true,  // GCNENC_MIMG,  // 1100
    false, // GCNENC_NONE,  // 1101 - illegal
    false, // GCNENC_NONE,  // 1110 - illegal
    false, // GCNENC_NONE   // 1111 - illegal
};

void GCNDisassembler::beforeDisassemble()
{
    labels.clear();
    
    const uint32_t* codeWords = reinterpret_cast<const uint32_t*>(input);
    const size_t codeWordsNum = (inputSize>>2);

    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                disassembler.getDeviceType());
    const bool isGCN11 = (arch == GPUArchitecture::GCN1_1);
    const bool isGCN12 = (arch == GPUArchitecture::GCN1_2);
    size_t pos;
    for (pos = 0; pos < codeWordsNum; pos++)
    {   /* scan all instructions and get jump addresses */
        const uint32_t insnCode = ULEV(codeWords[pos]);
        if ((insnCode & 0x80000000U) != 0)
        {   
            if ((insnCode & 0x40000000U) == 0)
            {   // SOP???
                if  ((insnCode & 0x30000000U) == 0x30000000U)
                {   // SOP1/SOPK/SOPC/SOPP
                    const uint32_t encPart = (insnCode & 0x0f800000U);
                    if (encPart == 0x0e800000U)
                    {   // SOP1
                        if ((insnCode&0xff) == 0xff) // literal
                            pos++;
                    }
                    else if (encPart == 0x0f000000U)
                    {   // SOPC
                        if ((insnCode&0xff) == 0xff ||
                            (insnCode&0xff00) == 0xff00) // literal
                            pos++;
                    }
                    else if (encPart == 0x0f800000U)
                    {   // SOPP
                        const cxuint opcode = (insnCode>>16)&0x7f;
                        if (opcode == 2 || (opcode >= 4 && opcode <= 9) ||
                            // GCN1.1 and GCN1.2 opcodes
                            ((isGCN11 || isGCN12) &&
                                    (opcode >= 23 && opcode <= 26))) // if jump
                            labels.push_back((pos+int16_t(insnCode&0xffff)+1)<<2);
                    }
                    else
                    {   // SOPK
                        const cxuint opcode = (insnCode>>23)&0x1f;
                        if ((!isGCN12 && opcode == 17) ||
                            (isGCN12 && opcode == 16)) // if branch fork
                            labels.push_back((pos+int16_t(insnCode&0xffff)+1)<<2);
                        else if ((!isGCN12 && opcode == 21) ||
                            (isGCN12 && opcode == 20))
                            pos++; // additional literal
                    }
                }
                else
                {   // SOP2
                    if ((insnCode&0xff) == 0xff || (insnCode&0xff00) == 0xff00)
                        pos++;  // literal
                }
            }
            else
            {   // SMRD and others
                const uint32_t encPart = (insnCode&0x3c000000U)>>26;
                if ((!isGCN12 && gcnSize11Table[encPart] && (encPart != 7 || isGCN11)) ||
                    (isGCN12 && gcnSize12Table[encPart]))
                    pos++;
            }
        }
        else
        {   // some vector instructions
            if ((insnCode & 0x7e000000U) == 0x7c000000U)
            {   // VOPC
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                    pos++;
            }
            else if ((insnCode & 0x7e000000U) == 0x7e000000U)
            {   // VOP1
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                    pos++;
            }
            else
            {   // VOP2
                const cxuint opcode = (insnCode >> 25)&0x3f;
                if ((!isGCN12 && (opcode == 32 || opcode == 33)) ||
                    (isGCN12 && (opcode == 23 || opcode == 24 ||
                    opcode == 36 || opcode == 37))) // V_MADMK and V_MADAK
                    pos++;  // inline 32-bit constant
                else if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                    pos++;  // literal
            }
        }
    }
    
    instrOutOfCode = (pos != codeWordsNum);
    
    std::sort(labels.begin(), labels.end());
    const auto newEnd = std::unique(labels.begin(), labels.end());
    labels.resize(newEnd-labels.begin());
    std::sort(namedLabels.begin(), namedLabels.end(), [](
                const std::pair<size_t, std::string>& a1,
                const std::pair<size_t, std::string>& a2) 
        { return a1.first < a2.first; });
}

static const cxbyte gcnEncoding11Table[16] =
{
    GCNENC_SMRD, // 0000
    GCNENC_SMRD, // 0001
    GCNENC_VINTRP, // 0010
    GCNENC_NONE, // 0011 - illegal
    GCNENC_VOP3A, // 0100
    GCNENC_NONE, // 0101 - illegal
    GCNENC_DS,   // 0110
    GCNENC_FLAT, // 0111
    GCNENC_MUBUF, // 1000
    GCNENC_NONE,  // 1001 - illegal
    GCNENC_MTBUF, // 1010
    GCNENC_NONE,  // 1011 - illegal
    GCNENC_MIMG,  // 1100
    GCNENC_NONE,  // 1101 - illegal
    GCNENC_EXP,   // 1110
    GCNENC_NONE   // 1111 - illegal
};

static const cxbyte gcnEncoding12Table[16] =
{
    GCNENC_SMEM, // 0000
    GCNENC_EXP, // 0001
    GCNENC_NONE, // 0010 - illegal
    GCNENC_NONE, // 0011 - illegal
    GCNENC_VOP3A, // 0100
    GCNENC_VINTRP, // 0101
    GCNENC_DS,   // 0110
    GCNENC_FLAT, // 0111
    GCNENC_MUBUF, // 1000
    GCNENC_NONE,  // 1001 - illegal
    GCNENC_MTBUF, // 1010
    GCNENC_NONE,  // 1011 - illegal
    GCNENC_MIMG,  // 1100
    GCNENC_NONE,  // 1101 - illegal
    GCNENC_NONE,  // 1110 - illegal
    GCNENC_NONE   // 1111 - illegal
};


struct CLRX_INTERNAL GCNEncodingOpcodeBits
{
    cxbyte bitPos;
    cxbyte bits;
};

static const GCNEncodingOpcodeBits gcnEncodingOpcodeTable[GCNENC_MAXVAL+1] =
{
    { 0, 0 },
    { 16, 7 }, /* GCNENC_SOPC, opcode = (7bit)<<16 */
    { 16, 7 }, /* GCNENC_SOPP, opcode = (7bit)<<16 */
    { 8, 8 }, /* GCNENC_SOP1, opcode = (8bit)<<8 */
    { 23, 7 }, /* GCNENC_SOP2, opcode = (7bit)<<23 */
    { 23, 5 }, /* GCNENC_SOPK, opcode = (5bit)<<23 */
    { 22, 6 }, /* GCNENC_SMRD, opcode = (6bit)<<22 */
    { 17, 8 }, /* GCNENC_VOPC, opcode = (8bit)<<17 */
    { 9, 8 }, /* GCNENC_VOP1, opcode = (8bit)<<9 */
    { 25, 6 }, /* GCNENC_VOP2, opcode = (6bit)<<25 */
    { 17, 9 }, /* GCNENC_VOP3A, opcode = (9bit)<<17 */
    { 17, 9 }, /* GCNENC_VOP3B, opcode = (9bit)<<17 */
    { 16, 2 }, /* GCNENC_VINTRP, opcode = (2bit)<<16 */
    { 18, 8 }, /* GCNENC_DS, opcode = (8bit)<<18 */
    { 18, 7 }, /* GCNENC_MUBUF, opcode = (7bit)<<18 */
    { 16, 3 }, /* GCNENC_MTBUF, opcode = (3bit)<<16 */
    { 18, 7 }, /* GCNENC_MIMG, opcode = (7bit)<<18 */
    { 0, 0 }, /* GCNENC_EXP, opcode = none */
    { 18, 8 } /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
};

static const GCNEncodingOpcodeBits gcnEncodingOpcode12Table[GCNENC_MAXVAL+1] =
{
    { 0, 0 },
    { 16, 7 }, /* GCNENC_SOPC, opcode = (7bit)<<16 */
    { 16, 7 }, /* GCNENC_SOPP, opcode = (7bit)<<16 */
    { 8, 8 }, /* GCNENC_SOP1, opcode = (8bit)<<8 */
    { 23, 7 }, /* GCNENC_SOP2, opcode = (7bit)<<23 */
    { 23, 5 }, /* GCNENC_SOPK, opcode = (5bit)<<23 */
    { 18, 8 }, /* GCNENC_SMEM, opcode = (8bit)<<18 */
    { 17, 8 }, /* GCNENC_VOPC, opcode = (8bit)<<17 */
    { 9, 8 }, /* GCNENC_VOP1, opcode = (8bit)<<9 */
    { 25, 6 }, /* GCNENC_VOP2, opcode = (6bit)<<25 */
    { 16, 10 }, /* GCNENC_VOP3A, opcode = (10bit)<<17 */
    { 16, 10 }, /* GCNENC_VOP3B, opcode = (10bit)<<17 */
    { 16, 2 }, /* GCNENC_VINTRP, opcode = (2bit)<<16 */
    { 17, 8 }, /* GCNENC_DS, opcode = (8bit)<<18 */
    { 18, 7 }, /* GCNENC_MUBUF, opcode = (7bit)<<18 */
    { 15, 4 }, /* GCNENC_MTBUF, opcode = (4bit)<<15 */
    { 18, 7 }, /* GCNENC_MIMG, opcode = (7bit)<<18 */
    { 0, 0 }, /* GCNENC_EXP, opcode = none */
    { 18, 8 } /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
};

static const char* gcnOperandFloatTable[] =
{
    "0.5", "-0.5", "1.0", "-1.0", "2.0", "-2.0", "4.0", "-4.0"
};

union CLRX_INTERNAL FloatUnion
{
    float f;
    uint32_t u;
};

static inline size_t putByteToBuf(cxuint op, char* buf)
{
    size_t pos = 0;
    cxuint val = op;
    if (val >= 100U)
    {
        cxuint digit2 = val/100U;
        buf[pos++] = digit2+'0';
        val -= digit2*100U;
    }
    {
        const cxuint digit1 = val/10U;
        if (digit1 != 0 || op >= 100U)
            buf[pos++] = digit1 + '0';
        buf[pos++] = val-digit1*10U + '0';
    }
    return pos;
}

static inline size_t putHexByteToBuf(cxuint op, char* buf)
{
    size_t bufPos = 0;
    const cxuint digit0 = op&0xf;
    const cxuint digit1 = (op&0xf0)>>4;
    buf[bufPos++] = '0';
    buf[bufPos++] = 'x';
    if (digit1 != 0)
        buf[bufPos++] = (digit1<=9)?'0'+digit1:'a'+digit1-10;
    buf[bufPos++] = (digit0<=9)?'0'+digit0:'a'+digit0-10;
    return bufPos;
}

static size_t regRanges(cxuint op, cxuint vregNum, char* buf)
{
    size_t pos = 0;
    if (vregNum!=1)
        buf[pos++] = '[';
    pos += putByteToBuf(op, buf+pos);
    if (vregNum!=1)
    {
        buf[pos++] = ':';
        op += vregNum-1;
        if (op > 255)
            op -= 256; // fix for VREGS
        pos += putByteToBuf(op, buf+pos);
        buf[pos++] = ']';
    }
    return pos;
}

static size_t decodeGCNVRegOperand(cxuint op, cxuint vregNum, char* buf)
{
    buf[0] = 'v';
    return regRanges(op, vregNum, buf+1)+1;
}

enum FloatLitType: cxbyte
{
    FLTLIT_NONE,
    FLTLIT_F32,
    FLTLIT_F16,
};

static size_t printLiteral(char* buf, uint32_t literal, FloatLitType floatLit)
{
    size_t pos = itocstrCStyle(literal, buf, 11, 16);
    if (floatLit != FLTLIT_NONE)
    {
        FloatUnion fu;
        fu.u = literal;
        buf[pos++] = ' ';
        buf[pos++] = '/';
        buf[pos++] = '*';
        buf[pos++] = ' ';
        if (floatLit == FLTLIT_F32)
            pos += ftocstrCStyle(fu.f, buf+pos, 20);
        else
            pos += htocstrCStyle(fu.u, buf+pos, 20);
        buf[pos++] = (floatLit == FLTLIT_F32)?'f':'h';
        buf[pos++] = ' ';
        buf[pos++] = '*';
        buf[pos++] = '/';
    }   
    return pos;
}

static size_t decodeGCNOperand(cxuint op, cxuint regNum, char* buf, uint16_t arch,
           uint32_t literal = 0, FloatLitType floatLit = FLTLIT_NONE)
{
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    if ((!isGCN12 && op < 104) || (isGCN12 && op < 102) || (op >= 256 && op < 512))
    {   // scalar
        if (op >= 256)
        {
            buf[0] = 'v';
            op -= 256;
        }
        else
            buf[0] = 's';
        return regRanges(op, regNum, buf+1)+1;
    }
    
    const cxuint op2 = op&~1U;
    if (op2 == 106 || op2 == 108 || op2 == 110 || op2 == 126 ||
        (op2 == 104 && (arch&ARCH_RX2X0)!=0) ||
        ((op2 == 102 || op2 == 104) && isGCN12))
    {   // VCC
        size_t pos = 0;
        switch(op2)
        {
            case 102:
                ::memcpy(buf+pos, "flat_scratch", 12);
                pos += 12;
                break;
            case 104:
                if (isGCN12)
                {   // GCN1.2
                    ::memcpy(buf+pos, "xnack_mask", 10);
                    pos += 10;
                }
                else
                {   // GCN1.1
                    ::memcpy(buf+pos, "flat_scratch", 12);
                    pos += 12;
                }
                break;
            case 106:
                buf[pos++] = 'v';
                buf[pos++] = 'c';
                buf[pos++] = 'c';
                break;
            case 108:
                buf[pos++] = 't';
                buf[pos++] = 'b';
                buf[pos++] = 'a';
                break;
            case 110:
                buf[pos++] = 't';
                buf[pos++] = 'm';
                buf[pos++] = 'a';
                break;
            case 126:
                buf[pos++] = 'e';
                buf[pos++] = 'x';
                buf[pos++] = 'e';
                buf[pos++] = 'c';
                break;
        }
        if (regNum >= 2)
        {
            if (op&1) // unaligned!!
            {
                buf[pos++] = '_';
                buf[pos++] = 'u';
                buf[pos++] = '!';
            }
            if (regNum > 2)
            {
                ::memcpy(buf+pos, "&ill!", 5);
                pos += 5;
            }
            return pos;
        }
        buf[pos++] = '_';
        if ((op&1) == 0)
        { buf[pos++] = 'l'; buf[pos++] = 'o'; }
        else
        { buf[pos++] = 'h'; buf[pos++] = 'i'; }
        return pos;
    }
    
    if (op == 255) // if literal
        return printLiteral(buf, literal, floatLit);
    
    if (op >= 112 && op < 124)
    {
        buf[0] = 't';
        buf[1] = 't';
        buf[2] = 'm';
        buf[3] = 'p';
        return regRanges(op-112, regNum, buf+4)+4;
    }
    if (op == 124)
    {
        buf[0] = 'm';
        buf[1] = '0';
        if (regNum > 1)
        {
            ::memcpy(buf+2, "&ill!", 5);
            return 7;
        }
        return 2;
    }
    
    if (op >= 128 && op <= 192)
    {   // integer constant
        size_t pos = 0;
        op -= 128;
        const cxuint digit1 = op/10U;
        if (digit1 != 0)
            buf[pos++] = digit1 + '0';
        buf[pos++] = op-digit1*10U + '0';
        return pos;
    }
    if (op > 192 && op <= 208)
    {
        buf[0] = '-';
        cxuint pos = 1;
        if (op >= 202)
        {
            buf[pos++] = '1';
            buf[pos++] = op-202+'0';
        }
        else
            buf[pos++] = op-192+'0';
        return pos;
    }
    if (op >= 240 && op < 248)
    {
        size_t pos = 0;
        const char* inOp = gcnOperandFloatTable[op-240];
        for (pos = 0; inOp[pos] != 0; pos++)
            buf[pos] = inOp[pos];
        return pos;
    }
    
    switch(op)
    {
        case 248:
            if (isGCN12)
            {   // 1/(2*PI)
                ::memcpy(buf, "0.15915494", 10);
                return 10;
            }
            break;
        case 251:
            ::memcpy(buf, "vccz", 4);
            return 4;
        case 252:
            ::memcpy(buf, "execz", 5);
            return 5;
        case 253:
            buf[0] = 's';
            buf[1] = 'c';
            buf[2] = 'c';
            return 3;
        case 254:
            buf[0] = 'l';
            buf[1] = 'd';
            buf[2] = 's';
            return 3;
    }
    
    // reserved value (illegal)
    buf[0] = 'i';
    buf[1] = 'l';
    buf[2] = 'l';
    buf[3] = '_';
    buf[4] = '0'+op/100U;
    buf[5] = '0'+(op/10U)%10U;
    buf[6] = '0'+op%10U;
    return 7;
}

static const char* sendMsgCodeMessageTable[16] =
{
    "0",
    "interrupt",
    "gs",
    "gs_done",
    "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "system"
};

static const char* sendGsOpMessageTable[4] =
{ "nop", "cut", "emit", "emit-cut" };

/* encoding routines */

static size_t addSpaces(char* buf, cxuint spacesToAdd)
{
    size_t bufPos = 0;
    for (cxuint k = spacesToAdd; k>0; k--)
        buf[bufPos++] = ' ';
    return bufPos;
}

static void decodeSOPCEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    char* buf = output.reserve(80);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bufPos += decodeGCNOperand(insnCode&0xff,
           (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf + bufPos, arch, literal);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    if ((gcnInsn.mode & GCN_SRC1_IMM) != 0)
        bufPos += putHexByteToBuf((insnCode>>8)&0xff, buf+bufPos);
    else
        bufPos += decodeGCNOperand((insnCode>>8)&0xff,
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos, arch, literal);
    output.forward(bufPos);
}


template<typename LabelWriter>
static void decodeSOPPEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         LabelWriter labelWriter, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t literal, size_t pos)
{
    char* buf = output.reserve(60);
    size_t bufPos = 0;
    const cxuint imm16 = insnCode&0xffff;
    switch(gcnInsn.mode&GCN_MASK1)
    {
        case GCN_IMM_REL:
        {
            const size_t branchPos = (pos + int16_t(imm16))<<2;
            bufPos = addSpaces(buf, spacesToAdd);
            output.forward(bufPos);
            labelWriter(branchPos);
            bufPos = 0;
            break;
        }
        case GCN_IMM_LOCKS:
        {
            bool prevLock = false;
            bufPos = addSpaces(buf, spacesToAdd);
            if ((imm16&15) != 15)
            {
                const cxuint lockCnt = imm16&15;
                ::memcpy(buf+bufPos, "vmcnt(", 6);
                bufPos += 6;
                if (lockCnt >= 10)
                    buf[bufPos++] = '1';
                buf[bufPos++] = '0' + ((lockCnt>=10)?lockCnt-10:lockCnt);
                buf[bufPos++] = ')';
                prevLock = true;
            }
            if (((imm16>>4)&7) != 7)
            {
                if (prevLock)
                {
                    buf[bufPos++] = ' ';
                    buf[bufPos++] = '&';
                    buf[bufPos++] = ' ';
                }
                ::memcpy(buf+bufPos, "expcnt(", 7);
                bufPos += 7;
                buf[bufPos++] = '0' + ((imm16>>4)&7);
                buf[bufPos++] = ')';
                prevLock = true;
            }
            if (((imm16>>8)&15) != 15)
            {   /* LGKMCNT bits is 4 (5????) */
                const cxuint lockCnt = (imm16>>8)&15;
                if (prevLock)
                {
                    buf[bufPos++] = ' ';
                    buf[bufPos++] = '&';
                    buf[bufPos++] = ' ';
                }
                ::memcpy(buf+bufPos, "lgkmcnt(", 8);
                bufPos += 8;
                const cxuint digit2 = lockCnt/10U;
                if (lockCnt >= 10)
                    buf[bufPos++] = '0'+digit2;
                buf[bufPos++] = '0' + lockCnt-digit2*10U;
                buf[bufPos++] = ')';
                prevLock = true;
            }
            if ((imm16&0xf080) != 0 || (imm16==0xf7f))
            {   /* additional info about imm16 */
                if (prevLock)
                {
                    buf[bufPos++] = ' ';
                    buf[bufPos++] = ':';
                }
                bufPos += itocstrCStyle(imm16, buf+bufPos, 11, 16);
            }
            break;
        }
        case GCN_IMM_MSGS:
        {
            cxuint illMask = 0xfff0;
            bufPos = addSpaces(buf, spacesToAdd);
            ::memcpy(buf+bufPos, "sendmsg(", 8);
            bufPos += 8;
            const cxuint msgType = imm16&15;
            const char* msgName = sendMsgCodeMessageTable[msgType];
            while (*msgName != 0)
                buf[bufPos++] = *msgName++;
            if ((msgType&14) == 2 || (msgType >= 4 && msgType <= 14) ||
                (imm16&0x3f0) != 0) // gs ops
            {
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
                illMask = 0xfcc0;
                const char* gsopName = sendGsOpMessageTable[(imm16>>4)&3];
                while (*gsopName != 0)
                    buf[bufPos++] = *gsopName++;
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
                buf[bufPos++] = '0' + ((imm16>>8)&3);
            }
            buf[bufPos++] = ')';
            if ((imm16&illMask) != 0)
            {
                buf[bufPos++] = ' ';
                buf[bufPos++] = ':';
                bufPos += itocstrCStyle(imm16, buf+bufPos, 11, 16);
            }
            break;
        }
        case GCN_IMM_NONE:
            if (imm16 != 0)
            {
                bufPos = addSpaces(buf, spacesToAdd);
                bufPos += itocstrCStyle(imm16, buf+bufPos, 11, 16);
            }
            break;
        default:
            bufPos = addSpaces(buf, spacesToAdd);
            bufPos += itocstrCStyle(imm16, buf+bufPos, 11, 16);
            break;
    }
    output.forward(bufPos);
}

static void decodeSOP1Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    char* buf = output.reserve(70);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bool isDst = (gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE;
    if (isDst)
        bufPos += decodeGCNOperand((insnCode>>16)&0x7f,
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
    
    if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
    {
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
        }
        bufPos += decodeGCNOperand(insnCode&0xff,
               (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf + bufPos, arch, literal);
    }
    else if ((insnCode&0xff) != 0)
    {
        ::memcpy(buf+bufPos, " ssrc=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle((insnCode&0xff), buf+bufPos, 6, 16);
    }
    
    if (!isDst && ((insnCode>>16)&0x7f) != 0)
    {
        ::memcpy(buf+bufPos, " sdst=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(((insnCode>>16)&0x7f), buf+bufPos, 6, 16);
    }
    output.forward(bufPos);
}

static void decodeSOP2Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    char* buf = output.reserve(80);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_REG_S1_JMP)
    {
        bufPos += decodeGCNOperand((insnCode>>16)&0x7f,
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
    }
    bufPos += decodeGCNOperand(insnCode&0xff,
           (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf + bufPos, arch, literal);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand((insnCode>>8)&0xff,
           (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos, arch, literal);
    
    if ((gcnInsn.mode & GCN_MASK1) == GCN_REG_S1_JMP &&
        ((insnCode>>16)&0x7f) != 0)
    {
        ::memcpy(buf+bufPos, " sdst=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(((insnCode>>16)&0x7f), buf+bufPos, 6, 16);
    }
    output.forward(bufPos);
}

static const char* hwregNames[13] =
{
    "0", "mode", "status", "trapsts",
    "hw_id", "gpr_alloc", "lds_alloc", "ib_sts",
    "pc_lo", "pc_hi", "inst_dw0", "inst_dw1",
    "ib_dbg0"
};

template<typename LabelWriter>
static void decodeSOPKEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         LabelWriter labelWriter, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t literal, size_t pos)
{
    char* buf = output.reserve(80);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    if ((gcnInsn.mode & GCN_IMM_DST) == 0)
    {
        bufPos += decodeGCNOperand((insnCode>>16)&0x7f,
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
    }
    const cxuint imm16 = insnCode&0xffff;
    if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL)
    {
        const size_t branchPos = (pos + int16_t(imm16))<<2;
        output.forward(bufPos);
        labelWriter(branchPos);
        buf = output.reserve(60);
        bufPos = 0;
    }
    else if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_SREG)
    {
        ::memcpy(buf+bufPos, "hwreg(", 6);
        bufPos += 6;
        const cxuint hwregId = imm16&0x3f;
        if (hwregId < 13)
        {
            const char* hwregName = hwregNames[hwregId];
            for (cxuint k = 0; hwregName[k] != 0; k++)
                buf[bufPos++] = hwregName[k];
        }
        else
        {
            const cxuint digit2 = hwregId/10U;
            buf[bufPos++] = '0' + digit2;
            buf[bufPos++] = '0' + hwregId - digit2*10U;
        }
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        const cxuint hwoffset = (imm16>>6)&31;
        {
            const cxuint digit2 = hwoffset/10U;
            if (digit2 != 0)
                buf[bufPos++] = '0' + digit2;
            buf[bufPos++] = '0' + hwoffset - 10U*digit2;
        }
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        const cxuint hwsize = ((imm16>>11)&31)+1;
        {
            const cxuint digit2 = hwsize/10U;
            if (digit2 != 0)
                buf[bufPos++] = '0' + digit2;
            buf[bufPos++] = '0' + hwsize - 10U*digit2;
        }
        buf[bufPos++] = ')';
    }
    else
        bufPos += itocstrCStyle(imm16, buf+bufPos, 11, 16);
    
    if (gcnInsn.mode & GCN_IMM_DST)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        if (gcnInsn.mode & GCN_SOPK_CONST)
        {
            bufPos += itocstrCStyle(literal, buf+bufPos, 11, 16);
            if (((insnCode>>16)&0x7f) != 0)
            {
                ::memcpy(buf+bufPos, " sdst=", 6);
                bufPos += 6;
                bufPos += itocstrCStyle((insnCode>>16)&0x7f, buf+bufPos, 6, 16);
            }
        }
        else
            bufPos += decodeGCNOperand((insnCode>>16)&0x7f,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
    }
    output.forward(bufPos);
}

static void decodeSMRDEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    char* buf = output.reserve(100);
    size_t bufPos = 0;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    
    bool spacesAdded = false;
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        bufPos += addSpaces(buf, spacesToAdd);
        bufPos += decodeGCNOperand((insnCode>>15)&0x7f,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
        useDst = true;
        spacesAdded = true;
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        bufPos += addSpaces(buf, spacesToAdd);
        bufPos += decodeGCNOperand((insnCode>>15)&0x7f, dregsNum, buf + bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNOperand((insnCode>>8)&0x7e,
                   (gcnInsn.mode&GCN_SBASE4)?4:2, buf + bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        if (insnCode&0x100) // immediate value
            bufPos += itocstrCStyle(insnCode&0xff, buf+bufPos, 11, 16);
        else // S register
            bufPos += decodeGCNOperand(insnCode&0xff, 1, buf + bufPos, arch);
        useDst = true;
        useOthers = true;
        spacesAdded = true;
    }
    
    if (!useDst && (insnCode & 0x3f8000U) != 0)
    {
        bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " sdst=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle((insnCode>>15)&0x7f, buf+bufPos, 6, 16);
    }
    if (!useOthers && (insnCode & 0x7e00U)!=0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " sbase=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle((insnCode>>9)&0x3f, buf+bufPos, 6, 16);
    }
    if (!useOthers && (insnCode & 0xffU)!=0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " offset=", 8);
        bufPos += 8;
        bufPos += itocstrCStyle(insnCode&0xff, buf+bufPos, 6, 16);
    }
    if (!useOthers && (insnCode & 0x100U)!=0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " imm=1", 6);
        bufPos += 6;
    }
    output.forward(bufPos);
}

static void decodeSMEMEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* buf = output.reserve(100);
    size_t bufPos = 0;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    bool spacesAdded = false;
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        bufPos += addSpaces(buf, spacesToAdd);
        bufPos += decodeGCNOperand((insnCode>>6)&0x7f,
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
        useDst = true;
        spacesAdded = true;
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        bufPos += addSpaces(buf, spacesToAdd);
        if (mode1 & GCN_SMEM_SDATA_IMM)
            bufPos += putHexByteToBuf((insnCode>>6)&0x7f, buf + bufPos);
        else
            bufPos += decodeGCNOperand((insnCode>>6)&0x7f, dregsNum, buf + bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNOperand((insnCode<<1)&0x7e,
                   (gcnInsn.mode&GCN_SBASE4)?4:2, buf + bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        if (insnCode&0x20000) // immediate value
            bufPos += itocstrCStyle(insnCode2&0xfffff, buf+bufPos, 11, 16);
        else // S register
            bufPos += decodeGCNOperand(insnCode2&0xff, 1, buf + bufPos, arch);
        useDst = true;
        useOthers = true;
        spacesAdded = true;
    }
    
    if ((insnCode & 0x10000) != 0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'g';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    
    if (!useDst && (insnCode & 0x1fc0U) != 0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " sdata=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle((insnCode>>6)&0x7f, buf+bufPos, 6, 16);
    }
    if (!useOthers && (insnCode & 0x3fU)!=0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " sbase=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle(insnCode&0x3f, buf+bufPos, 6, 16);
    }
    if (!useOthers && insnCode2!=0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " offset=", 8);
        bufPos += 8;
        bufPos += itocstrCStyle(insnCode2, buf+bufPos, 12, 16);
    }
    if (!useOthers && (insnCode & 0x20000U)!=0)
    {
        if (!spacesAdded)
            bufPos += addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        ::memcpy(buf+bufPos, " imm=1", 6);
        bufPos += 6;
    }
    
    output.forward(bufPos);
}

struct CLRX_INTERNAL VOPExtraWordOut
{
    cxuint src0: 9;
    cxuint sextSrc0: 1;
    cxuint negSrc0: 1;
    cxuint absSrc0: 1;
    cxuint sextSrc1: 1;
    cxuint negSrc1: 1;
    cxuint absSrc1: 1;
};

static const char* sdwaSelChoicesTbl[] =
{
    "byte0", "byte1", "byte2", "byte3", "word0", "word1", nullptr, "invalid"
};

static const char* sdwaDstUnusedTbl[] =
{
    nullptr, "sext", "preserve", "invalid"
};

/* returns mask of abs,neg,sext for src0 and src1 argument and src0 register */
static inline VOPExtraWordOut decodeVOPSDWAFlags(uint32_t insnCode2)
{
    return { (insnCode2&0xff)+256,
        (insnCode2&(1U<<19))!=0, (insnCode2&(1U<<20))!=0, (insnCode2&(1U<<21))!=0,
        (insnCode2&(1U<<27))!=0, (insnCode2&(1U<<28))!=0, (insnCode2&(1U<<29))!=0 };
}

static void decodeVOPSDWA(FastOutputBuffer& output, uint32_t insnCode2,
          bool src0Used, bool src1Used)
{
    char* buf = output.reserve(90);
    size_t bufPos = 0;
    const cxuint dstSel = (insnCode2>>8)&7;
    const cxuint dstUnused = (insnCode2>>11)&3;
    const cxuint src0Sel = (insnCode2>>16)&7;
    const cxuint src1Sel = (insnCode2>>24)&7;
    if (insnCode2 & 0x2000)
    {
        ::memcpy(buf+bufPos, " clamp", 6);
        bufPos += 6;
    }
    if (dstSel != 6)
    {
        ::memcpy(buf+bufPos, " dst_sel:", 9);
        bufPos += 9;
        size_t k = 0;
        for (; sdwaSelChoicesTbl[dstSel][k] != 0; k++)
            buf[bufPos+k] = sdwaSelChoicesTbl[dstSel][k];
        bufPos += k;
    }
    if (dstUnused!=0)
    {
        ::memcpy(buf+bufPos, " dst_unused:", 12);
        bufPos += 12;
        size_t k = 0;
        for (; sdwaDstUnusedTbl[dstUnused][k] != 0; k++)
            buf[bufPos+k] = sdwaDstUnusedTbl[dstUnused][k];
        bufPos += k;
    }
    if (src0Sel != 6)
    {
        ::memcpy(buf+bufPos, " src0_sel:", 10);
        bufPos += 10;
        size_t k = 0;
        for (; sdwaSelChoicesTbl[src0Sel][k] != 0; k++)
            buf[bufPos+k] = sdwaSelChoicesTbl[src0Sel][k];
        bufPos += k;
    }
    if (src1Sel != 6)
    {
        ::memcpy(buf+bufPos, " src1_sel:", 10);
        bufPos += 10;
        size_t k = 0;
        for (; sdwaSelChoicesTbl[src1Sel][k] != 0; k++)
            buf[bufPos+k] = sdwaSelChoicesTbl[src1Sel][k];
        bufPos += k;
    }
    
    if (!src0Used)
    {
        if ((insnCode2&(1U<<20))!=0)
        {
            ::memcpy(buf+bufPos, " sext0", 6);
            bufPos += 6;
        }
        if ((insnCode2&(1U<<20))!=0)
        {
            ::memcpy(buf+bufPos, " neg0", 5);
            bufPos += 5;
        }
        if ((insnCode2&(1U<<21))!=0)
        {
            ::memcpy(buf+bufPos, " abs0", 5);
            bufPos += 5;
        }
    }
    if (!src1Used)
    {
        if ((insnCode2&(1U<<27))!=0)
        {
            ::memcpy(buf+bufPos, " sext1", 6);
            bufPos += 6;
        }
        if ((insnCode2&(1U<<28))!=0)
        {
            ::memcpy(buf+bufPos, " neg1", 5);
            bufPos += 5;
        }
        if ((insnCode2&(1U<<29))!=0)
        {
            ::memcpy(buf+bufPos, " abs1", 5);
            bufPos += 5;
        }
    }
    
    output.forward(bufPos);
}

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
    return { (insnCode2&0xff)+256,
        false, (insnCode2&(1U<<20))!=0, (insnCode2&(1U<<21))!=0,
        false, (insnCode2&(1U<<22))!=0, (insnCode2&(1U<<23))!=0 };
}

static void decodeVOPDPP(FastOutputBuffer& output, uint32_t insnCode2,
        bool src0Used, bool src1Used)
{
    char* buf = output.reserve(100);
    size_t bufPos = 0;
    const cxuint dppCtrl = (insnCode2>>8)&0x1ff;
    
    if (dppCtrl<256)
    {   // quadperm
        ::memcpy(buf+bufPos, " quad_perm:[", 12);
        bufPos += 12;
        buf[bufPos++] = '0' + (dppCtrl&3);
        buf[bufPos++] = ',';
        buf[bufPos++] = '0' + ((dppCtrl>>2)&3);
        buf[bufPos++] = ',';
        buf[bufPos++] = '0' + ((dppCtrl>>4)&3);
        buf[bufPos++] = ',';
        buf[bufPos++] = '0' + ((dppCtrl>>6)&3);
        buf[bufPos++] = ']';
    }
    else if ((dppCtrl >= 0x101 && dppCtrl <= 0x12f) && ((dppCtrl&0xf) != 0))
    {   // row_shl
        if ((dppCtrl&0xf0) == 0)
            ::memcpy(buf+bufPos, " row_shl:", 9);
        else if ((dppCtrl&0xf0) == 16)
            ::memcpy(buf+bufPos, " row_shr:", 9);
        else
            ::memcpy(buf+bufPos, " row_ror:", 9);
        bufPos += 9;
        bufPos += putByteToBuf(dppCtrl&0xf, buf+bufPos);
    }
    else if (dppCtrl >= 0x130 && dppCtrl <= 0x143 && dppCtrl130Tbl[dppCtrl-0x130]!=nullptr)
    {
        size_t k = 0;
        const char* str = dppCtrl130Tbl[dppCtrl-0x130];
        for (; str[k] != 0; k++)
            buf[bufPos+k] = str[k];
        bufPos += k;
    }
    else if (dppCtrl != 0x100)
    {
        ::memcpy(buf+bufPos, " dppctrl:", 9);
        bufPos += 9;
        bufPos += itocstrCStyle(dppCtrl, buf+bufPos, 10, 16);
    }
    
    if (insnCode2 & (0x80000U))
    {   // bound ctrl
        ::memcpy(buf+bufPos, " bound_ctrl", 11);
        bufPos += 11;
    }
    ::memcpy(buf+bufPos, " bank_mask:", 11);
    bufPos += 11;
    bufPos += putByteToBuf((insnCode2>>24)&0xf, buf+bufPos);
    ::memcpy(buf+bufPos, " row_mask:", 10);
    bufPos += 10;
    bufPos += putByteToBuf((insnCode2>>28)&0xf, buf+bufPos);
    
    if (!src0Used)
    {
        if ((insnCode2&(1U<<20))!=0)
        {
            ::memcpy(buf+bufPos, " neg0", 5);
            bufPos += 5;
        }
        if ((insnCode2&(1U<<21))!=0)
        {
            ::memcpy(buf+bufPos, " abs0", 5);
            bufPos += 5;
        }
    }
    if (!src1Used)
    {
        if ((insnCode2&(1U<<22))!=0)
        {
            ::memcpy(buf+bufPos, " neg1", 5);
            bufPos += 5;
        }
        if ((insnCode2&(1U<<23))!=0)
        {
            ::memcpy(buf+bufPos, " abs1", 5);
            bufPos += 5;
        }
    }
    
    output.forward(bufPos);
}

static void decodeVOPCEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    char* buf;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    if (isGCN12)
        buf = output.reserve(100);
    else
        buf = output.reserve(80);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    
    const cxuint src0Field = (insnCode&0x1ff);
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    ::memcpy(buf+bufPos, "vcc, ", 5);
    bufPos += 5;
    if (isGCN12)
    {
        if (src0Field == 0xf9)
            extraFlags = decodeVOPSDWAFlags(literal);
        else if (src0Field == 0xfa)
            extraFlags = decodeVOPDPPFlags(literal);
        else
            extraFlags.src0 = src0Field;
    }
    else
        extraFlags.src0 = src0Field;
    
    if (extraFlags.sextSrc0)
    {
        ::memcpy(buf+bufPos, "sext(", 5);
        bufPos += 5;
    }
    if (extraFlags.negSrc0)
        buf[bufPos++] = '-';
    if (extraFlags.absSrc0)
    {
        buf[bufPos++] = 'a';
        buf[bufPos++] = 'b';
        buf[bufPos++] = 's';
        buf[bufPos++] = '(';
    }
    bufPos += decodeGCNOperand(extraFlags.src0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                           buf + bufPos, arch, literal, displayFloatLits);
    if (extraFlags.absSrc0)
        buf[bufPos++] = ')';
    if (extraFlags.sextSrc0)
        buf[bufPos++] = ')';
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    
    if (extraFlags.sextSrc1)
    {
        ::memcpy(buf+bufPos, "sext(", 5);
        bufPos += 5;
    }
    if (extraFlags.negSrc1)
        buf[bufPos++] = '-';
    if (extraFlags.absSrc1)
    {
        buf[bufPos++] = 'a';
        buf[bufPos++] = 'b';
        buf[bufPos++] = 's';
        buf[bufPos++] = '(';
    }
    bufPos += decodeGCNVRegOperand(((insnCode>>9)&0xff),
           (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos);
    if (extraFlags.absSrc1)
        buf[bufPos++] = ')';
    if (extraFlags.sextSrc1)
        buf[bufPos++] = ')';
    
    output.forward(bufPos);
    if (isGCN12)
    {
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, literal, true, true);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, true, true);
    }
}

static void decodeVOP1Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    char* buf;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    if (isGCN12)
        buf = output.reserve(110);
    else
        buf = output.reserve(90);
    size_t bufPos = 0;
    
    const cxuint src0Field = (insnCode&0x1ff);
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    
    if (isGCN12)
    {
        if (src0Field == 0xf9)
            extraFlags = decodeVOPSDWAFlags(literal);
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
        bufPos += addSpaces(buf, spacesToAdd);
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_SGPR)
            bufPos += decodeGCNVRegOperand(((insnCode>>17)&0xff),
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos);
        else
            bufPos += decodeGCNOperand(((insnCode>>17)&0xff),
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        if (extraFlags.sextSrc0)
        {
            ::memcpy(buf+bufPos, "sext(", 5);
            bufPos += 5;
        }
        if (extraFlags.negSrc0)
            buf[bufPos++] = '-';
        if (extraFlags.absSrc0)
        {
            buf[bufPos++] = 'a';
            buf[bufPos++] = 'b';
            buf[bufPos++] = 's';
            buf[bufPos++] = '(';
        }
        bufPos += decodeGCNOperand(extraFlags.src0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                           buf + bufPos, arch, literal, displayFloatLits);
        if (extraFlags.absSrc0)
            buf[bufPos++] = ')';
        if (extraFlags.sextSrc0)
            buf[bufPos++] = ')';
    }
    else if ((insnCode & 0x1fe01ffU) != 0)
    {
        bufPos += addSpaces(buf, spacesToAdd);
        if ((insnCode & 0x1fe0000U) != 0)
        {
            ::memcpy(buf+bufPos, "vdst=", 5);
            bufPos += 5;
            bufPos += itocstrCStyle((insnCode>>17)&0xff, buf+bufPos, 6, 16);
            if ((insnCode & 0x1ff) != 0)
                buf[bufPos++] = ' ';
        }
        if ((insnCode & 0x1ff) != 0)
        {
            ::memcpy(buf+bufPos, "src0=", 5);
            bufPos += 5;
            bufPos += itocstrCStyle(insnCode&0x1ff, buf+bufPos, 6, 16);
        }
        argsUsed = false;
    }
    output.forward(bufPos);
    if (isGCN12)
    {
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, literal, argsUsed, argsUsed);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, argsUsed, argsUsed);
    }
}

static void decodeVOP2Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    char* buf;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    if (isGCN12)
        buf = output.reserve(130);
    else
        buf = output.reserve(110);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    
    const cxuint src0Field = (insnCode&0x1ff);
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    
    if (isGCN12)
    {
        if (src0Field == 0xf9)
            extraFlags = decodeVOPSDWAFlags(literal);
        else if (src0Field == 0xfa)
            extraFlags = decodeVOPDPPFlags(literal);
        else
            extraFlags.src0 = src0Field;
    }
    else
        extraFlags.src0 = src0Field;
    
    if (mode1 == GCN_DS1_SGPR)
        bufPos += decodeGCNOperand(((insnCode>>17)&0xff),
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
    else
        bufPos += decodeGCNVRegOperand(((insnCode>>17)&0xff),
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos);
    if (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC)
    {
        ::memcpy(buf+bufPos, ", vcc", 5);
        bufPos += 5;
    }
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    if (extraFlags.sextSrc0)
    {
        ::memcpy(buf+bufPos, "sext(", 5);
        bufPos += 5;
    }
    if (extraFlags.negSrc0)
        buf[bufPos++] = '-';
    if (extraFlags.absSrc0)
    {
        buf[bufPos++] = 'a';
        buf[bufPos++] = 'b';
        buf[bufPos++] = 's';
        buf[bufPos++] = '(';
    }
    bufPos += decodeGCNOperand(extraFlags.src0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                       buf + bufPos, arch, literal, displayFloatLits);
    if (extraFlags.absSrc0)
            buf[bufPos++] = ')';
    if (extraFlags.sextSrc0)
        buf[bufPos++] = ')';
    if (mode1 == GCN_ARG1_IMM)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += printLiteral(buf+bufPos, literal, displayFloatLits);
    }
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    if (extraFlags.sextSrc1)
    {
        ::memcpy(buf+bufPos, "sext(", 5);
        bufPos += 5;
    }
    if (extraFlags.negSrc1)
        buf[bufPos++] = '-';
    if (extraFlags.absSrc1)
    {
        buf[bufPos++] = 'a';
        buf[bufPos++] = 'b';
        buf[bufPos++] = 's';
        buf[bufPos++] = '(';
    }
    if (mode1 == GCN_DS1_SGPR || mode1 == GCN_SRC1_SGPR)
        bufPos += decodeGCNOperand(((insnCode>>9)&0xff),
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos, arch);
    else
        bufPos += decodeGCNVRegOperand(((insnCode>>9)&0xff),
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos);
    if (extraFlags.absSrc1)
        buf[bufPos++] = ')';
    if (extraFlags.sextSrc1)
        buf[bufPos++] = ')';
    if (mode1 == GCN_ARG2_IMM)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += printLiteral(buf+bufPos, literal, displayFloatLits);
    }
    else if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
    {
        ::memcpy(buf+bufPos, ", vcc", 5);
        bufPos += 5;
    }
    output.forward(bufPos);
    if (isGCN12)
    {
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, literal, true, true);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, true, true);
    }
}

static const char* vintrpParamsTbl[] =
{ "p10", "p20", "p0" };

static size_t decodeVINTRPParam(uint16_t p, char* buf)
{
    if (p > 3)
    {
        ::memcpy(buf, "invalid_", 8);
        return 8+itocstrCStyle(p, buf+8, 8);
    }
    else
    {
        const char* paramName = vintrpParamsTbl[p];
        size_t k = 0;
        for (k = 0; paramName[k] != 0; k++)
            buf[k] = paramName[k];
        return k;
    }
}

static void decodeVOP3Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2,
         FloatLitType displayFloatLits)
{
    char* buf = output.reserve(140);
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    size_t bufPos = 0;
    const cxuint opcode = (isGCN12) ? ((insnCode>>16)&0x3ff) : ((insnCode>>17)&0x1ff);
    
    const cxuint vdst = insnCode&0xff;
    const cxuint vsrc0 = insnCode2&0x1ff;
    const cxuint vsrc1 = (insnCode2>>9)&0x1ff;
    const cxuint vsrc2 = (insnCode2>>18)&0x1ff;
    const cxuint sdst = (insnCode>>8)&0x7f;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    const uint16_t vop3Mode = (gcnInsn.mode&GCN_VOP3_MASK2);
    
    bool vdstUsed = false;
    bool vsrc0Used = false;
    bool vsrc1Used = false;
    bool vsrc2Used = false;
    bool vsrc2CC = false;
    cxuint absFlags = 0;
    
    if (gcnInsn.encoding == GCNENC_VOP3A)
        absFlags = (insnCode>>8)&7;
    
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        bufPos = addSpaces(buf, spacesToAdd);
        
        if (opcode < 256 || (gcnInsn.mode&GCN_VOP3_DST_SGPR)!=0) /* if compares */
            bufPos += decodeGCNOperand(vdst,
                       ((gcnInsn.mode&GCN_VOP3_DST_SGPR)==0)?2:1, buf + bufPos, arch);
        else /* regular instruction */
            bufPos += decodeGCNVRegOperand(vdst,
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos);
        
        if (gcnInsn.encoding == GCNENC_VOP3B &&
            (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC || mode1 == GCN_DST_VCC_VSRC2 ||
             mode1 == GCN_S0EQS12)) /* VOP3b */
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += decodeGCNOperand(((insnCode>>8)&0x7f), 2, buf + bufPos, arch);
        }
        
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        
        if (vop3Mode != GCN_VOP3_VINTRP)
        {
            if ((insnCode2 & (1U<<29)) != 0)
                buf[bufPos++] = '-';
            if (absFlags & 1)
            {
                buf[bufPos++] = 'a';
                buf[bufPos++] = 'b';
                buf[bufPos++] = 's';
                buf[bufPos++] = '(';
            }
            bufPos += decodeGCNOperand(vsrc0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                                   buf + bufPos, arch, 0, displayFloatLits);
            if (absFlags & 1)
                buf[bufPos++] = ')';
            vsrc0Used = true;
        }
        
        if (mode1 != GCN_SRC12_NONE && (vop3Mode != GCN_VOP3_VINTRP))
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            if ((insnCode2 & (1U<<30)) != 0)
                buf[bufPos++] = '-';
            if (absFlags & 2)
            {
                buf[bufPos++] = 'a';
                buf[bufPos++] = 'b';
                buf[bufPos++] = 's';
                buf[bufPos++] = '(';
            }
            bufPos += decodeGCNOperand(vsrc1,
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos, arch, 0, displayFloatLits);
            if (absFlags & 2)
                buf[bufPos++] = ')';
            /* GCN_DST_VCC - only sdst is used, no vsrc2 */
            if (mode1 != GCN_SRC2_NONE && mode1 != GCN_DST_VCC && opcode >= 256)
            {
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
                
                if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
                {
                    bufPos += decodeGCNOperand(vsrc2, 2, buf + bufPos, arch);
                    vsrc2CC = true;
                }
                else
                {
                    if ((insnCode2 & (1U<<31)) != 0)
                        buf[bufPos++] = '-';
                    if (absFlags & 4)
                    {
                        buf[bufPos++] = 'a';
                        buf[bufPos++] = 'b';
                        buf[bufPos++] = 's';
                        buf[bufPos++] = '(';
                    }
                    bufPos += decodeGCNOperand(vsrc2,
                       (gcnInsn.mode&GCN_REG_SRC2_64)?2:1, buf + bufPos, arch, 0,
                               displayFloatLits);
                    if (absFlags & 4)
                        buf[bufPos++] = ')';
                }
                vsrc2Used = true;
            }
            vsrc1Used = true;
        }
        else if (vop3Mode == GCN_VOP3_VINTRP)
        {
            if ((insnCode2 & (1U<<30)) != 0)
                buf[bufPos++] = '-';
            if (absFlags & 2)
            {
                buf[bufPos++] = 'a';
                buf[bufPos++] = 'b';
                buf[bufPos++] = 's';
                buf[bufPos++] = '(';
            }
            if (mode1 == GCN_P0_P10_P20)
                bufPos += decodeVINTRPParam(vsrc1, buf+bufPos);
            else
                bufPos += decodeGCNOperand(vsrc1, 1, buf + bufPos, arch,
                                           0, displayFloatLits);
            if (absFlags & 2)
                buf[bufPos++] = ')';
            ::memcpy(buf+bufPos, ", attr", 6);
            bufPos += 6;
            const cxuint attr = vsrc0&63;
            bufPos += putByteToBuf(attr, buf+bufPos);
            buf[bufPos++] = '.';
            buf[bufPos++] = "xyzw"[((vsrc0>>6)&3)]; // attrchannel
            
            if ((gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2)
            {
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
                if ((insnCode2 & (1U<<31)) != 0)
                    buf[bufPos++] = '-';
                if (absFlags & 4)
                {
                    buf[bufPos++] = 'a';
                    buf[bufPos++] = 'b';
                    buf[bufPos++] = 's';
                    buf[bufPos++] = '(';
                }
                bufPos += decodeGCNOperand(vsrc2, 1, buf + bufPos, arch, 0,
                                   displayFloatLits);
                if (absFlags & 4)
                    buf[bufPos++] = ')';
                vsrc2Used = true;
            }
            
            if (vsrc0 & 0x100)
            {
                ::memcpy(buf+bufPos, " high", 5);
                bufPos += 5;
            }
            vsrc0Used = true;
            vsrc1Used = true;
        }
        vdstUsed = true;
    }
    else
        bufPos = addSpaces(buf, spacesToAdd-1);
    
    const cxuint omod = (insnCode2>>27)&3;
    if (omod != 0)
    {
        const char* omodStr = (omod==3)?" div:2":(omod==2)?" mul:4":" mul:2";
        ::memcpy(buf+bufPos, omodStr, 6);
        bufPos += 6;
    }
    
    const bool clamp = (!isGCN12 && gcnInsn.encoding == GCNENC_VOP3A &&
            (insnCode&0x800) != 0) || (isGCN12 && (insnCode&0x8000) != 0);
    if (clamp)
    {
        ::memcpy(buf+bufPos, " clamp", 6);
        bufPos += 6;
    }
    
    /* as field */
    if (!vdstUsed && vdst != 0)
    {
        ::memcpy(buf+bufPos, " dst=", 5);
        bufPos += 5;
        bufPos += itocstrCStyle(vdst, buf+bufPos, 6, 16);
    }
        
    if (!vsrc0Used)
    {
        if (vsrc0 != 0)
        {
            ::memcpy(buf+bufPos, " src0=", 6);
            bufPos += 6;
            bufPos += itocstrCStyle(vsrc0, buf+bufPos, 6, 16);
        }
        if (absFlags & 1)
        {
            ::memcpy(buf+bufPos, " abs0", 5);
            bufPos += 5;
        }
        if ((insnCode2 & (1U<<29)) != 0)
        {
            ::memcpy(buf+bufPos, " neg0", 5);
            bufPos += 5;
        }
    }
    if (!vsrc1Used)
    {
        if (vsrc1 != 0)
        {
            ::memcpy(buf+bufPos, " vsrc1=", 7);
            bufPos += 7;
            bufPos += itocstrCStyle(vsrc1, buf+bufPos, 6, 16);
        }
        if (absFlags & 2)
        {
            ::memcpy(buf+bufPos, " abs1", 5);
            bufPos += 5;
        }
        if ((insnCode2 & (1U<<30)) != 0)
        {
            ::memcpy(buf+bufPos, " neg1", 5);
            bufPos += 5;
        }
    }
    if (!vsrc2Used)
    {
        if (vsrc2 != 0)
        {
            ::memcpy(buf+bufPos, " vsrc2=", 7);
            bufPos += 7;
            bufPos += itocstrCStyle(vsrc2, buf+bufPos, 6, 16);
        }
        if (absFlags & 4)
        {
            ::memcpy(buf+bufPos, " abs2", 5);
            bufPos += 5;
        }
        if ((insnCode2 & (1U<<31)) != 0)
        {
            ::memcpy(buf+bufPos, " neg2", 5);
            bufPos += 5;
        }
    }
    
    const cxuint usedMask = 7 & ~(vsrc2CC?4:0);
    /* check whether instruction is this same like VOP2/VOP1/VOPC */
    bool isVOP1Word = false;
    if (vop3Mode == GCN_VOP3_VINTRP)
    {
        if (mode1 != GCN_NEW_OPCODE) /* check clamp and abs flags */
            isVOP1Word = !((insnCode&(7<<8)) != 0 || (insnCode2&(7<<29)) != 0 ||
                    clamp || ((vsrc1 < 256 && mode1!=GCN_P0_P10_P20) || (mode1==GCN_P0_P10_P20 && vsrc1 >= 256)) ||
                    vsrc0 >= 256 || vsrc2 != 0);
    }
    else if (mode1 != GCN_ARG1_IMM && mode1 != GCN_ARG2_IMM)
    {
        const bool reqForVOP1Word = omod==0 && ((insnCode2&(usedMask<<29)) == 0);
        if (gcnInsn.encoding != GCNENC_VOP3B)
        {   /* for VOPC */
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
                ((mode1 != GCN_SRC2_VCC && vsrc2 == 0) || vsrc2 == 106))
                isVOP1Word = true;
            /* check clamp and abs and neg flags */
            if ((insnCode&(usedMask<<8)) != 0 || (insnCode2&(usedMask<<29)) != 0 || clamp)
                isVOP1Word = false;
        }
        /* for VOP2 encoded as VOP3b (v_addc....) */
        else if (gcnInsn.encoding == GCNENC_VOP3B && vop3Mode == GCN_VOP3_VOP2 &&
                vsrc1 >= 256 && sdst == 106 /* vcc */ &&
                ((vsrc2 == 106 && mode1 == GCN_DS2_VCC) || vsrc2 == 0)) /* VOP3b */
            isVOP1Word = true;
        
        if (isVOP1Word && !reqForVOP1Word)
            isVOP1Word = false;
    }
    else // force for v_madmk_f32 and v_madak_f32
        isVOP1Word = true;
    
    if (isVOP1Word) // add vop3 for distinguishing encoding
    {
        ::memcpy(buf+bufPos, " vop3", 5);
        bufPos += 5;
    }
    
    output.forward(bufPos);
}

static void decodeVINTRPEncoding(cxuint spacesToAdd, uint16_t arch,
           FastOutputBuffer& output, const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    char* buf = output.reserve(80);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bufPos += decodeGCNVRegOperand((insnCode>>18)&0xff, 1, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    if ((gcnInsn.mode & GCN_MASK1) == GCN_P0_P10_P20)
        bufPos += decodeVINTRPParam(insnCode&0xff, buf+bufPos);
    else
        bufPos += decodeGCNVRegOperand(insnCode&0xff, 1, buf+bufPos);
    ::memcpy(buf+bufPos, ", attr", 6);
    bufPos += 6;
    const cxuint attr = (insnCode>>10)&63;
    bufPos += putByteToBuf(attr, buf+bufPos);
    buf[bufPos++] = '.';
    buf[bufPos++] = "xyzw"[((insnCode>>8)&3)]; // attrchannel
    output.forward(bufPos);
}

static void decodeDSEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
           const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* buf = output.reserve(90);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bool vdstUsed = false;
    bool vaddrUsed = false;
    bool vdata0Used = false;
    bool vdata1Used = false;
    const cxuint vaddr = insnCode2&0xff;
    const cxuint vdata0 = (insnCode2>>8)&0xff;
    const cxuint vdata1 = (insnCode2>>16)&0xff;
    const cxuint vdst = insnCode2>>24;
    
    if ((gcnInsn.mode & GCN_ADDR_SRC) != 0 || (gcnInsn.mode & GCN_ONLYDST) != 0)
    {   /* vdst is dst */
        cxuint regsNum = (gcnInsn.mode&GCN_REG_DST_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        bufPos += decodeGCNVRegOperand(vdst, regsNum, buf+bufPos);
        vdstUsed = true;
    }
    if ((gcnInsn.mode & GCN_ONLYDST) == 0)
    {
        if (vdstUsed)
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
        }
        bufPos += decodeGCNVRegOperand(vaddr, 1, buf+bufPos);
        vaddrUsed = true;
    }
    
    const uint16_t srcMode = (gcnInsn.mode & GCN_SRCS_MASK);
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 &&
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) != 0 && srcMode != GCN_NOSRC)
    {   /* two vdata */
        if (vaddrUsed || vdstUsed)
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
        }
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        bufPos += decodeGCNVRegOperand(vdata0, regsNum, buf+bufPos);
        vdata0Used = true;
        if (srcMode == GCN_2SRCS)
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += decodeGCNVRegOperand(vdata1,
                (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf+bufPos);
            vdata1Used = true;
        }
    }
    
    const cxuint offset = (insnCode&0xffff);
    if (offset != 0)
    {
        if ((gcnInsn.mode & GCN_2OFFSETS) == 0) /* single offset */
        {
            ::memcpy(buf+bufPos, " offset:", 8);
            bufPos += 8;
            bufPos += itocstrCStyle(offset, buf+bufPos, 7, 10);
        }
        else
        {
            if ((offset&0xff) != 0)
            {
                ::memcpy(buf+bufPos, " offset0:", 9);
                bufPos += 9;
                bufPos += putByteToBuf(offset&0xff, buf+bufPos);
            }
            if ((offset&0xff00) != 0)
            {
                ::memcpy(buf+bufPos, " offset1:", 9);
                bufPos += 9;
                bufPos += putByteToBuf((offset>>8)&0xff, buf+bufPos);
            }
        }
    }
    
    if (insnCode&0x20000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'g';
        buf[bufPos++] = 'd';
        buf[bufPos++] = 's';
    }
    
    if (!vaddrUsed && vaddr != 0)
    {
        ::memcpy(buf+bufPos, " vaddr=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle(vaddr, buf+bufPos, 6, 16);
    }
    if (!vdata0Used && vdata0 != 0)
    {
        ::memcpy(buf+bufPos, " vdata0=", 8);
        bufPos += 8;
        bufPos += itocstrCStyle(vdata0, buf+bufPos, 6, 16);
    }
    if (!vdata1Used && vdata1 != 0)
    {
        ::memcpy(buf+bufPos, " vdata1=", 8);
        bufPos += 8;
        bufPos += itocstrCStyle(vdata1, buf+bufPos, 6, 16);
    }
    if (!vdstUsed && vdst != 0)
    {
        ::memcpy(buf+bufPos, " vdst=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(vdst, buf+bufPos, 6, 16);
    }
    output.forward(bufPos);
}

static const char* mtbufDFMTTable[] =
{
    "invalid", "8", "16", "8_8", "32", "16_16", "10_11_11", "11_11_10",
    "10_10_10_2", "2_10_10_10", "8_8_8_8", "32_32", "16_16_16_16", "32_32_32",
    "32_32_32_32", "reserved"
};

static const char* mtbufNFMTTable[] =
{
    "unorm", "snorm", "uscaled", "sscaled",
    "uint", "sint", "snorm_ogl", "float"
};

static void decodeMUBUFEncoding(cxuint spacesToAdd, uint16_t arch,
      FastOutputBuffer& output, const GCNInstruction& gcnInsn, uint32_t insnCode,
      uint32_t insnCode2, bool mtbuf)
{
    char* buf = output.reserve(150);
    size_t bufPos = 0;
    const cxuint vaddr = insnCode2&0xff;
    const cxuint vdata = (insnCode2>>8)&0xff;
    const cxuint srsrc = (insnCode2>>16)&0x1f;
    const cxuint soffset = insnCode2>>24;
    if ((gcnInsn.mode & GCN_MASK1) != GCN_ARG_NONE)
    {
        bufPos = addSpaces(buf, spacesToAdd);
        cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
        if (insnCode2 & 0x800000U)
            dregsNum++; // tfe
        
        bufPos += decodeGCNVRegOperand(vdata, dregsNum, buf+bufPos);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        // determine number of vaddr registers
        /* for addr32 - idxen+offen or 1, for addr64 - 2 (idxen and offen is illegal) */
        const cxuint aregsNum = ((insnCode & 0x3000U)==0x3000U ||
                (insnCode & 0x8000U))? 2 : 1;
        
        bufPos += decodeGCNVRegOperand(vaddr, aregsNum, buf+bufPos);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNOperand(srsrc<<2, 4, buf+bufPos, arch);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNOperand(soffset, 1, buf+bufPos, arch);
    }
    else
        bufPos = addSpaces(buf, spacesToAdd-1);
    
    if (insnCode & 0x1000U)
    {
        ::memcpy(buf+bufPos, " offen", 6);
        bufPos += 6;
    }
    if (insnCode & 0x2000U)
    {
        ::memcpy(buf+bufPos, " idxen", 6);
        bufPos += 6;
    }
    const cxuint offset = insnCode&0xfff;
    if (offset != 0)
    {
        ::memcpy(buf+bufPos, " offset:", 8);
        bufPos += 8;
        bufPos += itocstrCStyle(offset, buf+bufPos, 7, 10);
    }
    if (insnCode & 0x4000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'g';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    if (insnCode2 & 0x400000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 's';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    if (insnCode & 0x8000U)
    {
        ::memcpy(buf+bufPos, " addr64", 7);
        bufPos += 7;
    }
    if (!mtbuf && (insnCode & 0x10000U) != 0)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'd';
        buf[bufPos++] = 's';
    }
    if (insnCode2 & 0x800000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 't';
        buf[bufPos++] = 'f';
        buf[bufPos++] = 'e';
    }
    if (mtbuf)
    {
        ::memcpy(buf+bufPos, " format:[", 9);
        bufPos += 9;
        const char* dfmtStr = mtbufDFMTTable[(insnCode>>19)&15];
        for (cxuint k = 0; dfmtStr[k] != 0; k++)
            buf[bufPos++] = dfmtStr[k];
        buf[bufPos++] = ',';
        const char* nfmtStr = mtbufNFMTTable[(insnCode>>23)&7];
        for (cxuint k = 0; nfmtStr[k] != 0; k++)
            buf[bufPos++] = nfmtStr[k];
        buf[bufPos++] = ']';
    }
    
    if ((gcnInsn.mode & GCN_MASK1) == GCN_ARG_NONE)
    {
        if (vaddr != 0)
        {
            ::memcpy(buf+bufPos, " vaddr=", 7);
            bufPos += 7;
            bufPos += itocstrCStyle(vaddr, buf+bufPos, 6, 16);
        }
        if (vdata != 0)
        {
            ::memcpy(buf+bufPos, " vdata=", 7);
            bufPos += 7;
            bufPos += itocstrCStyle(vdata, buf+bufPos, 6, 16);
        }
        if (srsrc != 0)
        {
            ::memcpy(buf+bufPos, " srsrc=", 7);
            bufPos += 7;
            bufPos += itocstrCStyle(srsrc, buf+bufPos, 6, 16);
        }
        if (soffset != 0)
        {
            ::memcpy(buf+bufPos, " soffset=", 9);
            bufPos += 9;
            bufPos += itocstrCStyle(soffset, buf+bufPos, 6, 16);
        }
    }
    output.forward(bufPos);
}

static void decodeMIMGEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* buf = output.reserve(150);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    
    const cxuint dmask =  (insnCode>>8)&15;
    cxuint dregsNum = ((dmask & 1)?1:0) + ((dmask & 2)?1:0) + ((dmask & 4)?1:0) +
            ((dmask & 8)?1:0);
    dregsNum = (dregsNum == 0) ? 1 : dregsNum;
    if (insnCode & 0x10000)
        dregsNum++; // tfe
    
    bufPos += decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    // determine number of vaddr registers
    bufPos += decodeGCNVRegOperand(insnCode2&0xff, 4, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand(((insnCode2>>14)&0x7c),
                   (insnCode & 0x8000)?4:8, buf+bufPos, arch);
    
    const cxuint ssamp = (insnCode2>>21)&0x1f;
    
    if ((gcnInsn.mode & GCN_MASK2) == GCN_MIMG_SAMPLE)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNOperand(ssamp<<2, 4, buf+bufPos, arch);
    }
    if (dmask != 1)
    {
        ::memcpy(buf+bufPos, " dmask:", 7);
        bufPos += 7;
        if (dmask >= 10)
        {
            buf[bufPos++] = '1';
            buf[bufPos++] = '0' + dmask - 10;
        }
        else
            buf[bufPos++] = '0' + dmask;
    }
    
    if (insnCode & 0x1000)
    {
        ::memcpy(buf+bufPos, " unorm", 6);
        bufPos += 6;
    }
    if (insnCode & 0x2000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'g';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    if (insnCode & 0x2000000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 's';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    if (insnCode & 0x8000)
    {
        ::memcpy(buf+bufPos, " r128", 5);
        bufPos += 5;
    }
    if (insnCode & 0x10000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 't';
        buf[bufPos++] = 'f';
        buf[bufPos++] = 'e';
    }
    if (insnCode & 0x20000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'w';
        buf[bufPos++] = 'e';
    }
    if (insnCode & 0x4000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'd';
        buf[bufPos++] = 'a';
    }
    
    if ((gcnInsn.mode & GCN_MASK2) != GCN_MIMG_SAMPLE && ssamp != 0)
    {
        ::memcpy(buf+bufPos, " ssamp=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle(ssamp, buf+bufPos, 6, 16);
    }
    output.forward(bufPos);
}

static void decodeEXPEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
        const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* buf = output.reserve(90);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    /* export target */
    const cxuint target = (insnCode>>4)&63;
    if (target >= 32)
    {
        ::memcpy(buf+bufPos, "param", 5);
        bufPos += 5;
        const cxuint tpar = target-32;
        const cxuint digit2 = tpar/10;
        if (digit2 != 0)
            buf[bufPos++] = '0' + digit2;
        buf[bufPos++] = '0' + tpar - 10*digit2;
    }
    else if (target >= 12 && target <= 15)
    {
        buf[bufPos++] = 'p';
        buf[bufPos++] = 'o';
        buf[bufPos++] = 's';
        buf[bufPos++] = '0' + target-12;
    }
    else if (target < 8)
    {
        buf[bufPos++] = 'm';
        buf[bufPos++] = 'r';
        buf[bufPos++] = 't';
        buf[bufPos++] = '0' + target;
    }
    else if (target == 8)
    {
        buf[bufPos++] = 'm';
        buf[bufPos++] = 'r';
        buf[bufPos++] = 't';
        buf[bufPos++] = 'z';
    }
    else if (target == 9)
    {
        buf[bufPos++] = 'n';
        buf[bufPos++] = 'u';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'l';
    }
    else
    {   /* reserved */
        buf[bufPos++] = 'i';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'l';
        buf[bufPos++] = '_';
        const cxuint digit2 = target/10U;
        buf[bufPos++] = '0' + digit2;
        buf[bufPos++] = '0' + target - 10U*digit2;
    }
    
    /* vdata registers */
    cxuint vsrcsUsed = 0;
    for (cxuint i = 0; i < 4; i++)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        if (insnCode & (1U<<i))
        {
            if ((insnCode&0x400)==0)
            {
                bufPos += decodeGCNVRegOperand((insnCode2>>(i<<3))&0xff, 1, buf+bufPos);
                vsrcsUsed |= 1<<i;
            }
            else // if compr=1
            {
                bufPos += decodeGCNVRegOperand(((i>=2)?(insnCode2>>8):insnCode2)&0xff,
                               1, buf+bufPos);
                vsrcsUsed |= 1U<<(i>>1);
            }
        }
        else
        {
            buf[bufPos++] = 'o';
            buf[bufPos++] = 'f';
            buf[bufPos++] = 'f';
        }
    }
    
    if (insnCode&0x800)
    {
        ::memcpy(buf+bufPos, " done", 5);
        bufPos += 5;
    }
    if (insnCode&0x400)
    {
        ::memcpy(buf+bufPos, " compr", 6);
        bufPos += 6;
    }
    if (insnCode&0x1000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'v';
        buf[bufPos++] = 'm';
    }
    
    for (cxuint i = 0; i < 4; i++)
    {
        const cxuint val = (insnCode2>>(i<<3))&0xff;
        if ((vsrcsUsed&(1U<<i))==0 && val!=0)
        {
            ::memcpy(buf+bufPos, " vsrc0=", 7);
            buf[bufPos+5] += i; // number
            bufPos += 7;
            bufPos += itocstrCStyle(val, buf+bufPos, 6, 16);
        }
    }
    output.forward(bufPos);
}

static void decodeFLATEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* buf = output.reserve(130);
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bool vdstUsed = false;
    bool vdataUsed = false;
    const cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
    // tfe
    const cxuint dstRegsNum = (insnCode2 & 0x800000U)?dregsNum+1:dregsNum;
    
    if ((gcnInsn.mode & GCN_FLAT_ADST) == 0)
    {
        vdstUsed = true;
        bufPos += decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, buf+bufPos);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNVRegOperand(insnCode2&0xff, 2, buf+bufPos); // addr
    }
    else
    {   /* two vregs, because 64-bitness stored in PTR32 mode (at runtime) */
        bufPos += decodeGCNVRegOperand(insnCode2&0xff, 2, buf+bufPos); // addr
        if ((gcnInsn.mode & GCN_FLAT_NODST) == 0)
        {
            vdstUsed = true;
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, buf+bufPos);
        }
    }
    
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0) /* print data */
    {
        vdataUsed = true;
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, buf+bufPos);
    }
    
    if (insnCode & 0x10000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'g';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    if (insnCode & 0x20000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 's';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    if (insnCode2 & 0x800000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 't';
        buf[bufPos++] = 'f';
        buf[bufPos++] = 'e';
    }
    
    if (!vdataUsed && ((insnCode2>>8)&0xff) != 0)
    {
        ::memcpy(buf+bufPos, " vdata=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle((insnCode2>>8)&0xff, buf+bufPos, 6, 16);
    }
    if (!vdstUsed && (insnCode2>>24) != 0)
    {
        ::memcpy(buf+bufPos, " vdst=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(insnCode2>>24, buf+bufPos, 6, 16);
    }
    output.forward(bufPos);
}

/* main routine */

void GCNDisassembler::disassemble()
{
    LabelIter curLabel = labels.begin();
    NamedLabelIter curNamedLabel = namedLabels.begin();
    const uint32_t* codeWords = reinterpret_cast<const uint32_t*>(input);

    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(
                disassembler.getDeviceType());
    const bool isGCN11 = (arch == GPUArchitecture::GCN1_1);
    const bool isGCN12 = (arch == GPUArchitecture::GCN1_2);
    const uint16_t curArchMask = 
            1U<<int(getGPUArchitectureFromDeviceType(disassembler.getDeviceType()));
    const size_t codeWordsNum = (inputSize>>2);
    
    if ((inputSize&3) != 0)
        output.write(64,
           "        /* WARNING: Code size is not aligned to 4-byte word! */\n");
    if (instrOutOfCode)
        output.write(54, "        /* WARNING: Unfinished instruction at end! */\n");
    
    bool prevIsTwoWord = false;
    
    size_t pos = 0;
    while (true)
    {
        writeLabelsToPosition(pos<<2, curLabel, curNamedLabel);
        if (pos >= codeWordsNum)
            break;
        
        const size_t oldPos = pos;
        cxbyte gcnEncoding = GCNENC_NONE;
        const uint32_t insnCode = ULEV(codeWords[pos++]);
        uint32_t insnCode2 = 0;
        
        /* determine GCN encoding */
        if ((insnCode & 0x80000000U) != 0)
        {
            if ((insnCode & 0x40000000U) == 0)
            {   // SOP???
                if  ((insnCode & 0x30000000U) == 0x30000000U)
                {   // SOP1/SOPK/SOPC/SOPP
                    const uint32_t encPart = (insnCode & 0x0f800000U);
                    if (encPart == 0x0e800000U)
                    {   // SOP1
                        if ((insnCode&0xff) == 0xff) // literal
                        {
                            if (pos < codeWordsNum)
                                insnCode2 = ULEV(codeWords[pos++]);
                        }
                        gcnEncoding = GCNENC_SOP1;
                    }
                    else if (encPart == 0x0f000000U)
                    {   // SOPC
                        if ((insnCode&0xff) == 0xff ||
                            (insnCode&0xff00) == 0xff00) // literal
                        {
                            if (pos < codeWordsNum)
                                insnCode2 = ULEV(codeWords[pos++]);
                        }
                        gcnEncoding = GCNENC_SOPC;
                    }
                    else if (encPart == 0x0f800000U) // SOPP
                        gcnEncoding = GCNENC_SOPP;
                    else // SOPK
                    {
                        gcnEncoding = GCNENC_SOPK;
                        const uint32_t opcode = ((insnCode>>23)&0x1f);
                        if ((!isGCN12 && opcode == 21) ||
                            (isGCN12 && opcode == 20))
                        {
                            if (pos < codeWordsNum)
                                insnCode2 = ULEV(codeWords[pos++]);
                        }
                    }
                }
                else
                {   // SOP2
                    if ((insnCode&0xff) == 0xff || (insnCode&0xff00) == 0xff00)
                    {   // literal
                        if (pos < codeWordsNum)
                            insnCode2 = ULEV(codeWords[pos++]);
                    }
                    gcnEncoding = GCNENC_SOP2;
                }
            }
            else
            {   // SMRD and others
                const uint32_t encPart = (insnCode&0x3c000000U)>>26;
                if ((!isGCN12 && gcnSize11Table[encPart] && (encPart != 7 || isGCN11)) ||
                    (isGCN12 && gcnSize12Table[encPart]))
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                if (isGCN12)
                    gcnEncoding = gcnEncoding12Table[encPart];
                else
                    gcnEncoding = gcnEncoding11Table[encPart];
                if (gcnEncoding == GCNENC_FLAT && !isGCN11 && !isGCN12)
                    gcnEncoding = GCNENC_NONE; // illegal if not GCN1.1
            }
        }
        else
        {   // some vector instructions
            if ((insnCode & 0x7e000000U) == 0x7c000000U)
            {   // VOPC
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOPC;
            }
            else if ((insnCode & 0x7e000000U) == 0x7e000000U)
            {   // VOP1
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOP1;
            }
            else
            {   // VOP2
                const cxuint opcode = (insnCode >> 25)&0x3f;
                if ((!isGCN12 && (opcode == 32 || opcode == 33)) ||
                    (isGCN12 && (opcode == 23 || opcode == 24 ||
                    opcode == 36 || opcode == 37))) // V_MADMK and V_MADAK
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                else if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOP2;
            }
        }
        
        prevIsTwoWord = (oldPos+2 == pos);
        
        if (disassembler.getFlags() & DISASM_HEXCODE)
        {
            char* buf = output.reserve(50);
            size_t bufPos = 0;
            buf[bufPos++] = '/';
            buf[bufPos++] = '*';
            bufPos += itocstrCStyle(insnCode, buf+bufPos, 12, 16, 8, false);
            buf[bufPos++] = ' ';
            if (prevIsTwoWord)
                bufPos += itocstrCStyle(insnCode2, buf+bufPos, 12, 16, 8, false);
            else
                bufPos += addSpaces(buf+bufPos, 8);
            buf[bufPos++] = '*';
            buf[bufPos++] = '/';
            buf[bufPos++] = ' ';
            output.forward(bufPos);
        }
        else // add spaces
        {
            char* buf = output.reserve(8);
            output.forward(addSpaces(buf, 8));
        }
        
        if (gcnEncoding == GCNENC_NONE)
        {   // invalid encoding
            char* buf = output.reserve(24);
            size_t bufPos = 0;
            buf[bufPos++] = '.';
            buf[bufPos++] = 'i';
            buf[bufPos++] = 'n';
            buf[bufPos++] = 't';
            buf[bufPos++] = ' '; 
            bufPos += itocstrCStyle(insnCode, buf+bufPos, 11, 16);
            output.forward(bufPos);
        }
        else
        {
            const GCNEncodingOpcodeBits* encodingOpcodeTable = 
                    (isGCN12) ? gcnEncodingOpcode12Table : gcnEncodingOpcodeTable;
            const cxuint opcode =
                    (insnCode>>encodingOpcodeTable[gcnEncoding].bitPos) & 
                    ((1U<<encodingOpcodeTable[gcnEncoding].bits)-1U);
            
            /* decode instruction and put to output */
            const GCNEncodingSpace& encSpace = 
                (isGCN12) ? gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+3 + gcnEncoding] :
                  gcnInstrTableByCodeSpaces[gcnEncoding];
            const GCNInstruction* gcnInsn = gcnInstrTableByCode.get() +
                    encSpace.offset + opcode;
            
            const GCNInstruction defaultInsn = { nullptr, gcnInsn->encoding, GCN_STDMODE,
                        0, 0 };
            cxuint spacesToAdd = 16;
            bool isIllegal = false;
            if (!isGCN12 && gcnInsn->mnemonic != nullptr &&
                (curArchMask & gcnInsn->archMask) == 0 &&
                gcnEncoding == GCNENC_VOP3A)
            {    /* new overrides */
                const GCNEncodingSpace& encSpace2 =
                        gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+1];
                gcnInsn = gcnInstrTableByCode.get() + encSpace2.offset + opcode;
                if (gcnInsn->mnemonic == nullptr ||
                        (curArchMask & gcnInsn->archMask) == 0)
                        // illegal
                    isIllegal = true;
            }
            else if (gcnInsn->mnemonic == nullptr ||
                (curArchMask & gcnInsn->archMask) == 0)
                isIllegal = true;
            
            if (!isIllegal)
            {
                size_t k = ::strlen(gcnInsn->mnemonic);
                output.writeString(gcnInsn->mnemonic);
                spacesToAdd = spacesToAdd>=k+1?spacesToAdd-k:1;
            }
            else
            {
                char* buf = output.reserve(40);
                size_t bufPos = 0;
                if (!isGCN12 || gcnEncoding != GCNENC_SMEM)
                    for (cxuint k = 0; gcnEncodingNames[gcnEncoding][k] != 0; k++)
                        buf[bufPos++] = gcnEncodingNames[gcnEncoding][k];
                else
                {   /* SMEM encoding */
                    ::memcpy(buf+bufPos, "SMEM", 4);
                    bufPos+=4;
                }
                buf[bufPos++] = '_';
                buf[bufPos++] = 'i';
                buf[bufPos++] = 'l';
                buf[bufPos++] = 'l';
                buf[bufPos++] = '_';
                bufPos += itocstrCStyle(opcode, buf + bufPos, 6);
                spacesToAdd = spacesToAdd >= (bufPos+1)? spacesToAdd - (bufPos) : 1;
                gcnInsn = &defaultInsn;
                output.forward(bufPos);
            }
            
            const FloatLitType displayFloatLits = 
                    ((disassembler.getFlags()&DISASM_FLOATLITS) != 0) ?
                    (((gcnInsn->mode & GCN_MASK2) == GCN_FLOATLIT) ? FLTLIT_F32 : 
                    ((gcnInsn->mode & GCN_MASK2) == GCN_F16LIT) ? FLTLIT_F16 :
                     FLTLIT_NONE) : FLTLIT_NONE;
            
            switch(gcnEncoding)
            {
                case GCNENC_SOPC:
                    decodeSOPCEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2);
                    break;
                case GCNENC_SOPP:
                    decodeSOPPEncoding(spacesToAdd, curArchMask, output,
                             [this](size_t myPos)
                             { this->writeLocation(myPos); },
                                 *gcnInsn, insnCode, insnCode2, pos);
                    break;
                case GCNENC_SOP1:
                    decodeSOP1Encoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2);
                    break;
                case GCNENC_SOP2:
                    decodeSOP2Encoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2);
                    break;
                case GCNENC_SOPK:
                    decodeSOPKEncoding(spacesToAdd, curArchMask, output,
                               [this](size_t myPos)
                               { this->writeLocation(myPos); },
                               *gcnInsn, insnCode, insnCode2, pos);
                    break;
                case GCNENC_SMRD:
                    if (isGCN12)
                        decodeSMEMEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                     insnCode, insnCode2);
                    else
                        decodeSMRDEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                     insnCode);
                    break;
                case GCNENC_VOPC:
                    decodeVOPCEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VOP1:
                    decodeVOP1Encoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VOP2:
                    decodeVOP2Encoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VOP3A:
                    decodeVOP3Encoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VINTRP:
                    decodeVINTRPEncoding(spacesToAdd, curArchMask, output,
                                 *gcnInsn, insnCode);
                    break;
                case GCNENC_DS:
                    decodeDSEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2);
                    break;
                case GCNENC_MUBUF:
                    decodeMUBUFEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2, false);
                    break;
                case GCNENC_MTBUF:
                    decodeMUBUFEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2, true);
                    break;
                case GCNENC_MIMG:
                    decodeMIMGEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2);
                    break;
                case GCNENC_EXP:
                    decodeEXPEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2);
                    break;
                case GCNENC_FLAT:
                    decodeFLATEncoding(spacesToAdd, curArchMask, output, *gcnInsn,
                                 insnCode, insnCode2);
                    break;
                default:
                    break;
            }
        }
        output.put('\n');
    }
    writeLabelsToEnd(codeWordsNum<<2, curLabel, curNamedLabel);
    output.flush();
    disassembler.getOutput().flush();
    labels.clear(); // free labels
}
