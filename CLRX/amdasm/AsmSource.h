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
/*! \file AsmSource.h
 * \brief an assembler sources handling
 */

#ifndef __CLRX_ASMSOURCE_H__
#define __CLRX_ASMSOURCE_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <istream>
#include <ostream>
#include <iostream>
#include <vector>
#include <utility>
#include <memory>
#include <CLRX/amdasm/Commons.h>
#include <CLRX/utils/Utilities.h>


/// main namespace
namespace CLRX
{

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
    size_t colNo; ///< place in line where file is included
    const std::string file; ///< file path
    
    /// constructor
    explicit AsmFile(const std::string& _file) : 
            AsmSource(AsmSourceType::FILE), lineNo(1), file(_file)
    { }
    
    /// constructor with parent file inclustion
    AsmFile(const RefPtr<const AsmSource> _parent, uint64_t _lineNo, size_t _colNo,
        const std::string& _file) : AsmSource(AsmSourceType::FILE),
        parent(_parent), lineNo(_lineNo), colNo(_colNo), file(_file)
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
    size_t colNo; ///< place in line where macro substituted
    
    /// constructor
    AsmMacroSubst(RefPtr<const AsmSource> _source, uint64_t _lineNo, size_t _colNo) :
              source(_source), lineNo(_lineNo), colNo(_colNo)
    { }
    /// constructor with parent macro substitution
    AsmMacroSubst(RefPtr<const AsmMacroSubst> _parent, RefPtr<const AsmSource> _source,
              uint64_t _lineNo, size_t _colNo) : parent(_parent), source(_source),
              lineNo(_lineNo), colNo(_colNo)
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
    const AsmSourcePos* exprSourcePos; ///< expression sourcepos from what evaluation made
    
    /// print source position
    void print(std::ostream& os, cxuint indentLevel = 0) const;
};

/// line translations
struct LineTrans
{
    /// position in joined line, can be negative if filtered line is statement
    ssize_t position;
    uint64_t lineNo;    ///< source code line number
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
class AsmMacro: public FastRefCountable
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
    /// constructor with rlvalue for arguments
    AsmMacro(const AsmSourcePos& pos, Array<AsmMacroArg>&& args);
    
    /// adds line to macro from source
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
    /// get number of arguments
    const size_t getArgsNum() const
    { return args.size(); }
    /// get argument
    const AsmMacroArg& getArg(size_t i) const
    { return args[i]; }
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
protected:
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

/// assembler IRP
class AsmIRP: public AsmRepeat
{
private:
    bool irpc; // is irpc
    std::string symbolName;
    Array<std::string> symValues;
public:
    /// constructor
    explicit AsmIRP(const AsmSourcePos& pos, const std::string& symbolName,
               const std::string& symValString);
    /// constructor
    explicit AsmIRP(const AsmSourcePos& pos, const std::string& symbolName,
               const Array<std::string>& symValues);
    /// constructor
    explicit AsmIRP(const AsmSourcePos& pos, const std::string& symbolName,
               Array<std::string>&& symValues);
    /// get number of repetitions
    const std::string& getSymbolName() const
    { return symbolName; }
    /// get symbol value or string
    const std::string& getSymbolValue(size_t i) const
    { return symValues[i]; }
    /// get if IRPC
    bool isIRPC() const
    { return irpc; }
};

enum class AsmInputFilterType
{
    STREAM = 0,
    REPEAT,
    MACROSUBST
};

/// assembler input filter for reading lines
class AsmInputFilter
{
protected:
    AsmInputFilterType type;        ///< input filter type
    size_t pos;     ///< position in content
    RefPtr<const AsmMacroSubst> macroSubst; ///< current macro substitution
    RefPtr<const AsmSource> source; ///< current source
    std::vector<char> buffer;   ///< buffer of line (can be not used)
    std::vector<LineTrans> colTranslations; ///< column translations
    uint64_t lineNo;    ///< current line number
    
    /// empty constructor
    explicit AsmInputFilter(AsmInputFilterType _type):  type(_type), pos(0), lineNo(1)
    { }
    /// constructor with macro substitution and source
    explicit AsmInputFilter(RefPtr<const AsmMacroSubst> _macroSubst,
           RefPtr<const AsmSource> _source, AsmInputFilterType _type)
            : type(_type), pos(0), macroSubst(_macroSubst), source(_source), lineNo(1)
    { }
public:
    /// destructor
    virtual ~AsmInputFilter();
    
    /// read line and returns line except newline character
    virtual const char* readLine(Assembler& assembler, size_t& lineSize) = 0;
    
    /// get current line number after reading line
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
    
    /// get current source after reading line
    RefPtr<const AsmSource> getSource() const
    { return source; }
    /// get current macro substitution after reading line
    RefPtr<const AsmMacroSubst> getMacroSubst() const
    { return macroSubst; }
    
    /// get source position after reading line
    AsmSourcePos getSourcePos(size_t position) const
    {
        LineCol lineCol = translatePos(position);
        return { macroSubst, source, lineCol.lineNo, lineCol.colNo };
    }
    /// get input filter type
    AsmInputFilterType getType() const
    { return type; }
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
    size_t stmtPos;
    
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
public:
    /// macro argument map type
    typedef Array<std::pair<std::string, std::string> > MacroArgMap;
private:
    RefPtr<const AsmMacro> macro;  ///< input macro
    AsmMacroArgMap argMap;  ///< input macro argument map
    
    uint64_t macroCount;
    uint64_t contentLineNo;
    size_t sourceTransIndex;
    const LineTrans* curColTrans;
public:
    /// constructor with input macro, source position and arguments map
    AsmMacroInputFilter(RefPtr<const AsmMacro> macro, const AsmSourcePos& pos,
        const MacroArgMap& argMap, uint64_t macroCount);
    /// constructor with input macro, source position and rvalue of arguments map
    AsmMacroInputFilter(RefPtr<const AsmMacro> macro, const AsmSourcePos& pos,
        MacroArgMap&& argMap, uint64_t macroCount);
    
    const char* readLine(Assembler& assembler, size_t& lineSize);
};

/// assembler repeat input filter5
class AsmRepeatInputFilter: public AsmInputFilter
{
private:
    std::unique_ptr<const AsmRepeat> repeat;
    uint64_t repeatCount;
    uint64_t contentLineNo;
    size_t sourceTransIndex;
    const LineTrans* curColTrans;
public:
    /// constructor
    explicit AsmRepeatInputFilter(const AsmRepeat* repeat);
    
    const char* readLine(Assembler& assembler, size_t& lineSize);
    
    /// get current repeat count
    uint64_t getRepeatCount() const
    { return repeatCount; }
};

class AsmIRPInputFilter: public AsmInputFilter
{
private:
    std::unique_ptr<const AsmIRP> irp;
    uint64_t repeatCount;
    uint64_t contentLineNo;
    size_t sourceTransIndex;
    const LineTrans* curColTrans;
public:
    /// constructor
    explicit AsmIRPInputFilter(const AsmIRP* irp);
    
    const char* readLine(Assembler& assembler, size_t& lineSize);
    
    /// get current repeat count
    uint64_t getRepeatCount() const
    { return repeatCount; }
};

};

#endif
