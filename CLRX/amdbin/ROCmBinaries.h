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
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>

/// main namespace
namespace CLRX
{

enum : Flags {
    ROCMBIN_CREATE_KERNELMAP = 0x10,    ///< create kernel setups map
    
    ROCMBIN_CREATE_ALL = ELF_CREATE_ALL | 0xfff0 ///< all ROCm binaries flags
};

/// ROCm GPU metadata for kernel
struct ROCmKernel
{
    CString kernelName; ///< kernel name
    uint64_t setupOffset;      ///< setup data
    size_t codeSize;    ///< code size
    uint64_t codeOffset;     ///< code
};

/// ROCm main binary for GPU for 64-bit mode
/** This object doesn't copy binary code content.
 * Only it takes and uses a binary code.
 */
class ROCmBinary : public ElfBinary64, public NonCopyableAndNonMovable
{
public:
    typedef Array<std::pair<CString, size_t> > KernelMap;
private:
    size_t kernelsNum;
    std::unique_ptr<ROCmKernel[]> kernels;  ///< AMD metadatas
    KernelMap kernelsMap;
    size_t codeSize;
    cxbyte* code;
public:
    ROCmBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags = ROCMBIN_CREATE_ALL);
    ~ROCmBinary() = default;
    
    size_t getKernelsNum() const
    { return kernelsNum; }
    
    /// get kernel by index
    const ROCmKernel& getKernel(size_t index) const
    { return kernels[index]; }
    
    /// get kernel by name
    const ROCmKernel& getKernel(const char* name) const;
    
    /// get code size
    size_t getCodeSize() const
    { return codeSize; }
    /// get code
    const cxbyte* getCode() const
    { return code; }
    /// returns true if kernel map exists
    bool hasKernelMap() const
    { return (creationFlags & ROCMBIN_CREATE_KERNELMAP) != 0; };
};

/// check whether is Amd OpenCL 2.0 binary
extern bool isROCmBinary(size_t binarySize, const cxbyte* binary);

};

#endif
