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
#include <string>
#include <ostream>
#include <memory>
#include <memory>
#include <vector>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/GPUId.h>
#include "DisasmInternals.h"

using namespace CLRX;

/* helpers for main Disassembler class */

struct CLRX_INTERNAL CL2GPUDeviceCodeEntry
{
    uint32_t elfFlags;
    GPUDeviceType deviceType;
};

static const CL2GPUDeviceCodeEntry cl2GpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 17, GPUDeviceType::FIJI },
    { 16, GPUDeviceType::CARRIZO },
    { 15, GPUDeviceType::DUMMY }
};

static const CL2GPUDeviceCodeEntry cl2_16_3GpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 16, GPUDeviceType::FIJI },
    { 15, GPUDeviceType::CARRIZO },
    { 13, GPUDeviceType::GOOSE },
    { 12, GPUDeviceType::HORSE },
    { 17, GPUDeviceType::STONEY }
};

static const CL2GPUDeviceCodeEntry cl2_15_7GpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 16, GPUDeviceType::FIJI },
    { 15, GPUDeviceType::CARRIZO }
};
/* driver version: 2036.3 */
static const CL2GPUDeviceCodeEntry cl2GPUPROGpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 14, GPUDeviceType::FIJI },
    { 13, GPUDeviceType::CARRIZO },
    { 17, GPUDeviceType::ELLESMERE },
    { 16, GPUDeviceType::BAFFIN },
    { 15, GPUDeviceType::STONEY }
};

