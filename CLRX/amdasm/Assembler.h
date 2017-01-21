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
#include <unordered_set>
#include <unordered_map>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Commons.h>
#include <CLRX/amdasm/AsmSource.h>
#include <CLRX/amdasm/AsmFormats.h>

/// main namespace
namespace CLRX
{

enum: Flags
{
    ASM_WARNINGS = 1,   ///< enable all warnings for assembler
    ASM_FORCE_ADD_SYMBOLS = 2,
    ASM_ALTMACRO = 4,
    ASM_BUGGYFPLIT = 8, // buggy handling of fpliterals (including fp constants)
    ASM_TESTRUN = (1U<<31), ///< only for running tests
    ASM_ALL = FLAGS_ALL&~(ASM_TESTRUN|ASM_BUGGYFPLIT)  ///< all flags
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

struct AsmRegVar;

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
    
    /// assemble single line
    virtual void assemble(const CString& mnemonic, const char* mnemPlace,
              const char* linePtr, const char* lineEnd, std::vector<cxbyte>& output) = 0;
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
    union {
        Regs regs;
        cxuint regTable[2];
    };
    uint16_t curArchMask;
public:
    /// constructor
    explicit GCNAssembler(Assembler& assembler);
    /// destructor
    ~GCNAssembler();
    
    void assemble(const CString& mnemonic, const char* mnemPlace, const char* linePtr,
                  const char* lineEnd, std::vector<cxbyte>& output);
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
        AsmRegVar* var;
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
            size_t offset;      ///< offset of destination
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
    GCNFIELD_SSRC0 = 0,
    GCNFIELD_SSRC1,
    GCNFIELD_SDST,
    GCNFIELD_SMRD_SBASE,
    GCNFIELD_SMRD_SDST,
    GCNFIELD_SMRD_SOFFSET,
    GCNFIELD_VOP_SRC0,
    GCNFIELD_VOP_SRC1,
    GCNFIELD_VOP_VDST,
    GCNFIELD_VOP3_VSRC0,
    GCNFIELD_VOP3_SRC1,
    GCNFIELD_VOP3_VDST,
    GCNFIELD_VOP3_SSRC,
    GCNFIELD_VOP3_SDST,
    GCNFIELD_VINTRP_VSRC0,
    GCNFIELD_VINTRP_VDST,
    GCNFIELD_DS_ADDR,
    GCNFIELD_DS_DATA0,
    GCNFIELD_DS_DATA1,
    GCNFIELD_DS_VDST,
    GCNFIELD_M_VADDR,
    GCNFIELD_M_VDATA,
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
    GCNFIELD_DPPSDWA_SRC0,
    GCNFIELD_SMEM_SBASE,
    GCNFIELD_SMEM_SDATA,
    GCNFIELD_SMEM_OFFSET,
    ASMFIELD_NONE = 255
};

enum : cxbyte
{
    GCNREGTYPE_SGPR,
    GCNREGTYPE_VGPR
};

struct AsmRegVar
{
    cxuint type;    // scalar/vector/other
    uint16_t size;  // in regs
};

struct AsmVarUsage
{
    size_t offset;
    AsmRegField regField;   ///< place in instruction
    uint16_t rstart, rend;
    bool read;
    bool write;
    cxbyte align;   /// register alignment
    const AsmRegVar* regVar;    // if null, then usage of called register
};

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
    
    /// register variables
    std::unordered_map<CString, AsmRegVar> regVars;
    /// reg-var usage in section
    std::vector<AsmVarUsage> regVarUsages;
    
    bool addRegVar(const CString& name, const AsmRegVar& var);
    
    bool getRegVar(const CString& name, const AsmRegVar*& regVar) const;
    
    void addVarUsage(const AsmVarUsage& varUsage)
    { regVarUsages.push_back(varUsage); }
    
    /// get section's size
    size_t getSize() const
    { return ((flags&ASMSECT_WRITEABLE) != 0) ? content.size() : size; }
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

/// kernel entry structure
struct AsmKernel
{
    const char* name;   ///< name of kernel
    AsmSourcePos sourcePos; ///< source position of definition
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
    /// macro map type
    typedef std::unordered_map<CString, RefPtr<const AsmMacro> > MacroMap;
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
    AsmSymbolMap symbolMap;
    std::unordered_set<AsmSymbolEntry*> symbolSnapshots;
    std::vector<AsmRelocation> relocations;
    MacroMap macroMap;
    KernelMap kernelMap;
    std::vector<AsmKernel> kernels;
    Flags flags;
    uint64_t macroCount;
    uint64_t localCount; // macro's local count
    bool alternateMacro;
    bool buggyFPLit;
    
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
    
    bool checkReservedName(const CString& name);
    
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
    { return symbolMap; }
    /// get sections
    const std::vector<AsmSection>& getSections() const
    { return sections; }
    /// get kernel map
    const KernelMap& getKernelMap() const
    { return kernelMap; }
    /// get kernels
    const std::vector<AsmKernel>& getKernels() const
    { return kernels; }
    
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
