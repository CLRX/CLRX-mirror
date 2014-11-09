/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
#include <CLRX/Utilities.h>
#include <CLRX/Assembler.h>
#include <CLRX/MemAccess.h>
#include "AsmInternals.h"

using namespace CLRX;

static std::once_flag clrxGCNDisasmOnceFlag; 
static GCNInstruction* gcnInstrTableByCode = nullptr;

static bool checkGCN11(GPUDeviceType deviceType)
{ return deviceType >= GPUDeviceType::BONAIRE && deviceType != GPUDeviceType::HAINAN; }

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

static const GCNEncodingSpace gcnInstrTableByCodeSpaces[GCNENC_MAXVAL+1] =
{
    { 0, 0 },
    { 0, 0x80 }, // GCNENC_SOPC, opcode = (7bit)<<16 */
    { 0x0080, 0x80 }, // GCNENC_SOPP, opcode = (7bit)<<16 */
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
    { 0x092d, 0x100 } /* GCNENC_FLAT, opcode = (8bit)<<18 (???8bit) */
};

static const size_t gcnInstrTableByCodeLength = 0x0a2d;

static void initializeGCNDisassembler()
{
    gcnInstrTableByCode = new GCNInstruction[gcnInstrTableByCodeLength];
    for (cxuint i = 0; i < gcnInstrTableByCodeLength; i++)
    {
        gcnInstrTableByCode[i].mnemonic = nullptr;
        gcnInstrTableByCode[i].mode = GCN_STDMODE;
    }
    
    for (cxuint i = 0; gcnInstrsTable[i].mnemonic != nullptr; i++)
    {
        const GCNInstruction& instr = gcnInstrsTable[i];
        const GCNEncodingSpace& encSpace = gcnInstrTableByCodeSpaces[instr.encoding];
        gcnInstrTableByCode[encSpace.offset + instr.code] = instr;
    }
}

GCNDisassembler::GCNDisassembler(Disassembler& disassembler)
        : ISADisassembler(disassembler)
{
    std::call_once(clrxGCNDisasmOnceFlag, initializeGCNDisassembler);
}

GCNDisassembler::~GCNDisassembler()
{ }

void GCNDisassembler::beforeDisassemble()
{
    if ((inputSize&3) != 0)
        throw Exception("Input code size must be aligned to 4 bytes!");
    const uint32_t* codeWords = reinterpret_cast<const uint32_t*>(input);
    
    const bool isGCN11 = checkGCN11(disassembler.getInput()->deviceType);
    size_t pos;
    for (pos = 0; pos < (inputSize>>2);)
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
                            // GCN1.1 opcodes
                            (isGCN11 && (opcode >= 23 && opcode <= 26))) // if jump
                            labels.push_back(pos+int16_t(insnCode&0xffff)+1);
                    }
                    else
                    {   // SOPK
                        const cxuint opcode = (insnCode>>23)&0x1f;
                        if (opcode == 17) // if branch fork
                            labels.push_back(pos+int16_t(insnCode&0xffff)+1);
                        else if (opcode == 21)
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
                const uint32_t encPart = (insnCode&0x3c000000U);
                if (encPart == 0x10000000U) // VOP3
                    pos++;
                else if (encPart == 0x18000000U || (encPart == 0x1c000000U && isGCN11) ||
                        encPart == 0x20000000U || encPart == 0x28000000U ||
                        encPart == 0x30000000U || encPart == 0x38000000U)
                    pos++; // all DS,FLAT,MUBUF,MTBUF,MIMG,EXP have 8-byte opcode
            }
        }
        else
        {   // some vector instructions
            if ((insnCode & 0x7e000000U) == 0x7c000000U)
            {   // VOPC
                if ((insnCode&0x1ff) == 0xff) // literal
                    pos++;
            }
            else if ((insnCode & 0x7e000000U) == 0x7e000000U)
            {   // VOP1
                if ((insnCode&0x1ff) == 0xff) // literal
                    pos++;
            }
            else
            {   // VOP2
                const cxuint opcode = (insnCode >> 25)&0x3f;
                if (opcode == 32 || opcode == 33) // V_MADMK and V_MADAK
                    pos++;  // inline 32-bit constant
                else if ((insnCode&0x1ff) == 0xff)
                    pos++;  // literal
            }
        }
        pos++;
    }
    if (pos != (inputSize>>2))
        throw Exception("Instruction outside of code space!");
    
    std::sort(labels.begin(), labels.end());
    const auto newEnd = std::unique(labels.begin(), labels.end());
    labels.resize(newEnd-labels.begin());
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

struct GCNEncodingOpcodeBits
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

static const char* gcnOperandFloatTable[] =
{
    "0.5", "-0.5", "1.0", "-1.0", "2.0", "-2.0", "4.0", "-4.0"
};

union FloatUnion
{
    float f;
    uint32_t u;
};

static size_t regRanges(cxuint op, cxuint vregNum, char* buf)
{
    size_t pos = 0;
    if (vregNum!=1)
        buf[pos++] = '[';
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
    if (vregNum!=1)
    {
        buf[pos++] = ':';
        op += vregNum-1;
        if (op > 255)
            op -= 256; // fix for VREGS
        val = op;
        if (val >= 100)
        {
            cxuint digit2 = val/100U;
            buf[pos++] = digit2+'0';
            val -= digit2*100U;
        }
        {
            const cxuint digit1 = val/10U;
            if (digit1 != 0 || op >= 100)
                buf[pos++] = digit1 + '0';
            buf[pos++] = val-digit1*10U + '0';
        }
        buf[pos++] = ']';
    }
    return pos;
}

