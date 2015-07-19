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
/*! \file Assembler.h
 * \brief an assembler for Radeon GPU's
 */

#ifndef __CLRX_ASSEMBLER_H__
#define __CLRX_ASSEMBLER_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
#include <iostream>
#include <vector>
#include <utility>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Commons.h>
#include <CLRX/amdasm/AsmSource.h>

/// main namespace
namespace CLRX
{

enum: Flags
{
    ASM_WARNINGS = 1,   ///< enable all warnings for assembler
    ASM_64BIT_MODE = 2, ///< assemble to 64-bit addressing mode
    ASM_GNU_AS_COMPAT = 4, ///< compatibility with GNU as (expressions)
    ASM_ALL = FLAGS_ALL  ///< all flags
};

enum: cxuint
{
    ASMSECT_ABS = UINT_MAX,  ///< absolute section id
    ASMSECT_NONE = UINT_MAX,  ///< none section id
    ASMKERN_GLOBAL = UINT_MAX ///< no kernel, global space
};

/// assembler section type
enum class AsmSectionType: cxbyte
{
    DATA = 0,       ///< kernel or global data
    CODE,           ///< code of program or kernel
    CONFIG,         ///< configuration (global or for kernel)
    LAST_COMMON = CONFIG,   ///< last common type
    
    AMD_HEADER = LAST_COMMON+1, ///< AMD Catalyst kernel's header
    AMD_METADATA,       ///< AMD Catalyst kernel's metadata
    AMD_CALNOTE,        ///< AMD CALNote
    
    GALLIUM_COMMENT = LAST_COMMON+1,    ///< gallium comment section
    EXTRA_SECTION = 0xff
};

class Assembler;

enum: Flags
{
    ASMSECT_WRITEABLE = 1,
    ASMSECT_ABS_ADDRESSABLE = 2
};

/// assembler format exception
class AsmFormatException: public Exception
{
public:
    /// default constructor
    AsmFormatException() = default;
    /// constructor from message
    explicit AsmFormatException(const std::string& message);
    /// destructor
    virtual ~AsmFormatException() noexcept = default;
};

/// assdembler format handler
class AsmFormatHandler: public NonCopyableAndNonMovable
{
public:
    struct SectionInfo
    {
        const char* name;
        AsmSectionType type;
        Flags flags;
    };
protected:
    friend struct AsmGalliumPseudoOps;
    
    Assembler& assembler;
    GPUDeviceType deviceType;
    bool _64Bit;
    
    cxuint currentKernel;
    cxuint currentSection;
    
    AsmFormatHandler(Assembler& assembler, GPUDeviceType deviceType, bool is64Bit);
public:
    virtual ~AsmFormatHandler();
    
    /// add/set kernel
    /** adds new kernel. throw AsmFormatException when addition failed.
     * Method should handles any constraint that doesn't allow to add kernel except
     * duplicate of name. */
    virtual cxuint addKernel(const char* kernelName) = 0;
    /// add section
    /** adds new section . throw AsmFormatException when addition failed.
     * Method should handles any constraint that doesn't allow to add section except
     * duplicate of name. */
    virtual cxuint addSection(const char* sectionName, cxuint kernelId) = 0;
    
    /// get section id if exists in current context, otherwise returns ASMSECT_NONE
    virtual cxuint getSectionId(const char* sectionName) const = 0;
    
    /// set current kernel
    virtual void setCurrentKernel(cxuint kernel) = 0;
    /// set current section, this method can change current kernel if that required 
    virtual void setCurrentSection(cxuint sectionId) = 0;
    
    /// get current section
    cxuint getCurrentSection() const
    { return currentSection; }
    /// get current kertnel
    cxuint getCurrentKernel() const
    { return currentKernel; }
    
    // get current section flags and type
    virtual SectionInfo getSectionInfo(cxuint sectionId) const = 0;
    /// parse pseudo-op
    virtual void parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char*& linePtr) = 0;
    /// prepare binary for use
    virtual bool prepareBinary() = 0;
    /// write binary to output stream
    virtual void writeBinary(std::ostream& os) = 0;
    /// write binaery to output stream
    virtual void writeBinary(Array<cxbyte>& array) = 0;
};

