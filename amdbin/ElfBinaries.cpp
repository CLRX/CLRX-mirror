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
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <climits>
#include <utility>
#include <string>
#include <cassert>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdBinaries.h>

static const uint32_t elfMagicValue = 0x464c457fU;

/* INFO: in this file is used ULEV function for conversion
 * from LittleEndian and unaligned access to other memory access policy and endianness
 * Please use this function whenever you want to get or set word in ELF binary,
 * because ELF binaries can be unaligned in memory (as inner binaries).
 */

using namespace CLRX;

// BinException costuctor
BinException::BinException(const std::string& message) : Exception(message)
{ }

// BinGenException costuctor
BinGenException::BinGenException(const std::string& message) : Exception(message)
{ }

/* determine unfinished strings region in string table for checking further consistency */
static size_t unfinishedRegionOfStringTable(const cxbyte* table, size_t size)
{
    if (size == 0) // if zero
        return 0;
    size_t k;
    for (k = size-1; k>0 && table[k]!=0; k--);
    
    return (table[k]==0)?k+1:k;
}

/* elf32 types */

const cxbyte CLRX::Elf32Types::ELFCLASS = ELFCLASS32;
const uint32_t CLRX::Elf32Types::bitness = 32;
const char* CLRX::Elf32Types::bitName = "32";

/* elf64 types */

const cxbyte CLRX::Elf64Types::ELFCLASS = ELFCLASS64;
const cxuint CLRX::Elf64Types::bitness = 64;
const char* CLRX::Elf64Types::bitName = "64";

/* ElfBinaryTemplate */

template<typename Types>
ElfBinaryTemplate<Types>::ElfBinaryTemplate() : binaryCodeSize(0), binaryCode(nullptr),
        sectionStringTable(nullptr), symbolStringTable(nullptr),
        symbolTable(nullptr), dynSymStringTable(nullptr), dynSymTable(nullptr),
        noteTable(nullptr), symbolsNum(0), dynSymbolsNum(0),
        noteTableSize(0), dynamicsNum(0), symbolEntSize(0), dynSymEntSize(0),
        dynamicEntSize(0)
{ }

template<typename Types>
ElfBinaryTemplate<Types>::~ElfBinaryTemplate()
{ }

