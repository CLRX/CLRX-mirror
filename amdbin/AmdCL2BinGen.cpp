/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2016 Mateusz Szpakowski
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
#include <cstdint>
#include <cassert>
#include <bitset>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdbin/AmdCL2BinGen.h>

using namespace CLRX;

static const cxuint innerBinSectonTableLen = AMDCL2SECTID_MAX+1-ELFSECTID_START;

void AmdCL2Input::addEmptyKernel(const char* kernelName)
{
    AmdCL2KernelInput kernel{};
    kernel.kernelName = kernelName;
    
    kernel.config.usedSGPRsNum = kernel.config.usedVGPRsNum = BINGEN_DEFAULT;
    kernel.config.floatMode = 0xc0;
    kernel.config.dimMask = BINGEN_DEFAULT;
    kernels.push_back(std::move(kernel));
}

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator() : manageable(false), input(nullptr)
{ }

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator(const AmdCL2Input* amdInput)
        : manageable(false), input(amdInput)
{ }

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator(GPUDeviceType deviceType,
       uint32_t driverVersion, size_t globalDataSize, const cxbyte* globalData,
       size_t rwDataSize, const cxbyte* rwData, 
       const std::vector<AmdCL2KernelInput>& kernelInputs)
        : manageable(true), input(nullptr)
{
    input = new AmdCL2Input{deviceType, globalDataSize, globalData,
                rwDataSize, rwData, 0, 0, 0, nullptr, false, { }, { },
                driverVersion, "", "", kernelInputs };
}

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator(GPUDeviceType deviceType,
       uint32_t driverVersion, size_t globalDataSize, const cxbyte* globalData,
       size_t rwDataSize, const cxbyte* rwData,
       std::vector<AmdCL2KernelInput>&& kernelInputs)
        : manageable(true), input(nullptr)
{
    input = new AmdCL2Input{deviceType, globalDataSize, globalData,
                rwDataSize, rwData, 0, 0, 0, nullptr, false, { }, { },
                driverVersion, "", "", std::move(kernelInputs) };
}

AmdCL2GPUBinGenerator::~AmdCL2GPUBinGenerator()
{
    if (manageable)
        delete input;
}

void AmdCL2GPUBinGenerator::setInput(const AmdCL2Input* input)
{
    if (manageable)
        delete input;
    manageable = false;
    this->input = input;
}

struct CLRX_INTERNAL TempAmdCL2KernelData
{
    size_t metadataSize;
    size_t isaMetadataSize;
    size_t stubSize;
    size_t setupSize;
    size_t codeSize;
    bool useLocals;
    uint32_t pipesUsed;
    Array<uint16_t> argResIds;
};

struct CLRX_INTERNAL ArgTypeSizes
{
    cxbyte type;
    cxbyte elemSize;
    cxbyte vectorSize;
};

static const ArgTypeSizes argTypeSizesTable[] =
{
    { 6, 1, 1 /*void */ },
    { 6, 1, 1 /*uchar*/ }, { 6, 1, 1, /*char*/ },
    { 7, 2, 1, /*ushort*/ }, { 7, 2, 1, /*short*/ },
    { 8, 4, 1, /*uint*/ }, { 8, 4, 1, /*INT*/ },
    { 9, 8, 1, /*ulong*/ }, { 9, 8, 1, /*long*/ },
    { 11, 4, 1, /*float*/ }, { 12, 8, 1, /*double*/ },
    { 7, 8, 1, /*pointer*/ },
    { 0, 32, 1, /*image*/ }, { 0, 32, 1, /*image1d*/ }, { 0, 32, 1, /*image1da */ },
    { 0, 32, 1, /*image1db*/ }, { 0, 32, 1, /*image2d*/ }, { 0, 32, 1, /*image2da*/ },
    { 0, 32, 1, /*image3d*/ },
    { 6, 1, 2, /*uchar2*/ }, { 6, 1, 3, /*uchar3*/ }, { 6, 1, 4, /*uchar4*/ },
    { 6, 1, 8, /*uchar8*/ }, { 6, 1, 16, /*uchar16*/ },
    { 6, 1, 2, /*char2*/ }, { 6, 1, 3, /*char3*/ }, { 6, 1, 4, /*char4*/ },
    { 6, 1, 8, /*char8*/ }, { 6, 1, 16, /*char16*/ },
    { 7, 2, 2, /*ushort2*/ }, { 7, 2, 3, /*ushort3*/ }, { 7, 2, 4, /*ushort4*/ },
    { 7, 2, 8, /*ushort8*/ }, { 7, 2, 16, /*ushort16*/ },
    { 7, 2, 2, /*short2*/ }, { 7, 2, 3, /*short3*/ }, { 7, 2, 4, /*short4*/ },
    { 7, 2, 8, /*short8*/ }, { 7, 2, 16, /*short16*/ },
    { 8, 4, 2, /*uint2*/ }, { 8, 4, 3, /*uint3*/ }, { 8, 4, 4, /*uint4*/ },
    { 8, 4, 8, /*uint8*/ }, { 8, 4, 16, /*uint16*/ },
    { 8, 4, 2, /*int2*/ }, { 8, 4, 3, /*int3*/ }, { 8, 4, 4, /*int4*/ },
    { 8, 4, 8, /*int8*/ }, { 8, 4, 16, /*int16*/ },
    { 9, 8, 2, /*ulong2*/ }, { 9, 8, 3, /*ulong3*/ }, { 9, 8, 4, /*ulong4*/ },
    { 9, 8, 8, /*ulong8*/ }, { 9, 8, 16, /*ulong16*/ },
    { 9, 8, 2, /*long2*/ }, { 9, 8, 3, /*long3*/ }, { 9, 8, 4, /*long4*/ },
    { 9, 8, 8, /*long8*/ }, { 9, 8, 16, /*long16*/ },
    { 11, 4, 2, /*float2*/ }, { 11, 4, 3, /*float3*/ }, { 11, 4, 4, /*float4*/ },
    { 11, 4, 8, /*float8*/ }, { 11, 4, 16, /*float16*/ },
    { 12, 8, 2, /*double2*/ }, { 12, 8, 3, /*double3*/ }, { 12, 8, 4, /*double4*/ },
    { 12, 8, 8, /*double8*/ }, { 12, 8, 16, /*double16*/ },
    { 0, 16, 1, /* sampler*/ }, { 15, 0, 0, /*structure*/ }, { 0, 0, 0, /*counter*/ },
    { 0, 0, 0, /*counter64*/ }, { 7, 16, 1, /* pipe*/ }, { 18, 16, 1, /*cmdqueue*/ },
    { 7, 8, 1, /*clkevent*/ }
};

static const uint32_t gpuDeviceCodeTable[21] =
{
    UINT_MAX, // GPUDeviceType::CAPE_VERDE
    UINT_MAX, // GPUDeviceType::PITCAIRN
    UINT_MAX, // GPUDeviceType::TAHITI
    UINT_MAX, // GPUDeviceType::OLAND
    6, // GPUDeviceType::BONAIRE
    1, // GPUDeviceType::SPECTRE
    2, // GPUDeviceType::SPOOKY
    3, // GPUDeviceType::KALINDI
    UINT_MAX, // GPUDeviceType::HAINAN
    7, // GPUDeviceType::HAWAII
    8, // GPUDeviceType::ICELAND
    9, // GPUDeviceType::TONGA
    4, // GPUDeviceType::MULLINS
    17, // GPUDeviceType::FIJI
    16, // GPUDeviceType::CARRIZO
    15, // GPUDeviceType::DUMMY
    UINT_MAX, // GPUDeviceType::GOOSE
    UINT_MAX, // GPUDeviceType::HORSE
    UINT_MAX, // GPUDeviceType::STONEY
    UINT_MAX, // GPUDeviceType::ELLESMERE
    UINT_MAX  // GPUDeviceType::BAFFIN
};

static const uint32_t gpuDeviceCodeTable15_7[21] =
{
    UINT_MAX, // GPUDeviceType::CAPE_VERDE
    UINT_MAX, // GPUDeviceType::PITCAIRN
    UINT_MAX, // GPUDeviceType::TAHITI
    UINT_MAX, // GPUDeviceType::OLAND
    6, // GPUDeviceType::BONAIRE
    1, // GPUDeviceType::SPECTRE
    2, // GPUDeviceType::SPOOKY
    3, // GPUDeviceType::KALINDI
    UINT_MAX, // GPUDeviceType::HAINAN
    7, // GPUDeviceType::HAWAII
    8, // GPUDeviceType::ICELAND
    9, // GPUDeviceType::TONGA
    4, // GPUDeviceType::MULLINS
    16, // GPUDeviceType::FIJI
    15, // GPUDeviceType::CARRIZO
    UINT_MAX, // GPUDeviceType::DUMMY
    UINT_MAX, // GPUDeviceType::GOOSE
    UINT_MAX, // GPUDeviceType::HORSE
    UINT_MAX, // GPUDeviceType::STONEY
    UINT_MAX, // GPUDeviceType::ELLESMERE
    UINT_MAX  // GPUDeviceType::BAFFIN
};

static const uint32_t gpuDeviceCodeTable16_3[21] =
{
    UINT_MAX, // GPUDeviceType::CAPE_VERDE
    UINT_MAX, // GPUDeviceType::PITCAIRN
    UINT_MAX, // GPUDeviceType::TAHITI
    UINT_MAX, // GPUDeviceType::OLAND
    6, // GPUDeviceType::BONAIRE
    1, // GPUDeviceType::SPECTRE
    2, // GPUDeviceType::SPOOKY
    3, // GPUDeviceType::KALINDI
    UINT_MAX, // GPUDeviceType::HAINAN
    7, // GPUDeviceType::HAWAII
    8, // GPUDeviceType::ICELAND
    9, // GPUDeviceType::TONGA
    4, // GPUDeviceType::MULLINS
    16, // GPUDeviceType::FIJI
    15, // GPUDeviceType::CARRIZO
    UINT_MAX, // GPUDeviceType::DUMMY
    13, // GPUDeviceType::GOOSE
    12, // GPUDeviceType::HORSE
    17, // GPUDeviceType::STONEY
    UINT_MAX, // GPUDeviceType::ELLESMERE
    UINT_MAX  // GPUDeviceType::BAFFIN
};

static const uint32_t gpuDeviceCodeTableGPUPRO[21] =
{
    UINT_MAX, // GPUDeviceType::CAPE_VERDE
    UINT_MAX, // GPUDeviceType::PITCAIRN
    UINT_MAX, // GPUDeviceType::TAHITI
    UINT_MAX, // GPUDeviceType::OLAND
    6, // GPUDeviceType::BONAIRE
    1, // GPUDeviceType::SPECTRE
    2, // GPUDeviceType::SPOOKY
    3, // GPUDeviceType::KALINDI
    UINT_MAX, // GPUDeviceType::HAINAN
    7, // GPUDeviceType::HAWAII
    8, // GPUDeviceType::ICELAND
    9, // GPUDeviceType::TONGA
    4, // GPUDeviceType::MULLINS
    14, // GPUDeviceType::FIJI
    13, // GPUDeviceType::CARRIZO
    UINT_MAX, // GPUDeviceType::DUMMY
    UINT_MAX, // GPUDeviceType::GOOSE
    UINT_MAX, // GPUDeviceType::HORSE
    15, // GPUDeviceType::STONEY
    17, // GPUDeviceType::ELLESMERE
    16 // GPUDeviceType::BAFFIN
};

