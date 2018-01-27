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
          regionsNum(0), codeSize(0), code(nullptr),
          globalDataSize(0), globalData(nullptr), metadataSize(0), metadata(nullptr),
          newBinFormat(false)
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
    
    cxuint rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { } // ignore failed
    // find '.text' section
    if (rodataIndex!=SHN_UNDEF)
    {
        globalData = getSectionContent(rodataIndex);
        const Elf64_Shdr& rodataShdr = getSectionHeader(rodataIndex);
        globalDataSize = ULEV(rodataShdr.sh_size);
    }
    
    cxuint gpuConfigIndex = SHN_UNDEF;
    try
    { gpuConfigIndex = getSectionIndex(".AMDGPU.config"); }
    catch(const Exception& ex)
    { } // ignore failed
    newBinFormat = (gpuConfigIndex == SHN_UNDEF);
    
    // counts regions (symbol or kernel)
    regionsNum = 0;
    const size_t symbolsNum = getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        // count regions number
        const Elf64_Sym& sym = getSymbol(i);
        const cxbyte symType = ELF64_ST_TYPE(sym.st_info);
        const cxbyte bind = ELF64_ST_BIND(sym.st_info);
        if (ULEV(sym.st_shndx)==textIndex &&
            (symType==STT_GNU_IFUNC || symType==STT_FUNC ||
                (bind==STB_GLOBAL && symType==STT_OBJECT)))
            regionsNum++;
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
                type = ROCmRegionType::FKERNEL;
            symOffsets[j] = std::make_pair(value, j);
            if (type!=ROCmRegionType::DATA && value+0x100 > codeOffset+codeSize)
                throw BinException("Kernel or code offset is too big!");
            regions[j++] = { getSymbolName(i), size, value, type };
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
        if (region.type==ROCmRegionType::KERNEL && symOffsets[i-1].first+0x100 > end)
            throw BinException("Kernel size is too small!");
        
        const size_t regSize = end - symOffsets[i-1].first;
        if (region.size==0)
            region.size = regSize;
        else
            region.size = std::min(regSize, region.size);
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
        
        if (namesz==4 &&
            ::strcmp((const char*)noteContent+offset+ sizeof(Elf64_Nhdr), "AMD")==0)
        {
            const uint32_t noteType = ULEV(nhdr->n_type);
            if (noteType == 0xa)
            {
                metadata = (char*)(noteContent+offset+sizeof(Elf64_Nhdr) + 4);
                metadataSize = descsz;
            }
            else if (noteType == 0xb)
                target.assign((char*)(noteContent+offset+sizeof(Elf64_Nhdr) + 4), descsz);
        }
        size_t align = (((namesz+descsz)&3)!=0) ? 4-((namesz+descsz)&3) : 0;
        offset += sizeof(Elf64_Nhdr) + namesz + descsz + align;
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
        size_t globalDataSize, const cxbyte* globalData,
        const std::vector<ROCmSymbolInput>& symbols)
{
    input = new ROCmInput{ deviceType, archMinor, archStepping, 0, false,
            globalDataSize, globalData, symbols, codeSize, code };
}

