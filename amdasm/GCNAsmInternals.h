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

#ifndef __CLRX_GCNASMINTERNALS_H__
#define __CLRX_GCNASMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <memory>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/GPUId.h>
#include "AsmInternals.h"
#include "GCNInternals.h"

namespace CLRX
{

enum : Flags {
    INSTROP_SREGS = 1,
    INSTROP_SSOURCE = 2,
    INSTROP_VREGS = 4,
    INSTROP_LDS = 8,
    INSTROP_VOP3MODS = 0x10,
    // used internally by parseGCNOperand to continuing parsing with modifiers
    INSTROP_PARSEWITHNEG = 0x20,
    INSTROP_VOP3NEG = 0x40,
    
    INSTROP_ONLYINLINECONSTS = 0x80, /// accepts only inline constants
    INSTROP_NOLITERALERROR = 0x100, /// accepts only inline constants
    INSTROP_NOLITERALERRORMUBUF = 0x200, /// accepts only inline constants
    
    INSTROP_TYPE_MASK = 0x3000,
    INSTROP_INT = 0x000,    // integer literal
    INSTROP_FLOAT = 0x1000, // floating point literal
    INSTROP_F16 = 0x2000,   // half floating point literal
    INSTROP_V64BIT = 0x3000, // 64-bit value (specific for vector instructions)
    INSTROP_UNALIGNED = 0xc000, // not aligned, use by parseRegisterRange
    INSTROP_ALIGNED = 0x0000,
    INSTROP_SGPR_UNALIGNED = 0x4000,
    
    INSTROP_READ = 0x10000,
    INSTROP_WRITE = 0x20000,
    INSTROP_ACCESS_MASK = 0x30000,
    INSTROP_NOSEXT = 0x40000,
    INSTROP_VOP3P = 0x80000, // VOP3P encoding
    
    // for parseSRregRange/parseVRegRange
    INSTROP_SYMREGRANGE = 1
};

// standard VOP operand modifers
enum: cxbyte {
    VOPOP_ABS = 1,
    VOPOP_NEG = 2,
    VOPOP_SEXT = 4
};

// VOP3 modifiers
enum: cxbyte {
    VOP3_MUL2 = 1,
    VOP3_MUL4 = 2,
    VOP3_DIV2 = 3,
    VOP3_CLAMP = 16,
    VOP3_VOP3 = 32,
    VOP3_BOUNDCTRL = 64
};

// represent register range with regvar
struct CLRX_INTERNAL RegRange
{
    uint16_t start, end;
    const AsmRegVar* regVar;
    
    RegRange(): start(0), end(0), regVar(nullptr)
    { }
    template<typename U1, typename U2>
    RegRange(U1 t1, U2 t2 = 0, const AsmRegVar* rv = nullptr)
            : start(t1), end(t2), regVar(rv)
    { }
    
    bool operator!() const
    { return start==0 && end==0; }
    operator bool() const
    { return start!=0 || end!=0; }
    // return start register or zero if regvar (start is not specified)
    uint16_t bstart() const
    { return (regVar==nullptr) ? start : 0; }
    
    bool isRegVar() const
    { return regVar!=nullptr; }
    // return if start register is reg
    bool isVal(uint16_t reg) const
    { return (regVar==nullptr && start==reg); }
    
    bool isSGPR() const
    { return (regVar==nullptr && start<108) || (regVar!=nullptr &&
                regVar->type==REGTYPE_SGPR); }
    bool isNonVGPR() const
    { return (regVar==nullptr && start<256) || (regVar!=nullptr &&
                regVar->type==REGTYPE_SGPR); }
    bool isVGPR() const
    { return (regVar==nullptr && start>=256) || (regVar!=nullptr &&
                regVar->type==REGTYPE_VGPR); }
};

// GCN operand (can be regrange or value) with VOP modifiers
struct CLRX_INTERNAL GCNOperand
{
    RegRange range;
    uint32_t value;
    cxbyte vopMods;
    