static const uint16_t mainBuiltinSectionTable[] =
{
    1, // ELFSECTID_SHSTRTAB
    2, // ELFSECTID_STRTAB
    3, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    6, // ELFSECTID_TEXT
    5, // ELFSECTID_RODATA
    SHN_UNDEF, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    4 // ELFSECTID_COMMENT
};

static const cxbyte kernelIsaMetadata[] =
{
    0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x42, 0x09, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static void prepareKernelTempData(const AmdCL2Input* input,
          Array<TempAmdCL2KernelData>& tempDatas)
{
    const bool newBinaries = input->driverVersion >= 191205;
    const bool is16_3Ver = input->driverVersion >= 200406;
    const size_t kernelsNum = input->kernels.size();
    
    const size_t samplersNum = (input->samplerConfig) ?
                input->samplers.size() : (input->samplerInitSize>>3);
    
    for (size_t i = 0; i < kernelsNum; i++)
    {
        const AmdCL2KernelInput& kernel = input->kernels[i];
        TempAmdCL2KernelData& tempData = tempDatas[i];
        if (newBinaries && (kernel.isaMetadataSize!=0 || kernel.isaMetadata!=nullptr))
            throw Exception("ISA metadata allowed for old driver binaries");
        if (newBinaries && (kernel.stubSize!=0 || kernel.stub!=nullptr))
            throw Exception("Kernel stub allowed for old driver binaries");
        /* check relocations */
        if (newBinaries)
            for (const AmdCL2RelInput& rel: kernel.relocations)
            {
                if (rel.type > RELTYPE_HIGH_32BIT || rel.symbol > 2)
                    throw Exception("Wrong relocation symbol or type");
                if (rel.offset+4 > kernel.codeSize)
                    throw Exception("Relocation offset outside code size");
            }
        
        if (!kernel.useConfig)
        {   // if no kernel configuration
            tempData.metadataSize = kernel.metadataSize;
            tempData.isaMetadataSize = (kernel.isaMetadata!=nullptr) ?
                        kernel.isaMetadataSize : sizeof(kernelIsaMetadata);
            tempData.setupSize = kernel.setupSize;
            tempData.stubSize = kernel.stubSize;
        }
        else
        {   // if kernel configuration present
            const cxuint argsNum = kernel.config.args.size();
            size_t out = ((newBinaries) ? ((is16_3Ver) ? 303 : 254) : 246) +
                        (argsNum + 1)*88;
            for (const AmdKernelArgInput& arg: kernel.config.args)
                    out += arg.argName.size() + arg.typeName.size() + 2;
            out += 48;
            
            /// if kernels uses locals
            tempData.pipesUsed = 0;
            tempData.useLocals = (kernel.config.localSize != 0);
            if (!tempData.useLocals)
                for (const AmdKernelArgInput& inarg: kernel.config.args)
                    if (inarg.argType == KernelArgType::POINTER &&
                                inarg.ptrSpace == KernelPtrSpace::LOCAL)
                    {
                        tempData.useLocals = true;
                        break;
                    }
            for (const AmdKernelArgInput& inarg: kernel.config.args)
                if (inarg.argType == KernelArgType::PIPE && inarg.used)
                    tempData.pipesUsed++;
            
            std::bitset<128> imgRoMask;
            std::bitset<64> imgWoMask;
            std::bitset<64> imgRWMask;
            std::bitset<16> samplerMask;
            tempData.argResIds.resize(argsNum);
            // collect set bit to masks
            for (cxuint k = 0; k < argsNum; k++)
            {
                const AmdKernelArgInput& inarg = kernel.config.args[k];
                if (inarg.resId != BINGEN_DEFAULT)
                {
                    if (inarg.argType == KernelArgType::SAMPLER)
                    {
                        if (inarg.resId >= 16)
                            throw Exception("SamplerId out of range!");
                        if (samplerMask[inarg.resId])
                            throw Exception("SamplerId already used!");
                        samplerMask.set(inarg.resId);
                        tempData.argResIds[k] = inarg.resId;
                    }
                    else if (inarg.argType >= KernelArgType::MIN_IMAGE &&
                        inarg.argType <= KernelArgType::MAX_IMAGE)
                    {
                        uint32_t imgAccess = inarg.ptrAccess & KARG_PTR_ACCESS_MASK;
                        if (imgAccess == KARG_PTR_READ_ONLY)
                        {
                            if (inarg.resId >= 128)
                                throw Exception("RdOnlyImgId out of range!");
                            if (imgRoMask[inarg.resId])
                                throw Exception("RdOnlyImgId already used!");
                            imgRoMask.set(inarg.resId);
                            tempData.argResIds[k] = inarg.resId;
                        }
                        else if (imgAccess == KARG_PTR_WRITE_ONLY)
                        {
                            if (inarg.resId >= 64)
                                throw Exception("WrOnlyImgId out of range!");
                            if (imgWoMask[inarg.resId])
                                throw Exception("WrOnlyImgId already used!");
                            imgWoMask.set(inarg.resId);
                            tempData.argResIds[k] = inarg.resId;
                        }
                        else // read-write images
                        {
                            if (inarg.resId >= 64)
                                throw Exception("RdWrImgId out of range!");
                            if (imgRWMask[inarg.resId])
                                throw Exception("RdWrImgId already used!");
                            imgRWMask.set(inarg.resId);
                            tempData.argResIds[k] = inarg.resId;
                        }
                    }
                }
            }
            
            cxuint imgRoCount = 0;
            cxuint imgWoCount = 0;
            cxuint imgRWCount = 0;
            cxuint samplerCount = 0;
            // now, we can set resId for argument that have no resid
            for (cxuint k = 0; k < argsNum; k++)
            {
                const AmdKernelArgInput& inarg = kernel.config.args[k];
                if (inarg.resId == BINGEN_DEFAULT)
                {
                    if (inarg.argType == KernelArgType::SAMPLER)
                    {
                        for (; samplerCount < 16 && samplerMask[samplerCount];
                             samplerCount++);
                        if (samplerCount == 16)
                            throw Exception("SamplerId out of range!");
                        tempData.argResIds[k] = samplerCount++;
                    }
                    else if (inarg.argType >= KernelArgType::MIN_IMAGE &&
                        inarg.argType <= KernelArgType::MAX_IMAGE)
                    {
                        uint32_t imgAccess = inarg.ptrAccess & KARG_PTR_ACCESS_MASK;
                        if (imgAccess == KARG_PTR_READ_ONLY)
                        {
                            for (; imgRoCount < 128 && imgRoMask[imgRoCount]; imgRoCount++);
                            if (imgRoCount == 128)
                                throw Exception("RdOnlyImgId out of range!");
                            tempData.argResIds[k] = imgRoCount++;
                        }
                        else if (imgAccess == KARG_PTR_WRITE_ONLY)
                        {
                            for (; imgWoCount < 64 && imgWoMask[imgWoCount]; imgWoCount++);
                            if (imgWoCount == 64)
                                throw Exception("WrOnlyImgId out of range!");
                            tempData.argResIds[k] = imgWoCount++;
                        }
                        else // read-write images
                        {
                            for (; imgRWCount < 64 && imgRWMask[imgRWCount]; imgRWCount++);
                            if (imgRWCount == 128)
                                throw Exception("RdWrImgId out of range!");
                            tempData.argResIds[k] = imgRWCount++;
                        }
                    }
                }
            }
            
            // check sampler list
            for (cxuint sampler: kernel.config.samplers)
                if (sampler >= samplersNum)
                    throw Exception("SamplerId out of range");
            
            tempData.metadataSize = out;
            tempData.setupSize = 256;
            tempData.stubSize = tempData.isaMetadataSize = 0;
            if (!newBinaries)
            {
                tempData.stubSize = 0xa60;
                tempData.isaMetadataSize = sizeof(kernelIsaMetadata);
            }
        }
        tempData.codeSize = kernel.codeSize;
    }
}

// fast and memory efficient String table generator for main binary
class CLRX_INTERNAL CL2MainStrTabGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    bool withBrig;
public:
    explicit CL2MainStrTabGen(const AmdCL2Input* _input) : input(_input), withBrig(false)
    {
        for (const BinSection& section: input->extraSections)
            if (section.name==".brig")
            {
                withBrig = true;
                break;
            }
    }
    
    size_t size() const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        size_t size = 1;
        if (!input->compileOptions.empty())
            size += 26;
        if (withBrig)
            size += 9; // __BRIG__
        //size += 
        if (newBinaries)
            for (const AmdCL2KernelInput& kernel: input->kernels)
                size += kernel.kernelName.size() + 19 + 17;
        else // old binaries
            for (const AmdCL2KernelInput& kernel: input->kernels)
                size += kernel.kernelName.size()*3 + 19 + 17 + 16*2 + 17 + 15;
        size += 19; // acl version string
        /// extra symbols
        for (const BinSymbol& symbol: input->extraSymbols)
            size += symbol.name.size()+1;
        return size;
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        fob.put(0);
        if (!input->compileOptions.empty())
            fob.write(26, "__OpenCL_compiler_options");
        
        for (const AmdCL2KernelInput& kernel: input->kernels)
        {   // put kernel metadata symbol names
            fob.write(19, "__OpenCL_&__OpenCL_");
            fob.write(kernel.kernelName.size(), kernel.kernelName.c_str());
            fob.write(17, "_kernel_metadata");
        }
        if (withBrig)
            fob.write(9, "__BRIG__");
        if (!newBinaries)
            for (const AmdCL2KernelInput& kernel: input->kernels)
            {   // put kernel ISA/binary symbol names
                fob.write(16, "__ISA_&__OpenCL_");
                fob.write(kernel.kernelName.size(), kernel.kernelName.c_str());
                fob.write(31, "_kernel_binary\000__ISA_&__OpenCL_");
                fob.write(kernel.kernelName.size(), kernel.kernelName.c_str());
                fob.write(17, "_kernel_metadata");
            }
        fob.write(19, "acl_version_string");
        /// extra symbols
        for (const BinSymbol& symbol: input->extraSymbols)
            fob.write(symbol.name.size()+1, symbol.name.c_str());
    }
};

// fast and memory efficient symbol table generator for main binary
class CLRX_INTERNAL CL2MainSymTabGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    bool withBrig;
    uint16_t brigIndex;
    const Array<TempAmdCL2KernelData>& tempDatas;
    const CString& aclVersion;
    cxuint extraSectionIndex;
