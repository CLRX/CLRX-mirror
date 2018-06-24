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
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cstdint>
#include <utility>
#include <vector>
#include <CLRX/amdbin/Elf.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>

using namespace CLRX;

/* class AmdCL2InnerGPUBinaryBase */

AmdCL2InnerGPUBinaryBase::~AmdCL2InnerGPUBinaryBase()
{ }

const AmdCL2GPUKernel& AmdCL2InnerGPUBinaryBase::getKernelData(const char* name) const
{
    KernelDataMap::const_iterator it = binaryMapFind(
            kernelDataMap.begin(), kernelDataMap.end(), name);
    if (it == kernelDataMap.end())
        throw BinException("Can't find kernel name");
    return kernels[it->second];
}

/* AmdCL2OldInnerGPUBinary */

template<typename Types>
AmdCL2OldInnerGPUBinary::AmdCL2OldInnerGPUBinary(ElfBinaryTemplate<Types>* mainBinary,
            size_t binaryCodeSize, cxbyte* binaryCode, Flags _creationFlags)
        : creationFlags(_creationFlags), binarySize(binaryCodeSize), binary(binaryCode)
{
    if ((creationFlags & (AMDCL2BIN_CREATE_KERNELDATA|AMDCL2BIN_CREATE_KERNELSTUBS)) == 0)
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
        const typename Types::Sym& sym = mainBinary->getSymbol(index);
        const char* symName = mainBinary->getSymbolName(index);
        const size_t binOffset = ULEV(sym.st_value);
        const size_t binSize = ULEV(sym.st_size);
        /// check conditions for symbol
        if (textIndex != ULEV(sym.st_shndx))
            throw BinException("Kernel symbol outside text section");
        if (binOffset >= binaryCodeSize)
            throw BinException("Kernel binary code offset out of range");
        if (usumGt(binOffset, binSize, binaryCodeSize))
            throw BinException("Kernel binary code offset and size out of range");
        if (binSize < 256+192)
            throw BinException("Kernel binary code size is too short");
        
        AmdCL2GPUKernelStub kernelStub;
        AmdCL2GPUKernel kernelData;
        // set data for stub
        kernelStub.data = binaryCode + binOffset;
        const size_t setupOffset = ULEV(*reinterpret_cast<uint32_t*>(kernelStub.data));
        if (setupOffset >= binSize)
            throw BinException("Kernel setup offset out of range");
        // get size of setup (offset 16 of setup)
        kernelStub.size = setupOffset;
        kernelData.setup = kernelStub.data + setupOffset;
        // get text (code) offset after setup (HSA config)
        const size_t textOffset = ULEV(*reinterpret_cast<uint32_t*>(kernelData.setup+16));
        if (usumGe(textOffset, setupOffset, binSize))
            throw BinException("Kernel text offset out of range");
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
    // sort kernel data map array
    if (hasKernelDataMap())
        mapSort(kernelDataMap.begin(), kernelDataMap.end());
}

const AmdCL2GPUKernelStub& AmdCL2OldInnerGPUBinary::getKernelStub(const char* name) const
{
    KernelDataMap::const_iterator it = binaryMapFind(
            kernelDataMap.begin(), kernelDataMap.end(), name);
    if (it == kernelDataMap.end())
        throw BinException("Can't find kernel name");
    return kernelStubs[it->second];
}

/* AmdCL2InnerGPUBinary */

