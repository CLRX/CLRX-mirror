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

#include <CLRX/Config.h>
#include <elf.h>
#include <string>
#include <cstring>
#include <cassert>
#include <fstream>
#include <vector>
#include <stack>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/amdasm/Assembler.h>
#include "AsmInternals.h"

using namespace CLRX;

static const char* offlinePseudoOpNamesTbl[] =
{
    "else", "elseif", "elseifb", "elseifc", "elseifdef",
    "elseifeq", "elseifeqs", "elseifge", "elseifgt",
    "elseifle", "elseiflt", "elseifnb", "elseifnc",
    "elseifndef", "elseifne", "elseifnes", "elseifnotdef",
    "endif", "endm", "endr",
    "if", "ifb", "ifc", "ifdef", "ifeq",
    "ifeqs", "ifge", "ifgt", "ifle",
    "iflt", "ifnb", "ifnc", "ifndef",
    "ifne", "ifnes", "ifnotdef", "macro", "rept"
};

enum
{
    ASMCOP_ELSE, ASMCOP_ELSEIF, ASMCOP_ELSEIFB, ASMCOP_ELSEIFC, ASMCOP_ELSEIFDEF,
    ASMCOP_ELSEIFEQ, ASMCOP_ELSEIFEQS, ASMCOP_ELSEIFGE, ASMCOP_ELSEIFGT,
    ASMCOP_ELSEIFLE, ASMCOP_ELSEIFLT, ASMCOP_ELSEIFNB, ASMCOP_ELSEIFNC,
    ASMCOP_ELSEIFNDEF, ASMCOP_ELSEIFNE, ASMCOP_ELSEIFNES, ASMCOP_ELSEIFNOTDEF,
    ASMCOP_ENDIF, ASMCOP_ENDM, ASMCOP_ENDR,
    ASMCOP_IF, ASMCOP_IFB, ASMCOP_IFC, ASMCOP_IFDEF, ASMCOP_IFEQ,
    ASMCOP_IFEQS, ASMCOP_IFGE, ASMCOP_IFGT, ASMCOP_IFLE,
    ASMCOP_IFLT, ASMCOP_IFNB, ASMCOP_IFNC, ASMCOP_IFNDEF,
    ASMCOP_IFNE, ASMCOP_IFNES, ASMCOP_IFNOTDEF, ASMCOP_MACRO, ASMCOP_REPT
};

static const char* pseudoOpNamesTbl[] =
{
    "32bit", "64bit", "abort", "align",
    "amd", "arch", "ascii", "asciz",
    "balign", "balignl", "balignw", "byte",
    "config", "data", "double", "else",
    "elseif", "elseifb", "elseifc", "elseifdef",
    "elseifeq", "elseifeqs", "elseifge", "elseifgt",
    "elseifle", "elseiflt", "elseifnb", "elseifnc",
    "elseifndef", "elseifne", "elseifnes", "elseifnotdef",
    "end", "endif", "endm",
    "endr", "equ", "equiv", "eqv",
    "err", "error", "exitm", "extern",
    "fail", "file", "fill", "fillq",
    "float", "format", "gallium", "global",
    "gpu", "half", "hword", "if",
    "ifb", "ifc", "ifdef", "ifeq",
    "ifeqs", "ifge", "ifgt", "ifle",
    "iflt", "ifnb", "ifnc", "ifndef",
    "ifne", "ifnes", "ifnotdef", "incbin",
    "include", "int", "kernel", "line",
    "ln", "loc", "local", "long",
    "macro", "octa", "offset", "org",
    "p2align", "print", "purgem", "quad",
    "rawcode", "rept", "section", "set",
    "short", "single", "size", "skip",
    "space", "string", "string16", "string32",
    "string64", "struct", "text", "title",
    "warning", "weak", "word"
};

enum
{
    ASMOP_32BIT = 0, ASMOP_64BIT, ASMOP_ABORT, ASMOP_ALIGN,
    ASMOP_AMD, ASMOP_ARCH, ASMOP_ASCII, ASMOP_ASCIZ,
    ASMOP_BALIGN, ASMOP_BALIGNL, ASMOP_BALIGNW, ASMOP_BYTE,
    ASMOP_CONFIG, ASMOP_DATA, ASMOP_DOUBLE, ASMOP_ELSE,
    ASMOP_ELSEIF, ASMOP_ELSEIFB, ASMOP_ELSEIFC, ASMOP_ELSEIFDEF,
    ASMOP_ELSEIFEQ, ASMOP_ELSEIFEQS, ASMOP_ELSEIFGE, ASMOP_ELSEIFGT,
    ASMOP_ELSEIFLE, ASMOP_ELSEIFLT, ASMOP_ELSEIFNB, ASMOP_ELSEIFNC,
    ASMOP_ELSEIFNDEF, ASMOP_ELSEIFNE, ASMOP_ELSEIFNES, ASMOP_ELSEIFNOTDEF,
    ASMOP_END, ASMOP_ENDIF, ASMOP_ENDM,
    ASMOP_ENDR, ASMOP_EQU, ASMOP_EQUIV, ASMOP_EQV,
    ASMOP_ERR, ASMOP_ERROR, ASMOP_EXITM, ASMOP_EXTERN,
    ASMOP_FAIL, ASMOP_FILE, ASMOP_FILL, ASMOP_FILLQ,
    ASMOP_FLOAT, ASMOP_FORMAT, ASMOP_GALLIUM, ASMOP_GLOBAL,
    ASMOP_GPU, ASMOP_HALF, ASMOP_HWORD, ASMOP_IF,
    ASMOP_IFB, ASMOP_IFC, ASMOP_IFDEF, ASMOP_IFEQ,
    ASMOP_IFEQS, ASMOP_IFGE, ASMOP_IFGT, ASMOP_IFLE,
    ASMOP_IFLT, ASMOP_IFNB, ASMOP_IFNC, ASMOP_IFNDEF,
    ASMOP_IFNE, ASMOP_IFNES, ASMOP_IFNOTDEF, ASMOP_INCBIN,
    ASMOP_INCLUDE, ASMOP_INT, ASMOP_KERNEL, ASMOP_LINE,
    ASMOP_LN, ASMOP_LOC, ASMOP_LOCAL, ASMOP_LONG,
    ASMOP_MACRO, ASMOP_OCTA, ASMOP_OFFSET, ASMOP_ORG,
    ASMOP_P2ALIGN, ASMOP_PRINT, ASMOP_PURGEM, ASMOP_QUAD,
    ASMOP_RAWCODE, ASMOP_REPT, ASMOP_SECTION, ASMOP_SET,
    ASMOP_SHORT, ASMOP_SINGLE, ASMOP_SIZE, ASMOP_SKIP,
    ASMOP_SPACE, ASMOP_STRING, ASMOP_STRING16, ASMOP_STRING32,
    ASMOP_STRING64, ASMOP_STRUCT, ASMOP_TEXT, ASMOP_TITLE,
    ASMOP_WARNING, ASMOP_WEAK, ASMOP_WORD
};

