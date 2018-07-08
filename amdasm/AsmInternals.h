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

#ifndef __CLRX_ASMINTERNALS_H__
#define __CLRX_ASMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
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
CString extractSymName(const char*& string, const char* end, bool localLabelSymName);

CString extractScopedSymName(const char*& string, const char* end,
           bool localLabelSymName = false);

// extract label name from string (must be at start)
// (but not symbol of backward of forward labels)
static inline CString extractLabelName(const char*& string, const char* end)
{
    if (string != end && isDigit(*string))
    {
        const char* startString = string;
        while (string != end && isDigit(*string)) string++;
        return CString(startString, string);
    }
    return extractScopedSymName(string, end, false);
}

void skipSpacesAndLabels(const char*& linePtr, const char* end);

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
    
    static bool getAnyValueArg(Assembler& asmr, uint64_t& value, AsmSectionId& sectionId,
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
    
    /* IMPORTANT NOTE: table must be sorted by name!!! 
     * the longest name in table must shorter than 72 */
    static bool getEnumeration(Assembler& asmr, const char*& linePtr, const char* objName,
          size_t tableSize, const std::pair<const char*, cxuint>* table, cxuint& value,
          const char* prefix = nullptr);
    
    // skip comma
    // errorWhenNoEnd - print error if not end and not comma
    static bool skipComma(Assembler& asmr, bool& haveComma, const char*& linePtr,
                    bool errorWhenNoEnd = true);
    // skip required comma, (returns false if not found comma)
    static bool skipRequiredComma(Assembler& asmr, const char*& linePtr);
    
    // skip comma for multiple argument pseudo-ops
    static bool skipCommaForMultipleArgs(Assembler& asmr, const char*& linePtr);
    
    static bool parseSingleDimensions(Assembler& asmr, const char*& linePtr,
                    cxuint& dimMask);
    static bool parseDimensions(Assembler& asmr, const char*& linePtr, cxuint& dimMask,
                    bool twoFields = false);
    
    static void setSymbolValue(Assembler& asmr, const char* linePtr,
                    uint64_t value, AsmSectionId sectionId);
};

enum class AsmPredefined: cxbyte
{
    ARCH,
    BIT64,
    FORMAT,
    GPU,
    VERSION,
    POLICY
};

struct CLRX_INTERNAL AsmPseudoOps: AsmParseUtils
{
    /*
     * pseudo-ops logic
     */
    // set bitnesss
    static void setBitness(Assembler& asmr, const char* linePtr, bool _64Bit);
    
    static bool parseFormat(Assembler& asmr, const char*& linePtr, BinaryFormat& format);
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
                   const char* linePtr, bool isPseudoOp = false);
    
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
    
    // ignore .extern
    static void ignoreExtern(Assembler& asmr, const char* linePtr);
    
    // .fill
    static void doFill(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
               bool _64bit = false);
    // .skip
    static void doSkip(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    
    // .align
    static void doAlign(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                    bool powerOf2 = false);
    
    template<typename Word>
    static void doAlignWord(Assembler& asmr, const char* pseudoOpPlace,
                            const char* linePtr);
    // .org
    static void doOrganize(Assembler& asmr, const char* linePtr);
    
    // .print
    static void doPrint(Assembler& asmr, const char* linePtr);
    
    // .ifXX for integers
    static void doIfInt(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                IfIntComp compType, bool elseIfClause);
    // .ifdef
    static void doIfDef(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    
    // .ifb
    static void doIfBlank(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    
    // .if64 and .if32
    static void doIf64Bit(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    // .ifarch
    static void doIfArch(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    // .ifgpu
    static void doIfGpu(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    // .iffmt
    static void doIfFmt(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
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
    // do 'for'
    static void doFor(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    // do 'while'
    static void doWhile(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    // do open scope (.scope)
    static void openScope(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    // do close scope (.ends or .endscope)
    static void closeScope(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    // start using scope (.using)
    static void startUsing(Assembler& asmr, const char* linePtr);
    // stop using scope (.unusing)
    static void stopUsing(Assembler& asmr, const char* linePtr);
    // .usereg ?
    static void doUseReg(Assembler& asmr, const char* linePtr);
    // .rvlin
    static void declareRegVarLinearDeps(Assembler& asmr, const char* linePtr,
                bool usedOnce);
    // .undef
    static void undefSymbol(Assembler& asmr, const char* linePtr);
    // .regvar
    static void defRegVar(Assembler& asmr, const char* linePtr);
    // .cf_ (code flow pseudo-ops)
    static void addCodeFlowEntries(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr, AsmCodeFlowType type);
    
    static void setAbsoluteOffset(Assembler& asmr, const char* linePtr);
    
    static void getPredefinedValue(Assembler& asmr, const char* linePtr,
                        AsmPredefined predefined);
    
    // enumerate
    static void doEnum(Assembler& asmr, const char* linePtr);
    // set policy version
    static void setPolicyVersion(Assembler& asmr, const char* linePtr);
    
    static void ignoreString(Assembler& asmr, const char* linePtr);
    
    static bool checkPseudoOpName(const CString& string);
};

struct CLRX_INTERNAL AsmKcodePseudoOps : AsmParseUtils
{
    // .kcode (open kernel code)
    static void doKCode(AsmKcodeHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    // .kcodeend (close kernel code)
    static void doKCodeEnd(AsmKcodeHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void updateKCodeSel(AsmKcodeHandler& handler,
                      const std::vector<cxuint>& oldset);
};

// macro helper to handle printing error

#define PSEUDOOP_RETURN_BY_ERROR(STRING) \
    { \
        asmr.printError(pseudoOpPlace, STRING); \
        return; \
    }

#define ASM_RETURN_BY_ERROR(PLACE, STRING) \
    { \
        asmr.printError(PLACE, STRING); \
        return; \
    }

#define ASM_FAIL_BY_ERROR(PLACE, STRING) \
    { \
        asmr.printError(PLACE, STRING); \
        return false; \
    }

#define ASM_NOTGOOD_BY_ERROR(PLACE, STRING) \
    { \
        asmr.printError(PLACE, STRING); \
        good = false; \
    }

#define ASM_NOTGOOD_BY_ERROR1(GOOD, PLACE, STRING) \
    { \
        asmr.printError(PLACE, STRING); \
        GOOD = false; \
    }

extern CLRX_INTERNAL cxbyte cstrtobyte(const char*& str, const char* end);

extern const cxbyte tokenCharTable[96] CLRX_INTERNAL;

};

#endif
