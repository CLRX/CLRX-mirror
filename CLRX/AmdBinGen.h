/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
/*! \file AmdBinGen.h
 * \brief AMD binaries generator
 */

#ifndef __CLRX_AMDBINGEN_H__
#define __CLRX_AMDBINGEN_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <CLRX/AmdBinaries.h>

/// main namespace
namespace CLRX
{
    
enum class GPUDeviceType: cxbyte
{
    UNDEFINED = 0,
    CAPE_VERDE, ///< Radeon HD7700
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

struct AmdUserData
{
    uint32_t dataClass;
    uint32_t apiSlot;
    uint32_t regStart;
    uint32_t regSize;
};
    
struct AmdKernelConfig
{
    std::vector<KernelArg> args;
    uint32_t reqdWorkGroupSize[3];
    uint32_t usedVGPRsNum;
    uint32_t usedSGPRsNum;
    uint32_t pgmRSRC2;
    uint32_t ieeeMode;
    uint32_t floatMode;
    size_t hwLocalSize;
    uint32_t hwRegion;
    uint32_t scratchBufferSize;
    uint32_t uavPrivate;
    uint32_t uavId;
    bool cbIdEnable;
    bool printfIdEnable;
    uint32_t cbId;
    uint32_t printfId;
    uint32_t privateId;
    bool constDataRequired;
    cxuint userDataElemsNum;
    AmdUserData userDatas[16];
};

struct AmdKernelInput
{
    std::string kernelName; ///< kernel name
    size_t dataSize;
    const cxbyte* dataCode;
    size_t headerSize;  ///< kernel header size
    const cxbyte* header;   ///< kernel header size
    size_t metadataSize;    ///< metadata size
    const char* metadata;   ///< kernel's metadata
    std::vector<CALNote> calNotes;
    bool useConfig;
    AmdKernelConfig config;
    size_t codeSize;
    const cxbyte* code;
};

struct AmdInput
{
    bool is64Bit;
    GPUDeviceType deviceType;
    size_t globalDataSize;
    const cxbyte* globalData;
    uint32_t driverVersion;
    std::string compileOptions;
    std::string driverInfo;
    std::vector<AmdKernelInput> kernels;
};

extern std::vector<KernelArg> parseAmdKernelArgsFromString(const std::string& argsString);

class AmdGPUBinGenerator
{
private:
    AmdInput input;
    
    size_t binarySize;
    cxbyte* binary;
public:
    AmdGPUBinGenerator();
    AmdGPUBinGenerator(const AmdInput& amdInput);
    AmdGPUBinGenerator(bool _64bitMode, GPUDeviceType deviceType, uint32_t driverVersion,
           size_t globalDataSize, const cxbyte* globalData, 
           const std::vector<AmdKernelInput>& kernelInputs);
    ~AmdGPUBinGenerator();
    
    // non-copyable and non-movable
    AmdGPUBinGenerator(const AmdGPUBinGenerator& c) = delete;
    AmdGPUBinGenerator& operator=(const AmdGPUBinGenerator& c) = delete;
    AmdGPUBinGenerator(AmdGPUBinGenerator&& c) = delete;
    AmdGPUBinGenerator& operator=(AmdGPUBinGenerator&& c) = delete;
    
    void setInput(const AmdInput& amdInput);
    void setKernels(const std::vector<AmdKernelInput>& kernelInputs);
    
    void set64BitMode(bool _64bitMode)
    { input.is64Bit = _64bitMode; }
    void setDeviceType(GPUDeviceType deviceType)
    { input.deviceType = deviceType; }
    
    void setGlobalData(size_t size, const cxbyte* globalData)
    {
        input.globalDataSize = size;
        input.globalData = globalData;
    }
    void setDriverInfo(const char* compileOptions, const char* driverInfo)
    {
        input.compileOptions = compileOptions;
        input.driverInfo = driverInfo;
    }
    void setDriverVersion(uint32_t version)
    { input.driverVersion = version; }
    
    void addKernel(const AmdKernelInput& kernelInput);
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const AmdKernelConfig& config, size_t dataSize = 0,
           const cxbyte* data = nullptr);
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const std::vector<CALNote>& calNotes, const cxbyte* header,
           size_t metadataSize, const char* metadata,
           size_t dataSize = 0, const cxbyte* data = nullptr);
    
    void generate();
    
    size_t getBinarySize() const
    { return binarySize; }
    const cxbyte* getBinary() const
    { return binary; }
};

};

#endif
