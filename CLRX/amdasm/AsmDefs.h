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
/*! \file AsmDefs.h
 * \brief an assembler for Radeon GPU's
 */

#ifndef __CLRX_ASMDEFS_H__
#define __CLRX_ASMDEFS_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <vector>
#include <utility>
#include <list>
#include <unordered_map>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Commons.h>
#include <CLRX/amdasm/AsmSource.h>
#include <CLRX/amdasm/AsmFormats.h>

/// main namespace
namespace CLRX
{

/// Assembler exception class
class AsmException: public Exception
{
public:
    /// empty constructor
    AsmException() = default;
    /// constructor with messasge
    explicit AsmException(const std::string& message);
    /// destructor
    virtual ~AsmException() noexcept = default;
};

enum: cxbyte {
    WS_UNSIGNED = 0,  // only unsigned
    WS_BOTH = 1,  // both signed and unsigned range checking
};

/// expression target type (one byte)
typedef cxbyte AsmExprTargetType;

enum : AsmExprTargetType
{
    ASMXTGT_SYMBOL = 0, ///< target is symbol
    ASMXTGT_DATA8,      ///< target is byte
    ASMXTGT_DATA16,     ///< target is 16-bit word
    ASMXTGT_DATA32,     ///< target is 32-bit word
    ASMXTGT_DATA64,     ///< target is 64-bit word
    ASMXTGT_CODEFLOW    ///< code flow entry
};

/*
 * assembler expressions
 */

/// assembler expression operator
enum class AsmExprOp : cxbyte
{
    ARG_VALUE = 0,  ///< value
    ARG_SYMBOL = 1,  ///< symbol without defined value
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

struct AsmRegVar;
struct AsmScope;

/// assembler symbol structure
struct AsmSymbol
{
    cxuint refCount;    ///< reference counter (for internal use only)
    AsmSectionId sectionId;       ///< section id
    cxbyte info;           ///< ELF symbol info
    cxbyte other;           ///< ELF symbol other
    cxuint hasValue:1;         ///< symbol is defined
    cxuint onceDefined:1;       ///< symbol can be only once defined (likes labels)
    cxuint resolving:1;         ///< helper
    cxuint base:1;              ///< with base expression
    cxuint snapshot:1;          ///< if symbol is snapshot
    cxuint regRange:1;          ///< if symbol is register range
    cxuint detached:1;
    cxuint withUnevalExpr:1;
    uint64_t value;         ///< value of symbol
    uint64_t size;          ///< size of symbol
    union {
        AsmExpression* expression;      ///< expression of symbol (if not resolved)
        const AsmRegVar* regVar;
    };
    
    /** list of occurrences in expressions */
    std::vector<AsmExprSymbolOccurrence> occurrencesInExprs;
    
