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
#include <ostream>
#include <cstring>
#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Disassembler.h>
#include "DisasmInternals.h"

using namespace CLRX;

DisasmException::DisasmException(const std::string& message) : Exception(message)
{ }

ISADisassembler::ISADisassembler(Disassembler& _disassembler, cxuint outBufSize)
        : disassembler(_disassembler), startOffset(0), labelStartOffset(0),
          dontPrintLabelsAfterCode(false), output(outBufSize, _disassembler.getOutput())
{ }

ISADisassembler::~ISADisassembler()
{ }

void ISADisassembler::writeLabelsToPosition(size_t pos, LabelIter& labelIter,
              NamedLabelIter& namedLabelIter)
{
    pos += startOffset; // fix
    if ((namedLabelIter != namedLabels.end() && namedLabelIter->first <= pos) ||
            (labelIter != labels.end() && *labelIter <= pos))
    {
        size_t curPos = SIZE_MAX;
        // set current position (from first label)
        if (labelIter != labels.end())
            curPos = *labelIter;
        if (namedLabelIter != namedLabels.end())
            curPos = std::min(curPos, namedLabelIter->first);
        
        bool haveLabel;
        do {
            haveLabel = false;
            const bool haveNumberedLabel =
                    labelIter != labels.end() && *labelIter <= pos;
            const bool haveNamedLabel =
                    (namedLabelIter != namedLabels.end() && namedLabelIter->first <= pos);
            
            size_t namedPos = SIZE_MAX;
            size_t numberedPos = SIZE_MAX;
            if (haveNumberedLabel)
                numberedPos = *labelIter;
            if (haveNamedLabel)
                namedPos = namedLabelIter->first;
            
            /// print numbered (not named) label in form .L[position]_[sectionCount]
            if (numberedPos <= namedPos && haveNumberedLabel)
            {
                curPos = *labelIter;
                char* buf = output.reserve(70);
                size_t bufPos = 0;
                // numbered label in form: '.L[POS]_[SECTION]'
                buf[bufPos++] = '.';
                buf[bufPos++] = 'L';
                bufPos += itocstrCStyle(*labelIter, buf+bufPos, 22, 10, 0, false);
                buf[bufPos++] = '_';
                bufPos += itocstrCStyle(disassembler.sectionCount,
                                buf+bufPos, 22, 10, 0, false);
                if (curPos != pos)
                {
                    // if label shifted back by some bytes before encoded instruction
                    buf[bufPos++] = '=';
                    buf[bufPos++] = '.';
                    buf[bufPos++] = '-';
                    bufPos += itocstrCStyle((pos-curPos), buf+bufPos, 22, 10, 0, false);
                    buf[bufPos++] = '\n';
                }
                else
                {
                    // just in this place: add ':'
                    buf[bufPos++] = ':';
                    buf[bufPos++] = '\n';
                }
                output.forward(bufPos);
                ++labelIter;
                haveLabel = true;
            }
            
            /// print named label
            if(namedPos <= numberedPos && haveNamedLabel)
            {
                curPos = namedLabelIter->first;
                output.write(namedLabelIter->second.size(),
                       namedLabelIter->second.c_str());
                char* buf = output.reserve(50);
                size_t bufPos = 0;
                if (curPos != pos)
                {
                    // if label shifted back by some bytes before encoded instruction
                    buf[bufPos++] = '=';
                    buf[bufPos++] = '.';
                    buf[bufPos++] = '-';
                    bufPos += itocstrCStyle((pos-curPos), buf+bufPos, 22, 10, 0, false);
                    buf[bufPos++] = '\n';
                }
                else
                {
                    // just in this place: add ':'
                    buf[bufPos++] = ':';
                    buf[bufPos++] = '\n';
                }
                output.forward(bufPos);
                ++namedLabelIter;
                haveLabel = true;
            }
            
        } while(haveLabel);
    }
}

