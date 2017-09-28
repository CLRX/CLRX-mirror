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

#include <CLRX/Config.h>
#include <cstring>
#include <utility>
#include <cstdint>
#include <climits>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>

using namespace CLRX;

// length of GPU device table (number of recognized GPU devices)
static const size_t gpuDeviceTableSize = 24;

static const char* gpuDeviceNameTable[gpuDeviceTableSize] =
{
    "CapeVerde",
    "Pitcairn",
    "Tahiti",
    "Oland",
    "Bonaire",
    "Spectre",
    "Spooky",
    "Kalindi",
    "Hainan",
    "Hawaii",
    "Iceland",
    "Tonga",
    "Mullins",
    "Fiji",
    "Carrizo",
    "Dummy",
    "Goose",
    "Horse",
    "Stoney",
    "Ellesmere",
    "Baffin",
    "GFX804",
    "GFX900",
    "GFX901"
};

// sorted GPU device names with device types
static const std::pair<const char*, GPUDeviceType>
lowerCaseGpuDeviceEntryTable[] =
{
    { "baffin", GPUDeviceType::BAFFIN },
    { "bonaire", GPUDeviceType::BONAIRE },
    { "capeverde", GPUDeviceType::CAPE_VERDE },
    { "carrizo", GPUDeviceType::CARRIZO },
    { "dummy", GPUDeviceType::DUMMY },
    { "ellesmere", GPUDeviceType::ELLESMERE },
    { "fiji", GPUDeviceType::FIJI },
    { "gfx804", GPUDeviceType::GFX804 },
    { "gfx900", GPUDeviceType::GFX900 },
    { "gfx901", GPUDeviceType::GFX901 },
    { "goose", GPUDeviceType::GOOSE },
    { "hainan", GPUDeviceType::HAINAN },
    { "hawaii", GPUDeviceType::HAWAII },
    { "horse", GPUDeviceType::HORSE },
    { "iceland", GPUDeviceType::ICELAND },
    { "kalindi", GPUDeviceType::KALINDI },
    { "mullins", GPUDeviceType::MULLINS },
    { "oland", GPUDeviceType::OLAND },
    { "pitcairn", GPUDeviceType::PITCAIRN },
    { "polaris10", GPUDeviceType::ELLESMERE },
    { "polaris11", GPUDeviceType::BAFFIN },
    { "polaris12", GPUDeviceType::GFX804 },
    { "polaris20", GPUDeviceType::ELLESMERE },
    { "polaris21", GPUDeviceType::BAFFIN },
    { "polaris22", GPUDeviceType::GFX804 },
    { "raven", GPUDeviceType::GFX901 },
    { "spectre", GPUDeviceType::SPECTRE },
    { "spooky", GPUDeviceType::SPOOKY },
    { "stoney", GPUDeviceType::STONEY },
    { "tahiti", GPUDeviceType::TAHITI },
    { "tonga", GPUDeviceType::TONGA },
    { "topaz", GPUDeviceType::ICELAND },
    { "vega10", GPUDeviceType::GFX900 },
    { "vega11", GPUDeviceType::GFX901 }
};

static const size_t lowerCaseGpuDeviceEntryTableSize =
    sizeof(lowerCaseGpuDeviceEntryTable) / sizeof(std::pair<const char*, GPUDeviceType>);

