/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <cstdint>
#include <string>
#include <ostream>
#include <cstdio>
#include <memory>
#include <vector>
#include <utility>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdbin/AmdCL2BinGen.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/GPUId.h>
#include "DisasmInternals.h"

using namespace CLRX;

struct CLRX_INTERNAL AmdCL2Types32: Elf32Types
{
    typedef AmdCL2MainGPUBinary32 AmdCL2MainBinary;
    typedef AmdCL2GPUMetadataHeader32 MetadataHeader;
    typedef AmdCL2GPUKernelArgEntry32 KernelArgEntry;
};

struct CLRX_INTERNAL AmdCL2Types64: Elf64Types
{
    typedef AmdCL2MainGPUBinary64 AmdCL2MainBinary;
    typedef AmdCL2GPUMetadataHeader64 MetadataHeader;
    typedef AmdCL2GPUKernelArgEntry64 KernelArgEntry;
};

template<typename AmdCL2Types>
static AmdCL2DisasmInput* getAmdCL2DisasmInputFromBinary(
            const typename AmdCL2Types::AmdCL2MainBinary& binary, cxuint driverVersion)
{
    std::unique_ptr<AmdCL2DisasmInput> input(new AmdCL2DisasmInput);
    input->is64BitMode = (binary.getHeader().e_ident[EI_CLASS] == ELFCLASS64);
    
    input->deviceType = binary.determineGPUDeviceType(input->archMinor,
                  input->archStepping, driverVersion);
    if (driverVersion == 0)
        input->driverVersion = binary.getDriverVersion();
    else
        input->driverVersion = driverVersion;
    
    input->compileOptions = binary.getCompileOptions();
    input->aclVersionString = binary.getAclVersionString();
    bool isInnerNewBinary = binary.hasInnerBinary() &&
                binary.getDriverVersion()>=191205;
    
    input->samplerInitSize = 0;
    input->samplerInit = nullptr;
    input->globalDataSize = 0;
    input->globalData = nullptr;
    input->rwDataSize = 0;
    input->rwData = nullptr;
    input->bssAlignment = input->bssSize = 0;
    std::vector<std::pair<size_t, size_t> > sortedRelocs; // by offset
    const cxbyte* textPtr = nullptr;
    const size_t kernelInfosNum = binary.getKernelInfosNum();
    
    uint16_t gDataSectionIdx = SHN_UNDEF;
    uint16_t rwDataSectionIdx = SHN_UNDEF;
    uint16_t bssDataSectionIdx = SHN_UNDEF;
    
    if (isInnerNewBinary)
    {
        const AmdCL2InnerGPUBinary& innerBin = binary.getInnerBinary();
        input->globalDataSize = innerBin.getGlobalDataSize();
        input->globalData = innerBin.getGlobalData();
        input->rwDataSize = innerBin.getRwDataSize();
        input->rwData = innerBin.getRwData();
        input->samplerInitSize = innerBin.getSamplerInitSize();
        input->samplerInit = innerBin.getSamplerInit();
        input->bssAlignment = innerBin.getBssAlignment();
        input->bssSize = innerBin.getBssSize();
        
        // if no kernels and data
        if (kernelInfosNum==0)
            return input.release();
        
        size_t relaNum = innerBin.getTextRelaEntriesNum();
        for (size_t i = 0; i < relaNum; i++)
        {
            const Elf64_Rela& rel = innerBin.getTextRelaEntry(i);
            sortedRelocs.push_back(std::make_pair(size_t(ULEV(rel.r_offset)), i));
        }
        // sort map
        mapSort(sortedRelocs.begin(), sortedRelocs.end());
        // get code section pointer for determine relocation position kernel code
        textPtr = innerBin.getSectionContent(".hsatext");
        
        try
        { gDataSectionIdx = innerBin.getSectionIndex(".hsadata_readonly_agent"); }
        catch(const Exception& ex)
        { }
        try
        { rwDataSectionIdx = innerBin.getSectionIndex(".hsadata_global_agent"); }
        catch(const Exception& ex)
        { }
        try
        { bssDataSectionIdx = innerBin.getSectionIndex(".hsabss_global_agent"); }
        catch(const Exception& ex)
        { }
        // relocations for global data section (sampler symbols)
        relaNum = innerBin.getGlobalDataRelaEntriesNum();
        // section index for samplerinit (will be used for comparing sampler symbol section
        uint16_t samplerInitSecIndex = SHN_UNDEF;
        try
        { samplerInitSecIndex = innerBin.getSectionIndex(".hsaimage_samplerinit"); }
        catch(const Exception& ex)
        { }
        
        for (size_t i = 0; i < relaNum; i++)
        {
            const Elf64_Rela& rel = innerBin.getGlobalDataRelaEntry(i);
            size_t symIndex = ELF64_R_SYM(ULEV(rel.r_info));
            const Elf64_Sym& sym = innerBin.getSymbol(symIndex);
            // check symbol type, section and value
            if (ELF64_ST_TYPE(sym.st_info) != 12)
                throw Exception("Wrong sampler symbol");
            uint64_t value = ULEV(sym.st_value);
            if (ULEV(sym.st_shndx) != samplerInitSecIndex)
                throw Exception("Wrong section for sampler symbol");
            if ((value&7) != 0)
                throw Exception("Wrong value of sampler symbol");
            input->samplerRelocs.push_back({ size_t(ULEV(rel.r_offset)),
                size_t(value>>3) });
        }
    }
    else if (kernelInfosNum==0)
        return input.release();
    
    input->kernels.resize(kernelInfosNum);
    auto sortedRelocIter = sortedRelocs.begin();
    
    for (cxuint i = 0; i < kernelInfosNum; i++)
    {
        const KernelInfo& kernelInfo = binary.getKernelInfo(i);
        AmdCL2DisasmKernelInput& kinput = input->kernels[i];
        kinput.kernelName = kernelInfo.kernelName;
        kinput.metadataSize = binary.getMetadataSize(i);
        kinput.metadata = binary.getMetadata(i);
        
        kinput.isaMetadataSize = 0;
        kinput.isaMetadata = nullptr;
        // setup isa metadata content
        const AmdCL2GPUKernelMetadata* isaMetadata = nullptr;
        if (i < binary.getISAMetadatasNum())
            isaMetadata = &binary.getISAMetadataEntry(i);
        if (isaMetadata == nullptr || isaMetadata->kernelName != kernelInfo.kernelName)
        {   // fallback if not in order
            try
            { isaMetadata = &binary.getISAMetadataEntry(
                            kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex) // failed
            { isaMetadata = nullptr; }
        }
        if (isaMetadata!=nullptr)
        {
            kinput.isaMetadataSize = isaMetadata->size;
            kinput.isaMetadata = isaMetadata->data;
        }
        
        kinput.code = nullptr;
        kinput.codeSize = 0;
        kinput.setup = nullptr;
        kinput.setupSize = 0;
        kinput.stub = nullptr;
        kinput.stubSize = 0;
        if (!binary.hasInnerBinary())
            continue; // nothing else to set
        
        // get kernel code, setup and stub content
        const AmdCL2InnerGPUBinaryBase& innerBin = binary.getInnerBinaryBase();
        const AmdCL2GPUKernel* kernelData = nullptr;
        if (i < innerBin.getKernelsNum())
            kernelData = &innerBin.getKernelData(i);
        if (kernelData==nullptr || kernelData->kernelName != kernelInfo.kernelName)
            kernelData = &innerBin.getKernelData(kernelInfo.kernelName.c_str());
        
        if (kernelData!=nullptr)
        {
            kinput.code = kernelData->code;
            kinput.codeSize = kernelData->codeSize;
            kinput.setup = kernelData->setup;
            kinput.setupSize = kernelData->setupSize;
        }
        if (!isInnerNewBinary)
        {   // old drivers
            const AmdCL2OldInnerGPUBinary& oldInnerBin = binary.getOldInnerBinary();
            const AmdCL2GPUKernelStub* kstub = nullptr;
            if (i < innerBin.getKernelsNum())
                kstub = &oldInnerBin.getKernelStub(i);
            if (kstub==nullptr || kernelData->kernelName != kernelInfo.kernelName)
                kstub = &oldInnerBin.getKernelStub(kernelInfo.kernelName.c_str());
            if (kstub!=nullptr)
            {
                kinput.stubSize = kstub->size;
                kinput.stub = kstub->data;
            }
        }
        else
        {   // relocations
            const AmdCL2InnerGPUBinary& innerBin = binary.getInnerBinary();
            
            if (sortedRelocIter != sortedRelocs.end() &&
                    sortedRelocIter->first < size_t(kinput.code-textPtr))
                throw Exception("Code relocation offset outside kernel code");
            
            if (sortedRelocIter != sortedRelocs.end())
            {
                size_t end = kinput.code+kinput.codeSize-textPtr;
                for (; sortedRelocIter != sortedRelocs.end() &&
                    sortedRelocIter->first<=end; ++sortedRelocIter)
                {   // add relocations
                    const Elf64_Rela& rela = innerBin.getTextRelaEntry(
                                sortedRelocIter->second);
                    uint32_t symIndex = ELF64_R_SYM(ULEV(rela.r_info));
                    int64_t addend = ULEV(rela.r_addend);
                    cxuint rsym = 0;
                    // check this symbol
                    const Elf64_Sym& sym = innerBin.getSymbol(symIndex);
                    uint16_t symShndx = ULEV(sym.st_shndx);
                    if (symShndx!=gDataSectionIdx && symShndx!=rwDataSectionIdx &&
                        symShndx!=bssDataSectionIdx)
                        throw Exception("Symbol is not placed in global or "
                                "rwdata data or bss is illegal");
                    addend += ULEV(sym.st_value);
                    rsym = (symShndx==rwDataSectionIdx) ? 1 : 
                        ((symShndx==bssDataSectionIdx) ? 2 : 0);
                    
                    RelocType relocType;
                    uint32_t rtype = ELF64_R_TYPE(ULEV(rela.r_info));
                    if (rtype==1)
                        relocType = RELTYPE_LOW_32BIT;
                    else if (rtype==2)
                        relocType = RELTYPE_HIGH_32BIT;
                    else
                        throw Exception("Unknown relocation type");
                    // put text relocs. compute offset by subtracting current code offset
                    kinput.textRelocs.push_back(AmdCL2RelaEntry{sortedRelocIter->first-
                        (kinput.code-textPtr), relocType, rsym, addend });
                }
            }
        }
    }
    if (sortedRelocIter != sortedRelocs.end())
        throw Exception("Code relocation offset outside kernel code");
    return input.release();
}

AmdCL2DisasmInput* CLRX::getAmdCL2DisasmInputFromBinary32(
                const AmdCL2MainGPUBinary32& binary, cxuint driverVersion)
{
    return getAmdCL2DisasmInputFromBinary<AmdCL2Types32>(binary, driverVersion);
}

AmdCL2DisasmInput* CLRX::getAmdCL2DisasmInputFromBinary64(
                const AmdCL2MainGPUBinary64& binary, cxuint driverVersion)
{
    return getAmdCL2DisasmInputFromBinary<AmdCL2Types64>(binary, driverVersion);
}

struct CLRX_INTERNAL IntAmdCL2SetupData
{
    uint32_t pgmRSRC1;
    uint32_t pgmRSRC2;
    uint16_t setup1;
    uint16_t archInd;
    uint32_t scratchBufferSize;
    uint32_t localSize; // in bytes
    uint32_t gdsSize; // in bytes
    uint32_t kernelArgsSize;
    uint32_t zeroes[2];
    uint16_t sgprsNumAll;
    uint16_t vgprsNum16;
    uint32_t vgprsNum;
    uint32_t sgprsNum;
    uint32_t zero3;
    uint32_t version; // ??
};

static const size_t disasmArgTypeNameMapSize = sizeof(disasmArgTypeNameMap)/
            sizeof(std::pair<const char*, KernelArgType>);

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

template<typename Types>
static AmdCL2KernelConfig genKernelConfig(size_t metadataSize, const cxbyte* metadata,
        size_t setupSize, const cxbyte* setup, const std::vector<size_t> samplerOffsets,
        const std::vector<AmdCL2RelaEntry>& textRelocs)
{
    AmdCL2KernelConfig config{};
    const typename Types::MetadataHeader* mdHdr =
            reinterpret_cast<const typename Types::MetadataHeader*>(metadata);
    size_t headerSize = ULEV(mdHdr->size);
    for (size_t i = 0; i < 3; i++)
        config.reqdWorkGroupSize[i] = ULEV(mdHdr->reqdWorkGroupSize[i]);
    const IntAmdCL2SetupData* setupData =
            reinterpret_cast<const IntAmdCL2SetupData*>(setup + 48);
    uint32_t pgmRSRC1 = ULEV(setupData->pgmRSRC1);
    uint32_t pgmRSRC2 = ULEV(setupData->pgmRSRC2);
    /* initializing fields from PGM_RSRC1 and PGM_RSRC2 */
    config.dimMask = (pgmRSRC2>>7)&7;
    config.ieeeMode = (pgmRSRC1>>23)&1;
    config.exceptions = (pgmRSRC2>>24)&0xff;
    config.floatMode = (pgmRSRC1>>12)&0xff;
    config.priority = (pgmRSRC1>>10)&3;
    config.tgSize = (pgmRSRC2>>10)&1;
    config.privilegedMode = (pgmRSRC1>>20)&1;
    config.dx10Clamp = (pgmRSRC1>>21)&1;
    config.debugMode = (pgmRSRC1>>22)&1;
    config.pgmRSRC2 = pgmRSRC2;
    config.pgmRSRC1 = pgmRSRC1;
    config.usedVGPRsNum = ULEV(setupData->vgprsNum);
    config.usedSGPRsNum = ULEV(setupData->sgprsNum);
    config.scratchBufferSize = ULEV(setupData->scratchBufferSize);
    config.localSize = ULEV(setupData->localSize);
    config.gdsSize = ULEV(setupData->gdsSize);
    uint16_t ksetup1 = ULEV(setupData->setup1);
    config.useSetup = (ksetup1&2)!=0;
    config.useArgs = (ksetup1&8)!=0;
    config.useGeneric = config.useEnqueue = false;
    if (ksetup1==0x2f) // if generic pointer support
        config.useGeneric = true;
    else
        config.useEnqueue = (ksetup1&0x20)!=0;
    
    // get samplers
    for (const AmdCL2RelaEntry& reloc: textRelocs)
    {   // check if sampler
        auto it = std::find(samplerOffsets.begin(), samplerOffsets.end(), reloc.addend);
        if (it!=samplerOffsets.end())
            config.samplers.push_back(it-samplerOffsets.begin());
    }
    std::sort(config.samplers.begin(), config.samplers.end());
    config.samplers.resize(std::unique(config.samplers.begin(), config.samplers.end()) -
                config.samplers.begin());
    // get kernel args
    size_t argOffset = headerSize + ULEV(mdHdr->firstNameLength) + 
            ULEV(mdHdr->secondNameLength)+2;
    if (ULEV(*(const uint32_t*)(metadata+argOffset)) ==
                (sizeof(typename Types::KernelArgEntry)<<8))
        argOffset++;    // fix for AMD GPUPRO driver (2036.03) */
    const typename Types::KernelArgEntry* argPtr = reinterpret_cast<
            const typename Types::KernelArgEntry*>(metadata + argOffset);
    const uint32_t argsNum = ULEV(mdHdr->argsNum);
    const char* strBase = (const char*)metadata;
    size_t strOffset = argOffset + sizeof(typename Types::KernelArgEntry)*(argsNum+1);
    
    for (uint32_t i = 0; i < argsNum; i++, argPtr++)
    {
        AmdKernelArgInput arg{};
        size_t nameSize = ULEV(argPtr->argNameSize);
        arg.argName.assign(strBase+strOffset, nameSize);
        strOffset += nameSize+1;
        nameSize = ULEV(argPtr->typeNameSize);
        arg.typeName.assign(strBase+strOffset, nameSize);
        strOffset += nameSize+1;
        
        uint32_t vectorSize = ULEV(argPtr->vectorLength);
        uint32_t argType = ULEV(argPtr->argType);
        uint32_t kindOfType = ULEV(argPtr->kindOfType);
        
        arg.ptrSpace = KernelPtrSpace::NONE;
        arg.ptrAccess = KARG_PTR_NORMAL;
        arg.argType = KernelArgType::VOID;
        
        if (ULEV(argPtr->isConst))
            arg.ptrAccess |= KARG_PTR_CONST;
        arg.used = AMDCL2_ARGUSED_READ_WRITE; // default is used
        
        if (!ULEV(argPtr->isPointerOrPipe))
        { // if not point or pipe (get regular type: scalar, image, sampler,...)
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
                    if (kindOfType==2) // not image
                    {
                        arg.argType = KernelArgType::IMAGE;
                        arg.ptrAccess = (argType==1) ? KARG_PTR_READ_ONLY : (argType==2) ?
                                 KARG_PTR_WRITE_ONLY : KARG_PTR_READ_WRITE;
                        arg.ptrSpace = KernelPtrSpace::GLOBAL;
                    }
                    else if (argType==2 || argType == 3)
                    {
                        if (kindOfType!=4) // not scalar
                            throw Exception("Wrong kernel argument type");
                        arg.argType = (argType==3) ?
                            KernelArgType::SHORT : KernelArgType::CHAR;
                    }
                    else
                        throw Exception("Wrong kernel argument type");
                    break;
                case 4: // int
                case 5: // long
                    if (kindOfType!=4) // not scalar
                        throw Exception("Wrong kernel argument type");
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
                    break;
            }
            
            auto it = binaryMapFind(disasmArgTypeNameMap,
                        disasmArgTypeNameMap + disasmArgTypeNameMapSize,
                        arg.typeName.c_str(), CStringLess());
            if (it != disasmArgTypeNameMap + disasmArgTypeNameMapSize) // if found
                arg.argType = it->second;
            
            if (arg.argType == KernelArgType::STRUCTURE)
                arg.structSize = ULEV(argPtr->structSize);
            else if (isKernelArgImage(arg.argType) || arg.argType==KernelArgType::SAMPLER)
                arg.resId = LEV(argPtr->resId); // sampler and images have resource id
        }
        else
        {   // pointer or pipe
            if (argPtr->isPipe)
                arg.used = (ULEV(argPtr->isPointerOrPipe))==3;
            else
                arg.used = (ULEV(argPtr->isPointerOrPipe));
            uint32_t ptrType = ULEV(argPtr->ptrType);
            uint32_t ptrSpace = ULEV(argPtr->ptrSpace);
            if (argType == 7) // pointer or pipe
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
                arg.ptrAccess |= KARG_PTR_NORMAL;
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
            
            if (arg.argType == KernelArgType::POINTER)
            {
                size_t ptrTypeNameSize=0;
                if (arg.typeName.empty()) // ctx_struct_fld1
                {
                    if (ptrType >= 6 && ptrType <= 12)
                        arg.pointerType = cl20ArgTypeVectorTable[(ptrType-6)*6];
                    else
                        arg.pointerType = KernelArgType::FLOAT;
                }
                else
                {
                    arg.pointerType = KernelArgType::VOID;
                    switch (ptrType)
                    {
                        case 6: // char
                        case 7: // short
                        case 8: // int
                        case 9: // long
                        case 11: // float
                        case 12: // double
                            arg.pointerType = cl20ArgTypeVectorTable[(ptrType-6)*6];
                            break;
                    }
                    
                    while (isAlnum(arg.typeName[ptrTypeNameSize]) ||
                        arg.typeName[ptrTypeNameSize]=='_') ptrTypeNameSize++;
                    CString ptrTypeName(arg.typeName.c_str(), ptrTypeNameSize);
                    if (arg.typeName.find('*')!=CString::npos) // assume 'void*'
                    {
                        auto it = binaryMapFind(disasmArgTypeNameMap,
                                    disasmArgTypeNameMap + disasmArgTypeNameMapSize,
                                    ptrTypeName.c_str(), CStringLess());
                        if (it != disasmArgTypeNameMap + disasmArgTypeNameMapSize)
                            // if found
                            arg.pointerType = it->second;
                        else if (arg.pointerType==KernelArgType::VOID)
                            arg.pointerType = KernelArgType::STRUCTURE;
                    }
                    else if (ptrType==18)
                    {   /* if clkevent */
                        arg.argType = KernelArgType::CLKEVENT;
                        arg.pointerType = KernelArgType::VOID;
                    }
                    else /* unknown type of pointer */
                        arg.pointerType = KernelArgType::VOID;
                        
                    if (arg.pointerType == KernelArgType::STRUCTURE)
                        arg.structSize = ULEV(argPtr->ptrAlignment);
                }
            }
        }
        config.args.push_back(arg);
    }
    return config;
}

