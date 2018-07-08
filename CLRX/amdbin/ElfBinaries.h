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
/*! \file ElfBinaries.h
 * \brief Elf binaries handling
 */

#ifndef __CLRX_ELFBINARIES_H__
#define __CLRX_ELFBINARIES_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <string>
#include <utility>
#include <ostream>
#include <CLRX/amdbin/Elf.h>
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

enum: cxuint {
    BINGEN_DEFAULT = UINT_MAX,    ///< if set in field then field has been filled later
    BINGEN_NOTSUPPLIED  = UINT_MAX-1 ///< if set in field then field has been ignored
};

enum: uint64_t {
    BINGEN64_DEFAULT = UINT64_MAX,    ///< if set in field then field has been filled later
    BINGEN64_NOTSUPPLIED  = UINT64_MAX-1 ///< if set in field then field has been ignored
};

enum: uint16_t {
    BINGEN16_DEFAULT = UINT16_MAX,    ///< if set in field then field has been filled later
    BINGEN16_NOTSUPPLIED  = UINT16_MAX-1 ///< if set in field then field has been ignored
};

enum: uint8_t {
    BINGEN8_DEFAULT = UINT8_MAX,    ///< if set in field then field has been filled later
    BINGEN8_NOTSUPPLIED  = UINT8_MAX-1 ///< if set in field then field has been ignored
};

enum : Flags {
    ELF_CREATE_SECTIONMAP = 1,  ///< create map of sections
    ELF_CREATE_SYMBOLMAP = 2,   ///< create map of symbols
    ELF_CREATE_DYNSYMMAP = 4,   ///< create map of dynamic symbols
    ELF_CREATE_ALL = 0xf  ///< creation flags for ELF binaries
};

/// Bin exception class
class BinException: public Exception
{
public:
    /// empty constructor
    BinException() = default;
    /// constructor with messasge
    explicit BinException(const std::string& message);
    /// destructor
    virtual ~BinException() noexcept = default;
};

/// Binary generator exception class
class BinGenException: public Exception
{
public:
    /// empty constructor
    BinGenException() = default;
    /// constructor with messasge
    explicit BinGenException(const std::string& message);
    /// destructor
    virtual ~BinGenException() noexcept = default;
};

/// ELF 32-bit types
struct Elf32Types
{
    typedef uint32_t Size;  ///< size used to return size value
    typedef uint32_t Word;  ///< word size in ELF
    typedef uint32_t SectionFlags;  ///< section flags
    typedef Elf32_Ehdr Ehdr;    ///< ELF header
    typedef Elf32_Shdr Shdr;    ///< Section header
    typedef Elf32_Phdr Phdr;    ///< program header
    typedef Elf32_Sym Sym;      ///< symbol header
    typedef Elf32_Nhdr Nhdr;    ///< note header
    typedef Elf32_Dyn Dyn;      ///< dynamic entry
    typedef Elf32_Rel Rel;    ///< relocation
    typedef Elf32_Rela Rela;    ///< relocation with addend
    static const cxbyte ELFCLASS;   ///< ELF class
    static const cxuint bitness;    ///< ELF bitness
    static const char* bitName;     ///< bitness name
    static const Word nobase = Word(0)-1;   ///< address with zero base
    static const cxuint relSymShift = 8;
};