namespace CLRX
{

enum class IfIntComp
{
    EQUAL = 0,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL
};
    

struct CLRX_INTERNAL AsmPseudoOps
{
    /* IMPORTANT:
     * about string argumenbt - string points to place of current line
     * processed by assembler
     * pseudoOpStr - points to first character from line of pseudo-op name
     */
    
    static bool checkGarbagesAtEnd(Assembler& asmr, const char* string);
    /* parsing helpers */
    /* get absolute value arg resolved at this time.
       if empty expression value is not set */
    static bool getAbsoluteValueArg(Assembler& asmr, uint64_t& value, const char*& string,
                    bool requredExpr = false);
    
    static bool getAnyValueArg(Assembler& asmr, uint64_t& value, cxuint& sectionId,
                    const char*& string);
    // get name (not symbol name)
    static bool getNameArg(Assembler& asmr, std::string& outStr, const char*& string,
               const char* objName);
    // skip comma
    static bool skipComma(Assembler& asmr, bool& haveComma, const char*& string);
    
    // skip comma for multiple argument pseudo-ops
    static bool skipCommaForMultipleArgd(Assembler& asmr, const char*& string);
    
    /*
     * pseudo-ops logic
     */
    // set bitnesss
    static void setBitness(Assembler& asmr, const char*& string, bool _64Bit);
    // set output format
    static void setOutFormat(Assembler& asmr, const char*& string);
    // change kernel
    static void goToKernel(Assembler& asmr, const char*& string);
    
    /// include file
    static void includeFile(Assembler& asmr, const char* pseudoOpStr, const char*& string);
    // include binary file
    static void includeBinFile(Assembler& asmr, const char*& string);
    
    // fail
    static void doFail(Assembler& asmr, const char* pseudoOpStr, const char*& string);
    // .error
    static void printError(Assembler& asmr, const char* pseudoOpStr, const char*& string);
    // .warning
    static void printWarning(Assembler& asmr, const char* pseudoOpStr, const char*& string);
    
    // .byte, .short, .int, .word, .long, .quad
    template<typename T>
    static void putIntegers(Assembler& asmr, const char*& string);
    
    // .half, .float, .double
    template<typename UIntType>
    static void putFloats(Assembler& asmr, const char*& string);
    
    /// .string, ascii
    static void putStrings(Assembler& asmr, const char*& string, bool addZero = false);
    // .string16, .string32, .string64
    template<typename T>
    static void putStringsToInts(Assembler& asmr, const char*& string);
    
    /// .octa
    static void putUInt128s(Assembler& asmr, const char*& string);
    
    /// .set, .equ, .eqv, .equiv
    static void setSymbol(Assembler& asmr, const char*& string, bool reassign = true,
                bool baseExpr = false);
    
    // .global, .local, .extern
    static void setSymbolBind(Assembler& asmr, const char*& string, cxbyte elfInfo);
    
    static void setSymbolSize(Assembler& asmr, const char*& string);
    
    static void ignoreExtern(Assembler& asmr, const char*& string);
    
    static void doFill(Assembler& asmr, const char* pseudoOpStr, const char*& string,
               bool _64bit = false);
    static void doSkip(Assembler& asmr, const char*& string);
    
    /* TODO: add no-op fillin for text sections */
    static void doAlign(Assembler& asmr,  const char*& string, bool powerOf2 = false);
    
    /* TODO: add no-op fillin for text sections */
    template<typename Word>
    static void doAlignWord(Assembler& asmr, const char* pseudoOpStr, const char*& string);
    
    static void doOrganize(Assembler& asmr, const char*& string);
    
    static void doIfInt(Assembler& asmr, const char* pseudoOpStr, const char*& string,
                IfIntComp compType, bool elseIfClause);
    
    static void doIfDef(Assembler& asmr, const char* pseudoOpStr, const char*& string,
                bool negation, bool elseIfClause);
    
    static void doIfBlank(Assembler& asmr, const char* pseudoOpStr, const char*& string,
                bool negation, bool elseIfClause);
    /// .ifc
    static void doIfCmpStr(Assembler& asmr, const char* pseudoOpStr, const char*& string,
                bool negation, bool elseIfClause);
    /// ifeqs, ifnes
    static void doIfStrEqual(Assembler& asmr, const char* pseudoOpStr, const char*& string,
                bool negation, bool elseIfClause);
    
    static void doElse(Assembler& asmr, const char* pseudoOpStr, const char*& string);
    
    static void doEndIf(Assembler& asmr, const char* pseudoOpStr, const char*& string);
};


bool AsmPseudoOps::checkGarbagesAtEnd(Assembler& asmr, const char* string)
{
    string = skipSpacesToEnd(string, asmr.line + asmr.lineSize);
    if (string != asmr.line + asmr.lineSize)
    {
        asmr.printError(string, "Garbages at end of line");
        return false;
    }
    return true;
}

bool AsmPseudoOps::getAbsoluteValueArg(Assembler& asmr, uint64_t& value,
                      const char*& string, bool requiredExpr)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* exprStr = string;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, string, string,
                 false, true));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty() && requiredExpr)
    {
        asmr.printError(exprStr, "Expected expression");
        return false;
    }
    if (expr->isEmpty()) // do not set if empty expression
        return true;
    cxuint sectionId; // for getting
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    else if (sectionId != ASMSECT_ABS)
    {   // if not absolute value
        asmr.printError(exprStr, "Expression must be absolute!");
        return false;
    }
    return true;
}

bool AsmPseudoOps::getAnyValueArg(Assembler& asmr, uint64_t& value,
                   cxuint& sectionId, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* exprStr = string;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, string, string,
                 false, true));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty())
    {
        asmr.printError(exprStr, "Expected expression");
        return false;
    }
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    return true;
}

bool AsmPseudoOps::getNameArg(Assembler& asmr, std::string& outStr, const char*& string,
            const char* objName)
{
    const char* end = asmr.line + asmr.lineSize;
    outStr.clear();
    string = skipSpacesToEnd(string, end);
    if (string == end)
    {
        std::string error("Expected");
        error += objName;
        asmr.printError(string, error.c_str());
        return false;
    }
    const char* nameStr = string;
    if (isAlpha(*string) || *string == '_')
    {
        string++;
        while (string != end && (isAlnum(*string) || *string == '_')) string++;
    }
    else
    {
        asmr.printError(string, "Some garbages at name place");
        return false;
    }
    outStr.assign(nameStr, string);
    return true;
}

inline bool AsmPseudoOps::skipComma(Assembler& asmr, bool& haveComma, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string == end)
    {
        haveComma = false;
        return true;
    }
    if (*string != ',')
    {
        asmr.printError(string, "Expected ',' before argument");
        return false;
    }
    string++;
    haveComma = true;
    return true;
}

bool AsmPseudoOps::skipCommaForMultipleArgd(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end); // spaces before ','
    if (string == end)
        return false;
    if (*string != ',')
    {
        asmr.printError(string, "Expected ',' before next value");
        string++;
        return false;
    }
    else
        string = skipSpacesToEnd(string+1, end);
    return true;
}