public:
    CL2MainSymTabGen(const AmdCL2Input* _input,
             const Array<TempAmdCL2KernelData>& _tempDatas,
             const CString& _aclVersion, cxuint _extraSectionIndex)
             : input(_input), withBrig(false), tempDatas(_tempDatas),
               aclVersion(_aclVersion), extraSectionIndex(_extraSectionIndex)
    {
        for (brigIndex = 0; brigIndex < input->extraSections.size(); brigIndex++)
        {
            const BinSection& section = input->extraSections[brigIndex];
            if (section.name==".brig")
            {
                withBrig = true;
                break;
            }
        }
    }
    
    size_t size() const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        return sizeof(Elf64_Sym)*(1 + (!input->compileOptions.empty()) +
            input->kernels.size()*(newBinaries ? 1 : 3) +
            (withBrig) + 1 /* acl_version */ + input->extraSymbols.size());
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        fob.fill(sizeof(Elf64_Sym), 0);
        Elf64_Sym sym;
        size_t nameOffset = 1;
        if (!input->compileOptions.empty())
        {
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, 4);
            SLEV(sym.st_value, 0);
            SLEV(sym.st_size, input->compileOptions.size());
            sym.st_info = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameOffset += 26;
            fob.writeObject(sym);
        }
        size_t rodataPos = 0;
        size_t textPos = 0;
        for (size_t i = 0; i < input->kernels.size(); i++)
        {
            const AmdCL2KernelInput& kernel = input->kernels[i];
            const TempAmdCL2KernelData& tempData = tempDatas[i];
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, 5);
            SLEV(sym.st_value, rodataPos);
            SLEV(sym.st_size, tempData.metadataSize);
            sym.st_info = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameOffset += kernel.kernelName.size() + 19 + 17;
            rodataPos += tempData.metadataSize;
            fob.writeObject(sym);
        }
        if (withBrig)
        {   // put __BRIG__ symbol
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, 7 + brigIndex);
            SLEV(sym.st_value, 0);
            SLEV(sym.st_size, input->extraSections[brigIndex].size);
            sym.st_info = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameOffset += 9;
            fob.writeObject(sym);
        }
        
        if (!newBinaries)
        { // put binary and ISA metadata symbols
            for (size_t i = 0; i < input->kernels.size(); i++)
            {
                const AmdCL2KernelInput& kernel = input->kernels[i];
                const TempAmdCL2KernelData& tempData = tempDatas[i];
                // put kernel binary symbol
                SLEV(sym.st_name, nameOffset);
                SLEV(sym.st_shndx, 6);
                SLEV(sym.st_value, textPos);
                SLEV(sym.st_size, tempData.stubSize+tempData.setupSize+tempData.codeSize);
                sym.st_info = ELF64_ST_INFO(STB_LOCAL, STT_FUNC);
                sym.st_other = 0;
                nameOffset += 16 + kernel.kernelName.size() + 15;
                textPos += tempData.stubSize+tempData.setupSize+tempData.codeSize;
                fob.writeObject(sym);
                // put ISA metadata symbol
                SLEV(sym.st_name, nameOffset);
                SLEV(sym.st_shndx, 5);
                SLEV(sym.st_value, rodataPos);
                SLEV(sym.st_size, tempData.isaMetadataSize);
                sym.st_info = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
                sym.st_other = 0;
                nameOffset += 16 + kernel.kernelName.size() + 17;
                rodataPos += tempData.isaMetadataSize;
                fob.writeObject(sym);
            }
        }
        // acl_version_string
        SLEV(sym.st_name, nameOffset);
        SLEV(sym.st_shndx, 4);
        SLEV(sym.st_value, input->compileOptions.size());
        SLEV(sym.st_size, aclVersion.size());
        sym.st_info = ELF64_ST_INFO(STB_LOCAL, STT_OBJECT);
        sym.st_other = 0;
        fob.writeObject(sym);
        nameOffset += 19;
        
        for (const BinSymbol& symbol: input->extraSymbols)
        {
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, convertSectionId(symbol.sectionId, mainBuiltinSectionTable,
                            ELFSECTID_STD_MAX, extraSectionIndex));
            SLEV(sym.st_size, symbol.size);
            SLEV(sym.st_value, symbol.value);
            sym.st_info = symbol.info;
            sym.st_other = symbol.other;
            nameOffset += symbol.name.size()+1;
            fob.writeObject(sym);
        }
    }
};

class CLRX_INTERNAL CL2MainCommentGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    const CString& aclVersion;
public:
    explicit CL2MainCommentGen(const AmdCL2Input* _input, const CString& _aclVersion) :
            input(_input), aclVersion(_aclVersion)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        fob.write(input->compileOptions.size(), input->compileOptions.c_str());
        fob.write(aclVersion.size(), aclVersion.c_str());
    }
};

struct CLRX_INTERNAL TypeNameVecSize
{
    cxbyte elemSize;
    cxbyte vecSize;
};

static const uint32_t ptrSpacesTable[4] = { 0, 3, 5, 4 };

class CLRX_INTERNAL CL2MainRodataGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    const Array<TempAmdCL2KernelData>& tempDatas;
public:
    explicit CL2MainRodataGen(const AmdCL2Input* _input,
              const Array<TempAmdCL2KernelData>& _tempDatas) : input(_input),
              tempDatas(_tempDatas)
    { }
    
    size_t size() const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        size_t out = 0;
        for (const TempAmdCL2KernelData& tempData: tempDatas)
            out += tempData.metadataSize;
            
        if (!newBinaries)
            for (const TempAmdCL2KernelData& tempData: tempDatas)
                out += tempData.isaMetadataSize;
        return out;
    }
    
    void writeMetadata(cxuint kernelId, const TempAmdCL2KernelData& tempData,
               const AmdCL2KernelConfig& config, FastOutputBuffer& fob) const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        const bool is16_3Ver = input->driverVersion >= 200406;
        AmdCL2GPUMetadataHeader header;
        cxuint argsNum = config.args.size();
        
        SLEV(header.size, (newBinaries) ? (is16_3Ver ? 0x110 : 0xe0) : 0xd8);
        SLEV(header.metadataSize, tempData.metadataSize);
        SLEV(header.unknown1[0], 0x3);
        SLEV(header.unknown1[1], 0x1);
        SLEV(header.unknown1[2], 0x68);
        uint32_t options = config.reqdWorkGroupSize[0]!=0 ? 0x24 : 0x20;
        if (((config.useEnqueue || config.localSize!=0 || tempData.pipesUsed!=0 ||
                config.scratchBufferSize!=0) && !newBinaries))
            options |= 0x100U;
        SLEV(header.options, options);
        SLEV(header.kernelId, kernelId+1024);
        header.unknownx = 0;
        header.unknowny = 0;
        SLEV(header.unknown2[0], 0x0100000008ULL);
        SLEV(header.unknown2[1], 0x0200000001ULL);
        SLEV(header.reqdWorkGroupSize[0], config.reqdWorkGroupSize[0]);
        SLEV(header.reqdWorkGroupSize[1], config.reqdWorkGroupSize[1]);
        SLEV(header.reqdWorkGroupSize[2], config.reqdWorkGroupSize[2]);
        header.unknown3[0] = 0;
        header.unknown3[1] = 0;
        SLEV(header.firstNameLength, 0x15);
        SLEV(header.secondNameLength, 0x7);
        for (cxuint i = 0; i < 3; i++)
            header.unknown4[i] = 0;
        if (!newBinaries)
            SLEV(header.pipesUsage, (config.scratchBufferSize!=0) ?
                    config.scratchBufferSize : (tempData.pipesUsed<<4));
        else // new binaries
            header.pipesUsage = 0;
        header.unknown5[0] = header.unknown5[1] = 0;
        SLEV(header.argsNum, argsNum);
        fob.writeObject(header);
        fob.fill(40, 0); // fill up
        fob.writeObject(LEV(uint32_t(config.useEnqueue?1:0)));
        fob.writeObject(LEV(uint32_t(kernelId)));
        if (newBinaries) // additional data
        {
            fob.writeObject(LEV(uint32_t(0x00000006U)));
            if (is16_3Ver)
                fob.writeObject(uint32_t(0));
            fob.writeObject(LEV(uint32_t(
                        tempData.pipesUsed==0 && !config.useEnqueue ? 0xffffffffU : 0)));
        }
        if (is16_3Ver)
            fob.fill(44, 0);
        // two null terminated strings
        fob.writeArray(22, "__OpenCL_dummy_kernel");
        fob.writeArray(8, "generic");
        if (is16_3Ver)
            fob.writeObject<cxbyte>(0);
        
        // put argument entries
        cxuint argOffset = 0;
        for (cxuint i = 0; i < argsNum; i++)
        {   //
            const AmdKernelArgInput& arg = config.args[i];
            AmdCL2GPUKernelArgEntry argEntry;
            SLEV(argEntry.size, 88);
            SLEV(argEntry.argNameSize, arg.argName.size());
            SLEV(argEntry.typeNameSize, arg.typeName.size());
            argEntry.unknown1 = 0;
            argEntry.unknown2 = 0;
            
            bool isImage = (arg.argType >= KernelArgType::MIN_IMAGE &&
                 arg.argType <= KernelArgType::MAX_IMAGE);
            
            const ArgTypeSizes& argTypeSizes = argTypeSizesTable[cxuint(arg.argType)];
            cxuint vectorLength = argTypeSizes.vectorSize;
            if (newBinaries && vectorLength==3)
                vectorLength = 4;
            if (isImage || arg.argType==KernelArgType::SAMPLER)
                // image/sampler resid
                SLEV(argEntry.resId, tempData.argResIds[i]);
            else if (arg.argType == KernelArgType::STRUCTURE)
                SLEV(argEntry.structSize, arg.structSize);
            else
                SLEV(argEntry.vectorLength, vectorLength);
            size_t argSize = (arg.argType==KernelArgType::STRUCTURE) ? arg.structSize :
                    // argSize for argOffset: clamp elem size to 4 bytes
                    std::max(cxbyte(4), argTypeSizes.elemSize)*vectorLength;
            
            SLEV(argEntry.unknown3, (arg.argType!=KernelArgType::SAMPLER));
            SLEV(argEntry.argOffset, argOffset);
            argOffset += (argSize + 15) & ~15U; // align to 16 bytes
            
            uint32_t argType = 0;
            if (isImage)
            {   // if image
                cxuint ptrAccMask = arg.ptrAccess & KARG_PTR_ACCESS_MASK;
                argType = ptrAccMask==KARG_PTR_READ_ONLY ? 1 :
                        ptrAccMask==KARG_PTR_WRITE_ONLY ? 2 : 3 /* read-write */;
            }
            else // otherwise
            {
                argType = argTypeSizes.type;
                if (is16_3Ver && (arg.argType==KernelArgType::CHAR ||
                     arg.argType==KernelArgType::SHORT ||
                     arg.argType==KernelArgType::INT||
                     arg.argType==KernelArgType::LONG))
                    argType -= 4; // fix for crimson 16.4/gpupro
            }
            SLEV(argEntry.argType, argType);
            
            uint32_t ptrAlignment = 0;
            if (arg.argType == KernelArgType::CMDQUEUE)
                ptrAlignment = newBinaries ? 4 : 2;
            else if (arg.argType == KernelArgType::PIPE)
                ptrAlignment = 256;
            else if (arg.argType == KernelArgType::CLKEVENT)
                ptrAlignment = 4;
            else if (isImage)
                ptrAlignment = 1;
            else if (arg.argType == KernelArgType::POINTER) // otherwise
            {
                cxuint vectorLength = argTypeSizesTable[cxuint(arg.pointerType)].vectorSize;
                if (newBinaries && vectorLength==3)
                vectorLength = 4;
                size_t ptrTypeSize = (arg.pointerType==KernelArgType::STRUCTURE) ?
                    arg.structSize :argTypeSizesTable[cxuint(arg.pointerType)].elemSize *
                    vectorLength;
                ptrAlignment = 1U<<(31-CLZ32(ptrTypeSize));
                if (ptrAlignment != ptrTypeSize)
                    ptrAlignment <<= 1;
            }
            
            SLEV(argEntry.ptrAlignment, ptrAlignment);
            
            if (arg.argType == KernelArgType::CLKEVENT)
            {
                SLEV(argEntry.ptrType, 18);
                SLEV(argEntry.ptrSpace, ptrSpacesTable[cxuint(arg.ptrSpace)]);
            }
            else if (arg.argType == KernelArgType::POINTER)
            {
                SLEV(argEntry.ptrType, argTypeSizesTable[cxuint(arg.pointerType)].type);
                SLEV(argEntry.ptrSpace, ptrSpacesTable[cxuint(arg.ptrSpace)]);
            }
            else if (arg.argType == KernelArgType::PIPE)
            {
                SLEV(argEntry.ptrType, 15);
                SLEV(argEntry.ptrSpace, ptrSpacesTable[cxuint(arg.ptrSpace)]);
            }
            else
            {
                argEntry.ptrType = 0;
                argEntry.ptrSpace = 0;
            }
            cxuint isPointerOrPipe = 0;
            if (arg.argType==KernelArgType::PIPE)
                isPointerOrPipe = (arg.used) ? 3 : 1;
            else if (arg.argType==KernelArgType::POINTER ||
                    arg.argType==KernelArgType::CLKEVENT)
            {
                if (newBinaries)
                    isPointerOrPipe = (arg.used!=0) ? arg.used : 1;
                else // ???
                    isPointerOrPipe = 3;
            }
            SLEV(argEntry.isPointerOrPipe, isPointerOrPipe);
            
            SLEV(argEntry.isConst, (arg.ptrAccess & KARG_PTR_CONST) != 0);
            argEntry.isVolatile = ((arg.ptrAccess & KARG_PTR_VOLATILE) != 0);
            argEntry.isRestrict = ((arg.ptrAccess & KARG_PTR_RESTRICT) != 0);
            argEntry.unknown4 = 0;
            argEntry.isPipe = (arg.argType==KernelArgType::PIPE);
            
            uint32_t kindOfType;
            if (arg.argType==KernelArgType::SAMPLER)
                kindOfType = 1;
            else if (isImage)
                kindOfType = 2;
            else if (isPointerOrPipe)
                kindOfType = 5;
            else if (arg.argType==KernelArgType::CMDQUEUE)
                kindOfType = 7;
            else // otherwise
                kindOfType = 4;
            SLEV(argEntry.kindOfType, kindOfType);
            SLEV(argEntry.unknown5, 0);
            fob.writeObject(argEntry);
        }
        fob.fill(88, 0); // NULL arg
        
        // arg names and type names
        for (const AmdKernelArgInput& arg: config.args)
        {
            fob.writeArray(arg.argName.size()+1, arg.argName.c_str());
            fob.writeArray(arg.typeName.size()+1, arg.typeName.c_str());
        }
        fob.fill(48, 0);
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        for (size_t i = 0; i < input->kernels.size(); i++)
        {
            const AmdCL2KernelInput& kernel = input->kernels[i];
            if (kernel.useConfig)
                writeMetadata(i, tempDatas[i], kernel.config, fob);
            else
                fob.writeArray(kernel.metadataSize, kernel.metadata);
        }
        if (!newBinaries)
            for (const AmdCL2KernelInput& kernel: input->kernels)
            {
                if (kernel.isaMetadata!=nullptr)
                    fob.writeArray(kernel.isaMetadataSize, kernel.isaMetadata);
                else // default values
                    fob.writeArray(sizeof(kernelIsaMetadata), kernelIsaMetadata);
            }
    }
};

