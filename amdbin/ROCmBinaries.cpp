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
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <memory>
#include <unordered_set>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/ROCmBinaries.h>

using namespace CLRX;


/*
 * ROCm binary reader and generator
 */

/* TODO: add support for various kernel code offset (now only 256 is supported) */

ROCmBinary::ROCmBinary(size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags)
        : ElfBinary64(binaryCodeSize, binaryCode, creationFlags),
          regionsNum(0), codeSize(0), code(nullptr),
          globalDataSize(0), globalData(nullptr), metadataSize(0), metadata(nullptr),
          newBinFormat(false), llvm10BinFormat(false), metadataV3Format(false)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    uint64_t codeOffset = 0;
    // find '.text' section
    if (textIndex!=SHN_UNDEF)
    {
        code = getSectionContent(textIndex);
        const Elf64_Shdr& textShdr = getSectionHeader(textIndex);
        codeSize = ULEV(textShdr.sh_size);
        codeOffset = ULEV(textShdr.sh_offset);
    }
    
    if (getHeader().e_ident[EI_ABIVERSION] == 1)
        llvm10BinFormat = true; // likely llvm10 bin format
    
    cxuint rodataIndex = SHN_UNDEF;
    const cxbyte* rodataContent = nullptr;
    try
    { rodataIndex = getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { } // ignore failed
    // find '.text' section
    if (rodataIndex!=SHN_UNDEF)
    {
        rodataContent = globalData = getSectionContent(rodataIndex);
        const Elf64_Shdr& rodataShdr = getSectionHeader(rodataIndex);
        globalDataSize = ULEV(rodataShdr.sh_size);
    }
    
    cxuint gpuConfigIndex = SHN_UNDEF;
    try
    { gpuConfigIndex = getSectionIndex(".AMDGPU.config"); }
    catch(const Exception& ex)
    { } // ignore failed
    newBinFormat = (gpuConfigIndex == SHN_UNDEF);
    
    cxuint relaDynIndex = SHN_UNDEF;
    try
    { relaDynIndex = getSectionIndex(".rela.dyn"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    cxuint gotIndex = SHN_UNDEF;
    try
    { gotIndex = getSectionIndex(".got"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    // counts regions (symbol or kernel)
    std::vector<std::pair<CString, size_t> > tmpKernelDescs;
    regionsNum = 0;
    const size_t symbolsNum = getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        // count regions number
        const Elf64_Sym& sym = getSymbol(i);
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        Elf64_Half shndx = ULEV(sym.st_shndx);
        if (shndx==textIndex &&
            (symType==STT_GNU_IFUNC ||
                (symType==STT_FUNC && (!newBinFormat || llvm10BinFormat)) ||
                (bind==STB_GLOBAL && symType==STT_OBJECT)))
            regionsNum++;
        if (llvm10BinFormat && shndx==rodataIndex && symType==STT_OBJECT)
        {
            const char* symName = getSymbolName(i);
            size_t symNameLen = ::strlen(symName);
            // if symname have '.kd' at end
            if (symNameLen > 3 && symName[symNameLen-3]=='.' &&
                symName[symNameLen-2]=='k' && symName[symNameLen-1]=='d')
                tmpKernelDescs.push_back({ CString(symName, symName+symNameLen-3),
                            ULEV(sym.st_value) });
        }
    }
    if (llvm10BinFormat)
    {
        if (rodataContent==nullptr)
            throw BinException("No rodata section in ROCm LLVM10Bin format");
        mapSort(tmpKernelDescs.begin(), tmpKernelDescs.end());
        kernelDescs.resize(regionsNum);
        std::fill(kernelDescs.begin(), kernelDescs.end(), nullptr);
    }
    
    if (code==nullptr && regionsNum!=0)
        throw BinException("No code if regions number is not zero");
    regions.reset(new ROCmRegion[regionsNum]);
    size_t j = 0;
    typedef std::pair<uint64_t, size_t> RegionOffsetEntry;
    std::unique_ptr<RegionOffsetEntry[]> symOffsets(new RegionOffsetEntry[regionsNum]);
    
    // get regions info
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const Elf64_Sym& sym = getSymbol(i);
        if (ULEV(sym.st_shndx)!=textIndex)
            continue;   // if not in '.text' section
        const size_t value = ULEV(sym.st_value);
        if (value < codeOffset)
            throw BinException("Region offset is too small!");
        const size_t size = ULEV(sym.st_size);
        
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        if (symType==STT_GNU_IFUNC || symType==STT_FUNC ||
                (bind==STB_GLOBAL && symType==STT_OBJECT))
        {
            ROCmRegionType type = ROCmRegionType::DATA;
            // if kernel
            if (symType==STT_GNU_IFUNC) 
                type = ROCmRegionType::KERNEL;
            // if function kernel
            else if (symType==STT_FUNC)
            {
                if (newBinFormat && !llvm10BinFormat)
                    continue;
                type = llvm10BinFormat ? ROCmRegionType::KERNEL : ROCmRegionType::FKERNEL;
            }
            symOffsets[j] = std::make_pair(value, j);
            if (type!=ROCmRegionType::DATA && value+0x100 > codeOffset+codeSize)
                throw BinException("Kernel or code offset is too big!");
            const char* symName = getSymbolName(i);
            regions[j] = { symName, size, value, type };
            if (llvm10BinFormat)
            {
                auto it = binaryMapFind(tmpKernelDescs.begin(), tmpKernelDescs.end(),
                                        CString(symName));
                if (it != tmpKernelDescs.end())
                    kernelDescs[j] = reinterpret_cast<const ROCmKernelDescriptor*>(
                                binaryCode + it->second);
                else
                    kernelDescs[j] = nullptr;
            }
            j++;
        }
    }
    // sort regions by offset
    std::sort(symOffsets.get(), symOffsets.get()+regionsNum,
            [](const RegionOffsetEntry& a, const RegionOffsetEntry& b)
            { return a.first < b.first; });
    // checking distance between regions
    for (size_t i = 1; i <= regionsNum; i++)
    {
        size_t end = (i<regionsNum) ? symOffsets[i].first : codeOffset+codeSize;
        ROCmRegion& region = regions[symOffsets[i-1].second];
        if (!llvm10BinFormat)
            if (region.type==ROCmRegionType::KERNEL && symOffsets[i-1].first+0x100 > end)
                throw BinException("Kernel size is too small!");
        
        const size_t regSize = end - symOffsets[i-1].first;
        if (region.size==0)
            region.size = regSize;
        else
            region.size = std::min(regSize, region.size);
    }
    
    // load got symbols
    if (relaDynIndex != SHN_UNDEF && gotIndex != SHN_UNDEF)
    {
        const Elf64_Shdr& relaShdr = getSectionHeader(relaDynIndex);
        const Elf64_Shdr& gotShdr = getSectionHeader(gotIndex);
        
        size_t relaEntrySize = ULEV(relaShdr.sh_entsize);
        if (relaEntrySize==0)
            relaEntrySize = sizeof(Elf64_Rela);
        const size_t relaEntriesNum = ULEV(relaShdr.sh_size)/relaEntrySize;
        const size_t gotEntriesNum = ULEV(gotShdr.sh_size) >> 3;
        if (gotEntriesNum != relaEntriesNum)
            throw BinException("RelaDyn entries number and GOT entries "
                        "number doesn't match!");
        
        // initialize GOT symbols table
        gotSymbols.resize(gotEntriesNum);
        const cxbyte* relaDyn = getSectionContent(relaDynIndex);
        for (size_t i = 0; i < relaEntriesNum; i++)
        {
            const Elf64_Rela& rela = *reinterpret_cast<const Elf64_Rela*>(
                            relaDyn + relaEntrySize*i);
            // check rela entry fields
            if (ULEV(rela.r_offset) != ULEV(gotShdr.sh_offset) + i*8)
                throw BinException("Wrong dyn relocation offset");
            if (ULEV(rela.r_addend) != 0ULL)
                throw BinException("Wrong dyn relocation addend");
            size_t symIndex = ELF64_R_SYM(ULEV(rela.r_info));
            if (symIndex >= getDynSymbolsNum())
                throw BinException("Dyn relocation symbol index out of range");
            // just set in gotSymbols
            gotSymbols[i] = symIndex;
        }
    }
    
    // get metadata
    const size_t notesSize = getNotesSize();
    const cxbyte* noteContent = (const cxbyte*)getNotes();
    
    for (size_t offset = 0; offset < notesSize; )
    {
        const Elf64_Nhdr* nhdr = (const Elf64_Nhdr*)(noteContent + offset);
        size_t namesz = ULEV(nhdr->n_namesz);
        size_t descsz = ULEV(nhdr->n_descsz);
        if (usumGt(offset, namesz+descsz, notesSize))
            throw BinException("Note offset+size out of range");
        
        const size_t alignedNamesz = ((namesz+3)&~size_t(3));
        if ((namesz==4 &&
            ::strcmp((const char*)noteContent+offset+ sizeof(Elf64_Nhdr), "AMD")==0) ||
            (namesz==7 &&
            ::strcmp((const char*)noteContent+offset+ sizeof(Elf64_Nhdr), "AMDGPU")==0))
        {
            const uint32_t noteType = ULEV(nhdr->n_type);
            if ((noteType == 0xa && namesz==4) || (noteType == 0x20 && namesz==7))
            {
                if (namesz==7)
                    metadataV3Format = true;
                if (namesz==4 && metadataV3Format)
                    throw Exception("MetadataV2 in MetadataV3 compliant binary!");
                metadata = (char*)(noteContent+offset+sizeof(Elf64_Nhdr) + alignedNamesz);
                metadataSize = descsz;
            }
            else if (noteType == 0xb && namesz==4)
                target.assign((char*)(noteContent+offset+sizeof(Elf64_Nhdr) +
                                        alignedNamesz), descsz);
        }
        size_t align = (((alignedNamesz+descsz)&3)!=0) ? 4-((alignedNamesz+descsz)&3) : 0;
        offset += sizeof(Elf64_Nhdr) + alignedNamesz + descsz + align;
    }
    
    if (hasRegionMap())
    {
        // create region map
        regionsMap.resize(regionsNum);
        for (size_t i = 0; i < regionsNum; i++)
            regionsMap[i] = std::make_pair(regions[i].regionName, i);
        // sort region map
        mapSort(regionsMap.begin(), regionsMap.end());
    }
    
    if ((creationFlags & ROCMBIN_CREATE_METADATAINFO) != 0 &&
        metadata != nullptr && metadataSize != 0)
    {
        metadataInfo.reset(new ROCmMetadata());
        if (!metadataV3Format)
            parseROCmMetadata(metadataSize, metadata, *metadataInfo);
        else
            parseROCmMetadataMsgPack(metadataSize,
                    reinterpret_cast<const cxbyte*>(metadata), *metadataInfo);
        
        if (hasKernelInfoMap())
        {
            const std::vector<ROCmKernelMetadata>& kernels = metadataInfo->kernels;
            kernelInfosMap.resize(kernels.size());
            for (size_t i = 0; i < kernelInfosMap.size(); i++)
                kernelInfosMap[i] = std::make_pair(kernels[i].name, i);
            // sort region map
            mapSort(kernelInfosMap.begin(), kernelInfosMap.end());
        }
    }
}

/// determint GPU device from ROCm notes
GPUDeviceType ROCmBinary::determineGPUDeviceType(uint32_t& outArchMinor,
                     uint32_t& outArchStepping) const
{
    uint32_t archMajor = 0;
    uint32_t archMinor = 0;
    uint32_t archStepping = 0;
    
    {
        const cxbyte* noteContent = (const cxbyte*)getNotes();
        if (noteContent==nullptr)
            throw BinException("Missing notes in inner binary!");
        size_t notesSize = getNotesSize();
        // find note about AMDGPU
        for (size_t offset = 0; offset < notesSize; )
        {
            const Elf64_Nhdr* nhdr = (const Elf64_Nhdr*)(noteContent + offset);
            size_t namesz = ULEV(nhdr->n_namesz);
            size_t descsz = ULEV(nhdr->n_descsz);
            if (usumGt(offset, namesz+descsz, notesSize))
                throw BinException("Note offset+size out of range");
            if (ULEV(nhdr->n_type) == 0x3 && namesz==4 && descsz>=0x1a &&
                ::strcmp((const char*)noteContent+offset+sizeof(Elf64_Nhdr), "AMD")==0)
            {    // AMDGPU type
                const uint32_t* content = (const uint32_t*)
                        (noteContent+offset+sizeof(Elf64_Nhdr) + 4);
                archMajor = ULEV(content[1]);
                archMinor = ULEV(content[2]);
                archStepping = ULEV(content[3]);
            }
            size_t align = (((namesz+descsz)&3)!=0) ? 4-((namesz+descsz)&3) : 0;
            offset += sizeof(Elf64_Nhdr) + namesz + descsz + align;
        }
    }
    if (llvm10BinFormat && archMajor==0 && archMajor==0 && archStepping==0)
    {
        // default is Navi
        outArchMinor = 1;
        outArchStepping = 0;
        return GPUDeviceType::GFX1010;
    }
    // determine device type
    GPUDeviceType deviceType = getGPUDeviceTypeFromArchVersion(archMajor, archMinor,
                                    archStepping);
    outArchMinor = archMinor;
    outArchStepping = archStepping;
    return deviceType;
}

const ROCmRegion& ROCmBinary::getRegion(const char* name) const
{
    RegionMap::const_iterator it = binaryMapFind(regionsMap.begin(),
                             regionsMap.end(), name);
    if (it == regionsMap.end())
        throw BinException("Can't find region name");
    return regions[it->second];
}

const ROCmKernelMetadata& ROCmBinary::getKernelInfo(const char* name) const
{
    if (!hasMetadataInfo())
        throw BinException("Can't find kernel info name");
    RegionMap::const_iterator it = binaryMapFind(kernelInfosMap.begin(),
                             kernelInfosMap.end(), name);
    if (it == kernelInfosMap.end())
        throw BinException("Can't find kernel info name");
    return metadataInfo->kernels[it->second];
}

const ROCmKernelDescriptor* ROCmBinary::getKernelDescriptor(const char* name) const
{
    RegionMap::const_iterator it = binaryMapFind(regionsMap.begin(),
                             regionsMap.end(), name);
    if (it == regionsMap.end())
        throw BinException("Can't find kernel descriptor name");
    return kernelDescs[it->second];
}

// if ROCm binary
bool CLRX::isROCmBinary(size_t binarySize, const cxbyte* binary)
{
    if (!isElfBinary(binarySize, binary))
        return false;
    if (binary[EI_CLASS] != ELFCLASS64)
        return false;
    const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binary);
    if (ULEV(ehdr->e_machine) != 0xe0)
        return false;
    return true;
}


void ROCmInput::addEmptyKernel(const char* kernelName)
{
    symbols.push_back({ kernelName, 0, 0, ROCmRegionType::KERNEL });
}

/* ROCm section generators */

class CLRX_INTERNAL ROCmGotGen: public ElfRegionContent
{
private:
    const ROCmInput* input;
public:
    explicit ROCmGotGen(const ROCmInput* _input) : input(_input)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        fob.fill(input->gotSymbols.size()*8, 0);
    }
};