static void dumpAmdCL2KernelConfig(std::ostream& output, const AmdCL2KernelConfig& config)
{
    size_t bufSize;
    char buf[100];
    output.write("    .config\n", 12);
    if (config.dimMask != BINGEN_DEFAULT)
    {
        strcpy(buf, "        .dims ");
        bufSize = 14;
        if ((config.dimMask & 1) != 0)
            buf[bufSize++] = 'x';
        if ((config.dimMask & 2) != 0)
            buf[bufSize++] = 'y';
        if ((config.dimMask & 4) != 0)
            buf[bufSize++] = 'z';
        buf[bufSize++] = '\n';
        output.write(buf, bufSize);
    }
    bufSize = 0;
    if (config.reqdWorkGroupSize[2] != 0)
        bufSize = snprintf(buf, 100, "        .cws %u, %u, %u\n",
               config.reqdWorkGroupSize[0], config.reqdWorkGroupSize[1],
               config.reqdWorkGroupSize[2]);
    else if (config.reqdWorkGroupSize[1] != 0)
        bufSize = snprintf(buf, 100, "        .cws %u, %u\n", config.reqdWorkGroupSize[0],
                   config.reqdWorkGroupSize[1]);
    else if (config.reqdWorkGroupSize[0] != 0)
        bufSize = snprintf(buf, 100, "        .cws %u\n", config.reqdWorkGroupSize[0]);
    if (bufSize != 0) // if we have cws
        output.write(buf, bufSize);
    
    bufSize = snprintf(buf, 100, "        .sgprsnum %u\n", config.usedSGPRsNum);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .vgprsnum %u\n", config.usedVGPRsNum);
    output.write(buf, bufSize);
    
    if (config.localSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .localsize %llu\n",
                       cxullong(config.localSize));
        output.write(buf, bufSize);
    }
    if (config.gdsSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .gdssize %llu\n",
                       cxullong(config.gdsSize));
        output.write(buf, bufSize);
    }
    bufSize = snprintf(buf, 100, "        .floatmode 0x%02x\n", config.floatMode);
    output.write(buf, bufSize);
    if (config.scratchBufferSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .scratchbuffer %u\n",
                           config.scratchBufferSize);
        output.write(buf, bufSize);
    }
    bufSize = snprintf(buf, 100, "        .pgmrsrc1 0x%08x\n", config.pgmRSRC1);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .pgmrsrc2 0x%08x\n", config.pgmRSRC2);
    output.write(buf, bufSize);
    if (config.privilegedMode)
        output.write("        .privmode\n", 18);
    if (config.debugMode)
        output.write("        .debugmode\n", 19);
    if (config.dx10Clamp)
        output.write("        .dx10clamp\n", 19);
    if (config.ieeeMode)
        output.write("        .ieeemode\n", 18);
    if (config.tgSize)
        output.write("        .tgsize\n", 16);
    if ((config.exceptions & 0x7f) != 0)
    {
        bufSize = snprintf(buf, 100, "        .exceptions 0x%02x\n",
                   cxuint(config.exceptions));
        output.write(buf, bufSize);
    }
    if (config.useArgs)
        output.write("        .useargs\n", 17);
    if (config.useSetup)
        output.write("        .usesetup\n", 18);
    if (config.useEnqueue)
        output.write("        .useenqueue\n", 20);
    if (config.useGeneric)
        output.write("        .usegeneric\n", 20);
    bufSize = snprintf(buf, 100, "        .priority %u\n", config.priority);
    output.write(buf, bufSize);
    // arguments
    for (const AmdKernelArgInput& arg: config.args)
        dumpAmdKernelArg(output, arg, true);
    // samplers
    for (cxuint sampler: config.samplers)
    {
        bufSize = snprintf(buf, 100, "        .sampler %u\n", sampler);
        output.write(buf, bufSize);
    }
}

