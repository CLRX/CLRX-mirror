/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
#include <vector>
#include <algorithm>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinaries.h>
#include <CLRX/Assembler.h>
#include <CLRX/MemAccess.h>

using namespace CLRX;

ISADisassembler::ISADisassembler(Disassembler& disassembler_)
        : disassembler(disassembler_)
{ }

ISADisassembler::~ISADisassembler()
{ }

void ISADisassembler::setInput(size_t inputSize, const cxbyte* input)
{
    this->inputSize = inputSize;
    this->input = input;
}

/* helpers for main Disassembler class */

struct GPUDeviceCodeEntry
{
    uint16_t elfMachine;
    GPUDeviceType deviceType;
};

static const GPUDeviceCodeEntry gpuDeviceCodeTable[10] =
{
    { 0x3fd, GPUDeviceType::TAHITI },
    { 0x3fe, GPUDeviceType::PITCAIRN },
    { 0x3ff, GPUDeviceType::CAPE_VERDE },
    { 0x402, GPUDeviceType::OLAND },
    { 0x403, GPUDeviceType::BONAIRE },
    { 0x404, GPUDeviceType::SPECTRE },
    { 0x405, GPUDeviceType::SPOOKY },
    { 0x406, GPUDeviceType::KALINDI },
    { 0x407, GPUDeviceType::HAINAN },
    { 0x408, GPUDeviceType::HAWAII }
};

static const char* gpuDeviceNameTable[11] =
{
    "UNDEFINED",
    "CapeVerde",
    "Pitcairn",
    "Tahiti",
    "Oland",
    "Bonaire",
    "Spectre",
    "Spooky",
    "Kalindi",
    "Hainan",
    "Hawaii"
};

