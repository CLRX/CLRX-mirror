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
    { 0x1822, 0x100 } /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
};

static const size_t gcnInstrTableByCodeLength = 0x1922;

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
    { 17, 8 }, /* GCNENC_DS, opcode = (8bit)<<17 */
    { 18, 7 }, /* GCNENC_MUBUF, opcode = (7bit)<<18 */
    { 15, 4 }, /* GCNENC_MTBUF, opcode = (4bit)<<15 */
    { 18, 7 }, /* GCNENC_MIMG, opcode = (7bit)<<18 */
    { 0, 0 }, /* GCNENC_EXP, opcode = none */
    { 18, 8 } /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
};

static inline void putChars(char*& buf, const char* input, size_t size)
{
    ::memcpy(buf, input, size);
    buf += size;
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

static inline void putByteToBuf(cxuint op, char*& buf)
{
    cxuint val = op;
    if (val >= 100U)
    {
        cxuint digit2 = val/100U;
        *buf++ = digit2+'0';
        val -= digit2*100U;
    }
    {
        const cxuint digit1 = val/10U;
        if (digit1 != 0 || op >= 100U)
            *buf++ = digit1 + '0';
        *buf++ = val-digit1*10U + '0';
    }
}

static inline void putHexByteToBuf(cxuint op, char*& buf)
{
    const cxuint digit0 = op&0xf;
    const cxuint digit1 = (op&0xf0)>>4;
    *buf++ = '0';
    *buf++ = 'x';
    if (digit1 != 0)
        *buf++ = (digit1<=9)?'0'+digit1:'a'+digit1-10;
    *buf++ = (digit0<=9)?'0'+digit0:'a'+digit0-10;
}

static void regRanges(cxuint op, cxuint vregNum, char*& buf)
{
    if (vregNum!=1)
        *buf++ = '[';
    putByteToBuf(op, buf);
    if (vregNum!=1)
    {
        *buf++ = ':';
        op += vregNum-1;
        if (op > 255)
            op -= 256; // fix for VREGS
        putByteToBuf(op, buf);
        *buf++ = ']';
    }
}

static void decodeGCNVRegOperand(cxuint op, cxuint vregNum, char*& buf)
{
    *buf++ = 'v';
    return regRanges(op, vregNum, buf);
}

enum FloatLitType: cxbyte
{
    FLTLIT_NONE,
    FLTLIT_F32,
    FLTLIT_F16,
};

static void printLiteral(char*& buf, uint32_t literal, FloatLitType floatLit)
{
    buf += itocstrCStyle(literal, buf, 11, 16);
    if (floatLit != FLTLIT_NONE)
    {
        FloatUnion fu;
        fu.u = literal;
        putChars(buf, " /* ", 4);
        if (floatLit == FLTLIT_F32)
            buf += ftocstrCStyle(fu.f, buf, 20);
        else
            buf += htocstrCStyle(fu.u, buf, 20);
        *buf++ = (floatLit == FLTLIT_F32)?'f':'h';
        putChars(buf, " */", 3);
    }
}

static void decodeGCNOperand(cxuint op, cxuint regNum, char*& buf, uint16_t arch,
           uint32_t literal = 0, FloatLitType floatLit = FLTLIT_NONE)
{
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    if ((!isGCN12 && op < 104) || (isGCN12 && op < 102) || (op >= 256 && op < 512))
    {   // scalar
        if (op >= 256)
        {
            *buf++ = 'v';
            op -= 256;
        }
        else
            *buf++ = 's';
        regRanges(op, regNum, buf);
        return;
    }
    
    const cxuint op2 = op&~1U;
    if (op2 == 106 || op2 == 108 || op2 == 110 || op2 == 126 ||
        (op2 == 104 && (arch&ARCH_RX2X0)!=0) ||
        ((op2 == 102 || op2 == 104) && isGCN12))
    {   // VCC
        switch(op2)
        {
            case 102:
                putChars(buf, "flat_scratch", 12);
                break;
            case 104:
                if (isGCN12) // GCN1.2
                    putChars(buf, "xnack_mask", 10);
                else // GCN1.1
                    putChars(buf, "flat_scratch", 12);
                break;
            case 106:
                *buf++ = 'v';
                *buf++ = 'c';
                *buf++ = 'c';
                break;
            case 108:
                *buf++ = 't';
                *buf++ = 'b';
                *buf++ = 'a';
                break;
            case 110:
                *buf++ = 't';
                *buf++ = 'm';
                *buf++ = 'a';
                break;
            case 126:
                putChars(buf, "exec", 4);
                break;
        }
        if (regNum >= 2)
        {
            if (op&1) // unaligned!!
            {
                *buf++ = '_';
                *buf++ = 'u';
                *buf++ = '!';
            }
            if (regNum > 2)
                putChars(buf, "&ill!", 5);
            return;
        }
        *buf++ = '_';
        if ((op&1) == 0)
        { *buf++ = 'l'; *buf++ = 'o'; }
        else
        { *buf++ = 'h'; *buf++ = 'i'; }
        return;
    }
    
    if (op == 255) // if literal
    {
        printLiteral(buf, literal, floatLit);
        return;
    }
    
    if (op >= 112 && op < 124)
    {
        putChars(buf, "ttmp", 4);
        regRanges(op-112, regNum, buf);
        return;
    }
    if (op == 124)
    {
        *buf++ = 'm';
        *buf++ = '0';
        if (regNum > 1)
            putChars(buf, "&ill!", 5);
        return;
    }
    
    if (op >= 128 && op <= 192)
    {   // integer constant
        op -= 128;
        const cxuint digit1 = op/10U;
        if (digit1 != 0)
            *buf++ = digit1 + '0';
        *buf++ = op-digit1*10U + '0';
        return;
    }
    if (op > 192 && op <= 208)
    {
        *buf++ = '-';
        if (op >= 202)
        {
            *buf++ = '1';
            *buf++ = op-202+'0';
        }
        else
            *buf++ = op-192+'0';
        return;
    }
    if (op >= 240 && op < 248)
    {
        const char* inOp = gcnOperandFloatTable[op-240];
        putChars(buf, inOp, ::strlen(inOp));
        return;
    }
    
    switch(op)
    {
        case 248:
            if (isGCN12)
            {   // 1/(2*PI)
                putChars(buf, "0.15915494", 10);
                return;
            }
            break;
        case 251:
            putChars(buf, "vccz", 4);
            return;
        case 252:
            putChars(buf, "execz", 5);
            return;
        case 253:
            *buf++ = 's';
            *buf++ = 'c';
            *buf++ = 'c';
            return;
        case 254:
            *buf++ = 'l';
            *buf++ = 'd';
            *buf++ = 's';
            return;
    }
    
    // reserved value (illegal)
    putChars(buf, "ill_", 4);
    *buf++ = '0'+op/100U;
    *buf++ = '0'+(op/10U)%10U;
    *buf++ = '0'+op%10U;
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

static void addSpaces(char*& buf, cxuint spacesToAdd)
{
    for (cxuint k = spacesToAdd; k>0; k--)
        *buf++ = ' ';
}

static size_t addSpacesOld(char* buf, cxuint spacesToAdd)
{
    for (cxuint k = spacesToAdd; k>0; k--)
        *buf++ = ' ';
    return spacesToAdd;
}

static void decodeSOPCEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    char* bufStart = output.reserve(80);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    decodeGCNOperand(insnCode&0xff, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf, arch, literal);
    *buf++ = ',';
    *buf++ = ' ';
    if ((gcnInsn.mode & GCN_SRC1_IMM) != 0)
        putHexByteToBuf((insnCode>>8)&0xff, buf);
    else
        decodeGCNOperand((insnCode>>8)&0xff,
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf, arch, literal);
    output.forward(buf-bufStart);
}


template<typename LabelWriter>
static void decodeSOPPEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         LabelWriter labelWriter, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t literal, size_t pos)
{
    char* bufStart = output.reserve(60);
    char* buf = bufStart;
    const cxuint imm16 = insnCode&0xffff;
    switch(gcnInsn.mode&GCN_MASK1)
    {
        case GCN_IMM_REL:
        {
            const size_t branchPos = (pos + int16_t(imm16))<<2;
            addSpaces(buf, spacesToAdd);
            output.forward(buf-bufStart);
            labelWriter(branchPos);
            return;
        }
        case GCN_IMM_LOCKS:
        {
            bool prevLock = false;
            addSpaces(buf, spacesToAdd);
            if ((imm16&15) != 15)
            {
                const cxuint lockCnt = imm16&15;
                putChars(buf, "vmcnt(", 6);
                if (lockCnt >= 10)
                    *buf++ = '1';
                *buf++ = '0' + ((lockCnt>=10)?lockCnt-10:lockCnt);
                *buf++ = ')';
                prevLock = true;
            }
            if (((imm16>>4)&7) != 7)
            {
                if (prevLock)
                {
                    *buf++ = ' ';
                    *buf++ = '&';
                    *buf++ = ' ';
                }
                putChars(buf, "expcnt(", 7);
                *buf++ = '0' + ((imm16>>4)&7);
                *buf++ = ')';
                prevLock = true;
            }
            if (((imm16>>8)&15) != 15)
            {   /* LGKMCNT bits is 4 (5????) */
                const cxuint lockCnt = (imm16>>8)&15;
                if (prevLock)
                {
                    *buf++ = ' ';
                    *buf++ = '&';
                    *buf++ = ' ';
                }
                putChars(buf, "lgkmcnt(", 8);
                const cxuint digit2 = lockCnt/10U;
                if (lockCnt >= 10)
                    *buf++ = '0'+digit2;
                *buf++= '0' + lockCnt-digit2*10U;
                *buf++ = ')';
                prevLock = true;
            }
            if ((imm16&0xf080) != 0 || (imm16==0xf7f))
            {   /* additional info about imm16 */
                if (prevLock)
                {
                    *buf++ = ' ';
                    *buf++ = ':';
                }
                buf += itocstrCStyle(imm16, buf, 11, 16);
            }
            break;
        }
        case GCN_IMM_MSGS:
        {
            cxuint illMask = 0xfff0;
            addSpaces(buf, spacesToAdd);
            putChars(buf, "sendmsg(", 8);
            const cxuint msgType = imm16&15;
            const char* msgName = sendMsgCodeMessageTable[msgType];
            while (*msgName != 0)
                *buf++ = *msgName++;
            if ((msgType&14) == 2 || (msgType >= 4 && msgType <= 14) ||
                (imm16&0x3f0) != 0) // gs ops
            {
                *buf++ = ',';
                *buf++ = ' ';
                illMask = 0xfcc0;
                const char* gsopName = sendGsOpMessageTable[(imm16>>4)&3];
                while (*gsopName != 0)
                    *buf++ = *gsopName++;
                *buf++ = ',';
                *buf++ = ' ';
                *buf++ = '0' + ((imm16>>8)&3);
            }
            *buf++ = ')';
            if ((imm16&illMask) != 0)
            {
                *buf++ = ' ';
                *buf++ = ':';
                buf += itocstrCStyle(imm16, buf, 11, 16);
            }
            break;
        }
        case GCN_IMM_NONE:
            if (imm16 != 0)
            {
                addSpaces(buf, spacesToAdd);
                buf += itocstrCStyle(imm16, buf, 11, 16);
            }
            break;
        default:
            addSpaces(buf, spacesToAdd);
            buf += itocstrCStyle(imm16, buf, 11, 16);
            break;
    }
    output.forward(buf-bufStart);
}

