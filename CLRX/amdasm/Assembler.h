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
/*! \file Assembler.h
 * \brief an assembler for Radeon GPU's
 */

#ifndef __CLRX_ASSEMBLER_H__
#define __CLRX_ASSEMBLER_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
#include <iostream>
#include <vector>
#include <utility>
#include <stack>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Commons.h>
#include <CLRX/amdasm/AsmSource.h>
#include <CLRX/amdasm/AsmFormats.h>
#include <CLRX/amdasm/AsmDefs.h>

/// main namespace
namespace CLRX
{

enum: Flags
{
    ASM_WARNINGS = 1,   ///< enable all warnings for assembler
    ASM_FORCE_ADD_SYMBOLS = 2,  ///< force add symbols to binary
    ASM_ALTMACRO = 4,   ///< enable altmacro mode
    ASM_BUGGYFPLIT = 8, ///< buggy handling of fpliterals (including fp constants)
    ASM_MACRONOCASE = 16, /// disable case-insensitive naming (default)
    ASM_OLDMODPARAM = 32,   ///< use old modifier parametrization (values 0 and 1 only)
    ASM_TESTRESOLVE = (1U<<30), ///< enable resolving symbols if ASM_TESTRUN enabled
    ASM_TESTRUN = (1U<<31), ///< only for running tests
    ASM_ALL = FLAGS_ALL&~(ASM_TESTRUN|ASM_TESTRESOLVE|ASM_BUGGYFPLIT|ASM_MACRONOCASE|
                    ASM_OLDMODPARAM)  ///< all flags
};

struct AsmRegVar;

/// ISA (register and regvar) Usage handler
class ISAUsageHandler
{
public:
    /// stgructure that hold read position to store later
    struct ReadPos
    {
        size_t readOffset;  ///< read offset
        size_t instrStructPos;  ///< position instrStructs
        size_t regUsagesPos;    ///< position in reg usage
        size_t regUsages2Pos;   ///< position in regUsage2
        size_t regVarUsagesPos;    ///< position in regVarUsage
        uint16_t pushedArgs;    ///< pushed argds number
        bool useRegMode;        ///< true if in usereg mode
    };
protected:
    std::vector<cxbyte> instrStruct;    ///< structure of register usage
    std::vector<AsmRegUsageInt> regUsages;  ///< register usage
    std::vector<AsmRegUsage2Int> regUsages2;  ///< register usage (by .usereg)
    std::vector<AsmRegVarUsageInt> regVarUsages;    ///< regvar usage
    const std::vector<cxbyte>& content; ///< code content
    size_t lastOffset;  ///< last offset
    size_t readOffset;  ///< read offset
    size_t instrStructPos;  ///< position in instr struct
    size_t regUsagesPos;    ///< position in reg usage
    size_t regUsages2Pos;   ///< position in reg usage 2
    size_t regVarUsagesPos; ///< position in regvar usage
    uint16_t pushedArgs;    ///< pushed args
    cxbyte argPos;      ///< argument position
    cxbyte argFlags;    ///< ???
    cxbyte defaultInstrSize;    ///< default instruction size
    bool isNext;        ///< is next
    bool useRegMode;    ///< true if in usereg mode
    
    void skipBytesInInstrStruct();
    /// put space to offset
    void putSpace(size_t offset);
    
    /// constructor
    explicit ISAUsageHandler(const std::vector<cxbyte>& content);
public:
    /// destructor
    virtual ~ISAUsageHandler();
    /// copy this usage handler
    virtual ISAUsageHandler* copy() const = 0;
    
    /// push regvar or register usage
    void pushUsage(const AsmRegVarUsage& rvu);
    /// rewind to start for reading
    void rewind();
    /// flush last pending register usages
    void flush();
    /// has next regvar usage
    bool hasNext() const
    { return isNext; }
    /// get next usage
    AsmRegVarUsage nextUsage();
    
