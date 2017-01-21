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

#include <CLRX/Config.h>
#include <string>
#include <cstring>
#include <cassert>
#include <fstream>
#include <vector>
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

// pseudo-ops used while skipping clauses
static const char* offlinePseudoOpNamesTbl[] =
{
    "else", "elseif", "elseif32", "elseif64", "elseifarch",
    "elseifb", "elseifc", "elseifdef",
    "elseifeq", "elseifeqs", "elseiffmt",
    "elseifge", "elseifgpu", "elseifgt",
    "elseifle", "elseiflt", "elseifnarch", "elseifnb", "elseifnc",
    "elseifndef", "elseifne", "elseifnes",
    "elseifnfmt", "elseifngpu", "elseifnotdef",
    "endif", "endm", "endr",
    "if", "if32", "if64", "ifarch", "ifb", "ifc", "ifdef", "ifeq",
    "ifeqs", "iffmt", "ifge", "ifgpu", "ifgt", "ifle",
    "iflt", "ifnarch", "ifnb", "ifnc", "ifndef",
    "ifne", "ifnes", "ifnfmt", "ifngpu", "ifnotdef",
    "irp", "irpc", "macro", "rept"
};

/// pseudo-ops not ignored while putting macro content
static const char* macroRepeatPseudoOpNamesTbl[] =
{
    "endm", "endr", "irp", "irpc", "macro", "rept"
};

// pseudo-ops used while skipping clauses
enum
{
    ASMCOP_ELSE = 0, ASMCOP_ELSEIF, ASMCOP_ELSEIF32, ASMCOP_ELSEIF64, ASMCOP_ELSEIFARCH,
    ASMCOP_ELSEIFB, ASMCOP_ELSEIFC, ASMCOP_ELSEIFDEF,
    ASMCOP_ELSEIFEQ, ASMCOP_ELSEIFEQS, ASMCOP_ELSEIFFMT,
    ASMCOP_ELSEIFGE, ASMCOP_ELSEIFGPU, ASMCOP_ELSEIFGT,
    ASMCOP_ELSEIFLE, ASMCOP_ELSEIFLT, ASMCOP_ELSEIFNARCH, ASMCOP_ELSEIFNB, ASMCOP_ELSEIFNC,
    ASMCOP_ELSEIFNDEF, ASMCOP_ELSEIFNE, ASMCOP_ELSEIFNES,
    ASMCOP_ELSEIFNFMT, ASMCOP_ELSEIFNGPU, ASMCOP_ELSEIFNOTDEF,
    ASMCOP_ENDIF, ASMCOP_ENDM, ASMCOP_ENDR,
    ASMCOP_IF, ASMCOP_IF32, ASMCOP_IF64, ASMCOP_IFARCH, ASMCOP_IFB,
    ASMCOP_IFC, ASMCOP_IFDEF, ASMCOP_IFEQ,
    ASMCOP_IFEQS, ASMCOP_IFFMT, ASMCOP_IFGE, ASMCOP_IFGPU, ASMCOP_IFGT, ASMCOP_IFLE,
    ASMCOP_IFLT, ASMCOP_IFNARCH, ASMCOP_IFNB, ASMCOP_IFNC, ASMCOP_IFNDEF,
    ASMCOP_IFNE, ASMCOP_IFNES, ASMCOP_IFNFMT, ASMCOP_IFNGPU, ASMCOP_IFNOTDEF,
    ASMCOP_IRP, ASMCOP_IRPC, ASMCOP_MACRO, ASMCOP_REPT
};

/// pseudo-ops not ignored while putting macro content
enum
{ ASMMROP_ENDM = 0, ASMMROP_ENDR, ASMMROP_IRP, ASMMROP_IRPC, ASMMROP_MACRO, ASMMROP_REPT };

/// all main pseudo-ops
static const char* pseudoOpNamesTbl[] =
{
    "32bit", "64bit", "abort", "align", "altmacro",
    "amd", "amdcl2", "arch", "ascii", "asciz",
    "balign", "balignl", "balignw", "buggyfplit", "byte",
    "data", "double", "else",
    "elseif", "elseif32", "elseif64",
    "elseifarch", "elseifb", "elseifc", "elseifdef",
    "elseifeq", "elseifeqs", "elseiffmt",
    "elseifge", "elseifgpu", "elseifgt",
    "elseifle", "elseiflt", "elseifnarch", "elseifnb",
    "elseifnc", "elseifndef", "elseifne", "elseifnes",
    "elseifnfmt", "elseifngpu", "elseifnotdef",
    "end", "endif", "endm",
    "endr", "equ", "equiv", "eqv",
    "err", "error", "exitm", "extern",
    "fail", "file", "fill", "fillq",
    "float", "format", "gallium", "global",
    "globl", "gpu", "half", "hword", "if", "if32", "if64",
    "ifarch", "ifb", "ifc", "ifdef", "ifeq",
    "ifeqs", "iffmt", "ifge", "ifgpu", "ifgt", "ifle",
    "iflt", "ifnarch", "ifnb", "ifnc", "ifndef",
    "ifne", "ifnes", "ifnfmt", "ifngpu", "ifnotdef", "incbin",
    "include", "int", "irp", "irpc", "kernel", "lflags",
    "line", "ln", "local", "long",
    "macro", "main", "noaltmacro",
    "nobuggyfplit", "octa", "offset", "org",
    "p2align", "print", "purgem", "quad",
    "rawcode", "register", "rept", "rocm", "rodata",
    "sbttl", "section", "set",
    "short", "single", "size", "skip",
    "space", "string", "string16", "string32",
    "string64", "struct", "text", "title",
    "undef", "version", "warning", "weak", "word"
};

enum
{
    ASMOP_32BIT = 0, ASMOP_64BIT, ASMOP_ABORT, ASMOP_ALIGN, ASMOP_ALTMACRO,
    ASMOP_AMD, ASMOP_AMDCL2, ASMOP_ARCH, ASMOP_ASCII, ASMOP_ASCIZ,
    ASMOP_BALIGN, ASMOP_BALIGNL, ASMOP_BALIGNW, ASMOP_BUGGYFPLIT, ASMOP_BYTE,
    ASMOP_DATA, ASMOP_DOUBLE, ASMOP_ELSE,
    ASMOP_ELSEIF, ASMOP_ELSEIF32, ASMOP_ELSEIF64,
    ASMOP_ELSEIFARCH, ASMOP_ELSEIFB, ASMOP_ELSEIFC, ASMOP_ELSEIFDEF,
    ASMOP_ELSEIFEQ, ASMOP_ELSEIFEQS, ASMOP_ELSEIFFMT,
    ASMOP_ELSEIFGE, ASMOP_ELSEIFGPU, ASMOP_ELSEIFGT,
    ASMOP_ELSEIFLE, ASMOP_ELSEIFLT, ASMOP_ELSEIFNARCH, ASMOP_ELSEIFNB,
    ASMOP_ELSEIFNC, ASMOP_ELSEIFNDEF, ASMOP_ELSEIFNE, ASMOP_ELSEIFNES,
    ASMOP_ELSEIFNFMT, ASMOP_ELSEIFNGPU, ASMOP_ELSEIFNOTDEF,
    ASMOP_END, ASMOP_ENDIF, ASMOP_ENDM,
    ASMOP_ENDR, ASMOP_EQU, ASMOP_EQUIV, ASMOP_EQV,
    ASMOP_ERR, ASMOP_ERROR, ASMOP_EXITM, ASMOP_EXTERN,
    ASMOP_FAIL, ASMOP_FILE, ASMOP_FILL, ASMOP_FILLQ,
    ASMOP_FLOAT, ASMOP_FORMAT, ASMOP_GALLIUM, ASMOP_GLOBAL,
    ASMOP_GLOBL, ASMOP_GPU, ASMOP_HALF, ASMOP_HWORD, ASMOP_IF, ASMOP_IF32, ASMOP_IF64,
    ASMOP_IFARCH, ASMOP_IFB, ASMOP_IFC, ASMOP_IFDEF, ASMOP_IFEQ,
    ASMOP_IFEQS, ASMOP_IFFMT, ASMOP_IFGE, ASMOP_IFGPU, ASMOP_IFGT, ASMOP_IFLE,
    ASMOP_IFLT, ASMOP_IFNARCH, ASMOP_IFNB, ASMOP_IFNC, ASMOP_IFNDEF,
    ASMOP_IFNE, ASMOP_IFNES, ASMOP_IFNFMT, ASMOP_IFNGPU, ASMOP_IFNOTDEF, ASMOP_INCBIN,
    ASMOP_INCLUDE, ASMOP_INT, ASMOP_IRP, ASMOP_IRPC, ASMOP_KERNEL, ASMOP_LFLAGS,
    ASMOP_LINE, ASMOP_LN, ASMOP_LOCAL, ASMOP_LONG,
    ASMOP_MACRO, ASMOP_MAIN, ASMOP_NOALTMACRO,
    ASMOP_NOBUGGYFPLIT, ASMOP_OCTA, ASMOP_OFFSET, ASMOP_ORG,
    ASMOP_P2ALIGN, ASMOP_PRINT, ASMOP_PURGEM, ASMOP_QUAD,
    ASMOP_RAWCODE, ASMOP_REGISTER, ASMOP_REPT, ASMOP_ROCM, ASMOP_RODATA,
    ASMOP_SBTTL, ASMOP_SECTION, ASMOP_SET,
    ASMOP_SHORT, ASMOP_SINGLE, ASMOP_SIZE, ASMOP_SKIP,
    ASMOP_SPACE, ASMOP_STRING, ASMOP_STRING16, ASMOP_STRING32,
    ASMOP_STRING64, ASMOP_STRUCT, ASMOP_TEXT, ASMOP_TITLE,
    ASMOP_UNDEF, ASMOP_VERSION, ASMOP_WARNING, ASMOP_WEAK, ASMOP_WORD
};

