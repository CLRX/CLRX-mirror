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
/*! \file GalliumBinaries.h
 * \brief GalliumCompute binaries handling (only with LLVM 3.6)
 */

#ifndef __CLRX_GALLIUMBINARIES_H__
#define __CLRX_GALLIUMBINARIES_H__

#include <CLRX/Config.h>
#include <cstdint>
#include <string>
#include <vector>
#include <ostream>
#include <utility>
#include <memory>
#include <CLRX/amdbin/Commons.h>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/InputOutput.h>

/// main namespace
namespace CLRX
{

enum : Flags {
    GALLIUM_INNER_CREATE_SECTIONMAP = 0x10, ///< create map of sections for inner binaries
    GALLIUM_INNER_CREATE_SYMBOLMAP = 0x20,  ///< create map of kernels for inner binaries
    /** create map of dynamic kernels for inner binaries */
    GALLIUM_INNER_CREATE_DYNSYMMAP = 0x40,
    GALLIUM_INNER_CREATE_PROGINFOMAP = 0x100, ///< create prinfomap for inner binaries
    
    GALLIUM_ELF_CREATE_PROGINFOMAP = 0x10,  ///< create elf proginfomap
    
    GALLIUM_CREATE_ALL = ELF_CREATE_ALL | 0xfff0, ///< all Gallium binaries flags
    GALLIUM_INNER_SHIFT = 4 ///< shift for convert inner binary flags into elf binary flags
};

/// GalliumCompute Kernel Arg type
enum class GalliumArgType: cxbyte
{
    SCALAR = 0, ///< scalar type
    CONSTANT,   ///< constant pointer
    GLOBAL,     ///< global pointer
    LOCAL,      ///< local pointer
    IMAGE2D_RDONLY, ///< read-only 2d image
    IMAGE2D_WRONLY, ///< write-only 2d image
    IMAGE3D_RDONLY, ///< read-only 3d image
    IMAGE3D_WRONLY, ///< write-only 3d image
    SAMPLER,    ///< sampler
    MAX_VALUE = SAMPLER ///< last value
};

/// Gallium semantic field type
enum class GalliumArgSemantic: cxbyte
{
    GENERAL = 0,    ///< general
    GRID_DIMENSION,     ///< ???
    GRID_OFFSET,    ///< ???
    IMAGE_SIZE,     ///< image size
    IMAGE_FORMAT,   ///< image format
    MAX_VALUE = IMAGE_FORMAT ///< last value
};

/// kernel program info entry for Gallium binaries
struct GalliumProgInfoEntry
{
    uint32_t address;   ///< address
    uint32_t value;     ///< value
};

/// kernel argument (Gallium binaries)
struct GalliumArgInfo
{
    GalliumArgType type;    ///< type of argument
    bool signExtended;  ///< is signed extended
    GalliumArgSemantic semantic;    ///< semantic of array
    uint32_t size;  ///< size of argument
    uint32_t targetSize;    ///< target size
    uint32_t targetAlign;   ///< target alignment
};

/// kernel info structure (Gallium binaries)
struct GalliumKernel
{
    CString kernelName;   ///< kernel's name
    uint32_t sectionId; ///< section id
    uint32_t offset;    ///< offset in ElfBinary
    Array<GalliumArgInfo> argInfos;   ///< arguments
};

/// Gallium format section type
enum class GalliumSectionType: cxbyte
{
    TEXT = 0,   ///< section for text (binary code)
    DATA_CONSTANT,  ///< constant data (???)
    DATA_GLOBAL,    ///< global data (???)
    DATA_LOCAL,     ///< local data (???)
    DATA_PRIVATE,   ///< private data (???)
    TEXT_INTERMEDIATE_170 = 0,
    TEXT_LIBRARY_170,
    TEXT_EXECUTABLE_170,
    DATA_CONSTANT_170,
    DATA_GLOBAL_170,
    DATA_LOCAL_170,
    DATA_PRIVATE_170,
    MAX_VALUE = DATA_PRIVATE_170    ///< last value
};

/// Gallium binarie's Section
struct GalliumSection
{
    uint32_t sectionId; ///< section id
    GalliumSectionType type;    ///< type of section
    uint32_t offset;    ///< offset in binary
    uint32_t size;      ///< size of section
};

// scratch buffer relocation entry
struct GalliumScratchReloc
{
    size_t offset;  ///< offset
    RelocType type; ///< relocation type
};

/// Gallium elf binary base (for 32-bit and 64-bit)
class GalliumElfBinaryBase
{
public:
    /// program info entry index map
    typedef Array<std::pair<const char*, size_t> > ProgInfoEntryIndexMap;
protected:
    uint32_t progInfosNum;  ///< program info entries number
    GalliumProgInfoEntry* progInfoEntries;  ///< program info entries
    ProgInfoEntryIndexMap progInfoEntryMap; ///< program info map
    size_t disasmSize;  ///< disassembly size
    size_t disasmOffset;    ///< disassembly offset
    bool llvm390;   ///< true if >= LLVM 3.9
    Array<GalliumScratchReloc> scratchRelocs;
    