    /// get reading position
    ReadPos getReadPos() const
    {
        return { readOffset, instrStructPos, regUsagesPos, regUsages2Pos,
            regVarUsagesPos, pushedArgs, useRegMode };
    }
    /// set reading position
    void setReadPos(const ReadPos rpos)
    {
        readOffset = rpos.readOffset;
        instrStructPos = rpos.instrStructPos;
        regUsagesPos = rpos.regUsagesPos;
        regUsages2Pos = rpos.regUsages2Pos;
        regVarUsagesPos = rpos.regVarUsagesPos;
        pushedArgs = rpos.pushedArgs;
        useRegMode = rpos.useRegMode;
    }
    
    /// push regvar or register from usereg pseudo-op
    void pushUseRegUsage(const AsmRegVarUsage& rvu);
    
    /// get RW flags (used by assembler)
    virtual cxbyte getRwFlags(AsmRegField regField, uint16_t rstart,
                      uint16_t rend) const = 0;
    /// get reg pair (used by assembler)
    virtual std::pair<uint16_t,uint16_t> getRegPair(AsmRegField regField,
                    cxbyte rwFlags) const = 0;
    /// get usage dependencies around single instruction
    virtual void getUsageDependencies(cxuint rvusNum, const AsmRegVarUsage* rvus,
                    cxbyte* linearDeps, cxbyte* equalToDeps) const = 0;
};

/// GCN (register and regvar) Usage handler
class GCNUsageHandler: public ISAUsageHandler
{
private:
    uint16_t archMask;
public:
    /// constructor
    GCNUsageHandler(const std::vector<cxbyte>& content, uint16_t archMask);
    /// destructor
    ~GCNUsageHandler();
    
    /// copy this usage handler
    ISAUsageHandler* copy() const;
    
    cxbyte getRwFlags(AsmRegField regFied, uint16_t rstart, uint16_t rend) const;
    std::pair<uint16_t,uint16_t> getRegPair(AsmRegField regField, cxbyte rwFlags) const;
    void getUsageDependencies(cxuint rvusNum, const AsmRegVarUsage* rvus,
                    cxbyte* linearDeps, cxbyte* equalToDeps) const;
};

/// ISA assembler class
class ISAAssembler: public NonCopyableAndNonMovable
{
protected:
    Assembler& assembler;       ///< assembler
    
    /// print warning for position pointed by line pointer
    void printWarning(const char* linePtr, const char* message);
    /// print error for position pointed by line pointer
    void printError(const char* linePtr, const char* message);
    /// print warning for source position
    void printWarning(const AsmSourcePos& sourcePos, const char* message);
    /// print error for source position
    void printError(const AsmSourcePos& sourcePos, const char* message);
    /// print warning about integer out of range
    void printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos,
                cxbyte signess = WS_BOTH);
    void addCodeFlowEntry(cxuint sectionId, const AsmCodeFlowEntry& entry);
    /// constructor
    explicit ISAAssembler(Assembler& assembler);
public:
    /// destructor
    virtual ~ISAAssembler();
    /// create usage handler
    virtual ISAUsageHandler* createUsageHandler(std::vector<cxbyte>& content) const = 0;
    
    /// assemble single line
    virtual void assemble(const CString& mnemonic, const char* mnemPlace,
              const char* linePtr, const char* lineEnd, std::vector<cxbyte>& output,
              ISAUsageHandler* usageHandler) = 0;
    /// resolve code with location, target and value
    virtual bool resolveCode(const AsmSourcePos& sourcePos, cxuint targetSectionId,
                 cxbyte* sectionData, size_t offset, AsmExprTargetType targetType,
                 cxuint sectionId, uint64_t value) = 0;
    /// check if name is mnemonic
    virtual bool checkMnemonic(const CString& mnemonic) const = 0;
    /// set allocated registers (if regs is null then reset them)
    virtual void setAllocatedRegisters(const cxuint* regs = nullptr,
                Flags regFlags = 0) = 0;
    /// get allocated register numbers after assemblying
    virtual const cxuint* getAllocatedRegisters(size_t& regTypesNum,
                Flags& regFlags) const = 0;
    /// get max registers number
    virtual void getMaxRegistersNum(size_t& regTypesNum, cxuint* maxRegs) const = 0;
    /// get registers ranges
    virtual void getRegisterRanges(size_t& regTypesNum, cxuint* regRanges) const = 0;
    /// fill alignment when value is not given
    virtual void fillAlignment(size_t size, cxbyte* output) = 0;
    /// parse register range
    virtual bool parseRegisterRange(const char*& linePtr, cxuint& regStart,
                        cxuint& regEnd, const AsmRegVar*& regVar) = 0;
    /// return true if expresion of target fit to value with specified bits
    virtual bool relocationIsFit(cxuint bits, AsmExprTargetType tgtType) = 0;
    /// parse register type for '.reg' pseudo-op
    virtual bool parseRegisterType(const char*& linePtr,
                       const char* end, cxuint& type) = 0;
    /// get size of instruction
    virtual size_t getInstructionSize(size_t codeSize, const cxbyte* code) const = 0;
};

