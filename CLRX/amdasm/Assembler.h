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
#include <deque>
#include <unordered_map>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/utils/Utilities.h>

/// main namespace
namespace CLRX
{

enum: cxuint
{
    ASM_WARNINGS = 1,   ///< enable all warnings for assembler
    ASM_64BIT_MODE = 2, ///< assemble to 64-bit addressing mode
    ASM_GNU_AS_COMPAT = 4, ///< compatibility with GNU as (expressions)
    ASM_ALL = 0xff  ///< all flags
};

/// assembler output format
enum class AsmFormat: cxbyte
{
    CATALYST = 0,       ///< AMD Catalyst binary format
    GALLIUM,            ///< GalliumCompute binary format
    RAWCODE             ///< raw code
};

enum: cxuint
{
    ASMSECT_ABS = UINT_MAX  ///< absolute section id
};

/// assembler section type
enum class AsmSectionType: cxbyte
{
    AMD_GLOBAL_DATA = 0,    ///< global data
    AMD_KERNEL_CODE,        ///< code (.text) of single kernel
    AMD_KERNEL_DATA,        ///< data from single kernel inner exec (unusable)
    AMD_KERNEL_HEADER,      ///< header of kernel
    AMD_KERNEL_METADATA,    ///< metadata
    
    GALLIUM_GLOBAL_DATA = 64,   ///< global data
    GALLIUM_COMMENT,            ///< comment section
    GALLIUM_DISASSEMBLY,        ///< disasm section
    GALLIUM_CODE,               ///< code of kernels
    
    RAWCODE_CODE = 128          ///< code
};

class Assembler;

/// line and column
struct LineCol
{
    uint64_t lineNo;    ///< line number
    size_t colNo;       ///< column number
};

/*
 * assembler source position
 */

/// source tpye
enum class AsmSourceType : cxbyte
{
    FILE,       ///< include file
    MACRO,      ///< macro substitution
    REPT        ///< repetition
};

/// descriptor of assembler source for source position
struct AsmSource: public FastRefCountable
{
    AsmSourceType type;    ///< type of Asm source (file or macro)
    
    /// constructor
    explicit AsmSource(AsmSourceType _type) : type(_type)
    { }
    /// destructor
    virtual ~AsmSource();
};

/// descriptor of file inclusion
struct AsmFile: public AsmSource
{
    ///  parent source for this source (for file is parent file or macro substitution,
    /// for macro substitution is parent substitution
    RefPtr<const AsmSource> parent; ///< parent file (or null if root)
    uint64_t lineNo; ///< place where file is included (0 if root)
    const std::string file; ///< file path
    
    /// constructor
    explicit AsmFile(const std::string& _file) : 
            AsmSource(AsmSourceType::FILE), lineNo(1), file(_file)
    { }
    
    /// constructor with parent file inclustion
    AsmFile(const RefPtr<const AsmSource> _parent, uint64_t _lineNo,
        const std::string& _file) : AsmSource(AsmSourceType::FILE),
        parent(_parent), lineNo(_lineNo), file(_file)
    { }
    /// destructor
    virtual ~AsmFile();
};

/// descriptor assembler macro substitution
struct AsmMacroSubst: public FastRefCountable
{
    ///  parent source for this source (for file is parent file or macro substitution,
    /// for macro substitution is parent substitution
    RefPtr<const AsmMacroSubst> parent;   ///< parent macro substition
    RefPtr<const AsmSource> source; ///< source of content where macro substituted
    uint64_t lineNo;  ///< place where macro substituted
    
    /// constructor
    AsmMacroSubst(RefPtr<const AsmSource> _source, uint64_t _lineNo) :
              source(_source), lineNo(_lineNo)
    { }
    /// constructor with parent macro substitution
    AsmMacroSubst(RefPtr<const AsmMacroSubst> _parent, RefPtr<const AsmSource> _source,
              size_t _lineNo) : parent(_parent), source(_source), lineNo(_lineNo)
    { }
};

/// descriptor of macro source (used in source fields)
struct AsmMacroSource: public AsmSource
{
    RefPtr<const AsmMacroSubst> macro;   ///< macro substition
    RefPtr<const AsmSource> source; ///< source of substituted content

