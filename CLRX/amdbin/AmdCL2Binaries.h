/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2016 Mateusz Szpakowski
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
/*! \file AmdCL2Binaries.h
 * \brief AMD OpenCL 2.0 binaries handling
 */

#ifndef __CLRX_AMDCL2BINARIES_H__
#define __CLRX_AMDCL2BINARIES_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>

/// main namespace
namespace CLRX
{

enum : Flags {
    AMDBIN_INNER_CREATE_KERNELDATAS = 0x10000,    ///< create kernel setup
    AMDBIN_INNER_CREATE_KERNELDATASMAP = 0x20000,    ///< create kernel setups map
    AMDBIN_INNER_CREATE_KERNELSTUBS = 0x40000,    ///< create kernel stub
    AMDBIN_INNER_CREATE_KERNELSTUBSMAP = 0x80000    ///< create kernel stub map
};

/// AMD OpenCL 2.0 inner binary type
enum AmdCL2InnerBinaryType : cxbyte
{
    CAT15_7 = 0,
    OLD = 0,
    CRIMSON = 1,
    NEW = 1
};

/// AMD OpenCL 2.0 GPU metadata for kernel
struct AmdCL2GPUKernel
{
    size_t setupSize;   /// setup size
    cxbyte* setup;      /// < setup data
    size_t codeSize;    ///< size
    cxbyte* code;     ///< data
};

struct AmdCL2GPUKernelStub
{
    size_t size;   /// setup size
    cxbyte* data;      /// < setup data
};

/// AMD OpenCL 2.0 inner binary base class
class AmdCL2InnerGPUBinaryBase
{
public:
    /// inner binary map type
    typedef Array<std::pair<CString, size_t> > KernelDataMap;
private:
    AmdCL2InnerBinaryType binaryType;
    std::unique_ptr<AmdCL2GPUKernel[]> kernels;    ///< kernel headers
    KernelDataMap kernelDataMap;
public:
    ~AmdCL2InnerGPUBinaryBase() = default;
    
    /// get binary type
    AmdCL2InnerBinaryType getBinaryType() const
    { return binaryType; }
    
    /// get kernel setup size for specified inner binary
    size_t getKernelSetupSize(size_t index) const
    { return kernels[index].setupSize; }
    
    /// get kernel setup for specified inner binary
    const cxbyte* getKernelSetup(size_t index) const
    { return kernels[index].setup; }
    
    /// get kernel setup for specified inner binary
    cxbyte* getKernelSetup(size_t index)
    { return kernels[index].setup; }
    
    /// get kernel code size for specified inner binary
    size_t getKernelCodeSize(size_t index) const
    { return kernels[index].codeSize; }
    
    /// get kernel code for specified inner binary
    const cxbyte* getKernelCode(size_t index) const
    { return kernels[index].code; }
    