// table of architectures for specific GPU devices
static const GPUArchitecture gpuDeviceArchTable[gpuDeviceTableSize] =
{
    GPUArchitecture::GCN1_0, // CapeVerde
    GPUArchitecture::GCN1_0, // Pitcairn
    GPUArchitecture::GCN1_0, // Tahiti
    GPUArchitecture::GCN1_0, // Oland
    GPUArchitecture::GCN1_1, // Bonaire
    GPUArchitecture::GCN1_1, // Spectre
    GPUArchitecture::GCN1_1, // Spooky
    GPUArchitecture::GCN1_1, // Kalindi
    GPUArchitecture::GCN1_0, // Hainan
    GPUArchitecture::GCN1_1, // Hawaii
    GPUArchitecture::GCN1_2, // Iceland
    GPUArchitecture::GCN1_2, // Tonga
    GPUArchitecture::GCN1_1, // Mullins
    GPUArchitecture::GCN1_2, // Fiji
    GPUArchitecture::GCN1_2, // Carrizo
    GPUArchitecture::GCN1_2, // Dummy
    GPUArchitecture::GCN1_2, // Goose
    GPUArchitecture::GCN1_2, // Horse
    GPUArchitecture::GCN1_2, // Stoney
    GPUArchitecture::GCN1_2, // Ellesmere
    GPUArchitecture::GCN1_2, // Baffin
    GPUArchitecture::GCN1_2, // GFX804
    GPUArchitecture::GCN1_4, // GFX900
    GPUArchitecture::GCN1_4  // GFX901
};

static const char* gpuArchitectureNameTable[4] =
{
    "GCN1.0",
    "GCN1.1",
    "GCN1.2",
    "GCN1.4"
};

/* three names for every architecture (GCN, GFX?, Shortcut) used by recognizing
 * architecture by name */
static const char* gpuArchitectureNameTable2[12] =
{
    "GCN1.0", "GFX6", "SI",
    "GCN1.1", "GFX7", "CI",
    "GCN1.2", "GFX8", "VI",
    "GCN1.4", "GFX9", "Vega"
};

/// lowest device for architecture
static const GPUDeviceType gpuLowestDeviceFromArchTable[4] =
{
    GPUDeviceType::CAPE_VERDE,
    GPUDeviceType::BONAIRE,
    GPUDeviceType::ICELAND,
    GPUDeviceType::GFX900
};

GPUDeviceType CLRX::getGPUDeviceTypeFromName(const char* name)
{
    auto it = binaryMapFind(lowerCaseGpuDeviceEntryTable,
                 lowerCaseGpuDeviceEntryTable+lowerCaseGpuDeviceEntryTableSize,
                 name, CStringCaseLess());
    if (it == lowerCaseGpuDeviceEntryTable+lowerCaseGpuDeviceEntryTableSize)
        throw Exception("Unknown GPU device type");
    return it->second;
}

GPUArchitecture CLRX::getGPUArchitectureFromName(const char* name)
{
    cxuint found = 0;
    for (; found < sizeof gpuArchitectureNameTable2 /
                sizeof(const char*); found++)
        if (::strcasecmp(name, gpuArchitectureNameTable2[found]) == 0)
            break;
    if (found == sizeof(gpuArchitectureNameTable2) / sizeof(const char*))
        throw Exception("Unknown GPU architecture");
    return GPUArchitecture(found/3);
}

GPUArchitecture CLRX::getGPUArchitectureFromDeviceType(GPUDeviceType deviceType)
{
    if (deviceType > GPUDeviceType::GPUDEVICE_MAX)
        throw Exception("Unknown GPU device type");
    return gpuDeviceArchTable[cxuint(deviceType)];
}

GPUDeviceType CLRX::getLowestGPUDeviceTypeFromArchitecture(GPUArchitecture architecture)
{
    if (architecture > GPUArchitecture::GPUARCH_MAX)
        throw Exception("Unknown GPU architecture");
    return gpuLowestDeviceFromArchTable[cxuint(architecture)];
}

const char* CLRX::getGPUDeviceTypeName(GPUDeviceType deviceType)
{
    if (deviceType > GPUDeviceType::GPUDEVICE_MAX)
        throw Exception("Unknown GPU device type");
    return gpuDeviceNameTable[cxuint(deviceType)];
}

const char* CLRX::getGPUArchitectureName(GPUArchitecture architecture)
{
    if (architecture > GPUArchitecture::GPUARCH_MAX)
        throw Exception("Unknown GPU architecture");
    return gpuArchitectureNameTable[cxuint(architecture)];
}

