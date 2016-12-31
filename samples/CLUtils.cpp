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

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <memory>
#include <CL/cl.h>
#include <CLRX/amdasm/Assembler.h>
#include "CLUtils.h"

using namespace CLRX;

static char* stripCString(char* str)
{
    while (*str==' ') str++;
    char* last = str+::strlen(str);
    while (last!=str && (*last==0||*last==' '))
        last--;
    if (*last!=0) last[1] = 0;
    return str;
}

bool CLFacade::parseArgs(const char* progName, const char* usagePart, int argc,
                  const char** argv, cl_uint& deviceIndex, bool& useCL2)
{
    if (argc >= 2 && ::strcmp(argv[1], "-?")==0)
    {
        std::cout << "Usage: " << progName << " [DEVICE_INDEX[cl2]] " << usagePart << "\n"
                "Print device list: " << progName << " -L" << "\n"
                "Print help: " << progName << " -?\n"
                "'cl2' after DEVICE_INDEX enables AMD OpenCL 2.0 mode" << std::endl;
        return true;
    }
    
    cl_uint platformsNum;
    std::unique_ptr<cl_platform_id[]> platforms;
    cl_int error = 0;
    error = clGetPlatformIDs(0, nullptr, &platformsNum);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformIDs");
    platforms.reset(new cl_platform_id[platformsNum]);
    error = clGetPlatformIDs(platformsNum, platforms.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformIDs");
    
    cl_platform_id choosenPlatform = nullptr;
    /// find platform with AMD devices
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
        
        const char* splatformName = stripCString(platformName.get());
        if (::strcmp(splatformName, "AMD Accelerated Parallel Processing")==0 ||
            ::strcmp(splatformName, "Clover")==0)
        {
            choosenPlatform = platforms[i];
            break;
        }
    }
    
    if (choosenPlatform==nullptr)
        throw Exception("PlatformNotFound");
    
    if (argc >= 2 && ::strcmp(argv[1], "-L")==0)
    {
        cl_uint devicesNum;
        std::unique_ptr<cl_device_id[]> devices;
        error = clGetDeviceIDs(choosenPlatform, CL_DEVICE_TYPE_GPU, 0,
                               nullptr, &devicesNum);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDeviceIDs");
        
        devices.reset(new cl_device_id[devicesNum]);
        error = clGetDeviceIDs(choosenPlatform, CL_DEVICE_TYPE_GPU,
                        devicesNum, devices.get(), nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDeviceIDs");
        
        for (cl_uint i = 0; i < devicesNum; i++)
        {
            cl_device_id device = devices[i];
            // get device and print that
            size_t deviceNameSize;
            std::unique_ptr<char[]> deviceName;
            error = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &deviceNameSize);
            if (error != CL_SUCCESS)
                throw CLError(error, "clGetDeviceInfoName");
            deviceName.reset(new char[deviceNameSize]);
            error = clGetDeviceInfo(device, CL_DEVICE_NAME, deviceNameSize,
                                     deviceName.get(), nullptr);
            if (error != CL_SUCCESS)
                throw CLError(error, "clGetDeviceInfoName");
            std::cout << "Device: " << i << " - " << deviceName.get() << "\n";
        }
        std::cout.flush();
        return true;
    }
    else if (argc >= 2)
    {
        const char* end;
        useCL2 = false;
        deviceIndex = cstrtovCStyle<cl_uint>(argv[1], nullptr, end);
        if (strcasecmp(end, "cl2")==0)
            useCL2 = true;
    }
    return false;
}

static const char* binaryFormatNamesTbl[] =
{
    "AMD OpenCL 1.2", "GalliumCompute", "Raw code", "AMD OpenCL 2.0"
};

