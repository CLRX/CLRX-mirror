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
#include <cassert>
#include <climits>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <memory>
#include <mutex>
#include <fstream>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/GalliumBinaries.h>

using namespace CLRX;

/* Gallium ELF binary */

GalliumElfBinaryBase::GalliumElfBinaryBase() :
        progInfosNum(0), progInfoEntries(nullptr), disasmSize(0), disasmOffset(0),
        llvm390(false)
{ }

template<typename ElfBinary>
void GalliumElfBinaryBase::loadFromElf(ElfBinary& elfBinary, size_t kernelsNum)
{
    uint16_t amdGpuConfigIndex = SHN_UNDEF;
    try
    { amdGpuConfigIndex = elfBinary.getSectionIndex(".AMDGPU.config"); }
    catch(const Exception& ex)
    { }
    
    uint16_t amdGpuDisasmIndex = SHN_UNDEF;
    try
    { amdGpuDisasmIndex = elfBinary.getSectionIndex(".AMDGPU.disasm"); }
    catch(const Exception& ex)
    { }
    if (amdGpuDisasmIndex != SHN_UNDEF)
    {
        // set disassembler section
        const auto& shdr = elfBinary.getSectionHeader(amdGpuDisasmIndex);
        disasmOffset = ULEV(shdr.sh_offset);
        disasmSize = ULEV(shdr.sh_size);
    }
    
    uint16_t textIndex = SHN_UNDEF;
    size_t textSize = 0;
    try
    {
        textIndex = elfBinary.getSectionIndex(".text");
        textSize = ULEV(elfBinary.getSectionHeader(textIndex).sh_size);
    }
    catch(const Exception& ex)
    { }
    
    if (amdGpuConfigIndex == SHN_UNDEF || textIndex == SHN_UNDEF)
        return;
    // create amdGPU config systems
    const auto& shdr = elfBinary.getSectionHeader(amdGpuConfigIndex);
    size_t shdrSize = ULEV(shdr.sh_size);
    size_t amdGPUConfigSize = (shdrSize / kernelsNum);
    if (amdGPUConfigSize != 24 && amdGPUConfigSize != 40 &&
        shdrSize % amdGPUConfigSize != 0)
        throw BinException("Wrong size of .AMDGPU.config section!");
    // detect whether is binary generated for LLVM >= 3.9.0 by amdGPUConfig size
    llvm390 = amdGPUConfigSize==40;
    const cxuint progInfoEntriesNum = amdGPUConfigSize>>3;
    
    const bool hasProgInfoMap = (elfBinary.getCreationFlags() &
                        GALLIUM_ELF_CREATE_PROGINFOMAP) != 0;
    /* check symbols */
    const size_t symbolsNum = elfBinary.getSymbolsNum();
    progInfosNum = 0;
    if (hasProgInfoMap)
        progInfoEntryMap.resize(symbolsNum);
    // fill up program info entries
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const auto& sym = elfBinary.getSymbol(i);
        const char* symName = elfBinary.getSymbolName(i);
        if (ULEV(sym.st_shndx) == textIndex && ELF32_ST_BIND(sym.st_info) == STB_GLOBAL)
        {
            if (ULEV(sym.st_value) >= textSize)
                throw BinException("kernel symbol offset out of range");
            if (hasProgInfoMap)
                progInfoEntryMap[progInfosNum] = std::make_pair(symName,
                                progInfoEntriesNum*progInfosNum);
            progInfosNum++;
        }
    }
    if (progInfosNum*amdGPUConfigSize != ULEV(shdr.sh_size))
        throw BinException("Number of symbol kernels doesn't match progInfos number!");
    cxbyte* binaryCode = (cxbyte*)elfBinary.getBinaryCode();
    progInfoEntries = reinterpret_cast<GalliumProgInfoEntry*>(binaryCode +
                ULEV(shdr.sh_offset));
    
    if (hasProgInfoMap)
    {
        progInfoEntryMap.resize(progInfosNum);
        mapSort(progInfoEntryMap.begin(), progInfoEntryMap.end(), CStringLess());
    }
}

template<typename GalliumElfBinary>
static void innerBinaryGetScratchRelocs(const GalliumElfBinary& binary,
                    Array<GalliumScratchReloc>& scratchRelocs)
{
    if (binary.getTextRelEntriesNum() != 0)
    {
        size_t scratchRSRCDword0Sym = SIZE_MAX;
        size_t scratchRSRCDword1Sym = SIZE_MAX;
        // parse scratch buffer relocations
        scratchRelocs.resize(binary.getTextRelEntriesNum());
        size_t j = 0;
        for (size_t i = 0; i < binary.getTextRelEntriesNum(); i++)
        {
            const auto& rel = binary.getTextRelEntry(i);
            const size_t symIndex = GalliumElfBinary::getElfRelSym(ULEV(rel.r_info));
            const char* symName = binary.getSymbolName(symIndex);
            const auto& sym = binary.getSymbol(symIndex);
            // if scratch RSRC symbol is not defined (just set if found)
            if (scratchRSRCDword0Sym == SIZE_MAX && ULEV(sym.st_shndx == SHN_UNDEF) &&
                    ::strcmp(symName, "SCRATCH_RSRC_DWORD0")==0)
                scratchRSRCDword0Sym = symIndex;
            else if (scratchRSRCDword1Sym == SIZE_MAX && ULEV(sym.st_shndx == SHN_UNDEF) &&
                    ::strcmp(symName, "SCRATCH_RSRC_DWORD1")==0)
                scratchRSRCDword1Sym = symIndex;
            
            if (scratchRSRCDword0Sym == symIndex)
                scratchRelocs[j++] = { size_t(ULEV(rel.r_offset)), RELTYPE_LOW_32BIT };
            else if (scratchRSRCDword1Sym == symIndex)
                scratchRelocs[j++] = { size_t(ULEV(rel.r_offset)), RELTYPE_HIGH_32BIT };
        }
        // final resizing
        scratchRelocs.resize(j);
    }
}

