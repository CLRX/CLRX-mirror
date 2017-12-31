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
#include <algorithm>
#include <exception>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <cstring>
#include <CLRX/utils/Utilities.h>
#include "CLWrapper.h"

extern "C"
{

/* public API definitions */
#ifdef _MSC_VER
#  ifdef HAVE_32BIT
#    pragma comment(linker,"/export:clGetPlatformIDs=_clrxclGetPlatformIDs@12")
#    pragma comment(linker,"/export:clIcdGetPlatformIDsKHR=_clrxclIcdGetPlatformIDsKHR@12")
#    pragma comment(linker,"/export:clGetPlatformInfo=_clrxclGetPlatformInfo@20")
#    pragma comment(linker,"/export:clGetDeviceIDs=_clrxclGetDeviceIDs@24")
#    pragma comment(linker,"/export:clGetDeviceInfo=_clrxclGetDeviceInfo@20")
#    pragma comment(linker,"/export:clCreateContext=_clrxclCreateContext@24")
#    pragma comment(linker,"/export:clCreateContextFromType=_clrxclCreateContextFromType@24")
#    pragma comment(linker,"/export:clRetainContext=_clrxclRetainContext@4")
#    pragma comment(linker,"/export:clReleaseContext=_clrxclReleaseContext@4")
#    pragma comment(linker,"/export:clGetContextInfo=_clrxclGetContextInfo@20")
#    pragma comment(linker,"/export:clCreateCommandQueue=_clrxclCreateCommandQueue@20")
#    pragma comment(linker,"/export:clRetainCommandQueue=_clrxclRetainCommandQueue@4")
#    pragma comment(linker,"/export:clReleaseCommandQueue=_clrxclReleaseCommandQueue@4")
#    pragma comment(linker,"/export:clGetCommandQueueInfo=_clrxclGetCommandQueueInfo@20")
#    pragma comment(linker,"/export:clSetCommandQueueProperty=_clrxclSetCommandQueueProperty@20")
#    pragma comment(linker,"/export:clCreateBuffer=_clrxclCreateBuffer@24")
#    pragma comment(linker,"/export:clCreateImage2D=_clrxclCreateImage2D@36")
#    pragma comment(linker,"/export:clCreateImage3D=_clrxclCreateImage3D@44")
#    pragma comment(linker,"/export:clRetainMemObject=_clrxclRetainMemObject@4")
#    pragma comment(linker,"/export:clReleaseMemObject=_clrxclReleaseMemObject@4")
#    pragma comment(linker,"/export:clGetSupportedImageFormats=_clrxclGetSupportedImageFormats@28")
#    pragma comment(linker,"/export:clGetMemObjectInfo=_clrxclGetMemObjectInfo@20")
#    pragma comment(linker,"/export:clGetImageInfo=_clrxclGetImageInfo@20")
#    pragma comment(linker,"/export:clCreateSampler=_clrxclCreateSampler@20")
#    pragma comment(linker,"/export:clRetainSampler=_clrxclRetainSampler@4")
#    pragma comment(linker,"/export:clReleaseSampler=_clrxclReleaseSampler@4")
#    pragma comment(linker,"/export:clGetSamplerInfo=_clrxclGetSamplerInfo@20")
#    pragma comment(linker,"/export:clCreateProgramWithSource=_clrxclCreateProgramWithSource@20")
#    pragma comment(linker,"/export:clCreateProgramWithBinary=_clrxclCreateProgramWithBinary@28")
#    pragma comment(linker,"/export:clRetainProgram=_clrxclRetainProgram@4")
#    pragma comment(linker,"/export:clReleaseProgram=_clrxclReleaseProgram@4")
#    pragma comment(linker,"/export:clBuildProgram=_clrxclBuildProgram@24")
#    pragma comment(linker,"/export:clUnloadCompiler=_clrxclUnloadCompiler@0")
#    pragma comment(linker,"/export:clGetProgramInfo=_clrxclGetProgramInfo@20")
#    pragma comment(linker,"/export:clGetProgramBuildInfo=_clrxclGetProgramBuildInfo@24")
#    pragma comment(linker,"/export:clCreateKernel=_clrxclCreateKernel@12")
#    pragma comment(linker,"/export:clCreateKernelsInProgram=_clrxclCreateKernelsInProgram@16")
#    pragma comment(linker,"/export:clRetainKernel=_clrxclRetainKernel@4")
#    pragma comment(linker,"/export:clReleaseKernel=_clrxclReleaseKernel@4")
#    pragma comment(linker,"/export:clSetKernelArg=_clrxclSetKernelArg@16")
#    pragma comment(linker,"/export:clGetKernelInfo=_clrxclGetKernelInfo@20")
#    pragma comment(linker,"/export:clGetKernelWorkGroupInfo=_clrxclGetKernelWorkGroupInfo@24")
#  else
#    pragma comment(linker,"/export:clGetPlatformIDs=clrxclGetPlatformIDs")
#    pragma comment(linker,"/export:clIcdGetPlatformIDsKHR=clrxclIcdGetPlatformIDsKHR")
#    pragma comment(linker,"/export:clGetPlatformInfo=clrxclGetPlatformInfo")
#    pragma comment(linker,"/export:clGetDeviceIDs=clrxclGetDeviceIDs")
#    pragma comment(linker,"/export:clGetDeviceInfo=clrxclGetDeviceInfo")
#    pragma comment(linker,"/export:clCreateContext=clrxclCreateContext")
#    pragma comment(linker,"/export:clCreateContextFromType=clrxclCreateContextFromType")
#    pragma comment(linker,"/export:clRetainContext=clrxclRetainContext")
#    pragma comment(linker,"/export:clReleaseContext=clrxclReleaseContext")
#    pragma comment(linker,"/export:clGetContextInfo=clrxclGetContextInfo")
#    pragma comment(linker,"/export:clCreateCommandQueue=clrxclCreateCommandQueue")
#    pragma comment(linker,"/export:clRetainCommandQueue=clrxclRetainCommandQueue")
#    pragma comment(linker,"/export:clReleaseCommandQueue=clrxclReleaseCommandQueue")
#    pragma comment(linker,"/export:clGetCommandQueueInfo=clrxclGetCommandQueueInfo")
#    pragma comment(linker,"/export:clSetCommandQueueProperty=clrxclSetCommandQueueProperty")
#    pragma comment(linker,"/export:clCreateBuffer=clrxclCreateBuffer")
#    pragma comment(linker,"/export:clCreateImage2D=clrxclCreateImage2D")
#    pragma comment(linker,"/export:clCreateImage3D=clrxclCreateImage3D")
#    pragma comment(linker,"/export:clRetainMemObject=clrxclRetainMemObject")
#    pragma comment(linker,"/export:clReleaseMemObject=clrxclReleaseMemObject")
#    pragma comment(linker,"/export:clGetSupportedImageFormats=clrxclGetSupportedImageFormats")
#    pragma comment(linker,"/export:clGetMemObjectInfo=clrxclGetMemObjectInfo")
#    pragma comment(linker,"/export:clGetImageInfo=clrxclGetImageInfo")
#    pragma comment(linker,"/export:clCreateSampler=clrxclCreateSampler")
#    pragma comment(linker,"/export:clRetainSampler=clrxclRetainSampler")
#    pragma comment(linker,"/export:clReleaseSampler=clrxclReleaseSampler")
#    pragma comment(linker,"/export:clGetSamplerInfo=clrxclGetSamplerInfo")
#    pragma comment(linker,"/export:clCreateProgramWithSource=clrxclCreateProgramWithSource")
#    pragma comment(linker,"/export:clCreateProgramWithBinary=clrxclCreateProgramWithBinary")
#    pragma comment(linker,"/export:clRetainProgram=clrxclRetainProgram")
#    pragma comment(linker,"/export:clReleaseProgram=clrxclReleaseProgram")
#    pragma comment(linker,"/export:clBuildProgram=clrxclBuildProgram")
#    pragma comment(linker,"/export:clUnloadCompiler=clrxclUnloadCompiler")
#    pragma comment(linker,"/export:clGetProgramInfo=clrxclGetProgramInfo")
#    pragma comment(linker,"/export:clGetProgramBuildInfo=clrxclGetProgramBuildInfo")
#    pragma comment(linker,"/export:clCreateKernel=clrxclCreateKernel")
#    pragma comment(linker,"/export:clCreateKernelsInProgram=clrxclCreateKernelsInProgram")
#    pragma comment(linker,"/export:clRetainKernel=clrxclRetainKernel")
#    pragma comment(linker,"/export:clReleaseKernel=clrxclReleaseKernel")
#    pragma comment(linker,"/export:clSetKernelArg=clrxclSetKernelArg")
#    pragma comment(linker,"/export:clGetKernelInfo=clrxclGetKernelInfo")
#    pragma comment(linker,"/export:clGetKernelWorkGroupInfo=clrxclGetKernelWorkGroupInfo")
#  endif
#else
CLRX_CL_PUBLIC_SYM(clGetPlatformIDs)
CLRX_CL_PUBLIC_SYM(clIcdGetPlatformIDsKHR)
CLRX_CL_PUBLIC_SYM(clGetPlatformInfo)
CLRX_CL_PUBLIC_SYM(clGetDeviceIDs)
CLRX_CL_PUBLIC_SYM(clGetDeviceInfo)
CLRX_CL_PUBLIC_SYM(clCreateContext)
CLRX_CL_PUBLIC_SYM(clCreateContextFromType)
CLRX_CL_PUBLIC_SYM(clRetainContext)
CLRX_CL_PUBLIC_SYM(clReleaseContext)
CLRX_CL_PUBLIC_SYM(clGetContextInfo)
CLRX_CL_PUBLIC_SYM(clCreateCommandQueue)
CLRX_CL_PUBLIC_SYM(clRetainCommandQueue)
CLRX_CL_PUBLIC_SYM(clReleaseCommandQueue)
CLRX_CL_PUBLIC_SYM(clGetCommandQueueInfo)
CLRX_CL_PUBLIC_SYM(clSetCommandQueueProperty)
CLRX_CL_PUBLIC_SYM(clCreateBuffer)
CLRX_CL_PUBLIC_SYM(clCreateImage2D)
CLRX_CL_PUBLIC_SYM(clCreateImage3D)
CLRX_CL_PUBLIC_SYM(clRetainMemObject)
CLRX_CL_PUBLIC_SYM(clReleaseMemObject)
CLRX_CL_PUBLIC_SYM(clGetSupportedImageFormats)
CLRX_CL_PUBLIC_SYM(clGetMemObjectInfo)
CLRX_CL_PUBLIC_SYM(clGetImageInfo)
CLRX_CL_PUBLIC_SYM(clCreateSampler)
CLRX_CL_PUBLIC_SYM(clRetainSampler)
CLRX_CL_PUBLIC_SYM(clReleaseSampler)
CLRX_CL_PUBLIC_SYM(clGetSamplerInfo)
CLRX_CL_PUBLIC_SYM(clCreateProgramWithSource)
CLRX_CL_PUBLIC_SYM(clCreateProgramWithBinary)
CLRX_CL_PUBLIC_SYM(clRetainProgram)
CLRX_CL_PUBLIC_SYM(clReleaseProgram)
CLRX_CL_PUBLIC_SYM(clBuildProgram)
CLRX_CL_PUBLIC_SYM(clUnloadCompiler)
CLRX_CL_PUBLIC_SYM(clGetProgramInfo)
CLRX_CL_PUBLIC_SYM(clGetProgramBuildInfo)
CLRX_CL_PUBLIC_SYM(clCreateKernel)
CLRX_CL_PUBLIC_SYM(clCreateKernelsInProgram)
CLRX_CL_PUBLIC_SYM(clRetainKernel)
CLRX_CL_PUBLIC_SYM(clReleaseKernel)
CLRX_CL_PUBLIC_SYM(clSetKernelArg)
CLRX_CL_PUBLIC_SYM(clGetKernelInfo)
CLRX_CL_PUBLIC_SYM(clGetKernelWorkGroupInfo)
#endif

/* end of public API definitions */

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetPlatformIDs(cl_uint          num_entries,
                 cl_platform_id * platforms,
                 cl_uint *        num_platforms) CL_API_SUFFIX__VERSION_1_0
{
    CLRX_INITIALIZE
    
    if (num_entries == 0 && platforms != nullptr)
        return CL_INVALID_VALUE;
    if (platforms == nullptr && num_platforms == nullptr)
        return CL_INVALID_VALUE;
    
    if (!useCLRXWrapper) // ignore wrapper, returns original AMD platforms
    {
        if (amdOclGetPlatformIDs != nullptr)
            return amdOclGetPlatformIDs(num_entries, platforms, num_platforms);
        else
            clrxAbort("Very strange error: clGetPlatformIDs is not yet initialized!");
    }
    
    if (num_platforms != nullptr)
        *num_platforms = amdOclNumPlatforms;
    
    if (platforms != nullptr && clrxPlatforms != nullptr)
    {
        cl_uint toCopy = std::min(num_entries, amdOclNumPlatforms);
        for (cl_uint i = 0; i < toCopy; i++)
            platforms[i] = clrxPlatforms + i;
    }
    
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL clrxclIcdGetPlatformIDsKHR(
            cl_uint num_entries, 
            cl_platform_id *platforms, 
            cl_uint *num_platforms) CL_API_SUFFIX__VERSION_1_0
{
    cl_uint myNumPlatforms;
    const cl_int status = clrxclGetPlatformIDs(num_entries, platforms, &myNumPlatforms);
    
    if (status != CL_SUCCESS)
        return status;
    if (myNumPlatforms == 0)
        return CL_PLATFORM_NOT_FOUND_KHR;
    if (num_platforms != nullptr)
        *num_platforms = myNumPlatforms;
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL 
clrxclGetPlatformInfo(cl_platform_id   platform,
                  cl_platform_info param_name,
                  size_t           param_value_size,
                  void *           param_value,
                  size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    CLRX_INITIALIZE
    
    if (platform == nullptr)
        return CL_INVALID_PLATFORM;
    
    const CLRXPlatform* p = static_cast<const CLRXPlatform*>(platform);
    
    if (param_name != CL_PLATFORM_EXTENSIONS && param_name != CL_PLATFORM_VERSION)
        // if no extensions and version
        return p->amdOclPlatform->
                dispatch->clGetPlatformInfo(p->amdOclPlatform, param_name,
                    param_value_size, param_value, param_value_size_ret);
    else
    {
        // getting our extensions or platfom version strings
        if (param_value != nullptr)
        {
            if (param_name == CL_PLATFORM_EXTENSIONS)
            {
                if (param_value_size < p->extensionsSize)
                    return CL_INVALID_VALUE;
                ::memcpy(param_value, p->extensions.get(), p->extensionsSize);
            }
            else // CL_PLATFORM_VERSION
            {
                if (param_value_size < p->versionSize)
                    return CL_INVALID_VALUE;
                ::memcpy(param_value, p->version.get(), p->versionSize);
            }
        }
        
        if (param_value_size_ret != nullptr)
        {
            if (param_name == CL_PLATFORM_EXTENSIONS)
                *param_value_size_ret = p->extensionsSize;
            else // CL_PLATFORM_VERSION
                *param_value_size_ret = p->versionSize;
        }
        return CL_SUCCESS;
    }
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetDeviceIDs(cl_platform_id   platform,
               cl_device_type   device_type,
               cl_uint          num_entries,
               cl_device_id *   devices,
               cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0
{
    CLRX_INITIALIZE
    
    if (platform == nullptr)
        return CL_INVALID_PLATFORM;
    
    if (num_entries == 0 && devices != nullptr)
        return CL_INVALID_VALUE;
    if (devices == nullptr && num_devices == nullptr)
        return CL_INVALID_VALUE;
    
    CLRXPlatform* p = static_cast<CLRXPlatform*>(platform);
    try
    { callOnce(p->onceFlag, clrxPlatformInitializeDevices, p); }
    catch(const std::exception& ex)
    { clrxAbort("Fatal error at device initialization: ", ex.what()); }
    catch(...)
    { clrxAbort("Fatal and unknown error at device initialization"); }
    if (p->deviceInitStatus != CL_SUCCESS)
        return p->deviceInitStatus;
    
    /* real function */
    /* we get from original function and translate them,
     * function must returns devices in original order */
    cl_uint myNumDevices;
    const cl_int status = p->amdOclPlatform->dispatch->clGetDeviceIDs(
            p->amdOclPlatform, device_type, num_entries, devices, &myNumDevices);
    if (status == CL_DEVICE_NOT_FOUND)
    {
        if (num_devices != nullptr)
            *num_devices = 0;
        return status;
    }
    if (status != CL_SUCCESS)
        return status;
    
    if (devices != nullptr)
        translateAMDDevicesIntoCLRXDevices(p->devicesNum,
                const_cast<const CLRXDevice**>(p->devicePtrs.get()),
                std::min(myNumDevices, num_entries), devices);
    
    if (num_devices != nullptr)
        *num_devices = myNumDevices;
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetDeviceInfo(cl_device_id    device,
                cl_device_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (device == nullptr)
        return CL_INVALID_DEVICE;
    
    const CLRXDevice* d = static_cast<const CLRXDevice*>(device);
    switch(param_name)
    {
        case CL_DEVICE_EXTENSIONS:
            if (d->extensions == nullptr)
                return d->amdOclDevice->
                    dispatch->clGetDeviceInfo(d->amdOclDevice, param_name,
                        param_value_size, param_value, param_value_size_ret);
            else
            {
                // our extensions string size
                if (param_value != nullptr)
                {
                    if (param_value_size < d->extensionsSize)
                        return CL_INVALID_VALUE;
                    ::memcpy(param_value, d->extensions.get(), d->extensionsSize);
                }
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = d->extensionsSize;
            }
            break;
        case CL_DEVICE_VERSION:
            if (d->version == nullptr)
                return d->amdOclDevice->
                    dispatch->clGetDeviceInfo(d->amdOclDevice, param_name,
                        param_value_size, param_value, param_value_size_ret);
            else
            {
                // our version string size
                if (param_value != nullptr)
                {
                    if (param_value_size < d->versionSize)
                        return CL_INVALID_VALUE;
                    ::memcpy(param_value, d->version.get(), d->versionSize);
                }
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = d->versionSize;
            }
            break;
        case CL_DEVICE_PLATFORM:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_platform_id))
                    return CL_INVALID_VALUE;
                *static_cast<cl_platform_id*>(param_value) = d->platform;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_platform_id);
            break;
        case CL_DEVICE_PARENT_DEVICE_EXT:
#ifdef CL_VERSION_1_2
        case CL_DEVICE_PARENT_DEVICE:
#endif
            if (d->platform->openCLVersionNum >= getOpenCLVersionNum(1, 2) ||
                (d->platform->openCLVersionNum >= getOpenCLVersionNum(1, 1) &&
                 d->platform->dispatch->clCreateSubDevicesEXT != nullptr &&
                 param_name == CL_DEVICE_PARENT_DEVICE_EXT))
            {
                /* if OpenCL 1.2 or later or
                 * clCreateSubDevicesEXT is available and OpenCL 1.1 and
                 *      CL_DEVICE_PARENT_DEVICE_EXT is param_name */
                if (param_value != nullptr)
                {
                    if (param_value_size < sizeof(cl_device_id))
                        return CL_INVALID_VALUE;
                    *static_cast<cl_device_id*>(param_value) = d->parent;
                }
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = sizeof(cl_device_id);
            }
            else // if param_name is not available
                return CL_INVALID_VALUE;
            break;
        default:
            return d->amdOclDevice->
                dispatch->clGetDeviceInfo(d->amdOclDevice, param_name,
                    param_value_size, param_value, param_value_size_ret);
            break;
    }
    return CL_SUCCESS;
}

CL_API_ENTRY cl_context CL_API_CALL
clrxclCreateContext(const cl_context_properties * properties,
                cl_uint                 num_devices,
                const cl_device_id *    devices,
                void (CL_CALLBACK * pfn_notify)(const char *, const void *, size_t, void *),
                void *                  user_data,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    CLRX_INITIALIZE_OBJ
    
    if (devices == nullptr || num_devices == 0)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_VALUE;
        return nullptr;
    }
    
    CLRXPlatform* platform = clrxPlatforms; // choose first AMD platform
    bool doTranslateProps = false;
    size_t propNums = 0;
    
    cl_context amdContext = nullptr;
    CLRXDevice* d = static_cast<CLRXDevice*>(devices[0]);
    try
    {
        std::vector<cl_context_properties> amdProps;
        /* translate props if needed (translate platform to amdocl platforms) */
        if (properties != nullptr)
        {
            for (const cl_context_properties* p = properties; *p != 0; p+=2)
            {
                if (*p == CL_CONTEXT_PLATFORM)
                    doTranslateProps = true;
                propNums+=2;
            }
            
            if (doTranslateProps)
            {
                amdProps.resize(propNums+1);
                // translate platforms in CL_CONTEXT_PLATOFRM properties
                for (size_t i = 0; i < propNums; i+=2)
                {
                    amdProps[i] = properties[i];
                    if (properties[i] == CL_CONTEXT_PLATFORM)
                    {
                        /* get original AMD OpenCL platform */
                        if (properties[i+1] != 0)
                            amdProps[i+1] = (cl_context_properties)
                                (((CLRXPlatform*)(properties[i+1]))->amdOclPlatform);
                        else
                            amdProps[i+1] = 0;
                        platform = (CLRXPlatform*)(properties[i+1]);
                    }
                    else // if other
                        amdProps[i+1] = properties[i+1];
                }
                amdProps[propNums] = 0;
            }
        }
        
        const cl_context_properties* amdPropsPtr = properties;
        if (doTranslateProps) // replace by original
            amdPropsPtr = amdProps.data();
        if (platform == nullptr) // fallback
            platform = clrxPlatforms;
    
        /* get amdocl devices */
        std::vector<cl_device_id> amdDevices(num_devices);
        
        // checking whehter devices is null
        for (cl_uint i = 0; i < num_devices; i++)
        {
            if (devices[i] == nullptr)
            {
                if (errcode_ret != nullptr)
                    *errcode_ret = CL_INVALID_DEVICE;
                return nullptr;
            }
            amdDevices[i] = static_cast<const CLRXDevice*>(devices[i])->amdOclDevice;
        }
    
        amdContext = d->amdOclDevice->dispatch->clCreateContext(amdPropsPtr,
                num_devices, amdDevices.data(), pfn_notify, user_data, errcode_ret);
    
        if (amdContext == nullptr)
            return nullptr;
    }
    catch(const std::bad_alloc& ex)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    
    cl_int error = CL_SUCCESS;
    /* create own context */
    std::unique_ptr<CLRXContext> outContext;
    try
    {
        outContext.reset(new CLRXContext);
        outContext->dispatch = platform->dispatch;
        outContext->amdOclContext = amdContext;
        outContext->openCLVersionNum = platform->openCLVersionNum;
        
        error = clrxSetContextDevices(outContext.get(), num_devices, devices);
        if (error == CL_SUCCESS)
        {
            if (properties != nullptr)
                outContext->properties.resize(propNums+1);
        }
    }
    catch(const std::bad_alloc& ex)
    { error = CL_OUT_OF_HOST_MEMORY; }
    
    if (error != CL_SUCCESS)
    {
        // release context
        if (d->amdOclDevice->dispatch->clReleaseContext(amdContext) != CL_SUCCESS)
            clrxAbort("Fatal Error at handling error at context creation!");
        if (errcode_ret != nullptr)
            *errcode_ret = error;
        return nullptr;
    }
    
    if (properties != nullptr)
        std::copy(properties, properties + propNums+1, outContext->properties.begin());
    
    for (cl_uint i = 0; i < outContext->devicesNum; i++)
        clrxRetainOnlyCLRXDevice(static_cast<CLRXDevice*>(outContext->devices[i]));
    
    return outContext.release();
}

CL_API_ENTRY cl_context CL_API_CALL
clrxclCreateContextFromType(const cl_context_properties * properties,
                    cl_device_type          device_type,
        void (CL_CALLBACK *     pfn_notify )(const char *, const void *, size_t, void *),
                    void *                  user_data,
                    cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    CLRX_INITIALIZE_OBJ
    
    CLRXPlatform* platform = clrxPlatforms; // choose first AMD platform
    
    bool doTranslateProps = false;
    size_t propNums = 0;
    cl_context amdContext = nullptr;
    try
    {
        std::vector<cl_context_properties> amdProps;
        /* translate props if needed  (translate platform to amdocl platforms) */
        if (properties != nullptr)
        {
            for (const cl_context_properties* p = properties; *p != 0; p+=2)
            {
                if (*p == CL_CONTEXT_PLATFORM)
                    doTranslateProps = true;
                propNums+=2;
            }
            
            if (doTranslateProps)
            {
                amdProps.resize(propNums+1);
                // translate platforms in CL_CONTEXT_PLATOFRM properties
                for (size_t i = 0; i < propNums; i+=2)
                {
                    amdProps[i] = properties[i];
                    if (properties[i] == CL_CONTEXT_PLATFORM)
                    {
                        /* get original AMD OpenCL platform */
                        if (properties[i+1] != 0)
                            amdProps[i+1] = (cl_context_properties)
                                (((CLRXPlatform*)(properties[i+1]))->amdOclPlatform);
                        else
                            amdProps[i+1] = 0;
                        platform = (CLRXPlatform*)(properties[i+1]);
                    }
                    else // if other
                        amdProps[i+1] = properties[i+1];
                }
                amdProps[propNums] = 0;
            }
        }
        
        const cl_context_properties* amdPropsPtr = properties;
        if (doTranslateProps) // replace by original
            amdPropsPtr = amdProps.data();
    
        if (platform == nullptr) // fallback
            platform = clrxPlatforms;
    
        amdContext = platform->amdOclPlatform->dispatch->clCreateContextFromType(
                    amdPropsPtr, device_type, pfn_notify, user_data, errcode_ret);
        
        if (amdContext == nullptr)
            return nullptr;
    }
    catch(const std::bad_alloc& ex)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    
    /* create own context */
    std::unique_ptr<CLRXContext> outContext;
    cl_int error = CL_SUCCESS;
    try
    { 
        outContext.reset(new CLRXContext);
        outContext->dispatch = platform->dispatch;
        outContext->amdOclContext = amdContext;
        outContext->openCLVersionNum = platform->openCLVersionNum;
        // call once platform initialization (initialize devices)
        try
        { callOnce(platform->onceFlag, clrxPlatformInitializeDevices, platform); }
        catch(const std::exception& ex)
        { clrxAbort("Fatal error at device initialization: ", ex.what()); }
        catch(...)
        { clrxAbort("Fatal and unknown error at device initialization"); }
        error = platform->deviceInitStatus;
        
        if (error == CL_SUCCESS)
            error = clrxSetContextDevices(outContext.get(), platform);
        if (error == CL_SUCCESS)
        {
            if (properties != nullptr)
                outContext->properties.resize(propNums+1);
        }
    }
    catch(const std::bad_alloc& ex)
    { error = CL_OUT_OF_HOST_MEMORY; }
    
    if (error != CL_SUCCESS)
    {
        // release context
        if (platform->amdOclPlatform->dispatch->clReleaseContext(amdContext) != CL_SUCCESS)
            clrxAbort("Fatal Error at handling error at context creation!");
        if (errcode_ret != nullptr)
            *errcode_ret = error;
        return nullptr;
    }
    
    if (properties != nullptr)
        std::copy(properties, properties + propNums+1, outContext->properties.begin());
    
    for (cl_uint i = 0; i < outContext->devicesNum; i++)
        clrxRetainOnlyCLRXDevice(static_cast<CLRXDevice*>(outContext->devices[i]));
    
    return outContext.release();
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
        return CL_INVALID_CONTEXT;
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_int status = c->amdOclContext->dispatch->clRetainContext(c->amdOclContext);
    if (status == CL_SUCCESS)
        c->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
        return CL_INVALID_CONTEXT;
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_int status = c->amdOclContext->dispatch->clReleaseContext(c->amdOclContext);
    if (status == CL_SUCCESS)
        clrxReleaseOnlyCLRXContext(c);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetContextInfo(cl_context         context, 
                 cl_context_info    param_name,
                 size_t             param_value_size,
                 void *             param_value, 
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
        return CL_INVALID_CONTEXT;
    
    const CLRXContext* c = static_cast<const CLRXContext*>(context);
    switch(param_name)
    {
        case CL_CONTEXT_NUM_DEVICES:
            if (c->openCLVersionNum >= getOpenCLVersionNum(1, 1))
            {
                if (param_value != nullptr)
                {
                    if (param_value_size < sizeof(cl_uint))
                        return CL_INVALID_VALUE;
                    *static_cast<cl_uint*>(param_value) = c->devicesNum;
                }
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = sizeof(cl_uint);
            }
            else // if not available for OpenCL 1.0
                return CL_INVALID_VALUE;
            break;
        case CL_CONTEXT_DEVICES:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_device_id)*c->devicesNum)
                    return CL_INVALID_VALUE;
                std::copy(c->devices.get(), c->devices.get() + c->devicesNum,
                          static_cast<cl_device_id*>(param_value));
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_device_id)*c->devicesNum;
            break;
        case CL_CONTEXT_PROPERTIES:
            if (c->properties.empty())
            {
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = 0;
            }
            else
            {
                const size_t propElems = c->properties.size();
                if (param_value != nullptr)
                {
                    if (param_value_size < sizeof(cl_context_properties)*propElems)
                        return CL_INVALID_VALUE;
                    std::copy(c->properties.begin(), c->properties.end(),
                              static_cast<cl_context_properties*>(param_value));
                }
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = sizeof(cl_context_properties)*propElems;
            }
            break;
        default:
            return c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
                    param_name, param_value_size, param_value, param_value_size_ret);
    }
    return CL_SUCCESS;
}