template<typename AmdMainBinary>
static DisasmInput* getDisasmInputFromBinary(const AmdMainBinary& binary)
{
    DisasmInput* input = new DisasmInput;
    try
    {   // for free input when exception
    cxuint index = 0;
    const uint16_t elfMachine = ULEV(binary.getHeader().e_machine);
    input->is64BitMode = (binary.getHeader().e_ident[EI_CLASS] == ELFCLASS64);
    const cxuint entriesNum = sizeof(gpuDeviceCodeTable)/sizeof(GPUDeviceCodeEntry);
    for (index = 0; index < entriesNum; index++)
        if (gpuDeviceCodeTable[index].elfMachine == elfMachine)
            break;
    if (entriesNum == index)
        throw Exception("Cant determine GPU device type");
    input->deviceType = gpuDeviceCodeTable[index].deviceType;
    input->metadata.compileOptions = binary.getCompileOptions();
    input->metadata.driverInfo = binary.getDriverInfo();
    input->globalDataSize = binary.getGlobalDataSize();
    input->globalData = binary.getGlobalData();
    const size_t kernelInfosNum = binary.getKernelInfosNum();
    const size_t kernelHeadersNum = binary.getKernelHeadersNum();
    const size_t innerBinariesNum = binary.getInnerBinariesNum();
    input->kernelInputs.resize(kernelInfosNum);
    
    for (cxuint i = 0; i < kernelInfosNum; i++)
    {
        const KernelInfo& kernelInfo = binary.getKernelInfo(i);
        const AmdInnerGPUBinary32* innerBin = nullptr;
        if (i < innerBinariesNum)
            innerBin = &binary.getInnerBinary(i);
        if (innerBin == nullptr || innerBin->getKernelName() != kernelInfo.kernelName)
        {   // fallback if not in order
            try
            { innerBin = &binary.getInnerBinary(kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex)
            { innerBin = nullptr; }
        }
        DisasmKernelInput& kernelInput = input->kernelInputs[i];
        kernelInput.metadataSize = binary.getMetadataSize(i);
        kernelInput.metadata = binary.getMetadata(i);
        
        // kernel header
        kernelInput.headerSize = 0;
        kernelInput.header = nullptr;
        const AmdGPUKernelHeader* khdr = nullptr;
        if (i < kernelHeadersNum)
            khdr = &binary.getKernelHeaderEntry(i);
        if (khdr == nullptr || khdr->kernelName != kernelInfo.kernelName)
        {   // fallback if not in order
            try
            { khdr = &binary.getKernelHeaderEntry(kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex) // failed
            { khdr = nullptr; }
        }
        if (khdr != nullptr)
        {
            kernelInput.headerSize = khdr->size;
            kernelInput.header = khdr->data;
        }
        
        kernelInput.kernelName = kernelInfo.kernelName;
        
        kernelInput.codeSize = kernelInput.dataSize = 0;
        kernelInput.code = kernelInput.data = nullptr;
        
        if (innerBin != nullptr)
        {   // if innerBinary exists
            bool codeFound = false;
            bool dataFound = false;
            /* in search in reverse order, because last .text have code from
             * (older Catalyst drivers) */
            for (cxint j = innerBin->getSectionHeadersNum()-1; j >= 0; j--)
            {
                const char* secName = innerBin->getSectionName(j);
                if (!codeFound && ::strcmp(secName, ".text") == 0)
                {   // if found last .text
                    const auto& shdr = innerBin->getSectionHeader(j);
                    kernelInput.codeSize = ULEV(shdr.sh_size);
                    kernelInput.code = innerBin->getSectionContent(j);
                    codeFound = true;
                }
                else if (!dataFound && ::strcmp(secName, ".data") == 0)
                {   // if found last .data
                    const auto& shdr = innerBin->getSectionHeader(j);
                    kernelInput.dataSize = ULEV(shdr.sh_size);
                    kernelInput.data = innerBin->getSectionContent(j);
                    dataFound = true;
                }
                
                if (codeFound && dataFound)
                    break; // end of finding
            }
        
            kernelInput.calNotes.resize(innerBin->getCALNotesNum());
            for (cxuint j = 0; j < kernelInput.calNotes.size(); j++)
            {
                const CALNoteHeader& calNoteHdr = innerBin->getCALNoteHeader(j);
                AsmCALNote& outCalNote = kernelInput.calNotes[j];
                outCalNote.header.nameSize = ULEV(calNoteHdr.nameSize);
                outCalNote.header.type = ULEV(calNoteHdr.type);
                outCalNote.header.descSize = ULEV(calNoteHdr.descSize);
                std::copy(calNoteHdr.name, calNoteHdr.name+8, outCalNote.header.name);
                outCalNote.data = const_cast<cxbyte*>(innerBin->getCALNoteData(j));
            }
        }
    }
    }
    catch(...)
    {
        delete input;
        throw; // if exception
    }
    return input;
}

Disassembler::Disassembler(const AmdMainGPUBinary32& binary, std::ostream& _output,
            cxuint flags) : fromBinary(true), input(nullptr), output(_output)
{
    this->flags = flags;
    input = getDisasmInputFromBinary(binary);
    try
    { isaDisassembler = new GCNDisassembler(*this); }
    catch(...)
    {
        delete input;
        throw;
    }
}

Disassembler::Disassembler(const AmdMainGPUBinary64& binary, std::ostream& _output,
            cxuint flags) : fromBinary(true), input(nullptr), output(_output)
{
    this->flags = flags;
    input = getDisasmInputFromBinary(binary);
    try
    { isaDisassembler = new GCNDisassembler(*this); }
    catch(...)
    {
        delete input;
        throw;
    }
}

Disassembler::Disassembler(const DisasmInput* disasmInput, std::ostream& _output,
            cxuint flags) : fromBinary(false), input(disasmInput), output(_output)
{
    this->flags = flags;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    isaDisassembler = new GCNDisassembler(*this);
}

Disassembler::~Disassembler()
{
    if (fromBinary)
        delete input;
    delete isaDisassembler;
}

static void printDisasmData(size_t size, const cxbyte* data, std::ostream& output,
                bool secondAlign = false)
{
    char buf[68];
    const char* linePrefix = "    .byte ";
    const char* fillPrefix = "    .fill ";
    size_t prefixSize = 10;
    if (secondAlign)
    {
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
        {   // if element repeated for least 1 line
            ::memcpy(buf, fillPrefix, prefixSize);
            const size_t oldP = p;
            p = (fillEnd != size) ? fillEnd&~size_t(7) : fillEnd;
            size_t bufPos = prefixSize;
            bufPos += itocstrCStyle(p-oldP, buf+bufPos, 22, 10);
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            buf[bufPos++] = '1';
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += itocstrCStyle(data[oldP], buf+bufPos, 6, 16, 2);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
            ::memcpy(buf, linePrefix, prefixSize);
            continue;
        }
        
        const size_t lineEnd = std::min(p+8, size);
        size_t bufPos = prefixSize;
        for (; p < lineEnd; p++)
        {
            buf[bufPos++] = '0';
            buf[bufPos++] = 'x';
            {   // inline byte in hexadecimal
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

static void printDisasmDataU32(size_t size, const uint32_t* data, std::ostream& output,
                bool secondAlign = false)
{
    char buf[68];
    const char* linePrefix = "    .int ";
    const char* fillPrefix = "    .fill ";
    size_t fillPrefixSize = 10;
    if (secondAlign)
    {
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
        {   // if element repeated for least 1 line
            ::memcpy(buf, fillPrefix, fillPrefixSize);
            const size_t oldP = p;
            p = (fillEnd != size) ? fillEnd&~size_t(3) : fillEnd;
            size_t bufPos = fillPrefixSize;
            bufPos += itocstrCStyle(p-oldP, buf+bufPos, 22, 10);
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            buf[bufPos++] = '4';
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += itocstrCStyle(ULEV(data[oldP]), buf+bufPos, 12, 16, 8);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
            ::memcpy(buf, linePrefix, fillPrefixSize);
            continue;
        }
        
        const size_t lineEnd = std::min(p+4, size);
        size_t bufPos = intPrefixSize;
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

static void printDisasmLongString(size_t size, const char* data, std::ostream& output,
            bool secondAlign = false)
{
    
    const char* linePrefix = "    .string \"";
    size_t prefixSize = 13;
    if (secondAlign)
    {
        linePrefix = "        .string \"";
        prefixSize += 4;
    }
    char buffer[96];
    ::memcpy(buffer, linePrefix, prefixSize);
    
    for (size_t pos = 0; pos < size; )
    {
        const size_t end = std::min(pos+72, size);
        const size_t oldPos = pos;
        while (pos < end && data[pos] != '\n') pos++;
        if (pos < end && data[pos] == '\n') pos++; // embrace newline
        size_t escapeSize;
        pos = oldPos + escapeStringCStyle(pos-oldPos, data+oldPos, 76,
                      buffer+prefixSize, escapeSize);
        buffer[prefixSize+escapeSize] = '\"';
        buffer[prefixSize+escapeSize+1] = '\n';
        output.write(buffer, prefixSize+escapeSize+2);
    }
}

static const char* disasmCALNoteNamesTable[] =
{
    ".proginfo",
    ".inputs",
    ".outputs",
    ".condout",
    ".floatconsts",
    ".intconsts",
    ".boolconsts",
    ".earlyexit",
    ".globalbuffers",
    ".constantbuffers",
    ".inputsamplers",
    ".persistentbuffers",
    ".scratchbuffers",
    ".subconstantbuffers",
    ".uavmailboxsize",
    ".uav",
    ".uavopmask"
};

void Disassembler::disassemble()
{
    const std::ios::iostate oldExceptions = output.exceptions();
    output.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
    if (input->deviceType == GPUDeviceType::UNDEFINED ||
        cxuint(input->deviceType) > cxuint(GPUDeviceType::GPUDEVICE_MAX))
        throw Exception("Undefined GPU device type");
    
    output.write(".gpu ", 5);
    const char* gpuName = gpuDeviceNameTable[cxuint(input->deviceType)];
    output.write(gpuName, ::strlen(gpuName));
    if (input->is64BitMode)
        output.write("\n.64bit\n", 8);
    else
        output.write("\n.32bit\n", 8);
    
    const bool doMetadata = ((flags & DISASM_METADATA) != 0);
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    
    if (doMetadata)
    {
        output.write(".compile_options \"", 18);
        const std::string escapedCompileOptions = 
                escapeStringCStyle(input->metadata.compileOptions);
        output.write(escapedCompileOptions.c_str(), escapedCompileOptions.size());
        output.write("\"\n.driver_info \"", 16);
        const std::string escapedDriverInfo =
                escapeStringCStyle(input->metadata.driverInfo);
        output.write(escapedDriverInfo.c_str(), escapedDriverInfo.size());
        output.write("\"\n", 2);
    }
    
    if (doDumpData && input->globalData != nullptr && input->globalDataSize != 0)
    {   //
        output.write(".data\n", 6);
        printDisasmData(input->globalDataSize, input->globalData, output);
    }
    
    for (const DisasmKernelInput& kinput: input->kernelInputs)
    {
        {
            output.write(".kernel \"", 9);
            const std::string escapedKernelName =
                    escapeStringCStyle(kinput.kernelName);
            output.write(escapedKernelName.c_str(), escapedKernelName.size());
            output.write("\"\n", 2);
        }
        if (doMetadata)
        {
            if (kinput.header != nullptr && kinput.headerSize != 0)
            {   // if kernel header available
                output.write("    .header\n", 12);
                printDisasmData(kinput.headerSize, kinput.header, output, true);
            }
            if (kinput.metadata != nullptr && kinput.metadataSize != 0)
            {   // if kernel metadata available
                output.write("    .metadata\n", 14);
                printDisasmLongString(kinput.metadataSize, kinput.metadata, output, true);
            }
        }
        if (doDumpData && kinput.data != nullptr && kinput.dataSize != 0)
        {   // if kernel data available
            output.write("    .kerneldata\n", 16);
            printDisasmData(kinput.dataSize, kinput.data, output, true);
        }
        
        if ((flags & DISASM_CALNOTES) != 0)
            for (const AsmCALNote& calNote: kinput.calNotes)
            {
                char buf[64];
                // calNote.header fields is already in native endian
                if (calNote.header.type != 0 && calNote.header.type <= CALNOTE_ATI_MAXTYPE)
                {
                    output.write("    ", 4);
                    output.write(disasmCALNoteNamesTable[calNote.header.type-1],
                                 ::strlen(disasmCALNoteNamesTable[calNote.header.type-1]));
                }
                else
                {
                    const size_t len = itocstrCStyle(calNote.header.type, buf, 32, 16);
                    output.write("    .calnote ", 13);
                    output.write(buf, len);
                }
                
                switch(calNote.header.type)
                {   // handle CAL note types
                    case CALNOTE_ATI_PROGINFO:
                    {
                        output.put('\n');
                        const cxuint progInfosNum =
                                calNote.header.descSize/sizeof(CALProgramInfoEntry);
                        const CALProgramInfoEntry* progInfos =
                            reinterpret_cast<const CALProgramInfoEntry*>(calNote.data);
                        ::memcpy(buf, "        .set ", 13);
                        for (cxuint k = 0; k < progInfosNum; k++)
                        {
                            const CALProgramInfoEntry& progInfo = progInfos[k];
                            size_t bufPos = 13 + itocstrCStyle(ULEV(progInfo.address),
                                         buf+13, 32, 16, 8);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(progInfo.value),
                                      buf+bufPos, 32, 16, 8);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                                progInfosNum*sizeof(CALProgramInfoEntry),
                                calNote.data + progInfosNum*sizeof(CALProgramInfoEntry),
                                output, true);
                        break;
                    }
                    case CALNOTE_ATI_INPUTS:
                    case CALNOTE_ATI_OUTPUTS:
                    case CALNOTE_ATI_GLOBAL_BUFFERS:
                    case CALNOTE_ATI_SCRATCH_BUFFERS:
                    case CALNOTE_ATI_PERSISTENT_BUFFERS:
                        output.put('\n');
                        printDisasmDataU32(calNote.header.descSize>>2,
                               reinterpret_cast<const uint32_t*>(calNote.data),
                               output, true);
                        printDisasmData(calNote.header.descSize&3,
                               calNote.data + (calNote.header.descSize&~3U), output, true);
                        break;
                    case CALNOTE_ATI_INT32CONSTS:
                    case CALNOTE_ATI_FLOAT32CONSTS:
                    case CALNOTE_ATI_BOOL32CONSTS:
                    {
                        output.put('\n');
                        const cxuint segmentsNum =
                                calNote.header.descSize/sizeof(CALDataSegmentEntry);
                        const CALDataSegmentEntry* segments =
                                reinterpret_cast<const CALDataSegmentEntry*>(calNote.data);
                        ::memcpy(buf, "        .segment ", 17);
                        for (cxuint k = 0; k < segmentsNum; k++)
                        {
                            const CALDataSegmentEntry& segment = segments[k];
                            size_t bufPos = 17 + itocstrCStyle(
                                        ULEV(segment.offset), buf + 17, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(segment.size), buf+bufPos, 32);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                                segmentsNum*sizeof(CALDataSegmentEntry),
                                calNote.data + segmentsNum*sizeof(CALDataSegmentEntry),
                                output, true);
                        break;
                    }
                    case CALNOTE_ATI_INPUT_SAMPLERS:
                    {
                        output.put('\n');
                        const cxuint samplersNum =
                                calNote.header.descSize/sizeof(CALSamplerMapEntry);
                        const CALSamplerMapEntry* samplers =
                                reinterpret_cast<const CALSamplerMapEntry*>(calNote.data);
                        ::memcpy(buf, "        .sampler ", 17);
                        for (cxuint k = 0; k < samplersNum; k++)
                        {
                            const CALSamplerMapEntry& sampler = samplers[k];
                            size_t bufPos = 17 + itocstrCStyle(
                                        ULEV(sampler.input), buf + 17, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(sampler.sampler),
                                        buf+bufPos, 32, 16);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                                samplersNum*sizeof(CALSamplerMapEntry),
                                calNote.data + samplersNum*sizeof(CALSamplerMapEntry),
                                output, true);
                        break;
                    }
                    case CALNOTE_ATI_CONSTANT_BUFFERS:
                    {
                        output.put('\n');
                        const cxuint constBufMasksNum =
                                calNote.header.descSize/sizeof(CALConstantBufferMask);
                        const CALConstantBufferMask* constBufMasks =
                            reinterpret_cast<const CALConstantBufferMask*>(calNote.data);
                        ::memcpy(buf, "        .cbmask ", 16);
                        for (cxuint k = 0; k < constBufMasksNum; k++)
                        {
                            const CALConstantBufferMask& cbufMask = constBufMasks[k];
                            size_t bufPos = 16 + itocstrCStyle(
                                        ULEV(cbufMask.index), buf + 16, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(cbufMask.size), buf+bufPos, 32);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                            constBufMasksNum*sizeof(CALConstantBufferMask),
                            calNote.data + constBufMasksNum*sizeof(CALConstantBufferMask),
                            output, true);
                        break;
                    }
                    case CALNOTE_ATI_EARLYEXIT:
                    case CALNOTE_ATI_UAV_OP_MASK:
                    case CALNOTE_ATI_UAV_MAILBOX_SIZE:
                        if (calNote.header.descSize == 4)
                        {
                            const size_t len = itocstrCStyle(
                                    ULEV(*reinterpret_cast<const uint32_t*>(
                                        calNote.data)), buf, 32);
                            output.put(' ');
                            output.write(buf, len);
                            output.put('\n');
                        }
                        else // otherwise if size is not 4 bytes
                        {
                            output.put('\n');
                            printDisasmData(calNote.header.descSize,
                                    calNote.data, output, true);
                        }
                        break;
                    default:
                        output.put('\n');
                        printDisasmData(calNote.header.descSize, calNote.data,
                                        output, true);
                        break;
                }
            }
        
        if ((flags & DISASM_DUMPCODE) != 0 &&
            kinput.code != nullptr && kinput.codeSize != 0)
        {   // input kernel code (main disassembly)
            output.write("    .text\n", 10);
            isaDisassembler->setInput(kinput.codeSize, kinput.code);
            isaDisassembler->beforeDisassemble();
            isaDisassembler->disassemble();
        }
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