/// handles raw code format
class AsmRawCodeHandler: public AsmFormatHandler
{
private:
    std::string kernelName;
public:
    AsmRawCodeHandler(Assembler& assembler, GPUDeviceType deviceType, bool is64Bit);
    ~AsmRawCodeHandler() = default;
    
    /// set current kernel by name
    cxuint addKernel(const char* kernelName);
    /// set current section by name
    cxuint addSection(const char* sectionName, cxuint kernelId);

    /// get section id if exists in current context, otherwise returns ASMSECT_NONE
    cxuint getSectionId(const char* sectionName) const;
    
    /// set current kernel
    void setCurrentKernel(cxuint kernel);
    /// set current section, this method can change current kernel if that required 
    void setCurrentSection(cxuint sectionId);
    
    // get current section flags and type
    SectionInfo getSectionInfo(cxuint sectionId) const;
    /// parse pseudo-op
    void parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char*& linePtr);
    
    bool prepareBinary();
    /// write binary to output stream
    void writeBinary(std::ostream& os);
    /// write binary to array
    void writeBinary(Array<cxbyte>& array);
};

/// handles AMD Catalyst format
class AsmAmdHandler: public AsmFormatHandler
{
public:
    /// section map type
    typedef std::unordered_map<std::string, cxuint> SectionMap;
private:
    friend struct AsmAmdPseudoOps;
    AmdInput output;
    struct Section
    {
        cxuint kernelId;
        AsmSectionType type;
        cxuint extraSectionId;
    };
    struct Kernel
    {
        cxuint headerSection;
        cxuint metadataSection;
        cxuint configSection;
        cxuint codeSection;
        cxuint dataSection;
        std::vector<cxuint> calNoteSections;
        SectionMap extraSections;
    };
    std::vector<Section> sections;
    std::vector<Kernel> kernelStates;
    cxuint dataSection; // global
    SectionMap extraSections;
public:
    AsmAmdHandler(Assembler& assembler, GPUDeviceType deviceType, bool is64Bit);
    ~AsmAmdHandler() = default;
    
    /// set current kernel by name
    cxuint addKernel(const char* kernelName);
    /// set current section by name
    cxuint addSection(const char* sectionName, cxuint kernelId);
    
    /// get section id if exists in current context, otherwise returns ASMSECT_NONE
    cxuint getSectionId(const char* sectionName) const;
    /// set current kernel
    void setCurrentKernel(cxuint kernel);
    /// set current section, this method can change current kernel if that required 
    void setCurrentSection(cxuint sectionId);
    
    // get current section flags and type
    SectionInfo getSectionInfo(cxuint sectionId) const;
    /// parse pseudo-op
    void parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char*& linePtr);
    
    bool prepareBinary();
    /// write binary to output stream
    void writeBinary(std::ostream& os);
    /// write binary to array
    void writeBinary(Array<cxbyte>& array);
    /// get output object (input for bingenerator)
    const AmdInput* getOutput() const
    { return &output; }
};

/// handles GalliumCompute format
class AsmGalliumHandler: public AsmFormatHandler
{
public:
    /// kernel map type
    typedef std::unordered_map<std::string, cxuint> SectionMap;
private:
    friend struct AsmGalliumPseudoOps;
    GalliumInput output;
    struct Section
    {
        cxuint kernelId;
        AsmSectionType type;
        cxuint extraSectionIndex;
        const char* name;    // must be available by whole lifecycle
    };
    struct Kernel
    {
        cxuint defaultSection;
        bool hasProgInfo;
        cxbyte progInfoEntries;
    };
    std::vector<Kernel> kernelStates;
    std::vector<Section> sections;
    SectionMap extraSectionMap;
    cxuint codeSection;
    cxuint dataSection;
    cxuint commentSection;
    bool insideProgInfo;
    bool insideArgs;
    cxuint extraSectionCount;
public:
    AsmGalliumHandler(Assembler& assembler, GPUDeviceType deviceType, bool is64Bit);
    ~AsmGalliumHandler() = default;
    