namespace CLRX
{

void AsmPseudoOps::setBitness(Assembler& asmr, const char* linePtr, bool _64Bit)
{
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    if (asmr.formatHandler != nullptr)
        asmr.printError(linePtr, "Bitness is already defined");
    else if (asmr.format == BinaryFormat::ROCM)
    {
        if (!_64Bit)
            asmr.printWarning(linePtr, "For ROCm bitness is always 64bit");
    }
    else if (asmr.format != BinaryFormat::AMD && asmr.format != BinaryFormat::GALLIUM &&
        asmr.format != BinaryFormat::AMDCL2)
        asmr.printWarning(linePtr, "Bitness ignored for other formats than "
                "AMD Catalyst, ROCm and GalliumCompute");
    else
        asmr._64bit = (_64Bit);
}

bool AsmPseudoOps::parseFormat(Assembler& asmr, const char*& linePtr, BinaryFormat& format)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* formatPlace = linePtr;
    char formatName[10];
    if (!getNameArg(asmr, 10, formatName, linePtr, "output format type"))
        return false;
    
    toLowerString(formatName);
    if (::strcmp(formatName, "catalyst")==0 || ::strcmp(formatName, "amd")==0)
        format = BinaryFormat::AMD;
    else if (::strcmp(formatName, "amdcl2")==0)
        format = BinaryFormat::AMDCL2;
    else if (::strcmp(formatName, "gallium")==0)
        format = BinaryFormat::GALLIUM;
    else if (::strcmp(formatName, "rocm")==0)
        format = BinaryFormat::ROCM;
    else if (::strcmp(formatName, "raw")==0)
        format = BinaryFormat::RAWCODE;
    else
    {
        asmr.printError(formatPlace, "Unknown output format type");
        return false;
    }
    return true;
}

void AsmPseudoOps::setOutFormat(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    BinaryFormat format;
    skipSpacesToEnd(linePtr, end);
    const char* formatPlace = linePtr;
    if (!parseFormat(asmr, linePtr, format))
        return;
    
    if (checkGarbagesAtEnd(asmr, linePtr))
    {
        if (asmr.formatHandler!=nullptr)
        {
            asmr.printError(formatPlace, "Output format type is already defined");
            return;
        }
        asmr.format = format;
    }
}

void AsmPseudoOps::setGPUDevice(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    char deviceName[64];
    const char* deviceNamePlace = linePtr;
    if (!getNameArg(asmr, 64, deviceName, linePtr, "GPU device name"))
        return;
    try
    {
        GPUDeviceType deviceType = getGPUDeviceTypeFromName(deviceName);
        if (checkGarbagesAtEnd(asmr, linePtr))
            asmr.deviceType = deviceType;
    }
    catch(const Exception& ex)
    { asmr.printError(deviceNamePlace, ex.what()); }
}

void AsmPseudoOps::setGPUArchitecture(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    char deviceName[64];
    const char* archNamePlace = linePtr;
    if (!getNameArg(asmr, 64, deviceName, linePtr, "GPU architecture name"))
        return;
    try
    {
        GPUArchitecture arch = getGPUArchitectureFromName(deviceName);
        GPUDeviceType deviceType = getLowestGPUDeviceTypeFromArchitecture(arch);
        if (checkGarbagesAtEnd(asmr, linePtr))
            asmr.deviceType = deviceType;
    }
    catch(const Exception& ex)
    { asmr.printError(archNamePlace, ex.what()); }
}

void AsmPseudoOps::goToKernel(Assembler& asmr, const char* pseudoOpPlace,
                  const char* linePtr)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    CString kernelName;
    if (!getNameArg(asmr, kernelName, linePtr, "kernel name"))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.goToKernel(pseudoOpPlace, kernelName.c_str());
}

/// isPseudoOp - if true then name is pseudo-operation (we must convert to lower-case)
/// if pseudo-op we just do not try to parse section flags
void AsmPseudoOps::goToSection(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr, bool isPseudoOp)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    CString sectionName;
    if (!getNameArg(asmr, sectionName, linePtr, "section name"))
        return;
    if (isPseudoOp)
        toLowerString(sectionName);
    bool haveFlags = false;
    uint32_t sectFlags = 0;
    AsmSectionType sectType = AsmSectionType::EXTRA_SECTION;
    uint64_t sectionAlign = 0;
    if (!isPseudoOp)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr==',')
        {
            haveFlags = true;
            linePtr++;
        }
    }
    bool good = true;
    if (haveFlags)
    {
        std::string flagsStr;
        skipSpacesToEnd(linePtr, end);
        const char* flagsStrPlace = linePtr;
        if (asmr.parseString(flagsStr, linePtr))
        {
            bool flagsStrIsGood = true;
            for (const char c: flagsStr)
                if (c=='a')
                    sectFlags |= ASMELFSECT_ALLOCATABLE;
                else if (c=='x')
                    sectFlags |= ASMELFSECT_EXECUTABLE;
                else if (c=='w')
                    sectFlags |= ASMELFSECT_WRITEABLE;
                else if (flagsStrIsGood)
                {
                    asmr.printError(flagsStrPlace,
                            "Only 'a', 'w', 'x' is accepted in flags string");
                    flagsStrIsGood = good = false;
                }
        }
        else
            good = false;
        
        bool haveComma;
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {   // section type
            char typeBuf[20];
            skipSpacesToEnd(linePtr, end);
            const char* typePlace = linePtr;
            if (linePtr+1<end && *linePtr=='@' && isAlpha(linePtr[1]))
            {
                linePtr++;
                if (getNameArg(asmr, 20, typeBuf, linePtr, "section type"))
                {
                    toLowerString(typeBuf);
                    if (::strcmp(typeBuf, "progbits")==0)
                        sectType = AsmSectionType::EXTRA_PROGBITS;
                    else if (::strcmp(typeBuf, "note")==0)
                        sectType = AsmSectionType::EXTRA_NOTE;
                    else if (::strcmp(typeBuf, "nobits")==0)
                        sectType = AsmSectionType::EXTRA_NOBITS;
                    else
                    {
                        asmr.printError(typePlace, "Unknown section type");
                        good = false;
                    }
                }
                else
                    good = false;
            }
            else
            {
                asmr.printError(typePlace, "Section type was not preceded by '@'");
                good = false;
            }
        }
    }
    // parse alignment
    skipSpacesToEnd(linePtr, end);
    if (linePtr+6<end && ::strncasecmp(linePtr, "align", 5)==0 && !isAlpha(linePtr[5]))
    {   // if alignment
        linePtr+=5;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr=='=')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            const char* valuePtr = linePtr;
            if (getAbsoluteValueArg(asmr, sectionAlign, linePtr, true))
            {
                if (sectionAlign!=0 && (1ULL<<(63-CLZ64(sectionAlign))) != sectionAlign)
                {
                    asmr.printError(valuePtr, "Alignment must be power of two or zero");
                    good = false;
                }
            }
            else
                good = false;
        }
        else
        {
            asmr.printError(linePtr, "Expected '=' after 'align'");
            good = false;
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (!haveFlags)
        asmr.goToSection(pseudoOpPlace, sectionName.c_str(), sectionAlign);
    else
        asmr.goToSection(pseudoOpPlace, sectionName.c_str(), sectType, sectFlags,
                 sectionAlign);
}

void AsmPseudoOps::goToMain(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.goToMain(pseudoOpPlace);
}

void AsmPseudoOps::includeFile(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string filename, sysfilename;
    const char* namePlace = linePtr;
    if (asmr.parseString(filename, linePtr))
    {
        if (!checkGarbagesAtEnd(asmr, linePtr))
            return;
        sysfilename = filename;
        bool failedOpen = false;
        filesystemPath(sysfilename);
        try
        {
            asmr.includeFile(pseudoOpPlace, sysfilename);
            return;
        }
        catch(const Exception& ex)
        { failedOpen = true; }
        
        for (const CString& incDir: asmr.includeDirs)
        {
            failedOpen = false;
            std::string incDirPath(incDir.c_str());
            filesystemPath(incDirPath);
            try
            {
                asmr.includeFile(pseudoOpPlace, joinPaths(
                            std::string(incDirPath.c_str()), sysfilename));
                break;
            }
            catch(const Exception& ex)
            { failedOpen = true; }
        }
        if (failedOpen)
            asmr.printError(namePlace, (std::string("Include file '") + filename +
                    "' not found or unavailable in any directory").c_str());
    }
}

