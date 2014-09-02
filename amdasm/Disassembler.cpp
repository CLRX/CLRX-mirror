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
#include <vector>
#include <algorithm>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinaries.h>
#include <CLRX/Assembler.h>

using namespace CLRX;

struct GPUDeviceCodeEntry
{
    uint16_t elfMachine;
    GPUDeviceType deviceType;
};

static const GPUDeviceCodeEntry gpuDevbiceCodeTable[6] =
{
    { 0x3ff, GPUDeviceType::CAPE_VERDE },
    { 0x3fe, GPUDeviceType::PITCAIRN },
    { 0x3fd, GPUDeviceType::TAHITI },
    { 0x402, GPUDeviceType::OLAND },
    { 0x403, GPUDeviceType::BONAIRE },
    { 0x408, GPUDeviceType::HAWAII }
};

template<typename AmdMainBinary>
static const DisasmInput* getDisasmInputFromBinary(const AmdMainBinary& binary)
{
    DisasmInput* input = new DisasmInput;
    cxuint index = 0;
    const uint16_t elfMachine = ULEV(binary.getHeader().e_machine);
    input->is64BitMode = (binary.getHeader().e_ident[EI_CLASS] == ELFCLASS64);
    const cxuint entriesNum = sizeof(gpuDevbiceCodeTable)/sizeof(GPUDeviceCodeEntry);
    for (index = 0; index < entriesNum; index++)
        if (gpuDevbiceCodeTable[index].elfMachine == elfMachine)
            break;
    if (entriesNum == index)
        throw Exception("Cant determine GPU device type");
    input->deviceType = gpuDevbiceCodeTable[index].deviceType;
    input->metadata.compileOptions = binary.getCompileOptions();
    input->metadata.driverInfo = binary.getDriverInfo();
    input->globalDataSize = binary.getGlobalDataSize();
    input->globalData = binary.getGlobalData();
    const size_t kernelInfosNum = binary.getKernelInfosNum();
    input->kernelInputs.resize(kernelInfosNum);
    if (kernelInfosNum != binary.getInnerBinariesNum())
        throw Exception("KernelInfosNum != InnerBinariesNum");
    if (kernelInfosNum != binary.getKernelHeadersNum())
        throw Exception("KernelInfosNum != KernelHeadersNum");
    
    for (cxuint i = 0; i < kernelInfosNum; i++)
    {
        const KernelInfo& kernelInfo = binary.getKernelInfo(i);
        const AmdInnerGPUBinary32* innerBin = &binary.getInnerBinary(i);
        if (kernelInfo.kernelName != kernelInfo.kernelName)
            // fallback if not in order
            innerBin = &binary.getInnerBinary(kernelInfo.kernelName.c_str());
        DisasmKernelInput& kernelInput = input->kernelInputs[i];
        kernelInput.metadataSize = binary.getMetadataSize(i);
        kernelInput.metadata = binary.getMetadata(i);
        
        // kernel header
        const AmdGPUKernelHeader* khdr = &binary.getKernelHeaderEntry(i);
        if (khdr->kernelName != kernelInfo.kernelName)
            // fallback if not in order
            khdr = &binary.getKernelHeaderEntry(kernelInfo.kernelName.c_str());
        kernelInput.headerSize = khdr->size;
        kernelInput.header = khdr->data;
        
        bool codeFound = false, dataFound = false;
        kernelInput.codeSize = kernelInput.dataSize = 0;
        kernelInput.code = kernelInput.data = nullptr;
        
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
    return input;
}

Disassembler::Disassembler(const AmdMainGPUBinary32& binary, std::ostream& _output,
            cxuint flags) : fromBinary(true), output(_output)
{
    this->flags = flags;
    input = getDisasmInputFromBinary(binary);
}

Disassembler::Disassembler(const AmdMainGPUBinary64& binary, std::ostream& _output,
            cxuint flags) : fromBinary(true), output(_output)
{
    this->flags = flags;
    input = getDisasmInputFromBinary(binary);
}

Disassembler::Disassembler(const DisasmInput* disasmInput, std::ostream& _output,
            cxuint flags) : fromBinary(false), input(disasmInput), output(_output)
{
    this->flags = flags;
}

Disassembler::~Disassembler()
{
    if (fromBinary)
        delete input;
    delete isaDisassembler;
}

void Disassembler::disassemble()
{
}
