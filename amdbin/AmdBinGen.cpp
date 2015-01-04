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
#include <elf.h>
#include <cassert>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <bitset>
#include <set>
#include <string>
#include <vector>
#include <CLRX/Utilities.h>
#include <CLRX/MemAccess.h>
#include <CLRX/AmdBinGen.h>

using namespace CLRX;

template<typename ElfSection>
static void putElfSectionLE(ElfSection* section, size_t shName, uint32_t shType,
        uint32_t shFlags, size_t offset, size_t size, uint32_t link,
        uint32_t info = 0, uint32_t addrAlign = 1, size_t addr = 0, uint32_t entSize = 0)
{
    SULEV(section->sh_name, shName);
    SULEV(section->sh_type, shType);
    SULEV(section->sh_flags, shFlags);
    SULEV(section->sh_addr, 0);
    SULEV(section->sh_offset, offset);
    SULEV(section->sh_size, size);
    SULEV(section->sh_link, link);
    SULEV(section->sh_info, info);
    SULEV(section->sh_addralign, addrAlign);
    SULEV(section->sh_entsize, entSize);
}

template<typename ElfSym>
static void putElfSymbolLE(ElfSym* symbol, size_t symName, size_t value, size_t size,
        uint16_t shndx, cxbyte info, cxbyte other = 0)
{
    SULEV(symbol->st_name, symName); 
    SULEV(symbol->st_value, value);
    SULEV(symbol->st_size, size);
    SULEV(symbol->st_shndx, shndx);
    symbol->st_info = info;
    symbol->st_other = other;
}

static inline void putCALNoteLE(CALNoteHeader* noteHdr, uint32_t type, uint32_t descSize)
{
    SULEV(noteHdr->nameSize, 8);
    SULEV(noteHdr->type, type);
    SULEV(noteHdr->descSize, descSize);
    ::memcpy(noteHdr->name, "ATI CAL", 8);
}

static inline void putCALUavEntry(CALUAVEntry* uavEntry, uint32_t uavId, uint32_t f1,
          uint32_t f2, uint32_t type)
{
    SULEV(uavEntry->uavId, uavId);
    SULEV(uavEntry->f1, f1);
    SULEV(uavEntry->f2, f2);
    SULEV(uavEntry->type, type);
}

static const uint32_t gpuDeviceCodeTable[14] =
{
    0, // GPUDeviceType::UNDEFINED
    0x3ff, // GPUDeviceType::CAPE_VERDE
    0x3fe, // GPUDeviceType::PITCAIRN
    0x3fd, // GPUDeviceType::TAHITI
    0x402, // GPUDeviceType::OLAND
    0x403, // GPUDeviceType::BONAIRE
    0x404, // GPUDeviceType::SPECTRE
    0x405, // GPUDeviceType::SPOOKY
    0x406, // GPUDeviceType::KALINDI
    0x407, // GPUDeviceType::HAINAN
    0x408, // GPUDeviceType::HAWAII
    0x409, // GPUDeviceType::ICELAND
    0x40a, // GPUDeviceType::TONGA
    0x40b // GPUDeviceType::MULLINS
};

static const uint16_t gpuDeviceInnerCodeTable[14] =
{
    0, // GPUDeviceType::UNDEFINED
    0x1c, // GPUDeviceType::CAPE_VERDE
    0x1b, // GPUDeviceType::PITCAIRN
    0x1a, // GPUDeviceType::TAHITI
    0x20, // GPUDeviceType::OLAND
    0x21, // GPUDeviceType::BONAIRE
    0x22, // GPUDeviceType::SPECTRE
    0x23, // GPUDeviceType::SPOOKY
    0x24, // GPUDeviceType::KALINDI
    0x25, // GPUDeviceType::HAINAN
    0x27, // GPUDeviceType::HAWAII
    0x29, // GPUDeviceType::ICELAND
    0x2a, // GPUDeviceType::TONGA
    0x2b // GPUDeviceType::MULLINS
};

void AmdInput::addKernel(const AmdKernelInput& kernelInput)
{
    kernels.push_back(kernelInput);
}

void AmdInput::addKernel(const char* kernelName, size_t codeSize,
       const cxbyte* code, const AmdKernelConfig& config,
       size_t dataSize, const cxbyte* data)
{
    kernels.push_back({ kernelName, dataSize, data, 0, nullptr, 0, nullptr, {},
                true, config, codeSize, code });
}

void AmdInput::addKernel(const char* kernelName, size_t codeSize,
       const cxbyte* code, const std::vector<CALNoteInput>& calNotes, const cxbyte* header,
       size_t metadataSize, const char* metadata, size_t dataSize, const cxbyte* data)
{
    kernels.push_back({ kernelName, dataSize, data, 32, header,
        metadataSize, metadata, calNotes, false, AmdKernelConfig(), codeSize, code });
}

AmdGPUBinGenerator::AmdGPUBinGenerator() : manageable(false)
{ }

AmdGPUBinGenerator::AmdGPUBinGenerator(const AmdInput* amdInput)
        : manageable(false), input(amdInput)
{ }

AmdGPUBinGenerator::AmdGPUBinGenerator(bool _64bitMode, GPUDeviceType deviceType,
       uint32_t driverVersion, size_t globalDataSize, const cxbyte* globalData, 
       const std::vector<AmdKernelInput>& kernelInputs)
        : manageable(true), input(nullptr)
{
    AmdInput* newInput = new AmdInput;
    try
    {
        newInput->is64Bit = _64bitMode;
        newInput->deviceType = deviceType;
        newInput->driverVersion = driverVersion;
        newInput->globalDataSize = globalDataSize;
        newInput->globalData = globalData;
        newInput->kernels = kernelInputs;
    }
    catch(...)
    {
        delete newInput;
        throw;
    }
    input = newInput;
}

AmdGPUBinGenerator::~AmdGPUBinGenerator()
{
    if (manageable)
        delete input;
}

void AmdGPUBinGenerator::setInput(const AmdInput* input)
{
    if (manageable)
        delete input;
    manageable = false;
    this->input = input;
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

static const cxuint imgUavDimTable[] = { 2, 1, 0, 0, 2, 2, 3 };

enum KindOfType : cxbyte
{
    KT_UNSIGNED = 0,
    KT_SIGNED,
    KT_FLOAT,
    KT_DOUBLE,
    KT_STRUCT,
    KT_UNKNOWN
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
    { "u8", KT_UNSIGNED, 1, 1 }, // VOID
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
    { "i32", KT_SIGNED, 4, 2 }, { "i32", KT_SIGNED, 4, 3 },
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
    { "double", KT_DOUBLE, 8, 2 }, { "double", KT_DOUBLE, 8, 3 },
    { "double", KT_DOUBLE, 8, 4 }, { "double", KT_DOUBLE, 8, 8 },
    { "double", KT_DOUBLE, 8, 16 },
    { "u32", KT_UNSIGNED, 4, 1 }, /* SAMPLER */ { "struct", KT_STRUCT, 0, 1 },
    { nullptr, KT_UNKNOWN, 1, 1 }, /* COUNTER32 */
    { nullptr, KT_UNKNOWN, 1, 1 } // COUNTER64
};

struct TempAmdKernelConfig
{
    uint32_t hwRegion;
    uint32_t uavPrivate;
    uint32_t uavId;
    uint32_t constBufferId;
    uint32_t printfId;
    uint32_t privateId;
    uint32_t argsSpace;
    std::vector<uint16_t> argResIds;
};

template<typename ElfSym>
static void putMainSymbols(cxbyte* binary, size_t& offset, const AmdInput* input,
    const std::vector<std::string>& kmetadatas, const std::vector<cxuint>& innerBinSizes)
{
    ElfSym* symbolTable = reinterpret_cast<ElfSym*>(binary+offset);
    ::memset(symbolTable, 0, sizeof(ElfSym));
    size_t namePos = 1;
    if (!input->compileOptions.empty())
    {
        putElfSymbolLE(symbolTable+1, namePos, 0, input->compileOptions.size(), 
                (input->kernels.size()!=0)?6:4, ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0);
        namePos += 25;
        symbolTable++;
        offset += sizeof(ElfSym);
    }
    symbolTable++;
    size_t rodataPos = 0;
    if (input->globalData != nullptr)
    {
        putElfSymbolLE(symbolTable++, namePos, 0, input->globalDataSize, 4,
                ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0);
        namePos += 18;
        rodataPos = input->globalDataSize;
        offset += sizeof(ElfSym);
    }
    size_t textPos = 0;
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        const AmdKernelInput& kernel = input->kernels[i];
        /* kernel metatadata */
        const size_t metadataSize = (kernel.useConfig) ?
                kmetadatas[i].size() : kernel.metadataSize;
        putElfSymbolLE(symbolTable++, namePos, rodataPos, metadataSize, 4,
                ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0);
        namePos += 19 + kernel.kernelName.size();
        rodataPos += metadataSize;
        /* kernel code */
        putElfSymbolLE(symbolTable++, namePos, textPos, innerBinSizes[i], 5,
                ELF32_ST_INFO(STB_LOCAL, STT_FUNC), 0);
        namePos += 17 + kernel.kernelName.size();
        textPos += innerBinSizes[i];
        /* kernel header */
        const size_t headerSize = (kernel.useConfig) ? 32 : kernel.headerSize;
        putElfSymbolLE(symbolTable++, namePos, rodataPos, headerSize, 4,
                ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0);
        namePos += 17 + kernel.kernelName.size();
        rodataPos += headerSize;
    }
    offset += sizeof(ElfSym)*(input->kernels.size()*3 + 1);
}