/// ELF 32-bit types
struct Elf64Types
{
    typedef size_t Size;  ///< size used to return size value
    typedef uint64_t Word;  ///< word size in ELF
    typedef uint64_t SectionFlags;  ///< section flags
    typedef Elf64_Ehdr Ehdr;    ///< ELF header
    typedef Elf64_Shdr Shdr;    ///< Section header
    typedef Elf64_Phdr Phdr;    ///< program header
    typedef Elf64_Sym Sym;      ///< symbol header
    typedef Elf64_Nhdr Nhdr;    ///< note header
    typedef Elf64_Dyn Dyn;      ///< dynamic entry
    typedef Elf64_Rel Rel;    ///< relocation
    typedef Elf64_Rela Rela;    ///< relocation with addend
    static const cxbyte ELFCLASS;   ///< ELF class
    static const cxuint bitness;    ///< ELF bitness
    static const char* bitName;     ///< bitness name
    static const Word nobase = Word(0)-1;   ///< address with zero base
    static const cxuint relSymShift = 32;
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
    Flags creationFlags;   ///< creation flags holder
    size_t binaryCodeSize;  ///< binary code size
    cxbyte* binaryCode;       ///< pointer to binary code
    cxbyte* sectionStringTable;   ///< pointer to section's string table
    cxbyte* symbolStringTable;    ///< pointer to symbol's string table
    cxbyte* symbolTable;          ///< pointer to symbol table
    cxbyte* dynSymStringTable;    ///< pointer to dynamic symbol's string table
    cxbyte* dynSymTable;          ///< pointer to dynamic symbol table
    cxbyte* noteTable;            ///< pointer to note table
    cxbyte* dynamicTable;         ///< pointer to dynamic table
    SectionIndexMap sectionIndexMap;    ///< section's index map
    SymbolIndexMap symbolIndexMap;      ///< symbol's index map
    SymbolIndexMap dynSymIndexMap;      ///< dynamic symbol's index map
    
