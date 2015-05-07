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
/*! \file ElfBinaries.h
 * \brief Elf binaries handling
 */

#ifndef __CLRX_ELFBINARIES_H__
#define __CLRX_ELFBINARIES_H__

#include <CLRX/Config.h>
#include <elf.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <ostream>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/InputOutput.h>

/* INFO: in this file is used ULEV function for conversion
 * from LittleEndian and unaligned access to other memory access policy and endianness
 * Please use this function whenever you want to get or set word in ELF binary,
 * because ELF binaries can be unaligned in memory (as inner binaries).
 */

/// main namespace
namespace CLRX
{

enum : cxuint {
    ELF_CREATE_SECTIONMAP = 1,  ///< create map of sections
    ELF_CREATE_SYMBOLMAP = 2,   ///< create map of symbols
    ELF_CREATE_DYNSYMMAP = 4,   ///< create map of dynamic symbols
    ELF_CREATE_ALL = 0xf, ///< creation flags for ELF binaries
};

struct Elf32Types
{
    typedef uint32_t Size;
    typedef uint32_t Word;
    typedef Elf32_Ehdr Ehdr;
    typedef Elf32_Shdr Shdr;
    typedef Elf32_Phdr Phdr;
    typedef Elf32_Sym Sym;
    static const cxbyte ELFCLASS;
    static const cxuint bitness;
    static const char* bitName;
};

struct Elf64Types
{
    typedef size_t Size;
    typedef uint64_t Word;
    typedef Elf64_Ehdr Ehdr;
    typedef Elf64_Shdr Shdr;
    typedef Elf64_Phdr Phdr;
    typedef Elf64_Sym Sym;
    static const cxbyte ELFCLASS;
    static const cxuint bitness;
    static const char* bitName;
};

/// ELF binary class
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
template<typename Types>
class ElfBinaryTemplate
{
public:
    /// section index map
    typedef Array<std::pair<const char*, size_t> > SectionIndexMap;
    /// symbol index map
    typedef Array<std::pair<const char*, size_t> > SymbolIndexMap;
protected:
    cxuint creationFlags;   ///< creation flags holder
    size_t binaryCodeSize;  ///< binary code size
    cxbyte* binaryCode;       ///< pointer to binary code
    cxbyte* sectionStringTable;   ///< pointer to section's string table
    cxbyte* symbolStringTable;    ///< pointer to symbol's string table
    cxbyte* symbolTable;          ///< pointer to symbol table
    cxbyte* dynSymStringTable;    ///< pointer to dynamic symbol's string table
    cxbyte* dynSymTable;          ///< pointer to dynamic symbol table
    SectionIndexMap sectionIndexMap;    ///< section's index map
    SymbolIndexMap symbolIndexMap;      ///< symbol's index map
    SymbolIndexMap dynSymIndexMap;      ///< dynamic symbol's index map
    
