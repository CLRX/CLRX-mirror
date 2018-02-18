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

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS

#include <CLRX/Config.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <memory>
#include <utility>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/clhelper/CLHelper.h>

using namespace CLRX;

CLError::CLError() : error(0)
{ }

CLError::CLError(const char* _description) : Exception(_description), error(0)
{ }

CLError::CLError(cl_int _error, const char* _description): error(_error)
{
    // construct message
    char buf[20];
    ::snprintf(buf, 20, "%d", _error);
    message = "CLError code: ";
    message += buf;
    message += ", Desc: ";
    message += _description;
}

CLError::~CLError() noexcept
{ }

const char* CLError::what() const noexcept
{
    // print no error if no error
    return (!message.empty()) ? message.c_str() : "No error!";
}

// strip CString (remove spaces from first and last characters
static char* stripCString(char* str)
{
    while (*str==' ') str++;
    char* last = str+::strlen(str);
    while (last!=str && (*last==0||*last==' '))
        last--;
    if (*last!=0) last[1] = 0;
    return str;
}

/* main routines */

static std::vector<cl_platform_id> chooseCLPlatformsForCLRXInt(bool single = true)
{
    cl_int error = CL_SUCCESS;
    cl_uint platformsNum;
    std::unique_ptr<cl_platform_id[]> platforms;
    error = clGetPlatformIDs(0, nullptr, &platformsNum);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformIDs");
    platforms.reset(new cl_platform_id[platformsNum]);
    error = clGetPlatformIDs(platformsNum, platforms.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformIDs");
    
    std::vector<cl_platform_id> choosenPlatforms;
    /// find platform with AMD or GalliumCompute devices
    for (cl_uint i = 0; i < platformsNum; i++)
    {
        size_t platformNameSize;
        std::unique_ptr<char[]> platformName;
        error = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, nullptr,
                           &platformNameSize);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetPlatformInfo");
        platformName.reset(new char[platformNameSize]);
        error = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, platformNameSize,
                   platformName.get(), nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetPlatformInfo");
        
        // find correct platform (supported GalliumCompute and AMD APP)
        const char* splatformName = stripCString(platformName.get());
        if (::strcmp(splatformName, "AMD Accelerated Parallel Processing")==0 ||
            ::strcmp(splatformName, "Clover")==0)
        {
            choosenPlatforms.push_back(platforms[i]);
            if (single)
                break;
        }
    }
    
    if (choosenPlatforms.empty())
        throw Exception("PlatformNotFound");
    
    return choosenPlatforms;
}

cl_platform_id CLRX::chooseCLPlatformForCLRX()
{
    return chooseCLPlatformsForCLRXInt()[0];
}

std::vector<cl_platform_id> CLRX::chooseCLPlatformsForCLRX()
{
    return chooseCLPlatformsForCLRXInt(false);
}