    /// construcror
    AsmMacroSource(RefPtr<const AsmMacroSubst> _macro, RefPtr<const AsmSource> _source)
                : AsmSource(AsmSourceType::MACRO), macro(_macro), source(_source)
    { }
    /// destructor
    virtual ~AsmMacroSource();
};

/// descriptor of assembler repetition
struct AsmRepeatSource: public AsmSource
{
    RefPtr<const AsmSource> source; ///< source of content
    uint64_t repeatCount;   ///< number of repetition
    uint64_t repeatsNum;    ///< number of all repetitions
    
    /// constructor
    AsmRepeatSource(RefPtr<const AsmSource> _source, uint64_t _repeatCount,
            uint64_t _repeatsNum) : AsmSource(AsmSourceType::REPT), source(_source),
            repeatCount(_repeatCount), repeatsNum(_repeatsNum)
    { }
    /// destructor
    virtual ~AsmRepeatSource();
};

/// assembler source position
struct AsmSourcePos
{
    RefPtr<const AsmMacroSubst> macro; ///< macro substitution in which message occurred
    RefPtr<const AsmSource> source;   ///< source in which message occurred
    uint64_t lineNo;    ///< line number of top-most source
    size_t colNo;       ///< column number
    
    /// print source position
    void print(std::ostream& os, cxuint indentLevel = 0) const;
};

/// line translations
struct LineTrans
{
    size_t position;    ///< position in line
    uint64_t lineNo;    ///< destination line number
};

/// assembler macro aegument
struct AsmMacroArg
{
    std::string name;   ///< name
    std::string defaultValue;   ///< default value
    bool vararg;        ///< is variadic argument
    bool required;      ///< is required
};

/// assembler macro
class AsmMacro
{
public:
    /// source translation
    struct SourceTrans
    {
        uint64_t lineNo;    ///< line number
        RefPtr<const AsmSource> source; ///< source
    };
private:
    uint64_t contentLineNo;
    AsmSourcePos pos;
    Array<AsmMacroArg> args;
    std::vector<char> content;
    std::vector<SourceTrans> sourceTranslations;
    std::vector<LineTrans> colTranslations;
public:
    /// constructor
    AsmMacro(const AsmSourcePos& pos, const Array<AsmMacroArg>& args);
    
    /// adds line to macro from source
    /**
     * \param source source of line
     * \param colTrans column translations (for backslashes)
     * \param lineSize line size
     * \param line line text (can be with newline character)
     */
    void addLine(RefPtr<const AsmSource> source, const std::vector<LineTrans>& colTrans,
             size_t lineSize, const char* line);
    /// get column translations
    const std::vector<LineTrans>& getColTranslations() const
    { return colTranslations; }
    /// get content vector
    const std::vector<char>& getContent() const
    { return content; }
    /// get source translations size
    size_t getSourceTransSize() const
    { return sourceTranslations.size(); }
    /// get source translations
    const SourceTrans&  getSourceTrans(uint64_t index) const
    { return sourceTranslations[index]; }
    /// get source position
    const AsmSourcePos& getPos() const
    { return pos; }
};

/// assembler repeat
class AsmRepeat
{
public:
    /// source translations
    struct SourceTrans
    {
        uint64_t lineNo;    ///< line number
        RefPtr<const AsmMacroSubst> macro;  ///< macro substitution
        RefPtr<const AsmSource> source;     ///< source
    };
private:
    uint64_t contentLineNo;
    AsmSourcePos pos;
    uint64_t repeatsNum;
    std::vector<char> content;
    std::vector<SourceTrans> sourceTranslations;
    std::vector<LineTrans> colTranslations;
public:
    /// constructor
    explicit AsmRepeat(const AsmSourcePos& pos, uint64_t repeatsNum);
    
