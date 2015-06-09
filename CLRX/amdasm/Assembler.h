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
    
enum class AsmFormat: cxbyte
{
    CATALYST = 0,
    GALLIUM,
    RAWCODE
};

enum: cxuint
{
    ASMSECT_ABS = UINT_MAX
};

enum class AsmSectionType: cxbyte
{
    AMD_GLOBAL_DATA = 0,
    AMD_KERNEL_CODE,
    AMD_KERNEL_DATA,
    AMD_KERNEL_HEADER,
    AMD_KERNEL_METADATA,
    
    GALLIUM_GLOBAL_DATA = 64,
    GALLIUM_COMMENT,
    GALLIUM_DISASSEMBLY,
    GALLIUM_CODE,
    
    RAWCODE_CODE = 128
};

class Assembler;

struct LineCol
{
    uint64_t lineNo;
    size_t colNo;
};

/*
 * assembler source position
 */

enum class AsmSourceType : cxbyte
{
    FILE,
    MACRO,
    REPT
};

struct AsmSource: public FastRefCountable
{
    AsmSourceType type;    ///< type of Asm source (file or macro)
    explicit AsmSource(AsmSourceType _type) : type(_type)
    { }
    virtual ~AsmSource();
};

struct AsmFile: public AsmSource
{
    ///  parent source for this source (for file is parent file or macro substitution,
    /// for macro substitution is parent substitution
    RefPtr<const AsmSource> parent; ///< parent file (or null if root)
    uint64_t lineNo; // place where file is included (0 if root)
    const std::string file; // file path
    
    explicit AsmFile(const std::string& _file) : 
            AsmSource(AsmSourceType::FILE), lineNo(1), file(_file)
    { }
    
    AsmFile(const RefPtr<const AsmSource> _parent, uint64_t _lineNo,
        const std::string& _file) : AsmSource(AsmSourceType::FILE),
        parent(_parent), lineNo(_lineNo), file(_file)
    { }
    
    virtual ~AsmFile();
};

struct AsmMacroSubst: public FastRefCountable
{
    ///  parent source for this source (for file is parent file or macro substitution,
    /// for macro substitution is parent substitution
    RefPtr<const AsmMacroSubst> parent;   ///< parent macro substition
    RefPtr<const AsmSource> source; ///< source of content where macro substituted
    uint64_t lineNo;  ///< place where macro substituted
    
    AsmMacroSubst(RefPtr<const AsmSource> _source, uint64_t _lineNo) :
              source(_source), lineNo(_lineNo)
    { }
    
    AsmMacroSubst(RefPtr<const AsmMacroSubst> _parent, RefPtr<const AsmSource> _source,
              size_t _lineNo) : parent(_parent), source(_source), lineNo(_lineNo)
    { }
};

/// position in macro
struct AsmMacroSource: public AsmSource
{
    RefPtr<const AsmMacroSubst> macro;   ///< macro substition
    RefPtr<const AsmSource> source; ///< source of substituted content
    
    AsmMacroSource(RefPtr<const AsmMacroSubst> _macro, RefPtr<const AsmSource> _source)
                : AsmSource(AsmSourceType::MACRO), macro(_macro), source(_source)
    { }
    
    virtual ~AsmMacroSource();
};

struct AsmRepeatSource: public AsmSource
{
    RefPtr<const AsmSource> source; ///< source of content
    uint64_t repeatCount;
    uint64_t repeatsNum;
    
    AsmRepeatSource(RefPtr<const AsmSource> _source, uint64_t _repeatCount,
            uint64_t _repeatsNum) : AsmSource(AsmSourceType::REPT), source(_source),
            repeatCount(_repeatCount), repeatsNum(_repeatsNum)
    { }
    
    virtual ~AsmRepeatSource();
};

struct AsmSourcePos
{
    RefPtr<const AsmMacroSubst> macro; ///< macro substitution in which message occurred
    RefPtr<const AsmSource> source;   ///< source in which message occurred
    uint64_t lineNo;
    size_t colNo;
    
    void print(std::ostream& os, cxuint indentLevel = 0) const;
};

struct LineTrans
{
    size_t position;
    uint64_t lineNo;
};

struct AsmMacroArg
{
    std::string name;
    std::string defaultValue;
    bool vararg;
    bool required;
};

class AsmMacro
{
public:
    struct SourceTrans
    {
        uint64_t lineNo;
        RefPtr<const AsmSource> source;
    };
private:
    uint64_t contentLineNo;
    AsmSourcePos pos;
    Array<AsmMacroArg> args;
    std::vector<char> content;
    std::vector<SourceTrans> sourceTranslations;
    std::vector<LineTrans> colTranslations;
public:
    AsmMacro(const AsmSourcePos& pos, const Array<AsmMacroArg>& args);
    
