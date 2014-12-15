/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
/*! \file AmdBinaries.h
 * \brief AMD binaries handling
 */

#ifndef __CLRX_AMDBINARIES_H__
#define __CLRX_AMDBINARIES_H__

#include <CLRX/Config.h>
#include <elf.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <CLRX/MemAccess.h>
#include <CLRX/Utilities.h>

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
    AMDBIN_INNER_INT_CREATE_ALL = 0xff,
    AMDBIN_CREATE_CALNOTES = 0x10, ///< only for internal usage
    
    AMDBIN_CREATE_KERNELINFO = 0x10,    ///< create kernel informations
    AMDBIN_CREATE_KERNELINFOMAP = 0x20, ///< create map of kernel informations
    AMDBIN_CREATE_INNERBINMAP = 0x40,   ///< create map of inner binaries
    AMDBIN_CREATE_INFOSTRINGS = 0x80, ///< create compile options and driver info
    AMDBIN_CREATE_KERNELHEADERS = 0x100,    ///< create kernel headers
    AMDBIN_CREATE_KERNELHEADERMAP = 0x200,    ///< create kernel headers map
    AMDBIN_INNER_CREATE_SECTIONMAP = 0x1000, ///< create map of sections for inner binaries
    AMDBIN_INNER_CREATE_SYMBOLMAP = 0x2000,  ///< create map of symbols for inner binaries
    /** create map of dynamic symbols for inner binaries */
    AMDBIN_INNER_CREATE_DYNSYMMAP = 0x4000,
    AMDBIN_INNER_CREATE_CALNOTES = 0x10000, ///< create CAL notes for AMD inner GPU binary
    
    AMDBIN_CREATE_ALL = ELF_CREATE_ALL | 0xffff0, ///< all AMD binaries creation flags
    AMDBIN_INNER_SHIFT = 12 ///< shift for convert inner binary flags into elf binary flags
};

enum : cxuint {
    ELF_M_X86 = 0x7d4
};

/// kernel argument type
enum class KernelArgType : uint8_t
{
    VOID = 0,
    UCHAR, CHAR, USHORT, SHORT, UINT, INT, ULONG, LONG, FLOAT, DOUBLE, POINTER, IMAGE,
    IMAGE1D, IMAGE1D_ARRAY, IMAGE1D_BUFFER, IMAGE2D, IMAGE2D_ARRAY, IMAGE3D,
    UCHAR2, UCHAR3, UCHAR4, UCHAR8, UCHAR16,
    CHAR2, CHAR3, CHAR4, CHAR8, CHAR16,
    USHORT2, USHORT3, USHORT4, USHORT8, USHORT16,
    SHORT2, SHORT3, SHORT4, SHORT8, SHORT16,
    UINT2, UINT3, UINT4, UINT8, UINT16,
    INT2, INT3, INT4, INT8, INT16,
    ULONG2, ULONG3, ULONG4, ULONG8, ULONG16,
    LONG2, LONG3, LONG4, LONG8, LONG16,
    FLOAT2, FLOAT3, FLOAT4, FLOAT8, FLOAT16,
    DOUBLE2, DOUBLE3, DOUBLE4, DOUBLE8, DOUBLE16,
    SAMPLER, STRUCTURE, COUNTER32, COUNTER64,
    MAX_VALUE = COUNTER64,
    MIN_IMAGE = IMAGE,
    MAX_IMAGE = IMAGE3D
};

/// kernel pointer type of argument
enum class KernelPtrSpace : uint8_t
{
    NONE = 0,   ///< no pointer
    LOCAL,      ///< pointer to local memory
    CONSTANT,   ///< pointer to constant memory
    GLOBAL      ///< pointer to global memory
};

enum : uint8_t
{
    KARG_PTR_NORMAL = 0,    ///< no special flags
    KARG_PTR_READ_ONLY = 1, ///< read only image
    KARG_PTR_WRITE_ONLY = 2,    ///< write only image
    KARG_PTR_READ_WRITE = 3,    ///< read-write image (???)
    KARG_PTR_ACCESS_MASK = 3,
    KARG_PTR_CONST = 4,     ///< constant buffer
    KARG_PTR_RESTRICT = 8,  ///< buffer is restrict specifier
    KARG_PTR_VOLATILE = 16  ///< volatile buffer
};

