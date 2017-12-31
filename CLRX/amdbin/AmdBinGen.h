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
/*! \file AmdBinGen.h
 * \brief AMD binaries generator
 */

#ifndef __CLRX_AMDBINGEN_H__
#define __CLRX_AMDBINGEN_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <ostream>
#include <vector>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/InputOutput.h>

/// main namespace
namespace CLRX
{

/// AMD OpenCL kernel argument description
struct AmdKernelArgInput
{
    CString argName;    ///< argument name
    CString typeName;   ///< name of type of argument
    KernelArgType argType;  ///< argument type
    KernelArgType pointerType;  ///< pointer type
    KernelPtrSpace ptrSpace;///< pointer space for argument if argument is pointer or image
    cxbyte ptrAccess;  ///< pointer access flags
    cxuint structSize; ///< structure size (if structure)
    size_t constSpaceSize; ///< constant space size
    uint32_t resId; ///< uavid or cbid or counterId
    cxbyte used; ///< flags that indicate how kernel uses this argument (1 - used)
    
    /// create simple type argument
    static AmdKernelArgInput arg(const CString& argName, const CString& typeName,
         KernelArgType argType)
    {
        return { argName, typeName, argType, KernelArgType::VOID, KernelPtrSpace::NONE,
            KARG_PTR_NORMAL, 0, 0, 0, true };
    }
    /// create global pointer
    static AmdKernelArgInput gptr(const CString& argName, const CString& typeName,
         KernelArgType ptrType, cxuint structSize = 0, cxbyte ptrAccess = KARG_PTR_NORMAL,
         uint32_t resId = BINGEN_DEFAULT, bool used = true)
    {
        return { argName, typeName, KernelArgType::POINTER, ptrType,
            KernelPtrSpace::GLOBAL, ptrAccess, structSize, 0, resId, used };
    }
    /// create constant pointer
    static AmdKernelArgInput cptr(const CString& argName, const CString& typeName,
         KernelArgType ptrType, cxuint structSize = 0, cxbyte ptrAccess = KARG_PTR_NORMAL,
         size_t constSpaceSize = 0, uint32_t resId = BINGEN_DEFAULT, bool used = true)
    {
        return { argName, typeName, KernelArgType::POINTER, ptrType,
            KernelPtrSpace::CONSTANT, ptrAccess, structSize, constSpaceSize, resId, used };
    }
    /// create local pointer
    static AmdKernelArgInput lptr(const CString& argName, const CString& typeName,
         KernelArgType ptrType, cxuint structSize = 0)
    {
        return { argName, typeName, KernelArgType::POINTER, ptrType,
            KernelPtrSpace::LOCAL, KARG_PTR_NORMAL, structSize, 0, 1, true };
    }
    