    /// empty constructor
    explicit AsmSymbol(bool _onceDefined = false) :
            refCount(1), sectionId(ASMSECT_ABS), info(0), other(0), hasValue(false),
            onceDefined(_onceDefined), resolving(false), base(false), snapshot(false),
            regRange(false), detached(false), withUnevalExpr(false),
            value(0), size(0), expression(nullptr)
    { }
    /// constructor with expression
    explicit AsmSymbol(AsmExpression* expr, bool _onceDefined = false, bool _base = false) :
            refCount(1), sectionId(ASMSECT_ABS), info(0), other(0), hasValue(false),
            onceDefined(_onceDefined), resolving(false), base(_base),
            snapshot(false), regRange(false), detached(false), withUnevalExpr(false),
            value(0), size(0), expression(expr)
    { }
    /// constructor with value and section id
    explicit AsmSymbol(AsmSectionId _sectionId, uint64_t _value, bool _onceDefined = false)
            : refCount(1), sectionId(_sectionId), info(0), other(0), hasValue(true),
            onceDefined(_onceDefined), resolving(false), base(false), snapshot(false),
            regRange(false), detached(false), withUnevalExpr(false),
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
    /// return true if symbol defined (have value or expression)
    bool isDefined() const
    { return hasValue || expression!=nullptr; }
};

/// assembler symbol map
typedef std::unordered_map<CString, AsmSymbol> AsmSymbolMap;
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
            AsmSectionId sectionId;   ///< section id of destination
            union {
                size_t offset;      ///< offset of destination
                size_t cflowId;     ///< cflow index of destination
            };
        };
    };
    /// empty constructor
    AsmExprTarget() { }
    
    /// constructor to create custom target
    AsmExprTarget(AsmExprTargetType _type, AsmSectionId _sectionId, size_t _offset)
            : type(_type), sectionId(_sectionId), offset(_offset)
    { }
    
    /// make symbol target for expression
    static AsmExprTarget symbolTarget(AsmSymbolEntry* entry)
    { 
        AsmExprTarget target;
        target.type = ASMXTGT_SYMBOL;
        target.symbol = entry;
        return target;
    }
    /// make code flow target for expression
    static AsmExprTarget codeFlowTarget(AsmSectionId sectionId, size_t cflowIndex)
    {
        AsmExprTarget target;
        target.type = ASMXTGT_CODEFLOW;
        target.sectionId = sectionId;
        target.cflowId = cflowIndex;
        return target;
    }
    
    /// make n-bit word target for expression
    template<typename T>
    static AsmExprTarget dataTarget(AsmSectionId sectionId, size_t offset)
    {
        AsmExprTarget target;
        target.type = (sizeof(T)==1) ? ASMXTGT_DATA8 : (sizeof(T)==2) ? ASMXTGT_DATA16 :
                (sizeof(T)==4) ? ASMXTGT_DATA32 : ASMXTGT_DATA64;
        target.sectionId = sectionId;
        target.offset = offset;
        return target;
    }
};

/// assembler relocation
struct AsmRelocation
{
    AsmSectionId sectionId;   ///< section id where relocation is present
    size_t offset;  ///< offset of relocation
    RelocType type; ///< relocation type
    union {
        AsmSymbolEntry* symbol; ///< symbol
        AsmSectionId relSectionId;    ///< section for which relocation is defined
    };
    uint64_t addend;    ///< addend
};

/// class of return value for a trying routines
enum class AsmTryStatus
{
    FAILED = 0, ///< if failed now, no later trial
    TRY_LATER,  ///< try later (after some)
    SUCCESS     ///< succeed, no trial needed
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
    
    /// try to evaluate expression with/without section differences
    /** 
     * \param assembler assembler instace
     * \param opStart start operand
     * \param opEnd end operand
     * \param value output value
     * \param sectionId output section id
     * \param withSectionDiffs evaluate including precalculated section differences
     * \return operation status
     */
    AsmTryStatus tryEvaluate(Assembler& assembler, size_t opStart, size_t opEnd,
            uint64_t& value, AsmSectionId& sectionId, bool withSectionDiffs = false) const;
    
    /// try to evaluate expression with/without section differences
    /** 
     * \param assembler assembler instace
     * \param value output value
     * \param sectionId output section id
     * \param withSectionDiffs evaluate including precalculated section differences
     * \return operation status
     */
    AsmTryStatus tryEvaluate(Assembler& assembler, uint64_t& value, AsmSectionId& sectionId,
                    bool withSectionDiffs = false) const
    { return tryEvaluate(assembler, 0, ops.size(), value, sectionId, withSectionDiffs); }
    
    /// try to evaluate expression
    /**
     * \param assembler assembler instace
     * \param value output value
     * \param sectionId output section id
     * \return true if evaluated
     */
    bool evaluate(Assembler& assembler, uint64_t& value, AsmSectionId& sectionId) const
    { return tryEvaluate(assembler, 0, ops.size(), value, sectionId) !=
                    AsmTryStatus::FAILED; }
    
    /// try to evaluate expression
    /**
     * \param assembler assembler instace
     * \param value output value
     * \param opStart start operand
     * \param opEnd end operand
     * \param sectionId output section id
     * \return true if evaluated
     */
    bool evaluate(Assembler& assembler, size_t opStart, size_t opEnd,
                  uint64_t& value, AsmSectionId& sectionId) const
    { return tryEvaluate(assembler, opStart, opEnd, value, sectionId) !=
                    AsmTryStatus::FAILED; }
    