GalliumElfBinaryBase::~GalliumElfBinaryBase()
{ }

GalliumElfBinary32::GalliumElfBinary32()
    : textRelsNum(0), textRelEntrySize(0), textRel(nullptr)
{ }

GalliumElfBinary32::~GalliumElfBinary32()
{ }

GalliumElfBinary32::GalliumElfBinary32(size_t binaryCodeSize, cxbyte* binaryCode,
           Flags creationFlags, size_t kernelsNum) :
           ElfBinary32(binaryCodeSize, binaryCode, creationFlags|ELF_CREATE_SYMBOLMAP),
           textRelsNum(0), textRelEntrySize(0), textRel(nullptr)
{
    loadFromElf(static_cast<const ElfBinary32&>(*this), kernelsNum);
    
    // get relocation section for text
    try
    {
        const Elf32_Shdr& relShdr = getSectionHeader(".rel.text");
        textRelEntrySize = ULEV(relShdr.sh_entsize);
        if (textRelEntrySize==0)
            textRelEntrySize = sizeof(Elf32_Rel);
        textRelsNum = ULEV(relShdr.sh_size)/textRelEntrySize;
        textRel = binaryCode + ULEV(relShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
    
    innerBinaryGetScratchRelocs(*this, scratchRelocs);
}

GalliumElfBinary64::GalliumElfBinary64()
    : textRelsNum(0), textRelEntrySize(0), textRel(nullptr)
{ }

GalliumElfBinary64::~GalliumElfBinary64()
{ }

GalliumElfBinary64::GalliumElfBinary64(size_t binaryCodeSize, cxbyte* binaryCode,
           Flags creationFlags, size_t kernelsNum) :
           ElfBinary64(binaryCodeSize, binaryCode, creationFlags|ELF_CREATE_SYMBOLMAP),
           textRelsNum(0), textRelEntrySize(0), textRel(nullptr)
{
    loadFromElf(static_cast<const ElfBinary64&>(*this), kernelsNum);
    // get relocation section for text
    try
    {
        const Elf64_Shdr& relShdr = getSectionHeader(".rel.text");
        textRelEntrySize = ULEV(relShdr.sh_entsize);
        if (textRelEntrySize==0)
            textRelEntrySize = sizeof(Elf64_Rel);
        textRelsNum = ULEV(relShdr.sh_size)/textRelEntrySize;
        textRel = binaryCode + ULEV(relShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
    
    innerBinaryGetScratchRelocs(*this, scratchRelocs);
}

uint32_t GalliumElfBinaryBase::getProgramInfoEntriesNum(uint32_t index) const
{ return 3; }

uint32_t GalliumElfBinaryBase::getProgramInfoEntryIndex(const char* name) const
{
    ProgInfoEntryIndexMap::const_iterator it = binaryMapFind(progInfoEntryMap.begin(),
                         progInfoEntryMap.end(), name, CStringLess());
    if (it == progInfoEntryMap.end())
        throw BinException("Can't find GalliumElf ProgInfoEntry");
    return it->second;
}

const GalliumProgInfoEntry* GalliumElfBinaryBase::getProgramInfo(uint32_t index) const
{
    return progInfoEntries + index*(llvm390 ? 5U : 3U);
}

GalliumProgInfoEntry* GalliumElfBinaryBase::getProgramInfo(uint32_t index)
{
    return progInfoEntries + index*(llvm390 ? 5U : 3U);
}

/* main GalliumBinary */

template<typename GalliumElfBinary>
static void verifyKernelSymbols(size_t kernelsNum, const GalliumKernel* kernels,
                const GalliumElfBinary& elfBinary)
{
    uint16_t textIndex = elfBinary.getSectionIndex(".text");
    for (uint32_t i = 0; i < kernelsNum; i++)
    {
        const GalliumKernel& kernel = kernels[i];
        size_t symIndex = 0;
        try 
        { symIndex = elfBinary.getSymbolIndex(kernel.kernelName.c_str()); }
        catch(const Exception& ex)
        { throw BinException("Kernel symbol not found"); }
        const auto& sym = elfBinary.getSymbol(symIndex);
        const char* symName = elfBinary.getSymbolName(symIndex);
        // kernel symol must be defined as global and must be bound to text section
        if (ULEV(sym.st_shndx) == textIndex &&
            ELF32_ST_BIND(sym.st_info) == STB_GLOBAL)
        {
            // names must be stored in order
            if (kernel.kernelName != symName)
                throw BinException("Kernel symbols out of order!");
            if (ULEV(sym.st_value) != kernel.offset)
                throw BinException("Kernel symbol value and Kernel "
                            "offset doesn't match");
        }
        else
            throw BinException("Wrong section or binding for kernel symbol");
    }
}

GalliumBinary::GalliumBinary(size_t _binaryCodeSize, cxbyte* _binaryCode,
                 Flags _creationFlags) : creationFlags(_creationFlags),
         binaryCodeSize(_binaryCodeSize), binaryCode(_binaryCode),
         kernelsNum(0), sectionsNum(0), kernels(nullptr), sections(nullptr),
         elf64BitBinary(false), mesa170(false)
{
    if (binaryCodeSize < 4)
        throw BinException("GalliumBinary is too small!!!");
    uint32_t* data32 = reinterpret_cast<uint32_t*>(binaryCode);
    kernelsNum = ULEV(*data32);
    if (binaryCodeSize < uint64_t(kernelsNum)*16U)
        throw BinException("Kernels number is too big!");
    kernels.reset(new GalliumKernel[kernelsNum]);
    cxbyte* data = binaryCode + 4;
    // parse kernels symbol info and their arguments
    for (cxuint i = 0; i < kernelsNum; i++)
    {
        GalliumKernel& kernel = kernels[i];
        if (usumGt(uint32_t(data-binaryCode), 4U, binaryCodeSize))
            throw BinException("GalliumBinary is too small!!!");
        
        const cxuint symNameLen = ULEV(*reinterpret_cast<const uint32_t*>(data));
        data+=4;
        if (usumGt(uint32_t(data-binaryCode), symNameLen, binaryCodeSize))
            throw BinException("Kernel name length is too long!");
        
        kernel.kernelName.assign((const char*)data, symNameLen);
        
        data += symNameLen;
        if (usumGt(uint32_t(data-binaryCode), 12U, binaryCodeSize))
            throw BinException("GalliumBinary is too small!!!");
        
        data32 = reinterpret_cast<uint32_t*>(data);
        kernel.sectionId = ULEV(data32[0]);
        kernel.offset = ULEV(data32[1]);
        const uint32_t argsNum = ULEV(data32[2]);
        data32 += 3;
        data = reinterpret_cast<cxbyte*>(data32);
        
        if (UINT32_MAX/24U < argsNum)
            throw BinException("Number of arguments number is too high!");
        if (usumGt(uint32_t(data-binaryCode), 24U*argsNum, binaryCodeSize))
            throw BinException("GalliumBinary is too small!!!");
        
        kernel.argInfos.resize(argsNum);
        for (uint32_t j = 0; j < argsNum; j++)
        {
            GalliumArgInfo& argInfo = kernel.argInfos[j];
            const cxuint type = ULEV(data32[0]);
            // accept not known arg type by this CLRadeonExtender
            if (type > 255)
                throw BinException("Type of kernel argument out of handled range");
            argInfo.type = GalliumArgType(type);
            argInfo.size = ULEV(data32[1]);
            argInfo.targetSize = ULEV(data32[2]);
            argInfo.targetAlign = ULEV(data32[3]);
            argInfo.signExtended = ULEV(data32[4])!=0; // force 0 or 1
            const cxuint semType = ULEV(data32[5]);
            // accept not known semantic type by this CLRadeonExtender
            if (semType > 255)
                throw BinException("Semantic of kernel argument out of handled range");
            argInfo.semantic = GalliumArgSemantic(semType);
            data32 += 6;
        }
        data = reinterpret_cast<cxbyte*>(data32);
    }
    
    if (usumGt(uint32_t(data-binaryCode), 4U, binaryCodeSize))
        throw BinException("GalliumBinary is too small!!!");
    
    sectionsNum = ULEV(data32[0]);
    if (binaryCodeSize-(data-binaryCode) < uint64_t(sectionsNum)*20U)
        throw BinException("Sections number is too big!");
    sections.reset(new GalliumSection[sectionsNum]);
    // parse sections and their content
    data32++;
    data += 4;
    
    uint32_t elfSectionId = 0; // initialize warning
    for (uint32_t i = 0; i < sectionsNum; i++)
    {
        GalliumSection& section = sections[i];
        if (usumGt(uint32_t(data-binaryCode), 20U, binaryCodeSize))
            throw BinException("GalliumBinary is too small!!!");
        
        section.sectionId = ULEV(data32[0]);
        const uint32_t secType = ULEV(data32[1]);
        // section type must be lower than 256
        if (secType > 255)
            throw BinException("Type of section out of range");
        section.type = GalliumSectionType(secType);
        section.size = ULEV(data32[2]);
        const uint32_t sizeOfData = ULEV(data32[3]);
        const uint32_t sizeFromHeader = ULEV(data32[4]); // from LLVM binary
        if (section.size != sizeOfData-4 || section.size != sizeFromHeader)
            throw BinException("Section size fields doesn't match itself!");
        
        data = reinterpret_cast<cxbyte*>(data32+5);
        if (usumGt(uint32_t(data-binaryCode), section.size, binaryCodeSize))
            throw BinException("Section size is too big!!!");
        
        section.offset = data-binaryCode;
        
        if (!elfBinary && (section.type == GalliumSectionType::TEXT ||
            section.type == GalliumSectionType::TEXT_EXECUTABLE_170))
        {
            // if new Mesa3D 17.0
            mesa170 = (section.type == GalliumSectionType::TEXT_EXECUTABLE_170);
            if (section.size < sizeof(Elf32_Ehdr))
                throw BinException("Wrong GalliumElfBinary size");
            const Elf32_Ehdr& ehdr = *reinterpret_cast<const Elf32_Ehdr*>(data);
            if (ehdr.e_ident[EI_CLASS] == ELFCLASS32)
            {
                // 32-bit
                elfBinary.reset(new GalliumElfBinary32(section.size, data,
                                 creationFlags>>GALLIUM_INNER_SHIFT, kernelsNum));
                elf64BitBinary = false;
            }
            else if (ehdr.e_ident[EI_CLASS] == ELFCLASS64)
            {
                // 64-bit
                elfSectionId = section.sectionId;
                elfBinary.reset(new GalliumElfBinary64(section.size, data,
                                 creationFlags>>GALLIUM_INNER_SHIFT, kernelsNum));
                elf64BitBinary = true;
            }
            else // wrong class
                throw BinException("Wrong GalliumElfBinary class");
        }
        data += section.size;
        data32 = reinterpret_cast<uint32_t*>(data);
    }
    
    if (!elfBinary)
        throw BinException("Gallium Elf binary not found!");
    for (uint32_t i = 0; i < kernelsNum; i++)
        if (kernels[i].sectionId != elfSectionId)
            throw BinException("Kernel not in text section!");
    // verify kernel offsets
    if (!elf64BitBinary)
        verifyKernelSymbols(kernelsNum, kernels.get(), getElfBinary32());
    else
        verifyKernelSymbols(kernelsNum, kernels.get(), getElfBinary64());
}

uint32_t GalliumBinary::getKernelIndex(const char* name) const
{
    const GalliumKernel v = { name };
    const GalliumKernel* it = binaryFind(kernels.get(), kernels.get()+kernelsNum, v,
       [](const GalliumKernel& k1, const GalliumKernel& k2)
       { return k1.kernelName < k2.kernelName; });
    if (it == kernels.get()+kernelsNum || it->kernelName != name)
        throw BinException("Can't find Gallium Kernel Index");
    return it-kernels.get();
}

void GalliumInput::addEmptyKernel(const char* kernelName, cxuint llvmVersion)
{
    GalliumKernelInput kinput = { kernelName, {
        /* default values */
        { 0x0000b848U, 0x000c0000U },
        { 0x0000b84cU, 0x00001788U },
        { 0x0000b860U, 0 } }, false, { }, 0, {} };
    kinput.config.dimMask = BINGEN_DEFAULT;
    kinput.config.usedVGPRsNum = BINGEN_DEFAULT;
    kinput.config.usedSGPRsNum = BINGEN_DEFAULT;
    kinput.config.floatMode = 0xc0;
    // for binary for LLVM>=4.0.0 userDataNum will be filled by AsmGalliumFormat or user
    kinput.config.userDataNum = (llvmVersion >= 40000U) ? BINGEN8_DEFAULT : 4;
    kinput.config.spilledVGPRs = kinput.config.spilledSGPRs = 0;
    kernels.push_back(std::move(kinput));
}

/*
 * GalliumBinGenerator 
 */

GalliumBinGenerator::GalliumBinGenerator() : manageable(false), input(nullptr)
{ }

GalliumBinGenerator::GalliumBinGenerator(const GalliumInput* galliumInput)
        : manageable(false), input(galliumInput)
{ }

GalliumBinGenerator::GalliumBinGenerator(bool _64bitMode, GPUDeviceType deviceType,
        size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        const std::vector<GalliumKernelInput>& kernels)
        : manageable(true), input(nullptr)
{
    std::unique_ptr<GalliumInput> _input(new GalliumInput{});
    _input->is64BitElf = _64bitMode;
    _input->isLLVM390 = _input->isMesa170 = false;
    _input->deviceType = deviceType;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->kernels = kernels;
    _input->codeSize = codeSize;
    _input->code = code;
    _input->commentSize = 0;
    _input->comment = nullptr;
    input = _input.release();
}

GalliumBinGenerator::GalliumBinGenerator(bool _64bitMode, GPUDeviceType deviceType,
        size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        std::vector<GalliumKernelInput>&& kernels)
        : manageable(true), input(nullptr)
{
    std::unique_ptr<GalliumInput> _input(new GalliumInput{});
    _input->is64BitElf = _64bitMode;
    _input->isLLVM390 = _input->isMesa170 = false;
    _input->deviceType = deviceType;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->kernels = std::move(kernels);
    _input->codeSize = codeSize;
    _input->code = code;
    _input->commentSize = 0;
    _input->comment = nullptr;
    input = _input.release();
}


GalliumBinGenerator::~GalliumBinGenerator()
{
    if (manageable)
        delete input;
}

void GalliumBinGenerator::setInput(const GalliumInput* input)
{
    if (manageable)
        delete input;
    manageable = false;
    this->input = input;
}

// section index for symbol binding
static const uint16_t mainBuiltinSectionTable[] =
{
    8, // ELFSECTID_SHSTRTAB
    10, // ELFSECTID_STRTAB
    9, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    1, // ELFSECTID_TEXT
    5, // ELFSECTID_RODATA
    2, // ELFSECTID_DATA
    3, // ELFSECTID_BSS
    6, // ELFSECTID_COMMENT
    4, // GALLIUMSECTID_GPUCONFIG
    7, // GALLIUMSECTID_NOTEGNUSTACK
    SHN_UNDEF // GALLIUMSECTID_RELTEXT
};

// section index for symbol binding
static const uint16_t mainBuiltinSectionTable2[] =
{
    7, // ELFSECTID_SHSTRTAB
    9, // ELFSECTID_STRTAB
    8, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    1, // ELFSECTID_TEXT
    SHN_UNDEF, // ELFSECTID_RODATA
    2, // ELFSECTID_DATA
    3, // ELFSECTID_BSS
    5, // ELFSECTID_COMMENT
    4, // GALLIUMSECTID_GPUCONFIG
    6, // GALLIUMSECTID_NOTEGNUSTACK
    SHN_UNDEF // GALLIUMSECTID_RELTEXT
};

// section index for symbol binding
static const uint16_t mainBuiltinSectionTableRel[] =
{
    9, // ELFSECTID_SHSTRTAB
    11, // ELFSECTID_STRTAB
    10, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    1, // ELFSECTID_TEXT
    6, // ELFSECTID_RODATA
    3, // ELFSECTID_DATA
    4, // ELFSECTID_BSS
    7, // ELFSECTID_COMMENT
    5, // GALLIUMSECTID_GPUCONFIG
    8, // GALLIUMSECTID_NOTEGNUSTACK
    2 // GALLIUMSECTID_RELTEXT
};

// section index for symbol binding
static const uint16_t mainBuiltinSectionTable2Rel[] =
{
    8, // ELFSECTID_SHSTRTAB
    10, // ELFSECTID_STRTAB
    9, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    1, // ELFSECTID_TEXT
    SHN_UNDEF, // ELFSECTID_RODATA
    3, // ELFSECTID_DATA
    4, // ELFSECTID_BSS
    6, // ELFSECTID_COMMENT
    5, // GALLIUMSECTID_GPUCONFIG
    7, // GALLIUMSECTID_NOTEGNUSTACK
    2  // GALLIUMSECTID_RELTEXT
};

// helper to writing AMDGPU configuration content
class CLRX_INTERNAL AmdGpuConfigContent: public ElfRegionContent
{
private:
    const Array<uint32_t>& kernelsOrder;
    const GalliumInput& input;
public:
    AmdGpuConfigContent(const Array<uint32_t>& inKernelsOrder,
            const GalliumInput& inInput) : kernelsOrder(inKernelsOrder), input(inInput)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        const GPUArchitecture arch = getGPUArchitectureFromDeviceType(input.deviceType);
        const cxuint progInfoEntriesNum = input.isLLVM390 ? 5 : 3;
        
        for (uint32_t korder: kernelsOrder)
        {
            const GalliumKernelInput& kernel = input.kernels[korder];
            GalliumProgInfoEntry outEntries[5];
            if (kernel.useConfig)
            {
                const GalliumKernelConfig& config = kernel.config;
                // set entry addresses (PGM_RSRC1, PGM_RSRC2, SCRATCHSIZE, spilledSGPRS
                outEntries[0].address = ULEV(0x0000b848U);
                outEntries[1].address = ULEV(0x0000b84cU);
                outEntries[2].address = ULEV(0x0000b860U);
                outEntries[3].address = ULEV(0x00000004U);
                outEntries[4].address = ULEV(0x00000008U);
                
                uint32_t scratchBlocks = ((config.scratchBufferSize<<6) + 1023)>>10;
                cxuint sgprsNum = std::max(config.usedSGPRsNum, 1U);
                cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
                /// pgmRSRC1 and pgmRSRC2
                outEntries[0].value = (config.pgmRSRC1) |
                    calculatePgmRSrc1(arch, vgprsNum, sgprsNum, config.priority,
                            config.floatMode, config.privilegedMode, config.dx10Clamp,
                            config.debugMode, config.ieeeMode);
                outEntries[1].value = (config.pgmRSRC2 & 0xffffe440U) |
                    calculatePgmRSrc2(arch, (config.scratchBufferSize != 0),
                            config.userDataNum, false, config.dimMask,
                            (config.pgmRSRC2 & 0x1b80U), config.tgSize,
                            config.localSize, config.exceptions);
                
                outEntries[2].value = (scratchBlocks)<<12;
                if (input.isLLVM390)
                {
                    outEntries[3].value = config.spilledSGPRs;
                    outEntries[4].value = config.spilledVGPRs;
                }
                for (cxuint k = 0; k < progInfoEntriesNum; k++)
                    outEntries[k].value = ULEV(outEntries[k].value);
            }
            else
            {
                for (cxuint k = 0; k < progInfoEntriesNum; k++)
                {
                    outEntries[k].address = ULEV(kernel.progInfo[k].address);
                    outEntries[k].value = ULEV(kernel.progInfo[k].value);
                }
            }
            fob.writeArray(progInfoEntriesNum, outEntries);
        }
    }
};

// helper to writing AMDGPU configuration content
template<typename Types>
class CLRX_INTERNAL GalliumRelTextContent: public ElfRegionContent
{
private:
    const GalliumInput& input;
public:
    GalliumRelTextContent(const GalliumInput& inInput) : input(inInput)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        const size_t symIndex = 8 + (input.globalData!=nullptr) +
                    (!input.scratchRelocs.empty());
        for (const GalliumScratchReloc& reloc: input.scratchRelocs)
        {
            typename Types::Rel outReloc{};
            SLEV(outReloc.r_offset, reloc.offset);
            SLEV(outReloc.r_info, ElfBinaryGenTemplate<Types>::getRelInfo(
                    symIndex + (reloc.type == RELTYPE_HIGH_32BIT), 0x81));
            fob.writeObject(outReloc);
        }
    }
};

template<typename Types>
static void putSectionsAndSymbols(ElfBinaryGenTemplate<Types>& elfBinGen,
      const GalliumInput* input, const Array<uint32_t>& kernelsOrder,
      const AmdGpuConfigContent& amdGpuConfigContent,
      const GalliumRelTextContent<Types>& relTextContent)
{
    const char* comment = "CLRX GalliumBinGenerator " CLRX_VERSION;
    uint32_t commentSize = ::strlen(comment);
    
    /* choose builtin section table and extraSectionStartIndex */
    const uint16_t* curMainBuiltinSections = nullptr;
    if (input->scratchRelocs.empty())
        curMainBuiltinSections = (input->globalData!=nullptr) ?
            mainBuiltinSectionTable : mainBuiltinSectionTable2;
    else // if relocations
        curMainBuiltinSections = (input->globalData!=nullptr) ?
            mainBuiltinSectionTableRel : mainBuiltinSectionTable2Rel;
    cxuint startSectionIndex = 10 + (input->globalData!=nullptr) +
                (!input->scratchRelocs.empty());
    
    typedef ElfRegionTemplate<Types> ElfRegion;
    typedef ElfSymbolTemplate<Types> ElfSymbol;
    const uint32_t kernelsNum = input->kernels.size();
    elfBinGen.addRegion(ElfRegion(input->codeSize, input->code, 256, ".text",
            SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR));
    if (!input->scratchRelocs.empty())
        // .rel.text section
        elfBinGen.addRegion(ElfRegion(input->scratchRelocs.size()*
                sizeof(typename Types::Rel), &relTextContent,
                sizeof(typename Types::Word), ".rel.text", SHT_REL, 0,
                curMainBuiltinSections[ELFSECTID_SYMTAB-ELFSECTID_START], 1, 0,
                sizeof(typename Types::Rel), false, sizeof(typename Types::Word)));
    
    elfBinGen.addRegion(ElfRegion(0, (const cxbyte*)nullptr, 4, ".data",
            SHT_PROGBITS, SHF_ALLOC|SHF_WRITE));
    elfBinGen.addRegion(ElfRegion(0, (const cxbyte*)nullptr, 4, ".bss",
            SHT_NOBITS, SHF_ALLOC|SHF_WRITE));
    // write configuration for kernel execution
    const cxuint progInfoEntriesNum = input->isLLVM390 ? 5 : 3;
    elfBinGen.addRegion(ElfRegion(uint64_t(8U*progInfoEntriesNum)*kernelsNum,
            &amdGpuConfigContent, 1, ".AMDGPU.config", SHT_PROGBITS, 0));
    
    if (input->globalData!=nullptr)
        elfBinGen.addRegion(ElfRegion(input->globalDataSize, input->globalData, 4,
                ".rodata", SHT_PROGBITS, SHF_ALLOC));
    
    if (input->comment!=nullptr)
    {
        // if comment, store comment section
        comment = input->comment;
        commentSize = input->commentSize;
        if (commentSize==0)
            commentSize = ::strlen(comment);
    }
    elfBinGen.addRegion(ElfRegion(commentSize, (const cxbyte*)comment, 1,
                ".comment", SHT_PROGBITS, SHF_STRINGS|SHF_MERGE, 0, 0, 0, 1));
    elfBinGen.addRegion(ElfRegion(0, (const cxbyte*)nullptr, 1, ".note.GNU-stack",
            SHT_PROGBITS, 0));
    elfBinGen.addRegion(ElfRegion::shstrtabSection());
    elfBinGen.addRegion(ElfRegion::sectionHeaderTable());
    elfBinGen.addRegion(ElfRegion::symtabSection(false));
    elfBinGen.addRegion(ElfRegion::strtabSection());
    /* symbols */
    /// EndOfTextLabel - ?? always is at end of symbol table
    elfBinGen.addSymbol({"EndOfTextLabel", 1, ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE),
            0, false, uint32_t(input->codeSize), 0});
    const cxuint sectSymsNum = 6 + (input->globalData!=nullptr) +
            (!input->scratchRelocs.empty());
    // local symbols for sections
    for (cxuint i = 0; i < sectSymsNum; i++)
        elfBinGen.addSymbol({"", uint16_t(i+1), ELF32_ST_INFO(STB_LOCAL, STT_SECTION),
                0, false, 0, 0});
    if (!input->scratchRelocs.empty())
    {
        // put scratch rsrc symbols
        elfBinGen.addSymbol({"SCRATCH_RSRC_DWORD0", SHN_UNDEF,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0, false, 0, 0});
        elfBinGen.addSymbol({"SCRATCH_RSRC_DWORD1", SHN_UNDEF,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0, false, 0, 0});
    }
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        elfBinGen.addSymbol({kernel.kernelName.c_str(), 1,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0, false, kernel.offset, 0});
    }
    
    /* extra sections */
    for (const BinSection& section: input->extraSections)
        elfBinGen.addRegion(ElfRegion(section, curMainBuiltinSections,
                         GALLIUMSECTID_MAX, startSectionIndex));
    /* extra symbols */
    for (const BinSymbol& symbol: input->extraSymbols)
        elfBinGen.addSymbol(ElfSymbol(symbol, curMainBuiltinSections,
                         GALLIUMSECTID_MAX, startSectionIndex));
}

void GalliumBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    const uint32_t kernelsNum = input->kernels.size();
    /* compute size of binary */
    uint64_t elfSize = 0;
    uint64_t binarySize = uint64_t(8) + size_t(kernelsNum)*16U + 20U /* section */;
    for (const GalliumKernelInput& kernel: input->kernels)
        binarySize += uint64_t(kernel.argInfos.size())*24U + kernel.kernelName.size();
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
    const cxuint maxSGPRSNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, 0);
    const cxuint maxVGPRSNum = getGPUMaxRegistersNum(arch, REGTYPE_VGPR, 0);
    for (const GalliumKernelInput& kernel: input->kernels)
        if (kernel.useConfig)
        {
            // veryfying values gallium config
            const GalliumKernelConfig& config = kernel.config;
            if (config.usedVGPRsNum > maxVGPRSNum)
                throw BinGenException("Used VGPRs number out of range");
            if (config.usedSGPRsNum > maxSGPRSNum)
                throw BinGenException("Used SGPRs number out of range");
            if (config.localSize > 32768)
                throw BinGenException("LocalSize out of range");
            if (config.priority >= 4)
                throw BinGenException("Priority out of range");
            if (config.userDataNum > 16)
                throw BinGenException("UserDataNum out of range");
        }
    
    // sort kernels by name (for correct order in binary file) */
    Array<uint32_t> kernelsOrder(kernelsNum);
    for (cxuint i = 0; i < kernelsNum; i++)
        kernelsOrder[i] = i;
    std::sort(kernelsOrder.begin(), kernelsOrder.end(),
          [this](const uint32_t& a,const uint32_t& b)
          { return input->kernels[a].kernelName < input->kernels[b].kernelName; });
    
    std::unique_ptr<ElfBinaryGen32> elfBinGen32;
    std::unique_ptr<ElfBinaryGen64> elfBinGen64;
    
    AmdGpuConfigContent amdGpuConfigContent(kernelsOrder, *input);
    GalliumRelTextContent<Elf32Types> relTextContent32(*input);
    GalliumRelTextContent<Elf64Types> relTextContent64(*input);
    if (!input->is64BitElf)
    {
        /* 32-bit ELF */
        if (!input->isMesa170)
            elfBinGen32.reset(new ElfBinaryGen32({ 0, 0, ELFOSABI_SYSV, 0,  ET_REL, 0,
                        EV_CURRENT, UINT_MAX, 0, 0 }));
        else
            elfBinGen32.reset(new ElfBinaryGen32({ 0, 0, 0x40, 0,  ET_REL, 0xe0,
                        EV_CURRENT, UINT_MAX, 0, 0 }));
        putSectionsAndSymbols(*elfBinGen32, input, kernelsOrder, amdGpuConfigContent,
                        relTextContent32);
        elfSize = elfBinGen32->countSize();
    }
    else
    {
        /* 64-bit ELF */
        if (!input->isMesa170)
            elfBinGen64.reset(new ElfBinaryGen64({ 0, 0, ELFOSABI_SYSV, 0,  ET_REL, 0,
                        EV_CURRENT, UINT_MAX, 0, 0 }));
        else // new Mesa3D 17.0.0
            elfBinGen64.reset(new ElfBinaryGen64({ 0, 0, 0x40, 0,  ET_REL, 0xe0,
                        EV_CURRENT, UINT_MAX, 0, 0 }));
        putSectionsAndSymbols(*elfBinGen64, input, kernelsOrder, amdGpuConfigContent,
                        relTextContent64);
        elfSize = elfBinGen64->countSize();
    }

    binarySize += elfSize;
    
    if (
#ifdef HAVE_64BIT
        !input->is64BitElf &&
#endif
        elfSize > UINT32_MAX)
        throw BinGenException("Elf binary size is too big!");
    