template<typename ElfShdr, typename ElfSym>
static void putMainSections(cxbyte* binary, size_t &offset,
        const size_t* sectionOffsets, size_t alignFix, bool noKernels, bool haveSource,
        bool haveLLVMIR)
{
    ElfShdr* sectionHdrTable = reinterpret_cast<ElfShdr*>(binary + offset);
    ::memset(sectionHdrTable, 0, sizeof(ElfShdr));
    sectionHdrTable++;
    // .shstrtab
    putElfSectionLE(sectionHdrTable++, 1, SHT_STRTAB, SHF_STRINGS, sectionOffsets[0],
            sectionOffsets[1]-sectionOffsets[0], 0);
    // .strtab
    putElfSectionLE(sectionHdrTable++, 11, SHT_STRTAB, SHF_STRINGS, sectionOffsets[1],
            sectionOffsets[2]-sectionOffsets[1]-alignFix, 0);
    // .symtab
    putElfSectionLE(sectionHdrTable++, 19, SHT_SYMTAB, 0, sectionOffsets[2],
            sectionOffsets[3]-sectionOffsets[2], 2, 0, 8, 0, sizeof(ElfSym));
    size_t shNamePos = 27;
    if (!noKernels) // .text not emptyu
    {   // .rodata
        putElfSectionLE(sectionHdrTable++, 27, SHT_PROGBITS, SHF_ALLOC, sectionOffsets[3],
                sectionOffsets[4]-sectionOffsets[3], 0);
        // .text
        putElfSectionLE(sectionHdrTable++, 35, SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR,
                sectionOffsets[4], sectionOffsets[5]-sectionOffsets[4], 0);
        shNamePos += 14;
    }
    // .comment
    putElfSectionLE(sectionHdrTable++, shNamePos, SHT_PROGBITS, 0, sectionOffsets[5],
            sectionOffsets[6]-sectionOffsets[5], 0);
    shNamePos += 9;
    if (haveSource)
    {
        putElfSectionLE(sectionHdrTable++, shNamePos, SHT_PROGBITS, 0, sectionOffsets[6],
                sectionOffsets[7]-sectionOffsets[6], 0);
        shNamePos += 8;
    }
    if (haveLLVMIR)
        putElfSectionLE(sectionHdrTable++, shNamePos, SHT_PROGBITS, 0, sectionOffsets[7],
                sectionOffsets[8]-sectionOffsets[7], 0);
    
    offset += sizeof(ElfShdr)*(5 + (noKernels?0:2) + haveSource + haveLLVMIR);
}

static void prepareTempConfigs(cxuint driverVersion, const AmdInput* input,
       std::vector<TempAmdKernelConfig>& tempAmdKernelConfigs)
{
    const bool isOlderThan1348 = driverVersion < 134805;
    const bool isOlderThan1598 = driverVersion < 159805;
    
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        const AmdKernelInput& kinput = input->kernels[i];
        if (!kinput.useConfig)
        {
            if (kinput.metadata == nullptr || kinput.metadataSize == 0)
                throw Exception("No metadata for kernel");
            if (kinput.header == nullptr || kinput.headerSize == 0)
                throw Exception("No header for kernel");
            if (kinput.code == nullptr)
                throw Exception("No code for kernel");
            continue; // and skip
        }
        const AmdKernelConfig& config = kinput.config;
        TempAmdKernelConfig& tempConfig = tempAmdKernelConfigs[i];
        if (config.userDataElemsNum > 16)
            throw Exception("UserDataElemsNum must not be greater than 16");
        /* filling input */
        if (config.hwRegion == AMDBIN_DEFAULT)
            tempConfig.hwRegion = 0;
        else
            tempConfig.hwRegion = config.hwRegion;
        
        if (config.uavPrivate == AMDBIN_DEFAULT)
        {   /* compute uavPrivate */
            bool hasStructures = false;
            uint32_t amountOfArgs = 0;
            for (const AmdKernelArg& arg: config.args)
            {
                if (arg.argType == KernelArgType::STRUCTURE ||
                    arg.argType == KernelArgType::COUNTER32)
                    hasStructures = true;
                if (!isOlderThan1598 && arg.argType != KernelArgType::STRUCTURE)
                    continue; // no older driver and no structure
                if (arg.argType == KernelArgType::POINTER)
                    amountOfArgs += 32;
                else if (arg.argType == KernelArgType::STRUCTURE)
                {
                    if (!isOlderThan1598)
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
            
            tempConfig.uavPrivate = (hasStructures) ? amountOfArgs : 0;
            if (tempConfig.uavPrivate < config.scratchBufferSize)
                tempConfig.uavPrivate = config.scratchBufferSize;
        }
        else
            tempConfig.uavPrivate = config.uavPrivate;
        
        if (config.uavId == AMDBIN_DEFAULT)
        {
            if (driverVersion < 134805 || driverVersion > 144505)
                tempConfig.uavId = (isOlderThan1348)?9:11;
            else
            {
                bool hasPointer = false;
                for (const AmdKernelArg& arg: config.args)
                    if (arg.argType == KernelArgType::POINTER &&
                        (arg.ptrSpace == KernelPtrSpace::CONSTANT ||
                         arg.ptrSpace == KernelPtrSpace::GLOBAL))
                        hasPointer = true;
                
                tempConfig.uavId = (hasPointer || config.usePrintf)?11:AMDBIN_NOTSUPPLIED;
            }
        }
        else
            tempConfig.uavId = config.uavId;
        
        if (config.constBufferId == AMDBIN_DEFAULT)
            tempConfig.constBufferId = (isOlderThan1348)?AMDBIN_NOTSUPPLIED : 10;
        else
            tempConfig.constBufferId = config.constBufferId;
        
        if (config.printfId == AMDBIN_DEFAULT)
            tempConfig.printfId = (!isOlderThan1348 &&
                (driverVersion >= 152603 || config.usePrintf)) ? 9 : AMDBIN_NOTSUPPLIED;
        else
            tempConfig.printfId = config.printfId;
        
        if (config.privateId == AMDBIN_DEFAULT)
            tempConfig.privateId = 8;
        else
            tempConfig.privateId = config.privateId;
        
        /* fill argUavIds for global/constant pointers */
        cxuint puavIdsCount = tempConfig.uavId+1;
        std::bitset<1024> puavMask;
        cxuint cntIdsCount = 0;
        std::bitset<8> cntIdMask;
        cxuint cbIdsCount = 2 + (input->globalData != nullptr);
        std::bitset<160> cbIdMask;
        cxuint rdImgsCount = 0;
        std::bitset<128> rdImgMask;
        cxuint wrImgsCount = 0;
        std::bitset<8> wrImgMask;
        puavMask.reset();
        cntIdMask.reset();
        cbIdMask.reset();
        rdImgMask.reset();
        wrImgMask.reset();
        tempConfig.argResIds.resize(config.args.size());
        
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArg& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                (arg.ptrSpace == KernelPtrSpace::GLOBAL ||
                 (arg.ptrSpace == KernelPtrSpace::CONSTANT && !isOlderThan1348)) &&
               arg.resId != AMDBIN_DEFAULT)
            {
                if ((arg.resId < 9 && arg.used) ||
                    (!arg.used && arg.resId != tempConfig.uavId) || arg.resId >= 1024)
                    throw Exception("UavId out of range!");
                if (puavMask[arg.resId] && arg.resId != tempConfig.uavId)
                    throw Exception("UavId already used!");
                puavMask.set(arg.resId);
                tempConfig.argResIds[k] = arg.resId;
            }
            else if (arg.argType == KernelArgType::POINTER &&
                    arg.ptrSpace == KernelPtrSpace::CONSTANT && arg.resId != AMDBIN_DEFAULT)
            {   // old constant buffers
                if (arg.resId < 2 || arg.resId >= 160)
                    throw Exception("CbId out of range!");
                if (cbIdMask[arg.resId])
                    throw Exception("CbId already used!");
                cbIdMask.set(arg.resId);
                tempConfig.argResIds[k] = arg.resId;
            }
            else if (arg.argType >= KernelArgType::MIN_IMAGE &&
                     arg.argType <= KernelArgType::MAX_IMAGE && arg.resId != AMDBIN_DEFAULT)
            {   // images
                if (arg.ptrAccess & KARG_PTR_READ_ONLY)
                {
                    if (arg.resId >= 128)
                        throw Exception("RdImgId out of range!");
                    if (rdImgMask[arg.resId])
                        throw Exception("RdImgId already used!");
                    rdImgMask.set(arg.resId);
                    tempConfig.argResIds[k] = arg.resId;
                }
                else if (arg.ptrAccess & KARG_PTR_WRITE_ONLY)
                {
                    if (arg.resId >= 8)
                        throw Exception("WrImgId out of range!");
                    if (wrImgMask[arg.resId])
                        throw Exception("WrImgId already used!");
                    wrImgMask.set(arg.resId);
                    tempConfig.argResIds[k] = arg.resId;
                }
            }
            else if (arg.argType == KernelArgType::COUNTER32 && arg.resId != AMDBIN_DEFAULT)
            {
                if (arg.resId >= 8)
                    throw Exception("CounterId out of range!");
                if (cntIdMask[arg.resId])
                    throw Exception("CounterId already used!");
                cntIdMask.set(arg.resId);
                tempConfig.argResIds[k] = arg.resId;
            }
        }
        
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArg& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                (arg.ptrSpace == KernelPtrSpace::GLOBAL ||
                 (arg.ptrSpace == KernelPtrSpace::CONSTANT && !isOlderThan1348)) &&
                arg.resId == AMDBIN_DEFAULT)
            {
                if (arg.used)
                {
                    for (; puavIdsCount < 1024 && puavMask[puavIdsCount];
                         puavIdsCount++);
                    if (puavIdsCount == 1024)
                        throw Exception("UavId out of range!");
                    tempConfig.argResIds[k] = puavIdsCount++;
                }
                else // use unused uavId (9 or 11)
                    tempConfig.argResIds[k] = tempConfig.uavId;
            }
            else if (arg.argType == KernelArgType::POINTER &&
                    arg.ptrSpace == KernelPtrSpace::CONSTANT && arg.resId == AMDBIN_DEFAULT)
            {   // old constant buffers
                for (; cbIdsCount < 1024 && cbIdMask[cbIdsCount]; cbIdsCount++);
                if (cbIdsCount == 160)
                    throw Exception("CbId out of range!");
                tempConfig.argResIds[k] = cbIdsCount++;
            }
            else if (arg.argType >= KernelArgType::MIN_IMAGE &&
                     arg.argType <= KernelArgType::MAX_IMAGE && arg.resId == AMDBIN_DEFAULT)
            {   // images
                if (arg.ptrAccess & KARG_PTR_READ_ONLY)
                {
                    for (; rdImgsCount < 128 && rdImgMask[rdImgsCount]; rdImgsCount++);
                    if (rdImgsCount == 128)
                        throw Exception("RdImgId out of range!");
                    tempConfig.argResIds[k] = rdImgsCount++;
                }
                else if (arg.ptrAccess & KARG_PTR_WRITE_ONLY)
                {
                    for (; wrImgsCount < 8 && wrImgMask[wrImgsCount]; wrImgsCount++);
                    if (wrImgsCount == 8)
                        throw Exception("WrImgId out of range!");
                    tempConfig.argResIds[k] = wrImgsCount++;
                }
            }
            else if (arg.argType == KernelArgType::COUNTER32 && arg.resId == AMDBIN_DEFAULT)
            {
                for (; cntIdsCount < 8 && cntIdMask[cntIdsCount]; cntIdsCount++);
                if (cntIdsCount == 8)
                    throw Exception("CounterId out of range!");
                tempConfig.argResIds[k] = cntIdsCount++;
            }
        }
    }
}