    typename Types::Size symbolsNum;    ///< symbols number
    typename Types::Size dynSymbolsNum; ///< dynamic symbols number
    uint16_t symbolEntSize; ///< symbol entry size in a symbol's table
    uint16_t dynSymEntSize; ///< dynamic symbol entry size in a dynamic symbol's table
    
public:
    ElfBinaryTemplate();
    /** constructor.
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    ElfBinaryTemplate(size_t binaryCodeSize, cxbyte* binaryCode,
                cxuint creationFlags = ELF_CREATE_ALL);
    virtual ~ElfBinaryTemplate();
    
    /// get creation flags
    cxuint getCreationFlags() const
    { return creationFlags; }
    
    /// returns true if object has a section's index map
    bool hasSectionMap() const
    { return (creationFlags & ELF_CREATE_SECTIONMAP) != 0; }
    
    /// returns true if object has a symbol's index map
    bool hasSymbolMap() const
    { return (creationFlags & ELF_CREATE_SYMBOLMAP) != 0; }
    
    /// returns true if object has a dynamic symbol's index map
    bool hasDynSymbolMap() const
    { return (creationFlags & ELF_CREATE_DYNSYMMAP) != 0; }
    
    /// get size of binaries
    size_t getSize() const
    { return binaryCodeSize; }
    
    /// returns true if object is initialized
    operator bool() const
    { return binaryCode!=nullptr; }
    
    /// returns true if object is uninitialized
    bool operator!() const
    { return binaryCode==nullptr; }
    
    const cxbyte* getBinaryCode() const
    { return binaryCode; }
    
    cxbyte* getBinaryCode()
    { return binaryCode; }
    
    /// get ELF binary header
    const typename Types::Ehdr& getHeader() const
    { return *reinterpret_cast<const typename Types::Ehdr*>(binaryCode); }
    
    /// get ELF binary header
    typename Types::Ehdr& getHeader()
    { return *reinterpret_cast<typename Types::Ehdr*>(binaryCode); }
    
    /// get section headers number
    uint16_t getSectionHeadersNum() const
    { return ULEV(getHeader().e_shnum); }
    
    /// get section header with specified index
    const typename Types::Shdr& getSectionHeader(uint16_t index) const
    {
        const typename Types::Ehdr& ehdr = getHeader();
        return *reinterpret_cast<const typename Types::Shdr*>(binaryCode +
                ULEV(ehdr.e_shoff) + size_t(ULEV(ehdr.e_shentsize))*index);
    }
    
    /// get section header with specified index
    typename Types::Shdr& getSectionHeader(uint16_t index)
    {
        const typename Types::Ehdr& ehdr = getHeader();
        return *reinterpret_cast<typename Types::Shdr*>(binaryCode +
                ULEV(ehdr.e_shoff) + size_t(ULEV(ehdr.e_shentsize))*index);
    }
    
    /// get program headers number
    uint16_t getProgramHeadersNum() const
    { return ULEV(getHeader().e_phnum); }
    
    /// get program header with specified index
    const typename Types::Phdr& getProgramHeader(uint16_t index) const
    {
        const typename Types::Ehdr& ehdr = getHeader();
        return *reinterpret_cast<const typename Types::Phdr*>(binaryCode +
                ULEV(ehdr.e_phoff) + size_t(ULEV(ehdr.e_phentsize))*index);
    }
    
    /// get program header with specified index
    typename Types::Phdr& getProgramHeader(uint16_t index)
    {
        const typename Types::Ehdr& ehdr = getHeader();
        return *reinterpret_cast<typename Types::Phdr*>(binaryCode +
                ULEV(ehdr.e_phoff) + size_t(ULEV(ehdr.e_phentsize))*index);
    }
    
    /// get symbols number
    typename Types::Size getSymbolsNum() const
    { return symbolsNum; }
    
    /// get dynamic symbols number
    typename Types::Size getDynSymbolsNum() const
    { return dynSymbolsNum; }
    
    /// get symbol with specified index
    const typename Types::Sym& getSymbol(typename Types::Size index) const
    {
        return *reinterpret_cast<const typename Types::Sym*>(symbolTable +
                    size_t(index)*symbolEntSize);
    }
    
    /// get symbol with specified index
    typename Types::Sym& getSymbol(typename Types::Size index)
    {
        return *reinterpret_cast<typename Types::Sym*>(
            symbolTable + size_t(index)*symbolEntSize);
    }
    
    /// get dynamic symbol with specified index
    const typename Types::Sym& getDynSymbol(typename Types::Size index) const
    {
        return *reinterpret_cast<const typename Types::Sym*>(dynSymTable +
            size_t(index)*dynSymEntSize);
    }
    
    /// get dynamic symbol with specified index
    typename Types::Sym& getDynSymbol(typename Types::Size index)
    {
        return *reinterpret_cast<typename Types::Sym*>(dynSymTable +
            size_t(index)*dynSymEntSize);
    }
    
    /// get symbol name with specified index
    const char* getSymbolName(typename Types::Size index) const
    {
        const typename Types::Sym& sym = getSymbol(index);
        return reinterpret_cast<const char*>(symbolStringTable + ULEV(sym.st_name));
    }
    
    /// get dynamic symbol name with specified index
    const char* getDynSymbolName(typename Types::Size index) const
    {
        const typename Types::Sym& sym = getDynSymbol(index);
        return reinterpret_cast<const char*>(dynSymStringTable + ULEV(sym.st_name));
    }
    
    /// get section name with specified index
    const char* getSectionName(uint16_t index) const
    {
        const typename Types::Shdr& section = getSectionHeader(index);
        return reinterpret_cast<const char*>(sectionStringTable + ULEV(section.sh_name));
    }
    
    /// get end iterator if section index map
    SectionIndexMap::const_iterator getSectionIterEnd() const
    { return sectionIndexMap.end(); }
    
    /// get section iterator with specified name (requires section index map)
    SectionIndexMap::const_iterator getSectionIter(const char* name) const
    {
        SectionIndexMap::const_iterator it = binaryMapFind(
                    sectionIndexMap.begin(), sectionIndexMap.end(), name, CStringLess());
        if (it == sectionIndexMap.end())
            throw Exception(std::string("Can't find Elf")+Types::bitName+" Section");
        return it;
    }
    
    /// get section index with specified name
    uint16_t getSectionIndex(const char* name) const;
    
    /// get symbol index with specified name (requires symbol index map)
    typename Types::Size getSymbolIndex(const char* name) const;
    
    /// get dynamic symbol index with specified name (requires dynamic symbol index map)
    typename Types::Size getDynSymbolIndex(const char* name) const;
    
    /// get end iterator of symbol index map
    SymbolIndexMap::const_iterator getSymbolIterEnd() const
    { return symbolIndexMap.end(); }
    
    /// get end iterator of dynamic symbol index map
    SymbolIndexMap::const_iterator getDynSymbolIterEnd() const
    { return dynSymIndexMap.end(); }
    
    /// get symbol iterator with specified name (requires symbol index map)
    SymbolIndexMap::const_iterator getSymbolIter(const char* name) const
    {
        SymbolIndexMap::const_iterator it = binaryMapFind(
                    symbolIndexMap.begin(), symbolIndexMap.end(), name, CStringLess());
        if (it == symbolIndexMap.end())
            throw Exception(std::string("Can't find Elf")+Types::bitName+" Symbol");
        return it;
    }

    /// get dynamic symbol iterator with specified name (requires dynamic symbol index map)
    SymbolIndexMap::const_iterator getDynSymbolIter(const char* name) const
    {
        SymbolIndexMap::const_iterator it = binaryMapFind(
                    dynSymIndexMap.begin(), dynSymIndexMap.end(), name, CStringLess());
        if (it == dynSymIndexMap.end())
            throw Exception(std::string("Can't find Elf")+Types::bitName+" DynSymbol");
        return it;
    }
    
    /// get section header with specified name
    const typename Types::Shdr& getSectionHeader(const char* name) const
    { return getSectionHeader(getSectionIndex(name)); }
    
    /// get section header with specified name
    typename Types::Shdr& getSectionHeader(const char* name)
    { return getSectionHeader(getSectionIndex(name)); }
    
    /// get symbol with specified name (requires symbol index map)
    const typename Types::Sym& getSymbol(const char* name) const
    { return getSymbol(getSymbolIndex(name)); }
    
    /// get symbol with specified name (requires symbol index map)
    typename Types::Sym& getSymbol(const char* name)
    { return getSymbol(getSymbolIndex(name)); }
    
    /// get dynamic symbol with specified name (requires dynamic symbol index map)
    const typename Types::Sym& getDynSymbol(const char* name) const
    { return getDynSymbol(getDynSymbolIndex(name)); }
    
    /// get dynamic symbol with specified name (requires dynamic symbol index map)
    typename Types::Sym& getDynSymbol(const char* name)
    { return getDynSymbol(getDynSymbolIndex(name)); }
    
    /// get section content pointer
    const cxbyte* getSectionContent(uint16_t index) const
    {
        const typename Types::Shdr& shdr = getSectionHeader(index);
        return binaryCode + ULEV(shdr.sh_offset);
    }
    
    /// get section content pointer
    cxbyte* getSectionContent(uint16_t index)
    {
        typename Types::Shdr& shdr = getSectionHeader(index);
        return binaryCode + ULEV(shdr.sh_offset);
    }
    
    /// get section content pointer
    const cxbyte* getSectionContent(const char* name) const
    {  return getSectionContent(getSectionIndex(name)); }
    
    /// get section content pointer
    cxbyte* getSectionContent(const char* name)
    {  return getSectionContent(getSectionIndex(name)); }
};

extern template class ElfBinaryTemplate<Elf32Types>;
extern template class ElfBinaryTemplate<Elf64Types>;

/// type for 32-bit ELF binary
typedef class ElfBinaryTemplate<Elf32Types> ElfBinary32;
/// type for 64-bit ELF binary
typedef class ElfBinaryTemplate<Elf64Types> ElfBinary64;

/* utilities for writing Elf binaries */

template<typename ElfSection, typename BOS>
inline void putElfSectionLE(BOS& bos, size_t shName, uint32_t shType,
        uint32_t shFlags, size_t offset, size_t size, uint32_t link,
        uint32_t info = 0, size_t addrAlign = 1, uint64_t addr = 0, size_t entSize = 0)
{
    ElfSection shdr;
    SLEV(shdr.sh_name, shName);
    SLEV(shdr.sh_type, shType);
    SLEV(shdr.sh_flags, shFlags);
    SLEV(shdr.sh_addr, addr);
    SLEV(shdr.sh_offset, offset);
    SLEV(shdr.sh_size, size);
    SLEV(shdr.sh_link, link);
    SLEV(shdr.sh_info, info);
    SLEV(shdr.sh_addralign, addrAlign);
    SLEV(shdr.sh_entsize, entSize);
    bos.writeObject(shdr);
}

template<typename ElfSym, typename BOS>
inline void putElfSymbolLE(BOS& bos, size_t symName, size_t value,
        size_t size, uint16_t shndx, cxbyte info, cxbyte other = 0)
{
    ElfSym sym;
    SLEV(sym.st_name, symName); 
    SLEV(sym.st_value, value);
    SLEV(sym.st_size, size);
    SLEV(sym.st_shndx, shndx);
    sym.st_info = info;
    sym.st_other = other;
    bos.writeObject(sym);
}

template<typename ElfProgHeader, typename BOS>
inline void putElfProgramHeaderLE(BOS& bos, uint32_t type, size_t offset,
        size_t filesz, uint32_t flags, size_t align, size_t memsz = 0,
        uint64_t vaddr = 0, uint64_t paddr = 0)
{
    ElfProgHeader phdr;
    SLEV(phdr.p_type, type);
    SLEV(phdr.p_offset, offset);
    SLEV(phdr.p_vaddr, vaddr);
    SLEV(phdr.p_paddr, paddr);
    SLEV(phdr.p_filesz, filesz);
    SLEV(phdr.p_memsz, memsz);
    SLEV(phdr.p_flags, flags);
    SLEV(phdr.p_align, align);
    bos.writeObject(phdr);
}

/// type of Elf region
enum class ElfRegionType: cxbyte
{
    PHDR_TABLE, ///< program header table
    SHDR_TABLE, ///< section header table
    SECTION,    ///< section
    USER        ///< user region
};

/// elf region content generator for elf region
class ElfRegionContent
{
public:
    virtual ~ElfRegionContent();
    