    /// set current kernel by name
    cxuint addKernel(const char* kernelName);
    /// set current section by name
    cxuint addSection(const char* sectionName, cxuint kernelId);
    
    /// get section id if exists in current context, otherwise returns ASMSECT_NONE
    cxuint getSectionId(const char* sectionName) const;
    /// set current kernel
    void setCurrentKernel(cxuint kernel);
    /// set current section, this method can change current kernel if that required 
    void setCurrentSection(cxuint sectionId);
    
    // get current section flags and type
    SectionInfo getSectionInfo(cxuint sectionId) const;
    /// parse pseudo-op
    void parsePseudoOp(const std::string& firstName,
           const char* stmtPlace, const char*& linePtr);
    
    bool prepareBinary();
    /// write binary to output stream
    void writeBinary(std::ostream& os);
    /// write binary to array
    void writeBinary(Array<cxbyte>& array);
    /// get output object (input for bingenerator)
    const GalliumInput* getOutput() const
    { return &output; }
};

/// ISA assembler class
class ISAAssembler
{
protected:
    Assembler& assembler;       ///< assembler
    
    /// constructor
    explicit ISAAssembler(Assembler& assembler);
public:
    /// destructor
    virtual ~ISAAssembler();
    
    /// assemble single line
    virtual size_t assemble(uint64_t lineNo, const char* line,
                std::vector<cxbyte>& output) = 0;
    /// resolve code with location, target and value
    virtual bool resolveCode(cxbyte* location, cxbyte targetType, uint64_t value) = 0;
    /// check if name is mnemonic
    virtual bool checkMnemonic(const std::string& mnemonic) const = 0;
    
    virtual cxuint* getAllocatedRegisters(size_t& regTypesNum) const = 0;
};

/// GCN arch assembler
class GCNAssembler: public ISAAssembler
{
public:
    /// constructor
    explicit GCNAssembler(Assembler& assembler);
    /// destructor
    ~GCNAssembler();
    
    /// assemble single line
    size_t assemble(uint64_t lineNo, const char* line, std::vector<cxbyte>& output);
    /// resolve code with location, target and value
    bool resolveCode(cxbyte* location, cxbyte targetType, uint64_t value);
    /// check if name is mnemonic
    bool checkMnemonic(const std::string& mnemonic) const;
    
    cxuint* getAllocatedRegisters(size_t& regTypesNum) const;
};

/*
 * assembler expressions
 */

/// assembler expression operator
enum class AsmExprOp : cxbyte
{
    ARG_VALUE = 0,  ///< absolute value
    ARG_SYMBOL = 1,  ///< absolute symbol without defined value
    NEGATE = 2, ///< negation
    BIT_NOT,    ///< binary negation
    LOGICAL_NOT,    ///< logical negation
    PLUS,   ///< plus (nothing)
    ADDITION,   ///< addition
    SUBTRACT,       ///< subtraction
    MULTIPLY,       ///< multiplication
    DIVISION,       ///< unsigned division
    SIGNED_DIVISION,    ///< signed division
    MODULO,         ///< unsigned modulo
    SIGNED_MODULO,      ///< signed modulo
    BIT_AND,        ///< bitwise AND
    BIT_OR,         //< bitwise OR
    BIT_XOR,        ///< bitwise XOR
    BIT_ORNOT,      ///< bitwise OR-not
    SHIFT_LEFT,     ///< shift left
    SHIFT_RIGHT,    ///< logical shift irght
    SIGNED_SHIFT_RIGHT, ///< signed (arithmetic) shift right
    LOGICAL_AND,    ///< logical AND
    LOGICAL_OR,     ///< logical OR
    EQUAL,          ///< equality
    NOT_EQUAL,      ///< unequality
    LESS,           ///< less than
    LESS_EQ,        ///< less or equal than
    GREATER,        ///< greater than
    GREATER_EQ,     ///< greater or equal than
    BELOW, ///< unsigned less
    BELOW_EQ, ///< unsigned less or equal
    ABOVE, ///< unsigned less
    ABOVE_EQ, ///< unsigned less or equal
    CHOICE,  ///< a ? b : c
    CHOICE_START,   ///< helper
    FIRST_ARG = ARG_VALUE,
    LAST_ARG = ARG_SYMBOL,
    FIRST_UNARY = NEGATE,   ///< helper
    LAST_UNARY = PLUS,  ///< helper
    FIRST_BINARY = ADDITION,    ///< helper
    LAST_BINARY = ABOVE_EQ, ///< helper
    NONE = 0xff ///< none operation
};