/// GCN arch assembler
class GCNAssembler: public ISAAssembler
{
public:
    /// register pool numbers
    struct Regs {
        cxuint sgprsNum;    ///< SGPRs number
        cxuint vgprsNum;    ///< VGPRs number
        Flags regFlags; ///< define what extra register must be included
    };
private:
    friend struct GCNAsmUtils; // INTERNAL LOGIC
    union {
        Regs regs;
        cxuint regTable[2];
    };
    uint16_t curArchMask;
    cxbyte currentRVUIndex;
    AsmRegVarUsage instrRVUs[6];
    
    void resetInstrRVUs()
    {
        for (AsmRegVarUsage& rvu: instrRVUs)
            rvu.regField = ASMFIELD_NONE;
    }
    void setCurrentRVU(cxbyte idx)
    { currentRVUIndex = idx; }
    
    void setRegVarUsage(const AsmRegVarUsage& rvu)
    { instrRVUs[currentRVUIndex] = rvu; }
    
    void flushInstrRVUs(ISAUsageHandler* usageHandler)
    {
        for (const AsmRegVarUsage& rvu: instrRVUs)
            if (rvu.regField != ASMFIELD_NONE)
                usageHandler->pushUsage(rvu);
    }
public:
    /// constructor
    explicit GCNAssembler(Assembler& assembler);
    /// destructor
    ~GCNAssembler();
    
    ISAUsageHandler* createUsageHandler(std::vector<cxbyte>& content) const;
    
    void assemble(const CString& mnemonic, const char* mnemPlace, const char* linePtr,
                  const char* lineEnd, std::vector<cxbyte>& output,
                  ISAUsageHandler* usageHandler);
    bool resolveCode(const AsmSourcePos& sourcePos, cxuint targetSectionId,
                 cxbyte* sectionData, size_t offset, AsmExprTargetType targetType,
                 cxuint sectionId, uint64_t value);
    bool checkMnemonic(const CString& mnemonic) const;
    void setAllocatedRegisters(const cxuint* regs, Flags regFlags);
    const cxuint* getAllocatedRegisters(size_t& regTypesNum, Flags& regFlags) const;
    void getMaxRegistersNum(size_t& regTypesNum, cxuint* maxRegs) const;
    void getRegisterRanges(size_t& regTypesNum, cxuint* regRanges) const;
    void fillAlignment(size_t size, cxbyte* output);
    bool parseRegisterRange(const char*& linePtr, cxuint& regStart, cxuint& regEnd,
                const AsmRegVar*& regVar);
    bool relocationIsFit(cxuint bits, AsmExprTargetType tgtType);
    bool parseRegisterType(const char*& linePtr, const char* end, cxuint& type);
    size_t getInstructionSize(size_t codeSize, const cxbyte* code) const;
};