    /// adds line to repeat from source
    /**
     * \param macro macro substitution
     * \param source source of line
     * \param colTrans column translations (for backslashes)
     * \param lineSize line size
     * \param line line text (can be with newline character)
     */
    void addLine(RefPtr<const AsmMacroSubst> macro, RefPtr<const AsmSource> source,
             const std::vector<LineTrans>& colTrans, size_t lineSize, const char* line);
    /// get column translations
    const std::vector<LineTrans>& getColTranslations() const
    { return colTranslations; }
    /// get content of repetition
    const std::vector<char>& getContent() const
    { return content; }
    /// get source translations size
    size_t getSourceTransSize() const
    { return sourceTranslations.size(); }
    /// get source translation
    const SourceTrans&  getSourceTrans(uint64_t index) const
    { return sourceTranslations[index]; }
    /// get source position
    const AsmSourcePos& getPos() const
    { return pos; }
    /// get number of repetitions
    uint64_t getRepeatsNum() const
    { return repeatsNum; }
};

/// assembler input filter for reading lines
class AsmInputFilter
{
protected:
    size_t pos;     ///< position in content
    RefPtr<const AsmMacroSubst> macroSubst; ///< current macro substitution
    RefPtr<const AsmSource> source; ///< current source
    std::vector<char> buffer;   ///< buffer of line (can be not used)
    std::vector<LineTrans> colTranslations; ///< column translations
    uint64_t lineNo;    ///< current line number
    
    /// empty constructor
    AsmInputFilter():  pos(0), lineNo(1)
    { }
    /// constructor with macro substitution and source
    AsmInputFilter(RefPtr<const AsmMacroSubst> _macroSubst,
           RefPtr<const AsmSource> _source)
            : pos(0), macroSubst(_macroSubst), source(_source), lineNo(1)
    { }
public:
    /// destructor
    virtual ~AsmInputFilter();
    
    /// read line and returns line except newline character
    virtual const char* readLine(Assembler& assembler, size_t& lineSize) = 0;
    
    /// get current line number before reading line
    uint64_t getLineNo() const
    { return lineNo; }
    
    /// translate position to line number and column number
    /**
     * \param position position in line (from zero)
     * \return line and column
     */
    LineCol translatePos(size_t position) const;
    
    /// returns column translations after reading line
    const std::vector<LineTrans> getColTranslations() const
    { return colTranslations; }
    
    /// get current source before reading line
    RefPtr<const AsmSource> getSource() const
    { return source; }
    /// get current macro substitution before reading line
    RefPtr<const AsmMacroSubst> getMacroSubst() const
    { return macroSubst; }
    
    /// get source position after reading line
    AsmSourcePos getSourcePos(size_t position) const
    {
        LineCol lineCol = translatePos(position);
        return { macroSubst, source, lineCol.lineNo, lineCol.colNo };
    }
};

/// assembler input layout filter
/** filters input from comments and join splitted lines by backslash.
 * readLine returns prepared line which have only space (' ') and
 * non-space characters. */
class AsmStreamInputFilter: public AsmInputFilter
{
private:
    enum class LineMode: cxbyte
    {
        NORMAL = 0,
        LSTRING,
        STRING,
        LONG_COMMENT,
        LINE_COMMENT
    };
    
    bool managed;
    std::istream* stream;
    LineMode mode;
public:
    /// constructor with input stream and their filename
    explicit AsmStreamInputFilter(std::istream& is, const std::string& filename = "");
    /// constructor with input filename
    explicit AsmStreamInputFilter(const std::string& filename);
    /// constructor with source position, input stream and their filename
    AsmStreamInputFilter(const AsmSourcePos& pos, std::istream& is,
             const std::string& filename = "");
    /// constructor with source position and input filename
    AsmStreamInputFilter(const AsmSourcePos& pos, const std::string& filename);
    /// destructor
    ~AsmStreamInputFilter();
    
    const char* readLine(Assembler& assembler, size_t& lineSize);
};

/// assembler macro map
typedef Array<std::pair<std::string, std::string> > AsmMacroArgMap;

/// assembler macro input filter (for macro filtering)
class AsmMacroInputFilter: public AsmInputFilter
{
private:
    const AsmMacro& macro;  ///< input macro
    AsmMacroArgMap argMap;  ///< input macro argument map
    
