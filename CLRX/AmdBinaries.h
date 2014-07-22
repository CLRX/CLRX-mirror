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
 * \brief an AMD binaries handling
 */

#ifndef __CLRX_AMDBINARIES_H__
#define __CLRX_AMDBINARIES_H__

#include <CLRX/Config.h>
#include <elf.h>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include <CLRX/Utilities.h>

/// main namespace
namespace CLRX
{

enum : cxuint {
    ELF_CREATE_SECTIONMAP = 1,  ///< create map of sections
    ELF_CREATE_SYMBOLMAP = 2,   ///< create map of symbols
    ELF_CREATE_DYNSYMMAP = 4,   ///< create map of dynamic symbols
    ELF_CREATE_ALL = 7, ///< creation flags for ELF binaries
    AMDBIN_CREATE_KERNELINFO = 0x10,    ///< create kernel informations
    AMDBIN_CREATE_KERNELINFOMAP = 0x20, ///< create map of kernel informations
    AMDBIN_CREATE_INNERBINMAP = 0x40,   ///< create map of inner binaries
    AMDBIN_INNER_CREATE_SECTIONMAP = 0x100, ///< create map of sections for inner binaries
    AMDBIN_INNER_CREATE_SYMBOLMAP = 0x200,  ///< create map of symbols for inner binaries
    /** create map of dynamic symbols for inner binaries */
    AMDBIN_INNER_CREATE_DYNSYMMAP = 0x400,  
    
    AMDBIN_CREATE_ALL = ELF_CREATE_ALL | 0xff0, ///< all AMD binaries creation flags
    AMDBIN_INNER_SHIFT = 8 ///< shift for convert inner binary flags into elf binary flags
};

enum : cxuint {
    ELF_M_X86 = 0x7d4
};

/// kernel argument type
enum KernelArgType : uint8_t
{
    VOID = 0,
    UCHAR, CHAR, USHORT, SHORT, UINT, INT, ULONG, LONG, FLOAT, DOUBLE, POINTER, IMAGE,
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
    SAMPLER,
    MAX_VALUE = SAMPLER
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
    KARG_PTR_CONST = 4,     ///< constant buffer
    KARG_PTR_RESTRICT = 8,  ///< buffer is restrict specifier
    KARG_PTR_VOLATILE = 16  ///< volatile buffer
};

/// kernel argument info structure
struct KernelArg
{
    KernelArgType argType;  ///< argument type
    KernelPtrSpace ptrSpace;  ///< pointer space for argument if argument is pointer or image
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
};

struct X86_64KernelArgSym
{
    uint32_t argType;
    uint32_t ptrType;
    uint32_t ptrAccess;
    uint32_t nameOffsetLo;
    uint32_t nameOffsetHi;
};

/// kernel informations
struct KernelInfo
{
    std::string kernelName; ///< kernel name
    cxuint argsNum; ///< number of arguments
    KernelArg* argInfos;    ///< array of argument informations
    
    KernelInfo();
    KernelInfo(const KernelInfo& cp);
    KernelInfo(KernelInfo&& cp);
    ~KernelInfo();
    
    KernelInfo& operator=(const KernelInfo& cp);
    KernelInfo& operator=(KernelInfo&& cp);
    
    /// allocate arguments to specified size
    void allocateArgs(cxuint argsNum);
};

/// ELF binary base class
class ElfBinaryBase
{
public:
    /// section index map
    typedef std::unordered_map<const char*, size_t, CLRX::CStringHash,
            CLRX::CStringEqual> SectionIndexMap;
    /// symbol index map
    typedef std::unordered_map<const char*, size_t, CLRX::CStringHash,
            CLRX::CStringEqual> SymbolIndexMap;
protected:
    cxuint creationFlags;   ///< creation flags holder
    size_t binaryCodeSize;  ///< binary code size
    char* binaryCode;       ///< pointer to binary code
    char* sectionStringTable;   ///< pointer to section's string table
    char* symbolStringTable;    ///< pointer to symbol's string table
    char* symbolTable;          ///< pointer to symbol table
    char* dynSymStringTable;    ///< pointer to dynamic symbol's string table
    char* dynSymTable;          ///< pointer to dynamic symbol table
    SectionIndexMap sectionIndexMap;    ///< section's index map
    SymbolIndexMap symbolIndexMap;      ///< symbol's index map
    SymbolIndexMap dynSymIndexMap;      ///< dynamic symbol's index map
    