cxuint CLRX::getGPUMaxRegistersNum(GPUArchitecture architecture, cxuint regType,
                         Flags flags)
{
    if (architecture > GPUArchitecture::GPUARCH_MAX)
        throw Exception("Unknown GPU architecture");
    if (regType == REGTYPE_VGPR)
        return 256; // VGPRS
    cxuint maxSgprs = (architecture>=GPUArchitecture::GCN1_2) ? 102 : 104;
    // subtract from max SGPRs num number of special registers (VCC, ...)
    if ((flags & REGCOUNT_NO_FLAT)!=0 && (architecture>GPUArchitecture::GCN1_0))
        maxSgprs -= (architecture>=GPUArchitecture::GCN1_2) ? 6 : 4;
    else if ((flags & REGCOUNT_NO_XNACK)!=0 && (architecture>GPUArchitecture::GCN1_1))
        maxSgprs -= 4;
    else if ((flags & REGCOUNT_NO_VCC)!=0)
        maxSgprs -= 2;
    return maxSgprs;
}

cxuint CLRX::getGPUMaxRegsNumByArchMask(uint16_t archMask, cxuint regType)
{
    if (regType == REGTYPE_VGPR)
        return 256;
    else
        return (archMask&(7U<<int(GPUArchitecture::GCN1_2))) ? 102 : 104;
}

void CLRX::getGPUSetupMinRegistersNum(GPUArchitecture architecture, cxuint dimMask,
              cxuint userDataNum, Flags flags, cxuint* gprsOut)
{
    /// SGPRs
    gprsOut[0] = ((dimMask&1)!=0) + ((dimMask&2)!=0) + ((dimMask&4)!=0);
    /// VGPRS
    gprsOut[1] = ((dimMask&4) ? 3 : ((dimMask&2) ? 2: (dimMask&1) ? 1 : 0));
    gprsOut[0] += userDataNum + ((flags & GPUSETUP_TGSIZE_EN)!=0) +
            ((flags & GPUSETUP_SCRATCH_EN)!=0);
}

size_t CLRX::getGPUMaxLocalSize(GPUArchitecture architecture)
{
    return 32768;
}

size_t CLRX::getGPUMaxGDSSize(GPUArchitecture architecture)
{
    return 65536;
}

// get extra (special) registers depends on architectures and flags
cxuint CLRX::getGPUExtraRegsNum(GPUArchitecture architecture, cxuint regType, Flags flags)
{
    if (regType == REGTYPE_VGPR)
        return 0;
    if ((flags & GCN_FLAT)!=0 && (architecture>GPUArchitecture::GCN1_0))
        return (architecture>=GPUArchitecture::GCN1_2) ? 6 : 4;
    else if ((flags & GCN_XNACK)!=0 && (architecture>GPUArchitecture::GCN1_1))
        return 4;
    else if ((flags & GCN_VCC)!=0)
        return 2;
    return 0;
}

uint32_t CLRX::calculatePgmRSrc1(GPUArchitecture arch, cxuint vgprsNum, cxuint sgprsNum,
            cxuint priority, cxuint floatMode, bool privMode, bool dx10Clamp,
            bool debugMode, bool ieeeMode)
{
    return ((vgprsNum-1)>>2) | (((sgprsNum-1)>>3)<<6) |
            ((uint32_t(floatMode)&0xff)<<12) |
            (ieeeMode?1U<<23:0) | (uint32_t(priority&3)<<10) |
            (privMode?1U<<20:0) | (dx10Clamp?1U<<21:0) |
            (debugMode?1U<<22:0);
}