    /// create image
    static AmdKernelArgInput img(const CString& argName, const CString& typeName,
        KernelArgType imgType, cxbyte ptrAccess = KARG_PTR_READ_ONLY,
        uint32_t resId = BINGEN_DEFAULT, bool used = true)
    {
        return { argName, typeName, imgType, KernelArgType::VOID, KernelPtrSpace::GLOBAL,
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
    uint32_t dimMask;    ///< mask of dimension (bits: 0 - X, 1 - Y, 2 - Z)
    uint32_t reqdWorkGroupSize[3];  ///< reqd_work_group_size
    uint32_t usedVGPRsNum;  ///< number of used VGPRs
    uint32_t usedSGPRsNum;  ///< number of used SGPRs
    uint32_t pgmRSRC2;      ///< pgmRSRC2 register value
    uint32_t floatMode; ///< float mode
    size_t hwLocalSize; ///< used local size (not local defined in kernel arguments)
    uint32_t hwRegion;  ///< hwRegion ????
    uint32_t scratchBufferSize; ///< size of scratch buffer
    uint32_t uavPrivate;    ///< uav private size
    uint32_t uavId; ///< uavid, first uavid for kernel argument minus 1
    uint32_t constBufferId; ///< constant buffer id
    uint32_t printfId;  ///< UAV ID for printf
    uint32_t privateId; ///< private id (???)
    uint32_t earlyExit; ///< CALNOTE_EARLYEXIT value
    uint32_t condOut;   ///< CALNOTE_CONDOUT value
    bool ieeeMode;  ///< IEEE mode
    cxbyte exceptions;  ///< enabled exception handling
    bool tgSize;    ///< enable tgSize
    bool usePrintf;     ///< if kernel uses printf function
    bool useConstantData; ///< if const data required
    std::vector<AmdUserData> userDatas;  ///< user datas
};

/// AMD kernel input
struct AmdKernelInput
{
    CString kernelName; ///< kernel name
    size_t dataSize;    ///< data size
    const cxbyte* data; ///< data
    size_t headerSize;  ///< kernel header size (used if useConfig=false)
    const cxbyte* header;   ///< kernel header size (used if useConfig=false)
    size_t metadataSize;    ///< metadata size (used if useConfig=false)
    const char* metadata;   ///< kernel's metadata (used if useConfig=false)
    std::vector<CALNoteInput> calNotes; ///< CAL Note array (used if useConfig=false)
    bool useConfig;         ///< true if configuration has been used to generate binary
    AmdKernelConfig config; ///< kernel's configuration
    size_t codeSize;        ///< code size
    const cxbyte* code;     ///< code
    std::vector<BinSection> extraSections;      ///< list of extra sections
    std::vector<BinSymbol> extraSymbols;        ///< list of extra symbols
};

/// main Input for AmdGPUBinGenerator
struct AmdInput
{
    bool is64Bit;   ///< is 64-bit binary
    GPUDeviceType deviceType;   ///< GPU device type
    size_t globalDataSize;  ///< global constant data size
    const cxbyte* globalData;   ///< global constant data
    uint32_t driverVersion;     ///< driver version (majorVersion*100 + minorVersion)
    CString compileOptions; ///< compile options
    CString driverInfo;     ///< driver info
    std::vector<AmdKernelInput> kernels;    ///< kernels
    std::vector<BinSection> extraSections;  ///< extra sections
    std::vector<BinSymbol> extraSymbols;    ///< extra symbols
    
    /// add kernel to input
    void addKernel(const AmdKernelInput& kernelInput);
    /// add kernel to input
    void addKernel(AmdKernelInput&& kernelInput);
    
    /// add kernel to input with configuration
    /**
     * \param kernelName kernel name
     * \param codeSize size of kernel code size
     * \param code kernel code
     * \param config kernel configuration
     * \param dataSize size of data in .data section
     * \param data content of .data section
     */
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const AmdKernelConfig& config, size_t dataSize = 0,
           const cxbyte* data = nullptr);
    /// add kernel to input
    /**
     * \param kernelName kernel name
     * \param codeSize size of kernel code size
     * \param code kernel code
     * \param calNotes list of ATI CAL notes
     * \param header kernel header (32-bytes)
     * \param metadataSize size of metadata text in bytes
     * \param metadata metadata text (not null-terminated)
     * \param dataSize size of data in .data section
     * \param data content of .data section
     */
    void addKernel(const char* kernelName, size_t codeSize, const cxbyte* code,
           const std::vector<CALNoteInput>& calNotes, const cxbyte* header,
           size_t metadataSize, const char* metadata,
           size_t dataSize = 0, const cxbyte* data = nullptr);

    /// add empty kernel with default values (even for configuration)
    void addEmptyKernel(const char* kernelName);
};

/// main AMD GPU Binary generator
class AmdGPUBinGenerator: public NonCopyableAndNonMovable
{
private:
    bool manageable;
    const AmdInput* input;
    
    void generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const;
public:
    AmdGPUBinGenerator();
    
    /// constructor from amdInput
    explicit AmdGPUBinGenerator(const AmdInput* amdInput);
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
    /// constructor
    AmdGPUBinGenerator(bool _64bitMode, GPUDeviceType deviceType, uint32_t driverVersion,
           size_t globalDataSize, const cxbyte* globalData,
           std::vector<AmdKernelInput>&& kernelInputs);
    ~AmdGPUBinGenerator();
    
    /// get input
    const AmdInput* getInput() const
    { return input; }
    
    /// set input
    void setInput(const AmdInput* input);
    
    /// generates binary
    void generate(Array<cxbyte>& array) const;
    
    /// generates binary to output stream
    void generate(std::ostream& os) const;
    
    /// generates binary to vector
    void generate(std::vector<char>& vector) const;
};

/// detect driver version in the system
extern uint32_t detectAmdDriverVersion();

};

#endif