ROCmBinGenerator::ROCmBinGenerator(GPUDeviceType deviceType,
        uint32_t archMinor, uint32_t archStepping, size_t codeSize, const cxbyte* code,
        size_t globalDataSize, const cxbyte* globalData,
        std::vector<ROCmSymbolInput>&& symbols)
{
    input = new ROCmInput{ deviceType, archMinor, archStepping, 0, false,
            globalDataSize, globalData, std::move(symbols), codeSize, code };
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

// ELF notes contents
static const cxbyte noteDescType1[8] =
{ 2, 0, 0, 0, 1, 0, 0, 0 };

static const cxbyte noteDescType3[27] =
{ 4, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  'A', 'M', 'D', 0, 'A', 'M', 'D', 'G', 'P', 'U', 0 };

static inline void addMainSectionToTable(cxuint& sectionsNum, uint16_t* builtinTable,
                cxuint elfSectId)
{ builtinTable[elfSectId - ELFSECTID_START] = sectionsNum++; }

void ROCmBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    AMDGPUArchVersion amdGpuArchValues = getGPUArchVersion(input->deviceType,
                GPUArchVersionTable::OPENSOURCE);
    if (input->archMinor!=UINT32_MAX)
        amdGpuArchValues.minor = input->archMinor;
    if (input->archStepping!=UINT32_MAX)
        amdGpuArchValues.stepping = input->archStepping;
    
    const char* comment = "CLRX ROCmBinGenerator " CLRX_VERSION;
    uint32_t commentSize = ::strlen(comment);
    if (input->comment!=nullptr)
    {
        // if comment, store comment section
        comment = input->comment;
        commentSize = input->commentSize;
        if (commentSize==0)
            commentSize = ::strlen(comment);
    }
    
    uint32_t eflags = input->newBinFormat ? 2 : 0;
    if (input->eflags != BINGEN_DEFAULT)
        eflags = input->eflags;
    
    ElfBinaryGen64 elfBinGen64({ 0U, 0U, 0x40, 0, ET_DYN,
            0xe0, EV_CURRENT, UINT_MAX, 0, eflags },
            true, true, true, PHREGION_FILESTART);
    // add symbols (kernels, function kernels and data symbols)
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
        // add to symbols and dynamic symbols table
        elfBinGen64.addSymbol(elfsym);
        elfBinGen64.addDynSymbol(elfsym);
    }
    
    static const int32_t dynTags[] = {
        DT_SYMTAB, DT_SYMENT, DT_STRTAB, DT_STRSZ, DT_HASH };
    elfBinGen64.addDynamics(sizeof(dynTags)/sizeof(int32_t), dynTags);
    
    uint16_t mainBuiltinSectTable[ROCMSECTID_MAX-ELFSECTID_START+1];
    std::fill(mainBuiltinSectTable,
              mainBuiltinSectTable + ROCMSECTID_MAX-ELFSECTID_START+1, SHN_UNDEF);
    cxuint mainSectionsNum = 1;
    
    // generate main builtin section table (for section id translation)
    if (input->newBinFormat)
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_NOTE);
    if (input->globalData != nullptr)
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_RODATA);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_DYNSYM);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_HASH);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_DYNSTR);
    const cxuint execProgHeaderRegionIndex = mainSectionsNum;
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_TEXT);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_DYNAMIC);
    if (!input->newBinFormat)
    {
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_NOTE);
        addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ROCMSECTID_GPUCONFIG);
    }
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_COMMENT);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_SYMTAB);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_SHSTRTAB);
    addMainSectionToTable(mainSectionsNum, mainBuiltinSectTable, ELFSECTID_STRTAB);
    
    // elf program headers
    elfBinGen64.addProgramHeader({ PT_PHDR, PF_R, 0, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64.addProgramHeader({ PT_LOAD, PF_R, PHREGION_FILESTART,
                    execProgHeaderRegionIndex,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 0x1000 });
    elfBinGen64.addProgramHeader({ PT_LOAD, PF_R|PF_X, execProgHeaderRegionIndex, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64.addProgramHeader({ PT_LOAD, PF_R|PF_W, execProgHeaderRegionIndex+1, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0 });
    elfBinGen64.addProgramHeader({ PT_DYNAMIC, PF_R|PF_W, execProgHeaderRegionIndex+1, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 8 });
    elfBinGen64.addProgramHeader({ PT_GNU_RELRO, PF_R, execProgHeaderRegionIndex+1, 1,
                    true, Elf64Types::nobase, Elf64Types::nobase, 0, 1 });
    elfBinGen64.addProgramHeader({ PT_GNU_STACK, PF_R|PF_W, PHREGION_FILESTART, 0,
                    true, 0, 0, 0 });
    
    if (input->newBinFormat)
        // program header for note (new binary format)
        elfBinGen64.addProgramHeader({ PT_NOTE, PF_R, 1, 1, true, 0, 0, 0 });
    
    // elf notes
    elfBinGen64.addNote({"AMD", sizeof noteDescType1, noteDescType1, 1U});
    std::unique_ptr<cxbyte[]> noteBuf(new cxbyte[0x1b]);
    ::memcpy(noteBuf.get(), noteDescType3, 0x1b);
    SULEV(*(uint32_t*)(noteBuf.get()+4), amdGpuArchValues.major);
    SULEV(*(uint32_t*)(noteBuf.get()+8), amdGpuArchValues.minor);
    SULEV(*(uint32_t*)(noteBuf.get()+12), amdGpuArchValues.stepping);
    elfBinGen64.addNote({"AMD", 0x1b, noteBuf.get(), 3U});
    if (!input->target.empty())
        elfBinGen64.addNote({"AMD", input->target.size(),
                (const cxbyte*)input->target.c_str(), 0xbU});
    if (input->metadataSize != 0)
        elfBinGen64.addNote({"AMD", input->metadataSize,
                (const cxbyte*)input->metadata, 0xaU});
    
    /// region and sections
    elfBinGen64.addRegion(ElfRegion64::programHeaderTable());
    if (input->newBinFormat)
        elfBinGen64.addRegion(ElfRegion64::noteSection());
    if (input->globalData != nullptr)
        elfBinGen64.addRegion(ElfRegion64(input->globalDataSize, input->globalData, 4,
                ".rodata", SHT_PROGBITS, SHF_ALLOC, 0, 0, Elf64Types::nobase));
    
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".dynsym", SHT_DYNSYM, SHF_ALLOC, 0, 1, Elf64Types::nobase));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 4,
                ".hash", SHT_HASH, SHF_ALLOC,
                mainBuiltinSectTable[ELFSECTID_DYNSYM-ELFSECTID_START], 0,
                Elf64Types::nobase));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".dynstr", SHT_STRTAB,
                SHF_ALLOC, 0, 0, Elf64Types::nobase));
    // '.text' with alignment=4096
    elfBinGen64.addRegion(ElfRegion64(input->codeSize, (const cxbyte*)input->code, 
              0x1000, ".text", SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 0, 0,
              Elf64Types::nobase, 0, false, 256));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 0x1000,
                ".dynamic", SHT_DYNAMIC, SHF_ALLOC|SHF_WRITE,
                mainBuiltinSectTable[ELFSECTID_DYNSTR-ELFSECTID_START], 0,
                Elf64Types::nobase, 0, false, 8));
    if (!input->newBinFormat)
    {
        elfBinGen64.addRegion(ElfRegion64::noteSection());
        elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1,
                    ".AMDGPU.config", SHT_PROGBITS, 0));
    }
    elfBinGen64.addRegion(ElfRegion64(commentSize, (const cxbyte*)comment, 1, ".comment",
              SHT_PROGBITS, SHF_MERGE|SHF_STRINGS, 0, 0, 0, 1));
    elfBinGen64.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8,
                ".symtab", SHT_SYMTAB, 0, 0, 1));
    elfBinGen64.addRegion(ElfRegion64::shstrtabSection());
    elfBinGen64.addRegion(ElfRegion64::strtabSection());
    elfBinGen64.addRegion(ElfRegion64::sectionHeaderTable());
    
    /* extra sections */
    for (const BinSection& section: input->extraSections)
        elfBinGen64.addRegion(ElfRegion64(section, mainBuiltinSectTable,
                         ROCMSECTID_MAX, mainSectionsNum));
    /* extra symbols */
    for (const BinSymbol& symbol: input->extraSymbols)
        elfBinGen64.addSymbol(ElfSymbol64(symbol, mainBuiltinSectTable,
                         ROCMSECTID_MAX, mainSectionsNum));
    
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