AmdCL2InnerGPUBinary::AmdCL2InnerGPUBinary(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags): ElfBinary64(binaryCodeSize, binaryCode, creationFlags),
            globalDataSize(0), globalData(nullptr), rwDataSize(0), rwData(nullptr),
            bssAlignment(0), bssSize(0), samplerInitSize(0), samplerInit(nullptr),
            textRelsNum(0), textRelEntrySize(0), textRela(nullptr),
            globalDataRelsNum(0), globalDataRelEntrySize(0), globalDataRela(nullptr)
{
    if (hasKernelData())
    {
        // get kernel datas and kernel stubs 
        std::vector<size_t> choosenSyms;
        const size_t symbolsNum = getSymbolsNum();
        // choosen symbols to find kernels
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
                throw BinException("Kernel section index out of range");
            const char* symName = getSymbolName(index);
            const size_t binOffset = ULEV(sym.st_value);
            const size_t binSize = ULEV(sym.st_size);
            /// check conditions for symbol
            if (binOffset >= ULEV(dataShdr.sh_size))
                throw BinException("Kernel binary code offset out of range");
            if (usumGt(binOffset, binSize, ULEV(dataShdr.sh_size)))
                throw BinException("Kernel binary code offset and size out of range");
            if (binSize < 192)
                throw BinException("Kernel binary code size is too short");
            
            kernels[ki].setup = binaryCode + ULEV(dataShdr.sh_offset) + binOffset;
            // get size of setup (offset 16 of setup) and code offset
            const size_t textOffset = ULEV(*reinterpret_cast<uint32_t*>(
                            kernels[ki].setup+16));
            
            if (textOffset >= binSize)
                throw BinException("Kernel text offset out of range");
            kernels[ki].setupSize = textOffset;
            kernels[ki].code = kernels[ki].setup + textOffset;
            kernels[ki].codeSize = binSize-textOffset;
            const size_t len = ::strlen(symName);
            // kernel name in string in form: '__OpenCL_XXXX_kernel', get name
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
    
    // get hsadata_global_agent (used by atomics)
    try
    {
        const Elf64_Shdr& rwShdr = getSectionHeader(".hsadata_global_agent");
        rwDataSize = ULEV(rwShdr.sh_size);
        rwData = binaryCode + ULEV(rwShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
    // get hsabss_gobal_agent
    try
    {
        const Elf64_Shdr& bssShdr = getSectionHeader(".hsabss_global_agent");
        bssSize = ULEV(bssShdr.sh_size);
        bssAlignment = ULEV(bssShdr.sh_addralign);
    }
    catch(const Exception& ex)
    { }
    
    // get ssection with sampler data '.hsaimage_samplerinit'
    try
    {
        const Elf64_Shdr& dataShdr = getSectionHeader(".hsaimage_samplerinit");
        samplerInitSize = ULEV(dataShdr.sh_size);
        samplerInit = binaryCode + ULEV(dataShdr.sh_offset);
    }
    catch(const Exception& ex)
    { }
    
    // get relocation section for text
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
    
    /// get relocation for hsadata_readonly_agent (readonly data section)
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

/* AmdCL2MainGPUBinary64 */

/// argument type vector table for OpenCL 2.0 binary format
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

// table of recognized vector sizes
static const cxuint vectorIdTable[17] =
{ UINT_MAX, 0, 1, 2, 3, UINT_MAX, UINT_MAX, UINT_MAX, 4,
  UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, 5 };

template<typename Types>
static void getCL2KernelInfo(size_t metadataSize, cxbyte* metadata,
             KernelInfo& kernelInfo, AmdGPUKernelHeader& kernelHeader, bool& crimson16)
{
    crimson16 = false;
    if (metadataSize < 8+32+32)
        throw BinException("Kernel metadata is too short");
    
    const typename Types::MetadataHeader* hdrStruc =
            reinterpret_cast<const typename Types::MetadataHeader*>(metadata);
    kernelHeader.size = ULEV(hdrStruc->size);
    // checking kernel header size in metadata region
    if (kernelHeader.size >= metadataSize)
        throw BinException("Metadata header size out of range");
    if (kernelHeader.size < sizeof(typename Types::MetadataHeader))
        throw BinException("Metadata header is too short");
    kernelHeader.data = metadata;
    const uint32_t argsNum = ULEV(hdrStruc->argsNum);
    size_t vecTypeHintLength = 0;
    if (kernelHeader.size >= Types::newMetadataHeaderSize)
    {
        const typename Types::MetadataHeaderEnd* hdrEnd =
            reinterpret_cast<const typename Types::MetadataHeaderEnd*>(
                metadata +  Types::newMetadataHeaderSize -
                        sizeof(typename Types::MetadataHeaderEnd));
        vecTypeHintLength = ULEV(hdrEnd->vecTypeHintLength);
    }
    
    if (usumGt(ULEV(hdrStruc->firstNameLength), ULEV(hdrStruc->secondNameLength),
                metadataSize-kernelHeader.size-2))
        throw BinException("KernelArgEntries offset out of range");
    
    size_t argOffset = kernelHeader.size +
            ULEV(hdrStruc->firstNameLength)+ULEV(hdrStruc->secondNameLength)+2;
    // fix for latest Crimson drivers
    if (vecTypeHintLength!=0 || ULEV(*(const uint32_t*)(metadata+argOffset)) ==
                (sizeof(typename Types::KernelArgEntry)<<8))
    {
        crimson16 = true;
        argOffset += vecTypeHintLength + 1;
    }
    const typename Types::KernelArgEntry* argPtr = reinterpret_cast<
            const typename Types::KernelArgEntry*>(metadata + argOffset);
    
    if(usumGt(argOffset, sizeof(typename Types::KernelArgEntry)*(argsNum+1), metadataSize))
        throw BinException("Number of arguments out of range");
    
    const char* strBase = (const char*)metadata;
    size_t strOffset = argOffset + sizeof(typename Types::KernelArgEntry)*(argsNum+1);
    
    kernelInfo.argInfos.resize(argsNum);
    for (uint32_t i = 0; i < argsNum; i++, argPtr++)
    {
        AmdKernelArg& arg = kernelInfo.argInfos[i];
        if (ULEV(argPtr->size)!=sizeof(typename Types::KernelArgEntry))
            throw BinException("Kernel ArgEntry size doesn't match");
        // get name of argument
        size_t nameSize = ULEV(argPtr->argNameSize);
        if (usumGt(strOffset, nameSize+1, metadataSize))
            throw BinException("Kernel ArgEntry name size out of range");
        arg.argName.assign(strBase+strOffset, nameSize);
        // get name of type of argument
        strOffset += nameSize+1;
        nameSize = ULEV(argPtr->typeNameSize);
        if (usumGt(strOffset, nameSize+1, metadataSize))
            throw BinException("Kernel ArgEntry typename size out of range");
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
                        throw BinException("Wrong kernel argument type");
                    arg.argType = KernelArgType::SAMPLER;
                    break;
                case 1:  // read_only image
                case 2:  // write_only image
                case 3:  // read_write image
                    if (kindOfType==2) // image
                    {
                        arg.argType = KernelArgType::IMAGE;
                        arg.ptrAccess = (argType==1) ? KARG_PTR_READ_ONLY : (argType==2) ?
                                 KARG_PTR_WRITE_ONLY : KARG_PTR_READ_WRITE;
                        arg.ptrSpace = KernelPtrSpace::GLOBAL;
                    }
                    else if (argType==2 || argType == 3)
                    {
                        if (kindOfType!=4) // not scalar
                            throw BinException("Wrong kernel argument type");
                        arg.argType = (argType==3) ?
                            KernelArgType::SHORT : KernelArgType::CHAR;
                    }
                    else
                        throw BinException("Wrong kernel argument type");
                    break;
                case 4: // int
                case 5: // long
                    if (kindOfType!=4) // not scalar
                        throw BinException("Wrong kernel argument type");
                    arg.argType = (argType==5) ?
                        KernelArgType::LONG : KernelArgType::INT;
                    break;
                case 6: // char
                case 7: // short
                case 8: // int
                case 9: // long
                case 11: // float
                case 12: // double
                {
                    if (kindOfType!=4) // not scalar
                        throw BinException("Wrong kernel argument type");
                    const cxuint vectorId = vectorIdTable[vectorSize];
                    if (vectorId == UINT_MAX)
                        throw BinException("Wrong vector size");
                    arg.argType = cl20ArgTypeVectorTable[(argType-6)*6 + vectorId];
                    break;
                }
                case 15:
                    if (kindOfType!=4) // not scalar
                        throw BinException("Wrong kernel argument type");
                    arg.argType = KernelArgType::STRUCTURE;
                    break;
                case 18:
                    if (kindOfType!=7) // not scalar
                        throw BinException("Wrong kernel argument type");
                    arg.argType = KernelArgType::CMDQUEUE;
                    break;
                default:
                    throw BinException("Wrong kernel argument type");
                    break;
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
            {
                // if pointer
                if (ptrSpace==3)
                    arg.ptrSpace = KernelPtrSpace::LOCAL;
                else if (ptrSpace==4)
                    arg.ptrSpace = KernelPtrSpace::GLOBAL;
                else if (ptrSpace==5)
                    arg.ptrSpace = KernelPtrSpace::CONSTANT;
                else
                    throw BinException("Illegal pointer space");
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
                // global space for pipe
                if (ptrSpace!=4)
                    throw BinException("Illegal pipe space");
                arg.ptrSpace = KernelPtrSpace::GLOBAL;
            }
        }
    }
}

struct CLRX_INTERNAL AmdCL2Types32 : Elf32Types
{
    typedef ElfBinary32 ElfBinary;
    typedef AmdCL2GPUMetadataHeader32 MetadataHeader;
    typedef AmdCL2GPUMetadataHeaderEnd32 MetadataHeaderEnd;
    typedef AmdCL2GPUKernelArgEntry32 KernelArgEntry;
    static const size_t newMetadataHeaderSize = 0xa4;
};

struct CLRX_INTERNAL AmdCL2Types64 : Elf64Types
{
    typedef ElfBinary64 ElfBinary;
    typedef AmdCL2GPUMetadataHeader64 MetadataHeader;
    typedef AmdCL2GPUMetadataHeaderEnd64 MetadataHeaderEnd;
    typedef AmdCL2GPUKernelArgEntry64 KernelArgEntry;
    static const size_t newMetadataHeaderSize = 0x110;
};

/* AMD CL2 GPU Binary base class */

AmdCL2MainGPUBinaryBase::AmdCL2MainGPUBinaryBase(AmdMainType mainType)
        : AmdMainBinaryBase(mainType), driverVersion(180005),
          kernelsNum(0)
{ }

const AmdCL2GPUKernelMetadata& AmdCL2MainGPUBinaryBase::getMetadataEntry(
                    const char* name) const
{
    auto it = binaryMapFind(kernelInfosMap.begin(), kernelInfosMap.end(), name);
    if (it == kernelInfosMap.end())
        throw BinException("Can't find kernel metadata by name");
    return metadatas[it->second];
}

const AmdCL2GPUKernelMetadata& AmdCL2MainGPUBinaryBase::getISAMetadataEntry(
                    const char* name) const
{
    auto it = binaryMapFind(isaMetadataMap.begin(), isaMetadataMap.end(), name);
    if (it == isaMetadataMap.end())
        throw BinException("Can't find kernel ISA metadata by name");
    return isaMetadatas[it->second];
}

template<typename Types>
void AmdCL2MainGPUBinaryBase::initMainGPUBinary(typename Types::ElfBinary& elfBin)
{
    std::vector<size_t> choosenMetadataSyms;
    std::vector<size_t> choosenISAMetadataSyms;
    std::vector<size_t> choosenBinSyms;
    
    Flags creationFlags = elfBin.getCreationFlags();
    cxbyte* binaryCode = elfBin.getBinaryCode();
    
    const size_t symbolsNum = elfBin.getSymbolsNum();
    
    if ((creationFlags & AMDBIN_CREATE_INFOSTRINGS) != 0)
        /// get info strings if needed
        for (size_t i = 0; i < symbolsNum; i++)
        {
            const char* symName = elfBin.getSymbolName(i);
            if (::strcmp(symName, "__OpenCL_compiler_options")==0)
            {
                // compile options
                const typename Types::Sym& sym = elfBin.getSymbol(i);
                if (ULEV(sym.st_shndx) >= elfBin.getSectionHeadersNum())
                    throw BinException("Compiler options section header out of range");
                const typename Types::Shdr& shdr =
                            elfBin.getSectionHeader(ULEV(sym.st_shndx));
                const size_t coOffset = ULEV(sym.st_value);
                const size_t coSize = ULEV(sym.st_size);
                // checking compile options offset and size
                if (coOffset >= ULEV(shdr.sh_size))
                    throw BinException("Compiler options offset out of range");
                if (usumGt(coOffset, coSize, ULEV(shdr.sh_size)))
                    throw BinException("Compiler options offset and size out of range");
                
                const char* coData = reinterpret_cast<const char*>(binaryCode) +
                            ULEV(shdr.sh_offset) + coOffset;
                compileOptions.assign(coData, coData + coSize);
            }
            else if (::strcmp(symName, "acl_version_string")==0)
            {
                // acl version string
                const typename Types::Sym& sym = elfBin.getSymbol(i);
                if (ULEV(sym.st_shndx) >= elfBin.getSectionHeadersNum())
                    throw BinException("AclVersionString section header out of range");
                const typename Types::Shdr& shdr =
                        elfBin.getSectionHeader(ULEV(sym.st_shndx));
                const size_t aclOffset = ULEV(sym.st_value);
                const size_t aclSize = ULEV(sym.st_size);
                // checking acl offset and acl size
                if (aclOffset >= ULEV(shdr.sh_size))
                    throw BinException("AclVersionString offset out of range");
                if (usumGt(aclOffset, aclSize, ULEV(shdr.sh_size)))
                    throw BinException("AclVersionString offset and size out of range");
                
                const char* aclVersionData = reinterpret_cast<const char*>(binaryCode) +
                            ULEV(shdr.sh_offset) + aclOffset;
                aclVersionString.assign(aclVersionData, aclVersionData + aclSize);
            }
        }
    
    // find symbol of kernel metadata, ISA metadata and binary
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = elfBin.getSymbolName(i);
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
    uint16_t textIndex = SHN_UNDEF;
    driverVersion = newInnerBinary ? 191205: 180005;
    try
    { textIndex = elfBin.getSectionIndex(".text"); }
    catch(const Exception& ex)
    {
        if (!choosenMetadataSyms.empty())
            throw;  // throw exception if least one kernel is present
        else // old driver version
            driverVersion = 180005;
    }
    if (textIndex != SHN_UNDEF)
    {
        const typename Types::Shdr& textShdr = elfBin.getSectionHeader(textIndex);
        if (newInnerBinary)
        {
            innerBinary.reset(new AmdCL2InnerGPUBinary(ULEV(textShdr.sh_size),
                           binaryCode + ULEV(textShdr.sh_offset),
                           creationFlags >> AMDBIN_INNER_SHIFT));
            // detect new format from Crimson 16.4
            const auto& innerBin = getInnerBinary();
            driverVersion = (innerBin.getSymbolsNum()!=0 &&
                    innerBin.getSymbolName(0)[0]==0) ? 200406 : 191205;
            try
            {
                // special detection for first AMDGPU-PRO driver (may be bug in driver)
                const Elf64_Shdr& noteShdr = innerBin.getSectionHeader(".note");
                const cxbyte* noteContent = innerBin.getSectionContent(".note");
                const size_t noteSize = ULEV(noteShdr.sh_size);
                if (noteSize == 200 && noteContent[197]!=0)
                    driverVersion = 203603;
            }
            catch(const Exception& ex)
            { }
        }
        else // old driver
            innerBinary.reset(new AmdCL2OldInnerGPUBinary(&elfBin, ULEV(textShdr.sh_size),
                           binaryCode + ULEV(textShdr.sh_offset),
                           creationFlags >> AMDBIN_INNER_SHIFT));
    }
    
    // get metadata
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
    {
        kernelInfos.resize(choosenMetadataSyms.size());
        kernelHeaders.reset(new AmdGPUKernelHeader[choosenMetadataSyms.size()]);
        if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
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
            const typename Types::Sym& mtsym = elfBin.getSymbol(index);
            const char* mtName = elfBin.getSymbolName(index);
            if (ULEV(mtsym.st_shndx) >= elfBin.getSectionHeadersNum())
                throw BinException("Kernel Metadata section header out of range");
            const typename Types::Shdr& shdr =
                    elfBin.getSectionHeader(ULEV(mtsym.st_shndx));
            const size_t mtOffset = ULEV(mtsym.st_value);
            const size_t mtSize = ULEV(mtsym.st_size);
            /// offset and size verifying
            if (mtOffset >= ULEV(shdr.sh_size))
                throw BinException("Kernel Metadata offset out of range");
            if (usumGt(mtOffset, mtSize, ULEV(shdr.sh_size)))
                throw BinException("Kernel Metadata offset and size out of range");
            
            cxbyte* metadata = binaryCode + ULEV(shdr.sh_offset) + mtOffset;
            bool crimson16 = false;
            getCL2KernelInfo<Types>(mtSize, metadata, kernelInfos[ki],
                                        kernelHeaders[ki], crimson16);
            size_t len = ::strlen(mtName);
            // set kernel name from symbol name (__OpenCL_&__OpenCL_[name]_kernel_metadata)
            kernelHeaders[ki].kernelName = kernelInfos[ki].kernelName =
                        CString(mtName+19, mtName+len-16);
            if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
                kernelInfosMap[ki] = std::make_pair(kernelInfos[ki].kernelName, ki);
            metadatas[ki] = { kernelInfos[ki].kernelName, mtSize, metadata };
            ki++;
            if (crimson16 && driverVersion < 200406) // if AMD Crimson 16
                driverVersion = 200406;
        }
        
        ki = 0;
        // getting isa metadatas
        for (size_t index: choosenISAMetadataSyms)
        {
            const typename Types::Sym& mtsym = elfBin.getSymbol(index);
            const char* mtName = elfBin.getSymbolName(index);
            if (ULEV(mtsym.st_shndx) >= elfBin.getSectionHeadersNum())
                throw BinException("Kernel ISAMetadata section header out of range");
            const typename Types::Shdr& shdr =
                        elfBin.getSectionHeader(ULEV(mtsym.st_shndx));
            const size_t mtOffset = ULEV(mtsym.st_value);
            const size_t mtSize = ULEV(mtsym.st_size);
            /// offset and size verifying
            if (mtOffset >= ULEV(shdr.sh_size))
                throw BinException("Kernel ISAMetadata offset out of range");
            if (usumGt(mtOffset, mtSize, ULEV(shdr.sh_size)))
                throw BinException("Kernel ISAMetadata offset and size out of range");
            
            cxbyte* metadata = binaryCode + ULEV(shdr.sh_offset) + mtOffset;
            size_t len = ::strlen(mtName);
            CString kernelName = CString(mtName+16, mtName+len-16);
            isaMetadatas[ki] = { kernelName, mtSize, metadata };
            if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
                isaMetadataMap[ki] = std::make_pair(kernelName, ki);
            ki++;
        }
        // sort kernel info map and isa metadata map (are arrays)
        if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
        {
            mapSort(kernelInfosMap.begin(), kernelInfosMap.end());
            mapSort(isaMetadataMap.begin(), isaMetadataMap.end());
        }
    }
}

/* helpers for determing gpu device type */

struct CLRX_INTERNAL CL2GPUDeviceCodeEntry
{
    uint32_t elfFlags;
    GPUDeviceType deviceType;
};

// tables with GPU device codes for specific driver version
// for almost cases are matches.
// IMPORTANT: gpu device code tables must be sorted by elfflag

/* 1912.05 driver device table list */
static const CL2GPUDeviceCodeEntry cl2GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 15, GPUDeviceType::DUMMY },
    { 16, GPUDeviceType::CARRIZO },
    { 17, GPUDeviceType::FIJI }
};

