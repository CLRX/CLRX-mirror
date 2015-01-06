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
#include <cstdlib>
#include <elf.h>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <string>
#include <vector>
#include <CLRX/Utilities.h>
#include <CLRX/MemAccess.h>
#include <CLRX/GalliumBinaries.h>

using namespace CLRX;

/* GalliumKernel class */

GalliumKernel::GalliumKernel(): sectionId(0), offset(0), argsNum(0), argInfos(nullptr)
{ }

GalliumKernel::GalliumKernel(const GalliumKernel& cp)
{
    kernelName = cp.kernelName;
    sectionId = cp.sectionId;
    offset = cp.offset;
    argsNum = cp.argsNum;
    argInfos = new GalliumArg[argsNum];
    std::copy(cp.argInfos, cp.argInfos + argsNum, argInfos);
}

GalliumKernel::GalliumKernel(GalliumKernel&& cp) noexcept
{
    kernelName = std::move(cp.kernelName);
    argsNum = cp.argsNum;
    argInfos = cp.argInfos;
    offset = cp.offset;
    argsNum = cp.argsNum;
    cp.argsNum = 0;
    cp.argInfos = nullptr; // reset pointer
}

GalliumKernel::~GalliumKernel()
{
    delete[] argInfos;
}

GalliumKernel& GalliumKernel::operator=(const GalliumKernel& cp)
{
    delete[] argInfos;
    argInfos = nullptr;
    kernelName = cp.kernelName;
    sectionId = cp.sectionId;
    offset = cp.offset;
    argsNum = cp.argsNum;
    argInfos = new GalliumArg[argsNum];
    std::copy(cp.argInfos, cp.argInfos + argsNum, argInfos);
    return *this;
}

GalliumKernel& GalliumKernel::operator=(GalliumKernel&& cp) noexcept
{
    delete[] argInfos;
    argInfos = nullptr;
    kernelName = std::move(cp.kernelName);
    sectionId = cp.sectionId;
    offset = cp.offset;
    argsNum = cp.argsNum;
    argInfos = cp.argInfos;
    cp.argsNum = 0;
    cp.argInfos = nullptr; // reset pointer
    return *this;
}

void GalliumKernel::allocateArgs(cxuint argsNum)
{
    delete[] argInfos;
    argInfos = nullptr;
    this->argsNum = argsNum;
    argInfos = new GalliumArg[argsNum];
}

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
    try
    { textIndex = getSectionIndex(".text"); }
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
        throw Exception("Can't find Gallium32 ProgInfoEntry");
    return it->second;
}

const GalliumProgInfoEntry* GalliumElfBinary::getProgramInfo(uint32_t index) const
{
    return progInfoEntries + index*3;
}

GalliumProgInfoEntry* GalliumElfBinary::getProgramInfo(uint32_t index)
{
    return progInfoEntries + index*3;
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
    
    const uint32_t* data32 = reinterpret_cast<const uint32_t*>(binaryCode);
    const uint32_t kernelsNum = ULEV(*data32);
    kernels.resize(kernelsNum);
    const cxbyte* data = binaryCode + 4;
    // parse kernels symbol info and their arguments
    for (uint32_t i = 0; i < kernelsNum; i++)
    {
        GalliumKernel& kernel = kernels[i];
        const cxuint symNameLen = ULEV(*reinterpret_cast<const uint32_t*>(data));
        kernel.kernelName.assign((const char*)data + 4, symNameLen);
        data += symNameLen;
        data32 = reinterpret_cast<const uint32_t*>(data);
        kernel.sectionId = ULEV(data32[0]);
        kernel.offset = ULEV(data32[1]);
        const uint32_t argsNum = ULEV(data32[2]);
        kernel.allocateArgs(argsNum);
        data32 += 3;
        for (uint32_t j = 0; j < argsNum; j++)
        {
            GalliumArg& argInfo = kernel.argInfos[j];
            const cxuint type = ULEV(data32[0]);
            if (type > cxuint(GalliumArgType::MAX_VALUE))
                throw Exception("Wrong type of kernel argument");
            argInfo.type = GalliumArgType(type);
            argInfo.size = ULEV(data32[1]);
            argInfo.targetSize = ULEV(data32[2]);
            argInfo.targetAlign = ULEV(data32[3]);
            argInfo.signExtented = ULEV(data32[4])!=0;
            const cxuint semType = ULEV(data32[5]);
            if (type > cxuint(GalliumArgSemantic::MAX_VALUE))
                throw Exception("Wrong semantic of kernel argument");
            argInfo.semantic = GalliumArgSemantic(semType);
            data32 += 5;
        }
    }
    // parse sections and their content
}