void ISADisassembler::writeLabelsToEnd(size_t start, LabelIter labelIter,
                   NamedLabelIter namedLabelIter)
{
    size_t pos = startOffset + start;
    while (namedLabelIter != namedLabels.end() || labelIter != labels.end())
    {
        size_t namedPos = SIZE_MAX;
        size_t numberedPos = SIZE_MAX;
        if (labelIter != labels.end())
            numberedPos = *labelIter;
        if (namedLabelIter != namedLabels.end())
            namedPos = namedLabelIter->first;
        // print numbered label if it before named label
        if (numberedPos <= namedPos && labelIter != labels.end())
        {
            if (pos != *labelIter)
            {
                // print shift position to label (.org pseudo-op)
                char* buf = output.reserve(30);
                size_t bufPos = 0;
                memcpy(buf+bufPos, ".org ", 5);
                bufPos += 5;
                bufPos += itocstrCStyle(*labelIter, buf+bufPos, 20, 16);
                buf[bufPos++] = '\n';
                output.forward(bufPos);
            }
            char* buf = output.reserve(50);
            size_t bufPos = 0;
            // numbered label in form: '.L[POS]_[SECTION]'
            buf[bufPos++] = '.';
            buf[bufPos++] = 'L';
            bufPos += itocstrCStyle(*labelIter, buf+bufPos, 22, 10, 0, false);
            buf[bufPos++] = '_';
            bufPos += itocstrCStyle(disassembler.sectionCount,
                            buf+bufPos, 22, 10, 0, false);
            buf[bufPos++] = ':';
            buf[bufPos++] = '\n';
            output.forward(bufPos);
            pos = *labelIter;
            ++labelIter;
        }
        // this same for named label
        if (namedPos <= numberedPos && namedLabelIter != namedLabels.end())
        {
            if (pos != namedLabelIter->first)
            {
                // print shift position to label (.org pseudo-op)
                char* buf = output.reserve(30);
                size_t bufPos = 0;
                memcpy(buf+bufPos, ".org ", 5);
                bufPos += 5;
                bufPos += itocstrCStyle(namedLabelIter->first, buf+bufPos, 20, 16);
                buf[bufPos++] = '\n';
                output.forward(bufPos);
            }
            output.write(namedLabelIter->second.size(),
                        namedLabelIter->second.c_str());
            pos = namedLabelIter->first;
            ++namedLabelIter;
        }
    }
}

void ISADisassembler::writeLocation(size_t pos)
{
    const auto namedLabelIt = binaryMapFind(namedLabels.begin(), namedLabels.end(), pos);
    if (namedLabelIt != namedLabels.end())
    {
        /* print named label */
        output.write(namedLabelIt->second.size(), namedLabelIt->second.c_str());
        return;
    }
    /* otherwise we print numbered label */
    char* buf = output.reserve(50);
    size_t bufPos = 0;
    buf[bufPos++] = '.';
    buf[bufPos++] = 'L';
    bufPos += itocstrCStyle(pos, buf+bufPos, 22, 10, 0, false);
    buf[bufPos++] = '_';
    bufPos += itocstrCStyle(disassembler.sectionCount, buf+bufPos, 22, 10, 0, false);
    output.forward(bufPos);
}

bool ISADisassembler::writeRelocation(size_t pos, RelocIter& relocIter)
{
    while (relocIter != relocations.end() && relocIter->first < pos)
        relocIter++;
    if (relocIter == relocations.end() || relocIter->first != pos)
        return false;
    const Relocation& reloc = relocIter->second;
    // put relocation symbol in parenthesis
    if (reloc.addend != 0 && 
        (reloc.type==RELTYPE_LOW_32BIT || reloc.type==RELTYPE_HIGH_32BIT))
        output.write(1, "(");
    /// write name+value
    output.writeString(relSymbols[reloc.symbol].c_str());
    char* buf = output.reserve(50);
    size_t bufPos = 0;
    if (reloc.addend != 0)
    {
        // add addend
        if (reloc.addend > 0)
            buf[bufPos++] = '+';
        bufPos += itocstrCStyle(reloc.addend, buf+bufPos, 22, 10, 0, false);
        if (reloc.type==RELTYPE_LOW_32BIT || reloc.type==RELTYPE_HIGH_32BIT)
            buf[bufPos++] = ')';
    }
    // handle relocation types
    if (reloc.type==RELTYPE_LOW_32BIT)
    {
        ::memcpy(buf+bufPos, "&0xffffffff", 11);
        bufPos += 11;
    }
    else if (reloc.type==RELTYPE_HIGH_32BIT)
    {
        ::memcpy(buf+bufPos, ">>32", 4);
        bufPos += 4;
    }
    output.forward(bufPos);
    ++relocIter;
    return true;
}

