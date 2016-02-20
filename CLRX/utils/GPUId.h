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
/*! \file GPUId.h
 * \brief GPU identification utilities
 */

#ifndef __CLRX_GPUID_H__
#define __CLRX_GPUID_H__

#include <CLRX/Config.h>
#include <string>

/// main namespace
namespace CLRX
{
/*
 * GPU identification utilities
 */

/// type of GPU device
enum class GPUDeviceType: cxbyte
{
    CAPE_VERDE = 0, ///< Radeon HD7700
    PITCAIRN, ///< Radeon HD7800
    TAHITI, ///< Radeon HD7900
    OLAND, ///< Radeon R7 250
    BONAIRE, ///< Radeon R7 260
    SPECTRE, ///< Kaveri
    SPOOKY, ///< Kaveri
    KALINDI, ///< ???  GCN1.1
    HAINAN, ///< ????  GCN1.0
    HAWAII, ///< Radeon R9 290
    ICELAND, ///< ???
    TONGA, ///< Radeon R9 285
    MULLINS, ///< ???
    FIJI,  ///< Radeon Fury
    CARRIZO, ///< APU
    DUMMY,
    GPUDEVICE_MAX = DUMMY,    ///< last value
    
    RADEON_HD7700 = CAPE_VERDE, ///< Radeon HD7700
    RADEON_HD7800 = PITCAIRN,   ///< Radeon HD7800
    RADEON_HD7900 = TAHITI,     ///< Radeon HD7900
    RADEON_R7_250 = OLAND,      ///< Radeon R7 250
    RADEON_R7_260 = BONAIRE,    ///< Radeon R7 260
    RADEON_R9_290 = HAWAII      ///< Radeon R7 290
};

/// GPU architecture
enum class GPUArchitecture: cxbyte
{
    GCN1_0 = 0, ///< first iteration (Radeon HD7000 series)
    GCN1_1,     ///< second iteration (Radeon Rx 200 series)
    GCN1_2,     ///< third iteration (Radeon Rx 300 series and Tonga)
    GPUARCH_MAX = GCN1_2    /// last value
};

/// get GPU device type from name
extern GPUDeviceType getGPUDeviceTypeFromName(const char* name);

/// get GPU device type name
extern const char* getGPUDeviceTypeName(GPUDeviceType deviceType);

/// get GPU architecture from name
extern GPUArchitecture getGPUArchitectureFromName(const char* name);

/// get GPUArchitecture from GPU device type
extern GPUArchitecture getGPUArchitectureFromDeviceType(GPUDeviceType deviceType);

/// get lowest GPU device for architecture
extern GPUDeviceType getLowestGPUDeviceTypeFromArchitecture(GPUArchitecture arch);

/// get GPU architecture name
extern const char* getGPUArchitectureName(GPUArchitecture architecture);

enum: cxuint {
    REGCOUNT_INCLUDE_VCC = 1
};

enum: cxuint {
    REGTYPE_SGPR = 0,
    REGTYPE_VGPR
};

/// get maximum available registers for GPU (type: 0 - scalar, 1 - vector)
extern cxuint getGPUMaxRegistersNum(GPUArchitecture architecture, cxuint regType,
                         cxuint flags = 0);

};

#endif
