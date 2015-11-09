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

CLFacade::CLFacade(cl_uint deviceIndex, const char* sourceCode, const char* kernelNames)
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
        
        if (::strcmp(platformName.get(), "AMD Accelerated Parallel Processing")==0 ||
            ::strcmp(platformName.get(), "Clover")==0)
        {
            choosenPlatform = platforms[i];
            binaryFormat = ::strcmp(platformName.get(), "Clover")==0 ?
                    BinaryFormat::GALLIUM : BinaryFormat::AMD;
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
    
    cl_uint addressBits;
    error = clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
                             &addressBits, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceAddressBits");
    
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
    
    /// create command queue
    queue = clCreateCommandQueue(context, device, 0, &error);
    if (queue==nullptr)
        throw CLError(error, "clCreateCommandQueue");
    
    Array<cxbyte> binary;
    {   /* assemble source code */
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
        /// determine device type
        const char* devNamePtr = (binaryFormat==BinaryFormat::GALLIUM &&
                ::strncmp(deviceName.get(), "AMD ", 4)==0) ?
                deviceName.get()+4 : deviceName.get();
        const GPUDeviceType devType = getGPUDeviceTypeFromName(devNamePtr);
        
        ArrayIStream astream(::strlen(sourceCode), sourceCode);
        std::string msgString;
        StringOStream msgStream(msgString);
        // by default assembler put logs to stderr
        Assembler assembler("", astream, 0, binaryFormat, devType);
        assembler.set64Bit(addressBits==64);
        if (assembler.assemble())
        {   // if good
            const AsmFormatHandler* formatHandler = assembler.getFormatHandler();
            if (formatHandler!=nullptr) // and if binary generated
                formatHandler->writeBinary(binary);
        }
    }
    
    if (binary.empty())
        throw Exception("No binary generated");
    
    size_t binarySize = binary.size();
    const cxbyte* binaryContent = binary.data();
    program = clCreateProgramWithBinary(context, 1, &device, &binarySize,
                        &binaryContent, nullptr, &error);
    if (program==nullptr)
        throw CLError(error, "clCreateProgramWithBinary");
    // build program
    error = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
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