static void decodeSOP1Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    char* bufStart = output.reserve(70);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    bool isDst = (gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE;
    if (isDst)
        decodeGCNOperand((insnCode>>16)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf, arch);
    
    if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
    {
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
        {
            *buf++ = ',';
            *buf++ = ' ';
        }
        decodeGCNOperand(insnCode&0xff, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf,
                     arch, literal);
    }
    else if ((insnCode&0xff) != 0)
    {
        putChars(buf," ssrc=", 6);
        buf += itocstrCStyle((insnCode&0xff), buf, 6, 16);
    }
    
    if (!isDst && ((insnCode>>16)&0x7f) != 0)
    {
        putChars(buf, " sdst=", 6);
        buf += itocstrCStyle(((insnCode>>16)&0x7f), buf, 6, 16);
    }
    output.forward(buf-bufStart);
}

static void decodeSOP2Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    char* bufStart = output.reserve(80);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_REG_S1_JMP)
    {
        decodeGCNOperand((insnCode>>16)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf, arch);
        *buf++ = ',';
        *buf++ = ' ';
    }
    decodeGCNOperand(insnCode&0xff, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf, arch, literal);
    *buf++ = ',';
    *buf++ = ' ';
    decodeGCNOperand((insnCode>>8)&0xff, (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf,
                     arch, literal);
    
    if ((gcnInsn.mode & GCN_MASK1) == GCN_REG_S1_JMP &&
        ((insnCode>>16)&0x7f) != 0)
    {
        putChars(buf, " sdst=", 6);
        buf += itocstrCStyle(((insnCode>>16)&0x7f), buf, 6, 16);
    }
    output.forward(buf-bufStart);
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
    char* bufStart = output.reserve(80);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    if ((gcnInsn.mode & GCN_IMM_DST) == 0)
    {
        decodeGCNOperand((insnCode>>16)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf, arch);
        *buf++ = ',';
        *buf++ = ' ';
    }
    const cxuint imm16 = insnCode&0xffff;
    if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL)
    {
        const size_t branchPos = (pos + int16_t(imm16))<<2;
        output.forward(buf-bufStart);
        labelWriter(branchPos);
        buf = bufStart = output.reserve(60);
    }
    else if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_SREG)
    {
        putChars(buf, "hwreg(", 6);
        const cxuint hwregId = imm16&0x3f;
        if (hwregId < 13)
            putChars(buf, hwregNames[hwregId], ::strlen(hwregNames[hwregId]));
        else
        {
            const cxuint digit2 = hwregId/10U;
            *buf++ = '0' + digit2;
            *buf++ = '0' + hwregId - digit2*10U;
        }
        *buf++ = ',';
        *buf++ = ' ';
        putByteToBuf((imm16>>6)&31, buf);
        *buf++ = ',';
        *buf++ = ' ';
        putByteToBuf(((imm16>>11)&31)+1, buf);
        *buf++ = ')';
    }
    else
        buf += itocstrCStyle(imm16, buf, 11, 16);
    
    if (gcnInsn.mode & GCN_IMM_DST)
    {
        *buf++ = ',';
        *buf++ = ' ';
        if (gcnInsn.mode & GCN_SOPK_CONST)
        {
            buf += itocstrCStyle(literal, buf, 11, 16);
            if (((insnCode>>16)&0x7f) != 0)
            {
                putChars(buf, " sdst=", 6);
                buf += itocstrCStyle((insnCode>>16)&0x7f, buf, 6, 16);
            }
        }
        else
            decodeGCNOperand((insnCode>>16)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1,
                     buf, arch);
    }
    output.forward(buf-bufStart);
}