uint32_t CLRX::calculatePgmRSrc2(GPUArchitecture arch, bool scratchEn, cxuint userDataNum,
            bool trapPresent, cxuint dimMask, cxuint defDimValues, bool tgSizeEn,
            cxuint ldsSize, cxuint exceptions)
{
    // GCN1.1 and later have 512 byte banks instead 256
    const cxuint ldsShift = arch<GPUArchitecture::GCN1_1 ? 8 : 9;
    const uint32_t ldsMask = (1U<<ldsShift)-1U;
    uint32_t dimValues = 0;
    // calculate dimMask (TGID_X_EN, ..., TIDIG_COMP_CNT fields)
    if (dimMask != UINT_MAX)
        dimValues = ((dimMask&7)<<7) | (((dimMask&4) ? 2 : (dimMask&2) ? 1 : 0)<<11);
    else // use default value for dimensions
        dimValues = defDimValues;
    return uint32_t(scratchEn ? 1U : 0U) | (uint32_t(userDataNum)<<1) |
            dimValues | (tgSizeEn ? 0x400U : 0U) | (trapPresent ? 0x40U : 0U) |
            (((uint32_t(ldsSize+ldsMask)>>ldsShift)&0x1ff)<<15) |
            ((uint32_t(exceptions)&0x7f)<<24);
}

// AMD GPU architecture for Gallium and ROCm
static const AMDGPUArchValues galliumGpuArchValuesTbl[] =
{
    { 0, 0, 0 }, // GPUDeviceType::CAPE_VERDE
    { 0, 0, 0 }, // GPUDeviceType::PITCAIRN
    { 0, 0, 0 }, // GPUDeviceType::TAHITI
    { 0, 0, 0 }, // GPUDeviceType::OLAND
    { 7, 0, 0 }, // GPUDeviceType::BONAIRE
    { 7, 0, 0 }, // GPUDeviceType::SPECTRE
    { 7, 0, 0 }, // GPUDeviceType::SPOOKY
    { 7, 0, 0 }, // GPUDeviceType::KALINDI
    { 0, 0, 0 }, // GPUDeviceType::HAINAN
    { 7, 0, 1 }, // GPUDeviceType::HAWAII
    { 8, 0, 0 }, // GPUDeviceType::ICELAND
    { 8, 0, 0 }, // GPUDeviceType::TONGA
    { 7, 0, 0 }, // GPUDeviceType::MULLINS
    { 8, 0, 3 }, // GPUDeviceType::FIJI
    { 8, 0, 1 }, // GPUDeviceType::CARRIZO
    { 0, 0, 0 }, // GPUDeviceType::DUMMY
    { 0, 0, 0 }, // GPUDeviceType::GOOSE
    { 0, 0, 0 }, // GPUDeviceType::HORSE
    { 8, 0, 1 }, // GPUDeviceType::STONEY
    { 8, 0, 4 }, // GPUDeviceType::ELLESMERE
    { 8, 0, 4 }, // GPUDeviceType::BAFFIN
    { 8, 0, 4 }, // GPUDeviceType::GFX804
    { 9, 0, 0 }, // GPUDeviceType::GFX900
    { 9, 0, 1 }  // GPUDeviceType::GFX901
};