    ElfBinaryBase();
public:
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
};

/// 32-bit ELF binary class
class ElfBinary32: public ElfBinaryBase
{
protected:
    uint32_t symbolsNum;    ///< symbols number
    uint32_t dynSymbolsNum; ///< dynamic symbols number
    uint16_t symbolEntSize; ///< symbol entry size in a symbol's table
    uint16_t dynSymEntSize; ///< dynamic symbol entry size in a dynamic symbol's table
    
public:
    ElfBinary32();
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    ElfBinary32(size_t binaryCodeSize, char* binaryCode,
                cxuint cretionFlags = ELF_CREATE_ALL);
    ~ElfBinary32() = default;
    
    /// get ELF binary header
    const Elf32_Ehdr& getHeader() const
    { return *reinterpret_cast<const Elf32_Ehdr*>(binaryCode); }
    
    /// get ELF binary header
    Elf32_Ehdr& getHeader()
    { return *reinterpret_cast<Elf32_Ehdr*>(binaryCode); }
    
    /// get section headers number
    uint16_t getSectionHeadersNum() const
    { return getHeader().e_shnum; }
    
    /// get section header with specified index
    const Elf32_Shdr& getSectionHeader(uint16_t index) const
    {
        const Elf32_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<const Elf32_Shdr*>(binaryCode + ehdr.e_shoff +
                size_t(ehdr.e_shentsize)*index);
    }
    
    /// get section header with specified index
    Elf32_Shdr& getSectionHeader(uint16_t index)
    {
        const Elf32_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<Elf32_Shdr*>(binaryCode + ehdr.e_shoff +
                size_t(ehdr.e_shentsize)*index);
    }
    
    /// get program headers number
    uint16_t getProgramHeadersNum() const
    { return getHeader().e_phnum; }
    
    /// get program header with specified index
    const Elf32_Phdr& getProgramHeader(uint16_t index) const
    {
        const Elf32_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<const Elf32_Phdr*>(binaryCode + ehdr.e_phoff +
                size_t(ehdr.e_phentsize)*index);
    }
    
    /// get program header with specified index
    Elf32_Phdr& getProgramHeader(uint16_t index)
    {
        const Elf32_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<Elf32_Phdr*>(binaryCode + ehdr.e_phoff +
                size_t(ehdr.e_phentsize)*index);
    }
    
    /// get symbols number
    uint32_t getSymbolsNum() const
    { return symbolsNum; }
    
    /// get dynamic symbols number
    uint32_t getDynSymbolsNum() const
    { return dynSymbolsNum; }
    
    /// get symbol with specified index
    const Elf32_Sym& getSymbol(uint32_t index) const
    {
        return *reinterpret_cast<const Elf32_Sym*>(symbolTable +
                    size_t(index)*symbolEntSize);
    }
    
    /// get symbol with specified index
    Elf32_Sym& getSymbol(uint32_t index)
    { return *reinterpret_cast<Elf32_Sym*>(symbolTable + size_t(index)*symbolEntSize); }
    
    /// get dynamic symbol with specified index
    const Elf32_Sym& getDynSymbol(uint32_t index) const
    {
        return *reinterpret_cast<const Elf32_Sym*>(dynSymTable +
                    size_t(index)*dynSymEntSize);
    }
    
    /// get dynamic symbol with specified index
    Elf32_Sym& getDynSymbol(uint32_t index)
    { return *reinterpret_cast<Elf32_Sym*>(dynSymTable + size_t(index)*dynSymEntSize); }
    
    /// get symbol name with specified index
    const char* getSymbolName(uint32_t index) const
    {
        const Elf32_Sym& sym = getSymbol(index);
        return symbolStringTable + sym.st_name;
    }
    
    /// get dynamic symbol name with specified index
    const char* getDynSymbolName(uint32_t index) const
    {
        const Elf32_Sym& sym = getDynSymbol(index);
        return dynSymStringTable + sym.st_name;
    }
    
