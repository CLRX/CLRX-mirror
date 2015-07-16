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

#ifndef __CLRX_ASMINTERNALS_H__
#define __CLRX_ASMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <CLRX/utils/Utilities.h>

namespace CLRX
{

enum : cxbyte
{
    GCNENC_NONE,
    GCNENC_SOPC,    /* 0x17e<<23, opcode = (7bit)<<16 */
    GCNENC_SOPP,    /* 0x17f<<23, opcode = (7bit)<<16 */
    GCNENC_SOP1,    /* 0x17d<<23, opcode = (8bit)<<8 */
    GCNENC_SOP2,    /* 0x2<<30,   opcode = (7bit)<<23 */
    GCNENC_SOPK,    /* 0xb<<28,   opcode = (5bit)<<23 */
    GCNENC_SMRD,    /* 0x18<<27,  opcode = (6bit)<<22 */
    GCNENC_SMEM = GCNENC_SMRD,    /* 0x18<<27,  opcode = (6bit)<<22 */
    GCNENC_VOPC,    /* 0x3e<<25,  opcode = (8bit)<<17 */
    GCNENC_VOP1,    /* 0x3f<<25,  opcode = (8bit)<<9 */
    GCNENC_VOP2,    /* 0x0<<31,   opcode = (6bit)<<25 */
    GCNENC_VOP3A,   /* 0x34<<26,  opcode = (9bit)<<17 */
    GCNENC_VOP3B,   /* 0x34<<26,  opcode = (9bit)<<17 */
    GCNENC_VINTRP,  /* 0x32<<26,  opcode = (2bit)<<16 */
    GCNENC_DS,      /* 0x36<<26,  opcode = (8bit)<<18 */
    GCNENC_MUBUF,   /* 0x38<<26,  opcode = (7bit)<<18 */
    GCNENC_MTBUF,   /* 0x3a<<26,  opcode = (3bit)<<16 */
    GCNENC_MIMG,    /* 0x3c<<26,  opcode = (7bit)<<18 */
    GCNENC_EXP,     /* 0x3e<<26,  opcode = none */
    GCNENC_FLAT,    /* 0x37<<26,  opcode = (8bit)<<18 (???8bit) */
    GCNENC_MAXVAL = GCNENC_FLAT
};

enum : uint16_t
{
    ARCH_SOUTHERN_ISLANDS = 1,
    ARCH_SEA_ISLANDS = 2,
    ARCH_VOLCANIC_ISLANDS = 4,
    ARCH_HD7X00 = 1,
    ARCH_RX2X0 = 2,
    ARCH_RX3X0 = 4,
    ARCH_GCN_1_0_1 = 0x3,
    ARCH_GCN_1_1_2 = 0x6,
    ARCH_GCN_ALL = 0xffff,
};

enum : uint16_t
{
    GCN_STDMODE = 0,
    GCN_REG_ALL_64 = 15,
    GCN_REG_DST_64 = 1,
    GCN_REG_SRC0_64 = 2,
    GCN_REG_SRC1_64 = 4,
    GCN_REG_SRC2_64 = 8,
    GCN_REG_DS0_64 = 3,
    GCN_REG_DS1_64 = 5,
    GCN_REG_DS2_64 = 9,
    /* SOP */
    GCN_IMM_NONE = 0x10, // used in Scall insns
    GCN_ARG_NONE = 0x20,
    GCN_REG_S1_JMP = 0x20,
    GCN_IMM_REL = 0x30,
    GCN_IMM_LOCKS = 0x40,
    GCN_IMM_MSGS = 0x50,
    GCN_IMM_SREG = 0x60,
    GCN_SRC_NONE = 0x70,
    GCN_DST_NONE = 0x80,
    GCN_IMM_DST = 0x100,
    GCN_SOPK_CONST = 0x200,
    GCN_SOPK_SRIMM32 = 0x300,
    /* SOPC */
    GCN_SRC1_IMM = 0x10,
    /* VOP */
    GCN_SRC2_NONE = 0x10,
    GCN_DS2_VCC = 0x20,
    GCN_SRC12_NONE = 0x30,
    GCN_ARG1_IMM = 0x40,
    GCN_ARG2_IMM = 0x50,
    GCN_S0EQS12 = 0x60,
    GCN_DST_VCC = 0x70,
    GCN_SRC2_VCC = 0x80,
    GCN_DST_VCC_VSRC2 = 0x90,
    GCN_DS1_SGPR = 0xa0,
    GCN_SRC1_SGPR = 0xb0,
    GCN_DST_SGPR = 0xc0,
    GCN_VOP_ARG_NONE = 0xd0,
    GCN_NEW_OPCODE = 0xe0,
    GCN_P0_P10_P20 = 0xf0,
    GCN_VOP3_VOP2 = 0x100,
    GCN_VOP3_VOP1 = 0x200,
    GCN_VOP3_VINTRP = 0x300,
    GCN_VOP3_VINTRP_NEW = 0x3e0,
    GCN_VOP3_VOP2_DS12 = 0x110,
    GCN_VOP3_VOP1_DS1 = 0x230,
    GCN_VOP3_DST_SGPR = 0x400,
    GCN_VOP3_SRC1_SGPR = 0x800,
    GCN_VOP3_DS1_SGPR = 0xc00,
    GCN_VOP3_MASK2 = 0x300,
    GCN_VINTRP_SRC2 = 0x1000,
    GCN_VOP3_MASK3 = 0xf000,
    // DS encoding modes
    GCN_ADDR_NONE = 0x0,
    GCN_ADDR_DST = 0x10,
    GCN_ADDR_SRC = 0x20,
    GCN_ADDR_DST64 = 0x1f,
    GCN_ADDR_SRC64 = 0x2f,
    GCN_ADDR_SRC_D64 = 0x21,
    GCN_2OFFSETS = 0x100, /* two offset2 */
    GCN_VDATA2 = 0x140, /* two datas, two offsets1 */
    GCN_NOSRC = 0x80, /* only address */
    GCN_2SRCS = 0x40, /* two datas, one offset */
    GCN_NOSRC_2OFF = 0x180, /* only address */
    GCN_SRC_ADDR2  = 0x200,
    GCN_SRC_ADDR2_64  = 0x20f,
    GCN_DS_96  = 0x800,
    GCN_DS_128  = 0x1000,
    GCN_ONLYDST = 0x400, /* only vdst */
    GCN_DSMASK = 0x3f0,
    GCN_DSMASK2 = 0x3c0,
    GCN_SRCS_MASK = 0xc0,
    // others
    GCN_SBASE4 = 0x10,
    GCN_FLOATLIT = 0x100,
    GCN_F16LIT = 0x200,
    GCN_SMRD_ONLYDST = 0x30,
    GCN_SMEM_SDATA_IMM = 0x40,
    GCN_MEMOP_MX1 = 0x0,
    GCN_MEMOP_MX2 = 0x100,
    GCN_MEMOP_MX4 = 0x200,
    GCN_MEMOP_MX8 = 0x300,
    GCN_MEMOP_MX16 = 0x400,
    GCN_MUBUF_X = 0x0,
    GCN_MUBUF_NOVAD = 0x10, /* no vaddr and vdata */
    GCN_MUBUF_XY = 0x100,
    GCN_MUBUF_XYZ = 0x200,
    GCN_MUBUF_XYZW = 0x300,
    GCN_MUBUF_MX1 = 0x0,
    GCN_MUBUF_MX2 = 0x100,
    GCN_MUBUF_MX3 = 0x200,
    GCN_MUBUF_MX4 = 0x300,
    GCN_MIMG_SAMPLE = 0x100,
    GCN_FLAT_DDST = 0x00,
    GCN_FLAT_ADST = 0x10,
    GCN_FLAT_NODATA = 0x20,
    GCN_FLAT_NODST = 0x40,
    GCN_FLAT_STORE = 0x50,
    GCN_CMPSWAP =  0x80,
    GCN_MASK1 = 0xf0,
    GCN_MASK2 = 0xf00,
    GCN_DSIZE_MASK = 0x700,
    GCN_SHIFT2 = 8
};

struct CLRX_INTERNAL GCNInstruction
{
    const char* mnemonic;
    cxbyte encoding;
    uint16_t mode;
    uint16_t code;
    uint16_t archMask; // mask of architectures whose have instruction
};

CLRX_INTERNAL extern const GCNInstruction gcnInstrsTable[];

static inline void skipCharAndSpacesToEnd(const char*& string, const char* end)
{
    ++string;
    while (string!=end && *string == ' ') string++;
}

static inline void skipSpacesToEnd(const char*& string, const char* end)
{ while (string!=end && *string == ' ') string++; }

// extract sybol name or argument name or other identifier
static inline const std::string extractSymName(const char* startString, const char* end,
           bool localLabelSymName)
{
    const char* string = startString;
    if (string != end)
    {
        if(isAlpha(*string) || *string == '_' || *string == '.' || *string == '$')
            for (string++; string != end && (isAlnum(*string) || *string == '_' ||
                 *string == '.' || *string == '$') ; string++);
        else if (localLabelSymName && isDigit(*string)) // local label
        {
            for (string++; string!=end && isDigit(*string); string++);
            // check whether is local forward or backward label
            string = (string != end && (*string == 'f' || *string == 'b')) ?
                    string+1 : startString;
            // if not part of binary number or illegal bin number
            if (startString != string && (string!=end && (isAlnum(*string))))
                string = startString;
        }
    }
    if (startString == string) // not parsed
        return std::string();
    
    return std::string(startString, string);
}

static inline const std::string extractLabelName(const char* startString, const char* end)
{
    if (startString != end && isDigit(*startString))
    {
        const char* string = startString;
        while (string != end && isDigit(*string)) string++;
        return std::string(startString, string);
    }
    return extractSymName(startString, end, false);
}

class Assembler;

enum class IfIntComp
{
    EQUAL = 0,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL
};

struct CLRX_INTERNAL AsmPseudoOps
{
    /* IMPORTANT:
     * about string argumenbt - string points to place of current line
     * processed by assembler
     * pseudoOpPlace - points to first character from line of pseudo-op name
     */
    static bool checkGarbagesAtEnd(Assembler& asmr, const char* linePtr);
    /* parsing helpers */
    /* get absolute value arg resolved at this time.
       if empty expression value is not set */
    static bool getAbsoluteValueArg(Assembler& asmr, uint64_t& value, const char*& linePtr,
                    bool requiredExpr = false);
    
