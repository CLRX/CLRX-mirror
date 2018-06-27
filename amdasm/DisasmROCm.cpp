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
#include <cstdint>
#include <cstdio>
#include <cstring>
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

ROCmDisasmInput* CLRX::getROCmDisasmInputFromBinary(const ROCmBinary& binary)
{
    std::unique_ptr<ROCmDisasmInput> input(new ROCmDisasmInput);
    input->deviceType = binary.determineGPUDeviceType(input->archMinor,
                              input->archStepping);
    
    const size_t regionsNum = binary.getRegionsNum();
    input->regions.resize(regionsNum);
    size_t codeOffset = binary.getCode()-binary.getBinaryCode();
    // ger regions of code
    for (size_t i = 0; i < regionsNum; i++)
    {
        const ROCmRegion& region = binary.getRegion(i);
        input->regions[i] = { region.regionName, size_t(region.size),
            size_t(region.offset - codeOffset), region.type };
    }
    
    const size_t gotSymbolsNum = binary.getGotSymbolsNum();
    input->gotSymbols.resize(gotSymbolsNum);
    
    // get rodata index and offset
    cxuint rodataIndex = SHN_UNDEF;
    size_t rodataOffset = 0;
    try
    {
        rodataIndex = binary.getSectionIndex(".rodata");
        rodataOffset = ULEV(binary.getSectionHeader(rodataIndex).sh_offset);
    }
    catch(Exception& ex)
    { }
    
    // setup got symbols
    for (size_t i = 0; i < gotSymbolsNum; i++)
    {
        const size_t gotSymIndex = binary.getGotSymbol(i);
        const Elf64_Sym& sym = binary.getDynSymbol(gotSymIndex);
        size_t offset = SIZE_MAX;
        // set offset if symbol refer to some place in globaldata (rodata)
        if (rodataIndex != SHN_UNDEF && ULEV(sym.st_shndx) == rodataIndex)
            offset = ULEV(sym.st_value) - rodataOffset;
        input->gotSymbols[i] = { binary.getDynSymbolName(gotSymIndex), offset };
    }
    
    // setup code
    input->eflags = ULEV(binary.getHeader().e_flags);
    input->code = binary.getCode();
    input->codeSize = binary.getCodeSize();
    input->metadata = binary.getMetadata();
    input->metadataSize = binary.getMetadataSize();
    input->globalData = binary.getGlobalData();
    input->globalDataSize = binary.getGlobalDataSize();
    input->target = binary.getTarget();
    input->newBinFormat = binary.isNewBinaryFormat();
    return input.release();
}

