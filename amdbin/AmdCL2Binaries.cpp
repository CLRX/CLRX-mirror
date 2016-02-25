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
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>

using namespace CLRX;

/* class AmdCL2InnerGPUBinaryBase */

const AmdCL2GPUKernel& AmdCL2InnerGPUBinaryBase::getKernelDatas(const char* name) const
{
    KernelDataMap::const_iterator it = binaryMapFind(
            kernelDatasMap.begin(), kernelDatasMap.end(), name);
    if (it == kernelDatasMap.end())
        throw Exception("Can't find kernel name");
    return kernels[it->second];
}

AmdCL2GPUKernel& AmdCL2InnerGPUBinaryBase::getKernelDatas(const char* name)
{
    KernelDataMap::iterator it = binaryMapFind(kernelDatasMap.begin(),
                   kernelDatasMap.end(), name);
    if (it == kernelDatasMap.end())
        throw Exception("Can't find kernel name");
    return kernels[it->second];
}

/* AmdCL2OldInnerGPUBinary */

AmdCL2OldInnerGPUBinary::AmdCL2OldInnerGPUBinary(AmdCL2MainGPUBinary* mainBinary,
            size_t binaryCodeSize, cxbyte* binaryCode, Flags _creationFlags)
        : creationFlags(_creationFlags), binarySize(binaryCodeSize), binary(binaryCode)
{
    if ((creationFlags & (AMDBIN_CREATE_KERNELDATAS|AMDBIN_CREATE_KERNELSTUBS)) == 0)
        return; // nothing to initialize
    uint16_t textIndex = SHN_UNDEF;
    try
    { textIndex = mainBinary->getSectionIndex(".text"); }
    catch(const Exception& ex)
    { }
        
    std::vector<size_t> choosenSyms;
    const size_t symbolsNum = mainBinary->getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = mainBinary->getSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len < 30 || (::strncmp(symName, "__ISA_&__OpenCL_", 16) != 0 &&
            ::strcmp(symName+len-14, "_kernel_binary") != 0)) // not binary, skip
            continue;
        choosenSyms.push_back(i);
    }
    
    if (hasKernelDatas())
        kernels.reset(new AmdCL2GPUKernel[choosenSyms.size()]);
    if (hasKernelStubs())
        kernelStubs.reset(new AmdCL2GPUKernelStub[choosenSyms.size()]);
    if (hasKernelDatasMap())
        kernelDatasMap.resize(choosenSyms.size());
    
    size_t ki = 0;
    for (size_t index: choosenSyms)
    {
        const Elf64_Sym& sym = mainBinary->getSymbol(index);
        const char* symName = mainBinary->getSymbolName(index);
        const size_t binOffset = ULEV(sym.st_value);
        const size_t binSize = ULEV(sym.st_size);
        if (textIndex != ULEV(sym.st_shndx))
            throw Exception("Kernel symbol outside text section");
        if (binOffset >= binaryCodeSize)
            throw Exception("Kernel binary code offset out of range");
        if (usumGt(binOffset, binSize, binaryCodeSize))
            throw Exception("Kernel binary code offset and size out of range");
        if (binSize < 256+192)
            throw Exception("Kernel binary code size is too short");
        
        AmdCL2GPUKernelStub kernelStub;
        AmdCL2GPUKernel kernelData;
        kernelStub.data = binaryCode + binOffset;
        const size_t setupOffset = ULEV(*reinterpret_cast<uint32_t*>(kernelStub.data));
        if (setupOffset >= binSize)
            throw Exception("Kernel setup offset out of range");
        kernelStub.size = setupOffset;
        kernelData.setup = kernelStub.data + setupOffset;
        const size_t textOffset = ULEV(*reinterpret_cast<uint32_t*>(kernelData.setup));
        if (usumGe(textOffset, setupOffset, binSize))
            throw Exception("Kernel text offset out of range");
        kernelData.setupSize = textOffset;
        kernelData.code = kernelData.setup + textOffset;
        kernelData.codeSize = binSize - (kernelData.code - kernelStub.data);
        if (hasKernelDatasMap())
        {   // kernel data map
            const size_t len = ::strlen(symName);
            kernelDatasMap[ki] = std::make_pair(CString(symName+16, symName+len-14), ki);
        }
        if (hasKernelStubs())
            kernelStubs[ki] = kernelStub;
        // put to kernels table
        if (hasKernelDatas())
            kernels[ki] = kernelData;
        ki++;
    }
    if (hasKernelDatasMap())
        mapSort(kernelDatasMap.begin(), kernelDatasMap.end());
}

const AmdCL2GPUKernelStub& AmdCL2OldInnerGPUBinary::getKernelStub(const char* name) const
{
    KernelDataMap::const_iterator it = binaryMapFind(
            kernelDatasMap.begin(), kernelDatasMap.end(), name);
    if (it == kernelDatasMap.end())
        throw Exception("Can't find kernel name");
    return kernelStubs[it->second];
}