    /// get section name with specified index
    const char* getSectionName(uint16_t index) const
    {
        const Elf32_Shdr& section = getSectionHeader(index);
        return sectionStringTable + section.sh_name;
    }
    
    /// get section index with specified name
    uint16_t getSectionIndex(const char* name) const;
    
    /// get symbol index with specified name (requires symbol index map)
    uint32_t getSymbolIndex(const char* name) const;
    
    /// get dynamic symbol index with specified name (requires dynamic symbol index map)
    uint32_t getDynSymbolIndex(const char* name) const;
    
    /// get section header with specified name
    const Elf32_Shdr& getSectionHeader(const char* name) const
    { return getSectionHeader(getSectionIndex(name)); }
    
    /// get section header with specified name
    Elf32_Shdr& getSectionHeader(const char* name)
    { return getSectionHeader(getSectionIndex(name)); }
    
    /// get symbol with specified name (requires symbol index map)
    const Elf32_Sym& getSymbol(const char* name) const
    { return getSymbol(getSymbolIndex(name)); }
    
    /// get symbol with specified name (requires symbol index map)
    Elf32_Sym& getSymbol(const char* name)
    { return getSymbol(getSymbolIndex(name)); }
    
    /// get dynamic symbol with specified name (requires dynamic symbol index map)
    const Elf32_Sym& getDynSymbol(const char* name) const
    { return getDynSymbol(getDynSymbolIndex(name)); }
    
    /// get dynamic symbol with specified name (requires dynamic symbol index map)
    Elf32_Sym& getDynSymbol(const char* name)
    { return getDynSymbol(getDynSymbolIndex(name)); }
    
    /// get section content pointer
    const char* getSectionContent(uint16_t index) const
    {
        const Elf32_Shdr& shdr = getSectionHeader(index);
        return binaryCode + shdr.sh_offset;
    }
    
    /// get section content pointer
    char* getSectionContent(uint16_t index)
    {
        Elf32_Shdr& shdr = getSectionHeader(index);
        return binaryCode + shdr.sh_offset;
    }
};

/// 64-bit ELF binary class
class ElfBinary64 : public ElfBinaryBase
{
protected:
    size_t symbolsNum;      ///< symbols number
    size_t dynSymbolsNum; ///< dynamic symbols number
    uint16_t symbolEntSize; ///< symbol entry size in a symbol's table
    uint16_t dynSymEntSize; ///< dynamic symbol entry size in a dynamic symbol's table
public:
    ElfBinary64();
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    ElfBinary64(size_t binaryCodeSize, char* binaryCode,
                cxuint cretionFlags = ELF_CREATE_ALL);
    ~ElfBinary64() = default;
    
    /// get ELF binary header
    const Elf64_Ehdr& getHeader() const
    { return *reinterpret_cast<const Elf64_Ehdr*>(binaryCode); }
    
    /// get ELF binary header
    Elf64_Ehdr& getHeader()
    { return *reinterpret_cast<Elf64_Ehdr*>(binaryCode); }
    
    /// get section headers number
    uint16_t getSectionHeadersNum() const
    { return getHeader().e_shnum; }
    
    /// get section header with specified index
    const Elf64_Shdr& getSectionHeader(uint16_t index) const
    {
        const Elf64_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<const Elf64_Shdr*>(binaryCode + ehdr.e_shoff +
                size_t(ehdr.e_shentsize)*index);
    }
    
    /// get section header with specified index
    Elf64_Shdr& getSectionHeader(uint16_t index)
    {
        const Elf64_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<Elf64_Shdr*>(binaryCode + ehdr.e_shoff +
                size_t(ehdr.e_shentsize)*index);
    }
    
    /// get program headers number
    uint16_t getProgramHeadersNum() const
    { return getHeader().e_phnum; }
    
    /// get program header with specified index
    const Elf64_Phdr& getProgramHeader(uint16_t index) const
    {
        const Elf64_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<const Elf64_Phdr*>(binaryCode + ehdr.e_phoff +
                size_t(ehdr.e_phentsize)*index);
    }
    
