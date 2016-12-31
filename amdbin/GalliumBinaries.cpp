/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/GalliumBinaries.h>

using namespace CLRX;

/* Gallium ELF binary */

GalliumElfBinaryBase::GalliumElfBinaryBase() :
        progInfosNum(0), progInfoEntries(nullptr), disasmSize(0), disasmOffset(0)
{ }

template<typename ElfBinary>
void GalliumElfBinaryBase::loadFromElf(ElfBinary& elfBinary)
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
    {   // set disassembler section
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
    if ((ULEV(shdr.sh_size) % 24U) != 0)
        throw Exception("Wrong size of .AMDGPU.config section!");
    
    const bool hasProgInfoMap = (elfBinary.getCreationFlags() &
                        GALLIUM_ELF_CREATE_PROGINFOMAP) != 0;
    /* check symbols */
    const size_t symbolsNum = elfBinary.getSymbolsNum();
    progInfosNum = 0;
    if (hasProgInfoMap)
        progInfoEntryMap.resize(symbolsNum);
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const auto& sym = elfBinary.getSymbol(i);
        const char* symName = elfBinary.getSymbolName(i);
        if (ULEV(sym.st_shndx) == textIndex && ELF32_ST_BIND(sym.st_info) == STB_GLOBAL)
        {
            if (ULEV(sym.st_value) >= textSize)
                throw Exception("kernel symbol offset out of range");
            if (hasProgInfoMap)
                progInfoEntryMap[progInfosNum] = std::make_pair(symName, 3*progInfosNum);
            progInfosNum++;
        }
    }
    if (progInfosNum*24U != ULEV(shdr.sh_size))
        throw Exception("Number of symbol kernels doesn't match progInfos number!");
    cxbyte* binaryCode = (cxbyte*)elfBinary.getBinaryCode();
    progInfoEntries = reinterpret_cast<GalliumProgInfoEntry*>(binaryCode +
                ULEV(shdr.sh_offset));
    
    if (hasProgInfoMap)
    {
        progInfoEntryMap.resize(progInfosNum);
        mapSort(progInfoEntryMap.begin(), progInfoEntryMap.end(), CStringLess());
    }
}

GalliumElfBinaryBase::~GalliumElfBinaryBase()
{ }

GalliumElfBinary32::GalliumElfBinary32()
{ }

GalliumElfBinary32::~GalliumElfBinary32()
{ }

GalliumElfBinary32::GalliumElfBinary32(size_t binaryCodeSize, cxbyte* binaryCode,
           Flags creationFlags) : ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
       
{
    loadFromElf(static_cast<const ElfBinary32&>(*this));
}

GalliumElfBinary64::GalliumElfBinary64()
{ }

GalliumElfBinary64::~GalliumElfBinary64()
{ }

GalliumElfBinary64::GalliumElfBinary64(size_t binaryCodeSize, cxbyte* binaryCode,
           Flags creationFlags) : ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
       
{
    loadFromElf(static_cast<const ElfBinary64&>(*this));
}

uint32_t GalliumElfBinaryBase::getProgramInfoEntriesNum(uint32_t index) const
{ return 3; }

uint32_t GalliumElfBinaryBase::getProgramInfoEntryIndex(const char* name) const
{
    ProgInfoEntryIndexMap::const_iterator it = binaryMapFind(progInfoEntryMap.begin(),
                         progInfoEntryMap.end(), name, CStringLess());
    if (it == progInfoEntryMap.end())
        throw Exception("Can't find GalliumElf ProgInfoEntry");
    return it->second;
}

const GalliumProgInfoEntry* GalliumElfBinaryBase::getProgramInfo(uint32_t index) const
{
    return progInfoEntries + index*3U;
}

GalliumProgInfoEntry* GalliumElfBinaryBase::getProgramInfo(uint32_t index)
{
    return progInfoEntries + index*3U;
}

/* main GalliumBinary */