#ifdef HAVE_32BIT
    if (binarySize > UINT32_MAX)
        throw BinGenException("Binary size is too big!");
#endif
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
    FastOutputBuffer bos(256, *os);
    bos.writeObject<uint32_t>(LEV(kernelsNum));
    // write Gallium kernel info
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        if (kernel.offset >= input->codeSize)
            throw BinGenException("Kernel offset out of range");
        
        bos.writeObject<uint32_t>(LEV(uint32_t(kernel.kernelName.size())));
        bos.writeArray(kernel.kernelName.size(), kernel.kernelName.c_str());
        const uint32_t other[3] = { 0, LEV(kernel.offset),
            LEV(cxuint(kernel.argInfos.size())) };
        bos.writeArray(3, other);
        
        // put kernel arguments
        for (const GalliumArgInfo arg: kernel.argInfos)
        {
            const uint32_t argData[6] = { LEV(cxuint(arg.type)),
                LEV(arg.size), LEV(arg.targetSize), LEV(arg.targetAlign),
                LEV(arg.signExtended?1U:0U), LEV(cxuint(arg.semantic)) };
            bos.writeArray(6, argData);
        }
    }
    /* section */
    {
        const uint32_t secType = uint32_t(input->isMesa170 ?
                GalliumSectionType::TEXT_EXECUTABLE_170 : GalliumSectionType::TEXT);
        const uint32_t section[6] = { LEV(1U), LEV(0U), LEV(secType),
            LEV(uint32_t(elfSize)), LEV(uint32_t(elfSize+4)), LEV(uint32_t(elfSize)) };
        bos.writeArray(6, section);
    }
    if (!input->is64BitElf)
        elfBinGen32->generate(bos);
    else // 64-bit
        elfBinGen64->generate(bos);
    assert(bos.getWritten() == binarySize);
    }
    catch(...)
    {
        os->exceptions(oldExceptions);
        throw;
    }
    os->exceptions(oldExceptions);
}