class CLRX_INTERNAL ROCmRelaDynGen: public ElfRegionContent
{
private:
    size_t gotOffset;
    const ROCmInput* input;
public:
    explicit ROCmRelaDynGen(const ROCmInput* _input) : gotOffset(0), input(_input)
    { }
    
    void setGotOffset(size_t _gotOffset)
    { gotOffset = _gotOffset; }
    
    void operator()(FastOutputBuffer& fob) const
    {
        for (size_t i = 0; i < input->gotSymbols.size(); i++)
        {
            size_t symIndex = input->gotSymbols[i];
            Elf64_Rela rela{};
            SLEV(rela.r_offset, gotOffset + 8*i);
            SLEV(rela.r_info, ELF64_R_INFO(symIndex + 1, 3));
            rela.r_addend = 0;
            fob.writeObject(rela);
        }
    }
};

/*
 * ROCm Binary Generator
 */

ROCmBinGenerator::ROCmBinGenerator() : manageable(false), input(nullptr)
{ }

ROCmBinGenerator::ROCmBinGenerator(const ROCmInput* rocmInput)
        : manageable(false), input(rocmInput), rocmGotGen(nullptr), rocmRelaDynGen(nullptr)
{ }

ROCmBinGenerator::ROCmBinGenerator(GPUDeviceType deviceType,
        uint32_t archMinor, uint32_t archStepping, size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        const std::vector<ROCmSymbolInput>& symbols) :
        rocmGotGen(nullptr), rocmRelaDynGen(nullptr)
{
    std::unique_ptr<ROCmInput> _input(new ROCmInput{});
    _input->deviceType = deviceType;
    _input->archMinor = archMinor;
    _input->archStepping = archStepping;
    _input->eflags = 0;
    _input->newBinFormat = false;
    _input->llvm10BinFormat = false;
    _input->metadataV3Format = false;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->symbols = symbols;
    _input->codeSize = codeSize;
    _input->code = code;
    _input->commentSize = 0;
    _input->comment = nullptr;
    _input->target = "";
    _input->targetTripple = "";
    _input->metadataSize = 0;
    _input->metadata = nullptr;
    _input->useMetadataInfo = false;
    _input->metadataInfo = ROCmMetadata{};
    input = _input.release();
}

