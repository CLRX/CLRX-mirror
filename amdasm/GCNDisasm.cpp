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
#include <algorithm>
#include <cstring>
#include <mutex>
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/MemAccess.h>
#include "GCNInternals.h"

namespace CLRX
{

enum FloatLitType: cxbyte
{
    FLTLIT_NONE,
    FLTLIT_F32,
    FLTLIT_F16,
};
    
struct CLRX_INTERNAL GCNDisasmUtils
{
    typedef GCNDisassembler::RelocIter RelocIter;
    static void printLiteral(GCNDisassembler& dasm, size_t codePos, RelocIter& relocIter,
              uint32_t literal, FloatLitType floatLit, bool optional);
    static void decodeGCNOperandNoLit(GCNDisassembler& dasm, cxuint op, cxuint regNum,
              char*& bufPtr, uint16_t arch, FloatLitType floatLit = FLTLIT_NONE);
    static char* decodeGCNOperand(GCNDisassembler& dasm, size_t codePos,
              RelocIter& relocIter, cxuint op, cxuint regNum, uint16_t arch,
              uint32_t literal = 0, FloatLitType floatLit = FLTLIT_NONE);
    
    static void decodeSOPCEncoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSOPPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode,
             uint32_t literal, size_t pos);
    
    static void decodeSOP1Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSOP2Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSOPKEncoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSMRDEncoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode);
    
    static void decodeSMEMEncoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2);
    
    static void decodeVOPCEncoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
             FloatLitType displayFloatLits);
    
    static void decodeVOP1Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
             FloatLitType displayFloatLits);
    
    static void decodeVOP2Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
             FloatLitType displayFloatLits);
    
    static void decodeVOP3Encoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2,
             FloatLitType displayFloatLits);
    
    static void decodeVINTRPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode);
    
    static void decodeDSEncoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2);
    
    static void decodeMUBUFEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
            uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2);
    
    static void decodeMIMGEncoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2);
    
    static void decodeEXPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2);
    
    static void decodeFLATEncoding(GCNDisassembler& dasm, cxuint spacesToAdd, uint16_t arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insnCode2);
};

};

using namespace CLRX;

static std::once_flag clrxGCNDisasmOnceFlag;
static std::unique_ptr<GCNInstruction[]> gcnInstrTableByCode = nullptr;

struct CLRX_INTERNAL GCNEncodingSpace
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
            else if((instr.archMask & ARCH_RX2X0) != 0) /* otherwise we for GCN1.1 */
            {
                const GCNEncodingSpace& encSpace2 =
                        gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+1];
                gcnInstrTableByCode[encSpace2.offset + instr.code] = instr;
            }
            // otherwise we ignore this entry
        }
        if ((instr.archMask & ARCH_RX3X0) != 0)
        {
            const GCNEncodingSpace& encSpace3 = gcnInstrTableByCodeSpaces[
                        GCNENC_MAXVAL+3+instr.encoding];
            if (gcnInstrTableByCode[encSpace3.offset + instr.code].mnemonic == nullptr)
                gcnInstrTableByCode[encSpace3.offset + instr.code] = instr;
            // otherwise we ignore this entry
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
    false // GCNENC_NONE   // 1111 - illegal
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
    false // GCNENC_NONE   // 1111 - illegal
};