    void addLine(RefPtr<const AsmSource> source, const std::vector<LineTrans>& colTrans,
             size_t lineSize, const char* line);
    
    const std::vector<LineTrans>& getColTranslations() const
    { return colTranslations; }
    
    const std::vector<char>& getContent() const
    { return content; }
    
    size_t getSourceTransSize() const
    { return sourceTranslations.size(); }
    const SourceTrans&  getSourceTrans(uint64_t index) const
    { return sourceTranslations[index]; }
    const AsmSourcePos& getPos() const
    { return pos; }
};

class AsmRepeat
{
public:
    struct SourceTrans
    {
        uint64_t lineNo;
        RefPtr<const AsmMacroSubst> macro;
        RefPtr<const AsmSource> source;
    };
private:
    uint64_t contentLineNo;
    AsmSourcePos pos;
    uint64_t repeatsNum;
    std::vector<char> content;
    std::vector<SourceTrans> sourceTranslations;
    std::vector<LineTrans> colTranslations;
public:
    explicit AsmRepeat(const AsmSourcePos& pos, uint64_t repeatsNum);
    
    void addLine(RefPtr<const AsmMacroSubst> macro, RefPtr<const AsmSource> source,
             const std::vector<LineTrans>& colTrans, size_t lineSize, const char* line);
    
    const std::vector<LineTrans>& getColTranslations() const
    { return colTranslations; }
    
    const std::vector<char>& getContent() const
    { return content; }
    
    size_t getSourceTransSize() const
    { return sourceTranslations.size(); }
    const SourceTrans&  getSourceTrans(uint64_t index) const
    { return sourceTranslations[index]; }
    const AsmSourcePos& getPos() const
    { return pos; }
    
    uint64_t getRepeatsNum() const
    { return repeatsNum; }
};

class AsmInputFilter
{
protected:
    size_t pos;
    RefPtr<const AsmMacroSubst> macroSubst;
    RefPtr<const AsmSource> source;
    std::vector<char> buffer;
    std::vector<LineTrans> colTranslations;
    uint64_t lineNo;
    
    AsmInputFilter():  pos(0), lineNo(1)
    { }
    AsmInputFilter(RefPtr<const AsmMacroSubst> _macroSubst,
           RefPtr<const AsmSource> _source)
            : pos(0), macroSubst(_macroSubst), source(_source), lineNo(1)
    { }
public:
    virtual ~AsmInputFilter();
    
    /// read line and returns line except newline character
    virtual const char* readLine(Assembler& assembler, size_t& lineSize) = 0;
    
    /// get current line number before reading line
    uint64_t getLineNo() const
    { return lineNo; }
    