class AsmRegAllocator
{
public:
    struct NextBlock
    {
        size_t block;
        bool isCall;
    };
    struct SSAInfo
    {
        size_t ssaIdBefore; ///< SSA id before first SSA in block
        size_t ssaIdFirst; // SSA id at first change
        size_t ssaId;   ///< original SSA id
        size_t ssaIdLast; ///< last SSA id in last
        size_t ssaIdChange; ///< number of SSA id changes
        size_t firstPos;
        size_t lastPos;
        bool readBeforeWrite;   ///< have read before write
        SSAInfo(size_t _bssaId = SIZE_MAX, size_t _ssaIdF = SIZE_MAX,
                size_t _ssaId = SIZE_MAX, size_t _ssaIdL = SIZE_MAX,
                size_t _ssaIdChange = 0, bool _readBeforeWrite = false)
            : ssaIdBefore(_bssaId), ssaIdFirst(_ssaIdF), ssaId(_ssaId),
              ssaIdLast(_ssaIdL), ssaIdChange(_ssaIdChange),
              readBeforeWrite(_readBeforeWrite)
        { }
    };
    struct CodeBlock
    {
        size_t start, end; // place in code
        std::vector<NextBlock> nexts; ///< nexts blocks, if empty then direct next block
        bool haveCalls;
        bool haveReturn;
        bool haveEnd;
        // key - regvar, value - SSA info for this regvar
        std::unordered_map<AsmSingleVReg, SSAInfo> ssaInfoMap;
        ISAUsageHandler::ReadPos usagePos;
    };
    
     // first - orig ssaid, second - dest ssaid
    typedef std::pair<size_t, size_t> SSAReplace;
    typedef std::unordered_map<AsmSingleVReg, VectorSet<SSAReplace> > SSAReplacesMap;
    // interference graph type
    typedef Array<std::unordered_set<size_t> > InterGraph;
    typedef std::unordered_map<AsmSingleVReg, std::vector<size_t> > VarIndexMap;
    struct LinearDep
    {
        cxbyte align;
        std::vector<size_t> prevVidxes;
        std::vector<size_t> nextVidxes;
    };
    struct EqualToDep
    {
        std::vector<size_t> prevVidxes;
        std::vector<size_t> nextVidxes;
    };
private:
    Assembler& assembler;
    std::vector<CodeBlock> codeBlocks;
    SSAReplacesMap ssaReplacesMap;
    size_t regTypesNum;
    
    VarIndexMap vregIndexMaps[MAX_REGTYPES_NUM]; // indices to igraph for 2 reg types
    InterGraph interGraphs[MAX_REGTYPES_NUM]; // for 2 register 
    Array<cxuint> graphColorMaps[MAX_REGTYPES_NUM];
    std::unordered_map<size_t, LinearDep> linearDepMaps[MAX_REGTYPES_NUM];
    std::unordered_map<size_t, EqualToDep> equalToDepMaps[MAX_REGTYPES_NUM];
    std::unordered_map<size_t, size_t> equalSetMaps[MAX_REGTYPES_NUM];
    std::vector<std::vector<size_t> > equalSetLists[MAX_REGTYPES_NUM];
    
public:
    AsmRegAllocator(Assembler& assembler);
    // constructor for testing
    AsmRegAllocator(Assembler& assembler, const std::vector<CodeBlock>& codeBlocks,
                const SSAReplacesMap& ssaReplacesMap);
    
    void createCodeStructure(const std::vector<AsmCodeFlowEntry>& codeFlow,
             size_t codeSize, const cxbyte* code);
    void createSSAData(ISAUsageHandler& usageHandler);
    void applySSAReplaces();
    void createInterferenceGraph(ISAUsageHandler& usageHandler);
    void colorInterferenceGraph();
    
    void allocateRegisters(cxuint sectionId);
    
    const std::vector<CodeBlock>& getCodeBlocks() const
    { return codeBlocks; }
    const SSAReplacesMap& getSSAReplacesMap() const
    { return ssaReplacesMap; }
};

/// type of clause
enum class AsmClauseType
{
    IF,     ///< if clause
    ELSEIF, ///< elseif clause
    ELSE,   ///< else clause
    REPEAT, ///< rept clause or irp/irpc clause
    MACRO   ///< macro clause
};

/// assembler's clause (if,else,macro,rept)
struct AsmClause
{
    AsmClauseType type; ///< type of clause
    AsmSourcePos sourcePos;   ///< position in source code
    bool condSatisfied; ///< if conditional clause has already been satisfied
    AsmSourcePos prevIfPos; ///< position of previous if-clause
};

