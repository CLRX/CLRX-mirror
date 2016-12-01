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
#include <cstdint>
#include <cstdio>
#include <inttypes.h>
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
        const cxbyte* noteContent = (const cxbyte*)binary.getNotes();
        if (noteContent==nullptr)
            throw Exception("Missing notes in inner binary!");
        size_t notesSize = binary.getNotesSize();
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
        input->regions[i] = { region.regionName, size_t(region.size),
            size_t(region.offset - codeOffset), region.type };
    }
    
    input->code = binary.getCode();
    input->codeSize = binary.getCodeSize();
    return input.release();
}

static void dumpKernelConfig(std::ostream& output, cxuint maxSgprsNum,
             GPUArchitecture arch, const ROCmKernelConfig& config)
{
    output.write("    .config\n", 12);
    // convert to native-endian
    uint32_t amdCodeVersionMajor = ULEV(config.amdCodeVersionMajor);
    uint32_t amdCodeVersionMinor = ULEV(config.amdCodeVersionMinor);
    uint16_t amdMachineKind = ULEV(config.amdMachineKind);
    uint16_t amdMachineMajor = ULEV(config.amdMachineMajor);
    uint16_t amdMachineMinor = ULEV(config.amdMachineMinor);
    uint16_t amdMachineStepping = ULEV(config.amdMachineStepping);
    uint64_t kernelCodeEntryOffset = ULEV(config.kernelCodeEntryOffset);
    uint64_t kernelCodePrefetchOffset = ULEV(config.kernelCodePrefetchOffset);
    uint64_t kernelCodePrefetchSize = ULEV(config.kernelCodePrefetchSize);
    uint64_t maxScrachBackingMemorySize = ULEV(config.maxScrachBackingMemorySize);
    uint32_t computePgmRsrc1 = ULEV(config.computePgmRsrc1);
    uint32_t computePgmRsrc2 = ULEV(config.computePgmRsrc2);
    uint16_t enableSpgrRegisterFlags = ULEV(config.enableSpgrRegisterFlags);
    uint16_t enableFeatureFlags = ULEV(config.enableFeatureFlags);
    uint32_t workitemPrivateSegmentSize = ULEV(config.workitemPrivateSegmentSize);
    uint32_t workgroupGroupSegmentSize = ULEV(config.workgroupGroupSegmentSize);
    uint32_t gdsSegmentSize = ULEV(config.gdsSegmentSize);
    uint64_t kernargSegmentSize = ULEV(config.kernargSegmentSize);
    uint32_t workgroupFbarrierCount = ULEV(config.workgroupFbarrierCount);
    uint16_t wavefrontSgprCount = ULEV(config.wavefrontSgprCount);
    uint16_t workitemVgprCount = ULEV(config.workitemVgprCount);
    uint16_t reservedVgprFirst = ULEV(config.reservedVgprFirst);
    uint16_t reservedVgprCount = ULEV(config.reservedVgprCount);
    uint16_t reservedSgprFirst = ULEV(config.reservedSgprFirst);
    uint16_t reservedSgprCount = ULEV(config.reservedSgprCount);
    uint16_t debugWavefrontPrivateSegmentOffsetSgpr =
            ULEV(config.debugWavefrontPrivateSegmentOffsetSgpr);
    uint16_t debugPrivateSegmentBufferSgpr = ULEV(config.debugPrivateSegmentBufferSgpr);
    uint32_t callConvention = ULEV(config.callConvention);
    uint64_t runtimeLoaderKernelSymbol = ULEV(config.runtimeLoaderKernelSymbol);
    
    size_t bufSize;
    char buf[100];
    const cxuint ldsShift = arch<GPUArchitecture::GCN1_1 ? 8 : 9;
    const uint32_t pgmRsrc1 = computePgmRsrc1;
    const uint32_t pgmRsrc2 = computePgmRsrc2;
    
    const cxuint dimMask = (pgmRsrc2 >> 7) & 7;
    strcpy(buf, "        .dims ");
    bufSize = 14;
    if ((dimMask & 1) != 0)
        buf[bufSize++] = 'x';
    if ((dimMask & 2) != 0)
        buf[bufSize++] = 'y';
    if ((dimMask & 4) != 0)
        buf[bufSize++] = 'z';
    buf[bufSize++] = '\n';
    output.write(buf, bufSize);
    
    bufSize = snprintf(buf, 100, "        .sgprsnum %u\n",
              std::min((((pgmRsrc1>>6) & 0xf)<<3)+8, maxSgprsNum));
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .vgprsnum %u\n", ((pgmRsrc1 & 0x3f)<<2)+4);
    output.write(buf, bufSize);
    if ((pgmRsrc1 & (1U<<20)) != 0)
        output.write("        .privmode\n", 18);
    if ((pgmRsrc1 & (1U<<22)) != 0)
        output.write("        .debugmode\n", 19);
    if ((pgmRsrc1 & (1U<<21)) != 0)
        output.write("        .dx10clamp\n", 19);
    if ((pgmRsrc1 & (1U<<23)) != 0)
        output.write("        .ieeemode\n", 18);
    if ((pgmRsrc2 & 0x400) != 0)
        output.write("        .tgsize\n", 16);
    
    bufSize = snprintf(buf, 100, "        .floatmode 0x%02x\n", (pgmRsrc1>>12) & 0xff);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .priority %u\n", (pgmRsrc1>>10) & 3);
    output.write(buf, bufSize);
    if (((pgmRsrc1>>24) & 0x7f) != 0)
    {
        bufSize = snprintf(buf, 100, "        .exceptions 0x%02x\n",
                   (pgmRsrc1>>24) & 0x7f);
        output.write(buf, bufSize);
    }
    const cxuint localSize = ((pgmRsrc2>>15) & 0x1ff) << ldsShift;
    if (localSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .localsize %u\n", localSize);
        output.write(buf, bufSize);
    }
    bufSize = snprintf(buf, 100, "        .userdatanum %u\n", (pgmRsrc2>>1) & 0x1f);
    output.write(buf, bufSize);
    
    bufSize = snprintf(buf, 100, "        .pgmrsrc1 0x%08x\n", pgmRsrc1);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .pgmrsrc2 0x%08x\n", pgmRsrc2);
    output.write(buf, bufSize);
    
    bufSize = snprintf(buf, 100, "        .codeversion %u, %u\n",
                   amdCodeVersionMajor, amdCodeVersionMinor);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .machine %hu, %hu, %hu, %hu\n",
                   amdMachineKind, amdMachineMajor,
                   amdMachineMinor, amdMachineStepping);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .kernel_code_entry_offset 0x%" PRIx64 "\n",
                       kernelCodeEntryOffset);
    output.write(buf, bufSize);
    if (kernelCodePrefetchOffset!=0)
    {
        bufSize = snprintf(buf, 100,
                   "        .kernel_code_prefetch_offset 0x%" PRIx64 "\n",
                           kernelCodePrefetchOffset);
        output.write(buf, bufSize);
    }
    if (kernelCodePrefetchSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .kernel_code_prefetch_size %" PRIu64 "\n",
                           kernelCodePrefetchSize);
        output.write(buf, bufSize);
    }
    if (maxScrachBackingMemorySize!=0)
    {
        bufSize = snprintf(buf, 100, "        .max_scratch_backing_memory %" PRIu64 "\n",
                           maxScrachBackingMemorySize);
        output.write(buf, bufSize);
    }
    
    const uint16_t sgprFlags = enableSpgrRegisterFlags;
    if ((sgprFlags&1) != 0)
        output.write("        .use_private_segment_buffer\n", 36);
    if ((sgprFlags&2) != 0)
        output.write("        .use_dispatch_ptr\n", 26);
    if ((sgprFlags&4) != 0)
        output.write("        .use_queue_ptr\n", 23);
    if ((sgprFlags&8) != 0)
        output.write("        .use_kernarg_segment_ptr\n", 33);
    if ((sgprFlags&16) != 0)
        output.write("        .use_dispatch_id\n", 25);
    if ((sgprFlags&32) != 0)
        output.write("        .use_flat_scratch_init\n", 31);
    if ((sgprFlags&64) != 0)
        output.write("        .use_private_segment_size\n", 34);
    
    if ((sgprFlags&(7U<<7)) != 0)
    {
        strcpy(buf, "        .use_grid_workgroup_count ");
        bufSize = 34;
        if ((sgprFlags&128) != 0)
            buf[bufSize++] = 'x';
        if ((sgprFlags&256) != 0)
            buf[bufSize++] = 'y';
        if ((sgprFlags&512) != 0)
            buf[bufSize++] = 'z';
        buf[bufSize++] = '\n';
        output.write(buf, bufSize);
    }
    
    const uint16_t featureFlags = enableFeatureFlags;
    if ((featureFlags&1) != 0)
        output.write("        .use_ordered_append_gds\n", 32);
    bufSize = snprintf(buf, 100, "        .private_elem_size %u\n",
                       2U<<((featureFlags>>1)&3));
    output.write(buf, bufSize);
    if ((featureFlags&8) != 0)
        output.write("        .use_ptr64\n", 19);
    if ((featureFlags&16) != 0)
        output.write("        .use_dynamic_call_stack\n", 32);
    if ((featureFlags&32) != 0)
        output.write("        .use_debug_enabled\n", 27);
    if ((featureFlags&64) != 0)
        output.write("        .use_xnack_enabled\n", 27);
    
    if (workitemPrivateSegmentSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .workitem_private_segment_size %u\n",
                         workitemPrivateSegmentSize);
        output.write(buf, bufSize);
    }
    if (workgroupGroupSegmentSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .workgroup_group_segment_size %u\n",
                         workgroupGroupSegmentSize);
        output.write(buf, bufSize);
    }
    if (gdsSegmentSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .gds_segment_size %u\n",
                         gdsSegmentSize);
        output.write(buf, bufSize);
    }
    if (kernargSegmentSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .kernarg_segment_size %" PRIu64 "\n",
                         kernargSegmentSize);
        output.write(buf, bufSize);
    }
    if (workgroupFbarrierCount!=0)
    {
        bufSize = snprintf(buf, 100, "        .workgroup_fbarrier_count %u\n",
                         workgroupFbarrierCount);
        output.write(buf, bufSize);
    }
    if (wavefrontSgprCount!=0)
    {
        bufSize = snprintf(buf, 100, "        .wavefront_sgpr_count %hu\n",
                         wavefrontSgprCount);
        output.write(buf, bufSize);
    }
    if (workitemVgprCount!=0)
    {
        bufSize = snprintf(buf, 100, "        .workitem_vgpr_count %hu\n",
                         workitemVgprCount);
        output.write(buf, bufSize);
    }
    if (reservedVgprFirst!=0)
    {
        bufSize = snprintf(buf, 100, "        .reserved_vgpr_first %hu\n",
                         reservedVgprFirst);
        output.write(buf, bufSize);
    }
    if (reservedVgprCount!=0)
    {
        bufSize = snprintf(buf, 100, "        .reserved_vgpr_count %hu\n",
                         reservedVgprCount);
        output.write(buf, bufSize);
    }
    if (reservedSgprFirst!=0)
    {
        bufSize = snprintf(buf, 100, "        .reserved_sgpr_first %hu\n",
                         reservedSgprFirst);
        output.write(buf, bufSize);
    }
    if (reservedSgprCount!=0)
    {
        bufSize = snprintf(buf, 100, "        .reserved_sgpr_count %hu\n",
                         reservedSgprCount);
        output.write(buf, bufSize);
    }
    if (debugWavefrontPrivateSegmentOffsetSgpr!=0)
    {
        bufSize = snprintf(buf, 100, "        "
                        ".debug_wavefront_private_segment_offset_sgpr %hu\n",
                         debugWavefrontPrivateSegmentOffsetSgpr);
        output.write(buf, bufSize);
    }
    if (debugPrivateSegmentBufferSgpr!=0)
    {
        bufSize = snprintf(buf, 100, "        .debug_private_segment_buffer_sgpr %hu\n",
                         debugPrivateSegmentBufferSgpr);
        output.write(buf, bufSize);
    }
    bufSize = snprintf(buf, 100, "        .kernarg_segment_align %u\n",
                     1U<<(config.kernargSegmentAlignment));
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .group_segment_align %u\n",
                     1U<<(config.groupSegmentAlignment));
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .private_segment_align %u\n",
                     1U<<(config.privateSegmentAlignment));
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .wavefront_size %u\n",
                     1U<<(config.wavefrontSize));
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .call_convention 0x%x\n",
                     callConvention);
    output.write(buf, bufSize);
    if (runtimeLoaderKernelSymbol!=0)
    {
        bufSize = snprintf(buf, 100,
                   "        .runtime_loader_kernel_symbol 0x%" PRIx64 "\n",
                         runtimeLoaderKernelSymbol);
        output.write(buf, bufSize);
    }
    // new section, control_directive, outside .config
    output.write("    .control_directive\n", 23);
    printDisasmData(sizeof config.controlDirective, config.controlDirective, output, true);
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
    
    {
        char buf[40];
        size_t size = snprintf(buf, 40, ".arch_minor %u\n", rocmInput->archMinor);
        output.write(buf, size);
        size = snprintf(buf, 40, ".arch_stepping %u\n", rocmInput->archStepping);
        output.write(buf, size);
    }
    
    for (const ROCmDisasmRegionInput& rinput: rocmInput->regions)
        if (rinput.type != ROCmRegionType::DATA)
        {
            output.write(".kernel ", 8);
            output.write(rinput.regionName.c_str(), rinput.regionName.size());
            output.put('\n');
            if (doMetadata && doDumpConfig)
                dumpKernelConfig(output, maxSgprsNum, arch,
                     *reinterpret_cast<const ROCmKernelConfig*>(
                             rocmInput->code + rinput.offset));
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
        // clear labels
        isaDisassembler->clearNumberedLabels();
        
        /// analyze code with collecting labels
        for (size_t i = 0; i < regionsNum; i++)
        {
            const ROCmDisasmRegionInput& region = rocmInput->regions[sorted[i].second];
            if (region.type==ROCmRegionType::KERNEL && doDumpCode)
            {   // kernel code
                isaDisassembler->setInput(region.size-256, code + region.offset+256,
                                    region.offset+256);
                isaDisassembler->analyzeBeforeDisassemble();
            }
            else if (region.type==ROCmRegionType::CODE && doDumpCode)
            {   // function code
                isaDisassembler->setInput(region.size, code + region.offset,
                                    region.offset);
                isaDisassembler->analyzeBeforeDisassemble();
            }
            isaDisassembler->addNamedLabel(region.offset, region.regionName);
        }
        isaDisassembler->prepareLabelsAndRelocations();
        
        ISADisassembler::LabelIter curLabel;
        ISADisassembler::NamedLabelIter curNamedLabel;
        const auto& labels = isaDisassembler->getLabels();
        const auto& namedLabels = isaDisassembler->getNamedLabels();
        // real disassemble
        size_t prevRegionPos = 0;
        for (size_t i = 0; i < regionsNum; i++)
        {
            const ROCmDisasmRegionInput& region = rocmInput->regions[sorted[i].second];
            // set labelIters to previous position
            isaDisassembler->setInput(prevRegionPos, code + region.offset,
                                    region.offset, prevRegionPos);
            curLabel = std::lower_bound(labels.begin(), labels.end(), prevRegionPos);
            curNamedLabel = std::lower_bound(namedLabels.begin(), namedLabels.end(),
                std::make_pair(prevRegionPos, CString()),
                  [](const std::pair<size_t,CString>& a,
                                 const std::pair<size_t, CString>& b)
                  { return a.first < b.first; });
            isaDisassembler->writeLabelsToPosition(0, curLabel, curNamedLabel);
            isaDisassembler->flushOutput();
            
            size_t dataSize = rocmInput->codeSize - region.offset;
            if (i+1<regionsNum)
            {
                const ROCmDisasmRegionInput& newRegion =
                        rocmInput->regions[sorted[i+1].second];
                dataSize = newRegion.offset - region.offset;
            }
            if (region.type!=ROCmRegionType::DATA)
            {
                if (doMetadata)
                {
                    if (!doDumpConfig)
                        printDisasmData(0x100, code + region.offset, output, true);
                    else    // skip, config was dumped in kernel configuration
                        output.write(".skip 256\n", 10);
                }
                
                if (doDumpCode)
                {
                    isaDisassembler->setInput(dataSize-256, code + region.offset+256,
                                    region.offset+256, region.offset+1);
                    isaDisassembler->setDontPrintLabels(i+1<regionsNum);
                    isaDisassembler->disassemble();
                }
                prevRegionPos = region.offset + dataSize + 1;
            }
            else if (doDumpData)
            {
                output.write(".global ", 8);
                output.write(region.regionName.c_str(), region.regionName.size());
                output.write("\n", 1);
                printDisasmData(dataSize, code + region.offset, output, true);
                prevRegionPos = region.offset + dataSize;
            }
        }
        
        if (regionsNum!=0 &&
            !rocmInput->regions[sorted[regionsNum-1].second].type!=ROCmRegionType::DATA)
        {   // if last region is kernel, then print labels after last region
            const ROCmDisasmRegionInput& region =
                        rocmInput->regions[sorted[regionsNum-1].second];
            isaDisassembler->writeLabelsToEnd(region.size, curLabel, curNamedLabel);
            isaDisassembler->flushOutput();
        }
    }
}
