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

struct CLRX_INTERNAL AsmPseudoOps
{
    /* parsing helpers */
    /* get absolute value arg resolved at this time.
       if empty expression value is not set */
    static bool getAbsoluteValueArg(Assembler& asmr, uint64_t& value, const char*& string,
                    bool requredExpr = false);
    
    static bool getRelativeValueArg(Assembler& asmr, uint64_t& value, cxuint& sectionId,
                    const char*& string);
    // get name (not symbol name)
    static bool getNameArg(Assembler& asmr, std::string& outStr, const char*& string,
               const char* objName);
    // skip comma
    static bool skipComma(Assembler& asmr, bool& haveComma, const char*& string);
    
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
    static void includeFile(Assembler& asmr, const char* pseudoStr, const char*& string);
    // include binary file
    static void includeBinFile(Assembler& asmr, const char*& string);
    
    // fail
    static void doFail(Assembler& asmr, const char* pseudoStr, const char*& string);
    // .error
    static void printError(Assembler& asmr, const char* pseudoStr, const char*& string);
    // .warning
    static void printWarning(Assembler& asmr, const char* pseudoStr, const char*& string);
    
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
    
    static void doFill(Assembler& asmr, const char* pseudoStr, const char*& string,
               bool _64bit = false);
    static void doSkip(Assembler& asmr, const char*& string);
    
    /* TODO: add no-op fillin for text sections */
    static void doAlign(Assembler& asmr,  const char*& string, bool powerOf2 = false);
    
    /* TODO: add no-op fillin for text sections */
    template<typename Word>
    static void doAlignWord(Assembler& asmr, const char* pseudoStr, const char*& string);
    
    static void doOrganize(Assembler& asmr, const char*& string);
};

bool AsmPseudoOps::getAbsoluteValueArg(Assembler& asmr, uint64_t& value,
                      const char*& string, bool requiredExpr)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* exprStr = string;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, string, string));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty() && requiredExpr)
    {
        asmr.printError(exprStr, "Expected expression");
        return false;
    }
    if (expr->isEmpty()) // do not set if empty expression
        return true;
    if (expr->getSymOccursNum() != 0)
    {   // not resolved at this time
        asmr.printError(exprStr, "Expression has an unresolved symbols!");
        return false;
    }
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

bool AsmPseudoOps::getRelativeValueArg(Assembler& asmr, uint64_t& value,
                   cxuint& sectionId, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* exprStr = string;
    std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, string, string));
    if (expr == nullptr)
        return false;
    if (expr->isEmpty())
    {
        asmr.printError(exprStr, "Expected expression");
        return false;
    }
    if (expr->getSymOccursNum() != 0)
    {   // not resolved at this time
        asmr.printError(exprStr, "Expression has an unresolved symbols!");
        return false;
    }
    if (!expr->evaluate(asmr, value, sectionId)) // failed evaluation!
        return false;
    else if (sectionId == ASMSECT_ABS)
    {   // if not absolute value
        asmr.printError(exprStr, "Expression must be relative!");
        return false;
    }
    
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
    if ((*string >= 'a' && *string <= 'z') || (*string >= 'A' && *string <= 'Z') ||
            *string == '_')
    {
        string++;
        while (string != end && ((*string >= 'a' && *string <= 'z') ||
           (*string >= 'A' && *string <= 'Z') || (*string >= '0' && *string <= '9') ||
           *string == '_')) string++;
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

void AsmPseudoOps::setBitness(Assembler& asmr, const char*& string, bool _64Bit)
{
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

void AsmPseudoOps::includeFile(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    std::string filename;
    const char* nameStr = string;
    if (asmr.parseString(filename, string, string))
    {
        bool failedOpen = false;
        filesystemPath(filename);
        try
        {
            std::unique_ptr<AsmInputFilter> newInputFilter(new AsmStreamInputFilter(
                    asmr.getSourcePos(pseudoStr), filename));
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
                        asmr.getSourcePos(pseudoStr), inDirFilename));
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
    uint64_t offset = 0, count = UINT64_MAX;
    bool good = asmr.parseString(filename, string, string);
    bool haveComma;
    if (!skipComma(asmr, haveComma, string))
        return;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        good &= getAbsoluteValueArg(asmr, offset, string);
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            good &= getAbsoluteValueArg(asmr, count, string);
        }
    }
    if (!good) // failed parsing
        return;
    
    if (count == 0)
    {
        asmr.printWarning(nameStr, "Number of bytes is zero, ignoring .incbin");
        return;
    }
    
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

void AsmPseudoOps::doFail(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t value = 0;
    if (!getAbsoluteValueArg(asmr, value, string, true))
        return;
    char buf[50];
    ::memcpy(buf, ".fail ", 6);
    const size_t pos = 6+itocstrCStyle(int64_t(value), buf+6, 50-6);
    ::memcpy(buf+pos, " encountered", 13);
    if (int64_t(value) >= 500)
        asmr.printWarning(pseudoStr, buf);
    else
        asmr.printError(pseudoStr, buf);
}

void AsmPseudoOps::printError(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, string, string))
            return; // error
        asmr.printError(pseudoStr, outStr.c_str());
    }
    else
        asmr.printError(pseudoStr, ".error encountered");
}