void ISADisassembler::clearNumberedLabels()
{
    labels.clear();
}

void ISADisassembler::prepareLabelsAndRelocations()
{
    std::sort(labels.begin(), labels.end());
    const auto newEnd = std::unique(labels.begin(), labels.end());
    labels.resize(newEnd-labels.begin());
    mapSort(namedLabels.begin(), namedLabels.end());
    mapSort(relocations.begin(), relocations.end());
}

void ISADisassembler::beforeDisassemble()
{
    clearNumberedLabels();
    analyzeBeforeDisassemble();
    prepareLabelsAndRelocations();
}

Disassembler::Disassembler(const AmdMainGPUBinary32& binary, std::ostream& _output,
            Flags _flags) : fromBinary(true), binaryFormat(BinaryFormat::AMD),
            amdInput(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    amdInput = getAmdDisasmInputFromBinary32(binary, flags);
}

Disassembler::Disassembler(const AmdMainGPUBinary64& binary, std::ostream& _output,
            Flags _flags) : fromBinary(true), binaryFormat(BinaryFormat::AMD),
            amdInput(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    amdInput = getAmdDisasmInputFromBinary64(binary, flags);
}

Disassembler::Disassembler(const AmdCL2MainGPUBinary32& binary, std::ostream& _output,
           Flags _flags, cxuint driverVersion) : fromBinary(true),
            binaryFormat(BinaryFormat::AMDCL2), amdCL2Input(nullptr), output(_output),
            flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    amdCL2Input = getAmdCL2DisasmInputFromBinary32(binary, driverVersion,
                (flags & DISASM_HSALAYOUT) != 0);
}

Disassembler::Disassembler(const AmdCL2MainGPUBinary64& binary, std::ostream& _output,
           Flags _flags, cxuint driverVersion) : fromBinary(true),
            binaryFormat(BinaryFormat::AMDCL2), amdCL2Input(nullptr), output(_output),
            flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    amdCL2Input = getAmdCL2DisasmInputFromBinary64(binary, driverVersion,
                (flags & DISASM_HSALAYOUT) != 0);
}

Disassembler::Disassembler(const ROCmBinary& binary, std::ostream& _output, Flags _flags)
         : fromBinary(true), binaryFormat(BinaryFormat::ROCM),
           rocmInput(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    rocmInput = getROCmDisasmInputFromBinary(binary);
}

Disassembler::Disassembler(const AmdDisasmInput* disasmInput, std::ostream& _output,
            Flags _flags) : fromBinary(false), binaryFormat(BinaryFormat::AMD),
            amdInput(disasmInput), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
}

Disassembler::Disassembler(const AmdCL2DisasmInput* disasmInput, std::ostream& _output,
            Flags _flags) : fromBinary(false), binaryFormat(BinaryFormat::AMDCL2),
            amdCL2Input(disasmInput), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
}

Disassembler::Disassembler(const ROCmDisasmInput* disasmInput, std::ostream& _output,
                 Flags _flags) : fromBinary(false), binaryFormat(BinaryFormat::ROCM),
            rocmInput(disasmInput), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
}

Disassembler::Disassembler(GPUDeviceType deviceType, const GalliumBinary& binary,
           std::ostream& _output, Flags _flags, cxuint llvmVersion) :
           fromBinary(true), binaryFormat(BinaryFormat::GALLIUM),
           galliumInput(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    galliumInput = getGalliumDisasmInputFromBinary(deviceType, binary, llvmVersion);
}

Disassembler::Disassembler(const GalliumDisasmInput* disasmInput, std::ostream& _output,
             Flags _flags) : fromBinary(false), binaryFormat(BinaryFormat::GALLIUM),
            galliumInput(disasmInput), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
}

Disassembler::Disassembler(GPUDeviceType deviceType, size_t rawCodeSize,
           const cxbyte* rawCode, std::ostream& _output, Flags _flags)
       : fromBinary(true), binaryFormat(BinaryFormat::RAWCODE),
         output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    rawInput = new RawCodeInput{ deviceType, rawCodeSize, rawCode };
}

