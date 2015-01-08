/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
#include <elf.h>
#include <climits>
#include <cstdint>
#include <string>
#include <utility>
#include <CLRX/Utilities.h>
#include <CLRX/MemAccess.h>
#include <CLRX/GalliumBinaries.h>

using namespace CLRX;

/* Gallium ELF binary */

GalliumElfBinary::GalliumElfBinary()
{ }

GalliumElfBinary::GalliumElfBinary(size_t binaryCodeSize, cxbyte* binaryCode,
               cxuint creationFlags) :
       ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{
    uint16_t amdGpuConfigIndex = SHN_UNDEF;
    try
    { amdGpuConfigIndex = getSectionIndex(".AMDGPU.config"); }
    catch(const Exception& ex)
    { }
    
    uint16_t textIndex = SHN_UNDEF;
    uint32_t textSize;
    try
    {
        textIndex = getSectionIndex(".text");
        textSize = ULEV(getSectionHeader(textIndex).sh_size);
    }
    catch(const Exception& ex)
    { }
    
    if (amdGpuConfigIndex == SHN_UNDEF || textIndex == SHN_UNDEF)
        return;
    // create amdGPU config systems
    const Elf32_Shdr& shdr = getSectionHeader(amdGpuConfigIndex);
    if ((ULEV(shdr.sh_size) % 24U) != 0)
        throw Exception("Wrong size of .AMDGPU.config section!");
    
    /* check symbols */
    const cxuint symbolsNum = getSymbolsNum();
    progInfosNum = 0;
    for (cxuint i = 0; i < symbolsNum; i++)
    {
        const Elf32_Sym& sym = getSymbol(i);
        const char* symName = getSymbolName(i);
        if (*symName != 0 && ::strcmp(symName, "EndOfTextLabel") != 0 &&
            ULEV(sym.st_shndx) == textIndex)
        {
            if (hasProgInfoMap())
                progInfoEntryMap.insert(std::make_pair(symName, 3*progInfosNum));
            if (ULEV(sym.st_value) >= textSize)
                throw Exception("kernel symbol offset out of range");
            progInfosNum++;
        }
    }
    if (progInfosNum*24U != ULEV(shdr.sh_size))
        throw Exception("Number of symbol kernels doesn't match progInfos number!");
    progInfoEntries = reinterpret_cast<GalliumProgInfoEntry*>(binaryCode +
                ULEV(shdr.sh_offset));
}

size_t GalliumElfBinary::getProgramInfoEntriesNum(uint32_t index) const
{ return 3; }

uint32_t GalliumElfBinary::getProgramInfoEntryIndex(const char* name) const
{
    ProgInfoEntryIndexMap::const_iterator it = progInfoEntryMap.find(name);
    if (it == progInfoEntryMap.end())
        throw Exception("Can't find GalliumElf ProgInfoEntry");
    return it->second;
}

const GalliumProgInfoEntry* GalliumElfBinary::getProgramInfo(uint32_t index) const
{
    return progInfoEntries + index*3U;
}

GalliumProgInfoEntry* GalliumElfBinary::getProgramInfo(uint32_t index)
{
    return progInfoEntries + index*3U;
}

/* main GalliumBinary */

GalliumBinary::GalliumBinary()
{ }