    uint64_t contentLineNo;
    size_t sourceTransIndex;
    const LineTrans* curColTrans;
public:
    /// constructor with input macro, source position and arguments map
    AsmMacroInputFilter(const AsmMacro& macro, const AsmSourcePos& pos,
        const Array<std::pair<std::string, std::string> >& argMap);
    
    const char* readLine(Assembler& assembler, size_t& lineSize);
};

/// assembler repeat input filter5
class AsmRepeatInputFilter: public AsmInputFilter
{
private:
    const AsmRepeat& repeat;
    uint64_t repeatCount;
    uint64_t contentLineNo;
    size_t sourceTransIndex;
    const LineTrans* curColTrans;
public:
    /// constructor
    explicit AsmRepeatInputFilter(const AsmRepeat& repeat);
    
    const char* readLine(Assembler& assembler, size_t& lineSize);
    
    /// get current repeat count
    uint64_t getRepeatCount() const
    { return repeatCount; }
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
    virtual void resolveCode(cxbyte* location, cxbyte targetType, uint64_t value) = 0;
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
    void resolveCode(cxbyte* location, cxbyte targetType, uint64_t value);
};

/*
 * assembler expressions
 */

/// assembler expression operator
enum class AsmExprOp : cxbyte
{
    ARG_VALUE = 0,  ///< is value not operator
    ARG_SYMBOL = 1,  ///< is value not operator
    ARG_RELSYMBOL = 2, ///< is relative symbol
    NEGATE = 3, ///< negation
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
    FIRST_UNARY = NEGATE,   ///< helper
    LAST_UNARY = PLUS,  ///< helper
    FIRST_BINARY = ADDITION,    ///< helper
    LAST_BINARY = ABOVE_EQ, ///< helper
    NONE = 0xff ///< none operation
};

struct AsmExprTarget;

enum : cxbyte
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
struct AsmExprSymbolOccurence
{
    AsmExpression* expression;      ///< target expression pointer
    size_t argIndex;        ///< argument index
    size_t opIndex;         ///< operator index
    
    /// comparison operator
    bool operator==(const AsmExprSymbolOccurence& b) const
    { return expression==b.expression && opIndex==b.opIndex && argIndex==b.argIndex; }
};

/// assembler symbol structure
struct AsmSymbol
{
    cxuint sectionId;       ///< section id
    bool isDefined;         ///< symbol is defined
    bool onceDefined;       ///< symbol can be only once defined (likes labels)
    bool resolving;         ///< helper
    uint64_t value;         ///< value of symbol
    std::vector<AsmSourcePos> occurrences;  ///< occurrences in source
    AsmExpression* expression;      ///< expression of symbol (if not resolved)

    /** list of occurrences in expressions */
    std::vector<AsmExprSymbolOccurence> occurrencesInExprs;
    
    /// empty constructor
    explicit AsmSymbol(bool _onceDefined = false) : sectionId(ASMSECT_ABS), isDefined(false),
            onceDefined(_onceDefined), resolving(false), value(0), expression(nullptr)
    { }
    /// constructor with expression
    explicit AsmSymbol(AsmExpression* expr, bool _onceDefined = false) :
            sectionId(ASMSECT_ABS), isDefined(false), onceDefined(_onceDefined), 
            resolving(false), value(0), expression(expr)
    { }
    /// constructor with value and section id
    explicit AsmSymbol(cxuint _sectionId, uint64_t _value, bool _onceDefined = false) :
            sectionId(_sectionId), isDefined(true), onceDefined(_onceDefined),
            resolving(false), value(_value), expression(nullptr)
    { }
    /// destructor
    ~AsmSymbol()
    { clearOccurrencesInExpr(); }
    
    /// adds occurrence to list
    void addOccurrence(const AsmSourcePos& pos)
    { occurrences.push_back(pos); }
    /// adds occurrence in expression
    void addOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex)
    { occurrencesInExprs.push_back({expr, argIndex, opIndex}); }
    /// remove occurrence in expression
    void removeOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex);
    /// clear list of occurrences in expression
    void clearOccurrencesInExpr();
};

