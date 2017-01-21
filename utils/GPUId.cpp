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
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>

using namespace CLRX;

static const size_t gpuDeviceTableSize = 21;

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
    "Baffin"
};

static std::pair<const char*, GPUDeviceType>
lowerCaseGpuDeviceEntryTable[gpuDeviceTableSize] =
{
    { "baffin", GPUDeviceType::BAFFIN },
    { "bonaire", GPUDeviceType::BONAIRE },
    { "capeverde", GPUDeviceType::CAPE_VERDE },
    { "carrizo", GPUDeviceType::CARRIZO },
    { "dummy", GPUDeviceType::DUMMY },
    { "ellesmere", GPUDeviceType::ELLESMERE },
    { "fiji", GPUDeviceType::FIJI },
    { "goose", GPUDeviceType::GOOSE },
    { "hainan", GPUDeviceType::HAINAN },
    { "hawaii", GPUDeviceType::HAWAII },
    { "horse", GPUDeviceType::HORSE },
    { "iceland", GPUDeviceType::ICELAND },
    { "kalindi", GPUDeviceType::KALINDI },
    { "mullins", GPUDeviceType::MULLINS },
    { "oland", GPUDeviceType::OLAND },
    { "pitcairn", GPUDeviceType::PITCAIRN },
    { "spectre", GPUDeviceType::SPECTRE },
    { "spooky", GPUDeviceType::SPOOKY },
    { "stoney", GPUDeviceType::STONEY },
    { "tahiti", GPUDeviceType::TAHITI },
    { "tonga", GPUDeviceType::TONGA }
};

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
    GPUArchitecture::GCN1_2  // Baffin
};

static const char* gpuArchitectureNameTable[3] =
{
    "GCN1.0",
    "GCN1.1",
    "GCN1.2"
};

static const GPUDeviceType gpuLowestDeviceFromArchTable[3] =
{
    GPUDeviceType::CAPE_VERDE,
    GPUDeviceType::BONAIRE,
    GPUDeviceType::TONGA
};

GPUDeviceType CLRX::getGPUDeviceTypeFromName(const char* name)
{
    auto it = binaryMapFind(lowerCaseGpuDeviceEntryTable,
                 lowerCaseGpuDeviceEntryTable+gpuDeviceTableSize, name, CStringCaseLess());
    if (it == lowerCaseGpuDeviceEntryTable+gpuDeviceTableSize)
        throw Exception("Unknown GPU device type");
    return it->second;
}

GPUArchitecture CLRX::getGPUArchitectureFromName(const char* name)
{
    cxuint found = 0;
    for (; found < sizeof gpuArchitectureNameTable / sizeof(const char*); found++)
        if (::strcasecmp(name, gpuArchitectureNameTable[found]) == 0)
            break;
    if (found == sizeof(gpuArchitectureNameTable) / sizeof(const char*))
        throw Exception("Unknown GPU architecture");
    return GPUArchitecture(found);
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
    if (regType == 1)
        return 256; // VGPRS
    cxuint maxSpgrs = (architecture==GPUArchitecture::GCN1_2) ? 102 : 104;
    if ((flags & REGCOUNT_NO_FLAT)!=0 && (architecture>GPUArchitecture::GCN1_0))
        maxSpgrs -= (architecture==GPUArchitecture::GCN1_2) ? 6 : 4;
    else if ((flags & REGCOUNT_NO_XNACK)!=0 && (architecture>GPUArchitecture::GCN1_1))
        maxSpgrs -= 4;
    else if ((flags & REGCOUNT_NO_VCC)!=0)
        maxSpgrs -= 2;
    return maxSpgrs;
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

cxuint CLRX::getGPUExtraRegsNum(GPUArchitecture architecture, cxuint regType, Flags flags)
{
    if (regType == 1)
        return 0;
    if ((flags & GCN_FLAT)!=0 && (architecture>GPUArchitecture::GCN1_0))
        return (architecture==GPUArchitecture::GCN1_2) ? 6 : 4;
    else if ((flags & GCN_XNACK)!=0 && (architecture>GPUArchitecture::GCN1_1))
        return 4;
    else if ((flags & REGCOUNT_NO_VCC)!=0)
        return 2;
    return 0;
}