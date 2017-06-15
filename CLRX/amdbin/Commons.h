/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
    AMDHSAFLAG_USE_PRIVATE_SEGMENT_BUFFER = 1,
    AMDHSAFLAG_USE_DISPATCH_PTR = 2,
    AMDHSAFLAG_USE_QUEUE_PTR = 4,
    AMDHSAFLAG_USE_KERNARG_SEGMENT_PTR = 8,
    AMDHSAFLAG_USE_DISPATCH_ID = 16,
    AMDHSAFLAG_USE_FLAT_SCRATCH_INIT = 32,
    AMDHSAFLAG_USE_PRIVATE_SEGMENT_SIZE = 64,
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_BIT = 7,
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_X = 128,
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Y = 256,
    AMDHSAFLAG_USE_GRID_WORKGROUP_COUNT_Z = 512,
    
    AMDHSAFLAG_USE_ORDERED_APPEND_GDS = 1,
    AMDHSAFLAG_PRIVATE_ELEM_SIZE_BIT = 1,
    AMDHSAFLAG_USE_PTR64 = 8,
    AMDHSAFLAG_USE_DYNAMIC_CALL_STACK = 16,
    AMDHSAFLAG_USE_DEBUG_ENABLED = 32,
    AMDHSAFLAG_USE_XNACK_ENABLED = 64
};


struct AmdHsaKernelConfig
{
    uint32_t amdCodeVersionMajor;
    uint32_t amdCodeVersionMinor;
    uint16_t amdMachineKind;
    uint16_t amdMachineMajor;
    uint16_t amdMachineMinor;
    uint16_t amdMachineStepping;
    uint64_t kernelCodeEntryOffset;
    uint64_t kernelCodePrefetchOffset;
    uint64_t kernelCodePrefetchSize;
    uint64_t maxScrachBackingMemorySize;
    uint32_t computePgmRsrc1;
    uint32_t computePgmRsrc2;
    uint16_t enableSpgrRegisterFlags;
    uint16_t enableFeatureFlags;
    uint32_t workitemPrivateSegmentSize;
    uint32_t workgroupGroupSegmentSize;
    uint32_t gdsSegmentSize;
    uint64_t kernargSegmentSize;
    uint32_t workgroupFbarrierCount;
    uint16_t wavefrontSgprCount;
    uint16_t workitemVgprCount;
    uint16_t reservedVgprFirst;
    uint16_t reservedVgprCount;
    uint16_t reservedSgprFirst;
    uint16_t reservedSgprCount;
    uint16_t debugWavefrontPrivateSegmentOffsetSgpr;
    uint16_t debugPrivateSegmentBufferSgpr;
    cxbyte kernargSegmentAlignment;
    cxbyte groupSegmentAlignment;
    cxbyte privateSegmentAlignment;
    cxbyte wavefrontSize;
    uint32_t callConvention;
    uint32_t reserved1[3];
    uint64_t runtimeLoaderKernelSymbol;
    cxbyte controlDirective[128];
};

};

#endif