/// kernel argument info structure
struct KernelArg
{
    KernelArgType argType;  ///< argument type
    KernelPtrSpace ptrSpace;///< pointer space for argument if argument is pointer or image
    uint8_t ptrAccess;  ///< pointer access flags
    std::string typeName;   ///< name of type of argument
    std::string argName;    ///< argument name
};

struct X86KernelArgSym
{
    uint32_t argType;
    uint32_t ptrType;
    uint32_t ptrAccess;
    uint32_t nameOffset;
    
    size_t getNameOffset() const
    { return ULEV(nameOffset); }
};

struct X86_64KernelArgSym
{
    uint32_t argType;
    uint32_t ptrType;
    uint32_t ptrAccess;
    uint32_t nameOffsetLo;
    uint32_t nameOffsetHi;
    
    size_t getNameOffset() const
    { return (uint64_t(ULEV(nameOffsetHi))<<32)+uint64_t(ULEV(nameOffsetLo)); }
};

enum : uint32_t
{   /* this cal note types comes from MultiSim-4.2 sources */
    CALNOTE_ATI_PROGINFO = 1,
    CALNOTE_ATI_INPUTS = 2,
    CALNOTE_ATI_OUTPUTS = 3,
    CALNOTE_ATI_CONDOUT = 4,
    CALNOTE_ATI_FLOAT32CONSTS = 5,
    CALNOTE_ATI_INT32CONSTS = 6,
    CALNOTE_ATI_BOOL32CONSTS = 7,
    CALNOTE_ATI_EARLYEXIT = 8,
    CALNOTE_ATI_GLOBAL_BUFFERS = 9,
    CALNOTE_ATI_CONSTANT_BUFFERS = 10,
    CALNOTE_ATI_INPUT_SAMPLERS = 11,
    CALNOTE_ATI_PERSISTENT_BUFFERS = 12,
    CALNOTE_ATI_SCRATCH_BUFFERS = 13,
    CALNOTE_ATI_SUB_CONSTANT_BUFFERS = 14,
    CALNOTE_ATI_UAV_MAILBOX_SIZE = 15,
    CALNOTE_ATI_UAV = 16,
    CALNOTE_ATI_UAV_OP_MASK = 17,
    CALNOTE_ATI_MAXTYPE = CALNOTE_ATI_UAV_OP_MASK
};

/// CALEncodingEntry
struct CALEncodingEntry
{
    uint32_t machine;   ///< machine type
    uint32_t type;      ///< type of entry
    uint32_t offset;    ///< offset in ELF
    uint32_t size;      ///< size
    uint32_t flags;     ///< flags
};

/// ATI CAL Note header
struct CALNoteHeader
{
    uint32_t nameSize;  ///< name size (must be 8)
    uint32_t descSize;  ///< description size
    uint32_t type;  ///< type
    char name[8];   ///< name string
};

/// ATI CAL note
struct CALNote
{
    CALNoteHeader* header;  ///< header of CAL note
    cxbyte* data;   ///< data of CAL note
};

/// CAL program info entry
struct CALProgramInfoEntry
{
    uint32_t address;   ///< address of value
    uint32_t value;     ///< value to set
};

struct CALUAVEntry
{
    uint32_t uavId;
    uint32_t f1, f2;
    uint32_t type;
};

struct CALDataSegmentEntry
{
    uint32_t offset;
    uint32_t size;
};

struct CALConstantBufferMask
{
    uint32_t index;
    uint32_t size;
};

struct CALSamplerMapEntry
{
    uint32_t input;
    uint32_t sampler;
};

/// kernel informations
struct KernelInfo
{
    std::string kernelName; ///< kernel name
    cxuint argsNum; ///< number of arguments
    KernelArg* argInfos;    ///< array of argument informations
    
    KernelInfo();
    KernelInfo(const KernelInfo& cp);
    KernelInfo(KernelInfo&& cp) noexcept;
    ~KernelInfo();
    
    KernelInfo& operator=(const KernelInfo& cp);
    KernelInfo& operator=(KernelInfo&& cp) noexcept;
    
    /// allocate arguments to specified size
    void allocateArgs(cxuint argsNum);
    
    void reallocateArgs(cxuint argsNum);
};