void AsmPseudoOps::setBitness(Assembler& asmr, const char*& string, bool _64Bit)
{
    if (!checkGarbagesAtEnd(asmr, string))
        return;
    if (asmr.outFormatInitialized)
        asmr.printError(string, "Bitness is already defined");
    else if (asmr.format != BinaryFormat::AMD)
        asmr.printWarning(string, "Bitness ignored for other formats than AMD Catalyst");
    else
        asmr._64bit = (_64Bit);
    
}

void AsmPseudoOps::setOutFormat(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    std::string formatName;
    if (!getNameArg(asmr, formatName, string, "output format type"))
        return;
    
    toLowerString(formatName);
    if (formatName == "catalyst" || formatName == "amd")
        asmr.format = BinaryFormat::AMD;
    else if (formatName == "gallium")
        asmr.format = BinaryFormat::GALLIUM;
    else if (formatName == "raw")
        asmr.format = BinaryFormat::RAWCODE;
    else
        asmr.printError(string, "Unknown output format type");
    if (asmr.outFormatInitialized)
        asmr.printError(string, "Output format type is already defined");
    
    checkGarbagesAtEnd(asmr, string);
}

void AsmPseudoOps::goToKernel(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    if (asmr.format == BinaryFormat::AMD || asmr.format == BinaryFormat::GALLIUM)
    {
        string = skipSpacesToEnd(string, end);
        std::string kernelName;
        if (!getNameArg(asmr, kernelName, string, "kernel name"))
            return;
        if (!checkGarbagesAtEnd(asmr, string))
            return;
        
        if (asmr.format == BinaryFormat::AMD)
        {
            asmr.kernelMap.insert(std::make_pair(kernelName,
                            asmr.amdOutput->kernels.size()));
            asmr.amdOutput->addEmptyKernel(kernelName.c_str());
        }
        else
        {
            asmr.kernelMap.insert(std::make_pair(kernelName,
                        asmr.galliumOutput->kernels.size()));
            //galliumOutput->addEmptyKernel(kernelName.c_str());
        }
    }
    else if (asmr.format == BinaryFormat::RAWCODE)
        asmr.printError(string, "Raw code can have only one unnamed kernel");
}

void AsmPseudoOps::includeFile(Assembler& asmr, const char* pseudoOpStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    std::string filename;
    const char* nameStr = string;
    if (asmr.parseString(filename, string, string))
    {
        if (!checkGarbagesAtEnd(asmr, string))
            return;
        
        bool failedOpen = false;
        filesystemPath(filename);
        try
        {
            std::unique_ptr<AsmInputFilter> newInputFilter(new AsmStreamInputFilter(
                    asmr.getSourcePos(pseudoOpStr), filename));
            asmr.asmInputFilters.push(newInputFilter.release());
            asmr.currentInputFilter = asmr.asmInputFilters.top();
            return;
        }
        catch(const Exception& ex)
        { failedOpen = true; }
        
        for (const std::string& incDir: asmr.includeDirs)
        {
            failedOpen = false;
            std::string inDirFilename;
            try
            {
                inDirFilename = joinPaths(incDir, filename);
                std::unique_ptr<AsmInputFilter> newInputFilter(new AsmStreamInputFilter(
                        asmr.getSourcePos(pseudoOpStr), inDirFilename));
                asmr.asmInputFilters.push(newInputFilter.release());
                asmr.currentInputFilter = asmr.asmInputFilters.top();
                break;
            }
            catch(const Exception& ex)
            { failedOpen = true; }
        }
        if (failedOpen)
        {
            std::string error("Include file '");
            error += filename;
            error += "' not found or unavailable in any directory";
            asmr.printError(nameStr, error.c_str());
        }
    }
}

void AsmPseudoOps::includeBinFile(Assembler& asmr, const char*& string)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    std::string filename;
    const char* nameStr = string;
    const char* offsetStr = string;
    const char* countStr = string;
    uint64_t offset = 0, count = INT64_MAX;
    bool good = asmr.parseString(filename, string, string);
    bool haveComma;
    
    if (!skipComma(asmr, haveComma, string))
        return;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        offsetStr = string;
        good &= getAbsoluteValueArg(asmr, offset, string);
        if (int64_t(offset) < 0)
        {
            asmr.printError(offsetStr, "Offset is negative!");
            good = false;
        }
        
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            countStr = string;
            good &= getAbsoluteValueArg(asmr, count, string);
            if (int64_t(count) < 0)
            {
                asmr.printError(countStr, "Count bytes is negative!");
                good = false;
            }
        }
    }
    if (count == 0)
    {
        asmr.printWarning(nameStr, "Number of bytes is zero, ignoring .incbin");
        return;
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, string)) // failed parsing
        return;
    
    std::ifstream ifs;
    filesystemPath(filename);
    ifs.open(filename.c_str(), std::ios::binary);
    if (!ifs)
    {
        for (const std::string& incDir: asmr.includeDirs)
        {
            std::string inDirFilename = joinPaths(incDir, filename);
            ifs.open(inDirFilename.c_str(), std::ios::binary);
            if (ifs)
                break;
        }
    }
    if (!ifs)
    {
        std::string error("Binary file '");
        error += filename;
        error += "' not found or unavailable in any directory";
        asmr.printError(nameStr, error.c_str());
    }
    // exception for checking file seeking
    bool seekingIsWorking = true;
    ifs.exceptions(std::ios::badbit | std::ios::failbit); // exceptions
    try
    { ifs.seekg(0, std::ios::end); /* to end of file */ }
    catch(const std::exception& ex)
    {   /* oh, no! this is not regular file */
        seekingIsWorking = false;
        ifs.clear();
    }
    ifs.exceptions(std::ios::badbit);  // exceptions for reading
    if (seekingIsWorking)
    {   /* for regular files */
        const uint64_t size = ifs.tellg();
        if (size < offset)
            return; // do nothing
        ifs.seekg(offset, std::ios::beg);
        const uint64_t toRead = std::min(size-offset, count);
        char* output = reinterpret_cast<char*>(asmr.reserveData(toRead));
        ifs.read(output, toRead);
        if (ifs.gcount() != std::streamsize(toRead))
        {
            asmr.printError(nameStr, "Can't read whole needed file content");
            return;
        }
    }
    else
    {   /* for sequential files, likes fifo */
        char tempBuf[256];
        for (uint64_t pos = 0; pos < offset; )
        {
            const size_t toRead = std::min(offset-pos, uint64_t(256));
            ifs.read(tempBuf, toRead);
            const uint64_t readed = ifs.gcount();
            pos += readed;
            if (readed < toRead)
                break;
        }
        // read data from binary file
        for (uint64_t bytes = 0; bytes < count; )
        {
            const size_t toRead = std::min(uint64_t(256), count-bytes);
            ifs.read(tempBuf, toRead);
            const uint64_t readed = ifs.gcount();
            asmr.putData(readed, (cxbyte*)tempBuf);
            bytes += readed;
            if (readed < toRead)
                break;
        }
    }
}

