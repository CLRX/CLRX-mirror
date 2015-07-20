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
    // skip required comma, (returns false if not found comma)
    static bool skipRequiredComma(Assembler& asmr, const char*& linePtr,
                      const char* nameArg);
    
    // skip comma for multiple argument pseudo-ops
    static bool skipCommaForMultipleArgs(Assembler& asmr, const char*& linePtr);
    
    /*
     * pseudo-ops logic
     */
    // set bitnesss
    static void setBitness(Assembler& asmr, const char* linePtr, bool _64Bit);
    // set output format
    static void setOutFormat(Assembler& asmr, const char* linePtr);
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
    
    static bool checkPseudoOpName(const std::string& string);
};

class AsmGalliumHandler;

struct CLRX_INTERNAL AsmGalliumPseudoOps: AsmPseudoOps
{
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

struct CLRX_INTERNAL AsmAmdPseudoOps: AsmPseudoOps
{
    static void setCompileOptions(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setDriverInfo(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setDriverVersion(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setSource(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setLLVMIR(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doKernelData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void addCALNote(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, uint32_t calNoteId);
    
    static void addMetadata(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void addHeader(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void addKernelData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /// add any entry with two 32-bit integers
    static void doEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    /// add any entry with two 32-bit integers
    static void doUavEntry(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    /* user condfiguration pseudo-ops */
    static void doArg(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doSampler(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setCWS(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setSPGRsNum(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setVPGRsNum(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setPgmRsrc2(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setIeeeMode(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setFloatMode(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setHwLocal(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setHwRegion(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setScratchBuffer(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setUavPrivate(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setPrivateId(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setUavId(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setCbId(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setPrintfId(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setEarlyExit(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setUseConstData(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setCondOut(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setUsePrintf(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setUserData(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
};

extern const cxbyte tokenCharTable[96] CLRX_INTERNAL;

};

#endif