static void decodeSMRDEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    char* bufStart = output.reserve(100);
    char* buf = bufStart;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    
    bool spacesAdded = false;
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        addSpaces(buf, spacesToAdd);
        decodeGCNOperand((insnCode>>15)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf, arch);
        useDst = true;
        spacesAdded = true;
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        addSpaces(buf, spacesToAdd);
        decodeGCNOperand((insnCode>>15)&0x7f, dregsNum, buf, arch);
        *buf++ = ',';
        *buf++ = ' ';
        decodeGCNOperand((insnCode>>8)&0x7e, (gcnInsn.mode&GCN_SBASE4)?4:2, buf, arch);
        *buf++ = ',';
        *buf++ = ' ';
        if (insnCode&0x100) // immediate value
            buf += itocstrCStyle(insnCode&0xff, buf, 11, 16);
        else // S register
            decodeGCNOperand(insnCode&0xff, 1, buf, arch);
        useDst = true;
        useOthers = true;
        spacesAdded = true;
    }
    
    if (!useDst && (insnCode & 0x3f8000U) != 0)
    {
        addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " sdst=", 6);
        buf += itocstrCStyle((insnCode>>15)&0x7f, buf, 6, 16);
    }
    if (!useOthers && (insnCode & 0x7e00U)!=0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " sbase=", 7);
        buf += itocstrCStyle((insnCode>>9)&0x3f, buf, 6, 16);
    }
    if (!useOthers && (insnCode & 0xffU)!=0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " offset=", 8);
        buf += itocstrCStyle(insnCode&0xff, buf, 6, 16);
    }
    if (!useOthers && (insnCode & 0x100U)!=0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " imm=1", 6);
    }
    output.forward(buf-bufStart);
}