    /// operator that generates content
    virtual void operator()(CountableFastOutputBuffer& fob) const = 0;
};

/// elf header template
template<typename Types>
struct ElfHeaderTemplate
{
    typename Types::Word paddrBase; ///< physical address base
    typename Types::Word vaddrBase; ///< virtual address base
    cxbyte osABI;   ///< os abi
    cxbyte abiVersion;  ///< ABI version
    uint16_t type;  ///< type
    uint16_t machine;   ///< machine
    uint32_t version;   ///< version
    cxuint entryRegion; ///< region in which is entry
    typename Types::Word entry; ///< entry offset relative to region
    uint32_t flags; ///< flags
};

/// 32-bit elf header
typedef ElfHeaderTemplate<Elf32Types> ElfHeader32;
/// 64-bit elf header
typedef ElfHeaderTemplate<Elf64Types> ElfHeader64;

/// template of ElfRegion
template<typename Types>
struct ElfRegionTemplate
{
    ElfRegionType type; ///< type of region
    
    /// true if content from pointer, otherwise will be generated from class
    bool dataFromPointer;
    typename Types::Word size;   ///< region size
    typename Types::Word align;  ///< region alignment
    union
    {
        const cxbyte* data; ///< content from pointer
        const ElfRegionContent* dataGen;    ///< content generator pointer
    };
    struct {
        const char* name;   ///< section name
        uint32_t type;  ///< section type
        uint32_t flags; ///< section flags
        uint32_t link;  ///< section link
        uint32_t info;  ///< section info
        typename Types::Word addrBase;   ///< section address base
        typename Types::Word entSize;    ///< entries size
    } section;  ///< section structure
    
