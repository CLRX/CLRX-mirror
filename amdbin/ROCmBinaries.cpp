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
#include <map>
#include <utility>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/ROCmBinaries.h>

using namespace CLRX;

ROCmBinary::ROCmBinary(size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags)
        : ElfBinary64(binaryCodeSize, binaryCode, creationFlags),
          kernelsNum(0), codeSize(0), code(nullptr)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    uint64_t codeOffset = 0;
    if (textIndex!=SHN_UNDEF)
    {
        code = getSectionContent(textIndex);
        const Elf64_Shdr& textShdr = getSectionHeader(textIndex);
        codeSize = ULEV(textShdr.sh_size);
        codeOffset = ULEV(textShdr.sh_offset);
    }
    
    kernelsNum = 0;
    const size_t symbolsNum = getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const Elf64_Sym& sym = getSymbol(i);
        if (sym.st_shndx==textIndex)
            kernelsNum++;
    }
    if (code==nullptr && kernelsNum!=0)
        throw Exception("No code if kernels number is not zero");
    kernels.reset(new ROCmKernel[kernelsNum]);
    size_t j = 0;
    
    typedef std::pair<uint64_t, size_t> KernelOffsetEntry;
    std::unique_ptr<KernelOffsetEntry[]> kernelOffsets(new KernelOffsetEntry[kernelsNum]);
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const Elf64_Sym& sym = getSymbol(i);
        if (sym.st_shndx!=textIndex)
            continue;
        const size_t value = ULEV(sym.st_value);
        if (value < codeOffset)
            throw Exception("Kernel offset is too small!");
        const size_t size = ULEV(sym.st_size);
        if (value+0x100 > codeOffset+codeSize)
            throw Exception("Kernel offset is too big!");
        kernelOffsets[j] = std::make_pair(value, j);
        kernels[j++] = { getSymbolName(i), value, size, value+0x100 };
    }
    std::sort(kernelOffsets.get(), kernelOffsets.get()+kernelsNum,
            [](const KernelOffsetEntry& a, const KernelOffsetEntry& b)
            { return a.first < b.first; });
    // checking distance between kernels
    for (size_t i = 1; i < kernelsNum; i++)
    {
        if (kernelOffsets[i-1].first+0x100 > kernelOffsets[i].first)
            throw Exception("Kernel size is too small!");
        ROCmKernel& kernel = kernels[kernelOffsets[i-1].second];
        uint64_t kcodeSize = kernelOffsets[i].first - (kernelOffsets[i-1].first+0x100);
        if (kernel.codeSize==0 && kcodeSize>0)
            kernel.codeSize = kcodeSize;
        if (kernel.codeSize > kcodeSize)
            throw Exception("Kernel code size out of range");
    }
    {   // last kernel in position
        if (kernelOffsets[kernelsNum-1].first+0x100 > codeOffset+codeSize)
            throw Exception("Kernel size is too small!");
        ROCmKernel& kernel = kernels[kernelOffsets[kernelsNum-1].second];
        uint64_t kcodeSize = codeSize -
                (kernelOffsets[kernelsNum-1].first+0x100-codeOffset);
        if (kernel.codeSize==0 && kcodeSize>0)
            kernel.codeSize = kcodeSize;
        if (kernel.codeSize > kcodeSize)
            throw Exception("Kernel code size out of range");
    }
    
    if (hasKernelMap())
    {   // create kernels map
        kernelsMap.resize(kernelsNum);
        for (size_t i = 0; i < kernelsNum; i++)
            kernelsMap[i] = std::make_pair(kernels[i].kernelName, i);
        mapSort(kernelsMap.begin(), kernelsMap.end());
    }
}

const ROCmKernel& ROCmBinary::getKernel(const char* name) const
{
    KernelMap::const_iterator it = binaryMapFind(kernelsMap.begin(),
                             kernelsMap.end(), name);
    if (it == kernelsMap.end())
        throw Exception("Can't find kernel name");
    return kernels[it->second];
}

bool CLRX::isROCmBinary(size_t binarySize, const cxbyte* binary)
{
    if (!isElfBinary(binarySize, binary))
        return false;
        return false;
    if (binary[EI_CLASS] != ELFCLASS64)
        return false;
    const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binary);
    if (ULEV(ehdr->e_machine) != 0xe0 || ULEV(ehdr->e_flags)!=0)
        return false;
    return true;
}