/// main class of assembler
class Assembler: public NonCopyableAndNonMovable
{
public:
    /// defined symbol entry
    typedef std::pair<CString, uint64_t> DefSym;
    /// kernel map type
    typedef std::unordered_map<CString, cxuint> KernelMap;
private:
    friend class AsmStreamInputFilter;
    friend class AsmMacroInputFilter;
    friend class AsmForInputFilter;
    friend class AsmExpression;
    friend class AsmFormatHandler;
    friend class AsmRawCodeHandler;
    friend class AsmAmdHandler;
    friend class AsmAmdCL2Handler;
    friend class AsmGalliumHandler;
    friend class AsmROCmHandler;
    friend class ISAAssembler;
    friend class AsmRegAllocator;
    
    friend struct AsmParseUtils; // INTERNAL LOGIC
    friend struct AsmPseudoOps; // INTERNAL LOGIC
    friend struct AsmGalliumPseudoOps; // INTERNAL LOGIC
    friend struct AsmAmdPseudoOps; // INTERNAL LOGIC
    friend struct AsmAmdCL2PseudoOps; // INTERNAL LOGIC
    friend struct AsmROCmPseudoOps; // INTERNAL LOGIC
    friend struct GCNAsmUtils; // INTERNAL LOGIC

    Array<CString> filenames;
    BinaryFormat format;
    GPUDeviceType deviceType;
    uint32_t driverVersion;
    uint32_t llvmVersion; // GalliumCompute
    bool _64bit;    ///
    bool newROCmBinFormat;
    bool good;
    bool resolvingRelocs;
    bool doNotRemoveFromSymbolClones;
    ISAAssembler* isaAssembler;
    std::vector<DefSym> defSyms;
    std::vector<CString> includeDirs;
    std::vector<AsmSection> sections;
    std::vector<Array<cxuint> > relSpacesSections;
    std::unordered_set<AsmSymbolEntry*> symbolSnapshots;
    std::unordered_set<AsmSymbolEntry*> symbolClones;
    std::vector<AsmExpression*> unevalExpressions;
    std::vector<AsmRelocation> relocations;
    AsmScope globalScope;
    AsmMacroMap macroMap;
    std::stack<AsmScope*> scopeStack;
    std::vector<AsmScope*> abandonedScopes;
    AsmScope* currentScope;
    KernelMap kernelMap;
    std::vector<AsmKernel> kernels;
    /// register variables
    Flags flags;
    uint64_t macroCount;
    uint64_t localCount; // macro's local count
    bool alternateMacro;
    bool buggyFPLit;
    bool macroCase;
    bool oldModParam;
    
    cxuint inclusionLevel;
    cxuint macroSubstLevel;
    cxuint repetitionLevel;
    bool lineAlreadyRead; // if line already read
    
    size_t lineSize;
    const char* line;
    bool endOfAssembly;
    bool sectionDiffsPrepared;
    
    cxuint filenameIndex;
    std::stack<AsmInputFilter*> asmInputFilters;
    AsmInputFilter* currentInputFilter;
    
    std::ostream& messageStream;
    std::ostream& printStream;
    
    AsmFormatHandler* formatHandler;
    
    std::stack<AsmClause> clauses;
    
    cxuint currentKernel;
    cxuint& currentSection;
    uint64_t& currentOutPos;
    
    bool withSectionDiffs() const
    { return formatHandler!=nullptr && formatHandler->isSectionDiffsResolvable(); }
    
    AsmSourcePos getSourcePos(LineCol lineCol) const
    {
        return { currentInputFilter->getMacroSubst(), currentInputFilter->getSource(),
            lineCol.lineNo, lineCol.colNo };
    }
    
    AsmSourcePos getSourcePos(size_t pos) const
    { return currentInputFilter->getSourcePos(pos); }
    AsmSourcePos getSourcePos(const char* linePtr) const
    { return getSourcePos(linePtr-line); }
    
    void printWarning(const AsmSourcePos& pos, const char* message);
    void printError(const AsmSourcePos& pos, const char* message);
    
    void printWarning(const char* linePtr, const char* message)
    { printWarning(getSourcePos(linePtr), message); }
    void printError(const char* linePtr, const char* message)
    { printError(getSourcePos(linePtr), message); }
    