    static bool getAnyValueArg(Assembler& asmr, uint64_t& value, cxuint& sectionId,
                    const char*& linePtr);
    // get name (not symbol name)
    static bool getNameArg(Assembler& asmr, std::string& outStr, const char*& linePtr,
               const char* objName, bool requiredExpr = true);
    // skip comma
    static bool skipComma(Assembler& asmr, bool& haveComma, const char*& linePtr);
    
    // skip comma for multiple argument pseudo-ops
    static bool skipCommaForMultipleArgd(Assembler& asmr, const char*& linePtr);
    
    /*
     * pseudo-ops logic
     */
    // set bitnesss
    static void setBitness(Assembler& asmr, const char*& linePtr, bool _64Bit);
    // set output format
    static void setOutFormat(Assembler& asmr, const char*& linePtr);
    // change kernel
    static void goToKernel(Assembler& asmr, const char*& linePtr);
    
    /// include file
    static void includeFile(Assembler& asmr, const char* pseudoOpPlace,
                            const char*& linePtr);
    // include binary file
    static void includeBinFile(Assembler& asmr, const char* pseudoOpPlace,
                       const char*& linePtr);
    
    // fail
    static void doFail(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr);
    // .error
    static void doError(Assembler& asmr, const char* pseudoOpPlace,
                           const char*& linePtr);
    // .warning
    static void doWarning(Assembler& asmr, const char* pseudoOpPlace,
                     const char*& linePtr);
    