/// assembler symbol map
typedef std::unordered_map<std::string, AsmSymbol> AsmSymbolMap;
/// assembler symbol entry
typedef AsmSymbolMap::value_type AsmSymbolEntry;

/// target for assembler expression
struct AsmExprTarget
{
    cxbyte type;    ///< type of target
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
class AsmExpression
{
private:
    AsmExprTarget target;
    AsmSourcePos sourcePos;
    size_t symOccursNum;
    bool relativeSymOccurs;
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
public:
    /// constructor of expression (helper)
    AsmExpression(const AsmSourcePos& pos, size_t symOccursNum, bool relativeSymOccurs,
            size_t opsNum, size_t opPosNum, size_t argsNum);
    /// constructor of expression (helper)
    AsmExpression(const AsmSourcePos& pos, size_t symOccursNum, bool relativeSymOccurs,
              size_t opsNum, const AsmExprOp* ops, size_t opPosNum,
              const LineCol* opPos, size_t argsNum, const AsmExprArg* args);
    /// destructor
    ~AsmExpression() = default;
    
    /// set target of expression
    void setTarget(AsmExprTarget _target)
    { target = _target; }
    
    /// try to evaluate expression
    /**
     * \param assembler assembler instace
     * \param value output value
     * \return true if evaluated
     */
    bool evaluate(Assembler& assembler, uint64_t& value, cxuint& sectionId) const;
    
    /// parse expression (helper)
    static AsmExpression* parse(Assembler& assembler, size_t linePos, size_t& outLinePos);
    
    /// parse expression (helper)
    static AsmExpression* parse(Assembler& assembler, const char* string,
              const char*& outend);
    
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
    void substituteOccurrence(AsmExprSymbolOccurence occurrence, uint64_t value);
    /// substitute occurrence in expression by value
    void substituteOccurrence(AsmExprSymbolOccurence occurrence, AsmSymbolEntry* entry);
    /// get operators list
    const Array<AsmExprOp>& getOps() const
    { return ops; }
    /// get argument list
    const AsmExprArg* getArgs() const
    { return args.get(); }
    /// get source position
    const AsmSourcePos& getSourcePos() const
    { return sourcePos; }
};

/// assembler expression argument
union AsmExprArg
{
    AsmSymbolEntry* symbol; ///< if symbol
    uint64_t value;         ///< value
};

inline void AsmExpression::substituteOccurrence(AsmExprSymbolOccurence occurrence,
                        uint64_t value)
{
    ops[occurrence.opIndex] = AsmExprOp::ARG_VALUE;
    args[occurrence.argIndex].value = value;
}

inline void AsmExpression::substituteOccurrence(AsmExprSymbolOccurence occurrence,
                    AsmSymbolEntry* entry)
{
    ops[occurrence.opIndex] = AsmExprOp::ARG_RELSYMBOL;
    args[occurrence.argIndex].symbol = entry;
}

/// assembler section
struct AsmSection
{
    cxuint kernelId;    ///< kernel id (optional)
    AsmSectionType type;        ///< type of section
    std::vector<cxbyte> content;    ///< content of section
};

/// ???
struct AsmCondClause
{
    RefPtr<AsmMacroSubst> macroSubst;
    std::vector<std::pair<AsmSourcePos, uint64_t> > positions;
};

/// main class of assembler
class Assembler
{
public:
    /// defined symbol entry
    typedef std::pair<std::string, uint64_t> DefSym;
    /// macro map type
    typedef std::unordered_map<std::string, AsmMacro> MacroMap;
    /// kernel map type
    typedef std::unordered_map<std::string, cxuint> KernelMap;
private:
    friend class AsmStreamInputFilter;
    friend class AsmMacroInputFilter;
    friend class AsmExpression;
    friend struct AsmPseudoOps; // INTERNAL LOGIC
    AsmFormat format;
    GPUDeviceType deviceType;
    bool _64bit;    ///
    bool good;
    ISAAssembler* isaAssembler;
    std::vector<DefSym> defSyms;
    std::vector<std::string> includeDirs;
    std::vector<AsmSection*> sections;
    AsmSymbolMap symbolMap;
    std::vector<AsmRepeat*> repeats;
    MacroMap macroMap;
    KernelMap kernelMap;
    cxuint flags;
    uint64_t macroCount;
    