    /// get kernel code for specified inner binary
    cxbyte* getKernelCode(size_t index)
    { return kernels[index].code; }
};

/// AMD OpenCL 2.0 old inner binary for GPU binaries that represent a single kernel
class AmdCL2OldInnerGPUBinary: public NonCopyableAndNonMovable,
        public AmdCL2InnerGPUBinaryBase
{
private:
    /// ATTENTION: Do not put non-copyable stuff (likes pointers, arrays),
    /// because this object will copied
    std::unique_ptr<AmdCL2GPUKernelStub[]> kernelStubs;
public:
    AmdCL2OldInnerGPUBinary() = default;
    ~AmdCL2OldInnerGPUBinary() = default;
    
    /// get kernel stub size for specified inner binary
    size_t getKernelStubSize(size_t index) const
    { return kernelStubs[index].size; }
    
    /// get kernel stub for specified inner binary
    const cxbyte* getKernelStub(size_t index) const
    { return kernelStubs[index].data; }
    
    /// get kernel stub for specified inner binary
    cxbyte* getKernelStub(size_t index)
    { return kernelStubs[index].data; }
};

/// AMD OpenCL 2.0 inner binary for GPU binaries that represent a single kernel
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdCL2InnerGPUBinary: public AmdCL2InnerGPUBinaryBase, public ElfBinary64
{
private:
    /// ATTENTION: Do not put non-copyable stuff (likes pointers, arrays),
    /// because this object will copied
    CString kernelName; ///< kernel name
    size_t globalDataSize;  ///< global data size
    cxbyte* globalData; ///< global data content
public:
    AmdCL2InnerGPUBinary() = default;
    ~AmdCL2InnerGPUBinary() = default;
    
    /// get global data size
    size_t getGlobalDataSize() const
    { return globalDataSize; }
    
    /// get global data
    const cxbyte* getGlobalData() const
    { return globalData; }
    /// get global data
    cxbyte* getGlobalData()
    { return globalData; }
};

/// AMD OpenCL 2.0 GPU metadata for kernel
struct AmdCL2GPUKernelMetadata
{
    size_t size;    ///< size
    cxbyte* data;     ///< data
};

/// AMD OpenCL 2.0 main binary for GPU for 64-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdCL2MainGPUBinary: public AmdMainBinaryBase, public ElfBinary64
{
public:
    typedef Array<std::pair<CString, size_t> > MetadataMap;
protected:
    size_t kernelsNum;
    std::unique_ptr<AmdCL2GPUKernelMetadata[]> metadatas;  ///< AMD metadatas
    std::unique_ptr<AmdCL2GPUKernelMetadata[]> isaMetadatas;  ///< AMD metadatas
    MetadataMap metadataMap;
    MetadataMap isaMetadataMap;
    
    CString aclVersionString; ///< driver info string
    std::unique_ptr<AmdCL2InnerGPUBinaryBase> innerBinary;
public:
    AmdCL2MainGPUBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMDBIN_CREATE_ALL);
    ~AmdCL2MainGPUBinary() = default;
    
    /// returns true if binary has kernel informations
    bool hasKernelInfo() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0; }
    
    /// returns true if binary has kernel informations map
    bool hasKernelInfoMap() const
    { return (creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0; }
    
    /// get inner binary type
    AmdCL2InnerBinaryType getInnerBinaryType() const
    { return innerBinary.get()->getBinaryType(); }
    
    /// get inner binary
    const AmdCL2InnerGPUBinary& getInnerBinary() const
    { return *reinterpret_cast<const AmdCL2InnerGPUBinary*>(innerBinary.get()); }
    
    /// get inner binary
    AmdCL2InnerGPUBinary& getInnerBinary()
    { return *reinterpret_cast<AmdCL2InnerGPUBinary*>(innerBinary.get()); }
    
    /// get old inner binary
    const AmdCL2OldInnerGPUBinary& getOldInnerBinary() const
    { return *reinterpret_cast<const AmdCL2OldInnerGPUBinary*>(innerBinary.get()); }
    
    /// get old inner binary
    AmdCL2OldInnerGPUBinary& getOldInnerBinary()
    { return *reinterpret_cast<AmdCL2OldInnerGPUBinary*>(innerBinary.get()); }
    
    /// get ISA metadata size for specified inner binary
    size_t getISAMetadataSize(size_t index) const
    { return isaMetadatas[index].size; }
    
    /// get ISA metadata for specified inner binary
    const cxbyte* getISAMetadata(size_t index) const
    { return isaMetadatas[index].data; }
    
    /// get ISA metadata for specified inner binary
    cxbyte* getISAMetadata(size_t index)
    { return isaMetadatas[index].data; }
    
    /// get metadata size for specified inner binary
    size_t getMetadataSize(size_t index) const
    { return metadatas[index].size; }
    
    /// get metadata for specified inner binary
    const cxbyte* getMetadata(size_t index) const
    { return metadatas[index].data; }
    
    /// get metadata for specified inner binary
    cxbyte* getMetadata(size_t index)
    { return metadatas[index].data; }
    
    /// get acl version string
    const CString& getAclVersionString() const
    { return aclVersionString; }
};

};

#endif