void GalliumBinGenerator::generate(Array<cxbyte>& array) const
{
    generateInternal(nullptr, nullptr, &array);
}

void GalliumBinGenerator::generate(std::ostream& os) const
{
    generateInternal(&os, nullptr, nullptr);
}

void GalliumBinGenerator::generate(std::vector<char>& v) const
{
    generateInternal(nullptr, &v, nullptr);
}

static const char* mesaOclMagicString = "OpenCL 1.1 MESA";
static const char* mesaOclMagicString2 = "OpenCL 1.1 Mesa";
static std::mutex detectionMutex;
static uint64_t detectionFileTimestamp = 0;
static std::string detectionMesaOclPath;
static uint64_t detectionLLVMFileTimestamp = 0;
static std::string detectionLlvmConfigPath;
static uint32_t detectedDriverVersion = 0;
static uint32_t detectedLLVMVersion = 0;

static std::string escapePath(const std::string& path)
{
    std::string newPath;
    for (char c: path)
        if (c == CLRX_NATIVE_DIR_SEP)
            newPath.append("%#");
        else if (c == '%')
            newPath.append("%%");
        else if (c == ':')
            newPath.append("%$");
        else
            newPath += c;
    return newPath;
}

uint32_t CLRX::detectMesaDriverVersion()
{
    std::lock_guard<std::mutex> lock(detectionMutex);
    std::string mesaOclPath = findMesaOCL();
    
    bool notThisSameFile = false;
    if (mesaOclPath != detectionMesaOclPath)
    {
        notThisSameFile = true;
        detectionMesaOclPath = mesaOclPath;
    }
    
    uint64_t timestamp = 0;
    try
    { timestamp = getFileTimestamp(mesaOclPath.c_str()); }
    catch(const Exception& ex)
    { }
    
    if (!notThisSameFile && timestamp == detectionFileTimestamp)
        return detectedDriverVersion;
    
    std::string clrxTimestampPath = getHomeDir();
    if (!clrxTimestampPath.empty())
    {
        // first, we check from stored version in config files
        clrxTimestampPath = joinPaths(clrxTimestampPath, ".clrxmesaocltstamp");
        try
        { makeDir(clrxTimestampPath.c_str()); }
        catch(const std::exception& ex)
        { }
        // file path
        clrxTimestampPath = joinPaths(clrxTimestampPath, escapePath(mesaOclPath));
        try
        {
        std::ifstream ifs(clrxTimestampPath.c_str(), std::ios::binary);
        if (ifs)
        {
            // read driver version from stored config files
            ifs.exceptions(std::ios::badbit | std::ios::failbit);
            uint64_t readedTimestamp = 0;
            uint32_t readedVersion = 0;
            ifs >> readedTimestamp >> readedVersion;
            if (readedTimestamp!=0 && readedVersion!=0 && timestamp==readedTimestamp)
            {
                // Mesa3D files has not been changed
                detectionFileTimestamp = readedTimestamp;
                detectedDriverVersion = readedVersion;
                return readedVersion;
            }
        }
        }
        catch(const std::exception& ex)
        { }
    }
    detectionFileTimestamp = timestamp;
    detectedDriverVersion = 0;
    try
    {
        std::ifstream fs(mesaOclPath.c_str(), std::ios::binary);
        if (!fs) return 0;
        FastInputBuffer fib(256, fs);
        size_t index = 0;
        // find MESA3D magic in library file
        while (mesaOclMagicString[index]!=0)
        {
            int c = fib.get();
            if (c == std::streambuf::traits_type::eof())
                break;
            if (mesaOclMagicString[index]==c)
                index++;
            else if (mesaOclMagicString2[index]==c)
                index++;
            else // reset
                index=0;
        }
        if (mesaOclMagicString[index]==0)
        { //
            char buf[30];
            ::memset(buf, 0, 30); // zeroing for safe
            if (fib.get()!=' ')
                return 0; // skip space
            fib.read(buf, 30);
            // getting version from this string (after magic)
            const char* next;
            detectedDriverVersion = cstrtoui(buf, buf+30, next)*10000;
            if (next!=buf+30 && *next=='.') // minor version
                detectedDriverVersion += (cstrtoui(next+1, buf+30, next)%100)*100;
            if (next!=buf+30 && *next=='.') // micro version
                detectedDriverVersion += cstrtoui(next+1, buf+30, next)%100;
        }
        
        // write to config
        if (!clrxTimestampPath.empty()) // if clrxamdocltstamp set
        {
            // write to
            std::ofstream ofs(clrxTimestampPath.c_str(), std::ios::binary);
            ofs << detectionFileTimestamp << " " << detectedDriverVersion;
        }
    }
    catch(const std::exception& ex)
    { }
    return detectedDriverVersion;
}

