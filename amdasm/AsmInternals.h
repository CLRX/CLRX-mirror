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
#include <utility>
#include <CLRX/utils/Utilities.h>
#include "GCNInternals.h"

namespace CLRX
{

static inline void skipCharAndSpacesToEnd(const char*& string, const char* end)
{
    ++string;
    while (string!=end && *string == ' ') string++;
}

static inline void skipSpacesToEnd(const char*& string, const char* end)
{ while (string!=end && *string == ' ') string++; }

// extract sybol name or argument name or other identifier
static inline CString extractSymName(const char*& string, const char* end,
           bool localLabelSymName)
{
    const char* startString = string;
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
    return CString(startString, string);
}

static inline CString extractLabelName(const char*& string, const char* end)
{
    if (string != end && isDigit(*string))
    {
        const char* startString = string;
        while (string != end && isDigit(*string)) string++;
        return CString(startString, string);
    }
    return extractSymName(string, end, false);
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

struct CLRX_INTERNAL AsmParseUtils
{
    static bool checkGarbagesAtEnd(Assembler& asmr, const char* linePtr);
    /* parsing helpers */
    /* get absolute value arg resolved at this time.
       if empty expression value is not set */
    static bool getAbsoluteValueArg(Assembler& asmr, uint64_t& value, const char*& linePtr,
                    bool requiredExpr = false);
    
    static bool getAnyValueArg(Assembler& asmr, uint64_t& value, cxuint& sectionId,
                    const char*& linePtr);
    
    static bool getJumpValueArg(Assembler& asmr, uint64_t& value,
            std::unique_ptr<AsmExpression>& outTargetExpr, const char*& linePtr);
    // get name (not symbol name)
    static bool getNameArg(Assembler& asmr, CString& outStr, const char*& linePtr,
               const char* objName, bool requiredArg = true,
               bool skipCommaAtError = false);
    
    // get name (not symbol name)
    static bool getNameArg(Assembler& asmr, size_t maxOutStrSize, char* outStr,
               const char*& linePtr, const char* objName, bool requiredArg = true,
               bool ignoreLongerName = false, bool skipCommaAtError = false);
    
    // get name (not symbol name), skipping spaces at error by default
    static bool getNameArgS(Assembler& asmr, CString& outStr, const char*& linePtr,
               const char* objName, bool requiredArg = true)
    { return getNameArg(asmr, outStr, linePtr, objName, requiredArg, true); }
    
    // get name (not symbol name), skipping spaces at error by default
    static bool getNameArgS(Assembler& asmr, size_t maxOutStrSize, char* outStr,
               const char*& linePtr, const char* objName, bool requiredArg = true,
               bool ignoreLongerName = false)
    {
        return getNameArg(asmr, maxOutStrSize, outStr, linePtr, objName,
                    requiredArg, ignoreLongerName, true);
    }
    
    // skip comma
    static bool skipComma(Assembler& asmr, bool& haveComma, const char*& linePtr);
    // skip required comma, (returns false if not found comma)
    static bool skipRequiredComma(Assembler& asmr, const char*& linePtr);
    
    // skip comma for multiple argument pseudo-ops
    static bool skipCommaForMultipleArgs(Assembler& asmr, const char*& linePtr);
};

enum : Flags {
    INSTROP_SREGS = 1,
    INSTROP_SSOURCE = 2,
    INSTROP_VREGS = 4,
    INSTROP_LDS = 8,
    INSTROP_VOP3MODS = 0x10,
    INSTROP_PARSEWITHNEG = 0x20,
    INSTROP_VOP3NEG = 0x40,
    
    INSTROP_ONLYINLINECONSTS = 0x80, /// accepts only inline constants
    INSTROP_NOLITERALERROR = 0x100, /// accepts only inline constants
    
    INSTROP_TYPE_MASK = 0x3000,
    INSTROP_INT = 0x000,    // integer literal
    INSTROP_FLOAT = 0x1000, // floating point literal
    INSTROP_F16 = 0x2000,   // half floating point literal
};

enum: cxbyte {
    VOPOPFLAG_ABS = 1,
    VOPOPFLAG_NEG = 2,
    VOPOPFLAG_SEXT = 4
};

enum: cxbyte {
    VOP3_MUL2 = 1,
    VOP3_MUL4 = 2,
    VOP3_DIV2 = 3,
    VOP3_CLAMP = 16,
    VOP3_VOP3 = 32
};

struct RegRange
{
    uint16_t start, end;
    
    RegRange(): start(0), end(0) { }
    template<typename U1, typename U2>
    RegRange(U1 t1, U2 t2 = 0) : start(t1), end(t2) { }
    
    bool operator!() const
    { return start==0 && end==0; }
    operator bool() const
    { return start!=0 || end!=0; }
};

struct GCNOperand {
    RegRange range;
    uint32_t value;
    cxbyte vop3Mods;
    
    bool operator!() const
    { return !range; }
    operator bool() const
    { return range; }
};

struct CLRX_INTERNAL GCNAsmUtils: AsmParseUtils
{
    static bool printRegisterRangeExpected(Assembler& asmr, const char* linePtr,
               const char* regPoolName, cxuint regsNum, bool required);
    
    static void printXRegistersRequired(Assembler& asmr, const char* linePtr,
               const char* regPoolName, cxuint requiredRegsNum);
    
    /* return true if no error */
    static bool parseVRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                   cxuint regsNum, bool required = true);
    /* return true if no error */
    static bool parseSRegRange(Assembler& asmr, const char*& linePtr, RegRange& regPair,
                   uint16_t arch, cxuint regsNum, bool required = true);
    /* return true if no error */
    template<typename T>
    static bool parseImm(Assembler& asmr, const char*& linePtr, T& value,
            std::unique_ptr<AsmExpression>& outTargetExpr);
    
    static bool parseLiteralImm(Assembler& asmr, const char*& linePtr, uint32_t& value,
            std::unique_ptr<AsmExpression>& outTargetExpr, Flags instropMask = 0);
    
    static bool parseVOP3Modifiers(Assembler& asmr, const char*& linePtr, cxbyte& mods,
                           bool withClamp = true);
    
    static bool parseOperand(Assembler& asmr, const char*& linePtr, GCNOperand& operand,
               std::unique_ptr<AsmExpression>& outTargetExpr, uint16_t arch,
               cxuint regsNum, Flags instrOpMask);
        
    static void parseSOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOPKEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseSOPPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    
    static void parseSMRDEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    
    static void parseVOP2Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVOP1Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVOPCEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVOP3Encoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseVINTRPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseDSEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseMUBUFEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* instrPlace, const char* linePtr, uint16_t arch,
                      std::vector<cxbyte>& output, GCNAssembler::Regs& gcnRegs);
    static void parseMIMGEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseEXPEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
    static void parseFLATEncoding(Assembler& asmr, const GCNAsmInstruction& gcnInsn,
                      const char* linePtr, uint16_t arch, std::vector<cxbyte>& output,
                      GCNAssembler::Regs& gcnRegs);
};