AmdCL2GPUKernelStub& AmdCL2OldInnerGPUBinary::getKernelStub(const char* name)
{
    KernelDataMap::const_iterator it = binaryMapFind(
            kernelDatasMap.begin(), kernelDatasMap.end(), name);
    if (it == kernelDatasMap.end())
        throw Exception("Can't find kernel name");
    return kernelStubs[it->second];
}

/* AmdCL2InnerGPUBinary */

AmdCL2InnerGPUBinary::AmdCL2InnerGPUBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags): ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{
    if ((creationFlags & (AMDBIN_CREATE_KERNELDATAS|AMDBIN_CREATE_KERNELSTUBS)) != 0)
    {
        std::vector<size_t> choosenSyms;
        const size_t symbolsNum = getSymbolsNum();
        for (size_t i = 0; i < symbolsNum; i++)
        {
            const char* symName = getSymbolName(i);
            const size_t len = ::strlen(symName);
            if (len < 17 || (::strncmp(symName, "&__OpenCL_", 10) != 0 ||
                ::strcmp(symName+len-7, "_kernel") != 0)) // not binary, skip
                continue;
            choosenSyms.push_back(i);
        }
        
        size_t ki = 0;
        if (hasKernelDatas())
            kernels.reset(new AmdCL2GPUKernel[choosenSyms.size()]);
        if (hasKernelDatasMap())
            kernelDatasMap.resize(choosenSyms.size());
        
        for (size_t index: choosenSyms)
        {
            const Elf64_Sym& sym = getSymbol(index);
            const Elf64_Shdr& dataShdr = getSectionHeader(ULEV(sym.st_shndx));
            if (ULEV(sym.st_shndx) >= getSectionHeadersNum())
                throw Exception("Kernel section index out of range");
            const char* symName = getSymbolName(index);
            const size_t binOffset = ULEV(sym.st_value);
            const size_t binSize = ULEV(sym.st_size);
            if (binOffset >= ULEV(dataShdr.sh_size))
                throw Exception("Kernel binary code offset out of range");
            if (usumGt(binOffset, binSize, ULEV(dataShdr.sh_size)))
                throw Exception("Kernel binary code offset and size out of range");
            if (binSize < 192)
                throw Exception("Kernel binary code size is too short");
            
            if (hasKernelDatas())
            {
                AmdCL2GPUKernel kernelData;
                kernelData.setup = binaryCode + ULEV(dataShdr.sh_offset) +
                                ULEV(dataShdr.sh_offset) + binOffset;
                const size_t textOffset = ULEV(*reinterpret_cast<uint32_t*>(
                                    kernelData.setup));
                if (textOffset >= binSize)
                    throw Exception("Kernel text offset out of range");
                kernelData.setupSize = textOffset;
                kernelData.code = kernelData.setup + textOffset;
                kernelData.codeSize = binSize-textOffset;
                kernels[ki] = kernelData;
            }
            if (hasKernelDatasMap())
            {   // kernel data map
                const size_t len = ::strlen(symName);
                kernelDatasMap[ki] = std::make_pair(
                                CString(symName+10, symName+len-7), ki);
            }
            ki++;
        }
        if (hasKernelDatasMap())
            mapSort(kernelDatasMap.begin(), kernelDatasMap.end());
    }
    
    uint16_t gdataSecIndex = SHN_UNDEF;
    try
    { gdataSecIndex = getSectionIndex(".hsadata_readonly_agent"); }
    catch(const Exception& ex)
    { }
    if (gdataSecIndex != SHN_UNDEF)
    {
        const Elf64_Shdr& gdataShdr = getSectionHeader(gdataSecIndex);
        globalDataSize = ULEV(gdataShdr.sh_size);
        globalData = binaryCode + ULEV(gdataShdr.sh_offset);
    }
}

/* AmdCL2MainGPUBinary */

//static void getCL2KernelInfo()

AmdCL2MainGPUBinary::AmdCL2MainGPUBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags) : AmdMainBinaryBase(AmdMainType::GPU_CL2_BINARY),
            ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{
    std::vector<size_t> choosenMetadataSyms;
    std::vector<size_t> choosenBinSyms;
    
    const size_t symbolsNum = getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = getSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len >= 35 && (::strncmp(symName, "__OpenCL_&__OpenCL_", 19) == 0 &&
                ::strcmp(symName+len-16, "_kernel_metadata") == 0)) // not binary, skip
            choosenMetadataSyms.push_back(i);
        if (len >= 30 && (::strncmp(symName, "__ISA_&__OpenCL_", 16) == 0 &&
                ::strcmp(symName+len-14, "_kernel_binary") == 0)) // not binary, skip
            choosenBinSyms.push_back(i);
    }
    
    bool newInnerBinary;
}
