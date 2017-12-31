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
#include <iostream>
#include <sstream>
#include <map>
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/AmdBinGen.h>

using namespace CLRX;

static const char* origBinaryFiles[] =
{
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_12_10.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_12_10_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_13_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_13_12_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_14_9.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_14_9_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_14_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo1_14_12_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_12_10.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_12_10_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_13_4.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_13_4_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_13_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_13_12_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_14_9.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_14_9_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_14_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo4_14_12_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_12_10.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_12_10_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_13_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_13_12_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_14_9.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_14_9_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_14_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo7_14_12_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_12_10.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_12_10_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_13_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_13_12_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_9.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_9_64.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12.clo.1_0",
    CLRX_SOURCE_DIR "/tests/amdbin/amdbins/prginfo8_14_12_64.clo.1_0"
};

static const char* openclArgTypeNamesTbl[] =
{
    "void",
    "uchar", "char", "ushort", "short", "uint", "int", "ulong", "long", "float",
    "double", nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
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
    "sampler_t", nullptr, "counter32_t", nullptr
};

static const size_t openclArgTypeNamesTblLength =
        sizeof(openclArgTypeNamesTbl)/sizeof(const char*);

static AmdKernelConfig getAmdKernelConfig(size_t metadataSize, const char* metadata,
            const AmdInnerGPUBinary32& innerBin, const CString& driverInfo,
            const cxbyte* kernelHeader)
{
    cxuint driverVersion = 9999909U;
    {
        // parse version
        size_t pos = driverInfo.find("AMD-APP"); // find AMDAPP
        try
        {
            if (pos != std::string::npos)
            {   /* let to parse version number */
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
    
    AmdKernelConfig config;
    std::vector<cxuint> argUavIds;
    std::map<cxuint,cxuint> argCbIds;
    std::istringstream iss(std::string(metadata, metadataSize));
    config.dimMask = BINGEN_DEFAULT;
    config.printfId = BINGEN_DEFAULT;
    config.constBufferId = BINGEN_DEFAULT;
    config.uavPrivate = BINGEN_DEFAULT;
    config.uavId = BINGEN_DEFAULT;
    config.privateId = BINGEN_DEFAULT;
    config.exceptions = 0;
    config.usePrintf = ((ULEV(reinterpret_cast<const uint32_t*>(
                kernelHeader)[4]) & 2) != 0);
    config.tgSize = config.useConstantData = false;
    config.reqdWorkGroupSize[0] = config.reqdWorkGroupSize[1] =
            config.reqdWorkGroupSize[2] = 0;
    
    std::vector<cxuint> woImageIds;
    cxuint argSamplers = 0;
    
    cxuint uavIdToCompare = 0;
    /* parse arguments from metadata
     * very similar code as from AmdBinaries to parse kernel metadata */
    while (!iss.eof())
    {
        std::string line;
        std::getline(iss, line);
        const char* outEnd;
        // cws
        if (line.compare(0, 16, ";memory:hwlocal:")==0)
            config.hwLocalSize = cstrtovCStyle<size_t>(line.c_str()+16, nullptr, outEnd);
        else if (line.compare(0, 17, ";memory:hwregion:")==0)
            config.hwRegion = cstrtovCStyle<uint32_t>(line.c_str()+17, nullptr, outEnd);
        else if (line.compare(0, 5, ";cws:")==0)
        {
            // cws (reqd_work_group_size)
            config.reqdWorkGroupSize[0] = cstrtovCStyle<uint32_t>(
                        line.c_str()+5, nullptr, outEnd);
            outEnd++;
            config.reqdWorkGroupSize[1] = cstrtovCStyle<uint32_t>(
                        outEnd, nullptr, outEnd);
            outEnd++;
            config.reqdWorkGroupSize[2] = cstrtovCStyle<uint32_t>(
                        outEnd, nullptr, outEnd);
        }
        else if (line.compare(0, 7, ";value:")==0)
        {
            // parse single value argument
            AmdKernelArgInput arg;
            size_t pos = line.find(':', 7);
            arg.argName = line.substr(7, pos-7);
            arg.argType = KernelArgType::VOID;
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            arg.resId = BINGEN_DEFAULT;
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.resId = 0;
            arg.used = true;
            pos++;
            size_t nextPos = line.find(':', pos);
            std::string typeStr = line.substr(pos, nextPos-pos);
            pos = nextPos+1;
            if (typeStr == "struct")
            {
                arg.argType = KernelArgType::STRUCTURE;
                arg.structSize = cstrtovCStyle<uint32_t>(line.c_str()+pos, nullptr, outEnd);
            }
            argUavIds.push_back(0);
            config.args.push_back(arg);
        }
        else if (line.compare(0, 9, ";pointer:")==0)
        {
            AmdKernelArgInput arg;
            size_t pos = line.find(':', 9);
            arg.argName = line.substr(9, pos-9);
            arg.argType = KernelArgType::POINTER;
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.resId = BINGEN_DEFAULT;
            arg.used = true;
            pos++;
            size_t nextPos = line.find(':', pos);
            std::string typeName = line.substr(pos, nextPos-pos);
            pos = nextPos;
            pos += 5; // to argOffset
            pos = line.find(':', pos);
            pos++;
            // qualifier
            if (line.compare(pos, 3, "uav") == 0)
                arg.ptrSpace = KernelPtrSpace::GLOBAL;
            else if (line.compare(pos, 2, "hc") == 0 || line.compare(pos, 1, "c") == 0)
                arg.ptrSpace = KernelPtrSpace::CONSTANT;
            else if (line.compare(pos, 2, "hl") == 0)
                arg.ptrSpace = KernelPtrSpace::LOCAL;
            pos = line.find(':', pos);
            pos++;
            arg.resId = cstrtovCStyle<uint32_t>(line.c_str()+pos, nullptr, outEnd);
            
            if (arg.ptrSpace == KernelPtrSpace::CONSTANT)
                argCbIds[arg.resId] = config.args.size();
            
            argUavIds.push_back(arg.resId);
            //arg.resId = AMDBIN_DEFAULT;
            pos = line.find(':', pos);
            pos++;
            // set structure type for opaque or struct
            if (typeName == "opaque")
            {
                arg.pointerType = KernelArgType::STRUCTURE;
                pos = line.find(':', pos);
            }
            else if (typeName == "struct")
            {
                arg.pointerType = KernelArgType::STRUCTURE;
                arg.structSize = cstrtovCStyle<uint32_t>(line.c_str()+pos, nullptr, outEnd);
            }
            else
                pos = line.find(':', pos);
            pos++;
            // accesss qualifier (RO,RW)
            if (line.compare(pos, 2, "RO") == 0 && arg.ptrSpace == KernelPtrSpace::GLOBAL)
                arg.ptrAccess |= KARG_PTR_CONST;
            else if (line.compare(pos, 2, "RW") == 0)
                arg.ptrAccess |= KARG_PTR_NORMAL;
            pos = line.find(':', pos);
            pos++;
            // volatile qualifier
            if (line[pos] == '1')
                arg.ptrAccess |= KARG_PTR_VOLATILE;
            pos+=2;
            // restrict qualifier
            if (line[pos] == '1')
                arg.ptrAccess |= KARG_PTR_RESTRICT;
            config.args.push_back(arg);
        }
        else if (line.compare(0, 7, ";image:")==0)
        {
            AmdKernelArgInput arg;
            size_t pos = line.find(':', 7);
            arg.argName = line.substr(7, pos-7);
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            arg.resId = BINGEN_DEFAULT;
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.used = true;
            pos++;
            size_t nextPos = line.find(':', pos);
            std::string imgType = line.substr(pos, nextPos-pos);
            pos = nextPos+1;
            // parse image type:
            // image 1D
            if (imgType == "1D")
                arg.argType = KernelArgType::IMAGE1D;
            else if (imgType == "1DA")
                arg.argType = KernelArgType::IMAGE1D_ARRAY;
            else if (imgType == "1DB")
                arg.argType = KernelArgType::IMAGE1D_BUFFER;
            // image 2d
            else if (imgType == "2D")
                arg.argType = KernelArgType::IMAGE2D;
            else if (imgType == "2DA")
                arg.argType = KernelArgType::IMAGE2D_ARRAY;
            else if (imgType == "3D")
                arg.argType = KernelArgType::IMAGE3D;
            std::string accessStr = line.substr(pos, 2);
            // access qualifier
            if (line.compare(pos, 2, "RO") == 0)
                arg.ptrAccess |= KARG_PTR_READ_ONLY;
            else if (line.compare(pos, 2, "WO") == 0)
            {
                arg.ptrAccess |= KARG_PTR_WRITE_ONLY;
                woImageIds.push_back(config.args.size());
            }
            pos += 3;
            arg.resId = cstrtovCStyle<uint32_t>(line.c_str()+pos, nullptr, outEnd);
            argUavIds.push_back(0);
            config.args.push_back(arg);
        }
        else if (line.compare(0, 9, ";counter:")==0)
        {
            AmdKernelArgInput arg;
            size_t pos = line.find(':', 9);
            arg.argName = line.substr(9, pos-9);
            arg.argType = KernelArgType::COUNTER32;
            arg.pointerType = KernelArgType::VOID;
            arg.ptrSpace = KernelPtrSpace::NONE;
            arg.resId = BINGEN_DEFAULT;
            arg.ptrAccess = 0;
            arg.structSize = 0;
            arg.constSpaceSize = 0;
            arg.used = true;
            argUavIds.push_back(0);
            config.args.push_back(arg);
        }
        else if (line.compare(0, 10, ";constarg:")==0)
        {
            cxuint argNo = cstrtovCStyle<cxuint>(line.c_str()+10, nullptr, outEnd);
            config.args[argNo].ptrAccess |= KARG_PTR_CONST;
        }
        else if (line.compare(0, 9, ";sampler:")==0)
        {
            size_t pos = line.find(':', 9);
            std::string samplerName = line.substr(9, pos-9);
            if (samplerName.compare(0, 8, "unknown_") == 0)
            {
                // add sampler
                pos++;
                cxuint sampId = cstrtovCStyle<cxuint>(line.c_str()+pos, nullptr, outEnd);
                pos = line.find(':', pos);
                pos += 3;
                cxuint value = cstrtovCStyle<cxuint>(line.c_str()+pos, nullptr, outEnd);
                if (config.samplers.size() < sampId+1)
                    config.samplers.resize(sampId+1);
                config.samplers[sampId] = value;
            }
            else
                argSamplers++;
        }
        else if (line.compare(0, 12, ";reflection:")==0)
        {
            size_t pos = 12;
            cxuint argNo = cstrtovCStyle<cxuint>(line.c_str()+pos, nullptr, outEnd);
            pos = line.find(':', pos);
            pos++;
            AmdKernelArgInput& arg = config.args[argNo];
            arg.typeName = line.substr(pos);
            /* determine type */
            if (arg.argType == KernelArgType::POINTER &&
                arg.pointerType == KernelArgType::VOID)
            {
                cxuint found  = 0;
                CString ptrTypeName = arg.typeName.substr(0, arg.typeName.size()-1);
                for (found = 0; found < openclArgTypeNamesTblLength; found++)
                    if (openclArgTypeNamesTbl[found]!=nullptr &&
                        ::strcmp(ptrTypeName.c_str(), openclArgTypeNamesTbl[found]) == 0)
                        break;
                if (found != openclArgTypeNamesTblLength)
                    arg.pointerType = KernelArgType(found);
                else if (arg.typeName.compare(0, 5, "enum ")==0)
                    arg.pointerType = KernelArgType::UINT;
            }
            else if (arg.argType == KernelArgType::VOID)
            {
                // value
                cxuint found  = 0;
                for (found = 0; found < openclArgTypeNamesTblLength; found++)
                    if (openclArgTypeNamesTbl[found]!=nullptr &&
                        ::strcmp(arg.typeName.c_str(), openclArgTypeNamesTbl[found]) == 0)
                        break;
                if (found != openclArgTypeNamesTblLength)
                    arg.argType = KernelArgType(found);
                else if (arg.typeName.compare(0, 5, "enum ")==0)
                    arg.argType = KernelArgType::UINT;
            }
        }
        else if (line.compare(0, 7, ";uavid:")==0)
        {
            cxuint uavId = cstrtovCStyle<cxuint>(line.c_str()+7, nullptr, outEnd);
            uavIdToCompare = uavId;
            if (driverVersion < 134805)
                uavIdToCompare = 9;
        }
    }
    // add samplers to config
    if (argSamplers != 0 && !config.samplers.empty())
    {
        for (cxuint k = argSamplers; k < config.samplers.size(); k++)
            config.samplers[k-argSamplers] = config.samplers[k];
        config.samplers.resize(config.samplers.size()-argSamplers);
    }
    
    /* from ATI CAL NOTES */
    size_t encId = innerBin.getCALEncodingEntriesNum()-1;
    const cxuint calNotesNum = innerBin.getCALNotesNum(encId);
    for (cxuint i = 0; i < calNotesNum; i++)
    {
        const CALNoteHeader& cnHdr = innerBin.getCALNoteHeader(encId, i);
        const cxbyte* cnData = innerBin.getCALNoteData(encId, i);
        switch (ULEV(cnHdr.type))
        {
            case CALNOTE_ATI_SCRATCH_BUFFERS:
                config.scratchBufferSize =
                        (ULEV(*reinterpret_cast<const uint32_t*>(cnData))<<2);
                break;
            case CALNOTE_ATI_CONSTANT_BUFFERS:
                if (driverVersion < 112402)
                {
                    const CALConstantBufferMask* cbMask =
                            reinterpret_cast<const CALConstantBufferMask*>(cnData);
                    cxuint entriesNum = ULEV(cnHdr.descSize)/sizeof(CALConstantBufferMask);
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
                const CALProgramInfoEntry* piEntry =
                        reinterpret_cast<const CALProgramInfoEntry*>(cnData);
                const cxuint piEntriesNum =
                        ULEV(cnHdr.descSize)/sizeof(CALProgramInfoEntry);
                for (cxuint k = 0; k < piEntriesNum; k++)
                {
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
                            config.pgmRSRC2 = ULEV(piEntry[k].value);
                            break;
                        default:
                        {
                            uint32_t addr = ULEV(piEntry[k].address);
                            uint32_t val = ULEV(piEntry[k].value);
                            if (addr >= 0x80001001 && addr < 0x80001041)
                            {
                                cxuint elIndex = (addr-0x80001001)>>2;
                                if (config.userDatas.size() <= elIndex)
                                    continue;
                                switch (addr&3)
                                {
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
                        }
                    }
                }
                break;
            }
            case CALNOTE_ATI_UAV_OP_MASK:
            {
                cxuint k = 0;
                for (AmdKernelArgInput& arg: config.args)
                {
                    if (arg.argType == KernelArgType::POINTER &&
                        (arg.ptrSpace == KernelPtrSpace::GLOBAL ||
                         (arg.ptrSpace == KernelPtrSpace::CONSTANT &&
                             driverVersion >= 134805)) &&
                        (cnData[argUavIds[k]>>3] & (1U<<(argUavIds[k]&7))) == 0)
                        arg.used = false;
                    k++;
                }
                break;
            }
        }
    }
    return config;
}

template<typename AmdGpuBin>
static AmdInput genAmdInput(bool useConfig, const AmdGpuBin* amdGpuBin,
            bool haveSource, bool haveLLVMIR)
{
    AmdInput amdInput;
    amdInput.deviceType = GPUDeviceType::PITCAIRN;
    amdInput.globalDataSize = amdGpuBin->getGlobalDataSize();
    amdInput.globalData = amdGpuBin->getGlobalData();
    amdInput.driverVersion = 0;
    amdInput.driverInfo = amdGpuBin->getDriverInfo();
    amdInput.compileOptions = amdGpuBin->getCompileOptions();
    //amdInput.sourceCode = nullptr;
    //amdInput.sourceCodeSize = 0;
    if (haveSource)
    {
        try
        {
            const auto& shdr = amdGpuBin->getSectionHeader(".source");
            amdInput.extraSections.push_back(BinSection{ ".source",
                size_t(ULEV(shdr.sh_size)),
                amdGpuBin->getBinaryCode() + ULEV(shdr.sh_offset), 1,
                SHT_PROGBITS, 0, ELFSECTID_NULL, 0 });
        }
        catch(const Exception& ex)
        { }
    }
    
    //amdInput.llvmir = nullptr;
    //amdInput.llvmirSize = 0;
    if (haveLLVMIR)
    {
        try
        {
            const auto& shdr = amdGpuBin->getSectionHeader(".llvmir");
            amdInput.extraSections.push_back(BinSection{ ".llvmir",
                size_t(ULEV(shdr.sh_size)),
                amdGpuBin->getBinaryCode() + ULEV(shdr.sh_offset), 1,
                SHT_PROGBITS, 0, ELFSECTID_NULL, 0 });
        }
        catch(const Exception& ex)
        { }
    }
    
    for (cxuint i = 0; i < amdGpuBin->getInnerBinariesNum(); i++)
    {
        const AmdInnerGPUBinary32& innerBin = amdGpuBin->getInnerBinary(i);
        const cxuint sectionsNum = innerBin.getSectionHeadersNum();
        const cxbyte* data = nullptr;
        size_t dataSize = 0;
        const cxbyte* code = nullptr;
        size_t codeSize = 0;
        for (cxint k = sectionsNum-1; k >= 0 ; k--)
        {
            // set kernel code size and kernel data (from inner binary)
            if (::strcmp(".text", innerBin.getSectionName(k)) == 0)
            {
                const auto& secHdr = innerBin.getSectionHeader(k);
                codeSize = ULEV(secHdr.sh_size);
                code = innerBin.getSectionContent(k);
            }
            else if (::strcmp(".data", innerBin.getSectionName(k)) == 0)
            {
                const auto& secHdr = innerBin.getSectionHeader(k);
                dataSize = ULEV(secHdr.sh_size);
                data = innerBin.getSectionContent(k);
            }
            if (data!=nullptr && code!=nullptr)
                break;
        }
        // get CALNoteInputs
        const uint32_t encId = innerBin.getCALEncodingEntriesNum()-1;
        const uint32_t calNotesNum = innerBin.getCALNotesNum(encId);
        std::vector<CALNoteInput> calNotes;
        for (cxuint k = 0; k < calNotesNum; k++)
        {
            const CALNoteHeader& hdr = innerBin.getCALNoteHeader(encId, k);
            CALNoteInput cninput;
            cninput.header.type = ULEV(hdr.type);
            cninput.header.nameSize = ULEV(hdr.nameSize);
            cninput.header.descSize = ULEV(hdr.descSize);
            cninput.data = innerBin.getCALNoteData(encId, k);
            ::memcpy(cninput.header.name, "ATI CAL", 8);
            calNotes.push_back(cninput);
        }
        
        if (!useConfig)
            // generate from data
            amdInput.addKernel(innerBin.getKernelName().c_str(), codeSize, code, calNotes,
                    amdGpuBin->getKernelHeader(i), amdGpuBin->getMetadataSize(i),
                    amdGpuBin->getMetadata(i), dataSize, data/*0, nullptr*/);
        else
        {
            // generate from config
            AmdKernelConfig config = getAmdKernelConfig(amdGpuBin->getMetadataSize(i), 
                    amdGpuBin->getMetadata(i), innerBin, amdGpuBin->getDriverInfo(),
                    amdGpuBin->getKernelHeader(i));
            amdInput.addKernel(innerBin.getKernelName().c_str(), codeSize, code, config,
                    dataSize, data/*0, nullptr*/);
        }
    }
    return amdInput;
}

static void testOrigBinary(cxuint testCase, const char* origBinaryFilename, bool reconf)
{
    Array<cxbyte> inputData;
    std::unique_ptr<AmdMainBinaryBase> base;
    Array<cxbyte> output;
    AmdInput amdInput;
    
    inputData = loadDataFromFile(origBinaryFilename);
    // load binary file
    const Flags binFlags = AMDBIN_CREATE_KERNELINFO | AMDBIN_CREATE_KERNELINFOMAP |
                AMDBIN_CREATE_INNERBINMAP | AMDBIN_CREATE_KERNELHEADERS |
                AMDBIN_CREATE_KERNELHEADERMAP | AMDBIN_INNER_CREATE_CALNOTES |
                ELF_CREATE_SECTIONMAP |
                AMDBIN_CREATE_INFOSTRINGS;
    base.reset(createAmdBinaryFromCode(inputData.size(), inputData.data(), binFlags));
    
    if (base->getType() == AmdMainType::GPU_BINARY)
    {
        // generate input from 32-bit binary
        AmdMainGPUBinary32* amdGpuBin = static_cast<AmdMainGPUBinary32*>(base.get());
        amdInput = genAmdInput(reconf, amdGpuBin, false, false);
        amdInput.is64Bit = false;
    }
    else if (base->getType() == AmdMainType::GPU_64_BINARY)
    {
        // generate input from 64-bit binary
        AmdMainGPUBinary64* amdGpuBin = static_cast<AmdMainGPUBinary64*>(base.get());
        amdInput = genAmdInput(reconf, amdGpuBin, false, false);
        amdInput.is64Bit = true;
    }
    else
        throw Exception("This is not AMDGPU binary file!");
    
    AmdGPUBinGenerator binGen(&amdInput);
    binGen.generate(output);
    
    // compare result output binary
    if (output.size() != inputData.size())
    {
        std::ostringstream oss;
        oss << "Failed for #" << testCase << " file=" << origBinaryFilename <<
                ": expectedSize=" << inputData.size() <<
                ", resultSize=" << output.size();
        throw Exception(oss.str());
    }
    for (size_t i = 0; i < inputData.size(); i++)
        if (output[i] != inputData[i])
        {
            std::ostringstream oss;
            oss << "Failed for #" << testCase << " file=" << origBinaryFilename <<
                    ": byte=" << i;
            throw Exception(oss.str());
        }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(origBinaryFiles)/sizeof(const char*); i++)
    {
        std::string regenName = origBinaryFiles[i];
        filesystemPath(regenName); // convert to system path (native separators)
        regenName += ".regen";
        std::string reconfName = origBinaryFiles[i];
        filesystemPath(reconfName); // convert to system path (native separators)
        reconfName += ".reconf";
        try
        { testOrigBinary(i, regenName.c_str(), false); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
        try
        { testOrigBinary(i, reconfName.c_str(), true); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    }
    return retVal;
}