    /// constructor for user region
    ElfRegionTemplate(typename Types::Word inSize,
              const cxbyte* inData, typename Types::Word inAlign)
            : type(ElfRegionType::USER), dataFromPointer(true), size(inSize),
              align(inAlign), data(inData)
    { }
    
    /// constructor for user region with content generator
    ElfRegionTemplate(typename Types::Word inSize,
              const ElfRegionContent* contentGen, typename Types::Word inAlign)
            : type(ElfRegionType::USER), dataFromPointer(false), size(inSize),
              align(inAlign), dataGen(contentGen)
    { }
    
    /// constructor for region
    ElfRegionTemplate(ElfRegionType inType, typename Types::Word inSize,
              const cxbyte* inData, typename Types::Word inAlign)
            : type(inType), dataFromPointer(true), size(inSize),
              align(inAlign), data(inData)
    { }
    
    /// constructor for region with content generator
    ElfRegionTemplate(ElfRegionType inType, typename Types::Word inSize,
              const ElfRegionContent* contentGen, typename Types::Word inAlign)
            : type(inType), dataFromPointer(false), size(inSize),
              align(inAlign), dataGen(contentGen)
    { }
    
    /// constructor for section
    ElfRegionTemplate(typename Types::Word inSize, const cxbyte* inData,
              typename Types::Word inAlign, const char* inName, uint32_t inType,
              uint32_t inFlags, uint32_t inLink = 0, uint32_t inInfo = 0,
              typename Types::Word inAddrBase = 0,
              typename Types::Word inEntSize = 0)
            : type(ElfRegionType::SECTION), dataFromPointer(true), size(inSize),
              align(inAlign), data(inData)
    {
        section = {inName, inType, inFlags, inLink, inInfo, inAddrBase, inEntSize};
    }
    
