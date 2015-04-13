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
    GALLIUM
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
    GALLIUM_CODE
};

class Assembler;

/// assembler input layout filter
/** filters input from comments and join splitted lines by backslash.
 * readLine returns prepared line which have only space (' ') and
 * non-space characters. */
class AsmInputFilter
{
public:
    struct LineTrans
    {
        size_t position;
        size_t lineNo;
    };
private:
    Assembler& assembler;
    
    enum class LineMode: cxbyte
    {
        NORMAL = 0,
        LSTRING,
        STRING,
        LONG_COMMENT,
        LINE_COMMENT
    };
    
    std::istream& stream;
    LineMode mode;
    size_t pos;
    std::vector<char> buffer;
    std::vector<LineTrans> colTranslations;
    size_t lineNo;
public:
    explicit AsmInputFilter(Assembler& assembler, std::istream& is);
    
    /// read line and returns line except newline character
    const char* readLine(size_t& lineSize);
    
    /// get current line number before reading line
    size_t getLineNo() const
    { return lineNo; }
    
    /// translate position to line number and column number
    /**
     * \param position position in line (from zero)
     * \param outLineNo output line number
     * \param outColNo output column number
     */
    void translatePos(size_t position, size_t& outLineNo, size_t& outColNo) const;
    
    /// returns column translations
    const std::vector<LineTrans> getColTranslations() const
    { return colTranslations; }
};

class ISAAssembler
{
protected:
    Assembler& assembler;
    explicit ISAAssembler(Assembler& assembler);
    size_t outputSize;
    char* output;
    void reallocateOutput(size_t newSize);
public:
    virtual ~ISAAssembler();
    
    virtual size_t assemble(size_t lineNo, const char* line) = 0;
    virtual void finish() = 0;
    
    size_t getOutputSize() const
    { return outputSize; }
    
    const char* getOutput() const
    { return output; }
};

class GCNAssembler: public ISAAssembler
{
public:
    explicit GCNAssembler(Assembler& assembler);
    ~GCNAssembler();
    
    size_t assemble(size_t lineNo, const char* line);
    void finish();
};

/*
 * assembler source position
 */

struct AsmFile: public RefCountable
{
    RefPtr<const AsmFile> parent; ///< parent file (or null if root)
    size_t lineNo; // place where file is included (0 if root)
    const std::string file; // file path
};

struct AsmMacroSubst: public RefCountable
{
    RefPtr<const AsmMacroSubst> parent;   ///< parent macro (null if global scope)
    RefPtr<const AsmFile> file; ///< file where macro substituted
    size_t lineNo;  ///< place where macro substituted
    std::string macro;  ///< a substituting macro
};

struct AsmSourcePos
{
    RefPtr<const AsmFile> file;   ///< file in which message occurred
    RefPtr<const AsmMacroSubst> macro; ///< macro substitution in which message occurred
    
    size_t lineNo;
    size_t colNo;
    
    void print(std::ostream& os) const;
};

struct AsmMessage
{
    bool error;
    std::string message;
    
    void print(std::ostream& os) const;
};

/*
 * assembler expressions
 */

enum class AsmExprOp : cxbyte
{
    ARG_VALUE = 0,  ///< is value not operator
    ARG_SYMBOL = 1,  ///< is value not operator
    ADDITION = 2,
    SUBTRACT,
    NEGATE,
    MULTIPLY,
    DIVISION,
    SIGNED_DIVISION,
    MODULO,
    SIGNED_MODUL0,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_NOT,
    SHIFT_LEFT,
    SHIFT_RIGHT,
    SIGNED_SHIFT_RIGHT,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    CHOICE,  ///< a ? b : c
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQ,
    GREATER,
    GREATER_EQ
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

struct AsmExpression
{
    size_t symOccursNum;
    AsmExprOp* ops;
    AsmExprArg* args;
    
    ~AsmExpression();
    
    uint64_t operator()() const;
};

struct AsmExprSymbolOccurence
{
    AsmExpression* expression;
    size_t argIndex;
};

struct AsmSymbol
{
    cxuint sectionId;
    bool isDefined;
    uint64_t value;
    std::vector<AsmSourcePos> occurrences;
    AsmExpression resolvingExpression;
    std::vector<std::pair<AsmExprTarget, AsmExprSymbolOccurence> > occurrencesInExprs;
};

union AsmExprArg
{
    AsmSymbol* symbol;
    uint64_t value;
};

struct AsmSection
{
    cxuint kernelId;
    AsmSectionType type;
    std::vector<cxbyte> content;
};

struct AsmExprTarget
{
    cxbyte type;
    union
    {
        AsmSymbol* symbol;
        struct {
            cxuint sectionId;
            cxuint size;
            size_t offset;
        };
    };
};

typedef std::unordered_map<std::string, AsmSymbol> AsmSymbolMap;


class Assembler
{
public:
    typedef std::pair<std::string, uint64_t> DefSym;
    typedef std::unordered_map<std::string, std::string> MacroMap;
    typedef std::unordered_map<std::string, cxuint> KernelMap;
private:
    friend class AsmInputFilter;
    AsmFormat format;
    GPUDeviceType deviceType;
    ISAAssembler* isaAssembler;
    std::vector<DefSym> defSyms;
    std::vector<std::string> includeDirs;
    std::vector<AsmSection> sections;
    std::vector<char> symNames;
    AsmSymbolMap symbolMap;
    MacroMap macroMap;
    KernelMap kernelMap;
    std::vector<AsmExpression*> pendingExpressions;
    cxuint flags;
    
    cxuint inclusionLevel;
    cxuint macroSubstLevel;
    RefPtr<AsmFile> topInclusion;
    RefPtr<AsmMacroSubst> topMacroSubst;
    
    union {
        AmdInput* amdOutput;
        GalliumInput* galliumOutput;
    };
    
    std::vector<std::pair<AsmSourcePos, AsmMessage> > messages;
    
    bool inGlobal;
    bool isInAmdConfig;
    cxuint currentKernel;
    cxuint currentSection;
    
    void addWarning(size_t lineNo, size_t colNo, const std::string& message);
    void addError(size_t lineNo, size_t colNo, const std::string& message);
public:
    explicit Assembler(std::istream& input, cxuint flags);
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
    
    AsmExpression* parseExpression(size_t lineNo, size_t colNo, size_t stringSize,
             const char* string, uint64_t& outValue) const;
    
    const AmdInput* getAmdOutput() const
    { return amdOutput; }
    
    const GalliumInput* getGalliumOutput() const
    { return galliumOutput; }
    
    void assemble(size_t inputSize, const char* inputString,
                  std::ostream& msgStream = std::cerr);
    void assemble(std::istream& inputStream, std::ostream& msgStream = std::cerr);
};

}

#endif
