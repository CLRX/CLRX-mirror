/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
#include <climits>
#include <cstdint>
#include <string>
#include <vector>
#include <CLRX/amdbin/AmdBinaries.h>

/// main namespace
namespace CLRX
{

/// type of GPU device
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
    AMDBIN_DEFAULT = UINT_MAX,    ///< if set in field then field has been filled later
    AMDBIN_NOTSUPPLIED  = UINT_MAX-1 ///< if set in field then field has been ignored
};

/// AMD OpenCL kernel argument description
struct AmdKernelArgInput
{
    std::string argName;    ///< argument name
    std::string typeName;   ///< name of type of argument
    KernelArgType argType;  ///< argument type
    KernelArgType pointerType;  ///< pointer type
    KernelPtrSpace ptrSpace;///< pointer space for argument if argument is pointer or image
    uint8_t ptrAccess;  ///< pointer access flags
    cxuint structSize; ///< structure size (if structure)
    size_t constSpaceSize;
    uint32_t resId; ///< uavid or cbid or counterId
    bool used; ///< used by kernel
    
    /// create simple type argument
    static AmdKernelArgInput arg(const std::string& argName, const std::string& typeName,
         KernelArgType argType)
    {
        return { argName, typeName, argType, KernelArgType::VOID, KernelPtrSpace::NONE,
            KARG_PTR_NORMAL, 0, 0, 0, true };
    }
    /// create global pointer
    static AmdKernelArgInput gptr(const std::string& argName, const std::string& typeName,
         KernelArgType ptrType, cxuint structSize = 0, uint8_t ptrAccess = KARG_PTR_NORMAL,
         uint32_t resId = AMDBIN_DEFAULT, bool used = true)
    {
        return { argName, typeName, KernelArgType::POINTER, ptrType,
            KernelPtrSpace::GLOBAL, ptrAccess, structSize, 0, resId, used };
    }
    /// create constant pointer
    static AmdKernelArgInput cptr(const std::string& argName, const std::string& typeName,
         KernelArgType ptrType, cxuint structSize = 0, uint8_t ptrAccess = KARG_PTR_NORMAL,
         size_t constSpaceSize = 0, uint32_t resId = AMDBIN_DEFAULT, bool used = true)
    {
        return { argName, typeName, KernelArgType::POINTER, ptrType,
            KernelPtrSpace::CONSTANT, ptrAccess, structSize, constSpaceSize, resId, used };
    }
    /// create local pointer
    static AmdKernelArgInput lptr(const std::string& argName, const std::string& typeName,
         KernelArgType ptrType, cxuint structSize = 0)
    {
        return { argName, typeName, KernelArgType::POINTER, ptrType,
            KernelPtrSpace::LOCAL, KARG_PTR_NORMAL, structSize, 0, 1, true };
    }
    
    /// create image
    static AmdKernelArgInput img(const std::string& argName, const std::string& typeName,
        KernelArgType imgType, uint8_t ptrAccess = KARG_PTR_READ_ONLY,
        uint32_t resId = AMDBIN_DEFAULT, bool used = true)
    {
        return { argName, typeName, imgType, KernelArgType::VOID, KernelPtrSpace::NONE,
            ptrAccess, 0, 0, resId, used };
    }
};

/// user data for in CAL PROGINFO
struct AmdUserData
{
    uint32_t dataClass; ///< type of data
    uint32_t apiSlot;   ///< slot
    uint32_t regStart;  ///< number of beginning SGPR register
    uint32_t regSize;   ///< number of used SGPRS registers
};