struct CLRX_INTERNAL AsmPseudoOps: AsmParseUtils
{
    /*
     * pseudo-ops logic
     */
    // set bitnesss
    static void setBitness(Assembler& asmr, const char* linePtr, bool _64Bit);
    // set output format
    static void setOutFormat(Assembler& asmr, const char* linePtr);
    // set GPU architecture type
    static void setGPUDevice(Assembler& asmr, const char* linePtr);
    // set GPU architecture
    static void setGPUArchitecture(Assembler& asmr, const char* linePtr);
    
    // change kernel
    static void goToKernel(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);
    // change section
    static void goToSection(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);
    
    static void goToMain(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);
    
    /// include file
    static void includeFile(Assembler& asmr, const char* pseudoOpPlace,
                            const char* linePtr);
    // include binary file
    static void includeBinFile(Assembler& asmr, const char* pseudoOpPlace,
                       const char* linePtr);
    
    // fail
    static void doFail(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    // .error
    static void doError(Assembler& asmr, const char* pseudoOpPlace,
                           const char* linePtr);
    // .warning
    static void doWarning(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr);
    
    // .byte, .short, .int, .word, .long, .quad
    template<typename T>
    static void putIntegers(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr);
    
    // .half, .float, .double
    template<typename UIntType>
    static void putFloats(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    
    /// .string, ascii
    static void putStrings(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr, bool addZero = false);
    // .string16, .string32, .string64
    template<typename T>
    static void putStringsToInts(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);
    
    /// .octa
    static void putUInt128s(Assembler& asmr, const char* pseudoOpPlace,
                        const char* linePtr);
    
    /// .set, .equ, .eqv, .equiv
    static void setSymbol(Assembler& asmr, const char* linePtr, bool reassign = true,
                bool baseExpr = false);
    
    // .global, .local, .extern
    static void setSymbolBind(Assembler& asmr, const char* linePtr, cxbyte elfInfo);
    
    static void setSymbolSize(Assembler& asmr, const char* linePtr);
    
    static void ignoreExtern(Assembler& asmr, const char* linePtr);
    
    static void doFill(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
               bool _64bit = false);
    static void doSkip(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    
    /* TODO: add no-op fillin for text sections */
    static void doAlign(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                    bool powerOf2 = false);
    
    /* TODO: add no-op fillin for text sections */
    template<typename Word>
    static void doAlignWord(Assembler& asmr, const char* pseudoOpPlace,
                            const char* linePtr);
    
    static void doOrganize(Assembler& asmr, const char* linePtr);
    
    static void doPrint(Assembler& asmr, const char* linePtr);
    
    static void doIfInt(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                IfIntComp compType, bool elseIfClause);
    
    static void doIfDef(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    
    static void doIfBlank(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    /// .ifc
    static void doIfCmpStr(Assembler& asmr, const char* pseudoOpPlace,
               const char* linePtr, bool negation, bool elseIfClause);
    /// ifeqs, ifnes
    static void doIfStrEqual(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr, bool negation, bool elseIfClause);
    // else
    static void doElse(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    // endif
    static void endIf(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    /// start repetition content
    static void doRepeat(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    /// end repetition content
    static void endRepeat(Assembler& asmr, const char* pseudoOpPlace,
                          const char* linePtr);
    /// start macro definition
    static void doMacro(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    /// ends macro definition
    static void endMacro(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    // immediately exit from macro
    static void exitMacro(Assembler& asmr, const char* pseudoOpPlace,
                      const char* linePtr);
    // purge macro
    static void purgeMacro(Assembler& asmr, const char* linePtr);
    // do IRP
    static void doIRP(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                      bool perChar = false);
    
    static void undefSymbol(Assembler& asmr, const char* linePtr);
    
    static void setAbsoluteOffset(Assembler& asmr, const char* linePtr);
    
    static void ignoreString(Assembler& asmr, const char* linePtr);
    
    static bool checkPseudoOpName(const CString& string);
};

class AsmGalliumHandler;

struct CLRX_INTERNAL AsmGalliumPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    static void doGlobalData(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // open argument list
    static void doArgs(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // add argument info
    static void doArg(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /// open progInfo
    static void doProgInfo(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /// add progInfo entry
    static void doEntry(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
};

enum AmdConfigValueTarget
{
    AMDCVAL_SGPRSNUM,
    AMDCVAL_VGPRSNUM,
    AMDCVAL_PGMRSRC2,
    AMDCVAL_IEEEMODE,
    AMDCVAL_FLOATMODE,
    AMDCVAL_HWLOCAL,
    AMDCVAL_HWREGION,
    AMDCVAL_PRIVATEID,
    AMDCVAL_SCRATCHBUFFER,
    AMDCVAL_UAVPRIVATE,
    AMDCVAL_UAVID,
    AMDCVAL_CBID,
    AMDCVAL_PRINTFID,
    AMDCVAL_EARLYEXIT,
    AMDCVAL_CONDOUT,
    AMDCVAL_USECONSTDATA,
    AMDCVAL_USEPRINTF
};

struct CLRX_INTERNAL AsmAmdPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    static void doGlobalData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setCompileOptions(AsmAmdHandler& handler, const char* linePtr);
    static void setDriverInfo(AsmAmdHandler& handler, const char* linePtr);
    static void setDriverVersion(AsmAmdHandler& handler, const char* linePtr);
    
    static void addCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, uint32_t calNoteId);
    static void addCustomCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void addMetadata(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void addHeader(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doConfig(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /// add any entry with two 32-bit integers
    static void doEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
              const char* linePtr, uint32_t requiredCalNoteIdMask, const char* entryName);
    /// add any entry with four 32-bit integers
    static void doUavEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /* dual pseudo-ops (for configured kernels and non-config kernels) */
    static void doCBId(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doCondOut(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doEarlyExit(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doSampler(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    /* user configuration pseudo-ops */
    static void doArg(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setConfigValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target);
    
    static void setCWS(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setConfigBoolValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target);
    
    static void addUserData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
};

extern const cxbyte tokenCharTable[96] CLRX_INTERNAL;

};

#endif