static std::string generateMetadata(cxuint driverVersion, const AmdInput* input,
        const AmdKernelInput& kinput, TempAmdKernelConfig& tempConfig,
        cxuint argSamplersNum, cxuint uniqueId)
{
    const bool isOlderThan1124 = driverVersion < 112402;
    const bool isOlderThan1348 = driverVersion < 134805;
    const AmdKernelConfig& config = kinput.config;
    /* compute metadataSize */
    std::string metadata;
    metadata.reserve(100);
    metadata += ";ARGSTART:__OpenCL_";
    metadata += kinput.kernelName;
    metadata += "_kernel\n";
    if (isOlderThan1124)
        metadata += ";version:3:1:104\n";
    else
        metadata += ";version:3:1:111\n";
    metadata += ";device:";
    metadata += gpuDeviceNameTable[cxuint(input->deviceType)];
    char numBuf[21];
    metadata += "\n;uniqueid:";
    itocstrCStyle(uniqueId, numBuf, 21);
    metadata += numBuf;
    metadata += "\n;memory:uavprivate:";
    itocstrCStyle(tempConfig.uavPrivate, numBuf, 21);
    metadata += numBuf;
    metadata += "\n;memory:hwlocal:";
    itocstrCStyle(config.hwLocalSize, numBuf, 21);
    metadata += numBuf;
    metadata += "\n;memory:hwregion:";
    itocstrCStyle(tempConfig.hwRegion, numBuf, 21);
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
            argOffset += (arg.structSize+15)&~15;
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
            if (arg.structSize == 0 && arg.pointerType == KernelArgType::STRUCTURE)
                metadata += "opaque";
            else
                metadata += tp.name;
            metadata += ":1:1:";
            itocstrCStyle(argOffset, numBuf, 21);
            metadata += numBuf;
            metadata += ':';
            if (arg.ptrSpace == KernelPtrSpace::LOCAL)
                metadata += "hl:1";
            else if (arg.ptrSpace == KernelPtrSpace::CONSTANT ||
                     arg.ptrSpace == KernelPtrSpace::GLOBAL)
            {
                if (arg.ptrSpace == KernelPtrSpace::GLOBAL)
                    metadata += "uav:";
                else
                    metadata += (isOlderThan1348)?"hc:":"c:";
                itocstrCStyle(tempConfig.argResIds[k], numBuf, 21);
                metadata += numBuf;
            }
            else
                throw Exception("Other memory spaces are not supported");
            metadata += ':';
            const size_t elemSize = (arg.pointerType==KernelArgType::STRUCTURE)?
                ((arg.structSize!=0)?arg.structSize:4) : typeSize;
            itocstrCStyle(elemSize, numBuf, 21);
            metadata += numBuf;
            metadata += ':';
            metadata += ((arg.ptrAccess & KARG_PTR_CONST) ||
                    arg.ptrSpace == KernelPtrSpace::CONSTANT)?"RO":"RW";
            metadata += ':';
            metadata += (arg.ptrAccess & KARG_PTR_VOLATILE)?'1':'0';
            metadata += ':';
            metadata += (arg.ptrAccess & KARG_PTR_RESTRICT)?'1':'0';
            metadata += '\n';
            argOffset += 16;
        }
        else if ((arg.argType >= KernelArgType::MIN_IMAGE) &&
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
            itocstrCStyle(tempConfig.argResIds[k], numBuf, 21);
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
            metadata += ":32:";
            itocstrCStyle(tempConfig.argResIds[k], numBuf, 21);
            metadata += numBuf;
            metadata += ":1:";
            itocstrCStyle(argOffset, numBuf, 21);
            metadata += numBuf;
            metadata += '\n';
            argOffset += 16;
        }
        else
        {
            metadata += ";value:";
            metadata += arg.argName;
            metadata += ':';
            const TypeNameVecSize& tp = argTypeNamesTable[cxuint(arg.argType)];
            if (tp.kindOfType == KT_UNKNOWN)
                throw Exception("Type not supported!");
            const cxuint typeSize =
                cxuint((tp.vecSize==3) ? 4 : tp.vecSize)*std::max(cxbyte(4), tp.elemSize);
            metadata += tp.name;
            metadata += ':';
            itocstrCStyle(tp.vecSize, numBuf, 21);
            metadata += numBuf;
            metadata += ":1:";
            itocstrCStyle(argOffset, numBuf, 21);
            metadata += numBuf;
            metadata += '\n';
            argOffset += (typeSize+15)&~15;
        }
        
        if (arg.ptrAccess & KARG_PTR_CONST)
        {
            metadata += ";constarg:";
            itocstrCStyle(k, numBuf, 21);
            metadata += numBuf;
            metadata += ':';
            metadata += arg.argName;
            metadata += '\n';
        }
    }
    
    tempConfig.argsSpace = argOffset;
    
    if (input->globalData != nullptr)
        metadata += ";memory:datareqd\n";
    metadata += ";function:1:";
    itocstrCStyle(uniqueId, numBuf, 21);
    metadata += numBuf;
    metadata += '\n';
    
    for (cxuint sampId = 0; sampId < config.samplers.size(); sampId++)
    {   /* constant samplers */
        const cxuint samp = config.samplers[sampId];
        metadata += ";sampler:unknown_";
        itocstrCStyle(samp, numBuf, 21);
        metadata += numBuf;
        metadata += ':';
        itocstrCStyle(sampId+argSamplersNum, numBuf, 21);
        metadata += numBuf;
        metadata += ":1:";
        itocstrCStyle(samp, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    cxuint sampId = 0;
    /* kernel argument samplers */
    for (const AmdKernelArg& arg: config.args)
        if (arg.argType == KernelArgType::SAMPLER)
        {
            metadata += ";sampler:";
            metadata += arg.argName;
            metadata += ':';
            itocstrCStyle(sampId, numBuf, 21);
            metadata += numBuf;
            metadata += ":0:0\n";
            sampId++;
        }
    
    if (input->is64Bit)
        metadata += ";memory:64bitABI\n";
    if (tempConfig.uavId != AMDBIN_NOTSUPPLIED)
    {
        metadata += ";uavid:";
        itocstrCStyle(tempConfig.uavId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    if (tempConfig.printfId != AMDBIN_NOTSUPPLIED)
    {
        metadata += ";printfid:";
        itocstrCStyle(tempConfig.printfId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    if (tempConfig.constBufferId != AMDBIN_NOTSUPPLIED)
    {
        metadata += ";cbid:";
        itocstrCStyle(tempConfig.constBufferId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    metadata += ";privateid:";
    itocstrCStyle(tempConfig.privateId, numBuf, 21);
    metadata += numBuf;
    metadata += '\n';
    for (cxuint k = 0; k < config.args.size(); k++)
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
    
    return metadata;
}

static void generateCALNotes(cxbyte* binary, size_t& offset, const AmdInput* input,
         cxuint driverVersion, const AmdKernelInput& kernel,
         const TempAmdKernelConfig& tempConfig)
{
    const bool isOlderThan1124 = driverVersion < 112402;
    const bool isOlderThan1348 = driverVersion < 134805;
    const AmdKernelConfig& config = kernel.config;
    cxuint readOnlyImages = 0;
    cxuint writeOnlyImages = 0;
    cxuint samplersNum = config.samplers.size();
    cxuint argSamplersNum = 0;
    bool isLocalPointers = false;
    cxuint constBuffersNum = 2 + (isOlderThan1348 /* cbid:2 for older drivers*/ &&
            (input->globalData != nullptr));
    cxuint woUsedImagesMask = 0;
    for (const AmdKernelArg& arg: config.args)
    {
        if (arg.argType >= KernelArgType::MIN_IMAGE &&
            arg.argType <= KernelArgType::MAX_IMAGE)
        {
            if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_READ_ONLY)
                readOnlyImages++;
            if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
            {
                if (arg.used)
                    woUsedImagesMask |= (1U<<writeOnlyImages);
                writeOnlyImages++;
            }
        }
        else if (arg.argType == KernelArgType::POINTER)
        {
            if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
                constBuffersNum++;
            else if (arg.ptrSpace == KernelPtrSpace::LOCAL)
                isLocalPointers = true;
        }
       else if (arg.argType == KernelArgType::SAMPLER)
           argSamplersNum++;
    }
    samplersNum += argSamplersNum;
    
    // CAL CALNOTE_INPUTS
    CALNoteHeader* noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_INPUTS, 4*readOnlyImages);
    offset += sizeof(CALNoteHeader);
    uint32_t* data32 = reinterpret_cast<uint32_t*>(binary + offset);
    for (cxuint k = 0; k < readOnlyImages; k++)
        SULEV(data32[k], isOlderThan1124 ? readOnlyImages-k-1 : k);
    offset += 4*readOnlyImages;
    // CALNOTE_OUTPUTS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_OUTPUTS, 0);
    offset += sizeof(CALNoteHeader);
    // CALNOTE_UAV
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    offset += sizeof(CALNoteHeader);
    
    CALUAVEntry* startUavEntry = reinterpret_cast<CALUAVEntry*>(binary + offset);
    CALUAVEntry* uavEntry = startUavEntry;
    if (isOlderThan1124)
    {   // for old drivers
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArg& arg = config.args[k];
            if (arg.argType >= KernelArgType::MIN_IMAGE &&
                arg.argType <= KernelArgType::MAX_IMAGE &&
                (arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                /* writeOnlyImages */
                putCALUavEntry(uavEntry++, tempConfig.argResIds[k], 2, imgUavDimTable[
                        cxuint(arg.argType)-cxuint(KernelArgType::MIN_IMAGE)], 3);
        }
        if (config.usePrintf)
            putCALUavEntry(uavEntry++, tempConfig.uavId, 4, 0, 5);
        // global buffers
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArg& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::GLOBAL) // uavid
                putCALUavEntry(uavEntry++, tempConfig.argResIds[k], 4, 0, 5);
        }
    }
    else
    {   /* in argument order */
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArg& arg = config.args[k];
            if (arg.argType >= KernelArgType::MIN_IMAGE &&
                arg.argType <= KernelArgType::MAX_IMAGE &&
                (arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                // write_only images
                putCALUavEntry(uavEntry++, tempConfig.argResIds[k], 2, 2, 5);
            else if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::GLOBAL) // uavid
                putCALUavEntry(uavEntry++, tempConfig.argResIds[k], 4, 0, 5);
        }
        
        if (config.usePrintf)
            putCALUavEntry(uavEntry++, tempConfig.uavId, 0, 0, 5);
    }
    offset += 16*(uavEntry-startUavEntry);
    putCALNoteLE(noteHdr, CALNOTE_ATI_UAV, 16*(uavEntry-startUavEntry));
    
    // CALNOTE_CONDOUT
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_CONDOUT, 4);
    offset += sizeof(CALNoteHeader);
    data32 = reinterpret_cast<uint32_t*>(binary + offset);
    SULEV(*data32, config.condOut);
    offset += 4;
    // CALNOTE_FLOAT32CONSTS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_FLOAT32CONSTS, 0);
    offset += sizeof(CALNoteHeader);
    // CALNOTE_INT32CONSTS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_INT32CONSTS, 0);
    offset += sizeof(CALNoteHeader);
    // CALNOTE_BOOL32CONSTS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_BOOL32CONSTS, 0);
    offset += sizeof(CALNoteHeader);
    
    // CALNOTE_EARLYEXIT
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_EARLYEXIT, 4);
    offset += sizeof(CALNoteHeader);
    data32 = reinterpret_cast<uint32_t*>(binary + offset);
    SULEV(*data32, config.earlyExit);
    offset += 4;
    
    // CALNOTE_GLOBAL_BUFFERS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_GLOBAL_BUFFERS, 0);
    offset += sizeof(CALNoteHeader);
    
    // CALNOTE_CONSTANT_BUFFERS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_CONSTANT_BUFFERS, 8*constBuffersNum);
    offset += sizeof(CALNoteHeader);
    data32 = reinterpret_cast<uint32_t*>(binary + offset);
    
    CALConstantBufferMask* cbufMask =
        reinterpret_cast<CALConstantBufferMask*>(binary + offset);
    offset += 8*constBuffersNum;
    
    if (isOlderThan1124)
    {   /* for driver 12.10 */
        for (cxuint k = config.args.size(); k > 0; k--)
        {
            const AmdKernelArg& arg = config.args[k-1];
            if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::CONSTANT)
            {
                SULEV(cbufMask->index, tempConfig.argResIds[k-1]);
                SULEV(cbufMask->size, arg.constSpaceSize!=0 ?
                    ((arg.constSpaceSize+15)>>4) : 4096);
                cbufMask++;
            }
        }
        if (input->globalData != nullptr)
        {
            SULEV(cbufMask->index, 2); // ????
            SULEV(cbufMask->size, (input->globalDataSize+15)>>4);
            cbufMask++;
        }
        SULEV(cbufMask->index, 1);
        SULEV(cbufMask->size, (tempConfig.argsSpace+15)>>4);
        SULEV(cbufMask[1].index, 0);
        SULEV(cbufMask[1].size, 15);
    }
    else 
    {
        SULEV(cbufMask->index, 0);
        SULEV(cbufMask->size, 0);
        SULEV(cbufMask[1].index, 1);
        SULEV(cbufMask[1].size, 0);
        cbufMask += 2;
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArg& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::CONSTANT)
            {
                SULEV(cbufMask->index, tempConfig.argResIds[k]);
                SULEV(cbufMask->size, 0);
                cbufMask++;
            }
        }
        if (isOlderThan1348 && input->globalData != nullptr)
        {
            SULEV(cbufMask->index, 2);
            SULEV(cbufMask->size, 0);
        }
    }
    
    // CALNOTE_INPUT_SAMPLERS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_INPUT_SAMPLERS, 8*samplersNum);
    offset += sizeof(CALNoteHeader);
    
    CALSamplerMapEntry* sampEntry = reinterpret_cast<CALSamplerMapEntry*>(
                binary + offset);
    for (cxuint k = 0; k < config.samplers.size(); k++)
    {
        SULEV(sampEntry[k].input, 0);
        SULEV(sampEntry[k].sampler, k+argSamplersNum);
    }
    sampEntry += config.samplers.size();
    for (cxuint k = 0; k < argSamplersNum; k++)
    {
        SULEV(sampEntry[k].input, 0);
        SULEV(sampEntry[k].sampler, k);
    }
    offset += 8*samplersNum;
    
    // CALNOTE_SCRATCH_BUFFERS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_SCRATCH_BUFFERS, 4);
    offset += sizeof(CALNoteHeader);
    data32 = reinterpret_cast<uint32_t*>(binary + offset);
    SULEV(*data32, config.scratchBufferSize>>2);
    offset += 4;
    
    // CALNOTE_PERSISTENT_BUFFERS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_PERSISTENT_BUFFERS, 0);
    offset += sizeof(CALNoteHeader);
    
    /* PROGRAM_INFO */
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    offset += sizeof(CALNoteHeader);
    
    CALProgramInfoEntry* progInfo = reinterpret_cast<CALProgramInfoEntry*>(
                binary + offset);
    SULEV(progInfo[0].address, 0x80001000U);
    SULEV(progInfo[0].value, config.userDataElemsNum);
    cxuint k = 0;
    for (k = 0; k < config.userDataElemsNum; k++)
    {
        SULEV(progInfo[1+(k<<2)].address, 0x80001001U+(k<<2));
        SULEV(progInfo[1+(k<<2)].value, config.userDatas[k].dataClass);
        SULEV(progInfo[1+(k<<2)+1].address, 0x80001002U+(k<<2));
        SULEV(progInfo[1+(k<<2)+1].value, config.userDatas[k].apiSlot);
        SULEV(progInfo[1+(k<<2)+2].address, 0x80001003U+(k<<2));
        SULEV(progInfo[1+(k<<2)+2].value, config.userDatas[k].regStart);
        SULEV(progInfo[1+(k<<2)+3].address, 0x80001004U+(k<<2));
        SULEV(progInfo[1+(k<<2)+3].value, config.userDatas[k].regSize);
    }
    if (isOlderThan1124)
        for (k =  config.userDataElemsNum; k < 16; k++)
        {
            SULEV(progInfo[1+(k<<2)].address, 0x80001001U+(k<<2));
            SULEV(progInfo[1+(k<<2)].value, 0);
            SULEV(progInfo[1+(k<<2)+1].address, 0x80001002U+(k<<2));
            SULEV(progInfo[1+(k<<2)+1].value, 0);
            SULEV(progInfo[1+(k<<2)+2].address, 0x80001003U+(k<<2));
            SULEV(progInfo[1+(k<<2)+2].value, 0);
            SULEV(progInfo[1+(k<<2)+3].address, 0x80001004U+(k<<2));
            SULEV(progInfo[1+(k<<2)+3].value, 0);
        }
    k = (k<<2)+1;
    
    const cxuint localSize = (isLocalPointers) ? 32768 : config.hwLocalSize;
    uint32_t curPgmRSRC2 = config.pgmRSRC2;
    curPgmRSRC2 = (curPgmRSRC2 & 0xff007fffU) | ((((localSize+255)>>8)&0x1ff)<<15);
    cxuint pgmUserSGPRsNum = 0;
    for (cxuint p = 0; p < config.userDataElemsNum; p++)
        pgmUserSGPRsNum = std::max(pgmUserSGPRsNum,
                 config.userDatas[p].regStart+config.userDatas[p].regSize);
    pgmUserSGPRsNum = (pgmUserSGPRsNum != 0) ? pgmUserSGPRsNum : 2;
    curPgmRSRC2 = (curPgmRSRC2 & 0xffffffc1U) | ((pgmUserSGPRsNum&0x1f)<<1);
    
    SULEV(progInfo[k].address, 0x80001041U);
    SULEV(progInfo[k++].value, config.usedVGPRsNum);
    SULEV(progInfo[k].address, 0x80001042U);
    SULEV(progInfo[k++].value, config.usedSGPRsNum);
    SULEV(progInfo[k].address, 0x80001863U);
    SULEV(progInfo[k++].value, 102);
    SULEV(progInfo[k].address, 0x80001864U);
    SULEV(progInfo[k++].value, 256);
    SULEV(progInfo[k].address, 0x80001043U);
    SULEV(progInfo[k++].value, config.floatMode);
    SULEV(progInfo[k].address, 0x80001044U);
    SULEV(progInfo[k++].value, config.ieeeMode);
    SULEV(progInfo[k].address, 0x80001045U);
    SULEV(progInfo[k++].value, config.scratchBufferSize>>2);
    SULEV(progInfo[k].address, 0x00002e13U);
    SULEV(progInfo[k++].value, curPgmRSRC2);
    SULEV(progInfo[k].address, 0x8000001cU);
    SULEV(progInfo[k+1].address, 0x8000001dU);
    SULEV(progInfo[k+2].address, 0x8000001eU);
    if (config.reqdWorkGroupSize[0] != 0 && config.reqdWorkGroupSize[1] != 0 &&
        config.reqdWorkGroupSize[2] != 0)
    {
        SULEV(progInfo[k].value, config.reqdWorkGroupSize[0]);
        SULEV(progInfo[k+1].value, config.reqdWorkGroupSize[1]);
        SULEV(progInfo[k+2].value, config.reqdWorkGroupSize[2]);
    }
    else
    {   /* default */
        SULEV(progInfo[k].value, 256);
        SULEV(progInfo[k+1].value, 0);
        SULEV(progInfo[k+2].value, 0);
    }
    k+=3;
    SULEV(progInfo[k].address, 0x80001841U);
    SULEV(progInfo[k++].value, 0);
    uint32_t uavMask[32];
    ::memset(uavMask, 0, 128);
    uavMask[0] = woUsedImagesMask;
    for (cxuint l = 0; l < config.args.size(); l++)
    {
        const AmdKernelArg& arg = config.args[l];
        if (arg.used && arg.argType == KernelArgType::POINTER &&
            (arg.ptrSpace == KernelPtrSpace::GLOBAL ||
             (arg.ptrSpace == KernelPtrSpace::CONSTANT && !isOlderThan1348)))
        {
            const cxuint u = tempConfig.argResIds[l];
            uavMask[u>>5] |= (1U<<(u&31));
        }
    }
    if (!isOlderThan1348 && config.useConstantData)
        uavMask[0] |= 1U<<tempConfig.constBufferId;
    if (config.usePrintf) //if printf used
    {
        if (tempConfig.printfId != AMDBIN_NOTSUPPLIED)
            uavMask[0] |= 1U<<tempConfig.printfId;
        else
            uavMask[0] |= 1U<<9;
    }
    
    SULEV(progInfo[k].address, 0x8000001fU);
    SULEV(progInfo[k++].value, uavMask[0]);
    for (cxuint p = 0; p < 32; p++)
    {
        SULEV(progInfo[k].address, 0x80001843U+p);
        SULEV(progInfo[k++].value, uavMask[p]);
    }
    SULEV(progInfo[k].address, 0x8000000aU);
    SULEV(progInfo[k++].value, 1);
    SULEV(progInfo[k].address, 0x80000078U);
    SULEV(progInfo[k++].value, 64);
    SULEV(progInfo[k].address, 0x80000081U);
    SULEV(progInfo[k++].value, 32768);
    SULEV(progInfo[k].address, 0x80000082U);
    SULEV(progInfo[k++].value, ((curPgmRSRC2>>15)&0x1ff)<<8);
    offset += 8*k;
    putCALNoteLE(noteHdr, CALNOTE_ATI_PROGINFO, 8*k);
    
    // CAL_SUBCONSTANT_BUFFERS
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_SUB_CONSTANT_BUFFERS, 0);
    offset += sizeof(CALNoteHeader);
    
    // CAL_UAV_MAILBOX_SIZE
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_UAV_MAILBOX_SIZE, 4);
    offset += sizeof(CALNoteHeader);
    data32 = reinterpret_cast<uint32_t*>(binary + offset);
    SULEV(*data32, 0);
    offset += 4;
    
    // CAL_UAV_OP_MASK
    noteHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
    putCALNoteLE(noteHdr, CALNOTE_ATI_UAV_OP_MASK, 128);
    offset += sizeof(CALNoteHeader);
    data32 = reinterpret_cast<uint32_t*>(binary + offset);
    for (cxuint k = 0; k < 32; k++)
        SULEV(data32[k], uavMask[k]);
    offset += 128;
}