void AsmPseudoOps::includeBinFile(Assembler& asmr, const char* pseudoOpPlace,
                          const char* linePtr)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    
    if (!asmr.isWriteableSection())
    {
        asmr.printError(pseudoOpPlace,
                        "Writing data into non-writeable section is illegal");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    std::string filename, sysfilename;
    const char* namePlace = linePtr;
    const char* offsetPlace = linePtr;
    const char* countPlace = linePtr;
    uint64_t offset = 0, count = INT64_MAX;
    
    bool good = asmr.parseString(filename, linePtr);
    bool haveComma;
    
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        offsetPlace = linePtr;
        if (getAbsoluteValueArg(asmr, offset, linePtr))
        {
            if (int64_t(offset) < 0)
            {
                asmr.printError(offsetPlace, "Offset is negative!");
                good = false;
            }
        }
        else
            good = false;
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            countPlace = linePtr;
            good &= getAbsoluteValueArg(asmr, count, linePtr);
            if (int64_t(count) < 0)
            {
                asmr.printError(countPlace, "Count bytes is negative!");
                good = false;
            }
        }
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr)) // failed parsing
        return;
    
    if (count == 0)
    {
        asmr.printWarning(namePlace, "Number of bytes is zero, ignoring .incbin");
        return;
    }
    
    std::ifstream ifs;
    sysfilename = filename;
    filesystemPath(sysfilename);
    ifs.open(sysfilename.c_str(), std::ios::binary);
    if (!ifs)
    {
        for (const CString& incDir: asmr.includeDirs)
        {
            std::string incDirPath(incDir.c_str());
            filesystemPath(incDirPath);
            ifs.open(joinPaths(incDirPath.c_str(), sysfilename).c_str(), std::ios::binary);
            if (ifs)
                break;
        }
    }
    if (!ifs)
    {
        asmr.printError(namePlace, (std::string("Binary file '") + filename +
                    "' not found or unavailable in any directory").c_str());
        return;
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
            asmr.printError(namePlace, "Can't read whole needed file content");
            return;
        }
    }
    else
    {   /* for sequential files, likes fifo */
        char tempBuf[256];
        /// first we skipping bytes given in offset
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

void AsmPseudoOps::doFail(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value = 0;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    char buf[50];
    ::memcpy(buf, ".fail ", 6);
    const size_t pos = 6+itocstrCStyle(int64_t(value), buf+6, 50-6);
    ::memcpy(buf+pos, " encountered", 13);
    if (int64_t(value) >= 500)
        asmr.printWarning(pseudoOpPlace, buf);
    else
        asmr.printError(pseudoOpPlace, buf);
}

void AsmPseudoOps::doError(Assembler& asmr, const char* pseudoOpPlace,
                      const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, linePtr))
            return; // error
        if (!checkGarbagesAtEnd(asmr, linePtr))
            return;
        asmr.printError(pseudoOpPlace, outStr.c_str());
    }
    else
        asmr.printError(pseudoOpPlace, ".error encountered");
}

void AsmPseudoOps::doWarning(Assembler& asmr, const char* pseudoOpPlace,
                        const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr != end)
    {
        std::string outStr;
        if (!asmr.parseString(outStr, linePtr))
            return; // error
        if (!checkGarbagesAtEnd(asmr, linePtr))
            return;
        asmr.printWarning(pseudoOpPlace, outStr.c_str());
    }
    else
        asmr.printWarning(pseudoOpPlace, ".warning encountered");
}

template<typename T>
void AsmPseudoOps::putIntegers(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    if (!asmr.isWriteableSection())
    {
        asmr.printError(pseudoOpPlace,
                "Writing data into non-writeable section is illegal");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        const char* exprPlace = linePtr;
        std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr));
        if (expr)
        {
            if (expr->isEmpty()) // empty expression print warning
                asmr.printWarning(linePtr, "No expression, zero has been put");
            
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
                                         asmr.getSourcePos(exprPlace));
                        T out;
                        SLEV(out, value);
                        asmr.putData(sizeof(T), reinterpret_cast<const cxbyte*>(&out));
                    }
                    else
                        asmr.printError(exprPlace, "Expression must be absolute!");
                }
            }
            else // expression, we set target of expression (just data)
            {   /// will be resolved later
                expr->setTarget(AsmExprTarget::dataTarget<T>(
                                asmr.currentSection, asmr.currentOutPos));
                expr.release();
                asmr.reserveData(sizeof(T));
            }
        }
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
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
void AsmPseudoOps::putFloats(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    if (!asmr.isWriteableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Writing data into non-writeable section is illegal");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        UIntType out = 0;
        const char* literalPlace = linePtr;
        if (linePtr != end && *linePtr != ',')
        {
            try
            { out = asmcstrtofCStyleLEV<UIntType>(linePtr, end, linePtr); }
            catch(const ParseException& ex)
            { asmr.printError(literalPlace, ex.what()); }
        }
        else // warning
            asmr.printWarning(literalPlace,
                      "No floating point literal, zero has been put");
        asmr.putData(sizeof(UIntType), reinterpret_cast<const cxbyte*>(&out));
        
    } while (skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::putUInt128s(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    if (!asmr.isWriteableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Writing data into non-writeable section is illegal");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        UInt128 value = { 0, 0 };
        if (linePtr != end && *linePtr != ',')
        {
            const char* literalPlace = linePtr;
            bool negative = false;
            if (*linePtr == '+')
                linePtr++;
            else if (*linePtr == '-')
            {
                negative = true;
                linePtr++;
            }
            try
            { value = cstrtou128CStyle(linePtr, end, linePtr); }
            catch(const ParseException& ex)
            { asmr.printError(literalPlace, ex.what()); }
            if (negative)
            {   // negate value
                value.hi = ~value.hi + (value.lo==0);
                value.lo = -value.lo;
            }
        }
        else // warning
            asmr.printWarning(linePtr, "No 128-bit literal, zero has been put");
        UInt128 out;
        SLEV(out.lo, value.lo);
        SLEV(out.hi, value.hi);
        asmr.putData(16, reinterpret_cast<const cxbyte*>(&out));
    } while (skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::putStrings(Assembler& asmr, const char* pseudoOpPlace,
                      const char* linePtr, bool addZero)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    if (!asmr.isWriteableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Writing data into non-writeable section is illegal");
        return;
    }
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        std::string outStr;
        if (*linePtr != ',')
        {
            if (asmr.parseString(outStr, linePtr))
                asmr.putData(outStr.size()+(addZero), (const cxbyte*)outStr.c_str());
        }
    } while (skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

template<typename T>
void AsmPseudoOps::putStringsToInts(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    if (!asmr.isWriteableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Writing data into non-writeable section is illegal");
        return;
    }
    asmr.initializeOutputFormat();
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        std::string outStr;
        if (*linePtr != ',')
        {
            if (asmr.parseString(outStr, linePtr))
            {
                const size_t strSize = outStr.size()+1;
                T* outData = reinterpret_cast<T*>(
                        asmr.reserveData(sizeof(T)*(strSize)));
                /// put as integer including nul-terminated string
                for (size_t i = 0; i < strSize; i++)
                    SULEV(outData[i], T(outStr[i])&T(0xff));
            }
        }
        
    } while (skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::setSymbol(Assembler& asmr, const char* linePtr, bool reassign,
                 bool baseExpr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* strAtSymName = linePtr;
    CString symName = extractSymName(linePtr, end, false);
    bool good = true;
    if (symName.empty())
    {
        asmr.printError(linePtr, "Expected symbol");
        good = false;
    }
    if (!skipRequiredComma(asmr, linePtr))
        return;
    if (good) // is not so good
        asmr.assignSymbol(symName, strAtSymName, linePtr, reassign, baseExpr);
}

void AsmPseudoOps::setSymbolBind(Assembler& asmr, const char* linePtr, cxbyte bind)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    do {
        const char* symNamePlace = linePtr;
        AsmSymbolEntry* symEntry;
        Assembler::ParseState state = asmr.parseSymbol(linePtr, symEntry, false);
        bool good = (state != Assembler::ParseState::FAILED);
        if (symEntry == nullptr)
        {
            asmr.printError(symNamePlace, "Expected symbol name");
            good = false;
        }
        else if (symEntry->second.regRange)
        {
            asmr.printError(symNamePlace, "Symbol must not be register symbol");
            good = false;
        }
        else if (symEntry->second.base)
        {
            asmr.printError(symNamePlace,
                "Symbol must not be set by .eqv pseudo-op or must be constant");
            good = false;
        }
        
        if (good)
        {
            if (symEntry->first != ".")
                symEntry->second.info = ELF32_ST_INFO(bind,
                      ELF32_ST_TYPE(symEntry->second.info));
            else
                asmr.printWarning(symNamePlace, "Symbol '.' is ignored");
        }
        
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::setSymbolSize(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    AsmSymbolEntry* symEntry;
    Assembler::ParseState state = asmr.parseSymbol(linePtr, symEntry, false);
    bool good = (state != Assembler::ParseState::FAILED);
    if (symEntry == nullptr)
    {
        asmr.printError(symNamePlace, "Expected symbol name");
        good = false;
    }
    if (!skipRequiredComma(asmr, linePtr))
        return;
    // parse size
    uint64_t size;
    good &= getAbsoluteValueArg(asmr, size, linePtr, true);
    bool ignore = false;
    if (symEntry != nullptr)
    {
        if (symEntry->second.base)
        {
            asmr.printError(symNamePlace,
                    "Symbol must not be set by .eqv pseudo-op or must be constant");
            good = false;
        }
        else if (symEntry->second.regRange)
        {
            asmr.printError(symNamePlace, "Symbol must not be register symbol");
            good = false;
        }
        else if (symEntry->first == ".")
        {
            asmr.printWarning(symNamePlace, "Symbol '.' is ignored");
            ignore = true;
        }
    }
    
    if (good && checkGarbagesAtEnd(asmr, linePtr))
        if (!ignore) // ignore if '.'
            symEntry->second.size = size;
}

void AsmPseudoOps::ignoreExtern(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        asmr.skipSymbol(linePtr);
    } while (skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::doFill(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
          bool _64bit)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    
    if (!asmr.isWriteableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Writing data into non-writeable section is illegal");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t repeat = 0, size = 1, value = 0;
    
    const char* reptStr = linePtr;
    bool good = getAbsoluteValueArg(asmr, repeat, linePtr, true);
    
    if (int64_t(repeat) < 0)
        asmr.printWarning(reptStr, "Negative repeat has no effect");
    
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    const char* sizePlace = linePtr;
    const char* fillValuePlace = linePtr;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        sizePlace = linePtr; //
        if (getAbsoluteValueArg(asmr, size, linePtr))
        {
            if (int64_t(size) < 0)
                asmr.printWarning(sizePlace, "Negative size has no effect");
        }
        else
            good = false;
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            fillValuePlace = linePtr;
            good &= getAbsoluteValueArg(asmr, value, linePtr);
        }
    }
    if (int64_t(size) > 0 && int64_t(repeat) > 0 && SSIZE_MAX/size < repeat)
    {
        asmr.printError(pseudoOpPlace, "Product of repeat and size is too big");
        good = false;
    }
    
    cxuint truncBits = std::min(uint64_t(8), size)<<3;
    /* honors old behaviour from original GNU as (just cut to 32-bit values)
     * do not that for .fillq (_64bit=true) */
    if (!_64bit)
        truncBits = std::min(cxuint(32), truncBits);
    if (truncBits != 0 && truncBits < 64) // if print 
        asmr.printWarningForRange(truncBits, value, asmr.getSourcePos(fillValuePlace));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr)) // if parsing failed
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
    // main filling route (slow)
    for (uint64_t r = 0; r < repeat; r++)
    {
        ::memcpy(content, &outValue, valueSize);
        ::memset(content+valueSize, 0, size-valueSize);
        content += size;
    }
}

void AsmPseudoOps::doSkip(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    
    if (!asmr.isAddressableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Change output counter inside non-addressable section is illegal");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t size = 1, value = 0;
    
    const char* sizePlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, size, linePtr);
    if (int64_t(size) < 0)
        asmr.printWarning(sizePlace, "Negative size has no effect");
    
    bool haveComma = false;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    const char* fillValuePlace = linePtr;
    if (haveComma)
    {
        skipSpacesToEnd(linePtr, end);
        fillValuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, value, linePtr))
            asmr.printWarningForRange(8, value, asmr.getSourcePos(fillValuePlace));
        else
            good = false;
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (int64_t(size) < 0)
        return;
    
    if (asmr.currentSection==ASMSECT_ABS && value != 0)
        asmr.printWarning(fillValuePlace, "Fill value is ignored inside absolute section");
    asmr.reserveData(size, value&0xff);
}

void AsmPseudoOps::doAlign(Assembler& asmr, const char* pseudoOpPlace,
                           const char* linePtr, bool powerOf2)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    
    if (!asmr.isAddressableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Change output counter inside non-addressable section is illegal");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t alignment, value = 0, maxAlign = 0;
    const char* alignPlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, alignment, linePtr, true);
    
    if (good)
    {
        if (powerOf2)
        {
            if (alignment > 63)
            {
                asmr.printError(alignPlace, "Power of 2 of alignment is greater than 63");
                good = false;
            }
            else
                alignment = (1ULL<<alignment);
        }
        else if (alignment == 0 || (1ULL<<(63-CLZ64(alignment))) != alignment)
        {
            asmr.printError(alignPlace, "Alignment is not power of 2");
            good = false;
        }
    }
    
    bool haveValue = false;
    if (!skipComma(asmr, haveValue, linePtr))
        return;
    const char* valuePlace = linePtr;
    if (haveValue)
    {
        skipSpacesToEnd(linePtr, end);
        valuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, value, linePtr))
            asmr.printWarningForRange(8, value, asmr.getSourcePos(valuePlace));
        else
            good = false;
        
        bool haveComma = false;
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            good &= getAbsoluteValueArg(asmr, maxAlign, linePtr);
        }
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr)) //if parsing failed
        return;
    
    uint64_t outPos = asmr.currentOutPos;
    const uint64_t bytesToFill = ((outPos&(alignment-1))!=0) ?
            alignment - (outPos&(alignment-1)) : 0;
    if (maxAlign!=0 && bytesToFill > maxAlign)
        return; // do not make alignment
    
    if (asmr.currentSection==ASMSECT_ABS && value != 0)
        asmr.printWarning(valuePlace, "Fill value is ignored inside absolute section");
    
    if (haveValue || asmr.sections[asmr.currentSection].type != AsmSectionType::CODE)
        asmr.reserveData(bytesToFill, value&0xff);
    else /* only if no value and is code section */
    {
        cxbyte* output = asmr.reserveData(bytesToFill, 0);
        asmr.isaAssembler->fillAlignment(bytesToFill, output);
    }
}

