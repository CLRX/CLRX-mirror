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
#include <cstring>
#include <elf.h>
#include <cstdint>
#include <utility>
#include <string>
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

const uint8_t CLRX::Elf32Types::ELFCLASS = ELFCLASS32;
const uint32_t CLRX::Elf32Types::bitness = 32;
const char* CLRX::Elf32Types::bitName = "32";

/* elf64 types */

const uint8_t CLRX::Elf64Types::ELFCLASS = ELFCLASS64;
const cxuint CLRX::Elf64Types::bitness = 64;
const char* CLRX::Elf64Types::bitName = "64";

/* ElfBinaryTemplate */

template<typename Types>
ElfBinaryTemplate<Types>::ElfBinaryTemplate() : binaryCodeSize(0), binaryCode(nullptr),
        sectionStringTable(nullptr), symbolStringTable(nullptr),
        symbolTable(nullptr), dynSymStringTable(nullptr), dynSymTable(nullptr),
        symbolsNum(0), dynSymbolsNum(0),
        symbolEntSize(0), dynSymEntSize(0)
{ }

template<typename Types>
ElfBinaryTemplate<Types>::~ElfBinaryTemplate()
{ }

template<typename Types>
ElfBinaryTemplate<Types>::ElfBinaryTemplate(size_t binaryCodeSize, cxbyte* binaryCode,
             cxuint creationFlags) :
        binaryCodeSize(0), binaryCode(nullptr),
        sectionStringTable(nullptr), symbolStringTable(nullptr),
        symbolTable(nullptr), dynSymStringTable(nullptr), dynSymTable(nullptr),
        symbolsNum(0), dynSymbolsNum(0), symbolEntSize(0), dynSymEntSize(0)
{
    this->creationFlags = creationFlags;
    this->binaryCode = binaryCode;
    this->binaryCodeSize = binaryCodeSize;
    
    if (binaryCodeSize < sizeof(typename Types::Ehdr))
        throw Exception("Binary is too small!!!");
    
    const typename Types::Ehdr* ehdr =
            reinterpret_cast<const typename Types::Ehdr*>(binaryCode);
    
    if (ULEV(*reinterpret_cast<const uint32_t*>(binaryCode)) != elfMagicValue)
        throw Exception("This is not ELF binary");
    if (ehdr->e_ident[EI_CLASS] != Types::ELFCLASS)
        throw Exception(std::string("This is not ")+Types::bitName+"bit ELF binary");
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        throw Exception("Other than little-endian binaries are not supported!");
    
    if ((ULEV(ehdr->e_phoff) == 0 && ULEV(ehdr->e_phnum) != 0))
        throw Exception("Elf invalid phoff and phnum combination");
    if (ULEV(ehdr->e_phoff) != 0)
    {   /* reading and checking program headers */
        if (ULEV(ehdr->e_phoff) > binaryCodeSize)
            throw Exception("ProgramHeaders offset out of range!");
        if (usumGt(ULEV(ehdr->e_phoff),
                   ((typename Types::Word)ULEV(ehdr->e_phentsize))*ULEV(ehdr->e_phnum),
                   binaryCodeSize))
            throw Exception("ProgramHeaders offset+size out of range!");
        
        cxuint phnum = ULEV(ehdr->e_phnum);
        for (cxuint i = 0; i < phnum; i++)
        {
            const typename Types::Phdr& phdr = getProgramHeader(i);
            if (ULEV(phdr.p_offset) > binaryCodeSize)
                throw Exception("Segment offset out of range!");
            if (usumGt(ULEV(phdr.p_offset), ULEV(phdr.p_filesz), binaryCodeSize))
                throw Exception("Segment offset+size out of range!");
        }
    }
    
    if ((ULEV(ehdr->e_shoff) == 0 && ULEV(ehdr->e_shnum) != 0))
        throw Exception("Elf invalid shoff and shnum combination");
    if (ULEV(ehdr->e_shoff) != 0 && ULEV(ehdr->e_shstrndx) != SHN_UNDEF)
    {   /* indexing of sections */
        if (ULEV(ehdr->e_shoff) > binaryCodeSize)
            throw Exception("SectionHeaders offset out of range!");
        if (usumGt(ULEV(ehdr->e_shoff),
                  ((typename Types::Word)ULEV(ehdr->e_shentsize))*ULEV(ehdr->e_shnum),
                  binaryCodeSize))
            throw Exception("SectionHeaders offset+size out of range!");
        if (ULEV(ehdr->e_shstrndx) >= ULEV(ehdr->e_shnum))
            throw Exception("Shstrndx out of range!");
        
        typename Types::Shdr& shstrShdr = getSectionHeader(ULEV(ehdr->e_shstrndx));
        sectionStringTable = binaryCode + ULEV(shstrShdr.sh_offset);
        const size_t unfinishedShstrPos = unfinishedRegionOfStringTable(
                    sectionStringTable, ULEV(shstrShdr.sh_size));
        
        const typename Types::Shdr* symTableHdr = nullptr;
        const typename Types::Shdr* dynSymTableHdr = nullptr;
        
        cxuint shnum = ULEV(ehdr->e_shnum);
        if ((creationFlags & ELF_CREATE_SECTIONMAP) != 0)
            sectionIndexMap.resize(shnum);
        for (cxuint i = 0; i < shnum; i++)
        {
            const typename Types::Shdr& shdr = getSectionHeader(i);
            if (ULEV(shdr.sh_offset) > binaryCodeSize)
                throw Exception("Section offset out of range!");
            if (ULEV(shdr.sh_type) != SHT_NOBITS)
                if (usumGt(ULEV(shdr.sh_offset), ULEV(shdr.sh_size), binaryCodeSize))
                    throw Exception("Section offset+size out of range!");
            if (ULEV(shdr.sh_link) >= ULEV(ehdr->e_shnum))
                throw Exception("Section link out of range!");
            
            const typename Types::Size sh_nameindx = ULEV(shdr.sh_name);
            if (sh_nameindx >= ULEV(shstrShdr.sh_size))
                throw Exception("Section name index out of range!");
            
            if (sh_nameindx >= unfinishedShstrPos)
                throw Exception("Unfinished section name!");
            
            const char* shname =
                reinterpret_cast<const char*>(sectionStringTable + sh_nameindx);
            
            if ((creationFlags & ELF_CREATE_SECTIONMAP) != 0)
                sectionIndexMap[i] = std::make_pair(shname, i);
            if (ULEV(shdr.sh_type) == SHT_SYMTAB)
                symTableHdr = &shdr;
            if (ULEV(shdr.sh_type) == SHT_DYNSYM)
                dynSymTableHdr = &shdr;
        }
        if ((creationFlags & ELF_CREATE_SECTIONMAP) != 0)
            mapSort(sectionIndexMap.begin(), sectionIndexMap.end(), CStringLess());
        
        if (symTableHdr != nullptr)
        {   // indexing symbols
            if (ULEV(symTableHdr->sh_entsize) < sizeof(typename Types::Sym))
                throw Exception("SymTable entry size is too small!");
            
            symbolEntSize = ULEV(symTableHdr->sh_entsize);
            symbolTable = binaryCode + ULEV(symTableHdr->sh_offset);
            if (ULEV(symTableHdr->sh_link) == SHN_UNDEF)
                throw Exception("Symbol table doesnt have string table");
            
            typename Types::Shdr& symstrShdr = getSectionHeader(ULEV(symTableHdr->sh_link));
            symbolStringTable = binaryCode + ULEV(symstrShdr.sh_offset);
            
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    symbolStringTable, ULEV(symstrShdr.sh_size));
            symbolsNum = ULEV(symTableHdr->sh_size)/ULEV(symTableHdr->sh_entsize);
            if ((creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                symbolIndexMap.resize(symbolsNum);
            
            for (typename Types::Size i = 0; i < symbolsNum; i++)
            {   /* verify symbol names */
                const typename Types::Sym& sym = getSymbol(i);
                const typename Types::Size symnameindx = ULEV(sym.st_name);
                if (symnameindx >= ULEV(symstrShdr.sh_size))
                    throw Exception("Symbol name index out of range!");
                if (symnameindx >= unfinishedSymstrPos)
                    throw Exception("Unfinished symbol name!");
                
                const char* symname =
                    reinterpret_cast<const char*>(symbolStringTable + symnameindx);
                // add to symbol map
                if ((creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                    symbolIndexMap[i] = std::make_pair(symname, i);
            }
            if ((creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                mapSort(symbolIndexMap.begin(), symbolIndexMap.end(), CStringLess());
        }
        if (dynSymTableHdr != nullptr)
        {   // indexing dynamic symbols
            if (ULEV(dynSymTableHdr->sh_entsize) < sizeof(typename Types::Sym))
                throw Exception("DynSymTable entry size is too small!");
            
            dynSymEntSize = ULEV(dynSymTableHdr->sh_entsize);
            dynSymTable = binaryCode + ULEV(dynSymTableHdr->sh_offset);
            if (ULEV(dynSymTableHdr->sh_link) == SHN_UNDEF)
                throw Exception("DynSymbol table doesnt have string table");
            
            typename Types::Shdr& dynSymstrShdr =
                    getSectionHeader(ULEV(dynSymTableHdr->sh_link));
            dynSymbolsNum = ULEV(dynSymTableHdr->sh_size)/ULEV(dynSymTableHdr->sh_entsize);
            
            dynSymStringTable = binaryCode + ULEV(dynSymstrShdr.sh_offset);
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    dynSymStringTable, ULEV(dynSymstrShdr.sh_size));
            
            if ((creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                dynSymIndexMap.resize(dynSymbolsNum);
            
            for (typename Types::Size i = 0; i < dynSymbolsNum; i++)
            {   /* verify symbol names */
                const typename Types::Sym& sym = getDynSymbol(i);
                const typename Types::Size symnameindx = ULEV(sym.st_name);
                if (symnameindx >= ULEV(dynSymstrShdr.sh_size))
                    throw Exception("DynSymbol name index out of range!");
                if (symnameindx >= unfinishedSymstrPos)
                    throw Exception("Unfinished dynsymbol name!");
                
                const char* symname =
                    reinterpret_cast<const char*>(dynSymStringTable + symnameindx);
                // add to symbol map
                if ((creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                    dynSymIndexMap[i] = std::make_pair(symname, i);
            }
            if ((creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                mapSort(dynSymIndexMap.begin(), dynSymIndexMap.end(), CStringLess());
        }
    }
}

template<typename Types>
uint16_t ElfBinaryTemplate<Types>::getSectionIndex(const char* name) const
{
    if (hasSectionMap())
    {
        SectionIndexMap::const_iterator it = binaryMapFind(
                    sectionIndexMap.begin(), sectionIndexMap.end(), name, CStringLess());
        if (it == sectionIndexMap.end())
            throw Exception(std::string("Can't find Elf")+Types::bitName+" Section");
        return it->second;
    }
    else
    {
        for (cxuint i = 0; i < getSectionHeadersNum(); i++)
        {
            if (::strcmp(getSectionName(i), name) == 0)
                return i;
        }
        throw Exception(std::string("Can't find Elf")+Types::bitName+" Section");
    }
}

template<typename Types>
typename Types::Size ElfBinaryTemplate<Types>::getSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = binaryMapFind(
                    symbolIndexMap.begin(), symbolIndexMap.end(), name, CStringLess());
    if (it == symbolIndexMap.end())
        throw Exception(std::string("Can't find Elf")+Types::bitName+" Symbol");
    return it->second;
}

template<typename Types>
typename Types::Size ElfBinaryTemplate<Types>::getDynSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = binaryMapFind(
                    dynSymIndexMap.begin(), dynSymIndexMap.end(), name, CStringLess());
    if (it == dynSymIndexMap.end())
        throw Exception(std::string("Can't find Elf")+Types::bitName+" DynSymbol");
    return it->second;
}

template class CLRX::ElfBinaryTemplate<CLRX::Elf32Types>;
template class CLRX::ElfBinaryTemplate<CLRX::Elf64Types>;
