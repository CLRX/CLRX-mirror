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

#include <CLRX/Config.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <cstring>
#include <CLRX/Utilities.h>
#include "CLWrapper.h"

extern "C"
{

/* public API definitions */

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
        {
            std::cerr << "Very strange error: clGetPlatformIDs is not yet initialized!" <<
                    std::endl;
            abort();
        }
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
        if (param_value != nullptr)
        {
            if (param_name == CL_PLATFORM_EXTENSIONS)
            {
                if (param_value_size < p->extensionsSize)
                    return CL_INVALID_VALUE;
                memcpy(param_value, p->extensions, p->extensionsSize);
            }
            else // CL_PLATFORM_VERSION
            {
                if (param_value_size < p->versionSize)
                    return CL_INVALID_VALUE;
                memcpy(param_value, p->version, p->versionSize);
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
    if (device_type == 0)
        return CL_INVALID_DEVICE_TYPE;
    
    if (num_entries == 0 && devices != nullptr)
        return CL_INVALID_VALUE;
    if (devices == nullptr && num_devices == nullptr)
        return CL_INVALID_VALUE;
    
    CLRXPlatform* p = static_cast<CLRXPlatform*>(platform);
    try
    { std::call_once(p->onceFlag, clrxPlatformInitializeDevices, p); }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal error at device initialization: " << ex.what() << std::endl;
        abort();
    }
    catch(...)
    {
        std::cerr << "Fatal and unknown error at device initialization: " << std::endl;
        abort();
    }
    if (p->deviceStatusInit != CL_SUCCESS)
        return p->deviceStatusInit;
    
    /* real function */
    cl_uint outIdx = 0;
    if (p->devices != nullptr)
    {
        for (cl_uint i = 0; i < p->devicesNum; i++)
        {
            CLRXDevice& clrxDevice = p->devices[i];
            if ((clrxDevice.type & device_type) == 0)
                continue;
            if (num_entries != 0 && outIdx < num_entries)
            {
                if (devices != nullptr)
                    devices[outIdx] = &clrxDevice;
            }
            outIdx++;
        }
    }
    
    if (num_devices != nullptr)
        *num_devices = outIdx;
    
    return (outIdx != 0) ? CL_SUCCESS : CL_DEVICE_NOT_FOUND;
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
                if (param_value != nullptr)
                {
                    if (param_value_size < d->extensionsSize)
                        return CL_INVALID_VALUE;
                    memcpy(param_value, d->extensions, d->extensionsSize);
                }
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = d->extensionsSize;
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
        case CL_DEVICE_PARENT_DEVICE:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_device_id))
                    return CL_INVALID_VALUE;
                *static_cast<cl_device_id*>(param_value) = d->parent;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_device_id);
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
        /* translate props if needed */
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
                
                for (size_t i = 0; i < propNums; i+=2)
                {
                    amdProps[i] = properties[i];
                    if (properties[i] == CL_CONTEXT_PLATFORM)
                    {
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
    
    /* create own context */
    CLRXContext* outContext = nullptr;
    try
    {
        outContext = new CLRXContext;
        outContext->dispatch = const_cast<CLRXIcdDispatch*>(&clrxDispatchRecord);
        outContext->amdOclContext = amdContext;
        // handle out of memory
        if (clrxSetContextDevices(outContext, num_devices, devices) != CL_SUCCESS)
        {
            std::cerr << "Error at associating devices in context" << std::endl;
            abort();
        }
        outContext->propertiesNum = propNums>>1;
        if (properties != nullptr)
            outContext->properties = new cl_context_properties[propNums+1];
    }
    catch(const std::bad_alloc& ex)
    {   
        delete outContext; // and deletes devices and properties
        // release context
        if (d->amdOclDevice->dispatch->clReleaseContext(amdContext) != CL_SUCCESS)
        {
            std::cerr << "Fatal Error at handling error at context creation!" << std::endl;
            abort();
        }
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    
    if (properties != nullptr)
        std::copy(properties, properties + propNums+1, outContext->properties);
    
    for (cl_uint i = 0; i < outContext->devicesNum; i++)
        clrxRetainOnlyCLRXDevice(static_cast<CLRXDevice*>(outContext->devices[i]));
    
    return outContext;
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
    std::vector<cl_context_properties> amdProps;
    cl_context amdContext = nullptr;
    try
    {
        /* translate props if needed */
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
                
                for (size_t i = 0; i < propNums; i+=2)
                {
                    amdProps[i] = properties[i];
                    if (properties[i] == CL_CONTEXT_PLATFORM)
                    {
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
    CLRXContext* outContext = nullptr;
    try
    { 
        outContext = new CLRXContext;
        outContext->dispatch = const_cast<CLRXIcdDispatch*>(&clrxDispatchRecord);
        outContext->amdOclContext = amdContext;
        // handle out of memory
        if (clrxSetContextDevices(outContext, platform) != CL_SUCCESS)
        {
            std::cerr << "Error at associating devices in context" << std::endl;
            abort();
        }
        outContext->propertiesNum = propNums>>1;
        if (properties != nullptr)
            outContext->properties = new cl_context_properties[propNums+1];
    }
    catch(const std::bad_alloc& ex)
    {   
        delete outContext; // and deletes properties
        // release context
        if (platform->amdOclPlatform->dispatch->clReleaseContext(amdContext) != CL_SUCCESS)
        {
            std::cerr << "Fatal Error at handling error at context creation!" << std::endl;
            abort();
        }
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    
    if (properties != nullptr)
        std::copy(properties, properties + propNums+1, outContext->properties);
    
    for (cl_uint i = 0; i < outContext->devicesNum; i++)
        clrxRetainOnlyCLRXDevice(static_cast<CLRXDevice*>(outContext->devices[i]));
    
    return outContext;
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
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_uint))
                    return CL_INVALID_VALUE;
                *static_cast<cl_uint*>(param_value) = c->devicesNum;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_uint);
            break;
        case CL_CONTEXT_DEVICES:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_device_id)*c->devicesNum)
                    return CL_INVALID_VALUE;
                std::copy(c->devices, c->devices + c->devicesNum,
                          static_cast<cl_device_id*>(param_value));
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_device_id)*c->devicesNum;
            break;
        case CL_CONTEXT_PROPERTIES:
            if (c->properties == nullptr)
            {
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = 0;
            }
            else
            {
                const size_t propElems = c->propertiesNum*2+1;
                if (param_value != nullptr)
                {
                    if (param_value_size < sizeof(cl_context_properties)*propElems)
                        return CL_INVALID_VALUE;
                    std::copy(c->properties, c->properties + propElems,
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
        if (q->refCount.fetch_sub(1) == 1)
        {
            clrxReleaseOnlyCLRXDevice(q->device);
            clrxReleaseOnlyCLRXContext(q->context);
            delete q;
        }
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
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_mem))
                    return CL_INVALID_VALUE;
                *static_cast<cl_mem*>(param_value) = m->parent;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_mem);
            break;
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
    if (param_name != CL_IMAGE_BUFFER)
        return m->amdOclMemObject->dispatch->clGetImageInfo(m->amdOclMemObject, param_name,
                param_value_size, param_value, param_value_size_ret);
    else
    {
        if (param_value != nullptr)
        {
            if (param_value_size < sizeof(cl_mem))
                return CL_INVALID_VALUE;
            // returns parent (parent is associated buffer for image)
            *static_cast<cl_mem*>(param_value) = m->parent;
        }
        if (param_value_size_ret != nullptr)
            *param_value_size_ret = sizeof(cl_mem);
        return CL_SUCCESS;
    }
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
    if (param_name != CL_SAMPLER_CONTEXT && param_name != CL_SAMPLER_REFERENCE_COUNT)
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
    
    CLRXProgram* outProgram = clrxCreateCLRXProgram(c, amdProgram, errcode_ret);
    
    return outProgram;
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
    
    CLRXProgram* outProgram = clrxCreateCLRXProgram(c, amdProgram, errcode_ret);
    
    return outProgram;
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
    
    if (status == CL_SUCCESS)
        try
        {
            std::lock_guard<std::mutex> lock(p->mutex); // lock if assocDevices is changed
            if (p->refCount.fetch_sub(1) == 1)
            {
                clrxReleaseOnlyCLRXContext(p->context);
                delete p;
            }
        }
        catch(const std::exception& ex)
        {
            std::cerr << "Fatal exception happened: " << ex.what() << std::endl;
            abort();
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
    
    void* destUserData = user_data;
    CLRXBuildProgramUserData* wrappedData = nullptr;
    void (CL_CALLBACK *  notifyToCall)(cl_program, void *) =  nullptr;
    if (pfn_notify != nullptr)
    {
        wrappedData = new CLRXBuildProgramUserData;
        wrappedData->realNotify = pfn_notify;
        wrappedData->clrxProgram = static_cast<CLRXProgram*>(program);
        wrappedData->realUserData = user_data;
        destUserData = wrappedData;
        notifyToCall = clrxBuildProgramNotifyWrapper;
    }
    
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    
    try
    { // catching system_error
    {
        std::lock_guard<std::mutex> lock(p->mutex);
        p->concurrentBuilds++;
        p->kernelArgFlagsInitialized = false; // 
    }
    
    if (num_devices == 0)
    {
        try
        {
            const cl_int status = p->amdOclProgram->dispatch->clBuildProgram(
                p->amdOclProgram, 0, nullptr, options, notifyToCall,
                destUserData);
            wrappedData = nullptr; // consumed by original clBuildProgram
            
            std::lock_guard<std::mutex> lock(p->mutex);
            p->concurrentBuilds--; // after this building
            const cl_int newStatus = clrxUpdateProgramAssocDevices(p);
            if (newStatus != CL_SUCCESS)
                return newStatus;
            
            if (status == CL_SUCCESS)
            {
                cl_int newStatus = clrxInitKernelArgFlagsMap(p);
                if (newStatus != CL_SUCCESS)
                    return newStatus;
            }
            return status;
        }
        catch(const std::bad_alloc& ex)
        {
            delete wrappedData; // delete only if original clBuildProgram is not called
            return CL_OUT_OF_HOST_MEMORY;
        }
    }
    
    // if devices list is supplied, we change associated devices for program
    std::vector<cl_device_id> amdDevices(num_devices);
    
    for (cl_uint i = 0; i < num_devices; i++)
    {
        if (device_list[i] == nullptr)
        {
            delete wrappedData;
            return CL_INVALID_DEVICE;
        }
        amdDevices[i] = static_cast<const CLRXDevice*>(device_list[i])->amdOclDevice;
    }
    
    const cl_int status = p->amdOclProgram->dispatch->clBuildProgram(
            p->amdOclProgram, num_devices, amdDevices.data(), options,
                     notifyToCall, destUserData);
    
    wrappedData = nullptr; // consumed by original clBuildProgram
    
    std::lock_guard<std::mutex> lock(p->mutex);
    p->concurrentBuilds--; // after this building
    if (status != CL_INVALID_DEVICE)
    {
        const cl_int newStatus = clrxUpdateProgramAssocDevices(p);
        if (newStatus != CL_SUCCESS)
            return newStatus;
    }
    
    if (status == CL_SUCCESS)
    {
        const cl_int newStatus = clrxInitKernelArgFlagsMap(p);
        if (newStatus != CL_SUCCESS)
            return newStatus;
    }
    
    if (status != CL_SUCCESS)
        return status;
    
    return CL_SUCCESS;
    }
    catch(const std::bad_alloc& ex)
    {
        delete wrappedData; // delete only if original clBuildProgram is not called
        return CL_OUT_OF_HOST_MEMORY;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal exception happened: " << ex.what() << std::endl;
        abort();
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
            {   // update only when concurrent builds is working
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
            {   // update only when concurrent builds is working
                const cl_int status = clrxUpdateProgramAssocDevices(p);
                if (status != CL_SUCCESS)
                    return status;
            }
            
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_device_id)*p->assocDevicesNum)
                    return CL_INVALID_VALUE;
                std::copy(p->assocDevices, p->assocDevices + p->assocDevicesNum,
                        static_cast<cl_device_id*>(param_value));
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_device_id)*p->assocDevicesNum;
        }
            break;
        default:
            return p->amdOclProgram->
                dispatch->clGetProgramInfo(p->amdOclProgram, param_name,
                    param_value_size, param_value, param_value_size_ret);
    }
    return CL_SUCCESS;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal exception happened: " << ex.what() << std::endl;
        abort();
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
    const CLRXProgram* p = static_cast<const CLRXProgram*>(program);
    const CLRXDevice* d = static_cast<const CLRXDevice*>(device);
    
    return p->amdOclProgram->dispatch->clGetProgramBuildInfo(p->amdOclProgram,
            d->amdOclDevice, param_name, param_value_size, param_value,
            param_value_size_ret);
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
    const cl_kernel amdKernel = p->amdOclProgram->dispatch->clCreateKernel(
                p->amdOclProgram, kernel_name, errcode_ret);
    if (amdKernel == nullptr)
        return nullptr;
    
    CLRXKernel* outKernel = nullptr;
    try
    {
        std::lock_guard<std::mutex> l(p->mutex);
        p->kernelsAttached = true;
        if (p->concurrentBuilds != 0)
        {
            // update only when concurrent builds is working
            cl_int status = clrxUpdateProgramAssocDevices(p);
            if (status != CL_SUCCESS)
            {
                if (errcode_ret != nullptr)
                    *errcode_ret = status;
                return nullptr;
            }
            
            status = clrxInitKernelArgFlagsMap(p);
            if (status != CL_SUCCESS)
            {
                if (errcode_ret != nullptr)
                    *errcode_ret = status;
                return nullptr;
            }
        }
        
        CLRXKernelArgFlagMap::const_iterator argFlagMapIt =
                p->kernelArgFlagsMap.find(kernel_name);
        if (argFlagMapIt == p->kernelArgFlagsMap.end())
        {
            std::cerr << "Cant find kernel arg flag!" << std::endl;
            abort();
        }
        outKernel = new CLRXKernel(argFlagMapIt->second);
    }
    catch(const std::bad_alloc& ex)
    {
        if (p->amdOclProgram->dispatch->clReleaseKernel(amdKernel) != CL_SUCCESS)
        {
            std::cerr << "Fatal Error at handling error at kernel creation!" << std::endl;
            abort();
        }
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal Error at handling error at kernel creation!" << std::endl;
        abort();
    }
    
    outKernel->dispatch = const_cast<CLRXIcdDispatch*>(&clrxDispatchRecord);
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
    if (kernels == nullptr)
        return p->amdOclProgram->dispatch->clCreateKernelsInProgram(p->amdOclProgram,
                        num_kernels, kernels, num_kernels_ret);
    
    cl_uint numKernelsOut = 0; // for numKernelsOut
    if (num_kernels_ret != nullptr)
        numKernelsOut = *num_kernels_ret; 
    const cl_int status = p->amdOclProgram->dispatch->clCreateKernelsInProgram(
        p->amdOclProgram, num_kernels, kernels, &numKernelsOut);
    
    if (status != CL_SUCCESS)
        return status;
    
    /* replaces original kernels by our (CLRXKernels) */
    const cl_uint kernelsToCreate = std::min(num_kernels, numKernelsOut);
    cl_uint kp = 0; // kernel already processed
    char* kernelName = nullptr;
    size_t maxKernelNameSize = 0;
    try
    {
        std::lock_guard<std::mutex> l(p->mutex);
        p->kernelsAttached = true;
        if (p->concurrentBuilds != 0)
        {   // update only when concurrent builds is working
            cl_int status = clrxUpdateProgramAssocDevices(p);
            if (status != CL_SUCCESS)
                return status;
            
            status = clrxInitKernelArgFlagsMap(p);
            if (status != CL_SUCCESS)
                return status;
        }
        
        for (kp = 0; kp < kernelsToCreate; kp++)
        {
            size_t kernelNameSize;
            cl_int status = kernels[kp]->dispatch->clGetKernelInfo(
                    kernels[kp], CL_KERNEL_FUNCTION_NAME, 0, nullptr, &kernelNameSize);
            if (status != CL_SUCCESS)
            {
                std::cerr << "Cant get kernel function name" << std::endl;
                abort();
            }
            if (kernelName != nullptr && kernelNameSize > maxKernelNameSize)
            {
                delete[] kernelName;
                kernelName = new char[kernelNameSize];
                maxKernelNameSize = kernelNameSize;
            }
            
            status = kernels[kp]->dispatch->clGetKernelInfo(kernels[kp],
                        CL_KERNEL_FUNCTION_NAME, kernelNameSize, kernelName, nullptr);
            if (status != CL_SUCCESS)
            {
                std::cerr << "Cant get kernel function name" << std::endl;
                abort();
            }
            
            CLRXKernelArgFlagMap::const_iterator argFlagMapIt =
                    p->kernelArgFlagsMap.find(kernelName);
            if (argFlagMapIt == p->kernelArgFlagsMap.end())
            {
                std::cerr << "Cant find kernel arg flag!" << std::endl;
                abort();
            }
            
            CLRXKernel* outKernel = new CLRXKernel(argFlagMapIt->second);
            outKernel->dispatch = const_cast<CLRXIcdDispatch*>(&clrxDispatchRecord);
            outKernel->amdOclKernel = kernels[kp];
            outKernel->program = p;
            kernels[kp] = outKernel;
        }
    }
    catch(const std::bad_alloc& ex)
    {   // already processed kernels
        delete[] kernelName;
        for (cl_uint i = 0; i < kp; i++)
        {
            CLRXKernel* outKernel = static_cast<CLRXKernel*>(kernels[i]);
            kernels[i] = outKernel->amdOclKernel;
            delete outKernel;
        }
        // freeing original kernels
        for (cl_uint i = 0; i < kernelsToCreate; i++)
            if (p->amdOclProgram->dispatch->clReleaseKernel(kernels[i]) != CL_SUCCESS)
            {
                std::cerr <<
                    "Fatal Error at handling error at kernel creation!" << std::endl;
                abort();
            }
        return CL_OUT_OF_HOST_MEMORY;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal Error at handling error at kernel creation!" << std::endl;
        abort();
    }
    delete[] kernelName;
    
    if (num_kernels_ret != nullptr)
        *num_kernels_ret = numKernelsOut;
    
    clrxRetainOnlyCLRXProgramNTimes(p, kernelsToCreate);
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainKernel(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    CLRXKernel* k = static_cast<CLRXKernel*>(kernel);
    const cl_int status = k->dispatch->clRetainKernel(k->amdOclKernel);
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
    
    const cl_int status = k->amdOclKernel->dispatch->clReleaseKernel(k->amdOclKernel);
    if (status == CL_SUCCESS)
        if (k->refCount.fetch_sub(1) == 1)
        {
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
    
    if (k->argTypes[arg_index<<1]) // buffer
    {
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
        {
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_program))
                    return CL_INVALID_VALUE;
                *static_cast<cl_program*>(param_value) = k->program;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_program);
        }
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
