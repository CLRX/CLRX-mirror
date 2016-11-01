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
#include <utility>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/ROCmBinaries.h>

using namespace CLRX;

ROCmBinary::ROCmBinary(size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags)
        : ElfBinary64(binaryCodeSize, binaryCode, creationFlags),
          regionsNum(0), codeSize(0), code(nullptr)
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
    
    regionsNum = 0;
    const size_t symbolsNum = getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const Elf64_Sym& sym = getSymbol(i);
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        if (sym.st_shndx==textIndex &&
            (symType==STT_GNU_IFUNC || symType==STT_OBJECT))
            regionsNum++;
    }
    if (code==nullptr && regionsNum!=0)
        throw Exception("No code if regions number is not zero");
    regions.reset(new ROCmRegion[regionsNum]);
    size_t j = 0;
    typedef std::pair<uint64_t, size_t> RegionOffsetEntry;
    std::unique_ptr<RegionOffsetEntry[]> symOffsets(new RegionOffsetEntry[regionsNum]);
    
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const Elf64_Sym& sym = getSymbol(i);
        if (sym.st_shndx!=textIndex)
            continue;
        const size_t value = ULEV(sym.st_value);
        if (value < codeOffset)
            throw Exception("Region offset is too small!");
        const size_t size = ULEV(sym.st_size);
        
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        if (symType==STT_GNU_IFUNC || symType==STT_OBJECT)
        {
            const bool isKernel = (symType==STT_GNU_IFUNC);
            symOffsets[j] = std::make_pair(value, j);
            if (isKernel && value+0x100 > codeOffset+codeSize)
                throw Exception("Kernel offset is too big!");
            regions[j++] = { getSymbolName(i), size, value, isKernel };
        }
    }
    std::sort(symOffsets.get(), symOffsets.get()+regionsNum,
            [](const RegionOffsetEntry& a, const RegionOffsetEntry& b)
            { return a.first < b.first; });
    // checking distance between regions
    for (size_t i = 1; i <= regionsNum; i++)
    {
        size_t end = (i<regionsNum) ? symOffsets[i].first : codeOffset+codeSize;
        ROCmRegion& region = regions[symOffsets[i-1].second];
        if (region.isKernel && symOffsets[i-1].first+0x100 > end)
            throw Exception("Kernel size is too small!");
        
        const uint64_t regSize = end - symOffsets[i-1].first;
        if (region.size==0)
            region.size = regSize;
        else
            region.size = std::min(regSize, region.size);
    }
    
    if (hasRegionMap())
    {   // create region map
        regionsMap.resize(regionsNum);
        for (size_t i = 0; i < regionsNum; i++)
            regionsMap[i] = std::make_pair(regions[i].regionName, i);
        mapSort(regionsMap.begin(), regionsMap.end());
    }
}

const ROCmRegion& ROCmBinary::getRegion(const char* name) const
{
    RegionMap::const_iterator it = binaryMapFind(regionsMap.begin(),
                             regionsMap.end(), name);
    if (it == regionsMap.end())
        throw Exception("Can't find region name");
    return regions[it->second];
}

bool CLRX::isROCmBinary(size_t binarySize, const cxbyte* binary)
{
    if (!isElfBinary(binarySize, binary))
        return false;
    if (binary[EI_CLASS] != ELFCLASS64)
        return false;
    const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binary);
    if (ULEV(ehdr->e_machine) != 0xe0 || ULEV(ehdr->e_flags)!=0)
        return false;
    return true;
}