void AsmPseudoOps::doFail(Assembler& asmr, const char* pseudoOpStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t value = 0;
    if (!getAbsoluteValueArg(asmr, value, string, true))
        return;
    if (!checkGarbagesAtEnd(asmr, string))
        return;
    
    char buf[50];
    ::memcpy(buf, ".fail ", 6);
    const size_t pos = 6+itocstrCStyle(int64_t(value), buf+6, 50-6);
    ::memcpy(buf+pos, " encountered", 13);
    if (int64_t(value) >= 500)
        asmr.printWarning(pseudoOpStr, buf);
    else
        asmr.printError(pseudoOpStr, buf);
}

void AsmPseudoOps::printError(Assembler& asmr, const char* pseudoOpStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, string, string))
            return; // error
        if (!checkGarbagesAtEnd(asmr, string))
            return;
        asmr.printError(pseudoOpStr, outStr.c_str());
    }
    else
        asmr.printError(pseudoOpStr, ".error encountered");
}

void AsmPseudoOps::printWarning(Assembler& asmr, const char* pseudoOpStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, string, string))
            return; // error
        if (!checkGarbagesAtEnd(asmr, string))
            return;
        asmr.printWarning(pseudoOpStr, outStr.c_str());
    }
    else
        asmr.printWarning(pseudoOpStr, ".warning encountered");
}

template<typename T>
void AsmPseudoOps::putIntegers(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    do {
        const char* exprStr = string;
        std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, string, string));
        if (expr)
        {
            if (expr->isEmpty()) // empty expression print warning
                asmr.printWarning(string, "No expression, zero has been put");
            
            if (expr->getSymOccursNum()==0)
            {   // put directly to section
                uint64_t value;
                cxuint sectionId;
                if (expr->evaluate(asmr, value, sectionId))
                {
                    if (sectionId == ASMSECT_ABS)
                    {
                        if (sizeof(T) < 8)
                            asmr.printWarningForRange(sizeof(T)<<3, value,
                                         asmr.getSourcePos(exprStr));
                        T out;
                        SLEV(out, value);
                        asmr.putData(sizeof(T), reinterpret_cast<const cxbyte*>(&out));
                    }
                    else
                        asmr.printError(exprStr, "Expression must be absolute!");
                }
            }
            else // expression
            {
                expr->setTarget(AsmExprTarget::dataTarget<T>(
                                asmr.currentSection, asmr.currentOutPos));
                expr.release();
                asmr.reserveData(sizeof(T));
            }
        }
    } while(skipCommaForMultipleArgd(asmr, string));
    checkGarbagesAtEnd(asmr, string);
}

template<typename T> inline
T asmcstrtofCStyleLEV(const char* str, const char* inend, const char*& outend);

template<> inline
uint16_t asmcstrtofCStyleLEV<uint16_t>(const char* str, const char* inend,
                   const char*& outend)
{ return LEV(cstrtohCStyle(str, inend, outend)); }

template<> inline
uint32_t asmcstrtofCStyleLEV<uint32_t>(const char* str, const char* inend,
                   const char*& outend)
{
    union {
        float f;
        uint32_t u;
    } value;
    value.f = cstrtovCStyle<float>(str, inend, outend);
    return LEV(value.u);
}

template<> inline
uint64_t asmcstrtofCStyleLEV<uint64_t>(const char* str, const char* inend,
                   const char*& outend)
{
    union {
        double f;
        uint64_t u;
    } value;
    value.f = cstrtovCStyle<double>(str, inend, outend);
    return LEV(value.u);
}

template<typename UIntType>
void AsmPseudoOps::putFloats(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    do {
        UIntType out = 0;
        const char* literalString = string;
        if (string != end && *string != ',')
        {
            try
            { out = asmcstrtofCStyleLEV<UIntType>(string, end, string); }
            catch(const ParseException& ex)
            { asmr.printError(literalString, ex.what()); }
        }
        else // warning
            asmr.printWarning(literalString,
                      "No floating point literal, zero has been put");
        asmr.putData(sizeof(UIntType), reinterpret_cast<const cxbyte*>(&out));
        
    } while (skipCommaForMultipleArgd(asmr, string));
    checkGarbagesAtEnd(asmr, string);
}

void AsmPseudoOps::putUInt128s(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    do {
        UInt128 value = { 0, 0 };
        if (string != end && *string != ',')
        {
            const char* literalString = string;
            bool negative = false;
            if (*string == '+')
                string++;
            else if (*string == '-')
            {
                negative = true;
                string++;
            }
            try
            { value = cstrtou128CStyle(string, end, string); }
            catch(const ParseException& ex)
            { asmr.printError(literalString, ex.what()); }
            if (negative)
            {
                value.hi = ~value.hi + (value.lo==0);
                value.lo = -value.lo;
            }
        }
        else // warning
            asmr.printWarning(string, "No 128-bit literal, zero has been put");
        UInt128 out;
        SLEV(out.lo, value.lo);
        SLEV(out.hi, value.hi);
        asmr.putData(16, reinterpret_cast<const cxbyte*>(&out));
    } while (skipCommaForMultipleArgd(asmr, string));
    checkGarbagesAtEnd(asmr, string);
}

void AsmPseudoOps::putStrings(Assembler& asmr, const char*& string, bool addZero)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    do {
        std::string outStr;
        if (*string != ',')
        {
            if (asmr.parseString(outStr, string, string))
                asmr.putData(outStr.size()+(addZero), (const cxbyte*)outStr.c_str());
        }
    } while (skipCommaForMultipleArgd(asmr, string));
    checkGarbagesAtEnd(asmr, string);
}

template<typename T>
void AsmPseudoOps::putStringsToInts(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    do {
        std::string outStr;
        if (*string != ',')
        {
            if (asmr.parseString(outStr, string, string))
            {
                const size_t strSize = outStr.size()+1;
                T* outData = reinterpret_cast<T*>(
                        asmr.reserveData(sizeof(T)*(strSize)));
                for (size_t i = 0; i < strSize; i++)
                    SULEV(outData[i], T(outStr[i])&T(0xff));
            }
        }
        
    } while (skipCommaForMultipleArgd(asmr, string));
    checkGarbagesAtEnd(asmr, string);
}

void AsmPseudoOps::setSymbol(Assembler& asmr, const char*& string, bool reassign,
                 bool baseExpr)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* strAtSymName = string;
    std::string symName = extractSymName(string, end, false);
    bool good = true;
    if (symName.empty())
    {
        asmr.printError(string, "Expected symbol");
        good = false;
    }
    string += symName.size();
    bool haveComma;
    if (!skipComma(asmr, haveComma, string))
        return;
    if (!haveComma)
    {
        asmr.printError(string, "Expected expression");
        return;
    }
    if (good) // is not so good
        asmr.assignSymbol(symName, strAtSymName, string, reassign, baseExpr);
}