AmdCL2DisasmInput* CLRX::getAmdCL2DisasmInputFromBinary(const AmdCL2MainGPUBinary& binary)
{
    std::unique_ptr<AmdCL2DisasmInput> input(new AmdCL2DisasmInput);
    const uint32_t elfFlags = ULEV(binary.getHeader().e_flags);
    // detect GPU device from elfMachine field from ELF header
    cxuint entriesNum = 0;
    const CL2GPUDeviceCodeEntry* gpuCodeTable = nullptr;
    input->driverVersion = binary.getDriverVersion();
    if (input->driverVersion < 191205)
    {
        gpuCodeTable = cl2_15_7GpuDeviceCodeTable;
        entriesNum = sizeof(cl2_15_7GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    else if (input->driverVersion < 200406)
    {
        gpuCodeTable = cl2GpuDeviceCodeTable;
        entriesNum = sizeof(cl2GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    else if (input->driverVersion < 203603)
    {
        gpuCodeTable = cl2_16_3GpuDeviceCodeTable;
        entriesNum = sizeof(cl2_16_3GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    else
    {
        gpuCodeTable = cl2GPUPROGpuDeviceCodeTable;
        entriesNum = sizeof(cl2GPUPROGpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    //const cxuint entriesNum = sizeof(cl2GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    cxuint index;
    for (index = 0; index < entriesNum; index++)
        if (gpuCodeTable[index].elfFlags == elfFlags)
            break;
    if (entriesNum == index)
        throw Exception("Can't determine GPU device type");
    
    input->deviceType = gpuCodeTable[index].deviceType;
    input->compileOptions = binary.getCompileOptions();
    input->aclVersionString = binary.getAclVersionString();
    bool isInnerNewBinary = binary.hasInnerBinary() &&
                binary.getDriverVersion()>=191205;
    
    input->samplerInitSize = 0;
    input->samplerInit = nullptr;
    input->globalDataSize = 0;
    input->globalData = nullptr;
    input->rwDataSize = 0;
    input->rwData = nullptr;
    input->bssAlignment = input->bssSize = 0;
    std::vector<std::pair<size_t, size_t> > sortedRelocs; // by offset
    const cxbyte* textPtr = nullptr;
    const size_t kernelInfosNum = binary.getKernelInfosNum();
    
    uint16_t gDataSectionIdx = SHN_UNDEF;
    uint16_t rwDataSectionIdx = SHN_UNDEF;
    uint16_t bssDataSectionIdx = SHN_UNDEF;
    
    if (isInnerNewBinary)
    {
        const AmdCL2InnerGPUBinary& innerBin = binary.getInnerBinary();
        input->globalDataSize = innerBin.getGlobalDataSize();
        input->globalData = innerBin.getGlobalData();
        input->rwDataSize = innerBin.getRwDataSize();
        input->rwData = innerBin.getRwData();
        input->samplerInitSize = innerBin.getSamplerInitSize();
        input->samplerInit = innerBin.getSamplerInit();
        input->bssAlignment = innerBin.getBssAlignment();
        input->bssSize = innerBin.getBssSize();
        // if no kernels and data
        if (kernelInfosNum==0)
            return input.release();
        
        size_t relaNum = innerBin.getTextRelaEntriesNum();
        for (size_t i = 0; i < relaNum; i++)
        {
            const Elf64_Rela& rel = innerBin.getTextRelaEntry(i);
            sortedRelocs.push_back(std::make_pair(size_t(ULEV(rel.r_offset)), i));
        }
        // sort map
        mapSort(sortedRelocs.begin(), sortedRelocs.end());
        // get code section pointer for determine relocation position kernel code
        textPtr = innerBin.getSectionContent(".hsatext");
        
        try
        { gDataSectionIdx = innerBin.getSectionIndex(".hsadata_readonly_agent"); }
        catch(const Exception& ex)
        { }
        try
        { rwDataSectionIdx = innerBin.getSectionIndex(".hsadata_global_agent"); }
        catch(const Exception& ex)
        { }
        try
        { bssDataSectionIdx = innerBin.getSectionIndex(".hsabss_global_agent"); }
        catch(const Exception& ex)
        { }
        // relocations for global data section (sampler symbols)
        relaNum = innerBin.getGlobalDataRelaEntriesNum();
        // section index for samplerinit (will be used for comparing sampler symbol section
        uint16_t samplerInitSecIndex = SHN_UNDEF;
        try
        { samplerInitSecIndex = innerBin.getSectionIndex(".hsaimage_samplerinit"); }
        catch(const Exception& ex)
        { }
        
        for (size_t i = 0; i < relaNum; i++)
        {
            const Elf64_Rela& rel = innerBin.getGlobalDataRelaEntry(i);
            size_t symIndex = ELF64_R_SYM(ULEV(rel.r_info));
            const Elf64_Sym& sym = innerBin.getSymbol(symIndex);
            // check symbol type, section and value
            if (ELF64_ST_TYPE(sym.st_info) != 12)
                throw Exception("Wrong sampler symbol");
            uint64_t value = ULEV(sym.st_value);
            if (ULEV(sym.st_shndx) != samplerInitSecIndex)
                throw Exception("Wrong section for sampler symbol");
            if ((value&7) != 0)
                throw Exception("Wrong value of sampler symbol");
            input->samplerRelocs.push_back({ size_t(ULEV(rel.r_offset)),
                size_t(value>>3) });
        }
    }
    else if (kernelInfosNum==0)
        return input.release();
    
    input->kernels.resize(kernelInfosNum);
    auto sortedRelocIter = sortedRelocs.begin();
    
    for (cxuint i = 0; i < kernelInfosNum; i++)
    {
        const KernelInfo& kernelInfo = binary.getKernelInfo(i);
        AmdCL2DisasmKernelInput& kinput = input->kernels[i];
        kinput.kernelName = kernelInfo.kernelName;
        kinput.metadataSize = binary.getMetadataSize(i);
        kinput.metadata = binary.getMetadata(i);
        
        kinput.isaMetadataSize = 0;
        kinput.isaMetadata = nullptr;
        // setup isa metadata content
        const AmdCL2GPUKernelMetadata* isaMetadata = nullptr;
        if (i < binary.getISAMetadatasNum())
            isaMetadata = &binary.getISAMetadataEntry(i);
        if (isaMetadata == nullptr || isaMetadata->kernelName != kernelInfo.kernelName)
        {   // fallback if not in order
            try
            { isaMetadata = &binary.getISAMetadataEntry(
                            kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex) // failed
            { isaMetadata = nullptr; }
        }
        if (isaMetadata!=nullptr)
        {
            kinput.isaMetadataSize = isaMetadata->size;
            kinput.isaMetadata = isaMetadata->data;
        }
        
        kinput.code = nullptr;
        kinput.codeSize = 0;
        kinput.setup = nullptr;
        kinput.setupSize = 0;
        kinput.stub = nullptr;
        kinput.stubSize = 0;
        if (!binary.hasInnerBinary())
            continue; // nothing else to set
        
        // get kernel code, setup and stub content
        const AmdCL2InnerGPUBinaryBase& innerBin = binary.getInnerBinaryBase();
        const AmdCL2GPUKernel* kernelData = nullptr;
        if (i < innerBin.getKernelsNum())
            kernelData = &innerBin.getKernelData(i);
        if (kernelData==nullptr || kernelData->kernelName != kernelInfo.kernelName)
            kernelData = &innerBin.getKernelData(kernelInfo.kernelName.c_str());
        
        if (kernelData!=nullptr)
        {
            kinput.code = kernelData->code;
            kinput.codeSize = kernelData->codeSize;
            kinput.setup = kernelData->setup;
            kinput.setupSize = kernelData->setupSize;
        }
        if (!isInnerNewBinary)
        {   // old drivers
            const AmdCL2OldInnerGPUBinary& oldInnerBin = binary.getOldInnerBinary();
            const AmdCL2GPUKernelStub* kstub = nullptr;
            if (i < innerBin.getKernelsNum())
                kstub = &oldInnerBin.getKernelStub(i);
            if (kstub==nullptr || kernelData->kernelName != kernelInfo.kernelName)
                kstub = &oldInnerBin.getKernelStub(kernelInfo.kernelName.c_str());
            if (kstub!=nullptr)
            {
                kinput.stubSize = kstub->size;
                kinput.stub = kstub->data;
            }
        }
        else
        {   // relocations
            const AmdCL2InnerGPUBinary& innerBin = binary.getInnerBinary();
            
            if (sortedRelocIter != sortedRelocs.end() &&
                    sortedRelocIter->first < size_t(kinput.code-textPtr))
                throw Exception("Code relocation offset outside kernel code");
            
            if (sortedRelocIter != sortedRelocs.end())
            {
                size_t end = kinput.code+kinput.codeSize-textPtr;
                for (; sortedRelocIter != sortedRelocs.end() &&
                    sortedRelocIter->first<=end; ++sortedRelocIter)
                {   // add relocations
                    const Elf64_Rela& rela = innerBin.getTextRelaEntry(
                                sortedRelocIter->second);
                    uint32_t symIndex = ELF64_R_SYM(ULEV(rela.r_info));
                    int64_t addend = ULEV(rela.r_addend);
                    cxuint rsym = 0;
                    // check this symbol
                    const Elf64_Sym& sym = innerBin.getSymbol(symIndex);
                    uint16_t symShndx = ULEV(sym.st_shndx);
                    if (symShndx!=gDataSectionIdx && symShndx!=rwDataSectionIdx &&
                        symShndx!=bssDataSectionIdx)
                        throw Exception("Symbol is not placed in global or "
                                "rwdata data or bss is illegal");
                    addend += ULEV(sym.st_value);
                    rsym = (symShndx==rwDataSectionIdx) ? 1 : 
                        ((symShndx==bssDataSectionIdx) ? 2 : 0);
                    
                    RelocType relocType;
                    uint32_t rtype = ELF64_R_TYPE(ULEV(rela.r_info));
                    if (rtype==1)
                        relocType = RELTYPE_LOW_32BIT;
                    else if (rtype==2)
                        relocType = RELTYPE_HIGH_32BIT;
                    else
                        throw Exception("Unknown relocation type");
                    // put text relocs. compute offset by subtracting current code offset
                    kinput.textRelocs.push_back(AmdCL2RelaEntry{sortedRelocIter->first-
                        (kinput.code-textPtr), relocType, rsym, addend });
                }
            }
        }
    }
    if (sortedRelocIter != sortedRelocs.end())
        throw Exception("Code relocation offset outside kernel code");
    return input.release();
}

void CLRX::disassembleAmdCL2(std::ostream& output, const AmdCL2DisasmInput* amdCL2Input,
       ISADisassembler* isaDisassembler, size_t& sectionCount, Flags flags)
{
    const bool doMetadata = ((flags & DISASM_METADATA) != 0);
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    const bool doSetup = ((flags & DISASM_SETUP) != 0);
    
    {
        char buf[40];
        size_t size = snprintf(buf, 40, ".driver_version %u\n",
                   amdCL2Input->driverVersion);
        output.write(buf, size);
    }
    
    if (doMetadata)
    {
        output.write(".compile_options \"", 18);
        const std::string escapedCompileOptions = 
                escapeStringCStyle(amdCL2Input->compileOptions);
        output.write(escapedCompileOptions.c_str(), escapedCompileOptions.size());
        output.write("\"\n.acl_version \"", 16);
        const std::string escapedAclVersionString =
                escapeStringCStyle(amdCL2Input->aclVersionString);
        output.write(escapedAclVersionString.c_str(), escapedAclVersionString.size());
        output.write("\"\n", 2);
    }
    if (doSetup && amdCL2Input->samplerInit!=nullptr && amdCL2Input->samplerInitSize!=0)
    {   /// sampler init entries
        output.write(".samplerinit\n", 13);
        printDisasmData(amdCL2Input->samplerInitSize, amdCL2Input->samplerInit, output);
    }
    
    if (doDumpData && amdCL2Input->globalData != nullptr &&
        amdCL2Input->globalDataSize != 0)
    {
        output.write(".globaldata\n", 12);
        output.write(".gdata:\n", 8); /// symbol used by text relocations
        printDisasmData(amdCL2Input->globalDataSize, amdCL2Input->globalData, output);
        /// put sampler relocations at global data section
        for (auto v: amdCL2Input->samplerRelocs)
        {
            output.write("    .samplerreloc ", 18);
            char buf[64];
            size_t bufPos = itocstrCStyle<size_t>(v.first, buf, 22);
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += itocstrCStyle<size_t>(v.second, buf+bufPos, 22);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
        }
    }
    if (doDumpData && amdCL2Input->rwData != nullptr &&
        amdCL2Input->rwDataSize != 0)
    {
        output.write(".data\n", 6);
        output.write(".ddata:\n", 8); /// symbol used by text relocations
        printDisasmData(amdCL2Input->rwDataSize, amdCL2Input->rwData, output);
    }
    
    if (doDumpData && amdCL2Input->bssSize)
    {
        output.write(".section .bss align=", 20);
        char buf[64];
        size_t bufPos = itocstrCStyle<size_t>(amdCL2Input->bssAlignment, buf, 22);
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
        output.write(".bdata:\n", 8); /// symbol used by text relocations
        output.write("    .skip ", 10);
        bufPos = itocstrCStyle<size_t>(amdCL2Input->bssSize, buf, 22);
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
    }
    
    for (const AmdCL2DisasmKernelInput& kinput: amdCL2Input->kernels)
    {
        output.write(".kernel ", 8);
        output.write(kinput.kernelName.c_str(), kinput.kernelName.size());
        output.put('\n');
        if (doMetadata)
        {
            if (kinput.metadata != nullptr && kinput.metadataSize != 0)
            {   // if kernel metadata available
                output.write("    .metadata\n", 14);
                printDisasmData(kinput.metadataSize, kinput.metadata, output, true);
            }
            if (kinput.isaMetadata != nullptr && kinput.isaMetadataSize != 0)
            {   // if kernel isametadata available
                output.write("    .isametadata\n", 17);
                printDisasmData(kinput.isaMetadataSize, kinput.isaMetadata, output, true);
            }
        }
        if (doSetup)
        {
            if (kinput.stub != nullptr && kinput.stubSize != 0)
            {   // if kernel setup available
                output.write("    .stub\n", 10);
                printDisasmData(kinput.stubSize, kinput.stub, output, true);
            }
            if (kinput.setup != nullptr && kinput.setupSize != 0)
            {   // if kernel setup available
                output.write("    .setup\n", 11);
                printDisasmData(kinput.setupSize, kinput.setup, output, true);
            }
        }
        if (doDumpCode && kinput.code != nullptr && kinput.codeSize != 0)
        {   // input kernel code (main disassembly)
            isaDisassembler->clearRelocations();
            isaDisassembler->addRelSymbol(".gdata");
            isaDisassembler->addRelSymbol(".ddata"); // rw data
            isaDisassembler->addRelSymbol(".bdata"); // .bss data
            for (const AmdCL2RelaEntry& entry: kinput.textRelocs)
                isaDisassembler->addRelocation(entry.offset, entry.type, 
                               cxuint(entry.symbol), entry.addend);
    
            output.write("    .text\n", 10);
            isaDisassembler->setInput(kinput.codeSize, kinput.code);
            isaDisassembler->beforeDisassemble();
            isaDisassembler->disassemble();
            sectionCount++;
        }
    }
}