static size_t decodeGCNVRegOperand(cxuint op, cxuint vregNum, char* buf)
{
    buf[0] = 'v';
    return regRanges(op, vregNum, buf+1)+1;
}

static size_t decodeGCNOperand(cxuint op, cxuint regNum, char* buf, uint16_t arch,
           uint32_t literal = 0, bool floatLit = false)
{
    if (op < 104 || (op >= 256 && op < 512))
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
        (op2 == 104 && (arch&ARCH_RX2X0)!=0))
    {   // VCC
        size_t pos = 0;
        switch(op2)
        {
            case 104:
                memcpy(buf+pos, "flat_scratch", 12);
                pos += 12;
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
                buf[pos++] = '&';
                buf[pos++] = 'i';
                buf[pos++] = 'l';
                buf[pos++] = 'l';
                buf[pos++] = '!';
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
    {
        size_t pos = itocstrCStyle(literal, buf, 11, 16);
        if (floatLit)
        {
            FloatUnion fu;
            fu.u = literal;
            buf[pos++] = ' ';
            buf[pos++] = '/';
            buf[pos++] = '*';
            buf[pos++] = ' ';
            pos += ftocstrCStyle(fu.f, buf+pos, 27);
            buf[pos++] = 'f';
            buf[pos++] = ' ';
            buf[pos++] = '*';
            buf[pos++] = '/';
        }
        return pos;
    }
    
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
            size_t pos = 2;
            buf[pos++] = '&';
            buf[pos++] = 'i';
            buf[pos++] = 'l';
            buf[pos++] = 'l';
            buf[pos++] = '!';
            return pos;
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
        case 251:
            buf[0] = 'v';
            buf[1] = 'c';
            buf[2] = 'c';
            buf[3] = 'z';
            return 4;
        case 252:
            buf[0] = 'e';
            buf[1] = 'x';
            buf[2] = 'e';
            buf[3] = 'c';
            buf[4] = 'z';
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

static size_t decodeSOPCEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bufPos += decodeGCNOperand(insnCode&0xff,
           (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf + bufPos, arch, literal);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand((insnCode>>8)&0xff,
           (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos, arch, literal);
    return bufPos;
}

static size_t decodeSOPPEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal, size_t pos)
{
    size_t bufPos = 0;
    const cxuint imm16 = insnCode&0xffff;
    switch(gcnInsn.mode&GCN_MASK1)
    {
        case GCN_IMM_REL:
        {
            const size_t branchPos = pos + int16_t(imm16);
            bufPos = addSpaces(buf, spacesToAdd);
            buf[bufPos++] = 'L';
            bufPos += itocstrCStyle(branchPos, buf+bufPos, 22, 10, 0, false);
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
            {   /* LGKMCNT bits is 4 */
                const cxuint lockCnt = (imm16>>8)&15;
                if (prevLock)
                {
                    buf[bufPos++] = ' ';
                    buf[bufPos++] = '&';
                    buf[bufPos++] = ' ';
                }
                ::memcpy(buf+bufPos, "lgkmcnt(", 8);
                bufPos += 8;
                if (lockCnt >= 10)
                    buf[bufPos++] = '1';
                buf[bufPos++] = '0' + ((lockCnt>=10)?lockCnt-10:lockCnt);
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
    return bufPos;
}

static size_t decodeSOP1Encoding(cxuint spacesToAdd, uint16_t arch, char* buf,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
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
    
    return bufPos;
}

static size_t decodeSOP2Encoding(cxuint spacesToAdd, uint16_t arch, char* buf,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
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
        buf[bufPos++] = ' ';
        buf[bufPos++] = 's';
        buf[bufPos++] = 'd';
        buf[bufPos++] = 's';
        buf[bufPos++] = 't';
        buf[bufPos++] = '=';
        bufPos += itocstrCStyle(((insnCode>>16)&0x7f), buf+bufPos, 6, 16);
    }
    return bufPos;
}

static const char* hwregNames[13] =
{
    "0", "MODE", "STATUS", "TRAPSTS",
    "HW_ID", "GPR_ALLOC", "LDS_ALLOC", "IB_STS",
    "PC_LO", "PC_HI", "INST_DW0", "INST_DW1",
    "IB_DBG0"
};

static size_t decodeSOPKEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal, size_t pos)
{
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
        const size_t branchPos = pos + int16_t(imm16);
        buf[bufPos++] = 'L';
        bufPos += itocstrCStyle(branchPos, buf+bufPos, 22, 10, 0, false);
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
        if (hwoffset >= 10)
        {
            const cxuint digit2 = hwoffset/10U;
            buf[bufPos++] = '0' + digit2;
            buf[bufPos++] = '0' + hwoffset - 10U*digit2;
        }
        else
            buf[bufPos++] = '0' + hwoffset;
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        const cxuint hwsize = ((imm16>>11)&31)+1;
        if (hwsize >= 10)
        {
            const cxuint digit2 = hwsize/10U;
            buf[bufPos++] = '0' + digit2;
            buf[bufPos++] = '0' + hwsize - 10U*digit2;
        }
        else
            buf[bufPos++] = '0' + hwsize;
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
    return bufPos;
}

static size_t decodeSMRDEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal)
{
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
        const cxuint dregsNum = 1<<((gcnInsn.mode & GCN_MASK2)>>GCN_SHIFT2);
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
    return bufPos;
}

static size_t decodeVOPCEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         bool displayFloatLits)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    buf[bufPos++] = 'v';
    buf[bufPos++] = 'c';
    buf[bufPos++] = 'c';
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand(insnCode&0x1ff, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                           buf + bufPos, arch, literal, displayFloatLits);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNVRegOperand(((insnCode>>9)&0xff),
           (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos);
    return bufPos;
}

static size_t decodeVOP1Encoding(cxuint spacesToAdd, uint16_t arch, char* buf,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         bool displayFloatLits)
{
    size_t bufPos = 0;
    if ((gcnInsn.mode & GCN_MASK1) != GCN_ARG_NONE)
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
        bufPos += decodeGCNOperand(insnCode&0x1ff, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                           buf + bufPos, arch, literal, displayFloatLits);
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
            ::memcpy(buf+bufPos, "vsrc=", 5);
            bufPos += 5;
            bufPos += itocstrCStyle(insnCode&0x1ff, buf+bufPos, 6, 16);
        }
    }
    return bufPos;
}