    void printWarning(LineCol lineCol, const char* message)
    { printWarning(getSourcePos(lineCol), message); }
    void printError(LineCol lineCol, const char* message)
    { printError(getSourcePos(lineCol), message); }
    
    LineCol translatePos(const char* linePtr) const
    { return currentInputFilter->translatePos(linePtr-line); }
    LineCol translatePos(size_t pos) const
    { return currentInputFilter->translatePos(pos); }
    
    bool parseLiteral(uint64_t& value, const char*& linePtr);
    bool parseLiteralNoError(uint64_t& value, const char*& linePtr);
    bool parseString(std::string& outString, const char*& linePtr);
    
    enum class ParseState
    {
        FAILED = 0,
        PARSED,
        MISSING // missing element
    };
    
    /** parse symbol
     * \return state
     */
    ParseState parseSymbol(const char*& linePtr, AsmSymbolEntry*& entry,
                   bool localLabel = true, bool dontCreateSymbol = false);
    bool skipSymbol(const char*& linePtr);
    
    bool setSymbol(AsmSymbolEntry& symEntry, uint64_t value, cxuint sectionId);
    
    bool assignSymbol(const CString& symbolName, const char* symbolPlace,
                  const char* linePtr, bool reassign = true, bool baseExpr = false);
    
    bool assignOutputCounter(const char* symbolPlace, uint64_t value, cxuint sectionId,
                     cxbyte fillValue = 0);
    
    void parsePseudoOps(const CString& firstName, const char* stmtPlace,
                const char* linePtr);
    
    /// exitm - exit macro mode
    bool skipClauses(bool exitm = false);
    bool putMacroContent(RefPtr<AsmMacro> macro);
    bool putRepetitionContent(AsmRepeat& repeat);
    
    void initializeOutputFormat();
    
    bool pushClause(const char* string, AsmClauseType clauseType)
    {
        bool included; // to ignore
        return pushClause(string, clauseType, true, included);
    }
    bool pushClause(const char* string, AsmClauseType clauseType,
                  bool satisfied, bool& included);
     // return false when failed (for example no clauses)
    bool popClause(const char* string, AsmClauseType clauseType);
    
    // recursive function to find scope in scope
    AsmScope* findScopeInScope(AsmScope* scope, const CString& scopeName,
                    std::unordered_set<AsmScope*>& scopeSet);
    // find scope by identifier
    AsmScope* getRecurScope(const CString& scopePlace, bool ignoreLast = false,
                    const char** lastStep = nullptr);
    // find symbol in scopes
    // internal recursive function to find symbol in scope
    AsmSymbolEntry* findSymbolInScopeInt(AsmScope* scope, const CString& symName,
                    std::unordered_set<AsmScope*>& scopeSet);
    // scope - return scope from scoped name
    AsmSymbolEntry* findSymbolInScope(const CString& symName, AsmScope*& scope,
                      CString& sameSymName, bool insertMode = false);
    // similar to map::insert, but returns pointer
    std::pair<AsmSymbolEntry*, bool> insertSymbolInScope(const CString& symName,
                 const AsmSymbol& symbol);
    
    // internal recursive function to find symbol in scope
    AsmRegVarEntry* findRegVarInScopeInt(AsmScope* scope, const CString& rvName,
                    std::unordered_set<AsmScope*>& scopeSet);
    // scope - return scope from scoped name
    AsmRegVarEntry* findRegVarInScope(const CString& rvName, AsmScope*& scope,
                      CString& sameRvName, bool insertMode = false);
    // similar to map::insert, but returns pointer
    std::pair<AsmRegVarEntry*, bool> insertRegVarInScope(const CString& rvName,
                 const AsmRegVar& regVar);
    
    // create scope
    bool getScope(AsmScope* parent, const CString& scopeName, AsmScope*& scope);
    // push new scope level
    bool pushScope(const CString& scopeName);
    bool popScope();
    
    /// returns false when includeLevel is too deep, throw error if failed a file opening
    bool includeFile(const char* pseudoOpPlace, const std::string& filename);
    
