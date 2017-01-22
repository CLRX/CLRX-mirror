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

#ifndef __CLRX_ASMINTERNALS_H__
#define __CLRX_ASMINTERNALS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <utility>
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
    
    /* IMPORTANT NOTE: table must be sorted by name!!! 
     * the longest name in table must shorter than 72 */
    static bool getEnumeration(Assembler& asmr, const char*& linePtr, const char* objName,
          size_t tableSize, const std::pair<const char*, cxuint>* table, cxuint& value,
          const char* prefix = nullptr);
    
    // skip comma
    static bool skipComma(Assembler& asmr, bool& haveComma, const char*& linePtr);
    // skip required comma, (returns false if not found comma)
    static bool skipRequiredComma(Assembler& asmr, const char*& linePtr);
    
    // skip comma for multiple argument pseudo-ops
    static bool skipCommaForMultipleArgs(Assembler& asmr, const char*& linePtr);
    
    static bool parseDimensions(Assembler& asmr, const char*& linePtr, cxuint& dimMask);
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
    
    static void ignoreExtern(Assembler& asmr, const char* linePtr);
    
    static void doFill(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
               bool _64bit = false);
    static void doSkip(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr);
    
    static void doAlign(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                    bool powerOf2 = false);
    
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
    
    static void doIf64Bit(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    static void doIfArch(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
    static void doIfGpu(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                bool negation, bool elseIfClause);
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
    
    static void undefSymbol(Assembler& asmr, const char* linePtr);
    
    static void doDefRegVar(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr);
    
    static void setAbsoluteOffset(Assembler& asmr, const char* linePtr);
    
    static void ignoreString(Assembler& asmr, const char* linePtr);
    
    static bool checkPseudoOpName(const CString& string);
};

class AsmGalliumHandler;

enum GalliumConfigValueTarget
{
    GALLIUMCVAL_SGPRSNUM,
    GALLIUMCVAL_VGPRSNUM,
    GALLIUMCVAL_PGMRSRC1,
    GALLIUMCVAL_PGMRSRC2,
    GALLIUMCVAL_IEEEMODE,
    GALLIUMCVAL_FLOATMODE,
    GALLIUMCVAL_LOCALSIZE,
    GALLIUMCVAL_SCRATCHBUFFER,
    GALLIUMCVAL_PRIORITY,
    GALLIUMCVAL_USERDATANUM,
    GALLIUMCVAL_TGSIZE,
    GALLIUMCVAL_DEBUGMODE,
    GALLIUMCVAL_DX10CLAMP,
    GALLIUMCVAL_PRIVMODE,
    GALLIUMCVAL_EXCEPTIONS
};

struct CLRX_INTERNAL AsmGalliumPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    /* user configuration pseudo-ops */
    static void doConfig(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setConfigValue(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, GalliumConfigValueTarget target);
    
    static void setConfigBoolValue(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, GalliumConfigValueTarget target);
    
    static void setDimensions(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
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
    
    static void doKCode(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doKCodeEnd(AsmGalliumHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void updateKCodeSel(AsmGalliumHandler& handler,
          const std::vector<cxuint>& oldset);
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
    AMDCVAL_USEPRINTF,
    AMDCVAL_TGSIZE,
    AMDCVAL_EXCEPTIONS
};

struct CLRX_INTERNAL AsmAmdPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    static void doGlobalData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setCompileOptions(AsmAmdHandler& handler, const char* linePtr);
    static void setDriverInfo(AsmAmdHandler& handler, const char* linePtr);
    static void setDriverVersion(AsmAmdHandler& handler, const char* linePtr);
    
    static void getDriverVersion(AsmAmdHandler& handler, const char* linePtr);
    
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
    static void setDimensions(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static bool parseArg(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
         const std::unordered_set<CString>& argNamesSet, AmdKernelArgInput& arg, bool cl20);
    static void doArg(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setConfigValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target);
    
    static bool parseCWS(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
                     uint64_t* out);
    static void setCWS(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void setConfigBoolValue(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, AmdConfigValueTarget target);
    
    static void addUserData(AsmAmdHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
};

enum AmdCL2ConfigValueTarget
{
    AMDCL2CVAL_SGPRSNUM,
    AMDCL2CVAL_VGPRSNUM,
    AMDCL2CVAL_PGMRSRC1,
    AMDCL2CVAL_PGMRSRC2,
    AMDCL2CVAL_IEEEMODE,
    AMDCL2CVAL_FLOATMODE,
    AMDCL2CVAL_LOCALSIZE,
    AMDCL2CVAL_GDSSIZE,
    AMDCL2CVAL_SCRATCHBUFFER,
    AMDCL2CVAL_PRIORITY,
    AMDCL2CVAL_TGSIZE,
    AMDCL2CVAL_DEBUGMODE,
    AMDCL2CVAL_DX10CLAMP,
    AMDCL2CVAL_PRIVMODE,
    AMDCL2CVAL_EXCEPTIONS,
    AMDCL2CVAL_USESETUP,
    AMDCL2CVAL_USEARGS,
    AMDCL2CVAL_USEENQUEUE,
    AMDCL2CVAL_USEGENERIC
};

struct CLRX_INTERNAL AsmAmdCL2PseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    static void setArchMinor(AsmAmdCL2Handler& handler, const char* linePtr);
    static void setArchStepping(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void setAclVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    static void setCompileOptions(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void setDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void getDriverVersion(AsmAmdCL2Handler& handler, const char* linePtr);
    
    static void doInner(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doGlobalData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doRwData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doBssData(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doSamplerInit(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doSamplerReloc(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doSampler(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setConfigValue(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
           const char* linePtr, AmdCL2ConfigValueTarget target);
    static void setConfigBoolValue(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
           const char* linePtr, AmdCL2ConfigValueTarget target);
    
    static void setDimensions(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setCWS(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doArg(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doSetupArgs(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                       const char* linePtr);
    
    static void addMetadata(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void addISAMetadata(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void addKernelSetup(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void addKernelStub(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doConfig(AsmAmdCL2Handler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
};

enum ROCmConfigValueTarget
{
    ROCMCVAL_SGPRSNUM,
    ROCMCVAL_VGPRSNUM,
    ROCMCVAL_PRIVMODE,
    ROCMCVAL_DEBUGMODE,
    ROCMCVAL_DX10CLAMP,
    ROCMCVAL_IEEEMODE,
    ROCMCVAL_TGSIZE,
    ROCMCVAL_FLOATMODE,
    ROCMCVAL_PRIORITY,
    ROCMCVAL_EXCEPTIONS,
    ROCMCVAL_USERDATANUM,
    ROCMCVAL_PGMRSRC1,
    ROCMCVAL_PGMRSRC2,
    ROCMCVAL_KERNEL_CODE_ENTRY_OFFSET,
    ROCMCVAL_KERNEL_CODE_PREFETCH_OFFSET,
    ROCMCVAL_KERNEL_CODE_PREFETCH_SIZE,
    ROCMCVAL_MAX_SCRATCH_BACKING_MEMORY,
    ROCMCVAL_USE_PRIVATE_SEGMENT_BUFFER,
    ROCMCVAL_USE_DISPATCH_PTR,
    ROCMCVAL_USE_QUEUE_PTR,
    ROCMCVAL_USE_KERNARG_SEGMENT_PTR,
    ROCMCVAL_USE_DISPATCH_ID,
    ROCMCVAL_USE_FLAT_SCRATCH_INIT,
    ROCMCVAL_USE_PRIVATE_SEGMENT_SIZE,
    ROCMCVAL_USE_ORDERED_APPEND_GDS,
    ROCMCVAL_PRIVATE_ELEM_SIZE,
    ROCMCVAL_USE_PTR64,
    ROCMCVAL_USE_DYNAMIC_CALL_STACK,
    ROCMCVAL_USE_DEBUG_ENABLED,
    ROCMCVAL_USE_XNACK_ENABLED,
    ROCMCVAL_WORKITEM_PRIVATE_SEGMENT_SIZE,
    ROCMCVAL_WORKGROUP_GROUP_SEGMENT_SIZE,
    ROCMCVAL_GDS_SEGMENT_SIZE,
    ROCMCVAL_KERNARG_SEGMENT_SIZE,
    ROCMCVAL_WORKGROUP_FBARRIER_COUNT,
    ROCMCVAL_WAVEFRONT_SGPR_COUNT,
    ROCMCVAL_WORKITEM_VGPR_COUNT,
    ROCMCVAL_DEBUG_WAVEFRONT_PRIVATE_SEGMENT_OFFSET_SGPR,
    ROCMCVAL_DEBUG_PRIVATE_SEGMENT_BUFFER_SGPR,
    ROCMCVAL_KERNARG_SEGMENT_ALIGN,
    ROCMCVAL_GROUP_SEGMENT_ALIGN,
    ROCMCVAL_PRIVATE_SEGMENT_ALIGN,
    ROCMCVAL_WAVEFRONT_SIZE,
    ROCMCVAL_CALL_CONVENTION,
    ROCMCVAL_RUNTIME_LOADER_KERNEL_SYMBOL
};

struct CLRX_INTERNAL AsmROCmPseudoOps: AsmPseudoOps
{
    static bool checkPseudoOpName(const CString& string);
    
    static void setArchMinor(AsmROCmHandler& handler, const char* linePtr);
    static void setArchStepping(AsmROCmHandler& handler, const char* linePtr);
    
    /* user configuration pseudo-ops */
    static void doConfig(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doControlDirective(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void doFKernel(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setConfigValue(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, ROCmConfigValueTarget target);
    
    static void setConfigBoolValue(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, ROCmConfigValueTarget target);
    
    static void setDimensions(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setMachine(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setCodeVersion(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    
    static void setReservedXgprs(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr, bool inVgpr);
    
    static void setUseGridWorkGroupCount(AsmROCmHandler& handler,
                      const char* pseudoOpPlace, const char* linePtr);
    
    static void doKCode(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void doKCodeEnd(AsmROCmHandler& handler, const char* pseudoOpPlace,
                      const char* linePtr);
    static void updateKCodeSel(AsmROCmHandler& handler,
                      const std::vector<cxuint>& oldset);
};

extern CLRX_INTERNAL cxbyte cstrtobyte(const char*& str, const char* end);

extern const cxbyte tokenCharTable[96] CLRX_INTERNAL;

};

#endif
