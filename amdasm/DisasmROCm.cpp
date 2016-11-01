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
    GPUDeviceType deviceType;
};

static const AMDGPUArchValues amdGpuArchValuesTbl[] =
{
    { 0, 0, 0, GPUDeviceType::CAPE_VERDE },
    { 7, 0, 0, GPUDeviceType::BONAIRE },
    { 7, 0, 1, GPUDeviceType::HAWAII },
    { 8, 0, 0, GPUDeviceType::ICELAND },
    { 8, 0, 1, GPUDeviceType::CARRIZO },
    { 8, 0, 2, GPUDeviceType::ICELAND },
    { 8, 0, 3, GPUDeviceType::FIJI },
    { 8, 0, 4, GPUDeviceType::FIJI },
    { 8, 1, 0, GPUDeviceType::STONEY }
};

static const size_t amdGpuArchValuesNum = sizeof(amdGpuArchValuesTbl) /
                sizeof(AMDGPUArchValues);

ROCmDisasmInput* CLRX::getROCmDisasmInputFromBinary(const ROCmBinary& binary)
{
    std::unique_ptr<ROCmDisasmInput> input(new ROCmDisasmInput);
    uint32_t archMajor = 0;
    input->archMinor = 0;
    input->archStepping = 0;
    
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
    input->deviceType = GPUDeviceType::CAPE_VERDE;
    if (archMajor==0)
        input->deviceType = GPUDeviceType::CAPE_VERDE;
    else if (archMajor==7)
        input->deviceType = GPUDeviceType::BONAIRE;
    else if (archMajor==8)
        input->deviceType = GPUDeviceType::ICELAND;
    
    for (cxuint i = 0; i < amdGpuArchValuesNum; i++)
        if (amdGpuArchValuesTbl[i].major==archMajor &&
            amdGpuArchValuesTbl[i].minor==input->archMinor &&
            amdGpuArchValuesTbl[i].stepping==input->archStepping)
        {
            input->deviceType = amdGpuArchValuesTbl[i].deviceType;
            break;
        }
    
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
    
    const size_t regionsNum = rocmInput->regions.size();
    typedef std::pair<size_t, size_t> SortEntry;
    std::unique_ptr<SortEntry[]> sorted(new SortEntry[regionsNum]);
    for (size_t i = 0; i < regionsNum; i++)
        sorted[i] = std::make_pair(rocmInput->regions[i].offset, i);
    mapSort(sorted.get(), sorted.get() + regionsNum);
    
    if (rocmInput->code != nullptr && rocmInput->codeSize != 0)
    {
        const cxbyte* code = rocmInput->code;
        output.write(".text\n", 6);
        for (size_t i = 0; i < regionsNum; i++)
        {
            const ROCmDisasmRegionInput& region = rocmInput->regions[sorted[i].second];
            output.write(region.regionName.c_str(), region.regionName.size());
            output.write(":\n", 2);
            if (region.isKernel)
            {
                if (doMetadata && !doDumpConfig)
                    printDisasmData(0x100, code + region.offset, output, true);
                if (doDumpCode)
                {
                    isaDisassembler->setInput(region.size-256, code + region.offset+256,
                                    region.offset+256);
                    isaDisassembler->setDontPrintLabels(i+1<regionsNum);
                    isaDisassembler->beforeDisassemble();
                    isaDisassembler->disassemble();
                }
            }
            else if (doDumpData)
            {
                output.write(".global ", 8);
                output.write(region.regionName.c_str(), region.regionName.size());
                output.write("\n", 1);
                printDisasmData(region.size, code + region.offset, output, true);
            }
        }
    }
}