    /// translate position to line number and column number
    /**
     * \param position position in line (from zero)
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
    explicit AsmStreamInputFilter(std::istream& is, const std::string& filename = "");
    explicit AsmStreamInputFilter(const std::string& filename);
    AsmStreamInputFilter(const AsmSourcePos& pos, std::istream& is,
             const std::string& filename = "");
    AsmStreamInputFilter(const AsmSourcePos& pos, const std::string& filename);
    ~AsmStreamInputFilter();
    
    /// read line and returns line except newline character
    const char* readLine(Assembler& assembler, size_t& lineSize);
};

typedef Array<std::pair<std::string, std::string> > AsmMacroArgMap;

class AsmMacroInputFilter: public AsmInputFilter
{
private:
    const AsmMacro& macro;
    AsmMacroArgMap argMap;
    
    uint64_t contentLineNo;
    size_t sourceTransIndex;
    const LineTrans* curColTrans;
public:
    AsmMacroInputFilter(const AsmMacro& macro, const AsmSourcePos& pos,
        const Array<std::pair<std::string, std::string> >& argMap);
    
    /// read line and returns line except newline character
    const char* readLine(Assembler& assembler, size_t& lineSize);
};

class AsmRepeatInputFilter: public AsmInputFilter
{
private:
    const AsmRepeat& repeat;
    uint64_t repeatCount;
    uint64_t contentLineNo;
    size_t sourceTransIndex;
    const LineTrans* curColTrans;
public:
    explicit AsmRepeatInputFilter(const AsmRepeat& repeat);
    
    /// read line and returns line except newline character
    const char* readLine(Assembler& assembler, size_t& lineSize);
    
    uint64_t getRepeatCount() const
    { return repeatCount; }
};

class ISAAssembler
{
protected:
    Assembler& assembler;
    explicit ISAAssembler(Assembler& assembler);
public:
    virtual ~ISAAssembler();
    
    virtual size_t assemble(uint64_t lineNo, const char* line,
                std::vector<cxbyte>& output) = 0;
    
    virtual void resolveCode(cxbyte* location, cxbyte targetType, uint64_t value) = 0;
};

class GCNAssembler: public ISAAssembler
{
public:
    explicit GCNAssembler(Assembler& assembler);
    ~GCNAssembler();
    
    size_t assemble(uint64_t lineNo, const char* line, std::vector<cxbyte>& output);
    void resolveCode(cxbyte* location, cxbyte targetType, uint64_t value);
};

/*
 * assembler expressions
 */

enum class AsmExprOp : cxbyte
{
    ARG_VALUE = 0,  ///< is value not operator
    ARG_SYMBOL = 1,  ///< is value not operator
    NEGATE = 2,
    BIT_NOT,
    LOGICAL_NOT,
    PLUS,
    ADDITION,
    SUBTRACT,
    MULTIPLY,
    DIVISION,
    SIGNED_DIVISION,
    MODULO,
    SIGNED_MODULO,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_ORNOT,
    SHIFT_LEFT,
    SHIFT_RIGHT,
    SIGNED_SHIFT_RIGHT,
    LOGICAL_AND,
    LOGICAL_OR,
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQ,
    GREATER,
    GREATER_EQ,
    BELOW, // unsigned less
    BELOW_EQ, // unsigned less or equal
    ABOVE, // unsigned less
    ABOVE_EQ, // unsigned less or equal
    CHOICE,  ///< a ? b : c
    CHOICE_START,
    FIRST_UNARY = NEGATE,
    LAST_UNARY = PLUS,
    FIRST_BINARY = ADDITION,
    LAST_BINARY = ABOVE_EQ,
    NONE = 0xff
};

struct AsmExprTarget;

enum : cxbyte
{
    ASMXTGT_SYMBOL = 0,
    ASMXTGT_DATA8,
    ASMXTGT_DATA16,
    ASMXTGT_DATA32,
    ASMXTGT_DATA64
};

union AsmExprArg;

struct AsmExpression;

struct AsmExprSymbolOccurence
{
    AsmExpression* expression;
    size_t argIndex;
    size_t opIndex;
    
    bool operator==(const AsmExprSymbolOccurence& b) const
    { return expression==b.expression && opIndex==b.opIndex && argIndex==b.argIndex; }
};

struct AsmSymbol
{
    cxuint sectionId;
    bool isDefined;
    bool onceDefined;
    bool resolving;
    uint64_t value;
    std::vector<AsmSourcePos> occurrences;
    AsmExpression* expression;
    std::vector<AsmExprSymbolOccurence> occurrencesInExprs;
    
    AsmSymbol(bool _onceDefined = false) : sectionId(ASMSECT_ABS), isDefined(false),
            onceDefined(_onceDefined), resolving(false), value(0), expression(nullptr)
    { }
    AsmSymbol(AsmExpression* expr, bool _onceDefined = false) :
            sectionId(ASMSECT_ABS), isDefined(false), onceDefined(_onceDefined), 
            resolving(false), value(0), expression(expr)
    { }
    
    AsmSymbol(cxuint _sectionId, uint64_t _value, bool _onceDefined = false) :
            sectionId(_sectionId), isDefined(true), onceDefined(_onceDefined),
            resolving(false), value(_value), expression(nullptr)
    { }
    ~AsmSymbol()
    { clearOccurrencesInExpr(); }
    
    void addOccurrence(const AsmSourcePos& pos)
    { occurrences.push_back(pos); }
    void addOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex)
    { occurrencesInExprs.push_back({expr, argIndex, opIndex}); }
    void removeOccurrenceInExpr(AsmExpression* expr, size_t argIndex, size_t opIndex);
    
    void clearOccurrencesInExpr();
};

typedef std::unordered_map<std::string, AsmSymbol> AsmSymbolMap;
typedef AsmSymbolMap::value_type AsmSymbolEntry;

struct AsmExprTarget
{
    cxbyte type;
    union
    {
        AsmSymbolEntry* symbol;
        struct {
            cxuint sectionId;
            cxuint size;
            size_t offset;
        };
    };
    
    static AsmExprTarget symbolTarget(AsmSymbolEntry* entry)
    { 
        AsmExprTarget target;
        target.type = ASMXTGT_SYMBOL;
        target.symbol = entry;
        return target;
    }
    
