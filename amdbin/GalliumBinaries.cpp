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
#include <cassert>
#include <climits>
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/GalliumBinaries.h>

using namespace CLRX;

/* Gallium ELF binary */

GalliumElfBinary::GalliumElfBinary() :
        progInfosNum(0), progInfoEntries(nullptr), disasmSize(0), disasmOffset(0)
{ }

GalliumElfBinary::GalliumElfBinary(size_t binaryCodeSize, cxbyte* binaryCode,
               cxuint creationFlags) : 
       ElfBinary32(binaryCodeSize, binaryCode, creationFlags),
       progInfosNum(0), progInfoEntries(nullptr), disasmSize(0), disasmOffset(0)
       
{
    uint16_t amdGpuConfigIndex = SHN_UNDEF;
    try
    { amdGpuConfigIndex = getSectionIndex(".AMDGPU.config"); }
    catch(const Exception& ex)
    { }
    
    uint16_t amdGpuDisasmIndex = SHN_UNDEF;
    try
    { amdGpuDisasmIndex = getSectionIndex(".AMDGPU.disasm"); }
    catch(const Exception& ex)
    { }
    if (amdGpuDisasmIndex != SHN_UNDEF)
    {
        const Elf32_Shdr& shdr = getSectionHeader(amdGpuDisasmIndex);
        disasmOffset = ULEV(shdr.sh_offset);
        disasmSize = ULEV(shdr.sh_size);
    }
    
    uint16_t textIndex = SHN_UNDEF;
    uint32_t textSize = 0;
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
    if (hasProgInfoMap())
        progInfoEntryMap.resize(symbolsNum);
    for (cxuint i = 0; i < symbolsNum; i++)
    {
        const Elf32_Sym& sym = getSymbol(i);
        const char* symName = getSymbolName(i);
        if (ULEV(sym.st_shndx) == textIndex && ELF32_ST_BIND(sym.st_info) == STB_GLOBAL)
        {
            if (ULEV(sym.st_value) >= textSize)
                throw Exception("kernel symbol offset out of range");
            if (hasProgInfoMap())
                progInfoEntryMap[progInfosNum] = std::make_pair(symName, 3*progInfosNum);
            progInfosNum++;
        }
    }
    if (progInfosNum*24U != ULEV(shdr.sh_size))
        throw Exception("Number of symbol kernels doesn't match progInfos number!");
    progInfoEntries = reinterpret_cast<GalliumProgInfoEntry*>(binaryCode +
                ULEV(shdr.sh_offset));
    
    if (hasProgInfoMap())
    {
        progInfoEntryMap.resize(progInfosNum);
        mapSort(progInfoEntryMap.begin(), progInfoEntryMap.end(), CStringLess());
    }
}

uint32_t GalliumElfBinary::getProgramInfoEntriesNum(uint32_t index) const
{ return 3; }