    typename Types::Size symbolsNum;    ///< symbols number
    typename Types::Size dynSymbolsNum; ///< dynamic symbols number
    typename Types::Size noteTableSize;        ///< size of note table
    typename Types::Size dynamicsNum;   ///< get dynamic entries number
    uint16_t symbolEntSize; ///< symbol entry size in a symbol's table
    uint16_t dynSymEntSize; ///< dynamic symbol entry size in a dynamic symbol's table
    typename Types::Size dynamicEntSize; ///< get dynamic entry size
    
public:
    ElfBinaryTemplate();
    /** constructor.
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    ElfBinaryTemplate(size_t binaryCodeSize, cxbyte* binaryCode,
                Flags creationFlags = ELF_CREATE_ALL);
    virtual ~ElfBinaryTemplate();
    
    /// get creation flags
    Flags getCreationFlags() const
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

    /// get binary code    
    const cxbyte* getBinaryCode() const
    { return binaryCode; }
    /// get binary code
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
            throw BinException(std::string("Can't find Elf")+Types::bitName+" Section");
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
            throw BinException(std::string("Can't find Elf")+Types::bitName+" Symbol");
        return it;
    }

    /// get dynamic symbol iterator with specified name (requires dynamic symbol index map)
    SymbolIndexMap::const_iterator getDynSymbolIter(const char* name) const
    {
        SymbolIndexMap::const_iterator it = binaryMapFind(
                    dynSymIndexMap.begin(), dynSymIndexMap.end(), name, CStringLess());
        if (it == dynSymIndexMap.end())
            throw BinException(std::string("Can't find Elf")+Types::bitName+" DynSymbol");
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
    
    /// get note table
    const typename Types::Nhdr* getNotes() const
    { return reinterpret_cast<typename Types::Nhdr*>(noteTable); }
    
    /// get note table
    typename Types::Nhdr* getNotes()
    { return reinterpret_cast<typename Types::Nhdr*>(noteTable); }
    
    /// get size of notes in bytes
    typename Types::Size getNotesSize() const
    { return noteTableSize; }
    
    /// get dynamic entries number
    const typename Types::Size getDynamicsNum() const
    { return dynamicsNum; }
    
    /// get dynamic entries size
    const typename Types::Size getDynamicEntrySize() const
    { return dynamicEntSize; }
    
    /// get dynamic table
    const typename Types::Dyn* getDynamicTable() const
    { return reinterpret_cast<const typename Types::Dyn*>(dynamicTable); }
    
    /// get dynamic table
    typename Types::Dyn* getDynamicTable()
    { return reinterpret_cast<typename Types::Dyn*>(dynamicTable); }
    
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
    
    static uint32_t getElfRelType(typename Types::Word info);
    static uint32_t getElfRelSym(typename Types::Word info);
};

template<>
inline uint32_t ElfBinaryTemplate<Elf32Types>::getElfRelType(typename Elf32Types::Word info)
{ return ELF32_R_TYPE(info); }

template<>
inline uint32_t ElfBinaryTemplate<Elf32Types>::getElfRelSym(typename Elf32Types::Word info)
{ return ELF32_R_SYM(info); }

template<>
inline uint32_t ElfBinaryTemplate<Elf64Types>::getElfRelType(typename Elf64Types::Word info)
{ return ELF64_R_TYPE(info); }

template<>
inline uint32_t ElfBinaryTemplate<Elf64Types>::getElfRelSym(typename Elf64Types::Word info)
{ return ELF64_R_SYM(info); }


extern template class ElfBinaryTemplate<Elf32Types>;
extern template class ElfBinaryTemplate<Elf64Types>;

/// check whether binary data is is ELF binary
extern bool isElfBinary(size_t binarySize, const cxbyte* binary);

/// type for 32-bit ELF binary
typedef class ElfBinaryTemplate<Elf32Types> ElfBinary32;
/// type for 64-bit ELF binary
typedef class ElfBinaryTemplate<Elf64Types> ElfBinary64;

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
    virtual void operator()(FastOutputBuffer& fob) const = 0;
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

/// type for Elf BinSection Id (sectionIndex)
typedef cxuint ElfBinSectId;

enum: ElfBinSectId {
    ELFSECTID_VAL_MAX = UINT_MAX,
    ELFSECTID_START = ELFSECTID_VAL_MAX-255,
    ELFSECTID_SHSTRTAB = ELFSECTID_START,
    ELFSECTID_STRTAB,
    ELFSECTID_SYMTAB,
    ELFSECTID_DYNSTR,
    ELFSECTID_DYNSYM,
    ELFSECTID_TEXT,
    ELFSECTID_RODATA,
    ELFSECTID_DATA,
    ELFSECTID_BSS,
    ELFSECTID_COMMENT,
    ELFSECTID_STD_MAX = ELFSECTID_COMMENT,
    ELFSECTID_OTHER_BUILTIN = ELFSECTID_STD_MAX+1,
    ELFSECTID_NULL = ELFSECTID_VAL_MAX-2,
    ELFSECTID_ABS = ELFSECTID_VAL_MAX-1,
    ELFSECTID_UNDEF = ELFSECTID_VAL_MAX
};

/// section structure to external usage (for example in the binary generator input)
struct BinSection
{
    CString name;   ///< name of section
    size_t size;    ///< size of content
    const cxbyte* data; ///< data content
    uint64_t align;  ///< region alignment
    uint32_t type;  ///< section type
    uint64_t flags; ///< section flags
    cxuint linkId; ///< link section id (ELFSECTID_* or an extra section index)
    uint32_t info;  ///< section info
    size_t entSize;    ///< entries size
};

/// symbol structure to external usage (fo example in the binary generator input)
struct BinSymbol
{
    CString name;   ///< name
    uint64_t value;  ///< symbol value
    uint64_t size;   ///< symbol size
    cxuint sectionId; ///< section id (ELFSECTID_* or an extra section index)
    bool valueIsAddr;   ///< true if value should be treats as address
    cxbyte info;    ///< info
    cxbyte other;   ///< other
};

/// convert section id to elf section id
extern uint16_t convertSectionId(cxuint sectionIndex, const uint16_t* builtinSections,
                  cxuint maxBuiltinSection, ElfBinSectId extraSectionIndex);

/// template of ElfRegion
template<typename Types>
struct ElfRegionTemplate
{
    ElfRegionType type; ///< type of region
    
    /// true if content from pointer, otherwise will be generated from class
    bool dataFromPointer;
    
    /// region size
    /** if alignment of this region is zero and size doesn't to alignment of next region
     * then size of this region will be fixed to alignment of next region */
    typename Types::Word size;
    typename Types::Word align;  ///< region alignment
    union
    {
        const cxbyte* data; ///< content from pointer
        const ElfRegionContent* dataGen;    ///< content generator pointer
    };
    struct {
        const char* name;   ///< section name
        uint32_t type;  ///< section type
        typename Types::SectionFlags flags; ///< section flags
        uint32_t link;  ///< section link
        uint32_t info;  ///< section info
        typename Types::Word addrBase;   ///< section address base
        typename Types::Word entSize;    ///< entries size
        bool zeroOffset;
        typename Types::Word align;
    } section;  ///< section structure
    