template<typename Word>
void AsmPseudoOps::doAlignWord(Assembler& asmr, const char* pseudoOpPlace,
                       const char* linePtr)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    
    if (!asmr.isAddressableSection())
    {
        asmr.printError(pseudoOpPlace,
                    "Change output counter inside non-addressable section is illegal");
        return;
    }
    
    skipSpacesToEnd(linePtr, end);
    uint64_t alignment, value = 0, maxAlign = 0;
    const char* alignPlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, alignment, linePtr, true);
    if (good && alignment != 0 && (1ULL<<(63-CLZ64(alignment))) != alignment)
    {
        asmr.printError(alignPlace, "Alignment is not power of 2");
        good = false;
    }
    
    bool haveValue = false;
    if (!skipComma(asmr, haveValue, linePtr))
        return;
    const char* valuePlace = linePtr;
    if (haveValue)
    {
        skipSpacesToEnd(linePtr, end);
        valuePlace = linePtr;
        if (getAbsoluteValueArg(asmr, value, linePtr))
            asmr.printWarningForRange(sizeof(Word)<<3, value,
                          asmr.getSourcePos(valuePlace));
        else
            good = false;
        
        bool haveComma = false;
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            good &= getAbsoluteValueArg(asmr, maxAlign, linePtr);
        }
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (alignment == 0)
        return; // do nothing
    
    uint64_t outPos = asmr.currentOutPos;
    if (outPos&(sizeof(Word)-1))
    {
        asmr.printError(pseudoOpPlace, "Offset is not aligned to word");
        return;
    }
    
    const uint64_t bytesToFill = ((outPos&(alignment-1))!=0) ?
            alignment - (outPos&(alignment-1)) : 0;
    
    if (maxAlign!=0 && bytesToFill > maxAlign)
        return; // do not make alignment
    
    if (asmr.currentSection==ASMSECT_ABS && value != 0)
    {
        asmr.printWarning(valuePlace, "Fill value is ignored inside absolute section");
        asmr.reserveData(bytesToFill);
        return;
    }
    cxbyte* content = asmr.reserveData(bytesToFill);
    if (haveValue)
    {
        Word word;
        SLEV(word, value);
        std::fill(reinterpret_cast<Word*>(content),
                  reinterpret_cast<Word*>(content + bytesToFill), word);
    }
    else if (asmr.sections[asmr.currentSection].type == AsmSectionType::CODE)
        asmr.isaAssembler->fillAlignment(bytesToFill, content);
}

void AsmPseudoOps::doOrganize(Assembler& asmr, const char* linePtr)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    cxuint sectionId = ASMSECT_ABS;
    const char* valuePlace = linePtr;
    bool good = getAnyValueArg(asmr, value, sectionId, linePtr);
    
    uint64_t fillValue = 0;
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    const char* fillValuePlace = linePtr;
    if (haveComma) // optional fill argument
    {
        skipSpacesToEnd(linePtr, end);
        fillValuePlace = linePtr;
        good = getAbsoluteValueArg(asmr, fillValue, linePtr, true);
    }
    asmr.printWarningForRange(8, fillValue, asmr.getSourcePos(fillValuePlace));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.assignOutputCounter(valuePlace, value, sectionId, fillValue);
}