/* 2004.06 driver device table list */
static const CL2GPUDeviceCodeEntry cl2_16_3GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 12, GPUDeviceType::HORSE },
    { 13, GPUDeviceType::GOOSE },
    { 15, GPUDeviceType::CARRIZO },
    { 16, GPUDeviceType::FIJI },
    { 17, GPUDeviceType::STONEY }
};

/* 1801.05 driver device table list */
static const CL2GPUDeviceCodeEntry cl2_15_7GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 15, GPUDeviceType::CARRIZO },
    { 16, GPUDeviceType::FIJI }
};

/* driver version: 2036.3 */
static const CL2GPUDeviceCodeEntry cl2GPUPROGpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 13, GPUDeviceType::CARRIZO },
    { 14, GPUDeviceType::FIJI },
    { 15, GPUDeviceType::STONEY },
    { 16, GPUDeviceType::BAFFIN },
    { 17, GPUDeviceType::ELLESMERE }
};

static const CL2GPUDeviceCodeEntry cl2_2236GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 12, GPUDeviceType::CARRIZO },
    { 13, GPUDeviceType::FIJI },
    { 14, GPUDeviceType::STONEY },
    { 15, GPUDeviceType::BAFFIN },
    { 16, GPUDeviceType::ELLESMERE },
    { 17, GPUDeviceType::GFX900 },
    { 18, GPUDeviceType::GFX804 }
};