    ElfRegionTemplate() : type(ElfRegionType::USER), dataFromPointer(false), size(0),
              align(0), data(0)
    { }
    /// constructor for user region
    ElfRegionTemplate(typename Types::Word _size,
              const cxbyte* _data, typename Types::Word _align)
            : type(ElfRegionType::USER), dataFromPointer(true), size(_size),
              align(_align), data(_data)
    { }
    
    /// constructor for user region with content generator
    ElfRegionTemplate(typename Types::Word _size,
              const ElfRegionContent* contentGen, typename Types::Word _align)
            : type(ElfRegionType::USER), dataFromPointer(false), size(_size),
              align(_align), dataGen(contentGen)
    { }
    
    /// constructor for region
    ElfRegionTemplate(ElfRegionType _type, typename Types::Word _size,
              const cxbyte* _data, typename Types::Word _align)
            : type(_type), dataFromPointer(true), size(_size),
              align(_align), data(_data)
    { }
    
    /// constructor for region with content generator
    ElfRegionTemplate(ElfRegionType _type, typename Types::Word _size,
              const ElfRegionContent* contentGen, typename Types::Word _align)
            : type(_type), dataFromPointer(false), size(_size),
              align(_align), dataGen(contentGen)
    { }
    
    /// constructor for section
    ElfRegionTemplate(typename Types::Word _size, const cxbyte* _data,
              typename Types::Word _align, const char* _name, uint32_t _type,
              typename Types::SectionFlags _flags, uint32_t _link = 0, uint32_t _info = 0,
              typename Types::Word _addrBase = 0,
              typename Types::Word _entSize = 0, bool _zeroOffset = false,
              typename Types::Word _sectAlign = 0)
            : type(ElfRegionType::SECTION), dataFromPointer(true), size(_size),
              align(_align), data(_data)
    {
        section = {_name, _type, _flags, _link, _info, _addrBase,
            _entSize, _zeroOffset, _sectAlign};
    }
    
    /// constructor for section with generator
    ElfRegionTemplate(typename Types::Word _size, const ElfRegionContent* _data,
              typename Types::Word _align, const char* inName, uint32_t _type,
              typename Types::SectionFlags _flags, uint32_t _link = 0, uint32_t _info = 0,
              typename Types::Word _addrBase = 0,
              typename Types::Word _entSize = 0, bool _zeroOffset = false,
              typename Types::Word _sectAlign = 0)
            : type(ElfRegionType::SECTION), dataFromPointer(false), size(_size),
              align(_align), dataGen(_data)
    {
        section = {inName, _type, _flags, _link, _info, _addrBase,
            _entSize, _zeroOffset, _sectAlign};
    }
    /// constructor for external section (BinSection)
    /**
     * \param binSection external section
     * \param builtinSections ELF section indices for builtin sections
     * \param maxBuiltinSection maximal id of builtin section (as ELFSECTID_STD_MAX)
     * \param startExtraIndex first ELF section id for extra section
     */
    ElfRegionTemplate(const BinSection& binSection, const uint16_t* builtinSections,
                  cxuint maxBuiltinSection, ElfBinSectId startExtraIndex)
            : type(ElfRegionType::SECTION), dataFromPointer(true), size(binSection.size),
              align(binSection.align), data(binSection.data)
    {
        section = { binSection.name.c_str(), binSection.type, 
            typename Types::SectionFlags(binSection.flags),
            uint32_t(convertSectionId(binSection.linkId, builtinSections,
                             maxBuiltinSection, startExtraIndex)),
            binSection.info, 0, typename Types::Word(binSection.entSize), 0 };
    }
    
    /// get program header table region
    static ElfRegionTemplate programHeaderTable()
    { return ElfRegionTemplate(ElfRegionType::PHDR_TABLE, 0, (const cxbyte*)nullptr,
               sizeof(typename Types::Word)); }
    
    /// get program header table region
    static ElfRegionTemplate sectionHeaderTable()
    { return ElfRegionTemplate(ElfRegionType::SHDR_TABLE, 0, (const cxbyte*)nullptr,
                sizeof(typename Types::Word)); }
    
