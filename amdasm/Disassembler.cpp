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
#include <ostream>
#include <locale>
#include <vector>
#include <algorithm>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinaries.h>
#include <CLRX/Assembler.h>

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

GCNDisassembler::GCNDisassembler(Disassembler& disassembler)
        : ISADisassembler(disassembler)
{
}

GCNDisassembler::~GCNDisassembler()
{ }

void GCNDisassembler::beforeDisassemble()
{
}

size_t GCNDisassembler::disassemble(size_t maxSize, char* buffer)
{
    return 0;
}

/* helpers for main Disassembler class */

struct GPUDeviceCodeEntry
{
    uint16_t elfMachine;
    GPUDeviceType deviceType;
};

static const GPUDeviceCodeEntry gpuDeviceCodeTable[6] =
{
    { 0x3ff, GPUDeviceType::CAPE_VERDE },
    { 0x3fe, GPUDeviceType::PITCAIRN },
    { 0x3fd, GPUDeviceType::TAHITI },
    { 0x402, GPUDeviceType::OLAND },
    { 0x403, GPUDeviceType::BONAIRE },
    { 0x408, GPUDeviceType::HAWAII }
};

static const char* gpuDeviceNameTable[7] =
{
    "UNDEFINED",
    "CapeVerde",
    "Pitcairn",
    "Tahiti",
    "Oland",
    "Bonaire",
    "Hawaii"
};

template<typename AmdMainBinary>
static const DisasmInput* getDisasmInputFromBinary(const AmdMainBinary& binary)
{
    DisasmInput* input = new DisasmInput;
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
        if (innerBin == nullptr || kernelInfo.kernelName != kernelInfo.kernelName)
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
    return input;
}

Disassembler::Disassembler(const AmdMainGPUBinary32& binary, std::ostream& _output,
            cxuint flags) : fromBinary(true), output(_output)
{
    this->flags = flags;
    output.imbue(std::locale::classic());
    output.exceptions(std::ios::failbit | std::ios::badbit);
    input = getDisasmInputFromBinary(binary);
    isaDisassembler = new GCNDisassembler(*this);
}

Disassembler::Disassembler(const AmdMainGPUBinary64& binary, std::ostream& _output,
            cxuint flags) : fromBinary(true), output(_output)
{
    this->flags = flags;
    output.imbue(std::locale::classic());
    output.exceptions(std::ios::failbit | std::ios::badbit);
    input = getDisasmInputFromBinary(binary);
    isaDisassembler = new GCNDisassembler(*this);
}

Disassembler::Disassembler(const DisasmInput* disasmInput, std::ostream& _output,
            cxuint flags) : fromBinary(false), input(disasmInput), output(_output)
{
    this->flags = flags;
    output.imbue(std::locale::classic());
    output.exceptions(std::ios::failbit | std::ios::badbit);
    isaDisassembler = new GCNDisassembler(*this);
}

Disassembler::~Disassembler()
{
    if (fromBinary)
        delete input;
    delete isaDisassembler;
}

static void printDisasmData(size_t size, const cxbyte* data, std::ostream& output)
{
    char buf[6];
    for (size_t p = 0; p < size; p++)
    {
        if ((p & 15) == 0)
            output << "    .byte ";
        u32tocstrCStyle(data[p], buf, 6, 16, 2);
        output << buf;
        if (p+1 < size)
            output << ",";
        if ((p & 15) == 15 || p+1 < size)
            output << "\n";
    }
}

static void printDisasmLongString(size_t size, const char* data, std::ostream& output)
{
    for (size_t pos = 0; pos < size; )
    {
        const size_t end = std::min(size_t(80), size-pos);
        const size_t oldPos = pos;
        while (pos < end && data[pos] != '\n') pos++;
        output << "    .string \"" <<
                escapeStringCStyle(pos-oldPos, data+oldPos) << "\"\n";
    }
}

void Disassembler::disassemble()
{
    output << ".gpu " << gpuDeviceNameTable[cxuint(input->deviceType)] << "\n" <<
            ((input->is64BitMode) ? ".64bit\n" : ".32bit\n") <<
            ".compile_options \"" <<
                    escapeStringCStyle(input->metadata.compileOptions) << "\"\n" <<
            ".driver_info \"" <<
                    escapeStringCStyle(input->metadata.driverInfo) << "\"\n";
    
    if (input->globalData != nullptr && input->globalDataSize != 0)
    {   //
        output << ".data\n";
        printDisasmData(input->globalDataSize, input->globalData, output);
    }
    
    for (const DisasmKernelInput& kinput: input->kernelInputs)
    {
        output << ".kernel \"" << escapeStringCStyle(kinput.kernelName) << "\"\n";
        if (kinput.header != nullptr && kinput.headerSize != 0)
        {   // if kernel header available
            output << "    .header\n";
            printDisasmData(kinput.headerSize, kinput.header, output);
        }
        if (kinput.metadata != nullptr && kinput.metadataSize != 0)
        {   // if kernel metadata available
            output << "    .metadata\n";
            printDisasmLongString(kinput.metadataSize, kinput.metadata, output);
        }
    }
    output.flush();
}
