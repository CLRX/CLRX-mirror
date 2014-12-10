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

#include <CLRX/Config.h>
#include <elf.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinGen.h>

using namespace CLRX;

std::vector<KernelArg> CLRX::parseAmdKernelArgsFromString(const std::string& argsString)
{
    return std::vector<KernelArg>();
}

AmdGPUBinGenerator::AmdGPUBinGenerator() : binarySize(0), binary(nullptr)
{ }

AmdGPUBinGenerator::AmdGPUBinGenerator(const AmdInput& amdInput)
        : binarySize(0), binary(nullptr)
{
    setInput(amdInput);
}

AmdGPUBinGenerator::AmdGPUBinGenerator(bool _64bitMode, GPUDeviceType deviceType,
       uint32_t driverVersion, size_t globalDataSize, const cxbyte* globalData, 
       const std::vector<AmdKernelInput>& kernelInputs)
        : binarySize(0), binary(nullptr)
{
    input.is64Bit = _64bitMode;
    input.deviceType = deviceType;
    input.driverVersion = driverVersion;
    input.globalDataSize = globalDataSize;
    input.globalData = globalData;
    setKernels(kernelInputs);
}

AmdGPUBinGenerator::~AmdGPUBinGenerator()
{
    delete[] binary;
}

void AmdGPUBinGenerator::setInput(const AmdInput& amdInput)
{
    this->input = input;
}

void AmdGPUBinGenerator::setKernels(const std::vector<AmdKernelInput>& kernelInputs)
{
    input.kernels = kernelInputs;
}

void AmdGPUBinGenerator::addKernel(const AmdKernelInput& kernelInput)
{
    input.kernels.push_back(kernelInput);
}

void AmdGPUBinGenerator::addKernel(const char* kernelName, size_t codeSize,
       const cxbyte* code, const AmdKernelConfig& config,
       size_t dataSize, const cxbyte* data)
{
    input.kernels.push_back({ kernelName, dataSize, data, 0, nullptr, 0, nullptr, {},
                true, config, codeSize, code });
}

void AmdGPUBinGenerator::addKernel(const char* kernelName, size_t codeSize,
       const cxbyte* code, const std::vector<CALNote>& calNotes, const cxbyte* header,
       size_t metadataSize, const char* metadata, size_t dataSize, const cxbyte* data)
{
    input.kernels.push_back({ kernelName, dataSize, data, 32, header,
        metadataSize, metadata, calNotes, false, AmdKernelConfig(), codeSize, code });
}

void AmdGPUBinGenerator::generate()
{
}