template<typename GalliumElfBinary>
static void verifyKernelSymbols(size_t kernelsNum, const GalliumKernel* kernels,
                const GalliumElfBinary& elfBinary)
{
    size_t symIndex = 0;
    const size_t symsNum = elfBinary.getSymbolsNum();
    uint16_t textIndex = elfBinary.getSectionIndex(".text");
    for (uint32_t i = 0; i < kernelsNum; i++)
    {
        const GalliumKernel& kernel = kernels[i];
        for (; symIndex < symsNum; symIndex++)
        {
            const auto& sym = elfBinary.getSymbol(symIndex);
            const char* symName = elfBinary.getSymbolName(symIndex);
            // kernel symol must be defined as global and must be bound to text section
            if (ULEV(sym.st_shndx) == textIndex &&
                ELF32_ST_BIND(sym.st_info) == STB_GLOBAL)
            {   // names must be stored in order
                if (kernel.kernelName != symName)
                    throw Exception("Kernel symbols out of order!");
                if (ULEV(sym.st_value) != kernel.offset)
                    throw Exception("Kernel symbol value and Kernel "
                                "offset doesn't match");
                break;
            }
        }
        if (symIndex >= symsNum)
            throw Exception("Number of kernels in ElfBinary and "
                        "MainBinary doesn't match");
        symIndex++;
    }
}

GalliumBinary::GalliumBinary(size_t _binaryCodeSize, cxbyte* _binaryCode,
                 Flags _creationFlags) : creationFlags(_creationFlags),
         binaryCodeSize(_binaryCodeSize), binaryCode(_binaryCode),
         kernelsNum(0), sectionsNum(0), kernels(nullptr), sections(nullptr),
         elf64BitBinary(false)
{
    if (binaryCodeSize < 4)
        throw Exception("GalliumBinary is too small!!!");
    uint32_t* data32 = reinterpret_cast<uint32_t*>(binaryCode);
    kernelsNum = ULEV(*data32);
    if (binaryCodeSize < uint64_t(kernelsNum)*16U)
        throw Exception("Kernels number is too big!");
    kernels.reset(new GalliumKernel[kernelsNum]);
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
            // accept not known arg type by this CLRadeonExtender
            if (type > 255)
                throw Exception("Type of kernel argument out of handled range");
            argInfo.type = GalliumArgType(type);
            argInfo.size = ULEV(data32[1]);
            argInfo.targetSize = ULEV(data32[2]);
            argInfo.targetAlign = ULEV(data32[3]);
            argInfo.signExtended = ULEV(data32[4])!=0;
            const cxuint semType = ULEV(data32[5]);
            // accept not known semantic type by this CLRadeonExtender
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
    sections.reset(new GalliumSection[sectionsNum]);
    // parse sections and their content
    data32++;
    data += 4;
    
    uint32_t elfSectionId = 0; // initialize warning
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
            if (section.size < sizeof(Elf32_Ehdr))
                throw Exception("Wrong GalliumElfBinary size");
            const Elf32_Ehdr& ehdr = *reinterpret_cast<const Elf32_Ehdr*>(data);
            if (ehdr.e_ident[EI_CLASS] == ELFCLASS32)
            {   // 32-bit
                elfBinary.reset(new GalliumElfBinary32(section.size, data,
                                 creationFlags>>GALLIUM_INNER_SHIFT));
                elf64BitBinary = false;
            }
            else if (ehdr.e_ident[EI_CLASS] == ELFCLASS64)
            {   // 64-bit
                elfSectionId = section.sectionId;
                elfBinary.reset(new GalliumElfBinary64(section.size, data,
                                 creationFlags>>GALLIUM_INNER_SHIFT));
                elf64BitBinary = true;
            }
            else // wrong class
                throw Exception("Wrong GalliumElfBinary class");
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
        throw Exception("Can't find Gallium Kernel Index");
    return it-kernels.get();
}

void GalliumInput::addEmptyKernel(const char* kernelName)
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
    kinput.config.userDataNum = 4;
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
    input = new GalliumInput{ _64bitMode, deviceType, globalDataSize, globalData, kernels,
            codeSize, code, 0, nullptr };
}