uint32_t GalliumElfBinary::getProgramInfoEntryIndex(const char* name) const
{
    ProgInfoEntryIndexMap::const_iterator it = binaryMapFind(progInfoEntryMap.begin(),
                         progInfoEntryMap.end(), name, CStringLess());
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

GalliumBinary::GalliumBinary(size_t binaryCodeSize, cxbyte* binaryCode,
                 cxuint creationFlags)
try
         : kernelsNum(0), sectionsNum(0), kernels(nullptr), sections(nullptr)
{
    this->creationFlags = creationFlags;
    this->binaryCode = binaryCode;
    this->binaryCodeSize = binaryCodeSize;
    
    if (binaryCodeSize < 4)
        throw Exception("GalliumBinary is too small!!!");
    uint32_t* data32 = reinterpret_cast<uint32_t*>(binaryCode);
    kernelsNum = ULEV(*data32);
    if (binaryCodeSize < uint64_t(kernelsNum)*16U)
        throw Exception("Kernels number is too big!");
    kernels = new GalliumKernel[kernelsNum];
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
        
        /// check kernel name order (sorted order is required by Mesa3D radeon driver)
        if (i != 0 && kernel.kernelName < kernels[i-1].kernelName)
            throw Exception("Unsorted kernel table!");
        
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
            if (type > 255)
                throw Exception("Type of kernel argument out of handled range");
            argInfo.type = GalliumArgType(type);
            argInfo.size = ULEV(data32[1]);
            argInfo.targetSize = ULEV(data32[2]);
            argInfo.targetAlign = ULEV(data32[3]);
            argInfo.signExtended = ULEV(data32[4])!=0;
            const cxuint semType = ULEV(data32[5]);
            if (semType > 255)
                throw Exception("Semantic of kernel argument out of handled range");
            argInfo.semantic = GalliumArgSemantic(semType);
            data32 += 6;
        }
        data = reinterpret_cast<cxbyte*>(data32);
    }
    
    if (usumGt(uint32_t(data-binaryCode), 4U, binaryCodeSize))
        throw Exception("GalliumBinary is too small!!!");
    
    sectionsNum = ULEV(data32[0]);
    if (binaryCodeSize-(data-binaryCode) < uint64_t(sectionsNum)*20U)
        throw Exception("Sections number is too big!");
    sections = new GalliumSection[sectionsNum];
    // parse sections and their content
    data32++;
    data += 4;
    
    uint32_t elfSectionId;
    for (uint32_t i = 0; i < sectionsNum; i++)
    {
        GalliumSection& section = sections[i];
        if (usumGt(uint32_t(data-binaryCode), 20U, binaryCodeSize))
            throw Exception("GalliumBinary is too small!!!");
        
        section.sectionId = ULEV(data32[0]);
        const uint32_t secType = ULEV(data32[1]);
        if (secType > 255)
            throw Exception("Type of section out of range");
        section.type = GalliumSectionType(secType);
        section.size = ULEV(data32[2]);
        const uint32_t sizeOfData = ULEV(data32[3]);
        const uint32_t sizeFromHeader = ULEV(data32[4]); // from LLVM binary
        if (section.size != sizeOfData-4 || section.size != sizeFromHeader)
            throw Exception("Section size fields doesn't match itself!");
        
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
    for (uint32_t i = 0; i < kernelsNum; i++)
        if (kernels[i].sectionId != elfSectionId)
            throw Exception("Kernel not in text section!");
    // verify kernel offsets
    cxuint symIndex = 0;
    const cxuint symsNum = elfBinary.getSymbolsNum();
    uint16_t textIndex = elfBinary.getSectionIndex(".text");
    for (uint32_t i = 0; i < kernelsNum; i++)
    {
        const GalliumKernel& kernel = kernels[i];
        for (; symIndex < symsNum; symIndex++)
        {
            const Elf32_Sym& sym = elfBinary.getSymbol(symIndex);
            const char* symName = elfBinary.getSymbolName(symIndex);
            if (ULEV(sym.st_shndx) == textIndex && ELF32_ST_BIND(sym.st_info) == STB_GLOBAL)
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
catch(...)
{
    delete[] kernels;
    delete[] sections;
    throw;
}

GalliumBinary::~GalliumBinary()
{
    delete[] kernels;
    delete[] sections;
}

uint32_t GalliumBinary::getKernelIndex(const char* name) const
{
    const GalliumKernel v = { name };
    const GalliumKernel* it = binaryFind(kernels, kernels+kernelsNum, v,
       [](const GalliumKernel& k1, const GalliumKernel& k2)
       { return k1.kernelName < k2.kernelName; });
    if (it == kernels+kernelsNum || it->kernelName != name)
        throw Exception("Can't find Gallium Kernel Index");
    return it-kernels;
}

/*
 * GalliumBinGenerator 
 */

GalliumBinGenerator::GalliumBinGenerator() : manageable(false), input(nullptr)
{ }

GalliumBinGenerator::GalliumBinGenerator(const GalliumInput* galliumInput)
        : manageable(false), input(galliumInput)
{ }

GalliumBinGenerator::GalliumBinGenerator(size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        const std::vector<GalliumKernelInput>& kernels,
        size_t disassemblySize, const char* disassembly,
        size_t commentSize, const char* comment)
        : manageable(true), input(nullptr)
{
    GalliumInput* newInput = new GalliumInput;
    try
    {
        newInput->globalDataSize = globalDataSize;
        newInput->globalData = globalData;
        newInput->codeSize = codeSize;
        newInput->code = code;
        newInput->kernels = kernels;
        newInput->disassemblySize = disassemblySize;
        newInput->disassembly = disassembly;
        newInput->commentSize = commentSize;
        newInput->comment = comment;
    }
    catch(...)
    {
         delete newInput;
         throw;
    }
    input = newInput;
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

static void putElfSectionLE(Elf32_Shdr* section, uint32_t shName, uint32_t shType,
        uint32_t shFlags, uint32_t offset, uint32_t size, uint32_t link,
        uint32_t info = 0, uint32_t addrAlign = 1, uint32_t addr = 0, uint32_t entSize = 0)
{
    SULEV(section->sh_name, shName);
    SULEV(section->sh_type, shType);
    SULEV(section->sh_flags, shFlags);
    SULEV(section->sh_addr, 0);
    SULEV(section->sh_offset, offset);
    SULEV(section->sh_size, size);
    SULEV(section->sh_link, link);
    SULEV(section->sh_info, info);
    SULEV(section->sh_addralign, addrAlign);
    SULEV(section->sh_entsize, entSize);
}

static void putElfSymbolLE(Elf32_Sym* symbol, uint32_t symName, uint32_t value,
        uint32_t size, uint16_t shndx, cxbyte info, cxbyte other = 0)
{
    SULEV(symbol->st_name, symName); 
    SULEV(symbol->st_value, value);
    SULEV(symbol->st_size, size);
    SULEV(symbol->st_shndx, shndx);
    symbol->st_info = info;
    symbol->st_other = other;
}

static inline void fixAlignment(cxbyte* binary, size_t& offset, size_t elfOffset)
{
    if (((offset-elfOffset) & 3) != 0)
    {   // fix alignment
        const cxuint alignFix = 4-((offset-elfOffset)&3);
        ::memset(binary + offset, 0, alignFix);
        offset += alignFix;
    }
}

cxbyte* GalliumBinGenerator::generate(size_t& outBinarySize) const
{
    const uint32_t kernelsNum = input->kernels.size();
    /* compute size of binary */
    uint64_t elfSize = 0;
    uint64_t binarySize = uint64_t(8) + kernelsNum*16U + 20U /* section */;
    for (const GalliumKernelInput& kernel: input->kernels)
        binarySize += uint64_t(kernel.argInfos.size())*24U + kernel.kernelName.size();
    
    // elf extra data
    uint32_t commentSize = 28;
    const char* comment = "CLRX GalliumBinGenerator 0.1";
    if (input->comment!=nullptr)
    {
        comment = input->comment;
        commentSize = input->commentSize;
        if (commentSize==0)
            commentSize = ::strlen(comment);
    }
    uint32_t disassemblySize = input->disassemblySize;
    const char* disassembly = input->disassembly;
    if (input->disassembly!=nullptr)
    {
        if (disassemblySize==0)
            disassemblySize = ::strlen(disassembly);
    }
    else
        disassemblySize = 0;
    
    /* ELF binary */
    const cxuint elfSectionsNum = 10 + (input->globalData!=nullptr) +
            (disassembly!=nullptr);
    elfSize = 0x100 /* header */ + input->codeSize;
    if ((elfSize& 3) != 0) // alignment
        elfSize += 4-(elfSize&3);
    elfSize += uint64_t(24U)*kernelsNum; // AMDGPU.config
    elfSize += disassemblySize; // AMDGPU.disasm
    if ((elfSize & 3) != 0) // alignment
        elfSize += 4-(elfSize&3);
    if (input->globalData!=nullptr) // .rodata
        elfSize += input->globalDataSize;
    elfSize += commentSize;  // .comment
    /// .shstrtab
    const cxuint shstrtabSize = 84 + ((input->globalData!=nullptr)?8:0) +
            ((disassembly!=nullptr)?15:0);
    elfSize += shstrtabSize;
    if ((elfSize & 3) != 0) // alignment
        elfSize += 4-(elfSize&3);
    elfSize += sizeof(Elf32_Shdr) * elfSectionsNum; // section table
    // .symtab
    elfSize += sizeof(Elf32_Sym) * (kernelsNum + 8 +
            (input->globalData!=nullptr) + (disassembly!=nullptr));
    /* strtab */
    uint64_t strtabSize = 0;
    for (const GalliumKernelInput& kernel: input->kernels)
        strtabSize += kernel.kernelName.size() + 1;
    strtabSize += 16; // enf of label and zero byte
    elfSize += strtabSize;
    
    if (elfSize > UINT32_MAX)
        throw Exception("Elf binary size is too big!");
    
    binarySize += elfSize;
    // sort kernels by name (for correct order in binary file) */
    std::vector<uint32_t> kernelsOrder(kernelsNum);
    for (cxuint i = 0; i < kernelsNum; i++)
        kernelsOrder[i] = i;
    std::sort(kernelsOrder.begin(), kernelsOrder.end(),
          [this](const uint32_t& a,const uint32_t& b)
          { return input->kernels[a].kernelName < input->kernels[b].kernelName; });
    
#ifdef HAVE_32BIT
    if (binarySize > UINT32_MAX)
        throw Exception("Binary size is too big!");
#endif
    
    /****
     * write binary to output
     ****/
    cxbyte* binary = new cxbyte[binarySize];
    size_t offset = 0;
    
    try
    {
    uint32_t* data32 = reinterpret_cast<uint32_t*>(binary);
    SULEV(*data32, kernelsNum);
    offset += 4;
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        data32 = reinterpret_cast<uint32_t*>(binary + offset);
        SULEV(data32[0], kernel.kernelName.size());
        offset += 4;
        ::memcpy(binary+offset, kernel.kernelName.c_str(), kernel.kernelName.size());
        offset += kernel.kernelName.size();
        data32 = reinterpret_cast<uint32_t*>(binary + offset);
        offset += 12 + kernel.argInfos.size()*24U;
        SULEV(data32[0], 0);
        if (kernel.offset >= input->codeSize)
            throw Exception("Kernel offset out of range");
        SULEV(data32[1], kernel.offset);
        SULEV(data32[2], kernel.argInfos.size());
        data32 += 3;
        for (const GalliumArgInfo arg: kernel.argInfos)
        {
            SULEV(data32[0], cxuint(arg.type));
            SULEV(data32[1], arg.size);
            SULEV(data32[2], arg.targetSize);
            SULEV(data32[3], arg.targetAlign);
            SULEV(data32[4], arg.signExtended?1:0);
            SULEV(data32[5], cxuint(arg.semantic));
            data32 += 6;
        }
    }
    /* section */
    data32 = reinterpret_cast<uint32_t*>(binary + offset);
    offset += 24U;
    SULEV(data32[0], 1);
    SULEV(data32[1], 0); // resid
    SULEV(data32[2], 0); // section type
    SULEV(data32[3], elfSize); // sizes
    SULEV(data32[4], elfSize+4);
    SULEV(data32[5], elfSize);
    size_t elfOffset = offset;
    
    /****
     * put this ELF binary
     ****/
    Elf32_Ehdr& ehdr = *reinterpret_cast<Elf32_Ehdr*>(binary + offset);
    static const cxbyte elf32Ident[16] = {
            0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2LSB, EV_CURRENT, 
            ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0 };
    ::memcpy(ehdr.e_ident, elf32Ident, 16);
    SULEV(ehdr.e_type, ET_REL);
    SULEV(ehdr.e_machine, 0);
    SULEV(ehdr.e_version, EV_CURRENT);
    SULEV(ehdr.e_entry, 0);
    SULEV(ehdr.e_flags, 0);
    SULEV(ehdr.e_phoff, 0);
    SULEV(ehdr.e_ehsize, sizeof(Elf32_Ehdr));
    SULEV(ehdr.e_phnum, 0);
    SULEV(ehdr.e_phentsize, 0);
    SULEV(ehdr.e_shnum, elfSectionsNum);
    SULEV(ehdr.e_shentsize, sizeof(Elf32_Shdr));
    SULEV(ehdr.e_shstrndx, elfSectionsNum-3);
    ::memset(binary + offset + sizeof(Elf32_Ehdr), 0, 256-sizeof(Elf32_Ehdr));
    offset += 256;
    
    size_t sectionOffsets[7];
    /* text */
    ::memcpy(binary + offset, input->code, input->codeSize);
    offset += input->codeSize;
    fixAlignment(binary, offset, elfOffset);
    sectionOffsets[0] = offset-elfOffset;
    /* .AMDGPU.config */
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        GalliumProgInfoEntry* progInfoEntries =
                    reinterpret_cast<GalliumProgInfoEntry*>(binary + offset);
        for (const GalliumProgInfoEntry& entry: kernel.progInfo)
        {
            SULEV(progInfoEntries->address, entry.address);
            SULEV(progInfoEntries->value, entry.value);
            progInfoEntries++;
        }
        offset += 24U;
    }
    sectionOffsets[1] = offset-elfOffset;
    // .AMDGPU.disasm
    if (disassembly!=nullptr)
    {
        ::memcpy(binary + offset, disassembly, disassemblySize);
        offset += disassemblySize;
    }
    // .rodata
    if (input->globalData!=nullptr)
    {
        fixAlignment(binary, offset, elfOffset);
        sectionOffsets[2] = offset-elfOffset;
        ::memcpy(binary + offset, input->globalData, input->globalDataSize);
        offset += input->globalDataSize;
    }
    else // no global data
        sectionOffsets[2] = offset-elfOffset;
    
    sectionOffsets[3] = offset-elfOffset;
    // .comment
    ::memcpy(binary + offset, comment, commentSize);
    offset += commentSize;
    sectionOffsets[4] = offset-elfOffset;
    // .shstrtab
    ::memcpy(binary + offset, "\000.text\000.data\000.bss\000.AMDGPU.config", 33);
    offset += 33;
    if (disassembly!=nullptr)
    {
        ::memcpy(binary + offset, ".AMDGPU.disasm", 15);
        offset += 15;
    }
    if (input->globalData!=nullptr)
    {
        ::memcpy(binary + offset, ".rodata", 8);
        offset += 8;
    }
    ::memcpy(binary + offset, ".comment\000.note.GNU-stack\000.shstrtab\000"
            ".symtab\000.strtab", 35+16);
    offset += 35+16;
    fixAlignment(binary, offset, elfOffset);
    
    /*
     * put section table
     */
    cxuint sectionName = 1;
    SULEV(ehdr.e_shoff, offset-elfOffset);
    Elf32_Shdr* sectionTable = reinterpret_cast<Elf32_Shdr*>(binary + offset);
    ::memset(sectionTable, 0, sizeof(Elf32_Shdr));
    sectionTable++;
    // .text
    putElfSectionLE(sectionTable++, sectionName, SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR,
            0x100, input->codeSize, 0, 0, 256, 0);
    sectionName += 6;
    // .data
    putElfSectionLE(sectionTable++, sectionName,
            SHT_PROGBITS, SHF_ALLOC|SHF_WRITE, sectionOffsets[0], 0, 0, 0, 4);
    sectionName += 6;
    // .bss
    putElfSectionLE(sectionTable++, sectionName, 8, SHF_ALLOC|SHF_WRITE,
            sectionOffsets[0], 0, 0, 0, 4);
    sectionName += 5;
    // .AMDGPU.config
    putElfSectionLE(sectionTable++, sectionName, SHT_PROGBITS, 0, sectionOffsets[0],
                    input->kernels.size()*24U, 0, 0, 1);
    sectionName += 15;
    if (disassembly!=nullptr) // .AMDGPU.disasm
    {
        putElfSectionLE(sectionTable++, sectionName, SHT_PROGBITS, SHF_ALLOC,
                sectionOffsets[1], disassemblySize, 0, 0, 1);
        sectionName += 15;
    }
    if (input->globalData!=nullptr) // .rodata
    {
        putElfSectionLE(sectionTable++, sectionName, SHT_PROGBITS, SHF_ALLOC,
                sectionOffsets[2], input->globalDataSize, 0, 0, 4);
        sectionName += 8;
    }
    // .comment
    putElfSectionLE(sectionTable++, sectionName, SHT_PROGBITS, SHF_STRINGS|SHF_MERGE,
                    sectionOffsets[3], commentSize, 0, 0, 1, 0, 1);
    sectionName += 9;
    // .note.GNU-stack
    putElfSectionLE(sectionTable++, sectionName, SHT_PROGBITS, 0, sectionOffsets[4],
                    0, 0, 0, 1);
    sectionName += 16;
    // .shstrtab
    putElfSectionLE(sectionTable++, sectionName, SHT_STRTAB, 0, sectionOffsets[4],
                    shstrtabSize, 0, 0, 1);
    sectionName += 10;
    offset += sizeof(Elf32_Shdr)*elfSectionsNum;
    
    sectionOffsets[5] = offset-elfOffset;
    // .symtab
    Elf32_Sym* symTableStart = reinterpret_cast<Elf32_Sym*>(binary + offset);
    Elf32_Sym* symTable = symTableStart;
    ::memset(symTable, 0, sizeof(Elf32_Sym));
    symTable++;
    putElfSymbolLE(symTable++, 1, input->codeSize, 0, 1,
           ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE), 0);
    const cxuint sectSymsNum = 6 + (input->globalData!=nullptr) + (disassembly!=nullptr);
    // local symbols for sections
    for (cxuint i = 0; i < sectSymsNum; i++)
        putElfSymbolLE(symTable++, 0, 0, 0, i+1, ELF32_ST_INFO(STB_LOCAL, STT_SECTION), 0);
    uint32_t kernelNameOffset = 16;
    
    // put kernel symbols
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        putElfSymbolLE(symTable++, kernelNameOffset, kernel.offset, 0, 1,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0);
        kernelNameOffset += kernel.kernelName.size()+1;
    }
    offset += sizeof(Elf32_Sym) * (symTable-symTableStart);
    
    // finish section table
    // .symtab
    putElfSectionLE(sectionTable++, sectionName, SHT_SYMTAB, 0, sectionOffsets[5],
            sizeof(Elf32_Sym)*(symTable-symTableStart), elfSectionsNum-1, 0, 4, 0,
            sizeof(Elf32_Sym));
    sectionName += 8;
    // .strtab
    putElfSectionLE(sectionTable++, sectionName, SHT_STRTAB, 0, offset-elfOffset,
                    strtabSize, 0, 0, 1, 0, 0);
    
    // last section .strtab
    ::memcpy(binary + offset, "\000EndOfTextLabel", 16);
    offset += 16;
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        ::memcpy(binary + offset, kernel.kernelName.c_str(), kernel.kernelName.size()+1);
        offset += kernel.kernelName.size()+1;
    }
    } // try
    catch(...)
    {
        delete[] binary;
        throw;
    }
    assert(offset == binarySize);
    
    outBinarySize = binarySize;
    return binary;
}