void AsmPseudoOps::doPrint(Assembler& asmr, const char* linePtr)
{
    std::string outStr;
    if (!asmr.parseString(outStr, linePtr))
        return;
    if (!AsmPseudoOps::checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.printStream.write(outStr.c_str(), outStr.size());
    asmr.printStream.put('\n');
}

/// perform .if for integer comparisons '.iflt', '.ifle', '.ifne'
void AsmPseudoOps::doIfInt(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
               IfIntComp compType, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
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
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfDef(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
               bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    AsmSymbolEntry* entry;
    bool good = true;
    Assembler::ParseState state = asmr.parseSymbol(linePtr, entry, false, true);
    if (state == Assembler::ParseState::FAILED)
        return;
    if (state == Assembler::ParseState::MISSING)
    {
        asmr.printError(symNamePlace, "Expected symbol");
        good = false;
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    const bool symDefined = (entry!=nullptr && (entry->second.hasValue ||
                entry->second.expression!=nullptr));
    bool satisfied = (!negation) ?  symDefined : !symDefined;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfBlank(Assembler& asmr, const char* pseudoOpPlace,
             const char* linePtr, bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? linePtr==end : linePtr!=end;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
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
    cxbyte prevTok = 0;
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
            
            /* original GNU as tokenize line before processing, this code 'emulates'
             * this operation */
            cxbyte thisTok = (cxbyte(*s) >= 0x20 && cxbyte(*s) <= 0x80) ?
                    tokenCharTable[*s-0x20] : 0;
            if (!singleQuote && !dblQuote && !firstStr.empty() &&
                isSpace(firstStr.back()) &&
                ((prevTok != thisTok) || ((prevTok == thisTok) && (prevTok & 0x80)==0)))
                firstStr.pop_back();// delete space between different tokens
            
            firstStr.push_back(*s);
            prevTok = thisTok;
        }
    if (!firstStr.empty() && isSpace(firstStr.back()))
        firstStr.pop_back(); // remove last space
    return firstStr;
}

void AsmPseudoOps::doIfCmpStr(Assembler& asmr, const char* pseudoOpPlace,
               const char* linePtr, bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* firstStrStart = linePtr;
    bool good = true;
    while (linePtr != end && *linePtr != ',') linePtr++;
    if (linePtr == end)
    {
        asmr.printError(linePtr, "Missing second string");
        return;
    }
    const char* firstStrEnd = linePtr;
    if (good) linePtr++; // comma
    else return;
    
    std::string firstStr = getStringToCompare(firstStrStart, firstStrEnd);
    std::string secondStr = getStringToCompare(linePtr, end);
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? firstStr==secondStr : firstStr!=secondStr;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfStrEqual(Assembler& asmr, const char* pseudoOpPlace,
                const char* linePtr, bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string firstStr, secondStr;
    bool good = asmr.parseString(firstStr, linePtr);
    if (!skipRequiredComma(asmr, linePtr))
        return;
    skipSpacesToEnd(linePtr, end);
    good &= asmr.parseString(secondStr, linePtr);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? firstStr==secondStr : firstStr!=secondStr;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIf64Bit(Assembler& asmr, const char* pseudoOpPlace,
                const char* linePtr, bool negation, bool elseIfClause)
{
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? asmr._64bit : !asmr._64bit;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfArch(Assembler& asmr, const char* pseudoOpPlace,
            const char* linePtr, bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    char deviceName[64];
    const char* archNamePlace = linePtr;
    if (!getNameArg(asmr, 64, deviceName, linePtr, "GPU architecture name"))
        return;
    GPUArchitecture arch;
    try
    {
        arch = getGPUArchitectureFromName(deviceName);
        if (!checkGarbagesAtEnd(asmr, linePtr))
            return;
    }
    catch(const Exception& ex)
    { 
        asmr.printError(archNamePlace, ex.what());
        return;
    }
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    GPUArchitecture curArch = getGPUArchitectureFromDeviceType(asmr.getDeviceType());
    bool satisfied = (!negation) ? arch==curArch : arch!=curArch;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfGpu(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
            bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    char deviceName[64];
    const char* deviceNamePlace = linePtr;
    if (!getNameArg(asmr, 64, deviceName, linePtr, "GPU device name"))
        return;
    GPUDeviceType deviceType;
    try
    {
        deviceType = getGPUDeviceTypeFromName(deviceName);
        if (!checkGarbagesAtEnd(asmr, linePtr))
            return;
    }
    catch(const Exception& ex)
    {
        asmr.printError(deviceNamePlace, ex.what());
        return;
    }
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? deviceType==asmr.deviceType :
                deviceType!=asmr.deviceType;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doIfFmt(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
            bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    BinaryFormat format;
    if (!parseFormat(asmr, linePtr, format))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    bool satisfied = (!negation) ? format==asmr.format: format!=asmr.format;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::doElse(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr)
{
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    bool included;
    if (asmr.pushClause(pseudoOpPlace, AsmClauseType::ELSE, true, included))
    {
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

void AsmPseudoOps::endIf(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr)
{
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.popClause(pseudoOpPlace, AsmClauseType::IF);
}

void AsmPseudoOps::doRepeat(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t repeatsNum;
    bool good = getAbsoluteValueArg(asmr, repeatsNum, linePtr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (asmr.repetitionLevel == 1000)
    {
        asmr.printError(pseudoOpPlace, "Repetition level is greater than 1000");
        return;
    }
    asmr.pushClause(pseudoOpPlace, AsmClauseType::REPEAT);
    if (repeatsNum == 0)
    {   /* skip it */
        asmr.skipClauses();
        return;
    }
    /* create repetition (even if only 1 - for correct source position included
     * to messages from repetition) */
    std::unique_ptr<AsmRepeat> repeat(new AsmRepeat(
                asmr.getSourcePos(pseudoOpPlace), repeatsNum));
    if (asmr.putRepetitionContent(*repeat))
    {   // and input stream filter
        std::unique_ptr<AsmInputFilter> newInputFilter(
                    new AsmRepeatInputFilter(repeat.release()));
        asmr.asmInputFilters.push(newInputFilter.release());
        asmr.currentInputFilter = asmr.asmInputFilters.top();
        asmr.repetitionLevel++;
    }
}

void AsmPseudoOps::endRepeat(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr)
{
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.popClause(pseudoOpPlace, AsmClauseType::REPEAT);
}

void AsmPseudoOps::doMacro(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* macroNamePlace = linePtr;
    CString macroName = extractSymName(linePtr, end, false);
    if (macroName.empty())
    {
        asmr.printError(macroNamePlace, "Expected macro name");
        return;
    }
    toLowerString(macroName);
    /* parse args */
    std::vector<AsmMacroArg> args;
    
    bool good = true;
    bool haveVarArg = false;
    
    if (asmr.macroMap.find(macroName) != asmr.macroMap.end())
    {
        asmr.printError(macroNamePlace, (std::string("Macro '") + macroName.c_str() +
                "' is already defined").c_str());
        good = false;
    }
    
    {
    std::unordered_set<CString> macroArgSet;
    while(linePtr != end)
    {
        skipSpacesToEnd(linePtr, end);
        if (linePtr != end && *linePtr == ',')
            skipCharAndSpacesToEnd(linePtr, end);
        const char* argPlace = linePtr;
        CString argName = extractSymName(linePtr, end, false);
        if (argName.empty())
        {
            asmr.printError(argPlace, "Expected macro argument name");
            return; //
        }
        bool argRequired = false;
        bool argVarArgs = false;
        bool argGood = true;
        std::string defaultArgValue;
        
        if (!macroArgSet.insert(argName).second)
        {   // duplicate!
            asmr.printError(argPlace, (std::string("Duplicated macro argument '")+
                    argName.c_str()+'\'').c_str());
            argGood = false;
        }
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr != end && *linePtr == ':')
        {   // qualifier
            skipCharAndSpacesToEnd(linePtr, end);
            //extr
            if (linePtr+3 <= end && linePtr[0] == 'r' && linePtr[1] == 'e' &&
                    linePtr[2] == 'q') // required
            {
                argRequired = true;
                linePtr += 3;
            }
            else if (linePtr+6 <= end && ::memcmp(linePtr, "vararg", 6)==0) // required
            {
                argVarArgs = true;
                linePtr += 6;
            }
            else
            {   // otherwise
                asmr.printError(linePtr, "Expected qualifier 'req' or 'vararg'");
                argGood = false;
            }
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr != end && *linePtr == '=')
        {   // parse default value
            skipCharAndSpacesToEnd(linePtr, end);
            const char* defaultValueStr = linePtr;
            if (!asmr.parseMacroArgValue(linePtr, defaultArgValue))
            {
                good = false;
                continue; // error
            }
            if (argRequired)
                asmr.printWarning(defaultValueStr, (std::string(
                        "Pointless default value for argument '") +
                        argName.c_str()+'\'').c_str());
        }
        
        if (argGood) // push to arguments
        {
            if (haveVarArg)
            {
                asmr.printError(argPlace, "Variadic argument must be last");
                good = false;
            }
            else
                haveVarArg = argVarArgs;
        }
        else // not good
            good = false;
        
        if (argGood) // push argument
            args.push_back({argName, defaultArgValue, argVarArgs, argRequired});
    }
    }
    if (good)
    {   
        if (checkPseudoOpName(macroName))
        {   // ignore
            asmr.printWarning(pseudoOpPlace, (std::string(
                        "Attempt to redefine pseudo-op '")+macroName.c_str()+
                        "' as macro. Ignoring it...").c_str());
            asmr.pushClause(pseudoOpPlace, AsmClauseType::MACRO);
            asmr.skipClauses();
            return;
        }
        if (asmr.checkReservedName(macroName))
            asmr.printWarning(pseudoOpPlace, (std::string(
                    "Attempt to redefine instruction or prefix '")+macroName.c_str()+
                    "' as macro.").c_str());
        // create a macro
        RefPtr<const AsmMacro> macro(new AsmMacro(asmr.getSourcePos(pseudoOpPlace),
                        Array<AsmMacroArg>(args.begin(), args.end())));
        asmr.pushClause(pseudoOpPlace, AsmClauseType::MACRO);
        if (!asmr.putMacroContent(macro.constCast<AsmMacro>()))
            return;
        asmr.macroMap.insert(std::make_pair(std::move(macroName), std::move(macro)));
    }
}

void AsmPseudoOps::endMacro(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr)
{
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.popClause(pseudoOpPlace, AsmClauseType::MACRO);
}

void AsmPseudoOps::exitMacro(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr)
{
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const AsmInputFilterType type = asmr.currentInputFilter->getType();
    if (type == AsmInputFilterType::STREAM)
        asmr.printWarning(pseudoOpPlace, "'.exitm' is ignored outside macro content");
    else
    {
        if (type == AsmInputFilterType::REPEAT)
            asmr.printWarning(pseudoOpPlace, "Behavior of '.exitm' inside repeat is "
                    "undefined. Exiting from repeat...");
        asmr.skipClauses(true);
    }
}

void AsmPseudoOps::doIRP(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
              bool perChar)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    CString symName = extractSymName(linePtr, end, false);
    if (symName.empty())
    {
        asmr.printError(symNamePlace, "Expected argument name");
        return;
    }
    /* parse args */
    std::vector<CString> symValues;
    std::string symValString;
    
    bool good = true;
    skipSpacesToEnd(linePtr, end);
    if (linePtr != end && *linePtr == ',')
        skipCharAndSpacesToEnd(linePtr, end);
    
    while(linePtr != end)
    {
        if (linePtr != end && *linePtr == ',')
        {
            if (perChar)
                symValString.push_back(',');
            skipCharAndSpacesToEnd(linePtr, end);
        }
        std::string symValue;
        if (!asmr.parseMacroArgValue(linePtr, symValue))
        {
            good = false;
            continue; // error
        }
        skipSpacesToEnd(linePtr, end);
        if (!perChar)
            symValues.push_back(symValue);
        else
            symValString += symValue;
    }
    
    if (asmr.repetitionLevel == 1000)
    {
        asmr.printError(pseudoOpPlace, "Repetition level is greater than 1000");
        return;
    }
    
    if (symValues.empty())
        symValues.push_back("");
    if (good)
    {
        asmr.pushClause(pseudoOpPlace, AsmClauseType::REPEAT);
        std::unique_ptr<AsmIRP> repeat;
        if (!perChar)
            repeat.reset(new AsmIRP(asmr.getSourcePos(pseudoOpPlace),
                      symName, Array<CString>(symValues.begin(), symValues.end())));
        else // per char
            repeat.reset(new AsmIRP(asmr.getSourcePos(pseudoOpPlace),
                      symName, symValString));
        
        if (asmr.putRepetitionContent(*repeat))
        {   // and input stream filter
            std::unique_ptr<AsmInputFilter> newInputFilter(
                        new AsmIRPInputFilter(repeat.release()));
            asmr.asmInputFilters.push(newInputFilter.release());
            asmr.currentInputFilter = asmr.asmInputFilters.top();
            asmr.repetitionLevel++;
        }
    }
}

void AsmPseudoOps::purgeMacro(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* macroNamePlace = linePtr;
    CString macroName = extractSymName(linePtr, end, false);
    bool good = true;
    if (macroName.empty())
    {
        asmr.printError(macroNamePlace, "Expected macro name");
        good = false;
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    toLowerString(macroName); // macro name is lowered
    if (!asmr.macroMap.erase(macroName))
        asmr.printWarning(macroNamePlace, (std::string("Macro '")+macroName.c_str()+
                "' already doesn't exist").c_str());
}

void AsmPseudoOps::undefSymbol(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    CString symName = extractSymName(linePtr, end, false);
    bool good = true;
    if (symName.empty())
    {
        asmr.printError(symNamePlace, "Expected symbol name");
        good = false;
    }
    else if (symName == ".")
    {
        asmr.printError(symNamePlace, "Symbol '.' can not be undefined");
        good = false;
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    auto it = asmr.symbolMap.find(symName);
    if (it == asmr.symbolMap.end() ||
        (!it->second.hasValue && it->second.expression==nullptr))
        asmr.printWarning(symNamePlace, (std::string("Symbol '") + symName.c_str() +
                "' already doesn't exist").c_str());
    else if (it->second.occurrencesInExprs.empty())
        asmr.symbolMap.erase(it);
    else
        it->second.undefine();
}

void AsmPseudoOps::setAbsoluteOffset(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    asmr.initializeOutputFormat();
    skipSpacesToEnd(linePtr, end);
    uint64_t value = 0;
    bool good = getAbsoluteValueArg(asmr, value, linePtr, true);
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.currentSection = ASMSECT_ABS;
    asmr.currentOutPos = value;
}

void AsmPseudoOps::doDefRegVar(Assembler& asmr, const char* pseudoOpPlace,
                       const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    asmr.initializeOutputFormat();
    
    if (!asmr.isWriteableSection() || !asmr.isAddressableSection())
    {
        asmr.printError(pseudoOpPlace, "RegVar definition can be in "
                    "writable and addressable section");
        return;
    }
    AsmSection& section = asmr.sections[asmr.currentSection];
    
    do {
        skipSpacesToEnd(linePtr, end);
        const char* regNamePlace = linePtr;
        CString name = extractSymName(linePtr, end, false);
        bool good = true;
        if (name.empty())
        {
            asmr.printError(regNamePlace, "Expected reg-var name");
            good = false;
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!=':')
        {
            asmr.printError(linePtr, "Expected colon after reg-var");
            continue;
        }
        linePtr++;
        skipSpacesToEnd(linePtr, end);
        AsmRegVar var = { 0, 1 };
        if (!asmr.isaAssembler->parseRegisterType(linePtr, end, var.type))
        {
            asmr.printError(linePtr, "Expected name of register type");
            good = false;
        }
        skipSpacesToEnd(linePtr, end);
        
        if (linePtr!=end && *linePtr!=',')
        {
            if (*linePtr!=':')
            {
                asmr.printError(linePtr, "Expected colon after reg-var");
                continue;
            }
            linePtr++;
            skipSpacesToEnd(linePtr, end);
            uint64_t regSize;
            if (!getAbsoluteValueArg(asmr, regSize, linePtr, true))
                continue;
            GPUArchitecture arch = getGPUArchitectureFromDeviceType(asmr.deviceType);
            if (regSize==0)
            {
                asmr.printError(linePtr, "Size of reg-var is zero");
                good = false;
            }
            if (regSize>getGPUMaxRegistersNum(arch, regSize, 0))
            {
                asmr.printError(linePtr, "Size of reg-var out of max number of registers");
                good = false;
            }
            var.size = regSize;
        }
        
        if (!good)
            continue;
        
        if (!section.addRegVar(name, var))
            asmr.printError(regNamePlace, (std::string("Reg-var '")+name.c_str()+
                        "' was already defined").c_str());
        
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::ignoreString(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (asmr.parseString(out, linePtr))
        checkGarbagesAtEnd(asmr, linePtr);
}

bool AsmPseudoOps::checkPseudoOpName(const CString& string)
{
    if (string.empty() || string[0] != '.')
        return false;
    const size_t pseudoOp = binaryFind(pseudoOpNamesTbl, pseudoOpNamesTbl +
                    sizeof(pseudoOpNamesTbl)/sizeof(char*), string.c_str()+1,
                   CStringLess()) - pseudoOpNamesTbl;
    if (pseudoOp < sizeof(pseudoOpNamesTbl)/sizeof(char*))
        return true;
    if (AsmGalliumPseudoOps::checkPseudoOpName(string))
        return true;
    if (AsmAmdPseudoOps::checkPseudoOpName(string))
        return true;
    if (AsmAmdCL2PseudoOps::checkPseudoOpName(string))
        return true;
    if (AsmROCmPseudoOps::checkPseudoOpName(string))
        return true;
    return false;
}

};

void Assembler::parsePseudoOps(const CString& firstName,
       const char* stmtPlace, const char* linePtr)
{
    const size_t pseudoOp = binaryFind(pseudoOpNamesTbl, pseudoOpNamesTbl +
                    sizeof(pseudoOpNamesTbl)/sizeof(char*), firstName.c_str()+1,
                   CStringLess()) - pseudoOpNamesTbl;
    
    switch(pseudoOp)
    {
        case ASMOP_32BIT:
        case ASMOP_64BIT:
            AsmPseudoOps::setBitness(*this, linePtr, pseudoOp == ASMOP_64BIT);
            break;
        case ASMOP_ABORT:
            printError(stmtPlace, "Aborted!");
            endOfAssembly = true;
            break;
        case ASMOP_ALIGN:
        case ASMOP_BALIGN:
            AsmPseudoOps::doAlign(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ALTMACRO:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                alternateMacro = true;
            break;
        case ASMOP_ARCH:
            AsmPseudoOps::setGPUArchitecture(*this, linePtr);
            break;
        case ASMOP_ASCII:
            AsmPseudoOps::putStrings(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ASCIZ:
            AsmPseudoOps::putStrings(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_BALIGNL:
            AsmPseudoOps::doAlignWord<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_BALIGNW:
            AsmPseudoOps::doAlignWord<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_BUGGYFPLIT:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                buggyFPLit = true;
            break;
        case ASMOP_BYTE:
            AsmPseudoOps::putIntegers<cxbyte>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_AMD:
        case ASMOP_AMDCL2:
        case ASMOP_RAWCODE:
        case ASMOP_GALLIUM:
        case ASMOP_ROCM:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
            {
                if (formatHandler!=nullptr)
                    printError(linePtr, "Output format type is already defined");
                else
                    format = (pseudoOp == ASMOP_GALLIUM) ? BinaryFormat::GALLIUM :
                        (pseudoOp == ASMOP_AMD) ? BinaryFormat::AMD :
                        (pseudoOp == ASMOP_AMDCL2) ? BinaryFormat::AMDCL2 :
                        (pseudoOp == ASMOP_ROCM) ? BinaryFormat::ROCM :
                        BinaryFormat::RAWCODE;
            }
            break;
        case ASMOP_DATA:
            AsmPseudoOps::goToSection(*this, stmtPlace, stmtPlace, true);
            break;
        case ASMOP_DOUBLE:
            AsmPseudoOps::putFloats<uint64_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ELSE:
            AsmPseudoOps::doElse(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ELSEIF:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, true);
            break;
        case ASMOP_ELSEIF32:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIF64:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFEQ:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::EQUAL, true);
            break;
        case ASMOP_ELSEIFEQS:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFGE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER_EQUAL, true);
            break;
        case ASMOP_ELSEIFGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, false, true);
            break;
        case ASMOP_ELSEIFGT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER, true);
            break;
        case ASMOP_ELSEIFLE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS_EQUAL, true);
            break;
        case ASMOP_ELSEIFLT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS, true);
            break;
        case ASMOP_ELSEIFNARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNOTDEF:
        case ASMOP_ELSEIFNDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, true);
            break;
        case ASMOP_ELSEIFNES:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_ELSEIFNGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, true, true);
            break;
        case ASMOP_END:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                endOfAssembly = true;
            break;
        case ASMOP_ENDIF:
            AsmPseudoOps::endIf(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ENDM:
            AsmPseudoOps::endMacro(*this, stmtPlace, linePtr);
            break;
        case ASMOP_ENDR:
            AsmPseudoOps::endRepeat(*this, stmtPlace, linePtr);
            break;
        case ASMOP_EQU:
        case ASMOP_SET:
            AsmPseudoOps::setSymbol(*this, linePtr);
            break;
        case ASMOP_EQUIV:
            AsmPseudoOps::setSymbol(*this, linePtr, false);
            break;
        case ASMOP_EQV:
            AsmPseudoOps::setSymbol(*this, linePtr, false, true);
            break;
        case ASMOP_ERR:
            printError(stmtPlace, ".err encountered");
            break;
        case ASMOP_ERROR:
            AsmPseudoOps::doError(*this, stmtPlace, linePtr);
            break;
        case ASMOP_EXITM:
            AsmPseudoOps::exitMacro(*this, stmtPlace, linePtr);
            break;
        case ASMOP_EXTERN:
            AsmPseudoOps::ignoreExtern(*this, linePtr);
            break;
        case ASMOP_FAIL:
            AsmPseudoOps::doFail(*this, stmtPlace, linePtr);
            break;
        case ASMOP_FILE:
            printWarning(stmtPlace, "'.file' is ignored by this assembler.");
            break;
        case ASMOP_FILL:
            AsmPseudoOps::doFill(*this, stmtPlace, linePtr, false);
            break;
        case ASMOP_FILLQ:
            AsmPseudoOps::doFill(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_FLOAT:
            AsmPseudoOps::putFloats<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_FORMAT:
            AsmPseudoOps::setOutFormat(*this, linePtr);
            break;
        case ASMOP_GLOBAL:
        case ASMOP_GLOBL:
            AsmPseudoOps::setSymbolBind(*this, linePtr, STB_GLOBAL);
            break;
        case ASMOP_GPU:
            AsmPseudoOps::setGPUDevice(*this, linePtr);
            break;
        case ASMOP_HALF:
            AsmPseudoOps::putFloats<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_HWORD:
            AsmPseudoOps::putIntegers<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_IF:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, false);
            break;
        case ASMOP_IF32:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IF64:
            AsmPseudoOps::doIf64Bit(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFEQ:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::EQUAL, false);
            break;
        case ASMOP_IFEQS:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFGE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER_EQUAL, false);
            break;
        case ASMOP_IFGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, false, false);
            break;
        case ASMOP_IFGT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::GREATER, false);
            break;
        case ASMOP_IFLE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS_EQUAL, false);
            break;
        case ASMOP_IFLT:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::LESS, false);
            break;
        case ASMOP_IFNARCH:
            AsmPseudoOps::doIfArch(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNB:
            AsmPseudoOps::doIfBlank(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNC:
            AsmPseudoOps::doIfCmpStr(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNDEF:
        case ASMOP_IFNOTDEF:
            AsmPseudoOps::doIfDef(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNE:
            AsmPseudoOps::doIfInt(*this, stmtPlace, linePtr,
                      IfIntComp::NOT_EQUAL, false);
            break;
        case ASMOP_IFNES:
            AsmPseudoOps::doIfStrEqual(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNFMT:
            AsmPseudoOps::doIfFmt(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_IFNGPU:
            AsmPseudoOps::doIfGpu(*this, stmtPlace, linePtr, true, false);
            break;
        case ASMOP_INCBIN:
            AsmPseudoOps::includeBinFile(*this, stmtPlace, linePtr);
            break;
        case ASMOP_INCLUDE:
            AsmPseudoOps::includeFile(*this, stmtPlace, linePtr);
            break;
        case ASMOP_IRP:
            AsmPseudoOps::doIRP(*this, stmtPlace, linePtr, false);
            break;
        case ASMOP_IRPC:
            AsmPseudoOps::doIRP(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_INT:
        case ASMOP_LONG:
            AsmPseudoOps::putIntegers<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_KERNEL:
            AsmPseudoOps::goToKernel(*this, stmtPlace, linePtr);
            break;
        case ASMOP_LFLAGS:
            printWarning(stmtPlace, "'.lflags' is ignored by this assembler.");
            break;
        case ASMOP_LINE:
        case ASMOP_LN:
            printWarning(stmtPlace, "'.line' is ignored by this assembler.");
            break;
        case ASMOP_LOCAL:
            AsmPseudoOps::setSymbolBind(*this, linePtr, STB_LOCAL);
            break;
        case ASMOP_MACRO:
            AsmPseudoOps::doMacro(*this, stmtPlace, linePtr);
            break;
        case ASMOP_MAIN:
            AsmPseudoOps::goToMain(*this, stmtPlace, linePtr);
            break;
        case ASMOP_NOALTMACRO:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                alternateMacro = false;
            break;
        case ASMOP_NOBUGGYFPLIT:
            if (AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                buggyFPLit = false;
            break;
        case ASMOP_OCTA:
            AsmPseudoOps::putUInt128s(*this, stmtPlace, linePtr);
            break;
        case ASMOP_OFFSET:
            AsmPseudoOps::setAbsoluteOffset(*this, linePtr);
            break;
        case ASMOP_ORG:
            AsmPseudoOps::doOrganize(*this, linePtr);
            break;
        case ASMOP_P2ALIGN:
            AsmPseudoOps::doAlign(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_PRINT:
            AsmPseudoOps::doPrint(*this, linePtr);
            break;
        case ASMOP_PURGEM:
            AsmPseudoOps::purgeMacro(*this, linePtr);
            break;
        case ASMOP_QUAD:
            AsmPseudoOps::putIntegers<uint64_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_REGISTER:
            AsmPseudoOps::doDefRegVar(*this, stmtPlace, linePtr);
            break;
        case ASMOP_REPT:
            AsmPseudoOps::doRepeat(*this, stmtPlace, linePtr);
            break;
        case ASMOP_RODATA:
            AsmPseudoOps::goToSection(*this, stmtPlace, stmtPlace, true);
            break;
        case ASMOP_SECTION:
            AsmPseudoOps::goToSection(*this, stmtPlace, linePtr);
            break;
        case ASMOP_SHORT:
            AsmPseudoOps::putIntegers<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_SINGLE:
            AsmPseudoOps::putFloats<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_SIZE:
            AsmPseudoOps::setSymbolSize(*this, linePtr);
            break;
        case ASMOP_SKIP:
        case ASMOP_SPACE:
            AsmPseudoOps::doSkip(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRING:
            AsmPseudoOps::putStrings(*this, stmtPlace, linePtr, true);
            break;
        case ASMOP_STRING16:
            AsmPseudoOps::putStringsToInts<uint16_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRING32:
            AsmPseudoOps::putStringsToInts<uint32_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRING64:
            AsmPseudoOps::putStringsToInts<uint64_t>(*this, stmtPlace, linePtr);
            break;
        case ASMOP_STRUCT:
            AsmPseudoOps::setAbsoluteOffset(*this, linePtr);
            break;
        case ASMOP_TEXT:
            AsmPseudoOps::goToSection(*this, stmtPlace, stmtPlace, true);
            break;
        case ASMOP_SBTTL:
        case ASMOP_TITLE:
        case ASMOP_VERSION:
            AsmPseudoOps::ignoreString(*this, linePtr);
            break;
        case ASMOP_UNDEF:
            AsmPseudoOps::undefSymbol(*this, linePtr);
            break;
        case ASMOP_WARNING:
            AsmPseudoOps::doWarning(*this, stmtPlace, linePtr);
            break;
        case ASMOP_WEAK:
            AsmPseudoOps::setSymbolBind(*this, linePtr, STB_WEAK);
            break;
        case ASMOP_WORD:
            AsmPseudoOps::putIntegers<uint32_t>(*this, stmtPlace, linePtr);
            break;
        default:
        {
            bool isGalliumPseudoOp = AsmGalliumPseudoOps::checkPseudoOpName(firstName);
            bool isAmdPseudoOp = AsmAmdPseudoOps::checkPseudoOpName(firstName);
            bool isAmdCL2PseudoOp = AsmAmdCL2PseudoOps::checkPseudoOpName(firstName);
            bool isROCmPseudoOp = AsmROCmPseudoOps::checkPseudoOpName(firstName);
            if (isGalliumPseudoOp || isAmdPseudoOp || isAmdCL2PseudoOp || isROCmPseudoOp)
            {   // initialize only if gallium pseudo-op or AMD pseudo-op
                initializeOutputFormat();
                /// try to parse
                if (!formatHandler->parsePseudoOp(firstName, stmtPlace, linePtr))
                {   // check other
                    if (format != BinaryFormat::GALLIUM)
                    {   // check gallium pseudo-op
                        if (isGalliumPseudoOp)
                        {
                            printError(stmtPlace, "Gallium pseudo-op can be defined "
                                    "only in Gallium format code");
                            break;
                        }
                    }
                    if (format != BinaryFormat::AMD)
                    {   // check amd pseudo-op
                        if (isAmdPseudoOp)
                        {
                            printError(stmtPlace, "AMD pseudo-op can be defined only in "
                                    "AMD format code");
                            break;
                        }
                    }
                    if (format != BinaryFormat::AMDCL2)
                    {   // check amd pseudo-op
                        if (isAmdCL2PseudoOp)
                        {
                            printError(stmtPlace, "AMDCL2 pseudo-op can be defined only in "
                                    "AMDCL2 format code");
                            break;
                        }
                    }
                    if (format != BinaryFormat::ROCM)
                    {   // check rocm pseudo-op
                        if (isROCmPseudoOp)
                        {
                            printError(stmtPlace, "ROCm pseudo-op can be defined "
                                    "only in ROCm format code");
                            break;
                        }
                    }
                }
            }
            else if (makeMacroSubstitution(stmtPlace) == ParseState::MISSING)
                printError(stmtPlace, "This is neither pseudo-op and nor macro");
            break;
        }
    }
}

/* skipping clauses */
bool Assembler::skipClauses(bool exitm)
{
    const cxuint clauseLevel = clauses.size();
    AsmClauseType topClause = (!clauses.empty()) ? clauses.top().type :
            AsmClauseType::IF;
    const bool isTopIfClause = (topClause == AsmClauseType::IF ||
            topClause == AsmClauseType::ELSEIF || topClause == AsmClauseType::ELSE);
    bool good = true;
    const size_t inputFilterTop = asmInputFilters.size();
    while (exitm || clauses.size() >= clauseLevel)
    {
        if (!readLine())
            break;
        // if exit from macro mode, exit when macro filter exits
        if (exitm && inputFilterTop > asmInputFilters.size())
        {   // set lineAlreadyRead - next line after skipped region read will be read
            lineAlreadyRead  = true;
            break; // end of macro, 
        }
        
        const char* linePtr = line;
        const char* end = line+lineSize;
        skipSpacesAndLabels(linePtr, end);
        const char* stmtPlace = linePtr;
        if (linePtr == end || *linePtr != '.')
            continue;
        
        CString pseudoOpName = extractSymName(linePtr, end, false);
        toLowerString(pseudoOpName);
        
        const size_t pseudoOp = binaryFind(offlinePseudoOpNamesTbl,
               offlinePseudoOpNamesTbl + sizeof(offlinePseudoOpNamesTbl)/sizeof(char*),
               pseudoOpName.c_str()+1, CStringLess()) - offlinePseudoOpNamesTbl;
        
        // any conditional inside macro or repeat will be ignored
        bool insideMacroOrRepeat = !clauses.empty() && 
            (clauses.top().type == AsmClauseType::MACRO ||
                    clauses.top().type == AsmClauseType::REPEAT);
        switch(pseudoOp)
        {
            case ASMCOP_ENDIF:
                if (!AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false;   // if endif have garbages
                else if (!insideMacroOrRepeat)
                    if (!popClause(stmtPlace, AsmClauseType::IF))
                        good = false;
                break;
            case ASMCOP_ENDM:
                if (!AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false;   // if .endm have garbages
                else if (!popClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMCOP_ENDR:
                if (!AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false;   // if .endr have garbages
                else if (!popClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            case ASMCOP_ELSE:
            case ASMCOP_ELSEIFARCH:
            case ASMCOP_ELSEIF:
            case ASMCOP_ELSEIF32:
            case ASMCOP_ELSEIF64:
            case ASMCOP_ELSEIFB:
            case ASMCOP_ELSEIFC:
            case ASMCOP_ELSEIFDEF:
            case ASMCOP_ELSEIFEQ:
            case ASMCOP_ELSEIFEQS:
            case ASMCOP_ELSEIFFMT:
            case ASMCOP_ELSEIFGE:
            case ASMCOP_ELSEIFGPU:
            case ASMCOP_ELSEIFGT:
            case ASMCOP_ELSEIFLE:
            case ASMCOP_ELSEIFLT:
            case ASMCOP_ELSEIFNARCH:
            case ASMCOP_ELSEIFNB:
            case ASMCOP_ELSEIFNC:
            case ASMCOP_ELSEIFNDEF:
            case ASMCOP_ELSEIFNE:
            case ASMCOP_ELSEIFNES:
            case ASMCOP_ELSEIFNFMT:
            case ASMCOP_ELSEIFNGPU:
            case ASMCOP_ELSEIFNOTDEF:
                if (pseudoOp == ASMCOP_ELSE &&
                            !AsmPseudoOps::checkGarbagesAtEnd(*this, linePtr))
                    good = false; // if .else have garbages
                else if (!insideMacroOrRepeat)
                {
                    if (!exitm && clauseLevel == clauses.size() && isTopIfClause)
                    {   /* set lineAlreadyRead - next line after skipped region read
                         * will be read */
                        lineAlreadyRead = true; // read
                        return good; // do exit
                    }
                    if (!pushClause(stmtPlace, (pseudoOp==ASMCOP_ELSE ?
                                AsmClauseType::ELSE : AsmClauseType::ELSEIF)))
                        good = false;
                }
                break;
            case ASMCOP_IF:
            case ASMCOP_IFARCH:
            case ASMCOP_IF32:
            case ASMCOP_IF64:
            case ASMCOP_IFB:
            case ASMCOP_IFC:
            case ASMCOP_IFDEF:
            case ASMCOP_IFEQ:
            case ASMCOP_IFEQS:
            case ASMCOP_IFFMT:
            case ASMCOP_IFGE:
            case ASMCOP_IFGPU:
            case ASMCOP_IFGT:
            case ASMCOP_IFLE:
            case ASMCOP_IFLT:
            case ASMCOP_IFNARCH:
            case ASMCOP_IFNB:
            case ASMCOP_IFNC:
            case ASMCOP_IFNDEF:
            case ASMCOP_IFNE:
            case ASMCOP_IFNES:
            case ASMCOP_IFNFMT:
            case ASMCOP_IFNGPU:
            case ASMCOP_IFNOTDEF:
                if (!insideMacroOrRepeat)
                {
                    if (!pushClause(stmtPlace, AsmClauseType::IF))
                        good = false;
                }
                break;
            case ASMCOP_MACRO:
                if (!pushClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMCOP_IRP:
            case ASMCOP_IRPC:
            case ASMCOP_REPT:
                if (!pushClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            default:
                break;
        }
    }
    return good;
}

bool Assembler::putMacroContent(RefPtr<AsmMacro> macro)
{
    const cxuint clauseLevel = clauses.size();
    bool good = true;
    while (clauses.size() >= clauseLevel)
    {
        if (!readLine())
        {
            good = false;
            break;
        }
        
        const char* linePtr = line;
        const char* end = line+lineSize;
        skipSpacesAndLabels(linePtr, end);
        const char* stmtPlace = linePtr;
        if (linePtr == end || *linePtr != '.')
        {
            macro->addLine(currentInputFilter->getMacroSubst(),
                  currentInputFilter->getSource(),
                  currentInputFilter->getColTranslations(), lineSize, line);
            continue;
        }
        
        CString pseudoOpName = extractSymName(linePtr, end, false);
        toLowerString(pseudoOpName);
        
        const size_t pseudoOp = binaryFind(macroRepeatPseudoOpNamesTbl,
               macroRepeatPseudoOpNamesTbl + sizeof(macroRepeatPseudoOpNamesTbl) /
               sizeof(char*), pseudoOpName.c_str()+1, CStringLess()) -
               macroRepeatPseudoOpNamesTbl;
        switch(pseudoOp)
        {
            case ASMMROP_ENDM:
                if (!popClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_ENDR:
                if (!popClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            case ASMMROP_MACRO:
                if (!pushClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_IRP:
            case ASMMROP_IRPC:
            case ASMMROP_REPT:
                if (!pushClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            default:
                break;
        }
        if (pseudoOp != ASMMROP_ENDM || clauses.size() >= clauseLevel)
            macro->addLine(currentInputFilter->getMacroSubst(),
                  currentInputFilter->getSource(),
                  currentInputFilter->getColTranslations(), lineSize, line);
    }
    return good;
}

bool Assembler::putRepetitionContent(AsmRepeat& repeat)
{
    const cxuint clauseLevel = clauses.size();
    bool good = true;
    while (clauses.size() >= clauseLevel)
    {
        if (!readLine())
        {
            good = false;
            break;
        }
        
        const char* linePtr = line;
        const char* end = line+lineSize;
        skipSpacesAndLabels(linePtr, end);
        const char* stmtPlace = linePtr;
        if (linePtr == end || *linePtr != '.')
        {
            repeat.addLine(currentInputFilter->getMacroSubst(),
               currentInputFilter->getSource(), currentInputFilter->getColTranslations(),
               lineSize, line);
            continue;
        }
        
        CString pseudoOpName = extractSymName(linePtr, end, false);
        toLowerString(pseudoOpName);
        const size_t pseudoOp = binaryFind(macroRepeatPseudoOpNamesTbl,
               macroRepeatPseudoOpNamesTbl + sizeof(macroRepeatPseudoOpNamesTbl) /
               sizeof(char*), pseudoOpName.c_str()+1, CStringLess()) -
               macroRepeatPseudoOpNamesTbl;
        switch(pseudoOp)
        {
            case ASMMROP_ENDM:
                if (!popClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_ENDR:
                if (!popClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            case ASMMROP_MACRO:
                if (!pushClause(stmtPlace, AsmClauseType::MACRO))
                    good = false;
                break;
            case ASMMROP_IRP:
            case ASMMROP_IRPC:
            case ASMMROP_REPT:
                if (!pushClause(stmtPlace, AsmClauseType::REPEAT))
                    good = false;
                break;
            default:
                break;
        }
        if (pseudoOp != ASMMROP_ENDR || clauses.size() >= clauseLevel)
            repeat.addLine(currentInputFilter->getMacroSubst(),
                   currentInputFilter->getSource(),
                   currentInputFilter->getColTranslations(), lineSize, line);
    }
    return good;
}