static size_t decodeVOP2Encoding(cxuint spacesToAdd, uint16_t arch, char* buf,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
         bool displayFloatLits)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    
    if (mode1 == GCN_DS1_SGPR)
        bufPos += decodeGCNOperand(((insnCode>>17)&0xff),
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos, arch);
    else
        bufPos += decodeGCNVRegOperand(((insnCode>>17)&0xff),
               (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos);
    if (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'v';
        buf[bufPos++] = 'c';
        buf[bufPos++] = 'c';
    }
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand(insnCode&0x1ff, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                       buf + bufPos, arch, literal, displayFloatLits);
    if (mode1 == GCN_ARG1_IMM)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += itocstrCStyle(literal, buf+bufPos, 11, 16);
        if (displayFloatLits)
        {
            FloatUnion fu;
            fu.u = literal;
            buf[bufPos++] = ' ';
            buf[bufPos++] = '/';
            buf[bufPos++] = '*';
            buf[bufPos++] = ' ';
            bufPos += ftocstrCStyle(fu.f, buf+bufPos, 27);
            buf[bufPos++] = 'f';
            buf[bufPos++] = ' ';
            buf[bufPos++] = '*';
            buf[bufPos++] = '/';
        }
    }
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    if (mode1 == GCN_DS1_SGPR || mode1 == GCN_SRC1_SGPR)
        bufPos += decodeGCNOperand(((insnCode>>9)&0xff),
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos, arch);
    else
        bufPos += decodeGCNVRegOperand(((insnCode>>9)&0xff),
               (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos);
    if (mode1 == GCN_ARG2_IMM)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += itocstrCStyle(literal, buf+bufPos, 11, 16);
        if (displayFloatLits)
        {
            FloatUnion fu;
            fu.u = literal;
            buf[bufPos++] = ' ';
            buf[bufPos++] = '/';
            buf[bufPos++] = '*';
            buf[bufPos++] = ' ';
            bufPos += ftocstrCStyle(fu.f, buf+bufPos, 27);
            buf[bufPos++] = 'f';
            buf[bufPos++] = ' ';
            buf[bufPos++] = '*';
            buf[bufPos++] = '/';
        }
    }
    else if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'v';
        buf[bufPos++] = 'c';
        buf[bufPos++] = 'c';
    }
    return bufPos;
}