    bool operator!() const
    { return !range; }
    operator bool() const
    { return range; }
};

// VOP extra modifiers
struct CLRX_INTERNAL VOPExtraModifiers
{
    cxbyte dstSel;
    cxbyte dstUnused;
    cxbyte src0Sel;
    cxbyte src1Sel;
    cxbyte bankMask;
    cxbyte rowMask;
    uint16_t dppCtrl;
    bool needSDWA;
    bool needDPP;
};

// VOP operand modifiers (booleans)
struct CLRX_INTERNAL VOPOpModifiers
{
    cxbyte absMod;
    cxbyte negMod; // lo (low 4-bits) and hi (high 4-bits)
    cxbyte sextMod;
    cxbyte opselMod; // lo (low 4-bits) and hi (high 4-bits)
};

// GCN encoding sizes
enum class GCNEncSize
{
    UNKNOWN,
    BIT32,
    BIT64
};

enum class GCNVOPEnc
{
    NORMAL,
    DPP,
    SDWA
};

// flags for parse VOP modifiers (specifies what will be parsed)
enum {
    PARSEVOP_WITHCLAMP = 1,
    PARSEVOP_WITHSEXT = 2,
    PARSEVOP_WITHOPSEL = 4,
    PARSEVOP_VOP3P = 8, // VOP3P modifiers
    // do not parse dst_unused and dst_sel (for GCN1.4 SDWAB encoding)
    PARSEVOP_NODSTMODS = 16
};

struct CLRX_INTERNAL GCNAsmUtils: AsmParseUtils
{
    // helper that print error that specified regrange expected
    static bool printRegisterRangeExpected(Assembler& asmr, const char* linePtr,
               const char* regPoolName, cxuint regsNum, bool required);
    // helper that print error that specified regrange required
    static void printXRegistersRequired(Assembler& asmr, const char* linePtr,
               const char* regPoolName, cxuint requiredRegsNum);
    
    // parse register variable range
    static bool parseRegVarRange(Assembler& asmr, const char*& linePtr,
                 RegRange& regPair, GPUArchMask arch, cxuint regsNum, AsmRegField regField,
                 Flags flags, bool required = true);
    // parse symbol register range
    static bool parseSymRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                 GPUArchMask arch, cxuint regsNum, AsmRegField regField, Flags flags,
                 bool required = true);
    /* return true if no error */
    static bool parseVRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                   cxuint regsNum, AsmRegField regField, bool required = true,
                   Flags flags = INSTROP_SYMREGRANGE);
    /* return true if no error */
    static bool parseSRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                   GPUArchMask arch, cxuint regsNum, AsmRegField regField,
                   bool required = true, Flags flags = INSTROP_SYMREGRANGE);
    
    // bits=UINT_MAX - no range checking
    /* return true if no error */
    static bool parseImmInt(Assembler& asmr, const char*& linePtr, uint32_t& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, cxuint bits = 0,
            cxbyte signess = WS_BOTH);
    
    // parse immediate value (for field) with specified bits (can be signed)
    template<typename T>
    static bool parseImm(Assembler& asmr, const char*& linePtr, T& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, cxuint bits = 0,
            cxbyte signess = WS_BOTH)
    {
        uint32_t v32;
        bool ret = parseImmInt(asmr, linePtr, v32, outTargetExpr,
               (bits!=0?bits:(sizeof(T)<<3)), signess);
        if ((outTargetExpr==nullptr || *outTargetExpr==nullptr) && ret)
            value = v32;
        return ret;
    }
    
    // parse array of booleans
    static bool parseImmWithBoolArray(Assembler& asmr, const char*& linePtr,
            uint32_t& value, cxuint bits = 0, cxbyte signess = WS_BOTH);
    
    static bool parseLiteralImm(Assembler& asmr, const char*& linePtr, uint32_t& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, Flags instropMask = 0);
    
    static bool parseSingleOMODCLAMP(Assembler& asmr, const char*& linePtr,
                const char* modPlace, const char* mod, GPUArchMask arch, cxbyte& mods,
                VOPOpModifiers& opMods, cxuint modOperands, cxuint flags,
                bool& haveAbs, bool& haveNeg, bool& alreadyDefined, bool& good);
    
    /* withSDWAOperands - number operand that will be handled by SDWA modifer parser,
     * modOperands - number of operands for abs,neg,sext
     * (includes destination at begin) */
    static bool parseVOPModifiers(Assembler& asmr, const char*& linePtr, GPUArchMask arch,
                       cxbyte& mods, VOPOpModifiers& opMods, cxuint modOperands,
                       VOPExtraModifiers* extraMods = nullptr,
                       cxuint flags = PARSEVOP_WITHCLAMP|PARSEVOP_WITHSEXT,
                       cxuint withSDWAOperands = 3);
    
    static bool parseOperand(Assembler& asmr, const char*& linePtr, GCNOperand& operand,
               std::unique_ptr<AsmExpression>* outTargetExpr, GPUArchMask arch,
               cxuint regsNum, Flags instrOpMask, AsmRegField regField);
    
    template<typename T>
    static bool parseModImm(Assembler& asmr, const char*& linePtr, T& value,
            std::unique_ptr<AsmExpression>* outTargetExpr, const char* modName,
            cxuint bits = 0, cxbyte signess = WS_BOTH)
    {
        if (outTargetExpr!=nullptr)
            outTargetExpr->reset();
        const char* end = asmr.line+asmr.lineSize;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr==':')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            return parseImm(asmr, linePtr, value, outTargetExpr, bits, signess);
        }
        asmr.printError(linePtr, (std::string("Expected ':' before ")+modName).c_str());
        return false;
    }
    
    // parse modifier parameter (to enable or disable modifier: mod:[0:1])
    static bool parseModEnable(Assembler& asmr, const char*& linePtr, bool& value,
            const char* modName)
    {
        uint64_t val = 0;
        const char* end = asmr.line+asmr.lineSize;
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!=':')
        {
            // by default is enabled
            value = true;
            return true;
        }
        // OldModParam choice between old parametrization rules and new rules
        // in old rules (accepts only 0 or 1), in new can any value
        bool ret = parseModImm(asmr, linePtr, val, nullptr, modName,
                        asmr.isOldModParam() ? 1 : UINT_MAX, WS_UNSIGNED);
        value = val!=0;
        return ret;
    }
    
    static bool parseVINTRP0P10P20(Assembler& asmr, const char*& linePtr, RegRange& reg);
    static bool parseVINTRPAttr(Assembler& asmr, const char*& linePtr, cxbyte& attr);
    
    static bool getMUBUFFmtNameArg(Assembler& asmr, size_t maxOutStrSize, char* outStr,
               const char*& linePtr, const char* objName);
    
    // checking whether encoding size is match
    static bool checkGCNEncodingSize(Assembler& asmr, const char* insnPtr,
                     GCNEncSize gcnEncSize, uint32_t wordsNum);
    // checking whether VOP encoding is match
    static bool checkGCNVOPEncoding(Assembler& asmr, const char* insnPtr,
                     GCNVOPEnc vopEnc, const VOPExtraModifiers* modifiers);
    // checking whether VOP extra modifiers match
    static bool checkGCNVOPExtraModifers(Assembler& asmr, GPUArchMask arch,
                 bool needImm, bool sextFlags, bool vop3, GCNVOPEnc gcnVOPEnc,
                 const GCNOperand& src0Op, VOPExtraModifiers& extraMods,
                 const char* instrPlace);
    
    // routines to parse GCN encodings
    static bool parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseSOPKEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseSOPPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    
    static bool parseSMRDEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseSMEMEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    
    static bool parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc);
    static bool parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc);
    static bool parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc);
    static bool parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc);
    static bool parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize, GCNVOPEnc gcnVOPEnc);
    static bool parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseMUBUFEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    static bool parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, GPUArchMask arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs,
                      GCNEncSize gcnEncSize);
    
    static void prepareRVUAndWait(GCNAssembler* gcnAsm, GPUArchMask arch, bool vdataToRead,
            bool vdataToWrite, bool haveLds, bool haveTfe, std::vector<cxbyte>& output,
            const GCNAsmInstruction& gcnInsn);
};

