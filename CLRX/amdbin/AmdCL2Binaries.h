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
    AMDBIN_CREATE_KERNELDATA = 0x10,    ///< create kernel setup
    AMDBIN_CREATE_KERNELDATAMAP = 0x20,    ///< create kernel setups map
    AMDBIN_CREATE_KERNELSTUBS = 0x40,    ///< create kernel stub
    
    AMDBIN_INNER_CREATE_KERNELDATA = 0x10000,    ///< create kernel setup
    AMDBIN_INNER_CREATE_KERNELDATAMAP = 0x20000,    ///< create kernel setups map
    AMDBIN_INNER_CREATE_KERNELSTUBS = 0x40000    ///< create kernel stub
};

/// AMD OpenCL 2.0 inner binary type
enum class AmdCL2InnerBinaryType : cxbyte
{
    CAT15_7 = 0,
    OLD = 0,
    CRIMSON = 1,
    NEW = 1
};

/// AMD OpenCL 2.0 GPU metadata for kernel
struct AmdCL2GPUKernel
{
    CString kernelName; ///< kernel name
    size_t setupSize;   ///< setup size
    cxbyte* setup;      /// < setup data
    size_t codeSize;    ///< size
    cxbyte* code;     ///< data
};

struct AmdCL2GPUKernelStub
{
    size_t size;   /// setup size
    cxbyte* data;      /// < setup data
};

class AmdCL2MainGPUBinary;

/// AMD OpenCL 2.0 inner binary base class
class AmdCL2InnerGPUBinaryBase
{
public:
    /// inner binary map type
    typedef Array<std::pair<CString, size_t> > KernelDataMap;
protected:
    AmdCL2InnerBinaryType binaryType;
    Array<AmdCL2GPUKernel> kernels;    ///< kernel headers
    KernelDataMap kernelDataMap;
    
    explicit AmdCL2InnerGPUBinaryBase(AmdCL2InnerBinaryType type);
public:
    virtual ~AmdCL2InnerGPUBinaryBase();
    
    /// get binary type
    AmdCL2InnerBinaryType getBinaryType() const
    { return binaryType; }
    
    size_t getKernelsNum() const
    { return kernels.size(); }
    
    /// get kernel data for specified index
    const AmdCL2GPUKernel& getKernelData(size_t index) const
    { return kernels[index]; }
    
    /// get kernel data for specified index
    AmdCL2GPUKernel& getKernelData(size_t index)
    { return kernels[index]; }
    
    /// get kernel data for specified kernel name
    const AmdCL2GPUKernel& getKernelData(const char* name) const;
};

/// AMD OpenCL 2.0 old inner binary for GPU binaries that represent a single kernel
class AmdCL2OldInnerGPUBinary: public NonCopyableAndNonMovable,
        public AmdCL2InnerGPUBinaryBase
{
private:
    Flags creationFlags;
    size_t binarySize;
    cxbyte* binary;
    std::unique_ptr<AmdCL2GPUKernelStub[]> kernelStubs;
public:
    AmdCL2OldInnerGPUBinary() = default;
    AmdCL2OldInnerGPUBinary(AmdCL2MainGPUBinary* mainBinary, size_t binaryCodeSize,
            cxbyte* binaryCode, Flags creationFlags = AMDBIN_CREATE_ALL);
    ~AmdCL2OldInnerGPUBinary() = default;
    
    /// return binary size
    size_t getSize() const
    { return binarySize; }
    /// return binary code
    const cxbyte* getBinaryCode() const
    { return binary; }
    /// return binary code
    cxbyte* getBinaryCode()
    { return binary; }
    
    /// return if binary has kernel datas
    bool hasKernelData() const
    { return creationFlags & AMDBIN_CREATE_KERNELDATA; }
    /// return if binary has kernel datas map
    bool hasKernelDataMap() const
    { return creationFlags & AMDBIN_CREATE_KERNELDATAMAP; }
    /// return if binary has kernel stubs
    bool hasKernelStubs() const
    { return creationFlags & AMDBIN_CREATE_KERNELSTUBS; }
    
    /// get kernel stub for specified index
    const AmdCL2GPUKernelStub& getKernelStub(size_t index) const
    { return kernelStubs[index]; }
    
    /// get kernel stub for specified kernel name
    const AmdCL2GPUKernelStub& getKernelStub(const char* name) const;
};

/// AMD OpenCL 2.0 inner binary for GPU binaries that represent a single kernel
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class AmdCL2InnerGPUBinary: public AmdCL2InnerGPUBinaryBase, public ElfBinary64
{
private:
    size_t globalDataSize;  ///< global data size
    cxbyte* globalData; ///< global data content
public:
    AmdCL2InnerGPUBinary() = default;
    AmdCL2InnerGPUBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = AMDBIN_CREATE_ALL);
    ~AmdCL2InnerGPUBinary() = default;
    
    /// return if binary has kernel datas
    bool hasKernelData() const
    { return creationFlags & AMDBIN_CREATE_KERNELDATA; }
    /// return if binary has kernel datas map
    bool hasKernelDataMap() const
    { return creationFlags & AMDBIN_CREATE_KERNELDATAMAP; }
    
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
    CString kernelName; ///< kernel name
    size_t size;    ///< size
    cxbyte* data;     ///< data
};

