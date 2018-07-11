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
/*! \file GPUId.h
 * \brief GPU identification utilities
 */

#ifndef __CLRX_GPUID_H__
#define __CLRX_GPUID_H__

#include <CLRX/Config.h>
#include <CLRX/utils/Utilities.h>
#include <string>

/// main namespace
namespace CLRX
{

/// GPUId exception class
class GPUIdException: public Exception
{
public:
    /// empty constructor
    GPUIdException() = default;
    /// constructor with messasge
    explicit GPUIdException(const std::string& message);
    /// destructor
    virtual ~GPUIdException() noexcept = default;
};

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
    GOOSE,
    HORSE,
    STONEY,
    ELLESMERE,
    BAFFIN,
    GFX804,
    GFX900,
    GFX901,
    GFX902,
    GFX903,
    GFX904,
    GFX905,
    GFX906,
    GFX907,
    GPUDEVICE_MAX = GFX907,    ///< last value
    
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
    GCN1_4,     ///< GFX9 architecture (AMD RX VEGA)
    GCN1_4_1,    ///< GFX9 architecture with NN extensions (AMD VEGA 20)
    GPUARCH_MAX = GCN1_4_1    /// last value
};

/// GPU architecture mask (one bit represents single GPU architecture)
typedef uint16_t GPUArchMask;

// GCN architecture masks (bit represents architecture)
enum : GPUArchMask
{
    ARCH_SOUTHERN_ISLANDS = 1,
    ARCH_SEA_ISLANDS = 2,
    ARCH_VOLCANIC_ISLANDS = 4,
    ARCH_HD7X00 = 1,
    ARCH_RX2X0 = 2,
    ARCH_RX3X0 = 4,
    ARCH_RXVEGA = 8,
    ARCH_VEGA20 = 16,
    ARCH_GCN_1_0_1 = 0x3,
    ARCH_GCN_1_1_2 = 0x6,
    ARCH_GCN_1_1_2_4 = 0x1e,
    ARCH_GCN_1_2_4 = 0x1c,
    ARCH_GCN_1_4 = 0x18,
    ARCH_GCN_ALL = 0xffff
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

/// check whether is this GPU architecture
extern bool isThisGPUArchitecture(GPUArchitecture requiredArch, GPUArchitecture thisArch);

enum: Flags {
    REGCOUNT_NO_VCC = 1,
    REGCOUNT_NO_FLAT = 2,
    REGCOUNT_NO_XNACK = 4,
    REGCOUNT_NO_EXTRA = 0xffff
};

enum: cxuint {
    REGTYPE_SGPR = 0,
    REGTYPE_VGPR
};

enum : Flags
{
    GCN_VCC = 1,
    GCN_FLAT = 2,
    GCN_XNACK = 4
};

enum: Flags {
    GPUSETUP_TGSIZE_EN = 1,
    GPUSETUP_SCRATCH_EN = 2
};

enum: cxuint {
    MAX_REGTYPES_NUM = 4
};

/// get maximum available registers for GPU (type: 0 - scalar, 1 - vector)
extern cxuint getGPUMaxRegistersNum(GPUArchitecture architecture, cxuint regType,
                         Flags flags = 0);

/// get maximum available registers for GPU (type: 0 - scalar, 1 - vector)
extern cxuint getGPUMaxRegsNumByArchMask(GPUArchMask archMask, cxuint regType);

/// get maximum number of scalar register + extra scalar reg (VCC, FLAT_SCRATCH, ...)
extern bool isSpecialSGPRRegister(GPUArchMask archMask, cxuint index);

/// get minimal number of required registers
extern void getGPUSetupMinRegistersNum(GPUArchitecture architecture, cxuint dimMask,
               cxuint userDataNum, Flags flags, cxuint* gprsOut);

/// get default dimMask from PGMRSRC2
extern cxuint getDefaultDimMask(GPUArchitecture architecture, uint32_t pgmRSRC2);

/// get maximum local size for GPU architecture
extern size_t getGPUMaxLocalSize(GPUArchitecture architecture);

/// get maximum GDS size for GPU architecture
extern size_t getGPUMaxGDSSize(GPUArchitecture architecture);

/// get extra registers (like VCC,FLAT_SCRATCH)
extern cxuint getGPUExtraRegsNum(GPUArchitecture architecture, cxuint regType,
              Flags flags);

/// structure helper for AMDGPU architecture version
struct AMDGPUArchVersion
{
    uint32_t major;     ///< arch major number
    uint32_t minor;     ///< arch minor number
    uint32_t stepping;  ///< arch stepping number
};

/// calculate PGMRSRC1 register value
extern uint32_t calculatePgmRSrc1(GPUArchitecture arch, cxuint vgprsNum, cxuint sgprsNum,
            cxuint priority, cxuint floatMode, bool privMode, bool dx10clamp,
            bool debugMode, bool ieeeMode);

/// calculate PGMRSRC2 register value
extern uint32_t calculatePgmRSrc2(GPUArchitecture arch, bool scratchEn, cxuint userDataNum,
            bool trapPresent, cxuint dimMask, cxuint defDimValues, bool tgSizeEn,
            cxuint ldsSize, cxuint exceptions);


/// ADMGPUArchValues table type
enum class GPUArchVersionTable: cxuint
{
    AMDCL2 = 0, ///< AMD OpenCL 2.0 driver
    OPENSOURCE, ///< Gallium
    ROCM  ///< ROCm
};

/// get AMD GPU architecture version for specific device type and driver
extern AMDGPUArchVersion getGPUArchVersion(GPUDeviceType deviceType,
                        GPUArchVersionTable table);

// get GPU device type from architecture version
extern GPUDeviceType getGPUDeviceTypeFromArchVersion(cxuint archMajor, cxuint archMinor,
                            cxuint archStepping);

};

#endif