void AsmPseudoOps::setSymbolBind(Assembler& asmr, const char*& string, cxbyte bind)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    do {
        const char* symNameStr = string;
        AsmSymbolEntry* symEntry;
        Assembler::ParseState state = asmr.parseSymbol(string, string, symEntry, false);
        bool good = (state != Assembler::ParseState::FAILED);
        if (symEntry == nullptr)
        {
            asmr.printError(symNameStr, "Expected symbol name");
            good = false;
        }
        else if (symEntry->second.base)
        {
            asmr.printError(symNameStr,
                "Symbol must not be set by .eqv pseudo-op or must be constant");
            good = false;
        }
        
        if (good)
        {
            if (symEntry->first != ".")
                symEntry->second.info = ELF32_ST_INFO(bind,
                      ELF32_ST_TYPE(symEntry->second.info));
            else
                asmr.printWarning(symNameStr, "Symbol '.' is ignored");
        }
        
    } while(skipCommaForMultipleArgd(asmr, string));
    checkGarbagesAtEnd(asmr, string);
}

void AsmPseudoOps::setSymbolSize(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* symNameStr = string;
    AsmSymbolEntry* symEntry;
    Assembler::ParseState state = asmr.parseSymbol(string, string, symEntry, false);
    bool good = (state != Assembler::ParseState::FAILED);
    if (symEntry == nullptr)
    {
        asmr.printError(symNameStr, "Expected symbol name");
        good = false;
    }
    bool haveComma;
    if (!skipComma(asmr, haveComma, string))
        return;
    if (!haveComma)
    {
        asmr.printError(symNameStr, "Expected expression");
        return;
    }
    // parse size
    uint64_t size;
    good &= getAbsoluteValueArg(asmr, size, string, true);
    if (symEntry != nullptr)
    {
        if (symEntry->second.base)
        {
            asmr.printError(symNameStr,
                    "Symbol must not be set by .eqv pseudo-op or must be constant");
            good = false;
        }
        else if (symEntry->first == ".")
        {
            asmr.printWarning(symNameStr, "Symbol '.' is ignored");
            return;
        }
    }
    
    if (good && checkGarbagesAtEnd(asmr, string))
        symEntry->second.size = size;
}

void AsmPseudoOps::ignoreExtern(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    do {
        asmr.skipSymbol(string, string);
    } while (skipCommaForMultipleArgd(asmr, string));
    checkGarbagesAtEnd(asmr, string);
}

void AsmPseudoOps::doFill(Assembler& asmr, const char* pseudoOpStr, const char*& string,
          bool _64bit)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t repeat = 0, size = 1, value = 0;
    
    const char* reptStr = string;
    bool good = getAbsoluteValueArg(asmr, repeat, string, true);
    
    if (int64_t(repeat) < 0)
        asmr.printWarning(reptStr, "Negative repeat has no effect");
    
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* sizeStr = string;
    const char* fillValStr = string;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        sizeStr = string; //
        good &= getAbsoluteValueArg(asmr, size, string);
        
        if (int64_t(size) < 0)
            asmr.printWarning(sizeStr, "Negative size has no effect");
        
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            fillValStr = string;
            good &= getAbsoluteValueArg(asmr, value, string);
        }
    }
    if (int64_t(size) > 0 && int64_t(repeat) > 0 && SSIZE_MAX/size < repeat)
    {
        asmr.printError(pseudoOpStr, "Product of repeat and size is too big");
        good = false;
    }
    
    cxuint truncBits = std::min(uint64_t(8), size)<<3;
    if (!_64bit)
        truncBits = std::min(cxuint(32), truncBits);
    if (truncBits != 0 && truncBits < 64) // if print 
        asmr.printWarningForRange(truncBits, value, asmr.getSourcePos(fillValStr));
    
    if (!good || !checkGarbagesAtEnd(asmr, string)) // if parsing failed
        return;
    
    if (int64_t(repeat) <= 0 || int64_t(size) <= 0)
        return;
    
    if (!_64bit)
        value &= 0xffffffffUL;
    
    /* do fill */
    cxbyte* content = asmr.reserveData(size*repeat);
    const size_t valueSize = std::min(uint64_t(8), size);
    uint64_t outValue;
    SLEV(outValue, value);
    for (uint64_t r = 0; r < repeat; r++)
    {
        ::memcpy(content, &outValue, valueSize);
        ::memset(content+valueSize, 0, size-valueSize);
        content += size;
    }
}

void AsmPseudoOps::doSkip(Assembler& asmr, const char*& string)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t size = 1, value = 0;
    
    const char* sizeStr = string;
    bool good = getAbsoluteValueArg(asmr, size, string);
    if (int64_t(size) < 0)
        asmr.printWarning(sizeStr, "Negative size has no effect");
    
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* fillValStr = string;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        fillValStr = string;
        good &= getAbsoluteValueArg(asmr, value, string);
        asmr.printWarningForRange(8, value, asmr.getSourcePos(fillValStr));
    }
    if (!good || !checkGarbagesAtEnd(asmr, string))
        return;
    
    if (int64_t(size) < 0)
        return;
    
    cxbyte* content = asmr.reserveData(size);
    ::memset(content, value&0xff, size);
}

void AsmPseudoOps::doAlign(Assembler& asmr,  const char*& string, bool powerOf2)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    
    uint64_t alignment, value = 0, maxAlign = 0;
    const char* alignStr = string;
    bool good = getAbsoluteValueArg(asmr, alignment, string, true);
    
    if (good)
    {
        if (powerOf2)
        {
            if (alignment >= 63)
            {
                asmr.printError(alignStr, "Power of 2 of alignment is greater than 63");
                good = false;
            }
            alignment = (1ULL<< alignment);
        }
        if (alignment == 0 || (1ULL<<(63-CLZ64(alignment))) != alignment)
        {
            asmr.printError(alignStr, "Alignment is not power of 2");
            good = false;
        }
    }
    
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* valueStr = string;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        valueStr = string;
        good &= getAbsoluteValueArg(asmr, value, string);
        asmr.printWarningForRange(8, value, asmr.getSourcePos(valueStr));
        
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            good &= getAbsoluteValueArg(asmr, maxAlign, string);
        }
    }
    if (!good || !checkGarbagesAtEnd(asmr, string)) //if parsing failed
        return;
    
    uint64_t outPos = asmr.currentOutPos;
    const uint64_t bytesToFill = ((outPos&(alignment-1))!=0) ?
            alignment - (outPos&(alignment-1)) : 0;
    if (maxAlign!=0 && bytesToFill > maxAlign)
        return; // do not make alignment
    
    cxbyte* content = asmr.reserveData(bytesToFill);
    ::memset(content, value&0xff, bytesToFill);
}

template<typename Word>
void AsmPseudoOps::doAlignWord(Assembler& asmr, const char* pseudoOpStr, const char*& string)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t alignment, value = 0, maxAlign = 0;
    const char* alignStr = string;
    bool good = getAbsoluteValueArg(asmr, alignment, string, true);
    if (good && alignment != 0 && (1ULL<<(63-CLZ64(alignment))) != alignment)
    {
        asmr.printError(alignStr, "Alignment is not power of 2");
        good = false;
    }
    
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* valueStr = string;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        valueStr = string;
        good &= getAbsoluteValueArg(asmr, value, string);
        asmr.printWarningForRange(sizeof(Word)<<3, value, asmr.getSourcePos(valueStr));
        
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            good &= getAbsoluteValueArg(asmr, maxAlign, string);
        }
    }
    if (!good || !checkGarbagesAtEnd(asmr, string))
        return;
    
    if (alignment == 0)
        return; // do nothing
    
    uint64_t outPos = asmr.currentOutPos;
    if (outPos&(sizeof(Word)-1))
    {
        asmr.printError(pseudoOpStr, "Offset is not aligned to word");
        return;
    }
    
    const uint64_t bytesToFill = ((outPos&(alignment-1))!=0) ?
            alignment - (outPos&(alignment-1)) : 0;
    
    if (maxAlign!=0 && bytesToFill > maxAlign)
        return; // do not make alignment
    
    cxbyte* content = asmr.reserveData(bytesToFill);
    Word word;
    SLEV(word, value);
    std::fill(reinterpret_cast<Word*>(content),
              reinterpret_cast<Word*>(content + bytesToFill), word);
}