static void decodeSMEMEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* bufStart = output.reserve(100);
    char* buf = bufStart;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    bool spacesAdded = false;
    if (mode1 == GCN_SMRD_ONLYDST)
    {
        addSpaces(buf, spacesToAdd);
        decodeGCNOperand((insnCode>>6)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf, arch);
        useDst = true;
        spacesAdded = true;
    }
    else if (mode1 != GCN_ARG_NONE)
    {
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_DSIZE_MASK)>>GCN_SHIFT2);
        addSpaces(buf, spacesToAdd);
        if (mode1 & GCN_SMEM_SDATA_IMM)
            putHexByteToBuf((insnCode>>6)&0x7f, buf);
        else
            decodeGCNOperand((insnCode>>6)&0x7f, dregsNum, buf , arch);
        *buf++ = ',';
        *buf++ = ' ';
        decodeGCNOperand((insnCode<<1)&0x7e, (gcnInsn.mode&GCN_SBASE4)?4:2, buf, arch);
        *buf++ = ',';
        *buf++ = ' ';
        if (insnCode&0x20000) // immediate value
            buf += itocstrCStyle(insnCode2&0xfffff, buf, 11, 16);
        else // S register
            decodeGCNOperand(insnCode2&0xff, 1, buf, arch);
        useDst = true;
        useOthers = true;
        spacesAdded = true;
    }
    
    if ((insnCode & 0x10000) != 0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " glc", 4);
    }
    
    if (!useDst && (insnCode & 0x1fc0U) != 0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " sdata=", 7);
        buf += itocstrCStyle((insnCode>>6)&0x7f, buf, 6, 16);
    }
    if (!useOthers && (insnCode & 0x3fU)!=0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " sbase=", 7);
        buf += itocstrCStyle(insnCode&0x3f, buf, 6, 16);
    }
    if (!useOthers && insnCode2!=0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " offset=", 8);
        buf += itocstrCStyle(insnCode2, buf, 12, 16);
    }
    if (!useOthers && (insnCode & 0x20000U)!=0)
    {
        if (!spacesAdded)
            addSpaces(buf, spacesToAdd-1);
        spacesAdded = true;
        putChars(buf, " imm=1", 6);
    }
    
    output.forward(buf-bufStart);
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
    char* bufStart = output.reserve(90);
    char* buf = bufStart;
    const cxuint dstSel = (insnCode2>>8)&7;
    const cxuint dstUnused = (insnCode2>>11)&3;
    const cxuint src0Sel = (insnCode2>>16)&7;
    const cxuint src1Sel = (insnCode2>>24)&7;
    if (insnCode2 & 0x2000)
        putChars(buf, " clamp", 6);
    
    if (dstSel != 6)
    {
        putChars(buf, " dst_sel:", 9);
        putChars(buf, sdwaSelChoicesTbl[dstSel], ::strlen(sdwaSelChoicesTbl[dstSel]));
    }
    if (dstUnused!=0)
    {
        putChars(buf, " dst_unused:", 12);
        putChars(buf, sdwaDstUnusedTbl[dstUnused], ::strlen(sdwaDstUnusedTbl[dstUnused]));
    }
    if (src0Sel != 6)
    {
        putChars(buf, " src0_sel:", 10);
        putChars(buf, sdwaSelChoicesTbl[src0Sel], ::strlen(sdwaSelChoicesTbl[src0Sel]));
    }
    if (src1Sel != 6)
    {
        putChars(buf, " src1_sel:", 10);
        putChars(buf, sdwaSelChoicesTbl[src1Sel], ::strlen(sdwaSelChoicesTbl[src1Sel]));
    }
    
    if (!src0Used)
    {
        if ((insnCode2&(1U<<20))!=0)
            putChars(buf, " sext0", 6);
        if ((insnCode2&(1U<<20))!=0)
            putChars(buf, " neg0", 5);
        if ((insnCode2&(1U<<21))!=0)
            putChars(buf, " abs0", 5);
    }
    if (!src1Used)
    {
        if ((insnCode2&(1U<<27))!=0)
            putChars(buf, " sext1", 6);
        if ((insnCode2&(1U<<28))!=0)
            putChars(buf, " neg1", 5);
        if ((insnCode2&(1U<<29))!=0)
            putChars(buf, " abs1", 5);
    }
    
    output.forward(buf-bufStart);
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
    char* bufStart = output.reserve(100);
    char* buf = bufStart;
    const cxuint dppCtrl = (insnCode2>>8)&0x1ff;
    
    if (dppCtrl<256)
    {   // quadperm
        putChars(buf, " quad_perm:[", 12);
        *buf++ = '0' + (dppCtrl&3);
        *buf++ = ',';
        *buf++ = '0' + ((dppCtrl>>2)&3);
        *buf++ = ',';
        *buf++ = '0' + ((dppCtrl>>4)&3);
        *buf++ = ',';
        *buf++ = '0' + ((dppCtrl>>6)&3);
        *buf++ = ']';
    }
    else if ((dppCtrl >= 0x101 && dppCtrl <= 0x12f) && ((dppCtrl&0xf) != 0))
    {   // row_shl
        if ((dppCtrl&0xf0) == 0)
            putChars(buf, " row_shl:", 9);
        else if ((dppCtrl&0xf0) == 16)
            putChars(buf, " row_shr:", 9);
        else
            putChars(buf, " row_ror:", 9);
        putByteToBuf(dppCtrl&0xf, buf);
    }
    else if (dppCtrl >= 0x130 && dppCtrl <= 0x143 && dppCtrl130Tbl[dppCtrl-0x130]!=nullptr)
        putChars(buf, dppCtrl130Tbl[dppCtrl-0x130], ::strlen(dppCtrl130Tbl[dppCtrl-0x130]));
    else if (dppCtrl != 0x100)
    {
        putChars(buf, " dppctrl:", 9);
        buf += itocstrCStyle(dppCtrl, buf, 10, 16);
    }
    
    if (insnCode2 & (0x80000U)) // bound ctrl
        putChars(buf, " bound_ctrl", 11);
    
    putChars(buf, " bank_mask:", 11);
    putByteToBuf((insnCode2>>24)&0xf, buf);
    putChars(buf, " row_mask:", 10);
    putByteToBuf((insnCode2>>28)&0xf, buf);
    
    if (!src0Used)
    {
        if ((insnCode2&(1U<<20))!=0)
            putChars(buf, " neg0", 5);
        if ((insnCode2&(1U<<21))!=0)
            putChars(buf, " abs0", 5);
    }
    if (!src1Used)
    {
        if ((insnCode2&(1U<<22))!=0)
            putChars(buf, " neg1", 5);
        if ((insnCode2&(1U<<23))!=0)
            putChars(buf, " abs1", 5);
    }
    
    output.forward(buf-bufStart);
}

