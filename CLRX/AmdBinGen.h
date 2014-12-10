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

struct AmdUserData
{
    uint32_t dataClass;
    uint32_t apiSlot;
    uint32_t regStart;
    uint32_t regSize;
};
    
struct AmdGPUBinKernelConfig
{
    std::vector<KernelArg> args;
    uint32_t reqdWorkGroupSize[3];
    uint32_t usedVGPRsNum;
    uint32_t usedSGPRsNum;
    uint32_t pgmRSRC2;
    uint32_t ieeeMode;
    uint32_t floatMode;
    size_t hwLocalSize;
    uint32_t scratchBufferSize;
    cxuint userDataElemsNum;
    AmdUserData userDatas[16];
};

struct AmdGPUBinExtKernelConfig
{
    uint32_t hwRegion;
    uint32_t uavPrivate;
    uint32_t uavId;
    bool cbIdEnable;
    bool printfIdEnable;
    uint32_t cbId;
    uint32_t printfId;
    uint32_t privateId;
    bool constDataRequired;
};

struct AmdGPUBinKernelInput
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
    bool useExtConfig;
    AmdGPUBinKernelConfig config;
    AmdGPUBinExtKernelConfig extConfig;
    size_t codeSize;
    const cxbyte* code;
};

extern std::vector<KernelArg> parseAmdKernelArgsFromString(
        const std::string& argsString);

class AmdGPUBinGenerator
{
private:
    bool is64Bit;
    uint32_t deviceType;
    size_t globalDataSize;
    const cxbyte* globalData;
    uint32_t driverVersion;
    std::string compileOptions;
    std::string driverInfo;
    std::vector<AmdGPUBinKernelInput> kernels;
    
    size_t binarySize;
    cxbyte* binary;
public:
    AmdGPUBinGenerator();
    ~AmdGPUBinGenerator();
    
    // non-copyable and non-movable
    AmdGPUBinGenerator(const AmdGPUBinGenerator& c) = delete;
    AmdGPUBinGenerator& operator=(const AmdGPUBinGenerator& c) = delete;
    AmdGPUBinGenerator(AmdGPUBinGenerator&& c) = delete;
    AmdGPUBinGenerator& operator=(AmdGPUBinGenerator&& c) = delete;
    
    void set64BitMode(bool _64bitMode)
    { is64Bit = _64bitMode; }
    void setDeviceType(uint32_t deviceType)
    { this->deviceType = deviceType; }
    
    void setGlobalData(size_t size, const cxbyte* globalData)
    {
        this->globalDataSize = size;
        this->globalData = globalData;
    }
    void setDriverInfo(const char* compileOptions, const char* driverInfo)
    {
        this->compileOptions = compileOptions;
        this->driverInfo = driverInfo;
    }
    void setDriverVersion(uint32_t version)
    { driverVersion = version; }
    
    void addKernel(const AmdGPUBinKernelInput& kernelInput);
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const AmdGPUBinKernelConfig& config,
           size_t dataSize = 0, const cxbyte* data = nullptr);
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const AmdGPUBinKernelConfig& config, const AmdGPUBinExtKernelConfig& extConfig,
           size_t dataSize = 0, const cxbyte* data = nullptr);
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const std::vector<CALNote>& calNotes, const cxbyte* header,
           size_t metadataSize, const cxbyte* metadata,
           size_t dataSize = 0, const cxbyte* data = nullptr);
    
    void generate();
    
    size_t getBinarySize() const
    { return binarySize; }
    const cxbyte* getBinary() const
    { return binary; }
};

};

#endif