static const cxbyte kernelSetupBytesAfter8[40] =
{
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct CLRX_INTERNAL IntAmdCL2SetupData
{
    uint32_t pgmRSRC1;
    uint32_t pgmRSRC2;
    uint16_t setup1;
    uint16_t archInd;
    uint32_t scratchBufferSize;
    uint32_t localSize; // in bytes
    uint32_t zero1;
    uint32_t kernelArgsSize;
    uint32_t zeroes[2];
    uint16_t sgprsNumAll;
    uint16_t vgprsNum16;
    uint32_t vgprsNum;
    uint32_t sgprsNum;
    uint32_t zero3;
    uint32_t setup2; // ??
};

static uint32_t calculatePgmRSRC2(const AmdCL2KernelConfig& config,
                  bool storeLocalSize = false)
{
    uint32_t dimValues = 0;
    if (config.dimMask != BINGEN_DEFAULT)
    {
        dimValues = ((config.dimMask&7)<<7);
        if (!config.useEnqueue)
            dimValues |= (((config.dimMask&4) ? 2 : (config.dimMask&2) ? 1 : 0)<<11);
        else // enqueue needs TIDIG_COMP_CNT=2 ????
            dimValues |= (2U<<11);
    }
    else
        dimValues |= (config.pgmRSRC2 & 0x1b80U);
    
    const uint32_t localPart = (storeLocalSize) ? (((config.localSize+511)>>9)<<15) : 0;
    
    cxuint userDatasNum = 4;
    if (config.useEnqueue)
        userDatasNum = 10;
    else if (config.useSetup)
        userDatasNum = ((config.useSizes) ? 8 : 6);
    else if (config.useSizes)
        userDatasNum = 8;
    return (config.pgmRSRC2 & 0xffffe440U) | (userDatasNum<<1) |
            ((config.tgSize) ? 0x400 : 0) | ((config.scratchBufferSize)?1:0) | dimValues |
            (uint32_t(config.exceptions)<<24) | localPart;
}

static void generateKernelSetup(GPUArchitecture arch, const AmdCL2KernelConfig& config,
                FastOutputBuffer& fob, bool newBinaries, bool useLocals, bool usePipes)
{
    fob.writeObject<uint64_t>(LEV(uint64_t(newBinaries ? 0x100000001ULL : 1ULL)));
    fob.writeArray(40, kernelSetupBytesAfter8);
    IntAmdCL2SetupData setupData;
    const cxuint neededExtraSGPRsNum = arch==GPUArchitecture::GCN1_2 ? 4 : 2;
    const cxuint extraSGPRsNum = (config.useEnqueue) ? neededExtraSGPRsNum : 0;
    cxuint sgprsNum = std::max(config.usedSGPRsNum + extraSGPRsNum + 2, 1U);
    cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
    // pgmrsrc1
    SLEV(setupData.pgmRSRC1, config.pgmRSRC1 | ((vgprsNum-1)>>2) |
            (((sgprsNum-1)>>3)<<6) | ((uint32_t(config.floatMode)&0xff)<<12) |
            (newBinaries ? (1U<<21) : 0) /*dx11_clamp */ |
            (config.ieeeMode?1U<<23:0) | (uint32_t(config.priority&3)<<10) |
            (config.privilegedMode?1U<<20:0) | (config.dx10Clamp?1U<<21:0) |
            (config.debugMode?1U<<22:0));
    // pgmrsrc2 - without ldssize
    uint32_t dimValues = 0;
    if (config.dimMask != BINGEN_DEFAULT)
    {
        dimValues = ((config.dimMask&7)<<7);
        if (!config.useEnqueue)
            dimValues |= (((config.dimMask&4) ? 2 : (config.dimMask&2) ? 1 : 0)<<11);
        else // enqueue needs TIDIG_COMP_CNT=2 ????
            dimValues |= (2U<<11);
    }
    else
        dimValues |= (config.pgmRSRC2 & 0x1b80U);
    
    uint16_t setup1 = 0x1;
    if (config.useEnqueue)
        setup1 = 0x2b;
    else if (config.useSetup)
        setup1 = (config.useSizes) ? 0xb : 0x9;
    else if (config.useSizes)
        setup1 = 0xb;
    
    SLEV(setupData.pgmRSRC2, calculatePgmRSRC2(config));
    
    SLEV(setupData.setup1, setup1);
    SLEV(setupData.archInd, (arch==GPUArchitecture::GCN1_2 && newBinaries) ? 0x4a : 0x0a);
    SLEV(setupData.scratchBufferSize, config.scratchBufferSize);
    SLEV(setupData.localSize, config.localSize);
    setupData.zero1 = 0;
    setupData.zeroes[0] = setupData.zeroes[1] = 0;
    setupData.zero3 = 0;
    
    cxuint kernelArgSize = 0;
    for (const AmdKernelArgInput arg: config.args)
    {
        if (arg.argType == KernelArgType::POINTER || arg.argType == KernelArgType::PIPE ||
            arg.argType == KernelArgType::CLKEVENT ||
            arg.argType == KernelArgType::STRUCTURE ||
            arg.argType == KernelArgType::CMDQUEUE ||
            arg.argType == KernelArgType::SAMPLER ||
            (arg.argType >= KernelArgType::MIN_IMAGE &&
             arg.argType <= KernelArgType::MAX_IMAGE))
        {
            if ((kernelArgSize&7)!=0)    // alignment
                kernelArgSize += 8-(kernelArgSize&7);
            kernelArgSize += 8;
        }
        else
        {   // scalar
            const ArgTypeSizes& argTypeSizes = argTypeSizesTable[cxuint(arg.argType)];
            cxuint vectorLength = argTypeSizes.vectorSize;
            if (newBinaries && vectorLength==3)
                vectorLength = 4;
            if ((kernelArgSize & (argTypeSizes.elemSize-1))!=0)
                kernelArgSize += argTypeSizes.elemSize -
                        (kernelArgSize & (argTypeSizes.elemSize-1));
            kernelArgSize += vectorLength * argTypeSizes.elemSize;
        }
    }
    if (newBinaries)
        kernelArgSize = (kernelArgSize+15)&~15U;
    SLEV(setupData.kernelArgsSize, kernelArgSize);
    SLEV(setupData.sgprsNumAll, sgprsNum);
    SLEV(setupData.vgprsNum16, config.usedVGPRsNum);
    SLEV(setupData.vgprsNum, config.usedVGPRsNum);
    SLEV(setupData.sgprsNum, config.usedSGPRsNum);
    if (newBinaries)
        SLEV(setupData.setup2, 0x06040404U);
    else // old binaries
    {
        uint32_t extraBits = (config.useEnqueue) ? 0x30000U : 0;
        extraBits |= (!config.useEnqueue && config.scratchBufferSize!=0) ? 0x40000U : 0;
        extraBits |= (config.localSize!=0) ? 0x200U : 0;
        extraBits |= (usePipes && (extraBits&0x40000U)==0) ? 0x30000U : 0;
        SLEV(setupData.setup2, 0x06000003U | extraBits);
    }
    
    fob.writeObject(setupData);
    fob.fill(256 - sizeof(IntAmdCL2SetupData) - 48, 0);
}

struct CLRX_INTERNAL IntAmdCL2StubHeader
{
    uint32_t hsaTextOffset;
    uint32_t instrsNum;
    uint32_t vgprsNum;
    uint32_t zeroes[6];
    uint32_t sizeProgVal; // 0x24
    uint32_t globalMemOps;
    uint32_t localMemOps;
    uint32_t zero2;
    uint32_t programRegSize; // sum??
    uint32_t zero3;
    uint32_t sgprsNumAll;
};

static const bool gcnSize11Table[16] =
{
    false, // GCNENC_SMRD, // 0000
    false, // GCNENC_SMRD, // 0001
    false, // GCNENC_VINTRP, // 0010
    false, // GCNENC_NONE, // 0011 - illegal
    true,  // GCNENC_VOP3A, // 0100
    false, // GCNENC_NONE, // 0101 - illegal
    true,  // GCNENC_DS,   // 0110
    true,  // GCNENC_FLAT, // 0111
    true,  // GCNENC_MUBUF, // 1000
    false, // GCNENC_NONE,  // 1001 - illegal
    true,  // GCNENC_MTBUF, // 1010
    false, // GCNENC_NONE,  // 1011 - illegal
    true,  // GCNENC_MIMG,  // 1100
    false, // GCNENC_NONE,  // 1101 - illegal
    true,  // GCNENC_EXP,   // 1110
    false  // GCNENC_NONE   // 1111 - illegal
};

static const bool gcnSize12Table[16] =
{
    true,  // GCNENC_SMEM, // 0000
    true,  // GCNENC_EXP, // 0001
    false, // GCNENC_NONE, // 0010 - illegal
    false, // GCNENC_NONE, // 0011 - illegal
    true,  // GCNENC_VOP3A, // 0100
    false, // GCNENC_VINTRP, // 0101
    true,  // GCNENC_DS,   // 0110
    true,  // GCNENC_FLAT, // 0111
    true,  // GCNENC_MUBUF, // 1000
    false, // GCNENC_NONE,  // 1001 - illegal
    true,  // GCNENC_MTBUF, // 1010
    false, // GCNENC_NONE,  // 1011 - illegal
    true,  // GCNENC_MIMG,  // 1100
    false, // GCNENC_NONE,  // 1101 - illegal
    false, // GCNENC_NONE,  // 1110 - illegal
    false  // GCNENC_NONE   // 1111 - illegal
};

enum : cxbyte
{
    INSTRTYPE_OTHER = 0,
    INSTRTYPE_GLOBAL,
    INSTRTYPE_LOCAL,
};

static const cxbyte gcnEncInstrTable[16] =
{
    INSTRTYPE_OTHER, // 0000
    INSTRTYPE_OTHER, // 0001
    INSTRTYPE_OTHER, // 0010
    INSTRTYPE_OTHER, // 0011 - illegal
    INSTRTYPE_OTHER, // 0100
    INSTRTYPE_OTHER, // 0101 - illegal
    INSTRTYPE_LOCAL,  // 0110
    INSTRTYPE_GLOBAL, // 0111
    INSTRTYPE_GLOBAL, // 1000
    INSTRTYPE_OTHER,  // 1001 - illegal
    INSTRTYPE_GLOBAL, // 1010
    INSTRTYPE_OTHER,  // 1011 - illegal
    INSTRTYPE_GLOBAL, // 1100
    INSTRTYPE_OTHER,  // 1101 - illegal
    INSTRTYPE_OTHER,  // 1110
    INSTRTYPE_OTHER // 1111 - illegal
};

/* count number of instructions, local memory operations and global memory operations */

static void analyzeCode(GPUArchitecture arch, size_t codeSize, const cxbyte* code,
            IntAmdCL2StubHeader& stubHdr)
{
    uint32_t instrsNum = 0;
    uint32_t globalMemOps = 0;
    uint32_t localMemOps = 0;
    const size_t codeWordsNum = codeSize>>2;
    const uint32_t* codeWords = reinterpret_cast<const uint32_t*>(code);
    bool isGCN12 = (arch == GPUArchitecture::GCN1_2);
    bool isGCN11 = (arch == GPUArchitecture::GCN1_1);
    
    /* main analyzing code loop, parse and determine instr encoding, and counts
     * global/local memory ops */
    for (size_t pos = 0; pos < codeWordsNum; instrsNum++)
    {
        uint32_t insnCode = ULEV(codeWords[pos++]);
        
        if ((insnCode & 0x80000000U) != 0)
        {
            if ((insnCode & 0x40000000U) == 0)
            {   // SOP???
                if  ((insnCode & 0x30000000U) == 0x30000000U)
                {   // SOP1/SOPK/SOPC/SOPP
                    const uint32_t encPart = (insnCode & 0x0f800000U);
                    if (encPart == 0x0e800000U)
                    {   // SOP1
                        if ((insnCode&0xff) == 0xff) // literal
                        {
                            if (pos < codeWordsNum) pos++;
                        }
                    }
                    else if (encPart == 0x0f000000U)
                    {   // SOPC
                        if ((insnCode&0xff) == 0xff ||
                            (insnCode&0xff00) == 0xff00) // literal
                            if (pos < codeWordsNum) pos++;
                    }
                    else if (encPart != 0x0f800000U) // no SOPP
                    {
                        const uint32_t opcode = ((insnCode>>23)&0x1f);
                        if ((!isGCN12 && opcode == 21) ||
                            (isGCN12 && opcode == 20))
                            if (pos < codeWordsNum) pos++;
                    }
                }
                else
                {   // SOP2
                    if ((insnCode&0xff) == 0xff || (insnCode&0xff00) == 0xff00)
                        // literal
                        if (pos < codeWordsNum) pos++;
                }
            }
            else
            {   // SMRD and others
                const uint32_t encPart = (insnCode&0x3c000000U)>>26;
                if ((!isGCN12 && gcnSize11Table[encPart] && (encPart != 7 || isGCN11)) ||
                    (isGCN12 && gcnSize12Table[encPart]))
                {
                    if (pos < codeWordsNum) pos++;
                }
                cxbyte instrType = gcnEncInstrTable[encPart];
                if (instrType == INSTRTYPE_LOCAL)
                    localMemOps++;
                if (instrType == INSTRTYPE_GLOBAL)
                    globalMemOps++;
            }
        }
        else
        {   // some vector instructions
            if ((insnCode & 0x7e000000U) == 0x7c000000U)
            {   // VOPC
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                {
                    if (pos < codeWordsNum) pos++;
                }
            }
            else if ((insnCode & 0x7e000000U) == 0x7e000000U)
            {   // VOP1
                if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                    if (pos < codeWordsNum) pos++;
            }
            else
            {   // VOP2
                const cxuint opcode = (insnCode >> 25)&0x3f;
                if ((!isGCN12 && (opcode == 32 || opcode == 33)) ||
                    (isGCN12 && (opcode == 23 || opcode == 24 ||
                    opcode == 36 || opcode == 37))) // V_MADMK and V_MADAK
                {
                    if (pos < codeWordsNum) pos++;
                }
                else if ((insnCode&0x1ff) == 0xff || // literal
                    // SDWA, DDP
                    (isGCN12 && ((insnCode&0x1ff) == 0xf9 || (insnCode&0x1ff) == 0xfa)))
                    if (pos < codeWordsNum) pos++; 
            }
        }
    }
    
    SLEV(stubHdr.instrsNum, instrsNum);
    SLEV(stubHdr.localMemOps, localMemOps);
    SLEV(stubHdr.globalMemOps, globalMemOps);
}

struct CLRX_INTERNAL IntAmdCL2StubEnd
{
    uint64_t hsaTextOffset;
    uint32_t endSize;
    uint32_t hsaTextSize;
    uint32_t zeroes[2];
    uint32_t unknown1; // value - 0x200
    uint32_t unknown2;
    uint64_t kernelSize;    // 0x20
    uint32_t zeroesx[2];
    uint64_t kernelSize2;
    uint32_t vgprsNum;      // 0x30
    uint32_t sgprsNumAll;
    uint32_t zeroes2[2];
    uint32_t vgprsNum2;
    uint32_t sgprsNum;
    uint32_t floatMode; // 0x50
    uint32_t unknown3;
    uint32_t one; //??
    uint32_t zeroes3[3];    
    uint32_t scratchBufferSize; // 0x68
    uint32_t localSize;
    uint32_t allOnes;// 0x70
    uint32_t unknownlast; // 0x74 (alignment)
};

static void generateKernelStub(GPUArchitecture arch, const AmdCL2KernelConfig& config,
        FastOutputBuffer& fob, size_t codeSize, const cxbyte* code, bool useLocals,
        bool usePipes)
{
    const cxuint neededExtraSGPRsNum = arch==GPUArchitecture::GCN1_2 ? 4 : 2;
    const cxuint extraSGPRsNum = (config.useEnqueue) ? neededExtraSGPRsNum : 0;
    cxuint sgprsNumAll = config.usedSGPRsNum+2 + extraSGPRsNum;
    {
        IntAmdCL2StubHeader stubHdr;
        SLEV(stubHdr.hsaTextOffset, 0xa60);
        SLEV(stubHdr.instrsNum, 0xa60);
        SLEV(stubHdr.sgprsNumAll, sgprsNumAll);
        SLEV(stubHdr.vgprsNum, config.usedVGPRsNum);
        analyzeCode(arch, codeSize, code, stubHdr);
        std::fill(stubHdr.zeroes, stubHdr.zeroes+6, uint32_t(0));
        stubHdr.zero2 = 0;
        stubHdr.zero3 = 0;
        stubHdr.sizeProgVal = stubHdr.instrsNum; // this same? enough reliable?
        SLEV(stubHdr.programRegSize, config.usedSGPRsNum + config.usedVGPRsNum);
        fob.writeObject(stubHdr);
    }
    // next bytes
    fob.fill(0xb8 - sizeof(IntAmdCL2StubHeader), 0); // fill up
    fob.writeObject(LEV(0xa60));
    fob.fill(0x164-0xbc, 0); // fill up
    // 0x164
    fob.writeObject(LEV(3)); //?
    fob.writeObject(LEV(config.localSize!=0 ? 13 : 12)); //?
    fob.fill(0x9a0-0x16c, 0); // fill up
    { // end of stub - kernel config?
        IntAmdCL2StubEnd stubEnd;
        SLEV(stubEnd.hsaTextOffset, 0xa60);
        SLEV(stubEnd.endSize, 0x100);
        SLEV(stubEnd.hsaTextSize, codeSize + 0x100);
        stubEnd.zeroes[0] = stubEnd.zeroes[1] = 0;
        SLEV(stubEnd.unknown1, 0x200);
        stubEnd.unknown2 = 0;
        SLEV(stubEnd.kernelSize, codeSize + 0xb60);
        stubEnd.zeroesx[0] = stubEnd.zeroesx[1] = 0;
        SLEV(stubEnd.kernelSize2, codeSize + 0xb60);
        SLEV(stubEnd.vgprsNum, config.usedVGPRsNum);
        SLEV(stubEnd.sgprsNumAll, sgprsNumAll);
        SLEV(stubEnd.vgprsNum2, config.usedVGPRsNum);
        stubEnd.zeroes2[0] = stubEnd.zeroes2[1] = 0;
        SLEV(stubEnd.sgprsNum, config.usedSGPRsNum);
        SLEV(stubEnd.floatMode, config.floatMode&0xff);
        stubEnd.unknown3 = 0;
        SLEV(stubEnd.one, 1);
        stubEnd.zeroes3[0] = stubEnd.zeroes3[1] = stubEnd.zeroes3[2] = 0;
        SLEV(stubEnd.scratchBufferSize, (config.scratchBufferSize+3)>>2);
        SLEV(stubEnd.localSize, config.localSize);
        SLEV(stubEnd.allOnes, 0xffffffffU);
        SLEV(stubEnd.unknownlast, 0);
        fob.writeObject(stubEnd);
    }
    fob.fill(0xa8-sizeof(IntAmdCL2StubEnd), 0);
    fob.writeObject(LEV(calculatePgmRSRC2(config, true)));
    fob.fill(0xc0-0xac, 0);
}

class CLRX_INTERNAL CL2MainTextGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    const Array<TempAmdCL2KernelData>& tempDatas;
    ElfBinaryGen64* innerBinGen;
public:
    explicit CL2MainTextGen(const AmdCL2Input* _input,
            const Array<TempAmdCL2KernelData>& _tempDatas,
            ElfBinaryGen64* _innerBinGen) : input(_input), tempDatas(_tempDatas),
            innerBinGen(_innerBinGen)
    { }
    
    size_t size() const
    {
        if (innerBinGen)
            return innerBinGen->countSize();
        size_t out = 0;
        for (const TempAmdCL2KernelData tempData: tempDatas)
            out += tempData.stubSize + tempData.setupSize + tempData.codeSize;
        return out;
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        if (innerBinGen!=nullptr)
            innerBinGen->generate(fob);
        else // otherwise (old binaries)
        {
            GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
            for (size_t i = 0; i < input->kernels.size(); i++)
            {
                const AmdCL2KernelInput& kernel = input->kernels[i];
                const TempAmdCL2KernelData& tempData = tempDatas[i];
                if (!kernel.useConfig)
                {   // no configuration, get from kernel data
                    fob.writeArray(tempData.stubSize, kernel.stub);
                    fob.writeArray(tempData.setupSize, kernel.setup);
                }
                else // generate stub, setup from kernel config
                {
                    generateKernelStub(arch, kernel.config, fob, tempData.codeSize,
                               kernel.code, tempData.useLocals, tempData.pipesUsed!=0);
                    generateKernelSetup(arch, kernel.config, fob, false,
                                tempData.useLocals, tempData.pipesUsed!=0);
                }
                fob.writeArray(kernel.codeSize, kernel.code);
            }
        }
    }
};