    /// get .strtab section
    static ElfRegionTemplate strtabSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, 1, ".strtab", SHT_STRTAB, 0); }
    
    /// get .dynstr section
    static ElfRegionTemplate dynstrSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, 1, ".dynstr", SHT_STRTAB,
                SHF_ALLOC); }
    
    /// get .shstrtab section
    static ElfRegionTemplate shstrtabSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, 1, ".shstrtab", SHT_STRTAB, 0); }
    
    /// get symtab section
    static ElfRegionTemplate symtabSection(bool defInfo = true)
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, sizeof(typename Types::Word),
                ".symtab", SHT_SYMTAB, 0, 0, defInfo ? BINGEN_DEFAULT : 0); }
    
    /// get dynsym section
    static ElfRegionTemplate dynsymSection(bool defInfo = true)
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, sizeof(typename Types::Word),
                ".dynsym", SHT_DYNSYM, SHF_ALLOC, 0, defInfo ? BINGEN_DEFAULT : 0); }
    
    /// get hash section
    static ElfRegionTemplate hashSection(uint16_t link)
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, sizeof(typename Types::Word),
                ".hash", SHT_HASH, SHF_ALLOC, link); }
    
    /// get note section
    static ElfRegionTemplate noteSection()
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, 4, ".note", SHT_NOTE, 0); }
    
    /// get dynamic
    static ElfRegionTemplate dynamicSection(uint16_t link)
    { return ElfRegionTemplate(0, (const cxbyte*)nullptr, sizeof(typename Types::Word),
                ".dynamic", SHT_DYNAMIC, SHF_ALLOC|SHF_WRITE, link); }
};

/// ELF note structure
struct ElfNote
{
    const char* name;   ///< note name
    size_t descSize;    ///< description size
    const cxbyte* desc; ///< description
    uint32_t type;      ///< type
};

/// 32-bit region (for 32-bit elf)
typedef ElfRegionTemplate<Elf32Types> ElfRegion32;
/// 64-bit region (for 64-bit elf)
typedef ElfRegionTemplate<Elf64Types> ElfRegion64;

enum: cxuint {
    PHREGION_FILESTART = UINT_MAX
};
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
    typename Types::Word align;      ///< alignment
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
    bool valueIsAddr;   ///< true if value should be treats as address
    typename Types::Word value;  ///< symbol value
    typename Types::Word size;   ///< symbol size
    
    ElfSymbolTemplate() : name(nullptr), sectionIndex(0), info(0), other(0),
          valueIsAddr(false), value(0), size(0)
    { }
    
    /// constructor (to replace initializer list construction)
    ElfSymbolTemplate(const char* _name, uint16_t _sectionIndex,
                cxbyte _info, cxbyte _other, bool _valueIsAddr,
                typename Types::Word _value, typename Types::Word _size)
        : name(_name), sectionIndex(_sectionIndex), info(_info), other(_other),
          valueIsAddr(_valueIsAddr), value(_value), size(_size)
    { }
    /// constructor for extra symbol
    /**
     * \param binSymbol external symbol
     * \param builtinSections ELF section indices for builtin sections
     * \param maxBuiltinSection maximal id of builtin section (as ELFSECTID_STD_MAX)
     * \param startExtraIndex first ELF section id for extra section
     */
    ElfSymbolTemplate(const BinSymbol& binSymbol, const uint16_t* builtinSections,
                  cxuint maxBuiltinSection, ElfBinSectId startExtraIndex)
    {
        name = binSymbol.name.c_str();
        sectionIndex = convertSectionId(binSymbol.sectionId, builtinSections,
                    maxBuiltinSection, startExtraIndex);
        info = binSymbol.info;
        other = binSymbol.other;
        valueIsAddr = binSymbol.valueIsAddr;
        value = binSymbol.value;
        size = binSymbol.size;
    }
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
    bool addNullSym, addNullDynSym;
    bool addNullSection;
    cxuint addrStartRegion;
    uint16_t shStrTab, strTab, dynStr;
    cxuint shdrTabRegion, phdrTabRegion;
    uint16_t sectionsNum;
    typename Types::Word size;
    ElfHeaderTemplate<Types> header;
    std::vector<ElfRegionTemplate<Types> > regions;
    std::unique_ptr<typename Types::Word[]> regionOffsets;
    std::unique_ptr<typename Types::Word[]> regionAddresses;
    std::unique_ptr<cxuint[]> sectionRegions;
    std::vector<ElfProgramHeaderTemplate<Types> > progHeaders;
    std::vector<ElfSymbolTemplate<Types> > symbols;
    std::vector<ElfSymbolTemplate<Types> > dynSymbols;
    std::vector<ElfNote> notes;
    std::vector<int32_t> dynamics;
    std::unique_ptr<typename Types::Word[]> dynamicValues;
    uint32_t bucketsNum;
    std::unique_ptr<uint32_t[]> hashCodes;
    bool isHashDynSym;
    
    void computeSize();