struct AsmExprTarget;

typedef cxbyte AsmExprTargetType;

enum : AsmExprTargetType
{
    ASMXTGT_SYMBOL = 0, ///< target is symbol
    ASMXTGT_DATA8,      ///< target is byte
    ASMXTGT_DATA16,     ///< target is 16-bit word
    ASMXTGT_DATA32,     ///< target is 32-bit word
    ASMXTGT_DATA64      ///< target is 64-bit word
};

union AsmExprArg;

class AsmExpression;

/// assembler symbol occurrence in expression
struct AsmExprSymbolOccurrence
{
    AsmExpression* expression;      ///< target expression pointer
    size_t argIndex;        ///< argument index
    size_t opIndex;         ///< operator index
    
    /// comparison operator
    bool operator==(const AsmExprSymbolOccurrence& b) const
    { return expression==b.expression && opIndex==b.opIndex && argIndex==b.argIndex; }
};

/// assembler symbol structure
struct AsmSymbol
{
    cxuint refCount;    ///< reference counter (for internal use only)
    cxuint sectionId;       ///< section id
    cxbyte info;           ///< ELF symbol info
    cxbyte other;           ///< ELF symbol other
    cxuint hasValue:1;         ///< symbol is defined
    cxuint onceDefined:1;       ///< symbol can be only once defined (likes labels)
    cxuint resolving:1;         ///< helper
    cxuint base:1;              ///< with base expression
    cxuint snapshot:1;          ///< if symbol is snapshot
    uint64_t value;         ///< value of symbol
    uint64_t size;          ///< size of symbol
    AsmExpression* expression;      ///< expression of symbol (if not resolved)
    
    /** list of occurrences in expressions */
    std::vector<AsmExprSymbolOccurrence> occurrencesInExprs;
    
    /// empty constructor
    explicit AsmSymbol(bool _onceDefined = false) :
            refCount(1), sectionId(ASMSECT_ABS), info(0), other(0), hasValue(false),
            onceDefined(_onceDefined), resolving(false), base(false), snapshot(false),
            value(0), size(0), expression(nullptr)
    { }
    /// constructor with expression
    explicit AsmSymbol(AsmExpression* expr, bool _onceDefined = false, bool _base = false) :
            refCount(1), sectionId(ASMSECT_ABS), info(0), other(0), hasValue(false),
            onceDefined(_onceDefined), resolving(false), base(_base),
            snapshot(false), value(0), size(0), expression(expr)
    { }
    /// constructor with value and section id
    explicit AsmSymbol(cxuint _sectionId, uint64_t _value, bool _onceDefined = false) :
            refCount(1), sectionId(_sectionId), info(0), other(0), hasValue(true),
            onceDefined(_onceDefined), resolving(false), base(false), snapshot(false),
            value(_value), size(0), expression(nullptr)
    { }
    /// destructor
    ~AsmSymbol();
    
    /// adds occurrence in expression
    void addOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex)
    { occurrencesInExprs.push_back({expr, argIndex, opIndex}); }
    /// remove occurrence in expression
    void removeOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex);
    /// clear list of occurrences in expression
    void clearOccurrencesInExpr();
    /// make symbol as undefined
    void undefine();
    
    bool isDefined() const
    { return hasValue || expression!=nullptr; }
};

/// assembler symbol map
typedef std::unordered_map<std::string, AsmSymbol> AsmSymbolMap;
/// assembler symbol entry
typedef AsmSymbolMap::value_type AsmSymbolEntry;