static void decodeVOPCEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    char* bufStart = output.reserve(100);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    
    const cxuint src0Field = (insnCode&0x1ff);
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    putChars(buf, "vcc, ", 5);
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
        putChars(buf, "sext(", 5);
    if (extraFlags.negSrc0)
        *buf++ = '-';
    if (extraFlags.absSrc0)
        putChars(buf, "abs(", 4);
    
    decodeGCNOperand(extraFlags.src0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                           buf, arch, literal, displayFloatLits);
    if (extraFlags.absSrc0)
        *buf++ = ')';
    if (extraFlags.sextSrc0)
        *buf++ = ')';
    *buf++ = ',';
    *buf++ = ' ';
    
    if (extraFlags.sextSrc1)
        putChars(buf, "sext(", 5);
    
    if (extraFlags.negSrc1)
        *buf++ = '-';
    if (extraFlags.absSrc1)
        putChars(buf, "abs(", 4);
    
    decodeGCNVRegOperand(((insnCode>>9)&0xff), (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf);
    if (extraFlags.absSrc1)
        *buf++ = ')';
    if (extraFlags.sextSrc1)
        *buf++ = ')';
    
    output.forward(buf-bufStart);
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
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    char* bufStart = output.reserve(110);
    char* buf = bufStart;
    
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
        addSpaces(buf, spacesToAdd);
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_SGPR)
            decodeGCNVRegOperand(((insnCode>>17)&0xff),
                     (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf);
        else
            decodeGCNOperand(((insnCode>>17)&0xff),
                   (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf, arch);
        *buf++ = ',';
        *buf++ = ' ';
        if (extraFlags.sextSrc0)
            putChars(buf, "sext(", 5);
        
        if (extraFlags.negSrc0)
            *buf++ = '-';
        if (extraFlags.absSrc0)
            putChars(buf, "abs(", 4);
        decodeGCNOperand(extraFlags.src0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                           buf, arch, literal, displayFloatLits);
        if (extraFlags.absSrc0)
            *buf++ = ')';
        if (extraFlags.sextSrc0)
            *buf++ = ')';
    }
    else if ((insnCode & 0x1fe01ffU) != 0)
    {
        addSpaces(buf, spacesToAdd);
        if ((insnCode & 0x1fe0000U) != 0)
        {
            putChars(buf, "vdst=", 5);
            buf += itocstrCStyle((insnCode>>17)&0xff, buf, 6, 16);
            if ((insnCode & 0x1ff) != 0)
                *buf++ = ' ';
        }
        if ((insnCode & 0x1ff) != 0)
        {
            putChars(buf, "src0=", 5);
            buf += itocstrCStyle(insnCode&0x1ff, buf, 6, 16);
        }
        argsUsed = false;
    }
    output.forward(buf-bufStart);
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
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    char* bufStart = output.reserve(130);
    char* buf = bufStart;
    
    addSpaces(buf, spacesToAdd);
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
        decodeGCNOperand(((insnCode>>17)&0xff),
                 (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf, arch);
    else
        decodeGCNVRegOperand(((insnCode>>17)&0xff), (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf);
    if (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC)
        putChars(buf, ", vcc", 5);
    *buf++ = ',';
    *buf++ = ' ';
    if (extraFlags.sextSrc0)
        putChars(buf, "sext(", 5);
    if (extraFlags.negSrc0)
        *buf++ = '-';
    if (extraFlags.absSrc0)
        putChars(buf, "abs(", 4);
    decodeGCNOperand(extraFlags.src0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                       buf, arch, literal, displayFloatLits);
    if (extraFlags.absSrc0)
        *buf++ = ')';
    if (extraFlags.sextSrc0)
        *buf++ = ')';
    if (mode1 == GCN_ARG1_IMM)
    {
        *buf++ = ',';
        *buf++ = ' ';
        printLiteral(buf, literal, displayFloatLits);
    }
    *buf++ = ',';
    *buf++ = ' ';
    if (extraFlags.sextSrc1)
        putChars(buf, "sext(", 5);
    
    if (extraFlags.negSrc1)
        *buf++ = '-';
    if (extraFlags.absSrc1)
        putChars(buf, "abs(", 4);
    if (mode1 == GCN_DS1_SGPR || mode1 == GCN_SRC1_SGPR)
        decodeGCNOperand(((insnCode>>9)&0xff),
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf, arch);
    else
        decodeGCNVRegOperand(((insnCode>>9)&0xff), (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf);
    if (extraFlags.absSrc1)
        *buf++ = ')';
    if (extraFlags.sextSrc1)
        *buf++ = ')';
    if (mode1 == GCN_ARG2_IMM)
    {
        *buf++ = ',';
        *buf++ = ' ';
        printLiteral(buf, literal, displayFloatLits);
    }
    else if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
        putChars(buf, ", vcc", 5);
    
    output.forward(buf-bufStart);
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

static void decodeVINTRPParam(uint16_t p, char*& buf)
{
    if (p > 3)
    {
        putChars(buf, "invalid_", 8);
        buf += itocstrCStyle(p, buf, 8);
    }
    else
        putChars(buf, vintrpParamsTbl[p], ::strlen(vintrpParamsTbl[p]));
}

static void decodeVOP3Encoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2,
         FloatLitType displayFloatLits)
{
    char* bufStart = output.reserve(140);
    char* buf = bufStart;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
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
        addSpaces(buf, spacesToAdd);
        
        if (opcode < 256 || (gcnInsn.mode&GCN_VOP3_DST_SGPR)!=0) /* if compares */
            decodeGCNOperand(vdst, ((gcnInsn.mode&GCN_VOP3_DST_SGPR)==0)?2:1, buf, arch);
        else /* regular instruction */
            decodeGCNVRegOperand(vdst, (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf);
        
        if (gcnInsn.encoding == GCNENC_VOP3B &&
            (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC || mode1 == GCN_DST_VCC_VSRC2 ||
             mode1 == GCN_S0EQS12)) /* VOP3b */
        {
            *buf++ = ',';
            *buf++ = ' ';
            decodeGCNOperand(((insnCode>>8)&0x7f), 2, buf, arch);
        }
        *buf++ = ',';
        *buf++ = ' ';
        if (vop3Mode != GCN_VOP3_VINTRP)
        {
            if ((insnCode2 & (1U<<29)) != 0)
                *buf++ = '-';
            if (absFlags & 1)
                putChars(buf, "abs(", 4);
            decodeGCNOperand(vsrc0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                                   buf, arch, 0, displayFloatLits);
            if (absFlags & 1)
                *buf++ = ')';
            vsrc0Used = true;
        }
        
        if (mode1 != GCN_SRC12_NONE && (vop3Mode != GCN_VOP3_VINTRP))
        {
            *buf++ = ',';
            *buf++ = ' ';
            if ((insnCode2 & (1U<<30)) != 0)
                *buf++ = '-';
            if (absFlags & 2)
                putChars(buf, "abs(", 4);
            decodeGCNOperand(vsrc1, (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf, arch,
                     0, displayFloatLits);
            if (absFlags & 2)
                *buf++ = ')';
            /* GCN_DST_VCC - only sdst is used, no vsrc2 */
            if (mode1 != GCN_SRC2_NONE && mode1 != GCN_DST_VCC && opcode >= 256)
            {
                *buf++ = ',';
                *buf++ = ' ';
                
                if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
                {
                    decodeGCNOperand(vsrc2, 2, buf, arch);
                    vsrc2CC = true;
                }
                else
                {
                    if ((insnCode2 & (1U<<31)) != 0)
                        *buf++ = '-';
                    if (absFlags & 4)
                        putChars(buf, "abs(", 4);
                    decodeGCNOperand(vsrc2, (gcnInsn.mode&GCN_REG_SRC2_64)?2:1,
                                     buf, arch, 0, displayFloatLits);
                    if (absFlags & 4)
                        *buf++ = ')';
                }
                vsrc2Used = true;
            }
            vsrc1Used = true;
        }
        else if (vop3Mode == GCN_VOP3_VINTRP)
        {
            if ((insnCode2 & (1U<<30)) != 0)
                *buf++ = '-';
            if (absFlags & 2)
                putChars(buf, "abs(", 4);
            if (mode1 == GCN_P0_P10_P20)
                decodeVINTRPParam(vsrc1, buf);
            else
                decodeGCNOperand(vsrc1, 1, buf, arch, 0, displayFloatLits);
            if (absFlags & 2)
                *buf++ = ')';
            putChars(buf, ", attr", 6);
            const cxuint attr = vsrc0&63;
            putByteToBuf(attr, buf);
            *buf++ = '.';
            *buf++ = "xyzw"[((vsrc0>>6)&3)]; // attrchannel
            
            if ((gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2)
            {
                *buf++ = ',';
                *buf++ = ' ';
                if ((insnCode2 & (1U<<31)) != 0)
                    *buf++ = '-';
                if (absFlags & 4)
                    putChars(buf, "abs(", 4);
                decodeGCNOperand(vsrc2, 1, buf, arch, 0, displayFloatLits);
                if (absFlags & 4)
                    *buf++ = ')';
                vsrc2Used = true;
            }
            
            if (vsrc0 & 0x100)
                putChars(buf, " high", 5);
            vsrc0Used = true;
            vsrc1Used = true;
        }
        vdstUsed = true;
    }
    else
        addSpaces(buf, spacesToAdd-1);
    
    const cxuint omod = (insnCode2>>27)&3;
    if (omod != 0)
    {
        const char* omodStr = (omod==3)?" div:2":(omod==2)?" mul:4":" mul:2";
        putChars(buf, omodStr, 6);
    }
    
    const bool clamp = (!isGCN12 && gcnInsn.encoding == GCNENC_VOP3A &&
            (insnCode&0x800) != 0) || (isGCN12 && (insnCode&0x8000) != 0);
    if (clamp)
        putChars(buf, " clamp", 6);
    
    /* as field */
    if (!vdstUsed && vdst != 0)
    {
        putChars(buf, " dst=", 5);
        buf += itocstrCStyle(vdst, buf, 6, 16);
    }
        
    if (!vsrc0Used)
    {
        if (vsrc0 != 0)
        {
            putChars(buf, " src0=", 6);
            buf += itocstrCStyle(vsrc0, buf, 6, 16);
        }
        if (absFlags & 1)
            putChars(buf, " abs0", 5);
        if ((insnCode2 & (1U<<29)) != 0)
            putChars(buf, " neg0", 5);
    }
    if (!vsrc1Used)
    {
        if (vsrc1 != 0)
        {
            putChars(buf, " vsrc1=", 7);
            buf += itocstrCStyle(vsrc1, buf, 6, 16);
        }
        if (absFlags & 2)
            putChars(buf, " abs1", 5);
        if ((insnCode2 & (1U<<30)) != 0)
            putChars(buf, " neg1", 5);
    }
    if (!vsrc2Used)
    {
        if (vsrc2 != 0)
        {
            putChars(buf, " vsrc2=", 7);
            buf += itocstrCStyle(vsrc2, buf, 6, 16);
        }
        if (absFlags & 4)
            putChars(buf, " abs2", 5);
        if ((insnCode2 & (1U<<31)) != 0)
            putChars(buf, " neg2", 5);
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
    }
    else // force for v_madmk_f32 and v_madak_f32
        isVOP1Word = true;
    
    if (isVOP1Word) // add vop3 for distinguishing encoding
        putChars(buf, " vop3", 5);
    
    output.forward(buf-bufStart);
}

static void decodeVINTRPEncoding(cxuint spacesToAdd, uint16_t arch,
           FastOutputBuffer& output, const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    char* bufStart = output.reserve(80);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    decodeGCNVRegOperand((insnCode>>18)&0xff, 1, buf);
    *buf++ = ',';
    *buf++ = ' ';
    if ((gcnInsn.mode & GCN_MASK1) == GCN_P0_P10_P20)
        decodeVINTRPParam(insnCode&0xff, buf);
    else
        decodeGCNVRegOperand(insnCode&0xff, 1, buf);
    putChars(buf, ", attr", 6);
    const cxuint attr = (insnCode>>10)&63;
    putByteToBuf(attr, buf);
    *buf++ = '.';
    *buf++ = "xyzw"[((insnCode>>8)&3)]; // attrchannel
    output.forward(buf-bufStart);
}

static void decodeDSEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
           const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* bufStart = output.reserve(90);
    char* buf = bufStart;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    addSpaces(buf, spacesToAdd);
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
        decodeGCNVRegOperand(vdst, regsNum, buf);
        vdstUsed = true;
    }
    if ((gcnInsn.mode & GCN_ONLYDST) == 0)
    {
        if (vdstUsed)
        {
            *buf++ = ',';
            *buf++ = ' ';
        }
        decodeGCNVRegOperand(vaddr, 1, buf);
        vaddrUsed = true;
    }
    
    const uint16_t srcMode = (gcnInsn.mode & GCN_SRCS_MASK);
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 &&
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) != 0 && srcMode != GCN_NOSRC)
    {   /* two vdata */
        if (vaddrUsed || vdstUsed)
        {
            *buf++ = ',';
            *buf++ = ' ';
        }
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        decodeGCNVRegOperand(vdata0, regsNum, buf);
        vdata0Used = true;
        if (srcMode == GCN_2SRCS)
        {
            *buf++ = ',';
            *buf++ = ' ';
            decodeGCNVRegOperand(vdata1, (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf);
            vdata1Used = true;
        }
    }
    
    const cxuint offset = (insnCode&0xffff);
    if (offset != 0)
    {
        if ((gcnInsn.mode & GCN_2OFFSETS) == 0) /* single offset */
        {
            putChars(buf, " offset:", 8);
            buf += itocstrCStyle(offset, buf, 7, 10);
        }
        else
        {
            if ((offset&0xff) != 0)
            {
                putChars(buf, " offset0:", 9);
                putByteToBuf(offset&0xff, buf);
            }
            if ((offset&0xff00) != 0)
            {
                putChars(buf, " offset1:", 9);
                putByteToBuf((offset>>8)&0xff, buf);
            }
        }
    }
    
    if ((!isGCN12 && (insnCode&0x20000)!=0) || (isGCN12 && (insnCode&0x10000)!=0))
        putChars(buf, " gds", 4);
    
    if (!vaddrUsed && vaddr != 0)
    {
        putChars(buf, " vaddr=", 7);
        buf += itocstrCStyle(vaddr, buf, 6, 16);
    }
    if (!vdata0Used && vdata0 != 0)
    {
        putChars(buf, " vdata0=", 8);
        buf += itocstrCStyle(vdata0, buf, 6, 16);
    }
    if (!vdata1Used && vdata1 != 0)
    {
        putChars(buf, " vdata1=", 8);
        buf += itocstrCStyle(vdata1, buf, 6, 16);
    }
    if (!vdstUsed && vdst != 0)
    {
        putChars(buf, " vdst=", 6);
        buf += itocstrCStyle(vdst, buf, 6, 16);
    }
    output.forward(buf-bufStart);
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
    char* bufStart = output.reserve(150);
    char* buf = bufStart;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    const cxuint vaddr = insnCode2&0xff;
    const cxuint vdata = (insnCode2>>8)&0xff;
    const cxuint srsrc = (insnCode2>>16)&0x1f;
    const cxuint soffset = insnCode2>>24;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    if (mode1 != GCN_ARG_NONE)
    {
        addSpaces(buf, spacesToAdd);
        if (mode1 != GCN_MUBUF_NOVAD)
        {
            cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
            if (insnCode2 & 0x800000U)
                dregsNum++; // tfe
            
            decodeGCNVRegOperand(vdata, dregsNum, buf);
            *buf++ = ',';
            *buf++ = ' ';
            // determine number of vaddr registers
            /* for addr32 - idxen+offen or 1, for addr64 - 2 (idxen and offen is illegal) */
            const cxuint aregsNum = ((insnCode & 0x3000U)==0x3000U ||
                    /* addr64 only for older GCN than 1.2 */
                    (!isGCN12 && (insnCode & 0x8000U)))? 2 : 1;
            
            decodeGCNVRegOperand(vaddr, aregsNum, buf);
            *buf++ = ',';
            *buf++ = ' ';
        }
        decodeGCNOperand(srsrc<<2, 4, buf, arch);
        *buf++ = ',';
        *buf++ = ' ';
        decodeGCNOperand(soffset, 1, buf, arch);
    }
    else
        addSpaces(buf, spacesToAdd-1);
    
    if (insnCode & 0x1000U)
        putChars(buf, " offen", 6);
    if (insnCode & 0x2000U)
        putChars(buf, " idxen", 6);
    const cxuint offset = insnCode&0xfff;
    if (offset != 0)
    {
        putChars(buf, " offset:", 8);
        buf += itocstrCStyle(offset, buf, 7, 10);
    }
    if (insnCode & 0x4000U)
        putChars(buf, " glc", 4);
    
    if (((!isGCN12 || mtbuf) && (insnCode2 & 0x400000U)!=0) ||
        ((isGCN12 && !mtbuf) && (insnCode & 0x20000)!=0))
        putChars(buf, " slc", 4);
    
    if (!isGCN12 && (insnCode & 0x8000U)!=0)
        putChars(buf, " addr64", 7);
    if (!mtbuf && (insnCode & 0x10000U) != 0)
        putChars(buf, " lds", 4);
    if (insnCode2 & 0x800000U)
        putChars(buf, " tfe", 4);
    if (mtbuf)
    {
        putChars(buf, " format:[", 9);
        const char* dfmtStr = mtbufDFMTTable[(insnCode>>19)&15];
        putChars(buf, dfmtStr, ::strlen(dfmtStr));
        *buf++ = ',';
        const char* nfmtStr = mtbufNFMTTable[(insnCode>>23)&7];
        putChars(buf, nfmtStr, ::strlen(nfmtStr));
        *buf++ = ']';
    }
    
    if (mode1 == GCN_ARG_NONE || mode1 == GCN_MUBUF_NOVAD)
    {
        if (vaddr != 0)
        {
            putChars(buf, " vaddr=", 7);
            buf += itocstrCStyle(vaddr, buf, 6, 16);
        }
        if (vdata != 0)
        {
            putChars(buf, " vdata=", 7);
            buf += itocstrCStyle(vdata, buf, 6, 16);
        }
    }
    if (mode1 == GCN_ARG_NONE)
    {
        if (srsrc != 0)
        {
            putChars(buf, " srsrc=", 7);
            buf += itocstrCStyle(srsrc, buf, 6, 16);
        }
        if (soffset != 0)
        {
            putChars(buf, " soffset=", 9);
            buf += itocstrCStyle(soffset, buf, 6, 16);
        }
    }
    output.forward(buf-bufStart);
}

static void decodeMIMGEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* bufStart = output.reserve(150);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    
    const cxuint dmask =  (insnCode>>8)&15;
    cxuint dregsNum = ((dmask & 1)?1:0) + ((dmask & 2)?1:0) + ((dmask & 4)?1:0) +
            ((dmask & 8)?1:0);
    dregsNum = (dregsNum == 0) ? 1 : dregsNum;
    if (insnCode & 0x10000)
        dregsNum++; // tfe
    
    decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, buf);
    *buf++ = ',';
    *buf++ = ' ';
    // determine number of vaddr registers
    decodeGCNVRegOperand(insnCode2&0xff, 4, buf);
    *buf++ = ',';
    *buf++ = ' ';
    decodeGCNOperand(((insnCode2>>14)&0x7c), (insnCode & 0x8000)?4:8, buf, arch);
    
    const cxuint ssamp = (insnCode2>>21)&0x1f;
    
    if ((gcnInsn.mode & GCN_MASK2) == GCN_MIMG_SAMPLE)
    {
        *buf++ = ',';
        *buf++ = ' ';
        decodeGCNOperand(ssamp<<2, 4, buf, arch);
    }
    if (dmask != 1)
    {
        putChars(buf, " dmask:", 7);
        if (dmask >= 10)
        {
            *buf++ = '1';
            *buf++ = '0' + dmask - 10;
        }
        else
            *buf++ = '0' + dmask;
    }
    
    if (insnCode & 0x1000)
        putChars(buf, " unorm", 6);
    if (insnCode & 0x2000)
        putChars(buf, " glc", 4);
    if (insnCode & 0x2000000)
        putChars(buf, " slc", 4);
    if (insnCode & 0x8000)
        putChars(buf, " r128", 5);
    if (insnCode & 0x10000)
        putChars(buf, " tfe", 4);
    if (insnCode & 0x20000)
        putChars(buf, " lwe", 4);
    if (insnCode & 0x4000)
    {
        *buf++ = ' ';
        *buf++ = 'd';
        *buf++ = 'a';
    }
    if ((arch & ARCH_RX3X0)!=0 && (insnCode2 & (1U<<31)) != 0)
        putChars(buf, " d16", 4);
    
    if ((gcnInsn.mode & GCN_MASK2) != GCN_MIMG_SAMPLE && ssamp != 0)
    {
        putChars(buf, " ssamp=", 7);
        buf += itocstrCStyle(ssamp, buf, 6, 16);
    }
    output.forward(buf-bufStart);
}

