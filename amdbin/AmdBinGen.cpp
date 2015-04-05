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
#include <cctype>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <bitset>
#include <string>
#include <vector>
#include <memory>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/amdbin/AmdBinGen.h>

using namespace CLRX;

template<typename ElfSection>
static void putElfSectionLE(BinaryOStream& bos, size_t shName, uint32_t shType,
        uint32_t shFlags, size_t offset, size_t size, uint32_t link,
        uint32_t info = 0, uint32_t addrAlign = 1, size_t addr = 0, uint32_t entSize = 0)
{
    ElfSection shdr;
    SLEV(shdr.sh_name, shName);
    SLEV(shdr.sh_type, shType);
    SLEV(shdr.sh_flags, shFlags);
    SLEV(shdr.sh_addr, addr);
    SLEV(shdr.sh_offset, offset);
    SLEV(shdr.sh_size, size);
    SLEV(shdr.sh_link, link);
    SLEV(shdr.sh_info, info);
    SLEV(shdr.sh_addralign, addrAlign);
    SLEV(shdr.sh_entsize, entSize);
    bos.writeObject(shdr);
}

template<typename ElfSym>
static void putElfSymbolLE(BinaryOStream& bos, uint32_t symName, uint32_t value,
        uint32_t size, uint16_t shndx, cxbyte info, cxbyte other = 0)
{
    ElfSym sym;
    SLEV(sym.st_name, symName); 
    SLEV(sym.st_value, value);
    SLEV(sym.st_size, size);
    SLEV(sym.st_shndx, shndx);
    sym.st_info = info;
    sym.st_other = other;
    bos.writeObject(sym);
}

static void putElfProgramHeader32LE(BinaryOStream& bos, uint32_t type, uint32_t offset,
        uint32_t filesz, uint32_t flags, uint32_t align, uint32_t memsz = 0,
        uint32_t vaddr = 0, uint32_t paddr = 0)
{
    const Elf32_Phdr phdr = { LEV(type), LEV(offset), LEV(vaddr), LEV(paddr), LEV(filesz),
        LEV(memsz), LEV(flags), LEV(align) };
    bos.writeObject(phdr);
}

static inline void putCALNoteLE(BinaryOStream& bos, uint32_t type, uint32_t descSize)
{
    const CALNoteHeader nhdr = { LEV(8U), LEV(descSize), LEV(type),
        { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } };
    bos.writeObject(nhdr);
}

static inline void putCALUavEntryLE(BinaryOStream& bos, uint32_t uavId, uint32_t f1,
          uint32_t f2, uint32_t type)
{
    const CALUAVEntry uavEntry = { LEV(uavId), LEV(f1), LEV(f2), LEV(type) };
    bos.writeObject(uavEntry);
}

static inline void putCBMaskLE(BinaryOStream& bos, uint32_t index, uint32_t size)
{
    const CALConstantBufferMask cbMask = { LEV(index), LEV(size) };
    bos.writeObject(cbMask);
}

static inline void putSamplerEntryLE(BinaryOStream& bos, uint32_t input, uint32_t sampler)
{
    const CALSamplerMapEntry samplerEntry = { LEV(input), LEV(sampler) };
    bos.writeObject(samplerEntry);
}

static inline void putProgInfoEntryLE(BinaryOStream& bos, uint32_t address, uint32_t value)
{
    const CALProgramInfoEntry piEntry = { LEV(address), LEV(value) };
    bos.writeObject(piEntry);
}

