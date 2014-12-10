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
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinGen.h>

using namespace CLRX;

std::vector<AmdKernelArg> CLRX::parseAmdKernelArgsFromString(
            const std::string& argsString)
{
    return std::vector<AmdKernelArg>();
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
       const cxbyte* code, const std::vector<BinCALNote>& calNotes, const cxbyte* header,
       size_t metadataSize, const char* metadata, size_t dataSize, const cxbyte* data)
{
    input.kernels.push_back({ kernelName, dataSize, data, 32, header,
        metadataSize, metadata, calNotes, false, AmdKernelConfig(), codeSize, code });
}

static const char* gpuDeviceNameTable[14] =
{
    "UNDEFINED",
    "capeverde",
    "pitcairn",
    "tahiti",
    "oland",
    "bonaire",
    "spectre",
    "spooky",
    "kalindi",
    "hainan",
    "hawaii",
    "iceland",
    "tonga",
    "mullins"
};

void AmdGPUBinGenerator::generate()
{
    const size_t kernelsNum = input.kernels.size();
    if (input.driverInfo.empty())
    {
        char drvInfo[100];
        snprintf(drvInfo, 100, "@(#) OpenCL 1.2 AMD-APP (%u.%u).  "
                "Driver version: %u.%u (VM)",
                 input.driverVersion/100U, input.driverVersion%100U,
                 input.driverVersion/100U, input.driverVersion%100U);
        input.driverInfo = drvInfo;
    }
    else if (input.driverVersion == 0)
    {   // parse version
        size_t pos = input.driverInfo.find("AMD-APP"); // find AMDAPP
        try
        {
            if (pos != std::string::npos)
            {   /* let to parse version number */
                pos += 8;
                const char* end;
                input.driverVersion = cstrtovCStyle<cxuint>(
                        input.driverInfo.c_str()+pos, nullptr, end)*100;
                end++;
                input.driverVersion += cstrtovCStyle<cxuint>(end, nullptr, end);
            }
        }
        catch(const ParseException& ex)
        { input.driverVersion = 99999909U; /* newest possible */ }
    }
    const bool isOlderThan1124 = input.driverVersion < 112402;
    const bool isOlderThan1384 = input.driverVersion < 138405;
    /* count number of bytes required to save */
    if (!input.is64Bit)
        binarySize = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr)*7 +
                sizeof(Elf32_Sym)*(3*kernelsNum + 2);
    else
        binarySize = sizeof(Elf64_Ehdr) + sizeof(Elf64_Shdr)*7 +
                sizeof(Elf64_Sym)*(3*kernelsNum + 2);
    
    for (const AmdKernelInput& kinput: input.kernels)
        binarySize += (kinput.kernelName.size())*3;
    binarySize += 50 /*shstrtab */ + kernelsNum*(19+17+17) + 26/* static strtab size */ +
            input.driverInfo.size() + input.compileOptions.size() + input.globalDataSize;
    /* kernel inner binaries */
    binarySize += (sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)*3 + sizeof(Elf32_Shdr)*6 +
            sizeof(CALEncodingEntry) + 2 + 16 + 40 + 32/*header*/)*kernelsNum;
    
    std::vector<std::string> kmetadatas(input.kernels.size());
    size_t uniqueId = 1025;
    for (size_t i = 0; i < input.kernels.size(); i++)
    {
        const AmdKernelInput& kinput = input.kernels[i];
        const AmdKernelConfig& config = kinput.config;
        size_t readOnlyImages = 0;
        size_t uavsNum = 1;
        size_t samplersNum = config.samplers.size();
        size_t constBuffersNum = 2;
        for (const AmdKernelArg& arg: config.args)
        {
            if (arg.argType >= KernelArgType::MIN_IMAGE &&
                arg.argType <= KernelArgType::MAX_IMAGE)
            {
                if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_READ_ONLY)
                    readOnlyImages++;
                if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                    uavsNum++;
            }
            else if (arg.argType == KernelArgType::POINTER)
            {
                if (arg.ptrSpace == KernelPtrSpace::GLOBAL)
                    uavsNum++;
                if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
                    constBuffersNum++;
            }
           else if (arg.argType == KernelArgType::SAMPLER)
               samplersNum++;
        }
        binarySize += (kinput.data != nullptr) ? kinput.dataSize : 4736;
        binarySize += kinput.codeSize;
        // CAL notes size
        if (kinput.useConfig)
        {
            binarySize += 20*17 /*calNoteHeaders*/ + 16 + 128 + (18+32 +
                2*((isOlderThan1124)?16:config.userDataElemsNum))*8 /* proginfo */ +
                    readOnlyImages*4 /* inputs */ + 16*uavsNum /* uavs */ +
                    8*samplersNum /* samplers */ + 8*constBuffersNum /* cbids */;
            
            /* compute metadataSize */
            std::string& metadata = kmetadatas[i];
            metadata.reserve(100);
            metadata += ";ARGSTART:__OpenCL_";
            metadata += kinput.kernelName;
            metadata += "_kernel\n";
            if (isOlderThan1124)
                metadata += ";version:3:1:104\n";
            else
                metadata += ";version:3:1:111\n";
            metadata += ";device:";
            metadata += gpuDeviceNameTable[cxuint(input.deviceType)];
            char numBuf[21];
            metadata += "\n;uniqueid:";
            itocstrCStyle(uniqueId, numBuf, 21);
            metadata += numBuf;
            metadata += "\n;memory:uavprivate:";
            itocstrCStyle(config.uavPrivate, numBuf, 21);
            metadata += numBuf;
            metadata += "\n;memory:hwlocal:";
            itocstrCStyle(config.hwLocalSize, numBuf, 21);
            metadata += numBuf;
            metadata += "\n;memory:hwregion:";
            itocstrCStyle(config.hwRegion, numBuf, 21);
            metadata += numBuf;
            metadata += '\n';
            if (config.reqdWorkGroupSize[0] != 0 || config.reqdWorkGroupSize[1] != 0 ||
                config.reqdWorkGroupSize[1] != 0)
            {
                metadata += ";cws:";
                itocstrCStyle(config.reqdWorkGroupSize[0], numBuf, 21);
                metadata += numBuf;
                metadata += ':';
                itocstrCStyle(config.reqdWorkGroupSize[1], numBuf, 21);
                metadata += numBuf;
                metadata += ':';
                itocstrCStyle(config.reqdWorkGroupSize[2], numBuf, 21);
                metadata += numBuf;
                metadata += '\n';
            }
            
            size_t argOffset = 0;
            for (cxuint k = 0; k < config.args.size(); k++)
            {
                const AmdKernelArg& arg = config.args[k];
                if (arg.argType == KernelArgType::COUNTER32)
                {
                }
                if (arg.ptrAccess & KARG_PTR_CONST)
                {
                }
                argOffset += 16;
            }
            
            if (config.constDataRequired)
                metadata += ";memory:datareqd\n";
            metadata += ";function:1:";
            itocstrCStyle(uniqueId, numBuf, 21);
            metadata += numBuf;
            metadata += '\n';
            
            for (cxuint k = 0; k < config.samplers.size(); k++)
            {   /* constant samplers */
                const cxuint samp = config.samplers[k];
                metadata += ";sampler:unknown_";
                itocstrCStyle(samp, numBuf, 21);
                metadata += numBuf;
                metadata += ':';
                itocstrCStyle(k, numBuf, 21);
                metadata += numBuf;
                metadata += ":1:";
                itocstrCStyle(samp, numBuf, 21);
                metadata += numBuf;
                metadata += '\n';
            }
            
            if (input.is64Bit)
                metadata += ";memory:64bitABI\n";
            metadata += ";uavid:";
            itocstrCStyle(config.uavId, numBuf, 21);
            metadata += numBuf;
            metadata += '\n';
            if (config.printfIdEnable)
            {
                metadata += ";printfid:";
                itocstrCStyle(config.printfId, numBuf, 21);
                metadata += numBuf;
                metadata += '\n';
            }
            if (config.cbIdEnable)
            {
                metadata += ";cbid:";
                itocstrCStyle(config.cbId, numBuf, 21);
                metadata += numBuf;
                metadata += '\n';
            }
            metadata += ";privateid:";
            itocstrCStyle(config.privateId, numBuf, 21);
            metadata += numBuf;
            metadata += '\n';
            for (size_t k = 0; config.args.size(); k++)
            {
                const AmdKernelArg& arg = config.args[k];
                metadata += ";reflection:";
                itocstrCStyle(k, numBuf, 21);
                metadata += numBuf;
                metadata += ':';
                metadata += arg.typeName;
                metadata += '\n';
            }
            
            metadata += ";ARGEND:__OpenCL_";
            metadata += kinput.kernelName;
            metadata += "_kernel\n";
        }
        else // if defined in calNotes (no config)
        {
            for (const BinCALNote& calNote: kinput.calNotes)
                binarySize += 20 + calNote.header.descSize;
            binarySize += kinput.metadataSize;
        }
        uniqueId++;
    }
    /* writing data */
}