    /// routine to load binary fro internal ELF 
    template<typename ElfBinary>
    void loadFromElf(ElfBinary& elfBinary, size_t kernelsNum);
public:
    /// empty constructor
    GalliumElfBinaryBase();
    /// destructor
    virtual ~GalliumElfBinaryBase();
    
    /// returns program infos number
    uint32_t getProgramInfosNum() const
    { return progInfosNum; }
    
    /// returns number of program info entries for program info
    uint32_t getProgramInfoEntriesNum(uint32_t index) const;
    
    /// returns index for programinfo entries index for specified kernel name
    uint32_t getProgramInfoEntryIndex(const char* name) const;
    
    /// returns program info entries for specified kernel name
    const GalliumProgInfoEntry* getProgramInfo(const char* name) const
    { return progInfoEntries + getProgramInfoEntryIndex(name); }
    
    /// returns program info entries for specified kernel name
    GalliumProgInfoEntry* getProgramInfo(const char* name)
    { return progInfoEntries + getProgramInfoEntryIndex(name); }
    
    /// returns program info entries for specified kernel index
    const GalliumProgInfoEntry* getProgramInfo(uint32_t index) const;
    /// returns program info entries for specified kernel index
    GalliumProgInfoEntry* getProgramInfo(uint32_t index);
    
    /// returns true if disassembly available
    bool hasDisassembly() const
    { return disasmOffset != 0; }
    
    /// returns size of disassembly
    size_t getDisassemblySize() const
    { return disasmSize; }
    /// returns true binary for if >=LLVM 3.9
    bool isLLVM390() const
    { return llvm390; }
    
    /// return scratch buffer relocations number
    size_t getScratchRelocsNum() const
    { return scratchRelocs.size(); }
    
    /// return scratch buffer relocation by index
    const GalliumScratchReloc& getScratchReloc(size_t i) const
    { return scratchRelocs[i]; }
    
    /// return all scratch buffer relocations
    const Array<GalliumScratchReloc>& getScratchRelocs() const
    { return scratchRelocs; }
};

/* INFO: in this file is used ULEV function for conversion
 * from LittleEndian and unaligned access to other memory access policy and endianness
 * Please use this function whenever you want to get or set word in ELF binary,
 * because ELF binaries can be unaligned in memory (as inner binaries).
 */
/// 32-bit Gallium ELF binary
/** ULEV function is required to access programInfoEntry fields */
class GalliumElfBinary32: public GalliumElfBinaryBase, public ElfBinary32
{
private:
    size_t textRelsNum;
    size_t textRelEntrySize;
    cxbyte* textRel;
public:
    /// empty constructor
    GalliumElfBinary32();
    /// constructor
    GalliumElfBinary32(size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags,
            size_t kernelsNum);
    /// destructor
    virtual ~GalliumElfBinary32();
    
    /// return true if binary has program info map
    bool hasProgInfoMap() const
    { return (creationFlags & GALLIUM_ELF_CREATE_PROGINFOMAP) != 0; }
    
    /// return disassembly content (without null-character)
    const char* getDisassembly() const
    { return reinterpret_cast<const char*>(binaryCode + disasmOffset); }
    
