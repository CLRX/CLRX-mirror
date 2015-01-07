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
/*! \file GalliumBinaries.h
 * \brief GalliumCompute binaries handling (only with LLVM 3.6)
 */

#ifndef __CLRX_GALLIUMBINARIES_H__
#define __CLRX_GALLIUMBINARIES_H__

#include <CLRX/Config.h>
#include <elf.h>
#include <cstdint>
#include <string>
#include <vector>
#include <CLRX/ElfBinaries.h>
#include <CLRX/MemAccess.h>
#include <CLRX/Utilities.h>

/// main namespace
namespace CLRX
{

enum : cxuint {
    GALLIUM_CREATE_KERNELMAP = 0x10,
    GALLIUM_INNER_CREATE_SECTIONMAP = 0x100, ///< create map of sections for inner binaries
    GALLIUM_INNER_CREATE_SYMBOLMAP = 0x200,  ///< create map of kernels for inner binaries
    /** create map of dynamic kernels for inner binaries */
    GALLIUM_INNER_CREATE_DYNSYMMAP = 0x400,
    GALLIUM_INNER_CREATE_PROGINFOMAP = 0x1000,
    
    GALLIUM_ELF_CREATE_PROGINFOMAP = 0x10,
    
    GALLIUM_CREATE_ALL = ELF_CREATE_ALL | 0xfff0, ///< all Gallium binaries flalgs
    GALLIUM_INNER_SHIFT = 8 ///< shift for convert inner binary flags into elf binary flags
};

/// GalliumCompute Kernel Arg type
enum class GalliumArgType: cxbyte
{
    SCALAR = 0,
    CONSTANT,
    GLOBAL,
    LOCAL,
    IMAGE2D_RDONLY,
    IMAGE2D_WRONLY,
    IMAGE3D_RDONLY,
    IMAGE3D_WRONLY,
    SAMPLER,
    MAX_VALUE = SAMPLER
};

/// Gallium semantic field type
enum class GalliumArgSemantic: cxbyte
{
    GENERAL = 0,
    GRID_DIMENSION,
    GRID_OFFSET,
    MAX_VALUE = GRID_OFFSET
};

/// kernel program info entry for Gallium binaries
struct GalliumProgInfoEntry
{
    uint32_t address;   ///< address
    uint32_t value;     ///< value
};

/// kernel argument (Gallium binaries)
struct GalliumArg
{
    GalliumArgType type;    ///< type of argument
    bool signExtented;  ///< is signed extended
    GalliumArgSemantic semantic;    ///< semantic of array
    uint32_t size;  ///< size of argument
    uint32_t targetSize;    ///< target size
    uint32_t targetAlign;   ///< target alignment
};

/// kernel info structure (Gallium binaries)
struct GalliumKernel
{
    std::string kernelName;   ///< kernel's name
    uint32_t sectionId; ///< section id
    uint32_t offset;    ///< offset in ElfBinary
    std::vector<GalliumArg> argInfos;   ///< arguments
};

enum class GalliumSectionType: cxbyte
{
    TEXT = 0,   ///< section for text (binary code)
    DATA_CONSTANT,
    DATA_GLOBAL,
    DATA_LOCAL,
    DATA_PRIVATE,
    MAX_VALUE
};

/// Gallium binarie's Section
struct GalliumSection
{
    uint32_t sectionId; ///< section id
    GalliumSectionType type;    ///< type of section
    uint32_t offset;    ///< offset in binary
    uint32_t size;      ///< size of section
};

/* INFO: in this file is used ULEV function for conversion
 * from LittleEndian and unaligned access to other memory access policy and endianness
 * Please use this function whenever you want to get or set word in ELF binary,
 * because ELF binaries can be unaligned in memory (as inner binaries).
 */
/// Gallium ELF binary
/** ULEV function is required to access programInfoEntry fields */
class GalliumElfBinary: public ElfBinary32
{
private:
    typedef std::unordered_multimap<const char*, size_t, CLRX::CStringHash,
            CLRX::CStringEqual> ProgInfoEntryIndexMap;
private:
    size_t progInfosNum;
    GalliumProgInfoEntry* progInfoEntries;
    ProgInfoEntryIndexMap progInfoEntryMap;
public:
    GalliumElfBinary();
    GalliumElfBinary(size_t binaryCodeSize, cxbyte* binaryCode, cxuint creationFlags);
    ~GalliumElfBinary() = default;
    
    bool hasProgInfoMap() const
    { return (creationFlags & GALLIUM_ELF_CREATE_PROGINFOMAP) != 0; }
    
    /// returns program infos number
    size_t getProgramInfosNum() const
    { return progInfosNum; }
    