ROCmBinGenerator::ROCmBinGenerator(GPUDeviceType deviceType,
        uint32_t archMinor, uint32_t archStepping, size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        std::vector<ROCmSymbolInput>&& symbols) :
        rocmGotGen(nullptr), rocmRelaDynGen(nullptr)
{
    std::unique_ptr<ROCmInput> _input(new ROCmInput{});
    _input->deviceType = deviceType;
    _input->archMinor = archMinor;
    _input->archStepping = archStepping;
    _input->eflags = 0;
    _input->newBinFormat = false;
    _input->llvm10BinFormat = false;
    _input->metadataV3Format = false;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->symbols = std::move(symbols);
    _input->codeSize = codeSize;
    _input->code = code;
    _input->commentSize = 0;
    _input->comment = nullptr;
    _input->target = "";
    _input->targetTripple = "";
    _input->metadataSize = 0;
    _input->metadata = nullptr;
    _input->useMetadataInfo = false;
    _input->metadataInfo = ROCmMetadata{};
    input = _input.release();
}

ROCmBinGenerator::~ROCmBinGenerator()
{
    if (manageable)
        delete input;
    if (rocmGotGen!=nullptr)
        delete (ROCmGotGen*)rocmGotGen;
    if (rocmRelaDynGen!=nullptr)
        delete (ROCmRelaDynGen*)rocmRelaDynGen;
}

