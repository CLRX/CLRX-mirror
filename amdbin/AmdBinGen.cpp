/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
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
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <bitset>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/amdbin/AmdBinGen.h>

using namespace CLRX;

// helper to writing specific CALNote entries
static inline void putCALNoteLE(FastOutputBuffer& bos, uint32_t type, uint32_t descSize)
{
    const CALNoteHeader nhdr = { LEV(8U), LEV(descSize), LEV(type),
        { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } };
    bos.writeObject(nhdr);
}

static inline void putCALUavEntryLE(FastOutputBuffer& bos, uint32_t uavId, uint32_t f1,
          uint32_t f2, uint32_t type)
{
    const CALUAVEntry uavEntry = { LEV(uavId), LEV(f1), LEV(f2), LEV(type) };
    bos.writeObject(uavEntry);
}

static inline void putCBMaskLE(FastOutputBuffer& bos,
         uint32_t index, uint32_t size)
{
    const CALConstantBufferMask cbMask = { LEV(index), LEV(size) };
    bos.writeObject(cbMask);
}

static inline void putSamplerEntryLE(FastOutputBuffer& bos,
         uint32_t input, uint32_t sampler)
{
    const CALSamplerMapEntry samplerEntry = { LEV(input), LEV(sampler) };
    bos.writeObject(samplerEntry);
}

static inline void putProgInfoEntryLE(FastOutputBuffer& bos,
          uint32_t address, uint32_t value)
{
    const CALProgramInfoEntry piEntry = { LEV(address), LEV(value) };
    bos.writeObject(piEntry);
}

// e_type (16-bit)
static const uint16_t gpuDeviceCodeTable[28] =
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
    0x40b, // GPUDeviceType::MULLINS
    0x40c, // GPUDeviceType::FIJI
    0x40d, // GPUDeviceType::CARRIZO
    0x411, // GPUDeviceType::DUMMY
    0x40f, // GPUDeviceType::GOOSE
    0x40e, // GPUDeviceType::HORSE
    0x411, // GPUDeviceType::STONEY
    0x40e, // GPUDeviceType::ELLESMERE
    0x40f, // GPUDeviceType::BAFFIN
    0x412, // GPUDeviceType::GFX804
    0xffff, // GPUDeviceType::GFX900
    0xffff, // GPUDeviceType::GFX901
    0xffff, // GPUDeviceType::GFX902
    0xffff, // GPUDeviceType::GFX903
    0xffff, // GPUDeviceType::GFX904
    0xffff  // GPUDeviceType::GFX905
};

/// CALNoteEntry (32-bit)
static const uint32_t gpuDeviceInnerCodeTable[28] =
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
    0x2b, // GPUDeviceType::MULLINS
    0x2d, // GPUDeviceType::FIJI
    0x2e, // GPUDeviceType::CARRIZO
    0x31, // GPUDeviceType::DUMMY
    0x2c, // GPUDeviceType::GOOSE
    0x2f, // GPUDeviceType::HORSE
    0x31, // GPUDeviceType::STONEY
    0x2f, // GPUDeviceType::ELLESMERE
    0x2c, // GPUDeviceType::BAFFIN
    0x32, // GPUDeviceType::GFX804
    UINT_MAX, // GPUDeviceType::GFX900
    UINT_MAX, // GPUDeviceType::GFX901
    UINT_MAX, // GPUDeviceType::GFX902
    UINT_MAX, // GPUDeviceType::GFX903
    UINT_MAX, // GPUDeviceType::GFX904
    UINT_MAX  // GPUDeviceType::GFX905
};

void AmdInput::addKernel(const AmdKernelInput& kernelInput)
{
    kernels.push_back(kernelInput);
}

void AmdInput::addKernel(AmdKernelInput&& kernelInput)
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

void AmdInput::addEmptyKernel(const char* kernelName)
{
    AmdKernelInput kernel{};
    kernel.kernelName = kernelName;
    kernel.config.floatMode = 0xc0;
    kernel.config.dimMask = BINGEN_DEFAULT;
    kernel.config.usedSGPRsNum = kernel.config.usedVGPRsNum = BINGEN_DEFAULT;
    kernel.config.hwRegion = BINGEN_DEFAULT;
    kernel.config.uavId = kernel.config.privateId = kernel.config.printfId =
        kernel.config.uavPrivate = kernel.config.constBufferId = BINGEN_DEFAULT;
    kernels.push_back(std::move(kernel));
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
    std::unique_ptr<AmdInput> _input(new AmdInput{});
    _input->is64Bit = _64bitMode;
    _input->deviceType = deviceType;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->driverVersion = driverVersion;
    _input->compileOptions = "";
    _input->driverInfo = "";
    _input->kernels = kernelInputs;
    input = _input.release();
}

AmdGPUBinGenerator::AmdGPUBinGenerator(bool _64bitMode, GPUDeviceType deviceType,
       uint32_t driverVersion, size_t globalDataSize, const cxbyte* globalData,
       std::vector<AmdKernelInput>&& kernelInputs)
        : manageable(true), input(nullptr)
{
    std::unique_ptr<AmdInput> _input(new AmdInput{});
    _input->is64Bit = _64bitMode;
    _input->deviceType = deviceType;
    _input->globalDataSize = globalDataSize;
    _input->globalData = globalData;
    _input->driverVersion = driverVersion;
    _input->compileOptions = "";
    _input->driverInfo = "";
    _input->kernels = std::move(kernelInputs);
    input = _input.release();
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
    KT_STRUCT,  // structure
    KT_UNKNOWN  // used by types other than number types
};

struct CLRX_INTERNAL TypeNameVecSize
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

/// temporary kernel configuration. holds resolved resource ids values and other setup
struct CLRX_INTERNAL TempAmdKernelConfig
{
    uint32_t hwRegion;
    uint32_t uavPrivate;
    uint32_t uavId;
    uint32_t constBufferId;
    uint32_t printfId;
    uint32_t privateId;
    uint32_t argsSpace;
    Array<uint16_t> argResIds;
    uint32_t uavsNum;
    uint32_t calNotesSize;
};


static const uint16_t mainBuiltinSectionTable[] =
{
    1, // ELFSECTID_SHSTRTAB
    2, // ELFSECTID_STRTAB
    3, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    5, // ELFSECTID_TEXT
    4, // ELFSECTID_RODATA
    SHN_UNDEF, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    6 // ELFSECTID_COMMENT
};

static const uint16_t emptyMainBuiltinSectionTable[] =
{
    1, // ELFSECTID_SHSTRTAB
    2, // ELFSECTID_STRTAB
    3, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    SHN_UNDEF, // ELFSECTID_TEXT
    SHN_UNDEF, // ELFSECTID_RODATA
    SHN_UNDEF, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    4 // ELFSECTID_COMMENT
};

static const uint16_t kernelBuiltinSectionTable[] =
{
    1, // ELFSECTID_SHSTRTAB
    5, // ELFSECTID_STRTAB
    4, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    2, // ELFSECTID_TEXT
    SHN_UNDEF, // ELFSECTID_RODATA
    3, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    SHN_UNDEF // ELFSECTID_COMMENT
};