static size_t decodeVOP3Encoding(cxuint spacesToAdd, uint16_t arch, char* buf,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insn2Code,
         uint32_t literal, bool displayFloatLits)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    const cxuint opcode = ((insnCode>>17)&0x1ff);
    
    const cxuint vdst = insnCode&0xff;
    const cxuint vsrc0 = insn2Code&0x1ff;
    const cxuint vsrc1 = (insn2Code>>9)&0x1ff;
    const cxuint vsrc2 = (insn2Code>>18)&0x1ff;
    const cxuint sdst = (insnCode>>8)&0x7f;
    const uint16_t mode1 = (gcnInsn.mode & GCN_MASK1);
    if (opcode < 256) /* if compares */
        bufPos += decodeGCNOperand(vdst, 2, buf + bufPos, arch);
    else /* regular instruction */
        bufPos += decodeGCNVRegOperand(vdst,
           (gcnInsn.mode&GCN_REG_DST_64)?2:1, buf + bufPos);
    
    cxuint absFlags = 0;
    if (gcnInsn.encoding == GCNENC_VOP3A)
        absFlags = (insnCode>>8)&7;
    else if (mode1 == GCN_DS2_VCC || mode1 == GCN_DST_VCC) /* VOP3b */
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNOperand(((insnCode>>8)&0x7f), 2, buf + bufPos, arch);
    }
    
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    if ((insn2Code & (1U<<29)) != 0)
        buf[bufPos++] = '-';
    if (absFlags & 1)
    {
        buf[bufPos++] = 'a';
        buf[bufPos++] = 'b';
        buf[bufPos++] = 's';
        buf[bufPos++] = '(';
    }
    bufPos += decodeGCNOperand(vsrc0, (gcnInsn.mode&GCN_REG_SRC0_64)?2:1,
                           buf + bufPos, arch, literal, displayFloatLits);
    if (absFlags & 1)
        buf[bufPos++] = ')';
    
    bool vsrc1Used = false;
    bool vsrc2Used = false;
    if (mode1 != GCN_SRC12_NONE)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        if ((insn2Code & (1U<<30)) != 0)
            buf[bufPos++] = '-';
        if (absFlags & 2)
        {
            buf[bufPos++] = 'a';
            buf[bufPos++] = 'b';
            buf[bufPos++] = 's';
            buf[bufPos++] = '(';
        }
        bufPos += decodeGCNOperand(vsrc1,
           (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf + bufPos, arch, literal,
                   displayFloatLits);
        if (absFlags & 2)
            buf[bufPos++] = ')';
        
        if (mode1 != GCN_SRC2_NONE && opcode >= 256)
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            
            if (mode1 == GCN_DS2_VCC || mode1 == GCN_SRC2_VCC)
                bufPos += decodeGCNOperand(vsrc2, 2, buf + bufPos, arch);
            else
            {
                if ((insn2Code & (1U<<31)) != 0)
                    buf[bufPos++] = '-';
                if (absFlags & 4)
                {
                    buf[bufPos++] = 'a';
                    buf[bufPos++] = 'b';
                    buf[bufPos++] = 's';
                    buf[bufPos++] = '(';
                }
                bufPos += decodeGCNOperand(vsrc2,
                   (gcnInsn.mode&GCN_REG_SRC2_64)?2:1, buf + bufPos, arch, literal,
                           displayFloatLits);
                if (absFlags & 4)
                    buf[bufPos++] = ')';
            }
            vsrc2Used = true;
        }
        vsrc1Used = true;
    }
    
    const cxuint omod = (insn2Code>>27)&3;
    if (omod != 0)
    {
        const char* omodStr = (omod==3)?" div:2":(omod==2)?" mul:4":" mul:2";
        ::memcpy(buf+bufPos, omodStr, 6);
        bufPos += 6;
    }
    
    if (gcnInsn.encoding == GCNENC_VOP3A && (insnCode&0x800) != 0)
    {
        ::memcpy(buf+bufPos, " clamp", 6);
        bufPos += 6;
    }
    
    /* as field */
    if (!vsrc1Used && vsrc1 != 0)
    {
        ::memcpy(buf+bufPos, " src1=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(vsrc1, buf+bufPos, 6, 16);
    }
    if (!vsrc2Used && vsrc2 != 0)
    {
        ::memcpy(buf+bufPos, " src2=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(vsrc2, buf+bufPos, 6, 16);
    }
    
    const cxuint usedMask = (vsrc2Used?4:0) | (vsrc1Used?2:0) | 1;
    /* check whether instruction is this same like VOP2/VOP1/VOPC */
    bool isVOP1Word = false;
    if ((insnCode&(usedMask<<8)) == 0)
    {
        if (opcode < 256 && vdst == 106 /* vcc */ && omod==0 &&
            (insn2Code&(usedMask<<29)) == 0 &&
            vsrc0 >= 256 && vsrc1 >= 256 && vsrc2 == 0)
            isVOP1Word = true;
        else if ((gcnInsn.mode&GCN_MASK2) == GCN_VOP3_VOP1 && omod==0 &&
            (insn2Code&(usedMask<<29)) == 0 &&
            ((!vsrc1Used && vsrc0 == 0) || vsrc0 >= 256) && vsrc1 == 0 && vsrc2 == 0)
            isVOP1Word = true;
        else if ((gcnInsn.mode&GCN_MASK2) == GCN_VOP3_VOP2 && omod==0 &&
            (insn2Code&(usedMask<<29)) == 0 &&
            ((!vsrc1Used && vsrc0 == 0) || vsrc0 >= 256) && 
            ((!vsrc2Used && vsrc1 == 0) || vsrc1 >= 256) && vsrc2 == 0)
            isVOP1Word = true;
    }
    else if (gcnInsn.encoding == GCNENC_VOP3B && omod==0 && vsrc0 >= 256 &&
            (insn2Code&(usedMask<<29)) == 0 &&
            vsrc1 >= 256 && sdst == 106 /* vcc */ && vsrc2 == 0) /* VOP3b */
        isVOP1Word = true;
    
    if (isVOP1Word) // add vop3 for distinguishing encoding
    {
        ::memcpy(buf+bufPos, " vop3", 5);
        bufPos += 5;
    }
    
    return bufPos;
}

static size_t decodeVINTRPEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
           const GCNInstruction& gcnInsn, uint32_t insnCode)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bufPos += decodeGCNVRegOperand((insnCode>>18)&0xff, 1, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNVRegOperand(insnCode&0xff, 1, buf+bufPos);
    ::memcpy(buf+bufPos, ", attr", 6);
    bufPos += 6;
    const cxuint attr = (insnCode>>10)&63;
    if (attr >= 10)
    {
        const cxuint digit2 = attr/10U;
        buf[bufPos++] = '0' + digit2;
        buf[bufPos++] = '0' + attr - 10U/10*digit2;
    }
    else
        buf[bufPos++] = '0' + attr;
    buf[bufPos++] = '.';
    buf[bufPos++] = "xyzw"[((insnCode>>8)&3)]; // attrchannel
    return bufPos;
}

