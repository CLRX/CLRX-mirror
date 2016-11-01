/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2016 Mateusz Szpakowski
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
#include <iostream>
#include <cstdint>
#include <cstdio>
#include <string>
#include <ostream>
#include <memory>
#include <vector>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/GPUId.h>
#include "DisasmInternals.h"

using namespace CLRX;

struct AMDGPUArchValues
{
    uint32_t major;
    uint32_t minor;
    uint32_t stepping;
};

static const AMDGPUArchValues amdGpuArchValuesTbl[] =
{
    { 0, 0, 0 }, // GPUDeviceType::CAPE_VERDE
    { 0, 0, 0 }, // GPUDeviceType::PITCAIRN
    { 0, 0, 0 }, // GPUDeviceType::TAHITI
    { 0, 0, 0 }, // GPUDeviceType::OLAND
    { 7, 0, 0 }, // GPUDeviceType::BONAIRE
    { 7, 0, 0 }, // GPUDeviceType::SPECTRE
    { 7, 0, 0 }, // GPUDeviceType::SPOOKY
    { 7, 0, 0 }, // GPUDeviceType::KALINDI
    { 0, 0, 0 }, // GPUDeviceType::HAINAN
    { 7, 0, 1 }, // GPUDeviceType::HAWAII
    { 8, 0, 0 }, // GPUDeviceType::ICELAND
    { 8, 0, 0 }, // GPUDeviceType::TONGA
    { 7, 0, 0 }, // GPUDeviceType::MULLINS
    { 8, 0, 4 }, // GPUDeviceType::FIJI
    { 8, 0, 1 }, // GPUDeviceType::CARRIZO
    { 0, 0, 0 }, // GPUDeviceType::DUMMY
    { 0, 0, 0 }, // GPUDeviceType::GOOSE
    { 0, 0, 0 }, // GPUDeviceType::HORSE
    { 8, 1, 0 }, // GPUDeviceType::STONEY
    { 8, 0, 4 }, // GPUDeviceType::ELLESMERE
    { 8, 0, 4 } // GPUDeviceType::BAFFIN
};

ROCmDisasmInput* CLRX::getROCmDisasmInputFromBinary(const ROCmBinary& binary)
{
    std::unique_ptr<ROCmDisasmInput> input(new ROCmDisasmInput);
    uint32_t archMajor = 0;
    
    {
        const cxbyte* noteContent = binary.getSectionContent(".note");
        size_t notesSize = binary.getSectionHeader(".note").sh_size;
        // find note about AMDGPU
        for (size_t offset = 0; offset < notesSize; )
        {
            const Elf64_Nhdr* nhdr = (const Elf64_Nhdr*)(noteContent + offset);
            size_t namesz = ULEV(nhdr->n_namesz);
            size_t descsz = ULEV(nhdr->n_descsz);
            if (usumGt(offset, namesz+descsz, notesSize))
                throw Exception("Note offset+size out of range");
            if (ULEV(nhdr->n_type) == 0x3 && namesz==4 && descsz>=0x1a &&
                ::strcmp((const char*)noteContent+offset+sizeof(Elf64_Nhdr), "AMD")==0)
            {    // AMDGPU type
                const uint32_t* content = (const uint32_t*)
                        (noteContent+offset+sizeof(Elf64_Nhdr) + 4);
                archMajor = ULEV(content[1]);
                input->archMinor = ULEV(content[2]);
                input->archStepping = ULEV(content[3]);
            }
            size_t align = (((namesz+descsz)&3)!=0) ? 4-((namesz+descsz)&3) : 0;
            offset += sizeof(Elf64_Nhdr) + namesz + descsz + align;
        }
    }
    // determine device type
    std::cout << "archmajor: " << archMajor << ", archminor: " << input->archMinor <<
            ", archstepping: " << input->archStepping << "\n";
    cxuint deviceNumber = 0;
    for (deviceNumber = 0; deviceNumber <= cxuint(GPUDeviceType::GPUDEVICE_MAX);
                 deviceNumber++)
        if (amdGpuArchValuesTbl[deviceNumber].major==archMajor &&
            amdGpuArchValuesTbl[deviceNumber].minor==input->archMinor &&
            amdGpuArchValuesTbl[deviceNumber].stepping==input->archStepping)
            break;
    if (deviceNumber > cxuint(GPUDeviceType::GPUDEVICE_MAX))
        throw Exception("Can't determine device type from arch values!");
    input->deviceType = GPUDeviceType(deviceNumber);
    
    const size_t regionsNum = binary.getRegionsNum();
    input->regions.resize(regionsNum);
    size_t codeOffset = binary.getCode()-binary.getBinaryCode();
    for (size_t i = 0; i < regionsNum; i++)
    {
        const ROCmRegion& region = binary.getRegion(i);
        input->regions[i] = { region.regionName, region.size,
            region.offset - codeOffset, region.isKernel };
    }
    
    input->code = binary.getCode();
    input->codeSize = binary.getCodeSize();
    return input.release();
}

void CLRX::disassembleROCm(std::ostream& output, const ROCmDisasmInput* rocmInput,
           ISADisassembler* isaDisassembler, size_t& sectionCount, Flags flags)
{
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doMetadata = ((flags & (DISASM_METADATA|DISASM_CONFIG)) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    const bool doDumpConfig = ((flags & DISASM_CONFIG) != 0);
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(rocmInput->deviceType);
    const cxuint maxSgprsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    for (cxuint i = 0; i < rocmInput->regions.size(); i++)
    {
        const ROCmDisasmRegionInput& rinput = rocmInput->regions[i];
        if (rinput.isKernel)
        {
            output.write(".kernel ", 8);
            output.write(rinput.regionName.c_str(), rinput.regionName.size());
            output.put('\n');
        }
    }
    if (doDumpCode && rocmInput->code != nullptr && rocmInput->codeSize != 0)
    {
    }
}