static void decodeEXPEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
        const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* bufStart = output.reserve(90);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    /* export target */
    const cxuint target = (insnCode>>4)&63;
    if (target >= 32)
    {
        putChars(buf, "param", 5);
        const cxuint tpar = target-32;
        const cxuint digit2 = tpar/10;
        if (digit2 != 0)
            *buf++ = '0' + digit2;
        *buf++ = '0' + tpar - 10*digit2;
    }
    else if (target >= 12 && target <= 15)
    {
        putChars(buf, "pos0", 4);
        buf[-1] = '0' + target-12;
    }
    else if (target < 8)
    {
        putChars(buf, "mrt0", 4);
        buf[-1] = '0' + target;
    }
    else if (target == 8)
        putChars(buf, "mrtz", 4);
    else if (target == 9)
        putChars(buf, "null", 4);
    else
    {   /* reserved */
        putChars(buf, "ill_", 4);
        const cxuint digit2 = target/10U;
        *buf++ = '0' + digit2;
        *buf++ = '0' + target - 10U*digit2;
    }
    
    /* vdata registers */
    cxuint vsrcsUsed = 0;
    for (cxuint i = 0; i < 4; i++)
    {
        *buf++ = ',';
        *buf++ = ' ';
        if (insnCode & (1U<<i))
        {
            if ((insnCode&0x400)==0)
            {
                decodeGCNVRegOperand((insnCode2>>(i<<3))&0xff, 1, buf);
                vsrcsUsed |= 1<<i;
            }
            else // if compr=1
            {
                decodeGCNVRegOperand(((i>=2)?(insnCode2>>8):insnCode2)&0xff, 1, buf);
                vsrcsUsed |= 1U<<(i>>1);
            }
        }
        else
            putChars(buf, "off", 3);
    }
    
    if (insnCode&0x800)
        putChars(buf, " done", 5);
    if (insnCode&0x400)
        putChars(buf, " compr", 6);
    if (insnCode&0x1000)
    {
        *buf++ = ' ';
        *buf++ = 'v';
        *buf++ = 'm';
    }
    
    for (cxuint i = 0; i < 4; i++)
    {
        const cxuint val = (insnCode2>>(i<<3))&0xff;
        if ((vsrcsUsed&(1U<<i))==0 && val!=0)
        {
            putChars(buf, " vsrc0=", 7);
            buf[-2] += i; // number
            buf += itocstrCStyle(val, buf, 6, 16);
        }
    }
    output.forward(buf-bufStart);
}