// e_type (16-bit)
static const uint16_t gpuDeviceCodeTable[13] =
{
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

/// CALNoteEntry (32-bit)
static const uint32_t gpuDeviceInnerCodeTable[13] =
{
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

AmdGPUBinGenerator::AmdGPUBinGenerator() : manageable(false), input(nullptr)
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
    uint32_t uavsNum;
    uint32_t calNotesSize;
};

template<typename ElfSym>
static void putMainSymbols(BinaryOStream& bos, size_t& offset, const AmdInput* input,
    const std::vector<std::string>& kmetadatas, const std::vector<cxuint>& innerBinSizes)
{
    putElfSymbolLE<ElfSym>(bos, 0, 0, 0, 0, 0, 0);
    size_t namePos = 1;
    if (!input->compileOptions.empty())
    {
        putElfSymbolLE<ElfSym>(bos, namePos, 0, input->compileOptions.size(), 
                (input->kernels.size()!=0)?6:4, ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0);
        namePos += 25;
        offset += sizeof(ElfSym);
    }
    size_t rodataPos = 0;
    if (input->globalData != nullptr)
    {
        putElfSymbolLE<ElfSym>(bos, namePos, 0, input->globalDataSize, 4,
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
        putElfSymbolLE<ElfSym>(bos, namePos, rodataPos, metadataSize, 4,
                ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0);
        namePos += 19 + kernel.kernelName.size();
        rodataPos += metadataSize;
        /* kernel code */
        putElfSymbolLE<ElfSym>(bos, namePos, textPos, innerBinSizes[i], 5,
                ELF32_ST_INFO(STB_LOCAL, STT_FUNC), 0);
        namePos += 17 + kernel.kernelName.size();
        textPos += innerBinSizes[i];
        /* kernel header */
        const size_t headerSize = (kernel.useConfig) ? 32 : kernel.headerSize;
        putElfSymbolLE<ElfSym>(bos, namePos, rodataPos, headerSize, 4,
                ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0);
        namePos += 17 + kernel.kernelName.size();
        rodataPos += headerSize;
    }
    offset += sizeof(ElfSym)*(input->kernels.size()*3 + 1);
}

template<typename ElfShdr, typename ElfSym>
static void putMainSections(BinaryOStream& bos, size_t &offset,
        const size_t* sectionOffsets, size_t alignFix, bool noKernels, bool haveSource,
        bool haveLLVMIR)
{
    putElfSectionLE<ElfShdr>(bos, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    // .shstrtab
    putElfSectionLE<ElfShdr>(bos, 1, SHT_STRTAB, SHF_STRINGS, sectionOffsets[0],
            sectionOffsets[1]-sectionOffsets[0], 0);
    // .strtab
    putElfSectionLE<ElfShdr>(bos, 11, SHT_STRTAB, SHF_STRINGS, sectionOffsets[1],
            sectionOffsets[2]-sectionOffsets[1]-alignFix, 0);
    // .symtab
    putElfSectionLE<ElfShdr>(bos, 19, SHT_SYMTAB, 0, sectionOffsets[2],
            sectionOffsets[3]-sectionOffsets[2], 2, 0, 8, 0, sizeof(ElfSym));
    size_t shNamePos = 27;
    if (!noKernels) // .text not emptyu
    {   // .rodata
        putElfSectionLE<ElfShdr>(bos, 27, SHT_PROGBITS, SHF_ALLOC, sectionOffsets[3],
                sectionOffsets[4]-sectionOffsets[3], 0);
        // .text
        putElfSectionLE<ElfShdr>(bos, 35, SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR,
                sectionOffsets[4], sectionOffsets[5]-sectionOffsets[4], 0);
        shNamePos += 14;
    }
    // .comment
    putElfSectionLE<ElfShdr>(bos, shNamePos, SHT_PROGBITS, 0, sectionOffsets[5],
            sectionOffsets[6]-sectionOffsets[5], 0);
    shNamePos += 9;
    if (haveSource)
    {
        putElfSectionLE<ElfShdr>(bos, shNamePos, SHT_PROGBITS, 0, sectionOffsets[6],
                sectionOffsets[7]-sectionOffsets[6], 0);
        shNamePos += 8;
    }
    if (haveLLVMIR)
        putElfSectionLE<ElfShdr>(bos, shNamePos, SHT_PROGBITS, 0, sectionOffsets[7],
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
        if (config.usedVGPRsNum > 256)
            throw Exception("Used VGPRs number out of range");
        if (config.usedSGPRsNum > 102)
            throw Exception("Used SGPRs number out of range");
        if (config.hwLocalSize > 32768)
            throw Exception("HWLocalSize out of range");
        
        /* filling input */
        if (config.hwRegion == AMDBIN_DEFAULT)
            tempConfig.hwRegion = 0;
        else
            tempConfig.hwRegion = config.hwRegion;
        
        /* checking arg types */
        for (const AmdKernelArgInput& arg: config.args)
            if (arg.argType > KernelArgType::MAX_VALUE)
                throw Exception("Unknown argument type");
            else if (arg.argType == KernelArgType::POINTER)
            {
                if (arg.pointerType > KernelArgType::MAX_VALUE)
                    throw Exception("Unknown argument's pointer type");
                if (arg.ptrSpace > KernelPtrSpace::MAX_VALUE ||
                    arg.ptrSpace == KernelPtrSpace::NONE)
                    throw Exception("Wrong pointer space type");
            }
            else if (arg.argType >= KernelArgType::MIN_IMAGE &&
                arg.argType <= KernelArgType::MAX_IMAGE)
            {
                if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == 0)
                    throw Exception("Invalid access qualifier for image");
            }
        
        if (config.uavPrivate == AMDBIN_DEFAULT)
        {   /* compute uavPrivate */
            bool hasStructures = false;
            uint32_t amountOfArgs = 0;
            for (const AmdKernelArgInput& arg: config.args)
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
                for (const AmdKernelArgInput& arg: config.args)
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
        
        if (tempConfig.uavId != AMDBIN_NOTSUPPLIED && tempConfig.uavId >= 1024)
            throw Exception("UavId out of range");
        if (tempConfig.constBufferId != AMDBIN_NOTSUPPLIED &&
            tempConfig.constBufferId >= 1024)
            throw Exception("ConstBufferId out of range");
        if (tempConfig.printfId != AMDBIN_NOTSUPPLIED && tempConfig.printfId >= 1024)
            throw Exception("PrintfId out of range");
        if (tempConfig.privateId != AMDBIN_NOTSUPPLIED && tempConfig.privateId >= 1024)
            throw Exception("PrivateId out of range");
        
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
            const AmdKernelArgInput& arg = config.args[k];
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
            const AmdKernelArgInput& arg = config.args[k];
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
                for (; cbIdsCount < 160 && cbIdMask[cbIdsCount]; cbIdsCount++);
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
    const char* devName = getGPUDeviceTypeName(input->deviceType);
    while (*devName != 0) // dev name must be in lower letter
        metadata += ::tolower(*devName++);
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
        const AmdKernelArgInput& arg = config.args[k];
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
    for (const AmdKernelArgInput& arg: config.args)
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
    if (tempConfig.privateId != AMDBIN_NOTSUPPLIED)
    {
        metadata += ";privateid:";
        itocstrCStyle(tempConfig.privateId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    for (cxuint k = 0; k < config.args.size(); k++)
    {
        const AmdKernelArgInput& arg = config.args[k];
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

static void generateCALNotes(BinaryOStream& bos, size_t& offset, const AmdInput* input,
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
    for (const AmdKernelArgInput& arg: config.args)
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
    putCALNoteLE(bos, CALNOTE_ATI_INPUTS, 4*readOnlyImages);
    offset += sizeof(CALNoteHeader);
    {
        uint32_t rdimgIds[128];
        for (cxuint k = 0; k < readOnlyImages; k++)
            SLEV(rdimgIds[k], isOlderThan1124 ? readOnlyImages-k-1 : k);
        bos.writeArray(readOnlyImages, rdimgIds);
        offset += 4*readOnlyImages;
    }
    // CALNOTE_OUTPUTS
    putCALNoteLE(bos, CALNOTE_ATI_OUTPUTS, 0);
    offset += sizeof(CALNoteHeader);
    // CALNOTE_UAV
    putCALNoteLE(bos, CALNOTE_ATI_UAV, 16*tempConfig.uavsNum);
    offset += sizeof(CALNoteHeader);
    
    if (isOlderThan1124)
    {   // for old drivers
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (arg.argType >= KernelArgType::MIN_IMAGE &&
                arg.argType <= KernelArgType::MAX_IMAGE &&
                (arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                /* writeOnlyImages */
                putCALUavEntryLE(bos, tempConfig.argResIds[k], 2, imgUavDimTable[
                        cxuint(arg.argType)-cxuint(KernelArgType::MIN_IMAGE)], 3);
        }
        if (config.usePrintf)
            putCALUavEntryLE(bos, tempConfig.uavId, 4, 0, 5);
        // global buffers
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::GLOBAL) // uavid
                putCALUavEntryLE(bos, tempConfig.argResIds[k], 4, 0, 5);
        }
    }
    else
    {   /* in argument order */
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (arg.argType >= KernelArgType::MIN_IMAGE &&
                arg.argType <= KernelArgType::MAX_IMAGE &&
                (arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                // write_only images
                putCALUavEntryLE(bos, tempConfig.argResIds[k], 2, 2, 5);
            else if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::GLOBAL) // uavid
                putCALUavEntryLE(bos, tempConfig.argResIds[k], 4, 0, 5);
        }
        
        if (config.usePrintf)
            putCALUavEntryLE(bos, tempConfig.uavId, 0, 0, 5);
    }
    offset += 16*tempConfig.uavsNum;
    
    // CALNOTE_CONDOUT
    putCALNoteLE(bos, CALNOTE_ATI_CONDOUT, 4);
    bos.writeObject(LEV(uint32_t(config.condOut)));
    offset += sizeof(CALNoteHeader) + 4;
    // CALNOTE_FLOAT32CONSTS
    putCALNoteLE(bos, CALNOTE_ATI_FLOAT32CONSTS, 0);
    offset += sizeof(CALNoteHeader);
    // CALNOTE_INT32CONSTS
    putCALNoteLE(bos, CALNOTE_ATI_INT32CONSTS, 0);
    offset += sizeof(CALNoteHeader);
    // CALNOTE_BOOL32CONSTS
    putCALNoteLE(bos, CALNOTE_ATI_BOOL32CONSTS, 0);
    offset += sizeof(CALNoteHeader);
    
    // CALNOTE_EARLYEXIT
    putCALNoteLE(bos, CALNOTE_ATI_EARLYEXIT, 4);
    bos.writeObject(LEV(uint32_t(config.earlyExit)));
    offset += sizeof(CALNoteHeader) + 4;
    
    // CALNOTE_GLOBAL_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_GLOBAL_BUFFERS, 0);
    offset += sizeof(CALNoteHeader);
    
    // CALNOTE_CONSTANT_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_CONSTANT_BUFFERS, 8*constBuffersNum);
    offset += sizeof(CALNoteHeader);
    
    if (isOlderThan1124)
    {   /* for driver 12.10 */
        for (cxuint k = config.args.size(); k > 0; k--)
        {
            const AmdKernelArgInput& arg = config.args[k-1];
            if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::CONSTANT)
                putCBMaskLE(bos, tempConfig.argResIds[k-1], arg.constSpaceSize!=0 ?
                    ((arg.constSpaceSize+15)>>4) : 4096);
        }
        if (input->globalData != nullptr)
            putCBMaskLE(bos, 2, (input->globalDataSize+15)>>4);
        putCBMaskLE(bos, 1, (tempConfig.argsSpace+15)>>4);
        putCBMaskLE(bos, 0, 15);
    }
    else 
    {
        putCBMaskLE(bos, 0, 0);
        putCBMaskLE(bos, 1, 0);
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                arg.ptrSpace == KernelPtrSpace::CONSTANT)
                putCBMaskLE(bos, tempConfig.argResIds[k], 0);
        }
        if (isOlderThan1348 && input->globalData != nullptr)
            putCBMaskLE(bos, 2, 0);
    }
    offset += 8*constBuffersNum;
    
    // CALNOTE_INPUT_SAMPLERS
    putCALNoteLE(bos, CALNOTE_ATI_INPUT_SAMPLERS, 8*samplersNum);
    offset += sizeof(CALNoteHeader);
    
    for (cxuint k = 0; k < config.samplers.size(); k++)
        putSamplerEntryLE(bos, 0, k+argSamplersNum);
    for (cxuint k = 0; k < argSamplersNum; k++)
        putSamplerEntryLE(bos, 0, k);
    offset += 8*samplersNum;
    
    // CALNOTE_SCRATCH_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_SCRATCH_BUFFERS, 4);
    bos.writeObject<uint32_t>(LEV(config.scratchBufferSize>>2));
    offset += sizeof(CALNoteHeader) + 4;
    
    // CALNOTE_PERSISTENT_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_PERSISTENT_BUFFERS, 0);
    offset += sizeof(CALNoteHeader);
    
    /* PROGRAM_INFO */
    const cxuint progInfoSize = (18+32 +
            4*((isOlderThan1124)?16:config.userDataElemsNum))*8;
    putCALNoteLE(bos, CALNOTE_ATI_PROGINFO, progInfoSize);
    offset += sizeof(CALNoteHeader);
    
    putProgInfoEntryLE(bos, 0x80001000U, config.userDataElemsNum);
    cxuint k = 0;
    for (k = 0; k < config.userDataElemsNum; k++)
    {
        putProgInfoEntryLE(bos, 0x80001001U+(k<<2), config.userDatas[k].dataClass);
        putProgInfoEntryLE(bos, 0x80001002U+(k<<2), config.userDatas[k].apiSlot);
        putProgInfoEntryLE(bos, 0x80001003U+(k<<2), config.userDatas[k].regStart);
        putProgInfoEntryLE(bos, 0x80001004U+(k<<2), config.userDatas[k].regSize);
    }
    if (isOlderThan1124)
        for (k =  config.userDataElemsNum; k < 16; k++)
        {
            putProgInfoEntryLE(bos, 0x80001001U+(k<<2), 0);
            putProgInfoEntryLE(bos, 0x80001002U+(k<<2), 0);
            putProgInfoEntryLE(bos, 0x80001003U+(k<<2), 0);
            putProgInfoEntryLE(bos, 0x80001004U+(k<<2), 0);
        }
    
    const cxuint localSize = (isLocalPointers) ? 32768 : config.hwLocalSize;
    uint32_t curPgmRSRC2 = config.pgmRSRC2;
    curPgmRSRC2 = (curPgmRSRC2 & 0xff007fffU) | ((((localSize+255)>>8)&0x1ff)<<15);
    cxuint pgmUserSGPRsNum = 0;
    for (cxuint p = 0; p < config.userDataElemsNum; p++)
        pgmUserSGPRsNum = std::max(pgmUserSGPRsNum,
                 config.userDatas[p].regStart+config.userDatas[p].regSize);
    pgmUserSGPRsNum = (pgmUserSGPRsNum != 0) ? pgmUserSGPRsNum : 2;
    curPgmRSRC2 = (curPgmRSRC2 & 0xffffffc1U) | ((pgmUserSGPRsNum&0x1f)<<1);
    
    putProgInfoEntryLE(bos, 0x80001041U, config.usedVGPRsNum);
    putProgInfoEntryLE(bos, 0x80001042U, config.usedSGPRsNum);
    putProgInfoEntryLE(bos, 0x80001863U, 102);
    putProgInfoEntryLE(bos, 0x80001864U, 256);
    putProgInfoEntryLE(bos, 0x80001043U, config.floatMode);
    putProgInfoEntryLE(bos, 0x80001044U, config.ieeeMode);
    putProgInfoEntryLE(bos, 0x80001045U, config.scratchBufferSize>>2);
    putProgInfoEntryLE(bos, 0x00002e13U, curPgmRSRC2);
    
    if (config.reqdWorkGroupSize[0] != 0 && config.reqdWorkGroupSize[1] != 0 &&
        config.reqdWorkGroupSize[2] != 0)
    {
        putProgInfoEntryLE(bos, 0x8000001cU, config.reqdWorkGroupSize[0]);
        putProgInfoEntryLE(bos, 0x8000001dU, config.reqdWorkGroupSize[1]);
        putProgInfoEntryLE(bos, 0x8000001eU, config.reqdWorkGroupSize[2]);
    }
    else
    {   /* default */
        putProgInfoEntryLE(bos, 0x8000001cU, 256);
        putProgInfoEntryLE(bos, 0x8000001dU, 0);
        putProgInfoEntryLE(bos, 0x8000001eU, 0);
    }
    putProgInfoEntryLE(bos, 0x80001841U, 0);
    uint32_t uavMask[32];
    ::memset(uavMask, 0, 128);
    uavMask[0] = woUsedImagesMask;
    for (cxuint l = 0; l < config.args.size(); l++)
    {
        const AmdKernelArgInput& arg = config.args[l];
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
    
    putProgInfoEntryLE(bos, 0x8000001fU, uavMask[0]);
    for (cxuint p = 0; p < 32; p++)
        putProgInfoEntryLE(bos, 0x80001843U+p, uavMask[p]);
    putProgInfoEntryLE(bos, 0x8000000aU, 1);
    putProgInfoEntryLE(bos, 0x80000078U, 64);
    putProgInfoEntryLE(bos, 0x80000081U, 32768);
    putProgInfoEntryLE(bos, 0x80000082U, ((curPgmRSRC2>>15)&0x1ff)<<8);
    offset += progInfoSize;
    
    // CAL_SUBCONSTANT_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_SUB_CONSTANT_BUFFERS, 0);
    offset += sizeof(CALNoteHeader);
    
    // CAL_UAV_MAILBOX_SIZE
    putCALNoteLE(bos, CALNOTE_ATI_UAV_MAILBOX_SIZE, 4);
    bos.writeObject<uint32_t>(0);
    offset += sizeof(CALNoteHeader) + 4;
    
    // CAL_UAV_OP_MASK
    putCALNoteLE(bos, CALNOTE_ATI_UAV_OP_MASK, 128);
    for (cxuint k = 0; k < 32; k++)
        uavMask[k] = LEV(uavMask[k]);
    bos.writeArray(32, uavMask);
    offset += sizeof(CALNoteHeader) + 128;
}

static std::vector<cxuint> collectUniqueIdsAndFunctionIds(const AmdInput* input)
{
    std::vector<cxuint> uniqueIds;
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
            { uniqueIds.push_back(cstrtovCStyle<cxuint>(metadata+pos,
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
            { uniqueIds.push_back(cstrtovCStyle<cxuint>(funcIdStr,
                           metadata+kernel.metadataSize, outEnd)); }
            catch(const ParseException& ex)
            { } // ignore parse exception
        }
    std::sort(uniqueIds.begin(), uniqueIds.end());
    return uniqueIds;
}

/*
 * main routine to generate AmdBin for GPU
 */

void AmdGPUBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    uint64_t binarySize;
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
    if (input->deviceType > GPUDeviceType::GPUDEVICE_MAX)
        throw Exception("Unknown GPU device type");
    
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
    std::vector<cxuint> uniqueIds = collectUniqueIdsAndFunctionIds(input);
    
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        uint64_t innerBinSize = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr)*3 +
                sizeof(Elf32_Shdr)*6 + sizeof(CALEncodingEntry) + 2 + 16 + 40;
        
        const AmdKernelInput& kinput = input->kernels[i];
        
        innerBinSize += (kinput.data != nullptr) ? kinput.dataSize : 4736;
        innerBinSize += kinput.codeSize;
        size_t metadataSize = 0;
        uint64_t calNotesSize = 0;
        if (kinput.useConfig)
        {
            // get new free uniqueId
            while (std::binary_search(uniqueIds.begin(), uniqueIds.end(), uniqueId))
                    uniqueId++;
            
            const AmdKernelConfig& config = kinput.config;
            cxuint readOnlyImages = 0;
            cxuint writeOnlyImages = 0;
            cxuint uavsNum = 0;
            cxuint samplersNum = config.samplers.size();
            cxuint argSamplersNum = 0;
            cxuint constBuffersNum = 2 + (isOlderThan1348 /* cbid:2 for older drivers*/ &&
                    (input->globalData != nullptr));
            for (const AmdKernelArgInput& arg: config.args)
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
            
            tempAmdKernelConfigs[i].uavsNum = uavsNum;
            
            calNotesSize = uint64_t(20*17) /*calNoteHeaders*/ + 16 + 128 + (18+32 +
                4*((isOlderThan1124)?16:config.userDataElemsNum))*8 /* proginfo */ +
                    readOnlyImages*4 /* inputs */ + 16*uavsNum /* uavs */ +
                    8*samplersNum /* samplers */ + 8*constBuffersNum /* cbids */;
            innerBinSize += calNotesSize;
            
            kmetadatas[i] = generateMetadata(driverVersion, input, kinput,
                     tempAmdKernelConfigs[i], argSamplersNum, uniqueId);
            
            metadataSize = kmetadatas[i].size() + 32 /* header size */;
            uniqueId++;
        }
        else // if defined in calNotes (no config)
        {
            for (const CALNoteInput& calNote: kinput.calNotes)
                calNotesSize += uint64_t(20) + calNote.header.descSize;
            innerBinSize += calNotesSize;
            metadataSize = kinput.metadataSize + kinput.headerSize;
        }
        
        tempAmdKernelConfigs[i].calNotesSize = calNotesSize;
        
        if (innerBinSize > UINT32_MAX)
            throw Exception("Inner binary size is too big!");
        innerBinSizes[i] = innerBinSize;
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
    const size_t mainSectionsOffset = binarySize;
    if (!input->is64Bit)
        binarySize += sizeof(Elf32_Shdr)*(5 + (input->kernels.empty()?0:2) +
                (input->sourceCode != nullptr) + (input->llvmir != nullptr));
    else
        binarySize += sizeof(Elf64_Shdr)*(5 + (input->kernels.empty()?0:2) +
                (input->sourceCode != nullptr) + (input->llvmir != nullptr));
    
    if (
#ifdef HAVE_64BIT
        !input->is64Bit &&
#endif
        binarySize > UINT32_MAX)
        throw Exception("Binary size is too big!");
        
    /****
     * prepare for write binary to output
     ****/
    std::unique_ptr<std::ostream> outStreamHolder;
    std::ostream* os = nullptr;
    if (aPtr != nullptr)
    {
        aPtr->resize(binarySize);
        outStreamHolder.reset(
            new ArrayOStream(binarySize, reinterpret_cast<char*>(aPtr->data())));
        os = outStreamHolder.get();
    }
    else if (vPtr != nullptr)
    {
        vPtr->resize(binarySize);
        outStreamHolder.reset(new VectorOStream(*vPtr));
        os = outStreamHolder.get();
    }
    else // from argument
        os = osPtr;
    
    const std::ios::iostate oldExceptions = os->exceptions();
    size_t offset = 0;
    try
    {
    os->exceptions(std::ios::failbit | std::ios::badbit);
    /********
     * writing data
     *********/
    BinaryOStream bos(*os);
    
    const cxuint mainSectionsNum = ((input->kernels.empty())?5:7) +
            (input->sourceCode!=nullptr) + (input->llvmir!=nullptr);
    if (!input->is64Bit)
    {
        const Elf32_Ehdr mainHdr = { {
                0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2LSB, EV_CURRENT, 
                ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0 }, LEV(uint16_t(ET_EXEC)),
                LEV(uint16_t(gpuDeviceCodeTable[cxuint(input->deviceType)])),
                LEV(uint32_t(EV_CURRENT)), 0U, 0U, LEV(uint32_t(mainSectionsOffset)),
                0U, LEV(uint16_t(sizeof(Elf32_Ehdr))), 0U, 0U,
                LEV(uint16_t(sizeof(Elf32_Shdr))), LEV(uint16_t(mainSectionsNum)),
                LEV(uint16_t(1U)) };
        bos.writeObject(mainHdr);
        offset += sizeof(Elf32_Ehdr);
    }
    else
    {
        const Elf64_Ehdr mainHdr = { {
                0x7f, 'E', 'L', 'F', ELFCLASS64, ELFDATA2LSB, EV_CURRENT, 
                ELFOSABI_SYSV, 0, 0, 0, 0, 0, 0, 0, 0 }, LEV(uint16_t(ET_EXEC)),
                LEV(uint16_t(gpuDeviceCodeTable[cxuint(input->deviceType)])),
                LEV(uint32_t(EV_CURRENT)), 0U, 0U, LEV(uint64_t(mainSectionsOffset)),
                0U, LEV(uint16_t(sizeof(Elf64_Ehdr))), 0U, 0U,
                LEV(uint16_t(sizeof(Elf64_Shdr))), LEV(uint16_t(mainSectionsNum)),
                LEV(uint16_t(1U)) };
        bos.writeObject(mainHdr);
        offset += sizeof(Elf64_Ehdr);
    }
    size_t sectionOffsets[9];
    sectionOffsets[0] = offset;
    {
        char shstrtab[128];
        size_t shoffset = 0;
        if (!input->kernels.empty())
        {
            ::memcpy(shstrtab+shoffset,
                 "\000.shstrtab\000.strtab\000.symtab\000.rodata\000.text\000.comment", 50);
            shoffset += 50;
        }
        else
        {
            ::memcpy(shstrtab+shoffset,
                     "\000.shstrtab\000.strtab\000.symtab\000.comment", 36);
            shoffset += 36;
        }
        /* additional sections names */
        if (input->sourceCode!=nullptr)
        {
            ::memcpy(shstrtab+shoffset, ".source", 8);
            shoffset += 8;
        }
        if (input->llvmir!=nullptr)
        {
            ::memcpy(shstrtab+shoffset, ".llvmir", 8);
            shoffset += 8;
        }
        os->write(shstrtab, shoffset);
        offset += shoffset;
    }
    
    // .strtab
    sectionOffsets[1] = offset;
    if (!input->compileOptions.empty())
    {
        os->write("\000__OpenCL_compile_options", 26);
        offset += 26;
    }
    else // empty compile options
    {
        os->write("\000", 1);
        offset++;
    }
    
    if (input->globalData != nullptr)
    {
        if (!isOlderThan1348)
            os->write("__OpenCL_0_global", 18);
        else
            os->write("__OpenCL_2_global", 18);
        offset += 18;
    }
    for (const AmdKernelInput& kernel: input->kernels)
    {
        os->write("__OpenCL_", 9);
        offset += 9;
        os->write(kernel.kernelName.c_str(), kernel.kernelName.size());
        offset += kernel.kernelName.size();
        os->write("_metadata", 10);
        offset += 10;
        os->write("__OpenCL_", 9);
        offset += 9;
        os->write(kernel.kernelName.c_str(), kernel.kernelName.size());
        offset += kernel.kernelName.size();
        os->write("_kernel", 8);
        offset += 8;
        os->write("__OpenCL_", 9);
        offset += 9;
        os->write(kernel.kernelName.c_str(), kernel.kernelName.size());
        offset += kernel.kernelName.size();
        os->write("_header", 8);
        offset += 8;
    }
    
    const char zeroes[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    size_t alignFix = 0;
    if ((offset & 7) != 0)
    {
        alignFix = 8-(offset&7);
        os->write(zeroes, alignFix);
        offset += alignFix;
    }
    // .symtab
    sectionOffsets[2] = offset;
    if (!input->is64Bit)
        putMainSymbols<Elf32_Sym>(bos, offset, input, kmetadatas, innerBinSizes);
    else
        putMainSymbols<Elf64_Sym>(bos, offset, input, kmetadatas, innerBinSizes);
    // .rodata
    sectionOffsets[3] = offset;
    if (input->globalData != nullptr)
    {
        bos.writeArray(input->globalDataSize, input->globalData);
        offset += input->globalDataSize;
    }
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        const AmdKernelInput& kernel = input->kernels[i];
        if (kernel.useConfig)
        {
            const TempAmdKernelConfig& tempConfig = tempAmdKernelConfigs[i];
            os->write(kmetadatas[i].c_str(), kmetadatas[i].size());
            offset += kmetadatas[i].size();
            
            const uint32_t header[8] = {
                LEV((driverVersion >= 164205) ? tempConfig.uavPrivate : 0), 0U,
                LEV((isOlderThan1348)? 0 : tempConfig.uavPrivate),
                LEV(uint32_t(kernel.config.hwLocalSize)),
                LEV(uint32_t((input->is64Bit?8:0) | (kernel.config.usePrintf?2:0))),
                LEV(1U), 0U, 0U };
            bos.writeArray(8, header);
            offset += 32;
        }
        else
        {
            os->write(kernel.metadata, kernel.metadataSize);
            offset += kernel.metadataSize;
            bos.writeArray(kernel.headerSize, kernel.header);
            offset += kernel.headerSize;
        }
    }
    
    /* kernel binaries */
    sectionOffsets[4] = offset;
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        const size_t innerBinOffset = offset;
        const AmdKernelInput& kinput = input->kernels[i];
        
        const Elf32_Ehdr innerHdr = { {
                0x7f, 'E', 'L', 'F', ELFCLASS32, ELFDATA2LSB, EV_CURRENT, 
                0x64, 1, 0, 0, 0, 0, 0, 0, 0 }, LEV(uint16_t(ET_EXEC)), 
                LEV(uint16_t(0x7dU)), LEV(uint32_t(EV_CURRENT)), 0U,
                LEV(uint32_t(sizeof(Elf32_Ehdr))), LEV(0xd0U), LEV(1U),
                LEV(uint16_t(sizeof(Elf32_Ehdr))),
                LEV(uint16_t(sizeof(Elf32_Phdr))), LEV(uint16_t(3U)),
                LEV(uint16_t(sizeof(Elf32_Shdr))), LEV(uint16_t(6U)),
                LEV(uint16_t(1U)) };
        bos.writeObject(innerHdr);
        offset += sizeof(Elf32_Ehdr);
        
        /* ??? */
        offset += sizeof(Elf32_Phdr)*3;
        size_t sectionOffset = offset + sizeof(CALEncodingEntry) + 40 +
                sizeof(Elf32_Shdr)*6 + tempAmdKernelConfigs[i].calNotesSize -
                innerBinOffset;
        putElfProgramHeader32LE(bos, 0x70000002U, 0x94, sizeof(CALEncodingEntry), 0,
                0, 0, 0, 0);
        putElfProgramHeader32LE(bos, PT_NOTE, 0x1c0, sectionOffset-0x1c0, 0, 0);
        putElfProgramHeader32LE(bos, PT_LOAD, sectionOffset,
                    innerBinSizes[i]-sectionOffset, 0, 0, innerBinSizes[i]-sectionOffset);
        
        const size_t encOffset = offset + sizeof(CALEncodingEntry) + 40 +
                sizeof(Elf32_Shdr)*6;
        {   /* CALEncodingEntry */
            const CALEncodingEntry encEntry = {
                LEV(uint32_t(gpuDeviceInnerCodeTable[cxuint(input->deviceType)])), LEV(4U), 
                LEV(0x1c0U), LEV(uint32_t(innerBinSizes[i]-(encOffset-innerBinOffset))) };
            bos.writeObject(encEntry);
            offset += sizeof(CALEncodingEntry);
        }
        os->write("\000.shstrtab\000.text\000.data\000.symtab\000.strtab\000", 40);
        offset += 40;
        /* sections */
        putElfSectionLE<Elf32_Shdr>(bos, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        /// shstrtab
        putElfSectionLE<Elf32_Shdr>(bos, 1, SHT_STRTAB, 0, 0xa8, 40, 0, 0, 0);
        // .text
        putElfSectionLE<Elf32_Shdr>(bos, 11, SHT_PROGBITS, 0, sectionOffset,
                        kinput.codeSize, 0, 0, 0);
        sectionOffset += kinput.codeSize;
        const size_t dataSize = (kinput.data!=nullptr)?kinput.dataSize:4736;
        // .data
        putElfSectionLE<Elf32_Shdr>(bos, 17, SHT_PROGBITS, 0, sectionOffset,
                        dataSize, 0, 0, 0, 0, dataSize);
        sectionOffset += dataSize;
        // .symtab
        putElfSectionLE<Elf32_Shdr>(bos, 23, SHT_SYMTAB, 0, sectionOffset,
                        sizeof(Elf32_Sym), 5, 1, 0, 0, sizeof(Elf32_Sym));
        sectionOffset += sizeof(Elf32_Sym);
        // .strtab
        putElfSectionLE<Elf32_Shdr>(bos, 31, SHT_STRTAB, 0, sectionOffset, 2, 0, 0, 0);
        
        // CALnotes
        offset += sizeof(Elf32_Shdr)*6;
        if (kinput.useConfig)
            generateCALNotes(bos, offset, input, driverVersion, kinput,
                     tempAmdKernelConfigs[i]);
        else // from CALNotes array
            for (const CALNoteInput& calNote: kinput.calNotes)
            {   // all fields of CALNote
                CALNoteHeader cnHdr = { LEV(calNote.header.nameSize),
                    LEV(calNote.header.descSize), LEV(calNote.header.type) };
                ::memcpy(cnHdr.name, &calNote.header.name, 8);
                bos.writeObject(cnHdr);
                bos.writeArray(calNote.header.descSize, calNote.data);
                offset += sizeof(CALNoteHeader) + calNote.header.descSize;
            }
        // .text
        bos.writeArray(kinput.codeSize, kinput.code);
        offset += kinput.codeSize;
        // .data
        if (kinput.data != nullptr)
        {
            bos.writeArray(kinput.dataSize, kinput.data);
            offset += kinput.dataSize;
        }
        else
        {
            char fillup[64];
            std::fill(fillup, fillup+64, char(0));
            for (cxuint written = 0; written < 4736; written += 64)
                os->write(fillup, 64);
            offset += 4736;
        }
        // .symtab
        putElfSymbolLE<Elf32_Sym>(bos, 0, 0, 0, 0, 0, 0);
        offset += sizeof(Elf32_Sym);
        // .strtab
        bos.writeObject<uint16_t>(0);
        offset += 2;
    }
    // .comment
    sectionOffsets[5] = offset;
    os->write(input->compileOptions.c_str(), input->compileOptions.size());
    offset += input->compileOptions.size();
    os->write(driverInfo.c_str(), driverInfo.size());
    offset += driverInfo.size();
    sectionOffsets[6] = offset;
    // .source
    if (input->sourceCode!=nullptr)
    {
        os->write(input->sourceCode, sourceCodeSize);
        offset += sourceCodeSize;
    }
    sectionOffsets[7] = offset;
    // .source
    if (input->llvmir!=nullptr)
    {
        bos.writeArray(input->llvmirSize, input->llvmir);
        offset += input->llvmirSize;
    }
    sectionOffsets[8] = offset;
    if ((offset & (mainSectionsAlign-1)) != 0) // main section alignment
    {
        const size_t tofill = mainSectionsAlign - (offset & (mainSectionsAlign-1));
        os->write(zeroes, tofill);
        offset += tofill;
    }
    /* main sections */
    if (!input->is64Bit)
        putMainSections<Elf32_Shdr, Elf32_Sym>(bos, offset, sectionOffsets, alignFix,
                input->kernels.empty(), input->sourceCode!=nullptr, input->llvmir!=nullptr);
    else
        putMainSections<Elf64_Shdr, Elf64_Sym>(bos, offset, sectionOffsets, alignFix,
                input->kernels.empty(), input->sourceCode!=nullptr, input->llvmir!=nullptr);
    os->flush();
    }
    catch(...)
    {
        os->exceptions(oldExceptions);
        throw;
    }
    os->exceptions(oldExceptions);
    assert(offset == binarySize);
}


void AmdGPUBinGenerator::generate(Array<cxbyte>& array) const
{
    generateInternal(nullptr, nullptr, &array);
}

void AmdGPUBinGenerator::generate(std::ostream& os) const
{
    generateInternal(&os, nullptr, nullptr);
}

void AmdGPUBinGenerator::generate(std::vector<char>& vector) const
{
    generateInternal(nullptr, &vector, nullptr);
}
