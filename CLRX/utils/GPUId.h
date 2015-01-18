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
    ICELAND, ///<
    TONGA, ///<
    MULLINS, //
    GPUDEVICE_MAX = MULLINS,
    
    RADEON_HD7700 = CAPE_VERDE,
    RADEON_HD7800 = PITCAIRN,
    RADEON_HD7900 = TAHITI,
    RADEON_R7_250 = OLAND,
    RADEON_R7_260 = BONAIRE,
    RADEON_R9_290 = HAWAII
};

enum class GPUArchitecture: cxbyte
{
    GCN1_0 = 0,
    GCN1_1,
    GCN1_2,
    GPUARCH_MAX = GCN1_2
};

/// get GPU device type from name
extern GPUDeviceType getGPUDeviceTypeFromName(const char* name);

/// get GPU device type name
extern const char* getGPUDeviceTypeName(GPUDeviceType deviceType);

/// get GPU architecture from name
extern GPUArchitecture getGPUArchitectureFromName(const char* name);

/// get GPUArchitecture from GPU device type
extern GPUArchitecture getGPUArchitectureFromDeviceType(GPUDeviceType deviceType);

/// get GPU architecture name
extern const char* getGPUArchitectureName(GPUArchitecture architecture);

};

#endif