class CLRX_INTERNAL CL2InnerTextGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    const Array<TempAmdCL2KernelData>& tempDatas;
public:
    explicit CL2InnerTextGen(const AmdCL2Input* _input,
                const Array<TempAmdCL2KernelData>& _tempDatas) : input(_input),
                tempDatas(_tempDatas)
    { }
    
    size_t size() const
    {
        size_t out = 0;
        for (const TempAmdCL2KernelData& tempData: tempDatas)
        {
            if ((out & 255) != 0)
                out += 256-(out&255);
            out += tempData.setupSize + tempData.codeSize;
        }
        return out;
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
        size_t outSize = 0;
        for (size_t i = 0; i < input->kernels.size(); i++)
        {
            const AmdCL2KernelInput& kernel = input->kernels[i];
            const TempAmdCL2KernelData& tempData = tempDatas[i];
            if ((outSize & 255) != 0)
            {
                size_t toFill = 256-(outSize&255);
                fob.fill(toFill, 0);
                outSize += toFill;
            }
            if (!kernel.useConfig)
                fob.writeArray(tempData.setupSize, kernel.setup);
            else
                generateKernelSetup(arch, kernel.config, fob, true, tempData.useLocals,
                            tempData.pipesUsed!=0);
            fob.writeArray(tempData.codeSize, kernel.code);
            outSize += tempData.setupSize + tempData.codeSize;
        }
    }
};

