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
#include <CLRX/MemAccess.h>
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

static const char* imgTypeNamesTable[] = { "2D", "1D", "1DA", "1DB", "2D", "2DA", "3D" };

enum KindOfType : cxbyte
{
    KT_UNSIGNED = 0,
    KT_SIGNED,
    KT_FLOAT,
    KT_DOUBLE,
    KT_OPAQUE,
    KT_UNKNOWN,
};

struct TypeNameVecSize
{
    const char* name;
    KindOfType kindOfType;
    cxbyte elemSize;
    cxbyte vecSize;
};

static const TypeNameVecSize argTypeNamesTable[] =
{
    { "u8", KT_UNSIGNED, 1, 1 }, { "i8", KT_SIGNED, 1, 1 },
    { "u16", KT_UNSIGNED, 2, 1 }, { "i16", KT_SIGNED, 2, 1 },
    { "u32", KT_UNSIGNED, 4, 1 }, { "i32", KT_SIGNED, 4, 1 },
    { "u64", KT_UNSIGNED, 8, 1 }, { "i64", KT_SIGNED, 8, 1 },
    { "float", KT_FLOAT, 4, 1 }, { "double", KT_DOUBLE, 8, 1 },
    { nullptr, KT_UNKNOWN, 1, 1 }, // POINTER
    { nullptr, KT_UNKNOWN, 1, 1 }, // IMAGE
    { nullptr, KT_UNKNOWN, 1, 1 }, // IMAGE1D
    { nullptr, KT_UNKNOWN, 1, 1 }, // IMAGE1D_ARRAY
    { nullptr, KT_UNKNOWN, 1, 1 }, // IMAGE1D_BUFFER
    { nullptr, KT_UNKNOWN, 1, 1 }, // IMAGE2D
    { nullptr, KT_UNKNOWN, 1, 1 }, // IMAGE1D_ARRAY
    { nullptr, KT_UNKNOWN, 1, 1 }, // IMAGE3D
    { "u8", KT_UNSIGNED, 1, 2 }, { "u8", KT_UNSIGNED, 1, 3 }, { "u8", KT_UNSIGNED, 1, 4 },
    { "u8",KT_UNSIGNED, 1, 8 }, { "u8", KT_UNSIGNED, 1, 16 },
    { "i8", KT_SIGNED, 1, 2 }, { "i8", KT_SIGNED, 1, 3 }, { "i8", KT_SIGNED, 1, 4 },
    { "i8", KT_SIGNED, 1, 8 }, { "i8", KT_SIGNED, 1, 16 },
    { "u16", KT_UNSIGNED, 2, 2 }, { "u16", KT_UNSIGNED, 2, 3 },
    { "u16", KT_UNSIGNED, 2, 4 }, { "u16", KT_UNSIGNED, 2, 8 },
    { "u16", KT_UNSIGNED, 2, 16 },
    { "i16", KT_SIGNED, 2, 2 }, { "i16", KT_SIGNED, 2, 3 },
    { "i16", KT_SIGNED, 2, 4 }, { "i16", KT_SIGNED, 2, 8 },
    { "i16", KT_SIGNED, 2, 16 },
    { "u32", KT_UNSIGNED, 4, 2 }, { "u32", KT_UNSIGNED, 4, 3 },
    { "u32", KT_UNSIGNED, 4, 4 }, { "u32", KT_UNSIGNED, 4, 8 },
    { "u32", KT_UNSIGNED, 4, 16 },
    { "i16", KT_SIGNED, 4, 2 }, { "i32", KT_SIGNED, 4, 3 },
    { "i32", KT_SIGNED, 4, 4 }, { "i32", KT_SIGNED, 4, 8 },
    { "i32", KT_SIGNED, 4, 16 },
    { "u64", KT_UNSIGNED, 8, 2 }, { "u64", KT_UNSIGNED, 8, 3 },
    { "u64", KT_UNSIGNED, 8, 4 }, { "u64", KT_UNSIGNED, 8, 8 },
    { "u64", KT_UNSIGNED, 8, 16 },
    { "i64", KT_SIGNED, 8, 2 }, { "i64", KT_SIGNED, 8, 3 },
    { "i64", KT_SIGNED, 8, 4 }, { "i64", KT_SIGNED, 8, 8 },
    { "i64", KT_SIGNED, 8, 16 },
    { "float", KT_FLOAT, 4, 2 }, { "float", KT_FLOAT, 4, 3 },
    { "float", KT_FLOAT, 4, 4 }, { "float", KT_FLOAT, 4, 8 },
    { "float", KT_FLOAT, 4, 16 },
    { "double", KT_DOUBLE, 4, 2 }, { "double", KT_DOUBLE, 4, 3 },
    { "double", KT_DOUBLE, 4, 4 }, { "double", KT_DOUBLE, 4, 8 },
    { "double", KT_DOUBLE, 4, 16 },
    { "u32", KT_UNSIGNED, 4, 1 }, /* SAMPLER */ { "opaque", KT_OPAQUE, 0, 1 },
    { nullptr, KT_UNKNOWN, 1, 1 }, /* COUNTER32 */
    { nullptr, KT_UNKNOWN, 1, 1 } // COUNTER64
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
    const bool isOlderThan1598 = input.driverVersion < 159805;
    /* checking input */
    if (input.deviceType == GPUDeviceType::UNDEFINED ||
        cxuint(input.deviceType) > cxuint(GPUDeviceType::GPUDEVICE_MAX))
        throw Exception("Undefined GPU device type");
    
    for (AmdKernelInput& kinput: input.kernels)
    {
        if (!kinput.useConfig)
            continue;
        AmdKernelConfig& config = kinput.config;
        if (config.userDataElemsNum > 16)
            throw Exception("UserDataElemsNum must not be greater than 16");
        /* filling input */
        if (config.hwRegion == AMDBIN_DEFAULT)
            config.hwRegion = 0;
        if (config.uavPrivate == AMDBIN_DEFAULT)
        {   /* compute uavPrivate */
            bool hasStructures = false;
            uint32_t amountOfArgs = 0;
            for (const AmdKernelArg arg: config.args)
            {
                if (arg.argType != KernelArgType::STRUCTURE)
                    hasStructures = true;
                if (!isOlderThan1598 && arg.argType != KernelArgType::STRUCTURE)
                    continue; // no older driver and no structure
                if (arg.argType == KernelArgType::POINTER)
                    amountOfArgs += 32;
                else if (arg.argType == KernelArgType::STRUCTURE)
                {
                    if (isOlderThan1598)
                        amountOfArgs += (arg.structSize+15)&~15;
                    else // bug in older drivers
                        amountOfArgs += 32;
                }
                else
                {
                    const TypeNameVecSize& tp = argTypeNamesTable[cxuint(arg.argType)];
                    const size_t typeSize = cxuint(tp.vecSize==3?4:tp.vecSize)*tp.elemSize;
                    amountOfArgs += ((typeSize+15)>>4)<<5;
                }
            }
            if (hasStructures || config.scratchBufferSize != 0)
                config.uavPrivate = config.scratchBufferSize + amountOfArgs;
            else
                config.uavPrivate = 0;
        }
        if (config.uavId == AMDBIN_DEFAULT)
        {
        }
        if (config.constBufferId == AMDBIN_DEFAULT)
        {
        }
        if (config.printfId == AMDBIN_DEFAULT)
        {
            config.printfId = 9;
        }
    }
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
        bool notUsedUav = false;
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
                {   // only global pointers are defined in uav table
                    if (arg.used)
                        uavsNum++;
                    else
                        notUsedUav = true;
                }
                if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
                    constBuffersNum++;
            }
           else if (arg.argType == KernelArgType::SAMPLER)
               samplersNum++;
        }
        
        if (notUsedUav)
            uavsNum++; // adds uav for not used
        
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
            cxuint readOnlyImageCount = 0;
            cxuint writeOnlyImageCount = 0;
            cxuint uavId = config.uavId+1;
            cxuint constantId = 2;
            for (cxuint k = 0; k < config.args.size(); k++)
            {
                const AmdKernelArg& arg = config.args[k];
                if (arg.argType == KernelArgType::STRUCTURE)
                {
                    metadata += ";value:";
                    metadata += arg.argName;
                    metadata += ":struct:";
                    itocstrCStyle(arg.structSize, numBuf, 21);
                    metadata += numBuf;
                    metadata += ":1:";
                    itocstrCStyle(argOffset, numBuf, 21);
                    metadata += numBuf;
                    metadata += '\n';
                    argOffset += (arg.structSize+15)>>4;
                }
                else if (arg.argType == KernelArgType::POINTER)
                {
                    metadata += ";pointer:";
                    metadata += arg.argName;
                    metadata += ':';
                    const TypeNameVecSize& tp = argTypeNamesTable[cxuint(arg.pointerType)];
                    if (tp.kindOfType == KT_UNKNOWN)
                        throw Exception("Type not supported!");
                    const cxuint typeSize =
                        cxuint((tp.vecSize==3) ? 4 : tp.vecSize)*tp.elemSize;
                    metadata += tp.name;
                    metadata += ":1:1:";
                    itocstrCStyle(argOffset, numBuf, 21);
                    metadata += numBuf;
                    metadata += ':';
                    if (arg.ptrSpace == KernelPtrSpace::LOCAL)
                        metadata += "hl:1";
                    if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
                    {
                        metadata += (isOlderThan1384)?"hc":"c";
                        if (isOlderThan1384)
                            itocstrCStyle(constantId++, numBuf, 21);
                        else
                        {
                            if (arg.used)
                                itocstrCStyle(uavId++, numBuf, 21);
                            else // if has not been used in kernel
                                itocstrCStyle(config.uavId, numBuf, 21);
                        }
                        metadata += numBuf;
                    }
                    if (arg.ptrSpace == KernelPtrSpace::GLOBAL)
                    {
                        metadata += "uav:";
                        itocstrCStyle(uavId++, numBuf, 21);
                        metadata += numBuf;
                    }
                    metadata += ':';
                    const size_t elemSize = (arg.pointerType==KernelArgType::STRUCTURE)?
                        ((arg.structSize!=0)?arg.structSize:4) : typeSize;
                    itocstrCStyle(elemSize, numBuf, 21);
                    metadata += ':';
                    metadata += (arg.ptrAccess & KARG_PTR_CONST)?"RO":"RW";
                    metadata += ':';
                    metadata += (arg.ptrAccess & KARG_PTR_VOLATILE)?'1':'0';
                    metadata += ':';
                    metadata += (arg.ptrAccess & KARG_PTR_RESTRICT)?'1':'0';
                    metadata += '\n';
                    argOffset += 32;
                }
                else if ((arg.argType >= KernelArgType::MIN_IMAGE) ||
                    (arg.argType <= KernelArgType::MAX_IMAGE))
                {
                    metadata += ";image:";
                    metadata += arg.argName;
                    metadata += ':';
                    metadata += imgTypeNamesTable[
                            cxuint(arg.argType)-cxuint(KernelArgType::IMAGE)];
                    metadata += ':';
                    if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_READ_ONLY)
                        metadata += "RO";
                    else if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                        metadata += "WO";
                    else if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_READ_WRITE)
                        metadata += "RW";
                    else
                        throw Exception("Invalid image access qualifier!");
                    metadata += ':';
                    if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_READ_ONLY)
                        itocstrCStyle(readOnlyImageCount++, numBuf, 21);
                    else // write only
                        itocstrCStyle(writeOnlyImageCount++, numBuf, 21);
                    metadata += numBuf;
                    metadata += ":1:";
                    itocstrCStyle(argOffset, numBuf, 21);
                    metadata += numBuf;
                    metadata += '\n';
                    argOffset += 32;
                }
                else if (arg.argType == KernelArgType::COUNTER32)
                {
                    metadata += ";counter:";
                    metadata += arg.argName;
                    metadata += ":32:0:1:";
                    itocstrCStyle(argOffset, numBuf, 21);
                    metadata += numBuf;
                    metadata += '\n';
                    argOffset += 16;
                }
                else
                {
                    metadata += ";value:";
                    metadata += arg.argName;
                    const TypeNameVecSize& tp = argTypeNamesTable[cxuint(arg.argType)];
                    if (tp.kindOfType == KT_UNKNOWN)
                        throw Exception("Type not supported!");
                    const cxuint typeSize =
                        cxuint((tp.vecSize==3) ? 4 : tp.vecSize)*tp.elemSize;
                    metadata += tp.name;
                    metadata += ':';
                    itocstrCStyle(tp.vecSize, numBuf, 21);
                    metadata += numBuf;
                    metadata += ':';
                    itocstrCStyle(argOffset, numBuf, 21);
                    metadata += numBuf;
                    metadata += '\n';
                    argOffset += (typeSize+15)>>4;
                }
                
                if (arg.ptrAccess & KARG_PTR_CONST)
                {
                    metadata += ";constant:";
                    itocstrCStyle(k, numBuf, 21);
                    metadata += numBuf;
                    metadata += ':';
                    metadata += arg.argName;
                    metadata += '\n';
                }
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
            if (config.printfId != AMDBIN_NOTSUPPLIED)
            {
                metadata += ";printfid:";
                itocstrCStyle(config.printfId, numBuf, 21);
                metadata += numBuf;
                metadata += '\n';
            }
            if (config.constBufferId != AMDBIN_NOTSUPPLIED)
            {
                metadata += ";cbid:";
                itocstrCStyle(config.constBufferId, numBuf, 21);
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
            binarySize += metadata.size();
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
    delete[] binary;    // delete of pointer
    binary = nullptr;
    binary = new cxbyte[binarySize];
    size_t offset = 0;
    { /* main ELF header */
    }
}
