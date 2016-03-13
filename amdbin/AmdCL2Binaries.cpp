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
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>

using namespace CLRX;

/* class AmdCL2InnerGPUBinaryBase */

AmdCL2InnerGPUBinaryBase::AmdCL2InnerGPUBinaryBase(AmdCL2InnerBinaryType type)
        : binaryType(type)
{ }

AmdCL2InnerGPUBinaryBase::~AmdCL2InnerGPUBinaryBase()
{ }

const AmdCL2GPUKernel& AmdCL2InnerGPUBinaryBase::getKernelData(const char* name) const
{
    KernelDataMap::const_iterator it = binaryMapFind(
            kernelDataMap.begin(), kernelDataMap.end(), name);
    if (it == kernelDataMap.end())
        throw Exception("Can't find kernel name");
    return kernels[it->second];
}

/* AmdCL2OldInnerGPUBinary */

AmdCL2OldInnerGPUBinary::AmdCL2OldInnerGPUBinary(AmdCL2MainGPUBinary* mainBinary,
            size_t binaryCodeSize, cxbyte* binaryCode, Flags _creationFlags)
        : AmdCL2InnerGPUBinaryBase(AmdCL2InnerBinaryType::CAT15_7),
          creationFlags(_creationFlags), binarySize(binaryCodeSize), binary(binaryCode)
{
    if ((creationFlags & (AMDBIN_CREATE_KERNELDATA|AMDBIN_CREATE_KERNELSTUBS)) == 0)
        return; // nothing to initialize
    uint16_t textIndex = SHN_UNDEF;
    try
    { textIndex = mainBinary->getSectionIndex(".text"); }
    catch(const Exception& ex)
    { }
    // find symbols of ISA kernel binary
    std::vector<size_t> choosenSyms;
    const size_t symbolsNum = mainBinary->getSymbolsNum();
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = mainBinary->getSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len < 30 || ::strncmp(symName, "__ISA_&__OpenCL_", 16) != 0 ||
            ::strcmp(symName+len-14, "_kernel_binary") != 0) // not binary, skip
            continue;
        choosenSyms.push_back(i);
    }
    // allocate structures
    if (hasKernelData())
        kernels.resize(choosenSyms.size());
    if (hasKernelStubs())
        kernelStubs.reset(new AmdCL2GPUKernelStub[choosenSyms.size()]);
    if (hasKernelDataMap())
        kernelDataMap.resize(choosenSyms.size());
    
    size_t ki = 0;
    // main loop for kernel data and stub getting
    for (size_t index: choosenSyms)
    {
        const Elf64_Sym& sym = mainBinary->getSymbol(index);
        const char* symName = mainBinary->getSymbolName(index);
        const size_t binOffset = ULEV(sym.st_value);
        const size_t binSize = ULEV(sym.st_size);
        /// check conditions for symbol
        if (textIndex != ULEV(sym.st_shndx))
            throw Exception("Kernel symbol outside text section");
        if (binOffset >= binaryCodeSize)
            throw Exception("Kernel binary code offset out of range");
        if (usumGt(binOffset, binSize, binaryCodeSize))
            throw Exception("Kernel binary code offset and size out of range");
        if (binSize < 256+192)
            throw Exception("Kernel binary code size is too short");
        
        AmdCL2GPUKernelStub kernelStub;
        AmdCL2GPUKernel kernelData;
        // set data for stub
        kernelStub.data = binaryCode + binOffset;
        const size_t setupOffset = ULEV(*reinterpret_cast<uint32_t*>(kernelStub.data));
        if (setupOffset >= binSize)
            throw Exception("Kernel setup offset out of range");
        kernelStub.size = setupOffset;
        kernelData.setup = kernelStub.data + setupOffset;
        // get size of setup (offset 16 of setup)
        const size_t textOffset = ULEV(*reinterpret_cast<uint32_t*>(kernelData.setup+16));
        if (usumGe(textOffset, setupOffset, binSize))
            throw Exception("Kernel text offset out of range");
        kernelData.setupSize = textOffset;
        kernelData.code = kernelData.setup + textOffset;
        kernelData.codeSize = binSize - (kernelData.code - kernelStub.data);
        const size_t len = ::strlen(symName);
        const CString kernelName = CString(symName+16, symName+len-14);
        kernelData.kernelName = kernelName;
        // fill kernel data map and kernel stup info
        if (hasKernelDataMap()) // kernel data map
            kernelDataMap[ki] = std::make_pair(kernelName, ki);
        
        if (hasKernelStubs())
            kernelStubs[ki] = kernelStub;
        // put to kernels table
        if (hasKernelData())
            kernels[ki] = kernelData;
        ki++;
    }
    if (hasKernelDataMap())
        mapSort(kernelDataMap.begin(), kernelDataMap.end());
}