GalliumBinGenerator::GalliumBinGenerator(bool _64bitMode, GPUDeviceType deviceType,
        size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        std::vector<GalliumKernelInput>&& kernels)
        : manageable(true), input(nullptr)
{
    input = new GalliumInput{ _64bitMode, deviceType, globalDataSize, globalData,
        std::move(kernels), codeSize, code, 0, nullptr };
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
    7  // GALLIUMSECTID_NOTEGNUSTACK
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
    6  // GALLIUMSECTID_NOTEGNUSTACK
};

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
        const cxuint ldsShift = arch<GPUArchitecture::GCN1_1 ? 8 : 9;
        const uint32_t ldsMask = (1U<<ldsShift)-1U;
        for (uint32_t korder: kernelsOrder)
        {
            const GalliumKernelInput& kernel = input.kernels[korder];
            GalliumProgInfoEntry outEntries[3];
            if (kernel.useConfig)
            {
                const GalliumKernelConfig& config = kernel.config;
                outEntries[0].address = ULEV(0x0000b848U);
                outEntries[1].address = ULEV(0x0000b84cU);
                outEntries[2].address = ULEV(0x0000b860U);
                
                uint32_t scratchBlocks = ((config.scratchBufferSize<<6) + 1023)>>10;
                uint32_t dimValues = 0;
                if (config.dimMask != BINGEN_DEFAULT)
                    dimValues = ((config.dimMask&7)<<7) |
                            (((config.dimMask&4) ? 2 : (config.dimMask&2) ? 1 : 0)<<11);
                else
                    dimValues |= (config.pgmRSRC2 & 0x1b80U);
                cxuint sgprsNum = std::max(config.usedSGPRsNum, 1U);
                cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
                /// pgmRSRC1
                outEntries[0].value = (config.pgmRSRC1) | ((vgprsNum-1)>>2) |
                        (((sgprsNum-1)>>3)<<6) | ((uint32_t(config.floatMode)&0xff)<<12) |
                        (config.ieeeMode?1U<<23:0) | (uint32_t(config.priority&3)<<10) |
                        (config.privilegedMode?1U<<20:0) | (config.dx10Clamp?1U<<21:0) |
                        (config.debugMode?1U<<22:0);
                
                outEntries[1].value = (config.pgmRSRC2 & 0xffffe440U) |
                        (config.userDataNum<<1) | ((config.tgSize) ? 0x400 : 0) |
                        ((config.scratchBufferSize)?1:0) | dimValues |
                        (((config.localSize+ldsMask)>>ldsShift)<<15) |
                        ((uint32_t(config.exceptions)&0x7f)<<24);
                outEntries[2].value = (scratchBlocks)<<12;
                for (cxuint k = 0; k < 3; k++)
                    outEntries[k].value = ULEV(outEntries[k].value);
            }
            else
                for (cxuint k = 0; k < 3; k++)
                {
                    outEntries[k].address = ULEV(kernel.progInfo[k].address);
                    outEntries[k].value = ULEV(kernel.progInfo[k].value);
                }
            fob.writeArray(3, outEntries);
        }
    }
};