void AsmPseudoOps::printWarning(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    if (string != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, string, string))
            return; // error
        asmr.printWarning(pseudoStr, outStr.c_str());
    }
    else
        asmr.printWarning(pseudoStr, ".warning encountered");
}

template<typename T>
void AsmPseudoOps::putIntegers(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    while (true)
    {
        const char* literalStr = string;
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
                                         asmr.getSourcePos(literalStr));
                        T out;
                        SLEV(out, value);
                        asmr.putData(sizeof(T), reinterpret_cast<const cxbyte*>(&out));
                    }
                    else
                        asmr.printError(literalStr, "Expression must be absolute!");
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
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
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
    while (true)
    {
        UIntType out = 0;
        if (string != end && *string != ',')
        {
            try
            { out = asmcstrtofCStyleLEV<UIntType>(string, end, string); }
            catch(const ParseException& ex)
            { asmr.printError(string, ex.what()); }
        }
        else // warning
            asmr.printWarning(string, "No floating point literal, zero has been put");
        
        asmr.putData(sizeof(UIntType), reinterpret_cast<const cxbyte*>(&out));
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

void AsmPseudoOps::putUInt128s(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    if (string == end)
        return;
    while (true)
    {
        UInt128 value = { 0, 0 };
        if (string != end && *string != ',')
        {
            try
            { value = cstrtou128CStyle(string, end, string); }
            catch(const ParseException& ex)
            { asmr.printError(string, ex.what()); }
        }
        else // warning
            asmr.printWarning(string, "No 128-bit literal, zero has been put");
        UInt128 out;
        SLEV(out.lo, value.lo);
        SLEV(out.hi, value.hi);
        asmr.putData(16, reinterpret_cast<const cxbyte*>(&out));
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

void AsmPseudoOps::putStrings(Assembler& asmr, const char*& string, bool addZero)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    while (string != end)
    {
        std::string outStr;
        if (*string != ',')
        {
            if (asmr.parseString(outStr, string, string))
                asmr.putData(outStr.size()+(addZero), (const cxbyte*)outStr.c_str());
        }
        
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
}

template<typename T>
void AsmPseudoOps::putStringsToInts(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    string = skipSpacesToEnd(string, end);
    while (string != end)
    {
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
        
        string = skipSpacesToEnd(string, end); // spaces before ','
        if (string == end)
            break;
        if (*string != ',')
            asmr.printError(string, "Expected ',' before next value");
        else
            string = skipSpacesToEnd(string+1, end);
    }
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
    if (!good) // is not so good
        return;
    asmr.assignSymbol(symName, strAtSymName, string, reassign, baseExpr);
    string = end;
}

void AsmPseudoOps::setSymbolBind(Assembler& asmr, const char*& string, cxbyte bind)
{
    const char* end = asmr.line + asmr.lineSize;
    bool haveComma = true; // initial value
    while (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        const char* symNameStr = string;
        AsmSymbolEntry* symEntry;
        bool good = asmr.parseSymbol(string, string, symEntry, false);
        if (symEntry == nullptr)
        {
            asmr.printError(symNameStr, "Expected symbol name");
            good = false;
        }
        if (good)
            symEntry->second.info = ELF32_ST_INFO(bind,
                      ELF32_ST_TYPE(symEntry->second.info));
        if (!skipComma(asmr, haveComma, string))
            return;
    }
}

void AsmPseudoOps::setSymbolSize(Assembler& asmr, const char*& string)
{
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    const char* symNameStr = string;
    AsmSymbolEntry* symEntry;
    bool good = asmr.parseSymbol(string, string, symEntry, false);
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
    if (good)
        symEntry->second.size = size;
}

void AsmPseudoOps::ignoreExtern(Assembler& asmr, const char*& string)
{
    bool haveComma = true; // initial value
    while (haveComma)
    {
        asmr.skipSymbol(string, string);
        if (!skipComma(asmr, haveComma, string))
            return;
    }
}

void AsmPseudoOps::doFill(Assembler& asmr, const char* pseudoStr, const char*& string,
          bool _64bit)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t repeat, size = 1, value = 0;
    bool good = getAbsoluteValueArg(asmr, repeat, string, true);
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* secondParamStr = string;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        good &= getAbsoluteValueArg(asmr, size, string);
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            secondParamStr = string;
            good &= getAbsoluteValueArg(asmr, value, string);
        }
    }
    if (!good) // if parsing failed
        return;
    
    if (!_64bit && (value & (0xffffffffULL<<32))!=0)
    {
        asmr.printWarning(secondParamStr,
                  ".fill pseudo-op truncates pattern to 32-bit value");
        value &= 0xffffffffUL;
    }
    if (size == 0)
        return;
    if (SIZE_MAX/size < repeat)
    {
        asmr.printError(pseudoStr, "Product of repeat and size is too big");
        return;
    }
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
    bool good = getAbsoluteValueArg(asmr, size, string);
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        good &= getAbsoluteValueArg(asmr, value, string);
    }
    if (!good)
        return;
    
    if ((value & ~uint64_t(0xff)) != 0)
        asmr.printWarning(string, "Fill value is truncated to  8-bit value");
    
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
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* valueStr = string;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        valueStr = string;
        good &= getAbsoluteValueArg(asmr, value, string);
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            good &= getAbsoluteValueArg(asmr, maxAlign, string);
        }
    }
    if (!good) //if parsing failed
        return;
    
    if (powerOf2)
    {
        if (alignment >= 64)
        {
            asmr.printError(alignStr, "Power of 2 of alignment is greater than 64");
            return;
        }
        alignment = (1ULL<< alignment);
    }
    
    if ((value & ~uint64_t(0xff)) != 0)
        asmr.printWarning(valueStr, "Fill value is truncated to 8-bit value");
    if (alignment == 0 || (1ULL<<(63-CLZ64(alignment))) != alignment)
    {
        asmr.printError(alignStr, "Alignment is not power of 2");
        return;
    }
    uint64_t outPos = asmr.currentOutPos;
    const uint64_t bytesToFill = ((outPos&(alignment-1))!=0) ?
            alignment - (outPos&(alignment-1)) : 0;
    if (maxAlign!=0 && bytesToFill > maxAlign)
        return; // do not make alignment
    
    cxbyte* content = asmr.reserveData(bytesToFill);
    ::memset(content, value&0xff, bytesToFill);
}

