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
/*! \file Amd3Binaries.h
 * \brief AMD binaries for CL2 for Navi handling
 */

#ifndef __CLRX_AMD3BINARIES_H__
#define __CLRX_AMD3BINARIES_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/amdbin/ElfBinaries.h>
#include <CLRX/amdbin/Commons.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/amdbin/ROCmBinaries.h>

namespace CLRX
{

enum : Flags {
    AMD3BIN_CREATE_REGIONMAP = 0x10,    ///< create region map
    AMD3BIN_CREATE_METADATAINFO = 0x20,     ///< create metadata info object
    AMD3BIN_CREATE_KERNELINFOMAP = 0x40,    ///< create kernel metadata info map
    AMD3BIN_CREATE_KERNELDESCMAP = 0x40,    ///< create kernel metadata info map
    AMD3BIN_CREATE_ALL = ELF_CREATE_ALL | 0xfff0 ///< all ROCm binaries flags
};

/// ROCm region/symbol type
enum class Amd3RegionType: uint8_t
{
    DATA,   ///< data object
    KERNEL  ///< OpenCL kernel to call ??
};

/// ROCm data region
struct Amd3Region
{
    CString regionName; ///< region name
    size_t size;    ///< data size
    size_t offset;     ///< data
    Amd3RegionType type; ///< type
};

typedef ROCmValueKind Amd3ValueKind;
typedef ROCmValueType Amd3ValueType;
typedef ROCmAddressSpace Amd3AddressSpace;
typedef ROCmAccessQual Amd3AccessQual;
typedef ROCmKernelArgInfo Amd3KernelArgInfo;
typedef ROCmKernelMetadata Amd3KernelMetadata;
typedef ROCmPrintfInfo Amd3PrintfInfo;
typedef ROCmMetadata Amd3Metadata;

struct Amd3KernelDescriptor
{
    uint32_t groupSegmentFixedSize;
    uint32_t privateSegmentFixedSize;
    uint64_t reserved0;
    uint64_t kernelCodeEntryOffset;
    uint64_t reserved1;
    cxbyte reserved2[12];
    uint32_t pgmRsrc3;
    uint32_t pgmRsrc1;
    uint32_t pgmRsrc2;
    uint16_t initialKernelExecState;
    cxbyte reserved3[6];
    
    void toLE()
    {
        SLEV(groupSegmentFixedSize, groupSegmentFixedSize);
        SLEV(privateSegmentFixedSize, privateSegmentFixedSize);
        SLEV(kernelCodeEntryOffset, kernelCodeEntryOffset);
        SLEV(pgmRsrc3, pgmRsrc3);
        SLEV(pgmRsrc1, pgmRsrc1);
        SLEV(pgmRsrc2, pgmRsrc2);
        SLEV(initialKernelExecState, initialKernelExecState);
    }
};

class Amd3Binary : public ElfBinary64, public NonCopyableAndNonMovable
{
public:
    /// region map type
    typedef Array<std::pair<CString, size_t> > RegionMap;
    typedef Array<std::pair<CString, size_t> > KernelDescMap;
private:
    size_t regionsNum;
    std::unique_ptr<Amd3Region[]> regions;  ///< AMD metadatas
    RegionMap regionsMap;
    size_t codeSize;
    cxbyte* code;
    size_t globalDataSize;
    cxbyte* globalData;
    CString target;
    size_t metadataSize;
    char* metadata;
    const cxbyte *kernelDescData;
    std::unique_ptr<Amd3Metadata> metadataInfo;
    Array<size_t> kernelDescOffsets;
    KernelDescMap kernelDescMap;
    RegionMap kernelInfosMap;
    Array<size_t> gotSymbols;
public:
    /// constructor
    Amd3Binary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMD3BIN_CREATE_ALL);
    /// default destructor
    ~Amd3Binary() = default;
    
    /// determine GPU device type from this binary
    GPUDeviceType determineGPUDeviceType(uint32_t& archMinor,
                     uint32_t& archStepping) const;
    /// get regions number
    size_t getRegionsNum() const
    { return regionsNum; }
    
    /// get region by index
    const Amd3Region& getRegion(size_t index) const
    { return regions[index]; }
    
    /// get region by name
    const Amd3Region& getRegion(const char* name) const;
    
    /// get code size
    size_t getCodeSize() const
    { return codeSize; }
    /// get code
    const cxbyte* getCode() const
    { return code; }
    /// get code
    cxbyte* getCode()
    { return code; }
    
    /// get global data size
    size_t getGlobalDataSize() const
    { return globalDataSize; }
    
    /// get global data
    const cxbyte* getGlobalData() const
    { return globalData; }
    /// get global data
    cxbyte* getGlobalData()
    { return globalData; }
    
    /// get metadata size
    size_t getMetadataSize() const
    { return metadataSize; }
    /// get metadata
    const char* getMetadata() const
    { return metadata; }
    /// get metadata
    char* getMetadata()
    { return metadata; }
    
    /// has metadata info
    bool hasMetadataInfo() const
    { return metadataInfo!=nullptr; }
    
    /// get metadata info
    const ROCmMetadata& getMetadataInfo() const
    { return *metadataInfo; }
    
    /// get kernel metadata infos number
    size_t getKernelInfosNum() const
    { return metadataInfo->kernels.size(); }
    
    /// get kernel metadata info
    const Amd3KernelMetadata& getKernelInfo(size_t index) const
    { return metadataInfo->kernels[index]; }
    
    /// get kernel metadata info by name
    const Amd3KernelMetadata& getKernelInfo(const char* name) const;
    
    /// get kernel descriptors number
    size_t getKernelDescsNum() const
    { return kernelDescOffsets.size(); }
    
    /// get kernel descriptor
    const Amd3KernelDescriptor& getKernelDesc(size_t index) const
    { return *reinterpret_cast<const Amd3KernelDescriptor*>(
                    kernelDescData + kernelDescOffsets[index]); }
    
    /// get kernel metadata info by name
    const Amd3KernelMetadata& getKernelDesc(const char* name) const;
    
    /// get target
    const CString& getTarget() const
    { return target; }
    
    /// get GOT symbol index (from elfbin dynsymbols)
    size_t getGotSymbolsNum() const
    { return gotSymbols.size(); }
    
    /// get GOT symbols (indices) (from elfbin dynsymbols)
    const Array<size_t> getGotSymbols() const
    { return gotSymbols; }
    
    /// get GOT symbol index (from elfbin dynsymbols)
    size_t getGotSymbol(size_t index) const
    { return gotSymbols[index]; }
    
    /// returns true if kernel map exists
    bool hasRegionMap() const
    { return (creationFlags & AMD3BIN_CREATE_REGIONMAP) != 0; }
    /// returns true if object has kernel info map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMD3BIN_CREATE_KERNELINFOMAP) != 0; }
    /// returns true if object has kernel descriptor map
    bool hasKernelDescMap() const
    { return (creationFlags & AMD3BIN_CREATE_KERNELDESCMAP) != 0; }
};

};

#endif