static void generateCALNotes(FastOutputBuffer& bos, const AmdInput* input,
         cxuint driverVersion, const AmdKernelInput& kernel,
         const TempAmdKernelConfig& tempConfig);

class CLRX_INTERNAL CALNoteGen: public ElfRegionContent
{
private:
    const AmdInput* input;
    cxuint driverVersion;
    const AmdKernelInput* kernel;
    const TempAmdKernelConfig* tempConfig;
public:
    CALNoteGen() { }
    CALNoteGen(const AmdInput* _input, cxuint _driverVersion,
           const AmdKernelInput* _kernel, const TempAmdKernelConfig* _tempConfig)
        : input(_input), driverVersion(_driverVersion), kernel(_kernel),
          tempConfig(_tempConfig)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        if (kernel->useConfig)
            generateCALNotes(fob, input, driverVersion, *kernel, *tempConfig);
        else
            for (const CALNoteInput& calNote: kernel->calNotes)
            {
                // all fields of CALNote
                CALNoteHeader cnHdr = { LEV(calNote.header.nameSize),
                    LEV(calNote.header.descSize), LEV(calNote.header.type) };
                ::memcpy(cnHdr.name, &calNote.header.name, 8);
                fob.writeObject(cnHdr);
                fob.writeArray(calNote.header.descSize, calNote.data);
            }
    }
};

class CLRX_INTERNAL KernelDataGen: public ElfRegionContent
{
private:
    const AmdKernelInput* kernel;
public:
    KernelDataGen() { }
    KernelDataGen(const AmdKernelInput* _kernel) : kernel(_kernel)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        if (kernel->data!=nullptr)
            fob.writeArray(kernel->dataSize, kernel->data);
        else
            fob.fill(4736, 0);
    }
};

struct CLRX_INTERNAL TempAmdKernelData
{
    uint32_t innerBinSize;
    std::string metadata;
    CALNoteGen calNoteGen;
    KernelDataGen kernelDataGen;
    ElfBinaryGen32 elfBinGen; // for kernel
    CALEncodingEntry calEncEntry;
    uint32_t header[8];
};

// fast and memory efficient String table generator for main binary
class CLRX_INTERNAL CL1MainStrTabGen: public ElfRegionContent
{
private:
    cxuint driverVersion;
    const AmdInput* input;
public:
    CL1MainStrTabGen(cxuint _driverVersion, const AmdInput* _input)
            : driverVersion(_driverVersion), input(_input)
    { }
    
    // calculating size of strtab 
    size_t size() const
    {
        size_t size = 1;
        if (!input->compileOptions.empty())
            size += 25;
        if (input->globalData != nullptr)
            size += 18; // "_OpenCL_X_global"
        
        // storing three symbols with various names
        for (const AmdKernelInput& kernel: input->kernels)
            size += kernel.kernelName.size()*3 + 19 + 17 + 17;
        // extra symbol's strings sizes
        for (const BinSymbol& symbol: input->extraSymbols)
            size += symbol.name.size()+1;
        return size;
    }
    
    // fast way to write symbol name strings
    void operator()(FastOutputBuffer& fob) const
    {
        const bool isOlderThan1348 = driverVersion < 134805;
        fob.put(0);
        if (!input->compileOptions.empty())
            fob.write(25, "__OpenCL_compile_options");
        if (input->globalData != nullptr)
            fob.write(18, (!isOlderThan1348)?"__OpenCL_0_global":"__OpenCL_2_global");
        
        for (const AmdKernelInput& kernel: input->kernels)
        {
            fob.write(9, "__OpenCL_");
            fob.write(kernel.kernelName.size(), kernel.kernelName.c_str());
            fob.write(19, "_metadata\000__OpenCL_");
            fob.write(kernel.kernelName.size(), kernel.kernelName.c_str());
            fob.write(17, "_kernel\000__OpenCL_");
            fob.write(kernel.kernelName.size(), kernel.kernelName.c_str());
            fob.write(8, "_header");
        }
        for (const BinSymbol& symbol: input->extraSymbols)
            fob.write(symbol.name.size()+1, symbol.name.c_str());
    }
};

// fast and memory efficient symbol table generator for main binary
template<typename Types>
class CLRX_INTERNAL CL1MainSymTabGen: public ElfRegionContent
{
private:
    cxuint driverVersion;
    const AmdInput* input;
    const Array<TempAmdKernelData>& tempDatas;
public:
    CL1MainSymTabGen(cxuint _driverVersion, const AmdInput* _input,
                  const Array<TempAmdKernelData>& _tempDatas)
            : driverVersion(_driverVersion), input(_input), tempDatas(_tempDatas)
    { }
    
    size_t size() const
    {
        return sizeof(typename Types::Sym)*(1 + (!input->compileOptions.empty()) +
                (input->globalData != nullptr) + 3*input->kernels.size() +
                input->extraSymbols.size());
    }
    
    // fast way to store symbol table
    void operator()(FastOutputBuffer& fob) const
    {
        const uint16_t* mainSectTable = input->kernels.empty() ?
                emptyMainBuiltinSectionTable : mainBuiltinSectionTable;
        const ElfBinSectId extraSectId = input->kernels.empty() ? 5 : 7;
        
        fob.fill(sizeof(typename Types::Sym), 0);
        typename Types::Sym sym;
        size_t nameOffset = 1;
        // compile options symbol
        if (!input->compileOptions.empty())
        {
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, uint16_t((input->kernels.size()!=0)?6:4));
            SLEV(sym.st_value, 0);
            SLEV(sym.st_size, input->compileOptions.size());
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameOffset += 25;
            fob.writeObject(sym);
        }
        size_t rodataPos = 0;
        // global data symbol
        if (input->globalData != nullptr)
        {
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, 4);
            SLEV(sym.st_value, 0);
            SLEV(sym.st_size, input->globalDataSize);
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameOffset += 18;
            fob.writeObject(sym);
            rodataPos += input->globalDataSize;
        }
        
        size_t textPos = 0;
        for (size_t i = 0; i < input->kernels.size(); i++)
        {
            const AmdKernelInput& kernel = input->kernels[i];
            // metadata
            const size_t metadataSize = (kernel.useConfig) ?
                    tempDatas[i].metadata.size() : kernel.metadataSize;
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, 4);
            SLEV(sym.st_size, metadataSize);
            SLEV(sym.st_value, rodataPos);
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            fob.writeObject(sym);
            nameOffset += kernel.kernelName.size() + 19;
            rodataPos += metadataSize;
            // kernel
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, 5);
            SLEV(sym.st_size, tempDatas[i].innerBinSize);
            SLEV(sym.st_value, textPos);
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_FUNC);
            sym.st_other = 0;
            fob.writeObject(sym);
            nameOffset += kernel.kernelName.size() + 17;
            textPos += tempDatas[i].innerBinSize;
            // kernel
            const size_t headerSize = (kernel.useConfig) ? 32 : kernel.headerSize;
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, 4);
            SLEV(sym.st_size, headerSize);
            SLEV(sym.st_value, rodataPos);
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            fob.writeObject(sym);
            nameOffset += kernel.kernelName.size() + 17;
            rodataPos += headerSize;
        }
        // after this we write extra symbols
        for (const BinSymbol& symbol: input->extraSymbols)
        {
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, convertSectionId(symbol.sectionId, mainSectTable,
                            ELFSECTID_STD_MAX, extraSectId));
            SLEV(sym.st_size, symbol.size);
            SLEV(sym.st_value, symbol.value);
            sym.st_info = symbol.info;
            sym.st_other = symbol.other;
            nameOffset += symbol.name.size()+1;
            fob.writeObject(sym);
        }
    }
};