    cxuint inclusionLevel;
    cxuint macroSubstLevel;
    
    size_t lineSize;
    const char* line;
    uint64_t lineNo;
    
    std::stack<AsmInputFilter*> asmInputFilters;
    AsmInputFilter* currentInputFilter;
    
    std::ostream& messageStream;
    
    union {
        AmdInput* amdOutput;
        GalliumInput* galliumOutput;
        AsmSection* rawCode;
    };
    
    std::stack<AsmCondClause> condClauses;
    
    bool outFormatInitialized;
    
    bool inGlobal;
    bool inAmdConfig;
    cxuint currentKernel;
    cxuint currentSection;
    uint64_t& currentOutPos;
    
    AsmSourcePos getSourcePos(LineCol lineCol) const
    {
        return { currentInputFilter->getMacroSubst(), currentInputFilter->getSource(),
            lineCol.lineNo, lineCol.colNo };
    }
    
    AsmSourcePos getSourcePos(size_t pos) const
    { return currentInputFilter->getSourcePos(pos); }
    AsmSourcePos getSourcePos(const char* string) const
    { return getSourcePos(string-line); }
    
    void printWarning(const AsmSourcePos& pos, const char* message);
    void printError(const AsmSourcePos& pos, const char* message);
    
    void printWarning(const char* linePlace, const char* message)
    { printWarning(getSourcePos(linePlace), message); }
    void printError(const char* linePlace, const char* message)
    { printError(getSourcePos(linePlace), message); }
    
    void printWarning(LineCol lineCol, const char* message)
    { printWarning(getSourcePos(lineCol), message); }
    void printError(LineCol lineCol, const char* message)
    { printError(getSourcePos(lineCol), message); }
    
    LineCol translatePos(const char* string) const
    { return currentInputFilter->translatePos(string-line); }
    LineCol translatePos(size_t pos) const
    { return currentInputFilter->translatePos(pos); }
    
    bool parseLiteral(uint64_t& value, const char* string, const char*& outend);
    bool parseString(std::string& strarray, const char* string, const char*& outend);
    bool parseSymbol(const char* string, AsmSymbolEntry*& entry, bool localLabel = true);
    
    bool setSymbol(AsmSymbolEntry& symEntry, uint64_t value, cxuint sectionId);
    
    bool assignSymbol(const std::string& symbolName, const char* stringAtSymbol,
                  const char* string);
    
    void initializeOutputFormat();
    
    void putData(size_t size, const cxbyte* data)
    {
        AsmSection& section = *sections[currentSection];
        if (currentOutPos+size > section.content.size())
            section.content.resize(currentOutPos+size);
        ::memcpy(section.content.data() + currentOutPos, data, size);
        currentOutPos += size;
    }
    
    void reserveData(size_t size)
    {
        AsmSection& section = *sections[currentSection];
        if (currentOutPos+size > section.content.size())
            section.content.resize(currentOutPos+size);
        currentOutPos += size;
    }
    
    void printWarningForRange(cxuint bits, uint64_t value, const AsmSourcePos& pos); 
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
     */
    explicit Assembler(const std::string& filename, std::istream& input, cxuint flags = 0,
              AsmFormat format = AsmFormat::CATALYST,
              GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE,
              std::ostream& msgStream = std::cerr);
    /// destructor
    ~Assembler();
    
    /// get GPU device type
    GPUDeviceType getDeviceType() const
    { return deviceType; }
    /// set GPU device type
    void setDeviceType(const GPUDeviceType deviceType)
    { this->deviceType = deviceType; }
    /// get flags
    cxuint getFlags() const
    { return flags; }
    /// set flags
    void setFlags(cxuint flags)
    { this->flags = flags; }
    /// get include directory list
    const std::vector<std::string>& getIncludeDirs() const
    { return includeDirs; }
    /// adds include directory
    void addIncludeDir(const std::string& includeDir);
    /// get symbols map
    const AsmSymbolMap& getSymbolMap() const
    { return symbolMap; }
    
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
