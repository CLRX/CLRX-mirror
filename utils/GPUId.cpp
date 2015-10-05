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

#include <CLRX/Config.h>
#include <cstring>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>

using namespace CLRX;

static const size_t gpuDeviceTableSize = 15;

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
    "Carrizo"
};

static std::pair<const char*, GPUDeviceType>
lowerCaseGpuDeviceEntryTable[gpuDeviceTableSize] =
{
    { "bonaire", GPUDeviceType::BONAIRE },
    { "capeverde", GPUDeviceType::CAPE_VERDE },
    { "carrizo", GPUDeviceType::CARRIZO },
    { "fiji", GPUDeviceType::FIJI },
    { "hainan", GPUDeviceType::HAINAN },
    { "hawaii", GPUDeviceType::HAWAII },
    { "iceland", GPUDeviceType::ICELAND },
    { "kalindi", GPUDeviceType::KALINDI },
    { "mullins", GPUDeviceType::MULLINS },
    { "oland", GPUDeviceType::OLAND },
    { "pitcairn", GPUDeviceType::PITCAIRN },
    { "spectre", GPUDeviceType::SPECTRE },
    { "spooky", GPUDeviceType::SPOOKY },
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
    GPUArchitecture::GCN1_2  // Carrizo
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
                         cxuint flags)
{
    if (architecture > GPUArchitecture::GPUARCH_MAX)
        throw Exception("Unknown GPU architecture");
    if (architecture != GPUArchitecture::GCN1_2)
        return (regType == 0) ? ((flags & REGCOUNT_INCLUDE_VCC) ? 104  : 102) : 256;
    else    /* really 102 sgprsNum only for GCN1.2 ??? (check) */
        return (regType == 0) ? ((flags & REGCOUNT_INCLUDE_VCC) ? 102 : 100) : 256;
}