template<typename Types>
static void putSectionsAndSymbols(ElfBinaryGenTemplate<Types>& elfBinGen,
      const GalliumInput* input, const Array<uint32_t>& kernelsOrder,
      const AmdGpuConfigContent& amdGpuConfigContent)
{
    const char* comment = "CLRX GalliumBinGenerator " CLRX_VERSION;
    uint32_t commentSize = ::strlen(comment);
    
    typedef ElfRegionTemplate<Types> ElfRegion;
    typedef ElfSymbolTemplate<Types> ElfSymbol;
    const uint32_t kernelsNum = input->kernels.size();
    elfBinGen.addRegion(ElfRegion(input->codeSize, input->code, 256, ".text",
            SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR));
    elfBinGen.addRegion(ElfRegion(0, (const cxbyte*)nullptr, 4, ".data",
            SHT_PROGBITS, SHF_ALLOC|SHF_WRITE));
    elfBinGen.addRegion(ElfRegion(0, (const cxbyte*)nullptr, 4, ".bss",
            SHT_NOBITS, SHF_ALLOC|SHF_WRITE));
    // write configuration for kernel execution
    elfBinGen.addRegion(ElfRegion(uint64_t(24U)*kernelsNum, &amdGpuConfigContent, 1,
            ".AMDGPU.config", SHT_PROGBITS, 0));
    
    if (input->globalData!=nullptr)
        elfBinGen.addRegion(ElfRegion(input->globalDataSize, input->globalData, 4,
                ".rodata", SHT_PROGBITS, SHF_ALLOC));
    
    if (input->comment!=nullptr)
    {   // if comment, store comment section
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
    elfBinGen.addRegion(ElfRegion::symtabSection());
    elfBinGen.addRegion(ElfRegion::strtabSection());
    /* symbols */
    /// EndOfTextLabel - ?? always is at end of symbol table
    elfBinGen.addSymbol({"EndOfTextLabel", 1, ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE),
            0, false, uint32_t(input->codeSize), 0});
    const cxuint sectSymsNum = 6 + (input->globalData!=nullptr);
    // local symbols for sections
    for (cxuint i = 0; i < sectSymsNum; i++)
        elfBinGen.addSymbol({"", uint16_t(i+1), ELF32_ST_INFO(STB_LOCAL, STT_SECTION),
                0, false, 0, 0});
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        elfBinGen.addSymbol({kernel.kernelName.c_str(), 1,
                ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 0, false, kernel.offset, 0});
    }
    
    /* choose builtin section table and extraSectionStartIndex */
    const uint16_t* curMainBuiltinSections = (input->globalData!=nullptr) ?
            mainBuiltinSectionTable : mainBuiltinSectionTable2;
    cxuint startSectionIndex = (input->globalData!=nullptr) ? 11 : 10;
    
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
            const GalliumKernelConfig& config = kernel.config;
            if (config.usedVGPRsNum > maxVGPRSNum)
                throw Exception("Used VGPRs number out of range");
            if (config.usedSGPRsNum > maxSGPRSNum)
                throw Exception("Used SGPRs number out of range");
            if (config.localSize > 32768)
                throw Exception("LocalSize out of range");
            if (config.priority >= 4)
                throw Exception("Priority out of range");
            if (config.userDataNum > 16)
                throw Exception("UserDataNum out of range");
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
    if (!input->is64BitElf)
    {   /* 32-bit ELF */
        elfBinGen32.reset(new ElfBinaryGen32({ 0, 0, ELFOSABI_SYSV, 0,  ET_REL, 0,
                    EV_CURRENT, UINT_MAX, 0, 0 }));
        putSectionsAndSymbols(*elfBinGen32, input, kernelsOrder, amdGpuConfigContent);
        elfSize = elfBinGen32->countSize();
    }
    else
    {   /* 64-bit ELF */
        elfBinGen64.reset(new ElfBinaryGen64({ 0, 0, ELFOSABI_SYSV, 0,  ET_REL, 0,
                    EV_CURRENT, UINT_MAX, 0, 0 }));
        putSectionsAndSymbols(*elfBinGen64, input, kernelsOrder, amdGpuConfigContent);
        elfSize = elfBinGen64->countSize();
    }

    binarySize += elfSize;
    
    if (
#ifdef HAVE_64BIT
        !input->is64BitElf &&
#endif
        elfSize > UINT32_MAX)
        throw Exception("Elf binary size is too big!");
    
#ifdef HAVE_32BIT
    if (binarySize > UINT32_MAX)
        throw Exception("Binary size is too big!");
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
    for (uint32_t korder: kernelsOrder)
    {
        const GalliumKernelInput& kernel = input->kernels[korder];
        if (kernel.offset >= input->codeSize)
            throw Exception("Kernel offset out of range");
        
        bos.writeObject<uint32_t>(LEV(uint32_t(kernel.kernelName.size())));
        bos.writeArray(kernel.kernelName.size(), kernel.kernelName.c_str());
        const uint32_t other[3] = { 0, LEV(kernel.offset),
            LEV(cxuint(kernel.argInfos.size())) };
        bos.writeArray(3, other);
        
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
        const uint32_t section[6] = { LEV(1U), LEV(0U), LEV(0U), LEV(uint32_t(elfSize)),
            LEV(uint32_t(elfSize+4)), LEV(uint32_t(elfSize)) };
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