class CLRX_INTERNAL CL2InnerSamplerInitGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
public:
    explicit CL2InnerSamplerInitGen(const AmdCL2Input* _input) : input(_input)
    { }
    
    void operator()(FastOutputBuffer& fob) const
    {
        if (input->samplerConfig)
        {
            uint32_t sampDef[2];
            for (uint32_t sampler: input->samplers)
            {
                SLEV(sampDef[0], 0x10008);
                SLEV(sampDef[1], sampler);
                fob.writeArray(2, sampDef);
            }
        }
        else
            fob.writeArray(input->samplerInitSize, input->samplerInit);
    }
};

class CLRX_INTERNAL CL2InnerGlobalDataGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
public:
    explicit CL2InnerGlobalDataGen(const AmdCL2Input* _input) : input(_input)
    { }
    
    size_t size() const
    {
        if (input->driverVersion >= 200406)
            return input->globalDataSize;
        size_t rwDataSize = (input->rwData!=nullptr) ? input->rwDataSize : 0;
        size_t allSize = (input->globalDataSize + input->bssSize +
                rwDataSize + 255) & ~size_t(255);
        return allSize - rwDataSize - input->bssSize;
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        size_t gdataSize = size();
        fob.writeArray(input->globalDataSize, input->globalData);
        if (gdataSize > input->globalDataSize)
            fob.fill(gdataSize - input->globalDataSize, 0);
    }
};

class CLRX_INTERNAL CL2InnerGlobalDataRelsGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    size_t dataSymbolsNum;
public:
    explicit CL2InnerGlobalDataRelsGen(const AmdCL2Input* _input, size_t _dataSymNum)
            : input(_input), dataSymbolsNum(_dataSymNum)
    { }
    
    size_t size() const
    {
        size_t out = (input->samplerConfig) ? input->samplers.size() :
                (input->samplerInitSize>>3);
        return out*sizeof(Elf64_Rela);
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        Elf64_Rela rela;
        rela.r_addend = 0;
        const size_t samplersNum = (input->samplerConfig) ?
                input->samplers.size() : (input->samplerInitSize>>3);
        /* calculate first symbol for samplers (last symbols) */
        uint32_t symIndex = input->kernels.size() + samplersNum + dataSymbolsNum + 2 +
                (input->rwDataSize!=0 && input->rwData!=nullptr) /* globaldata symbol */ +
                (input->bssSize!=0) /* bss data symbol */ +
                (input->driverVersion>=200406);
        if (!input->samplerOffsets.empty())
            for (size_t sampOffset: input->samplerOffsets)
            {
                SLEV(rela.r_offset, sampOffset);
                SLEV(rela.r_info, ELF64_R_INFO(symIndex, 4U));
                fob.writeObject(rela);
                symIndex++;
            }
        else // default in last bytes
        {
            size_t globalOffset = input->globalDataSize - (samplersNum<<3);
            globalOffset &= ~size_t(7); // alignment
            for (size_t i = 0; i < samplersNum; i++)
            {
                SLEV(rela.r_offset, globalOffset + 8*i);
                SLEV(rela.r_info, ELF64_R_INFO(symIndex+i, 4U));
                fob.writeObject(rela);
            }
        }
    }
};

class CLRX_INTERNAL CL2InnerTextRelsGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    const Array<TempAmdCL2KernelData>& tempDatas;
    size_t dataSymbolsNum;
public:
    explicit CL2InnerTextRelsGen(const AmdCL2Input* _input,
            const Array<TempAmdCL2KernelData>& _tempDatas, size_t _dataSymNum) :
            input(_input), tempDatas(_tempDatas), dataSymbolsNum(_dataSymNum)
    { }
    
    size_t size() const
    {
        size_t out = 0;
        for (const AmdCL2KernelInput& kernel: input->kernels)
            out += kernel.relocations.size()*sizeof(Elf64_Rela);
        return out;
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        Elf64_Rela rela;
        size_t codeOffset = 0;
        uint32_t adataSymIndex = 0;
        cxuint samplersNum = (input->samplerConfig) ?
                input->samplers.size() : (input->samplerInitSize>>3);
        uint32_t gdataSymIndex = input->kernels.size() + samplersNum + dataSymbolsNum +
            (input->driverVersion>=200406);
        uint32_t bssSymIndex = input->kernels.size() + samplersNum + dataSymbolsNum +
            (input->driverVersion>=200406);
        if (input->rwDataSize!=0 && input->rwData!=nullptr)
        {   // atomic data available
            adataSymIndex = gdataSymIndex; // first is atomic data symbol index
            gdataSymIndex++;
        }
        if (input->bssSize!=0)
        {   // bss section available
            bssSymIndex = gdataSymIndex; // first is bss data symbol index
            gdataSymIndex++;
        }
        for (size_t i = 0; i < input->kernels.size(); i++)
        {
            const AmdCL2KernelInput& kernel = input->kernels[i];
            const TempAmdCL2KernelData& tempData = tempDatas[i];
            
            codeOffset += tempData.setupSize;
            for (const AmdCL2RelInput inRel: kernel.relocations)
            {
                SLEV(rela.r_offset, inRel.offset + codeOffset);
                uint32_t type = (inRel.type==RELTYPE_LOW_32BIT) ? 1 : 2;
                uint32_t symIndex = (inRel.symbol==1) ? adataSymIndex : 
                    ((inRel.symbol==2) ? bssSymIndex: gdataSymIndex);
                SLEV(rela.r_info, ELF64_R_INFO(symIndex, type));
                SLEV(rela.r_addend, inRel.addend);
                fob.writeObject(rela);
            }
            codeOffset += (kernel.codeSize+255)&~size_t(255);
        }
    }
};

/* NT_VERSION (0x1): size: 8, value: 0x1
 * NT_ARCH (0x2): size: 12, dwords: 0x1, 0x0, 0x010101
 * 0x5: size: 25, string: \x16\000-hsa_call_convention=\0\0
 * 0x3: size: 30, string: (version???)
 * "\4\0\7\0\7\0\0\0\0\0\0\0\0\0\0\0AMD\0AMDGPU\0\0\0\0"
 * 0x4: size: 4, random 64-bit value: 0x7ffXXXXXXXXX */
static const cxbyte noteSectionData[168] =
{
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x19, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x41, 0x4d, 0x44, 0x00, 0x16, 0x00, 0x2d, 0x68,
    0x73, 0x61, 0x5f, 0x63, 0x61, 0x6c, 0x6c, 0x5f,
    0x63, 0x6f, 0x6e, 0x76, 0x65, 0x6e, 0x74, 0x69,
    0x6f, 0x6e, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0x04, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x41, 0x4d, 0x44, 0x00, 0x41, 0x4d, 0x44, 0x47,
    0x50, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0xf0, 0x83, 0x17, 0xfb, 0xfc, 0x7f, 0x00, 0x00
};

static const cxbyte noteSectionData16_3[200] =
{
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x1a, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x41, 0x4d, 0x44, 0x00, 0x04, 0x00, 0x07, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0x41, 0x4d, 0x44, 0x47, 0x50, 0x55, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x00,
    0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0x4d, 0x44, 0x20,
    0x48, 0x53, 0x41, 0x20, 0x52, 0x75, 0x6e, 0x74,
    0x69, 0x6d, 0x65, 0x20, 0x46, 0x69, 0x6e, 0x61,
    0x6c, 0x69, 0x7a, 0x65, 0x72, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x1a, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x41, 0x4d, 0x44, 0x00, 0x16, 0x00, 0x2d, 0x68,
    0x73, 0x61, 0x5f, 0x63, 0x61, 0x6c, 0x6c, 0x5f,
    0x63, 0x6f, 0x6e, 0x76, 0x65, 0x6e, 0x74, 0x69,
    0x6f, 0x6e, 0x3d, 0x30, 0x00, 0x00, 0x00, 0x00,
};

static CString constructName(size_t prefixSize, const char* prefix, const CString& name,
                 size_t suffixSize, const char* suffix)
{
    const size_t nameLen = name.size();
    CString out(prefixSize + suffixSize + nameLen);
    char* outPtr = out.begin();
    std::copy(prefix, prefix+prefixSize, outPtr);
    std::copy(name.begin(), name.begin()+nameLen, outPtr+prefixSize);
    std::copy(suffix, suffix+suffixSize, outPtr+prefixSize+nameLen);
    return out;
}