// write rodata that hold kernel's metadatas and kernel's headers
class CLRX_INTERNAL CL1MainRoDataGen: public ElfRegionContent
{
private:
    const AmdInput* input;
    const Array<TempAmdKernelData>& tempDatas;
public:
    CL1MainRoDataGen(const AmdInput* _input, const Array<TempAmdKernelData>& _tempDatas)
            : input(_input), tempDatas(_tempDatas)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        if (input->globalData != nullptr)
            fob.writeArray(input->globalDataSize, input->globalData);
        for (size_t i = 0; i < input->kernels.size(); i++)
        {
            const AmdKernelInput& kernel = input->kernels[i];
            if (kernel.useConfig)
            {
                fob.write(tempDatas[i].metadata.size(), tempDatas[i].metadata.c_str());
                fob.writeArray(8, tempDatas[i].header);
            }
            else
            {
                fob.write(kernel.metadataSize, kernel.metadata);
                fob.writeArray(kernel.headerSize, kernel.header);
            }
        }
    }
};

class CLRX_INTERNAL CL1MainTextGen: public ElfRegionContent
{
private:
    Array<TempAmdKernelData>& tempDatas;
public:
    CL1MainTextGen(Array<TempAmdKernelData>& _tempDatas) : tempDatas(_tempDatas)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        for (TempAmdKernelData& kernel: tempDatas)
            kernel.elfBinGen.generate(fob);
    }
};

// writing comment (holds compile options and driver info)
class CLRX_INTERNAL CL1MainCommentGen: public ElfRegionContent
{
private:
    const AmdInput* input;
    const CString& driverInfo;
public:
    CL1MainCommentGen(const AmdInput* _input, const CString& _driverInfo)
            : input(_input), driverInfo(_driverInfo)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        fob.write(input->compileOptions.size(), input->compileOptions.c_str());
        fob.write(driverInfo.size(), driverInfo.c_str());
    }
};