static std::set<cxuint> collectUniqueIdsAndFunctionIds(const AmdInput* input)
{
    std::set<cxuint> uniqueIds;
    for (const AmdKernelInput& kernel: input->kernels)
        if (!kernel.useConfig)
        {
            const char* metadata = kernel.metadata;
            size_t pos = 0;
            for (pos = 0; pos < kernel.metadataSize-11; pos++)
                if (::memcmp(metadata+pos, "\n;uniqueid:", 11) == 0)
                    break;
            if (pos == kernel.metadataSize-11)
                continue; // not found
            const char* outEnd;
            pos += 11;
            try
            { uniqueIds.insert(cstrtovCStyle<cxuint>(metadata+pos,
                               metadata+kernel.metadataSize, outEnd)); }
            catch(const ParseException& ex)
            { } // ignore parse exception
            for (; pos < kernel.metadataSize-11; pos++)
                if (::memcmp(metadata+pos, "\n;function:", 11) == 0)
                    break;
            if (pos == kernel.metadataSize-11)
                continue; // not found
            pos += 11;
            const char* funcIdStr = (const char*)::memchr(metadata+pos, ':',
                      kernel.metadataSize-pos);
            if (funcIdStr == nullptr || funcIdStr+1 == metadata+kernel.metadataSize)
                continue;
            funcIdStr++;
            try
            { uniqueIds.insert(cstrtovCStyle<cxuint>(funcIdStr,
                           metadata+kernel.metadataSize, outEnd)); }
            catch(const ParseException& ex)
            { } // ignore parse exception
        }
    return uniqueIds;
}