    static AsmExprTarget data32Target(cxuint sectionId, size_t offset)
    {
        AsmExprTarget target;
        target.type = ASMXTGT_DATA32;
        target.sectionId = sectionId;
        target.size = 4;
        target.offset = offset;
        return target;
    }
};

class AsmExpression
{
private:
    AsmExprTarget target;
    AsmSourcePos sourcePos;
    size_t symOccursNum;
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
    AsmExpression(const AsmSourcePos& pos, size_t symOccursNum,
            size_t opsNum, size_t opPosNum, size_t argsNum);
    AsmExpression(const AsmSourcePos& pos, size_t symOccursNum,
              size_t opsNum, const AsmExprOp* ops, size_t opPosNum,
              const LineCol* opPos, size_t argsNum, const AsmExprArg* args);
    ~AsmExpression() = default;
    
    void setTarget(AsmExprTarget _target)
    { target = _target; }
    
    bool evaluate(Assembler& assembler, uint64_t& value) const;
    
    static AsmExpression* parse(Assembler& assembler, size_t linePos, size_t& outLinePos);
    
    static AsmExpression* parse(Assembler& assembler, const char* string,
              const char*& outend);
    
    static bool isUnaryOp(AsmExprOp op)
    { return (AsmExprOp::FIRST_UNARY <= op && op <= AsmExprOp::LAST_UNARY); }
    
    static bool isBinaryOp(AsmExprOp op)
    { return (AsmExprOp::FIRST_BINARY <= op && op <= AsmExprOp::LAST_BINARY); }
    
    const AsmExprTarget& getTarget() const
    { return target; }
    
    size_t getSymOccursNum() const
    { return symOccursNum; }
    
    bool unrefSymOccursNum()
    { return --symOccursNum!=0; }
    
    void substituteOccurrence(AsmExprSymbolOccurence occurrence, uint64_t value);
    
    const Array<AsmExprOp>& getOps() const
    { return ops; }
    const AsmExprArg* getArgs() const
    { return args.get(); }
    
    const AsmSourcePos& getSourcePos() const
    { return sourcePos; }
};

union AsmExprArg
{
    AsmSymbolEntry* symbol;
    uint64_t value;
};

inline void AsmExpression::substituteOccurrence(AsmExprSymbolOccurence occurrence,
                        uint64_t value)
{
    ops[occurrence.opIndex] = AsmExprOp::ARG_VALUE;
    args[occurrence.argIndex].value = value;
}

struct AsmSection
{
    cxuint kernelId;
    AsmSectionType type;
    std::vector<cxbyte> content;
};

struct AsmCondClause
{
    RefPtr<AsmMacroSubst> macroSubst;
    std::vector<std::pair<AsmSourcePos, uint64_t> > positions;
};

class Assembler
{
public:
    typedef std::pair<std::string, uint64_t> DefSym;
    typedef std::unordered_map<std::string, AsmMacro> MacroMap;
    typedef std::unordered_map<std::string, cxuint> KernelMap;
private:
    friend class AsmStreamInputFilter;
    friend class AsmMacroInputFilter;
    friend class AsmExpression;
    AsmFormat format;
    GPUDeviceType deviceType;
    bool _64bit;
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
    
    void includeFile(const std::string& filename);
    void applyMacro(const std::string& macroName, AsmMacroArgMap argMap);
    void exitFromMacro();
    
    bool setSymbol(AsmSymbolEntry& symEntry, uint64_t value);
    
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
    bool readLine();
public:
    explicit Assembler(const std::string& filename, std::istream& input, cxuint flags = 0,
              AsmFormat format = AsmFormat::CATALYST,
              GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE,
              std::ostream& msgStream = std::cerr);
    ~Assembler();
    
    GPUDeviceType getDeviceType() const
    { return deviceType; }
    
    void setDeviceType(const GPUDeviceType deviceType)
    { this->deviceType = deviceType; }
    
    cxuint getFlags() const
    { return flags; }
    void setFlags(cxuint flags)
    { this->flags = flags; }
    
    const std::vector<std::string>& getIncludeDirs() const
    { return includeDirs; }
    
    void addIncludeDir(const std::string& includeDir);
    
    const AsmSymbolMap& getSymbolMap() const
    { return symbolMap; }
    
    void addInitialDefSym(const std::string& symName, uint64_t name);
    
    const AmdInput* getAmdOutput() const
    { return amdOutput; }
    
    const GalliumInput* getGalliumOutput() const
    { return galliumOutput; }
    
    bool assemble();
};

}

#endif