void AsmPseudoOps::doOrganize(Assembler& asmr, const char*& string)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t value;
    cxuint sectionId = ASMSECT_ABS;
    const char* valStr = string;
    bool good = getAnyValueArg(asmr, value, sectionId, string);
    
    uint64_t fillValue = 0;
    bool haveComma;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* fillValueStr = string;
    if (haveComma) // optional fill argument
    {
        string = skipSpacesToEnd(string, end);
        fillValueStr = string;
        good = getAbsoluteValueArg(asmr, fillValue, string, true);
    }
    asmr.printWarningForRange(8, fillValue, asmr.getSourcePos(fillValueStr));
    
    if (!good || !checkGarbagesAtEnd(asmr, string))
        return;
    
    asmr.assignOutputCounter(valStr, value, sectionId, fillValue);
}

void AsmPseudoOps::doIfInt(Assembler& asmr, const char* pseudoOpStr, const char*& string,
               IfIntComp compType, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t value;
    bool good = getAbsoluteValueArg(asmr, value, string, true);
    if (!good || !checkGarbagesAtEnd(asmr, string))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool satisfied;
    switch(compType)
    {
        case IfIntComp::EQUAL:
            satisfied = (value == 0);
            break;
        case IfIntComp::NOT_EQUAL:
            satisfied = (value != 0);
            break;
        case IfIntComp::LESS:
            satisfied = (int64_t(value) < 0);
            break;
        case IfIntComp::LESS_EQUAL:
            satisfied = (int64_t(value) <= 0);
            break;
        case IfIntComp::GREATER:
            satisfied = (int64_t(value) > 0);
            break;
        case IfIntComp::GREATER_EQUAL:
            satisfied = (int64_t(value) >= 0);
            break;
        default:
            satisfied = false;
            break;
    }
    bool included;
    if (asmr.pushClause(asmr.getSourcePos(pseudoOpStr), clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfDef(Assembler& asmr, const char* pseudoOpStr, const char*& string,
               bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* strAtSymName = string;
    AsmSymbolEntry* entry;
    bool good = true;
    Assembler::ParseState state = asmr.parseSymbol(string, string, entry, false, true);
    if (state == Assembler::ParseState::FAILED)
        return;
    if (state == Assembler::ParseState::MISSING)
    {
        asmr.printError(strAtSymName, "Expected symbol");
        good = false;
    }
    if (!good || !checkGarbagesAtEnd(asmr, string))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    const bool symDefined = (entry!=nullptr && (entry->second.hasValue ||
                entry->second.expression!=nullptr));
    bool satisfied = (!negation) ?  symDefined : !symDefined;
    if (asmr.pushClause(asmr.getSourcePos(pseudoOpStr), clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfBlank(Assembler& asmr, const char* pseudoOpStr, const char*& string,
              bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? string==end : string!=end;
    if (asmr.pushClause(asmr.getSourcePos(pseudoOpStr), clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

static std::string getStringToCompare(const char* strStart, const char* strEnd)
{
    std::string firstStr;
    bool blank = true;
    bool singleQuote = false;
    bool dblQuote = false;
    for (const char* s = strStart; s != strEnd; ++s)
        if (isSpace(*s))
        {
            if (!blank || dblQuote || singleQuote)
                firstStr.push_back(*s);
            blank = true;
        }
        else
        {
            blank = false;
            if (*s == '"' && !singleQuote)
                dblQuote = !dblQuote;
            else if (*s == '\'' && !dblQuote)
                singleQuote = !singleQuote;
            firstStr.push_back(*s);
        }
    if (!firstStr.empty() && isSpace(firstStr.back()))
        firstStr.pop_back(); // remove last space
    return firstStr;
}

void AsmPseudoOps::doIfCmpStr(Assembler& asmr, const char* pseudoOpStr,
               const char*& string, bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* firstStrStart = string;
    bool good = true;
    while (string != end && *string != ',') string++;
    if (string == end)
    {
        asmr.printError(string, "Missing second string");
        return;
    }
    const char* firstStrEnd = string;
    if (good) string++; // comma
    else return;
    
    std::string firstStr = getStringToCompare(firstStrStart, firstStrEnd);
    std::string secondStr = getStringToCompare(string, end);
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? firstStr==secondStr : firstStr!=secondStr;
    if (asmr.pushClause(asmr.getSourcePos(pseudoOpStr), clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfStrEqual(Assembler& asmr, const char* pseudoOpStr,
                const char*& string, bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    std::string firstStr, secondStr;
    bool good = asmr.parseString(firstStr, string, string);
    bool haveComma;
    if (!skipComma(asmr, haveComma, string))
        return;
    if (!haveComma)
    {
        asmr.printError(string, "Expected two strings");
        return;
    }
    string = skipSpacesToEnd(string, end);
    good &= asmr.parseString(secondStr, string, string);
    if (!good || !checkGarbagesAtEnd(asmr, string))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? firstStr==secondStr : firstStr!=secondStr;
    if (asmr.pushClause(asmr.getSourcePos(pseudoOpStr), clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doElse(Assembler& asmr, const char* pseudoOpStr, const char*&string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (!checkGarbagesAtEnd(asmr, string))
        return;
    bool included;
    if (asmr.pushClause(asmr.getSourcePos(pseudoOpStr), AsmClauseType::ELSE,
                        true, included))
    {
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doEndIf(Assembler& asmr, const char* pseudoOpStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (!checkGarbagesAtEnd(asmr, string))
        return;
    asmr.popClause(asmr.getSourcePos(pseudoOpStr), AsmClauseType::IF);
}

};

void Assembler::parsePseudoOps(const std::string firstName,
       const char* stmtStartStr, const char*& string)
{
    const size_t pseudoOp = binaryFind(pseudoOpNamesTbl, pseudoOpNamesTbl +
                    sizeof(pseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - pseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case ASMOP_32BIT:
        case ASMOP_64BIT:
            AsmPseudoOps::setBitness(*this, string, pseudoOp == ASMOP_64BIT);
            break;
        case ASMOP_ABORT:
            printError(stmtStartStr, "Aborted!");
            endOfAssembly = true;
            break;
        case ASMOP_ALIGN:
        case ASMOP_BALIGN:
            AsmPseudoOps::doAlign(*this, string);
            break;
        case ASMOP_ARCH:
            break;
        case ASMOP_ASCII:
            AsmPseudoOps::putStrings(*this, string);
            break;
        case ASMOP_ASCIZ:
            AsmPseudoOps::putStrings(*this, string, true);
            break;
        case ASMOP_BALIGNL:
            AsmPseudoOps::doAlignWord<uint32_t>(*this, stmtStartStr, string);
            break;
        case ASMOP_BALIGNW:
            AsmPseudoOps::doAlignWord<uint16_t>(*this, stmtStartStr, string);
            break;
        case ASMOP_BYTE:
            AsmPseudoOps::putIntegers<cxbyte>(*this, string);
            break;
        case ASMOP_AMD:
        case ASMOP_RAWCODE:
        case ASMOP_GALLIUM:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, string))
            {
                if (outFormatInitialized)
                    printError(string, "Output format type is already defined");
                else
                    format = (pseudoOp == ASMOP_GALLIUM) ? BinaryFormat::GALLIUM :
                        (pseudoOp == ASMOP_AMD) ? BinaryFormat::AMD :
                        BinaryFormat::RAWCODE;
            }
            break;
        case ASMOP_CONFIG:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, string))
            {
                if (format == BinaryFormat::AMD)
                {
                    initializeOutputFormat();
                    if (inGlobal)
                        printError(string,
                               "Configuration in global layout is illegal");
                    else
                        inAmdConfig = true; // inside Amd Config
                }
                else
                    printError(string,
                       "Configuration section only for AMD Catalyst binaries");
            }
            break;
        case ASMOP_DATA:
            break;
        case ASMOP_DOUBLE:
            AsmPseudoOps::putFloats<uint64_t>(*this, string);
            break;
        case ASMOP_ELSE:
            AsmPseudoOps::doElse(*this, stmtStartStr, string);
            break;
        case ASMOP_ELSEIF:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::NOT_EQUAL, true);
            break;
        case ASMOP_ELSEIFB:
            AsmPseudoOps::doIfBlank(*this, stmtStartStr, string, false, true);
            break;
        case ASMOP_ELSEIFC:
            AsmPseudoOps::doIfCmpStr(*this, stmtStartStr, string, false, true);
            break;
        case ASMOP_ELSEIFDEF:
            AsmPseudoOps::doIfDef(*this, stmtStartStr, string, false, true);
            break;
        case ASMOP_ELSEIFEQ:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::EQUAL, true);
            break;
        case ASMOP_ELSEIFEQS:
            AsmPseudoOps::doIfStrEqual(*this, stmtStartStr, string, false, true);
            break;
        case ASMOP_ELSEIFGE:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::GREATER_EQUAL, true);
            break;
        case ASMOP_ELSEIFGT:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::GREATER, true);
            break;
        case ASMOP_ELSEIFLE:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::LESS_EQUAL, true);
            break;
        case ASMOP_ELSEIFLT:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::LESS, true);
            break;
        case ASMOP_ELSEIFNB:
            AsmPseudoOps::doIfBlank(*this, stmtStartStr, string, true, true);
            break;
        case ASMOP_ELSEIFNC:
            AsmPseudoOps::doIfCmpStr(*this, stmtStartStr, string, true, true);
            break;
        case ASMOP_ELSEIFNDEF:
            AsmPseudoOps::doIfDef(*this, stmtStartStr, string, true, true);
            break;
        case ASMOP_ELSEIFNE:
        case ASMOP_ELSEIFNOTDEF:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::NOT_EQUAL, true);
            break;
        case ASMOP_ELSEIFNES:
            AsmPseudoOps::doIfStrEqual(*this, stmtStartStr, string, true, true);
            break;
        case ASMOP_END:
            endOfAssembly = true;
            break;
        case ASMOP_ENDIF:
            AsmPseudoOps::doEndIf(*this, stmtStartStr, string);
            break;
        case ASMOP_ENDM:
            break;
        case ASMOP_ENDR:
            break;
        case ASMOP_EQU:
        case ASMOP_SET:
            AsmPseudoOps::setSymbol(*this, string);
            break;
        case ASMOP_EQUIV:
            AsmPseudoOps::setSymbol(*this, string, false);
            break;
        case ASMOP_EQV:
            AsmPseudoOps::setSymbol(*this, string, false, true);
            break;
        case ASMOP_ERR:
            printError(stmtStartStr, ".err encountered");
            break;
        case ASMOP_ERROR:
            AsmPseudoOps::printError(*this, stmtStartStr, string);
            break;
        case ASMOP_EXITM:
            break;
        case ASMOP_EXTERN:
            AsmPseudoOps::ignoreExtern(*this, string);
            break;
        case ASMOP_FAIL:
            AsmPseudoOps::doFail(*this, stmtStartStr, string);
            break;
        case ASMOP_FILE:
            break;
        case ASMOP_FILL:
            AsmPseudoOps::doFill(*this, stmtStartStr, string, false);
            break;
        case ASMOP_FILLQ:
            AsmPseudoOps::doFill(*this, stmtStartStr, string, true);
            break;
        case ASMOP_FLOAT:
            AsmPseudoOps::putFloats<uint32_t>(*this, string);
            break;
        case ASMOP_FORMAT:
            AsmPseudoOps::setOutFormat(*this, string);
            break;
        case ASMOP_GLOBAL:
            AsmPseudoOps::setSymbolBind(*this, string, STB_GLOBAL);
            break;
        case ASMOP_GPU:
            break;
        case ASMOP_HALF:
            AsmPseudoOps::putFloats<uint16_t>(*this, string);
            break;
        case ASMOP_HWORD:
            AsmPseudoOps::putIntegers<uint16_t>(*this, string);
            break;
        case ASMOP_IF:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::NOT_EQUAL, false);
            break;
        case ASMOP_IFB:
            AsmPseudoOps::doIfBlank(*this, stmtStartStr, string, false, false);
            break;
        case ASMOP_IFC:
            AsmPseudoOps::doIfCmpStr(*this, stmtStartStr, string, false, false);
            break;
        case ASMOP_IFDEF:
            AsmPseudoOps::doIfDef(*this, stmtStartStr, string, false, false);
            break;
        case ASMOP_IFEQ:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::EQUAL, false);
            break;
        case ASMOP_IFEQS:
            AsmPseudoOps::doIfStrEqual(*this, stmtStartStr, string, false, false);
            break;
        case ASMOP_IFGE:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::GREATER_EQUAL, false);
            break;
        case ASMOP_IFGT:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::GREATER, false);
            break;
        case ASMOP_IFLE:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::LESS_EQUAL, false);
            break;
        case ASMOP_IFLT:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::LESS, false);
            break;
        case ASMOP_IFNB:
            AsmPseudoOps::doIfBlank(*this, stmtStartStr, string, true, false);
            break;
        case ASMOP_IFNC:
            AsmPseudoOps::doIfCmpStr(*this, stmtStartStr, string, true, false);
            break;
        case ASMOP_IFNDEF:
        case ASMOP_IFNOTDEF:
            AsmPseudoOps::doIfDef(*this, stmtStartStr, string, true, false);
            break;
        case ASMOP_IFNE:
            AsmPseudoOps::doIfInt(*this, stmtStartStr, string,
                      IfIntComp::NOT_EQUAL, false);
            break;
        case ASMOP_IFNES:
            AsmPseudoOps::doIfStrEqual(*this, stmtStartStr, string, true, false);
            break;
        case ASMOP_INCBIN:
            AsmPseudoOps::includeBinFile(*this, string);
            break;
        case ASMOP_INCLUDE:
            AsmPseudoOps::includeFile(*this, stmtStartStr, string);
            break;
        case ASMOP_INT:
        case ASMOP_LONG:
            AsmPseudoOps::putIntegers<uint32_t>(*this, string);
            break;
        case ASMOP_KERNEL:
            AsmPseudoOps::goToKernel(*this, string);
            break;
        case ASMOP_LINE:
        case ASMOP_LN:
            break;
        case ASMOP_LOC:
            break;
        case ASMOP_LOCAL:
            AsmPseudoOps::setSymbolBind(*this, string, STB_LOCAL);
            break;
        case ASMOP_MACRO:
            break;
        case ASMOP_OCTA:
            AsmPseudoOps::putUInt128s(*this, string);
            break;
        case ASMOP_OFFSET:
            break;
        case ASMOP_ORG:
            AsmPseudoOps::doOrganize(*this, string);
            break;
        case ASMOP_P2ALIGN:
            AsmPseudoOps::doAlign(*this, string, true);
            break;
        case ASMOP_PRINT:
        {
            std::string outStr;
            if (parseString(outStr, string, string))
            {
                if (AsmPseudoOps::checkGarbagesAtEnd(*this, string))
                {
                    printStream.write(outStr.c_str(), outStr.size());
                    printStream.put('\n');
                }
            }
            break;
        }
        case ASMOP_PURGEM:
            break;
        case ASMOP_QUAD:
            AsmPseudoOps::putIntegers<uint64_t>(*this, string);
            break;
        case ASMOP_REPT:
            break;
        case ASMOP_SECTION:
            break;
        case ASMOP_SHORT:
            AsmPseudoOps::putIntegers<uint16_t>(*this, string);
            break;
        case ASMOP_SINGLE:
            AsmPseudoOps::putFloats<uint32_t>(*this, string);
            break;
        case ASMOP_SIZE:
            AsmPseudoOps::setSymbolSize(*this, string);
            break;
        case ASMOP_SKIP:
        case ASMOP_SPACE:
            AsmPseudoOps::doSkip(*this, string);
            break;
        case ASMOP_STRING:
            AsmPseudoOps::putStrings(*this, string, true);
            break;
        case ASMOP_STRING16:
            AsmPseudoOps::putStringsToInts<uint16_t>(*this, string);
            break;
        case ASMOP_STRING32:
            AsmPseudoOps::putStringsToInts<uint32_t>(*this, string);
            break;
        case ASMOP_STRING64:
            AsmPseudoOps::putStringsToInts<uint64_t>(*this, string);
            break;
        case ASMOP_STRUCT:
            break;
        case ASMOP_TEXT:
            break;
        case ASMOP_TITLE:
            break;
        case ASMOP_WARNING:
            AsmPseudoOps::printWarning(*this, stmtStartStr, string);
            break;
        case ASMOP_WEAK:
            AsmPseudoOps::setSymbolBind(*this, string, STB_WEAK);
            break;
        case ASMOP_WORD:
            AsmPseudoOps::putIntegers<uint32_t>(*this, string);
            break;
        default:
            // macro substitution
            break;
    }
}

