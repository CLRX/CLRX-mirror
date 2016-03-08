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
/*! \file AmdCL2BinGen.h
 * \brief AMD OpenCL2 binaries generator
 */

#ifndef __CLRX_AMDCL2BINGEN_H__
#define __CLRX_AMDCL2BINGEN_H__

#include <CLRX/Config.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <ostream>
#include <vector>
#include <CLRX/amdbin/AmdBinGen.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/utils/InputOutput.h>

namespace CLRX
{

/// kernel configuration
struct AmdCL2KernelConfig
{
    std::vector<AmdKernelArgInput> args; ///< arguments
    std::vector<cxuint> samplers;   ///< defined samplers
    uint32_t dimMask;    ///< mask of dimension (bits: 0 - X, 1 - Y, 2 - Z)
    uint32_t reqdWorkGroupSize[3];  ///< reqd_work_group_size
    uint32_t usedVGPRsNum;  ///< number of used VGPRs
    uint32_t usedSGPRsNum;  ///< number of used SGPRs
    uint32_t pgmRSRC2;      ///< pgmRSRC2 register value
    uint32_t ieeeMode;  ///< IEEE mode
    uint32_t floatMode; ///< float mode
    size_t localSize; ///< used local size (not local defined in kernel arguments)
    uint32_t scratchBufferSize; ///< size of scratch buffer
    bool tgSize;    ///< enable tgSize
    bool useConstantData; ///< if const data required
};

/// AMD CL2 Relocation entry input
struct AmdCL2RelInput
{
    size_t offset;
    size_t addend;
};

/// AMD kernel input
struct AmdCL2KernelInput
{
    CString kernelName; ///< kernel name
    size_t stubSize;  ///< kernel stub size (used if useConfig=false)
    const cxbyte* stub;   ///< kernel stub size (used if useConfig=false)
    size_t setupSize;  ///< kernel setup size (used if useConfig=false)
    const cxbyte* setup;   ///< kernel setup size (used if useConfig=false)
    size_t metadataSize;    ///< metadata size (used if useConfig=false)
    const cxbyte* metadata;   ///< kernel's metadata (used if useConfig=false)
    size_t isaMetadataSize;    ///< metadata size (used if useConfig=false)
    const cxbyte* isaMetadata;   ///< kernel's metadata (used if useConfig=false)
    bool useConfig;         ///< true if configuration has been used to generate binary
    AmdCL2KernelConfig config; ///< kernel's configuration
    std::vector<AmdCL2RelInput> relocations;    ///< relocation to kernel code
    size_t codeSize;        ///< code size
    const cxbyte* code;     ///< code
    
};

/// main Input for AmdCL2GPUBinGenerator
struct AmdCL2Input
{
    GPUDeviceType deviceType;   ///< GPU device type
    size_t globalDataSize;  ///< global constant data size
    const cxbyte* globalData;   ///< global constant data
    size_t samplerInitSize;  ///< sampler init size
    const cxbyte* samplerInit; ///< sampler init data
    bool samplerConfig;
    std::vector<cxuint> samplers;   ///< sampler config
    std::vector<size_t> samplerOffsets; ///< sampler offsets
    uint32_t driverVersion;     ///< driver version (majorVersion*100 + minorVersion)
    CString compileOptions; ///< compile options
    CString aclVersion;     ///< acl version string
    std::vector<AmdCL2KernelInput> kernels;    ///< kernels
    std::vector<BinSection> extraSections;  ///< extra sections
    std::vector<BinSymbol> extraSymbols;    ///< extra symbols
    std::vector<BinSection> innerExtraSections;      ///< list of extra sections
    std::vector<BinSymbol> innerExtraSymbols;        ///< list of extra symbols
};

/// main AMD OpenCL2.0 GPU Binary generator
class AmdCL2GPUBinGenerator: public NonCopyableAndNonMovable
{
private:
    bool manageable;
    const AmdCL2Input* input;
    
    void generateInternal(std::ostream* osPtr, std::vector<char>* vPtr,
             Array<cxbyte>* aPtr) const;
public:
    AmdCL2GPUBinGenerator();
    
    /// constructor from amdInput
    AmdCL2GPUBinGenerator(const AmdCL2Input* amdInput);
    /// constructor
    /**
     * \param deviceType GPU device type
     * \param driverVersion number of driver version (majorVersion*100 + minorVersion)
     * \param globalDataSize size of constant global data
     * \param globalData global constant data
     * \param kernelInputs array of kernel inputs
     */
    AmdCL2GPUBinGenerator(GPUDeviceType deviceType, uint32_t driverVersion,
           size_t globalDataSize, const cxbyte* globalData, 
           const std::vector<AmdCL2KernelInput>& kernelInputs);
    /// constructor
    AmdCL2GPUBinGenerator(GPUDeviceType deviceType, uint32_t driverVersion,
           size_t globalDataSize, const cxbyte* globalData, 
           std::vector<AmdCL2KernelInput>&& kernelInputs);
    ~AmdCL2GPUBinGenerator();
    
    /// get input
    const AmdCL2Input* getInput() const
    { return input; }
    
    /// set input
    void setInput(const AmdCL2Input* input);
    
    /// generates binary
    void generate(Array<cxbyte>& array) const;
    
    /// generates binary to output stream
    void generate(std::ostream& os) const;
    
    /// generates binary to vector
    void generate(std::vector<char>& vector) const;
};

};

#endif