uint32_t CLRX::detectLLVMCompilerVersion()
{
    std::lock_guard<std::mutex> lock(detectionMutex);
    std::string llvmConfigPath = findLLVMConfig();
    
    bool notThisSameFile = false;
    if (llvmConfigPath != detectionLlvmConfigPath)
    {
        notThisSameFile = true;
        detectionLlvmConfigPath = llvmConfigPath;
    }
    
    uint64_t timestamp = 0;
    try
    { timestamp = getFileTimestamp(llvmConfigPath.c_str()); }
    catch(const Exception& ex)
    { }
    
    if (!notThisSameFile && timestamp == detectionLLVMFileTimestamp)
        return detectedLLVMVersion;
    
    std::string clrxTimestampPath = getHomeDir();
    if (!clrxTimestampPath.empty())
    {
        // first, we check from stored version in config files
        clrxTimestampPath = joinPaths(clrxTimestampPath, ".clrxllvmcfgtstamp");
        try
        { makeDir(clrxTimestampPath.c_str()); }
        catch(const std::exception& ex)
        { }
        // file path
        clrxTimestampPath = joinPaths(clrxTimestampPath, escapePath(llvmConfigPath));
        try
        {
        std::ifstream ifs(clrxTimestampPath.c_str(), std::ios::binary);
        if (ifs)
        {
            // read driver version from stored config files
            ifs.exceptions(std::ios::badbit | std::ios::failbit);
            uint64_t readedTimestamp = 0;
            uint32_t readedVersion = 0;
            ifs >> readedTimestamp >> readedVersion;
            if (readedTimestamp!=0 && readedVersion!=0 && timestamp==readedTimestamp)
            {
                // LLVM files has not been changed
                detectionLLVMFileTimestamp = readedTimestamp;
                detectedLLVMVersion = readedVersion;
                return readedVersion;
            }
        }
        }
        catch(const std::exception& ex)
        { }
    }
    detectionLLVMFileTimestamp = timestamp;
    detectedLLVMVersion = 0;
    try
    {
        // execute llvm-config to get version
        const char* arguments[3] = { llvmConfigPath.c_str(), "--version", nullptr };
        Array<cxbyte> out = runExecWithOutput(llvmConfigPath.c_str(), arguments);
        const char* next;
        // parse this output (version)
        const char* end = ((const char*)out.data()) + out.size();
        detectedLLVMVersion = cstrtoui(((const char*)out.data()), end, next)*10000;
        if (next!=end && *next=='.') // minor version
            detectedLLVMVersion += (cstrtoui(next+1, end, next)%100)*100;
        if (next!=end && *next=='.') // micro version
            detectedLLVMVersion += cstrtoui(next+1, end, next)%100;
        
        // write to config
        if (!clrxTimestampPath.empty()) // if clrxamdocltstamp set
        {
            // write to
            std::ofstream ofs(clrxTimestampPath.c_str(), std::ios::binary);
            ofs << detectionLLVMFileTimestamp << " " << detectedLLVMVersion;
        }
    }
    catch(const std::exception& ex)
    { }
    return detectedLLVMVersion;
}
