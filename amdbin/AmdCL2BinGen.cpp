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

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator() : manageable(false), input(nullptr)
{ }

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator(const AmdCL2Input* amdInput)
        : manageable(false), input(amdInput)
{ }

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator(GPUDeviceType deviceType,
       uint32_t driverVersion, size_t globalDataSize, const cxbyte* globalData, 
       const std::vector<AmdCL2KernelInput>& kernelInputs)
        : manageable(true), input(nullptr)
{
    input = new AmdCL2Input{deviceType, globalDataSize, globalData, 0, nullptr, false,
                { }, { }, driverVersion, "", "", kernelInputs };
}

AmdCL2GPUBinGenerator::AmdCL2GPUBinGenerator(GPUDeviceType deviceType,
       uint32_t driverVersion, size_t globalDataSize, const cxbyte* globalData, 
       std::vector<AmdCL2KernelInput>&& kernelInputs)
        : manageable(true), input(nullptr)
{
    input = new AmdCL2Input{deviceType, globalDataSize, globalData, 0, nullptr, false,
                { }, { }, driverVersion, "", "", std::move(kernelInputs) };
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

static const uint32_t gpuDeviceCodeTable[16] =
{
    0, // GPUDeviceType::CAPE_VERDE
    0, // GPUDeviceType::PITCAIRN
    0, // GPUDeviceType::TAHITI
    0, // GPUDeviceType::OLAND
    6, // GPUDeviceType::BONAIRE
    1, // GPUDeviceType::SPECTRE
    2, // GPUDeviceType::SPOOKY
    3, // GPUDeviceType::KALINDI
    0, // GPUDeviceType::HAINAN
    7, // GPUDeviceType::HAWAII
    8, // GPUDeviceType::ICELAND
    9, // GPUDeviceType::TONGA
    4, // GPUDeviceType::MULLINS
    17, // GPUDeviceType::FIJI
    16, // GPUDeviceType::CARRIZO
    15 // GPUDeviceType::DUMMY
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
    const size_t kernelsNum = input->kernels.size();
    for (size_t i = 0; i < kernelsNum; i++)
    {
        const AmdCL2KernelInput& kernel = input->kernels[i];
        if (newBinaries && (kernel.isaMetadataSize!=0 || kernel.isaMetadata!=nullptr))
            throw Exception("ISA metadata allowed for old driver binaries");
        if (newBinaries && (kernel.stubSize!=0 || kernel.stub!=nullptr))
            throw Exception("Kernel stub allowed for old driver binaries");
        
        if (!kernel.useConfig)
        {   // if no kernel configuration
            tempDatas[i].metadataSize = kernel.metadataSize;
            tempDatas[i].isaMetadataSize = (kernel.isaMetadata!=nullptr) ?
                        kernel.isaMetadataSize : sizeof(kernelIsaMetadata);
            tempDatas[i].setupSize = kernel.setupSize;
            tempDatas[i].stubSize = kernel.stubSize;
        }
        else
        {   // kernel configuration present
            size_t out = ((newBinaries) ? 254 : 246) + (kernel.config.args.size() + 1)*88;
            for (const AmdKernelArgInput& arg: kernel.config.args)
                    out += arg.argName.size() + arg.typeName.size() + 2;
            
            tempDatas[i].metadataSize = out;
            tempDatas[i].setupSize = 256;
            tempDatas[i].stubSize = tempDatas[i].isaMetadataSize = 0;
            if (!newBinaries)
            {
                tempDatas[i].stubSize = 0xa60;
                tempDatas[i].isaMetadataSize = sizeof(kernelIsaMetadata);
            }
        }
        tempDatas[i].codeSize = kernel.codeSize;
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
public:
    CL2MainSymTabGen(const AmdCL2Input* _input,
                     const Array<TempAmdCL2KernelData>& _tempDatas,
                     const CString& _aclVersion) : input(_input), withBrig(false),
        tempDatas(_tempDatas), aclVersion(_aclVersion)
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
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
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
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
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
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
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
                sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_FUNC);
                sym.st_other = 0;
                nameOffset += 16 + kernel.kernelName.size() + 15;
                textPos += tempData.stubSize+tempData.setupSize+tempData.codeSize;
                fob.writeObject(sym);
                // put ISA metadata symbol
                SLEV(sym.st_name, nameOffset);
                SLEV(sym.st_shndx, 5);
                SLEV(sym.st_value, rodataPos);
                SLEV(sym.st_size, tempData.isaMetadataSize);
                sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
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
        sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
        sym.st_other = 0;
        fob.writeObject(sym);
        
        for (const BinSymbol& symbol: input->extraSymbols)
        {
            SLEV(sym.st_name, nameOffset);
            SLEV(sym.st_shndx, convertSectionId(symbol.sectionId, mainBuiltinSectionTable,
                            ELFSECTID_STD_MAX, 7));
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

static const uint32_t ptrSpacesTable[4] = { 0, 1, 3, 2 };

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
    
    void writeMetadata(bool newBinaries, const TempAmdCL2KernelData& tempData,
               AmdCL2KernelConfig& config, FastOutputBuffer& fob) const
    {
        AmdCL2GPUMetadataHeader header;
        cxuint argsNum = config.args.size();
        
        SLEV(header.size, (newBinaries) ? 0xe0 : 0xd8);
        SLEV(header.metadataSize, tempData.metadataSize);
        SLEV(header.unknown1[0], 0x3);
        SLEV(header.unknown1[1], 0x1);
        SLEV(header.unknown1[2], 0x68);
        SLEV(header.options, config.reqdWorkGroupSize[0]!=0 ? 0x24 : 0x20);
        SLEV(header.unknown2[0], 0x0400ULL);
        SLEV(header.unknown2[1], 0x0100000008ULL);
        SLEV(header.unknown2[2], 0x0200000001ULL);
        SLEV(header.reqdWorkGroupSize[0], config.reqdWorkGroupSize[0]);
        SLEV(header.reqdWorkGroupSize[1], config.reqdWorkGroupSize[1]);
        SLEV(header.reqdWorkGroupSize[2], config.reqdWorkGroupSize[2]);
        SLEV(header.unknown3[0], 0);
        SLEV(header.unknown3[1], 0);
        SLEV(header.firstNameLength, 0x15);
        SLEV(header.secondNameLength, 0x7);
        for (cxuint i = 0; i < 6; i++)
            SLEV(header.unknown4[i], 0);
        SLEV(header.argsNum, argsNum);
        fob.writeObject(header);
        fob.fill(48, 0); // fill up
        if (newBinaries) // additional data
            fob.writeObject<uint64_t>(LEV(uint64_t(0xffffffff00000006ULL)));
        // two null terminated strings
        fob.writeArray(22, "__OpenCL_dummy_kernel");
        fob.writeArray(8, "generic");
        
        // put argument entries
        cxuint argOffset = 0;
        for (cxuint i = 0; i < argsNum; i++)
        {   //
            const AmdKernelArgInput& arg = config.args[i];
            AmdCL2GPUKernelArgEntry argEntry;
            SLEV(argEntry.size, 88);
            SLEV(argEntry.argNameSize, arg.argName.size());
            SLEV(argEntry.typeNameSize, arg.typeName.size());
            SLEV(argEntry.unknown1, 0);
            SLEV(argEntry.unknown2, 0);
            
            bool isImage = (arg.argType >= KernelArgType::MIN_IMAGE &&
                 arg.argType <= KernelArgType::MAX_IMAGE);
            
            cxuint vectorLength = argTypeSizesTable[cxuint(arg.argType)].vectorSize;
            if (newBinaries && vectorLength==3)
                vectorLength = 4;
            if (isImage || arg.argType==KernelArgType::SAMPLER)
                // image/sampler resid
                SLEV(argEntry.resId, tempData.argResIds[i]);
            else if (arg.argType == KernelArgType::STRUCTURE)
                SLEV(argEntry.resId, arg.structSize);
            else
                SLEV(argEntry.vectorLength, vectorLength);
            size_t argSize = (arg.argType==KernelArgType::STRUCTURE) ? arg.structSize :
                    argTypeSizesTable[cxuint(arg.argType)].elemSize*vectorLength;
            
            SLEV(argEntry.unknown3, (arg.argType==KernelArgType::SAMPLER));
            SLEV(argEntry.argOffset, argOffset);
            argOffset += (argSize + 15) & ~15U; // align to 16 bytes
            
            uint32_t argType = 0;
            if (isImage)
            {   // if image
                cxuint ptrAccMask = arg.ptrAccess & KARG_PTR_ACCESS_MASK;
                argType = ptrAccMask==KARG_PTR_READ_ONLY ? 1 :
                        ptrAccMask==KARG_PTR_WRITE_ONLY ? 2 : 3 /* read-write */;
            }
            else if (arg.argType == KernelArgType::POINTER)
                argType = (arg.pointerType==KernelArgType::STRUCTURE) ? 15 : 7;
            else // otherwise
                argType = argTypeSizesTable[cxuint(arg.argType)].type;
            SLEV(argEntry.argType, argType);
            
            if (arg.argType == KernelArgType::CMDQUEUE)
                SLEV(argEntry.ptrAlignment, newBinaries ? 2 : 4);
            else // otherwise
            {
                cxuint powerof2 = 1U<<(32-CLZ32(argSize));
                if (powerof2==argSize)
                    powerof2++;
                SLEV(argEntry.ptrAlignment, powerof2);
            }
            
            if (arg.argType == KernelArgType::POINTER)
            {
                SLEV(argEntry.ptrType, argTypeSizesTable[cxuint(arg.pointerType)].type);
                SLEV(argEntry.ptrSpace, ptrSpacesTable[cxuint(arg.ptrSpace)]);
            }
            else
            {
                SLEV(argEntry.ptrType, 0);
                SLEV(argEntry.ptrSpace, 0);
            }
            bool isPointerOrPipe = (arg.argType==KernelArgType::POINTER ||
                    arg.argType==KernelArgType::CLKEVENT ||
                    arg.argType==KernelArgType::PIPE);
            SLEV(argEntry.isPointerOrPipe, isPointerOrPipe);
            
            SLEV(argEntry.isConst, (arg.ptrAccess & KARG_PTR_CONST) != 0);
            argEntry.isVolatile = ((arg.ptrAccess & KARG_PTR_VOLATILE) != 0);
            argEntry.isRestrict = ((arg.ptrAccess & KARG_PTR_RESTRICT) != 0);
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
        fob.write(88, 0); // NULL arg
        
        // arg names and type names
        for (const AmdKernelArgInput& arg: config.args)
        {
            fob.writeArray(arg.argName.size(), arg.argName.c_str());
            fob.writeArray(arg.typeName.size(), arg.typeName.c_str());
        }
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        const bool newBinaries = input->driverVersion >= 191205;
        for (const AmdCL2KernelInput& kernel: input->kernels)
            fob.writeArray(kernel.metadataSize, kernel.metadata);
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

static const cxbyte firstKernelSetupBytes[48] =
{
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
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
    uint32_t scratchBufferDwords;
    uint32_t localSize; // in bytes
    uint32_t zero1;
    uint32_t kernelArgsSize;
    uint32_t zeroes[2];
    uint16_t sgprsNumAll;
    uint16_t vgprsNum16;
    uint32_t vgprsNum;
    uint32_t sgprsNum;
    uint32_t zero3;
    uint32_t version; // ??
};

static void generateKernelSetup(GPUArchitecture arch, const AmdCL2KernelConfig& config,
                FastOutputBuffer& fob, bool newBinaries)
{
    fob.writeArray(48, firstKernelSetupBytes);
    IntAmdCL2SetupData setupData;
    cxuint sgprsNum = std::max(config.usedSGPRsNum, 1U);
    cxuint vgprsNum = std::max(config.usedVGPRsNum, 1U);
    // pgmrsrc1
    SLEV(setupData.pgmRSRC1, ((vgprsNum-1)>>2) | (((sgprsNum-1)>>3)<<6) |
            ((uint32_t(config.floatMode)&0xff)<<12) |
            (config.ieeeMode?1U<<23:0) | (uint32_t(config.priority&3)<<10));
    // pgmrsrc2 - without ldssize
    uint32_t dimValues = 0;
    if (config.dimMask != BINGEN_DEFAULT)
        dimValues = ((config.dimMask&7)<<7) |
                (((config.dimMask&4) ? 2 : (config.dimMask&2) ? 1 : 0)<<11);
    else
        dimValues |= (config.pgmRSRC2 & 0x1b80U);
    SLEV(setupData.pgmRSRC1, (config.pgmRSRC2 & 0xffffe040U) | (4<<1) /* userData=4*/ |
            ((config.tgSize) ? 0x400 : 0) | ((config.scratchBufferSize)?1:0) | dimValues);
    
    /// ohers
    SLEV(setupData.setup1, 0xb);
    SLEV(setupData.archInd, (arch == GPUArchitecture::GCN1_2) ? 0x4a : 0x0a);
    SLEV(setupData.scratchBufferDwords, (config.scratchBufferSize+3)>>2);
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
            (arg.argType >= KernelArgType::MIN_IMAGE &&
             arg.argType >= KernelArgType::MAX_IMAGE))
            kernelArgSize += 8;
        else if (arg.argType == KernelArgType::SAMPLER)
            kernelArgSize += 4;
        else
        {   // scalar
            const ArgTypeSizes& argTypeSizes = argTypeSizesTable[cxuint(arg.argType)];
            kernelArgSize += argTypeSizes.vectorSize * argTypeSizes.elemSize;
        }
    }
    SLEV(setupData.kernelArgsSize, kernelArgSize);
    SLEV(setupData.sgprsNumAll, config.usedSGPRsNum+2);
    SLEV(setupData.vgprsNum16, config.usedVGPRsNum);
    SLEV(setupData.vgprsNum, config.usedVGPRsNum);
    SLEV(setupData.sgprsNum, config.usedSGPRsNum);
    SLEV(setupData.version, newBinaries ? 0x06040404U : 0x06000003U);
    
    fob.writeObject(setupData);
    fob.fill(256 - sizeof(IntAmdCL2SetupData) - 48, 0);
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
        else // otherwise
            for (size_t i = 0; i < input->kernels.size(); i++)
            {
                const AmdCL2KernelInput& kernel = input->kernels[i];
                if (!kernel.useConfig)
                {   // no configuration, get from kernel data
                    fob.writeArray(kernel.stubSize, kernel.stub);
                    fob.writeArray(kernel.setupSize, kernel.setup);
                }
                else
                {   // generate stub, setup from kernel config
                    generateKernelSetup(
                        getGPUArchitectureFromDeviceType(input->deviceType),
                        kernel.config, fob, false);
                }
                fob.writeArray(kernel.codeSize, kernel.code);
            }
    }
};

class CLRX_INTERNAL CL2InnerTextGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
public:
    explicit CL2InnerTextGen(const AmdCL2Input* _input) : input(_input)
    { }
    
    size_t size() const
    {
        size_t out = 0;
        for (const AmdCL2KernelInput& kernel: input->kernels)
        {
            if ((out & 255) != 0)
                out += 256-(out&255);
            out += kernel.setupSize + kernel.codeSize;
        }
        return out;
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        size_t outSize = 0;
        for (const AmdCL2KernelInput& kernel: input->kernels)
        {
            if ((outSize & 255) != 0)
            {
                size_t toFill = 256-(outSize&255);
                fob.fill(toFill, 0);
                outSize += toFill;
            }
            fob.writeArray(kernel.setupSize, kernel.setup);
            fob.writeArray(kernel.codeSize, kernel.code);
            outSize += kernel.setupSize + kernel.codeSize;
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

class CLRX_INTERNAL CL2InnerGlobalDataRelsGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
public:
    explicit CL2InnerGlobalDataRelsGen(const AmdCL2Input* _input) : input(_input)
    { }
    
    size_t size() const
    {
        size_t out;
        if (!input->samplerOffsets.empty())
           out = input->samplerOffsets.size();
        else
            out= (input->samplerConfig) ? input->samplers.size() :
                    (input->samplerInitSize>>3);
        return out*sizeof(Elf64_Rela);
    }
    
    void operator()(FastOutputBuffer& fob) const
    {
        Elf64_Rela rela;
        rela.r_addend = 0;
        uint32_t symIndex = input->kernels.size() + input->samplerOffsets.size() + 2;
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
            const size_t samplersNum = (input->samplerConfig) ?
                        input->samplers.size() : (input->samplerInitSize>>3);
            size_t globalOffset = input->globalDataSize - samplersNum;
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
public:
    explicit CL2InnerTextRelsGen(const AmdCL2Input* _input) : input(_input)
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
        uint32_t symIndex = input->kernels.size() + input->samplerOffsets.size();
        for (const AmdCL2KernelInput& kernel: input->kernels)
        {
            codeOffset += kernel.setupSize;
            for (const AmdCL2RelInput inRel: kernel.relocations)
            {
                SLEV(rela.r_offset, inRel.offset + codeOffset);
                uint32_t type = (inRel.type==RelocType::LOW_32BIT) ? 1 : 2;
                SLEV(rela.r_info, ELF64_R_INFO(symIndex, type));
                SLEV(rela.r_addend, inRel.addend);
                fob.writeObject(rela);
            }
            codeOffset += (kernel.codeSize+255)&(~255);
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

static CString constructName(size_t prefixSize, const char* prefix, const CString& name,
                 size_t suffixSize, const char* suffix)
{
    const size_t nameLen = name.size();
    CString out(prefixSize + suffixSize + nameLen);
    char* outPtr = out.begin();
    std::copy(prefix, prefix+prefixSize, outPtr);
    std::copy(name.begin(), name.begin()+nameLen, outPtr+prefixSize);
    std::copy(suffix, suffix+suffixSize, outPtr+prefixSize+nameLen);
    outPtr[prefixSize+nameLen+suffixSize] = 0;
    return out;
}

static void putInnerSymbols(ElfBinaryGen64& innerBinGen, const AmdCL2Input* input,
        const uint16_t* builtinSectionTable, cxuint extraSeciontIndex,
        std::vector<CString>& stringPool)
{
    cxuint samplersNum;
    if (!input->samplerOffsets.empty())
       samplersNum = input->samplerOffsets.size();
    else // if no sampler offsets
        samplersNum = (input->samplerConfig) ? input->samplers.size() :
                (input->samplerInitSize>>3);
    // put kernel symbols
    std::vector<bool> samplerMask(samplersNum);
    size_t samplerOffset = input->globalDataSize - samplersNum;
    size_t codePos = 0;
    uint16_t textSectId = convertSectionId(ELFSECTID_TEXT, builtinSectionTable,
                             AMDCL2SECTID_MAX, extraSeciontIndex);
    for (const AmdCL2KernelInput& kernel: input->kernels)
    {   // first, we put sampler objects
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
                stringPool.push_back(sampName);
                innerBinGen.addSymbol(ElfSymbol64(stringPool.back().c_str(), 1,
                          ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0, false, value, 8));
                samplerMask[samp] = true;
            }
        // put kernel symbol
        stringPool.push_back(constructName(10, "&__OpenCL_", kernel.kernelName,
                        7, "_kernel"));
        
        innerBinGen.addSymbol(ElfSymbol64(stringPool.back().c_str(), textSectId,
                  ELF32_ST_INFO(STB_GLOBAL, 10), 0, false, codePos, 
                  kernel.codeSize+kernel.setupSize));
        codePos += kernel.codeSize+kernel.setupSize;
    }
    
    for (size_t i = 0; i < samplersNum; i++)
        if (!samplerMask[i])
        {
            const uint64_t value = !input->samplerOffsets.empty() ?
                    input->samplerOffsets[i] : samplerOffset + i*8;
            char sampName[64];
            memcpy(sampName, "&input_bc::&_.Samp", 18);
            itocstrCStyle<cxuint>(i, sampName+18, 64-18);
            stringPool.push_back(sampName);
            innerBinGen.addSymbol(ElfSymbol64(stringPool.back().c_str(), 1,
                      ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 0, false, value, 8));
            samplerMask[i] = true;
        }
    
    if (input->globalDataSize!=0 && input->globalData!=nullptr)
        innerBinGen.addSymbol(ElfSymbol64("__hsa_section.hsadata_readonly_agent", 1,
              ELF32_ST_INFO(STB_LOCAL, STT_SECTION), 0, false, 0, 0));
    innerBinGen.addSymbol(ElfSymbol64("__hsa_section.hsatext", textSectId,
              ELF32_ST_INFO(STB_LOCAL, STT_SECTION), 0, false, 0, 0));
    
    for (size_t i = 0; i < samplersNum; i++)
        innerBinGen.addSymbol(ElfSymbol64("", 3, ELF32_ST_INFO(STB_LOCAL, 12),
                      0, false, i*8, 0));
    /// add extra inner symbols
    for (const BinSymbol& extraSym: input->innerExtraSymbols)
        innerBinGen.addSymbol(ElfSymbol64(extraSym, builtinSectionTable, AMDCL2SECTID_MAX,
                                  extraSeciontIndex));
}

/// without global data and samplers
static const uint16_t innerBuiltinSectionTable1[] =
{
    5, // ELFSECTID_SHSTRTAB
    3, // ELFSECTID_STRTAB
    4, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    1, // ELFSECTID_TEXT
    SHN_UNDEF, // ELFSECTID_RODATA
    SHN_UNDEF, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    SHN_UNDEF, // ELFSECTID_COMMENT
    SHN_UNDEF, // AMDCL2SECTID_SAMPLERINIT
    2  // AMDCL2SECTID_NOTE
};

/// global data
static const uint16_t innerBuiltinSectionTable2[] =
{
    7, // ELFSECTID_SHSTRTAB
    5, // ELFSECTID_STRTAB
    6, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    2, // ELFSECTID_TEXT
    1, // ELFSECTID_RODATA
    SHN_UNDEF, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    SHN_UNDEF, // ELFSECTID_COMMENT
    SHN_UNDEF, // AMDCL2SECTID_SAMPLERINIT
    4  // AMDCL2SECTID_NOTE
};

/// global data and samplers
static const uint16_t innerBuiltinSectionTable3[] =
{
    9, // ELFSECTID_SHSTRTAB
    7, // ELFSECTID_STRTAB
    8, // ELFSECTID_SYMTAB
    SHN_UNDEF, // ELFSECTID_DYNSTR
    SHN_UNDEF, // ELFSECTID_DYNSYM
    2, // ELFSECTID_TEXT
    1, // ELFSECTID_RODATA
    SHN_UNDEF, // ELFSECTID_DATA
    SHN_UNDEF, // ELFSECTID_BSS
    SHN_UNDEF, // ELFSECTID_COMMENT
    3, // AMDCL2SECTID_SAMPLERINIT
    6  // AMDCL2SECTID_NOTE
};

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
                
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
    if (arch == GPUArchitecture::GCN1_0)
        throw Exception("OpenCL 2.0 supported only for GCN1.1 or later");
    
    if ((hasGlobalData || hasSamplers) && !newBinaries)
        throw Exception("Old driver binaries doesn't support global data or samplers");
    
    if (newBinaries)
    {
        if (!hasGlobalData && hasSamplers)
            throw Exception("Global data must be defined if samplers present");
        if (!input->samplerConfig)
        {
            if (input->samplerInitSize != input->samplerOffsets.size()*8)
                throw Exception("Sampler offsets and sampler init sizes doesn't match");
        }
        else if (input->samplers.size() != input->samplerOffsets.size())
            throw Exception("Sampler offsets and sampler sizes doesn't match");
        // check sampler offset range
        for (size_t sampOffset: input->samplerOffsets)
            if (sampOffset+8 > input->globalDataSize)
                throw Exception("Sampler offset outside global data");
    }
    
    ElfBinaryGen64 elfBinGen({ 0, 0, ELFOSABI_SYSV, 0, ET_EXEC, 0xaf5b, EV_CURRENT,
                UINT_MAX, 0, gpuDeviceCodeTable[cxuint(input->deviceType)] });
    
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
    
    CL2MainStrTabGen mainStrTabGen(input);
    CL2MainSymTabGen mainSymTabGen(input, tempDatas, aclVersion);
    CL2MainCommentGen mainCommentGen(input, aclVersion);
    CL2MainRodataGen mainRodataGen(input, tempDatas);
    CL2InnerTextGen innerTextGen(input);
    CL2InnerSamplerInitGen innerSamplerInitGen(input);
    CL2InnerTextRelsGen innerTextRelsGen(input);
    CL2InnerGlobalDataRelsGen innerGDataRels(input);
    
    // main section of main binary
    elfBinGen.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".shstrtab",
                    SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegion64(mainStrTabGen.size(), &mainStrTabGen, 1, ".strtab",
                    SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegion64(mainSymTabGen.size(), &mainSymTabGen, 8, ".symtab",
                    SHT_SYMTAB, 0));
    elfBinGen.addRegion(ElfRegion64(input->compileOptions.size()+aclVersion.size(),
                    &mainCommentGen, 1, ".comment", SHT_PROGBITS, 0));
    elfBinGen.addRegion(ElfRegion64(mainRodataGen.size(), &mainRodataGen, 1, ".rodata",
                    SHT_PROGBITS, SHF_ALLOC));
    
    std::unique_ptr<ElfBinaryGen64> innerBinGen;
    std::vector<CString> symbolNamePool;
    if (newBinaries)
    {   // new binaries - .text holds inner ELF binaries
        const uint16_t* innerBinSectionTable;
        cxuint extraSectionIndex = 0;
        /* check kernel text relocations */
        for (const AmdCL2KernelInput& kernel: input->kernels)
            for (const AmdCL2RelInput& rel: kernel.relocations)
                if (rel.offset >= kernel.codeSize)
                    throw Exception("Kernel text relocation offset outside kernel code");
        
        if (input->globalDataSize==0 || input->globalData==nullptr)
        {
            innerBinSectionTable = innerBuiltinSectionTable1;
            extraSectionIndex = 6;
        }
        else if (!hasSamplers) // no samplers
        {
            innerBinSectionTable = innerBuiltinSectionTable2;
            extraSectionIndex = 8;
        }
        else // samplers and global data
        {
            innerBinSectionTable = innerBuiltinSectionTable3;
            extraSectionIndex = 10;
        }
        
        innerBinGen.reset(new ElfBinaryGen64({ 0, 0, 0x40, 0, ET_REL, 0xe0, EV_CURRENT,
                        UINT_MAX, 0, 0 }, false));
        innerBinGen->addRegion(ElfRegion64::programHeaderTable());
        
        if (input->globalDataSize!=0 && input->globalData!=nullptr)
            // global data section
            innerBinGen->addRegion(ElfRegion64(input->globalDataSize, input->globalData,
                      8, ".hsadata_readonly_agent", SHT_PROGBITS, 0xa00003));
        innerBinGen->addRegion(ElfRegion64(innerTextGen.size(), &innerTextGen, 256,
                      ".hsatext", SHT_PROGBITS, 0xc00007, 0, 0, -0x100ULL));
        if (hasSamplers)
        {
            innerBinGen->addRegion(ElfRegion64(input->samplerConfig ?
                    input->samplers.size()*8 : input->samplerInitSize,
                    &innerSamplerInitGen, 1, ".hsaimage_samplerinit", SHT_PROGBITS,
                    SHF_MERGE, 0, 0, 0, 8));
            innerBinGen->addRegion(ElfRegion64(innerGDataRels.size(), &innerGDataRels, 8,
                    ".rela.hsadata_readonly_agent", SHT_RELA, 0, 8, 1, 0,
                    sizeof(Elf64_Rela)));
        }
        size_t textRelSize = innerTextRelsGen.size();
        if (textRelSize!=0) // if some relocations
            innerBinGen->addRegion(ElfRegion64(textRelSize, &innerTextRelsGen, 8,
                    ".rela.hsatext", SHT_RELA, 0, (hasSamplers)?8:6, 2,  0,
                    sizeof(Elf64_Rela)));
        innerBinGen->addRegion(ElfRegion64(sizeof(noteSectionData), noteSectionData, 8,
                    ".note", SHT_NOTE, 0));
        innerBinGen->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".strtab",
                              SHT_STRTAB, SHF_STRINGS, 0, 0));
        innerBinGen->addRegion(ElfRegion64::symtabSection());
        innerBinGen->addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".shstrtab",
                              SHT_STRTAB, SHF_STRINGS, 0, 0));
        
        putInnerSymbols(*innerBinGen, input, innerBinSectionTable, extraSectionIndex,
                        symbolNamePool);
        
        for (const BinSection& section: input->innerExtraSections)
            innerBinGen->addRegion(ElfRegion64(section, innerBinSectionTable,
                         AMDCL2SECTID_MAX, extraSectionIndex));
        // section table
        innerBinGen->addRegion(ElfRegion64::sectionHeaderTable());
        /// program headers
        cxuint textSectionReg = 1;
        if (hasGlobalData)
        {
            innerBinGen->addProgramHeader({ PT_LOOS+2, PF_W|PF_R, 1, 1, true, 0, 0, 0 });
            textSectionReg = 2;
        }
        innerBinGen->addProgramHeader({ PT_LOOS+3, PF_W|PF_R, textSectionReg, 1, true,
                        0, -0x100ULL, 0 });
    }
    
    CL2MainTextGen mainTextGen(input, tempDatas, innerBinGen.get());
    elfBinGen.addRegion(ElfRegion64(mainTextGen.size(), &mainTextGen, 1, ".text",
                    SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR));
    
    for (const BinSection& section: input->extraSections)
            elfBinGen.addRegion(ElfRegion64(section, mainBuiltinSectionTable,
                         ELFSECTID_STD_MAX, 7));
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