struct Elf32Types
{
    typedef uint32_t Size;
    typedef uint32_t Word;
    typedef Elf32_Ehdr Ehdr;
    typedef Elf32_Shdr Shdr;
    typedef Elf32_Phdr Phdr;
    typedef Elf32_Sym Sym;
    static const uint8_t ELFCLASS;
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
    static const uint8_t ELFCLASS;
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
    typedef std::unordered_multimap<const char*, size_t, CLRX::CStringHash,
            CLRX::CStringEqual> SectionIndexMap;
    /// symbol index map
    typedef std::unordered_multimap<const char*, size_t, CLRX::CStringHash,
            CLRX::CStringEqual> SymbolIndexMap;
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
    bool operator !() const
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
        SectionIndexMap::const_iterator it = sectionIndexMap.find(name);
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
        SymbolIndexMap::const_iterator it = symbolIndexMap.find(name);
        if (it == symbolIndexMap.end())
            throw Exception(std::string("Can't find Elf")+Types::bitName+" Symbol");
        return it;
    }

    /// get dynamic symbol iterator with specified name (requires dynamic symbol index map)
    SymbolIndexMap::const_iterator getDynSymbolIter(const char* name) const
    {
        SymbolIndexMap::const_iterator it = dynSymIndexMap.find(name);
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
};

extern template class ElfBinaryTemplate<Elf32Types>;
extern template class ElfBinaryTemplate<Elf64Types>;

/// type for 32-bit ELF binary
typedef class ElfBinaryTemplate<Elf32Types> ElfBinary32;
/// type for 64-bit ELF binary
typedef class ElfBinaryTemplate<Elf64Types> ElfBinary64;

