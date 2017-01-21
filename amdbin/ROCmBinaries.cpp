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
        if (ULEV(sym.st_shndx)==textIndex &&
            (symType==STT_GNU_IFUNC || symType==STT_FUNC ||
                (bind==STB_GLOBAL && symType==STT_OBJECT)))
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
        if (ULEV(sym.st_shndx)!=textIndex)
            continue;   // if not in '.text' section
        const size_t value = ULEV(sym.st_value);
        if (value < codeOffset)
            throw Exception("Region offset is too small!");
        const size_t size = ULEV(sym.st_size);
        
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        if (symType==STT_GNU_IFUNC || symType==STT_FUNC ||
                (bind==STB_GLOBAL && symType==STT_OBJECT))
        {
            ROCmRegionType type = ROCmRegionType::DATA;
            if (symType==STT_GNU_IFUNC) 
                type = ROCmRegionType::KERNEL;
            else if (symType==STT_FUNC)
                type = ROCmRegionType::FKERNEL;
            symOffsets[j] = std::make_pair(value, j);
            if (type!=ROCmRegionType::DATA && value+0x100 > codeOffset+codeSize)
                throw Exception("Kernel or code offset is too big!");
            regions[j++] = { getSymbolName(i), size, value, type };
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
        if (region.type==ROCmRegionType::KERNEL && symOffsets[i-1].first+0x100 > end)
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

struct AMDGPUArchValuesEntry
{
    uint32_t major;
    uint32_t minor;
    uint32_t stepping;
    GPUDeviceType deviceType;
};

static const AMDGPUArchValuesEntry amdGpuArchValuesTbl[] =
{
    { 0, 0, 0, GPUDeviceType::CAPE_VERDE },
    { 7, 0, 0, GPUDeviceType::BONAIRE },
    { 7, 0, 1, GPUDeviceType::HAWAII },
    { 8, 0, 0, GPUDeviceType::ICELAND },
    { 8, 0, 1, GPUDeviceType::CARRIZO },
    { 8, 0, 2, GPUDeviceType::ICELAND },
    { 8, 0, 3, GPUDeviceType::FIJI },
    { 8, 0, 4, GPUDeviceType::FIJI },
    { 8, 1, 0, GPUDeviceType::STONEY }
};

static const size_t amdGpuArchValuesNum = sizeof(amdGpuArchValuesTbl) /
                sizeof(AMDGPUArchValuesEntry);


GPUDeviceType ROCmBinary::determineGPUDeviceType(uint32_t& outArchMinor,
                     uint32_t& outArchStepping) const
{
    uint32_t archMajor = 0;
    uint32_t archMinor = 0;
    uint32_t archStepping = 0;
    
    {
        const cxbyte* noteContent = (const cxbyte*)getNotes();
        if (noteContent==nullptr)
            throw Exception("Missing notes in inner binary!");
        size_t notesSize = getNotesSize();
        // find note about AMDGPU
        for (size_t offset = 0; offset < notesSize; )
        {
            const Elf64_Nhdr* nhdr = (const Elf64_Nhdr*)(noteContent + offset);
            size_t namesz = ULEV(nhdr->n_namesz);
            size_t descsz = ULEV(nhdr->n_descsz);
            if (usumGt(offset, namesz+descsz, notesSize))
                throw Exception("Note offset+size out of range");
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
    // determine device type
    GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE;
    if (archMajor==0)
        deviceType = GPUDeviceType::CAPE_VERDE;
    else if (archMajor==7)
        deviceType = GPUDeviceType::BONAIRE;
    else if (archMajor==8)
        deviceType = GPUDeviceType::ICELAND;
    
    for (cxuint i = 0; i < amdGpuArchValuesNum; i++)
        if (amdGpuArchValuesTbl[i].major==archMajor &&
            amdGpuArchValuesTbl[i].minor==archMinor &&
            amdGpuArchValuesTbl[i].stepping==archStepping)
        {
            deviceType = amdGpuArchValuesTbl[i].deviceType;
            break;
        }
    outArchMinor = archMinor;
    outArchStepping = archStepping;
    return deviceType;
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


void ROCmInput::addEmptyKernel(const char* kernelName)
{
    symbols.push_back({ kernelName, 0, 0, ROCmRegionType::KERNEL });
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

static const cxbyte noteDescType1[8] =
{ 2, 0, 0, 0, 1, 0, 0, 0 };

static const cxbyte noteDescType3[27] =
{ 4, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  'A', 'M', 'D', 0, 'A', 'M', 'D', 'G', 'P', 'U', 0 };

// section index for symbol binding
static const uint16_t mainBuiltinSectionTable[] =
{
    10, // ELFSECTID_SHSTRTAB
    11, // ELFSECTID_STRTAB
    9, // ELFSECTID_SYMTAB
    3, // ELFSECTID_DYNSTR
    1, // ELFSECTID_DYNSYM
    4, // ELFSECTID_TEXT
    SHN_UNDEF, // ELFSECTID_RODATA
    SHN_UNDEF, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    8, // ELFSECTID_COMMENT
    2, // ROCMSECTID_HASH
    5, // ROCMSECTID_DYNAMIC
    6, // ROCMSECTID_NOTE
    7 // ROCMSECTID_GPUCONFIG
};

static const AMDGPUArchValues rocmAmdGpuArchValuesTbl[] =
{
    { 0, 0, 0 }, // GPUDeviceType::CAPE_VERDE
    { 0, 0, 0 }, // GPUDeviceType::PITCAIRN
    { 0, 0, 0 }, // GPUDeviceType::TAHITI
    { 0, 0, 0 }, // GPUDeviceType::OLAND
    { 7, 0, 0 }, // GPUDeviceType::BONAIRE
    { 7, 0, 0 }, // GPUDeviceType::SPECTRE
    { 7, 0, 0 }, // GPUDeviceType::SPOOKY
    { 7, 0, 0 }, // GPUDeviceType::KALINDI
    { 0, 0, 0 }, // GPUDeviceType::HAINAN
    { 7, 0, 1 }, // GPUDeviceType::HAWAII
    { 8, 0, 0 }, // GPUDeviceType::ICELAND
    { 8, 0, 0 }, // GPUDeviceType::TONGA
    { 7, 0, 0 }, // GPUDeviceType::MULLINS
    { 8, 0, 3 }, // GPUDeviceType::FIJI
    { 8, 0, 1 }, // GPUDeviceType::CARRIZO
    { 8, 0, 1 }, // GPUDeviceType::DUMMY
    { 8, 0, 4 }, // GPUDeviceType::GOOSE
    { 8, 0, 4 }, // GPUDeviceType::HORSE
    { 8, 0, 1 }, // GPUDeviceType::STONEY
    { 8, 0, 4 }, // GPUDeviceType::ELLESMERE
    { 8, 0, 4 } // GPUDeviceType::BAFFIN
};

void ROCmBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    AMDGPUArchValues amdGpuArchValues = rocmAmdGpuArchValuesTbl[cxuint(input->deviceType)];
    if (input->archMinor!=UINT32_MAX)
        amdGpuArchValues.minor = input->archMinor;
    if (input->archStepping!=UINT32_MAX)
        amdGpuArchValues.stepping = input->archStepping;
    
    const char* comment = "CLRX ROCmBinGenerator " CLRX_VERSION;
    uint32_t commentSize = ::strlen(comment);
    if (input->comment!=nullptr)
    {   // if comment, store comment section
        comment = input->comment;
        commentSize = input->commentSize;
        if (commentSize==0)
            commentSize = ::strlen(comment);
    }
    
    ElfBinaryGen64 elfBinGen64({ 0U, 0U, 0x40, 0, ET_DYN,
        0xe0, EV_CURRENT, UINT_MAX, 0, 0 }, true, true, true, PHREGION_FILESTART);
    // add symbols
    elfBinGen64.addSymbol(ElfSymbol64("_DYNAMIC", 5,
                  ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE), STV_HIDDEN, true, 0, 0));
    for (const ROCmSymbolInput& symbol: input->symbols)
    {
        ElfSymbol64 elfsym;
        switch (symbol.type)
        {
            case ROCmRegionType::KERNEL:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), 4,
                      ELF64_ST_INFO(STB_GLOBAL, STT_GNU_IFUNC), 0, true,
                      symbol.offset, symbol.size);
                break;
            case ROCmRegionType::FKERNEL:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), 4,
                      ELF64_ST_INFO(STB_GLOBAL, STT_FUNC), 0, true,
                      symbol.offset, symbol.size);
                break;
            case ROCmRegionType::DATA:
                elfsym = ElfSymbol64(symbol.symbolName.c_str(), 4,
                      ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT), 0, true,
                      symbol.offset, symbol.size);
                break;
            default:
                break;
        }
        elfBinGen64.addSymbol(elfsym);
        elfBinGen64.addDynSymbol(elfsym);
    }
    
    static const int32_t dynTags[] = {
        DT_SYMTAB, DT_SYMENT, DT_STRTAB, DT_STRSZ, DT_HASH };
    elfBinGen64.addDynamics(sizeof(dynTags)/sizeof(int32_t), dynTags);
    // elf program headers
    elfBinGen64.addProgramHeader({ PT_PHDR, PF_R, 0, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64.addProgramHeader({ PT_LOAD, PF_R, PHREGION_FILESTART, 4,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 0x1000 });
    elfBinGen64.addProgramHeader({ PT_LOAD, PF_R|PF_X, 4, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64.addProgramHeader({ PT_LOAD, PF_R|PF_W, 5, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64.addProgramHeader({ PT_DYNAMIC, PF_R|PF_W, 5, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 8 });
    elfBinGen64.addProgramHeader({ PT_GNU_RELRO, PF_R, 5, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 1 });
    elfBinGen64.addProgramHeader({ PT_GNU_STACK, PF_R|PF_W, PHREGION_FILESTART, 0,
                    true, 0, 0, 0 });
    
    // elf notes
    elfBinGen64.addNote({"AMD", sizeof noteDescType1, noteDescType1, 1U});
    std::unique_ptr<cxbyte[]> noteBuf(new cxbyte[0x1b]);
    ::memcpy(noteBuf.get(), noteDescType3, 0x1b);
    SULEV(*(uint32_t*)(noteBuf.get()+4), amdGpuArchValues.major);
    SULEV(*(uint32_t*)(noteBuf.get()+8), amdGpuArchValues.minor);
    SULEV(*(uint32_t*)(noteBuf.get()+12), amdGpuArchValues.stepping);
    elfBinGen64.addNote({"AMD", 0x1b, noteBuf.get(), 3U});
    
    /// region and sections
    elfBinGen64.addRegion(ElfRegion64::programHeaderTable());
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".dynsym", SHT_DYNSYM, SHF_ALLOC, 0, 1, Elf64Types::nobase));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 4,
                ".hash", SHT_HASH, SHF_ALLOC, 1, 0, Elf64Types::nobase));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".dynstr", SHT_STRTAB,
                SHF_ALLOC, 0, 0, Elf64Types::nobase));
    elfBinGen64.addRegion(ElfRegion64(input->codeSize, (const cxbyte*)input->code, 
              0x1000, ".text", SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 0, 0,
              Elf64Types::nobase, 0, false, 256));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 0x1000,
                ".dynamic", SHT_DYNAMIC, SHF_ALLOC|SHF_WRITE, 3, 0,
                Elf64Types::nobase, 0, false, 8));
    elfBinGen64.addRegion(ElfRegion64::noteSection());
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1,
                ".AMDGPU.config", SHT_PROGBITS, 0));
    elfBinGen64.addRegion(ElfRegion64(commentSize, (const cxbyte*)comment, 1, ".comment",
              SHT_PROGBITS, SHF_MERGE|SHF_STRINGS, 0, 0, 0, 1));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".symtab", SHT_SYMTAB, 0, 0, 1));
    elfBinGen64.addRegion(ElfRegion64::shstrtabSection());
    elfBinGen64.addRegion(ElfRegion64::strtabSection());
    elfBinGen64.addRegion(ElfRegion64::sectionHeaderTable());
    
    /* extra sections */
    for (const BinSection& section: input->extraSections)
        elfBinGen64.addRegion(ElfRegion64(section, mainBuiltinSectionTable,
                         ROCMSECTID_MAX, 12));
    /* extra symbols */
    for (const BinSymbol& symbol: input->extraSymbols)
        elfBinGen64.addSymbol(ElfSymbol64(symbol, mainBuiltinSectionTable,
                         ROCMSECTID_MAX, 12));
    
    size_t binarySize = elfBinGen64.countSize();
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
    elfBinGen64.generate(bos);
    assert(bos.getWritten() == binarySize);
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
