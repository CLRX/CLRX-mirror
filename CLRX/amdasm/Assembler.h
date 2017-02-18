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
    ASM_FORCE_ADD_SYMBOLS = 2,
    ASM_ALTMACRO = 4,
    ASM_BUGGYFPLIT = 8, // buggy handling of fpliterals (including fp constants)
    ASM_MACRONOCASE = 16, // disable case-insensitive naming (default)
    ASM_TESTRUN = (1U<<31), ///< only for running tests
    ASM_ALL = FLAGS_ALL&~(ASM_TESTRUN|ASM_BUGGYFPLIT|ASM_MACRONOCASE)  ///< all flags
};

struct AsmRegVar;

/* TODO: add some mechanism to resolve dependencies between register in single instruction
 * like, the same SGPR register, this same register in two fields, etc */
/// ISA (register and regvar) Usage handler
class ISAUsageHandler
{
protected:
    std::vector<cxbyte> instrStruct;
    std::vector<AsmRegUsageInt> regUsages;
    std::vector<AsmRegVarUsageInt> regVarUsages;
    const std::vector<cxbyte>& content;
    size_t lastOffset;
    size_t readOffset;
    size_t instrStructPos;
    size_t regUsagesPos;
    size_t regVarUsagesPos;
    cxbyte pushedArgs;
    cxbyte argPos;
    cxbyte argFlags;
    cxbyte defaultInstrSize;
    bool isNext;
    
    void skipBytesInInstrStruct();
    
    explicit ISAUsageHandler(const std::vector<cxbyte>& content);
public:
    virtual ~ISAUsageHandler();
    virtual ISAUsageHandler* copy() const = 0;
    
    void pushUsage(const AsmRegVarUsage& rvu);
    void rewind();
    void flush();
    bool hasNext() const
    { return isNext; }
    AsmRegVarUsage nextUsage();
    
    virtual cxbyte getRwFlags(AsmRegField regField, uint16_t rstart,
                      uint16_t rend) const = 0;
    virtual std::pair<uint16_t,uint16_t> getRegPair(AsmRegField regField,
                    cxbyte rwFlags) const = 0;
};

/// GCN (register and regvar) Usage handler
class GCNUsageHandler: public ISAUsageHandler
{
private:
    uint16_t archMask;
public:
    GCNUsageHandler(const std::vector<cxbyte>& content, uint16_t archMask);
    ~GCNUsageHandler();
    
    ISAUsageHandler* copy() const;
    
    cxbyte getRwFlags(AsmRegField regFied, uint16_t rstart, uint16_t rend) const;
    std::pair<uint16_t,uint16_t> getRegPair(AsmRegField regField, cxbyte rwFlags) const;
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
    /// constructor
    explicit ISAAssembler(Assembler& assembler);
public:
    /// destructor
    virtual ~ISAAssembler();
    
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
    void fillAlignment(size_t size, cxbyte* output);
    bool parseRegisterRange(const char*& linePtr, cxuint& regStart, cxuint& regEnd,
                const AsmRegVar*& regVar);
    bool relocationIsFit(cxuint bits, AsmExprTargetType tgtType);
    bool parseRegisterType(const char*& linePtr, const char* end, cxuint& type);
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
    friend class AsmExpression;
    friend class AsmFormatHandler;
    friend class AsmRawCodeHandler;
    friend class AsmAmdHandler;
    friend class AsmAmdCL2Handler;
    friend class AsmGalliumHandler;
    friend class AsmROCmHandler;
    friend class ISAAssembler;
    
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
    bool _64bit;    ///
    bool good;
    bool resolvingRelocs;
    ISAAssembler* isaAssembler;
    std::vector<DefSym> defSyms;
    std::vector<CString> includeDirs;
    std::vector<AsmSection> sections;
    std::unordered_set<AsmSymbolEntry*> symbolSnapshots;
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
    
    cxuint inclusionLevel;
    cxuint macroSubstLevel;
    cxuint repetitionLevel;
    bool lineAlreadyRead; // if line already read
    
    size_t lineSize;
    const char* line;
    bool endOfAssembly;
    
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
    
    void tryToResolveSymbols(AsmScope* scope);
    void printUnresolvedSymbols(AsmScope* scope);
    
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
    /// get bitness
    bool is64Bit() const
    { return _64bit; }
    /// set bitness
    void set64Bit(bool this64Bit)
    {  _64bit = this64Bit; }
    /// get flags
    Flags getFlags() const
    { return flags; }
    /// set flags
    void setFlags(Flags flags)
    { this->flags = flags; }
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
    bool getRegVarEntry(const CString& name, const AsmRegVarEntry*& regVarEntry);
    
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
{ return assembler.printWarning(linePtr, message); }

inline void ISAAssembler::printError(const char* linePtr, const char* message)
{ return assembler.printError(linePtr, message); }

inline void ISAAssembler::printWarningForRange(cxuint bits, uint64_t value,
                   const AsmSourcePos& pos, cxbyte signess)
{ return assembler.printWarningForRange(bits, value, pos, signess); }

inline void ISAAssembler::printWarning(const AsmSourcePos& sourcePos, const char* message)
{ return assembler.printWarning(sourcePos, message); }

inline void ISAAssembler::printError(const AsmSourcePos& sourcePos, const char* message)
{ return assembler.printError(sourcePos, message); }

};

#endif
