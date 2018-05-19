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
#include <cstdint>
#include <cstdio>
#include <string>
#include <ostream>
#include <memory>
#include <inttypes.h>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/AmdCL2BinGen.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/GPUId.h>
#include "DisasmInternals.h"

using namespace CLRX;

// get AMD kernel input from inner binary
static void getAmdDisasmKernelInputFromBinary(const AmdInnerGPUBinary32* innerBin,
        AmdDisasmKernelInput& kernelInput, Flags flags, GPUDeviceType inputDeviceType)
{
    kernelInput.codeSize = kernelInput.dataSize = 0;
    kernelInput.code = kernelInput.data = nullptr;
    
    if (innerBin != nullptr)
    {
        // if innerBinary exists
        bool codeFound = false;
        bool dataFound = false;
        cxuint encEntryIndex = innerBin->findCALEncodingEntryIndex(inputDeviceType);
        const CALEncodingEntry& encEntry = innerBin->getCALEncodingEntry(encEntryIndex);
        const size_t encEntryOffset = ULEV(encEntry.offset);
        const size_t encEntrySize = ULEV(encEntry.size);
        /* find suitable sections */
        for (cxuint j = 0; j < innerBin->getSectionHeadersNum(); j++)
        {
            const char* secName = innerBin->getSectionName(j);
            const Elf32_Shdr& shdr = innerBin->getSectionHeader(j);
            const size_t secOffset = ULEV(shdr.sh_offset);
            const size_t secSize = ULEV(shdr.sh_size);
            if (secOffset < encEntryOffset ||
                    usumGt(secOffset, secSize, encEntryOffset+encEntrySize))
                continue; // skip this section (not in choosen encoding)
            
            // set code and data sections (if exists)
            if (!codeFound && ::strcmp(secName, ".text") == 0)
            {
                kernelInput.codeSize = secSize;
                kernelInput.code = innerBin->getSectionContent(j);
                codeFound = true;
            }
            else if (!dataFound && ::strcmp(secName, ".data") == 0)
            {
                kernelInput.dataSize = secSize;
                kernelInput.data = innerBin->getSectionContent(j);
                dataFound = true;
            }
            
            if (codeFound && dataFound)
                break; // end of finding
        }
        
        if ((flags & DISASM_CALNOTES) != 0)
        {
            // get ATI CAL notes
            kernelInput.calNotes.resize(innerBin->getCALNotesNum(encEntryIndex));
            cxuint j = 0;
            for (const CALNote& calNote: innerBin->getCALNotes(encEntryIndex))
            {
                CALNoteInput& outCalNote = kernelInput.calNotes[j++];
                outCalNote.header.nameSize = ULEV(calNote.header->nameSize);
                outCalNote.header.type = ULEV(calNote.header->type);
                outCalNote.header.descSize = ULEV(calNote.header->descSize);
                std::copy(calNote.header->name, calNote.header->name+8,
                          outCalNote.header.name);
                outCalNote.data = calNote.data;
            }
        }
    }
}