/// target for assembler expression
struct AsmExprTarget
{
    AsmExprTargetType type;    ///< type of target
    union
    {
        AsmSymbolEntry* symbol; ///< symbol entry (if ASMXTGT_SYMBOL)
        struct {
            cxuint sectionId;   ///< section id of destination
            size_t offset;      ///< offset of destination
        };
    };
    /// make symbol target for expression
    static AsmExprTarget symbolTarget(AsmSymbolEntry* entry)
    { 
        AsmExprTarget target;
        target.type = ASMXTGT_SYMBOL;
        target.symbol = entry;
        return target;
    }
    
    /// make n-bit word target for expression
    template<typename T>
    static AsmExprTarget dataTarget(cxuint sectionId, size_t offset)
    {
        AsmExprTarget target;
        target.type = (sizeof(T)==1) ? ASMXTGT_DATA8 : (sizeof(T)==2) ? ASMXTGT_DATA16 :
                (sizeof(T)==4) ? ASMXTGT_DATA32 : ASMXTGT_DATA64;
        target.sectionId = sectionId;
        target.offset = offset;
        return target;
    }
};

/// assembler expression class
class AsmExpression: public NonCopyableAndNonMovable
{
private:
    class TempSymbolSnapshotMap;
    
    AsmExprTarget target;
    AsmSourcePos sourcePos;
    size_t symOccursNum;
    bool relativeSymOccurs;
    bool baseExpr;
    Array<AsmExprOp> ops;
    std::unique_ptr<LineCol[]> messagePositions;    ///< for every potential message
    std::unique_ptr<AsmExprArg[]> args;
    
    AsmSourcePos getSourcePos(size_t msgPosIndex) const
    {
        AsmSourcePos pos = sourcePos;
        pos.lineNo = messagePositions[msgPosIndex].lineNo;
        pos.colNo = messagePositions[msgPosIndex].colNo;
        return pos;
    }
    
    static bool makeSymbolSnapshot(Assembler& assembler,
               TempSymbolSnapshotMap* snapshotMap, const AsmSymbolEntry& symEntry,
               AsmSymbolEntry*& outSymEntry, const AsmSourcePos* topParentSourcePos);
    
    AsmExpression();
    void setParams(size_t symOccursNum, bool relativeSymOccurs,
            size_t _opsNum, const AsmExprOp* ops, size_t opPosNum, const LineCol* opPos,
            size_t argsNum, const AsmExprArg* args, bool baseExpr = false);
public:
    /// constructor of expression (helper)
    AsmExpression(const AsmSourcePos& pos, size_t symOccursNum, bool relativeSymOccurs,
            size_t opsNum, size_t opPosNum, size_t argsNum, bool baseExpr = false);
    /// constructor of expression (helper)
    AsmExpression(const AsmSourcePos& pos, size_t symOccursNum, bool relativeSymOccurs,
              size_t opsNum, const AsmExprOp* ops, size_t opPosNum,
              const LineCol* opPos, size_t argsNum, const AsmExprArg* args,
              bool baseExpr = false);
    /// destructor
    ~AsmExpression();
    
    /// return true if expression is empty
    bool isEmpty() const
    { return ops.empty(); }

    /// helper to create symbol snapshot. Creates initial expression for symbol snapshot
    AsmExpression* createForSnapshot(const AsmSourcePos* exprSourcePos) const;
    
    /// set target of expression
    void setTarget(AsmExprTarget _target)
    { target = _target; }
    
    /// try to evaluate expression
    /**
     * \param assembler assembler instace
     * \param value output value
     * \param sectionId output section id
     * \return true if evaluated
     */
    bool evaluate(Assembler& assembler, uint64_t& value, cxuint& sectionId) const;
    
    /// parse expression. By default, also gets values of symbol or  creates them
    /** parse expresion from assembler's line string. Accepts empty expression.
     * \param assembler assembler
     * \param linePos position in line and output position in line
     * \param makeBase do not evaluate resolved symbols, put them to expression
     * \param dontReolveSymbolsLater do not resolve symbols later
     * \return expression pointer
     */
    static AsmExpression* parse(Assembler& assembler, size_t& linePos,
                    bool makeBase = false, bool dontReolveSymbolsLater = false);
    