    // .byte, .short, .int, .word, .long, .quad
    template<typename T>
    static void putIntegers(Assembler& asmr, const char* pseudoOpPlace,
                    const char*& linePtr);
    
    // .half, .float, .double
    template<typename UIntType>
    static void putFloats(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr);
    
    /// .string, ascii
    static void putStrings(Assembler& asmr, const char* pseudoOpPlace,
                   const char*& linePtr, bool addZero = false);
    // .string16, .string32, .string64
    template<typename T>
    static void putStringsToInts(Assembler& asmr, const char* pseudoOpPlace,
                   const char*& linePtr);
    
    /// .octa
    static void putUInt128s(Assembler& asmr, const char* pseudoOpPlace,
                        const char*& linePtr);
    
    /// .set, .equ, .eqv, .equiv
    static void setSymbol(Assembler& asmr, const char*& linePtr, bool reassign = true,
                bool baseExpr = false);
    
    // .global, .local, .extern
    static void setSymbolBind(Assembler& asmr, const char*& linePtr, cxbyte elfInfo);
    
    static void setSymbolSize(Assembler& asmr, const char*& linePtr);
    
    static void ignoreExtern(Assembler& asmr, const char*& linePtr);
    
    static void doFill(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr,
               bool _64bit = false);
    static void doSkip(Assembler& asmr, const char*& linePtr);
    