    /// constructor for section with generator
    ElfRegionTemplate(typename Types::Word inSize, const ElfRegionContent* inData,
              typename Types::Word inAlign, const char* inName, uint32_t inType,
              uint32_t inFlags, uint32_t inLink = 0, uint32_t inInfo = 0,
              typename Types::Word inAddrBase = 0,
              typename Types::Word inEntSize = 0)
            : type(ElfRegionType::SECTION), dataFromPointer(false), size(inSize),
              align(inAlign), dataGen(inData)
    {
        section = {inName, inType, inFlags, inLink, inInfo, inAddrBase, inEntSize};
    }
    
    /// get program header table region
    static ElfRegionTemplate programHeaderTable()
    { return ElfRegionTemplate(ElfRegionType::PHDR_TABLE, 0, (const cxbyte*)nullptr, 0); }
    
    /// get program header table region
    static ElfRegionTemplate sectionHeaderTable()
    { return ElfRegionTemplate(ElfRegionType::SHDR_TABLE, 0, (const cxbyte*)nullptr, 0); }
    
    /// get .strtab section
    static ElfRegionTemplate strtabSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, 0, ".strtab", SHT_STRTAB, 0); }
    
    /// get .dynstr section
    static ElfRegionTemplate dynstrSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, 0, ".dynstr", SHT_STRTAB,
                SHF_ALLOC); }
    
    /// get .shstrtab section
    static ElfRegionTemplate shstrtabSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, 0, ".shstrtab", SHT_STRTAB, 0); }
    
    /// get symtab section
    static ElfRegionTemplate symtabSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, sizeof(typename Types::Word),
                ".symtab", SHT_SYMTAB, 0); }
    
    /// get dynsym section
    static ElfRegionTemplate dynsymSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr,  sizeof(typename Types::Word),
                ".dynsym", SHT_DYNSYM, 0); }
};