/* skipping clauses */
bool Assembler::skipClauses()
{
    cxuint clauseLevel = clauses.size();
    bool good = true;
    while (clauses.size() >= clauseLevel)
    {
        if (!readLine())
            break;
        
        const char* string = line;
        const char* end = line+lineSize;
        string = skipSpacesToEnd(string, end);
        const char* stmtString = string;
        if (string == end || *string != '.')
            continue;
        
        std::string pseudOpName = extractSymName(string, end, false);
        toLowerString(pseudOpName);
        
        const size_t pseudoOp = binaryFind(offlinePseudoOpNamesTbl,
               offlinePseudoOpNamesTbl + sizeof(offlinePseudoOpNamesTbl)/sizeof(char*),
               pseudOpName.c_str()+1, CStringLess()) - offlinePseudoOpNamesTbl;
        
        switch(pseudoOp)
        {
            case ASMCOP_ENDIF:
                if (!popClause(getSourcePos(stmtString), AsmClauseType::IF))
                    good = false;
                break;
            case ASMCOP_ENDM:
                if (!popClause(getSourcePos(stmtString), AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMCOP_ENDR:
                if (!popClause(getSourcePos(stmtString), AsmClauseType::REPEAT))
                    good = false;
                break;
            case ASMCOP_ELSE:
            case ASMCOP_ELSEIF:
            case ASMCOP_ELSEIFB:
            case ASMCOP_ELSEIFC:
            case ASMCOP_ELSEIFDEF:
            case ASMCOP_ELSEIFEQ:
            case ASMCOP_ELSEIFEQS:
            case ASMCOP_ELSEIFGE:
            case ASMCOP_ELSEIFGT:
            case ASMCOP_ELSEIFLE:
            case ASMCOP_ELSEIFLT:
            case ASMCOP_ELSEIFNB:
            case ASMCOP_ELSEIFNC:
            case ASMCOP_ELSEIFNDEF:
            case ASMCOP_ELSEIFNE:
            case ASMCOP_ELSEIFNES:
            case ASMCOP_ELSEIFNOTDEF:
                if (clauseLevel == clauses.size())
                {
                    lineAlreadyRead = true; // read
                    return good; // do exit
                }
                if (!pushClause(getSourcePos(stmtString), (pseudoOp==ASMCOP_ELSE ?
                            AsmClauseType::ELSE : AsmClauseType::ELSEIF)))
                    good = false;
                break;
            case ASMCOP_IF:
            case ASMCOP_IFB:
            case ASMCOP_IFC:
            case ASMCOP_IFDEF:
            case ASMCOP_IFEQ:
            case ASMCOP_IFEQS:
            case ASMCOP_IFGE:
            case ASMCOP_IFGT:
            case ASMCOP_IFLE:
            case ASMCOP_IFLT:
            case ASMCOP_IFNB:
            case ASMCOP_IFNC:
            case ASMCOP_IFNDEF:
            case ASMCOP_IFNE:
            case ASMCOP_IFNES:
            case ASMCOP_IFNOTDEF:
                if (!pushClause(getSourcePos(stmtString), AsmClauseType::IF))
                    good = false;
                break;
            case ASMCOP_MACRO:
                if (!pushClause(getSourcePos(stmtString), AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMCOP_REPT:
                if (!pushClause(getSourcePos(stmtString), AsmClauseType::REPEAT))
                    good = false;
                break;
            default:
                break;
        }
    }
    return good;
}

bool Assembler::putMacroContent(AsmMacro& macro)
{
    return true;
}

bool Assembler::putRepetitionContent(AsmRepeat& repeat)
{
    return true;
}
