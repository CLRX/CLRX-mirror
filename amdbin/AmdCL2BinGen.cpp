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
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/InputOutput.h>
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
    size_t isaMetadataSize;
    size_t stubSize;
    size_t setupSize;
    size_t codeSize;
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


// fast and memory efficient String table generator for main binary
class CLRX_INTERNAL CL2MainStrTabGen: public ElfRegionContent
{
private:
    const AmdCL2Input* input;
    bool withBrig;
public:
    CL2MainStrTabGen(const AmdCL2Input* _input) : input(_input), withBrig(false)
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
        if (newBinaries)
            for (const AmdCL2KernelInput& kernel: input->kernels)
            {   // put kernel ISA/binary symbol names
                fob.write(19, "__ISA_&__OpenCL_");
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
public:
    CL2MainSymTabGen(const AmdCL2Input* _input,
        const Array<TempAmdCL2KernelData>& _tempDatas) : input(_input), withBrig(false),
        tempDatas(_tempDatas)
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
        size_t nameIndex = 1;
        if (!input->compileOptions.empty())
        {
            SLEV(sym.st_name, nameIndex);
            SLEV(sym.st_shndx, 4);
            SLEV(sym.st_value, 0);
            SLEV(sym.st_size, input->compileOptions.size());
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameIndex += 26;
            fob.writeObject(sym);
        }
        size_t rodataPos = 0;
        size_t textPos = 0;
        for (const AmdCL2KernelInput& kernel: input->kernels)
        {
            SLEV(sym.st_name, nameIndex);
            SLEV(sym.st_shndx, 5);
            SLEV(sym.st_value, rodataPos);
            SLEV(sym.st_size, kernel.metadataSize);
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameIndex += kernel.kernelName.size() + 19 + 17;
            rodataPos += kernel.metadataSize;
            fob.writeObject(sym);
        }
        if (withBrig)
        {   // put __BRIG__ symbol
            SLEV(sym.st_name, nameIndex);
            SLEV(sym.st_name, 7 + brigIndex);
            SLEV(sym.st_value, 0);
            SLEV(sym.st_size, input->extraSections[brigIndex].size);
            sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
            sym.st_other = 0;
            nameIndex += 9;
            fob.writeObject(sym);
        }
        
        if (!newBinaries)
        { // put binary and ISA metadata symbols
            for (size_t i = 0; i < input->kernels.size(); i++)
            {
                const AmdCL2KernelInput& kernel = input->kernels[i];
                const TempAmdCL2KernelData& tempData = tempDatas[i];
                // put kernel binary symbol
                SLEV(sym.st_name, nameIndex);
                SLEV(sym.st_shndx, 6);
                SLEV(sym.st_value, textPos);
                SLEV(sym.st_size, tempData.stubSize+tempData.setupSize+tempData.codeSize);
                sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_FUNC);
                sym.st_other = 0;
                nameIndex += 19 + kernel.kernelName.size() + 15;
                textPos += tempData.stubSize+tempData.setupSize+tempData.codeSize;
                fob.writeObject(sym);
                // put ISA metadata symbol
                SLEV(sym.st_name, nameIndex);
                SLEV(sym.st_shndx, 5);
                SLEV(sym.st_value, rodataPos);
                SLEV(sym.st_size, tempData.isaMetadataSize);
                sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
                sym.st_other = 0;
                nameIndex += 19 + kernel.kernelName.size() + 17;
                rodataPos += tempData.isaMetadataSize;
                fob.writeObject(sym);
            }
        }
        // acl_version_string
        SLEV(sym.st_name, nameIndex);
        SLEV(sym.st_shndx, 4);
        SLEV(sym.st_value, input->compileOptions.size());
        SLEV(sym.st_size, input->aclVersion.size());
        sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
        sym.st_other = 0;
        fob.writeObject(sym);
        
        for (const BinSymbol& symbol: input->extraSymbols)
        {
            SLEV(sym.st_name, nameIndex);
            SLEV(sym.st_shndx, convertSectionId(symbol.sectionId, mainBuiltinSectionTable,
                            ELFSECTID_STD_MAX, 7));
            SLEV(sym.st_size, symbol.size);
            SLEV(sym.st_value, symbol.value);
            sym.st_info = symbol.info;
            sym.st_other = symbol.other;
            nameIndex += symbol.name.size()+1;
            fob.writeObject(sym);
        }
    }
};

/// main routine to generate OpenCL 2.0 binary
void AmdCL2GPUBinGenerator::generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const
{
    const size_t kernelsNum = input->kernels.size();
    const bool oldBinaries = input->driverVersion < 191205;
    const GPUArchitecture arch = getGPUArchitectureFromDeviceType(input->deviceType);
    if (arch == GPUArchitecture::GCN1_0)
        throw Exception("OpenCL 2.0 supported only for GCN1.1 or later");
    
    ElfBinaryGen64 elfBinGen({ 0, 0, ELFOSABI_SYSV, 0, ET_EXEC,  0xaf5b, EV_CURRENT,
                UINT_MAX, 0, gpuDeviceCodeTable[cxuint(input->deviceType)] });
    
    Array<TempAmdCL2KernelData> tempDatas(kernelsNum);
    for (size_t i = 0; i < kernelsNum; i++)
    {
        const AmdCL2KernelInput& kernel = input->kernels[i];
        tempDatas[i].isaMetadataSize = kernel.isaMetadataSize;
        tempDatas[i].setupSize = kernel.setupSize;
        tempDatas[i].stubSize = kernel.stubSize;
        tempDatas[i].codeSize = kernel.codeSize;
    }
    CL2MainStrTabGen mainStrTabGen(input);
    CL2MainSymTabGen mainSymTabGen(input, tempDatas);
    // main section of main binary
    elfBinGen.addRegion(ElfRegion64(0, (const cxbyte*)nullptr, 1, ".shstrtab",
                    SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegion64(mainStrTabGen.size(), &mainStrTabGen, 1, ".strtab",
                    SHT_STRTAB, SHF_STRINGS));
    elfBinGen.addRegion(ElfRegion64(mainSymTabGen.size(), &mainSymTabGen, 8, ".symtab",
                    SHT_SYMTAB, 0));
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