    /// get program header with specified index
    Elf64_Phdr& getProgramHeader(uint16_t index)
    {
        const Elf64_Ehdr& ehdr = getHeader();
        return *reinterpret_cast<Elf64_Phdr*>(binaryCode + ehdr.e_phoff +
                size_t(ehdr.e_phentsize)*index);
    }
    
    /// get symbols number
    size_t getSymbolsNum() const
    { return symbolsNum; }
    
    /// get dynamic symbols number
    size_t getDynSymbolsNum() const
    { return dynSymbolsNum; }
    
    /// get symbol with specified index
    const Elf64_Sym& getSymbol(size_t index) const
    {
        return *reinterpret_cast<const Elf64_Sym*>(symbolTable +
                    size_t(index)*symbolEntSize);
    }
    
    /// get symbol with specified index
    Elf64_Sym& getSymbol(size_t index)
    { return *reinterpret_cast<Elf64_Sym*>(symbolTable + size_t(index)*symbolEntSize); }
    
    /// get dynamic symbol with specified index
    const Elf64_Sym& getDynSymbol(size_t index) const
    {
        return *reinterpret_cast<const Elf64_Sym*>(dynSymTable +
                    size_t(index)*dynSymEntSize);
    }
    
    /// get dynamic symbol with specified index
    Elf64_Sym& getDynSymbol(size_t index)
    { return *reinterpret_cast<Elf64_Sym*>(dynSymTable + size_t(index)*dynSymEntSize); }
    
    /// get symbol with specified name
    const char* getSymbolName(size_t index) const
    {
        const Elf64_Sym& sym = getSymbol(index);
        return symbolStringTable + sym.st_name;
    }
    
    /// get dynamic symbol with specified name
    const char* getDynSymbolName(size_t index) const
    {
        const Elf64_Sym& sym = getDynSymbol(index);
        return dynSymStringTable + sym.st_name;
    }
    
    /// get section with specified name
    const char* getSectionName(uint16_t index) const
    {
        const Elf64_Shdr& section = getSectionHeader(index);
        return sectionStringTable + section.sh_name;
    }
    
    /// get section index with specified name
    uint16_t getSectionIndex(const char* name) const;
    
    /// get symbol index with specified name (requires symbol index map)
    size_t getSymbolIndex(const char* name) const;
    
    /// get dynamic symbol index with specified name (requires dynamic symbol index map)
    size_t getDynSymbolIndex(const char* name) const;
    
    /// get section header with specified name
    const Elf64_Shdr& getSectionHeader(const char* name) const
    { return getSectionHeader(getSectionIndex(name)); }
    
    /// get section header with specified name
    Elf64_Shdr& getSectionHeader(const char* name)
    { return getSectionHeader(getSectionIndex(name)); }
    
    /// get symbol with specified name (requires symbol index map)
    const Elf64_Sym& getSymbol(const char* name) const
    { return getSymbol(getSymbolIndex(name)); }
    
    /// get symbol with specified name (requires symbol index map)
    Elf64_Sym& getSymbol(const char* name)
    { return getSymbol(getSymbolIndex(name)); }
    
    /// get dynamic symbol with specified name (requires dynamic symbol index map)
    const Elf64_Sym& getDynSymbol(const char* name) const
    { return getDynSymbol(getDynSymbolIndex(name)); }
    
    /// get dynamic symbol with specified name (requires dynamic symbol index map)
    Elf64_Sym& getDynSymbol(const char* name)
    { return getDynSymbol(getDynSymbolIndex(name)); }
    
    /// get section content pointer
    const char* getSectionContent(uint16_t index) const
    {
        const Elf64_Shdr& shdr = getSectionHeader(index);
        return binaryCode + shdr.sh_offset;
    }
    
