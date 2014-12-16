/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
/*! \file AmdBinGen.h
 * \brief AMD binaries generator
 */

#ifndef __CLRX_AMDBINGEN_H__
#define __CLRX_AMDBINGEN_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <CLRX/AmdBinaries.h>

/// main namespace
namespace CLRX
{

enum class GPUDeviceType: cxbyte
{
    UNDEFINED = 0,
    CAPE_VERDE, ///< Radeon HD7700
    PITCAIRN, ///< Radeon HD7800
    TAHITI, ///< Radeon HD7900
    OLAND, ///< Radeon R7 250
    BONAIRE, ///< Radeon R7 260
    SPECTRE, ///< Kaveri
    SPOOKY, ///< Kaveri
    KALINDI, ///< ???  GCN1.1
    HAINAN, ///< ????  GCN1.0
    HAWAII, ///< Radeon R9 290
    ICELAND, ///<
    TONGA, ///<
    MULLINS, //
    GPUDEVICE_MAX = MULLINS,
    
    RADEON_HD7700 = CAPE_VERDE,
    RADEON_HD7800 = PITCAIRN,
    RADEON_HD7900 = TAHITI,
    RADEON_R7_250 = OLAND,
    RADEON_R7_260 = BONAIRE,
    RADEON_R9_290 = HAWAII
};

enum: cxuint {
    AMDBIN_DEFAULT = 0xfffffffU,    ///< if set in field then field has been filled later
    AMDBIN_NOTSUPPLIED  = 0xffffffeU ///< if set in field then field has been ignored
};

struct AmdKernelArg
{
    std::string argName;    ///< argument name
    std::string typeName;   ///< name of type of argument
    KernelArgType argType;  ///< argument type
    KernelArgType pointerType;  ///< pointer type
    KernelPtrSpace ptrSpace;///< pointer space for argument if argument is pointer or image
    uint8_t ptrAccess;  ///< pointer access flags
    cxuint structSize; ///< structure size (if structure)
    size_t constSpaceSize;
    bool used; ///< used by kernel
};

struct AmdUserData
{
    uint32_t dataClass;
    uint32_t apiSlot;
    uint32_t regStart;
    uint32_t regSize;
};

struct PgmRsrc2
{
    cxuint isScratch : 1;
    cxuint userSGRP : 5;
    cxuint trapPresent : 1;
    cxuint isTgidX : 1;
    cxuint isTgidY : 1;
    cxuint isTgidZ : 1;
    cxuint tgSize : 1;
    cxuint tidigCompCnt : 2;
    cxuint excpEnMsb : 2;
    cxuint ldsSize : 9;
    cxuint excpEn : 7;
    cxuint : 1;
};
    
struct AmdKernelConfig
{
    std::vector<AmdKernelArg> args;
    std::vector<cxuint> samplers;
    uint32_t reqdWorkGroupSize[3];
    uint32_t usedVGPRsNum;
    uint32_t usedSGPRsNum;
    union {
        PgmRsrc2 pgmRSRC2;
        uint32_t pgmRSRC2Value;
    };
    uint32_t ieeeMode;
    uint32_t floatMode;
    size_t hwLocalSize;
    uint32_t hwRegion;
    uint32_t scratchBufferSize;
    uint32_t uavPrivate;
    uint32_t uavId;
    uint32_t constBufferId;
    uint32_t printfId;
    uint32_t privateId;
    uint32_t earlyExit;
    uint32_t condOut;
    bool constDataRequired;
    cxuint userDataElemsNum;
    AmdUserData userDatas[16];
};

struct CALNoteInput
{
    CALNoteHeader header;  ///< header of CAL note
    const cxbyte* data;   ///< data of CAL note
};


struct AmdKernelInput
{
    std::string kernelName; ///< kernel name
    size_t dataSize;
    const cxbyte* data;
    size_t headerSize;  ///< kernel header size
    const cxbyte* header;   ///< kernel header size
    size_t metadataSize;    ///< metadata size
    const char* metadata;   ///< kernel's metadata
    std::vector<CALNoteInput> calNotes;
    bool useConfig;
    AmdKernelConfig config;
    size_t codeSize;
    const cxbyte* code;
};

struct AmdInput
{
    bool is64Bit;
    GPUDeviceType deviceType;
    size_t globalDataSize;
    const cxbyte* globalData;
    uint32_t driverVersion;
    std::string compileOptions;
    std::string driverInfo;
    std::vector<AmdKernelInput> kernels;
    
    void addKernel(const AmdKernelInput& kernelInput);
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const AmdKernelConfig& config, size_t dataSize = 0,
           const cxbyte* data = nullptr);
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const std::vector<CALNoteInput>& calNotes, const cxbyte* header,
           size_t metadataSize, const char* metadata,
           size_t dataSize = 0, const cxbyte* data = nullptr);
};

class AmdGPUBinGenerator
{
private:
    bool manageable;
    const AmdInput* input;
    
    size_t binarySize;
    cxbyte* binary;
public:
    AmdGPUBinGenerator();
    AmdGPUBinGenerator(const AmdInput* amdInput);
    AmdGPUBinGenerator(bool _64bitMode, GPUDeviceType deviceType, uint32_t driverVersion,
           size_t globalDataSize, const cxbyte* globalData, 
           const std::vector<AmdKernelInput>& kernelInputs);
    ~AmdGPUBinGenerator();
    
    const AmdInput* getInput() const
    { return input; }
    
    // non-copyable and non-movable
    AmdGPUBinGenerator(const AmdGPUBinGenerator& c) = delete;
    AmdGPUBinGenerator& operator=(const AmdGPUBinGenerator& c) = delete;
    AmdGPUBinGenerator(AmdGPUBinGenerator&& c) = delete;
    AmdGPUBinGenerator& operator=(AmdGPUBinGenerator&& c) = delete;
        
    void generate();
    
    size_t getBinarySize() const
    { return binarySize; }
    const cxbyte* getBinary() const
    { return binary; }
};

};

#endif
