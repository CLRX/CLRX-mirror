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

#include <CLRX/Config.h>
#include <string>
#include <cstring>
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
#include "AsmAmdInternals.h"
#include "AsmAmdCL2Internals.h"
#include "AsmGalliumInternals.h"
#include "AsmROCmInternals.h"

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
        // ROCm is always 64-bit, print warning about it
        if (!_64Bit)
            asmr.printWarning(linePtr, "For ROCm bitness is always 64bit");
    }
    else if (asmr.format != BinaryFormat::AMD && asmr.format != BinaryFormat::GALLIUM &&
        asmr.format != BinaryFormat::AMDCL2)
        // print warning (for raw format) that bitness is ignored
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
    // choose correct binary format
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
        ASM_FAIL_BY_ERROR(formatPlace, "Unknown output format type")
    return true;
}

// .format pseudo-op
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
        // set if no garbages at end
        if (asmr.formatHandler!=nullptr)
            ASM_RETURN_BY_ERROR(formatPlace, "Output format type is already defined")
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
            // set if no garbages at end
            asmr.deviceType = deviceType;
    }
    // if exception - print error
    catch(const Exception& ex)
    { asmr.printError(deviceNamePlace, ex.what()); }
}

void AsmPseudoOps::setGPUArchitecture(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    char archName[64];
    const char* archNamePlace = linePtr;
    if (!getNameArg(asmr, 64, archName, linePtr, "GPU architecture name"))
        return;
    try
    {
        GPUArchitecture arch = getGPUArchitectureFromName(archName);
        GPUDeviceType deviceType = getLowestGPUDeviceTypeFromArchitecture(arch);
        if (checkGarbagesAtEnd(asmr, linePtr))
            // set if no garbages at end
            asmr.deviceType = deviceType;
    }
    // if exception - print error
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
            // parse section flags (a,x,w)
            bool flagsStrIsGood = true;
            for (const char c: flagsStr)
                if (c=='a')
                    sectFlags |= ASMELFSECT_ALLOCATABLE;
                else if (c=='x')
                    sectFlags |= ASMELFSECT_EXECUTABLE;
                else if (c=='w')
                    sectFlags |= ASMELFSECT_WRITEABLE;
                else if (flagsStrIsGood)
                    ASM_NOTGOOD_BY_ERROR1(flagsStrIsGood = good, flagsStrPlace,
                            "Only 'a', 'w', 'x' is accepted in flags string")
        }
        else
            good = false;
        
        bool haveComma;
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            // section type
            char typeBuf[20];
            skipSpacesToEnd(linePtr, end);
            const char* typePlace = linePtr;
            if (linePtr+1<end && *linePtr=='@' && isAlpha(linePtr[1]))
            {
                // parse section type (progbits, note, nobits)
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
                        ASM_NOTGOOD_BY_ERROR(typePlace, "Unknown section type")
                }
                else
                    good = false;
            }
            else
                ASM_NOTGOOD_BY_ERROR(typePlace, "Section type was not preceded by '@'")
        }
    }
    // parse alignment
    skipSpacesToEnd(linePtr, end);
    if (linePtr+6<end && ::strncasecmp(linePtr, "align", 5)==0 && !isAlpha(linePtr[5]))
    {
        // if alignment
        linePtr+=5;
        skipSpacesToEnd(linePtr, end);
        if (linePtr!=end && *linePtr=='=')
        {
            skipCharAndSpacesToEnd(linePtr, end);
            const char* valuePtr = linePtr;
            if (getAbsoluteValueArg(asmr, sectionAlign, linePtr, true))
            {
                if (sectionAlign!=0 && (1ULL<<(63-CLZ64(sectionAlign))) != sectionAlign)
                    ASM_NOTGOOD_BY_ERROR(valuePtr, "Alignment must be power of two or zero")
            }
            else
                good = false;
        }
        else
            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected '=' after 'align'")
    }
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (!haveFlags)
        // if section flags and section type not supplied
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
        // convert path to system path (with system dir separators)
        filesystemPath(sysfilename);
        try
        {
            asmr.includeFile(pseudoOpPlace, sysfilename);
            return;
        }
        catch(const Exception& ex)
        { failedOpen = true; }
        
        // find in include paths
        for (const CString& incDir: asmr.includeDirs)
        {
            failedOpen = false;
            std::string incDirPath(incDir.c_str());
            // convert path to system path (with system dir separators)
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
        // if not found
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
        PSEUDOOP_RETURN_BY_ERROR("Writing data into non-writeable section is illegal")
    
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
        // parse offset argument
        offsetPlace = linePtr;
        if (getAbsoluteValueArg(asmr, offset, linePtr))
        {
            // offset must not be negative
            if (int64_t(offset) < 0)
                ASM_NOTGOOD_BY_ERROR(offsetPlace, "Offset is negative!")
        }
        else
            good = false;
        
        if (!skipComma(asmr, haveComma, linePtr))
            return;
        if (haveComma)
        {
            skipSpacesToEnd(linePtr, end);
            countPlace = linePtr;
            // parse count argument
            good &= getAbsoluteValueArg(asmr, count, linePtr);
            if (int64_t(count) < 0)
                ASM_NOTGOOD_BY_ERROR(countPlace, "Count bytes is negative!")
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
    // try in this directory
    ifs.open(sysfilename.c_str(), std::ios::binary);
    if (!ifs)
    {
        // find in include paths
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
        ASM_RETURN_BY_ERROR(namePlace, (std::string("Binary file '") + filename +
                    "' not found or unavailable in any directory").c_str())
    // exception for checking file seeking
    bool seekingIsWorking = true;
    ifs.exceptions(std::ios::badbit | std::ios::failbit); // exceptions
    try
    { ifs.seekg(0, std::ios::end); /* to end of file */ }
    catch(const std::exception& ex)
    {
        /* oh, no! this is not regular file */
        seekingIsWorking = false;
        ifs.clear();
    }
    ifs.exceptions(std::ios::badbit);  // exceptions for reading
    if (seekingIsWorking)
    {
        /* for regular files */
        const uint64_t size = ifs.tellg();
        if (size < offset)
            return; // do nothing
        // skip offset bytes
        ifs.seekg(offset, std::ios::beg);
        const uint64_t toRead = std::min(size-offset, count);
        char* output = reinterpret_cast<char*>(asmr.reserveData(toRead));
        // and just read directly to output
        ifs.read(output, toRead);
        if (ifs.gcount() != std::streamsize(toRead))
            ASM_RETURN_BY_ERROR(namePlace, "Can't read whole needed file content")
    }
    else
    {
        /* for sequential files, likes fifo */
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
        // value >=500 treat as error
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
        // if without string
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
        // if without string
        asmr.printWarning(pseudoOpPlace, ".warning encountered");
}

template<typename T>
void AsmPseudoOps::putIntegers(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    asmr.initializeOutputFormat();
    if (!asmr.isWriteableSection())
        PSEUDOOP_RETURN_BY_ERROR("Writing data into non-writeable section is illegal")
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        const char* exprPlace = linePtr;
        // try parse expression for this integer
        uint64_t value;
        if (AsmExpression::fastExprEvaluate(asmr, linePtr, value))
        {   // fast path (expression)
            if (sizeof(T) < 8)
                asmr.printWarningForRange(sizeof(T)<<3, value,
                                asmr.getSourcePos(exprPlace));
            T out;
            SLEV(out, value); // store in little-endian
            asmr.putData(sizeof(T), reinterpret_cast<const cxbyte*>(&out));
            continue;
        }
        std::unique_ptr<AsmExpression> expr(AsmExpression::parse(asmr, linePtr));
        if (expr)
        {
            if (expr->isEmpty()) // empty expression print warning
            {
                asmr.printWarning(linePtr, "No expression, zero has been put");
                uint64_t zero = 0;
                asmr.putData(sizeof(T), reinterpret_cast<const cxbyte*>(&zero));
            }
            else if (expr->getSymOccursNum()==0)
            {
                // put directly to section
                AsmSectionId sectionId;
                AsmTryStatus  evalStatus = expr->tryEvaluate(asmr, value, sectionId,
                                        asmr.withSectionDiffs());
                if (evalStatus == AsmTryStatus::SUCCESS)
                {
                    if (sectionId == ASMSECT_ABS)
                    {
                        if (sizeof(T) < 8)
                            asmr.printWarningForRange(sizeof(T)<<3, value,
                                         asmr.getSourcePos(exprPlace));
                        T out;
                        SLEV(out, value); // store in little-endian
                        asmr.putData(sizeof(T), reinterpret_cast<const cxbyte*>(&out));
                    }
                    else
                        asmr.printError(exprPlace, "Expression must be absolute!");
                }
                else if (evalStatus == AsmTryStatus::TRY_LATER)
                {
                    // if section diffs to resolve later
                    expr->setTarget(AsmExprTarget::dataTarget<T>(
                                    asmr.currentSection, asmr.currentOutPos));
                    asmr.unevalExpressions.push_back(expr.release());
                    asmr.reserveData(sizeof(T));
                }
            }
            else // expression, we set target of expression (just data)
            {
                /// will be resolved later
                expr->setTarget(AsmExprTarget::dataTarget<T>(
                                asmr.currentSection, asmr.currentOutPos));
                expr.release();
                asmr.reserveData(sizeof(T));
            }
        }
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

// helper for floating point parsing (with storing in little-endian)
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
        PSEUDOOP_RETURN_BY_ERROR("Writing data into non-writeable section is illegal")
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        UIntType out = 0;
        const char* literalPlace = linePtr;
        if (linePtr != end && *linePtr != ',')
        {
            // try parse floating point (returns as little-endian value)
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
        PSEUDOOP_RETURN_BY_ERROR("Writing data into non-writeable section is illegal")
    skipSpacesToEnd(linePtr, end);
    if (linePtr == end)
        return;
    do {
        UInt128 value = { 0, 0 };
        if (linePtr != end && *linePtr != ',')
        {
            const char* literalPlace = linePtr;
            bool negative = false;
            // handle '+' and '-' before number
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
            {
                // negate value
                value.hi = ~value.hi + (value.lo==0);
                value.lo = -value.lo;
            }
        }
        else // warning
            asmr.printWarning(linePtr, "No 128-bit literal, zero has been put");
        UInt128 out;
        // and store value in little-endian
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
        PSEUDOOP_RETURN_BY_ERROR("Writing data into non-writeable section is illegal")
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

// for .string16, .string32 and .string64 pseudo-ops
// store characters as 16-,32-,64-bit values
template<typename T>
void AsmPseudoOps::putStringsToInts(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    if (!asmr.isWriteableSection())
        PSEUDOOP_RETURN_BY_ERROR("Writing data into non-writeable section is illegal")
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
    CString symName = extractScopedSymName(linePtr, end, false);
    bool good = true;
    if (symName.empty())
        ASM_NOTGOOD_BY_ERROR(linePtr, "Expected symbol")
    if (!skipRequiredComma(asmr, linePtr))
        return;
    if (good) // is good
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
        // handle errors
        if (symEntry == nullptr)
            ASM_NOTGOOD_BY_ERROR(symNamePlace, "Expected symbol name")
        else if (symEntry->second.regRange)
            ASM_NOTGOOD_BY_ERROR(symNamePlace, "Symbol must not be register symbol")
        else if (symEntry->second.base)
            ASM_NOTGOOD_BY_ERROR(symNamePlace,
                "Symbol must not be set by .eqv pseudo-op or must be constant")
        
        if (good)
        {
            // set binding to symbol (except symbol '.')
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
        ASM_NOTGOOD_BY_ERROR(symNamePlace, "Expected symbol name")
    if (!skipRequiredComma(asmr, linePtr))
        return;
    // parse size
    uint64_t size;
    good &= getAbsoluteValueArg(asmr, size, linePtr, true);
    bool ignore = false;
    if (symEntry != nullptr)
    {
        if (symEntry->second.base)
            ASM_NOTGOOD_BY_ERROR(symNamePlace,
                    "Symbol must not be set by .eqv pseudo-op or must be constant")
        else if (symEntry->second.regRange)
            ASM_NOTGOOD_BY_ERROR(symNamePlace, "Symbol must not be register symbol")
        else if (symEntry->first == ".")
        {
            // do not set size for '.' symbol
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
        PSEUDOOP_RETURN_BY_ERROR("Writing data into non-writeable section is illegal")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t repeat = 0, size = 1, value = 0;
    
    const char* reptStr = linePtr;
    // parse repeat argument
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
        // parse size argument
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
            // parse value argument
            good &= getAbsoluteValueArg(asmr, value, linePtr);
        }
    }
    if (int64_t(size) > 0 && int64_t(repeat) > 0 && SSIZE_MAX/size < repeat)
        ASM_NOTGOOD_BY_ERROR(pseudoOpPlace, "Product of repeat and size is too big")
    
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
        PSEUDOOP_RETURN_BY_ERROR("Change output counter inside non-addressable "
                    "section is illegal")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t size = 1, value = 0;
    
    const char* sizePlace = linePtr;
    // parse size argument
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
        // parse value argument (optional)
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
        PSEUDOOP_RETURN_BY_ERROR("Change output counter inside non-addressable"
                    " section is illegal")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t alignment, value = 0, maxAlign = 0;
    const char* alignPlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, alignment, linePtr, true);
    
    if (good)
    {
        // checking alignment value
        if (powerOf2)
        {
            // if alignment is power of 2, then must be lesser than 64
            if (alignment > 63)
                ASM_NOTGOOD_BY_ERROR(alignPlace, "Power of 2 of alignment is "
                            "greater than 63")
            else
                alignment = (1ULL<<alignment);
        }
        // checking whether alignment is power of 2
        else if (alignment == 0 || (1ULL<<(63-CLZ64(alignment))) != alignment)
            ASM_NOTGOOD_BY_ERROR(alignPlace, "Alignment is not power of 2")
    }
    
    bool haveValue = false;
    if (!skipComma(asmr, haveValue, linePtr))
        return;
    const char* valuePlace = linePtr;
    if (haveValue)
    {
        skipSpacesToEnd(linePtr, end);
        valuePlace = linePtr;
        // parse value argument
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
            // maxalign argument
            good &= getAbsoluteValueArg(asmr, maxAlign, linePtr);
        }
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr)) //if parsing failed
        return;
    
    uint64_t outPos = asmr.currentOutPos;
    // calculate bytes to fill (to next alignment bytes)
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
        // call routine to filling alignment from ISA assembler (to fill code by nops)
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
        PSEUDOOP_RETURN_BY_ERROR("Change output counter inside non-addressable "
                    "section is illegal")
    
    skipSpacesToEnd(linePtr, end);
    uint64_t alignment, value = 0, maxAlign = 0;
    const char* alignPlace = linePtr;
    bool good = getAbsoluteValueArg(asmr, alignment, linePtr, true);
    if (good && alignment != 0 && (1ULL<<(63-CLZ64(alignment))) != alignment)
        ASM_NOTGOOD_BY_ERROR(alignPlace, "Alignment is not power of 2")
    
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
            // maxalign argument
            good &= getAbsoluteValueArg(asmr, maxAlign, linePtr);
        }
    }
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (alignment == 0)
        return; // do nothing
    
    uint64_t outPos = asmr.currentOutPos;
    if (outPos&(sizeof(Word)-1))
        PSEUDOOP_RETURN_BY_ERROR("Offset is not aligned to word")
    
    // calculate bytes to fill (to next alignment bytes)
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
        // call routine to filling alignment from ISA assembler (to fill code by nops)
        asmr.isaAssembler->fillAlignment(bytesToFill, content);
}

void AsmPseudoOps::doOrganize(Assembler& asmr, const char* linePtr)
{
    asmr.initializeOutputFormat();
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value;
    AsmSectionId sectionId = ASMSECT_ABS;
    const char* valuePlace = linePtr;
    bool good = getAnyValueArg(asmr, value, sectionId, linePtr);
    
    uint64_t fillValue = 0;
    bool haveComma;
    if (!skipComma(asmr, haveComma, linePtr))
        return;
    const char* fillValuePlace = linePtr;
    if (haveComma)
    {
        // optional fill argument
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

// .ifdef (or .ifndef if negation is true)
void AsmPseudoOps::doIfDef(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr,
               bool negation, bool elseIfClause)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    AsmSymbolEntry* entry;
    bool good = true;
    // parse symbol
    Assembler::ParseState state = asmr.parseSymbol(linePtr, entry, false, true);
    if (state == Assembler::ParseState::FAILED)
        return;
    if (state == Assembler::ParseState::MISSING)
        ASM_NOTGOOD_BY_ERROR(symNamePlace, "Expected symbol")
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    const bool symDefined = (entry!=nullptr && entry->second.isDefined());
    bool satisfied = (!negation) ?  symDefined : !symDefined;
    if (asmr.pushClause(pseudoOpPlace, clauseType, satisfied, included))
    {   // 
        if (!included) // skip clauses (do not perform statements)
            asmr.skipClauses();
    }
}

// .ifb (or .ifnb if negation is true)
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

// this routine get string to compare in rules given GNU as
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
        ASM_RETURN_BY_ERROR(linePtr, "Missing second string")
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
        // if architecture not found (unknown architecture)
        asmr.printError(archNamePlace, ex.what());
        return;
    }
    
    const AsmClauseType clauseType = elseIfClause ? AsmClauseType::ELSEIF :
            AsmClauseType::IF;
    bool included;
    GPUArchitecture curArch = getGPUArchitectureFromDeviceType(asmr.getDeviceType());
    const bool isThisArch = isThisGPUArchitecture(arch, curArch);
    bool satisfied = (!negation) ? isThisArch : !isThisArch;
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
        // if GPU device name is unknown, print error
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
    // parse binary format name
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
        PSEUDOOP_RETURN_BY_ERROR("Repetition level is greater than 1000")
    asmr.pushClause(pseudoOpPlace, AsmClauseType::REPEAT);
    if (repeatsNum == 0)
    {
        /* skip it */
        asmr.skipClauses();
        return;
    }
    /* create repetition (even if only 1 - for correct source position included
     * to messages from repetition) */
    std::unique_ptr<AsmRepeat> repeat(new AsmRepeat(
                asmr.getSourcePos(pseudoOpPlace), repeatsNum));
    if (asmr.putRepetitionContent(*repeat))
    {
        // and input stream filter
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
        ASM_RETURN_BY_ERROR(macroNamePlace, "Expected macro name")
    // convert to lower (name is case-insensitive)
    if (asmr.macroCase)
        toLowerString(macroName);
    /* parse args */
    std::vector<AsmMacroArg> args;
    
    bool good = true;
    bool haveVarArg = false;
    
    if (asmr.macroMap.find(macroName) != asmr.macroMap.end())
        ASM_NOTGOOD_BY_ERROR(macroNamePlace, (std::string("Macro '") + macroName.c_str() +
                "' is already defined").c_str())
    
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
            ASM_RETURN_BY_ERROR(argPlace, "Expected macro argument name")
        bool argRequired = false;
        bool argVarArgs = false;
        bool argGood = true;
        std::string defaultArgValue;
        
        if (!macroArgSet.insert(argName).second)
            // duplicate!
            ASM_NOTGOOD_BY_ERROR1(argGood, argPlace, (std::string(
                    "Duplicated macro argument '")+ argName.c_str()+'\'').c_str())
        
        skipSpacesToEnd(linePtr, end);
        if (linePtr != end && *linePtr == ':')
        {
            // parse argument's qualifier
            skipCharAndSpacesToEnd(linePtr, end);
            //extr
            if (linePtr+3 <= end && linePtr[0] == 'r' && linePtr[1] == 'e' &&
                    linePtr[2] == 'q') // reqd (if argument required)
            {
                argRequired = true;
                linePtr += 3;
            }
            else if (linePtr+6 <= end && ::memcmp(linePtr, "vararg", 6)==0) // required
            {
                argVarArgs = true;
                linePtr += 6;
            }
            else // otherwise
                ASM_NOTGOOD_BY_ERROR1(argGood, linePtr,
                        "Expected qualifier 'req' or 'vararg'")
        }
        skipSpacesToEnd(linePtr, end);
        if (linePtr != end && *linePtr == '=')
        {
            // parse default value
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
        
        if (argGood)
        {
            // push to arguments
            if (haveVarArg)
                ASM_NOTGOOD_BY_ERROR(argPlace, "Variadic argument must be last")
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
        {
            // ignore if name of macro is name of pseudo op name
            asmr.printWarning(pseudoOpPlace, (std::string(
                        "Attempt to redefine pseudo-op '")+macroName.c_str()+
                        "' as macro. Ignoring it...").c_str());
            asmr.pushClause(pseudoOpPlace, AsmClauseType::MACRO);
            asmr.skipClauses();
            return;
        }
        // create a macro and put macro map
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
        // skipping clauses to current macro
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
        ASM_RETURN_BY_ERROR(symNamePlace, "Expected argument name")
    /* parse args */
    std::vector<CString> symValues;
    std::string symValString;
    
    bool good = true;
    skipSpacesToEnd(linePtr, end);
    if (linePtr != end && *linePtr == ',')
        skipCharAndSpacesToEnd(linePtr, end);
    
    // parse list of symvalues for repetitions
    while(linePtr != end)
    {
        if (linePtr != end && *linePtr == ',')
        {
            if (perChar)
                // because if value stores as character we add ','
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
            // push single sym value
            symValues.push_back(symValue);
        else
            // otherwise this sym value as string of values
            symValString += symValue;
    }
    
    // check depth of repetitions
    if (asmr.repetitionLevel == 1000)
        PSEUDOOP_RETURN_BY_ERROR("Repetition level is greater than 1000")
    
    if (symValues.empty())
        symValues.push_back("");
    if (good)
    {
        // create IRP repetition
        asmr.pushClause(pseudoOpPlace, AsmClauseType::REPEAT);
        std::unique_ptr<AsmIRP> repeat;
        if (!perChar)
            repeat.reset(new AsmIRP(asmr.getSourcePos(pseudoOpPlace),
                      symName, Array<CString>(symValues.begin(), symValues.end())));
        else // per char
            repeat.reset(new AsmIRP(asmr.getSourcePos(pseudoOpPlace),
                      symName, symValString));
        
        if (asmr.putRepetitionContent(*repeat))
        {
            // and input stream filter
            std::unique_ptr<AsmInputFilter> newInputFilter(
                        new AsmIRPInputFilter(repeat.release()));
            asmr.asmInputFilters.push(newInputFilter.release());
            asmr.currentInputFilter = asmr.asmInputFilters.top();
            asmr.repetitionLevel++;
        }
    }
}

void AsmPseudoOps::doFor(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    AsmSymbolEntry* iterSymbol = nullptr;
    const CString symName = extractScopedSymName(linePtr, end, false);
    if (symName.empty())
        ASM_RETURN_BY_ERROR(symNamePlace, "Illegal symbol name")
    size_t symNameLength = symName.size();
    // special case for '.' symbol (check whether is in global scope)
    if (symNameLength >= 3 && symName.compare(symNameLength-3, 3, "::.")==0)
        ASM_RETURN_BY_ERROR(symNamePlace, "Symbol '.' can be only in global scope")
    
    bool good = true;
    skipSpacesToEnd(linePtr, end);
    if (linePtr==end || *linePtr!='=')
        ASM_NOTGOOD_BY_ERROR(linePtr, "Expected '='")
    skipCharAndSpacesToEnd(linePtr, end);
    uint64_t value = 0;
    AsmSectionId sectionId = ASMSECT_ABS;
    good &= AsmParseUtils::getAnyValueArg(asmr, value, sectionId, linePtr);
    if (good)
    {
        std::pair<AsmSymbolEntry*, bool> res = asmr.insertSymbolInScope(symName,
                    AsmSymbol(sectionId, value));
        if (!res.second)
        {
            // if symbol found
            if (res.first->second.onceDefined && res.first->second.isDefined()) // if label
            {
                asmr.printError(symNamePlace, (std::string("Symbol '")+symName.c_str()+
                            "' is already defined").c_str());
                good = false;
            }
            else // set value of symbol
                asmr.setSymbol(*res.first, value, sectionId);
        }
        else // set hasValue (by isResolvableSection
            res.first->second.hasValue = asmr.isResolvableSection(sectionId);
        iterSymbol = res.first;
    }
    
    if (!skipRequiredComma(asmr, linePtr))
        return;
    std::unique_ptr<AsmExpression> condExpr(AsmExpression::parse(asmr, linePtr, true));
    if (!skipRequiredComma(asmr, linePtr))
        return;
    std::unique_ptr<AsmExpression> nextExpr(AsmExpression::parse(asmr, linePtr, true));
    
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (condExpr==nullptr || nextExpr==nullptr)
        return; // if no expressions
    
    // check depth of repetitions
    if (asmr.repetitionLevel == 1000)
        PSEUDOOP_RETURN_BY_ERROR("Repetition level is greater than 1000")
    
    // create AsmFor
    asmr.pushClause(pseudoOpPlace, AsmClauseType::REPEAT);
    std::unique_ptr<AsmFor> repeat(new AsmFor(
                asmr.getSourcePos(pseudoOpPlace), iterSymbol, condExpr.get(),
                nextExpr.get()));
    condExpr.release();
    nextExpr.release();
    if (asmr.putRepetitionContent(*repeat))
    {
        // and input stream filter
        std::unique_ptr<AsmInputFilter> newInputFilter(
                    new AsmForInputFilter(repeat.release()));
        asmr.asmInputFilters.push(newInputFilter.release());
        asmr.currentInputFilter = asmr.asmInputFilters.top();
        asmr.repetitionLevel++;
    }
}

void AsmPseudoOps::doWhile(Assembler& asmr, const char* pseudoOpPlace, const char* linePtr)
{
    std::unique_ptr<AsmExpression> condExpr(AsmExpression::parse(asmr, linePtr, true));
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    if (condExpr==nullptr)
        return; // if no expressions
    
    // check depth of repetitions
    if (asmr.repetitionLevel == 1000)
        PSEUDOOP_RETURN_BY_ERROR("Repetition level is greater than 1000")
    
    // create AsmFor
    asmr.pushClause(pseudoOpPlace, AsmClauseType::REPEAT);
    std::unique_ptr<AsmFor> repeat(new AsmFor(
                asmr.getSourcePos(pseudoOpPlace), nullptr, condExpr.get(), nullptr));
    condExpr.release();
    if (asmr.putRepetitionContent(*repeat))
    {
        // and input stream filter
        std::unique_ptr<AsmInputFilter> newInputFilter(
                    new AsmForInputFilter(repeat.release()));
        asmr.asmInputFilters.push(newInputFilter.release());
        asmr.currentInputFilter = asmr.asmInputFilters.top();
        asmr.repetitionLevel++;
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
        ASM_NOTGOOD_BY_ERROR(macroNamePlace, "Expected macro name")
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    // convert to lower (name is case-insensitive)
    if (asmr.macroCase)
        toLowerString(macroName); // macro name is lowered
    
    if (!asmr.macroMap.erase(macroName))
        asmr.printWarning(macroNamePlace, (std::string("Macro '")+macroName.c_str()+
                "' already doesn't exist").c_str());
}

void AsmPseudoOps::openScope(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    CString scopeName = extractSymName(linePtr, end, false);
    bool good = true;
    // check depth of scopes
    if (asmr.scopeStack.size() == 1000)
        ASM_NOTGOOD_BY_ERROR(pseudoOpPlace, "Scope level is greater than 1000")
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.pushScope(scopeName);
}

void AsmPseudoOps::closeScope(Assembler& asmr, const char* pseudoOpPlace,
                      const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    bool good = true;
    if (asmr.scopeStack.empty())
        ASM_NOTGOOD_BY_ERROR(pseudoOpPlace, "Closing global scope is illegal")
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    asmr.popScope();
}

void AsmPseudoOps::startUsing(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* scopePathPlace = linePtr;
    CString scopePath = extractScopedSymName(linePtr, end);
    bool good = true;
    if (scopePath.empty() || scopePath == "::")
        ASM_NOTGOOD_BY_ERROR(scopePathPlace, "Expected scope path")
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    AsmScope* scope = asmr.getRecurScope(scopePath);
    // do add this
    asmr.currentScope->startUsingScope(scope);
}

void AsmPseudoOps::doUseReg(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    asmr.initializeOutputFormat();
    
    do {
        skipSpacesToEnd(linePtr, end);
        bool good = true;
        cxuint regStart, regEnd;
        const AsmRegVar* regVar;
        good = asmr.isaAssembler->parseRegisterRange(linePtr, regStart, regEnd, regVar);
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!=':')
        {
            asmr.printError(linePtr, "Expected colon after register");
            continue;
        }
        skipCharAndSpacesToEnd(linePtr, end);
        
        cxbyte rwFlags = 0;
        // parse read/write flags (r,w,rw)
        for (; linePtr != end; linePtr++)
        {
            char c = toLower(*linePtr);
            if (c!='r' && c!='w')
                break;
            rwFlags |= (c=='r') ? ASMRVU_READ : ASMRVU_WRITE;
        }
        if (rwFlags==0) // not parsed
        {
            asmr.printError(linePtr, "Expected access mask");
            continue;
        }
        
        if (good) // if good
        {
            // create usageHandler if needed
            if (asmr.sections[asmr.currentSection].usageHandler == nullptr)
                    asmr.sections[asmr.currentSection].usageHandler.reset(
                            asmr.isaAssembler->createUsageHandler());
            // put regVar usage
            asmr.sections[asmr.currentSection].usageHandler->pushUsage(
                AsmRegVarUsage{ size_t(asmr.currentOutPos), regVar, uint16_t(regStart),
                    uint16_t(regEnd), ASMFIELD_NONE, rwFlags, 0, true });
        }
        
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::declareRegVarLinearDeps(Assembler& asmr, const char* linePtr,
            bool usedOnce)
{
    const char* end = asmr.line+asmr.lineSize;
    asmr.initializeOutputFormat();
    
    do {
        skipSpacesToEnd(linePtr, end);
        const char* regVarPlace = linePtr;
        bool good = true;
        cxuint regStart, regEnd;
        const AsmRegVar* regVar;
        good = asmr.isaAssembler->parseRegisterRange(linePtr, regStart, regEnd, regVar);
        
        if (good) // if good
        {
            // create isaLinearDepHandler if needed
            if (usedOnce && asmr.sections[asmr.currentSection].linearDepHandler == nullptr)
                    asmr.sections[asmr.currentSection].linearDepHandler.reset(
                        new ISALinearDepHandler());
            // put regVar usage
            if (regVar != nullptr)
            {
                if (usedOnce)
                    asmr.sections[asmr.currentSection].linearDepHandler->pushLinearDep(
                            AsmRegVarLinearDep{ size_t(asmr.currentOutPos), regVar,
                                uint16_t(regStart), uint16_t(regEnd) });
                else
                {   // defined for whole live of regvar
                    AsmRegVarLinears& linears = asmr.regVarLinearsMap[regVar];
                    linears.insertRegion({ regStart, regEnd });
                }
            }
            else
                // otherwise, just ignore, print warning
                asmr.printWarning(regVarPlace, "Normal register range is ignored");
        }
        
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::stopUsing(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* scopePathPlace = linePtr;
    CString scopePath = extractScopedSymName(linePtr, end);
    bool good = true;
    if (scopePath == "::")
        ASM_NOTGOOD_BY_ERROR(scopePathPlace, "Expected scope path")
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    AsmScope* scope = asmr.getRecurScope(scopePath);
    if (!scopePath.empty())
        asmr.currentScope->stopUsingScope(scope);
    else // stop using all scopes
        asmr.currentScope->stopUsingScopes();
}

void AsmPseudoOps::undefSymbol(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    const char* symNamePlace = linePtr;
    CString symName = extractScopedSymName(linePtr, end, false);
    bool good = true;
    if (symName.empty())
        ASM_NOTGOOD_BY_ERROR(symNamePlace, "Expected symbol name")
    // symbol '.' can not be undefined
    else if (symName == ".")
        ASM_NOTGOOD_BY_ERROR(symNamePlace, "Symbol '.' can not be undefined")
    if (!good || !checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    CString sameSymName;
    AsmScope* outScope;
    AsmSymbolEntry* it = asmr.findSymbolInScope(symName, outScope, sameSymName);
    if (it == nullptr || !it->second.isDefined())
        asmr.printWarning(symNamePlace, (std::string("Symbol '") + symName.c_str() +
                "' already doesn't exist").c_str());
    else // always undefine (do not remove, due to .eqv evaluation)
        asmr.undefineSymbol(*it);
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

void AsmPseudoOps::defRegVar(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    asmr.initializeOutputFormat();
    
    do {
        skipSpacesToEnd(linePtr, end);
        const char* regNamePlace = linePtr;
        CString name = extractScopedSymName(linePtr, end, false);
        bool good = true;
        if (name.empty())
            ASM_NOTGOOD_BY_ERROR(regNamePlace, "Expected reg-var name")
        skipSpacesToEnd(linePtr, end);
        if (linePtr==end || *linePtr!=':')
        {
            asmr.printError(linePtr, "Expected colon after reg-var");
            continue;
        }
        skipCharAndSpacesToEnd(linePtr, end);
        AsmRegVar var = { 0, 1 };
        if (!asmr.isaAssembler->parseRegisterType(linePtr, end, var.type))
            ASM_NOTGOOD_BY_ERROR(linePtr, "Expected name of register type")
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
            if (regSize==0)
                ASM_NOTGOOD_BY_ERROR(linePtr, "Size of reg-var is zero")
            if (regSize>UINT16_MAX)
                ASM_NOTGOOD_BY_ERROR(linePtr, "Size of reg-var out of range")
            var.size = regSize;
        }
        
        if (!good)
            continue;
        
        if (!asmr.addRegVar(name, var))
            asmr.printError(regNamePlace, (std::string("Reg-var '")+name.c_str()+
                        "' was already defined").c_str());
        
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::addCodeFlowEntries(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr, AsmCodeFlowType type)
{
    const bool acceptArgs = (type==AsmCodeFlowType::JUMP || type==AsmCodeFlowType::CJUMP ||
            type==AsmCodeFlowType::CALL);
    asmr.initializeOutputFormat();
    
    if (asmr.currentSection==ASMSECT_ABS ||
        asmr.sections[asmr.currentSection].type != AsmSectionType::CODE)
        PSEUDOOP_RETURN_BY_ERROR("Defining codeflow in non-code section is illegal")
    
    const char* end = asmr.line+asmr.lineSize;
    if (acceptArgs)
    {
        // multiple entries
        do {
            bool good = true;
            skipSpacesToEnd(linePtr, end);
            if (!good)
                continue;
            std::unique_ptr<AsmExpression> expr;
            uint64_t target;
            if (!getJumpValueArg(asmr, target, expr, linePtr))
                continue;
            asmr.sections[asmr.currentSection].addCodeFlowEntry({ 
                    size_t(asmr.currentOutPos), size_t(target), type });
            if (expr)
                expr->setTarget(AsmExprTarget::codeFlowTarget(asmr.currentSection,
                        asmr.sections[asmr.currentSection].codeFlow.size()-1));
            expr.release();
        } while (skipCommaForMultipleArgs(asmr, linePtr));
    }
    else // single entry without target
        asmr.sections[asmr.currentSection].addCodeFlowEntry({ 
                    size_t(asmr.currentOutPos), 0, type });
    
    checkGarbagesAtEnd(asmr, linePtr);
}

// get predefine value ('.get_64bit', '.get_arch', '.get_format')
void AsmPseudoOps::getPredefinedValue(Assembler& asmr, const char* linePtr,
                            AsmPredefined predefined)
{
    cxuint predefValue = 0;
    // we must initialize output format before getting any arch, gpu or format
    // must be set before
    asmr.initializeOutputFormat();
    switch (predefined)
    {
        case AsmPredefined::ARCH:
            predefValue = cxuint(getGPUArchitectureFromDeviceType(asmr.deviceType));
            break;
        case AsmPredefined::BIT64:
            predefValue = asmr._64bit;
            break;
        case AsmPredefined::GPU:
            predefValue = cxuint(asmr.deviceType);
            break;
        case AsmPredefined::FORMAT:
            predefValue = cxuint(asmr.format);
            break;
        case AsmPredefined::VERSION:
            predefValue = CLRX_VERSION_NUMBER;
            break;
        case AsmPredefined::POLICY:
            predefValue = asmr.policyVersion;
            break;
        default:
            break;
    }
    AsmParseUtils::setSymbolValue(asmr, linePtr, predefValue, ASMSECT_ABS);
}

void AsmPseudoOps::doEnum(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t& enumCount = asmr.currentScope->enumCount;
    if (linePtr!=end && *linePtr=='>')
    {
        // parse enum start
        linePtr++;
        uint64_t enumStart = 0;
        if (!getAbsoluteValueArg(asmr, enumStart, linePtr, false))
            return;
        
        enumCount = enumStart;
        if (!skipRequiredComma(asmr, linePtr))
            return;
    }
    skipSpacesToEnd(linePtr, end);
    do {
        const char* strAtSymName = linePtr;
        CString symbolName = extractScopedSymName(linePtr, end, false);
        if (!symbolName.empty())
        {
            std::pair<AsmSymbolEntry*, bool> res =
                    asmr.insertSymbolInScope(symbolName, AsmSymbol());
            if (!res.second && res.first->second.isDefined())
                // found and can be only once defined
                asmr.printError(strAtSymName, (std::string("Symbol '") +
                        symbolName.c_str() + "' is already defined").c_str());
            else
            {
                asmr.setSymbol(*res.first, enumCount++, ASMSECT_ABS);
                res.first->second.onceDefined = true;
            }
        }
        else
            asmr.printError(linePtr, "Expected symbol name");
        
    } while(skipCommaForMultipleArgs(asmr, linePtr));
    checkGarbagesAtEnd(asmr, linePtr);
}

void AsmPseudoOps::setPolicyVersion(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line + asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    uint64_t value = 0;
    skipSpacesToEnd(linePtr, end);
    const char* valuePlace = linePtr;
    if (!getAbsoluteValueArg(asmr, value, linePtr, true))
        return;
    if (!checkGarbagesAtEnd(asmr, linePtr))
        return;
    
    asmr.printWarningForRange(sizeof(cxuint)*8, value,
                    asmr.getSourcePos(valuePlace), WS_UNSIGNED);
    asmr.setPolicyVersion(value);
}

void AsmPseudoOps::ignoreString(Assembler& asmr, const char* linePtr)
{
    const char* end = asmr.line+asmr.lineSize;
    skipSpacesToEnd(linePtr, end);
    std::string out;
    if (asmr.parseString(out, linePtr))
        checkGarbagesAtEnd(asmr, linePtr);
}

template
void AsmPseudoOps::doAlignWord<uint16_t>(Assembler& asmr, const char* pseudoOpPlace,
                       const char* linePtr);

template
void AsmPseudoOps::doAlignWord<uint32_t>(Assembler& asmr, const char* pseudoOpPlace,
                       const char* linePtr);

template
void AsmPseudoOps::putIntegers<cxbyte>(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);

template
void AsmPseudoOps::putIntegers<uint16_t>(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);

template
void AsmPseudoOps::putIntegers<uint32_t>(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);

template
void AsmPseudoOps::putIntegers<uint64_t>(Assembler& asmr, const char* pseudoOpPlace,
                   const char* linePtr);

template
void AsmPseudoOps::putFloats<uint16_t>(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr);

template
void AsmPseudoOps::putFloats<uint32_t>(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr);

template
void AsmPseudoOps::putFloats<uint64_t>(Assembler& asmr, const char* pseudoOpPlace,
                     const char* linePtr);

template
void AsmPseudoOps::putStringsToInts<uint16_t>(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr);

template
void AsmPseudoOps::putStringsToInts<uint32_t>(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr);

template
void AsmPseudoOps::putStringsToInts<uint64_t>(Assembler& asmr, const char* pseudoOpPlace,
                    const char* linePtr);

};