    /// get text rel entries number
    size_t getTextRelEntriesNum() const
    { return textRelsNum; }
    /// get text rel entry
    const Elf32_Rel& getTextRelEntry(size_t index) const
    { return *reinterpret_cast<const Elf32_Rel*>(textRel + textRelEntrySize*index); }
    /// get text rel entry
    Elf32_Rel& getTextRelEntry(size_t index)
    { return *reinterpret_cast<Elf32_Rel*>(textRel + textRelEntrySize*index); }
};

/// 64-bit Gallium ELF binary
class GalliumElfBinary64: public GalliumElfBinaryBase, public ElfBinary64
{
private:
    size_t textRelsNum;
    size_t textRelEntrySize;
    cxbyte* textRel;
public:
    /// empty constructor
    GalliumElfBinary64();
    /// constructor
    GalliumElfBinary64(size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags,
            size_t kernelsNum);
    /// destructor
    virtual ~GalliumElfBinary64();
    
    /// return true if binary has program info map
    bool hasProgInfoMap() const
    { return (creationFlags & GALLIUM_ELF_CREATE_PROGINFOMAP) != 0; }
    
    /// return disassembly content (without null-character)
    const char* getDisassembly() const
    { return reinterpret_cast<const char*>(binaryCode + disasmOffset); }
    /// get text rel entries number
    size_t getTextRelEntriesNum() const
    { return textRelsNum; }
    /// get text rel entry
    const Elf64_Rel& getTextRelEntry(size_t index) const
    { return *reinterpret_cast<const Elf64_Rel*>(textRel + textRelEntrySize*index); }
    /// get text rel entry
    Elf64_Rel& getTextRelEntry(size_t index)
    { return *reinterpret_cast<Elf64_Rel*>(textRel + textRelEntrySize*index); }
};

/** GalliumBinary object. This object converts to host-endian fields and
  * ULEV is not needed to access to fields of kernels and sections */
class GalliumBinary: public NonCopyableAndNonMovable
{
private:
    Flags creationFlags;
    size_t binaryCodeSize;
    cxbyte* binaryCode;
    uint32_t kernelsNum;
    uint32_t sectionsNum;
    std::unique_ptr<GalliumKernel[]> kernels;
    std::unique_ptr<GalliumSection[]> sections;
    
    bool elf64BitBinary;
    std::unique_ptr<GalliumElfBinaryBase> elfBinary;
    bool mesa170;
    
public:
    /// constructor
    GalliumBinary(size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags);
    /// destructor
    ~GalliumBinary() = default;
    
    /// get creation flags
    Flags getCreationFlags()
    { return creationFlags; }
    
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
    /// return true if inner binary is 64-bit
    bool is64BitElfBinary() const
    { return elf64BitBinary; }
    
    /// returns Gallium inner ELF 32-bit binary
    GalliumElfBinary32& getElfBinary32()
    { return *static_cast<GalliumElfBinary32*>(elfBinary.get()); }
    
    /// returns Gallium inner ELF 32-bit binary
    const GalliumElfBinary32& getElfBinary32() const
    { return *static_cast<const GalliumElfBinary32*>(elfBinary.get()); }
    
    /// returns Gallium inner ELF 64-bit binary
    GalliumElfBinary64& getElfBinary64()
    { return *static_cast<GalliumElfBinary64*>(elfBinary.get()); }
    
    /// returns Gallium inner ELF 64-bit binary
    const GalliumElfBinary64& getElfBinary64() const
    { return *static_cast<const GalliumElfBinary64*>(elfBinary.get()); }
    
    /// get sections number
    uint32_t getSectionsNum() const
    { return sectionsNum; }
    
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
    { return kernelsNum;; }
    
    /// returns kernel index
    uint32_t getKernelIndex(const char* name) const;
    
    /// get kernel by index
    const GalliumKernel& getKernel(uint32_t index) const
    { return kernels[index]; }
    