CLFacade::CLFacade(cl_uint deviceIndex, const char* sourceCode, const char* kernelNames,
            bool useCL2)
{
try
{
    context = nullptr;
    queue = nullptr;
    program = nullptr;
    
    cl_uint platformsNum;
    std::unique_ptr<cl_platform_id[]> platforms;
    cl_int error = 0;
    error = clGetPlatformIDs(0, nullptr, &platformsNum);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformIDs");
    platforms.reset(new cl_platform_id[platformsNum]);
    error = clGetPlatformIDs(platformsNum, platforms.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetPlatformIDs");
    
    BinaryFormat binaryFormat = BinaryFormat::GALLIUM;
    cl_platform_id choosenPlatform = nullptr;
    bool defaultCL2ForDriver = false;
    /// find platform with AMD devices
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
        
        const char* splatformName = stripCString(platformName.get());
        if (::strcmp(splatformName, "AMD Accelerated Parallel Processing")==0 ||
            ::strcmp(splatformName, "Clover")==0)
        {
            choosenPlatform = platforms[i];
            binaryFormat = ::strcmp(platformName.get(), "Clover")==0 ?
                    BinaryFormat::GALLIUM : BinaryFormat::AMD;
            if (binaryFormat == BinaryFormat::AMD && useCL2)
                binaryFormat = BinaryFormat::AMDCL2;
            if (binaryFormat == BinaryFormat::AMD && detectAmdDriverVersion() >= 200406)
                defaultCL2ForDriver = true;
            break;
        }
    }
    
    if (choosenPlatform==nullptr)
        throw Exception("PlatformNotFound");
    
    // find device
    cl_uint devicesNum;
    std::unique_ptr<cl_device_id[]> devices;
    error = clGetDeviceIDs(choosenPlatform, CL_DEVICE_TYPE_GPU, 0, nullptr, &devicesNum);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceIDs");
    
    if (deviceIndex >= devicesNum)
        throw CLError(0, "DeviceIndexOutOfRange");
        
    devices.reset(new cl_device_id[devicesNum]);
    error = clGetDeviceIDs(choosenPlatform, CL_DEVICE_TYPE_GPU,
                    devicesNum, devices.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceIDs");
    
    device = devices[deviceIndex];
    
    cl_uint bits = 32;
    if (binaryFormat != BinaryFormat::GALLIUM)
    {   // get address Bits from device info
        error = clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
                                 &bits, nullptr);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDeviceAddressBits");
    }
    
    error = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
                             &maxWorkGroupSize, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceMaxWorkGroupSize");
    error = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint),
                             &computeUnits, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceMaxComputeUnits");
    
    /// create context
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &error);
    if (context==nullptr)
        throw CLError(error, "clCreateContext");
    
    // get device and print that
    size_t deviceNameSize;
    std::unique_ptr<char[]> deviceName;
    error = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &deviceNameSize);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceInfoName");
    deviceName.reset(new char[deviceNameSize]);
    error = clGetDeviceInfo(device, CL_DEVICE_NAME, deviceNameSize,
                             deviceName.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceInfoName");
    std::cout << "Device: " << deviceIndex << " - " << deviceName.get() << std::endl;
    
    // get bits from device name (LLVM version)
#if HAVE_64BIT
    if (binaryFormat == BinaryFormat::GALLIUM)
    {
        const char* llvmPart = strstr(deviceName.get(), "LLVM ");
        if (llvmPart!=nullptr)
        {
            try
            {
                const char* majorVerPart = llvmPart+5;
                const char* minorVerPart;
                const char* end;
                cxuint majorVersion = cstrtoui(majorVerPart, nullptr, minorVerPart);
                minorVerPart++; // skip '.'
                cxuint minorVersion = cstrtoui(minorVerPart, nullptr, end);
                if (majorVersion*10000U + minorVersion*100U >= 30900U)
                    bits = 64; // use 64-bit
            }
            catch(const ParseException& ex)
            { } // ignore error
        }
    }
#endif
    /* assemble source code */
    /// determine device type
    char* sdeviceName = stripCString(deviceName.get());
    char* devNamePtr = (binaryFormat==BinaryFormat::GALLIUM &&
            ::strncmp(sdeviceName, "AMD ", 4)==0) ? sdeviceName+4 : sdeviceName;
    char* devNameEnd = devNamePtr;
    while (isAlnum(*devNameEnd)) devNameEnd++;
    *devNameEnd = 0; // finish at first word
    const GPUDeviceType devType = getGPUDeviceTypeFromName(devNamePtr);
    /* change binary format to AMDCL2 if default for this driver version and 
     * architecture >= GCN 1.1 */
    if (defaultCL2ForDriver &&
        getGPUArchitectureFromDeviceType(devType) >= GPUArchitecture::GCN1_1)
        binaryFormat = BinaryFormat::AMDCL2;

    std::cout << "BinaryFormat: " << binaryFormatNamesTbl[cxuint(binaryFormat)] << "\n"
        "Bitness: " << bits << std::endl;
    
    /// create command queue
    queue = clCreateCommandQueue(context, device, 0, &error);
    if (queue==nullptr)
        throw CLError(error, "clCreateCommandQueue");
    
    Array<cxbyte> binary;
    {   /* assemble source code */
        /// determine device type
        ArrayIStream astream(::strlen(sourceCode), sourceCode);
        // by default assembler put logs to stderr
        Assembler assembler("", astream, 0, binaryFormat, devType);
        assembler.set64Bit(bits==64);
        assembler.assemble();
        assembler.writeBinary(binary);
    }
    
    size_t binarySize = binary.size();
    const cxbyte* binaryContent = binary.data();
    program = clCreateProgramWithBinary(context, 1, &device, &binarySize,
                        &binaryContent, nullptr, &error);
    if (program==nullptr)
        throw CLError(error, "clCreateProgramWithBinary");
    // build program
    error = clBuildProgram(program, 1, &device,
               (binaryFormat==BinaryFormat::AMDCL2) ? "-cl-std=CL2.0" : "",
               nullptr, nullptr);
    if (error != CL_SUCCESS)
    {   /* get build logs */
        size_t buildLogSize;
        std::unique_ptr<char[]> buildLog;
        cl_int lerror = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                           0, nullptr, &buildLogSize);
        if (lerror == CL_SUCCESS)
        {
            buildLog.reset(new char[buildLogSize]);
            lerror = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                           buildLogSize, buildLog.get(), nullptr);
            if (lerror == CL_SUCCESS) // print build log
                std::cerr << "BuildLog:\n" << buildLog.get() << std::endl;
        }
        throw CLError(error, "clBuildProgram");
    }
    
    if (kernelNames!=nullptr)
    try
    {
        for (const char* kn = kernelNames; *kn!=0;)
        {
            const char* knameStart = kn;
            while (*kn!=0 && *kn!=' ') kn++;
            std::string kernelName(knameStart, kn);
            cl_kernel kernel = clCreateKernel(program, kernelName.c_str(), &error);
            if (error != CL_SUCCESS)
                throw CLError(error, "clCreateKernel");
            kernels.push_back(kernel);
            while (*kn==' ') kn++; // skip spaces
        }
    }
    catch(...)
    {
        for (cl_kernel kernel: kernels)
            clReleaseKernel(kernel);
        throw;
    }
}
catch(...)
{
    if (program!=nullptr)
        clReleaseProgram(program);
    if (queue!=nullptr)
        clReleaseCommandQueue(queue);
    if (context!=nullptr)
        clReleaseContext(context);
    throw;
}
}