/// 32-bit region (for 32-bit elf)
typedef ElfRegionTemplate<Elf32Types> ElfRegion32;
/// 64-bit region (for 64-bit elf)
typedef ElfRegionTemplate<Elf64Types> ElfRegion64;

/// template of ELF program header
template<typename Types>
struct ElfProgramHeaderTemplate
{
    uint32_t type;  ///< type
    uint32_t flags; ///< flags
    cxuint regionStart; ///< number of first region which is in program header data
    cxuint regionsNum;  ///< number of regions whose is in program header data
    bool haveMemSize;   ///< true if program header has memory size
    typename Types::Word paddrBase;  ///< paddr base
    typename Types::Word vaddrBase;  ///< vaddr base
    typename Types::Word memSize;    ///< size in memory
};

/// 32-bit elf program header
typedef ElfProgramHeaderTemplate<Elf32Types> ElfProgramHeader32;
/// 64-bit elf program header
typedef ElfProgramHeaderTemplate<Elf64Types> ElfProgramHeader64;

/// ELF symbol template
template<typename Types>
struct ElfSymbolTemplate
{
    const char* name;   ///< name
    uint16_t sectionIndex;  ///< section index for which symbol is
    cxbyte info;    ///< info
    cxbyte other;   ///< other
    bool valueIsAddr;
    typename Types::Word value;  ///< symbol value
    typename Types::Word size;   ///< symbol size
};

/// 32-bit elf symbol
typedef ElfSymbolTemplate<Elf32Types> ElfSymbol32;
/// 64-bit elf symbol
typedef ElfSymbolTemplate<Elf64Types> ElfSymbol64;

/// ELF binary generator
template<typename Types>
class ElfBinaryGenTemplate
{
private:
    bool sizeComputed;
    uint16_t shStrTab, strTab, dynStr;
    cxuint shdrTabRegion, phdrTabRegion;
    uint16_t sectionsNum;
    typename Types::Word size;
    ElfHeaderTemplate<Types> header;
    std::vector<ElfRegionTemplate<Types> > regions;
    std::unique_ptr<typename Types::Word[]> regionOffsets;
    std::unique_ptr<cxuint[]> sectionRegions;
    std::vector<ElfProgramHeaderTemplate<Types> > progHeaders;
    std::vector<ElfSymbolTemplate<Types> > symbols;
    std::vector<ElfSymbolTemplate<Types> > dynSymbols;
    
    void computeSize();
public:
    /// construcrtor
    ElfBinaryGenTemplate(const ElfHeaderTemplate<Types>& header);
    
    void addRegion(const ElfRegionTemplate<Types>& region);
    void addProgramHeader(const ElfProgramHeaderTemplate<Types>& progHeader);
    
    void addSymbol(const ElfSymbolTemplate<Types>& symbol);
    void addDynSymbol(const ElfSymbolTemplate<Types>& symbol);
    
    uint64_t countSize();
    void generate(CountableFastOutputBuffer& fob);
};

extern template class ElfBinaryGenTemplate<Elf32Types>;
extern template class ElfBinaryGenTemplate<Elf64Types>;

/// type for 32-bit ELF binary generator
typedef class ElfBinaryGenTemplate<Elf32Types> ElfBinaryGen32;
/// type for 64-bit ELF binary generator
typedef class ElfBinaryGenTemplate<Elf64Types> ElfBinaryGen64;

}

#endif
