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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <memory>
#ifdef __APPLE__
#  include <OpenCL/cl.h>
#else
#  include <CL/cl.h>
#endif
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/clhelper/CLHelper.h>
#include "CLUtils.h"

using namespace CLRX;

/* parse args from command line and handle options:
 * print help, list OpenCL devices, get choosen device and choosen OpenCL standard
 * return true if sample should exit */
bool CLFacade::parseArgs(const char* progName, const char* usagePart, int argc,
                  const char** argv, cl_uint& deviceIndex, cxuint& useCL)
{
    if (argc >= 2 && ::strcmp(argv[1], "-?")==0)
    {
        std::cout << "Usage: " << progName << " [DEVICE_INDEX[cl1|old|cl2]] " <<
                usagePart << "\n" "Print device list: " << progName << " -L" << "\n"
                "Print help: " << progName << " -?\n"
                "'cl2' after DEVICE_INDEX enables AMD OpenCL 2.0 mode\n"
                "'cl1' or 'old' after DEVICE_INDEX force old AMD OpenCL 1.2 mode"
                << std::endl;
        return true;
    }
    
    cl_int error = CL_SUCCESS;
    const std::vector<cl_platform_id> choosenPlatforms = chooseCLPlatformsForCLRX();
    
    if (argc >= 2 && ::strcmp(argv[1], "-L")==0)
    {
        cxuint devPlatformStart = 0;
        for (cl_platform_id choosenPlatform: choosenPlatforms)
        {
            size_t platformNameSize;
            std::unique_ptr<char[]> platformName;
            error = clGetPlatformInfo(choosenPlatform, CL_PLATFORM_NAME, 0, nullptr,
                            &platformNameSize);
            if (error != CL_SUCCESS)
                throw CLError(error, "clGetPlatformInfoName");
            platformName.reset(new char[platformNameSize]);
            error = clGetPlatformInfo(choosenPlatform, CL_PLATFORM_NAME, platformNameSize,
                                    platformName.get(), nullptr);
            if (error != CL_SUCCESS)
                throw CLError(error, "clGetPlatformInfoName");
            
            std::cout << "Platform: " << platformName.get() << "\n";
            
            // list devices, before it get GPU devices
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
                error = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr,
                                &deviceNameSize);
                if (error != CL_SUCCESS)
                    throw CLError(error, "clGetDeviceInfoName");
                deviceName.reset(new char[deviceNameSize]);
                error = clGetDeviceInfo(device, CL_DEVICE_NAME, deviceNameSize,
                                        deviceName.get(), nullptr);
                if (error != CL_SUCCESS)
                    throw CLError(error, "clGetDeviceInfoName");
                std::cout << "  Device: " << (i+devPlatformStart) << " - " <<
                            deviceName.get() << "\n";
            }
            devPlatformStart += devicesNum;
        }
        std::cout.flush();
        return true;
    }
    else if (argc >= 2)
    {
        const char* end;
        useCL = 0;
        deviceIndex = cstrtovCStyle<cl_uint>(argv[1], nullptr, end);
        if (strcasecmp(end, "cl2")==0)
            useCL = 2;
        else if (strcasecmp(end, "cl1")==0 || strcasecmp(end, "old")==0)
            useCL = 1;
    }
    return false;
}

static const char* binaryFormatNamesTbl[] =
{
    "AMD OpenCL 1.2", "GalliumCompute", "Raw code", "AMD OpenCL 2.0", "ROCm"
};

CLFacade::CLFacade(cl_uint deviceIndex, const char* sourceCode, const char* kernelNames,
            cxuint useCL)
{
try
{
    context = nullptr;
    device = nullptr;
    queue = nullptr;
    program = nullptr;
    
    cl_int error = CL_SUCCESS;
    const std::vector<cl_platform_id> choosenPlatforms = chooseCLPlatformsForCLRX();
    
    // find device
    cxuint devPlatformStart = 0;
    cl_uint devicesNum;
    cl_platform_id choosenPlatform = nullptr;
    std::unique_ptr<cl_device_id[]> devices;
    for (cl_platform_id platform: choosenPlatforms)
    {
        error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU,
                            0, nullptr, &devicesNum);
        if (error != CL_SUCCESS)
            throw CLError(error, "clGetDeviceIDs");
        
        if (deviceIndex - devPlatformStart < devicesNum)
        {
            choosenPlatform = platform;
            break;
        }
        devPlatformStart += devicesNum;
    }
    if (choosenPlatform == nullptr)
        throw CLError(0, "DeviceIndexOutOfRange");
        
    devices.reset(new cl_device_id[devicesNum]);
    error = clGetDeviceIDs(choosenPlatform, CL_DEVICE_TYPE_GPU,
                    devicesNum, devices.get(), nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clGetDeviceIDs");
    
    device = devices[deviceIndex-devPlatformStart];
    
    const CLAsmSetup asmSetup = assemblerSetupForCLDevice(device, useCL==1 ?
            CLHELPER_USEAMDLEGACY : useCL==2 ? CLHELPER_USEAMDCL2 : 0);
    
    // get workGroupSize and Compute Units of device
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
    
    std::cout << "BinaryFormat: " <<
        binaryFormatNamesTbl[cxuint(asmSetup.binaryFormat)] << "\n"
        "Bitness: " << (asmSetup.is64Bit ? 64U : 32U) << std::endl;
    
    /// create command queue
    queue = clCreateCommandQueue(context, device, 0, &error);
    if (queue==nullptr)
        throw CLError(error, "clCreateCommandQueue");
    
    CString buildLog;
    try
    { program = createProgramForCLDevice(context, device,
                    asmSetup, sourceCode, 0, &buildLog); }
    // function throw exception when fail
    catch(const CLError& error)
    {
        std::cerr << "BuildLog:\n" << buildLog << std::endl;
        throw;
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

// get work group size and work group size multiple from kernel
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