static void putInnerSymbols(ElfBinaryGen64& innerBinGen, const AmdCL2Input* input,
        const Array<TempAmdCL2KernelData>& tempDatas, const uint16_t* builtinSectionTable,
        cxuint extraSeciontIndex, std::vector<CString>& stringPool, size_t dataSymbolsNum)
{
    const size_t samplersNum = (input->samplerConfig) ? input->samplers.size() :
                (input->samplerInitSize>>3);
    // put kernel symbols
    std::vector<bool> samplerMask(samplersNum);
    size_t samplerOffset = input->globalDataSize - samplersNum;
    size_t codePos = 0;
    const uint16_t textSectId = builtinSectionTable[ELFSECTID_TEXT-ELFSECTID_START];
    const uint16_t globalSectId = builtinSectionTable[ELFSECTID_RODATA-ELFSECTID_START];
    const uint16_t atomicSectId = builtinSectionTable[
                ELFSECTID_DATA-ELFSECTID_START];
    const uint16_t sampInitSectId = builtinSectionTable[
                AMDCL2SECTID_SAMPLERINIT-ELFSECTID_START];
    const uint16_t bssSectId = builtinSectionTable[ELFSECTID_BSS-ELFSECTID_START];
    stringPool.resize(input->kernels.size() + samplersNum + dataSymbolsNum);
    size_t nameIdx = 0;
    
    /* put data symbols */
    for (const BinSymbol& symbol: input->innerExtraSymbols)
        if (symbol.sectionId==ELFSECTID_RODATA || symbol.sectionId==ELFSECTID_DATA ||
                   symbol.sectionId==ELFSECTID_BSS)
        {
            stringPool[nameIdx] = constructName(12, "&input_bc::&", symbol.name,
                        0, nullptr);
            innerBinGen.addSymbol(ElfSymbol64(stringPool[nameIdx].c_str(),
                  convertSectionId(symbol.sectionId, builtinSectionTable, AMDCL2SECTID_MAX,
                           extraSeciontIndex),
                  ELF64_ST_INFO(STB_LOCAL, STT_OBJECT), 0, false, symbol.value, 
                  symbol.size));
            nameIdx++;
        }
    for (size_t i = 0; i < input->kernels.size(); i++)
    {   // first, we put sampler objects
        const AmdCL2KernelInput& kernel = input->kernels[i];
        const TempAmdCL2KernelData& tempData = tempDatas[i];
        if ((codePos & 255) != 0)
            codePos += 256-(codePos&255);
        
        if (kernel.useConfig)
            for (cxuint samp: kernel.config.samplers)
            {
                if (samplerMask[samp])
                    continue; // if added to symbol table
                const uint64_t value = !input->samplerOffsets.empty() ?
                        input->samplerOffsets[samp] : samplerOffset + samp*8;
                char sampName[64];
                memcpy(sampName, "&input_bc::&_.Samp", 18);
                itocstrCStyle<cxuint>(samp, sampName+18, 64-18);
                stringPool[nameIdx] = sampName;
                innerBinGen.addSymbol(ElfSymbol64(stringPool[nameIdx].c_str(), globalSectId,
                          ELF64_ST_INFO(STB_LOCAL, STT_OBJECT), 0, false, value, 8));
                nameIdx++;
                samplerMask[samp] = true;
            }
        // put kernel symbol
        stringPool[nameIdx] = constructName(10, "&__OpenCL_", kernel.kernelName,
                        7, "_kernel");
        
        innerBinGen.addSymbol(ElfSymbol64(stringPool[nameIdx].c_str(), textSectId,
                  ELF64_ST_INFO(STB_GLOBAL, 10), 0, false, codePos, 
                  kernel.codeSize + tempData.setupSize));
        nameIdx++;
        codePos += kernel.codeSize + tempData.setupSize;
    }
    
    for (size_t i = 0; i < samplersNum; i++)
        if (!samplerMask[i])
        {
            const uint64_t value = !input->samplerOffsets.empty() ?
                    input->samplerOffsets[i] : samplerOffset + i*8;
            char sampName[64];
            memcpy(sampName, "&input_bc::&_.Samp", 18);
            itocstrCStyle<cxuint>(i, sampName+18, 64-18);
            stringPool[nameIdx] = sampName;
            innerBinGen.addSymbol(ElfSymbol64(stringPool[nameIdx].c_str(), globalSectId,
                      ELF64_ST_INFO(STB_LOCAL, STT_OBJECT), 0, false, value, 8));
            nameIdx++;
            samplerMask[i] = true;
        }
    
    if (input->rwDataSize!=0 && input->rwData!=nullptr)
        innerBinGen.addSymbol(ElfSymbol64("__hsa_section.hsadata_global_agent",
              atomicSectId, ELF64_ST_INFO(STB_LOCAL, STT_SECTION), 0, false, 0, 0));
    if (input->bssSize!=0)
        innerBinGen.addSymbol(ElfSymbol64("__hsa_section.hsabss_global_agent",
              bssSectId, ELF64_ST_INFO(STB_LOCAL, STT_SECTION), 0, false, 0, 0));
    if (input->globalDataSize!=0 && input->globalData!=nullptr)
        innerBinGen.addSymbol(ElfSymbol64("__hsa_section.hsadata_readonly_agent",
              globalSectId, ELF64_ST_INFO(STB_LOCAL, STT_SECTION), 0, false, 0, 0));
    innerBinGen.addSymbol(ElfSymbol64("__hsa_section.hsatext", textSectId,
              ELF64_ST_INFO(STB_LOCAL, STT_SECTION), 0, false, 0, 0));
    
    for (size_t i = 0; i < samplersNum; i++)
        innerBinGen.addSymbol(ElfSymbol64("", sampInitSectId, ELF64_ST_INFO(STB_LOCAL, 12),
                      0, false, i*8, 0));
    /// add extra inner symbols
    for (const BinSymbol& extraSym: input->innerExtraSymbols)
        innerBinGen.addSymbol(ElfSymbol64(extraSym, builtinSectionTable, AMDCL2SECTID_MAX,
                                  extraSeciontIndex));
}