    /// parse expression. By default, also gets values of symbol or  creates them
    /** parse expresion from assembler's line string. Accepts empty expression.
     * \param assembler assembler
     * \param linePtr string at position in line (returns output line pointer)
     * \param makeBase do not evaluate resolved symbols, put them to expression
     * \param dontReolveSymbolsLater do not resolve symbols later
     * \return expression pointer
     */
    static AsmExpression* parse(Assembler& assembler, const char*& linePtr,
              bool makeBase = false, bool dontReolveSymbolsLater = false);
    
    /// return true if is argument op
    static bool isArg(AsmExprOp op)
    { return (AsmExprOp::FIRST_ARG <= op && op <= AsmExprOp::LAST_ARG); }
    /// return true if is unary op
    static bool isUnaryOp(AsmExprOp op)
    { return (AsmExprOp::FIRST_UNARY <= op && op <= AsmExprOp::LAST_UNARY); }
    /// return true if is binary op
    static bool isBinaryOp(AsmExprOp op)
    { return (AsmExprOp::FIRST_BINARY <= op && op <= AsmExprOp::LAST_BINARY); }
    /// get targer of expression
    const AsmExprTarget& getTarget() const
    { return target; }
    /// get number of symbol occurrences in expression
    size_t getSymOccursNum() const
    { return symOccursNum; }
    /// get number of symbol occurrences in expression
    size_t hasRelativeSymOccurs() const
    { return relativeSymOccurs; }
    /// unreference symbol occurrences in expression (used internally)
    bool unrefSymOccursNum()
    { return --symOccursNum!=0; }
    
    /// substitute occurrence in expression by value
    void substituteOccurrence(AsmExprSymbolOccurrence occurrence, uint64_t value,
                  cxuint sectionId = ASMSECT_ABS);
    /// get operators list
    const Array<AsmExprOp>& getOps() const
    { return ops; }
    /// get argument list
    const AsmExprArg* getArgs() const
    { return args.get(); }
    /// get source position
    const AsmSourcePos& getSourcePos() const
    { return sourcePos; }

    /// make symbol snapshot (required to implement .eqv pseudo-op)    
    static bool makeSymbolSnapshot(Assembler& assembler, const AsmSymbolEntry& symEntry,
               AsmSymbolEntry*& outSymEntry, const AsmSourcePos* parentExprSourcePos);
};

inline AsmSymbol::~AsmSymbol()
{
    if (base) delete expression; // if symbol with base expresion
    clearOccurrencesInExpr();
}

/// assembler expression argument
union AsmExprArg
{
    AsmSymbolEntry* symbol; ///< if symbol
    uint64_t value;         ///< value
    struct {
        uint64_t value;         ///< value
        cxuint sectionId;       ///< sectionId
    } relValue; ///< relative value (with section)
};

inline void AsmExpression::substituteOccurrence(AsmExprSymbolOccurrence occurrence,
                        uint64_t value, cxuint sectionId)
{
    ops[occurrence.opIndex] = AsmExprOp::ARG_VALUE;
    args[occurrence.argIndex].relValue.value = value;
    args[occurrence.argIndex].relValue.sectionId = sectionId;
    if (sectionId != ASMSECT_ABS)
        relativeSymOccurs = true;
}

/// assembler section
struct AsmSection
{
    cxuint kernelId;    ///< kernel id (optional)
    AsmSectionType type;        ///< type of section
    std::vector<cxbyte> content;    ///< content of section
};

/// type of clause
enum class AsmClauseType
{
    IF,     ///< if clause
    ELSEIF, ///< elseif clause
    ELSE,   ///< else clause
    REPEAT, ///< rept clause
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
    typedef std::pair<std::string, uint64_t> DefSym;
    /// macro map type
    typedef std::unordered_map<std::string, RefPtr<const AsmMacro> > MacroMap;
    /// kernel map type
    typedef std::unordered_map<std::string, cxuint> KernelMap;
private:
    friend class AsmStreamInputFilter;
    friend class AsmMacroInputFilter;
    friend class AsmExpression;
    friend class AsmFormatHandler;
    friend class AsmAmdHandler;
    friend class AsmGalliumHandler;
    