static const CL2GPUDeviceCodeEntry cl2_2264GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 13, GPUDeviceType::CARRIZO },
    { 14, GPUDeviceType::FIJI },
    { 15, GPUDeviceType::STONEY },
    { 16, GPUDeviceType::BAFFIN },
    { 17, GPUDeviceType::ELLESMERE },
    { 18, GPUDeviceType::GFX804 },
    { 19, GPUDeviceType::GFX900 }
};

static const CL2GPUDeviceCodeEntry cl2_2348GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 14, GPUDeviceType::CARRIZO },
    { 15, GPUDeviceType::FIJI },
    { 16, GPUDeviceType::STONEY },
    { 17, GPUDeviceType::BAFFIN },
    { 18, GPUDeviceType::ELLESMERE },
    { 19, GPUDeviceType::GFX804 },
    { 20, GPUDeviceType::GFX900 },
    { 21, GPUDeviceType::GFX901 }
};

static const CL2GPUDeviceCodeEntry cl2_2442GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 12, GPUDeviceType::CARRIZO },
    { 13, GPUDeviceType::FIJI },
    { 14, GPUDeviceType::STONEY },
    { 15, GPUDeviceType::BAFFIN },
    { 16, GPUDeviceType::ELLESMERE },
    { 17, GPUDeviceType::GFX900 },
    { 18, GPUDeviceType::GFX804 },
    { 19, GPUDeviceType::GFX901 }
};