void GCNDisassembler::analyzeBeforeDisassemble()
{
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
                            labels.push_back(startOffset +
                                    ((pos+int16_t(insnCode&0xffff)+1)<<2));
                    }
                    else
                    {   // SOPK
                        const cxuint opcode = (insnCode>>23)&0x1f;
                        if ((!isGCN12 && opcode == 17) ||
                            (isGCN12 && opcode == 16)) // if branch fork
                            labels.push_back(startOffset +
                                    ((pos+int16_t(insnCode&0xffff)+1)<<2));
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
    { 16, 10 }, /* GCNENC_VOP3A, opcode = (10bit)<<16 */
    { 16, 10 }, /* GCNENC_VOP3B, opcode = (10bit)<<16 */
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
    if (dasm.writeRelocation(dasm.startOffset + (codePos<<2)-4, relocIter))
        return;
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(50);
    char* bufPtr = bufStart;
    if (optional && (int32_t(literal)<=64 && int32_t(literal)>=-16)) // use lit(...)
    {
        putChars(bufPtr, "lit(", 4);
        bufPtr += itocstrCStyle<int32_t>(literal, bufPtr, 11, 10);
        *bufPtr++ = ')';
    }
    else
        bufPtr += itocstrCStyle(literal, bufPtr, 11, 16);
    if (floatLit != FLTLIT_NONE)
    {
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
           cxuint regNum, char*& bufPtr, uint16_t arch, FloatLitType floatLit)
{
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    if ((!isGCN12 && op < 104) || (isGCN12 && op < 102) || (op >= 256 && op < 512))
    {   // scalar
        if (op >= 256)
        {
            *bufPtr++ = 'v';
            op -= 256;
        }
        else
            *bufPtr++ = 's';
        regRanges(op, regNum, bufPtr);
        return;
    }
    
    const cxuint op2 = op&~1U;
    if (op2 == 106 || op2 == 108 || op2 == 110 || op2 == 126 ||
        (op2 == 104 && (arch&ARCH_RX2X0)!=0) ||
        ((op2 == 102 || op2 == 104) && isGCN12))
    {   // if not SGPR, but other scalar registers
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
                *bufPtr++ = 'v';
                *bufPtr++ = 'c';
                *bufPtr++ = 'c';
                break;
            case 108:
                *bufPtr++ = 't';
                *bufPtr++ = 'b';
                *bufPtr++ = 'a';
                break;
            case 110:
                *bufPtr++ = 't';
                *bufPtr++ = 'm';
                *bufPtr++ = 'a';
                break;
            case 126:
                putChars(bufPtr, "exec", 4);
                break;
        }
        // if full 64-bit register
        if (regNum >= 2)
        {
            if (op&1) // unaligned!!
            {
                *bufPtr++ = '_';
                *bufPtr++ = 'u';
                *bufPtr++ = '!';
            }
            if (regNum > 2)
                putChars(bufPtr, "&ill!", 5);
            return;
        }
        *bufPtr++ = '_';
        if ((op&1) == 0)
        { *bufPtr++ = 'l'; *bufPtr++ = 'o'; }
        else
        { *bufPtr++ = 'h'; *bufPtr++ = 'i'; }
        return;
    }
    
    if (op == 255) // if literal
    {
        *bufPtr++ = '0'; // zero 
        *bufPtr++ = 'x'; // zero 
        *bufPtr++ = '0'; // zero 
        return;
    }
    
    if (op >= 112 && op < 124)
    {
        putChars(bufPtr, "ttmp", 4);
        regRanges(op-112, regNum, bufPtr);
        return;
    }
    if (op == 124)
    {
        *bufPtr++ = 'm';
        *bufPtr++ = '0';
        if (regNum > 1)
            putChars(bufPtr, "&ill!", 5);
        return;
    }
    
    if (op >= 128 && op <= 192)
    {   // integer constant
        op -= 128;
        const cxuint digit1 = op/10U;
        if (digit1 != 0)
            *bufPtr++ = digit1 + '0';
        *bufPtr++ = op-digit1*10U + '0';
        return;
    }
    if (op > 192 && op <= 208)
    {
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
    
    switch(op)
    {
        case 248:
            if (isGCN12)
            {   // 1/(2*PI)
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
            *bufPtr++ = 's';
            *bufPtr++ = 'c';
            *bufPtr++ = 'c';
            return;
        case 254:
            *bufPtr++ = 'l';
            *bufPtr++ = 'd';
            *bufPtr++ = 's';
            return;
    }
    
    // reserved value (illegal)
    putChars(bufPtr, "ill_", 4);
    *bufPtr++ = '0'+op/100U;
    *bufPtr++ = '0'+(op/10U)%10U;
    *bufPtr++ = '0'+op%10U;
}

char* GCNDisasmUtils::decodeGCNOperand(GCNDisassembler& dasm, size_t codePos,
              RelocIter& relocIter, cxuint op, cxuint regNum, uint16_t arch,
              uint32_t literal, FloatLitType floatLit)
{
    FastOutputBuffer& output = dasm.output;
    if (op == 255)
    {   // literal
        printLiteral(dasm, codePos, relocIter, literal, floatLit, true);
        return output.reserve(100);
    }
    char* bufStart = output.reserve(50);
    char* bufPtr = bufStart;
    decodeGCNOperandNoLit(dasm, op, regNum, bufPtr, arch, floatLit);
    output.forward(bufPtr-bufStart);
    return output.reserve(100);
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

static void addSpaces(char*& bufPtr, cxuint spacesToAdd)
{
    for (cxuint k = spacesToAdd; k>0; k--)
        *bufPtr++ = ' ';
}

static size_t addSpacesOld(char* bufPtr, cxuint spacesToAdd)
{
    for (cxuint k = spacesToAdd; k>0; k--)
        *bufPtr++ = ' ';
    return spacesToAdd;
}

void GCNDisasmUtils::decodeSOPCEncoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(80);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    output.forward(bufPtr-bufStart);
    bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, insnCode&0xff,
                     (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal);
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
    if ((gcnInsn.mode & GCN_SRC1_IMM) != 0)
    {
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
         uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t literal, size_t pos)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(60);
    char* bufPtr = bufStart;
    const cxuint imm16 = insnCode&0xffff;
    switch(gcnInsn.mode&GCN_MASK1)
    {
        case GCN_IMM_REL:
        {
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
            const bool isf7f = (imm16==0xf7f);
            if ((imm16&15) != 15 || isf7f)
            {
                const cxuint lockCnt = imm16&15;
                putChars(bufPtr, "vmcnt(", 6);
                if (lockCnt >= 10)
                    *bufPtr++ = '1';
                *bufPtr++ = '0' + ((lockCnt>=10)?lockCnt-10:lockCnt);
                *bufPtr++ = ')';
                prevLock = true;
            }
            if (((imm16>>4)&7) != 7 || isf7f)
            {
                if (prevLock)
                {
                    *bufPtr++ = ' ';
                    *bufPtr++ = '&';
                    *bufPtr++ = ' ';
                }
                putChars(bufPtr, "expcnt(", 7);
                *bufPtr++ = '0' + ((imm16>>4)&7);
                *bufPtr++ = ')';
                prevLock = true;
            }
            if (((imm16>>8)&15) != 15 || isf7f)
            {   /* LGKMCNT bits is 4 (5????) */
                const cxuint lockCnt = (imm16>>8)&15;
                if (prevLock)
                {
                    *bufPtr++ = ' ';
                    *bufPtr++ = '&';
                    *bufPtr++ = ' ';
                }
                putChars(bufPtr, "lgkmcnt(", 8);
                const cxuint digit2 = lockCnt/10U;
                if (lockCnt >= 10)
                    *bufPtr++ = '0'+digit2;
                *bufPtr++= '0' + lockCnt-digit2*10U;
                *bufPtr++ = ')';
                prevLock = true;
            }
            if ((imm16&0xf080) != 0)
            {   /* additional info about imm16 */
                if (prevLock)
                {
                    *bufPtr++ = ' ';
                    *bufPtr++ = ':';
                }
                bufPtr += itocstrCStyle(imm16, bufPtr, 11, 16);
            }
            break;
        }
        case GCN_IMM_MSGS:
        {
            cxuint illMask = 0xfff0;
            addSpaces(bufPtr, spacesToAdd);
            putChars(bufPtr, "sendmsg(", 8);
            const cxuint msgType = imm16&15;
            const char* msgName = sendMsgCodeMessageTable[msgType];
            while (*msgName != 0)
                *bufPtr++ = *msgName++;
            if ((msgType&14) == 2 || (msgType >= 4 && msgType <= 14) ||
                (imm16&0x3f0) != 0) // gs ops
            {
                *bufPtr++ = ',';
                *bufPtr++ = ' ';
                illMask = 0xfcc0;
                const cxuint gsopId = (imm16>>4)&3;
                const char* gsopName = sendGsOpMessageTable[gsopId];
                while (*gsopName != 0)
                    *bufPtr++ = *gsopName++;
                if (gsopId!=0 || ((imm16>>8)&3)!=0)
                {
                    *bufPtr++ = ',';
                    *bufPtr++ = ' ';
                    *bufPtr++ = '0' + ((imm16>>8)&3);
                }
            }
            *bufPtr++ = ')';
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
         RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(70);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    bool isDst = (gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE;
    if (isDst)
    {
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>16)&0x7f,
                         (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
    }
    
    if ((gcnInsn.mode & GCN_MASK1) != GCN_SRC_NONE)
    {
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
        {
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
        }
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, insnCode&0xff,
                         (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal);
    }
    else if ((insnCode&0xff) != 0)
    {   // print value, if some are not used, but values is not default
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
         RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(80);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_NONE)
    {
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>16)&0x7f,
                         (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
    }
    output.forward(bufPtr-bufStart);
    bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, insnCode&0xff,
                     (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal);
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
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

static const char* hwregNames[13] =
{
    "0", "mode", "status", "trapsts",
    "hw_id", "gpr_alloc", "lds_alloc", "ib_sts",
    "pc_lo", "pc_hi", "inst_dw0", "inst_dw1",
    "ib_dbg0"
};

/// about label writer - label is workaround for class hermetization
void GCNDisasmUtils::decodeSOPKEncoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(80);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    if ((gcnInsn.mode & GCN_IMM_DST) == 0)
    {
        output.forward(bufPtr-bufStart);
        bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter, (insnCode>>16)&0x7f,
                         (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
    }
    const cxuint imm16 = insnCode&0xffff;
    if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_REL)
    {
        const size_t branchPos = dasm.startOffset + ((codePos + int16_t(imm16))<<2);
        output.forward(bufPtr-bufStart);
        dasm.writeLocation(branchPos);
        bufPtr = bufStart = output.reserve(60);
    }
    else if ((gcnInsn.mode&GCN_MASK1) == GCN_IMM_SREG)
    {
        putChars(bufPtr, "hwreg(", 6);
        const cxuint hwregId = imm16&0x3f;
        if (hwregId < 13)
            putChars(bufPtr, hwregNames[hwregId], ::strlen(hwregNames[hwregId]));
        else
        {
            const cxuint digit2 = hwregId/10U;
            *bufPtr++ = '0' + digit2;
            *bufPtr++ = '0' + hwregId - digit2*10U;
        }
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        putByteToBuf((imm16>>6)&31, bufPtr);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        putByteToBuf(((imm16>>11)&31)+1, bufPtr);
        *bufPtr++ = ')';
    }
    else
        bufPtr += itocstrCStyle(imm16, bufPtr, 11, 16);
    
    if (gcnInsn.mode & GCN_IMM_DST)
    {
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        // print value, if some are not used, but values is not default
        if (gcnInsn.mode & GCN_SOPK_CONST)
        {
            bufPtr += itocstrCStyle(literal, bufPtr, 11, 16);
            if (((insnCode>>16)&0x7f) != 0)
            {
                putChars(bufPtr, " sdst=", 6);
                bufPtr += itocstrCStyle((insnCode>>16)&0x7f, bufPtr, 6, 16);
            }
        }
        else
        {
            output.forward(bufPtr-bufStart);
            bufPtr = bufStart = decodeGCNOperand(dasm, codePos, relocIter,
                     (insnCode>>16)&0x7f, (gcnInsn.mode&GCN_REG_DST_64)?2:1, arch);
        }
    }
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeSMRDEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(100);
    char* bufPtr = bufStart;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    
    bool spacesAdded = false;
    if (mode1 == GCN_SMRD_ONLYDST)
    {
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
        decodeGCNOperandNoLit(dasm, (insnCode>>15)&0x7f, dregsNum, bufPtr, arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        decodeGCNOperandNoLit(dasm, (insnCode>>8)&0x7e, (gcnInsn.mode&GCN_SBASE4)?4:2,
                          bufPtr, arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        if (insnCode&0x100) // immediate value
            bufPtr += itocstrCStyle(insnCode&0xff, bufPtr, 11, 16);
        else // S register
            decodeGCNOperandNoLit(dasm, insnCode&0xff, 1, bufPtr, arch);
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
         uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(100);
    char* bufPtr = bufStart;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    bool useDst = false;
    bool useOthers = false;
    bool spacesAdded = false;
    if (mode1 == GCN_SMRD_ONLYDST)
    {
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
        if (mode1 & GCN_SMEM_SDATA_IMM)
            putHexByteToBuf((insnCode>>6)&0x7f, bufPtr);
        else
            decodeGCNOperandNoLit(dasm, (insnCode>>6)&0x7f, dregsNum, bufPtr , arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        decodeGCNOperandNoLit(dasm, (insnCode<<1)&0x7e, (gcnInsn.mode&GCN_SBASE4)?4:2,
                          bufPtr, arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        if (insnCode&0x20000) // immediate value
            bufPtr += itocstrCStyle(insnCode2&0xfffff, bufPtr, 11, 16);
        else // S register
            decodeGCNOperandNoLit(dasm, insnCode2&0xff, 1, bufPtr, arch);
        useDst = true;
        useOthers = true;
        spacesAdded = true;
    }
    
    if ((insnCode & 0x10000) != 0)
    {
        if (!spacesAdded)
            addSpaces(bufPtr, spacesToAdd-1);
        spacesAdded = true;
        putChars(bufPtr, " glc", 4);
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

struct CLRX_INTERNAL VOPExtraWordOut
{
    uint16_t src0: 9;
    bool sextSrc0;
    bool negSrc0;
    bool absSrc0;
    bool sextSrc1;
    bool negSrc1;
    bool absSrc1;
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
    return { uint16_t((insnCode2&0xff)+256),
        (insnCode2&(1U<<19))!=0, (insnCode2&(1U<<20))!=0, (insnCode2&(1U<<21))!=0,
        (insnCode2&(1U<<27))!=0, (insnCode2&(1U<<28))!=0, (insnCode2&(1U<<29))!=0 };
}

static void decodeVOPSDWA(FastOutputBuffer& output, uint32_t insnCode2,
          bool src0Used, bool src1Used)
{
    char* bufStart = output.reserve(90);
    char* bufPtr = bufStart;
    const cxuint dstSel = (insnCode2>>8)&7;
    const cxuint dstUnused = (insnCode2>>11)&3;
    const cxuint src0Sel = (insnCode2>>16)&7;
    const cxuint src1Sel = (insnCode2>>24)&7;
    if (insnCode2 & 0x2000)
        putChars(bufPtr, " clamp", 6);
    
    if (dstSel != 6)
    {
        putChars(bufPtr, " dst_sel:", 9);
        putChars(bufPtr, sdwaSelChoicesTbl[dstSel],
                 ::strlen(sdwaSelChoicesTbl[dstSel]));
    }
    if (dstUnused!=0)
    {
        putChars(bufPtr, " dst_unused:", 12);
        putChars(bufPtr, sdwaDstUnusedTbl[dstUnused],
                 ::strlen(sdwaDstUnusedTbl[dstUnused]));
    }
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
        if ((insnCode2&(1U<<20))!=0)
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
    
    output.forward(bufPtr-bufStart);
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
    return { uint16_t((insnCode2&0xff)+256),
        false, (insnCode2&(1U<<20))!=0, (insnCode2&(1U<<21))!=0,
        false, (insnCode2&(1U<<22))!=0, (insnCode2&(1U<<23))!=0 };
}

static void decodeVOPDPP(FastOutputBuffer& output, uint32_t insnCode2,
        bool src0Used, bool src1Used)
{
    char* bufStart = output.reserve(100);
    char* bufPtr = bufStart;
    const cxuint dppCtrl = (insnCode2>>8)&0x1ff;
    
    if (dppCtrl<256)
    {   // quadperm
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
    {   // row_shl
        if ((dppCtrl&0xf0) == 0)
            putChars(bufPtr, " row_shl:", 9);
        else if ((dppCtrl&0xf0) == 16)
            putChars(bufPtr, " row_shr:", 9);
        else
            putChars(bufPtr, " row_ror:", 9);
        putByteToBuf(dppCtrl&0xf, bufPtr);
    }
    else if (dppCtrl >= 0x130 && dppCtrl <= 0x143 && dppCtrl130Tbl[dppCtrl-0x130]!=nullptr)
        putChars(bufPtr, dppCtrl130Tbl[dppCtrl-0x130],
                 ::strlen(dppCtrl130Tbl[dppCtrl-0x130]));
    else if (dppCtrl != 0x100)
    {
        putChars(bufPtr, " dppctrl:", 9);
        bufPtr += itocstrCStyle(dppCtrl, bufPtr, 10, 16);
    }
    
    if (insnCode2 & (0x80000U)) // bound ctrl
        putChars(bufPtr, " bound_ctrl", 11);
    
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
         RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    char* bufStart = output.reserve(100);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    
    const cxuint src0Field = (insnCode&0x1ff);
    VOPExtraWordOut extraFlags = { 0, 0, 0, 0, 0, 0, 0 };
    putChars(bufPtr, "vcc, ", 5);
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
        putChars(bufPtr, "sext(", 5);
    if (extraFlags.negSrc0)
        *bufPtr++ = '-';
    if (extraFlags.absSrc0)
        putChars(bufPtr, "abs(", 4);
    
    output.forward(bufPtr-bufStart);
    bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter, extraFlags.src0,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal, displayFloatLits);
    if (extraFlags.absSrc0)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc0)
        *bufPtr++ = ')';
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
    
    if (extraFlags.sextSrc1)
        putChars(bufPtr, "sext(", 5);
    
    if (extraFlags.negSrc1)
        *bufPtr++ = '-';
    if (extraFlags.absSrc1)
        putChars(bufPtr, "abs(", 4);
    
    decodeGCNVRegOperand(((insnCode>>9)&0xff), (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, bufPtr);
    if (extraFlags.absSrc1)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc1)
        *bufPtr++ = ')';
    
    output.forward(bufPtr-bufStart);
    if (isGCN12)
    {
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, literal, true, true);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, true, true);
    }
}

void GCNDisasmUtils::decodeVOP1Encoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    char* bufStart = output.reserve(110);
    char* bufPtr = bufStart;
    
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
        addSpaces(bufPtr, spacesToAdd);
        if ((gcnInsn.mode & GCN_MASK1) != GCN_DST_SGPR)
            decodeGCNVRegOperand(((insnCode>>17)&0xff),
                     (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr);
        else
            decodeGCNOperandNoLit(dasm, ((insnCode>>17)&0xff),
                          (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr, arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        if (extraFlags.sextSrc0)
            putChars(bufPtr, "sext(", 5);
        
        if (extraFlags.negSrc0)
            *bufPtr++ = '-';
        if (extraFlags.absSrc0)
            putChars(bufPtr, "abs(", 4);
        output.forward(bufPtr-bufStart);
        bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter, extraFlags.src0,
                     (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal, displayFloatLits);
        if (extraFlags.absSrc0)
            *bufPtr++ = ')';
        if (extraFlags.sextSrc0)
            *bufPtr++ = ')';
    }
    else if ((insnCode & 0x1fe01ffU) != 0)
    {   // unused, but set fields
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
        if (src0Field == 0xf9)
            decodeVOPSDWA(output, literal, argsUsed, false);
        else if (src0Field == 0xfa)
            decodeVOPDPP(output, literal, argsUsed, false);
    }
}

void GCNDisasmUtils::decodeVOP2Encoding(GCNDisassembler& dasm, size_t codePos,
         RelocIter& relocIter, cxuint spacesToAdd, uint16_t arch,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    char* bufStart = output.reserve(130);
    char* bufPtr = bufStart;
    
    addSpaces(bufPtr, spacesToAdd);
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
    
    if (mode1 != GCN_DS1_SGPR)
        decodeGCNVRegOperand(((insnCode>>17)&0xff),
                     (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr);
    else
        decodeGCNOperandNoLit(dasm, ((insnCode>>17)&0xff),
                 (gcnInsn.mode&GCN_REG_DST_64)?2:1, bufPtr, arch);
    if (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC)
        putChars(bufPtr, ", vcc", 5);
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
    if (extraFlags.sextSrc0)
        putChars(bufPtr, "sext(", 5);
    if (extraFlags.negSrc0)
        *bufPtr++ = '-';
    if (extraFlags.absSrc0)
        putChars(bufPtr, "abs(", 4);
    output.forward(bufPtr-bufStart);
    bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter, extraFlags.src0,
                 (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, arch, literal, displayFloatLits);
    if (extraFlags.absSrc0)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc0)
        *bufPtr++ = ')';
    if (mode1 == GCN_ARG1_IMM)
    {
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        output.forward(bufPtr-bufStart);
        printLiteral(dasm, codePos, relocIter, literal, displayFloatLits, false);
        bufStart = bufPtr = output.reserve(100);
    }
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
    if (extraFlags.sextSrc1)
        putChars(bufPtr, "sext(", 5);
    
    if (extraFlags.negSrc1)
        *bufPtr++ = '-';
    if (extraFlags.absSrc1)
        putChars(bufPtr, "abs(", 4);
    if (mode1 == GCN_DS1_SGPR || mode1 == GCN_SRC1_SGPR)
    {
        output.forward(bufPtr-bufStart);
        bufStart = bufPtr = decodeGCNOperand(dasm, codePos, relocIter,
                 ((insnCode>>9)&0xff), (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, arch);
    }
    else
        decodeGCNVRegOperand(((insnCode>>9)&0xff),
                     (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, bufPtr);
    if (extraFlags.absSrc1)
        *bufPtr++ = ')';
    if (extraFlags.sextSrc1)
        *bufPtr++ = ')';
    if (mode1 == GCN_ARG2_IMM)
    {
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        output.forward(bufPtr-bufStart);
        printLiteral(dasm, codePos, relocIter, literal, displayFloatLits, false);
    }
    else if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
    {
        putChars(bufPtr, ", vcc", 5);
        output.forward(bufPtr-bufStart);
    }
    else
        output.forward(bufPtr-bufStart);
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
         uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t insnCode2, FloatLitType displayFloatLits)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(140);
    char* bufPtr = bufStart;
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
    
    const bool is128Ops = (gcnInsn.mode&0xf000)==GCN_VOP3_DS2_128;
    
    if (mode1 != GCN_VOP_ARG_NONE)
    {
        addSpaces(bufPtr, spacesToAdd);
        
        if (opcode < 256 || (gcnInsn.mode&GCN_VOP3_DST_SGPR)!=0) /* if compares */
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
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
            decodeGCNOperandNoLit(dasm, ((insnCode>>8)&0x7f), 2, bufPtr, arch);
        }
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        if (vop3Mode != GCN_VOP3_VINTRP)
        {
            if ((insnCode2 & (1U<<29)) != 0)
                *bufPtr++ = '-';
            if (absFlags & 1)
                putChars(bufPtr, "abs(", 4);
            decodeGCNOperandNoLit(dasm, vsrc0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                                   bufPtr, arch, displayFloatLits);
            if (absFlags & 1)
                *bufPtr++ = ')';
            vsrc0Used = true;
        }
        
        if (vop3Mode == GCN_VOP3_VINTRP)
        {
            if ((insnCode2 & (1U<<30)) != 0)
                *bufPtr++ = '-';
            if (absFlags & 2)
                putChars(bufPtr, "abs(", 4);
            if (mode1 == GCN_P0_P10_P20)
                decodeVINTRPParam(vsrc1, bufPtr);
            else
                decodeGCNOperandNoLit(dasm, vsrc1, 1, bufPtr, arch, displayFloatLits);
            if (absFlags & 2)
                *bufPtr++ = ')';
            putChars(bufPtr, ", attr", 6);
            const cxuint attr = vsrc0&63;
            putByteToBuf(attr, bufPtr);
            *bufPtr++ = '.';
            *bufPtr++ = "xyzw"[((vsrc0>>6)&3)]; // attrchannel
            
            if ((gcnInsn.mode & GCN_VOP3_MASK3) == GCN_VINTRP_SRC2)
            {
                *bufPtr++ = ',';
                *bufPtr++ = ' ';
                if ((insnCode2 & (1U<<31)) != 0)
                    *bufPtr++ = '-';
                if (absFlags & 4)
                    putChars(bufPtr, "abs(", 4);
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
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
            if ((insnCode2 & (1U<<30)) != 0)
                *bufPtr++ = '-';
            if (absFlags & 2)
                putChars(bufPtr, "abs(", 4);
            decodeGCNOperandNoLit(dasm, vsrc1, (gcnInsn.mode&GCN_REG_SRC1_64)?2:1,
                      bufPtr, arch, displayFloatLits);
            if (absFlags & 2)
                *bufPtr++ = ')';
            /* GCN_DST_VCC - only sdst is used, no vsrc2 */
            if (mode1 != GCN_SRC2_NONE && mode1 != GCN_DST_VCC && opcode >= 256)
            {
                *bufPtr++ = ',';
                *bufPtr++ = ' ';
                
                if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
                {
                    decodeGCNOperandNoLit(dasm, vsrc2, 2, bufPtr, arch);
                    vsrc2CC = true;
                }
                else
                {
                    if ((insnCode2 & (1U<<31)) != 0)
                        *bufPtr++ = '-';
                    if (absFlags & 4)
                        putChars(bufPtr, "abs(", 4);
                    // for V_MQSAD_U32 SRC2 is 128-bit
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
    
    const cxuint omod = (insnCode2>>27)&3;
    if (omod != 0)
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
    
    const cxuint usedMask = 7 & ~(vsrc2CC?4:0);
    /* check whether instruction is this same like VOP2/VOP1/VOPC */
    bool isVOP1Word = false; // if can be write in single VOP dword
    if (vop3Mode == GCN_VOP3_VINTRP)
    {
        if (mode1 != GCN_NEW_OPCODE) /* check clamp and abs flags */
            isVOP1Word = !((insnCode&(7<<8)) != 0 || (insnCode2&(7<<29)) != 0 ||
                    clamp || ((vsrc1 < 256 && mode1!=GCN_P0_P10_P20) ||
                    (mode1==GCN_P0_P10_P20 && vsrc1 >= 256)) ||
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
        putChars(bufPtr, " vop3", 5);
    
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeVINTRPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
          uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(80);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    decodeGCNVRegOperand((insnCode>>18)&0xff, 1, bufPtr);
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
    if ((gcnInsn.mode & GCN_MASK1) == GCN_P0_P10_P20)
        decodeVINTRPParam(insnCode&0xff, bufPtr);
    else
        decodeGCNVRegOperand(insnCode&0xff, 1, bufPtr);
    putChars(bufPtr, ", attr", 6);
    const cxuint attr = (insnCode>>10)&63;
    putByteToBuf(attr, bufPtr);
    *bufPtr++ = '.';
    *bufPtr++ = "xyzw"[((insnCode>>8)&3)]; // attrchannel
    output.forward(bufPtr-bufStart);
}

void GCNDisasmUtils::decodeDSEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
          uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
          uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(90);
    char* bufPtr = bufStart;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    addSpaces(bufPtr, spacesToAdd);
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
        if ((gcnInsn.mode&GCN_DS_128) != 0 || (gcnInsn.mode&GCN_DST128) != 0)
            regsNum = 4;
        decodeGCNVRegOperand(vdst, regsNum, bufPtr);
        vdstUsed = true;
    }
    if ((gcnInsn.mode & GCN_ONLYDST) == 0)
    {   /// print VADDR
        if (vdstUsed)
        {
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
        }
        decodeGCNVRegOperand(vaddr, 1, bufPtr);
        vaddrUsed = true;
    }
    
    const uint16_t srcMode = (gcnInsn.mode & GCN_SRCS_MASK);
    
    if ((gcnInsn.mode & GCN_ONLYDST) == 0 &&
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) != 0 && srcMode != GCN_NOSRC)
    {   /* two vdata */
        if (vaddrUsed || vdstUsed)
        {   // comma after previous argument (VDST, VADDR)
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
        }
        cxuint regsNum = (gcnInsn.mode&GCN_REG_SRC0_64)?2:1;
        if ((gcnInsn.mode&GCN_DS_96) != 0)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DS_128) != 0)
            regsNum = 4;
        decodeGCNVRegOperand(vdata0, regsNum, bufPtr);
        vdata0Used = true;
        if (srcMode == GCN_2SRCS)
        {
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
            decodeGCNVRegOperand(vdata1, (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, bufPtr);
            vdata1Used = true;
        }
    }
    
    const cxuint offset = (insnCode&0xffff);
    if (offset != 0)
    {
        if ((gcnInsn.mode & GCN_2OFFSETS) == 0) /* single offset */
        {
            putChars(bufPtr, " offset:", 8);
            bufPtr += itocstrCStyle(offset, bufPtr, 7, 10);
        }
        else
        {
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

void GCNDisasmUtils::decodeMUBUFEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
          uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
          uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(150);
    char* bufPtr = bufStart;
    const bool isGCN12 = ((arch&ARCH_RX3X0)!=0);
    const cxuint vaddr = insnCode2&0xff;
    const cxuint vdata = (insnCode2>>8)&0xff;
    const cxuint srsrc = (insnCode2>>16)&0x1f;
    const cxuint soffset = insnCode2>>24;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    if (mode1 != GCN_ARG_NONE)
    {
        addSpaces(bufPtr, spacesToAdd);
        if (mode1 != GCN_MUBUF_NOVAD)
        {
            cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
            if (insnCode2 & 0x800000U)
                dregsNum++; // tfe
            
            decodeGCNVRegOperand(vdata, dregsNum, bufPtr);
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
            // determine number of vaddr registers
            /* for addr32 - idxen+offen or 1, for addr64 - 2 (idxen and offen is illegal) */
            const cxuint aregsNum = ((insnCode & 0x3000U)==0x3000U ||
                    /* addr64 only for older GCN than 1.2 */
                    (!isGCN12 && (insnCode & 0x8000U)))? 2 : 1;
            
            decodeGCNVRegOperand(vaddr, aregsNum, bufPtr);
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
        }
        decodeGCNOperandNoLit(dasm, srsrc<<2, 4, bufPtr, arch);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        decodeGCNOperandNoLit(dasm, soffset, 1, bufPtr, arch);
    }
    else
        addSpaces(bufPtr, spacesToAdd-1);
    
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
            putChars(bufPtr, " format:[", 9);
            if (dfmt!=1)
            {
                const char* dfmtStr = mtbufDFMTTable[dfmt];
                putChars(bufPtr, dfmtStr, ::strlen(dfmtStr));
            }
            if (dfmt!=1 && nfmt!=0)
                *bufPtr++ = ',';
            if (nfmt!=0)
            {
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
         uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
         uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(150);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    
    const cxuint dmask = (insnCode>>8)&15;
    cxuint dregsNum = 4;
    if ((gcnInsn.mode & GCN_MIMG_VDATA4) == 0)
        dregsNum = ((dmask & 1)?1:0) + ((dmask & 2)?1:0) + ((dmask & 4)?1:0) +
                ((dmask & 8)?1:0);
    
    dregsNum = (dregsNum == 0) ? 1 : dregsNum;
    if (insnCode & 0x10000)
        dregsNum++; // tfe
    
    decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, bufPtr);
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
    decodeGCNVRegOperand(insnCode2&0xff,
                 std::max(4, (gcnInsn.mode&GCN_MIMG_VA_MASK)+1), bufPtr);
    *bufPtr++ = ',';
    *bufPtr++ = ' ';
    decodeGCNOperandNoLit(dasm, ((insnCode2>>14)&0x7c), (insnCode & 0x8000)?4:8,
                          bufPtr, arch);
    
    const cxuint ssamp = (insnCode2>>21)&0x1f;
    if ((gcnInsn.mode & GCN_MIMG_SAMPLE) != 0)
    {
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        decodeGCNOperandNoLit(dasm, ssamp<<2, 4, bufPtr, arch);
    }
    if (dmask != 1)
    {
        putChars(bufPtr, " dmask:", 7);
        if (dmask >= 10)
        {
            *bufPtr++ = '1';
            *bufPtr++ = '0' + dmask - 10;
        }
        else
            *bufPtr++ = '0' + dmask;
    }
    
    if (insnCode & 0x1000)
        putChars(bufPtr, " unorm", 6);
    if (insnCode & 0x2000)
        putChars(bufPtr, " glc", 4);
    if (insnCode & 0x2000000)
        putChars(bufPtr, " slc", 4);
    if (insnCode & 0x8000)
        putChars(bufPtr, " r128", 5);
    if (insnCode & 0x10000)
        putChars(bufPtr, " tfe", 4);
    if (insnCode & 0x20000)
        putChars(bufPtr, " lwe", 4);
    if (insnCode & 0x4000)
    {
        *bufPtr++ = ' ';
        *bufPtr++ = 'd';
        *bufPtr++ = 'a';
    }
    if ((arch & ARCH_RX3X0)!=0 && (insnCode2 & (1U<<31)) != 0)
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
            uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(90);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    /* export target */
    const cxuint target = (insnCode>>4)&63;
    if (target >= 32)
    {
        putChars(bufPtr, "param", 5);
        const cxuint tpar = target-32;
        const cxuint digit2 = tpar/10;
        if (digit2 != 0)
            *bufPtr++ = '0' + digit2;
        *bufPtr++ = '0' + tpar - 10*digit2;
    }
    else if (target >= 12 && target <= 15)
    {
        putChars(bufPtr, "pos0", 4);
        bufPtr[-1] = '0' + target-12;
    }
    else if (target < 8)
    {
        putChars(bufPtr, "mrt0", 4);
        bufPtr[-1] = '0' + target;
    }
    else if (target == 8)
        putChars(bufPtr, "mrtz", 4);
    else if (target == 9)
        putChars(bufPtr, "null", 4);
    else
    {   /* reserved */
        putChars(bufPtr, "ill_", 4);
        const cxuint digit2 = target/10U;
        *bufPtr++ = '0' + digit2;
        *bufPtr++ = '0' + target - 10U*digit2;
    }
    
    /* vdata registers */
    cxuint vsrcsUsed = 0;
    for (cxuint i = 0; i < 4; i++)
    {
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        if (insnCode & (1U<<i))
        {
            if ((insnCode&0x400)==0)
            {
                decodeGCNVRegOperand((insnCode2>>(i<<3))&0xff, 1, bufPtr);
                vsrcsUsed |= 1<<i;
            }
            else // if compr=1
            {
                decodeGCNVRegOperand(((i>=2)?(insnCode2>>8):insnCode2)&0xff, 1, bufPtr);
                vsrcsUsed |= 1U<<(i>>1);
            }
        }
        else
            putChars(bufPtr, "off", 3);
    }
    
    if (insnCode&0x800)
        putChars(bufPtr, " done", 5);
    if (insnCode&0x400)
        putChars(bufPtr, " compr", 6);
    if (insnCode&0x1000)
    {
        *bufPtr++ = ' ';
        *bufPtr++ = 'v';
        *bufPtr++ = 'm';
    }
    
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

void GCNDisasmUtils::decodeFLATEncoding(GCNDisassembler& dasm ,cxuint spacesToAdd,
            uint16_t arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2)
{
    FastOutputBuffer& output = dasm.output;
    char* bufStart = output.reserve(130);
    char* bufPtr = bufStart;
    addSpaces(bufPtr, spacesToAdd);
    bool vdstUsed = false;
    bool vdataUsed = false;
    const cxuint dregsNum = ((gcnInsn.mode&GCN_DSIZE_MASK)>>GCN_SHIFT2)+1;
    /// cmpswap store only to half of number of data registers
    cxuint dstRegsNum = ((gcnInsn.mode & GCN_CMPSWAP)!=0) ? (dregsNum>>1) :  dregsNum;
    // tfe
    dstRegsNum = (insnCode2 & 0x800000U)?dstRegsNum+1:dstRegsNum;
    
    if ((gcnInsn.mode & GCN_FLAT_ADST) == 0)
    {
        vdstUsed = true;
        decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, bufPtr);
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        decodeGCNVRegOperand(insnCode2&0xff, 2, bufPtr); // addr
    }
    else
    {   /* two vregs, because 64-bitness stored in PTR32 mode (at runtime) */
        decodeGCNVRegOperand(insnCode2&0xff, 2, bufPtr); // addr
        if ((gcnInsn.mode & GCN_FLAT_NODST) == 0)
        {
            vdstUsed = true;
            *bufPtr++ = ',';
            *bufPtr++ = ' ';
            decodeGCNVRegOperand(insnCode2>>24, dstRegsNum, bufPtr);
        }
    }
    
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0) /* print data */
    {
        vdataUsed = true;
        *bufPtr++ = ',';
        *bufPtr++ = ' ';
        decodeGCNVRegOperand((insnCode2>>8)&0xff, dregsNum, bufPtr);
    }
    
    if (insnCode & 0x10000U)
        putChars(bufPtr, " glc", 4);
    if (insnCode & 0x20000U)
        putChars(bufPtr, " slc", 4);
    if (insnCode2 & 0x800000U)
        putChars(bufPtr, " tfe", 4);
    
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
    output.forward(bufPtr-bufStart);
}

/* main routine */

void GCNDisassembler::disassemble()
{
    LabelIter curLabel = std::lower_bound(labels.begin(), labels.end(), labelStartOffset);
    RelocIter curReloc = std::lower_bound(relocations.begin(), relocations.end(),
        std::make_pair(startOffset, Relocation()),
          [](const std::pair<size_t,Relocation>& a, const std::pair<size_t, Relocation>& b)
          { return a.first < b.first; });
    NamedLabelIter curNamedLabel = std::lower_bound(namedLabels.begin(), namedLabels.end(),
        std::make_pair(labelStartOffset, CString()),
          [](const std::pair<size_t,CString>& a, const std::pair<size_t, CString>& b)
          { return a.first < b.first; });
    
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
        if (insnCode == 0)
        {   /* fix for GalliumCOmpute disassemblying (assembler doesn't accep 
             * with two scalar operands */
            size_t count;
            for (count = 1; pos < codeWordsNum && codeWords[pos]==0; count++, pos++);
            // put to output
            char* buf = output.reserve(40);
            size_t bufPos = 0;
            memcpy(buf+bufPos, ".fill ", 6);
            bufPos += 6;
            bufPos += itocstrCStyle(count, buf+bufPos, 20);
            memcpy(buf+bufPos, ", 4, 0\n", 7);
            bufPos += 7;
            output.forward(bufPos);
            continue;
        }
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
                    isIllegal = true; // illegal
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
                char* bufPtr = bufStart;
                if (!isGCN12 || gcnEncoding != GCNENC_SMEM)
                    putChars(bufPtr, gcnEncodingNames[gcnEncoding],
                            ::strlen(gcnEncodingNames[gcnEncoding]));
                else /* SMEM encoding */
                    putChars(bufPtr, "SMEM", 4);
                putChars(bufPtr, "_ill_", 5);
                bufPtr += itocstrCStyle(opcode, bufPtr , 6);
                const size_t linePos = bufPtr-bufStart;
                spacesToAdd = spacesToAdd >= (linePos+1)? spacesToAdd - linePos : 1;
                gcnInsn = &defaultInsn;
                output.forward(bufPtr-bufStart);
            }
            
            const FloatLitType displayFloatLits = 
                    ((disassembler.getFlags()&DISASM_FLOATLITS) != 0) ?
                    (((gcnInsn->mode & GCN_MASK2) == GCN_FLOATLIT) ? FLTLIT_F32 : 
                    ((gcnInsn->mode & GCN_MASK2) == GCN_F16LIT) ? FLTLIT_F16 :
                     FLTLIT_NONE) : FLTLIT_NONE;
            
            switch(gcnEncoding)
            {
                case GCNENC_SOPC:
                    GCNDisasmUtils::decodeSOPCEncoding(*this, pos, curReloc,
                               spacesToAdd, curArchMask, *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_SOPP:
                    GCNDisasmUtils::decodeSOPPEncoding(*this, spacesToAdd, curArchMask, 
                                 *gcnInsn, insnCode, insnCode2, pos);
                    break;
                case GCNENC_SOP1:
                    GCNDisasmUtils::decodeSOP1Encoding(*this, pos, curReloc,
                               spacesToAdd, curArchMask, *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_SOP2:
                    GCNDisasmUtils::decodeSOP2Encoding(*this, pos, curReloc,
                               spacesToAdd, curArchMask, *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_SOPK:
                    GCNDisasmUtils::decodeSOPKEncoding(*this, pos, curReloc,
                               spacesToAdd, curArchMask, *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_SMRD:
                    if (isGCN12)
                        GCNDisasmUtils::decodeSMEMEncoding(*this, spacesToAdd, curArchMask,
                                  *gcnInsn, insnCode, insnCode2);
                    else
                        GCNDisasmUtils::decodeSMRDEncoding(*this, spacesToAdd, curArchMask,
                                  *gcnInsn, insnCode);
                    break;
                case GCNENC_VOPC:
                    GCNDisasmUtils::decodeVOPCEncoding(*this, pos, curReloc, spacesToAdd,
                           curArchMask, *gcnInsn, insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VOP1:
                    GCNDisasmUtils::decodeVOP1Encoding(*this, pos, curReloc, spacesToAdd,
                           curArchMask, *gcnInsn, insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VOP2:
                    GCNDisasmUtils::decodeVOP2Encoding(*this, pos, curReloc, spacesToAdd,
                           curArchMask, *gcnInsn, insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VOP3A:
                    GCNDisasmUtils::decodeVOP3Encoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode, insnCode2, displayFloatLits);
                    break;
                case GCNENC_VINTRP:
                    GCNDisasmUtils::decodeVINTRPEncoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode);
                    break;
                case GCNENC_DS:
                    GCNDisasmUtils::decodeDSEncoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_MUBUF:
                    GCNDisasmUtils::decodeMUBUFEncoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_MTBUF:
                    GCNDisasmUtils::decodeMUBUFEncoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_MIMG:
                    GCNDisasmUtils::decodeMIMGEncoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_EXP:
                    GCNDisasmUtils::decodeEXPEncoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode, insnCode2);
                    break;
                case GCNENC_FLAT:
                    GCNDisasmUtils::decodeFLATEncoding(*this, spacesToAdd, curArchMask,
                                 *gcnInsn, insnCode, insnCode2);
                    break;
                default:
                    break;
            }
        }
        output.put('\n');
    }
    if (!dontPrintLabelsAfterCode)
        writeLabelsToEnd(codeWordsNum<<2, curLabel, curNamedLabel);
    output.flush();
    disassembler.getOutput().flush();
}
