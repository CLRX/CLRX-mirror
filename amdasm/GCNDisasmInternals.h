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

#ifndef __CLRX_GCNDISASMINTERNALS_H__
#define __CLRX_GCNDISASMINTERNALS_H__

#include <CLRX/Config.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Disassembler.h>
#include "GCNInternals.h"

namespace CLRX
{

// type of floating litera
enum FloatLitType: cxbyte
{
    FLTLIT_NONE,    // none
    FLTLIT_F32,     // single precision
    FLTLIT_F16      // half precision
};

// GCN disassembler code in structure (this allow to access private code of
// GCNDisassembler by these routines
struct CLRX_INTERNAL GCNDisasmUtils
{
    typedef GCNDisassembler::RelocIter RelocIter;
    static void printLiteral(GCNDisassembler& dasm, size_t codePos, RelocIter& relocIter,
              uint32_t literal, FloatLitType floatLit, bool optional);
    // decode GCN operand (version without literal)
    static void decodeGCNOperandNoLit(GCNDisassembler& dasm, cxuint op, cxuint regNum,
              char*& bufPtr, GPUArchMask arch, FloatLitType floatLit = FLTLIT_NONE);
    // decodee GCN operand (include literal, can decode relocations)
    static char* decodeGCNOperand(GCNDisassembler& dasm, size_t codePos,
              RelocIter& relocIter, cxuint op, cxuint regNum, GPUArchMask arch,
              uint32_t literal = 0, FloatLitType floatLit = FLTLIT_NONE);
    
    static void decodeSOPCEncoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSOPPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
             uint32_t literal, size_t pos);
    
    static void decodeSOP1Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSOP2Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSOPKEncoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal);
    
    static void decodeSMRDEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
            GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode);
    
    static void decodeSMEMEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
            GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2);
    
    static void decodeVOPCEncoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
             FloatLitType displayFloatLits);
    
    static void decodeVOP1Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
             FloatLitType displayFloatLits);
    
    static void decodeVOP2Encoding(GCNDisassembler& dasm,
             size_t codePos, RelocIter& relocIter, cxuint spacesToAdd, GPUArchMask arch,
             const GCNInstruction& gcnInsn, uint32_t insnCode, uint32_t literal,
             FloatLitType displayFloatLits);
    
    static void decodeVOP3Encoding(GCNDisassembler& dasm, cxuint spacesToAdd,
            GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2, FloatLitType displayFloatLits);
    
    static void decodeVINTRPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode);
    
    static void decodeDSEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
             uint32_t insnCode2);
    
    static void decodeMUBUFEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
            GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
            uint32_t insnCode2);
    
    static void decodeMIMGEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
             uint32_t insnCode2);
    
    static void decodeEXPEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
             uint32_t insnCode2);

    static void printFLATAddr(cxuint flatMode, char*& bufPtr, uint32_t insnCode2);
    
    static void decodeFLATEncoding(GCNDisassembler& dasm, cxuint spacesToAdd,
             GPUArchMask arch, const GCNInstruction& gcnInsn, uint32_t insnCode,
             uint32_t insnCode2);
};

};

#endif
