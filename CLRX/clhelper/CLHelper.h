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
/*! \file CLHelper.h
 * \brief CL helper for creating binaries
 */

#ifndef __CLRX_CLHELPER_H__
#define __CLRX_CLHELPER_H__

#include <CLRX/Config.h>
#include <exception>
#include <vector>
#include <utility>
#ifdef __APPLE__
#  include <OpenCL/cl.h>
#else
#  include <CL/cl.h>
#endif
#include <CLRX/amdasm/Commons.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/utils/CString.h>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/GPUId.h>


namespace CLRX
{

/// error class based on std::exception
class CLError: public Exception
{
private:
    cl_int error;
public:
    //// empty constructor
    CLError();
    
    /// constructor with description
    explicit CLError(const char* _description);
    
    /// constructor with description and OpenCL error
    CLError(cl_int _error, const char* _description);
    
    /// destructor
    virtual ~CLError() noexcept;
    
    /// get what
    const char* what() const noexcept;
    
    /// get OpenCL error code
    int code() const
    { return error; }
};
    
/// choose suitable OpenCL platform for CLRX assembler programs
extern cl_platform_id chooseCLPlatformForCLRX();

/// choose suitable OpenCL many platforms for CLRX assembler programs
extern std::vector<cl_platform_id> chooseCLPlatformsForCLRX();

enum : Flags
{
    CLHELPER_USEAMDCL2 = 1,  ///< force using AMD OpenCL 2.0 binary format
    CLHELPER_USEAMDLEGACY = 2,  ///< force using AMD OpenCL 1.2 binary format
};

/// structure that holds assembler setup for OpenCL programs
struct CLAsmSetup
{
    GPUDeviceType deviceType;   ///< device typ
    BinaryFormat binaryFormat;  ///< binary format
    cxuint driverVersion;   ///< driver version
    cxuint llvmVersion;     ///< LLVM version
    bool is64Bit;        ///< if binary is 64-bit
    CString options; ///< OpenCL base options
    Flags asmFlags;     ///< assembler flags
    bool newROCmBinFormat; ///< new ROCm binary format
};

/// get assembler setup(compile assembler code) binary for OpenCL device
extern CLAsmSetup assemblerSetupForCLDevice(cl_device_id clDevice, Flags flags = 0,
            Flags asmFlags = 0);

/// create program binary for OpenCL
extern Array<cxbyte> createBinaryForOpenCL(const CLAsmSetup& asmSetup,
                const char* sourceCode, size_t sourceCodeLen = 0);

/// create program binary for OpenCL
extern Array<cxbyte> createBinaryForOpenCL(const CLAsmSetup& asmSetup,
                const std::vector<std::pair<CString, uint64_t> >& defSymbols,
                const char* sourceCode, size_t sourceCodeLen = 0);

/// create (build program) binary for OpenCL device
extern cl_program createProgramForCLDevice(cl_context clContext, cl_device_id clDevice,
            const CLAsmSetup& asmSetup, const Array<cxbyte>& binary,
            CString* buildLog = nullptr);

/// create (compile assembler code and build program) binary for OpenCL device
extern cl_program createProgramForCLDevice(cl_context clContext, cl_device_id clDevice,
            const CLAsmSetup& asmSetup, const char* sourceCode, size_t sourceCodeLen = 0,
            CString* buildLog = nullptr);

/// create (compile assembler code and build program) binary for OpenCL device
extern cl_program createProgramForCLDevice(cl_context clContext, cl_device_id clDevice,
            const CLAsmSetup& asmSetup,
            const std::vector<std::pair<CString, uint64_t> >& defSymbols,
            const char* sourceCode, size_t sourceCodeLen = 0,
            CString* buildLog = nullptr);

};

#endif
