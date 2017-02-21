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
    ASMXTGT_CODEFLOW,    ///< code flow entry
    
    GCNTGT_LITIMM = 16,
    GCNTGT_SOPKSIMM16,
    GCNTGT_SOPJMP,
    GCNTGT_SMRDOFFSET,
    GCNTGT_DSOFFSET16,
    GCNTGT_DSOFFSET8_0,
    GCNTGT_DSOFFSET8_1,
    GCNTGT_MXBUFOFFSET,
    GCNTGT_SMEMOFFSET,
    GCNTGT_SOPCIMM8,
    GCNTGT_SMEMIMM
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
    cxuint sectionId;       ///< section id
    cxbyte info;           ///< ELF symbol info
    cxbyte other;           ///< ELF symbol other
    cxuint hasValue:1;         ///< symbol is defined
    cxuint onceDefined:1;       ///< symbol can be only once defined (likes labels)
    cxuint resolving:1;         ///< helper
    cxuint base:1;              ///< with base expression
    cxuint snapshot:1;          ///< if symbol is snapshot
    cxuint regRange:1;          ///< if symbol is register range
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
            regRange(false), value(0), size(0), expression(nullptr)
    { }
    /// constructor with expression
    explicit AsmSymbol(AsmExpression* expr, bool _onceDefined = false, bool _base = false) :
            refCount(1), sectionId(ASMSECT_ABS), info(0), other(0), hasValue(false),
            onceDefined(_onceDefined), resolving(false), base(_base),
            snapshot(false), regRange(false), value(0), size(0), expression(expr)
    { }
    /// constructor with value and section id
    explicit AsmSymbol(cxuint _sectionId, uint64_t _value, bool _onceDefined = false) :
            refCount(1), sectionId(_sectionId), info(0), other(0), hasValue(true),
            onceDefined(_onceDefined), resolving(false), base(false), snapshot(false),
            regRange(false), value(_value), size(0), expression(nullptr)
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
            cxuint sectionId;   ///< section id of destination
            union {
                size_t offset;      ///< offset of destination
                size_t cflowId;     ///< cflow index of destination
            };
        };
    };
    /// empty constructor
    AsmExprTarget() { }
    
    /// constructor to create custom target
    AsmExprTarget(AsmExprTargetType _type, cxuint _sectionId, size_t _offset)
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
    
    static AsmExprTarget codeFlowTarget(cxuint sectionId, size_t cflowIndex)
    {
        AsmExprTarget target;
        target.type = ASMXTGT_CODEFLOW;
        target.sectionId = sectionId;
        target.cflowId = cflowIndex;
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

/// assembler relocation
struct AsmRelocation
{
    cxuint sectionId;   ///< section id where relocation is present
    size_t offset;  ///< offset of relocation
    RelocType type; ///< relocation type
    union {
        AsmSymbolEntry* symbol; ///< symbol
        cxuint relSectionId;    ///< section for which relocation is defined
    };
    uint64_t addend;    ///< addend
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
    bool evaluate(Assembler& assembler, uint64_t& value, cxuint& sectionId) const
    { return evaluate(assembler, 0, ops.size(), value, sectionId); }
    
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
                  uint64_t& value, cxuint& sectionId) const;
    
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
    
    size_t toTop(size_t opIndex) const;
    
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

typedef cxbyte AsmRegField;

enum : AsmRegField
{
    ASMFIELD_NONE = 0,
    GCNFIELD_SSRC0,
    GCNFIELD_SSRC1,
    GCNFIELD_SDST,
    GCNFIELD_SMRD_SBASE,
    GCNFIELD_SMRD_SDST,
    GCNFIELD_SMRD_SOFFSET,
    GCNFIELD_VOP_SRC0,
    GCNFIELD_VOP_VSRC1,
    GCNFIELD_VOP_SSRC1,
    GCNFIELD_VOP_VDST,
    GCNFIELD_VOP_SDST,
    GCNFIELD_VOP3_SRC0,
    GCNFIELD_VOP3_SRC1,
    GCNFIELD_VOP3_SRC2,
    GCNFIELD_VOP3_VDST,
    GCNFIELD_VOP3_SDST0,
    GCNFIELD_VOP3_SSRC,
    GCNFIELD_VOP3_SDST1,
    GCNFIELD_VINTRP_VSRC0,
    GCNFIELD_VINTRP_VDST,
    GCNFIELD_DS_ADDR,
    GCNFIELD_DS_DATA0,
    GCNFIELD_DS_DATA1,
    GCNFIELD_DS_VDST,
    GCNFIELD_M_VADDR,
    GCNFIELD_M_VDATA,
    GCNFIELD_M_VDATAH,
    GCNFIELD_M_VDATALAST,
    GCNFIELD_M_SRSRC,
    GCNFIELD_MIMG_SSAMP,
    GCNFIELD_M_SOFFSET,
    GCNFIELD_EXP_VSRC0,
    GCNFIELD_EXP_VSRC1,
    GCNFIELD_EXP_VSRC2,
    GCNFIELD_EXP_VSRC3,
    GCNFIELD_FLAT_ADDR,
    GCNFIELD_FLAT_DATA,
    GCNFIELD_FLAT_VDST,
    GCNFIELD_FLAT_VDSTLAST,
    GCNFIELD_DPPSDWA_SRC0
};

struct AsmScope;

struct AsmRegVar
{
    cxuint type;    // scalar/vector/other
    uint16_t size;  // in regs
};

/// regvar map
typedef std::unordered_map<CString, AsmRegVar> AsmRegVarMap;
/// regvar entry
typedef AsmRegVarMap::value_type AsmRegVarEntry;

enum : cxbyte {
    ASMRVU_READ = 1,
    ASMRVU_WRITE = 2,
    ASMRVU_ACCESS_MASK = 3,
    ASMRVU_REGTYPE_MASK = 4,
    ASMRVU_REGSIZE_SHIFT = 3,
    ASMRVU_REGSIZE_MASK = 0x78
};

/// regvar usage in code
struct AsmRegVarUsage
{
    size_t offset;
    const AsmRegVarEntry* regVar;    // if null, then usage of called register
    uint16_t rstart, rend;
    AsmRegField regField;   ///< place in instruction
    cxbyte rwFlags;  ///< 1 - read, 2 - write
    cxbyte align;   ///< register alignment
};

/// regvar usage (internal)
struct AsmRegVarUsageInt
{
    const AsmRegVarEntry* regVar;    // if null, then usage of called register
    uint16_t rstart, rend;
    AsmRegField regField;   ///< place in instruction
    cxbyte rwFlags;  ///< 1 - read, 2 - write
    cxbyte align;   ///< register alignment
    cxbyte nextDependency;
};

struct AsmRegUsageInt
{
    AsmRegField regField;
    cxbyte rwFlags;
};

enum AsmCodeFlowType
{
    JUMP = 0,
    CALL,
    RETURN,
    START,
    END     ///< code end
};

/// code flow entry
struct AsmCodeFlowEntry
{
    size_t offset;
    size_t target;      // target jump addreses
    AsmCodeFlowType type;
};

/// assembler macro map
typedef std::unordered_map<CString, RefPtr<const AsmMacro> > AsmMacroMap;

struct AsmScope;

typedef std::unordered_map<CString, AsmScope*> AsmScopeMap;

/// assembler scope for symbol, macros, regvars
struct AsmScope
{
    AsmScope* parent;
    AsmSymbolMap symbolMap;
    AsmRegVarMap regVarMap;
    AsmScopeMap scopeMap;
    bool temporary;
    std::list<AsmScope*> usedScopes;
    std::unordered_map<AsmScope*, std::list<AsmScope*>::iterator> usedScopesSet;
    
    AsmScope(AsmScope* _parent, const AsmSymbolMap& _symbolMap,
                     bool _temporary = false)
            : parent(_parent), symbolMap(_symbolMap), temporary(_temporary)
    { }
    AsmScope(AsmScope* _parent = nullptr, bool _temporary= false)
            : parent(_parent), temporary(_temporary)
    { }
    ~AsmScope();
    
    void startUsingScope(AsmScope* scope);
    void stopUsingScope(AsmScope* scope);
    void stopUsingScopes()
    {
        usedScopes.clear();
        usedScopesSet.clear();
    }
    void deleteSymbolsRecursively();
};

class ISAUsageHandler;

/// assembler section
struct AsmSection
{
    const char* name;       ///< section name
    cxuint kernelId;    ///< kernel id (optional)
    AsmSectionType type;        ///< type of section
    Flags flags;   ///< section flags
    uint64_t alignment; ///< section alignment
    uint64_t size;  ///< section size
    std::vector<cxbyte> content;    ///< content of section
    
    std::unique_ptr<ISAUsageHandler> usageHandler;
    /// code flow info
    std::vector<AsmCodeFlowEntry> codeFlow;
    
    AsmSection();
    AsmSection(const char* _name, cxuint _kernelId, AsmSectionType _type,
            Flags _flags, uint64_t _alignment, uint64_t _size = 0)
            : name(_name), kernelId(_kernelId), type(_type), flags(_flags),
              alignment(_alignment), size(_size)
    { }
    
    AsmSection(const AsmSection& section);
    AsmSection& operator=(const AsmSection& section);
    
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
    std::vector<std::pair<size_t, size_t> > codeRegions;
    
    /// open kernel region in code
    void openCodeRegion(size_t offset);
    /// close kernel region in code
    void closeCodeRegion(size_t offset);
};

};

#endif
