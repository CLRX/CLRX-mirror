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
#include <algorithm>
#include <utility>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/ROCmBinaries.h>

using namespace CLRX;

/* TODO: add support for various kernel code offset (now only 256 is supported) */

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
    {   // count regions number
        const Elf64_Sym& sym = getSymbol(i);
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        if (sym.st_shndx==textIndex &&
            (symType==STT_GNU_IFUNC || (bind==STB_GLOBAL && symType==STT_OBJECT)))
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
            continue;   // if not in '.text' section
        const size_t value = ULEV(sym.st_value);
        if (value < codeOffset)
            throw Exception("Region offset is too small!");
        const size_t size = ULEV(sym.st_size);
        
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        if (symType==STT_GNU_IFUNC || (bind==STB_GLOBAL && symType==STT_OBJECT))
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
        
        const size_t regSize = end - symOffsets[i-1].first;
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

/*
 * ROCm Binary Generator
 */

ROCmBinGenerator::ROCmBinGenerator() : manageable(false), input(nullptr)
{ }

ROCmBinGenerator::ROCmBinGenerator(const ROCmInput* rocmInput)
        : manageable(false), input(rocmInput)
{ }

ROCmBinGenerator::ROCmBinGenerator(GPUDeviceType deviceType,
        uint32_t archMinor, uint32_t archStepping, size_t codeSize, const cxbyte* code,
        const std::vector<ROCmSymbolInput>& symbols)
{
    input = new ROCmInput{ deviceType, archMinor, archStepping, symbols, codeSize, code };
}

ROCmBinGenerator::ROCmBinGenerator(GPUDeviceType deviceType,
        uint32_t archMinor, uint32_t archStepping, size_t codeSize, const cxbyte* code,
        std::vector<ROCmSymbolInput>&& symbols)
{
    input = new ROCmInput{ deviceType, archMinor, archStepping, std::move(symbols),
                codeSize, code };
}

ROCmBinGenerator::~ROCmBinGenerator()
{
    if (manageable)
        delete input;
}

void ROCmBinGenerator::setInput(const ROCmInput* input)
{
    if (manageable)
        delete input;
    manageable = false;
    this->input = input;
}

void ROCmBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    ElfBinaryGen64 elfBinGen64;
    
    const char* comment = "CLRX ROCmBinGenerator " CLRX_VERSION;
    uint32_t commentSize = ::strlen(comment);
    if (input->comment!=nullptr)
    {   // if comment, store comment section
        comment = input->comment;
        commentSize = input->commentSize;
        if (commentSize==0)
            commentSize = ::strlen(comment);
    }
    
    elfBinGen64.addRegion(ElfRegion64::programHeaderTable());
    elfBinGen64.addRegion(ElfRegion64::dynsymSection());
    elfBinGen64.addRegion(ElfRegion64::dynstrSection());
    elfBinGen64.addRegion(ElfRegion64(input->codeSize, (const cxbyte*)input->code, 
              0x1000, ".text", SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 0, 0, 0, 0,
              false, 256));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 0x1000, ".dynamic",
              SHT_DYNAMIC, SHF_ALLOC|SHF_WRITE, 2, 0, 0, 0, false, 8));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 4, ".note",
              SHT_NOTE, 0));
    elfBinGen64.addRegion(ElfRegion64(commentSize, (const cxbyte*)comment, 1, ".comment",
              SHT_PROGBITS, SHF_MERGE|SHF_STRINGS));
    elfBinGen64.addRegion(ElfRegion64::symtabSection());
    elfBinGen64.addRegion(ElfRegion64::shstrtabSection());
    elfBinGen64.addRegion(ElfRegion64::strtabSection());
    elfBinGen64.addRegion(ElfRegion64::sectionHeaderTable());
    
    size_t binarySize;
    /****
     * prepare for write binary to output
     ****/
    std::unique_ptr<std::ostream> outStreamHolder;
    std::ostream* os = nullptr;
    if (aPtr != nullptr)
    {
        aPtr->resize(binarySize);
        outStreamHolder.reset(
                new ArrayOStream(binarySize, reinterpret_cast<char*>(aPtr->data())));
        os = outStreamHolder.get();
    }
    else if (vPtr != nullptr)
    {
        vPtr->resize(binarySize);
        outStreamHolder.reset(new VectorOStream(*vPtr));
        os = outStreamHolder.get();
    }
    else // from argument
        os = osPtr;
    
    const std::ios::iostate oldExceptions = os->exceptions();
    try
    {
    os->exceptions(std::ios::failbit | std::ios::badbit);
    /****
     * write binary to output
     ****/
    //elfBinGen64->generate(bos);
    //assert(bos.getWritten() == binarySize);
    }
    catch(...)
    {
        os->exceptions(oldExceptions);
        throw;
    }
    os->exceptions(oldExceptions);
}

void ROCmBinGenerator::generate(Array<cxbyte>& array) const
{
    generateInternal(nullptr, nullptr, &array);
}

void ROCmBinGenerator::generate(std::ostream& os) const
{
    generateInternal(&os, nullptr, nullptr);
}

void ROCmBinGenerator::generate(std::vector<char>& v) const
{
    generateInternal(nullptr, &v, nullptr);
}