/// AMD inner binary for GPU binaries that represent a single kernel
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdInnerGPUBinary32: public ElfBinary32
{
private:
    /// ATTENTION: Do not put non-copyable stuff (likes pointers, arrays),
    /// because this object will copied
    std::string kernelName; ///< kernel name
    uint32_t encodingEntriesNum;
    CALEncodingEntry* encodingEntries;
    std::vector<std::vector<CALNote> > calNotesTable;
public:
    AmdInnerGPUBinary32() = default;
    /** constructor
     * \param kernelName kernel name
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerGPUBinary32(const std::string& kernelName, size_t binaryCodeSize,
            cxbyte* binaryCode, cxuint creationFlags = ELF_CREATE_ALL);
    ~AmdInnerGPUBinary32() = default;
    
    /// return true if binary has CAL notes infos
    bool hasCALNotes() const
    { return (creationFlags & AMDBIN_CREATE_CALNOTES) != 0; }
    
    /// get kernel name
    const std::string& getKernelName() const
    { return kernelName; }
    
    /// get CALEncoding entries number
    uint32_t getCALEncodingEntriesNum() const
    { return encodingEntriesNum; }
    
    /// get CALEncodingDictionaryEntries
    const CALEncodingEntry& getCALEncodingEntry(cxuint index) const
    { return encodingEntries[index]; }
    
    /// get CALEncodingDictionaryEntries
    CALEncodingEntry& getCALEncodingEntry(cxuint index)
    { return encodingEntries[index]; }
    
    /// get CAL Notes number
    uint32_t getCALNotesNum(cxuint encodingIndex) const
    { return calNotesTable[encodingIndex].size(); }
    
    /// get CAL Note header
    const CALNoteHeader& getCALNoteHeader(cxuint encodingIndex, uint32_t index) const
    { return *(calNotesTable[encodingIndex][index]).header; }
    
    /// get CAL Note header
    CALNoteHeader& getCALNoteHeader(cxuint encodingIndex, uint32_t index)
    { return *(calNotesTable[encodingIndex][index]).header; }
    
    /// get CAL Note data
    const cxbyte* getCALNoteData(cxuint encodingIndex, uint32_t index) const
    { return calNotesTable[encodingIndex][index].data; }
    /// get CAL Note data
    cxbyte* getCALNoteData(cxuint encodingIndex, uint32_t index)
    { return calNotesTable[encodingIndex][index].data; }
};

/// AMD inner X86 binary
class AmdInnerX86Binary32: public ElfBinary32
{
public:
    AmdInnerX86Binary32() = default;
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerX86Binary32(size_t binaryCodeSize, cxbyte* binaryCode,
            cxuint creationFlags = ELF_CREATE_ALL);
    ~AmdInnerX86Binary32() = default;
    
    /// generate kernel info from this binary and save to KernelInfo array
    uint32_t getKernelInfos(KernelInfo*& kernelInfos) const;
};

/// AMD inner binary for X86-64 binaries
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdInnerX86Binary64: public ElfBinary64
{
public:
    AmdInnerX86Binary64() = default;
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerX86Binary64(size_t binaryCodeSize, cxbyte* binaryCode,
            cxuint creationFlags = ELF_CREATE_ALL);
    ~AmdInnerX86Binary64() = default;
    
    /// generate kernel info from this binary and save to KernelInfo array
    size_t getKernelInfos(KernelInfo*& kernelInfos) const;
};

/// AMD main binary type
enum class AmdMainType
{
    GPU_BINARY, ///< binary for GPU
    GPU_64_BINARY, ///< binary for GPU with 64-bit memory model
    X86_BINARY, ///< binary for x86 systems
    X86_64_BINARY ///< binary for x86-64 systems
};

/// main AMD binary base class
class AmdMainBinaryBase
{
public:
    /// Kernel info map
    typedef std::unordered_map<std::string, size_t> KernelInfoMap;
protected:
    AmdMainType type;   ///< type of binaries
    size_t kernelInfosNum;  ///< number of kernel infos that is number of kernels
    KernelInfo* kernelInfos;    ///< kernel informations
    KernelInfoMap kernelInfosMap;   ///< kernel informations map
    
    std::string driverInfo;
    std::string compileOptions;
    
    explicit AmdMainBinaryBase(AmdMainType type);
public:
    // no copyable and no movable
    AmdMainBinaryBase(const AmdMainBinaryBase&) = delete;
    AmdMainBinaryBase(AmdMainBinaryBase&&) = delete;
    AmdMainBinaryBase& operator=(const AmdMainBinaryBase&) = delete;
    AmdMainBinaryBase& operator=(AmdMainBinaryBase&&) = delete;
    virtual ~AmdMainBinaryBase();
    
    /// get binary type
    AmdMainType getType() const
    { return type; }
    
    /// get kernel informations number
    size_t getKernelInfosNum() const
    { return kernelInfosNum; }
    
    /// get kernel informations array
    const KernelInfo* getKernelInfos() const
    { return kernelInfos; }
    
    /// get kernel information with specified index
    const KernelInfo& getKernelInfo(size_t index) const
    { return kernelInfos[index]; }
    
    /// get kernel information with specified kernel name (requires kernel info map)
    const KernelInfo& getKernelInfo(const char* name) const;
    
    /// get driver info string
    const std::string getDriverInfo() const
    { return driverInfo; }
    
    /// get compile options string
    const std::string getCompileOptions() const
    { return compileOptions; }
};

/// AMD GPU metadata for kernel
struct AmdGPUKernelMetadata
{
    size_t size;
    char* data;
};

/// AMD GPU header for kernel
struct AmdGPUKernelHeader
{
    std::string kernelName;
    size_t size;
    cxbyte* data;
};

/// main AMD GPU binary base class
class AmdMainGPUBinaryBase: public AmdMainBinaryBase
{
public:
    /// inner binary map
    typedef std::unordered_map<std::string, size_t> InnerBinaryMap;
    typedef std::unordered_map<std::string, size_t> KernelHeaderMap;
protected:
    size_t innerBinariesNum;
    AmdInnerGPUBinary32* innerBinaries;
    InnerBinaryMap innerBinaryMap;
    AmdGPUKernelMetadata* metadatas;
    size_t kernelHeadersNum;
    AmdGPUKernelHeader* kernelHeaders;
    KernelHeaderMap kernelHeaderMap;
    size_t globalDataSize;
    cxbyte* globalData;
    
    explicit AmdMainGPUBinaryBase(AmdMainType type);
    
    template<typename Types>
    void initMainGPUBinary(typename Types::ElfBinary& binary);
public:
    ~AmdMainGPUBinaryBase();
    
    /// get number of inner binaries
    size_t getInnerBinariesNum() const
    { return innerBinariesNum; }
    
    /// get inner binary with specified index
    AmdInnerGPUBinary32& getInnerBinary(size_t index)
    { return innerBinaries[index]; }
    
    /// get inner binary with specified index
    const AmdInnerGPUBinary32& getInnerBinary(size_t index) const
    { return innerBinaries[index]; }
    
    /// get inner binary with specified name (requires inner binary map)
    const AmdInnerGPUBinary32& getInnerBinary(const char* name) const;
    
    /// get metadata size for specified inner binary
    size_t getMetadataSize(size_t index) const
    { return metadatas[index].size; }
    
    /// get metadata for specified inner binary
    const char* getMetadata(size_t index) const
    { return metadatas[index].data; }
    
    /// get metadata for specified inner binary
    char* getMetadata(size_t index)
    { return metadatas[index].data; }
    
    /// get global data size
    size_t getGlobalDataSize() const
    { return globalDataSize; }
    
    /// get global data
    const cxbyte* getGlobalData() const
    { return globalData; }
    /// get global data
    cxbyte* getGlobalData()
    { return globalData; }
    
    /// get kernel header number
    size_t getKernelHeadersNum() const
    { return kernelHeadersNum; }
    
    /// get kernel header entry for specified index
    const AmdGPUKernelHeader& getKernelHeaderEntry(size_t index) const
    { return kernelHeaders[index]; }
    
    /// get kernel header entry for specified index
    const AmdGPUKernelHeader& getKernelHeaderEntry(const char* name) const;
    
    /// get kernel header entry for specified index
    AmdGPUKernelHeader& getKernelHeaderEntry(size_t index)
    { return kernelHeaders[index]; }
    
    /// get kernel header size for specified index
    size_t getKernelHeaderSize(size_t index) const
    { return kernelHeaders[index].size; }
    
    /// get kernel header for specified index
    const cxbyte* getKernelHeader(size_t index) const
    { return kernelHeaders[index].data; }
    
    /// get kernel header for specified index
    cxbyte* getKernelHeader(size_t index)
    { return kernelHeaders[index].data; }
};

/// AMD main binary for GPU for 32-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainGPUBinary32: public AmdMainGPUBinaryBase, public ElfBinary32
{
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainGPUBinary32(size_t binaryCodeSize, cxbyte* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainGPUBinary32() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has inner binary map
    bool hasInnerBinaryMap() const
    { return (creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has kernel headers
    bool hasKernelHeaders() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERS) != 0; }
    
    /// return true if binary has kernel header map
    bool hasKernelHeaderMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERMAP) != 0; }
};

/// AMD main binary for GPU for 64-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainGPUBinary64: public AmdMainGPUBinaryBase, public ElfBinary64
{
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainGPUBinary64(size_t binaryCodeSize, cxbyte* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainGPUBinary64() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has inner binary map
    bool hasInnerBinaryMap() const
    { return (creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has kernel headers
    bool hasKernelHeaders() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERS) != 0; }
    
    /// return true if binary has kernel header map
    bool hasKernelHeaderMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELHEADERMAP) != 0; }
};

/// AMD main binary for X86 systems
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainX86Binary32: public AmdMainBinaryBase, public ElfBinary32
{
private:
    AmdInnerX86Binary32 innerBinary;
    
    void initKernelInfos(cxuint creationFlags);
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainX86Binary32(size_t binaryCodeSize, cxbyte* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainX86Binary32() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has inner binary
    bool hasInnerBinary() const
    { return innerBinary; }
    
    /// get inner binary
    AmdInnerX86Binary32& getInnerBinary()
    { return innerBinary; }
    
    /// get inner binary
    const AmdInnerX86Binary32& getInnerBinary() const
    { return innerBinary; }
};

/// AMD main binary for X86-64 systems
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdMainX86Binary64: public AmdMainBinaryBase, public ElfBinary64
{
private:
    AmdInnerX86Binary64 innerBinary;
    
    void initKernelInfos(cxuint creationFlags);
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainX86Binary64(size_t binaryCodeSize, cxbyte* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    ~AmdMainX86Binary64() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    /// return true if binary has inner binary
    bool hasInnerBinary() const
    { return innerBinary; }
    
    /// get inner binary
    AmdInnerX86Binary64& getInnerBinary()
    { return innerBinary; }
    /// get inner binary
    const AmdInnerX86Binary64& getInnerBinary() const
    { return innerBinary; }
};

/// create AMD binary object from binary code
/**
 * \param binaryCodeSize binary code size
 * \param binaryCode pointer to binary code
 * \param creationFlags flags that specified what will be created during creation
 */
extern AmdMainBinaryBase* createAmdBinaryFromCode(
            size_t binaryCodeSize, cxbyte* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);

};

#endif