/// main routine to generate OpenCL 2.0 binary
void AmdCL2GPUBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    const size_t kernelsNum = input->kernels.size();
    const bool newBinaries = input->driverVersion >= 191205;
    const bool hasSamplers = !input->samplerOffsets.empty() ||
                (!input->samplerConfig && input->samplerInitSize!=0 &&
                    input->samplerInit!=nullptr) ||
                (input->samplerConfig && !input->samplers.empty());
    const bool hasGlobalData = input->globalDataSize!=0 && input->globalData!=nullptr;
    const bool hasRWData = input->rwDataSize!=0 && input->rwData!=nullptr;
    
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
    if (arch == GPUArchitecture::GCN1_0)
        throw Exception("OpenCL 2.0 supported only for GCN1.1 or later");
    
    if ((hasGlobalData || hasRWData || hasSamplers) && !newBinaries)
        throw Exception("Old driver binaries doesn't support "
                        "global/atomic data or samplers");
    
    if (newBinaries)
    {
        if (!hasGlobalData && hasSamplers)
            throw Exception("Global data must be defined if samplers present");
        // check sampler offset range
        for (size_t sampOffset: input->samplerOffsets)
            if (sampOffset+8 > input->globalDataSize)
                throw Exception("Sampler offset outside global data");
    }
    /* check samplers */
    const cxuint samplersNum = (input->samplerConfig) ? input->samplers.size() :
                (input->samplerInitSize>>3);
    if (!input->samplerOffsets.empty() && input->samplerOffsets.size() != samplersNum)
        throw Exception("SamplerOffset number doesn't match to samplers number");
    
    for (size_t sampOffset: input->samplerOffsets)
        if ((sampOffset&7) != 0 && sampOffset >= input->globalDataSize)
            throw Exception("Wrong sampler offset (out of range of unaligned)");
    
    /* determine correct flags for device type */
    const uint32_t* deviceCodeTable;
    if (input->driverVersion < 191205)
        deviceCodeTable = gpuDeviceCodeTable15_7;
    else if (input->driverVersion < 200406)
        deviceCodeTable = gpuDeviceCodeTable;
    else if (input->driverVersion < 203603)
        deviceCodeTable = gpuDeviceCodeTable16_3;
    else // AMD GPUPRO driver
        deviceCodeTable = gpuDeviceCodeTableGPUPRO;
    // if GPU type is not supported by driver version
    if (deviceCodeTable[cxuint(input->deviceType)] == UINT_MAX)
        throw Exception("Unsupported GPU device type by driver version");
    
    ElfBinaryGen64 elfBinGen({ 0, 0, ELFOSABI_SYSV, 0, ET_EXEC, 0xaf5b, EV_CURRENT,
                UINT_MAX, 0, deviceCodeTable[cxuint(input->deviceType)] });
    
    CString aclVersion = input->aclVersion;
    if (aclVersion.empty())
    {
        if (newBinaries)
            aclVersion = "AMD-COMP-LIB-v0.8 (0.0.SC_BUILD_NUMBER)";
        else // old binaries
            aclVersion = "AMD-COMP-LIB-v0.8 (0.0.326)";
    }
    
    Array<TempAmdCL2KernelData> tempDatas(kernelsNum);
    prepareKernelTempData(input, tempDatas);
    
    const size_t dataSymbolsNum = std::count_if(input->innerExtraSymbols.begin(),
        input->innerExtraSymbols.end(), [](const BinSymbol& symbol)
        { return symbol.sectionId==ELFSECTID_RODATA || symbol.sectionId==ELFSECTID_DATA ||
                   symbol.sectionId==ELFSECTID_BSS; });
    
    cxuint mainExtraSectionIndex = 6 + (kernelsNum != 0 || newBinaries);
    CL2MainStrTabGen mainStrTabGen(input);
    CL2MainSymTabGen mainSymTabGen(input, tempDatas, aclVersion, mainExtraSectionIndex);
    CL2MainCommentGen mainCommentGen(input, aclVersion);
    CL2MainRodataGen mainRodataGen(input, tempDatas);
    CL2InnerTextGen innerTextGen(input, tempDatas);
    CL2InnerGlobalDataGen innerGDataGen(input);
    CL2InnerSamplerInitGen innerSamplerInitGen(input);
    CL2InnerTextRelsGen innerTextRelsGen(input, tempDatas, dataSymbolsNum);
    CL2InnerGlobalDataRelsGen innerGDataRels(input, dataSymbolsNum);
    
    // main section of main binary
    elfBinGen.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".shstrtab",
                    SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegion64(mainStrTabGen.size(), &mainStrTabGen, 1, ".strtab",
                    SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegion64(mainSymTabGen.size(), &mainSymTabGen, 8, ".symtab",
                    SHT_SYMTAB, 0));
    elfBinGen.addRegion(ElfRegion64(input->compileOptions.size()+aclVersion.size(),
                    &mainCommentGen, 1, ".comment", SHT_PROGBITS, 0));
    if (kernelsNum != 0)
        elfBinGen.addRegion(ElfRegion64(mainRodataGen.size(), &mainRodataGen, 1, ".rodata",
                    SHT_PROGBITS, SHF_ALLOC));
    
    std::unique_ptr<ElfBinaryGen64> innerBinGen;
    std::vector<CString> symbolNamePool;
    std::unique_ptr<cxbyte[]> noteBuf;
    if (newBinaries)
    {   // new binaries - .text holds inner ELF binaries
        bool is16_3Ver = (input->driverVersion>=200406);
        uint16_t innerBinSectionTable[innerBinSectonTableLen];
        cxuint extraSectionIndex = 1;
        /* check kernel text relocations */
        for (const AmdCL2KernelInput& kernel: input->kernels)
            for (const AmdCL2RelInput& rel: kernel.relocations)
                if (rel.offset >= kernel.codeSize)
                    throw Exception("Kernel text relocation offset outside kernel code");
        
        std::fill(innerBinSectionTable,
                  innerBinSectionTable+innerBinSectonTableLen, SHN_UNDEF);
        
        cxuint symtabId, globalDataId, textId;
        if (!is16_3Ver)
        {
            symtabId = 4 + (hasSamplers?2:0) /* samplerinit&rela.global */ +
                    (hasRWData) + (hasGlobalData) +
                    (input->bssSize!=0) +
                    (hasRWData || hasGlobalData || input->bssSize!=0) /* rela.hsatext */;
            globalDataId = 1 + (hasRWData) + (input->bssSize!=0);
            textId = 1 + (hasRWData) + (hasGlobalData) + (input->bssSize!=0);
        }
        else
        {
            textId = 4 + (hasRWData) + (hasGlobalData) + (input->bssSize!=0);
            symtabId = 5 + (hasRWData) + (hasGlobalData) + (input->bssSize!=0);
            globalDataId = 4 + (hasRWData) + (input->bssSize!=0);
        }
        /* in innerbin we do not add null symbol, we count address for program headers
         * and section from first section */
        innerBinGen.reset(new ElfBinaryGen64({ 0, 0, 0x40, 0, ET_REL, 0xe0, EV_CURRENT,
                        UINT_MAX, 0, 0 }, (input->driverVersion>=200406), true, true, 
                        /* globaldata sectionid: for 200406 - 4, for older - 1 */
                        (!is16_3Ver) ? 1 : 4));
        innerBinGen->addRegion(ElfRegion64::programHeaderTable());
        
        if (is16_3Ver)
        {   /* first is shstrab and strtab */
            innerBinGen->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8, ".shstrtab",
                                  SHT_STRTAB, SHF_STRINGS, 0, 0));
            innerBinSectionTable[ELFSECTID_SHSTRTAB-ELFSECTID_START] = extraSectionIndex++;
            innerBinGen->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 8, ".strtab",
                                  SHT_STRTAB, SHF_STRINGS, 0, 0));
            innerBinSectionTable[ELFSECTID_STRTAB-ELFSECTID_START] = extraSectionIndex++;
            if (input->driverVersion < 203603)
                innerBinGen->addRegion(ElfRegion64(sizeof(noteSectionData16_3),
                           noteSectionData16_3, 8, ".note", SHT_NOTE, 0));
            else
            {   /* AMD GPU PRO */
                noteBuf.reset(new cxbyte[sizeof(noteSectionData16_3)]);
                ::memcpy(noteBuf.get(), noteSectionData16_3, sizeof(noteSectionData16_3));
                noteBuf[197] = 't';
                innerBinGen->addRegion(ElfRegion64(sizeof(noteSectionData16_3),
                           noteBuf.get(), 8, ".note", SHT_NOTE, 0));
            }
            innerBinSectionTable[AMDCL2SECTID_NOTE-ELFSECTID_START] = extraSectionIndex++;
        }
        if (hasRWData)
        {   // rw data section
            innerBinGen->addRegion(ElfRegion64(input->rwDataSize, input->rwData,
                      8, ".hsadata_global_agent", SHT_PROGBITS, 0x900003, 0, 0,
                      Elf64Types::nobase));
            innerBinSectionTable[ELFSECTID_DATA-ELFSECTID_START] =
                    extraSectionIndex++;
        }
        if (input->bssSize!=0)
        {
            innerBinGen->addRegion(ElfRegion64(input->bssSize, (const cxbyte*)nullptr,
                      input->bssAlignment!=0 ? input->bssAlignment : 8,
                      ".hsabss_global_agent", SHT_NOBITS, 0x900003,
                      0, 0, Elf64Types::nobase, 0, true));
            innerBinSectionTable[ELFSECTID_BSS-ELFSECTID_START] = extraSectionIndex++;
        }
        if (hasGlobalData)
        {// global data section
            innerBinGen->addRegion(ElfRegion64(innerGDataGen.size(), &innerGDataGen,
                      8, ".hsadata_readonly_agent", SHT_PROGBITS, 0xa00003, 0, 0,
                      Elf64Types::nobase));
            innerBinSectionTable[ELFSECTID_RODATA-ELFSECTID_START] = extraSectionIndex++;
        }
        if (kernelsNum != 0)
        {
            innerBinGen->addRegion(ElfRegion64(innerTextGen.size(), &innerTextGen, 256,
                      ".hsatext", SHT_PROGBITS, 0xc00007, 0, 0, 
                      Elf64Types::nobase, 0));
            innerBinSectionTable[ELFSECTID_TEXT-ELFSECTID_START] = extraSectionIndex++;
        }
        
        if (is16_3Ver)
        {   /* new driver version */
            innerBinGen->addRegion(ElfRegion64::symtabSection());
            innerBinSectionTable[ELFSECTID_SYMTAB-ELFSECTID_START] = extraSectionIndex++;
        }
        
        if (hasSamplers)
        {
            innerBinGen->addRegion(ElfRegion64(input->samplerConfig ?
                    input->samplers.size()*8 : input->samplerInitSize,
                    &innerSamplerInitGen, (is16_3Ver) ? 8 : 1,
                    ".hsaimage_samplerinit", SHT_PROGBITS, SHF_MERGE, 0, 0, 0, 8));
            innerBinSectionTable[AMDCL2SECTID_SAMPLERINIT-ELFSECTID_START] =
                        extraSectionIndex++;
            innerBinGen->addRegion(ElfRegion64(innerGDataRels.size(), &innerGDataRels,
                    8, ".rela.hsadata_readonly_agent", SHT_RELA, 0, symtabId,
                    globalDataId, 0, sizeof(Elf64_Rela)));
            innerBinSectionTable[AMDCL2SECTID_RODATARELA-ELFSECTID_START] =
                        extraSectionIndex++;
        }
        size_t textRelSize = innerTextRelsGen.size();
        if (textRelSize!=0) // if some relocations
        {
            innerBinGen->addRegion(ElfRegion64(textRelSize, &innerTextRelsGen, 8,
                    ".rela.hsatext", SHT_RELA, 0, symtabId, textId,  0,
                    sizeof(Elf64_Rela)));
            innerBinSectionTable[AMDCL2SECTID_TEXTRELA-ELFSECTID_START] =
                    extraSectionIndex++;
        }
        
        if (!is16_3Ver)
        {   /* this order of section for 1912.05 driver version */
            innerBinGen->addRegion(ElfRegion64(sizeof(noteSectionData), noteSectionData, 8,
                        ".note", SHT_NOTE, 0));
            innerBinSectionTable[AMDCL2SECTID_NOTE-ELFSECTID_START] = extraSectionIndex++;
            innerBinGen->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".strtab",
                                  SHT_STRTAB, SHF_STRINGS, 0, 0));
            innerBinSectionTable[ELFSECTID_STRTAB-ELFSECTID_START] = extraSectionIndex++;
            innerBinGen->addRegion(ElfRegion64::symtabSection());
            innerBinSectionTable[ELFSECTID_SYMTAB-ELFSECTID_START] = extraSectionIndex++;
            innerBinGen->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".shstrtab",
                                  SHT_STRTAB, SHF_STRINGS, 0, 0));
            innerBinSectionTable[ELFSECTID_SHSTRTAB-ELFSECTID_START] = extraSectionIndex++;
        }
        
        if (kernelsNum != 0)
            putInnerSymbols(*innerBinGen, input, tempDatas, innerBinSectionTable,
                        extraSectionIndex, symbolNamePool, dataSymbolsNum);
        
        for (const BinSection& section: input->innerExtraSections)
            innerBinGen->addRegion(ElfRegion64(section, innerBinSectionTable,
                         AMDCL2SECTID_MAX, extraSectionIndex));
        // section table
        innerBinGen->addRegion(ElfRegion64::sectionHeaderTable());
        /// program headers
        if (kernelsNum != 0)
        {
            cxuint textSectionReg = (is16_3Ver) ? 4 : 1;
            if (hasRWData && input->bssSize!=0)
            {
                innerBinGen->addProgramHeader({ PT_LOOS+1, PF_W|PF_R, textSectionReg, 2,
                                true, 0, 0, 0 });
                textSectionReg += 2;
            }
            else if (hasRWData)
            {
                innerBinGen->addProgramHeader({ PT_LOOS+1, PF_W|PF_R, textSectionReg, 1,
                                true, 0, 0, 0 });
                textSectionReg++;
            }
            else if (input->bssSize!=0)
            {
                innerBinGen->addProgramHeader({ PT_LOOS+1, PF_W|PF_R, textSectionReg, 1,
                                true, 0, 0, 0 });
                textSectionReg++;
            }
            if (hasGlobalData)
            {   // textSectionReg - now is global data section
                innerBinGen->addProgramHeader({ PT_LOOS+2, PF_W|PF_R, textSectionReg, 1,
                        true, 0, Elf64Types::nobase
                        /*(hasRWData || input->bssSize!=0) ? -0xe8ULL+ : 0*/, 0 });
                textSectionReg++; // now is text section index
            }
            uint32_t phFlags = (is16_3Ver) ? (PF_X|PF_R) : (PF_R|PF_W);
            innerBinGen->addProgramHeader({ PT_LOOS+3, phFlags, textSectionReg, 1,
                    true, 0, Elf64Types::nobase, 0 });
        }
    }
    
    CL2MainTextGen mainTextGen(input, tempDatas, innerBinGen.get());
    if (kernelsNum != 0 || newBinaries)
        elfBinGen.addRegion(ElfRegion64(mainTextGen.size(), &mainTextGen, 1, ".text",
                    SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR));
    
    for (const BinSection& section: input->extraSections)
            elfBinGen.addRegion(ElfRegion64(section, mainBuiltinSectionTable,
                         ELFSECTID_STD_MAX, mainExtraSectionIndex));
    elfBinGen.addRegion(ElfRegion64::sectionHeaderTable());
    
    const uint64_t binarySize = elfBinGen.countSize();
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
        elfBinGen.generate(fob);
    }
    catch(...)
    {
        os->exceptions(oldExceptions);
        throw;
    }
    os->exceptions(oldExceptions);
    assert(fob.getWritten() == binarySize);
}

void AmdCL2GPUBinGenerator::generate(Array<cxbyte>& array) const
{
    generateInternal(nullptr, nullptr, &array);
}

void AmdCL2GPUBinGenerator::generate(std::ostream& os) const
{
    generateInternal(&os, nullptr, nullptr);
}

void AmdCL2GPUBinGenerator::generate(std::vector<char>& vector) const
{
    generateInternal(nullptr, &vector, nullptr);
}