    /// parse expression. By default, also gets values of symbol or  creates them
    /** parse expresion from assembler's line string. Accepts empty expression.
     * \param assembler assembler
     * \param linePos position in line and output position in line
     * \param makeBase do not evaluate resolved symbols, put them to expression
     * \param dontResolveSymbolsLater do not resolve symbols later
     * \return expression pointer
     */
    static AsmExpression* parse(Assembler& assembler, size_t& linePos,
                    bool makeBase = false, bool dontResolveSymbolsLater = false);
    
    /// parse expression. By default, also gets values of symbol or  creates them
    /** parse expresion from assembler's line string. Accepts empty expression.
     * \param assembler assembler
     * \param linePtr string at position in line (returns output line pointer)
     * \param makeBase do not evaluate resolved symbols, put them to expression
     * \param dontResolveSymbolsLater do not resolve symbols later
     * \return expression pointer
     */
    static AsmExpression* parse(Assembler& assembler, const char*& linePtr,
              bool makeBase = false, bool dontResolveSymbolsLater = false);
    
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
                  AsmSectionId sectionId = ASMSECT_ABS);
    /// replace symbol in expression
    void replaceOccurrenceSymbol(AsmExprSymbolOccurrence occurrence,
                    AsmSymbolEntry* newSymEntry);
    /// get operators list
    const Array<AsmExprOp>& getOps() const
    { return ops; }
    /// get argument list
    const AsmExprArg* getArgs() const
    { return args.get(); }
    /// get source position
    const AsmSourcePos& getSourcePos() const
    { return sourcePos; }
    
    /// for internal usage
    size_t toTop(size_t opIndex) const;
    
    /// create an expression to evaluate from base expression
    AsmExpression* createExprToEvaluate(Assembler& assembler) const;
    
    /// make symbol snapshot (required to implement .eqv pseudo-op)    
    static bool makeSymbolSnapshot(Assembler& assembler, const AsmSymbolEntry& symEntry,
               AsmSymbolEntry*& outSymEntry, const AsmSourcePos* parentExprSourcePos);
    
    /// fast expression parse and evaluate
    static bool fastExprEvaluate(Assembler& assembler, const char*& linePtr,
                uint64_t& value);
    static bool fastExprEvaluate(Assembler& assembler, size_t& linePos,
                uint64_t& value);
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
        AsmSectionId sectionId;       ///< sectionId
    } relValue; ///< relative value (with section)
};

inline void AsmExpression::substituteOccurrence(AsmExprSymbolOccurrence occurrence,
                        uint64_t value, AsmSectionId sectionId)
{
    ops[occurrence.opIndex] = AsmExprOp::ARG_VALUE;
    args[occurrence.argIndex].relValue.value = value;
    args[occurrence.argIndex].relValue.sectionId = sectionId;
    if (sectionId != ASMSECT_ABS)
        relativeSymOccurs = true;
}

inline void AsmExpression::replaceOccurrenceSymbol(AsmExprSymbolOccurrence occurrence,
                    AsmSymbolEntry* newSymEntry)
{
    args[occurrence.argIndex].symbol = newSymEntry;
}

/// type of register field
typedef cxbyte AsmRegField;

enum : AsmRegField
{
    ASMFIELD_NONE = 0
};

struct AsmScope;

/// Regvar info structure
struct AsmRegVar
{
    cxuint type;    ///< scalar/vector/other
    uint16_t size;  ///< in regs
};

/// single regvar id
struct AsmSingleVReg // key for regvar
{
    const AsmRegVar* regVar;    ///< regvar
    uint16_t index; ///< index of regvar array
    
    /// equal operator
    bool operator==(const AsmSingleVReg& r2) const
    { return regVar == r2.regVar && index == r2.index; }
    /// not equal operator
    bool operator!=(const AsmSingleVReg& r2) const
    { return regVar == r2.regVar && index == r2.index; }
    
    /// less operator
    bool operator<(const AsmSingleVReg& r2) const
    { return regVar < r2.regVar || (regVar == r2.regVar && index < r2.index); }
};

/// regvar map
typedef std::unordered_map<CString, AsmRegVar> AsmRegVarMap;
/// regvar entry
typedef AsmRegVarMap::value_type AsmRegVarEntry;