void ROCmBinGenerator::setInput(const ROCmInput* input)
{
    if (manageable)
        delete input;
    manageable = false;
    this->input = input;
}

// ELF notes contents
static const cxbyte noteDescType1[8] =
{ 2, 0, 0, 0, 1, 0, 0, 0 };

static const cxbyte noteDescType3[27] =
{ 4, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  'A', 'M', 'D', 0, 'A', 'M', 'D', 'G', 'P', 'U', 0 };

static inline void addMainSectionToTable(cxuint& sectionsNum, uint16_t* builtinTable,
                cxuint elfSectId)
{ builtinTable[elfSectId - ELFSECTID_START] = sectionsNum++; }

void ROCmBinGenerator::prepareBinaryGen()
{
    if (input->globalData == nullptr && (input->llvm10BinFormat || input->metadataV3Format))
        throw BinGenException("LLVM10 binary format requires global data");
    
    AMDGPUArchVersion amdGpuArchValues = getGPUArchVersion(input->deviceType,
                GPUArchVersionTable::ROCM);
    if (input->archMinor!=UINT32_MAX)
        amdGpuArchValues.minor = input->archMinor;
    if (input->archStepping!=UINT32_MAX)
        amdGpuArchValues.stepping = input->archStepping;
    
    comment = "CLRX ROCmBinGenerator " CLRX_VERSION;
    commentSize = ::strlen(comment);
    if (input->comment!=nullptr)
    {
        // if comment, store comment section
        comment = input->comment;
        commentSize = input->commentSize;
        if (commentSize==0)
            commentSize = ::strlen(comment);
    }
    
    uint32_t eflags = input->newBinFormat ? ((input->llvm10BinFormat) ? 51 : 2) : 0;
    if (input->eflags != BINGEN_DEFAULT)
        eflags = input->eflags;
    
    std::fill(mainBuiltinSectTable,
              mainBuiltinSectTable + ROCMSECTID_MAX-ELFSECTID_START+1, SHN_UNDEF);
    mainSectionsNum = 1;
    
    // generate main builtin section table (for section id translation)
    if (input->newBinFormat)
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_NOTE);
    if (!input->llvm10BinFormat)
        if (input->globalData != nullptr)
            addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_RODATA);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_DYNSYM);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_HASH);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_DYNSTR);
    if (input->llvm10BinFormat)
        if (input->globalData != nullptr)
            addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_RODATA);
    if (!input->gotSymbols.empty())
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_RELADYN);
    const cxuint execProgHeaderRegionIndex = mainSectionsNum;
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_TEXT);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_DYNAMIC);
    if (!input->gotSymbols.empty())
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_GOT);
    if (!input->newBinFormat)
    {
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_NOTE);
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_GPUCONFIG);
    }
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_COMMENT);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_SYMTAB);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_SHSTRTAB);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_STRTAB);
    
    const cxbyte abiVer = (input->newBinFormat && input->llvm10BinFormat) ? 1 : 0;
    
    elfBinGen64.reset(new ElfBinaryGen64({ 0U, 0U, 0x40, abiVer, ET_DYN, 0xe0, EV_CURRENT,
            cxuint(input->newBinFormat ? execProgHeaderRegionIndex : UINT_MAX), 0, eflags },
            true, true, true, PHREGION_FILESTART));
    
    static const int32_t dynTags[] = {
        DT_SYMTAB, DT_SYMENT, DT_STRTAB, DT_STRSZ, DT_HASH };
    elfBinGen64->addDynamics(sizeof(dynTags)/sizeof(int32_t), dynTags);
    
    // elf program headers
    elfBinGen64->addProgramHeader({ PT_PHDR, PF_R, 0, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64->addProgramHeader({ PT_LOAD, PF_R, PHREGION_FILESTART,
                    execProgHeaderRegionIndex,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 0x1000 });
    elfBinGen64->addProgramHeader({ PT_LOAD, PF_R|PF_X, execProgHeaderRegionIndex, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64->addProgramHeader({ PT_LOAD, PF_R|PF_W, execProgHeaderRegionIndex+1,
                    cxuint(1 + (!input->gotSymbols.empty())),
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64->addProgramHeader({ PT_DYNAMIC, PF_R|PF_W, execProgHeaderRegionIndex+1, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 8 });
    elfBinGen64->addProgramHeader({ PT_GNU_RELRO, PF_R, execProgHeaderRegionIndex+1,
                    cxuint(1 + (!input->gotSymbols.empty())),
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 1 });
    elfBinGen64->addProgramHeader({ PT_GNU_STACK, PF_R|PF_W, PHREGION_FILESTART, 0,
                    true, 0, 0, 0 });
    
    if (input->newBinFormat)
        // program header for note (new binary format)
        elfBinGen64->addProgramHeader({ PT_NOTE, PF_R, 1, 1, true,
                    Elf64Types::nobase, Elf64Types::nobase, 0, 4 });
    
    if (!input->llvm10BinFormat)
    {
        target = input->target.c_str();
        if (target.empty() && !input->targetTripple.empty())
        {
            target = input->targetTripple.c_str();
            char dbuf[20];
            snprintf(dbuf, 20, "-gfx%u%u%u", amdGpuArchValues.major, amdGpuArchValues.minor,
                    amdGpuArchValues.stepping);
            target += dbuf;
        }
        // elf notes
        elfBinGen64->addNote({"AMD", sizeof noteDescType1, noteDescType1, 1U});
        noteBuf.reset(new cxbyte[0x1b]);
        ::memcpy(noteBuf.get(), noteDescType3, 0x1b);
        SULEV(*(uint32_t*)(noteBuf.get()+4), amdGpuArchValues.major);
        SULEV(*(uint32_t*)(noteBuf.get()+8), amdGpuArchValues.minor);
        SULEV(*(uint32_t*)(noteBuf.get()+12), amdGpuArchValues.stepping);
        elfBinGen64->addNote({"AMD", 0x1b, noteBuf.get(), 3U});
        if (!target.empty())
            elfBinGen64->addNote({"AMD", target.size(), (const cxbyte*)target.c_str(), 0xbU});
    }
    
    metadataSize = input->metadataSize;
    metadata = input->metadata;
    if (input->useMetadataInfo)
    {
        // generate ROCm metadata
        std::vector<std::pair<CString, size_t> > symbolIndices(input->symbols.size());
        // create sorted indices of symbols by its name
        for (size_t k = 0; k < input->symbols.size(); k++)
            symbolIndices[k] = std::make_pair(input->symbols[k].symbolName, k);
        mapSort(symbolIndices.begin(), symbolIndices.end());
        
        const size_t mdKernelsNum = input->metadataInfo.kernels.size();
        if (!input->metadataV3Format)
        {
            std::unique_ptr<const ROCmKernelConfig*[]> kernelConfigPtrs(
                    new const ROCmKernelConfig*[mdKernelsNum]);
            // generate ROCm kernel config pointers
            for (size_t k = 0; k < mdKernelsNum; k++)
            {
                auto it = binaryMapFind(symbolIndices.begin(), symbolIndices.end(),
                            input->metadataInfo.kernels[k].name);
                if (it == symbolIndices.end() ||
                    (input->symbols[it->second].type != ROCmRegionType::FKERNEL &&
                    input->symbols[it->second].type != ROCmRegionType::KERNEL))
                    throw BinGenException("Kernel in metadata doesn't exists in code");
                kernelConfigPtrs[k] = reinterpret_cast<const ROCmKernelConfig*>(
                            input->code + input->symbols[it->second].offset);
            }
            // just generate ROCm metadata from info
            generateROCmMetadata(input->metadataInfo, kernelConfigPtrs.get(), metadataStr);
            metadataSize = metadataStr.size();
            metadata = metadataStr.c_str();
        }
        else
        {
            std::unique_ptr<const ROCmKernelDescriptor*[]> kernelDescPtrs(
                    new const ROCmKernelDescriptor*[mdKernelsNum]);
            for (size_t k = 0; k < mdKernelsNum; k++)
                kernelDescPtrs[k] = reinterpret_cast<const ROCmKernelDescriptor*>(
                            input->globalData + k*sizeof(ROCmKernelDescriptor));
            // just generate ROCm metadata from info
            generateROCmMetadataMsgPack(input->metadataInfo,
                                kernelDescPtrs.get(), metadataBytes);
            metadataSize = metadataBytes.size();
            metadata = (const char*)metadataBytes.data();
        }
    }
    
    if (metadataSize != 0)
    {
        if (!input->metadataV3Format)
            elfBinGen64->addNote({"AMD", metadataSize, (const cxbyte*)metadata, 0xaU});
        else
            elfBinGen64->addNote({"AMDGPU", metadataSize, (const cxbyte*)metadata, 0x20U});
    }
    
    /// region and sections
    elfBinGen64->addRegion(ElfRegion64::programHeaderTable());
    if (input->newBinFormat)
        elfBinGen64->addRegion(ElfRegion64::noteSection());
    if (!input->llvm10BinFormat)
        if (input->globalData != nullptr)
            elfBinGen64->addRegion(ElfRegion64(input->globalDataSize, input->globalData, 4,
                    ".rodata", SHT_PROGBITS, SHF_ALLOC, 0, 0, Elf64Types::nobase));
    
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".dynsym", SHT_DYNSYM, SHF_ALLOC, 0, BINGEN_DEFAULT, Elf64Types::nobase));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 4,
                ".hash", SHT_HASH, SHF_ALLOC,
                mainBuiltinSectTable[ELFSECTID_DYNSYM-ELFSECTID_START], 0,
                Elf64Types::nobase));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".dynstr", SHT_STRTAB,
                SHF_ALLOC, 0, 0, Elf64Types::nobase));
    if (input->llvm10BinFormat)
        if (input->globalData != nullptr)
            elfBinGen64->addRegion(ElfRegion64(input->globalDataSize, input->globalData, 64,
                    ".rodata", SHT_PROGBITS, SHF_ALLOC, 0, 0, Elf64Types::nobase));
    if (!input->gotSymbols.empty())
    {
        ROCmRelaDynGen* sgen = new ROCmRelaDynGen(input);
        rocmRelaDynGen = (void*)sgen;
        elfBinGen64->addRegion(ElfRegion64(input->gotSymbols.size()*sizeof(Elf64_Rela),
                sgen, 8, ".rela.dyn", SHT_RELA, SHF_ALLOC,
                mainBuiltinSectTable[ELFSECTID_DYNSYM-ELFSECTID_START], 0,
                Elf64Types::nobase, sizeof(Elf64_Rela)));
    }
    // '.text' with alignment=4096
    elfBinGen64->addRegion(ElfRegion64(input->codeSize, (const cxbyte*)input->code, 
              0x1000, ".text", SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 0, 0,
              Elf64Types::nobase, 0, false, 256));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 0x1000,
                ".dynamic", SHT_DYNAMIC, SHF_ALLOC|SHF_WRITE,
                mainBuiltinSectTable[ELFSECTID_DYNSTR-ELFSECTID_START], 0,
                Elf64Types::nobase, 0, false, 8));
    if (!input->gotSymbols.empty())
    {
        ROCmGotGen* sgen = new ROCmGotGen(input);
        rocmGotGen = (void*)sgen;
        elfBinGen64->addRegion(ElfRegion64(input->gotSymbols.size()*8, sgen,
                8, ".got", SHT_PROGBITS,
                SHF_ALLOC|SHF_WRITE, 0, 0, Elf64Types::nobase));
    }
    if (!input->newBinFormat)
    {
        elfBinGen64->addRegion(ElfRegion64::noteSection());
        elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1,
                    ".AMDGPU.config", SHT_PROGBITS, 0));
    }
    elfBinGen64->addRegion(ElfRegion64(commentSize, (const cxbyte*)comment, 1, ".comment",
              SHT_PROGBITS, SHF_MERGE|SHF_STRINGS, 0, 0, 0, 1));
    elfBinGen64->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".symtab", SHT_SYMTAB, 0, 0, BINGEN_DEFAULT));
    elfBinGen64->addRegion(ElfRegion64::shstrtabSection());
    elfBinGen64->addRegion(ElfRegion64::strtabSection());
    elfBinGen64->addRegion(ElfRegion64::sectionHeaderTable());
    
    /* extra sections */
    for (const BinSection& section: input->extraSections)
        elfBinGen64->addRegion(ElfRegion64(section, mainBuiltinSectTable,
                         ROCMSECTID_MAX, mainSectionsNum));
    updateSymbols();
    binarySize = elfBinGen64->countSize();
    
    if (rocmRelaDynGen != nullptr)
        ((ROCmRelaDynGen*)rocmRelaDynGen)->setGotOffset(
                elfBinGen64->getRegionOffset(
                        mainBuiltinSectTable[ROCMSECTID_GOT - ELFSECTID_START]));
}