/*
 * main routine to generate AmdBin for GPU
 */

cxbyte* AmdGPUBinGenerator::generate(size_t& outBinarySize) const
{
    size_t binarySize;
    const size_t kernelsNum = input->kernels.size();
    std::string driverInfo;
    uint32_t driverVersion = 99999909U;
    if (input->driverInfo.empty())
    {
        char drvInfoBuf[100];
        ::snprintf(drvInfoBuf, 100, "@(#) OpenCL 1.2 AMD-APP (%u.%u).  "
                "Driver version: %u.%u (VM)",
                 input->driverVersion/100U, input->driverVersion%100U,
                 input->driverVersion/100U, input->driverVersion%100U);
        driverInfo = drvInfoBuf;
        driverVersion = input->driverVersion;
    }
    else if (input->driverVersion == 0)
    {   // parse version
        size_t pos = input->driverInfo.find("AMD-APP"); // find AMDAPP
        try
        {
            driverInfo = input->driverInfo;
            if (pos != std::string::npos)
            {   /* let to parse version number */
                pos += 9;
                const char* end;
                driverVersion = cstrtovCStyle<cxuint>(
                        input->driverInfo.c_str()+pos, nullptr, end)*100;
                end++;
                driverVersion += cstrtovCStyle<cxuint>(end, nullptr, end);
            }
        }
        catch(const ParseException& ex)
        { driverVersion = 99999909U; /* newest possible */ }
    }
    
    const size_t mainSectionsAlign = (input->is64Bit)?8:4;
    
    const bool isOlderThan1124 = driverVersion < 112402;
    const bool isOlderThan1348 = driverVersion < 134805;
    /* checking input */
    if (input->deviceType == GPUDeviceType::UNDEFINED ||
        cxuint(input->deviceType) > cxuint(GPUDeviceType::GPUDEVICE_MAX))
        throw Exception("Undefined GPU device type");
    
    std::vector<TempAmdKernelConfig> tempAmdKernelConfigs(input->kernels.size());
    prepareTempConfigs(driverVersion, input, tempAmdKernelConfigs);
    
    /* count number of bytes required to save */
    if (!input->is64Bit)
        binarySize = sizeof(Elf32_Ehdr);
    else
        binarySize = sizeof(Elf64_Ehdr);
    
    for (const AmdKernelInput& kinput: input->kernels)
        binarySize += (kinput.kernelName.size())*3;
    binarySize += ((input->kernels.empty())?36:50) +
            ((input->llvmir!=nullptr)?8:0) +
            ((input->sourceCode!=nullptr)?8:0) /*shstrtab */ + kernelsNum*(19+17+17) +
            ((!input->compileOptions.empty())?26:1) /* static strtab size */ +
            ((input->globalData!=nullptr)?18:0);
    
    // alignment for symtab!!! check
    binarySize += ((binarySize&7)!=0) ? 8-(binarySize&7) : 0;
    binarySize += driverInfo.size() + input->compileOptions.size() +
            input->globalDataSize;
    
    if (!input->is64Bit)
        binarySize += sizeof(Elf32_Sym)*(3*kernelsNum + 1 +
                (!input->compileOptions.empty()) + (input->globalData != nullptr));
    else
        binarySize += sizeof(Elf64_Sym)*(3*kernelsNum + 1 +
                (!input->compileOptions.empty()) + (input->globalData != nullptr));
    
    std::vector<cxuint> innerBinSizes(input->kernels.size());
    std::vector<std::string> kmetadatas(input->kernels.size());
    cxuint uniqueId = 1024;
    std::set<cxuint> uniqueIds = collectUniqueIdsAndFunctionIds(input);
    
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        cxuint& innerBinSize = innerBinSizes[i];
        innerBinSize = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)*3 + sizeof(Elf32_Shdr)*6 +
            sizeof(CALEncodingEntry) + 2 + 16 + 40;
        
        const AmdKernelInput& kinput = input->kernels[i];
        
        innerBinSize += (kinput.data != nullptr) ? kinput.dataSize : 4736;
        innerBinSize += kinput.codeSize;
        size_t metadataSize = 0;
        if (kinput.useConfig)
        {
            // get new free uniqueId
            while (uniqueIds.find(uniqueId) != uniqueIds.end()) uniqueId++;
            
            const AmdKernelConfig& config = kinput.config;
            cxuint readOnlyImages = 0;
            cxuint writeOnlyImages = 0;
            cxuint uavsNum = 0;
            cxuint samplersNum = config.samplers.size();
            cxuint argSamplersNum = 0;
            cxuint constBuffersNum = 2 + (isOlderThan1348 /* cbid:2 for older drivers*/ &&
                    (input->globalData != nullptr));
            for (const AmdKernelArg& arg: config.args)
            {
                if (arg.argType >= KernelArgType::MIN_IMAGE &&
                    arg.argType <= KernelArgType::MAX_IMAGE)
                {
                    if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_READ_ONLY)
                        readOnlyImages++;
                    if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                    {
                        uavsNum++;
                        writeOnlyImages++;
                        if (writeOnlyImages > 8)
                            throw Exception("Too many write only images");
                    }
                }
                else if (arg.argType == KernelArgType::POINTER)
                {
                    if (arg.ptrSpace == KernelPtrSpace::GLOBAL)
                        // only global pointers are defined in uav table
                        uavsNum++;
                    if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
                        constBuffersNum++;
                }
                else if (arg.argType == KernelArgType::SAMPLER)
                    argSamplersNum++;
            }
            samplersNum += argSamplersNum;
            
            if (config.usePrintf)
                uavsNum++;
            
            innerBinSize += 20*17 /*calNoteHeaders*/ + 16 + 128 + (18+32 +
                4*((isOlderThan1124)?16:config.userDataElemsNum))*8 /* proginfo */ +
                    readOnlyImages*4 /* inputs */ + 16*uavsNum /* uavs */ +
                    8*samplersNum /* samplers */ + 8*constBuffersNum /* cbids */;
            
            kmetadatas[i] = generateMetadata(driverVersion, input, kinput,
                     tempAmdKernelConfigs[i], argSamplersNum, uniqueId);
            
            metadataSize = kmetadatas[i].size() + 32 /* header size */;
            uniqueId++;
        }
        else // if defined in calNotes (no config)
        {
            for (const CALNoteInput& calNote: kinput.calNotes)
                innerBinSize += 20 + calNote.header.descSize;
            metadataSize = kinput.metadataSize + kinput.headerSize;
        }
        
        binarySize += innerBinSize + metadataSize;
    }
    
    size_t sourceCodeSize = 0;
    if (input->sourceCode != nullptr)
    {
        sourceCodeSize = input->sourceCodeSize;
        if (sourceCodeSize==0)
            sourceCodeSize = ::strlen(input->sourceCode);
    }
    
    if (input->sourceCode!=nullptr)
        binarySize += sourceCodeSize;
    if (input->llvmir!=nullptr)
        binarySize += input->llvmirSize;
    if ((binarySize & (mainSectionsAlign-1)) != 0) // main section alignment
        binarySize += mainSectionsAlign - (binarySize & (mainSectionsAlign-1));
    if (!input->is64Bit)
        binarySize += sizeof(Elf32_Shdr)*(5 + (input->kernels.empty()?0:2) +
                (input->sourceCode != nullptr) + (input->llvmir != nullptr));
    else
        binarySize += sizeof(Elf64_Shdr)*(5 + (input->kernels.empty()?0:2) +
                (input->sourceCode != nullptr) + (input->llvmir != nullptr));
    
    /********
     * writing data
     *********/
    cxbyte* binary = new cxbyte[binarySize];
    size_t offset = 0;
    
    try
    {
    const cxuint mainSectionsNum = ((input->kernels.empty())?5:7) +
            (input->sourceCode!=nullptr) + (input->llvmir!=nullptr);
    if (!input->is64Bit)
    {
        Elf32_Ehdr& mainHdr = *reinterpret_cast<Elf32_Ehdr*>(binary);
        static const cxbyte elf32Ident[16] = {
                0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2LSB, EV_CURRENT, 
                ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0 };
        ::memcpy(mainHdr.e_ident, elf32Ident, 16);
        SULEV(mainHdr.e_type, ET_EXEC);
        SULEV(mainHdr.e_machine, gpuDeviceCodeTable[cxuint(input->deviceType)]);
        SULEV(mainHdr.e_version, EV_CURRENT);
        SULEV(mainHdr.e_entry, 0);
        SULEV(mainHdr.e_flags, 0);
        SULEV(mainHdr.e_phoff, 0);
        SULEV(mainHdr.e_ehsize, sizeof(Elf32_Ehdr));
        SULEV(mainHdr.e_phnum, 0);
        SULEV(mainHdr.e_phentsize, 0);
        SULEV(mainHdr.e_shnum, mainSectionsNum);
        SULEV(mainHdr.e_shentsize, sizeof(Elf32_Shdr));
        SULEV(mainHdr.e_shstrndx, 1);
        offset += sizeof(Elf32_Ehdr);
    }
    else
    {
        Elf64_Ehdr& mainHdr = *reinterpret_cast<Elf64_Ehdr*>(binary);
        static const cxbyte elf64Ident[16] = {
                0x7f, 'E', 'L', 'F', ELFCLASS64, ELFDATA2LSB, EV_CURRENT, 
                ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0 };
        ::memcpy(mainHdr.e_ident, elf64Ident, 16);
        SULEV(mainHdr.e_type, ET_EXEC);
        SULEV(mainHdr.e_machine, gpuDeviceCodeTable[cxuint(input->deviceType)]);
        SULEV(mainHdr.e_version, EV_CURRENT);
        SULEV(mainHdr.e_entry, 0);
        SULEV(mainHdr.e_flags, 0);
        SULEV(mainHdr.e_phoff, 0);
        SULEV(mainHdr.e_ehsize, sizeof(Elf64_Ehdr));
        SULEV(mainHdr.e_phnum, 0);
        SULEV(mainHdr.e_phentsize, 0);
        SULEV(mainHdr.e_shnum, mainSectionsNum);
        SULEV(mainHdr.e_shentsize, sizeof(Elf64_Shdr));
        SULEV(mainHdr.e_shstrndx, 1);
        offset += sizeof(Elf64_Ehdr);
    }
    size_t sectionOffsets[9];
    sectionOffsets[0] = offset;
    if (!input->kernels.empty())
    {
        ::memcpy(binary+offset,
             "\000.shstrtab\000.strtab\000.symtab\000.rodata\000.text\000.comment", 50);
        offset += 50;
    }
    else
    {
        ::memcpy(binary+offset, "\000.shstrtab\000.strtab\000.symtab\000.comment", 36);
        offset += 36;
    }
    /* additional sections names */
    if (input->sourceCode!=nullptr)
    {
        ::memcpy(binary+offset, ".source", 8);
        offset += 8;
    }
    if (input->llvmir!=nullptr)
    {
        ::memcpy(binary+offset, ".llvmir", 8);
        offset += 8;
    }
    
    // .strtab
    sectionOffsets[1] = offset;
    if (!input->compileOptions.empty())
    {
        ::memcpy(binary+offset, "\000__OpenCL_compile_options", 26);
        offset += 26;
    }
    else // empty compile options
        binary[offset++] = 0;
    
    if (input->globalData != nullptr)
    {
        if (!isOlderThan1348)
            ::memcpy(binary+offset, "__OpenCL_0_global", 18);
        else
            ::memcpy(binary+offset, "__OpenCL_2_global", 18);
        offset += 18;
    }
    for (const AmdKernelInput& kernel: input->kernels)
    {
        ::memcpy(binary+offset, "__OpenCL_", 9);
        offset += 9;
        ::memcpy(binary+offset, kernel.kernelName.c_str(), kernel.kernelName.size());
        offset += kernel.kernelName.size();
        ::memcpy(binary+offset, "_metadata", 10);
        offset += 10;
        ::memcpy(binary+offset, "__OpenCL_", 9);
        offset += 9;
        ::memcpy(binary+offset, kernel.kernelName.c_str(), kernel.kernelName.size());
        offset += kernel.kernelName.size();
        ::memcpy(binary+offset, "_kernel", 8);
        offset += 8;
        ::memcpy(binary+offset, "__OpenCL_", 9);
        offset += 9;
        ::memcpy(binary+offset, kernel.kernelName.c_str(), kernel.kernelName.size());
        offset += kernel.kernelName.size();
        ::memcpy(binary+offset, "_header", 8);
        offset += 8;
    }
    
    size_t alignFix = 0;
    if ((offset & 7) != 0)
    {
        ::memset(binary + offset, 0, 8-(offset&7));
        alignFix = 8-(offset&7);
        offset += 8-(offset&7);
    }
    // .symtab
    sectionOffsets[2] = offset;
    if (!input->is64Bit)
        putMainSymbols<Elf32_Sym>(binary, offset, input, kmetadatas, innerBinSizes);
    else
        putMainSymbols<Elf64_Sym>(binary, offset, input, kmetadatas, innerBinSizes);
    // .rodata
    sectionOffsets[3] = offset;
    if (input->globalData != nullptr)
    {
        ::memcpy(binary+offset, input->globalData, input->globalDataSize);
        offset += input->globalDataSize;
    }
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        const AmdKernelInput& kernel = input->kernels[i];
        if (kernel.useConfig)
        {
            const TempAmdKernelConfig& tempConfig = tempAmdKernelConfigs[i];
            ::memcpy(binary+offset, kmetadatas[i].c_str(), kmetadatas[i].size());
            offset += kmetadatas[i].size();
            
            uint32_t* header = reinterpret_cast<uint32_t*>(binary+offset);
            SULEV(header[0], (driverVersion >= 164205) ? tempConfig.uavPrivate : 0);
            SULEV(header[1], 0);
            SULEV(header[2], (isOlderThan1348)? 0 : tempConfig.uavPrivate);
            SULEV(header[3], kernel.config.hwLocalSize);
            SULEV(header[4], (input->is64Bit?8:0) | (kernel.config.usePrintf?2:0));
            SULEV(header[5], 1);
            SULEV(header[6], 0);
            SULEV(header[7], 0);
            offset += 32;
        }
        else
        {
            ::memcpy(binary+offset, kernel.metadata, kernel.metadataSize);
            offset += kernel.metadataSize;
            ::memcpy(binary+offset, kernel.header, kernel.headerSize);
            offset += kernel.headerSize;
        }
    }
    
    /* kernel binaries */
    sectionOffsets[4] = offset;
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        const size_t innerBinOffset = offset;
        const AmdKernelInput& kinput = input->kernels[i];
        
        Elf32_Ehdr& innerHdr = *reinterpret_cast<Elf32_Ehdr*>(binary + offset);
        static const cxbyte elf32Ident[16] = {
                0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2LSB, EV_CURRENT, 
                0x64, 1, 0, 0, 0, 0, 0, 0, 0 };
        ::memcpy(innerHdr.e_ident, elf32Ident, 16);
        SULEV(innerHdr.e_type, ET_EXEC);
        SULEV(innerHdr.e_machine, 0x7d);
        SULEV(innerHdr.e_version, EV_CURRENT);
        SULEV(innerHdr.e_entry, 0);
        SULEV(innerHdr.e_flags, 1);
        SULEV(innerHdr.e_phoff, sizeof(Elf32_Ehdr));
        SULEV(innerHdr.e_shoff, 0xd0);
        SULEV(innerHdr.e_ehsize, sizeof(Elf32_Ehdr));
        SULEV(innerHdr.e_phnum, 3);
        SULEV(innerHdr.e_phentsize, sizeof(Elf32_Phdr));
        SULEV(innerHdr.e_shnum, 6);
        SULEV(innerHdr.e_shentsize, sizeof(Elf32_Shdr));
        SULEV(innerHdr.e_shstrndx, 1);
        offset += sizeof(Elf32_Ehdr);
        
        /* ??? */
        Elf32_Phdr* programHdrTable = reinterpret_cast<Elf32_Phdr*>(binary + offset);
        ::memset(programHdrTable, 0, sizeof(Elf32_Phdr));
        SULEV(programHdrTable->p_type, 0x70000002U);
        SULEV(programHdrTable->p_flags, 0);
        SULEV(programHdrTable->p_offset, 0x94);
        SULEV(programHdrTable->p_vaddr, 0);
        SULEV(programHdrTable->p_paddr, 0);
        SULEV(programHdrTable->p_filesz, sizeof(CALEncodingEntry));
        SULEV(programHdrTable->p_memsz, 0);
        SULEV(programHdrTable->p_align, 0);
        programHdrTable++;
        SULEV(programHdrTable->p_type, PT_NOTE);
        SULEV(programHdrTable->p_flags, 0);
        SULEV(programHdrTable->p_offset, 0x1c0);
        SULEV(programHdrTable->p_vaddr, 0);
        SULEV(programHdrTable->p_paddr, 0);
        SULEV(programHdrTable->p_memsz, 0);
        SULEV(programHdrTable->p_align, 0);
        Elf32_Phdr* notePrgHdr = programHdrTable;
        programHdrTable++;
        SULEV(programHdrTable->p_type, PT_LOAD);
        SULEV(programHdrTable->p_flags, 0);
        SULEV(programHdrTable->p_vaddr, 0);
        SULEV(programHdrTable->p_paddr, 0);
        SULEV(programHdrTable->p_align, 0);
        Elf32_Phdr* loadPrgHdr = programHdrTable;
        programHdrTable++;
        
        offset += sizeof(Elf32_Phdr)*3;
        /* CALEncodingEntry */
        CALEncodingEntry& encEntry = *reinterpret_cast<CALEncodingEntry*>(binary + offset);
        SULEV(encEntry.type, 4);
        SULEV(encEntry.machine, gpuDeviceInnerCodeTable[cxuint(input->deviceType)]);
        SULEV(encEntry.flags, 0);
        SULEV(encEntry.offset, 0x1c0);
        offset += sizeof(CALEncodingEntry);
        // shstrtab content
        ::memcpy(binary + offset,
                 "\000.shstrtab\000.text\000.data\000.symtab\000.strtab\000", 40);
        offset += 40;
        /* section headers */
        Elf32_Shdr* sectionHdrTable = reinterpret_cast<Elf32_Shdr*>(binary + offset);
        ::memset(binary+offset, 0, sizeof(Elf32_Shdr));
        sectionHdrTable++;
        // .shstrtab
        putElfSectionLE(sectionHdrTable++, 1, SHT_STRTAB, 0, 0xa8, 40, 0, 0, 0);
        
        offset += sizeof(Elf32_Shdr)*6;
        const size_t encOffset = offset;
        if (kinput.useConfig)
            generateCALNotes(binary, offset, input, driverVersion, kinput,
                     tempAmdKernelConfigs[i]);
        else // from CALNotes array
            for (const CALNoteInput& calNote: kinput.calNotes)
            {
                CALNoteHeader* cnHdr = reinterpret_cast<CALNoteHeader*>(binary + offset);
                SULEV(cnHdr->descSize, calNote.header.descSize);
                SULEV(cnHdr->nameSize, calNote.header.nameSize);
                SULEV(cnHdr->type, calNote.header.type);
                ::memcpy(cnHdr->name, &calNote.header.name, 8);
                offset += sizeof(CALNoteHeader);
                ::memcpy(binary + offset, calNote.data, calNote.header.descSize);
                offset += calNote.header.descSize;
            }
        SULEV(notePrgHdr->p_filesz, offset-innerBinOffset-0x1c0);
        SULEV(loadPrgHdr->p_offset, offset-innerBinOffset);
        
        const size_t loadPrgPartOffset = offset;
        // .text
        putElfSectionLE(sectionHdrTable++, 11, SHT_PROGBITS, 0, offset-innerBinOffset,
                        kinput.codeSize, 0, 0, 0);
        ::memcpy(binary + offset, kinput.code, kinput.codeSize);
        offset += kinput.codeSize;
        // .data
        putElfSectionLE(sectionHdrTable++, 17, SHT_PROGBITS, 0, offset-innerBinOffset,
                        (kinput.data!=nullptr)?kinput.dataSize:4736, 0, 0, 0, 0,
                        (kinput.data!=nullptr)?kinput.dataSize:4736);
        if (kinput.data != nullptr)
        {
            ::memcpy(binary + offset, kinput.data, kinput.dataSize);
            offset += kinput.dataSize;
        }
        else
        {
            ::memset(binary + offset, 0, 4736);
            offset += 4736;
        }
        // .symtab
        putElfSectionLE(sectionHdrTable++, 23, SHT_SYMTAB, 0, offset-innerBinOffset,
                        sizeof(Elf32_Sym), 5, 1, 0, 0, sizeof(Elf32_Sym));
        ::memset(binary + offset, 0, sizeof(Elf32_Sym));
        offset += sizeof(Elf32_Sym);
        // .strtab
        putElfSectionLE(sectionHdrTable, 31, SHT_STRTAB, 0, offset-innerBinOffset,
                        2, 0, 0, 0);
        binary[offset++] = 0;
        binary[offset++] = 0;
        // update encodingEntry and Program headers
        SULEV(loadPrgHdr->p_filesz, offset-loadPrgPartOffset);
        SULEV(loadPrgHdr->p_memsz, offset-loadPrgPartOffset);
        SULEV(encEntry.size, offset-encOffset);
    }
    // .comment
    sectionOffsets[5] = offset;
    ::memcpy(binary+offset, input->compileOptions.c_str(), input->compileOptions.size());
    offset += input->compileOptions.size();
    ::memcpy(binary+offset, driverInfo.c_str(), driverInfo.size());
    offset += driverInfo.size();
    sectionOffsets[6] = offset;
    // .source
    if (input->sourceCode!=nullptr)
    {
        ::memcpy(binary+offset, input->sourceCode, sourceCodeSize);
        offset += sourceCodeSize;
    }
    sectionOffsets[7] = offset;
    // .source
    if (input->llvmir!=nullptr)
    {
        ::memcpy(binary+offset, input->llvmir, input->llvmirSize);
        offset += input->llvmirSize;
    }
    sectionOffsets[8] = offset;
    if ((offset & (mainSectionsAlign-1)) != 0) // main section alignment
    {
        const size_t tofill = mainSectionsAlign - (offset & (mainSectionsAlign-1));
        ::memset(binary+offset, 0, tofill);
        offset += tofill;
    }
    /* main sections */
    if (!input->is64Bit)
    {
        Elf32_Ehdr& mainHdr = *reinterpret_cast<Elf32_Ehdr*>(binary);
        SULEV(mainHdr.e_shoff, offset);
        putMainSections<Elf32_Shdr, Elf32_Sym>(binary, offset, sectionOffsets, alignFix,
                input->kernels.empty(), input->sourceCode!=nullptr, input->llvmir!=nullptr);
    }
    else
    {
        Elf64_Ehdr& mainHdr = *reinterpret_cast<Elf64_Ehdr*>(binary);
        SULEV(mainHdr.e_shoff, offset);
        putMainSections<Elf64_Shdr, Elf64_Sym>(binary, offset, sectionOffsets, alignFix,
                input->kernels.empty(), input->sourceCode!=nullptr, input->llvmir!=nullptr);
    }
    } // try
    catch(...)
    {
        delete[] binary;
        throw;
    }
    assert(offset == binarySize);
    
    outBinarySize = binarySize;
    return binary;
}