static inline bool isXRegRange(RegRange pair, cxuint regsNum = 1)
{
    // second==0 - we assume that first is inline constant, otherwise we check range
    return (pair.end==0) || cxuint(pair.end-pair.start)==regsNum;
}

static inline void updateVGPRsNum(cxuint& vgprsNum, cxuint vgpr)
{
    vgprsNum = std::min(std::max(vgprsNum, vgpr+1), 256U);
}

static inline void updateSGPRsNum(cxuint& sgprsNum, cxuint sgpr, GPUArchMask arch)
{
    cxuint maxSGPRsNum = getGPUMaxRegsNumByArchMask(arch, REGTYPE_SGPR);
    if (sgpr < maxSGPRsNum) /* sgpr=-1 does not change sgprsNum */
        sgprsNum = std::min(std::max(sgprsNum, sgpr+1), maxSGPRsNum-2);
}

static inline void updateRegFlags(Flags& regFlags, cxuint reg, GPUArchMask arch)
{
    reg &= ~1;
    if (reg==106)
        regFlags |= GCN_VCC;
    else if (((arch&ARCH_GCN_1_2_4)!=0 && reg==102) ||
            ((arch&ARCH_GCN_1_2_4)==0 && reg==104))
        regFlags |= GCN_FLAT;
    else if ((arch&ARCH_GCN_1_2_4)!=0 && reg==104)
        regFlags |= GCN_XNACK;
}

};

#endif