template<typename Word>
void AsmPseudoOps::doAlignWord(Assembler& asmr, const char* pseudoStr, const char*& string)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    string = skipSpacesToEnd(string, end);
    uint64_t alignment, value = 0, maxAlign = 0;
    const char* alignStr = string;
    bool good = getAbsoluteValueArg(asmr, alignment, string, true);
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, string))
        return;
    const char* valueStr = string;
    if (haveComma)
    {
        string = skipSpacesToEnd(string, end);
        valueStr = string;
        good &= getAbsoluteValueArg(asmr, value, string);
        if (!skipComma(asmr, haveComma, string))
            return;
        if (haveComma)
        {
            string = skipSpacesToEnd(string, end);
            good &= getAbsoluteValueArg(asmr, maxAlign, string);
        }
    }
    if (!good)
        return;
    
    if ((value & ~((1ULL<<(sizeof(Word)<<3))-1)) != 0)
        asmr.printWarning(valueStr, "Fill value is truncated to word");
    
    if (alignment == 0)
        return; // do nothing
    
    if ((1ULL<<(63-CLZ64(alignment))) != alignment)
    {
        asmr.printError(alignStr, "Alignment is not power of 2");
        return;
    }
    uint64_t outPos = asmr.currentOutPos;
    if (outPos&(sizeof(Word)-1))
    {
        asmr.printError(pseudoStr, "Offset is not aligned to word");
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
    cxuint sectionId;
    const char* valStr = string;
    if (!getRelativeValueArg(asmr, value, sectionId, string))
        return;
    uint64_t fillValue = 0;
    bool haveComma;
    if (!skipComma(asmr, haveComma, string))
        return;
    if (haveComma) // optional fill argument
        if (!getAbsoluteValueArg(asmr, fillValue, string, true))
            return;
    asmr.assignOutputCounter(valStr, value, sectionId, fillValue);
}

};