void ROCmBinGenerator::updateSymbols()
{
    elfBinGen64->clearSymbols();
    elfBinGen64->clearDynSymbols();
    // add symbols (kernels, function kernels and data symbols)
    elfBinGen64->addSymbol(ElfSymbol64("_DYNAMIC",
                  mainBuiltinSectTable[ROCMSECTID_DYNAMIC-ELFSECTID_START],
                  ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE), STV_HIDDEN, true, 0, 0));
    const uint16_t textSectIndex = mainBuiltinSectTable[ELFSECTID_TEXT-ELFSECTID_START];
    const uint16_t rodataSectIndex = mainBuiltinSectTable[ELFSECTID_RODATA-ELFSECTID_START];
    
    const cxuint kernelSymType = input->llvm10BinFormat ? STT_FUNC : STT_GNU_IFUNC;
    const cxuint kernelSymVis = input->llvm10BinFormat ? STV_PROTECTED : STV_DEFAULT;
    size_t kdOffset = 0;
    size_t kdescIndex = 0;
    if (input->llvm10BinFormat)
        kdescSymNames.resize(input->symbols.size());
    for (const ROCmSymbolInput& symbol: input->symbols)
    {
        ElfSymbol64 elfsym;
        switch (symbol.type)
        {
            case ROCmRegionType::KERNEL:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), textSectIndex,
                      ELF64_ST_INFO(STB_GLOBAL, kernelSymType),
                      ELF64_ST_VISIBILITY(kernelSymVis), true,
                      symbol.offset, symbol.size);
                break;
            case ROCmRegionType::FKERNEL:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), textSectIndex,
                      ELF64_ST_INFO(STB_GLOBAL, STT_FUNC), 0, true,
                      symbol.offset, symbol.size);
                break;
            case ROCmRegionType::DATA:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), textSectIndex,
                      ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT), 0, true,
                      symbol.offset, symbol.size);
                break;
            default:
                break;
        }
        // add to symbols and dynamic symbols table
        elfBinGen64->addSymbol(elfsym);
        elfBinGen64->addDynSymbol(elfsym);
        if (symbol.type == ROCmRegionType::KERNEL && input->llvm10BinFormat)
        {
            std::string kdsym(symbol.symbolName.c_str());
            kdsym += ".kd";
            kdescSymNames[kdescIndex] = kdsym;
            elfsym = ElfSymbol64(kdescSymNames[kdescIndex].c_str(), rodataSectIndex,
                      ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT),
                      ELF64_ST_VISIBILITY(kernelSymVis), true, kdOffset, symbol.size);
            kdOffset += sizeof(ROCmKernelDescriptor);
            elfBinGen64->addSymbol(elfsym);
            elfBinGen64->addDynSymbol(elfsym);
            kdescIndex++;
        }
    }
    /* extra symbols */
    for (const BinSymbol& symbol: input->extraSymbols)
    {
        ElfSymbol64 sym(symbol, mainBuiltinSectTable,
                         ROCMSECTID_MAX, mainSectionsNum);
        elfBinGen64->addSymbol(sym);
        elfBinGen64->addDynSymbol(sym);
    }
}

void ROCmBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr)
{
    if (elfBinGen64 == nullptr)
        prepareBinaryGen();
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
    elfBinGen64->generate(bos);
    assert(bos.getWritten() == binarySize);
    
    if (rocmGotGen != nullptr)
    {
        delete (ROCmGotGen*)rocmGotGen;
        rocmGotGen = nullptr;
    }
    if (rocmRelaDynGen != nullptr)
    {
        delete (ROCmGotGen*)rocmRelaDynGen;
        rocmRelaDynGen = nullptr;
    }
    }
    catch(...)
    {
        os->exceptions(oldExceptions);
        throw;
    }
    os->exceptions(oldExceptions);
}

void ROCmBinGenerator::generate(Array<cxbyte>& array)
{
    generateInternal(nullptr, nullptr, &array);
}

void ROCmBinGenerator::generate(std::ostream& os)
{
    generateInternal(&os, nullptr, nullptr);
}

void ROCmBinGenerator::generate(std::vector<char>& v)
{
    generateInternal(nullptr, &v, nullptr);
}
