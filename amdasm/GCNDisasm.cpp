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

static OnceFlag clrxGCNDisasmOnceFlag;
static std::unique_ptr<GCNInstruction[]> gcnInstrTableByCode = nullptr;

// GCN encoding space
struct CLRX_INTERNAL GCNEncodingSpace
{
    cxuint offset;  // first position instrunctions list
    cxuint instrsNum;   // instruction list
};

// put chars to buffer (helper)
static inline void putChars(char*& buf, const char* input, size_t size)
{
    ::memcpy(buf, input, size);
    buf += size;
}

// add N spaces (old style), return number
static size_t addSpacesOld(char* bufPtr, cxuint spacesToAdd)
{
    for (cxuint k = spacesToAdd; k>0; k--)
        *bufPtr++ = ' ';
    return spacesToAdd;
}

// encoding names table
static const char* gcnEncodingNames[GCNENC_MAXVAL+1] =
{
    "NONE", "SOPC", "SOPP", "SOP1", "SOP2", "SOPK", "SMRD", "VOPC", "VOP1", "VOP2",
    "VOP3A", "VOP3B", "VINTRP", "DS", "MUBUF", "MTBUF", "MIMG", "EXP", "FLAT"
};

// table hold of GNC encoding regions in main instruction list
// instruciton position is sum of encoding offset and instruction opcode
static const GCNEncodingSpace gcnInstrTableByCodeSpaces[2*(GCNENC_MAXVAL+1)+2+3+2] =
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
    { 0x092d, 0x80 }, /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
    { 0x09ad, 0x200 }, /* GCNENC_VOP3A, opcode = (9bit)<<17 (GCN1.1) */
    { 0x09ad, 0x200 },  /* GCNENC_VOP3B, opcode = (9bit)<<17 (GCN1.1) */
    { 0x0bad, 0x0 },
    { 0x0bad, 0x80 }, /* GCNENC_SOPC, opcode = (7bit)<<16 (GCN1.2) */
    { 0x0c2d, 0x80 }, /* GCNENC_SOPP, opcode = (7bit)<<16 (GCN1.2) */
    { 0x0cad, 0x100 }, /* GCNENC_SOP1, opcode = (8bit)<<8 (GCN1.2) */
    { 0x0dad, 0x80 }, /* GCNENC_SOP2, opcode = (7bit)<<23 (GCN1.2) */
    { 0x0e2d, 0x20 }, /* GCNENC_SOPK, opcode = (5bit)<<23 (GCN1.2) */
    { 0x0e4d, 0x100 }, /* GCNENC_SMEM, opcode = (8bit)<<18 (GCN1.2) */
    { 0x0f4d, 0x100 }, /* GCNENC_VOPC, opcode = (8bit)<<27 (GCN1.2) */
    { 0x104d, 0x100 }, /* GCNENC_VOP1, opcode = (8bit)<<9 (GCN1.2) */
    { 0x114d, 0x40 }, /* GCNENC_VOP2, opcode = (6bit)<<25 (GCN1.2) */
    { 0x118d, 0x400 }, /* GCNENC_VOP3A, opcode = (10bit)<<16 (GCN1.2) */
    { 0x118d, 0x400 }, /* GCNENC_VOP3B, opcode = (10bit)<<16 (GCN1.2) */
    { 0x158d, 0x4 }, /* GCNENC_VINTRP, opcode = (2bit)<<16 (GCN1.2) */
    { 0x1591, 0x100 }, /* GCNENC_DS, opcode = (8bit)<<18 (GCN1.2) */
    { 0x1691, 0x80 }, /* GCNENC_MUBUF, opcode = (7bit)<<18 (GCN1.2) */
    { 0x1711, 0x10 }, /* GCNENC_MTBUF, opcode = (4bit)<<16 (GCN1.2) */
    { 0x1721, 0x80 }, /* GCNENC_MIMG, opcode = (7bit)<<18 (GCN1.2) */
    { 0x17a1, 0x1 }, /* GCNENC_EXP, opcode = none (GCN1.2) */
    { 0x17a2, 0x80 }, /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
    { 0x1822, 0x40 }, /* GCNENC_VOP2, opcode = (6bit)<<25 (RXVEGA) */
    { 0x1862, 0x400 }, /* GCNENC_VOP3B, opcode = (10bit)<<17  (RXVEGA) */
    { 0x1c62, 0x100 }, /* GCNENC_VOP1, opcode = (8bit)<<9 (RXVEGA) */
    { 0x1d62, 0x80 }, /* GCNENC_FLAT_SCRATCH, opcode = (8bit)<<18 (???8bit) RXVEGA */
    { 0x1de2, 0x80 }  /* GCNENC_FLAT_GLOBAL, opcode = (8bit)<<18 (???8bit) RXVEGA */
};