bool Assembler::parsePseudoOps(const std::string firstName,
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
            return good;
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
            if (outFormatInitialized)
                printError(string, "Output format type is already defined");
            else
                format = (pseudoOp == ASMOP_GALLIUM) ? BinaryFormat::GALLIUM :
                    (pseudoOp == ASMOP_AMD) ? BinaryFormat::AMD :
                    BinaryFormat::RAWCODE;
            break;
        case ASMOP_CONFIG:
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
            break;
        case ASMOP_DATA:
            break;
        case ASMOP_DOUBLE:
            AsmPseudoOps::putFloats<uint64_t>(*this, string);
            break;
        case ASMOP_ELSE:
            break;
        case ASMOP_ELSEIF:
            break;
        case ASMOP_ELSEIFB:
            break;
        case ASMOP_ELSEIFC:
            break;
        case ASMOP_ELSEIFDEF:
            break;
        case ASMOP_ELSEIFEQ:
            break;
        case ASMOP_ELSEIFEQS:
            break;
        case ASMOP_ELSEIFGE:
            break;
        case ASMOP_ELSEIFGT:
            break;
        case ASMOP_ELSEIFLE:
            break;
        case ASMOP_ELSEIFLT:
            break;
        case ASMOP_ELSEIFNB:
            break;
        case ASMOP_ELSEIFNC:
            break;
        case ASMOP_ELSEIFNDEF:
            break;
        case ASMOP_ELSEIFNE:
            break;
        case ASMOP_ELSEIFNES:
            break;
        case ASMOP_ELSEIFNOTDEF:
            break;
        case ASMOP_END:
            break;
        case ASMOP_ENDIF:
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
            break;
        case ASMOP_IFB:
            break;
        case ASMOP_IFC:
            break;
        case ASMOP_IFDEF:
            break;
        case ASMOP_IFEQ:
            break;
        case ASMOP_IFEQS:
            break;
        case ASMOP_IFGE:
            break;
        case ASMOP_IFGT:
            break;
        case ASMOP_IFLE:
            break;
        case ASMOP_IFLT:
            break;
        case ASMOP_IFNB:
            break;
        case ASMOP_IFNC:
            break;
        case ASMOP_IFNDEF:
            break;
        case ASMOP_IFNE:
            break;
        case ASMOP_IFNES:
            break;
        case ASMOP_IFNOTDEF:
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
                std::cout.write(outStr.c_str(), outStr.size());
                std::cout.put('\n');
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
    return true;
}