Disassembler::~Disassembler()
{
    if (fromBinary)
    {
        switch(binaryFormat)
        {
            case BinaryFormat::AMD:
                delete amdInput;
                break;
            case BinaryFormat::GALLIUM:
                delete galliumInput;
                break;
            case BinaryFormat::ROCM:
                delete rocmInput;
                break;
            case BinaryFormat::AMDCL2:
                delete amdCL2Input;
                break;
            default:
                delete rawInput;
        }
    }
}

GPUDeviceType Disassembler::getDeviceType() const
{
    // get device type from input
    switch(binaryFormat)
    {
        case BinaryFormat::AMD:
            return amdInput->deviceType;
        case BinaryFormat::AMDCL2:
            return amdCL2Input->deviceType;
        case BinaryFormat::ROCM:
            return rocmInput->deviceType;
        case BinaryFormat::GALLIUM:
            return galliumInput->deviceType;
        default:
            return rawInput->deviceType;
    }
}

void CLRX::printDisasmData(size_t size, const cxbyte* data, std::ostream& output,
                bool secondAlign)
{
    char buf[68];
    /// const strings for .byte and fill pseudo-ops
    const char* linePrefix = "    .byte ";
    const char* fillPrefix = "    .fill ";
    size_t prefixSize = 10;
    if (secondAlign)
    {
        // const string for double alignment
        linePrefix = "        .byte ";
        fillPrefix = "        .fill ";
        prefixSize += 4;
    }
    ::memcpy(buf, linePrefix, prefixSize);
    for (size_t p = 0; p < size;)
    {
        size_t fillEnd;
        // find max repetition of this element
        for (fillEnd = p+1; fillEnd < size && data[fillEnd]==data[p]; fillEnd++);
        if (fillEnd >= p+8)
        {
            // if element repeated for least 1 line
            // print .fill pseudo-op: .fill SIZE, 1, VALUE
            ::memcpy(buf, fillPrefix, prefixSize);
            const size_t oldP = p;
            p = (fillEnd != size) ? fillEnd&~size_t(7) : fillEnd;
            size_t bufPos = prefixSize;
            bufPos += itocstrCStyle(p-oldP, buf+bufPos, 22, 10);
            memcpy(buf+bufPos, ", 1, ", 5);
            bufPos += 5;
            // value to fill
            bufPos += itocstrCStyle(data[oldP], buf+bufPos, 6, 16, 2);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
            ::memcpy(buf, linePrefix, prefixSize);
            continue;
        }
        
        const size_t lineEnd = std::min(p+8, size);
        size_t bufPos = prefixSize;
        // print 8 or less (if end of data) bytes
        for (; p < lineEnd; p++)
        {
            buf[bufPos++] = '0';
            buf[bufPos++] = 'x';
            {
                // inline byte in hexadecimal
                cxuint digit = data[p]>>4;
                if (digit < 10)
                    buf[bufPos++] = '0'+digit;
                else
                    buf[bufPos++] = 'a'+digit-10;
                digit = data[p]&0xf;
                if (digit < 10)
                    buf[bufPos++] = '0'+digit;
                else
                    buf[bufPos++] = 'a'+digit-10;
            }
            if (p+1 < lineEnd)
            {
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
            }
        }
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
    }
}

void CLRX::printDisasmDataU32(size_t size, const uint32_t* data, std::ostream& output,
                bool secondAlign)
{
    char buf[68];
    /// const strings for .byte and fill pseudo-ops
    const char* linePrefix = "    .int ";
    const char* fillPrefix = "    .fill ";
    size_t fillPrefixSize = 10;
    if (secondAlign)
    {
        // const string for double alignment
        linePrefix = "        .int ";
        fillPrefix = "        .fill ";
        fillPrefixSize += 4;
    }
    const size_t intPrefixSize = fillPrefixSize-1;
    ::memcpy(buf, linePrefix, intPrefixSize);
    for (size_t p = 0; p < size;)
    {
        size_t fillEnd;
        // find max repetition of this char
        for (fillEnd = p+1; fillEnd < size && ULEV(data[fillEnd])==ULEV(data[p]);
             fillEnd++);
        if (fillEnd >= p+4)
        {
            // if element repeated for least 1 line
            // print .fill pseudo-op
            ::memcpy(buf, fillPrefix, fillPrefixSize);
            const size_t oldP = p;
            p = (fillEnd != size) ? fillEnd&~size_t(3) : fillEnd;
            size_t bufPos = fillPrefixSize;
            bufPos += itocstrCStyle(p-oldP, buf+bufPos, 22, 10);
            memcpy(buf+bufPos, ", 4, ", 5);
            bufPos += 5;
            // print fill value
            bufPos += itocstrCStyle(ULEV(data[oldP]), buf+bufPos, 12, 16, 8);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
            ::memcpy(buf, linePrefix, fillPrefixSize);
            continue;
        }
        
        const size_t lineEnd = std::min(p+4, size);
        size_t bufPos = intPrefixSize;
        // print four or less (if end of data) dwords
        for (; p < lineEnd; p++)
        {
            bufPos += itocstrCStyle(ULEV(data[p]), buf+bufPos, 12, 16, 8);
            if (p+1 < lineEnd)
            {
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
            }
        }
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
    }
}