    friend struct AsmPseudoOps; // INTERNAL LOGIC
    friend struct AsmGalliumPseudoOps; // INTERNAL LOGIC
    friend struct AsmAmdPseudoOps; // INTERNAL LOGIC
    BinaryFormat format;
    
    GPUDeviceType deviceType;
    bool _64bit;    ///
    bool good;
    ISAAssembler* isaAssembler;
    std::vector<DefSym> defSyms;
    std::vector<std::string> includeDirs;
    std::vector<AsmSection> sections;
    AsmSymbolMap symbolMap;
    std::unordered_set<AsmSymbolEntry*> symbolSnapshots;
    MacroMap macroMap;
    KernelMap kernelMap;
    std::vector<AsmSourcePos> kernelPositions;
    Flags flags;
    uint64_t macroCount;
    
    cxuint inclusionLevel;
    cxuint macroSubstLevel;
    cxuint repetitionLevel;
    bool lineAlreadyRead; // if line already read
    
    size_t lineSize;
    const char* line;
    bool endOfAssembly;
    
    std::stack<AsmInputFilter*> asmInputFilters;
    AsmInputFilter* currentInputFilter;
    
    std::ostream& messageStream;
    std::ostream& printStream;
    
    union {
        AmdInput* amdOutput;
        GalliumInput* galliumOutput;
        std::vector<AsmSection>* rawCode;
    };
    
    std::stack<AsmClause> clauses;
    
    bool outFormatInitialized;
    
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
    
    bool assignSymbol(const std::string& symbolName, const char* symbolPlace,
                  const char* linePtr, bool reassign = true, bool baseExpr = false);
    
    bool assignOutputCounter(const char* symbolPlace, uint64_t value, cxuint sectionId,
                     cxbyte fillValue = 0);
    
    void parsePseudoOps(const std::string firstName, const char* stmtPlace,
                const char*& linePtr);
    
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

    cxbyte* reserveData(size_t size, cxbyte fillValue = 0)
    {
        if (currentSection != ASMSECT_ABS)
        {
            size_t oldOutPos = currentOutPos;
            AsmSection& section = sections[currentSection];
            section.content.insert(section.content.end(), size, fillValue);
            currentOutPos += size;
            return section.content.data() + oldOutPos;
        }
        else
        {
            currentOutPos += size;
            return nullptr;
        }
    }
    
    void gotoKernel(const char* kernelName);
    void gotoSection(const char* sectionName);
    
    void printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos);
    
    bool checkReservedName(const std::string& name);
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
    explicit Assembler(const std::string& filename, std::istream& input, Flags flags = 0,
              BinaryFormat format = BinaryFormat::AMD,
              GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE,
              std::ostream& msgStream = std::cerr, std::ostream& printStream = std::cout);
    /// destructor
    ~Assembler();
    
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
    const std::vector<std::string>& getIncludeDirs() const
    { return includeDirs; }
    /// adds include directory
    void addIncludeDir(const std::string& includeDir);
    /// get symbols map
    const AsmSymbolMap& getSymbolMap() const
    { return symbolMap; }
    /// get sections
    const std::vector<AsmSection>& getSections() const
    { return sections; }
    /// get kernel map
    const KernelMap& getKernelMap() const
    { return kernelMap; }
    /// get kernel position
    const AsmSourcePos& getKernelPosition(cxuint kernelId) const
    { return kernelPositions[kernelId]; }
    
    /// returns true if symbol contains absolute value
    bool isAbsoluteSymbol(const AsmSymbol& symbol) const;
    
    /// add initiali defsyms
    void addInitialDefSym(const std::string& symName, uint64_t name);
    /// get AMD Catalyst output
    const AmdInput* getAmdOutput() const
    { return amdOutput; }
    /// get GalliumCompute output
    const GalliumInput* getGalliumOutput() const
    { return galliumOutput; }
    /// main routine to assemble code
    bool assemble();
};

};

#endif