static size_t decodeDSEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
           const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insn2Code)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bool vdstUsed = false;
    bool vaddrUsed = false;
    bool vdata0Used = false;
    bool vdata1Used = false;
    
    if ((gcnInsn.mode & GCN_ADDR_DST) != 0 ||
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) == 0) /* address is dst */
    {
        bufPos += decodeGCNVRegOperand(insn2Code&0xff, 1, buf+bufPos);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        vaddrUsed = true;
    }
    else
    {   /* vdst is dst */
        cxuint regsNum = (gcnInsn.mode&GCN_REG_DST_64)?2:1;
        if ((gcnInsn.mode&GCN_DSMASK) == GCN_ADDR_SRC96)
            regsNum = 3;
        if ((gcnInsn.mode&GCN_DSMASK) == GCN_ADDR_SRC128)
            regsNum = 4;
        bufPos += decodeGCNVRegOperand(insn2Code>>24, regsNum, buf+bufPos);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        vdstUsed = true;
    }
    
    
    if ((gcnInsn.mode & GCN_DSMASK2) != GCN_ONLYDST &&
        (gcnInsn.mode & (GCN_ADDR_DST|GCN_ADDR_SRC)) != 0)
    {   /* two vdata */
        bufPos += decodeGCNVRegOperand((insn2Code>>8)&0xff,
            (gcnInsn.mode&GCN_REG_SRC0_64)?2:1, buf+bufPos);
        vdata0Used = true;
        if ((gcnInsn.mode & GCN_DSMASK2) == GCN_2SRCS ||
            (gcnInsn.mode & GCN_DSMASK2) == GCN_VDATA2)
        {
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += decodeGCNVRegOperand((insn2Code>>16)&0xff,
                (gcnInsn.mode&GCN_REG_SRC1_64)?2:1, buf+bufPos);
            vdata1Used = true;
        }
    }
    
    const cxuint offset = (insnCode&0xffff);
    if (offset != 0)
    {
        if ((gcnInsn.mode & GCN_DSMASK2) != GCN_VDATA2) /* single offset */
        {
            ::memcpy(buf+bufPos, "offset:", 8);
            bufPos += 8;
            bufPos += itocstrCStyle(offset, buf+bufPos, 7, 10);
        }
        else
        {
            if ((offset&0xff) != 0)
            {
                ::memcpy(buf+bufPos, "offset0:", 9);
                bufPos += 9;
                bufPos += itocstrCStyle(offset&0xff, buf+bufPos, 7, 10);
            }
            if ((offset&0xff00) != 0)
            {
                ::memcpy(buf+bufPos, "offset1:", 9);
                bufPos += 9;
                bufPos += itocstrCStyle((offset>>8)&0xff, buf+bufPos, 7, 10);
            }
        }
    }
    
    if (insnCode&0x10000)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'g';
        buf[bufPos++] = 'd';
        buf[bufPos++] = 's';
    }
    
    if (!vaddrUsed && (insn2Code&0xff) != 0)
    {
        ::memcpy(buf+bufPos, " vaddr=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle((insn2Code&0xff), buf+bufPos, 6, 16);
    }
    if (!vdata0Used && ((insn2Code>>8)&0xff) != 0)
    {
        ::memcpy(buf+bufPos, " vdata0=", 8);
        bufPos += 8;
        bufPos += itocstrCStyle((insn2Code>>8)&0xff, buf+bufPos, 6, 16);
    }
    if (!vdata1Used && ((insn2Code>>16)&0xff) != 0)
    {
        ::memcpy(buf+bufPos, " vdata1=", 8);
        bufPos += 8;
        bufPos += itocstrCStyle((insn2Code>>16)&0xff, buf+bufPos, 6, 16);
    }
    if (!vdstUsed && (insn2Code>>24) != 0)
    {
        ::memcpy(buf+bufPos, " vdst=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(insn2Code>>24, buf+bufPos, 6, 16);
    }
    return bufPos;
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

static size_t decodeMUBUFEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
      const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insn2Code, bool mtbuf)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    const cxuint dregsNum = ((gcnInsn.mode&GCN_MASK2)>>GCN_SHIFT2)+1;
    bufPos += decodeGCNVRegOperand((insn2Code>>8)&0xff,
               dregsNum, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    // determine number of vaddr registers
    const cxuint aregsNum = (((insnCode & 0x1000U)? 1 : 0) +
            ((insnCode & 0x2000U)? 1 : 0))<<((insnCode & 0x8000U)?1:0);
    bufPos += decodeGCNVRegOperand(insn2Code&0xff, aregsNum, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand(((insn2Code>>14)&3), 4, buf+bufPos, arch);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand((insn2Code>>24), 1, buf+bufPos, arch);
    
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
    ::memcpy(buf+bufPos, "offset:", 9);
    bufPos += 9;
    bufPos += itocstrCStyle(insnCode&0xfff, buf+bufPos, 7, 16);
    if (insnCode & 0x4000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'g';
        buf[bufPos++] = 'l';
        buf[bufPos++] = 'c';
    }
    if (insn2Code & 0x400000U)
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
    if (insn2Code & 0x800000U)
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
    return bufPos;
}

static size_t decodeMIMGEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
         const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insn2Code)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bufPos += decodeGCNVRegOperand((insn2Code>>8)&0xff, 2, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    // determine number of vaddr registers
    bufPos += decodeGCNVRegOperand(insn2Code&0xff, 4, buf+bufPos);
    buf[bufPos++] = ',';
    buf[bufPos++] = ' ';
    bufPos += decodeGCNOperand(((insn2Code>>14)&3), 4, buf+bufPos, arch);
    if ((gcnInsn.mode & GCN_MASK2) == GCN_MIMG_SAMPLE)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNOperand(((insn2Code>>19)&3), 4, buf+bufPos, arch);
    }
    ::memcpy(buf+bufPos, " dmask:", 7);
    bufPos += 7;
    const cxuint dmask =  (insnCode>>8)&15;
    if (dmask >= 10)
    {
        buf[bufPos++] = '1';
        buf[bufPos++] = '0' + dmask - 10;
    }
    else
        buf[bufPos++] = '0' + dmask;
    
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
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'r';
        buf[bufPos++] = '1';
        buf[bufPos++] = '2';
        buf[bufPos++] = '8';
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
    return bufPos;
}