const AmdCL2GPUKernelStub& AmdCL2OldInnerGPUBinary::getKernelStub(const char* name) const
{
    KernelDataMap::const_iterator it = binaryMapFind(
            kernelDataMap.begin(), kernelDataMap.end(), name);
    if (it == kernelDataMap.end())
        throw Exception("Can't find kernel name");
    return kernelStubs[it->second];
}

/* AmdCL2InnerGPUBinary */

AmdCL2InnerGPUBinary::AmdCL2InnerGPUBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags):
            AmdCL2InnerGPUBinaryBase(AmdCL2InnerBinaryType::CRIMSON),
            ElfBinary64(binaryCodeSize, binaryCode, creationFlags),
            globalDataSize(0), globalData(nullptr),
            samplerInitSize(0), samplerInit(nullptr),
            textRelsNum(0), textRelEntrySize(0), textRela(nullptr),
            globalDataRelsNum(0), globalDataRelEntrySize(0), globalDataRela(nullptr)
{
    if (hasKernelData())
    {   // get kernel datas and kernel stubs 
        std::vector<size_t> choosenSyms;
        const size_t symbolsNum = getSymbolsNum();
        for (size_t i = 0; i < symbolsNum; i++)
        {
            const char* symName = getSymbolName(i);
            const size_t len = ::strlen(symName);
            if (len < 17 || ::strncmp(symName, "&__OpenCL_", 10) != 0 ||
                ::strcmp(symName+len-7, "_kernel") != 0) // not binary, skip
                continue;
            choosenSyms.push_back(i);
        }
        
        size_t ki = 0;
        kernels.resize(choosenSyms.size());
        if (hasKernelDataMap())
            kernelDataMap.resize(choosenSyms.size());
        
        // main loop for kernel data getting
        for (size_t index: choosenSyms)
        {
            const Elf64_Sym& sym = getSymbol(index);
            const Elf64_Shdr& dataShdr = getSectionHeader(ULEV(sym.st_shndx));
            if (ULEV(sym.st_shndx) >= getSectionHeadersNum())
                throw Exception("Kernel section index out of range");
            const char* symName = getSymbolName(index);
            const size_t binOffset = ULEV(sym.st_value);
            const size_t binSize = ULEV(sym.st_size);
            /// check conditions for symbol
            if (binOffset >= ULEV(dataShdr.sh_size))
                throw Exception("Kernel binary code offset out of range");
            if (usumGt(binOffset, binSize, ULEV(dataShdr.sh_size)))
                throw Exception("Kernel binary code offset and size out of range");
            if (binSize < 192)
                throw Exception("Kernel binary code size is too short");
            
            kernels[ki].setup = binaryCode + ULEV(dataShdr.sh_offset) + binOffset;
            // get size of setup (offset 16 of setup)
            const size_t textOffset = ULEV(*reinterpret_cast<uint32_t*>(
                            kernels[ki].setup+16));
            
            if (textOffset >= binSize)
                throw Exception("Kernel text offset out of range");
            kernels[ki].setupSize = textOffset;
            kernels[ki].code = kernels[ki].setup + textOffset;
            kernels[ki].codeSize = binSize-textOffset;
            const size_t len = ::strlen(symName);
            kernels[ki].kernelName = CString(symName+10, symName+len-7);
            if (hasKernelDataMap()) // kernel data map
                kernelDataMap[ki] = std::make_pair(kernels[ki].kernelName, ki);
            ki++;
        }
        // sort kernel data map
        if (hasKernelDataMap())
            mapSort(kernelDataMap.begin(), kernelDataMap.end());
    }
    // get global data - from section
    try
    {
        const Elf64_Shdr& gdataShdr = getSectionHeader(".hsadata_readonly_agent");
        globalDataSize = ULEV(gdataShdr.sh_size);
        globalData = binaryCode + ULEV(gdataShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
    
    try
    {
        const Elf64_Shdr& dataShdr = getSectionHeader(".hsaimage_samplerinit");
        samplerInitSize = ULEV(dataShdr.sh_size);
        samplerInit = binaryCode + ULEV(dataShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
    
    try
    {
        const Elf64_Shdr& relaShdr = getSectionHeader(".rela.hsatext");
        textRelEntrySize = ULEV(relaShdr.sh_entsize);
        if (textRelEntrySize==0)
            textRelEntrySize = sizeof(Elf64_Rela);
        textRelsNum = ULEV(relaShdr.sh_size)/textRelEntrySize;
        textRela = binaryCode + ULEV(relaShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
    
    try
    {
        const Elf64_Shdr& relaShdr = getSectionHeader(".rela.hsadata_readonly_agent");
        globalDataRelEntrySize = ULEV(relaShdr.sh_entsize);
        if (globalDataRelEntrySize==0)
            globalDataRelEntrySize = sizeof(Elf64_Rela);
        globalDataRelsNum = ULEV(relaShdr.sh_size)/globalDataRelEntrySize;
        globalDataRela = binaryCode + ULEV(relaShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
}

/* AmdCL2MainGPUBinary */

static const KernelArgType cl20ArgTypeVectorTable[] =
{
    KernelArgType::CHAR,
    KernelArgType::CHAR2,
    KernelArgType::CHAR3,
    KernelArgType::CHAR4,
    KernelArgType::CHAR8,
    KernelArgType::CHAR16,
    KernelArgType::SHORT,
    KernelArgType::SHORT2,
    KernelArgType::SHORT3,
    KernelArgType::SHORT4,
    KernelArgType::SHORT8,
    KernelArgType::SHORT16,
    KernelArgType::INT,
    KernelArgType::INT2,
    KernelArgType::INT3,
    KernelArgType::INT4,
    KernelArgType::INT8,
    KernelArgType::INT16,
    KernelArgType::LONG,
    KernelArgType::LONG2,
    KernelArgType::LONG3,
    KernelArgType::LONG4,
    KernelArgType::LONG8,
    KernelArgType::LONG16,
    KernelArgType::VOID,
    KernelArgType::VOID,
    KernelArgType::VOID,
    KernelArgType::VOID,
    KernelArgType::VOID,
    KernelArgType::VOID,
    KernelArgType::FLOAT,
    KernelArgType::FLOAT2,
    KernelArgType::FLOAT3,
    KernelArgType::FLOAT4,
    KernelArgType::FLOAT8,
    KernelArgType::FLOAT16,
    KernelArgType::DOUBLE,
    KernelArgType::DOUBLE2,
    KernelArgType::DOUBLE3,
    KernelArgType::DOUBLE4,
    KernelArgType::DOUBLE8,
    KernelArgType::DOUBLE16
};

static const cxuint vectorIdTable[17] =
{ UINT_MAX, 0, 1, 2, 3, UINT_MAX, UINT_MAX, UINT_MAX, 4,
  UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, 5 };

static void getCL2KernelInfo(size_t metadataSize, cxbyte* metadata,
             KernelInfo& kernelInfo, AmdGPUKernelHeader& kernelHeader)
{
    if (metadataSize < 8+32+32)
        throw Exception("Kernel metadata is too short");
    
    const AmdCL2GPUMetadataHeader* hdrStruc =
            reinterpret_cast<const AmdCL2GPUMetadataHeader*>(metadata);
    kernelHeader.size = ULEV(hdrStruc->size);
    if (kernelHeader.size >= metadataSize)
        throw Exception("Metadata header size out of range");
    if (kernelHeader.size < sizeof(AmdCL2GPUMetadataHeader))
        throw Exception("Metadata header is too short");
    kernelHeader.data = metadata;
    const uint32_t argNum = ULEV(hdrStruc->argNum);
    
    if (usumGt(ULEV(hdrStruc->firstNameLength), ULEV(hdrStruc->secondNameLength),
                metadataSize-kernelHeader.size-2))
        throw Exception("KernelArgEntries offset out of range");
    
    size_t argOffset = kernelHeader.size +
            ULEV(hdrStruc->firstNameLength)+ULEV(hdrStruc->secondNameLength)+2;
    const AmdCL2GPUKernelArgEntry* argPtr = reinterpret_cast<
            const AmdCL2GPUKernelArgEntry*>(metadata + kernelHeader.size +
            ULEV(hdrStruc->firstNameLength)+ULEV(hdrStruc->secondNameLength)+2);
    
    if(usumGt(argOffset, sizeof(AmdCL2GPUKernelArgEntry)*argNum, metadataSize))
        throw Exception("Number of arguments out of range");
    
    const char* strBase = (const char*)metadata;
    size_t strOffset = argOffset + sizeof(AmdCL2GPUKernelArgEntry)*(argNum+1);
    
    kernelInfo.argInfos.resize(argNum);
    for (uint32_t i = 0; i < argNum; i++, argPtr++)
    {
        AmdKernelArg& arg = kernelInfo.argInfos[i];
        if (ULEV(argPtr->size)!=sizeof(AmdCL2GPUKernelArgEntry))
            throw Exception("Kernel ArgEntry size doesn't match");
        // get name of argument
        size_t nameSize = ULEV(argPtr->argNameSize);
        if (usumGt(strOffset, nameSize+1, metadataSize))
            throw Exception("Kernel ArgEntry name size out of range");
        arg.argName.assign(strBase+strOffset, nameSize);
        // get name of type of argument
        strOffset += nameSize+1;
        nameSize = ULEV(argPtr->typeNameSize);
        if (usumGt(strOffset, nameSize+1, metadataSize))
            throw Exception("Kernel ArgEntry typename size out of range");
        arg.typeName.assign(strBase+strOffset, nameSize);
        strOffset += nameSize+1;
        
        /// determine arg type
        uint32_t vectorSize = ULEV(argPtr->vectorLength);
        uint32_t argType = ULEV(argPtr->argType);
        uint32_t kindOfType = ULEV(argPtr->kindOfType);
        arg.ptrSpace = KernelPtrSpace::NONE;
        arg.ptrAccess = KARG_PTR_NORMAL;
        arg.argType = KernelArgType::VOID;
        
        if (!ULEV(argPtr->isPointerOrPipe))
            // if not point or pipe (get regular type: scalar, image, sampler,...)
            switch(argType)
            {
                case 0:
                    if (kindOfType!=1) // not sampler
                        throw Exception("Wrong kernel argument type");
                    arg.argType = KernelArgType::SAMPLER;
                    break;
                case 1:  // read_only image
                case 2:  // write_only image
                case 3:  // read_write image
                    if (kindOfType!=2) // not image
                        throw Exception("Wrong kernel argument type");
                    arg.argType = KernelArgType::IMAGE;
                    arg.ptrAccess = (argType==1) ? KARG_PTR_READ_ONLY : (argType==2) ?
                             KARG_PTR_WRITE_ONLY : KARG_PTR_READ_WRITE;
                    arg.ptrSpace = KernelPtrSpace::GLOBAL;
                    break;
                case 6: // char
                case 7: // short
                case 8: // int
                case 9: // long
                case 11: // float
                case 12: // short
                {
                    if (kindOfType!=4) // not scalar
                        throw Exception("Wrong kernel argument type");
                    const cxuint vectorId = vectorIdTable[vectorSize];
                    if (vectorId == UINT_MAX)
                        throw Exception("Wrong vector size");
                    arg.argType = cl20ArgTypeVectorTable[(argType-6)*6 + vectorId];
                    break;
                }
                case 15:
                    if (kindOfType!=4) // not scalar
                        throw Exception("Wrong kernel argument type");
                    arg.argType = KernelArgType::STRUCTURE;
                    break;
                case 18:
                    if (kindOfType!=7) // not scalar
                        throw Exception("Wrong kernel argument type");
                    arg.argType = KernelArgType::CMDQUEUE;
                    break;
                default:
                    throw Exception("Wrong kernel argument type");
            }
        else // otherwise is pointer or pipe
        {
            uint32_t ptrSpace = ULEV(argPtr->ptrSpace);
            if (argType == 7) // pointer
                arg.argType = (argPtr->isPipe==0) ? KernelArgType::POINTER :
                    KernelArgType::PIPE;
            else if (argType == 15)
                arg.argType = KernelArgType::POINTER;
            if (arg.argType == KernelArgType::POINTER)
            {   // if pointer
                if (ptrSpace==3)
                    arg.ptrSpace = KernelPtrSpace::LOCAL;
                else if (ptrSpace==4)
                    arg.ptrSpace = KernelPtrSpace::GLOBAL;
                else if (ptrSpace==5)
                    arg.ptrSpace = KernelPtrSpace::CONSTANT;
                else
                    throw Exception("Illegal pointer space");
                // set access qualifiers (volatile, restrict, const)
                arg.ptrAccess = KARG_PTR_NORMAL;
                if (ULEV(argPtr->isConst))
                    arg.ptrAccess |= KARG_PTR_CONST;
                if (argPtr->isRestrict)
                    arg.ptrAccess |= KARG_PTR_RESTRICT;
                if (argPtr->isVolatile)
                    arg.ptrAccess |= KARG_PTR_VOLATILE;
            }
            else
            {
                if (ptrSpace!=4)
                    throw Exception("Illegal pipe space");
                arg.ptrSpace = KernelPtrSpace::GLOBAL;
            }
        }
    }
}

AmdCL2MainGPUBinary::AmdCL2MainGPUBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags) : AmdMainBinaryBase(AmdMainType::GPU_CL2_BINARY),
            ElfBinary64(binaryCodeSize, binaryCode, creationFlags), kernelsNum(0)
{
    std::vector<size_t> choosenMetadataSyms;
    std::vector<size_t> choosenISAMetadataSyms;
    std::vector<size_t> choosenBinSyms;
    
    const size_t symbolsNum = getSymbolsNum();
    
    if (hasInfoStrings())
        /// get info strings if needed
        for (size_t i = 0; i < symbolsNum; i++)
        {
            const char* symName = getSymbolName(i);
            if (::strcmp(symName, "__OpenCL_compiler_options")==0)
            {   // compile options
                const Elf64_Sym& sym = getSymbol(i);
                if (ULEV(sym.st_shndx) >= getSectionHeadersNum())
                    throw Exception("Compiler options section header out of range");
                const Elf64_Shdr& shdr = getSectionHeader(ULEV(sym.st_shndx));
                const size_t coOffset = ULEV(sym.st_value);
                const size_t coSize = ULEV(sym.st_size);
                if (coOffset >= ULEV(shdr.sh_size))
                    throw Exception("Compiler options offset out of range");
                if (usumGt(coOffset, coSize, ULEV(shdr.sh_size)))
                    throw Exception("Compiler options offset and size out of range");
                
                const char* coData = reinterpret_cast<const char*>(binaryCode) +
                            ULEV(shdr.sh_offset) + coOffset;
                compileOptions.assign(coData, coData + coSize);
            }
            else if (::strcmp(symName, "acl_version_string")==0)
            {   // acl version string
                const Elf64_Sym& sym = getSymbol(i);
                if (ULEV(sym.st_shndx) >= getSectionHeadersNum())
                    throw Exception("AclVersionString section header out of range");
                const Elf64_Shdr& shdr = getSectionHeader(ULEV(sym.st_shndx));
                const size_t aclOffset = ULEV(sym.st_value);
                const size_t aclSize = ULEV(sym.st_size);
                if (aclOffset >= ULEV(shdr.sh_size))
                    throw Exception("AclVersionString offset out of range");
                if (usumGt(aclOffset, aclSize, ULEV(shdr.sh_size)))
                    throw Exception("AclVersionString offset and size out of range");
                
                const char* aclVersionData = reinterpret_cast<const char*>(binaryCode) +
                            ULEV(shdr.sh_offset) + aclOffset;
                aclVersionString.assign(aclVersionData, aclVersionData + aclSize);
            }
        }
    
    // find symbol of kernel metadata, ISA metadata and binary
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = getSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len >= 35 && (::strncmp(symName, "__OpenCL_&__OpenCL_", 19) == 0 &&
                ::strcmp(symName+len-16, "_kernel_metadata") == 0)) // if metadata
            choosenMetadataSyms.push_back(i);
        else if (::strncmp(symName, "__ISA_&__OpenCL_", 16) == 0)
        {
            if (len >= 30 && ::strcmp(symName+len-14, "_kernel_binary") == 0)
                choosenBinSyms.push_back(i); //  if binary
            else if (len >= 32 && ::strcmp(symName+len-16, "_kernel_metadata") == 0)
                choosenISAMetadataSyms.push_back(i); // if ISA metadata
        }
    }
    
    const bool newInnerBinary = choosenBinSyms.empty();
    const Elf64_Shdr& textShdr = getSectionHeader(".text");
    if (newInnerBinary)
        innerBinary.reset(new AmdCL2InnerGPUBinary(ULEV(textShdr.sh_size),
                       binaryCode + ULEV(textShdr.sh_offset),
                       creationFlags >> AMDBIN_INNER_SHIFT));
    else // old driver
        innerBinary.reset(new AmdCL2OldInnerGPUBinary(this, ULEV(textShdr.sh_size),
                       binaryCode + ULEV(textShdr.sh_offset),
                       creationFlags >> AMDBIN_INNER_SHIFT));
    
    // get metadata
    if (hasKernelInfo())
    {
        kernelInfos.resize(choosenMetadataSyms.size());
        kernelHeaders.reset(new AmdGPUKernelHeader[choosenMetadataSyms.size()]);
        if (hasKernelInfoMap())
        {
            kernelInfosMap.resize(choosenMetadataSyms.size());
            isaMetadataMap.resize(choosenISAMetadataSyms.size());
        }
        metadatas.reset(new AmdCL2GPUKernelMetadata[kernelInfos.size()]);
        isaMetadatas.resize(choosenISAMetadataSyms.size());
        size_t ki = 0;
        // main loop
        for (size_t index: choosenMetadataSyms)
        {
            const Elf64_Sym& mtsym = getSymbol(index);
            const char* mtName = getSymbolName(index);
            if (ULEV(mtsym.st_shndx) >= getSectionHeadersNum())
                throw Exception("Kernel Metadata section header out of range");
            const Elf64_Shdr& shdr = getSectionHeader(ULEV(mtsym.st_shndx));
            const size_t mtOffset = ULEV(mtsym.st_value);
            const size_t mtSize = ULEV(mtsym.st_size);
            /// offset and size verifying
            if (mtOffset >= ULEV(shdr.sh_size))
                throw Exception("Kernel Metadata offset out of range");
            if (usumGt(mtOffset, mtSize, ULEV(shdr.sh_size)))
                throw Exception("Kernel Metadata offset and size out of range");
            
            cxbyte* metadata = binaryCode + ULEV(shdr.sh_offset) + mtOffset;
            getCL2KernelInfo(mtSize, metadata, kernelInfos[ki], kernelHeaders[ki]);
            size_t len = ::strlen(mtName);
            // set kernel name from symbol name (__OpenCL_&__OpenCL_[name]_kernel_metadata)
            kernelHeaders[ki].kernelName = kernelInfos[ki].kernelName =
                        CString(mtName+19, mtName+len-16);
            if (hasKernelInfoMap())
                kernelInfosMap[ki] = std::make_pair(kernelInfos[ki].kernelName, ki);
            metadatas[ki] = { kernelInfos[ki].kernelName, mtSize, metadata };
            ki++;
        }
        
        ki = 0;
        for (size_t index: choosenISAMetadataSyms)
        {
            const Elf64_Sym& mtsym = getSymbol(index);
            const char* mtName = getSymbolName(index);
            if (ULEV(mtsym.st_shndx) >= getSectionHeadersNum())
                throw Exception("Kernel ISAMetadata section header out of range");
            const Elf64_Shdr& shdr = getSectionHeader(ULEV(mtsym.st_shndx));
            const size_t mtOffset = ULEV(mtsym.st_value);
            const size_t mtSize = ULEV(mtsym.st_size);
            /// offset and size verifying
            if (mtOffset >= ULEV(shdr.sh_size))
                throw Exception("Kernel ISAMetadata offset out of range");
            if (usumGt(mtOffset, mtSize, ULEV(shdr.sh_size)))
                throw Exception("Kernel ISAMetadata offset and size out of range");
            
            cxbyte* metadata = binaryCode + ULEV(shdr.sh_offset) + mtOffset;
            size_t len = ::strlen(mtName);
            CString kernelName = CString(mtName+16, mtName+len-16);
            isaMetadatas[ki] = { kernelName, mtSize, metadata };
            if (hasKernelInfoMap())
                isaMetadataMap[ki] = std::make_pair(kernelName, ki);
            ki++;
        }
        
        if (hasKernelInfoMap())
        {
            mapSort(kernelInfosMap.begin(), kernelInfosMap.end());
            mapSort(isaMetadataMap.begin(), isaMetadataMap.end());
        }
    }
}

const AmdCL2GPUKernelMetadata& AmdCL2MainGPUBinary::getMetadataEntry(
                    const char* name) const
{
    auto it = binaryMapFind(kernelInfosMap.begin(), kernelInfosMap.end(), name);
    if (it == kernelInfosMap.end())
        throw Exception("Can't find kernel metadata by name");
    return metadatas[it->second];
}

const AmdCL2GPUKernelMetadata& AmdCL2MainGPUBinary::getISAMetadataEntry(
                    const char* name) const
{
    auto it = binaryMapFind(isaMetadataMap.begin(), isaMetadataMap.end(), name);
    if (it == isaMetadataMap.end())
        throw Exception("Can't find kernel ISA metadata by name");
    return isaMetadatas[it->second];
}

bool CLRX::isAmdCL2Binary(size_t binarySize, const cxbyte* binary)
{
    if (!isElfBinary(binarySize, binary))
        return false;
    if (binary[EI_CLASS] != ELFCLASS64)
        return false;
    const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binary);
    if (ULEV(ehdr->e_machine) != 0xaf5b || ULEV(ehdr->e_flags)==0)
        return false;
    return true;
}