void CLRX::printDisasmLongString(size_t size, const char* data, std::ostream& output,
            bool secondAlign)
{
    const char* linePrefix = "    .ascii \"";
    size_t prefixSize = 12;
    if (secondAlign)
    {
        linePrefix = "        .ascii \"";
        prefixSize += 4;
    }
    // we need 96 bytes
    char buffer[96];
    ::memcpy(buffer, linePrefix, prefixSize);
    
    for (size_t pos = 0; pos < size; )
    {
        const size_t end = std::min(pos+72, size);
        const size_t oldPos = pos;
        // go to end of data, or newline
        while (pos < end && data[pos] != '\n') pos++;
        if (pos < end && data[pos] == '\n') pos++; // embrace newline
        size_t escapeSize;
        // escape this part
        pos = oldPos + escapeStringCStyle(pos-oldPos, data+oldPos, 76,
                      buffer+prefixSize, escapeSize);
        buffer[prefixSize+escapeSize] = '\"';
        buffer[prefixSize+escapeSize+1] = '\n';
        output.write(buffer, prefixSize+escapeSize+2);
    }
}

static void disassembleRawCode(std::ostream& output, const RawCodeInput* rawInput,
       ISADisassembler* isaDisassembler, Flags flags)
{
    if ((flags & DISASM_DUMPCODE) != 0)
    {
        output.write(".text\n", 6);
        isaDisassembler->setInput(rawInput->codeSize, rawInput->code);
        isaDisassembler->beforeDisassemble();
        isaDisassembler->disassemble();
    }
}

void Disassembler::disassemble()
{
    const std::ios::iostate oldExceptions = output.exceptions();
    output.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
    sectionCount = 0;
    // write pseudo to set binary format
    switch(binaryFormat)
    {
        case BinaryFormat::AMD:
            output.write(".amd\n", 5);
            break;
        case BinaryFormat::AMDCL2:
            output.write(".amdcl2\n", 8);
            break;
        case BinaryFormat::ROCM:
            output.write(".rocm\n", 6);
            break;
        case BinaryFormat::GALLIUM: // Gallium
            output.write(".gallium\n", 9);
            break;
        default:
            output.write(".rawcode\n", 9);
    }
    
    // print GPU device (.gpu name)
    const GPUDeviceType deviceType = getDeviceType();
    output.write(".gpu ", 5);
    const char* gpuName = getGPUDeviceTypeName(deviceType);
    output.write(gpuName, ::strlen(gpuName));
    output.put('\n');
    
    // call main disasembly routine
    switch(binaryFormat)
    {
        case BinaryFormat::AMD:
            disassembleAmd(output, amdInput, isaDisassembler.get(), sectionCount, flags);
            break;
        case BinaryFormat::AMDCL2:
            disassembleAmdCL2(output, amdCL2Input, isaDisassembler.get(),
                              sectionCount, flags);
            break;
        case BinaryFormat::ROCM:
            disassembleROCm(output, rocmInput, isaDisassembler.get(), flags);
            break;
        case BinaryFormat::GALLIUM: // Gallium
            disassembleGallium(output, galliumInput, isaDisassembler.get(), flags);
            break;
        default:
            disassembleRawCode(output, rawInput, isaDisassembler.get(), flags);
    }
    output.flush();
    } /* try catch */
    catch(...)
    {
        output.exceptions(oldExceptions);
        throw;
    }
    output.exceptions(oldExceptions);
}