    /// returns number of program info entries for program info
    size_t getProgramInfoEntriesNum(uint32_t index) const;
    
    /// returns index for programinfo entries index for specified kernel name
    uint32_t getProgramInfoEntryIndex(const char* name) const;
    
    /// returns program info entries for specified kernel name
    const GalliumProgInfoEntry* getProgramInfo(const char* name) const
    { return reinterpret_cast<GalliumProgInfoEntry*>(progInfoEntries) +
        getProgramInfoEntryIndex(name); }
    
    /// returns program info entries for specified kernel name
    GalliumProgInfoEntry* getProgramInfo(const char* name)
    { return reinterpret_cast<GalliumProgInfoEntry*>(progInfoEntries) +
        getProgramInfoEntryIndex(name); }
    
    /// returns program info entries for specified kernel index
    const GalliumProgInfoEntry* getProgramInfo(uint32_t index) const;
    /// returns program info entries for specified kernel index
    GalliumProgInfoEntry* getProgramInfo(uint32_t index);
};

/** GalliumBinary object. This object converts to host-endian fields and
  * ULEV is not needed to access to fields of kernels and sections */
class GalliumBinary
{
private:
    /// symbol index map
    typedef std::unordered_multimap<std::string, size_t> KernelIndexMap;
private:
    size_t binaryCodeSize;
    cxbyte* binaryCode;
    cxuint creationFlags;
    
    std::vector<GalliumKernel> kernels;
    std::vector<GalliumSection> sections;
    
    KernelIndexMap kernelIndexMap;
    
    GalliumElfBinary elfBinary;
public:
    GalliumBinary();
    GalliumBinary(size_t binaryCodeSize, cxbyte* binaryCode, cxuint creationFlags);
    
    /// get creation flags
    cxuint getCreationFlags()
    { return creationFlags; }
    
    /// returns true if object has a symbol's index map
    bool hasKernelMap() const
    { return (creationFlags & GALLIUM_CREATE_KERNELMAP) != 0; }
    
    /// get size of binaries
    size_t getSize() const
    { return binaryCodeSize; }
    
    /// returns true if object is initialized
    operator bool() const
    { return binaryCode!=nullptr; }
    
    /// returns true if object is uninitialized
    bool operator!() const
    { return binaryCode==nullptr; }
    
    /// returns binary code data
    const cxbyte* getBinaryCode() const
    { return binaryCode; }
    
    /// returns binary code data
    cxbyte* getBinaryCode()
    { return binaryCode; }
    
    /// returns Gallium inner ELF binary
    GalliumElfBinary& getElfBinary()
    { return elfBinary; }
    
    /// returns Gallium inner ELF binary
    const GalliumElfBinary& getElfBinary() const
    { return elfBinary; }
    
    /// get sections number
    uint32_t getSectionsNum() const
    { return sections.size(); }
    
    /// get size of section with specified index
    uint32_t getSectionSize(uint32_t index) const
    { return sections[index].size; }
    
    /// get content for section with specified index
    const cxbyte* getSectionContent(uint32_t index) const
    { return binaryCode + sections[index].offset; }
    
    /// get content for section with specified index
    cxbyte* getSectionContent(uint32_t index)
    { return binaryCode + sections[index].offset; }
    
    /// get section with specified index
    const GalliumSection& getSection(uint32_t index) const
    { return sections[index]; }
    
    /// returns kernels number
    uint32_t getKernelsNum() const
    { return kernels.size(); }
    
    /// returns kernel index
    uint32_t getKernelIndex(const char* name) const;
    
    /// get kernel by index
    const GalliumKernel& getKernel(uint32_t index) const
    { return kernels[index]; }
    
    /// get kernel by index
    GalliumKernel& getKernel(uint32_t index)
    { return kernels[index]; }
    
    /// get kernel with speciified name
    const GalliumKernel& getKernel(const char* name) const
    { return kernels[getKernelIndex(name)]; }
    
    /// get kernel with specified name
    GalliumKernel& getKernel(const char* name)
    { return kernels[getKernelIndex(name)]; }
    
    /// get content of kernel with specified index
    const cxbyte* getKernelContent(uint32_t index) const
    { return binaryCode + kernels[index].offset; }
    
    /// get content of kernel with specified index
    cxbyte* getKernelContent(uint32_t index)
    { return binaryCode + kernels[index].offset; }
    
    /// get content of kernel with specified name
    const cxbyte* getKernelContent(const char* name) const
    { return binaryCode + kernels[getKernelIndex(name)].offset; }
    
    /// get content of kernel with specified name
    cxbyte* getKernelContent(const char* name)
    { return binaryCode + kernels[getKernelIndex(name)].offset; }
};

};

#endif