    /// get kernel with speciified name
    const GalliumKernel& getKernel(const char* name) const
    { return kernels[getKernelIndex(name)]; }
    /// returns true if binary for >=Mesa3D 17.0
    bool isMesa170() const
    { return mesa170; };
};

enum: cxuint {
    GALLIUMSECTID_GPUCONFIG = ELFSECTID_OTHER_BUILTIN,
    GALLIUMSECTID_NOTEGNUSTACK,
    GALLIUMSECTID_RELTEXT,
    GALLIUMSECTID_MAX = GALLIUMSECTID_RELTEXT
};

/*
 * Gallium Binary generator
 */

/// kernel config
struct GalliumKernelConfig
{
    cxuint dimMask;    ///< mask of dimension (bits: 0 - X, 1 - Y, 2 - Z)
    cxuint usedVGPRsNum;  ///< number of used VGPRs
    cxuint usedSGPRsNum;  ///< number of used SGPRs
    uint32_t pgmRSRC1;      ///< pgmRSRC1 register value
    uint32_t pgmRSRC2;      ///< pgmRSRC2 register value
    cxbyte userDataNum;   ///< number of user data
    bool ieeeMode;  ///< IEEE mode
    cxbyte floatMode; ///< float mode
    cxbyte priority;    ///< priority
    cxbyte exceptions;      ///< enabled exceptions
    bool tgSize;        ///< enable TG_SIZE_EN bit
    bool debugMode;     ///< debug mode
    bool privilegedMode;   ///< prvileged mode
    bool dx10Clamp;     ///< DX10 CLAMP mode
    size_t localSize; ///< used local size (not local defined in kernel arguments)
    uint32_t scratchBufferSize; ///< size of scratch buffer
    cxuint spilledVGPRs;    ///< number of spilled vector registers
    cxuint spilledSGPRs;    ///< number of spilled scalar registers
};

/// kernel info structure (Gallium binaries)
struct GalliumKernelInput
{
    CString kernelName;   ///< kernel's name
    GalliumProgInfoEntry progInfo[5];   ///< program info for kernel
    bool useConfig;         ///< true if configuration has been used to generate binary
    GalliumKernelConfig config; ///< kernel's configuration
    uint32_t offset;    ///< offset of kernel code
    std::vector<GalliumArgInfo> argInfos;   ///< arguments
};

/// Gallium input
struct GalliumInput
{
    bool is64BitElf;   ///< is 64-bit elf binary
    bool isLLVM390;     ///< true if binary for >= LLVM 3.9
    bool isMesa170;     ///< true if binary for >= Mesa3D 17.0
    GPUDeviceType deviceType;   ///< GPU device type
    size_t globalDataSize;  ///< global constant data size
    const cxbyte* globalData;   ///< global constant data
    std::vector<GalliumKernelInput> kernels;    ///< input kernel list
    size_t codeSize;        ///< code size
    const cxbyte* code;     ///< code
    size_t commentSize; ///< comment size (can be null)
    const char* comment; ///< comment
    std::vector<BinSection> extraSections;  ///< extra sections
    std::vector<BinSymbol> extraSymbols;    ///< extra symbols
    std::vector<GalliumScratchReloc> scratchRelocs; ///< scratchbuffer relocations
    
    /// add empty kernel with default values
    void addEmptyKernel(const char* kernelName, cxuint llvmVersion);
};

/// gallium code binary generator
class GalliumBinGenerator: public NonCopyableAndNonMovable
{
private:
    bool manageable;
    const GalliumInput* input;
    
    void generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const;
public:
    GalliumBinGenerator();
    /// constructor with gallium input
    explicit GalliumBinGenerator(const GalliumInput* galliumInput);
    
    /// constructor
    /**
     * \param _64bitMode 64-bit elf binary
     * \param deviceType device type
     * \param codeSize size of code
     * \param code code pointer
     * \param globalDataSize global data size
     * \param globalData global data pointer
     * \param kernels vector of kernels
     */
    GalliumBinGenerator(bool _64bitMode, GPUDeviceType deviceType, size_t codeSize,
            const cxbyte* code, size_t globalDataSize, const cxbyte* globalData,
            const std::vector<GalliumKernelInput>& kernels);
    /// constructor
    GalliumBinGenerator(bool _64bitMode, GPUDeviceType deviceType, size_t codeSize,
            const cxbyte* code, size_t globalDataSize, const cxbyte* globalData,
            std::vector<GalliumKernelInput>&& kernels);
    ~GalliumBinGenerator();
    
    /// get input
    const GalliumInput* getInput() const
    { return input; }
    
    /// set input
    void setInput(const GalliumInput* input);
    
    /// generates binary to array of bytes
    void generate(Array<cxbyte>& array) const;
    
    /// generates binary to output stream
    void generate(std::ostream& os) const;
    
    /// generates binary to vector of char
    void generate(std::vector<char>& vector) const;
};

/// detect driver version in the system
extern uint32_t detectMesaDriverVersion();
/// detect LLVM compiler version in the system
extern uint32_t detectLLVMCompilerVersion();

};

#endif