/// header for metadata
struct AmdCL2GPUMetadataHeader
{
    uint64_t size;      ///< size
    uint64_t metadataSize;  /// metadata size
    uint32_t unknown1[3];
    uint32_t options;
    uint64_t unknown2[3];
    uint64_t reqdWorkGroupSize[3];  ///< reqd work group size
    uint64_t unknown3[2];
    uint64_t firstNameLength;   ///< first name length
    uint64_t secondNameLength;  ///< second name length
    uint64_t unknown4[6];
    uint32_t argNum;
};

/// GPU kernel argument entry
struct AmdCL2GPUKernelArgEntry
{
    uint64_t size;      ///< entry size
    uint64_t argNameSize;   ///< argument name size
    uint64_t typeNameSize;  ///< type name size
    uint64_t unknown1, unknown2;
    union {
        uint32_t vectorLength;  ///< vector length (for old drivers not aligned)
        uint32_t resId;     ///< resource id
    };
    uint32_t unknown3;
    uint32_t argOffset; ///< virtual argument offset
    uint32_t argType;   ///< argument type
    uint32_t ptrAlignment; ///< pointer alignment
    uint32_t ptrType;   ///< pointer type
    uint32_t ptrSpace;  ///< pointer space
    uint32_t isPointerOrPipe;   /// nonzero if pointer or pipe
    cxbyte isVolatile;  ///< if pointer is volatile
    cxbyte isRestrict;  ///< if pointer is restrict
    cxbyte isPipe;      ///< if pipe
    cxbyte unknown4;
    uint32_t kindOfType;    ///< kind of type
    uint32_t isConst;   ///< is const pointer
    uint32_t unknown5;
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
    Array<AmdCL2GPUKernelMetadata> isaMetadatas;  ///< AMD metadatas
    std::unique_ptr<AmdGPUKernelHeader[]> kernelHeaders;
    MetadataMap isaMetadataMap;
    
    CString aclVersionString; ///< acl version string
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
    
    /// returns true if binary has info strings
    bool hasInfoStrings() const
    { return (creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0; }
    
    // returns true if inner binary exists
    bool hasInnerBinary() const
    { return innerBinary.get()!=nullptr; }
    
    /// get inner binary type
    AmdCL2InnerBinaryType getInnerBinaryType() const
    { return innerBinary->getBinaryType(); }
    
    /// get inner binary
    const AmdCL2InnerGPUBinary& getInnerBinary() const
    { return *static_cast<const AmdCL2InnerGPUBinary*>(innerBinary.get()); }
    
    /// get inner binary
    AmdCL2InnerGPUBinary& getInnerBinary()
    { return *static_cast<AmdCL2InnerGPUBinary*>(innerBinary.get()); }
    
    /// get old inner binary
    const AmdCL2OldInnerGPUBinary& getOldInnerBinary() const
    { return *static_cast<const AmdCL2OldInnerGPUBinary*>(innerBinary.get()); }
    
    /// get old inner binary
    AmdCL2OldInnerGPUBinary& getOldInnerBinary()
    { return *static_cast<AmdCL2OldInnerGPUBinary*>(innerBinary.get()); }
    
    /// get kernel header for specified index
    const AmdGPUKernelHeader& getKernelHeaderEntry(size_t index) const
    { return kernelHeaders[index]; }
    /// get kernel header for specified name
    const AmdGPUKernelHeader& getKernelHeaderEntry(const char* name) const;
    
    /// get number of ISA metadatas
    size_t getISAMetadatasNum() const
    { return isaMetadatas.size(); }
    
    /// get kernel ISA metadata by kernel index
    const AmdCL2GPUKernelMetadata& getISAMetadataEntry(size_t index) const
    { return isaMetadatas[index]; }
    /// get kernel ISA metadata by kernel name
    const AmdCL2GPUKernelMetadata& getISAMetadataEntry(const char* name) const;
    
    /// get ISA metadata size for specified inner binary
    size_t getISAMetadataSize(size_t index) const
    { return isaMetadatas[index].size; }
    
    /// get ISA metadata for specified inner binary
    const cxbyte* getISAMetadata(size_t index) const
    { return isaMetadatas[index].data; }
    
    /// get ISA metadata for specified inner binary
    cxbyte* getISAMetadata(size_t index)
    { return isaMetadatas[index].data; }
    
    /// get kernel metadata by index
    const AmdCL2GPUKernelMetadata& getMetadataEntry(size_t index) const
    { return metadatas[index]; }
    /// get kernel metadata by kernel name
    const AmdCL2GPUKernelMetadata& getMetadataEntry(const char* name) const;
    
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

/// check whether is Amd OpenCL 2.0 binary
extern bool isAmdCL2Binary(size_t binarySize, const cxbyte* binary);

};

#endif