// template for handling 32-bit and 64-bit bnaries
template<typename AmdMainBinary>
static AmdDisasmInput* getAmdDisasmInputFromBinary(const AmdMainBinary& binary,
           Flags flags)
{
    std::unique_ptr<AmdDisasmInput> input(new AmdDisasmInput);
    input->is64BitMode = (binary.getHeader().e_ident[EI_CLASS] == ELFCLASS64);
    
    // get global setup (device, compile options, driver info, ...)
    input->deviceType = binary.determineGPUDeviceType();
    input->compileOptions = binary.getCompileOptions();
    input->driverInfo = binary.getDriverInfo();
    input->globalDataSize = binary.getGlobalDataSize();
    input->globalData = binary.getGlobalData();
    const size_t kernelInfosNum = binary.getKernelInfosNum();
    const size_t kernelHeadersNum = binary.getKernelHeadersNum();
    const size_t innerBinariesNum = binary.getInnerBinariesNum();
    input->kernels.resize(kernelInfosNum);
    
    const Flags innerFlags = flags |
        (((flags&DISASM_CONFIG)!=0) ? DISASM_METADATA|DISASM_CALNOTES : 0);
    for (cxuint i = 0; i < kernelInfosNum; i++)
    {
        const KernelInfo& kernelInfo = binary.getKernelInfo(i);
        const AmdInnerGPUBinary32* innerBin = nullptr;
        if (i < innerBinariesNum)
            innerBin = &binary.getInnerBinary(i);
        if (innerBin == nullptr || innerBin->getKernelName() != kernelInfo.kernelName)
        {
            // fallback if not in order
            try
            { innerBin = &binary.getInnerBinary(kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex)
            { innerBin = nullptr; }
        }
        AmdDisasmKernelInput& kernelInput = input->kernels[i];
        // kernel metadata
        kernelInput.metadataSize = binary.getMetadataSize(i);
        kernelInput.metadata = binary.getMetadata(i);
        
        // kernel header
        kernelInput.headerSize = 0;
        kernelInput.header = nullptr;
        const AmdGPUKernelHeader* khdr = nullptr;
        if (i < kernelHeadersNum)
            khdr = &binary.getKernelHeaderEntry(i);
        if (khdr == nullptr || khdr->kernelName != kernelInfo.kernelName)
        {
            // fallback if not in order
            try
            { khdr = &binary.getKernelHeaderEntry(kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex) // failed
            { khdr = nullptr; }
        }
        if (khdr != nullptr)
        {
            kernelInput.headerSize = khdr->size;
            kernelInput.header = khdr->data;
        }
        
        // get kernek input from inner binary
        kernelInput.kernelName = kernelInfo.kernelName;
        getAmdDisasmKernelInputFromBinary(innerBin, kernelInput, innerFlags,
                  input->deviceType);
    }
    return input.release();
}

AmdDisasmInput* CLRX::getAmdDisasmInputFromBinary32(const AmdMainGPUBinary32& binary,
                  Flags flags)
{
    return getAmdDisasmInputFromBinary(binary, flags);
}

AmdDisasmInput* CLRX::getAmdDisasmInputFromBinary64(const AmdMainGPUBinary64& binary,
                  Flags flags)
{
    return getAmdDisasmInputFromBinary(binary, flags);
}

/* get AsmConfig */

// AMD kernel argument types map (sorted by type names)
const std::pair<const char*, KernelArgType> CLRX::disasmArgTypeNameMap[74] =
{
    { "char", KernelArgType::CHAR },
    { "char16", KernelArgType::CHAR16 },
    { "char2", KernelArgType::CHAR2 },
    { "char3", KernelArgType::CHAR3 },
    { "char4", KernelArgType::CHAR4 },
    { "char8", KernelArgType::CHAR8 },
    { "clk_event_t", KernelArgType::CLKEVENT },
    { "counter32", KernelArgType::COUNTER32 },
    { "double", KernelArgType::DOUBLE },
    { "double16", KernelArgType::DOUBLE16 },
    { "double2", KernelArgType::DOUBLE2 },
    { "double3", KernelArgType::DOUBLE3 },
    { "double4", KernelArgType::DOUBLE4 },
    { "double8", KernelArgType::DOUBLE8 },
    { "float", KernelArgType::FLOAT },
    { "float16", KernelArgType::FLOAT16 },
    { "float2", KernelArgType::FLOAT2 },
    { "float3", KernelArgType::FLOAT3 },
    { "float4", KernelArgType::FLOAT4 },
    { "float8", KernelArgType::FLOAT8 },
    { "image", KernelArgType::IMAGE },
    { "image1d", KernelArgType::IMAGE1D },
    { "image1d_array", KernelArgType::IMAGE1D_ARRAY },
    { "image1d_buffer", KernelArgType::IMAGE1D_BUFFER },
    { "image2d", KernelArgType::IMAGE2D },
    { "image2d_array", KernelArgType::IMAGE2D_ARRAY },
    { "image3d", KernelArgType::IMAGE3D },
    { "int", KernelArgType::INT },
    { "int16", KernelArgType::INT16 },
    { "int2", KernelArgType::INT2 },
    { "int3", KernelArgType::INT3 },
    { "int4", KernelArgType::INT4 },
    { "int8", KernelArgType::INT8 },
    { "long", KernelArgType::LONG },
    { "long16", KernelArgType::LONG16 },
    { "long2", KernelArgType::LONG2 },
    { "long3", KernelArgType::LONG3 },
    { "long4", KernelArgType::LONG4 },
    { "long8", KernelArgType::LONG8 },
    { "pipe", KernelArgType::PIPE },
    { "queue_t", KernelArgType::CMDQUEUE },
    { "sampler_t", KernelArgType::SAMPLER },
    { "short", KernelArgType::SHORT },
    { "short16", KernelArgType::SHORT16 },
    { "short2", KernelArgType::SHORT2 },
    { "short3", KernelArgType::SHORT3 },
    { "short4", KernelArgType::SHORT4 },
    { "short8", KernelArgType::SHORT8 },
    { "structure", KernelArgType::STRUCTURE },
    { "uchar", KernelArgType::UCHAR },
    { "uchar16", KernelArgType::UCHAR16 },
    { "uchar2", KernelArgType::UCHAR2 },
    { "uchar3", KernelArgType::UCHAR3 },
    { "uchar4", KernelArgType::UCHAR4 },
    { "uchar8", KernelArgType::UCHAR8 },
    { "uint", KernelArgType::UINT },
    { "uint16", KernelArgType::UINT16 },
    { "uint2", KernelArgType::UINT2 },
    { "uint3", KernelArgType::UINT3 },
    { "uint4", KernelArgType::UINT4 },
    { "uint8", KernelArgType::UINT8 },
    { "ulong", KernelArgType::ULONG},
    { "ulong16", KernelArgType::ULONG16 },
    { "ulong2", KernelArgType::ULONG2 },
    { "ulong3", KernelArgType::ULONG3 },
    { "ulong4", KernelArgType::ULONG4 },
    { "ulong8", KernelArgType::ULONG8 },
    { "ushort", KernelArgType::USHORT },
    { "ushort16", KernelArgType::USHORT16 },
    { "ushort2", KernelArgType::USHORT2 },
    { "ushort3", KernelArgType::USHORT3 },
    { "ushort4", KernelArgType::USHORT4 },
    { "ushort8", KernelArgType::USHORT8 },
    { "void", KernelArgType::VOID }
};

static const size_t disasmArgTypeNameMapLength =
        sizeof(disasmArgTypeNameMap)/sizeof(std::pair<const char*, KernelArgType>);

// GPU argument type table
const KernelArgType gpuArgTypeTable[] =
{
    KernelArgType::UCHAR,
    KernelArgType::UCHAR2,
    KernelArgType::UCHAR3,
    KernelArgType::UCHAR4,
    KernelArgType::UCHAR8,
    KernelArgType::UCHAR16,
    KernelArgType::CHAR,
    KernelArgType::CHAR2,
    KernelArgType::CHAR3,
    KernelArgType::CHAR4,
    KernelArgType::CHAR8,
    KernelArgType::CHAR16,
    KernelArgType::USHORT,
    KernelArgType::USHORT2,
    KernelArgType::USHORT3,
    KernelArgType::USHORT4,
    KernelArgType::USHORT8,
    KernelArgType::USHORT16,
    KernelArgType::SHORT,
    KernelArgType::SHORT2,
    KernelArgType::SHORT3,
    KernelArgType::SHORT4,
    KernelArgType::SHORT8,
    KernelArgType::SHORT16,
    KernelArgType::UINT,
    KernelArgType::UINT2,
    KernelArgType::UINT3,
    KernelArgType::UINT4,
    KernelArgType::UINT8,
    KernelArgType::UINT16,
    KernelArgType::INT,
    KernelArgType::INT2,
    KernelArgType::INT3,
    KernelArgType::INT4,
    KernelArgType::INT8,
    KernelArgType::INT16,
    KernelArgType::ULONG,
    KernelArgType::ULONG2,
    KernelArgType::ULONG3,
    KernelArgType::ULONG4,
    KernelArgType::ULONG8,
    KernelArgType::ULONG16,
    KernelArgType::LONG,
    KernelArgType::LONG2,
    KernelArgType::LONG3,
    KernelArgType::LONG4,
    KernelArgType::LONG8,
    KernelArgType::LONG16,
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

// determine kernel argument type from type name from metadata line
static KernelArgType determineKernelArgType(const char* typeString, cxuint vectorSize,
                    LineNo lineNo)
{
    KernelArgType outType;
    
    if (vectorSize > 16)
        throw ParseException(lineNo, "Wrong vector size");
    const cxuint vectorId = vectorIdTable[vectorSize];
    if (vectorId == UINT_MAX)
        throw ParseException(lineNo, "Wrong vector size");
    
    if (::strncmp(typeString, "float:", 6) == 0)
        outType = gpuArgTypeTable[8*6+vectorId];
    else if (::strncmp(typeString, "double:", 7) == 0)
        outType = gpuArgTypeTable[9*6+vectorId];
    else if ((typeString[0] == 'i' || typeString[0] == 'u'))
    {
        /* indexBase - choose between unsigned/signed */
        const cxuint indexBase = (typeString[0] == 'i')?6:0;
        if (typeString[1] == '8')
        {
            if (typeString[2] != ':')
                throw ParseException(lineNo, "Can't parse type");
            outType = gpuArgTypeTable[indexBase+vectorId];
        }
        else
        {
            if (typeString[1] == '1' && typeString[2] == '6')
                outType = gpuArgTypeTable[indexBase+2*6+vectorId];
            else if (typeString[1] == '3' && typeString[2] == '2')
                outType = gpuArgTypeTable[indexBase+4*6+vectorId];
            else if (typeString[1] == '6' && typeString[2] == '4')
                outType = gpuArgTypeTable[indexBase+6*6+vectorId];
            else // if not determined
                throw ParseException(lineNo, "Can't parse type");
            if (typeString[3] != ':')
                throw ParseException(lineNo, "Can't parse type");
        }
    }
    else
        throw ParseException(lineNo, "Can't parse type");
    
    return outType;
}

// find char to end
inline static const char* strechr(const char* str, const char* end, int c)
{
    const char* p = str;
    while (p!=end && *p != c && *p != 0) p++;
    return (p!=end && *p==c) ? p : nullptr;
}

inline static int strnecmp(const char* str1, const char* str2, size_t n, const char* end)
{
    if (str1+n > end) return -1;
    return strncmp(str1, str2, n);
}


/* get configuration to human readable form */
static AmdKernelConfig getAmdKernelConfig(size_t metadataSize, const char* metadata,
            const std::vector<CALNoteInput>& calNotes, const CString& driverInfo,
            const cxbyte* kernelHeader, GPUArchitecture arch)
{
    cxuint driverVersion = 9999909U;
    {
        // parse version
        size_t pos = driverInfo.find("AMD-APP"); // find AMDAPP
        try
        {
            if (pos != std::string::npos)
            {
                /* let to parse version number */
                pos += 9;
                const char* end;
                driverVersion = cstrtovCStyle<cxuint>(
                        driverInfo.c_str()+pos, nullptr, end)*100;
                end++;
                driverVersion += cstrtovCStyle<cxuint>(end, nullptr, end);
            }
        }
        catch(const ParseException& ex)
        { driverVersion = 99999909U; /* newest possible */ }
    }
    
    AmdKernelConfig config{};
    std::vector<cxuint> argUavIds;
    std::unordered_map<cxuint,cxuint> argCbIds;
    std::unordered_map<CString, cxuint> argIndexMap;
    std::vector<cxuint> samplerArgIndices;
    std::vector<cxuint> constArgIndices;
    // set as defaults
    config.dimMask = BINGEN_DEFAULT;
    config.printfId = BINGEN_DEFAULT;
    config.constBufferId = BINGEN_DEFAULT;
    config.uavPrivate = BINGEN_DEFAULT;
    config.uavId = BINGEN_DEFAULT;
    config.privateId = BINGEN_DEFAULT;
    config.hwRegion = BINGEN_DEFAULT;
    config.exceptions = 0;
    config.usePrintf = ((ULEV(reinterpret_cast<const uint32_t*>(
                kernelHeader)[4]) & 2) != 0);
    config.tgSize = config.useConstantData = false;
    config.reqdWorkGroupSize[0] = config.reqdWorkGroupSize[1] =
            config.reqdWorkGroupSize[2] = 0;
    
    std::vector<cxuint> woImageIds;
    cxuint argSamplers = 0;
    
    cxuint uavIdToCompare = 0;
    /* parse arguments from metadata string */
    /* this is very similar code as in AmdBinaries, but
     * includes type and all needed argument attributes */
    const char* linePtr = metadata;
    const char* mtEnd = metadata + metadataSize;
    LineNo lineNo = 1;
    for (; linePtr != mtEnd; lineNo++)
    {
        const char* lineEnd = linePtr;
        while (lineEnd!=mtEnd && *lineEnd!='\n') lineEnd++;
        const char* outEnd;
        // local size
        if (::strnecmp(linePtr, ";memory:hwlocal:", 16, lineEnd)==0)
            config.hwLocalSize = cstrtovCStyle<size_t>(linePtr+16, lineEnd, outEnd);
        // hw region
        else if (::strnecmp(linePtr, ";memory:hwregion:", 17, lineEnd)==0)
            config.hwRegion = cstrtovCStyle<uint32_t>(linePtr+17, lineEnd, outEnd);
        else if (::strnecmp(linePtr, ";cws:", 5, lineEnd)==0)
        { 
            // cws (reqd_work_group_size)
            config.reqdWorkGroupSize[0] = cstrtovCStyle<uint32_t>(
                        linePtr+5, lineEnd, outEnd);
            if (outEnd==lineEnd || *outEnd!=':')
                throw ParseException(lineNo, "Can't parse CWS");
            outEnd++;
            config.reqdWorkGroupSize[1] = cstrtovCStyle<uint32_t>(
                        outEnd, lineEnd, outEnd);
            if (outEnd==lineEnd || *outEnd!=':')
                throw ParseException(lineNo, "Can't parse CWS");
            outEnd++;
            config.reqdWorkGroupSize[2] = cstrtovCStyle<uint32_t>(
                        outEnd, lineEnd, outEnd);
        }
        else if (::strnecmp(linePtr, ";value:", 7, lineEnd)==0)
        {
            /* scalar value or structure */
            AmdKernelArgInput arg;
            const char* ptr = strechr(linePtr+7, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse value argument");
            arg.argName.assign(linePtr+7, ptr);
            arg.argType = KernelArgType::VOID;
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            arg.resId = BINGEN_DEFAULT;
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.resId = 0;
            arg.used = true;
            ptr++;
            const char* nextPtr = strechr(ptr, lineEnd, ':');
            if (nextPtr==nullptr)
                throw ParseException(lineNo, "Can't parse value argument");
            const char* typeStr = ptr;
            ptr = nextPtr+1;
            if (::strncmp(typeStr, "struct:", 7)==0)
            {
                // argument is structure
                arg.argType = KernelArgType::STRUCTURE;
                arg.structSize = cstrtovCStyle<uint32_t>(ptr, lineEnd, outEnd);
            }
            else
            {
                // regular type
                nextPtr = strechr(ptr, lineEnd, ':');
                if (nextPtr==nullptr)
                    throw ParseException(lineNo, "Can't parse value argument");
                nextPtr++;
                cxuint vectorSize = cstrtoui(nextPtr, lineEnd, outEnd);
                arg.argType = determineKernelArgType(typeStr, vectorSize, lineNo);
            }
            argUavIds.push_back(0);
            argIndexMap[arg.argName] = config.args.size();
            config.args.push_back(arg);
        }
        else if (::strnecmp(linePtr, ";pointer:", 9, lineEnd)==0)
        {
            /* pointer (local, global, constant */
            AmdKernelArgInput arg;
            const char* ptr = strechr(linePtr+9, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse pointer argument");
            arg.argName.assign(linePtr+9, ptr);
            arg.argType = KernelArgType::POINTER;
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.resId = BINGEN_DEFAULT;
            arg.used = true;
            ptr++;
            const char* nextPtr = strechr(ptr, lineEnd, ':');
            if (nextPtr==nullptr)
                throw ParseException(lineNo, "Can't parse pointer argument");
            const char* typeName = ptr;
            const char* typeNameEnd = nextPtr;
            ptr = nextPtr;
            //ptr += 5; // to argOffset
            ptr++;
            ptr = strechr(ptr, lineEnd, ':');
            ptr++;
            ptr = strechr(ptr, lineEnd, ':');
            ptr++;
            ptr = strechr(ptr, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse pointer argument");
            ptr++;
            // qualifier
            if (::strnecmp(ptr, "uav:", 4, lineEnd) == 0)
                arg.ptrSpace = KernelPtrSpace::GLOBAL;
            else if (::strnecmp(ptr, "hc:", 3, lineEnd) == 0 ||
                        ::strnecmp(ptr, "c:", 2, lineEnd) == 0)
                arg.ptrSpace = KernelPtrSpace::CONSTANT;
            else if (::strnecmp(ptr, "hl:", 3, lineEnd) == 0)
                arg.ptrSpace = KernelPtrSpace::LOCAL;
            else
                throw ParseException(lineNo, "Can't parse pointer space");
            ptr = strechr(ptr, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse pointer argument");
            ptr++;
            arg.resId = cstrtovCStyle<uint32_t>(ptr, lineEnd, outEnd);
            
            if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
                argCbIds[arg.resId] = config.args.size();
            
            argUavIds.push_back(arg.resId);
            //arg.resId = AMDBIN_DEFAULT;
            ptr = strechr(ptr, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse pointer argument");
            ptr++;
            if (::strnecmp(typeName, "opaque:", 7, typeNameEnd+1)==0)
                arg.pointerType = KernelArgType::STRUCTURE;
            else if (::strnecmp(typeName, "struct:", 7, typeNameEnd+1)==0)
            {
                arg.pointerType = KernelArgType::STRUCTURE;
                arg.structSize = cstrtovCStyle<uint32_t>(ptr, lineEnd, outEnd);
            }
            ptr = strechr(ptr, lineEnd, ':');
            
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse pointer argument");
            ptr++;
            // access qualifier
            if (::strnecmp(ptr, "RO:", 2, lineEnd)==0 &&
                    arg.ptrSpace==KernelPtrSpace::GLOBAL)
                arg.ptrAccess |= KARG_PTR_CONST;
            else if (::strnecmp(ptr, "RW:", 2, lineEnd) == 0)
                arg.ptrAccess |= KARG_PTR_NORMAL;
            
            ptr = strechr(ptr, lineEnd, ':');
            if (ptr==nullptr || ptr+2>=lineEnd)
                throw ParseException(lineNo, "Can't parse pointer argument");
            ptr++;
            // volatile
            if (*ptr == '1')
                arg.ptrAccess |= KARG_PTR_VOLATILE;
            ptr++;
            // restrict
            if (ptr+1>=lineEnd || *ptr!=':')
                throw ParseException(lineNo, "Can't parse pointer argument");
            ptr++;
            if (*ptr == '1')
                arg.ptrAccess |= KARG_PTR_RESTRICT;
            argIndexMap[arg.argName] = config.args.size();
            config.args.push_back(arg);
        }
        else if (::strnecmp(linePtr, ";image:", 7, lineEnd)==0)
        {
            /* parse image argument entry */
            AmdKernelArgInput arg;
            const char* ptr = strechr(linePtr+7, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse image argument");
            arg.argName.assign(linePtr+7, ptr);
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            arg.resId = BINGEN_DEFAULT;
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.used = true;
            ptr++;
            const char* nextPtr = strechr(ptr, lineEnd, ':');
            if (nextPtr==nullptr)
                throw ParseException(lineNo, "Can't parse image argument");
            const char* imgType = ptr;
            const char* imgTypeEnd = nextPtr;
            ptr = nextPtr+1;
            // parse type of image
            if (::strnecmp(imgType, "1D:", 3, imgTypeEnd+1)==0)
                arg.argType = KernelArgType::IMAGE1D;
            else if (::strnecmp(imgType, "1DA:", 4, imgTypeEnd+1)==0)
                arg.argType = KernelArgType::IMAGE1D_ARRAY;
            else if (::strnecmp(imgType, "1DB:", 4, imgTypeEnd+1)==0)
                arg.argType = KernelArgType::IMAGE1D_BUFFER;
            else if (::strnecmp(imgType, "2D:", 3, imgTypeEnd+1)==0)
                arg.argType = KernelArgType::IMAGE2D;
            else if (::strnecmp(imgType, "2DA:", 4, imgTypeEnd+1)==0)
                arg.argType = KernelArgType::IMAGE2D_ARRAY;
            else if (::strnecmp(imgType, "3D:", 3, imgTypeEnd+1)==0)
                arg.argType = KernelArgType::IMAGE3D;
            else
                throw ParseException(lineNo, "Can't parse image type");
            // image access qualifier
            if (::strnecmp(ptr,"RO:", 3, lineEnd) == 0)
                arg.ptrAccess |= KARG_PTR_READ_ONLY;
            else if (::strnecmp(ptr,"WO:", 3, lineEnd) == 0)
            {
                arg.ptrAccess |= KARG_PTR_WRITE_ONLY;
                woImageIds.push_back(config.args.size());
            }
            else
                throw ParseException(lineNo, "Can't parse image access");
            ptr += 3;
            arg.resId = cstrtovCStyle<uint32_t>(ptr, lineEnd, outEnd);
            argUavIds.push_back(0);
            argIndexMap[arg.argName] = config.args.size();
            config.args.push_back(arg);
        }
        else if (::strnecmp(linePtr, ";counter:", 9, lineEnd)==0)
        {
            /* counter */
            AmdKernelArgInput arg;
            const char* ptr = strechr(linePtr+9, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse counter argument");
            arg.argName.assign(linePtr+9, ptr);
            arg.argType = KernelArgType::COUNTER32;
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            ptr++; // skip ':'
            if (::strnecmp(ptr, "32:", 3, lineEnd)!=0)
                throw ParseException(lineNo, "Can't parse counter argument");
            ptr+=3; // '32:'
            arg.resId = cstrtovCStyle<uint32_t>(ptr, lineEnd, outEnd);
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.used = true;
            argUavIds.push_back(0);
            argIndexMap[arg.argName] = config.args.size();
            config.args.push_back(arg);
        }
        else if (::strnecmp(linePtr, ";constarg:", 10, lineEnd)==0)
        {
            /* constant argument to apply constant qualifier */
            cxuint argNo = cstrtovCStyle<cxuint>(linePtr+10, lineEnd, outEnd);
            constArgIndices.push_back(argNo);
        }
        else if (::strnecmp(linePtr, ";sampler:", 9, lineEnd)==0)
        {
            /* image sampler */
            const char* ptr = strechr(linePtr+9, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse sampler entry");
            std::string samplerName(linePtr+9, ptr);
            if (samplerName.compare(0, 8, "unknown_") == 0)
            {
                // add sampler
                ptr++;
                cxuint sampId = cstrtovCStyle<cxuint>(ptr, lineEnd, outEnd);
                ptr = strechr(ptr, lineEnd, ':');
                if (ptr==nullptr)
                    throw ParseException(lineNo, "Can't parse sampler entry");
                ptr += 3;
                cxuint value = cstrtovCStyle<cxuint>(ptr, lineEnd, outEnd);
                if (config.samplers.size() < sampId+1)
                    config.samplers.resize(sampId+1);
                config.samplers[sampId] = value;
            }
            else
            {
                // this argument's sampler
                auto it = argIndexMap.find(samplerName);
                if (it!=argIndexMap.end())
                    samplerArgIndices.push_back(it->second);
                argSamplers++;
            }
        }
        else if (::strnecmp(linePtr, ";reflection:", 12, lineEnd)==0)
        {
            /* reflection that have type name */
            cxuint argNo = cstrtovCStyle<cxuint>(linePtr+12, lineEnd, outEnd);
            if (argNo >= config.args.size())
                throw DisasmException("ArgNo out of range");
            const char* ptr = strechr(linePtr+12, lineEnd, ':');
            if (ptr==nullptr)
                throw ParseException(lineNo, "Can't parse reflection entry");
            ptr++;
            AmdKernelArgInput& arg = config.args[argNo];
            arg.typeName.assign(ptr, lineEnd);
            /* determine type */
            if (arg.argType == KernelArgType::POINTER &&
                arg.pointerType == KernelArgType::VOID)
            {
                // get type name from reflection and find name from type name
                CString ptrTypeName = arg.typeName.substr(0, arg.typeName.size()-1);
                auto it = binaryMapFind(disasmArgTypeNameMap, 
                        disasmArgTypeNameMap+disasmArgTypeNameMapLength,
                        ptrTypeName.c_str(), CStringLess());
                
                if (it != disasmArgTypeNameMap+disasmArgTypeNameMapLength)
                    arg.pointerType = it->second;
                // treat enum as unsigned integer
                else if (arg.typeName.compare(0, 5, "enum ")==0)
                    arg.pointerType = KernelArgType::UINT;
            }
            //else
        }
        else if (::strnecmp(linePtr, ";uavid:", 7, lineEnd)==0)
        {
            cxuint uavId = cstrtovCStyle<cxuint>(linePtr+7, lineEnd, outEnd);
            uavIdToCompare = uavId;
            if (driverVersion < 134805)
                uavIdToCompare = 9;
        }
        // get printfif
        else if (::strnecmp(linePtr, ";printfid:", 10, lineEnd)==0)
            config.printfId = cstrtovCStyle<cxuint>(linePtr+10, lineEnd, outEnd);
        // get privateid
        else if (::strnecmp(linePtr, ";privateid:", 11, lineEnd)==0)
            config.privateId = cstrtovCStyle<cxuint>(linePtr+11, lineEnd, outEnd);
        else if (::strnecmp(linePtr, ";cbid:", 6, lineEnd)==0)
            config.constBufferId= cstrtovCStyle<cxuint>(linePtr+6, lineEnd, outEnd);
        else if (::strnecmp(linePtr, ";memory:uavprivate:", 19, lineEnd)==0)
            config.uavPrivate = cstrtovCStyle<cxuint>(linePtr+19, lineEnd, outEnd);
        // to next line
        linePtr = (lineEnd!=mtEnd) ? lineEnd+1 : lineEnd;
    }
    
    if (argSamplers != 0 && !config.samplers.empty())
    {
        // get sampler's values
        for (cxuint k = argSamplers; k < config.samplers.size(); k++)
            config.samplers[k-argSamplers] = config.samplers[k];
        config.samplers.resize(config.samplers.size()-argSamplers);
    }
    // set sampler type to some arguments (if they are samplers)
    for (cxuint argIdx: samplerArgIndices)
        if (argIdx < config.args.size())
            config.args[argIdx].argType = KernelArgType::SAMPLER;
        else
            throw DisasmException("Sampler arg index out of range");
    
    // apply const qualifier to some pointer arguments
    for (cxuint argIdx: constArgIndices)
        if (argIdx < config.args.size())
            config.args[argIdx].ptrAccess |= KARG_PTR_CONST;
        else
            throw DisasmException("Const arg index out of range");
    
    /* from ATI CAL NOTES */
    cxbyte uavMask[128];
    std::fill(uavMask, uavMask+128, cxbyte(0));
    for (const CALNoteInput& calNoteInput: calNotes)
    {
        const CALNoteHeader& cnHdr = calNoteInput.header;
        const cxbyte* cnData = calNoteInput.data;
        
        switch (cnHdr.type)
        {
            case CALNOTE_ATI_SCRATCH_BUFFERS:
                config.scratchBufferSize =
                        (ULEV(*reinterpret_cast<const uint32_t*>(cnData))<<2);
                break;
            case CALNOTE_ATI_CONSTANT_BUFFERS:
                if (driverVersion < 112402)
                {
                    // older drivers holds info about constant buffer in that CALNote
                    const CALConstantBufferMask* cbMask =
                            reinterpret_cast<const CALConstantBufferMask*>(cnData);
                    const cxuint entriesNum = cnHdr.descSize/sizeof(CALConstantBufferMask);
                    for (cxuint k = 0; k < entriesNum; k++)
                    {
                        if (argCbIds.find(ULEV(cbMask[k].index)) == argCbIds.end())
                            continue;
                        config.args[argCbIds[ULEV(cbMask[k].index)]].constSpaceSize =
                                ULEV(cbMask[k].size)<<4;
                    }
                }
                break;
            case CALNOTE_ATI_CONDOUT:
                config.condOut = ULEV(*reinterpret_cast<const uint32_t*>(cnData));
                break;
            case CALNOTE_ATI_EARLYEXIT:
                config.earlyExit = ULEV(*reinterpret_cast<const uint32_t*>(cnData));
                break;
            case CALNOTE_ATI_PROGINFO:
            {
                // PROG INFO CAL notes
                const CALProgramInfoEntry* piEntry =
                        reinterpret_cast<const CALProgramInfoEntry*>(cnData);
                const cxuint piEntriesNum = cnHdr.descSize/sizeof(CALProgramInfoEntry);
                for (cxuint k = 0; k < piEntriesNum; k++)
                {
                    // determine type by address
                    switch(ULEV(piEntry[k].address))
                    {
                        case 0x80001000:
                            config.userDatas.resize(ULEV(piEntry[k].value));
                            break;
                        case 0x8000001f:
                        {
                            if (driverVersion >= 134805)
                                config.useConstantData =
                                        ((ULEV(piEntry[k].value) & 0x400) != 0);
                            cxuint imgid = 0;
                            for (cxuint woImg: woImageIds)
                            {
                                AmdKernelArgInput& imgArg = config.args[woImg];
                                imgArg.used = ((ULEV(piEntry[k].value)&(1U<<imgid)) != 0);
                                imgid++;
                            }
                            uavIdToCompare = ((ULEV(piEntry[k].value)&
                                    (1U<<uavIdToCompare)) != 0)?0:uavIdToCompare;
                            break;
                        }
                        case 0x80001041:
                            config.usedVGPRsNum = ULEV(piEntry[k].value);
                            break;
                        case 0x80001042:
                            config.usedSGPRsNum = ULEV(piEntry[k].value);
                            break;
                        case 0x80001043:
                            config.floatMode = ULEV(piEntry[k].value);
                            break;
                        case 0x80001044:
                            config.ieeeMode = ULEV(piEntry[k].value);
                            break;
                        case 0x00002e13:
                        {
                            // get PGM_RSRC2 value and set config values
                            config.pgmRSRC2 = ULEV(piEntry[k].value);
                            config.tgSize = (config.pgmRSRC2 & 0x400)!=0;
                            config.exceptions = (config.pgmRSRC2>>24)&0x7f;
                            config.dimMask = getDefaultDimMask(arch, config.pgmRSRC2);
                            break;
                        }
                        default:
                        {
                            const uint32_t addr = ULEV(piEntry[k].address);
                            const uint32_t val = ULEV(piEntry[k].value);
                            if (addr >= 0x80001001 && addr < 0x80001041)
                            {
                                // if USER DATA's
                                const cxuint elIndex = (addr-0x80001001)>>2;
                                if (config.userDatas.size() <= elIndex)
                                    continue; // ignore userdata beyond specified number
                                switch (addr&3)
                                {
                                    // get DATA_CLASS, APISLOIT, REGSTART and REGEND
                                    case 1:
                                        config.userDatas[elIndex].dataClass = val;
                                        break;
                                    case 2:
                                        config.userDatas[elIndex].apiSlot = val;
                                        break;
                                    case 3:
                                        config.userDatas[elIndex].regStart = val;
                                        break;
                                    case 0:
                                        config.userDatas[elIndex].regSize = val;
                                        break;
                                }
                            }
                            if (addr >= 0x80001843 && addr < 0x80001863)
                            {
                                // get uav mask from proginfo
                                const cxuint elIndex = (addr-0x80001843)<<2;
                                uavMask[elIndex] = val&0xff;
                                uavMask[elIndex+1] = (val>>8)&0xff;
                                uavMask[elIndex+2] = (val>>16)&0xff;
                                uavMask[elIndex+3] = val>>24;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    // check if argument is unused
    cxuint k = 0;
    for (AmdKernelArgInput& arg: config.args)
    {
        /* apply used flag for argument */
        if (arg.argType == KernelArgType::POINTER &&
            (arg.ptrSpace == KernelPtrSpace::GLOBAL ||
             (arg.ptrSpace == KernelPtrSpace::CONSTANT && driverVersion >= 134805)) &&
                 (uavMask[argUavIds[k]>>3] & (1U<<(argUavIds[k]&7))) == 0)
            arg.used = false;
        k++;
    }
    
    config.uavId = uavIdToCompare;
    return config;
}

// AMD CAL notes table by CAL note type
static const char* disasmCALNoteNamesTable[] =
{
    ".proginfo",
    ".inputs",
    ".outputs",
    ".condout",
    ".floatconsts",
    ".intconsts",
    ".boolconsts",
    ".earlyexit",
    ".globalbuffers",
    ".constantbuffers",
    ".inputsamplers",
    ".persistentbuffers",
    ".scratchbuffers",
    ".subconstantbuffers",
    ".uavmailboxsize",
    ".uav",
    ".uavopmask"
};

/* dump kernel configuration in machine readable form */
static void dumpAmdKernelDatas(std::ostream& output, const AmdDisasmKernelInput& kinput,
       Flags flags)
{
    if ((flags & DISASM_METADATA) != 0)
    {
        if (kinput.header != nullptr && kinput.headerSize != 0)
        {
            // if kernel header available
            output.write("    .header\n", 12);
            printDisasmData(kinput.headerSize, kinput.header, output, true);
        }
        if (kinput.metadata != nullptr && kinput.metadataSize != 0)
        {
            // if kernel metadata available
            output.write("    .metadata\n", 14);
            printDisasmLongString(kinput.metadataSize, kinput.metadata, output, true);
        }
    }
    if ((flags & DISASM_DUMPDATA) != 0 && kinput.data != nullptr && kinput.dataSize != 0)
    {
        // if kernel data available
        output.write("    .data\n", 10);
        printDisasmData(kinput.dataSize, kinput.data, output, true);
    }
    
    if ((flags & DISASM_CALNOTES) != 0)
        for (const CALNoteInput& calNote: kinput.calNotes)
        {
            char buf[80];
            // calNote.header fields is already in native endian
            if (calNote.header.type != 0 && calNote.header.type <= CALNOTE_ATI_MAXTYPE)
            {
                output.write("    ", 4);
                output.write(disasmCALNoteNamesTable[calNote.header.type-1],
                             ::strlen(disasmCALNoteNamesTable[calNote.header.type-1]));
            }
            else
            {
                // unknown CAL note type
                const size_t len = itocstrCStyle(calNote.header.type, buf, 32, 16);
                output.write("    .calnote ", 13);
                output.write(buf, len);
            }
            
            if (calNote.data == nullptr || calNote.header.descSize==0)
            {
                output.put('\n');
                continue; // skip if no data
            }
            
            switch(calNote.header.type)
            {
                // handle CAL note types
                case CALNOTE_ATI_PROGINFO:
                {
                    // print cal notes as prog info entries
                    output.put('\n');
                    const cxuint progInfosNum =
                            calNote.header.descSize/sizeof(CALProgramInfoEntry);
                    const CALProgramInfoEntry* progInfos =
                        reinterpret_cast<const CALProgramInfoEntry*>(calNote.data);
                    ::memcpy(buf, "        .entry ", 15);
                    for (cxuint k = 0; k < progInfosNum; k++)
                    {
                        // format: .proginfo address, value
                        const CALProgramInfoEntry& progInfo = progInfos[k];
                        size_t bufPos = 15 + itocstrCStyle(ULEV(progInfo.address),
                                     buf+15, 32, 16, 8);
                        buf[bufPos++] = ',';
                        buf[bufPos++] = ' ';
                        bufPos += itocstrCStyle(ULEV(progInfo.value),
                                  buf+bufPos, 32, 16, 8);
                        buf[bufPos++] = '\n';
                        output.write(buf, bufPos);
                    }
                    /// rest
                    printDisasmData(calNote.header.descSize -
                            progInfosNum*sizeof(CALProgramInfoEntry),
                            calNote.data + progInfosNum*sizeof(CALProgramInfoEntry),
                            output, true);
                    break;
                }
                case CALNOTE_ATI_INPUTS:
                case CALNOTE_ATI_OUTPUTS:
                case CALNOTE_ATI_GLOBAL_BUFFERS:
                case CALNOTE_ATI_SCRATCH_BUFFERS:
                case CALNOTE_ATI_PERSISTENT_BUFFERS:
                    output.put('\n');
                    printDisasmDataU32(calNote.header.descSize>>2,
                           reinterpret_cast<const uint32_t*>(calNote.data),
                           output, true);
                    printDisasmData(calNote.header.descSize&3,
                           calNote.data + (calNote.header.descSize&~3U), output, true);
                    break;
                case CALNOTE_ATI_INT32CONSTS:
                case CALNOTE_ATI_FLOAT32CONSTS:
                case CALNOTE_ATI_BOOL32CONSTS:
                {
                    // print cal notes as segment list
                    output.put('\n');
                    const cxuint segmentsNum =
                            calNote.header.descSize/sizeof(CALDataSegmentEntry);
                    const CALDataSegmentEntry* segments =
                            reinterpret_cast<const CALDataSegmentEntry*>(calNote.data);
                    ::memcpy(buf, "        .segment ", 17);
                    for (cxuint k = 0; k < segmentsNum; k++)
                    {
                        const CALDataSegmentEntry& segment = segments[k];
                        // format: .segment offset, size
                        size_t bufPos = 17 + itocstrCStyle(
                                    ULEV(segment.offset), buf + 17, 32);
                        buf[bufPos++] = ',';
                        buf[bufPos++] = ' ';
                        bufPos += itocstrCStyle(ULEV(segment.size), buf+bufPos, 32);
                        buf[bufPos++] = '\n';
                        output.write(buf, bufPos);
                    }
                    /// rest
                    printDisasmData(calNote.header.descSize -
                            segmentsNum*sizeof(CALDataSegmentEntry),
                            calNote.data + segmentsNum*sizeof(CALDataSegmentEntry),
                            output, true);
                    break;
                }
                case CALNOTE_ATI_INPUT_SAMPLERS:
                {
                    // print cal notes as sampler list
                    output.put('\n');
                    const cxuint samplersNum =
                            calNote.header.descSize/sizeof(CALSamplerMapEntry);
                    const CALSamplerMapEntry* samplers =
                            reinterpret_cast<const CALSamplerMapEntry*>(calNote.data);
                    ::memcpy(buf, "        .sampler ", 17);
                    for (cxuint k = 0; k < samplersNum; k++)
                    {
                        const CALSamplerMapEntry& sampler = samplers[k];
                        // format: .sampler input, sampler
                        size_t bufPos = 17 + itocstrCStyle(
                                    ULEV(sampler.input), buf + 17, 32);
                        buf[bufPos++] = ',';
                        buf[bufPos++] = ' ';
                        bufPos += itocstrCStyle(ULEV(sampler.sampler),
                                    buf+bufPos, 32, 16);
                        buf[bufPos++] = '\n';
                        output.write(buf, bufPos);
                    }
                    /// rest
                    printDisasmData(calNote.header.descSize -
                            samplersNum*sizeof(CALSamplerMapEntry),
                            calNote.data + samplersNum*sizeof(CALSamplerMapEntry),
                            output, true);
                    break;
                }
                case CALNOTE_ATI_CONSTANT_BUFFERS:
                {
                    // print cal notes as cbmasks
                    output.put('\n');
                    const cxuint constBufMasksNum =
                            calNote.header.descSize/sizeof(CALConstantBufferMask);
                    const CALConstantBufferMask* constBufMasks =
                        reinterpret_cast<const CALConstantBufferMask*>(calNote.data);
                    ::memcpy(buf, "        .cbmask ", 16);
                    for (cxuint k = 0; k < constBufMasksNum; k++)
                    {
                        // format: .cbmask index, size
                        const CALConstantBufferMask& cbufMask = constBufMasks[k];
                        size_t bufPos = 16 + itocstrCStyle(
                                    ULEV(cbufMask.index), buf + 16, 32);
                        buf[bufPos++] = ',';
                        buf[bufPos++] = ' ';
                        bufPos += itocstrCStyle(ULEV(cbufMask.size), buf+bufPos, 32);
                        buf[bufPos++] = '\n';
                        output.write(buf, bufPos);
                    }
                    /// rest
                    printDisasmData(calNote.header.descSize -
                        constBufMasksNum*sizeof(CALConstantBufferMask),
                        calNote.data + constBufMasksNum*sizeof(CALConstantBufferMask),
                        output, true);
                    break;
                }
                case CALNOTE_ATI_EARLYEXIT:
                case CALNOTE_ATI_CONDOUT:
                case CALNOTE_ATI_UAV_OP_MASK:
                case CALNOTE_ATI_UAV_MAILBOX_SIZE:
                    if (calNote.header.descSize == 4)
                    {
                        const size_t len = itocstrCStyle(
                                ULEV(*reinterpret_cast<const uint32_t*>(
                                    calNote.data)), buf, 32);
                        output.put(' ');
                        output.write(buf, len);
                        output.put('\n');
                    }
                    else
                    {
                        // otherwise if size is not 4 bytes
                        output.put('\n');
                        printDisasmData(calNote.header.descSize,
                                calNote.data, output, true);
                    }
                    break;
                case CALNOTE_ATI_UAV:
                {
                    output.put('\n');
                    const cxuint uavsNum =
                            calNote.header.descSize/sizeof(CALUAVEntry);
                    const CALUAVEntry* uavEntries =
                        reinterpret_cast<const CALUAVEntry*>(calNote.data);
                    ::memcpy(buf, "        .entry ", 15);
                    for (cxuint k = 0; k < uavsNum; k++)
                    {
                        const CALUAVEntry& uavEntry = uavEntries[k];
                        /// uav entry format: .entry UAVID, F1, F2, TYPE
                        size_t bufPos = 15 + itocstrCStyle(
                                    ULEV(uavEntry.uavId), buf + 15, 32);
                        buf[bufPos++] = ',';
                        buf[bufPos++] = ' ';
                        bufPos += itocstrCStyle(ULEV(uavEntry.f1), buf+bufPos, 32);
                        buf[bufPos++] = ',';
                        buf[bufPos++] = ' ';
                        bufPos += itocstrCStyle(ULEV(uavEntry.f2), buf+bufPos, 32);
                        buf[bufPos++] = ',';
                        buf[bufPos++] = ' ';
                        bufPos += itocstrCStyle(ULEV(uavEntry.type), buf+bufPos, 32);
                        buf[bufPos++] = '\n';
                        output.write(buf, bufPos);
                    }
                    /// rest
                    printDisasmData(calNote.header.descSize -
                        uavsNum*sizeof(CALUAVEntry),
                        calNote.data + uavsNum*sizeof(CALUAVEntry), output, true);
                    break;
                }
                default:
                    output.put('\n');
                    printDisasmData(calNote.header.descSize, calNote.data,
                                    output, true);
                    break;
            }
        }
}

// USER DATA class names by value
static const char* dataClassNameTbl[] =
{
    "imm_resource",
    "imm_sampler",
    "imm_const_buffer",
    "imm_vertex_buffer",
    "imm_uav",
    "imm_alu_float_const",
    "imm_alu_bool32_const",
    "imm_gds_counter_range",
    "imm_gds_memory_range",
    "imm_gws_base",
    "imm_work_item_range",
    "imm_work_group_range",
    "imm_dispatch_id",
    "imm_scratch_buffer",
    "imm_heap_buffer",
    "imm_kernel_arg",
    "sub_ptr_fetch_shader",
    "ptr_resource_table",
    "ptr_internal_resource_table",
    "ptr_sampler_table",
    "ptr_const_buffer_table",
    "ptr_vertex_buffer_table",
    "ptr_so_buffer_table",
    "ptr_uav_table",
    "ptr_internal_global_table",
    "ptr_extended_user_data",
    "ptr_indirect_resource",
    "ptr_indirect_internal_resource",
    "ptr_indirect_uav",
    "imm_context_base",
    "imm_lds_esgs_size",
    "imm_global_offset",
    "imm_generic_user_data"
};

// kernel arg types table
static const char* kernelArgTypeNamesTbl[] =
{
    "void",
    "uchar", "char", "ushort", "short", "uint", "int", "ulong", "long", "float",
    "double", "", "image", "image1d", "image1d_array", "image1d_buffer", "image2d",
    "image2d_array", "image3d",
    "uchar2", "uchar3", "uchar4", "uchar8", "uchar16",
    "char2", "char3", "char4", "char8", "char16",
    "ushort2", "ushort3", "ushort4", "ushort8", "ushort16",
    "short2", "short3", "short4", "short8", "short16",
    "uint2", "uint3", "uint4", "uint8", "uint16",
    "int2", "int3", "int4", "int8", "int16",
    "ulong2", "ulong3", "ulong4", "ulong8", "ulong16",
    "long2", "long3", "long4", "long8", "long16",
    "float2", "float3", "float4", "float8", "float16",
    "double2", "double3", "double4", "double8", "double16",
    "sampler", "structure", "counter32", "counter64", "pipe", "queue", "clkevent"
};

/* function to print kernel argument (used by DisasmAmd and DisasmAmdCL2 */
void CLRX::dumpAmdKernelArg(std::ostream& output, const AmdKernelArgInput& arg, bool cl20)
{
    size_t bufSize;
    char buf[100];
    output.write("        .arg ", 13);
    output.write(arg.argName.c_str(), arg.argName.size());
    output.write(", \"", 3);
    output.write(arg.typeName.c_str(), arg.typeName.size());
    if (arg.argType != KernelArgType::POINTER)
    {
        if (arg.argType > KernelArgType::MAX_VALUE_CL2)
            throw DisasmException("Unknown argument type");
        bufSize = snprintf(buf, 100, "\", %s",
                   kernelArgTypeNamesTbl[cxuint(arg.argType)]);
        output.write(buf, bufSize);
        if (arg.argType == KernelArgType::STRUCTURE)
        {
            // structure size
            bufSize = snprintf(buf, 100, ", %u", arg.structSize);
            output.write(buf, bufSize);
        }
        bool isImage = false;
        if (isKernelArgImage(arg.argType))
        {
            // images
            isImage = true;
            cxbyte access = arg.ptrAccess & KARG_PTR_ACCESS_MASK;
            // print access qualifier
            if (access == KARG_PTR_READ_ONLY)
                output.write(", read_only", 11);
            else if (access == KARG_PTR_WRITE_ONLY)
                output.write(", write_only", 12);
            else if (access == KARG_PTR_READ_WRITE)
                output.write(", read_write", 12);
            else
                output.write(", ", 2);
        }
        if (isImage || ((!cl20 && arg.argType == KernelArgType::COUNTER32) ||
            (cl20 && arg.argType == KernelArgType::SAMPLER)))
        {
            // print resource id: only for images counters and for samplers (if CL2.0)
            bufSize = snprintf(buf, 100, ", %u", arg.resId);
            output.write(buf, bufSize);
        }
    }
    else
    {
        if (arg.pointerType > KernelArgType::MAX_VALUE_CL2)
            throw DisasmException("Unknown argument pointer type");
        // pointer
        bufSize = snprintf(buf, 100, "\", %s*",
                   kernelArgTypeNamesTbl[cxuint(arg.pointerType)]);
        output.write(buf, bufSize);
        if (arg.pointerType == KernelArgType::STRUCTURE)
        {
            // structure size
            bufSize = snprintf(buf, 100, ", %u", arg.structSize);
            output.write(buf, bufSize);
        }
        // print pointer space
        if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
            output.write(", constant", 10);
        else if (arg.ptrSpace == KernelPtrSpace::LOCAL)
            output.write(", local", 7);
        else if (arg.ptrSpace == KernelPtrSpace::GLOBAL)
            output.write(", global", 8);
        
        if ((arg.ptrAccess & (KARG_PTR_CONST|KARG_PTR_VOLATILE|KARG_PTR_RESTRICT))!=0)
        {
            // pointer access
            bufSize = snprintf(buf, 100, ",%s%s%s",
                     ((arg.ptrAccess & KARG_PTR_CONST) ? " const" : ""),
                     ((arg.ptrAccess & KARG_PTR_RESTRICT) ? " restrict" : ""),
                     ((arg.ptrAccess & KARG_PTR_VOLATILE) ? " volatile" : ""));
            output.write(buf, bufSize);
        }
        else // empty
            output.write(", ", 2);
        if (arg.ptrSpace==KernelPtrSpace::CONSTANT && !cl20)
        {
            // constant size
            bufSize = snprintf(buf, 100, ", %" PRIu64, uint64_t(arg.constSpaceSize));
            output.write(buf, bufSize);
        }
        if (arg.ptrSpace!=KernelPtrSpace::LOCAL && !cl20)
        {
            // resid
            bufSize = snprintf(buf, 100, ", %u", arg.resId);
            output.write(buf, bufSize);
        }
    }
    if (!arg.used)
        output.write(", unused\n", 9);
    // further flags for OpenCL 2.0 binary format
    else if (cl20 && arg.used==AMDCL2_ARGUSED_READ)
        output.write(", rdonly\n", 9);
    else if (cl20 && arg.used==AMDCL2_ARGUSED_WRITE)
        output.write(", wronly\n", 9);
    else
        output.write("\n", 1);
}

static void dumpAmdKernelConfig(std::ostream& output, const AmdKernelConfig& config)
{
    size_t bufSize;
    char buf[100];
    output.write("    .config\n", 12);
    if (config.dimMask != BINGEN_DEFAULT)
    {
        // print dimensions (.dims xyz)
        strcpy(buf, "        .dims ");
        bufSize = 14;
        if ((config.dimMask & 1) != 0)
            buf[bufSize++] = 'x';
        if ((config.dimMask & 2) != 0)
            buf[bufSize++] = 'y';
        if ((config.dimMask & 4) != 0)
            buf[bufSize++] = 'z';
        if ((config.dimMask & 7) != ((config.dimMask>>3) & 7))
        {
            buf[bufSize++] = ',';
            buf[bufSize++] = ' ';
            if ((config.dimMask & 8) != 0)
                buf[bufSize++] = 'x';
            if ((config.dimMask & 16) != 0)
                buf[bufSize++] = 'y';
            if ((config.dimMask & 32) != 0)
                buf[bufSize++] = 'z';
        }
        buf[bufSize++] = '\n';
        output.write(buf, bufSize);
    }
    // print reqd_work_group_size: .cws XSIZE[,YSIZE[,ZSIZE]]
    if (config.reqdWorkGroupSize[0] != 0 || config.reqdWorkGroupSize[1] != 0 ||
        config.reqdWorkGroupSize[2] != 0)
    {
        bufSize = snprintf(buf, 100, "        .cws %u, %u, %u\n",
               config.reqdWorkGroupSize[0], config.reqdWorkGroupSize[1],
               config.reqdWorkGroupSize[2]);
        output.write(buf, bufSize);
    }
    
#if CLRX_VERSION_NUMBER >= CLRX_POLICY_UNIFIED_SGPR_COUNT
    bufSize = snprintf(buf, 100, "        .sgprsnum %u\n", config.usedSGPRsNum+2);
#else
    bufSize = snprintf(buf, 100, "        .sgprsnum %u\n", config.usedSGPRsNum);
#endif
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .vgprsnum %u\n", config.usedVGPRsNum);
    output.write(buf, bufSize);
    if (config.hwRegion!=0 && config.hwRegion!=BINGEN_DEFAULT)
    {
        bufSize = snprintf(buf, 100, "        .hwregion %u\n", config.hwRegion);
        output.write(buf, bufSize);
    }
    if (config.hwLocalSize!=0)
    {
        bufSize = snprintf(buf, 100, "        .hwlocal %" PRIu64 "\n",
                       uint64_t(config.hwLocalSize));
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
    if (config.uavId!=BINGEN_DEFAULT)
    {
        bufSize = snprintf(buf, 100, "        .uavid %u\n", config.uavId);
        output.write(buf, bufSize);
    }
    if (config.uavPrivate!=BINGEN_DEFAULT)
    {
        bufSize = snprintf(buf, 100, "        .uavprivate %u\n", config.uavPrivate);
        output.write(buf, bufSize);
    }
    if (config.printfId!=BINGEN_DEFAULT)
    {
        bufSize = snprintf(buf, 100, "        .printfid %u\n", config.printfId);
        output.write(buf, bufSize);
    }
    if (config.privateId!=BINGEN_DEFAULT)
    {
        bufSize = snprintf(buf, 100, "        .privateid %u\n", config.privateId);
        output.write(buf, bufSize);
    }
    if (config.constBufferId!=BINGEN_DEFAULT)
    {
        bufSize = snprintf(buf, 100, "        .cbid %u\n", config.constBufferId);
        output.write(buf, bufSize);
    }
    bufSize = snprintf(buf, 100, "        .earlyexit %u\n", config.earlyExit);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .condout %u\n", config.condOut);
    output.write(buf, bufSize);
    bufSize = snprintf(buf, 100, "        .pgmrsrc2 0x%08x\n", config.pgmRSRC2);
    output.write(buf, bufSize);
    // flags in PGMRSRC2
    if (config.ieeeMode)
        output.write("        .ieeemode\n", 18);
    if (config.tgSize)
        output.write("        .tgsize\n", 16);
    if (config.usePrintf)
        output.write("        .useprintf\n", 19);
    if (config.useConstantData)
        output.write("        .useconstdata\n", 22);
    if ((config.exceptions & 0x7f) != 0)
    {
        bufSize = snprintf(buf, 100, "        .exceptions 0x%02x\n",
                   cxuint(config.exceptions));
        output.write(buf, bufSize);
    }
    /* user datas */
    for (AmdUserData userData: config.userDatas)
    {
        const char* dataClassName = "unknown";
        if (userData.dataClass < sizeof(dataClassNameTbl)/sizeof(char*))
            dataClassName = dataClassNameTbl[userData.dataClass];
        bufSize = snprintf(buf, 100, "        .userdata %s, %u, %u, %u\n", dataClassName,
                userData.apiSlot, userData.regStart, userData.regSize);
        output.write(buf, bufSize);
    }
    // arguments
    for (const AmdKernelArgInput& arg: config.args)
        dumpAmdKernelArg(output, arg, false);
    // samplers
    for (cxuint sampler: config.samplers)
    {
        bufSize = snprintf(buf, 100, "        .sampler 0x%x\n", sampler);
        output.write(buf, bufSize);
    }
}

void CLRX::disassembleAmd(std::ostream& output, const AmdDisasmInput* amdInput,
       ISADisassembler* isaDisassembler, size_t& sectionCount, Flags flags)
{
    if (amdInput->is64BitMode)
        output.write(".64bit\n", 7);
    else
        output.write(".32bit\n", 7);
    
    const bool doMetadata = ((flags & DISASM_METADATA) != 0);
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    
    if (doMetadata)
    {
        // compile options and driver info belongs to metadata
        output.write(".compile_options \"", 18);
        const std::string escapedCompileOptions = 
                escapeStringCStyle(amdInput->compileOptions);
        output.write(escapedCompileOptions.c_str(), escapedCompileOptions.size());
        output.write("\"\n.driver_info \"", 16);
        const std::string escapedDriverInfo =
                escapeStringCStyle(amdInput->driverInfo);
        output.write(escapedDriverInfo.c_str(), escapedDriverInfo.size());
        output.write("\"\n", 2);
    }
    
    if (doDumpData && amdInput->globalData != nullptr && amdInput->globalDataSize != 0)
    {   //
        output.write(".globaldata\n", 12);
        printDisasmData(amdInput->globalDataSize, amdInput->globalData, output);
    }
    
    for (const AmdDisasmKernelInput& kinput: amdInput->kernels)
    {
        output.write(".kernel ", 8);
        output.write(kinput.kernelName.c_str(), kinput.kernelName.size());
        output.put('\n');
        if ((flags & DISASM_CONFIG) == 0) // if not config
            dumpAmdKernelDatas(output, kinput, flags);
        else
        {
            // dump in human readable configuration
            AmdKernelConfig config = getAmdKernelConfig(kinput.metadataSize,
                    kinput.metadata, kinput.calNotes, amdInput->driverInfo,
                    kinput.header, getGPUArchitectureFromDeviceType(amdInput->deviceType));
            dumpAmdKernelConfig(output, config);
        }
        
        if (doDumpCode && kinput.code != nullptr && kinput.codeSize != 0)
        {
            // input kernel code (main disassembly)
            output.write("    .text\n", 10);
            isaDisassembler->setInput(kinput.codeSize, kinput.code);
            isaDisassembler->beforeDisassemble();
            isaDisassembler->disassemble();
            sectionCount++;
        }
    }
}