enum : cxbyte {
    ASMRVU_READ = 1,        ///< register will be read
    ASMRVU_WRITE = 2,       ///< register will be written
    ASMRVU_ACCESS_MASK = 3, ///< access mask
    ASMRVU_REGTYPE_MASK = 4,    ///< for internal usage (regtype)
    ASMRVU_REGSIZE_SHIFT = 3,   ///< for internal usage
    ASMRVU_REGSIZE_MASK = 0x78  ///< for internal usage
};

/// regvar usage in code
struct AsmRegVarUsage
{
    size_t offset;  ///< offset in section
    const AsmRegVar* regVar;    ///< if null, then usage of called register
    uint16_t rstart; ///< register start
    uint16_t rend;  ///< register end
    AsmRegField regField;   ///< place in instruction
    cxbyte rwFlags;  ///< 1 - read, 2 - write
    cxbyte align;   ///< register alignment
    bool useRegMode; ///< if RVU from useReg pseudo-ops
};

/// internal structure for regvar linear dependencies
struct AsmRegVarLinearDep
{
    size_t offset;
    const AsmRegVar* regVar;    ///< regvar
    uint16_t rstart;    ///< register start
    uint16_t rend;      ///< register end
};

enum {
    ASM_DELAYED_OP_MAX_TYPES_NUM = 8,
    ASM_WAIT_MAX_TYPES_NUM = 4
};

/// asm delay instr type entry
struct AsmDelayedOpTypeEntry
{
    cxbyte waitType;
    bool ordered;
    /// waiting finished on register read out (true) or on operation
    bool finishOnRegReadOut;
    cxbyte counting; ///< counting (255 - per instr), 1-254 - per element size
};

/// asm wait system configuration
struct AsmWaitConfig
{
    cxuint delayedOpTypesNum;
    cxuint waitQueuesNum;
    AsmDelayedOpTypeEntry delayOpTypes[ASM_DELAYED_OP_MAX_TYPES_NUM];
    uint16_t waitQueueSizes[ASM_WAIT_MAX_TYPES_NUM];
};

enum : cxbyte
{
    ASMDELOP_NONE = 255
};

/// delayed result for register for instruction with delayed results
struct AsmDelayedOp
{
    size_t offset;
    const AsmRegVar* regVar;
    uint16_t rstart;
    uint16_t rend;
    cxbyte count;
    cxbyte delayedOpType;
    cxbyte delayedOpType2;
    cxbyte rwFlags:2;
    cxbyte rwFlags2:2;
};

/// description of the WAIT instruction (for waiting for results)
struct AsmWaitInstr
{
    size_t offset;
    uint16_t waits[ASM_WAIT_MAX_TYPES_NUM];
};

/// code flow type
enum AsmCodeFlowType
{
    JUMP = 0,   ///< jump
    CJUMP,   ///< conditional jump
    CALL,   ///< call of procedure
    RETURN, ///< return from procedure
    START,  ///< code start
    END     ///< code end
};

/// code flow entry
struct AsmCodeFlowEntry
{
    size_t offset;      ///< offset where is this entry
    size_t target;      ///< target jump addreses
    AsmCodeFlowType type;   ///< type of code flow entry
};

/// assembler macro map
typedef std::unordered_map<CString, RefPtr<const AsmMacro> > AsmMacroMap;

struct AsmScope;

/// type definition of scope's map
typedef std::unordered_map<CString, AsmScope*> AsmScopeMap;

/// assembler scope for symbol, macros, regvars
struct AsmScope
{
    AsmScope* parent;   ///< parent scope
    AsmSymbolMap symbolMap; ///< symbol map
    AsmRegVarMap regVarMap; ///< regvar map
    AsmScopeMap scopeMap;   ///< scope map
    bool temporary; ///< true if temporary
    std::list<AsmScope*> usedScopes;    ///< list of used scope in this scope
    uint64_t enumCount;
    
    /// set of used scopes in this scope
    std::unordered_map<AsmScope*, std::list<AsmScope*>::iterator> usedScopesSet;
    