void CLRX::disassembleAmdCL2(std::ostream& output, const AmdCL2DisasmInput* amdCL2Input,
       ISADisassembler* isaDisassembler, size_t& sectionCount, Flags flags)
{
    const bool doMetadata = ((flags & DISASM_METADATA) != 0);
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    const bool doDumpConfig = ((flags & DISASM_CONFIG) != 0);
    const bool doSetup = ((flags & DISASM_SETUP) != 0);
    
    if (amdCL2Input->is64BitMode)
        output.write(".64bit\n", 7);
    else
        output.write(".32bit\n", 7);
    
    {
        char buf[40];
        size_t size = snprintf(buf, 40, ".arch_minor %u\n", amdCL2Input->archMinor);
        output.write(buf, size);
        size = snprintf(buf, 40, ".arch_stepping %u\n", amdCL2Input->archStepping);
        output.write(buf, size);
        size = snprintf(buf, 40, ".driver_version %u\n",
                   amdCL2Input->driverVersion);
        output.write(buf, size);
    }
    
    if (doMetadata)
    {
        output.write(".compile_options \"", 18);
        const std::string escapedCompileOptions = 
                escapeStringCStyle(amdCL2Input->compileOptions);
        output.write(escapedCompileOptions.c_str(), escapedCompileOptions.size());
        output.write("\"\n.acl_version \"", 16);
        const std::string escapedAclVersionString =
                escapeStringCStyle(amdCL2Input->aclVersionString);
        output.write(escapedAclVersionString.c_str(), escapedAclVersionString.size());
        output.write("\"\n", 2);
    }
    if (doSetup && !doDumpCode)
    {
        if (amdCL2Input->samplerInit!=nullptr && amdCL2Input->samplerInitSize!=0)
        {   /// sampler init entries
            output.write(".samplerinit\n", 13);
            printDisasmData(amdCL2Input->samplerInitSize,
                            amdCL2Input->samplerInit, output);
        }
    }
    else if (doDumpConfig && amdCL2Input->samplerInit!=nullptr)
    {   // print sampler values instead raw samler data in .samplerinit
        char buf[50];
        const size_t samplersNum = amdCL2Input->samplerInitSize>>3;
        for (size_t i = 0; i < samplersNum; i++)
        {
            size_t bufSize = snprintf(buf, 50, ".sampler 0x%08x\n",
                      ULEV(((const uint32_t*)amdCL2Input->samplerInit)[i*2+1]));
            output.write(buf, bufSize);
        }
    }
    
    if (doDumpData && amdCL2Input->globalData != nullptr &&
        amdCL2Input->globalDataSize != 0)
    {
        output.write(".globaldata\n", 12);
        output.write(".gdata:\n", 8); /// symbol used by text relocations
        printDisasmData(amdCL2Input->globalDataSize, amdCL2Input->globalData, output);
        /// put sampler relocations at global data section
        for (auto v: amdCL2Input->samplerRelocs)
        {
            output.write("    .samplerreloc ", 18);
            char buf[64];
            size_t bufPos = itocstrCStyle<size_t>(v.first, buf, 22);
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += itocstrCStyle<size_t>(v.second, buf+bufPos, 22);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
        }
    }
    if (doDumpData && amdCL2Input->rwData != nullptr &&
        amdCL2Input->rwDataSize != 0)
    {
        output.write(".data\n", 6);
        output.write(".ddata:\n", 8); /// symbol used by text relocations
        printDisasmData(amdCL2Input->rwDataSize, amdCL2Input->rwData, output);
    }
    
    if (doDumpData && amdCL2Input->bssSize)
    {
        output.write(".section .bss align=", 20);
        char buf[64];
        size_t bufPos = itocstrCStyle<size_t>(amdCL2Input->bssAlignment, buf, 22);
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
        output.write(".bdata:\n", 8); /// symbol used by text relocations
        output.write("    .skip ", 10);
        bufPos = itocstrCStyle<size_t>(amdCL2Input->bssSize, buf, 22);
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
    }
    
    std::vector<size_t> samplerOffsets;
    if (doDumpConfig)
    {
        for (auto reloc: amdCL2Input->samplerRelocs)
        {
            if (samplerOffsets.size() >= reloc.second)
                samplerOffsets.resize(reloc.second+1);
            samplerOffsets[reloc.second] = reloc.first;
        }
    }
    
    for (const AmdCL2DisasmKernelInput& kinput: amdCL2Input->kernels)
    {
        output.write(".kernel ", 8);
        output.write(kinput.kernelName.c_str(), kinput.kernelName.size());
        output.put('\n');
        if (doMetadata && !doDumpConfig)
        {
            if (kinput.metadata != nullptr && kinput.metadataSize != 0)
            {   // if kernel metadata available
                output.write("    .metadata\n", 14);
                printDisasmData(kinput.metadataSize, kinput.metadata, output, true);
            }
            if (kinput.isaMetadata != nullptr && kinput.isaMetadataSize != 0)
            {   // if kernel isametadata available
                output.write("    .isametadata\n", 17);
                printDisasmData(kinput.isaMetadataSize, kinput.isaMetadata, output, true);
            }
        }
        if (doSetup && !doDumpConfig)
        {
            if (kinput.stub != nullptr && kinput.stubSize != 0)
            {   // if kernel setup available
                output.write("    .stub\n", 10);
                printDisasmData(kinput.stubSize, kinput.stub, output, true);
            }
            if (kinput.setup != nullptr && kinput.setupSize != 0)
            {   // if kernel setup available
                output.write("    .setup\n", 11);
                printDisasmData(kinput.setupSize, kinput.setup, output, true);
            }
        }
        
        if (doDumpConfig)
        {
            AmdCL2KernelConfig config;
            if (amdCL2Input->is64BitMode)
                config = genKernelConfig<AmdCL2Types64>(kinput.metadataSize,
                        kinput.metadata, kinput.setupSize, kinput.setup, samplerOffsets,
                        kinput.textRelocs);
            else
                config = genKernelConfig<AmdCL2Types32>(kinput.metadataSize,
                        kinput.metadata, kinput.setupSize, kinput.setup, samplerOffsets,
                        kinput.textRelocs);
            dumpAmdCL2KernelConfig(output, config);
        }
        
        if (doDumpCode && kinput.code != nullptr && kinput.codeSize != 0)
        {   // input kernel code (main disassembly)
            isaDisassembler->clearRelocations();
            isaDisassembler->addRelSymbol(".gdata");
            isaDisassembler->addRelSymbol(".ddata"); // rw data
            isaDisassembler->addRelSymbol(".bdata"); // .bss data
            for (const AmdCL2RelaEntry& entry: kinput.textRelocs)
                isaDisassembler->addRelocation(entry.offset, entry.type, 
                               cxuint(entry.symbol), entry.addend);
    
            output.write("    .text\n", 10);
            isaDisassembler->setInput(kinput.codeSize, kinput.code);
            isaDisassembler->beforeDisassemble();
            isaDisassembler->disassemble();
            sectionCount++;
        }
    }
}