// AMDGPU architecture values for specific GPU device type for AMDOCL 2.0
static const AMDGPUArchValues amdCL2GpuArchValuesTbl[] =
{
    { 0, 0, 0 }, // GPUDeviceType::CAPE_VERDE
    { 0, 0, 0 }, // GPUDeviceType::PITCAIRN
    { 0, 0, 0 }, // GPUDeviceType::TAHITI
    { 0, 0, 0 }, // GPUDeviceType::OLAND
    { 7, 0, 0 }, // GPUDeviceType::BONAIRE
    { 7, 0, 0 }, // GPUDeviceType::SPECTRE
    { 7, 0, 0 }, // GPUDeviceType::SPOOKY
    { 7, 0, 0 }, // GPUDeviceType::KALINDI
    { 0, 0, 0 }, // GPUDeviceType::HAINAN
    { 7, 0, 1 }, // GPUDeviceType::HAWAII
    { 8, 0, 0 }, // GPUDeviceType::ICELAND
    { 8, 0, 0 }, // GPUDeviceType::TONGA
    { 7, 0, 0 }, // GPUDeviceType::MULLINS
    { 8, 0, 4 }, // GPUDeviceType::FIJI
    { 8, 0, 1 }, // GPUDeviceType::CARRIZO
    { 8, 0, 1 }, // GPUDeviceType::DUMMY
    { 8, 0, 4 }, // GPUDeviceType::GOOSE
    { 8, 0, 4 }, // GPUDeviceType::HORSE
    { 8, 1, 0 }, // GPUDeviceType::STONEY
    { 8, 0, 4 }, // GPUDeviceType::ELLESMERE
    { 8, 0, 4 }, // GPUDeviceType::BAFFIN
    { 8, 0, 4 }, // GPUDeviceType::GFX804
    { 9, 0, 0 }, // GPUDeviceType::GFX900
    { 9, 0, 1 }  // GPUDeviceType::GFX901
};

AMDGPUArchValues CLRX::getGPUArchValues(GPUDeviceType deviceType, GPUArchValuesTable table)
{
    if (deviceType > GPUDeviceType::GPUDEVICE_MAX)
        throw Exception("Unknown GPU device type");
    // choose correct GPU arch values table
    const AMDGPUArchValues* archValuesTable = (table == GPUArchValuesTable::AMDCL2) ?
            amdCL2GpuArchValuesTbl : galliumGpuArchValuesTbl;
    return archValuesTable[cxuint(deviceType)];
}

// GPU arch values with device type
struct AMDGPUArchValuesEntry
{
    uint32_t major;
    uint32_t minor;
    uint32_t stepping;
    GPUDeviceType deviceType;
};

// list of AMDGPU arch entries for GPU devices
static const AMDGPUArchValuesEntry amdGpuArchValueEntriesTbl[] =
{
    { 0, 0, 0, GPUDeviceType::CAPE_VERDE },
    { 7, 0, 0, GPUDeviceType::BONAIRE },
    { 7, 0, 1, GPUDeviceType::HAWAII },
    { 8, 0, 0, GPUDeviceType::ICELAND },
    { 8, 0, 1, GPUDeviceType::CARRIZO },
    { 8, 0, 2, GPUDeviceType::ICELAND },
    { 8, 0, 3, GPUDeviceType::FIJI },
    { 8, 0, 4, GPUDeviceType::FIJI },
    { 8, 1, 0, GPUDeviceType::STONEY },
    { 9, 0, 0, GPUDeviceType::GFX900 },
    { 9, 0, 1, GPUDeviceType::GFX901 }
};

static const size_t amdGpuArchValueEntriesNum = sizeof(amdGpuArchValueEntriesTbl) /
                sizeof(AMDGPUArchValuesEntry);

GPUDeviceType CLRX::getGPUDeviceTypeFromArchValues(cxuint archMajor, cxuint archMinor,
                            cxuint archStepping)
{
    // determine device type
    GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE;
    // choose lowest GPU device by archMajor by default
    if (archMajor==0)
        deviceType = GPUDeviceType::CAPE_VERDE;
    else if (archMajor==7)
        deviceType = GPUDeviceType::BONAIRE;
    else if (archMajor==8)
        deviceType = GPUDeviceType::ICELAND;
    else if (archMajor==9)
        deviceType = GPUDeviceType::GFX900;
    
    // recognize device type by arch major, minor and stepping
    for (cxuint i = 0; i < amdGpuArchValueEntriesNum; i++)
        if (amdGpuArchValueEntriesTbl[i].major==archMajor &&
            amdGpuArchValueEntriesTbl[i].minor==archMinor &&
            amdGpuArchValueEntriesTbl[i].stepping==archStepping)
        {
            deviceType = amdGpuArchValueEntriesTbl[i].deviceType;
            break;
        }
    return deviceType;
}