public:
    ElfBinaryGenTemplate();
    /// construcrtor
    /**
     * \param header elf header template
     * \param addNullSym if true then add null symbol to symbol table
     * \param addNullDynSym if true then add null dynsymbol to dynsymbol table
     * \param addNullSection if true then add null section to section table
     * \param addrCountingFromRegion begins counting address from region with that index
     */
    explicit ElfBinaryGenTemplate(const ElfHeaderTemplate<Types>& header,
            bool addNullSym = true, bool addNullDynSym = true,
            bool addNullSection = true, cxuint addrCountingFromRegion = 0);
    
    /// set elf header
    void setHeader(const ElfHeaderTemplate<Types>& header)
    { this->header = header; }
    
    /// add new region (section, user region or shdr/phdr table
    void addRegion(const ElfRegionTemplate<Types>& region);
    /// add new program header
    void addProgramHeader(const ElfProgramHeaderTemplate<Types>& progHeader);
    
    /// clear symbols
    void clearSymbols()
    { symbols.clear(); }
    /// clear dynamic symbols
    void clearDynSymbols()
    { dynSymbols.clear(); }
    /// add symbol
    void addSymbol(const ElfSymbolTemplate<Types>& symbol)
    { symbols.push_back(symbol); }
    /// add dynamic symbol
    void addDynSymbol(const ElfSymbolTemplate<Types>& symbol)
    { dynSymbols.push_back(symbol); }
    /// add note
    void addNote(const ElfNote& note)
    { notes.push_back(note); }
    /// add dynamic
    void addDynamic(int32_t dynamicTag)
    { dynamics.push_back(dynamicTag); }
    /// add dynamic
    void addDynamics(size_t dynamicsNum, const int32_t* dynTags)
    { dynamics.insert(dynamics.end(), dynTags, dynTags + dynamicsNum); }
    
    /// count size of binary
    uint64_t countSize();
    
    // return offset for specified region
    typename Types::Word getRegionOffset(cxuint i) const
    { return regionOffsets[i]; }
    
    /// generate binary
    void generate(FastOutputBuffer& fob);
    
    /// generate binary
    void generate(std::ostream& os)
    {
        FastOutputBuffer fob(256, os);
        generate(fob);
    }
    
    static typename Types::Word getRelInfo(size_t symbolIndex, uint32_t rtype);
};

template<>
inline uint32_t ElfBinaryGenTemplate<Elf32Types>::getRelInfo(
            size_t symbolIndex, uint32_t rtype)
{ return ELF32_R_INFO(symbolIndex, rtype); }

template<>
inline uint64_t ElfBinaryGenTemplate<Elf64Types>::getRelInfo(
            size_t symbolIndex, uint32_t rtype)
{ return ELF64_R_INFO(symbolIndex, rtype); }

extern template class ElfBinaryGenTemplate<Elf32Types>;
extern template class ElfBinaryGenTemplate<Elf64Types>;

/// type for 32-bit ELF binary generator
typedef class ElfBinaryGenTemplate<Elf32Types> ElfBinaryGen32;
/// type for 64-bit ELF binary generator
typedef class ElfBinaryGenTemplate<Elf64Types> ElfBinaryGen64;

};

#endif