static void decodeFLATEncoding(cxuint spacesToAdd, uint16_t arch, FastOutputBuffer& output,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2)
{
    char* bufStart = output.reserve(130);
    char* buf = bufStart;
    addSpaces(buf, spacesToAdd);
    bool vdstUsed = false;
    bool vdataUsed = false;
    const cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
    // tfe
    const cxuint dstRegsNum = (insnCode2 & 0x800000U)?dregsNum+1:dregsNum;
    
    if ((gcnInsn.mode & GCN_FLAT_ADST) == 0)
    {
        vdstUsed = true;
        decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, buf);
        *buf++ = ',';
        *buf++ = ' ';
        decodeGCNVRegOperand(insnCode2&0xff, 2, buf); // addr
    }
    else
    {   /* two vregs, because 64-bitness stored in PTR32 mode (at runtime) */
        decodeGCNVRegOperand(insnCode2&0xff, 2, buf); // addr
        if ((gcnInsn.mode & GCN_FLAT_NODST) == 0)
        {
            vdstUsed = true;
            *buf++ = ',';
            *buf++ = ' ';
            decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, buf);
        }
    }
    
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0) /* print data */
    {
        vdataUsed = true;
        *buf++ = ',';
        *buf++ = ' ';
        decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, buf);
    }
    
    if (insnCode & 0x10000U)
        putChars(buf, " glc", 4);
    if (insnCode & 0x20000U)
        putChars(buf, " slc", 4);
    if (insnCode2 & 0x800000U)
        putChars(buf, " tfe", 4);
    
    if (!vdataUsed && ((insnCode2>>8)&0xff) != 0)
    {
        putChars(buf, " vdata=", 7);
        buf += itocstrCStyle((insnCode2>>8)&0xff, buf, 6, 16);
    }
    if (!vdstUsed && (insnCode2>>24) != 0)
    {
        putChars(buf, " vdst=", 6);
        buf += itocstrCStyle(insnCode2>>24, buf, 6, 16);
    }
    output.forward(buf-bufStart);
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
                bufPos += addSpacesOld(buf+bufPos, 8);
            buf[bufPos++] = '*';
            buf[bufPos++] = '/';
            buf[bufPos++] = ' ';
            output.forward(bufPos);
        }
        else // add spaces
        {
            char* buf = output.reserve(8);
            output.forward(addSpacesOld(buf, 8));
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
                char* bufStart = output.reserve(40);
                char* buf = bufStart;
                if (!isGCN12 || gcnEncoding != GCNENC_SMEM)
                    putChars(buf, gcnEncodingNames[gcnEncoding],
                            ::strlen(gcnEncodingNames[gcnEncoding]));
                else /* SMEM encoding */
                    putChars(buf, "SMEM", 4);
                putChars(buf, "_ill_", 5);
                buf += itocstrCStyle(opcode, buf , 6);
                spacesToAdd = spacesToAdd >= (buf-bufStart+1)?
                            spacesToAdd - (buf-bufStart) : 1;
                gcnInsn = &defaultInsn;
                output.forward(buf-bufStart);
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
