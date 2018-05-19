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
/*! \file amdbin/Commons.h
 * \brief common definitions for binaries
 */

#ifndef __CLRX_COMMONS_H__
#define __CLRX_COMMONS_H__

#include <CLRX/Config.h>
#include <CLRX/utils/MemAccess.h>

/// main namespace
namespace CLRX
{
/// relocation type
typedef cxuint RelocType;
    
enum
{
    RELTYPE_VALUE = 0,  ///< relocation that get value
    RELTYPE_LOW_32BIT,    ///< relocation that get low 32-bit of value
    RELTYPE_HIGH_32BIT    ///< relocation that get high 32-bit of value
};

enum {
    AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER = 1,  ///< use private segment buffer
    AMDHSAFLAG_USE_DISPATCH_PTR = 2,
    AMDHSAFLAG_USE_QUEUE_PTR = 4,       ///< use queue pointer
    AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR = 8, ///< use kernel argument segment pointer
    AMDHSAFLAG_USE_DISPATCH_ID = 16,
    AMDHSAFLAG_USE_FLAT_SCRATCH_INIT = 32,
    AMDHSAFLAG_USE_PRIVATE_SEGMENT_SIZE = 64,       ///< use private segment size
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_BIT = 7,
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_X = 128,    ///< use workgroup count for X dim
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Y = 256,    ///< use workgroup count for Y dim
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Z = 512,    ///< use workgroup count for Z dim
    
    AMDHSAFLAG_USE_ORDERED_APPEND_GDS = 1,  /// use ordered append gds
    AMDHSAFLAG_PRIVATE_ELEM_SIZE_BIT = 1,
    AMDHSAFLAG_USE_PTR64 = 8,       ///< use 64-bit pointers
    AMDHSAFLAG_USE_DYNAMIC_CALL_STACK = 16,
    AMDHSAFLAG_USE_DEBUG_ENABLED = 32,  ///< debug enabled
    AMDHSAFLAG_USE_XNACK_ENABLED = 64   ///< xnack enabled
};

/// AMD HSA kernel configuration structure
struct AmdHsaKernelConfig
{
    uint32_t amdCodeVersionMajor;   ///< AMD code version major number
    uint32_t amdCodeVersionMinor;   ///< AMD code version minor number
    uint16_t amdMachineKind;    ///< architecture kind
    uint16_t amdMachineMajor;   ///< arch major number
    uint16_t amdMachineMinor;   ///< arch minor number
    uint16_t amdMachineStepping;    ///< arch stepping number
    uint64_t kernelCodeEntryOffset;     ///< kernel relative to this config to kernel code
    uint64_t kernelCodePrefetchOffset;  ///< kernel code prefetch offset
    uint64_t kernelCodePrefetchSize;
    uint64_t maxScrachBackingMemorySize;
    uint32_t computePgmRsrc1;   ///< PGMRSRC1 register value
    uint32_t computePgmRsrc2;   ///< PGMRSRC2 register value
    uint16_t enableSgprRegisterFlags;   ///< bitfield of sg
    uint16_t enableFeatureFlags;    ///< bitfield of feature flags
    uint32_t workitemPrivateSegmentSize; ///< workitem private (scratchbuffer) segment size
    uint32_t workgroupGroupSegmentSize; ///< workgroup group segment (local memory) size
    uint32_t gdsSegmentSize;   ///< GDS segment size
    uint64_t kernargSegmentSize;    ///< kernel argument segment size
    uint32_t workgroupFbarrierCount;
    uint16_t wavefrontSgprCount;    ///< scalar register count per wavefront
    uint16_t workitemVgprCount;     ///< vector register count per workitem
    uint16_t reservedVgprFirst;     ///< reserved first vector register
    uint16_t reservedVgprCount;     ///< reserved vector register count
    uint16_t reservedSgprFirst;     ///< reserved first scalar register
    uint16_t reservedSgprCount;     ///< reserved scalar register count
    uint16_t debugWavefrontPrivateSegmentOffsetSgpr;
    uint16_t debugPrivateSegmentBufferSgpr;
    cxbyte kernargSegmentAlignment;     ///< kernel segment alignment
    cxbyte groupSegmentAlignment;       ///< group segment alignment
    cxbyte privateSegmentAlignment;     ///< private segment alignment
    cxbyte wavefrontSize;           ///< wavefront size
    uint32_t callConvention;        ///< call convention
    uint32_t reserved1[3];          ///< reserved
    uint64_t runtimeLoaderKernelSymbol;
    cxbyte controlDirective[128];       ///< control directives area
    
    void toLE()
    {
        SLEV(amdCodeVersionMajor, amdCodeVersionMajor);
        SLEV(amdCodeVersionMinor, amdCodeVersionMinor);
        SLEV(amdMachineKind, amdMachineKind);
        SLEV(amdMachineMajor, amdMachineMajor);
        SLEV(amdMachineMinor, amdMachineMinor);
        SLEV(amdMachineStepping, amdMachineStepping);
        SLEV(kernelCodeEntryOffset, kernelCodeEntryOffset);
        SLEV(kernelCodePrefetchOffset, kernelCodePrefetchOffset);
        SLEV(kernelCodePrefetchSize, kernelCodePrefetchSize);
        SLEV(maxScrachBackingMemorySize, maxScrachBackingMemorySize);
        SLEV(computePgmRsrc1, computePgmRsrc1);
        SLEV(computePgmRsrc2, computePgmRsrc2);
        SLEV(enableSgprRegisterFlags, enableSgprRegisterFlags);
        SLEV(enableFeatureFlags, enableFeatureFlags);
        SLEV(workitemPrivateSegmentSize, workitemPrivateSegmentSize);
        SLEV(workgroupGroupSegmentSize, workgroupGroupSegmentSize);
        SLEV(gdsSegmentSize, gdsSegmentSize);
        SLEV(kernargSegmentSize, kernargSegmentSize);
        SLEV(workgroupFbarrierCount, workgroupFbarrierCount);
        SLEV(wavefrontSgprCount, wavefrontSgprCount);
        SLEV(workitemVgprCount, workitemVgprCount);
        SLEV(reservedVgprFirst, reservedVgprFirst);
        SLEV(reservedVgprCount, reservedVgprCount);
        SLEV(reservedSgprFirst, reservedSgprFirst);
        SLEV(reservedSgprCount, reservedSgprCount);
        SLEV(debugWavefrontPrivateSegmentOffsetSgpr,
             debugWavefrontPrivateSegmentOffsetSgpr);
        SLEV(debugPrivateSegmentBufferSgpr, debugPrivateSegmentBufferSgpr);
        SLEV(callConvention, callConvention);
        SLEV(runtimeLoaderKernelSymbol, runtimeLoaderKernelSymbol);
    }
};

enum: cxuint {
    ASM_DIMMASK_SECONDFIELD_MASK = (7<<3),
    ASM_DIMMASK_SECONDFIELD_SHIFT = 3U,
    ASM_DIMMASK_SECONDFIELD_ENABLED = 0x100U
};

};

#endif