template<typename Types>
static void putMainSections(ElfBinaryGenTemplate<Types>& elfBinGen, cxuint driverVersion,
        const AmdInput* input, size_t allInnerBinSize, const CL1MainTextGen& textGen,
        size_t rodataSize, const CL1MainRoDataGen& rodataGen, const CString& driverInfo,
        const CL1MainCommentGen& commentGen, const CL1MainStrTabGen& mainStrGen,
        const CL1MainSymTabGen<Types>& mainSymGen)
{
    const uint16_t* mainSectTable = input->kernels.empty() ?
            emptyMainBuiltinSectionTable : mainBuiltinSectionTable;
    const cxuint extraSectId = input->kernels.empty() ? 5 : 7;
    
    elfBinGen.addRegion(ElfRegionTemplate<Types>(0, (const cxbyte*)nullptr, 1,
                 ".shstrtab", SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegionTemplate<Types>(mainStrGen.size(), &mainStrGen, 1,
                 ".strtab", SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegionTemplate<Types>(mainSymGen.size(), &mainSymGen, 8,
                 ".symtab", SHT_SYMTAB, 0));
    if (!input->kernels.empty())
    {
        // rodata and text
        elfBinGen.addRegion(ElfRegionTemplate<Types>(rodataSize, &rodataGen, 1,
                 ".rodata", SHT_PROGBITS, SHF_ALLOC));
        elfBinGen.addRegion(ElfRegionTemplate<Types>(allInnerBinSize, &textGen, 1,
                ".text", SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR));
    }
    // comment
    elfBinGen.addRegion(ElfRegionTemplate<Types>(
                input->compileOptions.size()+driverInfo.size(), &commentGen, 1,
                ".comment", SHT_PROGBITS, 0));
    for (const BinSection& section: input->extraSections)
        elfBinGen.addRegion(ElfRegionTemplate<Types>(section, mainSectTable,
                         ELFSECTID_STD_MAX, extraSectId));
    // store section header table as last region
    elfBinGen.addRegion(ElfRegionTemplate<Types>::sectionHeaderTable());
}

/* perform some checkings for resource ids and fill resource ids and other that was set
 * as default. store that setup in TempAndKernelConfig */
static void prepareTempConfigs(cxuint driverVersion, const AmdInput* input,
       Array<TempAmdKernelConfig>& tempAmdKernelConfigs)
{
    const bool isOlderThan1348 = driverVersion < 134805;
    const bool isOlderThan1598 = driverVersion < 159805;
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
    cxuint maxSGPRSNum = getGPUMaxRegistersNum(arch, REGTYPE_SGPR, REGCOUNT_NO_VCC);
    cxuint maxVGPRSNum = getGPUMaxRegistersNum(arch, REGTYPE_VGPR, 0);
    
    for (size_t i = 0; i < input->kernels.size(); i++)
    {
        const AmdKernelInput& kinput = input->kernels[i];
        if (!kinput.useConfig)
        {
            if (kinput.metadata == nullptr || kinput.metadataSize == 0)
                throw BinGenException("No metadata for kernel");
            if (kinput.header == nullptr || kinput.headerSize == 0)
                throw BinGenException("No header for kernel");
            if (kinput.code == nullptr)
                throw BinGenException("No code for kernel");
            continue; // and skip
        }
        // checking config parameters
        const AmdKernelConfig& config = kinput.config;
        TempAmdKernelConfig& tempConfig = tempAmdKernelConfigs[i];
        if (config.userDatas.size() > 16)
            throw BinGenException("UserDataElemsNum must not be greater than 16");
        if (config.usedVGPRsNum > maxVGPRSNum)
            throw BinGenException("Used VGPRs number out of range");
        if (config.usedSGPRsNum > maxSGPRSNum)
            throw BinGenException("Used SGPRs number out of range");
        if (config.hwLocalSize > 32768)
            throw BinGenException("HWLocalSize out of range");
        if (config.floatMode >= 256)
            throw BinGenException("FloatMode out of range");
        
        /* filling input */
        if (config.hwRegion == BINGEN_DEFAULT)
            tempConfig.hwRegion = 0;
        else
            tempConfig.hwRegion = config.hwRegion;
        
        /* checking arg types */
        for (const AmdKernelArgInput& arg: config.args)
            if (arg.argType > KernelArgType::MAX_VALUE)
                throw BinGenException("Unknown argument type");
            else if (arg.argType == KernelArgType::POINTER)
            {
                if (arg.pointerType > KernelArgType::MAX_VALUE)
                    throw BinGenException("Unknown argument's pointer type");
                if (arg.ptrSpace > KernelPtrSpace::MAX_VALUE ||
                    arg.ptrSpace == KernelPtrSpace::NONE)
                    throw BinGenException("Wrong pointer space type");
            }
            else if (isKernelArgImage(arg.argType))
            {
                if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == 0)
                    throw BinGenException("Invalid access qualifier for image");
            }
        
        if (config.uavPrivate == BINGEN_DEFAULT)
        {
            /* compute uavPrivate */
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
        
        if (config.uavId == BINGEN_DEFAULT)
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
                
                tempConfig.uavId = (hasPointer || config.usePrintf)?11:BINGEN_NOTSUPPLIED;
            }
        }
        else
            tempConfig.uavId = config.uavId;
        
        if (config.constBufferId == BINGEN_DEFAULT)
            tempConfig.constBufferId = (isOlderThan1348)?BINGEN_NOTSUPPLIED : 10;
        else
            tempConfig.constBufferId = config.constBufferId;
        
        if (config.printfId == BINGEN_DEFAULT)
            tempConfig.printfId = (!isOlderThan1348 &&
                (driverVersion >= 152603 || config.usePrintf)) ? 9 : BINGEN_NOTSUPPLIED;
        else
            tempConfig.printfId = config.printfId;
        
        if (config.privateId == BINGEN_DEFAULT)
            tempConfig.privateId = 8;
        else
            tempConfig.privateId = config.privateId;
        
        if (tempConfig.uavId != BINGEN_NOTSUPPLIED && tempConfig.uavId >= 1024)
            throw BinGenException("UavId out of range");
        if (tempConfig.constBufferId != BINGEN_NOTSUPPLIED &&
            tempConfig.constBufferId >= 1024)
            throw BinGenException("ConstBufferId out of range");
        if (tempConfig.printfId != BINGEN_NOTSUPPLIED && tempConfig.printfId >= 1024)
            throw BinGenException("PrintfId out of range");
        if (tempConfig.privateId != BINGEN_NOTSUPPLIED && tempConfig.privateId >= 1024)
            throw BinGenException("PrivateId out of range");
        
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
        tempConfig.argResIds.resize(config.args.size());
        // main loop to fill argUavIds
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                (arg.ptrSpace == KernelPtrSpace::GLOBAL ||
                 (arg.ptrSpace == KernelPtrSpace::CONSTANT && !isOlderThan1348)) &&
               arg.resId != BINGEN_DEFAULT)
            {
                if ((arg.resId < 9 && arg.used) ||
                    (!arg.used && arg.resId != tempConfig.uavId) || arg.resId >= 1024)
                    throw BinGenException("UavId out of range!");
                if (puavMask[arg.resId] && arg.resId != tempConfig.uavId)
                    throw BinGenException("UavId already used!");
                puavMask.set(arg.resId);
                tempConfig.argResIds[k] = arg.resId;
            }
            else if (arg.argType == KernelArgType::POINTER &&
                    arg.ptrSpace == KernelPtrSpace::CONSTANT && arg.resId != BINGEN_DEFAULT)
            {
                // old constant buffers
                if (arg.resId < 2 || arg.resId >= 160)
                    throw BinGenException("CbId out of range!");
                if (cbIdMask[arg.resId])
                    throw BinGenException("CbId already used!");
                cbIdMask.set(arg.resId);
                tempConfig.argResIds[k] = arg.resId;
            }
            else if (isKernelArgImage(arg.argType) && arg.resId != BINGEN_DEFAULT)
            {
                // images
                if (arg.ptrAccess & KARG_PTR_READ_ONLY)
                {
                    if (arg.resId >= 128)
                        throw BinGenException("RdImgId out of range!");
                    if (rdImgMask[arg.resId])
                        throw BinGenException("RdImgId already used!");
                    rdImgMask.set(arg.resId);
                    tempConfig.argResIds[k] = arg.resId;
                }
                else if (arg.ptrAccess & KARG_PTR_WRITE_ONLY)
                {
                    if (arg.resId >= 8)
                        throw BinGenException("WrImgId out of range!");
                    if (wrImgMask[arg.resId])
                        throw BinGenException("WrImgId already used!");
                    wrImgMask.set(arg.resId);
                    tempConfig.argResIds[k] = arg.resId;
                }
            }
            else if (arg.argType == KernelArgType::COUNTER32 && arg.resId != BINGEN_DEFAULT)
            {
                if (arg.resId >= 8)
                    throw BinGenException("CounterId out of range!");
                if (cntIdMask[arg.resId])
                    throw BinGenException("CounterId already used!");
                cntIdMask.set(arg.resId);
                tempConfig.argResIds[k] = arg.resId;
            }
        }
        
        // setting argUavIds to arguments in (tempConfig.argResIds)
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (arg.argType == KernelArgType::POINTER &&
                (arg.ptrSpace == KernelPtrSpace::GLOBAL ||
                 (arg.ptrSpace == KernelPtrSpace::CONSTANT && !isOlderThan1348)) &&
                arg.resId == BINGEN_DEFAULT)
            {
                if (arg.used)
                {
                    for (; puavIdsCount < 1024 && puavMask[puavIdsCount];
                         puavIdsCount++);
                    if (puavIdsCount == 1024)
                        throw BinGenException("UavId out of range!");
                    tempConfig.argResIds[k] = puavIdsCount++;
                }
                else // use unused uavId (9 or 11)
                    tempConfig.argResIds[k] = tempConfig.uavId;
            }
            else if (arg.argType == KernelArgType::POINTER &&
                    arg.ptrSpace == KernelPtrSpace::CONSTANT && arg.resId == BINGEN_DEFAULT)
            {
                // old constant buffers
                for (; cbIdsCount < 160 && cbIdMask[cbIdsCount]; cbIdsCount++);
                if (cbIdsCount == 160)
                    throw BinGenException("CbId out of range!");
                tempConfig.argResIds[k] = cbIdsCount++;
            }
            else if (isKernelArgImage(arg.argType) && arg.resId == BINGEN_DEFAULT)
            {
                // images
                if (arg.ptrAccess & KARG_PTR_READ_ONLY)
                {
                    for (; rdImgsCount < 128 && rdImgMask[rdImgsCount]; rdImgsCount++);
                    if (rdImgsCount == 128)
                        throw BinGenException("RdImgId out of range!");
                    tempConfig.argResIds[k] = rdImgsCount++;
                }
                else if (arg.ptrAccess & KARG_PTR_WRITE_ONLY)
                {
                    for (; wrImgsCount < 8 && wrImgMask[wrImgsCount]; wrImgsCount++);
                    if (wrImgsCount == 8)
                        throw BinGenException("WrImgId out of range!");
                    tempConfig.argResIds[k] = wrImgsCount++;
                }
            }
            else if (arg.argType == KernelArgType::COUNTER32 && arg.resId == BINGEN_DEFAULT)
            {
                for (; cntIdsCount < 8 && cntIdMask[cntIdsCount]; cntIdsCount++);
                if (cntIdsCount == 8)
                    throw BinGenException("CounterId out of range!");
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
    metadata += kinput.kernelName.c_str();
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
    /* reqd_work_group_size for kernel (cws) */
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
    /* put kernel arg info to metadata */
    for (cxuint k = 0; k < config.args.size(); k++)
    {
        const AmdKernelArgInput& arg = config.args[k];
        if (arg.argType == KernelArgType::STRUCTURE)
        {
            // store in form: ";value:Name:struct:SIZE:1:OFFSET
            metadata += ";value:";
            metadata += arg.argName.c_str();
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
            // pointer
            /* store: ';pointer:NAME::TYPENAME:1:1:OFFSET:
             *           QUAL:RESID:ESIZE:ACCESS:VOLATILE:RESTRICT' */
            metadata += ";pointer:";
            metadata += arg.argName.c_str();
            metadata += ':';
            const TypeNameVecSize& tp = argTypeNamesTable[cxuint(arg.pointerType)];
            if (tp.kindOfType == KT_UNKNOWN)
                throw BinGenException("Type not supported!");
            const cxuint typeSize =
                cxuint((tp.vecSize==3) ? 4 : tp.vecSize)*tp.elemSize;
            if (arg.structSize == 0 && arg.pointerType == KernelArgType::STRUCTURE)
                metadata += "opaque"; // opaque indicates pointer to structure
            else
                metadata += tp.name;
            metadata += ":1:1:";
            itocstrCStyle(argOffset, numBuf, 21);
            metadata += numBuf;
            metadata += ':';
            if (arg.ptrSpace == KernelPtrSpace::LOCAL)
            {
                metadata += "hl:";
                itocstrCStyle((arg.resId!=BINGEN_DEFAULT)?arg.resId:1, numBuf, 21);
                metadata += numBuf;
            }
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
                throw BinGenException("Other memory spaces are not supported");
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
        else if (isKernelArgImage(arg.argType))
        {
            // image
            // store ';image:NAME:IMGTYPE:ACCESS:RESID:1:ARGOFFSET'
            metadata += ";image:";
            metadata += arg.argName.c_str();
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
                throw BinGenException("Invalid image access qualifier!");
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
            // store counter: ';counter:NAME:32:RESID:1:OFFSET'
            metadata += ";counter:";
            metadata += arg.argName.c_str();
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
            // store value: ';value:NAME:TYPE:VECSIZE:1:OFFSET'
            metadata += ";value:";
            metadata += arg.argName.c_str();
            metadata += ':';
            const TypeNameVecSize& tp = argTypeNamesTable[cxuint(arg.argType)];
            if (tp.kindOfType == KT_UNKNOWN)
                throw BinGenException("Type not supported!");
            // type size is aligned (fix for 3 length vectors)
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
            // store constarg for constant pointers
            metadata += ";constarg:";
            itocstrCStyle(k, numBuf, 21);
            metadata += numBuf;
            metadata += ':';
            metadata += arg.argName.c_str();
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
    {
        /* constant samplers */
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
            metadata += arg.argName.c_str();
            metadata += ':';
            itocstrCStyle(sampId, numBuf, 21);
            metadata += numBuf;
            metadata += ":0:0\n";
            sampId++;
        }
    
    if (input->is64Bit)
        metadata += ";memory:64bitABI\n";
    if (tempConfig.uavId != BINGEN_NOTSUPPLIED)
    {
        metadata += ";uavid:";
        itocstrCStyle(tempConfig.uavId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    if (tempConfig.printfId != BINGEN_NOTSUPPLIED)
    {
        metadata += ";printfid:";
        itocstrCStyle(tempConfig.printfId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    if (tempConfig.constBufferId != BINGEN_NOTSUPPLIED)
    {
        metadata += ";cbid:";
        itocstrCStyle(tempConfig.constBufferId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    if (tempConfig.privateId != BINGEN_NOTSUPPLIED)
    {
        metadata += ";privateid:";
        itocstrCStyle(tempConfig.privateId, numBuf, 21);
        metadata += numBuf;
        metadata += '\n';
    }
    for (cxuint k = 0; k < config.args.size(); k++)
    {
        // store reflection: ';reflection:ARGINDEX:TYPENAME
        const AmdKernelArgInput& arg = config.args[k];
        metadata += ";reflection:";
        itocstrCStyle(k, numBuf, 21);
        metadata += numBuf;
        metadata += ':';
        metadata += arg.typeName.c_str();
        metadata += '\n';
    }
    
    metadata += ";ARGEND:__OpenCL_";
    metadata += kinput.kernelName.c_str();
    metadata += "_kernel\n";
    
    return metadata;
}

static void generateCALNotes(FastOutputBuffer& bos, const AmdInput* input,
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
    // count used read only images, write only images,const buffers and samplers
    for (const AmdKernelArgInput& arg: config.args)
    {
        if (isKernelArgImage(arg.argType))
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
    {
        uint32_t rdimgIds[128];
        for (cxuint k = 0; k < readOnlyImages; k++)
            SLEV(rdimgIds[k], isOlderThan1124 ? readOnlyImages-k-1 : k);
        bos.writeArray(readOnlyImages, rdimgIds);
    }
    // CALNOTE_OUTPUTS
    putCALNoteLE(bos, CALNOTE_ATI_OUTPUTS, 0);
    // CALNOTE_UAV
    putCALNoteLE(bos, CALNOTE_ATI_UAV, 16*tempConfig.uavsNum);
    
    if (isOlderThan1124)
    {
        // for old drivers
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (isKernelArgImage(arg.argType) &&
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
    {
        /* in argument order */
        for (cxuint k = 0; k < config.args.size(); k++)
        {
            const AmdKernelArgInput& arg = config.args[k];
            if (isKernelArgImage(arg.argType) &&
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
    
    // CALNOTE_CONDOUT
    putCALNoteLE(bos, CALNOTE_ATI_CONDOUT, 4);
    bos.writeObject(LEV(uint32_t(config.condOut)));
    // CALNOTE_FLOAT32CONSTS
    putCALNoteLE(bos, CALNOTE_ATI_FLOAT32CONSTS, 0);
    // CALNOTE_INT32CONSTS
    putCALNoteLE(bos, CALNOTE_ATI_INT32CONSTS, 0);
    // CALNOTE_BOOL32CONSTS
    putCALNoteLE(bos, CALNOTE_ATI_BOOL32CONSTS, 0);
    
    // CALNOTE_EARLYEXIT
    putCALNoteLE(bos, CALNOTE_ATI_EARLYEXIT, 4);
    bos.writeObject(LEV(uint32_t(config.earlyExit)));
    
    // CALNOTE_GLOBAL_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_GLOBAL_BUFFERS, 0);
    
    // CALNOTE_CONSTANT_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_CONSTANT_BUFFERS, 8*constBuffersNum);
    
    if (isOlderThan1124)
    {
        /* for driver 12.10 */
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
    
    // CALNOTE_INPUT_SAMPLERS
    putCALNoteLE(bos, CALNOTE_ATI_INPUT_SAMPLERS, 8*samplersNum);
    
    for (cxuint k = 0; k < config.samplers.size(); k++)
        putSamplerEntryLE(bos, 0, k+argSamplersNum);
    for (cxuint k = 0; k < argSamplersNum; k++)
        putSamplerEntryLE(bos, 0, k);
    
    // CALNOTE_SCRATCH_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_SCRATCH_BUFFERS, 4);
    bos.writeObject<uint32_t>(LEV((config.scratchBufferSize+3)>>2));
    
    // CALNOTE_PERSISTENT_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_PERSISTENT_BUFFERS, 0);
    
    /* PROGRAM_INFO */
    size_t userDataElemsNum = config.userDatas.size();
    const cxuint progInfoSize = (18+32 +
            4*((isOlderThan1124)?16:userDataElemsNum))*8;
    putCALNoteLE(bos, CALNOTE_ATI_PROGINFO, progInfoSize);
    
    // write USERDATA (USERSGPR specifiers) CAL notes
    putProgInfoEntryLE(bos, 0x80001000U, userDataElemsNum);
    cxuint k = 0;
    for (k = 0; k < userDataElemsNum; k++)
    {
        putProgInfoEntryLE(bos, 0x80001001U+(k<<2), config.userDatas[k].dataClass);
        putProgInfoEntryLE(bos, 0x80001002U+(k<<2), config.userDatas[k].apiSlot);
        putProgInfoEntryLE(bos, 0x80001003U+(k<<2), config.userDatas[k].regStart);
        putProgInfoEntryLE(bos, 0x80001004U+(k<<2), config.userDatas[k].regSize);
    }
    if (isOlderThan1124)
        // fill up empty USERDATA for older drivers
        for (k =  userDataElemsNum; k < 16; k++)
        {
            putProgInfoEntryLE(bos, 0x80001001U+(k<<2), 0);
            putProgInfoEntryLE(bos, 0x80001002U+(k<<2), 0);
            putProgInfoEntryLE(bos, 0x80001003U+(k<<2), 0);
            putProgInfoEntryLE(bos, 0x80001004U+(k<<2), 0);
        }
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
    const cxuint localSize = (isLocalPointers) ? 32768 : config.hwLocalSize;
    uint32_t curPgmRSRC2 = config.pgmRSRC2;
    cxuint pgmUserSGPRsNum = 0;
    for (cxuint p = 0; p < userDataElemsNum; p++)
        pgmUserSGPRsNum = std::max(pgmUserSGPRsNum,
                 config.userDatas[p].regStart+config.userDatas[p].regSize);
    pgmUserSGPRsNum = (pgmUserSGPRsNum != 0) ? pgmUserSGPRsNum : 2;
    curPgmRSRC2 = (curPgmRSRC2 & 0xffffe440U) |
            calculatePgmRSrc2(arch, (config.scratchBufferSize != 0), pgmUserSGPRsNum,
                    false, config.dimMask, (curPgmRSRC2 & 0x1b80U), config.tgSize,
                    localSize, config.exceptions);
    
    putProgInfoEntryLE(bos, 0x80001041U, config.usedVGPRsNum);
    putProgInfoEntryLE(bos, 0x80001042U, config.usedSGPRsNum);
    putProgInfoEntryLE(bos, 0x80001863U, getGPUMaxRegistersNum(arch, REGTYPE_SGPR,
                           REGCOUNT_NO_VCC));
    putProgInfoEntryLE(bos, 0x80001864U, 256);
    putProgInfoEntryLE(bos, 0x80001043U, config.floatMode);
    putProgInfoEntryLE(bos, 0x80001044U, config.ieeeMode);
    putProgInfoEntryLE(bos, 0x80001045U, (config.scratchBufferSize+3)>>2);
    putProgInfoEntryLE(bos, 0x00002e13U, curPgmRSRC2);
    
    // write reqd_work_group_size in CALnotes
    if (config.reqdWorkGroupSize[0] != 0 && config.reqdWorkGroupSize[1] != 0 &&
        config.reqdWorkGroupSize[2] != 0)
    {
        putProgInfoEntryLE(bos, 0x8000001cU, config.reqdWorkGroupSize[0]);
        putProgInfoEntryLE(bos, 0x8000001dU, config.reqdWorkGroupSize[1]);
        putProgInfoEntryLE(bos, 0x8000001eU, config.reqdWorkGroupSize[2]);
    }
    else
    {
        /* default */
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
        if (tempConfig.printfId != BINGEN_NOTSUPPLIED)
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
    
    // CAL_SUBCONSTANT_BUFFERS
    putCALNoteLE(bos, CALNOTE_ATI_SUB_CONSTANT_BUFFERS, 0);
    
    // CAL_UAV_MAILBOX_SIZE
    putCALNoteLE(bos, CALNOTE_ATI_UAV_MAILBOX_SIZE, 4);
    bos.writeObject<uint32_t>(0);
    
    // CAL_UAV_OP_MASK
    putCALNoteLE(bos, CALNOTE_ATI_UAV_OP_MASK, 128);
    for (cxuint k = 0; k < 32; k++)
        uavMask[k] = LEV(uavMask[k]);
    bos.writeArray(32, uavMask);
}

// collect unique ids and functions ids to determine free unique id or function id later
static std::vector<cxuint> collectUniqueIdsAndFunctionIds(const AmdInput* input)
{
    std::vector<cxuint> uniqueIds;
    for (const AmdKernelInput& kernel: input->kernels)
        if (!kernel.useConfig)
        {
            const char* metadata = kernel.metadata;
            if (kernel.metadataSize < 11) // if too short
                continue;
            size_t pos = 0;
            for (pos = 0; pos < kernel.metadataSize-11; pos++)
                if (::memcmp(metadata+pos, "\n;uniqueid:", 11) == 0)
                    break;
            if (pos == kernel.metadataSize-11)
                continue; // not found
            const char* outEnd;
            pos += 11;
            // parse and push back to unique ids
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
            // parse and push back to unique ids
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
 * this routine keep original structure of GPU binary (section order, alignment etc)
 */

void AmdGPUBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    const size_t kernelsNum = input->kernels.size();
    CString driverInfo;
    uint32_t driverVersion = 99999909U;
    if (input->driverInfo.empty())
    {
        // if driver info is not given, then format default driver info
        char drvInfoBuf[100];
        ::snprintf(drvInfoBuf, 100, "@(#) OpenCL 1.2 AMD-APP (%u.%u).  "
                "Driver version: %u.%u (VM)",
                 input->driverVersion/100U, input->driverVersion%100U,
                 input->driverVersion/100U, input->driverVersion%100U);
        driverInfo = drvInfoBuf;
        driverVersion = input->driverVersion;
    }
    else if (input->driverVersion == 0)
    {
        // parse version from given driver_info
        size_t pos = input->driverInfo.find("AMD-APP"); // find AMDAPP
        try
        {
            driverInfo = input->driverInfo;
            if (pos != std::string::npos)
            {
                /* let to parse version number */
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
    
    const bool isOlderThan1124 = driverVersion < 112402;
    const bool isOlderThan1348 = driverVersion < 134805;
    /* checking input */
    if (input->deviceType > GPUDeviceType::GPUDEVICE_MAX)
        throw BinGenException("Unknown GPU device type");
    
    Array<TempAmdKernelConfig> tempAmdKernelConfigs(kernelsNum);
    prepareTempConfigs(driverVersion, input, tempAmdKernelConfigs);
    
    std::unique_ptr<ElfBinaryGen32> elfBinGen32;
    std::unique_ptr<ElfBinaryGen64> elfBinGen64;
    
    if (gpuDeviceCodeTable[cxuint(input->deviceType)] == 0xffff)
        throw BinGenException("Unsupported GPU device type by OpenCL 1.2 binary format");
    
    if (input->is64Bit)
        elfBinGen64.reset(new ElfBinaryGen64({ 0, 0, ELFOSABI_SYSV, 0, ET_EXEC, 
            gpuDeviceCodeTable[cxuint(input->deviceType)], EV_CURRENT, UINT_MAX, 0, 0 }));
    else
        elfBinGen32.reset(new ElfBinaryGen32({ 0, 0, ELFOSABI_SYSV, 0, ET_EXEC, 
            gpuDeviceCodeTable[cxuint(input->deviceType)], EV_CURRENT, UINT_MAX, 0, 0 }));
    
    Array<TempAmdKernelData> tempAmdKernelDatas(kernelsNum);
    cxuint uniqueId = 1024;
    std::vector<cxuint> uniqueIds = collectUniqueIdsAndFunctionIds(input);
    
    uint64_t allInnerBinSize = 0;
    size_t rodataSize = 0;
    for (size_t i = 0; i < kernelsNum; i++)
    {
        size_t calNotesSize = 0;
        size_t metadataSize = 0;
        const AmdKernelInput& kinput = input->kernels[i];
        TempAmdKernelData& tempData = tempAmdKernelDatas[i];
        tempData.kernelDataGen = KernelDataGen(&kinput);
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
            // count UAVs, write only images, read only images, samplers
            // to calculate CALNotes size
            for (const AmdKernelArgInput& arg: config.args)
            {
                if (isKernelArgImage(arg.argType))
                {
                    if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_READ_ONLY)
                        readOnlyImages++;
                    if ((arg.ptrAccess & KARG_PTR_ACCESS_MASK) == KARG_PTR_WRITE_ONLY)
                    {
                        uavsNum++;
                        writeOnlyImages++;
                        if (writeOnlyImages > 8)
                            throw BinGenException("Too many write only images");
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
            
            TempAmdKernelConfig& tempConfig = tempAmdKernelConfigs[i];
            tempConfig.uavsNum = uavsNum;
            
            tempAmdKernelDatas[i].metadata = generateMetadata(driverVersion, input, kinput,
                     tempConfig, argSamplersNum, uniqueId);
            
            calNotesSize = uint64_t(20*17) /*calNoteHeaders*/ + 16 + 128 + (18+32 +
                4*((isOlderThan1124)?16:config.userDatas.size()))*8 /* proginfo */ +
                    readOnlyImages*4 /* inputs */ + 16*uavsNum /* uavs */ +
                    8*samplersNum /* samplers */ + 8*constBuffersNum /* cbids */;
            
            metadataSize = tempData.metadata.size() + 32 /* header size */;
            
            tempData.header[0] = LEV((driverVersion >= 164205)?tempConfig.uavPrivate:0);
            tempData.header[1] = 0U;
            tempData.header[2] = LEV((isOlderThan1348)? 0 : tempConfig.uavPrivate);
            tempData.header[3] = LEV(uint32_t(kinput.config.hwLocalSize));
            tempData.header[4] = LEV(uint32_t((input->is64Bit?8:0) |
                        (kinput.config.usePrintf?2:0)));
            tempData.header[5] = LEV(1U);
            tempData.header[6] = 0U;
            tempData.header[7] = 0U;
            
            uniqueId++;
        }
        else
        {
            for (const CALNoteInput& calNote: kinput.calNotes)
                calNotesSize += uint64_t(20) + calNote.header.descSize;
            
            metadataSize = kinput.metadataSize + kinput.headerSize;
        }
        
        rodataSize += metadataSize;
        /* kernel elf bin generator */
        ElfBinaryGen32& kelfBinGen = tempAmdKernelDatas[i].elfBinGen;
        
        tempAmdKernelDatas[i].calNoteGen = CALNoteGen(input, driverVersion, &kinput,
                         &tempAmdKernelConfigs[i]);
        
        kelfBinGen.setHeader({ 0, 0, 0x64, 1, ET_EXEC, 0x7dU, EV_CURRENT,
                    UINT_MAX, 0, 1 });
        kelfBinGen.addRegion(ElfRegion32::programHeaderTable());
        // CALNoteDir entries
        kelfBinGen.addRegion(ElfRegion32(sizeof(CALEncodingEntry),
                     (const cxbyte*)&tempAmdKernelDatas[i].calEncEntry, 0));
        kelfBinGen.addRegion(ElfRegion32(0, (const cxbyte*)nullptr, 0,
                 ".shstrtab", SHT_STRTAB, 0));
        kelfBinGen.addRegion(ElfRegion32::sectionHeaderTable());
        // CALNotes
        kelfBinGen.addRegion(ElfRegion32(calNotesSize,
                 &tempAmdKernelDatas[i].calNoteGen, 0));
        kelfBinGen.addRegion(ElfRegion32(kinput.codeSize, kinput.code, 0,
                 ".text", SHT_PROGBITS, 0));
        // empty '.data' section (zeroed)
        kelfBinGen.addRegion(ElfRegion32((kinput.data!=nullptr)?kinput.dataSize:4736,
                 &tempData.kernelDataGen, 0, ".data",
                 SHT_PROGBITS, 0, 0, 0, 0, kinput.dataSize));
        kelfBinGen.addRegion(ElfRegion32(0, (const cxbyte*)nullptr, 0,
                 ".symtab", SHT_SYMTAB, 0, 0, 1));
        if (kinput.extraSymbols.empty()) // default content (2 bytes)
            kelfBinGen.addRegion(ElfRegion32(2, (const cxbyte*)"\000\000", 0,
                     ".strtab", SHT_STRTAB, 0));
        else    // allow to overwirte by elfbingen if some symbol must be put)
            kelfBinGen.addRegion(ElfRegion32(0, (const cxbyte*)nullptr, 0,
                     ".strtab", SHT_STRTAB, 0));
        
        /* extra sections */
        for (const BinSection& section: kinput.extraSections)
            kelfBinGen.addRegion(ElfRegion32(section, kernelBuiltinSectionTable,
                         ELFSECTID_STD_MAX, 6));
        /* program headers */
        kelfBinGen.addProgramHeader({ 0x70000002U, 0, 1, 1, false, 0, 0, 0});
        kelfBinGen.addProgramHeader({ PT_NOTE, 0, 4, 1, false, 0, 0, 0 });
        kelfBinGen.addProgramHeader({ PT_LOAD, 0, 5, 4, true, 0, 0, 0 });
        
        /* extra symbols */
        for (const BinSymbol& symbol: kinput.extraSymbols)
            kelfBinGen.addSymbol(ElfSymbol32(symbol, kernelBuiltinSectionTable,
                         ELFSECTID_STD_MAX, 6));
        
        const uint64_t innerBinSize = kelfBinGen.countSize();
        if (innerBinSize > UINT32_MAX)
            throw BinGenException("Inner binary size is too big!");
        allInnerBinSize += tempAmdKernelDatas[i].innerBinSize = innerBinSize;
        
        tempAmdKernelDatas[i].calEncEntry =
            { LEV(uint32_t(gpuDeviceInnerCodeTable[cxuint(input->deviceType)])), LEV(4U), 
                LEV(0x1c0U), LEV(uint32_t(tempAmdKernelDatas[i].innerBinSize - 0x1c0U)) };
    }
    if (input->globalData!=nullptr)
        rodataSize += input->globalDataSize;
    
    CL1MainRoDataGen rodataGen(input, tempAmdKernelDatas);
    CL1MainTextGen textGen(tempAmdKernelDatas);
    CL1MainCommentGen commentGen(input, driverInfo);
    CL1MainStrTabGen mainStrTabGen(driverVersion, input);
    CL1MainSymTabGen<Elf32Types> mainSymTabGen32(driverVersion, input, tempAmdKernelDatas);
    CL1MainSymTabGen<Elf64Types> mainSymTabGen64(driverVersion, input, tempAmdKernelDatas);
    
    /* main sections and symbols */
    if (input->is64Bit)
        putMainSections(*elfBinGen64.get(), driverVersion, input, allInnerBinSize, textGen,
                    rodataSize, rodataGen, driverInfo, commentGen,
                    mainStrTabGen, mainSymTabGen64);
    else
        putMainSections(*elfBinGen32.get(), driverVersion, input, allInnerBinSize, textGen,
                    rodataSize, rodataGen, driverInfo, commentGen,
                    mainStrTabGen, mainSymTabGen32);
    
    const uint64_t binarySize = (input->is64Bit) ? elfBinGen64->countSize() : 
            elfBinGen32->countSize();
    if (
#ifdef HAVE_64BIT
        !input->is64Bit &&
#endif
        binarySize > UINT32_MAX)
        throw BinGenException("Binary size is too big!");
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
    FastOutputBuffer fob(256, *os);
    try
    {
        os->exceptions(std::ios::failbit | std::ios::badbit);
        if (input->is64Bit)
            elfBinGen64->generate(fob);
        else
            elfBinGen32->generate(fob);
    }
    catch(...)
    {
        os->exceptions(oldExceptions);
        throw;
    }
    os->exceptions(oldExceptions);
    assert(fob.getWritten() == binarySize);
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

static const char* amdOclMagicString = "AMD-APP";
static std::mutex detectionMutex;
static uint64_t detectionFileTimestamp = 0;
static std::string detectionAmdOclPath;
static uint32_t detectedDriverVersion = 0;

static std::string escapePath(const std::string& path)
{
    std::string newPath;
    for (char c: path)
        if (c == CLRX_NATIVE_DIR_SEP)
            newPath.append("%#");
        else if (c == '%')
            newPath.append("%%");
        else if (c == ':')
            newPath.append("%$");
        else
            newPath += c;
    return newPath;
}

uint32_t CLRX::detectAmdDriverVersion()
{
    std::lock_guard<std::mutex> lock(detectionMutex);
    std::string amdOclPath = findAmdOCL();
    
    bool notThisSameFile = false;
    if (amdOclPath != detectionAmdOclPath)
    {
        notThisSameFile = true;
        detectionAmdOclPath = amdOclPath;
    }
    
    uint64_t timestamp = 0;
    try
    { timestamp = getFileTimestamp(amdOclPath.c_str()); }
    catch(const Exception& ex)
    { }
    
    if (!notThisSameFile && timestamp == detectionFileTimestamp)
        return detectedDriverVersion;
    
    std::string clrxTimestampPath = getHomeDir();
    if (!clrxTimestampPath.empty())
    {
        // first, we check from stored version in config files
        clrxTimestampPath = joinPaths(clrxTimestampPath, ".clrxamdocltstamp");
        try
        { makeDir(clrxTimestampPath.c_str()); }
        catch(const std::exception& ex)
        { }
        // file path
        clrxTimestampPath = joinPaths(clrxTimestampPath, escapePath(amdOclPath));
        try
        {
        std::ifstream ifs(clrxTimestampPath.c_str(), std::ios::binary);
        if (ifs)
        {
            // read driver version from stored config files
            ifs.exceptions(std::ios::badbit | std::ios::failbit);
            uint64_t readedTimestamp = 0;
            uint32_t readedVersion = 0;
            ifs >> readedTimestamp >> readedVersion;
            if (readedTimestamp!=0 && readedVersion!=0 && timestamp==readedTimestamp)
            {
                // amdocl has not been changed
                detectionFileTimestamp = readedTimestamp;
                detectedDriverVersion = readedVersion;
                return readedVersion;
            }
        }
        }
        catch(const std::exception& ex)
        { }
    }
        
    detectionFileTimestamp = timestamp;
    detectedDriverVersion = 0;
    try
    {
        std::ifstream fs(amdOclPath.c_str(), std::ios::binary);
        if (!fs) return 0;
        FastInputBuffer fib(256, fs);
        size_t index = 0;
        while (amdOclMagicString[index]!=0)
        {
            int c = fib.get();
            if (c == std::streambuf::traits_type::eof())
                break;
            if (amdOclMagicString[index]==c)
                index++;
            else // reset
                index=0;
        }
        if (amdOclMagicString[index]==0)
        { //
            char buf[20];
            ::memset(buf, 0, 20);
            if (fib.get()!=' ')
                return 0; // skip space
            if (fib.get()!='(')
                return 0; // skip '('
            // get driver version
            fib.read(buf, 20);
            
            const char* next;
            detectedDriverVersion = cstrtoui(buf, buf+20, next)*100;
            if (next!=buf+20 && *next=='.') // minor version
                detectedDriverVersion += cstrtoui(next+1, buf+20, next)%100;
        }
        
        // write to config
        if (!clrxTimestampPath.empty()) // if clrxamdocltstamp set
        {
            // write to
            std::ofstream ofs(clrxTimestampPath.c_str(), std::ios::binary);
            ofs << detectionFileTimestamp << " " << detectedDriverVersion;
        }
    }
    catch(const std::exception& ex)
    { }
    return detectedDriverVersion;
}