    /* TODO: add no-op fillin for text sections */
    static void doAlign(Assembler& asmr,  const char*& linePtr, bool powerOf2 = false);
    
    /* TODO: add no-op fillin for text sections */
    template<typename Word>
    static void doAlignWord(Assembler& asmr, const char* pseudoOpPlace,
                            const char*& linePtr);
    
    static void doOrganize(Assembler& asmr, const char*& linePtr);
    
    static void doPrint(Assembler& asmr, const char*& linePtr);
    
    static void doIfInt(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr,
                IfIntComp compType, bool elseIfClause);
    
    static void doIfDef(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr,
                bool negation, bool elseIfClause);
    
    static void doIfBlank(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr,
                bool negation, bool elseIfClause);
    /// .ifc
    static void doIfCmpStr(Assembler& asmr, const char* pseudoOpPlace,
               const char*& linePtr, bool negation, bool elseIfClause);
    /// ifeqs, ifnes
    static void doIfStrEqual(Assembler& asmr, const char* pseudoOpPlace,
                     const char*& linePtr, bool negation, bool elseIfClause);
    // else
    static void doElse(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr);
    // endif
    static void endIf(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr);
    /// start repetition content
    static void doRepeat(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr);
    /// end repetition content
    static void endRepeat(Assembler& asmr, const char* pseudoOpPlace,
                          const char*& linePtr);
    /// start macro definition
    static void doMacro(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr);
    /// ends macro definition
    static void endMacro(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr);
    // immediately exit from macro
    static void exitMacro(Assembler& asmr, const char* pseudoOpPlace,
                      const char*& linePtr);
    // purge macro
    static void purgeMacro(Assembler& asmr, const char*& linePtr);
    // do IRP
    static void doIRP(Assembler& asmr, const char* pseudoOpPlace, const char*& linePtr,
                      bool perChar = false);
    
    static void undefSymbol(Assembler& asmr, const char*& linePtr);
    
    static void setAbsoluteOffset(Assembler& asmr, const char*& linePtr);
    
    static void ignoreString(Assembler& asmr, const char*& linePtr);
    
    static bool checkPseudoOpName(const std::string& string);
};

class AsmGalliumHandler;

struct CLRX_INTERNAL AsmFormatPseudoOps: AsmPseudoOps
{
    static void galliumDoArgs(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char*& linePtr);
    static void galliumDoArg(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char*& linePtr);
    static void galliumProgInfo(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char*& linePtr);
    static void galliumDoEntry(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char*& linePtr);
};

extern const cxbyte tokenCharTable[96] CLRX_INTERNAL;

};

#endif