// total instruction table length
static const size_t gcnInstrTableByCodeLength = 0x1e62;

// create main instruction table
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
    
    // fill up main instruction table
    for (cxuint i = 0; gcnInstrsTable[i].mnemonic != nullptr; i++)
    {
        const GCNInstruction& instr = gcnInstrsTable[i];
        const GCNEncodingSpace& encSpace = gcnInstrTableByCodeSpaces[instr.encoding];
        if ((instr.archMask & ARCH_GCN_1_0_1) != 0)
        {
            if (gcnInstrTableByCode[encSpace.offset + instr.code].mnemonic == nullptr)
                gcnInstrTableByCode[encSpace.offset + instr.code] = instr;
            else if((instr.archMask & ARCH_RX2X0) != 0)
            {
                /* otherwise we for GCN1.1 */
                const GCNEncodingSpace& encSpace2 =
                        gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+1];
                gcnInstrTableByCode[encSpace2.offset + instr.code] = instr;
            }
            // otherwise we ignore this entry
        }
        if ((instr.archMask & ARCH_GCN_1_2_4) != 0)
        {
            // for GCN 1.2/1.4
            const GCNEncodingSpace& encSpace3 = gcnInstrTableByCodeSpaces[
                        GCNENC_MAXVAL+3+instr.encoding];
            if (gcnInstrTableByCode[encSpace3.offset + instr.code].mnemonic == nullptr)
                gcnInstrTableByCode[encSpace3.offset + instr.code] = instr;
            else if((instr.archMask & ARCH_GCN_1_4) != 0 &&
                (instr.encoding == GCNENC_VOP2 || instr.encoding == GCNENC_VOP1 ||
                instr.encoding == GCNENC_VOP3A || instr.encoding == GCNENC_VOP3B))
            {
                /* otherwise we for GCN1.4 */
                const bool encNoVOP2 = instr.encoding != GCNENC_VOP2;
                const bool encVOP1 = instr.encoding == GCNENC_VOP1;
                // choose FLAT_GLOBAL or FLAT_SCRATCH space
                const GCNEncodingSpace& encSpace4 =
                    gcnInstrTableByCodeSpaces[2*GCNENC_MAXVAL+4 + encNoVOP2 + encVOP1];
                gcnInstrTableByCode[encSpace4.offset + instr.code] = instr;
            }
            else if((instr.archMask & ARCH_GCN_1_4) != 0 &&
                instr.encoding == GCNENC_FLAT && (instr.mode & GCN_FLAT_MODEMASK) != 0)
            {
                /* FLAT SCRATCH and GLOBAL instructions */
                const cxuint encFlatMode = (instr.mode & GCN_FLAT_MODEMASK)-1;
                const GCNEncodingSpace& encSpace4 =
                    gcnInstrTableByCodeSpaces[2*(GCNENC_MAXVAL+1)+2+3 + encFlatMode];
                gcnInstrTableByCode[encSpace4.offset + instr.code] = instr;
            }
            // otherwise we ignore this entry
        }
    }
}

GCNDisassembler::GCNDisassembler(Disassembler& disassembler)
        : ISADisassembler(disassembler), instrOutOfCode(false)
{
    callOnce(clrxGCNDisasmOnceFlag, initializeGCNDisassembler);
}

GCNDisassembler::~GCNDisassembler()
{ }

// gcn encoding sizes table: true - if 8 byte encoding, false - 4 byte encoding
// for GCN1.0/1.1
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