template<typename Types>
ElfBinaryTemplate<Types>::ElfBinaryTemplate(size_t _binaryCodeSize, cxbyte* _binaryCode,
             Flags _creationFlags) : creationFlags(_creationFlags),
        binaryCodeSize(_binaryCodeSize), binaryCode(_binaryCode),
        sectionStringTable(nullptr), symbolStringTable(nullptr),
        symbolTable(nullptr), dynSymStringTable(nullptr), dynSymTable(nullptr),
        noteTable(nullptr), symbolsNum(0), dynSymbolsNum(0),
        noteTableSize(0), dynamicsNum(0), symbolEntSize(0), dynSymEntSize(0),
        dynamicEntSize(0)     
{
    if (binaryCodeSize < sizeof(typename Types::Ehdr))
        throw BinException("Binary is too small!!!");
    
    const typename Types::Ehdr* ehdr =
            reinterpret_cast<const typename Types::Ehdr*>(binaryCode);
    
    // checking ELF magic, ELFCLASS and endian (only little-endian accepted)
    if (ULEV(*reinterpret_cast<const uint32_t*>(binaryCode)) != elfMagicValue)
        throw BinException("This is not ELF binary");
    if (ehdr->e_ident[EI_CLASS] != Types::ELFCLASS)
        throw BinException(std::string("This is not ")+Types::bitName+"bit ELF binary");
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        throw BinException("Other than little-endian binaries are not supported!");
    
    if ((ULEV(ehdr->e_phoff) == 0 && ULEV(ehdr->e_phnum) != 0))
        throw BinException("Elf invalid phoff and phnum combination");
    if (ULEV(ehdr->e_phoff) != 0)
    {
        /* reading and checking program headers */
        if (ULEV(ehdr->e_phoff) > binaryCodeSize)
            throw BinException("ProgramHeaders offset out of range!");
        if (usumGt(ULEV(ehdr->e_phoff),
                   ((typename Types::Word)ULEV(ehdr->e_phentsize))*ULEV(ehdr->e_phnum),
                   binaryCodeSize))
            throw BinException("ProgramHeaders offset+size out of range!");
        
        cxuint phnum = ULEV(ehdr->e_phnum);
        // checking program header segment offset ranges
        for (cxuint i = 0; i < phnum; i++)
        {
            const typename Types::Phdr& phdr = getProgramHeader(i);
            if (ULEV(phdr.p_offset) > binaryCodeSize)
                throw BinException("Segment offset out of range!");
            if (usumGt(ULEV(phdr.p_offset), ULEV(phdr.p_filesz), binaryCodeSize))
                throw BinException("Segment offset+size out of range!");
        }
    }
    
    if ((ULEV(ehdr->e_shoff) == 0 && ULEV(ehdr->e_shnum) != 0))
        throw BinException("Elf invalid shoff and shnum combination");
    if (ULEV(ehdr->e_shoff) != 0 && ULEV(ehdr->e_shstrndx) != SHN_UNDEF)
    {
        /* indexing of sections */
        if (ULEV(ehdr->e_shoff) > binaryCodeSize)
            throw BinException("SectionHeaders offset out of range!");
        if (usumGt(ULEV(ehdr->e_shoff),
                  ((typename Types::Word)ULEV(ehdr->e_shentsize))*ULEV(ehdr->e_shnum),
                  binaryCodeSize))
            throw BinException("SectionHeaders offset+size out of range!");
        if (ULEV(ehdr->e_shstrndx) >= ULEV(ehdr->e_shnum))
            throw BinException("Shstrndx out of range!");
        
        typename Types::Shdr& shstrShdr = getSectionHeader(ULEV(ehdr->e_shstrndx));
        sectionStringTable = binaryCode + ULEV(shstrShdr.sh_offset);
        const size_t unfinishedShstrPos = unfinishedRegionOfStringTable(
                    sectionStringTable, ULEV(shstrShdr.sh_size));
        
        const typename Types::Shdr* symTableHdr = nullptr;
        const typename Types::Shdr* dynSymTableHdr = nullptr;
        const typename Types::Shdr* noteTableHdr = nullptr;
        const typename Types::Shdr* dynamicTableHdr = nullptr;
        
        cxuint shnum = ULEV(ehdr->e_shnum);
        if ((creationFlags & ELF_CREATE_SECTIONMAP) != 0)
            sectionIndexMap.resize(shnum);
        for (cxuint i = 0; i < shnum; i++)
        {
            const typename Types::Shdr& shdr = getSectionHeader(i);
            /// checking section offset ranges
            if (ULEV(shdr.sh_offset) > binaryCodeSize)
                throw BinException("Section offset out of range!");
            if (ULEV(shdr.sh_type) != SHT_NOBITS)
                if (usumGt(ULEV(shdr.sh_offset), ULEV(shdr.sh_size), binaryCodeSize))
                    throw BinException("Section offset+size out of range!");
            if (ULEV(shdr.sh_link) >= ULEV(ehdr->e_shnum))
                throw BinException("Section link out of range!");
            
            const typename Types::Size sh_nameindx = ULEV(shdr.sh_name);
            if (sh_nameindx >= ULEV(shstrShdr.sh_size))
                throw BinException("Section name index out of range!");
            
            if (sh_nameindx >= unfinishedShstrPos)
                throw BinException("Unfinished section name!");
            
            const char* shname =
                reinterpret_cast<const char*>(sectionStringTable + sh_nameindx);
            
            if ((creationFlags & ELF_CREATE_SECTIONMAP) != 0)
                sectionIndexMap[i] = std::make_pair(shname, i);
            // set symbol table and dynamic symbol table pointers
            if (ULEV(shdr.sh_type) == SHT_SYMTAB)
                symTableHdr = &shdr;
            if (ULEV(shdr.sh_type) == SHT_DYNSYM)
                dynSymTableHdr = &shdr;
            if (ULEV(shdr.sh_type) == SHT_NOTE)
                noteTableHdr = &shdr;
            if (ULEV(shdr.sh_type) == SHT_DYNAMIC)
                dynamicTableHdr = &shdr;
        }
        // sort section's map (really is array of sections)
        if ((creationFlags & ELF_CREATE_SECTIONMAP) != 0)
            mapSort(sectionIndexMap.begin(), sectionIndexMap.end(), CStringLess());
        
        if (symTableHdr != nullptr)
        {
            // indexing symbols
            if (ULEV(symTableHdr->sh_entsize) < sizeof(typename Types::Sym))
                throw BinException("SymTable entry size is too small!");
            
            symbolEntSize = ULEV(symTableHdr->sh_entsize);
            symbolTable = binaryCode + ULEV(symTableHdr->sh_offset);
            if (ULEV(symTableHdr->sh_link) == SHN_UNDEF)
                throw BinException("Symbol table doesn't have string table");
            
            typename Types::Shdr& symstrShdr = getSectionHeader(ULEV(symTableHdr->sh_link));
            symbolStringTable = binaryCode + ULEV(symstrShdr.sh_offset);
            
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    symbolStringTable, ULEV(symstrShdr.sh_size));
            symbolsNum = ULEV(symTableHdr->sh_size)/ULEV(symTableHdr->sh_entsize);
            if ((creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                symbolIndexMap.resize(symbolsNum);
            
            for (typename Types::Size i = 0; i < symbolsNum; i++)
            {
                /* verify symbol names */
                const typename Types::Sym& sym = getSymbol(i);
                const typename Types::Size symnameindx = ULEV(sym.st_name);
                if (symnameindx >= ULEV(symstrShdr.sh_size))
                    throw BinException("Symbol name index out of range!");
                // check whether name is finished in string section content
                if (symnameindx >= unfinishedSymstrPos)
                    throw BinException("Unfinished symbol name!");
                
                const char* symname =
                    reinterpret_cast<const char*>(symbolStringTable + symnameindx);
                // add to symbol map
                if ((creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                    symbolIndexMap[i] = std::make_pair(symname, i);
            }
            // sort symbol's map (really is array of symbols)
            if ((creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                mapSort(symbolIndexMap.begin(), symbolIndexMap.end(), CStringLess());
        }
        if (dynSymTableHdr != nullptr)
        {
            // indexing dynamic symbols
            if (ULEV(dynSymTableHdr->sh_entsize) < sizeof(typename Types::Sym))
                throw BinException("DynSymTable entry size is too small!");
            
            dynSymEntSize = ULEV(dynSymTableHdr->sh_entsize);
            dynSymTable = binaryCode + ULEV(dynSymTableHdr->sh_offset);
            if (ULEV(dynSymTableHdr->sh_link) == SHN_UNDEF)
                throw BinException("DynSymbol table doesn't have string table");
            
            typename Types::Shdr& dynSymstrShdr =
                    getSectionHeader(ULEV(dynSymTableHdr->sh_link));
            dynSymbolsNum = ULEV(dynSymTableHdr->sh_size)/ULEV(dynSymTableHdr->sh_entsize);
            
            dynSymStringTable = binaryCode + ULEV(dynSymstrShdr.sh_offset);
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    dynSymStringTable, ULEV(dynSymstrShdr.sh_size));
            
            if ((creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                dynSymIndexMap.resize(dynSymbolsNum);
            
            for (typename Types::Size i = 0; i < dynSymbolsNum; i++)
            {
                /* verify symbol names */
                const typename Types::Sym& sym = getDynSymbol(i);
                const typename Types::Size symnameindx = ULEV(sym.st_name);
                if (symnameindx >= ULEV(dynSymstrShdr.sh_size))
                    throw BinException("DynSymbol name index out of range!");
                // check whether name is finished in string section content
                if (symnameindx >= unfinishedSymstrPos)
                    throw BinException("Unfinished dynsymbol name!");
                
                const char* symname =
                    reinterpret_cast<const char*>(dynSymStringTable + symnameindx);
                // add to symbol map
                if ((creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                    dynSymIndexMap[i] = std::make_pair(symname, i);
            }
            // sort dynamic symbol's map (really is array of dynamic symbols)
            if ((creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                mapSort(dynSymIndexMap.begin(), dynSymIndexMap.end(), CStringLess());
        }
        if (noteTableHdr != nullptr)
        {
            noteTable = binaryCode + ULEV(noteTableHdr->sh_offset);
            noteTableSize = ULEV(noteTableHdr->sh_size);
        }
        if (dynamicTableHdr != nullptr)
        {
            dynamicTable = binaryCode + ULEV(dynamicTableHdr->sh_offset);
            const typename Types::Size entSize = ULEV(dynamicTableHdr->sh_entsize);
            const typename Types::Size size = ULEV(dynamicTableHdr->sh_size);
            if (entSize < sizeof(typename Types::Dyn))
                throw BinException("Size of dynamic entry is too small!");
            if (size % entSize != 0)
                throw BinException("Size of dynamic section is not match!");
            dynamicsNum = entSize / size;
            dynamicEntSize = entSize;
        }
    }
}

template<typename Types>
uint16_t ElfBinaryTemplate<Types>::getSectionIndex(const char* name) const
{
    if (hasSectionMap())
    {
        // find in section map (sorted array)
        SectionIndexMap::const_iterator it = binaryMapFind(
                    sectionIndexMap.begin(), sectionIndexMap.end(), name, CStringLess());
        if (it == sectionIndexMap.end())
            throw BinException(std::string("Can't find Elf")+Types::bitName+" Section");
        return it->second;
    }
    else
    {
        // find in section headers (fallback)
        for (cxuint i = 0; i < getSectionHeadersNum(); i++)
        {
            if (::strcmp(getSectionName(i), name) == 0)
                return i;
        }
        throw BinException(std::string("Can't find Elf")+Types::bitName+" Section");
    }
}

template<typename Types>
typename Types::Size ElfBinaryTemplate<Types>::getSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = binaryMapFind(
                    symbolIndexMap.begin(), symbolIndexMap.end(), name, CStringLess());
    if (it == symbolIndexMap.end())
        throw BinException(std::string("Can't find Elf")+Types::bitName+" Symbol");
    return it->second;
}

template<typename Types>
typename Types::Size ElfBinaryTemplate<Types>::getDynSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = binaryMapFind(
                    dynSymIndexMap.begin(), dynSymIndexMap.end(), name, CStringLess());
    if (it == dynSymIndexMap.end())
        throw BinException(std::string("Can't find Elf")+Types::bitName+" DynSymbol");
    return it->second;
}

template class CLRX::ElfBinaryTemplate<CLRX::Elf32Types>;
template class CLRX::ElfBinaryTemplate<CLRX::Elf64Types>;

bool CLRX::isElfBinary(size_t binarySize, const cxbyte* binary)
{
    if (binarySize < sizeof(Elf32_Ehdr) ||
        ULEV(*reinterpret_cast<const uint32_t*>(binary)) != elfMagicValue)
        return false;
    if ((binary[EI_CLASS] != ELFCLASS32 && binary[EI_CLASS] != ELFCLASS64) ||
        binary[EI_DATA] != ELFDATA2LSB) // only LSB elf is supported
        return false;
    // binary must be greater than ELF header
    if ((binary[EI_CLASS] == ELFCLASS32 && binarySize < sizeof(Elf32_Ehdr)) ||
        (binary[EI_CLASS] == ELFCLASS64 && binarySize < sizeof(Elf64_Ehdr)))
        return false;
    if (ULEV(*((const uint64_t*)(binary+8))) != 0)
        return false;
    return true;
}

/*
 * Elf binary generator
 */

uint16_t CLRX::convertSectionId(cxuint sectionIndex, const uint16_t* builtinSections,
                  cxuint maxBuiltinSection, ElfBinSectId extraSectionIndex)
{
    if (sectionIndex == ELFSECTID_NULL)
        return 0;
    if (sectionIndex == ELFSECTID_ABS)
        return SHN_ABS;
    if (sectionIndex == ELFSECTID_UNDEF)
        return SHN_UNDEF;
    if (sectionIndex < ELFSECTID_START)
        return sectionIndex+extraSectionIndex;
    else if (sectionIndex >= ELFSECTID_START && sectionIndex <= maxBuiltinSection)
    {
        const uint16_t shndx = builtinSections[sectionIndex-ELFSECTID_START];
        if (shndx == SHN_UNDEF) // if table entry for sectionIndex is not defined
            throw BinGenException("Wrong BinSection:sectionId");
        return builtinSections[sectionIndex-ELFSECTID_START];
    }
    else // failed
        throw BinGenException("Wrong BinSection:sectionId");
}

ElfRegionContent::~ElfRegionContent()
{ }

template<typename Types>
static std::unique_ptr<uint32_t[]> calculateHashValuesForSymbols(bool addNullSymbol,
            const std::vector<ElfSymbolTemplate<Types> >& symbols)
{
    const size_t symsNum = symbols.size() + addNullSymbol;
    std::unique_ptr<uint32_t[]> hashCodes(new uint32_t[symsNum]);
    if (addNullSymbol)
        hashCodes[0] = 0;
    for (size_t i = 0; i < symbols.size(); i++)
    {
        // main routine of hash
        uint32_t h = 0, g;
        const cxbyte* name = reinterpret_cast<const cxbyte*>(symbols[i].name);
        while(*name!=0)
        {
            h = (h<<4) + *name++;
            g = h & 0xf0000000U;
            if (g) h ^= g>>24;
            h &= ~g;
        }
        hashCodes[i+addNullSymbol] = h;
    }
    return hashCodes;
}

/// return bucket number
static uint32_t optimizeHashBucketsNum(uint32_t hashNum, bool skipFirst,
                           const uint32_t* hashCodes)
{
    uint32_t bestBucketNum = 0;
    uint64_t bestValue = UINT64_MAX;
    uint32_t firstStep = std::max(uint32_t(hashNum>>2), 1U);
    uint64_t maxSteps = (uint64_t(hashNum)<<1) - (firstStep) + 1;
    // limit step of optimizations to 4000
    const uint32_t steps = std::min(maxSteps, uint64_t(4000U));
    
    std::unique_ptr<uint32_t[]> chainLengths(new uint32_t[(hashNum<<2)+1]);
    const uint32_t stepSize = maxSteps / steps;
    for (uint32_t buckets = firstStep; buckets <= (hashNum<<1); buckets += stepSize)
    {   //
        std::fill(chainLengths.get(), chainLengths.get() + buckets, 0U);
        // calculate chain lengths
        for (size_t i = skipFirst; i < hashNum; i++)
            chainLengths[hashCodes[i] % buckets]++;
        /// value, smaller is better
        uint64_t value = uint64_t(buckets);
        for (uint32_t i = 0; i < buckets; i++)
            value += chainLengths[i]*chainLengths[i];
        if (value < bestValue)
        {
            bestBucketNum = buckets;
            bestValue = value;
        }
    }
    return bestBucketNum;
}

template<typename Types>
ElfBinaryGenTemplate<Types>::ElfBinaryGenTemplate()
        : sizeComputed(false), addNullSym(true), addNullDynSym(true), addNullSection(true),
          addrStartRegion(0), shStrTab(0), strTab(0), dynStr(0), shdrTabRegion(0),
          phdrTabRegion(0), bucketsNum(0), isHashDynSym(false)
{ }

template<typename Types>
ElfBinaryGenTemplate<Types>::ElfBinaryGenTemplate(const ElfHeaderTemplate<Types>& _header,
            bool _addNullSym, bool _addNullDynSym, bool _addNullSection,
            cxuint addrCountingFromRegion)
        : sizeComputed(false), addNullSym(_addNullSym), addNullDynSym(_addNullDynSym),
          addNullSection(_addNullSection),  addrStartRegion(addrCountingFromRegion),
          shStrTab(0), strTab(0), dynStr(0), shdrTabRegion(0), phdrTabRegion(0),
          header(_header), bucketsNum(0), isHashDynSym(false)
{ }

template<typename Types>
void ElfBinaryGenTemplate<Types>::addRegion(const ElfRegionTemplate<Types>& region)
{ regions.push_back(region); }

template<typename Types>
void ElfBinaryGenTemplate<Types>::addProgramHeader(
            const ElfProgramHeaderTemplate<Types>& progHeader)
{ progHeaders.push_back(progHeader); }

static const int32_t dynTableSize = DT_NUM;


template<typename Types>
static inline typename Types::Word resolveSectionAddress(
        const ElfHeaderTemplate<Types>& header, const ElfRegionTemplate<Types>& region2,
        typename Types::Word regionAddr)
{
    /* addrBase is base address of first section. if not defined
     * use address base as virtual address base from elf header */
    if (region2.section.addrBase==Types::nobase)
        return regionAddr;
    else if (region2.section.addrBase != 0)
        return region2.section.addrBase+regionAddr;
    else if (header.vaddrBase != 0)
        return header.vaddrBase+regionAddr;
    else
        return 0;
}

template<typename Types>
void ElfBinaryGenTemplate<Types>::computeSize()
{
    if (sizeComputed) return;
    
    /* verify data */
    if (header.entryRegion != UINT_MAX && header.entryRegion >= regions.size())
        throw BinGenException("Header entry region out of range");
    
    regionOffsets.reset(new typename Types::Word[regions.size()]);
    regionAddresses.reset(new typename Types::Word[regions.size()]);
    size = sizeof(typename Types::Ehdr);
    sectionsNum = addNullSection; // if add null section
    cxuint hashSymSectionIdx = UINT_MAX;
    bool haveDynamic = false;
    for (const auto& region: regions)
        if (region.type == ElfRegionType::SECTION)
        {
            if (region.section.type==SHT_HASH)
                hashSymSectionIdx = region.section.link;
            else if (region.section.type==SHT_DYNAMIC)
                haveDynamic = true;
            sectionsNum++;
        }
    
    /// determine symbol name
    cxuint sectionCount = addNullSection;
    isHashDynSym = false;
    if (hashSymSectionIdx!=UINT_MAX)
    {
        bool hashSymDetected = false;
        // find hash symbol (or dynsymbol) section
        for (const auto& region: regions)
            if (region.type == ElfRegionType::SECTION)
            {
                if (hashSymSectionIdx==sectionCount)
                {
                    // get symbol section
                    if (region.section.type==SHT_SYMTAB)
                    {
                        isHashDynSym = false;
                        hashSymDetected = true;
                    }
                    else if (region.section.type==SHT_DYNSYM)
                    {
                        isHashDynSym = true;
                        hashSymDetected = true;
                    }
                    else
                        throw BinGenException("Wrong Hash Sym section!");
                }
                sectionCount++;
            }
        if (!hashSymDetected)
            throw BinGenException("Wrong Hash Sym is not detected!");
    }
    
    sectionRegions.reset(new cxuint[sectionsNum+1]);
    // first section can be null, if not null section provided this will be filed later
    sectionRegions[0] = UINT_MAX;
    sectionCount = addNullSection;
    typename Types::Word address = (addrStartRegion==PHREGION_FILESTART) ? size : 0;
    
    std::unique_ptr<typename Types::Word[]> dynValTable;
    if (haveDynamic)
    {
        // prepare dynamic structures
        dynamicValues.reset(new typename Types::Word[dynamics.size()]);
        dynValTable.reset(new typename Types::Word[dynTableSize]);
    }
    
    // verify symbol's sections (accepts SHN_ABS and SHN_UNDEF)
    for (const auto& sym: symbols)
        if (sym.sectionIndex >= sectionsNum && sym.sectionIndex!=SHN_ABS &&
                    sym.sectionIndex!=SHN_UNDEF)
            throw BinGenException("Symbol section index out of range");
    for (const auto& sym: dynSymbols)
        if (sym.sectionIndex >= sectionsNum && sym.sectionIndex!=SHN_ABS &&
                    sym.sectionIndex!=SHN_UNDEF)
            throw BinGenException("DynSymbol section index out of range");
    
    for (size_t i = 0; i < regions.size(); i++)
    {
        ElfRegionTemplate<Types>& region = regions[i];
        typename Types::Word ralign = (region.type==ElfRegionType::SECTION) ?
                        region.section.align : 0;
        ralign = std::max(region.align, ralign);
        if (ralign > 1)
        {
            // fix alignment
            if ((size&(ralign-1))!=0)
                size += ralign - (size&(ralign-1));
            if ((address&(ralign-1))!=0)
                address += ralign - (address&(ralign-1));
        }
        
        regionOffsets[i] = size;
        regionAddresses[i] = address;
        // add region size
        if (region.type == ElfRegionType::PHDR_TABLE)
        {
            size += uint64_t(progHeaders.size())*sizeof(typename Types::Phdr);
            region.size = size-regionOffsets[i];
            phdrTabRegion = i;
            // checking program header ranges
            for (const auto& progHdr: progHeaders)
            {
                if (progHdr.regionStart!=PHREGION_FILESTART &&
                            progHdr.regionStart >= regions.size())
                    throw BinGenException("Region start out of range");
                if ((progHdr.regionStart==PHREGION_FILESTART &&
                     progHdr.regionsNum > regions.size()) ||
                    (progHdr.regionStart!=PHREGION_FILESTART &&
                     uint64_t(progHdr.regionStart) + progHdr.regionsNum > regions.size()))
                    throw BinGenException("Region end out of range");
            }
            if (addrStartRegion==PHREGION_FILESTART)
                address = size;
        }
        else if (region.type == ElfRegionType::SHDR_TABLE)
        {
            size += uint64_t(sectionsNum)*sizeof(typename Types::Shdr);
            region.size = size-regionOffsets[i];
            shdrTabRegion = i;
            if (addrStartRegion==PHREGION_FILESTART)
                address = size;
        }
        else if (region.type == ElfRegionType::USER)
        {
            if (addrStartRegion==PHREGION_FILESTART)
                address = size;
            size += region.size;
        }
        else if (region.type == ElfRegionType::SECTION)
        {
            // if section
            if (region.section.link >= sectionsNum)
                throw BinGenException("Section link out of range");
            
            if (haveDynamic)
            {
                switch(region.section.type)
                {
                    case SHT_DYNSYM:
                        dynValTable[DT_SYMTAB] = resolveSectionAddress(header, region,
                                   regionAddresses[i]);
                        dynValTable[DT_SYMENT] = sizeof(typename Types::Sym);
                        break;
                    case SHT_STRTAB:
                        if (::strcmp(region.section.name, ".dynstr")==0)
                            dynValTable[DT_STRTAB] = resolveSectionAddress(header, region,
                                   regionAddresses[i]);
                        break;
                    case SHT_HASH:
                        dynValTable[DT_HASH] = resolveSectionAddress(header, region,
                                 regionAddresses[i]);
                        break;
                    case SHT_RELA:
                        dynValTable[DT_RELA] = resolveSectionAddress(header, region,
                                 regionAddresses[i]);
                        dynValTable[DT_RELASZ] = region.size;
                        dynValTable[DT_RELAENT] = sizeof(typename Types::Rela);
                        break;
                    case SHT_REL:
                        dynValTable[DT_REL] = resolveSectionAddress(header, region,
                                 regionAddresses[i]);
                        dynValTable[DT_RELSZ] = region.size;
                        dynValTable[DT_RELENT] = sizeof(typename Types::Rel);
                        break;
                }
            }
            
            if (region.section.type != SHT_NOBITS && region.size != 0)
                size += region.size;
            else
            {
                // otherwise get default size for symtab, dynsym, strtab, dynstr
                if (region.section.type == SHT_SYMTAB)
                    size += uint64_t(symbols.size()+addNullSym)*
                                sizeof(typename Types::Sym);
                else if (region.section.type == SHT_DYNSYM)
                    size += uint64_t(dynSymbols.size()+addNullDynSym)*
                                sizeof(typename Types::Sym);
                else if (region.section.type == SHT_HASH)
                {
                    const std::vector<ElfSymbolTemplate<Types> >& hashSymbols = 
                        (isHashDynSym) ? dynSymbols : symbols;
                    bool addNullHashSym = (isHashDynSym) ? addNullDynSym : addNullSym;
                    // calculating hashes of symbols and optimizing hash buckets
                    hashCodes = calculateHashValuesForSymbols(addNullDynSym, hashSymbols);
                    bucketsNum = optimizeHashBucketsNum(hashSymbols.size()+addNullHashSym,
                           addNullHashSym, hashCodes.get());
                    // and add these hash size to size
                    size += 4*(bucketsNum + hashSymbols.size()+addNullHashSym + 2);
                }
                else if (region.section.type == SHT_DYNAMIC)
                    size += (dynamics.size()+1) * sizeof(typename Types::Dyn);
                else if (region.section.type == SHT_NOTE)
                {
                    for (const ElfNote& note: notes)
                    {
                        // note size with data
                        size_t nameSize = ::strlen(note.name)+1;
                        if ((nameSize&3)!=0)
                            nameSize += 4 - (nameSize&3);
                        size_t descSize = note.descSize;
                        if ((descSize&3)!=0)
                            descSize += 4 - (descSize&3);
                        size += sizeof(typename Types::Nhdr) + nameSize + descSize;
                    }
                }
                else if (region.section.type == SHT_STRTAB)
                {
                    if (::strcmp(region.section.name, ".strtab") == 0)
                    {
                        size += (addNullSym);
                        for (const auto& sym: symbols)
                            if (sym.name != nullptr && sym.name[0] != 0)
                                size += ::strlen(sym.name)+1;
                    }
                    else if (::strcmp(region.section.name, ".dynstr") == 0)
                    {
                        size += (addNullDynSym);
                        for (const auto& sym: dynSymbols)
                            if (sym.name != nullptr && sym.name[0] != 0)
                                size += ::strlen(sym.name)+1;
                    }
                    else if (::strcmp(region.section.name, ".shstrtab") == 0)
                    {
                        size += (addNullSection);
                        for (const auto& region2: regions)
                        {
                            if (region2.type == ElfRegionType::SECTION &&
                                region2.section.name != nullptr &&
                                region2.section.name[0] != 0)
                                size += ::strlen(region2.section.name)+1;
                        }
                    }
                }
                if (region.section.type != SHT_NOBITS)
                    region.size = size-regionOffsets[i];
            }
            if (addrStartRegion==PHREGION_FILESTART)
                address = size;
            else if (i >= addrStartRegion) // begin counting address from that region
                address += region.size;
            
            if (haveDynamic)
            {
                // set dynamics
                if (region.section.type == SHT_STRTAB &&
                    ::strcmp(region.section.name, ".dynstr") == 0)
                    dynValTable[DT_STRSZ] = region.size;
            }
            
            if (::strcmp(region.section.name, ".strtab") == 0)
                strTab = sectionCount;
            else if (::strcmp(region.section.name, ".dynstr") == 0)
                dynStr = sectionCount;
            else if (::strcmp(region.section.name, ".shstrtab") == 0)
                shStrTab = sectionCount;
            sectionRegions[sectionCount] = i;
            sectionCount++;
        }
    }
    
    if (haveDynamic)
    {
        // set dynamic values
        for (size_t i = 0; i < dynamics.size(); i++)
            if (dynamics[i] >= 0 && dynamics[i] < dynTableSize)
                dynamicValues[i] = dynValTable[dynamics[i]];
    }
    
    sizeComputed = true;
}

template<typename Types>
uint64_t ElfBinaryGenTemplate<Types>::countSize()
{
    computeSize();
    return size;
}

// create hash table, really bucket chains and fill buckets
static void createHashTable(uint32_t bucketsNum, uint32_t hashNum, bool skipFirst,
                           const uint32_t* hashCodes, uint32_t* output)
{
    SLEV(output[0], bucketsNum);
    SLEV(output[1], hashNum);
    uint32_t* buckets = output + 2;
    uint32_t* chains = output + bucketsNum + 2;
    std::fill(buckets, buckets + bucketsNum, 0U);
    std::fill(chains, chains + hashNum, STN_UNDEF);
    
    std::unique_ptr<uint32_t[]> lastNodes(new uint32_t[bucketsNum]);
    std::fill(lastNodes.get(), lastNodes.get() + bucketsNum, UINT32_MAX);
    for (uint32_t i = skipFirst; i < hashNum; i++)
    {
        const uint32_t bucket = hashCodes[i] % bucketsNum;
        if (lastNodes[bucket] == UINT32_MAX)
        {
            // first entry of chain
            SLEV(buckets[bucket], i);
            lastNodes[bucket] = i;
        }
        else
        {
            SLEV(chains[lastNodes[bucket]], i);
            lastNodes[bucket] = i;
        }
    }
}

template<typename Types>
void ElfBinaryGenTemplate<Types>::generate(FastOutputBuffer& fob)
{
    computeSize();
    const uint64_t startOffset = fob.getWritten();
    /* write elf header */
    {
        typename Types::Ehdr ehdr;
        ::memset(ehdr.e_ident, 0, EI_NIDENT);
        ehdr.e_ident[0] = 0x7f;
        ehdr.e_ident[1] = 'E';
        ehdr.e_ident[2] = 'L';
        ehdr.e_ident[3] = 'F';
        ehdr.e_ident[4] = Types::ELFCLASS;
        ehdr.e_ident[5] = ELFDATA2LSB;
        ehdr.e_ident[6] = EV_CURRENT;
        ehdr.e_ident[EI_OSABI] = header.osABI;
        ehdr.e_ident[EI_ABIVERSION] = header.abiVersion;
        SLEV(ehdr.e_type, header.type);
        SLEV(ehdr.e_machine, header.machine);
        SLEV(ehdr.e_version, header.version);
        SLEV(ehdr.e_flags, header.flags);
        if (header.entryRegion != UINT_MAX)
        {
            // if have entry
            typename Types::Word entry = regionOffsets[header.entryRegion] + header.entry;
            if (regions[header.entryRegion].type == ElfRegionType::SECTION &&
                regions[header.entryRegion].section.addrBase != 0)
            {
                auto addrBase = regions[header.entryRegion].section.addrBase;
                entry += addrBase != Types::nobase ? addrBase : 0;
            }
            else
                entry += header.vaddrBase;
            
            SLEV(ehdr.e_entry, entry);
        }
        else
            SLEV(ehdr.e_entry, 0);
        SLEV(ehdr.e_ehsize, sizeof(typename Types::Ehdr));
        // if no program headers then fill by zeroes, otherwise fill fields
        if (!progHeaders.empty())
        {
            SLEV(ehdr.e_phentsize, sizeof(typename Types::Phdr));
            SLEV(ehdr.e_phoff, regionOffsets[phdrTabRegion]);
        }
        else
        {
            SLEV(ehdr.e_phentsize, 0);
            SLEV(ehdr.e_phoff, 0);
        }
        SLEV(ehdr.e_phnum, progHeaders.size());
        SLEV(ehdr.e_shentsize, sizeof(typename Types::Shdr));
        SLEV(ehdr.e_shnum, sectionsNum);
        SLEV(ehdr.e_shoff, regionOffsets[shdrTabRegion]);
        SLEV(ehdr.e_shstrndx, shStrTab);
        
        fob.writeObject(ehdr);
    }
    
    size_t nullSymNameOffset = 0;
    // if addNullSym is not set, then no empty symbol name added, then we
    // find first null character
    if (!addNullSym && !symbols.empty())
        nullSymNameOffset = ::strlen(symbols[0].name);
    size_t nullDynSymNameOffset = 0;
    // if addNullDynSym is not set, then no empty dynamic symbol name added, then we
    // find first null character
    if (!addNullDynSym && !dynSymbols.empty())
        nullDynSymNameOffset = ::strlen(dynSymbols[0].name);
    // if addNullSection is not set, then no empty section name added, then we
    // find first null character
    size_t nullSectionNameOffset = 0;
    if (!addNullSection)
    {
        for (const ElfRegionTemplate<Types>& reg: regions)
            if (reg.type == ElfRegionType::SECTION)
            {
                nullSectionNameOffset = ::strlen(reg.section.name);
                break;
            }
    }
    
    /* write regions */
    for (size_t i = 0; i < regions.size(); i++)
    {   
        const ElfRegionTemplate<Types>& region = regions[i];
        // fix alignment
        uint64_t toFill = 0;
        typename Types::Word ralign = (region.type==ElfRegionType::SECTION) ?
                        region.section.align : 0;
        ralign = std::max(region.align, ralign);
        if (ralign > 1)
        {
            const uint64_t curOffset = (fob.getWritten()-startOffset);
            if (ralign!=0 && (curOffset&(ralign-1))!=0)
                toFill = ralign - (curOffset&(ralign-1));
            fob.fill(toFill, 0);
        }
        assert(regionOffsets[i] == fob.getWritten()-startOffset);
        
        // write content
        if (region.type == ElfRegionType::PHDR_TABLE)
        {
            /* write program headers */
            for (const auto& progHeader: progHeaders)
            {
                typename Types::Phdr phdr;
                SLEV(phdr.p_type, progHeader.type);
                SLEV(phdr.p_flags, progHeader.flags);
                const ElfRegionTemplate<Types> startRegion(sizeof(typename Types::Ehdr),
                        (const cxbyte*)nullptr, sizeof(typename Types::Word));
                // get first region of program header and it offset, index and address
                const ElfRegionTemplate<Types>& sregion = 
                        (progHeader.regionStart==PHREGION_FILESTART) ? startRegion :
                        regions[progHeader.regionStart];
                const cxuint rstart = (progHeader.regionStart!=PHREGION_FILESTART) ?
                            progHeader.regionStart : 0;
                const typename Types::Word sroffset =
                        (progHeader.regionStart!=PHREGION_FILESTART) ?
                            regionOffsets[progHeader.regionStart] : 0;
                const typename Types::Word sraddress =
                        (progHeader.regionStart!=PHREGION_FILESTART) ?
                            regionAddresses[progHeader.regionStart] : 0;
                
                // zero offset, allow to set zero offset of section
                bool zeroOffset = sregion.type == ElfRegionType::SECTION &&
                        sregion.section.zeroOffset;
                SLEV(phdr.p_offset, !zeroOffset ? sroffset : 0);
                if (progHeader.align==0 && progHeader.regionsNum==0)
                    SLEV(phdr.p_align, 0);
                else if (progHeader.align==0)
                {
                    typename Types::Word align = (sregion.type==ElfRegionType::SECTION) ?
                            sregion.section.align : 0;
                    align = std::max(sregion.align, align);
                    SLEV(phdr.p_align, align);
                }
                else
                    SLEV(phdr.p_align, progHeader.align);
                
                /* paddrBase and vaddrBase is base to program header virtual and physical
                 * addresses for program header. if not defined then get address base
                 * from ELF header */
                if (progHeader.paddrBase == Types::nobase)
                    SLEV(phdr.p_paddr, sraddress);
                else if (progHeader.paddrBase != 0)
                    SLEV(phdr.p_paddr, progHeader.paddrBase + sraddress);
                else if (header.paddrBase != 0)
                    SLEV(phdr.p_paddr, header.paddrBase + sraddress);
                else
                    SLEV(phdr.p_paddr, 0);
                
                // these same rule for vaddrBase
                if (progHeader.vaddrBase == Types::nobase)
                    SLEV(phdr.p_vaddr, sraddress);
                else if (progHeader.vaddrBase != 0)
                    SLEV(phdr.p_vaddr, progHeader.vaddrBase + sraddress);
                else if (header.vaddrBase != 0)
                    SLEV(phdr.p_vaddr, header.vaddrBase + sraddress);
                else
                    SLEV(phdr.p_vaddr, 0);
                
                // last region size for file - if nobits section then we assume zero size
                if (progHeader.regionsNum!=0)
                {
                    const auto& lastReg = regions[rstart + progHeader.regionsNum-1];
                    uint64_t fileLastRegSize =(lastReg.type!=ElfRegionType::SECTION ||
                        lastReg.section.type!=SHT_NOBITS) ? lastReg.size : 0;
                    /// fileSize - add offset of first region to simulate region alignment
                    const typename Types::Word fileSize = regionOffsets[rstart+
                            progHeader.regionsNum-1] + fileLastRegSize - sroffset;
                    const typename Types::Word phSize = regionAddresses[rstart+
                            progHeader.regionsNum-1]+regions[rstart+
                            progHeader.regionsNum-1].size - sraddress;
                    
                    if (progHeader.haveMemSize)
                    {
                        if (progHeader.memSize != 0)
                            SLEV(phdr.p_memsz, progHeader.memSize);
                        else
                            SLEV(phdr.p_memsz, phSize);
                    }
                    else
                        SLEV(phdr.p_memsz, 0);
                    SLEV(phdr.p_filesz, fileSize);
                }
                else
                {
                    SLEV(phdr.p_memsz, 0);
                    SLEV(phdr.p_filesz, 0);
                }
                fob.writeObject(phdr);
            }
        }
        else if (region.type == ElfRegionType::SHDR_TABLE)
        {
            /* write section headers table */
            if (addNullSection)
                fob.fill(sizeof(typename Types::Shdr), 0);
            uint32_t nameOffset = (addNullSection);
            for (cxuint j = 0; j < regions.size(); j++)
            {
                const auto& region2 = regions[j];
                if (region2.type == ElfRegionType::SECTION)
                {
                    typename Types::Shdr shdr;
                    if (region2.section.name!=nullptr && region2.section.name[0]!=0)
                        SLEV(shdr.sh_name, nameOffset);
                    else // set empty name offset
                        SLEV(shdr.sh_name, nullSectionNameOffset);
                    SLEV(shdr.sh_type, region2.section.type);
                    SLEV(shdr.sh_flags, region2.section.flags);
                    SLEV(shdr.sh_offset, (!region2.section.zeroOffset) ?
                                regionOffsets[j] : 0);
                    SLEV(shdr.sh_addr, resolveSectionAddress(header, region2,
                                     regionAddresses[j]));
                    
                    if (region2.align != 0 || j+1 >= regions.size() ||
                        regionOffsets[j]+region2.size == regionOffsets[j+1])
                        SLEV(shdr.sh_size, region2.size);
                    else // otherwise if not match this size
                        SLEV(shdr.sh_size, regionOffsets[j+1]-regionOffsets[j]);
                    
                    if ((region2.section.type!=SHT_SYMTAB &&
                         region2.section.type!=SHT_DYNSYM) ||
                            region2.section.info != BINGEN_DEFAULT)
                        // put set info
                        SLEV(shdr.sh_info, region2.section.info);
                    else // if symbtabs
                    {
                        // otherwise if default for symtabs, put count of last local
                        const auto& symbolsList = (region2.section.type == SHT_SYMTAB) ?
                            symbols : dynSymbols;
                        cxuint lastLocal = 0;
                        for (size_t l = 0; l < symbolsList.size(); l++)
                            if (ELF32_ST_BIND(symbolsList[l].info)==STB_LOCAL)
                                lastLocal = l+1;
                        if ((region2.section.type==SHT_SYMTAB && addNullSym) ||
                            (region2.section.type==SHT_DYNSYM && addNullDynSym))
                            lastLocal++;
                        SLEV(shdr.sh_info, lastLocal);
                    }
                    
                    SLEV(shdr.sh_addralign, (region2.section.align==0) ?
                            region2.align : region2.section.align);
                    if (region2.section.link == 0)
                    {
                        // set up link (for symtab is .strtab for .dynsym is dynstr)
                        if (::strcmp(region2.section.name, ".symtab") == 0)
                            SLEV(shdr.sh_link, strTab);
                        else if (::strcmp(region2.section.name, ".dynsym") == 0)
                            SLEV(shdr.sh_link, dynStr);
                        else // otherwise is value is link (zero)
                            SLEV(shdr.sh_link, region2.section.link);
                    }
                    else
                        SLEV(shdr.sh_link, region2.section.link);
                    
                    // set up entry size for sections
                    if (region2.section.type == SHT_SYMTAB ||
                        region2.section.type == SHT_DYNSYM)
                        SLEV(shdr.sh_entsize, sizeof(typename Types::Sym));
                    else if (region2.section.type == SHT_DYNAMIC)
                        SLEV(shdr.sh_entsize, sizeof(typename Types::Dyn));
                    else // if not default
                        SLEV(shdr.sh_entsize, region2.section.entSize);
                    if (region2.section.name!=nullptr && region2.section.name[0]!=0)
                        nameOffset += ::strlen(region2.section.name)+1;
                    fob.writeObject(shdr);
                }
            }
        }
        else if (region.type == ElfRegionType::USER)
        {
            if (region.dataFromPointer)
                fob.writeArray(region.size, region.data);
            else
                (*region.dataGen)(fob);
        }
        else if (region.type == ElfRegionType::SECTION)
        {
            if (region.data == nullptr)
            {
                if (region.section.type == SHT_SYMTAB || region.section.type == SHT_DYNSYM)
                {
                    uint32_t nameOffset = 0;
                    // put null symbol if addNullSym or addNumDynSym is true
                    if (region.section.type == SHT_SYMTAB && addNullSym)
                    {
                        fob.fill(sizeof(typename Types::Sym), 0);
                        nameOffset = 1;
                    }
                    if (region.section.type == SHT_DYNSYM && addNullDynSym)
                    {
                        fob.fill(sizeof(typename Types::Sym), 0);
                        nameOffset = 1;
                    }
                    const auto& symbolsList = (region.section.type == SHT_SYMTAB) ?
                            symbols : dynSymbols;
                    for (const auto& inSym: symbolsList)
                    {
                        typename Types::Sym sym;
                        if (inSym.name != nullptr && inSym.name[0] != 0)
                            SLEV(sym.st_name, nameOffset);
                        else  // set empty name offset (symbol or dynamic symbol)
                            SLEV(sym.st_name, (region.section.type == SHT_SYMTAB) ?
                                        nullSymNameOffset : nullDynSymNameOffset);
                        
                        SLEV(sym.st_shndx, inSym.sectionIndex);
                        SLEV(sym.st_size, inSym.size);
                        /// if value defined as address
                        if (!inSym.valueIsAddr)
                            SLEV(sym.st_value, inSym.value);
                        // if not use conversion to address with section addrBase
                        else if ((inSym.sectionIndex != 0 || !addNullSection) &&
                                regions[sectionRegions[
                                    inSym.sectionIndex]].section.addrBase != 0)
                        {
                            // store symbol value as address or value
                            typename Types::Word addrBase = regions[sectionRegions[
                                    inSym.sectionIndex]].section.addrBase;
                            SLEV(sym.st_value, inSym.value + regionOffsets[
                                    sectionRegions[inSym.sectionIndex]] +
                                    (addrBase!=Types::nobase ? addrBase : 0));
                        }
                        else if (header.vaddrBase!=Types::nobase)
                            // use elf headerf virtual address base
                            SLEV(sym.st_value, inSym.value + regionOffsets[
                                sectionRegions[inSym.sectionIndex]] +
                                (header.vaddrBase!=Types::nobase ? header.vaddrBase : 0));
                        sym.st_other = inSym.other;
                        sym.st_info = inSym.info;
                        if (inSym.name != nullptr && inSym.name[0] != 0)
                            nameOffset += ::strlen(inSym.name)+1;
                        fob.writeObject(sym);
                    }
                }
                else if (region.section.type == SHT_DYNAMIC)
                {
                    // dynamic table
                    typename Types::Dyn dyn;
                    for (size_t k = 0; k < dynamics.size(); k++)
                    {
                        SLEV(dyn.d_tag, dynamics[k]);
                        SLEV(dyn.d_un.d_val, dynamicValues[k]);
                        fob.writeObject(dyn);
                    }
                    SLEV(dyn.d_tag, DT_NULL);
                    SLEV(dyn.d_un.d_val, 0U);
                    fob.writeObject(dyn);
                }
                else if (region.section.type == SHT_HASH)
                {
                    // creating hash table and put it
                    const std::vector<ElfSymbolTemplate<Types> >& hashSymbols = 
                        (isHashDynSym) ? dynSymbols : symbols;
                    bool addNullHashSym = (isHashDynSym) ? addNullDynSym : addNullSym;
                    Array<uint32_t> hashTable(2 + hashSymbols.size() + addNullHashSym +
                                bucketsNum);
                    createHashTable(bucketsNum, hashSymbols.size()+addNullHashSym,
                                addNullHashSym, hashCodes.get(), hashTable.data());
                    fob.writeArray(hashTable.size(), hashTable.data());
                }
                else if (region.section.type == SHT_NOTE)
                {
                    // putting ELF notes
                    for (const ElfNote& note: notes)
                    {
                        typename Types::Nhdr nhdr;
                        size_t nameSize = ::strlen(note.name)+1;
                        size_t descSize = note.descSize;
                        SLEV(nhdr.n_namesz, nameSize);
                        SLEV(nhdr.n_descsz, descSize);
                        SLEV(nhdr.n_type, note.type);
                        fob.writeObject(nhdr);
                        fob.write(nameSize, note.name);
                        if ((nameSize&3) != 0)
                            fob.fill(4 - (nameSize&3), 0);
                        fob.writeArray(descSize, note.desc);
                        if ((descSize&3) != 0)
                            fob.fill(4 - (descSize&3), 0);
                    }
                }
                else if (region.section.type == SHT_STRTAB)
                {
                    // put symbol names and section names
                    if (::strcmp(region.section.name, ".strtab") == 0)
                    {
                        if (addNullSym)
                            fob.put(0);
                        for (const auto& sym: symbols)
                            if (sym.name != nullptr && sym.name[0] != 0)
                                fob.write(::strlen(sym.name)+1, sym.name);
                    }
                    else if (::strcmp(region.section.name, ".dynstr") == 0)
                    {
                        if (addNullDynSym)
                            fob.put(0);
                        for (const auto& sym: dynSymbols)
                            if (sym.name != nullptr && sym.name[0] != 0)
                                fob.write(::strlen(sym.name)+1, sym.name);
                    }
                    else if (::strcmp(region.section.name, ".shstrtab") == 0)
                    {
                        if (addNullSection)
                            fob.put(0);
                        for (const auto& region2: regions)
                            if (region2.type == ElfRegionType::SECTION &&
                                region2.section.name != nullptr &&
                                region2.section.name[0] != 0)
                                fob.write(::strlen(region2.section.name)+1,
                                          region2.section.name);
                    }
                }
            }
            else if (region.section.type != SHT_NOBITS)
            {
                if (region.dataFromPointer)
                    fob.writeArray(region.size, region.data);
                else
                    (*region.dataGen)(fob);
            }
        }
    }
    fob.flush();
    fob.getOStream().flush();
    assert(size == fob.getWritten()-startOffset);
}

template class CLRX::ElfBinaryGenTemplate<CLRX::Elf32Types>;
template class CLRX::ElfBinaryGenTemplate<CLRX::Elf64Types>;