static size_t decodeEXPEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
        const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insn2Code)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    /* export target */
    const cxuint target = (insnCode>>4)&63;
    if (target >= 32)
    {
        buf[bufPos++] = 'p';
        buf[bufPos++] = 'a';
        buf[bufPos++] = 'r';
        buf[bufPos++] = 'a';
        buf[bufPos++] = 'm';
        const cxuint tpar = target-32;
        if (tpar >= 10)
        {
            const cxuint digit2 = tpar/10;
            buf[bufPos++] = '0' + digit2;
            buf[bufPos++] = '0' + tpar - 10*digit2;
        }
        else
            buf[bufPos++] = '0' + tpar;
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
        const cxuint digit2 = target/10;
        buf[bufPos++] = '0' + digit2;
        buf[bufPos++] = '0' + target - 10*digit2;
    }
    
    /* vdata registers */
    for (cxuint i = 0; i < 4; i++)
    {
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        if (insnCode & (1U<<i))
            bufPos += decodeGCNVRegOperand((insn2Code>>(i<<3))&0xff, 1, buf+bufPos);
        else
        {
            buf[bufPos++] = 'o';
            buf[bufPos++] = 'f';
            buf[bufPos++] = 'f';
        }
    }
    
    if (insnCode&0x800)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'd';
        buf[bufPos++] = 'o';
        buf[bufPos++] = 'n';
        buf[bufPos++] = 'e';
    }
    if (insnCode&0x400)
    {
        ::memcpy(buf+bufPos, " compr", 6);
        bufPos += 6;
    }
    if (insnCode&0x100)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 'v';
        buf[bufPos++] = 'm';
    }
    return bufPos;
}

