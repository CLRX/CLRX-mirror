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
    GALLIUM_KERNELSMAP = 0x10,
    GALLIUM_INNER_CREATE_SECTIONMAP = 0x100, ///< create map of sections for inner binaries
    GALLIUM_INNER_CREATE_SYMBOLMAP = 0x200,  ///< create map of kernels for inner binaries
    /** create map of dynamic kernels for inner binaries */
    GALLIUM_INNER_CREATE_DYNSYMMAP = 0x400,
    
    GALLIUM_CREATE_ALL = ELF_CREATE_ALL | 0xff0, ///< all Gallium binaries flalgs
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
    SAMPLER
};

/// Gallium semantic field type
enum class GalliumArgSemantic: cxbyte
{
    GENERAL = 0,
    GRID_DIMENSION,
    GRID_OFFSET
};

/// kernel program info entry for Gallium binaries
struct GalliumProgInfoEntry
{
    uint32_t address;   ///< address
    uint32_t value;     ///< value
};

/// kernel program info for Gallium binaries
struct GalliumProgramInfo
{
    cxuint entriesNum;  /// number of entries
    GalliumProgInfoEntry entries[16];
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
    std::string name;   ///< kernel's name
    uint32_t symId; ///< symbol id
    uint32_t offset;    ///< offset in binary
    cxuint argsNum;     ///< arguments number
    GalliumArg* args;   ///< arguments pointer
    
    GalliumKernel();
    GalliumKernel(const GalliumKernel& cp);
    GalliumKernel(GalliumKernel&& cp) noexcept;
    ~GalliumKernel();
    
    GalliumKernel& operator=(const GalliumKernel& cp);
    GalliumKernel& operator=(GalliumKernel&& cp) noexcept;
};

enum class GalliumSectionType: cxbyte
{
    TEXT = 0,   ///< section for text (binary code)
    DATA_CONSTANT,
    DATA_GLOBAL,
    DATA_LOCAL,
    DATA_PRIVATE
};

/// Gallium binarie's Section
struct GalliumSection
{
    uint32_t sectionId; ///< section id
    GalliumSectionType type;    ///< type of section
    uint32_t offset;    ///< section offset
    uint32_t size;      ///< size of section
};

/* INFO: in this file is used ULEV function for conversion
 * from LittleEndian and unaligned access to other memory access policy and endianness
 * Please use this function whenever you want to get or set word in ELF binary,
 * because ELF binaries can be unaligned in memory (as inner binaries).
 */
class GalliumElfBinary: public ElfBinary32
{
private:
    std::vector<GalliumProgramInfo> programInfos;
public:
    GalliumElfBinary(size_t binaryCodeSize, cxbyte* binaryCode, cxuint creationFlags);
    ~GalliumElfBinary();
    
    size_t getProgramInfosNum() const
    { return programInfos.size(); }
    
    const GalliumProgramInfo& getProgramInfo(uint32_t index) const
    { return programInfos[index]; }
    GalliumProgramInfo& getProgramInfo(uint32_t index)
    { return programInfos[index]; }
};

/** GalliumBinary object. This object converts to host-endian fields and
  * ULEV is not needed to access to fields of kernels and sections */
class GalliumBinary
{
private:
    /// symbol index map
    typedef std::unordered_multimap<std::string, size_t> SymbolIndexMap;
private:
    size_t binaryCodeSize;
    cxbyte* binaryCode;
    
    std::vector<GalliumKernel> kernels;
    std::vector<GalliumSection> sections;
    cxuint creationFlags;
    
    GalliumElfBinary elfBinary;
public:
    GalliumBinary();
    GalliumBinary(size_t binaryCodeSize, cxbyte* binaryCode, cxuint creationFlags);
    
    /// get creation flags
    cxuint getCreationFlags()
    { return creationFlags; }
    
    /// returns true if object has a symbol's index map
    bool hasSymbolMap() const
    { return (creationFlags & GALLIUM_KERNELSMAP) != 0; }
    
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
    uint32_t& getKernelIndex(const char* name) const;
    
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