    /// get section content pointer
    char* getSectionContent(uint16_t index)
    {
        Elf64_Shdr& shdr = getSectionHeader(index);
        return binaryCode + shdr.sh_offset;
    }
};

/// AMD inner binary for GPU binaries that represent a single kernel
class AmdInnerGPUBinary32: public ElfBinary32
{
private:
    std::string kernelName; ///< kernel name
public:
    AmdInnerGPUBinary32() = default;
    /** constructor
     * \param kernelName kernel name
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerGPUBinary32(const std::string& kernelName, size_t binaryCodeSize,
            char* binaryCode, cxuint creationFlags = ELF_CREATE_ALL);
    ~AmdInnerGPUBinary32() = default;
    
    /// get kernel name
    const std::string& getKernelName() const
    { return kernelName; }
};

/// AMD inner binary for X86 binaries
class AmdInnerX86Binary32: public ElfBinary32
{
public:
    AmdInnerX86Binary32() = default;
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerX86Binary32(size_t binaryCodeSize, char* binaryCode,
            cxuint creationFlags = ELF_CREATE_ALL);
    ~AmdInnerX86Binary32() = default;
    
    /// generate kernel info from this binary and save to KernelInfo array
    size_t getKernelInfos(KernelInfo*& kernelInfos) const;
};

/// AMD inner binary for X86-64 binaries
class AmdInnerX86Binary64: public ElfBinary64
{
public:
    AmdInnerX86Binary64() = default;
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdInnerX86Binary64(size_t binaryCodeSize, char* binaryCode,
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
    typedef std::unordered_map<std::string, size_t>KernelInfoMap;
protected:
    AmdMainType type;   ///< type of binaries
    size_t kernelInfosNum;  ///< number of kernel infos that is number of kernels
    KernelInfo* kernelInfos;    ///< kernel informations
    KernelInfoMap kernelInfosMap;   ///< kernel informations map
    
    explicit AmdMainBinaryBase(AmdMainType type);
public:
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
};

/// AMD main binary for GPU
class AmdMainGPUBinary32: public AmdMainBinaryBase, public ElfBinary32
{
public:
    /// inner binary map
    typedef std::unordered_map<std::string, size_t> InnerBinaryMap;
private:
    uint32_t innerBinariesNum;
    AmdInnerGPUBinary32* innerBinaries;
    InnerBinaryMap innerBinaryMap;
    
    void initKernelInfos(cxuint creationFlags, const std::vector<uint32_t>& metadataSyms);
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainGPUBinary32(size_t binaryCodeSize, char* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    virtual ~AmdMainGPUBinary32();
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has inner binary map
    bool hasInnerBinaryMap() const
    { return (creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0; }
    
    /// get number of inner binaries
    uint32_t getInnerBinariesNum() const
    { return innerBinariesNum; }
    
    /// get inner binary with specified index
    AmdInnerGPUBinary32& getInnerBinary(uint32_t index)
    { return innerBinaries[index]; }
    
    /// get inner binary with specified index
    const AmdInnerGPUBinary32& getInnerBinary(uint32_t index) const
    { return innerBinaries[index]; }
    
    /// get inner binary with specified name (requires inner binary map)
    const AmdInnerGPUBinary32& getInnerBinary(const char* name) const;
};

/// AMD main binary for GPU
class AmdMainGPUBinary64: public AmdMainBinaryBase, public ElfBinary64
{
public:
    /// inner binary map
    typedef std::unordered_map<std::string, size_t> InnerBinaryMap;
private:
    size_t innerBinariesNum;
    AmdInnerGPUBinary32* innerBinaries;
    InnerBinaryMap innerBinaryMap;
    
    void initKernelInfos(cxuint creationFlags, const std::vector<size_t>& metadataSyms);
public:
    /** constructor
     * \param binaryCodeSize binary code size
     * \param binaryCode pointer to binary code
     * \param creationFlags flags that specified what will be created during creation
     */
    AmdMainGPUBinary64(size_t binaryCodeSize, char* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    virtual ~AmdMainGPUBinary64();
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// returns true if binary has inner binary map
    bool hasInnerBinaryMap() const
    { return (creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0; }
    
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
};

/// AMD main binary for X86 systems
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
    AmdMainX86Binary32(size_t binaryCodeSize, char* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    virtual ~AmdMainX86Binary32() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
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
    AmdMainX86Binary64(size_t binaryCodeSize, char* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);
    virtual ~AmdMainX86Binary64() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
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
            size_t binaryCodeSize, char* binaryCode,
            cxuint creationFlags = AMDBIN_CREATE_ALL);

};

#endif