CL_API_ENTRY cl_command_queue CL_API_CALL
clrxclCreateCommandQueue(cl_context                     context, 
                     cl_device_id                   device, 
                     cl_command_queue_properties    properties,
                     cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    if (device == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_DEVICE;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    CLRXDevice* d = static_cast<CLRXDevice*>(device);
    const cl_command_queue amdCmdQueue = c->amdOclContext->dispatch->clCreateCommandQueue(
            c->amdOclContext, d->amdOclDevice, properties, errcode_ret);
    
    if (amdCmdQueue == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXCommandQueue, amdOclCommandQueue, amdCmdQueue,
              clReleaseCommandQueue,
              "Fatal Error at handling error at command queue creation!")
    outObject->device = static_cast<CLRXDevice*>(device);
    
    clrxRetainOnlyCLRXDevice(static_cast<CLRXDevice*>(device));
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const cl_int status = q->amdOclCommandQueue->dispatch->
            clRetainCommandQueue(q->amdOclCommandQueue);
    if (status == CL_SUCCESS)
        q->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const cl_int status = q->amdOclCommandQueue->dispatch->
            clReleaseCommandQueue(q->amdOclCommandQueue);
    if (status == CL_SUCCESS)
        clrxReleaseOnlyCLRXCommandQueue(q);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetCommandQueueInfo(cl_command_queue      command_queue,
              cl_command_queue_info param_name,
              size_t                param_value_size,
              void *                param_value,
              size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    switch(param_name)
    {
        case CL_QUEUE_CONTEXT:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_context))
                    return CL_INVALID_VALUE;
                *static_cast<cl_context*>(param_value) = q->context;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_context);
            break;
        case CL_QUEUE_DEVICE:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_device_id))
                    return CL_INVALID_VALUE;
                *static_cast<cl_device_id*>(param_value) = q->device;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_device_id);
            break;
        default:
            return q->amdOclCommandQueue->dispatch->clGetCommandQueueInfo(
                    q->amdOclCommandQueue, param_name, param_value_size, param_value,
                    param_value_size_ret);
    }
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL clrxclSetCommandQueueProperty(
    cl_command_queue              command_queue,
    cl_command_queue_properties   properties, 
    cl_bool                       enable,
    cl_command_queue_properties * old_properties) CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    return q->amdOclCommandQueue->dispatch->clSetCommandQueueProperty(
            q->amdOclCommandQueue, properties, enable, old_properties);
}