static const CL2GPUDeviceCodeEntry cl2_2482GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 12, GPUDeviceType::CARRIZO },
    { 13, GPUDeviceType::FIJI },
    { 14, GPUDeviceType::STONEY },
    { 16, GPUDeviceType::BAFFIN },
    { 18, GPUDeviceType::ELLESMERE },
    { 19, GPUDeviceType::GFX900 },
    { 20, GPUDeviceType::GFX804 },
    { 21, GPUDeviceType::GFX901 }
};

static const CL2GPUDeviceCodeEntry cl2_2527GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 12, GPUDeviceType::CARRIZO },
    { 13, GPUDeviceType::FIJI },
    { 14, GPUDeviceType::STONEY },
    { 16, GPUDeviceType::BAFFIN },
    { 18, GPUDeviceType::ELLESMERE },
    { 20, GPUDeviceType::GFX900 },
    { 21, GPUDeviceType::GFX804 },
    { 23, GPUDeviceType::GFX901 },
    { 24, GPUDeviceType::GFX902 },
    { 25, GPUDeviceType::GFX903 },
    { 26, GPUDeviceType::GFX904 },
    { 27, GPUDeviceType::GFX905 }
};

static const CL2GPUDeviceCodeEntry cl2_2580GpuDeviceCodeTable[] =
{
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 4, GPUDeviceType::MULLINS },
    { 6, GPUDeviceType::BONAIRE },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 12, GPUDeviceType::CARRIZO },
    { 13, GPUDeviceType::FIJI },
    { 14, GPUDeviceType::STONEY },
    { 16, GPUDeviceType::BAFFIN },
    { 18, GPUDeviceType::ELLESMERE },
    { 20, GPUDeviceType::GFX900 },
    { 21, GPUDeviceType::GFX804 },
    { 23, GPUDeviceType::GFX901 },
    { 24, GPUDeviceType::GFX902 },
    { 25, GPUDeviceType::GFX903 },
    { 26, GPUDeviceType::GFX904 },
    { 27, GPUDeviceType::GFX905 },
    { 28, GPUDeviceType::GFX906 },
    { 29, GPUDeviceType::GFX907 }
};