    ParseState makeMacroSubstitution(const char* string);
    
    bool parseMacroArgValue(const char*& linePtr, std::string& outStr);
    
    void putData(size_t size, const cxbyte* data)
    {
        AsmSection& section = sections[currentSection];
        section.content.insert(section.content.end(), data, data+size);
        currentOutPos += size;
    }
    
    cxbyte* reserveData(size_t size, cxbyte fillValue = 0);
    
    void goToMain(const char* pseudoOpPlace);
    void goToKernel(const char* pseudoOpPlace, const char* kernelName);
    void goToSection(const char* pseudoOpPlace, const char* sectionName, uint64_t align=0);
    void goToSection(const char* pseudoOpPlace, const char* sectionName,
                     AsmSectionType type, Flags flags, uint64_t align=0);
    void goToSection(const char* pseudoOpPlace, cxuint sectionId, uint64_t align=0);
    
    void printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos,
                  cxbyte signess = WS_BOTH);
    
    bool isAddressableSection() const
    {
        return currentSection==ASMSECT_ABS ||
                    (sections[currentSection].flags & ASMSECT_ADDRESSABLE) != 0;
    }
    bool isWriteableSection() const
    {
        return currentSection!=ASMSECT_ABS &&
                (sections[currentSection].flags & ASMSECT_WRITEABLE) != 0;
    }
    bool isResolvableSection() const
    {
        return currentSection==ASMSECT_ABS ||
                (sections[currentSection].flags & ASMSECT_UNRESOLVABLE) == 0;
    }
    bool isResolvableSection(cxuint sectionId) const
    {
        return sectionId==ASMSECT_ABS ||
                (sections[sectionId].flags & ASMSECT_UNRESOLVABLE) == 0;
    }
    
    // oldKernels and newKernels must be sorted
    void handleRegionsOnKernels(const std::vector<cxuint>& newKernels,
                const std::vector<cxuint>& oldKernels, cxuint codeSection);
    
    void tryToResolveSymbol(AsmSymbolEntry& symEntry);
    void tryToResolveSymbols(AsmScope* scope);
    void printUnresolvedSymbols(AsmScope* scope);
    
    bool resolveExprTarget(const AsmExpression* expr, uint64_t value, cxuint sectionId);
    
    void cloneSymEntryIfNeeded(AsmSymbolEntry& symEntry);
    
    void undefineSymbol(AsmSymbolEntry& symEntry);
    
protected:
    /// helper for testing
    bool readLine();