// for GCN1.2/1.4
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
    const bool isGCN12 = (arch >= GPUArchitecture::GCN1_2);
    const bool isGCN14 = (arch == GPUArchitecture::GCN1_4);
    size_t pos;
    for (pos = 0; pos < codeWordsNum; pos++)
    {
        /* scan all instructions and get jump addresses */
        const uint32_t insnCode = ULEV(codeWords[pos]);
        if ((insnCode & 0x80000000U) != 0)
        {
            if ((insnCode & 0x40000000U) == 0)
            {
                // SOP???
                if  ((insnCode & 0x30000000U) == 0x30000000U)
                {
                    // SOP1/SOPK/SOPC/SOPP
                    const uint32_t encPart = (insnCode & 0x0f800000U);
                    if (encPart == 0x0e800000U)
                    {
                        // SOP1
                        if ((insnCode&0xff) == 0xff) // literal
                            pos++;
                    }
                    else if (encPart == 0x0f000000U)
                    {
                        // SOPC
                        if ((insnCode&0xff) == 0xff ||
                            (insnCode&0xff00) == 0xff00) // literal
                            pos++;
                    }
                    else if (encPart == 0x0f800000U)
                    {
                        // SOPP
                        const cxuint opcode = (insnCode>>16)&0x7f;
                        if (opcode == 2 || (opcode >= 4 && opcode <= 9) ||
                            // GCN1.1 and GCN1.2 opcodes
                            ((isGCN11 || isGCN12) &&
                                    (opcode >= 23 && opcode <= 26))) // if jump
                            labels.push_back(startOffset +
                                    ((pos+int16_t(insnCode&0xffff)+1)<<2));
                    }
                    else
                    {
                        // SOPK
                        const cxuint opcode = (insnCode>>23)&0x1f;
                        if ((!isGCN12 && opcode == 17) ||
                            (isGCN12 && opcode == 16) || // if branch fork
                            (isGCN14 && opcode == 21)) // if s_call_b64
                            labels.push_back(startOffset +
                                    ((pos+int16_t(insnCode&0xffff)+1)<<2));
                        else if ((!isGCN12 && opcode == 21) ||
                            (isGCN12 && opcode == 20))
                            pos++; // additional literal
                    }
                }
                else
                {
                    // SOP2
                    if ((insnCode&0xff) == 0xff || (insnCode&0xff00) == 0xff00)
                        pos++;  // literal
                }
            }
            else
            {
                // SMRD and others
                const uint32_t encPart = (insnCode&0x3c000000U)>>26;
                if ((!isGCN12 && gcnSize11Table[encPart] && (encPart != 7 || isGCN11)) ||
                    (isGCN12 && gcnSize12Table[encPart]))
                    pos++;
            }
        }
        else
        {
            // some vector instructions
            if ((insnCode & 0x7e000000U) == 0x7c000000U)
            {
                // VOPC
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                    pos++;
            }
            else if ((insnCode & 0x7e000000U) == 0x7e000000U)
            {
                // VOP1
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                    pos++;
            }
            else
            {
                // VOP2
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

// table of opcode positions in encoding (GCN1.0/1.1)
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
    { 18, 7 } /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
};

// table of opcode positions in encoding (GCN1.2/1.4)
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
    { 18, 7 } /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
};


/* main routine */

void GCNDisassembler::disassemble()
{
    // select current label and reloc to first
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
    // set up GCN indicators
    const bool isGCN11 = (arch == GPUArchitecture::GCN1_1);
    const bool isGCN124 = (arch >= GPUArchitecture::GCN1_2);
    const bool isGCN14 = (arch >= GPUArchitecture::GCN1_4);
    const GPUArchMask curArchMask = 
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
        {
            /* fix for GalliumCOmpute disassemblying (assembler doesn't accep 
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
            {
                // SOP???
                if  ((insnCode & 0x30000000U) == 0x30000000U)
                {
                    // SOP1/SOPK/SOPC/SOPP
                    const uint32_t encPart = (insnCode & 0x0f800000U);
                    if (encPart == 0x0e800000U)
                    {
                        // SOP1
                        if ((insnCode&0xff) == 0xff) // literal
                        {
                            if (pos < codeWordsNum)
                                insnCode2 = ULEV(codeWords[pos++]);
                        }
                        gcnEncoding = GCNENC_SOP1;
                    }
                    else if (encPart == 0x0f000000U)
                    {
                        // SOPC
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
                        if ((!isGCN124 && opcode == 21) ||
                            (isGCN124 && opcode == 20))
                        {
                            if (pos < codeWordsNum)
                                insnCode2 = ULEV(codeWords[pos++]);
                        }
                    }
                }
                else
                {
                    // SOP2
                    if ((insnCode&0xff) == 0xff || (insnCode&0xff00) == 0xff00)
                    {
                        // literal
                        if (pos < codeWordsNum)
                            insnCode2 = ULEV(codeWords[pos++]);
                    }
                    gcnEncoding = GCNENC_SOP2;
                }
            }
            else
            {
                // SMRD and others
                const uint32_t encPart = (insnCode&0x3c000000U)>>26;
                if ((!isGCN124 && gcnSize11Table[encPart] && (encPart != 7 || isGCN11)) ||
                    (isGCN124 && gcnSize12Table[encPart]))
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                if (isGCN124)
                    gcnEncoding = gcnEncoding12Table[encPart];
                else
                    gcnEncoding = gcnEncoding11Table[encPart];
                if (gcnEncoding == GCNENC_FLAT && !isGCN11 && !isGCN124)
                    gcnEncoding = GCNENC_NONE; // illegal if not GCN1.1
            }
        }
        else
        {
            // some vector instructions
            if ((insnCode & 0x7e000000U) == 0x7c000000U)
            {
                // VOPC
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN124 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOPC;
            }
            else if ((insnCode & 0x7e000000U) == 0x7e000000U)
            {
                // VOP1
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN124 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOP1;
            }
            else
            {
                // VOP2
                const cxuint opcode = (insnCode >> 25)&0x3f;
                if ((!isGCN124 && (opcode == 32 || opcode == 33)) ||
                    (isGCN124 && (opcode == 23 || opcode == 24 ||
                    opcode == 36 || opcode == 37))) // V_MADMK and V_MADAK
                {
                    if (pos < codeWordsNum)
                        insnCode2 = ULEV(codeWords[pos++]);
                }
                else if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN124 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
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
            if (disassembler.getFlags() & DISASM_CODEPOS)
            {
                // print code position
                bufPos += itocstrCStyle(startOffset+(oldPos<<2),
                                buf+bufPos, 20, 16, 12, false);
                buf[bufPos++] = ':';
                buf[bufPos++] = ' ';
            }
            bufPos += itocstrCStyle(insnCode, buf+bufPos, 12, 16, 8, false);
            buf[bufPos++] = ' ';
            // if instruction is two word long
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
            if (disassembler.getFlags() & DISASM_CODEPOS)
            {
                // print only code position
                char* buf = output.reserve(30);
                size_t bufPos = 0;
                buf[bufPos++] = '/';
                buf[bufPos++] = '*';
                bufPos += itocstrCStyle(startOffset+(oldPos<<2),
                                buf+bufPos, 20, 16, 12, false);
                buf[bufPos++] = '*';
                buf[bufPos++] = '/';
                buf[bufPos++] = ' ';
                output.forward(bufPos);
            }
            else
            {
                // add spaces
                char* buf = output.reserve(8);
                output.forward(addSpacesOld(buf, 8));
            }
        }
        
        if (gcnEncoding == GCNENC_NONE)
        {
            // invalid encoding
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
                    (isGCN124) ? gcnEncodingOpcode12Table : gcnEncodingOpcodeTable;
            const cxuint opcode =
                    (insnCode>>encodingOpcodeTable[gcnEncoding].bitPos) & 
                    ((1U<<encodingOpcodeTable[gcnEncoding].bits)-1U);
            
            /* decode instruction and put to output */
            const GCNEncodingSpace& encSpace = 
                (isGCN124) ? gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+3 + gcnEncoding] :
                  gcnInstrTableByCodeSpaces[gcnEncoding];
            const GCNInstruction* gcnInsn = gcnInstrTableByCode.get() +
                    encSpace.offset + opcode;
            
            const GCNInstruction defaultInsn = { nullptr, gcnInsn->encoding, GCN_STDMODE,
                        0, 0 };
            
            // try to replace by FMA_MIX for VEGA20
            if ((curArchMask&ARCH_VEGA20) != 0 && gcnInsn->code>=928 && gcnInsn->code<=930)
            {
                const GCNEncodingSpace& encSpace4 =
                    gcnInstrTableByCodeSpaces[2*GCNENC_MAXVAL+4 + 1];
                const GCNInstruction* thisGCNInstr =
                        gcnInstrTableByCode.get() + encSpace4.offset + opcode;
                if (thisGCNInstr->mnemonic != nullptr)
                    // replace
                    gcnInsn = thisGCNInstr;
            }
            
            cxuint spacesToAdd = 16;
            bool isIllegal = false;
            if (!isGCN124 && gcnInsn->mnemonic != nullptr &&
                (curArchMask & gcnInsn->archMask) == 0 &&
                gcnEncoding == GCNENC_VOP3A)
            {    /* new overrides (VOP3A) */
                const GCNEncodingSpace& encSpace2 =
                        gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+1];
                gcnInsn = gcnInstrTableByCode.get() + encSpace2.offset + opcode;
                if (gcnInsn->mnemonic == nullptr ||
                        (curArchMask & gcnInsn->archMask) == 0)
                    isIllegal = true; // illegal
            }
            else if (isGCN14 && gcnInsn->mnemonic != nullptr &&
                (curArchMask & gcnInsn->archMask) == 0 &&
                (gcnEncoding == GCNENC_VOP3A || gcnEncoding == GCNENC_VOP2 ||
                    gcnEncoding == GCNENC_VOP1))
            {
                /* new overrides (VOP1/VOP3A/VOP2 for GCN 1.4) */
                const GCNEncodingSpace& encSpace4 =
                        gcnInstrTableByCodeSpaces[2*GCNENC_MAXVAL+4 +
                                (gcnEncoding != GCNENC_VOP2) +
                                (gcnEncoding == GCNENC_VOP1)];
                gcnInsn = gcnInstrTableByCode.get() + encSpace4.offset + opcode;
                if (gcnInsn->mnemonic == nullptr ||
                        (curArchMask & gcnInsn->archMask) == 0)
                    isIllegal = true; // illegal
            }
            else if (isGCN14 && gcnEncoding == GCNENC_FLAT && ((insnCode>>14)&3)!=0)
            {
                // GLOBAL_/SCRATCH_* instructions
                const GCNEncodingSpace& encSpace4 =
                    gcnInstrTableByCodeSpaces[2*(GCNENC_MAXVAL+1)+2+3 +
                        ((insnCode>>14)&3)-1];
                gcnInsn = gcnInstrTableByCode.get() + encSpace4.offset + opcode;
                if (gcnInsn->mnemonic == nullptr ||
                        (curArchMask & gcnInsn->archMask) == 0)
                    isIllegal = true; // illegal
            }
            else if (gcnInsn->mnemonic == nullptr ||
                (curArchMask & gcnInsn->archMask) == 0)
                isIllegal = true;
            
            if (!isIllegal)
            {
                // put spaces between mnemonic and operands
                size_t k = ::strlen(gcnInsn->mnemonic);
                output.writeString(gcnInsn->mnemonic);
                spacesToAdd = spacesToAdd>=k+1?spacesToAdd-k:1;
            }
            else
            {
                // print illegal instruction mnemonic
                char* bufStart = output.reserve(40);
                char* bufPtr = bufStart;
                if (!isGCN124 || gcnEncoding != GCNENC_SMEM)
                    putChars(bufPtr, gcnEncodingNames[gcnEncoding],
                            ::strlen(gcnEncodingNames[gcnEncoding]));
                else /* SMEM encoding */
                    putChars(bufPtr, "SMEM", 4);
                putChars(bufPtr, "_ill_", 5);
                // opcode value
                bufPtr += itocstrCStyle(opcode, bufPtr , 6);
                const size_t linePos = bufPtr-bufStart;
                spacesToAdd = spacesToAdd >= (linePos+1)? spacesToAdd - linePos : 1;
                gcnInsn = &defaultInsn;
                output.forward(bufPtr-bufStart);
            }
            
            // determine float literal type to display
            const FloatLitType displayFloatLits = 
                    ((disassembler.getFlags()&DISASM_FLOATLITS) != 0) ?
                    (((gcnInsn->mode & GCN_MASK2) == GCN_FLOATLIT) ? FLTLIT_F32 : 
                    ((gcnInsn->mode & GCN_MASK2) == GCN_F16LIT) ? FLTLIT_F16 :
                     FLTLIT_NONE) : FLTLIT_NONE;
            
            // print instruction in correct encoding
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
                    if (isGCN124)
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