GalliumBinary::GalliumBinary(size_t binaryCodeSize, cxbyte* binaryCode,
                 cxuint creationFlags) : binaryCodeSize(0), binaryCode(nullptr),
         creationFlags(0)
{
    this->creationFlags = creationFlags;
    this->binaryCode = binaryCode;
    this->binaryCodeSize = binaryCodeSize;
    
    if (binaryCodeSize < 4)
        throw Exception("GalliumBinary is too small!!!");
    uint32_t* data32 = reinterpret_cast<uint32_t*>(binaryCode);
    const uint32_t kernelsNum = ULEV(*data32);
    if (binaryCodeSize < uint64_t(kernelsNum)*16U)
        throw Exception("Kernels number is too big!");
    kernels.resize(kernelsNum);
    cxbyte* data = binaryCode + 4;
    // parse kernels symbol info and their arguments
    for (cxuint i = 0; i < kernelsNum; i++)
    {
        GalliumKernel& kernel = kernels[i];
        if (usumGt(uint32_t(data-binaryCode), 4U, binaryCodeSize))
            throw Exception("GalliumBinary is too small!!!");
        
        const cxuint symNameLen = ULEV(*reinterpret_cast<const uint32_t*>(data));
        data+=4;
        if (usumGt(uint32_t(data-binaryCode), symNameLen, binaryCodeSize))
            throw Exception("Kernel name length is too long!");
        
        kernel.kernelName.assign((const char*)data, symNameLen);
        if (hasKernelMap())
            kernelIndexMap.insert(std::make_pair(kernel.kernelName, i));
        
        data += symNameLen;
        if (usumGt(uint32_t(data-binaryCode), 12U, binaryCodeSize))
            throw Exception("GalliumBinary is too small!!!");
        
        data32 = reinterpret_cast<uint32_t*>(data);
        kernel.sectionId = ULEV(data32[0]);
        kernel.offset = ULEV(data32[1]);
        const uint32_t argsNum = ULEV(data32[2]);
        data32 += 3;
        data = reinterpret_cast<cxbyte*>(data32);
        
        if (UINT32_MAX/24U < argsNum)
            throw Exception("Number of arguments number is too high!");
        if (usumGt(uint32_t(data-binaryCode), 24U*argsNum, binaryCodeSize))
            throw Exception("GalliumBinary is too small!!!");
        
        kernel.argInfos.resize(argsNum);
        for (uint32_t j = 0; j < argsNum; j++)
        {
            GalliumArgInfo& argInfo = kernel.argInfos[j];
            const cxuint type = ULEV(data32[0]);
            if (type > cxuint(GalliumArgType::MAX_VALUE))
                throw Exception("Wrong type of kernel argument");
            argInfo.type = GalliumArgType(type);
            argInfo.size = ULEV(data32[1]);
            argInfo.targetSize = ULEV(data32[2]);
            argInfo.targetAlign = ULEV(data32[3]);
            argInfo.signExtended = ULEV(data32[4])!=0;
            const cxuint semType = ULEV(data32[5]);
            if (type > cxuint(GalliumArgSemantic::MAX_VALUE))
                throw Exception("Wrong semantic of kernel argument");
            argInfo.semantic = GalliumArgSemantic(semType);
            data32 += 6;
        }
        data = reinterpret_cast<cxbyte*>(data32);
    }
    
    if (usumGt(uint32_t(data-binaryCode), 4U, binaryCodeSize))
        throw Exception("GalliumBinary is too small!!!");
    
    const uint32_t sectionsNum = ULEV(data32[0]);
    if (binaryCodeSize-(data-binaryCode) < uint64_t(sectionsNum)*20U)
        throw Exception("Sections number is too big!");
    sections.resize(sectionsNum);
    // parse sections and their content
    data32++;
    data += 4;
    
    uint32_t elfSectionId;
    
    for (GalliumSection& section: sections)
    {
        if (usumGt(uint32_t(data-binaryCode), 20U, binaryCodeSize))
            throw Exception("GalliumBinary is too small!!!");
        
        section.sectionId = ULEV(data32[0]);
        const uint32_t secType = ULEV(data32[1]);
        if (secType > cxuint(GalliumSectionType::MAX_VALUE))
            throw Exception("Wrong type of section");
        section.type = GalliumSectionType(secType);
        section.size = ULEV(data32[2]);
        const uint32_t sizeOfData = ULEV(data32[3]);
        const uint32_t sizeFromHeader = ULEV(data32[4]); // from LLVM binary
        if (section.size != sizeOfData-4 || section.size != sizeFromHeader)
            throw Exception("Section size fields doesn't match to itself!");
        
        data = reinterpret_cast<cxbyte*>(data32+5);
        if (usumGt(uint32_t(data-binaryCode), section.size, binaryCodeSize))
            throw Exception("Section size is too big!!!");
        
        section.offset = data-binaryCode;
        
        if (!elfBinary && section.type == GalliumSectionType::TEXT)
        {
            elfSectionId = section.sectionId;
            elfBinary = GalliumElfBinary(section.size, data,
                             creationFlags>>GALLIUM_INNER_SHIFT);
        }
        data += section.size;
        data32 = reinterpret_cast<uint32_t*>(data);
    }
    
    if (!elfBinary)
        throw Exception("Gallium Elf binary not found!");
    for (const GalliumKernel& kernel: kernels)
        if (kernel.sectionId != elfSectionId)
            throw Exception("Kernel not in text section!");
    // verify kernel offsets
    cxuint symIndex = 0;
    const cxuint symsNum = elfBinary.getSymbolsNum();
    uint16_t textIndex = elfBinary.getSectionIndex(".text");
    for (const GalliumKernel& kernel: kernels)
    {
        for (; symIndex < symsNum; symIndex++)
        {
            const Elf32_Sym& sym = elfBinary.getSymbol(symIndex);
            const char* symName = elfBinary.getSymbolName(symIndex);
            if (*symName != 0 && ::strcmp(symName, "EndOfTextLabel") != 0 &&
                ULEV(sym.st_shndx) == textIndex)
            {
                if (kernel.kernelName != symName)
                    throw Exception("Kernel symbols out of order!");
                if (ULEV(sym.st_value) != kernel.offset)
                    throw Exception("Kernel symbol value and Kernel offset doesn't match");
                break;
            }
        }
        if (symIndex >= symsNum)
            throw Exception("Number of kernels in ElfBinary and MainBinary doesn't match");
        symIndex++;
    }
}

uint32_t GalliumBinary::getKernelIndex(const char* name) const
{
    KernelIndexMap::const_iterator it = kernelIndexMap.find(name);
    if (it == kernelIndexMap.end())
        throw Exception("Can't find Gallium Kernel Index");
    return it->second;
}