struct CLRX_INTERNAL CL2GPUCodeTable
{
    cxuint toDriverVersion;   // to driver version
    const CL2GPUDeviceCodeEntry* table;
    size_t tableSize;
};

// table with entries which refers specific GPU device code table
// choosen by toDriverVersion field (using lower_bound)
static const CL2GPUCodeTable cl2CodeTables[] =
{
    { 191205U, cl2_15_7GpuDeviceCodeTable,
        sizeof(cl2_15_7GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 200406U, cl2GpuDeviceCodeTable,
        sizeof(cl2GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 203603U, cl2_16_3GpuDeviceCodeTable,
        sizeof(cl2_16_3GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 223600U, cl2GPUPROGpuDeviceCodeTable,
        sizeof(cl2GPUPROGpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 226400U, cl2_2236GpuDeviceCodeTable,
        sizeof(cl2_2236GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 234800U, cl2_2264GpuDeviceCodeTable,
        sizeof(cl2_2264GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 244200U, cl2_2348GpuDeviceCodeTable,
        sizeof(cl2_2348GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 248200U, cl2_2442GpuDeviceCodeTable,
        sizeof(cl2_2442GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 252700U, cl2_2482GpuDeviceCodeTable,
        sizeof(cl2_2482GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { 258000U, cl2_2527GpuDeviceCodeTable,
        sizeof(cl2_2527GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) },
    { UINT_MAX, cl2_2580GpuDeviceCodeTable,
        sizeof(cl2_2580GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry) }
};

static const GPUDeviceType gpuGFX9SteppingTable[] =
{
    GPUDeviceType::GFX900,
    GPUDeviceType::GFX901,
    GPUDeviceType::GFX902,
    GPUDeviceType::GFX903,
    GPUDeviceType::GFX904,
    GPUDeviceType::GFX905,
    GPUDeviceType::GFX906,
    GPUDeviceType::GFX907
};

template<typename Types>
GPUDeviceType AmdCL2MainGPUBinaryBase::determineGPUDeviceTypeInt(
        const typename Types::ElfBinary& binary, uint32_t& outArchMinor,
        uint32_t& outArchStepping, cxuint inDriverVersion) const
{
    const uint32_t elfFlags = ULEV(binary.getHeader().e_flags);
    
    // detect GPU device from elfMachine field from ELF header
    cxuint entriesNum = 0;
    const CL2GPUDeviceCodeEntry* gpuCodeTable = nullptr;
    cxuint inputDriverVersion = 0;
    if (inDriverVersion == 0)
        inputDriverVersion = this->driverVersion;
    else
        inputDriverVersion = inDriverVersion;
    
    const size_t codeTablesNum = sizeof(cl2CodeTables)/sizeof(CL2GPUCodeTable);
    // ctit - iterator to GPU device code table entry for this driver version
    auto ctit = std::upper_bound(cl2CodeTables, cl2CodeTables+codeTablesNum,
                CL2GPUCodeTable{ inputDriverVersion },
                [](const CL2GPUCodeTable& a, const CL2GPUCodeTable& b)
                { return a.toDriverVersion < b.toDriverVersion; });
    if (ctit == cl2CodeTables+codeTablesNum)
        ctit--; // to previous table
    //std::cout << "ctitver: " << ctit->toDriverVersion << "\n";
    gpuCodeTable = ctit->table;
    entriesNum = ctit->tableSize;
    
    cxuint index = binaryFind(gpuCodeTable, gpuCodeTable + entriesNum, { elfFlags },
              [] (const CL2GPUDeviceCodeEntry& l, const CL2GPUDeviceCodeEntry& r)
              { return l.elfFlags < r.elfFlags; }) - gpuCodeTable;
    bool knownGPUType = index != entriesNum;
    
    GPUDeviceType deviceType = GPUDeviceType::CAPE_VERDE;
    GPUArchitecture arch = GPUArchitecture::GCN1_1;
    if (knownGPUType)
    {
        deviceType = gpuCodeTable[index].deviceType;
        arch = getGPUArchitectureFromDeviceType(deviceType);
    }
    uint32_t archMinor = 0;
    uint32_t archStepping = 0;
    
    bool isInnerNewBinary = hasInnerBinary() && this->driverVersion>=191205;
    if (isInnerNewBinary)
    {
        const AmdCL2InnerGPUBinary& innerBin = getInnerBinary();
        
        const cxbyte* noteContent = (const cxbyte*)innerBin.getNotes();
        if (noteContent==nullptr)
            throw BinException("Missing notes in inner binary!");
        size_t notesSize = innerBin.getNotesSize();
        // find note about AMDGPU
        for (size_t offset = 0; offset < notesSize; )
        {
            const typename Types::Nhdr* nhdr =
                        (const typename Types::Nhdr*)(noteContent + offset);
            size_t namesz = ULEV(nhdr->n_namesz);
            size_t descsz = ULEV(nhdr->n_descsz);
            if (usumGt(offset, namesz+descsz, notesSize))
                throw BinException("Note offset+size out of range");
            if (ULEV(nhdr->n_type) == 0x3 && namesz==4 && descsz>=0x1a &&
                ::strcmp((const char*)noteContent+offset+
                            sizeof(typename Types::Nhdr), "AMD")==0)
            {
                // get AMDGPU type and detect GPU device
                const uint32_t* content = (const uint32_t*)
                        (noteContent+offset+sizeof(typename Types::Nhdr) + 4);
                uint32_t major = ULEV(content[1]);
                if (knownGPUType)
                {
                    if ((arch==GPUArchitecture::GCN1_2 && major<8) ||
                        (arch==GPUArchitecture::GCN1_1 && major!=7))
                        throw BinException("Wrong arch major for GPU architecture");
                    // fix for GFX900 - we don't know what is type of device
                    if (arch==GPUArchitecture::GCN1_2 && major!=8)
                        knownGPUType = false;
                }
                else if (major != 9 && major != 8 && major != 7)
                    throw BinException("Unknown arch major");
                
                archMinor = ULEV(content[2]);
                archStepping = ULEV(content[3]);
                if (!knownGPUType)
                {
                    // try again to detect if still GPU type not known
                    arch = (major == 7) ? GPUArchitecture::GCN1_1 :
                            ((major == 8) ? GPUArchitecture::GCN1_2 :
                            GPUArchitecture::GCN1_4);
                    deviceType = getLowestGPUDeviceTypeFromArchitecture(arch);
                    // determine GPU type from arch minor,stepping for GFX9
                    if (major == 9 && archMinor == 0)
                    {
                        if (archStepping <
                            sizeof(gpuGFX9SteppingTable)/sizeof(GPUDeviceType))
                            deviceType = gpuGFX9SteppingTable[archStepping];
                        else // default
                            deviceType = GPUDeviceType::GFX900;
                    }
                    knownGPUType = true;
                }
            }
            size_t align = (((namesz+descsz)&3)!=0) ? 4-((namesz+descsz)&3) : 0;
            offset += sizeof(typename Types::Nhdr) + namesz + descsz + align;
        }
    }
    
    if (!knownGPUType)
        throw BinException("Can't determine GPU device type");
    
    outArchMinor = archMinor;
    outArchStepping = archStepping;
    return deviceType;
}

static const cxuint cl2GPUDeviceTypeMinDriverVersion[] =
{
    UINT_MAX, // CAPE_VERDE = 0, ///< Radeon HD7700
    UINT_MAX, // PITCAIRN, ///< Radeon HD7800
    UINT_MAX, // TAHITI, ///< Radeon HD7900
    UINT_MAX, // OLAND, ///< Radeon R7 250
    180005U, // BONAIRE, ///< Radeon R7 260
    180005U, // SPECTRE, ///< Kaveri
    180005U, // SPOOKY, ///< Kaveri
    180005U, // KALINDI, ///< ???  GCN1.1
    UINT_MAX, // HAINAN, ///< ????  GCN1.0
    180005U, // HAWAII, ///< Radeon R9 290
    180005U, // ICELAND, ///< ???
    180005U, // TONGA, ///< Radeon R9 285
    180005U, // MULLINS, ///< ???
    180005U, // FIJI,  ///< Radeon Fury
    180005U, // CARRIZO, ///< APU
    191205U, // DUMMY,
    200406U, // GOOSE,
    200406U, // HORSE,
    200406U, // STONEY,
    200406U, // ELLESMERE,
    200406U, // BAFFIN,
    223600U, // GFX804,
    223600U, // GFX900,
    226400U, // GFX901,
    252700U, // GFX902,
    252700U, // GFX903,
    252700U, // GFX904,
    252700U, // GFX905,
    258000U, // GFX906,
    258000U  // GFX907,
};

cxuint AmdCL2MainGPUBinaryBase::determineMinDriverVersionForGPUDeviceType(
                GPUDeviceType devType)
{
    if (devType > GPUDeviceType::GPUDEVICE_MAX)
        throw Exception("Wrong device type");
    const cxuint minDriverVersion = cl2GPUDeviceTypeMinDriverVersion[cxuint(devType)];
    if (minDriverVersion == UINT_MAX)
        throw Exception("Device is not supported by this format");
    return minDriverVersion;
}

/* AMD CL2 32-bit */

AmdCL2MainGPUBinary32::AmdCL2MainGPUBinary32(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags) : AmdCL2MainGPUBinaryBase(AmdMainType::GPU_CL2_BINARY),
            ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{
    initMainGPUBinary<AmdCL2Types32>(*this);
}

GPUDeviceType AmdCL2MainGPUBinary32::determineGPUDeviceType(uint32_t& archMinor,
                uint32_t& archStepping, cxuint driverVersion) const
{
    return determineGPUDeviceTypeInt<AmdCL2Types32>(*this,
                    archMinor, archStepping, driverVersion);
}

/* AMD CL2 64-bit */

AmdCL2MainGPUBinary64::AmdCL2MainGPUBinary64(size_t binaryCodeSize, cxbyte* binaryCode,
            Flags creationFlags) : AmdCL2MainGPUBinaryBase(AmdMainType::GPU_CL2_64_BINARY),
            ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{
    initMainGPUBinary<AmdCL2Types64>(*this);
}

GPUDeviceType AmdCL2MainGPUBinary64::determineGPUDeviceType(uint32_t& archMinor,
                uint32_t& archStepping, cxuint driverVersion) const
{
    return determineGPUDeviceTypeInt<AmdCL2Types64>(*this,
                    archMinor, archStepping, driverVersion);
}


AmdCL2MainGPUBinaryBase* CLRX::createAmdCL2BinaryFromCode(
            size_t binaryCodeSize, cxbyte* binaryCode, Flags creationFlags)
{
    if (binaryCode[EI_CLASS] == ELFCLASS32)
        return new AmdCL2MainGPUBinary32(binaryCodeSize, binaryCode, creationFlags);
    else
        return new AmdCL2MainGPUBinary64(binaryCodeSize, binaryCode, creationFlags);
}

bool CLRX::isAmdCL2Binary(size_t binarySize, const cxbyte* binary)
{
    // must be ELF file and must have specific machine field value for 32-bit and 64-bit
    if (!isElfBinary(binarySize, binary))
        return false;
    if (binary[EI_CLASS] == ELFCLASS32)
    {
        const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(binary);
        if (ULEV(ehdr->e_machine) != 0xaf5a || ULEV(ehdr->e_flags)==0)
            return false;
    }
    else
    {
        const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binary);
        if (ULEV(ehdr->e_machine) != 0xaf5b || ULEV(ehdr->e_flags)==0)
            return false;
    }
    return true;
}