CLFacade::~CLFacade()
{
    for (cl_mem memObj: memObjects)
        clReleaseMemObject(memObj);
    for (cl_kernel kernel: kernels)
        clReleaseKernel(kernel);
    if (program!=nullptr)
        clReleaseProgram(program);
    if (queue!=nullptr)
        clReleaseCommandQueue(queue);
    if (context!=nullptr)
        clReleaseContext(context);
}

void CLFacade::getKernelInfo(cl_kernel kernel, size_t& workGroupSize,
               size_t& workGroupSizeMultiple)
{
    cl_int error = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE,
                 sizeof(size_t), &workGroupSize, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetKernelWorkGroupSize");
    error = clGetKernelWorkGroupInfo(kernel, device,
                 CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                 sizeof(size_t), &workGroupSizeMultiple, nullptr);
    if (error != CL_SUCCESS || workGroupSizeMultiple == 1)
        workGroupSizeMultiple = 64; // fix for GalliumCompute
}

void CLFacade::callNDRangeKernel(cl_kernel kernel, cl_uint workDim, const size_t* offset,
               const size_t* workSize, const size_t* localSize)
{
    cl_event event;
    cl_int error = clEnqueueNDRangeKernel(queue, kernel, workDim, offset, workSize,
                                   localSize, 0, nullptr, &event);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueNDRangeKernel");
    error = clWaitForEvents(1, &event); // waiting for finish kernel
    if (error != CL_SUCCESS)
    {
        clReleaseEvent(event);
        throw CLError(error, "clWaitForEvents");
    }
    clReleaseEvent(event);
}