static size_t decodeFLATEncoding(cxuint spacesToAdd, uint16_t arch, char* buf,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t insn2Code)
{
    size_t bufPos = addSpaces(buf, spacesToAdd);
    bool vdstUsed = false;
    bool vdataUsed = false;
    const cxuint dregsNum = ((gcnInsn.mode&GCN_MASK2)>>GCN_SHIFT2)+1;
    if ((gcnInsn.mode & GCN_FLAT_ADST) == 0)
    {
        vdstUsed = true;
        bufPos += decodeGCNVRegOperand(insn2Code>>24, dregsNum, buf+bufPos);
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNVRegOperand(insn2Code&0xff, 2, buf+bufPos); // addr
    }
    else
    {   /* two vregs, because 64-bitness stored in PTR32 mode (at runtime) */
        bufPos += decodeGCNVRegOperand(insn2Code&0xff, 2, buf+bufPos); // addr
        if ((gcnInsn.mode & GCN_FLAT_NODST) != 0)
        {
            vdstUsed = true;
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += decodeGCNVRegOperand(insn2Code>>24, dregsNum, buf+bufPos);
        }
    }
    
    if ((gcnInsn.mode & GCN_FLAT_NODATA) == 0) /* print data */
    {
        vdataUsed = true;
        buf[bufPos++] = ',';
        buf[bufPos++] = ' ';
        bufPos += decodeGCNVRegOperand((insn2Code>>8)&0xff, dregsNum, buf+bufPos);
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
    if (insn2Code & 0x800000U)
    {
        buf[bufPos++] = ' ';
        buf[bufPos++] = 't';
        buf[bufPos++] = 'f';
        buf[bufPos++] = 'e';
    }
    
    if (!vdataUsed && ((insn2Code>>8)&0xff) != 0)
    {
        ::memcpy(buf+bufPos, " vdata=", 7);
        bufPos += 7;
        bufPos += itocstrCStyle((insn2Code>>8)&0xff, buf+bufPos, 6, 16);
    }
    if (!vdstUsed && (insn2Code>>24) != 0)
    {
        ::memcpy(buf+bufPos, " vdst=", 6);
        bufPos += 6;
        bufPos += itocstrCStyle(insn2Code>>24, buf+bufPos, 6, 16);
    }
    return bufPos;
}

/* main routine */

void GCNDisassembler::disassemble()
{
    if ((inputSize&3) != 0)
        throw Exception("Input code size must be aligned to 4 bytes!");
    
    auto curLabel = labels.begin();
    const uint32_t* codeWords = reinterpret_cast<const uint32_t*>(input);
    
    const bool isGCN11 = checkGCN11(disassembler.getInput()->deviceType);
    const uint16_t curArchMask = isGCN11?ARCH_RX2X0:ARCH_HD7X00;
    std::ostream& output = disassembler.getOutput();
    
    char buf[384];
    size_t bufPos = 0;
    const size_t codeWordsNum = (inputSize>>2);
    for (size_t pos = 0; pos < codeWordsNum;)
    {   // check label
        if (curLabel != labels.end() && pos == *curLabel)
        {   // put label
            buf[bufPos++] = 'L';
            bufPos += itocstrCStyle((pos<<2), buf+bufPos, 22, 10, 0, false);
            buf[bufPos++] = ':';
            buf[bufPos++] = '\n';
            if (bufPos+128 >= 384)
            {
                output.write(buf, bufPos);
                bufPos = 0;
            }
            curLabel++;
        }
        
        // add spaces
        for (size_t p = 0; p < 8; p++)
            buf[bufPos+p] = ' ';
        bufPos += 8;
        
        cxbyte gcnEncoding = GCNENC_NONE;
        const uint32_t insnCode = ULEV(codeWords[pos++]);
        uint32_t insn2Code = 0;
        uint32_t literal = 0;
        
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
                            if (pos >= codeWordsNum)
                                throw Exception("Instruction outside code space!");
                            literal = ULEV(codeWords[pos++]);
                        }
                        gcnEncoding = GCNENC_SOP1;
                    }
                    else if (encPart == 0x0f000000U)
                    {   // SOPC
                        if ((insnCode&0xff) == 0xff ||
                            (insnCode&0xff00) == 0xff00) // literal
                        {
                            if (pos >= codeWordsNum)
                                throw Exception("Instruction outside code space!");
                            literal = ULEV(codeWords[pos++]);
                        }
                        gcnEncoding = GCNENC_SOPC;
                    }
                    else if (encPart == 0x0f800000U) // SOPP
                        gcnEncoding = GCNENC_SOPP;
                    else // SOPK
                    {
                        gcnEncoding = GCNENC_SOPK;
                        if (((insnCode>>23)&0x1f) == 21)
                        {
                            if (pos >= codeWordsNum)
                                throw Exception("Instruction outside code space!");
                            literal = ULEV(codeWords[pos++]);
                        }
                    }
                }
                else
                {   // SOP2
                    if ((insnCode&0xff) == 0xff || (insnCode&0xff00) == 0xff00)
                    {   // literal
                        if (pos >= codeWordsNum)
                            throw Exception("Instruction outside code space!");
                        literal = ULEV(codeWords[pos++]);
                    }
                    gcnEncoding = GCNENC_SOP2;
                }
            }
            else
            {   // SMRD and others
                const uint32_t encPart = (insnCode&0x3c000000U);
                if (encPart == 0x10000000U)// VOP3
                {
                    if (pos >= codeWordsNum)
                        throw Exception("Instruction outside code space!");
                    insn2Code = ULEV(codeWords[pos++]);
                }
                else if (encPart == 0x18000000U || (encPart == 0x1c000000U && isGCN11) ||
                        encPart == 0x20000000U || encPart == 0x28000000U ||
                        encPart == 0x30000000U || encPart == 0x38000000U)
                {   // all DS,FLAT,MUBUF,MTBUF,MIMG,EXP have 8-byte opcode
                    if (pos+1 >= codeWordsNum)
                        throw Exception("Instruction outside code space!");
                    insn2Code = ULEV(codeWords[pos++]);
                }
                gcnEncoding = gcnEncoding11Table[(encPart>>26)&0xf];
                if (gcnEncoding == GCNENC_FLAT && !isGCN11)
                    gcnEncoding = GCNENC_NONE; // illegal if not GCN1.1
            }
        }
        else
        {   // some vector instructions
            if ((insnCode & 0x7e000000U) == 0x7c000000U)
            {   // VOPC
                if ((insnCode&0x1ff) == 0xff) // literal
                {
                    if (pos >= codeWordsNum)
                        throw Exception("Instruction outside code space!");
                    literal = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOPC;
            }
            else if ((insnCode & 0x7e000000U) == 0x7e000000U)
            {   // VOP1
                if ((insnCode&0x1ff) == 0xff) // literal
                {
                    if (pos >= codeWordsNum)
                        throw Exception("Instruction outside code space!");
                    literal = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOP1;
            }
            else
            {   // VOP2
                const cxuint opcode = (insnCode >> 25)&0x3f;
                if (opcode == 32 || opcode == 33) // V_MADMK and V_MADAK
                {
                    if (pos >= codeWordsNum)
                        throw Exception("Instruction outside code space!");
                    literal = ULEV(codeWords[pos++]);
                }
                else if ((insnCode&0x1ff) == 0xff)
                {
                    if (pos >= codeWordsNum)
                        throw Exception("Instruction outside code space!");
                    literal = ULEV(codeWords[pos++]);
                }
                gcnEncoding = GCNENC_VOP2;
            }
        }
        
        if (gcnEncoding == GCNENC_NONE)
        {   // invalid encoding
            buf[bufPos++] = '.';
            buf[bufPos++] = 'i';
            buf[bufPos++] = 'n';
            buf[bufPos++] = 't';
            buf[bufPos++] = ' '; 
            bufPos += itocstrCStyle(insnCode, buf+bufPos, 11, 16);
        }
        else
        {
            const cxuint opcode = 
                    (insnCode>>gcnEncodingOpcodeTable[gcnEncoding].bitPos) & 
                    ((1U<<gcnEncodingOpcodeTable[gcnEncoding].bits)-1U);
            
            /* decode instruction and put to output */
            const GCNEncodingSpace& encSpace = gcnInstrTableByCodeSpaces[gcnEncoding];
            const GCNInstruction* gcnInsn = gcnInstrTableByCode + encSpace.offset + opcode;
            
            GCNInstruction defaultInsn = { nullptr, gcnInsn->encoding, GCN_STDMODE,
                        0, 0 };
            cxuint spacesToAdd = 16;
            if (gcnInsn->mnemonic != nullptr &&
                (curArchMask & gcnInsn->archMask) != 0)
            {
                size_t k;
                const char* mnem = gcnInsn->mnemonic;
                for (k = 0; mnem[k]!=0; k++)
                    buf[bufPos++] = mnem[k];
                spacesToAdd = spacesToAdd>=k+1?spacesToAdd-k:1;
            }
            else
            {
                const size_t oldBufPos = bufPos;
                for (cxuint k = 0; gcnEncodingNames[gcnEncoding][k] != 0; k++)
                    buf[bufPos++] = gcnEncodingNames[gcnEncoding][k];
                buf[bufPos++] = '_';
                buf[bufPos++] = 'i';
                buf[bufPos++] = 'l';
                buf[bufPos++] = 'l';
                buf[bufPos++] = '_';
                bufPos += itocstrCStyle(opcode, buf + bufPos, 6);
                spacesToAdd = spacesToAdd >= (bufPos-oldBufPos+1)?
                    spacesToAdd - (bufPos-oldBufPos) : 1;
                gcnInsn = &defaultInsn;
            }
            
            const bool displayFloatLits = 
                    ((disassembler.getFlags()&DISASM_FLOATLITS) != 0 &&
                    (gcnInsn->mode & GCN_MASK2) == GCN_FLOATLIT);
            
            switch(gcnEncoding)
            {
                case GCNENC_SOPC:
                    bufPos += decodeSOPCEncoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, literal);
                    break;
                case GCNENC_SOPP:
                    bufPos += decodeSOPPEncoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, literal, pos);
                    break;
                case GCNENC_SOP1:
                    bufPos += decodeSOP1Encoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, literal);
                    break;
                case GCNENC_SOP2:
                    bufPos += decodeSOP2Encoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, literal);
                    break;
                case GCNENC_SOPK:
                    bufPos += decodeSOPKEncoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, literal, pos);
                    break;
                case GCNENC_SMRD:
                    bufPos += decodeSMRDEncoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, literal);
                    break;
                case GCNENC_VOPC:
                    bufPos += decodeVOPCEncoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, literal, displayFloatLits);
                    break;
                case GCNENC_VOP1:
                    bufPos += decodeVOP1Encoding(spacesToAdd, curArchMask,
                             buf+bufPos, *gcnInsn, insnCode, literal, displayFloatLits);
                    break;
                case GCNENC_VOP2:
                    bufPos += decodeVOP2Encoding(spacesToAdd, curArchMask,
                             buf+bufPos, *gcnInsn, insnCode, literal, displayFloatLits);
                    break;
                case GCNENC_VOP3A:
                    bufPos += decodeVOP3Encoding(spacesToAdd, curArchMask, buf+bufPos,
                             *gcnInsn, insnCode, insn2Code, literal, displayFloatLits);
                    break;
                case GCNENC_VINTRP:
                    bufPos += decodeVINTRPEncoding(spacesToAdd, curArchMask,
                                   buf+bufPos, *gcnInsn, insnCode);
                    break;
                case GCNENC_DS:
                    bufPos += decodeDSEncoding(spacesToAdd, curArchMask,
                                   buf+bufPos, *gcnInsn, insnCode, insn2Code);
                    break;
                case GCNENC_MUBUF:
                    bufPos += decodeMUBUFEncoding(spacesToAdd, curArchMask,
                                    buf+bufPos, *gcnInsn, insnCode, insn2Code, false);
                    break;
                case GCNENC_MTBUF:
                    bufPos += decodeMUBUFEncoding(spacesToAdd, curArchMask,
                                  buf+bufPos, *gcnInsn, insnCode, insn2Code, true);
                    break;
                case GCNENC_MIMG:
                    bufPos += decodeMIMGEncoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, insn2Code);
                    break;
                case GCNENC_EXP:
                    bufPos += decodeEXPEncoding(spacesToAdd, curArchMask,
                                buf+bufPos, *gcnInsn, insnCode, insn2Code);
                    break;
                case GCNENC_FLAT:
                    bufPos += decodeFLATEncoding(spacesToAdd, curArchMask,
                                 buf+bufPos, *gcnInsn, insnCode, insn2Code);
                    break;
                default:
                    break;
            }
        }
        buf[bufPos++] = '\n';
        
        if (bufPos+128 >= 384)
        {
            output.write(buf, bufPos);
            bufPos = 0;
        }
    }
    /* rest of the labels */
    for (; curLabel != labels.end(); ++curLabel)
    {   // put .org directory
        if (codeWordsNum != *curLabel)
        {
            buf[bufPos++] = '.';
            buf[bufPos++] = 'o';
            buf[bufPos++] = 'r';
            buf[bufPos++] = 'g';
            buf[bufPos++] = ' ';
            bufPos += itocstrCStyle((*curLabel)<<2, buf+bufPos, 20, 16);
            buf[bufPos++] = '\n';
        }
        // put label
        buf[bufPos++] = 'L';
        bufPos += itocstrCStyle(*curLabel, buf+bufPos, 22, 10, 0, false);
        buf[bufPos++] = ':';
        buf[bufPos++] = '\n';
        if (bufPos+40 >= 384)
        {
            output.write(buf, bufPos);
            bufPos = 0;
        }
    }
    output.write(buf, bufPos);
    output.flush();
}
