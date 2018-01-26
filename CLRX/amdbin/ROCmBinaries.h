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
/*! \file ROCmBinaries.h
 * \brief ROCm binaries handling
 */

#ifndef __CLRX_ROCMBINARIES_H__
#define __CLRX_ROCMBINARIES_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/amdbin/Commons.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/InputOutput.h>

/// main namespace
namespace CLRX
{

enum : Flags {
    ROCMBIN_CREATE_REGIONMAP = 0x10,    ///< create region map
    ROCMBIN_CREATE_ALL = ELF_CREATE_ALL | 0xfff0 ///< all ROCm binaries flags
};

/// ROCm region/symbol type
enum ROCmRegionType: uint8_t
{
    DATA,   ///< data object
    FKERNEL,   ///< function kernel (code)
    KERNEL  ///< OpenCL kernel to call ??
};

/// ROCm data region
struct ROCmRegion
{
    CString regionName; ///< region name
    size_t size;    ///< data size
    size_t offset;     ///< data
    ROCmRegionType type; ///< type
};

/// ROCm main binary for GPU for 64-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class ROCmBinary : public ElfBinary64, public NonCopyableAndNonMovable
{
public:
    /// region map type
    typedef Array<std::pair<CString, size_t> > RegionMap;
private:
    size_t regionsNum;
    std::unique_ptr<ROCmRegion[]> regions;  ///< AMD metadatas
    RegionMap regionsMap;
    size_t codeSize;
    cxbyte* code;
public:
    /// constructor
    ROCmBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = ROCMBIN_CREATE_ALL);
    /// default destructor
    ~ROCmBinary() = default;
    
    /// determine GPU device type from this binary
    GPUDeviceType determineGPUDeviceType(uint32_t& archMinor,
                     uint32_t& archStepping) const;
    
    /// get regions number
    size_t getRegionsNum() const
    { return regionsNum; }
    
    /// get region by index
    const ROCmRegion& getRegion(size_t index) const
    { return regions[index]; }
    
    /// get region by name
    const ROCmRegion& getRegion(const char* name) const;
    
    /// get code size
    size_t getCodeSize() const
    { return codeSize; }
    /// get code
    const cxbyte* getCode() const
    { return code; }
    
    /// returns true if kernel map exists
    bool hasRegionMap() const
    { return (creationFlags & ROCMBIN_CREATE_REGIONMAP) != 0; };
};

enum {
    ROCMFLAG_USE_PRIVATE_SEGMENT_BUFFER = AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER,
    ROCMFLAG_USE_DISPATCH_PTR = AMDHSAFLAG_USE_DISPATCH_PTR,
    ROCMFLAG_USE_QUEUE_PTR = AMDHSAFLAG_USE_QUEUE_PTR,
    ROCMFLAG_USE_KERNARG_SEGMENT_PTR = AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR,
    ROCMFLAG_USE_DISPATCH_ID = AMDHSAFLAG_USE_DISPATCH_ID,
    ROCMFLAG_USE_FLAT_SCRATCH_INIT = AMDHSAFLAG_USE_FLAT_SCRATCH_INIT,
    ROCMFLAG_USE_PRIVATE_SEGMENT_SIZE = AMDHSAFLAG_USE_PRIVATE_SEGMENT_SIZE,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_BIT = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_BIT,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_X = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_X,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Y = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Y,
    ROCMFLAG_USE_GRID_WORKGROUP_COUNT_Z = AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Z,
    
    ROCMFLAG_USE_ORDERED_APPEND_GDS = AMDHSAFLAG_USE_ORDERED_APPEND_GDS,
    ROCMFLAG_PRIVATE_ELEM_SIZE_BIT = AMDHSAFLAG_PRIVATE_ELEM_SIZE_BIT,
    ROCMFLAG_USE_PTR64 = AMDHSAFLAG_USE_PTR64,
    ROCMFLAG_USE_DYNAMIC_CALL_STACK = AMDHSAFLAG_USE_DYNAMIC_CALL_STACK,
    ROCMFLAG_USE_DEBUG_ENABLED = AMDHSAFLAG_USE_DEBUG_ENABLED,
    ROCMFLAG_USE_XNACK_ENABLED = AMDHSAFLAG_USE_XNACK_ENABLED
};

/// ROCm kernel configuration structure
typedef AmdHsaKernelConfig ROCmKernelConfig;

/// check whether is Amd OpenCL 2.0 binary
extern bool isROCmBinary(size_t binarySize, const cxbyte* binary);

/*
 * ROCm Binary Generator
 */

enum: cxuint {
    ROCMSECTID_HASH = ELFSECTID_OTHER_BUILTIN,
    ROCMSECTID_DYNAMIC,
    ROCMSECTID_NOTE,
    ROCMSECTID_GPUCONFIG,
    ROCMSECTID_MAX = ROCMSECTID_GPUCONFIG
};

/// ROCm binary symbol input
struct ROCmSymbolInput
{
    CString symbolName; ///< symbol name
    size_t offset;  ///< offset in code
    size_t size;    ///< size of symbol
    ROCmRegionType type;  ///< type
};

/// ROCm binary input structure
struct ROCmInput
{
    GPUDeviceType deviceType;   ///< GPU device type
    uint32_t archMinor;         ///< GPU arch minor
    uint32_t archStepping;      ///< GPU arch stepping
    std::vector<ROCmSymbolInput> symbols;   ///< symbols
    size_t codeSize;        ///< code size
    const cxbyte* code;     ///< code
    size_t commentSize; ///< comment size (can be null)
    const char* comment; ///< comment
    std::vector<BinSection> extraSections;  ///< extra sections
    std::vector<BinSymbol> extraSymbols;    ///< extra symbols
    uint32_t eflags;    ///< ELF headef e_flags field
    
    /// add empty kernel with default values
    void addEmptyKernel(const char* kernelName);
};

/// ROCm binary generator
class ROCmBinGenerator: public NonCopyableAndNonMovable
{
private:
    private:
    bool manageable;
    const ROCmInput* input;
    
    void generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const;
public:
    /// constructor
    ROCmBinGenerator();
    /// constructor with ROCm input
    explicit ROCmBinGenerator(const ROCmInput* rocmInput);
    
    /// constructor
    /**
     * \param deviceType device type
     * \param archMinor architecture minor number
     * \param archStepping architecture stepping number
     * \param codeSize size of code
     * \param code code pointer
     * \param symbols symbols (kernels, datas,...)
     */
    ROCmBinGenerator(GPUDeviceType deviceType, uint32_t archMinor, uint32_t archStepping,
            size_t codeSize, const cxbyte* code,
            const std::vector<ROCmSymbolInput>& symbols);
    /// constructor
    ROCmBinGenerator(GPUDeviceType deviceType, uint32_t archMinor, uint32_t archStepping,
            size_t codeSize, const cxbyte* code,
            std::vector<ROCmSymbolInput>&& symbols);
    /// destructor
    ~ROCmBinGenerator();
    
    /// get input
    const ROCmInput* getInput() const
    { return input; }
    
    /// set input
    void setInput(const ROCmInput* input);
    
    /// generates binary to array of bytes
    void generate(Array<cxbyte>& array) const;
    
    /// generates binary to output stream
    void generate(std::ostream& os) const;
    
    /// generates binary to vector of char
    void generate(std::vector<char>& vector) const;
};

};

#endif