/// kernel configuration
struct AmdKernelConfig
{
    std::vector<AmdKernelArgInput> args; ///< arguments
    std::vector<cxuint> samplers;   ///< defined samplers
    uint32_t reqdWorkGroupSize[3];  /// reqd_work_group_size
    uint32_t usedVGPRsNum;  ///< number of used VGPRs
    uint32_t usedSGPRsNum;  ///< number of used SGPRs
    uint32_t pgmRSRC2;
    uint32_t ieeeMode;
    uint32_t floatMode;
    size_t hwLocalSize; ///< used local size (not local defined in kernel arguments)
    uint32_t hwRegion;
    uint32_t scratchBufferSize; ///< size of scratch buffer
    uint32_t uavPrivate;    ///< uav private size
    uint32_t uavId; ///< uavid, first uavid for kernel argument minus 1
    uint32_t constBufferId;
    uint32_t printfId;  ///< UAV ID for printf
    uint32_t privateId;
    uint32_t earlyExit; ///< CALNOTE_EARLYEXIT value
    uint32_t condOut;   ///< CALNOTE_CONDOUT value
    bool usePrintf;     // if kernel uses printf function
    bool useConstantData; ///< if const data required
    cxuint userDataElemsNum;    ///< number of user data
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
    size_t dataSize;    ///< data size
    const cxbyte* data; /// data
    size_t headerSize;  ///< kernel header size (used if useConfig=false)
    const cxbyte* header;   ///< kernel header size (used if useConfig=false)
    size_t metadataSize;    ///< metadata size (used if useConfig=false)
    const char* metadata;   ///< kernel's metadata (used if useConfig=false)
    std::vector<CALNoteInput> calNotes; ///< CAL Note array (used if useConfig=false)
    bool useConfig;         ///< true if configuration has been used to generate binary
    AmdKernelConfig config; ///< kernel's configuration
    size_t codeSize;        ///< code size
    const cxbyte* code;     ///< code
};

/// main Input for AmdGPUBinGenerator
struct AmdInput
{
    bool is64Bit;   ///< is 64-bit binary
    GPUDeviceType deviceType;   ///< GPU device type
    size_t globalDataSize;  ///< global constant data size
    const cxbyte* globalData;   ///< global constant data
    uint32_t driverVersion;     ///< driver version (majorVersion*100 + minorVersion)
    std::string compileOptions; ///< compile options
    std::string driverInfo;     ///< driver info
    std::vector<AmdKernelInput> kernels;    ///< kernels
    size_t sourceCodeSize;  ///< source code size (can be zero)
    const char* sourceCode; ///< string of source (if size==0 then must be null-terminated)
    size_t llvmirSize;
    const cxbyte* llvmir;
    
    /// add kernel to input
    void addKernel(const AmdKernelInput& kernelInput);
    /// add kernel to input
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const AmdKernelConfig& config, size_t dataSize = 0,
           const cxbyte* data = nullptr);
    /// add kernel to input
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const std::vector<CALNoteInput>& calNotes, const cxbyte* header,
           size_t metadataSize, const char* metadata,
           size_t dataSize = 0, const cxbyte* data = nullptr);
};

/// main AMD GPU Binary generator
class AmdGPUBinGenerator: public NonCopyableAndNonMovable
{
private:
    bool manageable;
    const AmdInput* input;
public:
    AmdGPUBinGenerator();
    AmdGPUBinGenerator(const AmdInput* amdInput);
    /// constructor
    /**
     * \param _64bitMode true if binary will be 64-bit
     * \param deviceType GPU device type
     * \param driverVersion number of driver version (majorVersion*100 + minorVersion)
     * \param globalDataSize size of constant global data
     * \param globalData global constant data
     * \param kernelInputs array of kernel inputs
     */
    AmdGPUBinGenerator(bool _64bitMode, GPUDeviceType deviceType, uint32_t driverVersion,
           size_t globalDataSize, const cxbyte* globalData, 
           const std::vector<AmdKernelInput>& kernelInputs);
    ~AmdGPUBinGenerator();
    
    /// get input
    const AmdInput* getInput() const
    { return input; }
    
    /// set input
    void setInput(const AmdInput* input);
    
    /// generates binary
    /**
     * \param binarySize reference to binary size variable
     * \return binary content pointer
     */
    cxbyte* generate(size_t& binarySize) const;
};

};

#endif