    /// constructor
    AsmScope(AsmScope* _parent, const AsmSymbolMap& _symbolMap,
                     bool _temporary = false)
            : parent(_parent), symbolMap(_symbolMap), temporary(_temporary), enumCount(0)
    { }
    /// constructor
    AsmScope(AsmScope* _parent = nullptr, bool _temporary= false)
            : parent(_parent), temporary(_temporary), enumCount(0)
    { }
    /// destructor
    ~AsmScope();
    
    /// start using scope in this scope
    void startUsingScope(AsmScope* scope);
    /// stop using scope in this scope
    void stopUsingScope(AsmScope* scope);
    /// remove all usings
    void stopUsingScopes()
    {
        usedScopes.clear();
        usedScopesSet.clear();
    }
    /// delete symbols recursively
    void deleteSymbolsRecursively();
};

class ISAUsageHandler;
class ISALinearDepHandler;
class ISAWaitHandler;

/// assembler section
struct AsmSection
{
    const char* name;       ///< section name
    AsmKernelId kernelId;    ///< kernel id (optional)
    AsmSectionType type;        ///< type of section
    Flags flags;   ///< section flags
    uint64_t alignment; ///< section alignment
    uint64_t size;  ///< section size
    cxuint relSpace;    ///< relative space where is section
    uint64_t relAddress; ///< relative address
    std::vector<cxbyte> content;    ///< content of section
    
    std::unique_ptr<ISAUsageHandler> usageHandler;  ///< usage handler
    std::unique_ptr<ISALinearDepHandler> linearDepHandler; ///< linear dep handler
    std::unique_ptr<ISAWaitHandler> waitHandler; ///< wait handler
    std::vector<AsmCodeFlowEntry> codeFlow;  ///< code flow info
    AsmSourcePosHandler sourcePosHandler;
    
    /// constructor
    AsmSection();
    /// constructor
    AsmSection(const char* _name, AsmKernelId _kernelId, AsmSectionType _type, Flags _flags,
               uint64_t _alignment, uint64_t _size = 0, cxuint _relSpace = UINT_MAX,
               uint64_t _relAddress = UINT64_MAX)
            : name(_name), kernelId(_kernelId), type(_type), flags(_flags),
              alignment(_alignment), size(_size), relSpace(_relSpace),
              relAddress(_relAddress)
    { }
    
    /// copy constructor
    AsmSection(const AsmSection& section);
    /// copy assignment
    AsmSection& operator=(const AsmSection& section);
    
    /// add code flow entry to this section
    void addCodeFlowEntry(const AsmCodeFlowEntry& entry)
    { codeFlow.push_back(entry); }
    
    /// get section's size
    size_t getSize() const
    { return ((flags&ASMSECT_WRITEABLE) != 0) ? content.size() : size; }
};

/// kernel entry structure
struct AsmKernel
{
    const char* name;   ///< name of kernel
    AsmSourcePos sourcePos; ///< source position of definition
    std::vector<std::pair<size_t, size_t> > codeRegions; ///< code regions
    
    /// open kernel region in code
    void openCodeRegion(size_t offset);
    /// close kernel region in code
    void closeCodeRegion(size_t offset);
};

/// linears for regvars
class AsmRegVarLinears: std::vector<std::pair<uint16_t, uint16_t> >
{
public:
    /// constructor
    AsmRegVarLinears() { }
    
    /// insert new region if whole region is not some already region
    void insertRegion(const std::pair<uint16_t, uint16_t>& p)
    {
        const_iterator it;
        for (it = begin(); it != end(); ++it)
            if (it->first <= p.first && it->second >= p.second)
                break;
        if (it == end())
            push_back(p);
    }
};

};

namespace std
{

/// std::hash specialization for CLRX CString
template<>
struct hash<CLRX::AsmSingleVReg>
{
    typedef CLRX::AsmSingleVReg argument_type;    ///< argument type
    typedef std::size_t result_type;    ///< result type
    
    /// a calling operator
    size_t operator()(const CLRX::AsmSingleVReg& r1) const
    {
        std::hash<const CLRX::AsmRegVar*> h1;
        std::hash<uint16_t> h2;
        return h1(r1.regVar) ^ (h2(r1.index)<<1);
    }
};

}

#endif