public:
    /// constructor with filename and input stream
    /**
     * \param filename filename
     * \param input input stream
     * \param flags assembler flags
     * \param format output format type
     * \param deviceType GPU device type
     * \param msgStream stream for warnings and errors
     * \param printStream stream for printing message by .print pseudo-ops
     */
    explicit Assembler(const CString& filename, std::istream& input, Flags flags = 0,
              BinaryFormat format = BinaryFormat::AMD,
              GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE,
              std::ostream& msgStream = std::cerr, std::ostream& printStream = std::cout);
    
    /// constructor with filename and input stream
    /**
     * \param filenames filenames
     * \param flags assembler flags
     * \param format output format type
     * \param deviceType GPU device type
     * \param msgStream stream for warnings and errors
     * \param printStream stream for printing message by .print pseudo-ops
     */
    explicit Assembler(const Array<CString>& filenames, Flags flags = 0,
              BinaryFormat format = BinaryFormat::AMD,
              GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE,
              std::ostream& msgStream = std::cerr, std::ostream& printStream = std::cout);
    /// destructor
    ~Assembler();
    
    /// main routine to assemble code
    bool assemble();
    
    /// write binary to file
    void writeBinary(const char* filename) const;
    /// write binary to stream
    void writeBinary(std::ostream& outStream) const;
    /// write binary to array
    void writeBinary(Array<cxbyte>& array) const;
    
    /// get AMD driver version
    uint32_t getDriverVersion() const
    { return driverVersion; }
    /// set AMD driver version
    void setDriverVersion(uint32_t driverVersion)
    { this->driverVersion = driverVersion; }
    
    /// get LLVM version
    uint32_t getLLVMVersion() const
    { return llvmVersion; }
    /// set LLVM version
    void setLLVMVersion(uint32_t llvmVersion)
    { this->llvmVersion = llvmVersion; }
    
    /// get GPU device type
    GPUDeviceType getDeviceType() const
    { return deviceType; }
    /// set GPU device type
    void setDeviceType(const GPUDeviceType deviceType)
    { this->deviceType = deviceType; }
    /// get binary format
    BinaryFormat getBinaryFormat() const
    { return format; }
    /// set binary format
    void setBinaryFormat(BinaryFormat binFormat)
    { format = binFormat; }
    /// get bitness (true if 64-bit)
    bool is64Bit() const
    { return _64bit; }
    /// set bitness (true if 64-bit)
    void set64Bit(bool this64Bit)
    {  _64bit = this64Bit; }
    /// is new ROCm binary format
    bool isNewROCmBinFormat() const
    { return newROCmBinFormat; }
    /// set new ROCm binary format
    void setNewROCmBinFormat(bool newFmt)
    { newROCmBinFormat = newFmt; }
    /// get flags
    Flags getFlags() const
    { return flags; }
    /// set flags
    void setFlags(Flags flags)
    { this->flags = flags; }
    /// get true if altMacro enabled
    bool isAltMacro() const
    { return alternateMacro; }
    /// get true if macroCase enabled
    bool isMacroCase() const
    { return macroCase; }
    /// get true if oldModParam enabled (old modifier parametrization)
    bool isOldModParam() const
    { return oldModParam; }
    /// get true if buggyFPLit enabled
    bool isBuggyFPLit() const
    { return buggyFPLit; }
    /// get include directory list
    const std::vector<CString>& getIncludeDirs() const
    { return includeDirs; }
    /// adds include directory
    void addIncludeDir(const CString& includeDir);
    /// get symbols map
    const AsmSymbolMap& getSymbolMap() const
    { return globalScope.symbolMap; }
    /// get sections
    const std::vector<AsmSection>& getSections() const
    { return sections; }
    // get first sections for rel spaces
    const std::vector<Array<cxuint> >& getRelSpacesSections() const
    { return relSpacesSections; }
    /// get kernel map
    const KernelMap& getKernelMap() const
    { return kernelMap; }
    /// get kernels
    const std::vector<AsmKernel>& getKernels() const
    { return kernels; }
    /// get regvar map
    const AsmRegVarMap& getRegVarMap() const
    { return globalScope.regVarMap; }
    /// add regvar
    bool addRegVar(const CString& name, const AsmRegVar& var)
    { return insertRegVarInScope(name, var).second; }
    /// get regvar by name
    bool getRegVar(const CString& name, const AsmRegVar*& regVar);
    
    /// get global scope
    const AsmScope& getGlobalScope() const
    { return globalScope; }
    
    /// returns true if symbol contains absolute value
    bool isAbsoluteSymbol(const AsmSymbol& symbol) const;
    
    /// add initiali defsyms
    void addInitialDefSym(const CString& symName, uint64_t value);
    
    /// get format handler
    const AsmFormatHandler* getFormatHandler() const
    { return formatHandler; }
};

inline void ISAAssembler::printWarning(const char* linePtr, const char* message)
{ assembler.printWarning(linePtr, message); }

inline void ISAAssembler::printError(const char* linePtr, const char* message)
{ assembler.printError(linePtr, message); }

inline void ISAAssembler::printWarningForRange(cxuint bits, uint64_t value,
                   const AsmSourcePos& pos, cxbyte signess)
{ assembler.printWarningForRange(bits, value, pos, signess); }

inline void ISAAssembler::printWarning(const AsmSourcePos& sourcePos, const char* message)
{ assembler.printWarning(sourcePos, message); }

inline void ISAAssembler::printError(const AsmSourcePos& sourcePos, const char* message)
{ assembler.printError(sourcePos, message); }

inline void ISAAssembler::addCodeFlowEntry(cxuint sectionId, const AsmCodeFlowEntry& entry)
{ assembler.sections[sectionId].addCodeFlowEntry(entry); }

};

#endif