void CLRX::dumpAMDHSAConfig(std::ostream& output, cxuint maxSgprsNum,
             GPUArchitecture arch, const ROCmKernelConfig& config, bool amdhsaPrefix)
{
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
    uint16_t enableSgprRegisterFlags = ULEV(config.enableSgprRegisterFlags);
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
    
    const cxuint dimMask = getDefaultDimMask(arch, pgmRsrc2);
    // print dims (hsadims for gallium): .[hsa_]dims xyz
    if (!amdhsaPrefix)
    {
        strcpy(buf, "        .dims ");
        bufSize = 14;
    }
    else
    {
        strcpy(buf, "        .hsa_dims ");
        bufSize = 18;
    }
    if ((dimMask & 1) != 0)
        buf[bufSize++] = 'x';
    if ((dimMask & 2) != 0)
        buf[bufSize++] = 'y';
    if ((dimMask & 4) != 0)
        buf[bufSize++] = 'z';
    if ((dimMask & 7) != ((dimMask>>3) & 7))
    {
        buf[bufSize++] = ',';
        buf[bufSize++] = ' ';
        if ((dimMask & 8) != 0)
            buf[bufSize++] = 'x';
        if ((dimMask & 16) != 0)
            buf[bufSize++] = 'y';
        if ((dimMask & 32) != 0)
            buf[bufSize++] = 'z';
    }
    buf[bufSize++] = '\n';
    output.write(buf, bufSize);
    
    if (!amdhsaPrefix)
    {
        // print in original form
        // get sgprsnum and vgprsnum from PGMRSRC1
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
    }
    else
    {
        // print with 'hsa_' prefix (for gallium)
        // get sgprsnum and vgprsnum from PGMRSRC1
        bufSize = snprintf(buf, 100, "        .hsa_sgprsnum %u\n",
                std::min((((pgmRsrc1>>6) & 0xf)<<3)+8, maxSgprsNum));
        output.write(buf, bufSize);
        bufSize = snprintf(buf, 100, "        .hsa_vgprsnum %u\n", ((pgmRsrc1 & 0x3f)<<2)+4);
        output.write(buf, bufSize);
        if ((pgmRsrc1 & (1U<<20)) != 0)
            output.write("        .hsa_privmode\n", 22);
        if ((pgmRsrc1 & (1U<<22)) != 0)
            output.write("        .hsa_debugmode\n", 23);
        if ((pgmRsrc1 & (1U<<21)) != 0)
            output.write("        .hsa_dx10clamp\n", 23);
        if ((pgmRsrc1 & (1U<<23)) != 0)
            output.write("        .hsa_ieeemode\n", 22);
        if ((pgmRsrc2 & 0x400) != 0)
            output.write("        .hsa_tgsize\n", 20);
        
        bufSize = snprintf(buf, 100, "        .hsa_floatmode 0x%02x\n",
                    (pgmRsrc1>>12) & 0xff);
        output.write(buf, bufSize);
        bufSize = snprintf(buf, 100, "        .hsa_priority %u\n",
                    (pgmRsrc1>>10) & 3);
        output.write(buf, bufSize);
        if (((pgmRsrc1>>24) & 0x7f) != 0)
        {
            bufSize = snprintf(buf, 100, "        .hsa_exceptions 0x%02x\n",
                    (pgmRsrc1>>24) & 0x7f);
            output.write(buf, bufSize);
        }
        const cxuint localSize = ((pgmRsrc2>>15) & 0x1ff) << ldsShift;
        if (localSize!=0)
        {
            bufSize = snprintf(buf, 100, "        .hsa_localsize %u\n", localSize);
            output.write(buf, bufSize);
        }
        bufSize = snprintf(buf, 100, "        .hsa_userdatanum %u\n", (pgmRsrc2>>1) & 0x1f);
        output.write(buf, bufSize);
        
        bufSize = snprintf(buf, 100, "        .hsa_pgmrsrc1 0x%08x\n", pgmRsrc1);
        output.write(buf, bufSize);
        bufSize = snprintf(buf, 100, "        .hsa_pgmrsrc2 0x%08x\n", pgmRsrc2);
        output.write(buf, bufSize);
    }
    
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
    
    const uint16_t sgprFlags = enableSgprRegisterFlags;
    // print SGPRregister flags (features)
    if ((sgprFlags&ROCMFLAG_USE_PRIVATE_SEGMENT_BUFFER) != 0)
        output.write("        .use_private_segment_buffer\n", 36);
    if ((sgprFlags&ROCMFLAG_USE_DISPATCH_PTR) != 0)
        output.write("        .use_dispatch_ptr\n", 26);
    if ((sgprFlags&ROCMFLAG_USE_QUEUE_PTR) != 0)
        output.write("        .use_queue_ptr\n", 23);
    if ((sgprFlags&ROCMFLAG_USE_KERNARG_SEGMENT_PTR) != 0)
        output.write("        .use_kernarg_segment_ptr\n", 33);
    if ((sgprFlags&ROCMFLAG_USE_DISPATCH_ID) != 0)
        output.write("        .use_dispatch_id\n", 25);
    if ((sgprFlags&ROCMFLAG_USE_FLAT_SCRATCH_INIT) != 0)
        output.write("        .use_flat_scratch_init\n", 31);
    if ((sgprFlags&ROCMFLAG_USE_PRIVATE_SEGMENT_SIZE) != 0)
        output.write("        .use_private_segment_size\n", 34);
    
    if ((sgprFlags&(7U<<ROCMFLAG_USE_GRID_WORKGROUP_COUNT_BIT)) != 0)
    {
        // print .use_grid_workgroup_count xyz (dimensions)
        strcpy(buf, "        .use_grid_workgroup_count ");
        bufSize = 34;
        if ((sgprFlags&ROCMFLAG_USE_GRID_WORKGROUP_COUNT_X) != 0)
            buf[bufSize++] = 'x';
        if ((sgprFlags&ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Y) != 0)
            buf[bufSize++] = 'y';
        if ((sgprFlags&ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Z) != 0)
            buf[bufSize++] = 'z';
        buf[bufSize++] = '\n';
        output.write(buf, bufSize);
    }
    
    const uint16_t featureFlags = enableFeatureFlags;
    if ((featureFlags&ROCMFLAG_USE_ORDERED_APPEND_GDS) != 0)
        output.write("        .use_ordered_append_gds\n", 32);
    bufSize = snprintf(buf, 100, "        .private_elem_size %u\n",
                       2U<<((featureFlags>>ROCMFLAG_PRIVATE_ELEM_SIZE_BIT)&3));
    output.write(buf, bufSize);
    if ((featureFlags&ROCMFLAG_USE_PTR64) != 0)
        output.write("        .use_ptr64\n", 19);
    if ((featureFlags&ROCMFLAG_USE_DYNAMIC_CALL_STACK) != 0)
        output.write("        .use_dynamic_call_stack\n", 32);
    if ((featureFlags&ROCMFLAG_USE_DEBUG_ENABLED) != 0)
        output.write("        .use_debug_enabled\n", 27);
    if ((featureFlags&ROCMFLAG_USE_XNACK_ENABLED) != 0)
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
    if (reservedVgprCount!=0)
    {
        bufSize = snprintf(buf, 100, "        .reserved_vgprs %hu, %hu\n",
                     reservedVgprFirst, uint16_t(reservedVgprFirst+reservedVgprCount-1));
        output.write(buf, bufSize);
    }
    if (reservedSgprCount!=0)
    {
        bufSize = snprintf(buf, 100, "        .reserved_sgprs %hu, %hu\n",
                     reservedSgprFirst, uint16_t(reservedSgprFirst+reservedSgprCount-1));
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

static void dumpKernelConfig(std::ostream& output, cxuint maxSgprsNum,
             GPUArchitecture arch, const ROCmKernelConfig& config)
{
    output.write("    .config\n", 12);
    dumpAMDHSAConfig(output, maxSgprsNum, arch, config);
}

// routine to disassembly code in AMD HSA form (kernel with HSA config)
void CLRX::disassembleAMDHSACode(std::ostream& output,
            const std::vector<ROCmDisasmRegionInput>& regions,
            size_t codeSize, const cxbyte* code, ISADisassembler* isaDisassembler,
            Flags flags)
{
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doMetadata = ((flags & (DISASM_METADATA|DISASM_CONFIG)) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    const bool doDumpConfig = ((flags & DISASM_CONFIG) != 0);
    
    const size_t regionsNum = regions.size();
    typedef std::pair<size_t, size_t> SortEntry;
    std::unique_ptr<SortEntry[]> sorted(new SortEntry[regionsNum]);
    for (size_t i = 0; i < regionsNum; i++)
        sorted[i] = std::make_pair(regions[i].offset, i);
    mapSort(sorted.get(), sorted.get() + regionsNum);
    
    output.write(".text\n", 6);
    // clear labels
    isaDisassembler->clearNumberedLabels();
    
    /// analyze code with collecting labels
    // before first kernel
    if (doDumpCode && (regionsNum == 0 || sorted[0].first > 0))
    {
        const size_t regionSize = (regionsNum!=0 ? sorted[0].first : codeSize);
        isaDisassembler->setInput(regionSize, code, 0);
        isaDisassembler->analyzeBeforeDisassemble();
    }
    
    for (size_t i = 0; i < regionsNum; i++)
    {
        const ROCmDisasmRegionInput& region = regions[sorted[i].second];
        if ((region.type==ROCmRegionType::KERNEL ||
             region.type==ROCmRegionType::FKERNEL) && doDumpCode)
        {
            if (region.offset+256 > codeSize)
                throw DisasmException("Region Offset out of range");
            // kernel code begin after HSA config
            isaDisassembler->setInput(region.size-256, code + region.offset+256,
                                region.offset+256);
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
    // before first kernel
    size_t prevRegionPos = 0;
    if (doDumpCode && (regionsNum == 0 || sorted[0].first > 0))
    {
        const size_t regionSize = (regionsNum!=0 ? sorted[0].first : codeSize);
        if (doDumpCode)
        {
            // dump code of region
            isaDisassembler->setInput(regionSize, code, 0, 0);
            isaDisassembler->setDontPrintLabels(true);
            isaDisassembler->disassemble();
        }
        prevRegionPos = regionSize+1;
    }
    
    for (size_t i = 0; i < regionsNum; i++)
    {
        const ROCmDisasmRegionInput& region = regions[sorted[i].second];
        // set labelIters to previous position
        isaDisassembler->setInput(prevRegionPos, code + region.offset,
                                region.offset, prevRegionPos);
        curLabel = std::lower_bound(labels.begin(), labels.end(), prevRegionPos);
        curNamedLabel = std::lower_bound(namedLabels.begin(), namedLabels.end(),
            std::make_pair(prevRegionPos, CString()),
                [](const std::pair<size_t,CString>& a,
                                const std::pair<size_t, CString>& b)
                { return a.first < b.first; });
        // write labels to current region position
        isaDisassembler->writeLabelsToPosition(0, curLabel, curNamedLabel);
        isaDisassembler->flushOutput();
        
        size_t dataSize = codeSize - region.offset;
        if (i+1<regionsNum)
        {
            // if not last region, then set size as (next_offset - this_offset)
            const ROCmDisasmRegionInput& newRegion = regions[sorted[i+1].second];
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
                // dump code of region
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
            prevRegionPos = region.offset+1;
        }
    }
    
    if (regionsNum!=0 && regions[sorted[regionsNum-1].second].type==ROCmRegionType::DATA)
    {
        // if last region is data then finishing dumping data
        const ROCmDisasmRegionInput& region = regions[sorted[regionsNum-1].second];
        // set labelIters to previous position
        isaDisassembler->setInput(prevRegionPos, code + region.offset+region.size,
                                region.offset+region.size, prevRegionPos);
        curLabel = std::lower_bound(labels.begin(), labels.end(), prevRegionPos);
        curNamedLabel = std::lower_bound(namedLabels.begin(), namedLabels.end(),
            std::make_pair(prevRegionPos, CString()),
                [](const std::pair<size_t,CString>& a,
                                const std::pair<size_t, CString>& b)
                { return a.first < b.first; });
        // if last region is not kernel, then print labels after last region
        isaDisassembler->writeLabelsToPosition(0, curLabel, curNamedLabel);
        isaDisassembler->flushOutput();
        isaDisassembler->writeLabelsToEnd(region.size, curLabel, curNamedLabel);
        isaDisassembler->flushOutput();
    }
}

// helper for checking whether value is supplied
static inline bool hasValue(cxuint value)
{ return value!=BINGEN_NOTSUPPLIED && value!=BINGEN_DEFAULT; }

static inline bool hasValue(uint64_t value)
{ return value!=BINGEN64_NOTSUPPLIED && value!=BINGEN64_DEFAULT; }

static const char* disasmROCmValueKindNames[] =
{
    "value", "globalbuf", "dynshptr", "sampler", "image", "pipe", "queue",
    "gox", "goy", "goz", "none", "printfbuf", "defqueue", "complact"
};

static const char* disasmROCmValueTypeNames[] =
{ "struct", "i8", "u8", "i16", "u16", "f16", "i32", "u32", "f32", "i64", "u64", "f64" };

static const char* disasmROCmAddressSpaces[] =
{ "none", "private", "global", "constant", "local", "generic", "region" };

static const char* disasmROCmAccessQuals[] =
{ "default", "read_only", "write_only", "read_write" };

static void dumpKernelMetadataInfo(std::ostream& output, const ROCmKernelMetadata& kernel)
{
    output.write("    .config\n", 12);
    output.write("        .md_symname \"", 21);
    //output.write(kernel.symbolName.c_str(), kernel.symbolName.size());
    {
        std::string symName = escapeStringCStyle(kernel.symbolName);
        output.write(symName.c_str(), symName.size());
    }
    output.write("\"\n", 2);
    output.write("        .md_language \"", 22);
    {
        std::string langName = escapeStringCStyle(kernel.language);
        output.write(langName.c_str(), langName.size());
    }
    size_t bufSize = 0;
    char buf[100];
    if (kernel.langVersion[0] != BINGEN_NOTSUPPLIED)
    {
        output.write("\", ", 3);
        bufSize = snprintf(buf, 100, "%u, %u\n",
                           kernel.langVersion[0], kernel.langVersion[1]);
        output.write(buf, bufSize);
    }
    else // version not supplied
        output.write("\"\n", 2);
    
    // print reqd_work_group_size: .cws XSIZE[,YSIZE[,ZSIZE]]
    if (kernel.reqdWorkGroupSize[0] != 0 || kernel.reqdWorkGroupSize[1] != 0 ||
        kernel.reqdWorkGroupSize[2] != 0)
    {
        bufSize = snprintf(buf, 100, "        .reqd_work_group_size %u, %u, %u\n",
               kernel.reqdWorkGroupSize[0], kernel.reqdWorkGroupSize[1],
               kernel.reqdWorkGroupSize[2]);
        output.write(buf, bufSize);
    }
    
    // work group size hint
    if (kernel.workGroupSizeHint[0] != 0 || kernel.workGroupSizeHint[1] != 0 ||
        kernel.workGroupSizeHint[2] != 0)
    {
        bufSize = snprintf(buf, 100, "        .work_group_size_hint %u, %u, %u\n",
               kernel.workGroupSizeHint[0], kernel.workGroupSizeHint[1],
               kernel.workGroupSizeHint[2]);
        output.write(buf, bufSize);
    }
    if (!kernel.vecTypeHint.empty())
    {
        output.write("        .vectypehint ", 21);
        output.write(kernel.vecTypeHint.c_str(), kernel.vecTypeHint.size());
        output.write("\n", 1);
    }
    if (!kernel.runtimeHandle.empty())
    {
        output.write("        .runtime_handle ", 24);
        output.write(kernel.runtimeHandle.c_str(), kernel.runtimeHandle.size());
        output.write("\n", 1);
    }
    if (hasValue(kernel.kernargSegmentSize))
    {
        bufSize = snprintf(buf, 100, "        .md_kernarg_segment_size %" PRIu64 "\n",
                    kernel.kernargSegmentSize);
        output.write(buf, bufSize);
    }
    if (hasValue(kernel.kernargSegmentAlign))
    {
        bufSize = snprintf(buf, 100, "        .md_kernarg_segment_align %" PRIu64 "\n",
                    kernel.kernargSegmentAlign);
        output.write(buf, bufSize);
    }
    if (hasValue(kernel.groupSegmentFixedSize))
    {
        bufSize = snprintf(buf, 100, "        .md_group_segment_fixed_size %" PRIu64 "\n",
                    kernel.groupSegmentFixedSize);
        output.write(buf, bufSize);
    }
    if (hasValue(kernel.privateSegmentFixedSize))
    {
        bufSize = snprintf(buf, 100, "        .md_private_segment_fixed_size %" PRIu64 "\n",
                    kernel.privateSegmentFixedSize);
        output.write(buf, bufSize);
    }
    if (hasValue(kernel.wavefrontSize))
    {
        bufSize = snprintf(buf, 100, "        .md_wavefront_size %u\n",
                    kernel.wavefrontSize);
        output.write(buf, bufSize);
    }
    // SGPRs and VGPRs
    if (hasValue(kernel.sgprsNum))
    {
        bufSize = snprintf(buf, 100, "        .md_sgprsnum %u\n", kernel.sgprsNum);
        output.write(buf, bufSize);
    }
    if (hasValue(kernel.vgprsNum))
    {
        bufSize = snprintf(buf, 100, "        .md_vgprsnum %u\n", kernel.vgprsNum);
        output.write(buf, bufSize);
    }
    // spilled SGPRs and VGPRs
    if (hasValue(kernel.spilledSgprs))
    {
        bufSize = snprintf(buf, 100, "        .spilledsgprs %u\n",
                           kernel.spilledSgprs);
        output.write(buf, bufSize);
    }
    if (hasValue(kernel.spilledVgprs))
    {
        bufSize = snprintf(buf, 100, "        .spilledvgprs %u\n",
                           kernel.spilledVgprs);
        output.write(buf, bufSize);
    }
    if (hasValue(kernel.maxFlatWorkGroupSize))
    {
        bufSize = snprintf(buf, 100, "        .max_flat_work_group_size %" PRIu64 "\n",
                    kernel.maxFlatWorkGroupSize);
        output.write(buf, bufSize);
    }
    // fixed work group size
    if (kernel.fixedWorkGroupSize[0] != 0 || kernel.fixedWorkGroupSize[1] != 0 ||
        kernel.fixedWorkGroupSize[2] != 0)
    {
        bufSize = snprintf(buf, 100, "        .fixed_work_group_size %u, %u, %u\n",
               kernel.fixedWorkGroupSize[0], kernel.fixedWorkGroupSize[1],
               kernel.fixedWorkGroupSize[2]);
        output.write(buf, bufSize);
    }
    
    // dump kernel arguments
    for (const ROCmKernelArgInfo& argInfo: kernel.argInfos)
    {
        output.write("        .arg ", 13);
        output.write(argInfo.name.c_str(), argInfo.name.size());
        output.write(", ", 2);
        output.write("\"", 1);
        std::string typeName = escapeStringCStyle(argInfo.typeName);
        output.write(typeName.c_str(), typeName.size());
        output.write("\", ", 3);
        size_t bufSize = 0;
        char buf[100];
        bufSize = snprintf(buf, 100, "%" PRIu64 ", %" PRIu64,
                           argInfo.size, argInfo.align);
        output.write(buf, bufSize);
        
        if (argInfo.valueKind > ROCmValueKind::MAX_VALUE)
            throw DisasmException("Unknown argument value kind");
        if (argInfo.valueType > ROCmValueType::MAX_VALUE)
            throw DisasmException("Unknown argument value type");
        
        bufSize = snprintf(buf, 100, ", %s, %s", 
                    disasmROCmValueKindNames[cxuint(argInfo.valueKind)],
                    disasmROCmValueTypeNames[cxuint(argInfo.valueType)]);
        output.write(buf, bufSize);
        
        if (argInfo.valueKind == ROCmValueKind::DYN_SHARED_PTR)
        {
            bufSize = snprintf(buf, 100, ", %" PRIu64, argInfo.pointeeAlign);
            output.write(buf, bufSize);
        }
        
        if (argInfo.valueKind == ROCmValueKind::DYN_SHARED_PTR ||
            argInfo.valueKind == ROCmValueKind::GLOBAL_BUFFER)
        {
            if (argInfo.addressSpace > ROCmAddressSpace::MAX_VALUE)
                throw DisasmException("Unknown address space");
            buf[0] = ','; buf[1] = ' ';
            const char* name = disasmROCmAddressSpaces[cxuint(argInfo.addressSpace)];
            bufSize = strlen(name) + 2;
            ::memcpy(buf+2, name, bufSize-2);
            output.write(buf, bufSize);
        }
        
        if (argInfo.valueKind == ROCmValueKind::IMAGE ||
            argInfo.valueKind == ROCmValueKind::PIPE)
        {
            if (argInfo.accessQual > ROCmAccessQual::MAX_VALUE)
                throw DisasmException("Unknown access qualifier");
            buf[0] = ','; buf[1] = ' ';
            const char* name = disasmROCmAccessQuals[cxuint(argInfo.accessQual)];
            bufSize = strlen(name) + 2;
            ::memcpy(buf+2, name, bufSize-2);
            output.write(buf, bufSize);
        }
        if (argInfo.valueKind == ROCmValueKind::GLOBAL_BUFFER ||
            argInfo.valueKind == ROCmValueKind::IMAGE ||
            argInfo.valueKind == ROCmValueKind::PIPE)
        {
            if (argInfo.actualAccessQual > ROCmAccessQual::MAX_VALUE)
                throw DisasmException("Unknown actual access qualifier");
            buf[0] = ','; buf[1] = ' ';
            const char* name = disasmROCmAccessQuals[cxuint(argInfo.actualAccessQual)];
            bufSize = strlen(name) + 2;
            ::memcpy(buf+2, name, bufSize-2);
            output.write(buf, bufSize);
        }
        
        if (argInfo.isConst)
            output.write(" const", 6);
        if (argInfo.isRestrict)
            output.write(" restrict", 9);
        if (argInfo.isVolatile)
            output.write(" volatile", 9);
        if (argInfo.isPipe)
            output.write(" pipe", 5);
        
        output.write("\n", 1);
    }
}

void CLRX::disassembleROCm(std::ostream& output, const ROCmDisasmInput* rocmInput,
           ISADisassembler* isaDisassembler, Flags flags)
{
    const bool doMetadata = ((flags & (DISASM_METADATA|DISASM_CONFIG)) != 0);
    const bool doDumpConfig = ((flags & DISASM_CONFIG) != 0);
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(rocmInput->deviceType);
    const cxuint maxSgprsNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    
    ROCmMetadata metadataInfo;
    bool haveMetadataInfo = false;
    if (doDumpConfig && rocmInput->metadata!=nullptr)
    {
        metadataInfo.parse(rocmInput->metadataSize, rocmInput->metadata);
        haveMetadataInfo = true;
    }
    
    {
        // print AMD architecture version
        char buf[40];
        size_t size = snprintf(buf, 40, ".arch_minor %u\n", rocmInput->archMinor);
        output.write(buf, size);
        size = snprintf(buf, 40, ".arch_stepping %u\n", rocmInput->archStepping);
        output.write(buf, size);
    }
    
    if (rocmInput->eflags != 0)
    {
        // print eflags if not zero
        char buf[40];
        size_t size = snprintf(buf, 40, ".eflags %u\n", rocmInput->eflags);
        output.write(buf, size);
    }
    
    if (rocmInput->newBinFormat)
        output.write(".newbinfmt\n", 11);
    
    if (!rocmInput->target.empty())
    {
        output.write(".target \"", 9);
        const std::string escapedTarget = escapeStringCStyle(rocmInput->target);
        output.write(escapedTarget.c_str(), escapedTarget.size());
        output.write("\"\n", 2);
    }
    
    // print got symbols
    for (const auto& gotSymbol: rocmInput->gotSymbols)
    {
        char buf[24];
        if (gotSymbol.second != SIZE_MAX)
        {
            // print got symbol definition
            output.write(".global ", 8);
            output.write(gotSymbol.first.c_str(), gotSymbol.first.size());
            output.write("\n", 1);
            output.write(gotSymbol.first.c_str(), gotSymbol.first.size());
            output.write(" = .gdata + ", 12);
            size_t numSize = itocstrCStyle(gotSymbol.second, buf, 24);
            output.write(buf, numSize);
            output.write("\n", 1);
        }
        output.write(".gotsym ", 8);
        output.write(gotSymbol.first.c_str(), gotSymbol.first.size());
        output.write("\n", 1);
    }
    
    if (doDumpData && rocmInput->globalData != nullptr &&
        rocmInput->globalDataSize != 0)
    {
        output.write(".globaldata\n", 12);
        output.write(".gdata:\n", 8); /// symbol used by text relocations
        printDisasmData(rocmInput->globalDataSize, rocmInput->globalData, output);
    }
    
    if (doMetadata && !doDumpConfig &&
        rocmInput->metadataSize != 0 && rocmInput->metadata != nullptr)
    {
        output.write(".metadata\n", 10);
        printDisasmLongString(rocmInput->metadataSize, rocmInput->metadata, output);
    }
    
    Array<std::pair<CString, size_t> > sortedMdKernelIndices;
    
    if (doDumpConfig && haveMetadataInfo)
    {
        // main metadata info dump
        char buf[100];
        size_t bufSize;
        if (metadataInfo.version[0] != BINGEN_NOTSUPPLIED)
        {
            bufSize = snprintf(buf, 100, ".md_version %u, %u\n", metadataInfo.version[0],
                    metadataInfo.version[1]);
            output.write(buf, bufSize);
        }
        for (const ROCmPrintfInfo& printfInfo: metadataInfo.printfInfos)
        {
            bufSize = snprintf(buf, 100, ".printf %u", printfInfo.id);
            output.write(buf, bufSize);
            for (uint32_t argSize: printfInfo.argSizes)
            {
                bufSize = snprintf(buf, 100, ", %u", argSize);
                output.write(buf, bufSize);
            }
            output.write(", \"", 3);
            std::string format = escapeStringCStyle(printfInfo.format);
            output.write(format.c_str(), format.size());
            output.write("\"\n", 2);
        }
        // prepare order of rocm metadata kernels
        sortedMdKernelIndices.resize(metadataInfo.kernels.size());
        for (size_t i = 0; i < sortedMdKernelIndices.size(); i++)
            sortedMdKernelIndices[i] = std::make_pair(metadataInfo.kernels[i].name, i);
        mapSort(sortedMdKernelIndices.begin(), sortedMdKernelIndices.end());
    }
    
    // dump kernel config
    for (const ROCmDisasmRegionInput& rinput: rocmInput->regions)
        if (rinput.type != ROCmRegionType::DATA)
        {
            output.write(".kernel ", 8);
            output.write(rinput.regionName.c_str(), rinput.regionName.size());
            output.put('\n');
            if (rinput.type == ROCmRegionType::FKERNEL)
                output.write("    .fkernel\n", 13);
            if (doDumpConfig)
            {
                dumpKernelConfig(output, maxSgprsNum, arch,
                     *reinterpret_cast<const ROCmKernelConfig*>(
                             rocmInput->code + rinput.offset));
                
                if (!haveMetadataInfo)
                    continue; // no metatadata info
                auto it = binaryMapFind(sortedMdKernelIndices.begin(),
                        sortedMdKernelIndices.end(), rinput.regionName);
                if (it == sortedMdKernelIndices.end())
                    continue; // not found
                // dump kernel metadata config
                dumpKernelMetadataInfo(output, metadataInfo.kernels[it->second]);
            }
        }
    
    // disassembly code in HSA form
    if (rocmInput->code != nullptr && rocmInput->codeSize != 0)
        disassembleAMDHSACode(output, rocmInput->regions,
                        rocmInput->codeSize, rocmInput->code, isaDisassembler, flags);
}
