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
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>

using namespace CLRX;

static const char* gpuDeviceNameTable[13] =
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
    "Mullins"
};

static const GPUArchitecture gpuDeviceArchTable[13] =
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
    GPUArchitecture::GCN1_1 // Mullins
};

static const char* gpuArchitectureNameTable[3] =
{
    "GCN 1.0",
    "GCN 1.1",
    "GCN 1.2"
};

GPUDeviceType CLRX::getGPUDeviceTypeFromName(const char* name)
{
    cxuint found = 1;
    for (; found < sizeof gpuDeviceNameTable / sizeof(const char*); found++)
        if (::strcmp(name, gpuDeviceNameTable[found]) == 0)
            break;
    if (found == sizeof(gpuDeviceNameTable) / sizeof(const char*))
        throw Exception("Unknown GPU device type");
    return GPUDeviceType(found);
}

GPUArchitecture CLRX::getGPUArchitectureFromName(const char* name)
{
    cxuint found = 1;
    for (; found < sizeof gpuArchitectureNameTable / sizeof(const char*); found++)
        if (::strcmp(name, gpuArchitectureNameTable[found]) == 0)
            break;
    if (found == sizeof(gpuArchitectureNameTable) / sizeof(const char*))
        throw Exception("Unknown GPU architecture");
    return GPUArchitecture(found);
}

GPUArchitecture CLRX::getGPUArchitectureFromDeviceType(GPUDeviceType deviceType)
{
    return gpuDeviceArchTable[cxuint(deviceType)];
}

const char* CLRX::getGPUDeviceTypeName(GPUDeviceType deviceType)
{
    return gpuDeviceNameTable[cxuint(deviceType)];
}

const char* CLRX::getGPUArchitectureName(GPUArchitecture architecture)
{
    return gpuArchitectureNameTable[cxuint(architecture)];
}