CLAsmSetup CLRX::assemblerSetupForCLDevice(cl_device_id clDevice, Flags flags,
            Flags asmFlags)
{
    cl_int error = CL_SUCCESS;
    cl_platform_id platform;
    BinaryFormat binaryFormat = BinaryFormat::GALLIUM;
    cxuint amdappVersion = 0;
    const cxuint useCL = (flags & CLHELPER_USEAMDCL2) ? 2 :
            (flags & CLHELPER_USEAMDLEGACY) ? 1 : 0;
    bool defaultCL2ForDriver = false;
    
    // get platform from device
    error = clGetDeviceInfo(clDevice, CL_DEVICE_PLATFORM, sizeof(cl_platform_id),
                &platform, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceInfoPlatform");
    
    // get platform name to check whether is suitable platform
    size_t platformNameSize;
    std::unique_ptr<char[]> platformName;
    error = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, nullptr,
                        &platformNameSize);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformInfo");
    platformName.reset(new char[platformNameSize]);
    error = clGetPlatformInfo(platform, CL_PLATFORM_NAME, platformNameSize,
                platformName.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformInfo");
    
    const char* splatformName = stripCString(platformName.get());
    if (::strcmp(splatformName, "AMD Accelerated Parallel Processing")==0 ||
        ::strcmp(splatformName, "Clover")==0)
    {
        cl_platform_id choosenPlatform = platform;
        binaryFormat = ::strcmp(platformName.get(), "Clover")==0 ?
                BinaryFormat::GALLIUM : BinaryFormat::AMD;
        
        if (binaryFormat == BinaryFormat::AMD)
        {
            // get amdappVersion
            size_t platformVersionSize;
            std::unique_ptr<char[]> platformVersion;
            error = clGetPlatformInfo(choosenPlatform, CL_PLATFORM_VERSION, 0, nullptr,
                                    &platformVersionSize);
            if (error != CL_SUCCESS)
                throw CLError(error, "clGetPlatformInfoVersion");
            platformVersion.reset(new char[platformVersionSize]);
            error = clGetPlatformInfo(choosenPlatform, CL_PLATFORM_VERSION,
                            platformVersionSize, platformVersion.get(), nullptr);
            if (error != CL_SUCCESS)
                throw CLError(error, "clGetPlatformInfoVersion");
            
            const char* amdappPart = strstr(platformVersion.get(), " (");
            if (amdappPart!=nullptr)
            {
                // parse AMDAPP version
                try
                {
                    const char* majorVerPart = amdappPart+2;
                    const char* minorVerPart;
                    const char* end;
                    cxuint majorVersion = cstrtoui(majorVerPart, nullptr,
                                    minorVerPart);
                    
                    if (*minorVerPart!=0)
                    {
                        minorVerPart++; // skip '.'
                        cxuint minorVersion = cstrtoui(minorVerPart, nullptr, end);
                        amdappVersion = majorVersion*100U + minorVersion;
                    }
                }
                catch(const ParseException& ex)
                { } // ignore error
            }
        }
        
        if (binaryFormat == BinaryFormat::AMD && useCL==2)
            binaryFormat = BinaryFormat::AMDCL2;
        // for driver 2004.6 OpenCL 2.0 binary format is default
        if (binaryFormat == BinaryFormat::AMD && amdappVersion >= 200406)
            defaultCL2ForDriver = true;
    }
    else // if not good OpenCL platform
        throw Exception("OpenCL platform not suitable for CLRX programs");
    
    // check whether is GPU device
    cl_device_type clDevType;
    error = clGetDeviceInfo(clDevice, CL_DEVICE_TYPE, sizeof(cl_device_type),
                    &clDevType, nullptr);
    if ((clDevType & CL_DEVICE_TYPE_GPU) == 0)
        throw Exception("Device is not GPU");
    
    cl_uint bits = 32;
    if (binaryFormat != BinaryFormat::GALLIUM)
    {
        // get address Bits from device info (for AMDAPP)
        error = clGetDeviceInfo(clDevice, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
                                 &bits, nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDeviceAddressBits");
    }
    
    // get device and print that
    size_t deviceNameSize;
    std::unique_ptr<char[]> deviceName;
    error = clGetDeviceInfo(clDevice, CL_DEVICE_NAME, 0, nullptr, &deviceNameSize);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceInfoName");
    deviceName.reset(new char[deviceNameSize]);
    error = clGetDeviceInfo(clDevice, CL_DEVICE_NAME, deviceNameSize,
                             deviceName.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceInfoName");
    
    // get bits from device name (LLVM version)
    cxuint llvmVersion = 0;
    cxuint mesaVersion = 0;
    
    if (binaryFormat == BinaryFormat::GALLIUM)
    {
        const char* llvmPart = strstr(deviceName.get(), "LLVM ");
        if (llvmPart!=nullptr)
        {
            try
            {
                // parse LLVM version
                const char* majorVerPart = llvmPart+5;
                const char* minorVerPart;
                const char* end;
                cxuint majorVersion = cstrtoui(majorVerPart, nullptr, minorVerPart);
                if (*minorVerPart!=0)
                {
                    minorVerPart++; // skip '.'
                    cxuint minorVersion = cstrtoui(minorVerPart, nullptr, end);
                    llvmVersion = majorVersion*10000U + minorVersion*100U;
#if HAVE_64BIT
                    if (majorVersion*10000U + minorVersion*100U >= 30900U)
                        bits = 64; // use 64-bit
#endif
                }
            }
            catch(const ParseException& ex)
            { } // ignore error
        }
        
        // get device version - used for getting Mesa3D version and LLVM version
        size_t deviceVersionSize;
        std::unique_ptr<char[]> deviceVersion;
        error = clGetDeviceInfo(clDevice, CL_DEVICE_VERSION, 0, nullptr,
                                &deviceVersionSize);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDeviceInfoVersion");
        deviceVersion.reset(new char[deviceVersionSize]);
        error = clGetDeviceInfo(clDevice, CL_DEVICE_VERSION, deviceVersionSize,
                                deviceVersion.get(), nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDeviceInfoVersion");
        
        const char* mesaPart = strstr(deviceVersion.get(), "Mesa ");
        if (mesaPart==nullptr)
            mesaPart = strstr(deviceVersion.get(), "MESA ");
        if (mesaPart!=nullptr)
        {
            try
            {
                // parse Mesa3D version
                const char* majorVerPart = mesaPart+5;
                const char* minorVerPart;
                const char* end;
                cxuint majorVersion = cstrtoui(majorVerPart, nullptr, minorVerPart);
                if (*minorVerPart!=0)
                {
                    minorVerPart++; // skip '.'
                    cxuint minorVersion = cstrtoui(minorVerPart, nullptr, end);
                    mesaVersion = majorVersion*10000U + minorVersion*100U;
                }
            }
            catch(const ParseException& ex)
            { } // ignore error
        }
    }
    else
    {
        // check whether is ROCm-OpenCL
        // get driver version - used for getting ROCm-OpenCL info
        size_t driverVersionSize;
        std::unique_ptr<char[]> driverVersion;
        error = clGetDeviceInfo(clDevice, CL_DRIVER_VERSION, 0, nullptr,
                        &driverVersionSize);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDriverInfoVersion");
        driverVersion.reset(new char[driverVersionSize]);
        error = clGetDeviceInfo(clDevice, CL_DRIVER_VERSION, driverVersionSize,
                                driverVersion.get(), nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDriverInfoVersion");
        
        // parse ROCm-OpenCL (HSAXXX,LC)
        const char* hsaStr = strstr(driverVersion.get(), "(HSA");
        if (hsaStr != nullptr)
        {
            // now we check LC
            const char* lcStr = strstr(hsaStr+4, ",LC)");
            if (lcStr != nullptr)
                // we have ROCm binary format
                binaryFormat = BinaryFormat::ROCM;
        }
    }
    
    /// determine device type
    char* sdeviceName = stripCString(deviceName.get());
    char* devNamePtr = sdeviceName;
    if (binaryFormat==BinaryFormat::GALLIUM)
    {
        char* sptr = ::strstr(sdeviceName, "(AMD ");
        // if form 'AMD Radeon xxx (AMD CODENAME /...)
        if (sptr != nullptr) // if found 'AMD ';
            devNamePtr = sptr+5;
        else
        {
            // if form 'AMD CODENAME (....
            sptr = ::strstr(sdeviceName, "AMD ");
            if (sptr != nullptr) // if found 'AMD ';
                devNamePtr = sptr+4;
        }
    }
    char* devNameEnd = devNamePtr;
    while (isAlnum(*devNameEnd)) devNameEnd++;
    char oldChar = *devNameEnd; // for revert changes
    *devNameEnd = 0; // finish at first word
    GPUDeviceType devType = GPUDeviceType::CAPE_VERDE;
    try
    { devType = getGPUDeviceTypeFromName(devNamePtr); }
    catch(const GPUIdException& ex)
    {
        // yet another fallback for gallium device name
        if (binaryFormat==BinaryFormat::GALLIUM)
        {
            char* sptr = sdeviceName;
            while (true)
            {
                *devNameEnd = oldChar;
                sptr = ::strchr(sptr, '(');
                if (sptr == nullptr)
                    throw; // nothing found
                devNamePtr = sptr+1;
                // try again
                devNameEnd = devNamePtr;
                while (isAlnum(*devNameEnd)) devNameEnd++;
                oldChar = *devNameEnd;
                *devNameEnd = 0; // finish at first word
                try
                {
                    devType = getGPUDeviceTypeFromName(devNamePtr);
                    break; // if found
                }
                catch(const GPUIdException& ex2)
                { sptr++; /* not found skip this '(' */ }
            }
        }
        else
            throw;
    }
    /* change binary format to AMDCL2 if default for this driver version and 
     * architecture >= GCN 1.1 */
    bool useLegacy = false;
    if (defaultCL2ForDriver &&
        getGPUArchitectureFromDeviceType(devType) >= GPUArchitecture::GCN1_1)
    {
        if (useCL!=1) // if not cl1/old
            binaryFormat = BinaryFormat::AMDCL2;
        else // use legacy
            useLegacy = true;
    }
    
    // create OpenCL CLRX assembler setup
    CLAsmSetup asmSetup { };
    asmSetup.deviceType = devType;
    asmSetup.is64Bit = bits==64;
    asmSetup.binaryFormat = binaryFormat;
    // setting version (LLVM and driverVersion)
    if (binaryFormat == BinaryFormat::GALLIUM && llvmVersion != 0)
        asmSetup.llvmVersion = llvmVersion;
    if (binaryFormat == BinaryFormat::GALLIUM && mesaVersion != 0)
        asmSetup.driverVersion = mesaVersion;
    else if ((binaryFormat == BinaryFormat::AMD || binaryFormat == BinaryFormat::ROCM ||
            binaryFormat == BinaryFormat::AMDCL2) && amdappVersion != 0)
        asmSetup.driverVersion = amdappVersion;
    // base OpenCL options for program
    asmSetup.options = (binaryFormat==BinaryFormat::AMDCL2) ? "-cl-std=CL2.0" :
               (useLegacy ? "-legacy" : "");
    asmSetup.asmFlags = asmFlags;
    if (binaryFormat == BinaryFormat::ROCM)
        asmSetup.newROCmBinFormat = true;
    return asmSetup;
}

Array<cxbyte> CLRX::createBinaryForOpenCL(const CLAsmSetup& asmSetup,
                const char* sourceCode, size_t sourceCodeLen)
{
    return createBinaryForOpenCL(asmSetup, {}, sourceCode, sourceCodeLen);
}

Array<cxbyte> CLRX::createBinaryForOpenCL(const CLAsmSetup& asmSetup,
                const std::vector<std::pair<CString, uint64_t> >& defSymbols,
                const char* sourceCode, size_t sourceCodeLen)
{
    /// determine device type
    const size_t scodeLen = (sourceCodeLen == 0) ? ::strlen(sourceCode) : sourceCodeLen;
    ArrayIStream astream(scodeLen, sourceCode);
    // by default assembler put logs to stderr
    Assembler assembler("", astream, asmSetup.asmFlags, asmSetup.binaryFormat,
                        asmSetup.deviceType);
    assembler.set64Bit(asmSetup.is64Bit);
    // setting version (LLVM and driverVersion)
    const BinaryFormat binaryFormat = asmSetup.binaryFormat;
    if (binaryFormat == BinaryFormat::GALLIUM && asmSetup.llvmVersion != 0)
        assembler.setLLVMVersion(asmSetup.llvmVersion);
    if (asmSetup.driverVersion != 0)
        assembler.setDriverVersion(asmSetup.driverVersion);
    assembler.setNewROCmBinFormat(asmSetup.newROCmBinFormat);
    // initial def symbols
    for (const auto& symbol: defSymbols)
        assembler.addInitialDefSym(symbol.first, symbol.second);
    
    assembler.assemble();
    // write binary
    Array<cxbyte> binary;
    assembler.writeBinary(binary);
    return binary;
}


cl_program CLRX::createProgramForCLDevice(cl_context clContext, cl_device_id clDevice,
            const CLAsmSetup& asmSetup, const Array<cxbyte>& binary,
            CString* buildLog)
{
    cl_program clProgram;
    cl_int error = CL_SUCCESS;
    
    size_t binarySize = binary.size();
    const cxbyte* binaryContent = binary.data();
    clProgram = clCreateProgramWithBinary(clContext, 1, &clDevice, &binarySize,
                        &binaryContent, nullptr, &error);
    if (clProgram==nullptr)
        throw CLError(error, "clCreateProgramWithBinary");
    // build program
    error = clBuildProgram(clProgram, 1, &clDevice, asmSetup.options.c_str(),
               nullptr, nullptr);
    if (error != CL_SUCCESS)
    {
        /* get build logs */
        if (buildLog != nullptr)
        {
            buildLog->clear();
            size_t buildLogSize;
            std::unique_ptr<char[]> tempBuildLog;
            cl_int lerror = clGetProgramBuildInfo(clProgram, clDevice, CL_PROGRAM_BUILD_LOG,
                            0, nullptr, &buildLogSize);
            if (lerror == CL_SUCCESS)
            {
                tempBuildLog.reset(new char[buildLogSize]);
                lerror = clGetProgramBuildInfo(clProgram, clDevice, CL_PROGRAM_BUILD_LOG,
                            buildLogSize, tempBuildLog.get(), nullptr);
                if (lerror == CL_SUCCESS) // print build log
                    buildLog->assign(tempBuildLog.get());
            }
        }
        throw CLError(error, "clBuildProgram");
    }
    
    return clProgram;
}

// simple helper for this two stage of building
cl_program CLRX::createProgramForCLDevice(cl_context clContext, cl_device_id clDevice,
            const CLAsmSetup& asmSetup, const char* sourceCode, size_t sourceCodeLen,
            CString* buildLog)
{
    const Array<cxbyte>& binary = createBinaryForOpenCL(asmSetup,
                            sourceCode, sourceCodeLen);
    return createProgramForCLDevice(clContext, clDevice, asmSetup, binary, buildLog);
}

cl_program CLRX::createProgramForCLDevice(cl_context clContext, cl_device_id clDevice,
            const CLAsmSetup& asmSetup,
            const std::vector<std::pair<CString, uint64_t> >& defSymbols,
            const char* sourceCode, size_t sourceCodeLen, CString* buildLog)
{
    const Array<cxbyte>& binary = createBinaryForOpenCL(asmSetup, defSymbols,
                            sourceCode, sourceCodeLen);
    return createProgramForCLDevice(clContext, clDevice, asmSetup, binary, buildLog);
}