CL_API_ENTRY cl_mem CL_API_CALL
clrxclCreateBuffer(cl_context   context,
               cl_mem_flags flags,
               size_t       size,
               void *       host_ptr,
               cl_int *     errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdBuffer = c->amdOclContext->dispatch->clCreateBuffer(c->amdOclContext,
              flags, size, host_ptr, errcode_ret);
    
    if (amdBuffer == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdBuffer,
              clReleaseMemObject,
              "Fatal Error at handling error at buffer creation!")
    
    return outObject;
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_mem CL_API_CALL
clrxclCreateImage2D(cl_context              context,
            cl_mem_flags             flags,
            const cl_image_format * image_format,
            size_t                  image_width,
            size_t                  image_height,
            size_t                  image_row_pitch,
            void *                  host_ptr,
            cl_int *                errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdImage = c->amdOclContext->dispatch->clCreateImage2D(c->amdOclContext,
              flags, image_format, image_width, image_height, image_row_pitch,
              host_ptr, errcode_ret);
    
    if (amdImage == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdImage,
              clReleaseMemObject,
              "Fatal Error at handling error at image2d creation!")
    
    return outObject;
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_mem CL_API_CALL
clrxclCreateImage3D(cl_context              context,
            cl_mem_flags            flags,
            const cl_image_format * image_format,
            size_t                  image_width, 
            size_t                  image_height,
            size_t                  image_depth, 
            size_t                  image_row_pitch, 
            size_t                  image_slice_pitch, 
            void *                  host_ptr,
            cl_int *                errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdImage = c->amdOclContext->dispatch->clCreateImage3D(c->amdOclContext,
              flags, image_format, image_width, image_height, image_depth,
              image_row_pitch, image_slice_pitch, host_ptr, errcode_ret);
    
    if (amdImage == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdImage,
              clReleaseMemObject,
              "Fatal Error at handling error at image3d creation!")
    
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
    if (memobj == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    CLRXMemObject* m = static_cast<CLRXMemObject*>(memobj);
    const cl_int status = m->amdOclMemObject->dispatch->clRetainMemObject(
        m->amdOclMemObject);
    if (status == CL_SUCCESS)
        m->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
    if (memobj == nullptr)
        return CL_INVALID_MEM_OBJECT;
    CLRXMemObject* m = static_cast<CLRXMemObject*>(memobj);
    const cl_int status = m->amdOclMemObject->dispatch->clReleaseMemObject(
                m->amdOclMemObject);
    if (status == CL_SUCCESS)
        clrxReleaseOnlyCLRXMemObject(m);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetSupportedImageFormats(cl_context           context,
                   cl_mem_flags         flags,
                   cl_mem_object_type   image_type,
                   cl_uint              num_entries,
                   cl_image_format *    image_formats,
                   cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0

{
    if (context == nullptr)
        return CL_INVALID_CONTEXT;
    const CLRXContext* c = static_cast<const CLRXContext*>(context);
    return c->amdOclContext->dispatch->clGetSupportedImageFormats(c->amdOclContext, flags,
                   image_type, num_entries, image_formats, num_image_formats);
}                          

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetMemObjectInfo(cl_mem           memobj,
                   cl_mem_info      param_name, 
                   size_t           param_value_size,
                   void *           param_value,
                   size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (memobj == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(memobj);
    switch(param_name)
    {
        case CL_MEM_CONTEXT:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_context))
                    return CL_INVALID_VALUE;
                *static_cast<cl_context*>(param_value) = m->context;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_context);
            break;
        case CL_MEM_ASSOCIATED_MEMOBJECT:
        {
            /* check what is returned and translate it */
            if (m->context->openCLVersionNum < getOpenCLVersionNum(1, 1))
                return CL_INVALID_VALUE; /* if OpenCL 1.0 */
            
            cl_int status = m->amdOclMemObject->
                dispatch->clGetMemObjectInfo(m->amdOclMemObject, param_name,
                    param_value_size, param_value, param_value_size_ret);
            if (status != CL_SUCCESS)
                return status;
            if (param_value != nullptr)
            {
                cl_mem* outMem = static_cast<cl_mem*>(param_value);
                if (*outMem != nullptr)
                {
                    // fix output memobject
                    if (m->parent != nullptr && m->parent->amdOclMemObject != nullptr)
                        *outMem = m->parent;
                    else if (m->buffer != nullptr && m->buffer->amdOclMemObject != nullptr)
                        *outMem = m->buffer;
                    else
                        clrxAbort("Stupid AMD drivers "
                                "(because returns invalid assocMemObject)!");
                }
            }
            break;
        }
        default:
            return m->amdOclMemObject->
                dispatch->clGetMemObjectInfo(m->amdOclMemObject, param_name,
                    param_value_size, param_value, param_value_size_ret);
    }
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetImageInfo(cl_mem           image,
               cl_image_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (image == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(image);
#ifdef CL_VERSION_1_2
    if (param_name != CL_IMAGE_BUFFER)
#endif
        return m->amdOclMemObject->dispatch->clGetImageInfo(m->amdOclMemObject, param_name,
                param_value_size, param_value, param_value_size_ret);
#ifdef CL_VERSION_1_2
    else if (m->context->openCLVersionNum >= getOpenCLVersionNum(1, 2))
    {
        /* only if OpenCL 1.2 or later */
        if (param_value != nullptr)
        {
            if (param_value_size < sizeof(cl_mem))
                return CL_INVALID_VALUE;
            *static_cast<cl_mem*>(param_value) = m->buffer;
        }
        if (param_value_size_ret != nullptr)
            *param_value_size_ret = sizeof(cl_mem);
        return CL_SUCCESS;
    }
    else
        // CL_IMAGE_BUFFER is unsupported by earlier OpenCL version
        return CL_INVALID_VALUE;
#endif
}

CL_API_ENTRY cl_sampler CL_API_CALL
clrxclCreateSampler(cl_context          context,
                cl_bool             normalized_coords,
                cl_addressing_mode  addressing_mode,
                cl_filter_mode      filter_mode,
                cl_int *            errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_sampler amdSampler = c->amdOclContext->dispatch->clCreateSampler(
            c->amdOclContext, normalized_coords, addressing_mode, filter_mode, errcode_ret);
    
    if (amdSampler == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXSampler, amdOclSampler, amdSampler,
              clReleaseSampler,
              "Fatal Error at handling error at sampler creation!")
    
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0
{
    if (sampler == nullptr)
        return CL_INVALID_SAMPLER;
    
    CLRXSampler* s = static_cast<CLRXSampler*>(sampler);
    const cl_int status = s->amdOclSampler->dispatch->clRetainSampler(s->amdOclSampler);
    if (status == CL_SUCCESS)
        s->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0
{
    if (sampler == nullptr)
        return CL_INVALID_SAMPLER;
    CLRXSampler* s = static_cast<CLRXSampler*>(sampler);
    const cl_int status = s->amdOclSampler->dispatch->clReleaseSampler(s->amdOclSampler);
    if (status == CL_SUCCESS)
        if (s->refCount.fetch_sub(1) == 1)
        {
            clrxReleaseOnlyCLRXContext(s->context);
            delete s;
        }
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetSamplerInfo(cl_sampler         sampler,
                 cl_sampler_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (sampler == nullptr)
        return CL_INVALID_SAMPLER;
    const CLRXSampler* s = static_cast<const CLRXSampler*>(sampler);
    if (param_name != CL_SAMPLER_CONTEXT)
        // if no extensions (or not changed) and platform
        return s->amdOclSampler->
                dispatch->clGetSamplerInfo(s->amdOclSampler, param_name,
                    param_value_size, param_value, param_value_size_ret);
    else // 
    {
        if (param_value != nullptr)
        {
            if (param_value_size < sizeof(cl_context))
                return CL_INVALID_VALUE;
            *static_cast<cl_context*>(param_value) = s->context;
        }
        if (param_value_size_ret != nullptr)
            *param_value_size_ret = sizeof(cl_context);
        return CL_SUCCESS;
    }
}

CL_API_ENTRY cl_program CL_API_CALL
clrxclCreateProgramWithSource(cl_context        context,
                          cl_uint           count,
                          const char **     strings,
                          const size_t *    lengths,
                          cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_program amdProgram = c->amdOclContext->dispatch->clCreateProgramWithSource(
            c->amdOclContext, count, strings, lengths, errcode_ret);
    
    if (amdProgram == nullptr)
        return nullptr;
    
    return clrxCreateCLRXProgram(c, amdProgram, errcode_ret);
}

CL_API_ENTRY cl_program CL_API_CALL
clrxclCreateProgramWithBinary(cl_context                     context,
              cl_uint                        num_devices,
              const cl_device_id *           device_list,
              const size_t *                 lengths,
              const unsigned char **         binaries,
              cl_int *                       binary_status,
              cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    if (num_devices == 0 || device_list == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_VALUE;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    cl_program amdProgram = nullptr;
    try
    {
        std::vector<cl_device_id> amdDevices(num_devices);
        /* get original devices before calling */
        for (cl_uint i = 0; i < num_devices; i++)
        {
            if (device_list[i] == nullptr)
            {
                if (errcode_ret != nullptr)
                    *errcode_ret = CL_INVALID_DEVICE;
                return nullptr;
            }
            amdDevices[i] = static_cast<const CLRXDevice*>(device_list[i])->amdOclDevice;
        }
        
        amdProgram = c->amdOclContext->dispatch->clCreateProgramWithBinary(
            c->amdOclContext, num_devices, amdDevices.data(), lengths, binaries,
                       binary_status, errcode_ret);
    }
    catch(const std::bad_alloc& ex)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    
    if (amdProgram == nullptr)
        return nullptr;
    
    return clrxCreateCLRXProgram(c, amdProgram, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
    if (program == nullptr)
        return CL_INVALID_PROGRAM;
    
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    const cl_int status = p->amdOclProgram->dispatch->clRetainProgram(p->amdOclProgram);
    if (status == CL_SUCCESS)
        p->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
    if (program == nullptr)
        return CL_INVALID_PROGRAM;
    CLRXProgram * p = static_cast<CLRXProgram*>(program);
    
    const cl_int status = p->amdOclProgram->dispatch->clReleaseProgram(p->amdOclProgram);
    bool doDelete = false;
    if (status == CL_SUCCESS)
        try
        {
            std::lock_guard<std::mutex> lock(p->mutex); // lock if assocDevices is changed
            if (p->refCount.fetch_sub(1) == 1)
                doDelete = true;
        }
        catch(const std::exception& ex)
        { clrxAbort("Fatal exception happened: ", ex.what()); }
    if (doDelete)
    {
        clrxReleaseOnlyCLRXContext(p->context);
        delete p;
    }
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclBuildProgram(cl_program           program,
               cl_uint              num_devices,
               const cl_device_id * device_list,
               const char *         options, 
       void (CL_CALLBACK *  pfn_notify)(cl_program /* program */, void * /* user_data */),
               void *               user_data) CL_API_SUFFIX__VERSION_1_0
{
    if (program == nullptr)
        return CL_INVALID_PROGRAM;
    
    if ((num_devices != 0 && device_list == nullptr) ||
        (num_devices == 0 && device_list != nullptr))
        return CL_INVALID_VALUE;
    
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    if (options!=nullptr && detectCLRXCompilerCall(options))
    try
    {   {
            std::lock_guard<std::mutex> lock(p->mutex);
            if (p->kernelsAttached != 0) // if kernels attached
                return CL_INVALID_OPERATION;
            p->concurrentBuilds++;
            p->kernelArgFlagsInitialized = false;
        }
        // call own compiler
        cl_int error = clrxCompilerCall(p, options, num_devices,
                            (CLRXDevice* const*)device_list);
        if (pfn_notify!=nullptr)
            pfn_notify(program, user_data);
        
        {
            std::lock_guard<std::mutex> lock(p->mutex);
            p->concurrentBuilds--;
        }
        return error;
    }
    catch(const std::exception& ex)
    {
        clrxAbort("Fatal exception happened: ", ex.what());
        return -1;
    }
    
    bool doDeleteWrappedData = true;
    void* destUserData = user_data;
    CLRXBuildProgramUserData* wrappedData = nullptr;
    void (CL_CALLBACK *  notifyToCall)(cl_program, void *) =  nullptr;
    try
    {
        // catching system_error
    
    if (pfn_notify != nullptr)
    {
        // prepare to call wrapper for notify
        wrappedData = new CLRXBuildProgramUserData;
        wrappedData->realNotify = pfn_notify;
        wrappedData->clrxProgram = p;
        wrappedData->realUserData = user_data;
        wrappedData->callDone = false;
        wrappedData->inClFunction = false;
        destUserData = wrappedData;
        //std::cout << "WrappedData: " << wrappedData << std::endl;
        notifyToCall = clrxBuildProgramNotifyWrapper;
    }
    
    {
        std::lock_guard<std::mutex> lock(p->mutex);
        clrxClearProgramAsmState(p);
        
        if (p->kernelsAttached != 0) // if kernels attached
            return CL_INVALID_OPERATION;
        p->concurrentBuilds++;
        p->kernelArgFlagsInitialized = false;
    }
    
    cl_int status;
    if (num_devices == 0)
        status = p->amdOclProgram->dispatch->clBuildProgram(p->amdOclProgram, 0, nullptr,
                    options, notifyToCall, destUserData);
    else
    {
        try
        {
            std::vector<cl_device_id> amdDevices(num_devices);
            // get original AMD devices before calling
            for (cl_uint i = 0; i < num_devices; i++)
            {
                if (device_list[i] == nullptr)
                {
                    // if bad device
                    std::lock_guard<std::mutex> lock(p->mutex);
                    clrxReleaseConcurrentBuild(p);
                    delete wrappedData;
                    return CL_INVALID_DEVICE;
                }
                amdDevices[i] =
                    static_cast<const CLRXDevice*>(device_list[i])->amdOclDevice;
            }
            
            clrxInitProgramTransDevicesMap(p, num_devices, device_list, amdDevices);
            
            status = p->amdOclProgram->dispatch->clBuildProgram(
                    p->amdOclProgram, num_devices, amdDevices.data(), options,
                             notifyToCall, destUserData);
        }
        catch(std::bad_alloc& ex)
        {
            // if allocation failed
            std::lock_guard<std::mutex> lock(p->mutex);
            clrxReleaseConcurrentBuild(p);
            delete wrappedData;
            return CL_OUT_OF_HOST_MEMORY;
        }
    }
    /* after this we update associated devices and decrease concurrentBuilds in
     * in this place or in pfn_notify call */
    
    std::lock_guard<std::mutex> lock(p->mutex);
    // determine whether wrapped must deleted when out of memory happened
    // delete wrappedData if clBuildProgram not finished successfully
    // or callback is called
    doDeleteWrappedData = wrappedData != nullptr &&
            (status != CL_SUCCESS || wrappedData->callDone);
    
    if (wrappedData == nullptr || !wrappedData->callDone)
    {
        // do it if callback not called
        if (status != CL_INVALID_DEVICE)
        {
            const cl_int newStatus = clrxUpdateProgramAssocDevices(p);
            if (newStatus != CL_SUCCESS)
                status = newStatus;
        }
        if (wrappedData != nullptr)
            wrappedData->inClFunction = true;
        clrxReleaseConcurrentBuild(p);
    }
    // delete wrappedData if clBuildProgram not finished successfully
    // or callback is called
    if (doDeleteWrappedData)
    {
        //std::cout << "Delete WrappedData: " << wrappedData << std::endl;
        delete wrappedData;
        wrappedData = nullptr;
    }
    return status;
    }
    catch(const std::bad_alloc& ex)
    {
        if (doDeleteWrappedData)
        {
            //std::cout << "Delete WrappedData: " << wrappedData << std::endl;
            delete wrappedData;
        }
        return CL_OUT_OF_HOST_MEMORY;
    }
    catch(const std::exception& ex)
    {
        clrxAbort("Fatal exception happened: ", ex.what());
        return -1;
    }
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clrxclUnloadCompiler(void) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    CLRX_INITIALIZE
    
    if (amdOclUnloadCompiler != nullptr)
        return amdOclUnloadCompiler();
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetProgramInfo(cl_program         program,
                 cl_program_info    param_name,
                 size_t             param_value_size,
                 void *             param_value,
                 size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (program == nullptr)
        return CL_INVALID_PROGRAM;
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    
    try
    {
    switch(param_name)
    {
        case CL_PROGRAM_CONTEXT:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_context))
                    return CL_INVALID_VALUE;
                *static_cast<cl_context*>(param_value) = p->context;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_context);
            break;
        case CL_PROGRAM_NUM_DEVICES:
        {
            std::lock_guard<std::mutex> lock(p->mutex);
            if (p->concurrentBuilds != 0)
            {
                // update only when concurrent builds is working
                const cl_int status = clrxUpdateProgramAssocDevices(p);
                if (status != CL_SUCCESS)
                    return status;
            }
            
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_uint))
                    return CL_INVALID_VALUE;
                *static_cast<cl_uint*>(param_value) = p->assocDevicesNum;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_uint);
        }
            break;
        case CL_PROGRAM_DEVICES:
        {
            std::lock_guard<std::mutex> lock(p->mutex);
            if (p->concurrentBuilds != 0)
            {
                // update only when concurrent builds is working
                const cl_int status = clrxUpdateProgramAssocDevices(p);
                if (status != CL_SUCCESS)
                    return status;
            }
            
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_device_id)*p->assocDevicesNum)
                    return CL_INVALID_VALUE;
                std::copy(p->assocDevices.get(), p->assocDevices.get() + p->assocDevicesNum,
                        static_cast<cl_device_id*>(param_value));
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_device_id)*p->assocDevicesNum;
        }
            break;
#ifdef CL_VERSION_1_2
        case CL_PROGRAM_NUM_KERNELS:
        case CL_PROGRAM_KERNEL_NAMES:
        {
            if (p->context->openCLVersionNum < getOpenCLVersionNum(1, 2))
                return CL_INVALID_VALUE;
            cl_program prog;
            // getting CLRX assembler program state
            {
                std::lock_guard<std::mutex> lock(p->mutex);
                CLRXAsmState asmState = p->asmState.load();
                // getting program (for assembler code, real CL asm program)
                prog = (asmState != CLRXAsmState::NONE) ? p->amdOclAsmProgram :
                            p->amdOclProgram;
                if (asmState != CLRXAsmState::NONE && prog==nullptr)
                    return CL_INVALID_PROGRAM_EXECUTABLE;
            }
            return p->amdOclProgram->
                dispatch->clGetProgramInfo(prog, param_name,
                        param_value_size, param_value, param_value_size_ret);
        }
#endif
        case CL_PROGRAM_BINARY_SIZES:
        {
            std::lock_guard<std::mutex> lock(p->mutex);
            if (p->asmState.load()!=CLRXAsmState::NONE)
            {
                // get this info from clrx asm programs
                size_t expectedSize = sizeof(size_t)*p->assocDevicesNum;
                if (param_value!=nullptr)
                {
                    if (param_value_size < expectedSize)
                        return CL_INVALID_VALUE;
                    size_t outSize = 0;
                    if (p->amdOclAsmProgram!=nullptr)
                    {
                        cl_int error = p->amdOclProgram->dispatch->clGetProgramInfo(
                                    p->amdOclAsmProgram, param_name,
                                    param_value_size, param_value, &outSize);
                        if (error != CL_SUCCESS)
                            return error;
                    }
                    /// zeroing entries for failed devices
                    if (outSize < expectedSize && param_value!=nullptr)
                        ::memset((char*)param_value+outSize, 0, expectedSize-outSize);
                }
                /// set up output size
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = expectedSize;
            }
            else
                return p->amdOclProgram->dispatch->clGetProgramInfo(p->amdOclProgram,
                        param_name, param_value_size, param_value, param_value_size_ret);
        }
            break;
        case CL_PROGRAM_BINARIES:
        {
            std::lock_guard<std::mutex> lock(p->mutex);
            if (p->asmState.load()!=CLRXAsmState::NONE)
            {
                // get this info from clrx asm programs
                size_t expectedSize = sizeof(unsigned char*)*p->assocDevicesNum;
                if (param_value!=nullptr)
                {
                    if (param_value_size < expectedSize)
                        return CL_INVALID_VALUE;
                    if (p->amdOclAsmProgram!=nullptr)
                    {
                        cl_int error = p->amdOclProgram->dispatch->clGetProgramInfo(
                                p->amdOclAsmProgram, param_name,
                                param_value_size, param_value, nullptr);
                        if (error != CL_SUCCESS)
                            return error;
                    }
                }
                /// set up output size
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = expectedSize;
            }
            else
                return p->amdOclProgram->dispatch->clGetProgramInfo(p->amdOclProgram,
                        param_name, param_value_size, param_value, param_value_size_ret);
        }
            break;
        case CL_PROGRAM_SOURCE: // always from original impl
        case CL_PROGRAM_REFERENCE_COUNT:
            return p->amdOclProgram->
                dispatch->clGetProgramInfo(p->amdOclProgram, param_name,
                    param_value_size, param_value, param_value_size_ret);
        default:
            /* check whether second program (with asm binaries) is available */
            cl_program prog;
            {
                std::lock_guard<std::mutex> lock(p->mutex);
                prog = (p->asmState.load()!=CLRXAsmState::NONE) ? p->amdOclAsmProgram :
                            p->amdOclProgram;
            }
            return p->amdOclProgram->
                dispatch->clGetProgramInfo(prog, param_name,
                        param_value_size, param_value, param_value_size_ret);
    }
    return CL_SUCCESS;
    }
    catch(const std::exception& ex)
    {
        clrxAbort("Fatal exception happened: ", ex.what());
        return -1;
    }
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetProgramBuildInfo(cl_program            program,
              cl_device_id          device,
              cl_program_build_info param_name,
              size_t                param_value_size,
              void *                param_value,
              size_t *              param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (program == nullptr)
        return CL_INVALID_PROGRAM;
    if (device == nullptr)
        return CL_INVALID_DEVICE;
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    const CLRXDevice* d = static_cast<const CLRXDevice*>(device);
    
    try
    {
    std::unique_lock<std::mutex> lock(p->mutex);
    if (p->asmState.load() == CLRXAsmState::NONE)
    {
        // if no assembler program then get info from amdOCL program
        lock.unlock();
        return p->amdOclProgram->dispatch->clGetProgramBuildInfo(p->amdOclProgram,
                d->amdOclDevice, param_name, param_value_size, param_value,
                param_value_size_ret);
    }
    
    // get iterator to pprogram device info structure
    ProgDeviceMapEntry* progDevIt = nullptr;
    if (p->asmProgEntries!=nullptr)
    {
        progDevIt = CLRX::binaryMapFind(p->asmProgEntries.get(),
                 p->asmProgEntries.get()+p->assocDevicesNum, device);
        if (progDevIt == p->asmProgEntries.get()+p->assocDevicesNum)
            return CL_INVALID_DEVICE;
    }
    else if (p->assocDevicesNum!=0)
    {
        // if not program entries
        if (std::find(p->assocDevices.get(), p->assocDevices.get()+p->assocDevicesNum,
                        device) == p->assocDevices.get()+p->assocDevicesNum)
            return CL_INVALID_DEVICE;
    }
    else // otherwise no associated devices
        return CL_INVALID_DEVICE;
    
    // get specified info from this prog device table entry
    switch(param_name)
    {
        case CL_PROGRAM_BUILD_STATUS:
        {
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_build_status))
                    return CL_INVALID_VALUE;
                if (p->asmProgEntries)
                    *reinterpret_cast<cl_build_status*>(param_value) =
                            progDevIt->second.status;
                else // if not
                    *reinterpret_cast<cl_build_status*>(param_value) =
                            (p->asmState.load() == CLRXAsmState::IN_PROGRESS) ?
                            CL_BUILD_IN_PROGRESS : CL_BUILD_NONE;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_build_status);
        }
            break;
        case CL_PROGRAM_BUILD_OPTIONS:
        {
            if (param_value != nullptr)
            {
                if (param_value_size < p->asmOptions.size()+1)
                    return CL_INVALID_VALUE;
                strcpy((char*)param_value, p->asmOptions.c_str());
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = p->asmOptions.size()+1;
        }
            break;
        case CL_PROGRAM_BUILD_LOG:
        {
            size_t logSize = (p->asmProgEntries && progDevIt->second.log) ?
                        progDevIt->second.log->log.size()+1 : 1;
            if (param_value != nullptr)
            {
                if (param_value_size < logSize)
                    return CL_INVALID_VALUE;
                if (p->asmProgEntries && progDevIt->second.log && logSize!=1)
                    strcpy((char*)param_value, progDevIt->second.log->log.c_str());
                else
                    ((cxbyte*)param_value)[0] = 0;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = logSize;
        }
            break;
        default:
            return p->amdOclProgram->dispatch->clGetProgramBuildInfo(p->amdOclAsmProgram,
                    d->amdOclDevice, param_name, param_value_size, param_value,
                    param_value_size_ret);
    }
    }
    catch(const std::exception& ex)
    { clrxAbort("Fatal exception happened: ", ex.what()); }
    return CL_SUCCESS;
}

CL_API_ENTRY cl_kernel CL_API_CALL
clrxclCreateKernel(cl_program      program,
               const char *    kernel_name,
               cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (program == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_PROGRAM;
        return nullptr;
    }
    
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    
    cl_kernel amdKernel = nullptr;
    CLRXKernel* outKernel = nullptr;
    try
    {
        std::lock_guard<std::mutex> l(p->mutex);
        if (p->concurrentBuilds != 0)
        {
            // creating kernel is not legal when building in progress
            if (errcode_ret != nullptr)
                *errcode_ret = CL_INVALID_PROGRAM_EXECUTABLE;
            return nullptr;
        }
        
        const cl_program kernProgram = (p->amdOclAsmProgram!=nullptr) ?
                p->amdOclAsmProgram : p->amdOclProgram;
        
        amdKernel = p->amdOclProgram->dispatch->clCreateKernel(kernProgram,
                   kernel_name, errcode_ret);
        if (amdKernel == nullptr)
            return nullptr;
        
        const cl_int status = clrxInitKernelArgFlagsMap(p);
        if (status != CL_SUCCESS)
        {
            if (p->amdOclProgram->dispatch->clReleaseKernel(amdKernel) != CL_SUCCESS)
                clrxAbort("Fatal Error at handling error at kernel creation!");
            if (errcode_ret != nullptr)
                *errcode_ret = status;
            return nullptr;
        }
        
        /* get argflagmap for this kernel (will be used to choose correct routine to
         * translate for  arguments */
        CLRXKernelArgFlagMap::const_iterator argFlagMapIt =
            CLRX::binaryMapFind(p->kernelArgFlagsMap.begin(), p->kernelArgFlagsMap.end(),
                              kernel_name);
        if (argFlagMapIt == p->kernelArgFlagsMap.end())
            clrxAbort("Can't find kernel arg flag!");
        outKernel = new CLRXKernel(argFlagMapIt->second);
        p->kernelsAttached++; // notify clBuildProgram about attached kernels
        
        // retain original program if is assembly program
        if (p->amdOclAsmProgram!=nullptr)
        {
            if (p->amdOclProgram->dispatch->clRetainProgram(p->amdOclProgram)!=CL_SUCCESS)
                clrxAbort("Fatal Error at retaining original program");
            outKernel->fromAsm = true;
        }
    }
    catch(const std::bad_alloc& ex)
    {
        if (amdKernel!=nullptr)
            if (p->amdOclProgram->dispatch->clReleaseKernel(amdKernel) != CL_SUCCESS)
                clrxAbort("Fatal Error at handling error at kernel creation!");
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    catch(const std::exception& ex)
    { clrxAbort("Fatal Error at handling error at kernel creation: ", ex.what()); }
    
    outKernel->dispatch = p->dispatch;
    outKernel->amdOclKernel = amdKernel;
    outKernel->program = p;
    
    clrxRetainOnlyCLRXProgram(p);
    return outKernel;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclCreateKernelsInProgram(cl_program     program,
                         cl_uint        num_kernels,
                         cl_kernel *    kernels,
                         cl_uint *      num_kernels_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (program == nullptr)
        return CL_INVALID_PROGRAM;

    CLRXProgram* p = static_cast<CLRXProgram*>(program);    
    
    cl_uint numKernelsOut = 0; // for numKernelsOut
    if (num_kernels_ret != nullptr)
        numKernelsOut = *num_kernels_ret; 
    
    /* replaces original kernels by our (CLRXKernels) */
    cl_uint kernelsToCreate = 0;
    cl_uint kp = 0; // kernel already processed
    std::unique_ptr<char[]> kernelName = nullptr;
    size_t maxKernelNameSize = 0;
    try
    {
        std::lock_guard<std::mutex> l(p->mutex);
        if (p->concurrentBuilds != 0)
            // creating kernel is not legal when building in progress
            return CL_INVALID_PROGRAM_EXECUTABLE;
        
        const cl_program kernProgram = (p->amdOclAsmProgram!=nullptr) ?
                p->amdOclAsmProgram : p->amdOclProgram;
        // call after checking concurrent buildings
        cl_int status = p->amdOclProgram->dispatch->clCreateKernelsInProgram(
                   kernProgram, num_kernels, kernels, &numKernelsOut);
        
        if (status != CL_SUCCESS)
            return status;
        
        kernelsToCreate = std::min(num_kernels, numKernelsOut);
        
        status = clrxInitKernelArgFlagsMap(p);
        if (status != CL_SUCCESS)
        {
            // free if error happened
            if (kernels != nullptr)
            {
                for (cl_uint i = 0; i < kernelsToCreate; i++)
                    if (p->amdOclProgram->dispatch->
                        clReleaseKernel(kernels[i]) != CL_SUCCESS)
                        clrxAbort("Fatal Error at handling error at kernel creation!");
            }
            return status;
        }
        
        if (kernels != nullptr)
        {
            for (kp = 0; kp < kernelsToCreate; kp++)
            {
                size_t kernelNameSize;
                cl_int status = kernels[kp]->dispatch->clGetKernelInfo(
                        kernels[kp], CL_KERNEL_FUNCTION_NAME, 0, nullptr, &kernelNameSize);
                if (status != CL_SUCCESS)
                    clrxAbort("Can't get kernel function name");
                if (kernelName == nullptr ||
                    (kernelName != nullptr && kernelNameSize > maxKernelNameSize))
                {
                    kernelName.reset(new char[kernelNameSize]);
                    maxKernelNameSize = kernelNameSize;
                }
                
                status = kernels[kp]->dispatch->clGetKernelInfo(kernels[kp],
                        CL_KERNEL_FUNCTION_NAME, kernelNameSize, kernelName.get(), nullptr);
                if (status != CL_SUCCESS)
                    clrxAbort("Can't get kernel function name");
                
                CLRXKernelArgFlagMap::const_iterator argFlagMapIt =
                    CLRX::binaryMapFind(p->kernelArgFlagsMap.begin(),
                        p->kernelArgFlagsMap.end(), kernelName.get());
                if (argFlagMapIt == p->kernelArgFlagsMap.end())
                    clrxAbort("Can't find kernel arg flag!");
                
                CLRXKernel* outKernel = new CLRXKernel(argFlagMapIt->second);
                outKernel->dispatch = p->dispatch;
                outKernel->amdOclKernel = kernels[kp];
                outKernel->program = p;
                kernels[kp] = outKernel;
            }
            // notify clBuildProgram about attached kernels
            p->kernelsAttached += kernelsToCreate;
            
            // retain original program if is assembly program
            if (p->amdOclAsmProgram!=nullptr)
            {
                for (cxuint k = 0; k < kernelsToCreate; k++)
                {
                    if (p->amdOclProgram->dispatch->clRetainProgram(
                                    p->amdOclProgram)!=CL_SUCCESS)
                        clrxAbort("Fatal Error at retaining original program");
                    reinterpret_cast<CLRXKernel*>(kernels[k])->fromAsm = true;
                }
            }
        }
    }
    catch(const std::bad_alloc& ex)
    {
        if (kernels != nullptr)
        {
            // already processed kernels
            for (cl_uint i = 0; i < kp; i++)
            {
                CLRXKernel* outKernel = static_cast<CLRXKernel*>(kernels[i]);
                kernels[i] = outKernel->amdOclKernel;
                delete outKernel;
            }
            // freeing original kernels
            for (cl_uint i = 0; i < kernelsToCreate; i++)
                if (p->amdOclProgram->dispatch->clReleaseKernel(kernels[i]) != CL_SUCCESS)
                    clrxAbort("Fatal Error at handling error at kernel creation!");
        }
        return CL_OUT_OF_HOST_MEMORY;
    }
    catch(const std::exception& ex)
    { clrxAbort("Fatal Error at handling error at kernel creation: ", ex.what()); }
    
    if (num_kernels_ret != nullptr)
        *num_kernels_ret = numKernelsOut;
    
    if (kernels != nullptr)
        clrxRetainOnlyCLRXProgramNTimes(p, kernelsToCreate);
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainKernel(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    CLRXKernel* k = static_cast<CLRXKernel*>(kernel);
    const cl_int status = k->amdOclKernel->dispatch->clRetainKernel(k->amdOclKernel);
    if (status == CL_SUCCESS)
        k->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseKernel(cl_kernel   kernel) CL_API_SUFFIX__VERSION_1_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    CLRXKernel* k = static_cast<CLRXKernel*>(kernel);
    
    cl_int status = 0;
    bool doDelete = false;
    try
    {
        // must be in clauses, because
        std::lock_guard<std::mutex> l(k->program->mutex);
        status = k->amdOclKernel->dispatch->clReleaseKernel(k->amdOclKernel);
        if (status == CL_SUCCESS)
            if (k->refCount.fetch_sub(1) == 1)
            {
                k->program->kernelsAttached--; // decrease kernel attached
                doDelete = true;
                if (k->fromAsm)
                    if (k->program->amdOclProgram->dispatch->clReleaseProgram(
                                k->program->amdOclProgram)!=CL_SUCCESS)
                        clrxAbort("Fatal error at releasing original program");
            }
    }
    catch(const std::exception& ex)
    { clrxAbort("Fatal Error at releasing kernel: ", ex.what()); }
    if (doDelete)
    {
        // release program and delete kernel
        clrxReleaseOnlyCLRXProgram(k->program);
        delete k;
    }
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclSetKernelArg(cl_kernel    kernel,
               cl_uint      arg_index,
               size_t       arg_size,
               const void * arg_value) CL_API_SUFFIX__VERSION_1_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    if (arg_index >= (k->argTypes.size()>>1))
        return CL_INVALID_ARG_INDEX;
    
    if (k->argTypes[arg_index<<1] && k->argTypes[(arg_index<<1) + 1]) // cmdqueue
    {
        if (arg_size != sizeof(cl_command_queue))
            return CL_INVALID_ARG_SIZE;
        
        cl_command_queue amdCmdQueue = nullptr;
        if (arg_value != nullptr && *(cl_command_queue*)arg_value != nullptr)
            amdCmdQueue = (*(const CLRXCommandQueue**)(arg_value))->amdOclCommandQueue;
        return k->amdOclKernel->dispatch->clSetKernelArg(k->amdOclKernel, arg_index,
                     arg_size, &amdCmdQueue);
    }
    else if (k->argTypes[arg_index<<1]) // memobject
    {
        /*std::cout << "set buffer for kernel " << kernel << " for arg " <<
                    arg_index << std::endl;*/
        if (arg_size != sizeof(cl_mem))
            return CL_INVALID_ARG_SIZE;
        
        cl_mem amdMemObject = nullptr;
        if (arg_value != nullptr && *(cl_mem*)arg_value != nullptr)
            amdMemObject = (*(const CLRXMemObject**)(arg_value))->amdOclMemObject;
        return k->amdOclKernel->dispatch->clSetKernelArg(k->amdOclKernel, arg_index,
                     arg_size, &amdMemObject);
    }
    else if (k->argTypes[(arg_index<<1) + 1]) // sampler
    {
        /*std::cout << "set sampler for kernel " << kernel << " for arg " <<
                    arg_index << std::endl;*/
        if (arg_size != sizeof(cl_sampler))
            return CL_INVALID_ARG_SIZE;
        
        cl_sampler amdSampler = nullptr;
        if (arg_value != nullptr && *(cl_mem*)arg_value != nullptr)
            amdSampler = (*(const CLRXSampler**)(arg_value))->amdOclSampler;
        return k->amdOclKernel->dispatch->clSetKernelArg(k->amdOclKernel, arg_index,
                     arg_size, &amdSampler);
    }
    // other argument type
    return k->amdOclKernel->dispatch->clSetKernelArg(k->amdOclKernel, arg_index,
                 arg_size, arg_value);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetKernelInfo(cl_kernel       kernel,
                cl_kernel_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    
    switch(param_name)
    {
        case CL_KERNEL_CONTEXT:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_context))
                    return CL_INVALID_VALUE;
                *static_cast<cl_context*>(param_value) =
                        static_cast<const CLRXProgram*>(k->program)->context;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_context);
            break;
        case CL_KERNEL_PROGRAM:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_program))
                    return CL_INVALID_VALUE;
                *static_cast<cl_program*>(param_value) = k->program;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_program);
            break;
        default:
            return k->amdOclKernel->
                dispatch->clGetKernelInfo(k->amdOclKernel, param_name,
                    param_value_size, param_value, param_value_size_ret);
    }
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetKernelWorkGroupInfo(cl_kernel                  kernel,
         cl_device_id               device,
         cl_kernel_work_group_info  param_name,
         size_t                     param_value_size,
         void *                     param_value,
         size_t *                   param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    if (device == nullptr)
        return CL_INVALID_DEVICE;
    
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    const CLRXDevice* d = static_cast<const CLRXDevice*>(device);
    
    return k->amdOclKernel->dispatch->clGetKernelWorkGroupInfo(k->amdOclKernel,
           d->amdOclDevice, param_name, param_value_size, param_value,
           param_value_size_ret);
}

} /* extern "C" */
