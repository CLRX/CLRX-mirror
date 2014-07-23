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
#include <utility>
#include <thread>
#include <mutex>
#include <cstring>
#include <atomic>
#include <climits>
#include <cstdint>
#include <cstddef>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinaries.h>
#include "CLWrapper.h"

using namespace CLRX;

std::once_flag clrxOnceFlag;
bool useCLRXWrapper = true;
static DynLibrary* amdOclLibrary = nullptr;
cl_uint amdOclNumPlatforms = 0;
CLRXpfn_clGetPlatformIDs amdOclGetPlatformIDs = nullptr;
CLRXpfn_clUnloadCompiler amdOclUnloadCompiler = nullptr;
cl_int clrxWrapperInitStatus = CL_SUCCESS;

CLRXPlatform* clrxPlatforms = nullptr;

clEnqueueWaitSignalAMD_fn amdOclEnqueueWaitSignalAMD = nullptr;
clEnqueueWriteSignalAMD_fn amdOclEnqueueWriteSignalAMD = nullptr;
clEnqueueMakeBuffersResidentAMD_fn amdOclEnqueueMakeBuffersResidentAMD = nullptr;
CLRXpfn_clGetExtensionFunctionAddress amdOclGetExtensionFunctionAddress = nullptr;

/* extensions table - entries are sorted in function name's order */
CLRXExtensionEntry clrxExtensionsTable[18] =
{
    { "clCreateEventFromGLsyncKHR", (void*)clrxclCreateEventFromGLsyncKHR },
    { "clCreateFromGLBuffer", (void*)clrxclCreateFromGLBuffer },
    { "clCreateFromGLRenderbuffer", (void*)clrxclCreateFromGLRenderbuffer },
    { "clCreateFromGLTexture", (void*)clrxclCreateFromGLTexture },
    { "clCreateFromGLTexture2D", (void*)clrxclCreateFromGLTexture2D },
    { "clCreateFromGLTexture3D", (void*)clrxclCreateFromGLTexture3D },
    { "clCreateSubDevicesEXT", (void*)clrxclCreateSubDevicesEXT },
    { "clEnqueueAcquireGLObjects", (void*)clrxclEnqueueAcquireGLObjects },
    { "clEnqueueMakeBuffersResidentAMD", (void*)clrxclEnqueueMakeBuffersResidentAMD },
    { "clEnqueueReleaseGLObjects", (void*)clrxclEnqueueReleaseGLObjects },
    { "clEnqueueWaitSignalAMD", (void*)clrxclEnqueueWaitSignalAMD },
    { "clEnqueueWriteSignalAMD", (void*)clrxclEnqueueWriteSignalAMD },
    { "clGetGLContextInfoKHR", (void*)clrxclGetGLContextInfoKHR },
    { "clGetGLObjectInfo", (void*)clrxclGetGLObjectInfo },
    { "clGetGLTextureInfo", (void*)clrxclGetGLTextureInfo },
    { "clIcdGetPlatformIDsKHR", (void*)clrxclIcdGetPlatformIDsKHR },
    { "clReleaseDeviceEXT", (void*)clrxclReleaseDeviceEXT },
    { "clRetainDeviceEXT", (void*)clrxclRetainDeviceEXT }
};

/* dispatch structure */
const CLRXIcdDispatch clrxDispatchRecord = 
{
    clrxclGetPlatformIDs,
    clrxclGetPlatformInfo,
    clrxclGetDeviceIDs,
    clrxclGetDeviceInfo,
    clrxclCreateContext,
    clrxclCreateContextFromType,
    clrxclRetainContext,
    clrxclReleaseContext,
    clrxclGetContextInfo,
    clrxclCreateCommandQueue,
    clrxclRetainCommandQueue,
    clrxclReleaseCommandQueue,
    clrxclGetCommandQueueInfo,
    clrxclSetCommandQueueProperty,
    clrxclCreateBuffer,
    clrxclCreateImage2D,
    clrxclCreateImage3D,
    clrxclRetainMemObject,
    clrxclReleaseMemObject,
    clrxclGetSupportedImageFormats,
    clrxclGetMemObjectInfo,
    clrxclGetImageInfo,
    clrxclCreateSampler,
    clrxclRetainSampler,
    clrxclReleaseSampler,
    clrxclGetSamplerInfo,
    clrxclCreateProgramWithSource,
    clrxclCreateProgramWithBinary,
    clrxclRetainProgram,
    clrxclReleaseProgram,
    clrxclBuildProgram,
    clrxclUnloadCompiler,
    clrxclGetProgramInfo,
    clrxclGetProgramBuildInfo,
    clrxclCreateKernel,
    clrxclCreateKernelsInProgram,
    clrxclRetainKernel,
    clrxclReleaseKernel,
    clrxclSetKernelArg,
    clrxclGetKernelInfo,
    clrxclGetKernelWorkGroupInfo,
    clrxclWaitForEvents,
    clrxclGetEventInfo,
    clrxclRetainEvent,
    clrxclReleaseEvent,
    clrxclGetEventProfilingInfo,
    clrxclFlush,
    clrxclFinish,
    clrxclEnqueueReadBuffer,
    clrxclEnqueueWriteBuffer,
    clrxclEnqueueCopyBuffer,
    clrxclEnqueueReadImage,
    clrxclEnqueueWriteImage,
    clrxclEnqueueCopyImage,
    clrxclEnqueueCopyImageToBuffer,
    clrxclEnqueueCopyBufferToImage,
    clrxclEnqueueMapBuffer,
    clrxclEnqueueMapImage,
    clrxclEnqueueUnmapMemObject,
    clrxclEnqueueNDRangeKernel,
    clrxclEnqueueTask,
    clrxclEnqueueNativeKernel,
    clrxclEnqueueMarker,
    clrxclEnqueueWaitForEvents,
    clrxclEnqueueBarrier,
    clrxclGetExtensionFunctionAddress,
    clrxclCreateFromGLBuffer,
    clrxclCreateFromGLTexture2D,
    clrxclCreateFromGLTexture3D,
    clrxclCreateFromGLRenderbuffer,
    clrxclGetGLObjectInfo,
    clrxclGetGLTextureInfo,
    clrxclEnqueueAcquireGLObjects,
    clrxclEnqueueReleaseGLObjects,
    clrxclGetGLContextInfoKHR,

    nullptr, // clGetDeviceIDsFromD3D10KHR,
    nullptr, // clCreateFromD3D10BufferKHR,
    nullptr, // clCreateFromD3D10Texture2DKHR,
    nullptr, // clCreateFromD3D10Texture3DKHR,
    nullptr, // clEnqueueAcquireD3D10ObjectsKHR,
    nullptr, // clEnqueueReleaseD3D10ObjectsKHR,

    clrxclSetEventCallback,
    clrxclCreateSubBuffer,
    clrxclSetMemObjectDestructorCallback,
    clrxclCreateUserEvent,
    clrxclSetUserEventStatus,
    clrxclEnqueueReadBufferRect,
    clrxclEnqueueWriteBufferRect,
    clrxclEnqueueCopyBufferRect,

    clrxclCreateSubDevicesEXT,
    clrxclRetainDeviceEXT,
    clrxclReleaseDeviceEXT,

    clrxclCreateEventFromGLsyncKHR,

    clrxclCreateSubDevices,
    clrxclRetainDevice,
    clrxclReleaseDevice,
    clrxclCreateImage,
    clrxclCreateProgramWithBuiltInKernels,
    clrxclCompileProgram,
    clrxclLinkProgram,
    clrxclUnloadPlatformCompiler,
    clrxclGetKernelArgInfo,
    clrxclEnqueueFillBuffer,
    clrxclEnqueueFillImage,
    clrxclEnqueueMigrateMemObjects,
    clrxclEnqueueMarkerWithWaitList,
    clrxclEnqueueBarrierWithWaitList,
    clrxclGetExtensionFunctionAddressForPlatform,
    clrxclCreateFromGLTexture,

    nullptr, // clGetDeviceIDsFromD3D11KHR,
    nullptr, // clCreateFromD3D11BufferKHR,
    nullptr, // clCreateFromD3D11Texture2DKHR,
    nullptr, // clCreateFromD3D11Texture3DKHR,
    nullptr, // clCreateFromDX9MediaSurfaceKHR,
    nullptr, // clEnqueueAcquireD3D11ObjectsKHR,
    nullptr, // clEnqueueReleaseD3D11ObjectsKHR,

    nullptr, // clGetDeviceIDsFromDX9MediaAdapterKHR,
    nullptr, // clEnqueueAcquireDX9MediaSurfacesKHR,
    nullptr // clEnqueueReleaseDX9MediaSurfacesKHR
};

void clrxReleaseOnlyCLRXDevice(CLRXDevice* device)
{
    if (device->parent != nullptr)
        if (device->refCount.fetch_sub(1) == 1)
        {   // amdOclDevice has been already released, we release only our device
            clrxReleaseOnlyCLRXDevice(device->parent);
            delete device;
        }
}

void clrxReleaseOnlyCLRXContext(CLRXContext* context)
{
    if (context->refCount.fetch_sub(1) == 1)
    {   // amdOclContext has been already released, we release only our context
        for (cl_uint i = 0; i < context->devicesNum; i++)
            clrxReleaseOnlyCLRXDevice(context->devices[i]);
        delete context;
    }
}

void clrxReleaseOnlyCLRXMemObject(CLRXMemObject* memObject)
{
    if (memObject->refCount.fetch_sub(1) == 1)
    {   // amdOclContext has been already released, we release only our context
        clrxReleaseOnlyCLRXContext(memObject->context);
        if (memObject->parent != nullptr)
            clrxReleaseOnlyCLRXMemObject(memObject->parent);
        if (memObject->buffer != nullptr)
            clrxReleaseOnlyCLRXMemObject(memObject->buffer);
        delete memObject;
    }
}

void clrxWrapperInitialize()
{
    DynLibrary* tmpAmdOclLibrary = nullptr;
    try
    {
        useCLRXWrapper = !parseEnvVariable<bool>("CLRX_FORCE_ORIGINAL_AMDOCL", false);
        std::string amdOclPath = parseEnvVariable<std::string>("CLRX_AMDOCL_PATH",
                           DEFAULT_AMDOCLPATH);
        DynLibrary* tmpAmdOclLibrary = new DynLibrary(amdOclPath.c_str(), DYNLIB_NOW);
        
        amdOclGetPlatformIDs = (CLRXpfn_clGetPlatformIDs)
                tmpAmdOclLibrary->getSymbol("clGetPlatformIDs");
        if (amdOclGetPlatformIDs == nullptr)
            throw Exception("AMDOCL clGetPlatformIDs have invalid value!");
        
        try
        { amdOclUnloadCompiler = (CLRXpfn_clUnloadCompiler)
            tmpAmdOclLibrary->getSymbol("clUnloadCompiler"); }
        catch(const CLRX::Exception& ex)
        { /* ignore if not found */ }
        
        amdOclGetExtensionFunctionAddress = (CLRXpfn_clGetExtensionFunctionAddress)
                tmpAmdOclLibrary->getSymbol("clGetExtensionFunctionAddress");
        if (amdOclGetExtensionFunctionAddress == nullptr)
            throw Exception("AMDOCL clGetExtensionFunctionAddress have invalid value!");
        
        const pfn_clIcdGetPlatformIDs pgetPlatformIDs = (pfn_clIcdGetPlatformIDs)
                amdOclGetExtensionFunctionAddress("clIcdGetPlatformIDsKHR");
        if (amdOclGetExtensionFunctionAddress == nullptr)
            throw Exception("AMDOCL clIcdGetPlatformIDsKHR have invalid value!");
        
        /* specific amd extensions functions */
        amdOclEnqueueWaitSignalAMD = (clEnqueueWaitSignalAMD_fn)
            amdOclGetExtensionFunctionAddress("clEnqueueWaitSignalAMD");
        amdOclEnqueueWriteSignalAMD = (clEnqueueWriteSignalAMD_fn)
            amdOclGetExtensionFunctionAddress("clEnqueueWriteSignalAMD");
        amdOclEnqueueMakeBuffersResidentAMD = (clEnqueueMakeBuffersResidentAMD_fn)
            amdOclGetExtensionFunctionAddress("clEnqueueMakeBuffersResidentAMD");
        
        /* update clrxExtensionsTable */
        for (CLRXExtensionEntry& extEntry: clrxExtensionsTable)
            // erase CLRX extension entry if not reflected in AMD extensions
            if (amdOclGetExtensionFunctionAddress(extEntry.funcname) == nullptr)
                extEntry.address = nullptr;
        /* end of clrxExtensionsTable */
        
        cl_uint platformCount;
        cl_int status = pgetPlatformIDs(0, nullptr, &platformCount);
        if (status != CL_SUCCESS)
        {
            clrxWrapperInitStatus = status;
            delete tmpAmdOclLibrary;
            return;
        }
        
        if (platformCount == 0)
        {
            delete tmpAmdOclLibrary;
            return;
        }
        
        std::vector<cl_platform_id> amdOclPlatforms(platformCount);
        std::fill(amdOclPlatforms.begin(), amdOclPlatforms.end(), nullptr);
        
        status = pgetPlatformIDs(platformCount, amdOclPlatforms.data(), nullptr);
        if (status != CL_SUCCESS)
        {
            clrxWrapperInitStatus = status;
            delete tmpAmdOclLibrary;
            return;
        }
        
        if (useCLRXWrapper)
        {
            const cxuint clrxExtEntriesNum =
                    sizeof(clrxExtensionsTable)/sizeof(CLRXExtensionEntry);
            
            clrxPlatforms = new CLRXPlatform[platformCount];
            for (cl_uint i = 0; i < platformCount; i++)
            {
                if (amdOclPlatforms[i] == nullptr)
                    continue;
                
                const cl_platform_id amdOclPlatform = amdOclPlatforms[i];
                CLRXPlatform& clrxPlatform = clrxPlatforms[i];
                
                /* clrxPlatform init */
                clrxPlatform.amdOclPlatform = amdOclPlatform;
                clrxPlatform.dispatch = new CLRXIcdDispatch;
                ::memcpy(clrxPlatform.dispatch, &clrxDispatchRecord,
                         sizeof(CLRXIcdDispatch));
                
                // add to extensions "cl_radeon_extender"
                size_t extsSize;
                if (amdOclPlatform->dispatch->clGetPlatformInfo(amdOclPlatform,
                            CL_PLATFORM_EXTENSIONS, 0, nullptr, &extsSize) != CL_SUCCESS)
                    continue;
                char* extsBuffer = new char[extsSize + 19];
                if (amdOclPlatform->dispatch->clGetPlatformInfo(amdOclPlatform,
                        CL_PLATFORM_EXTENSIONS, extsSize, extsBuffer,
                        nullptr) != CL_SUCCESS)
                {
                    delete[] extsBuffer;
                    continue;
                }
                if (extsSize > 2 && extsBuffer[extsSize-2] != ' ')
                    ::strcat(extsBuffer, " cl_radeon_extender");
                else
                    ::strcat(extsBuffer, "cl_radeon_extender");
                clrxPlatform.extensions = extsBuffer;
                clrxPlatform.extensionsSize = ::strlen(extsBuffer)+1;
                
                // add to version " (clrx 0.0)"
                size_t versionSize;
                if (amdOclPlatform->dispatch->clGetPlatformInfo(amdOclPlatform,
                            CL_PLATFORM_VERSION, 0, nullptr, &versionSize) != CL_SUCCESS)
                    continue;
                char* versionBuffer = new char[versionSize+20];
                if (amdOclPlatform->dispatch->clGetPlatformInfo(amdOclPlatform,
                        CL_PLATFORM_VERSION, versionSize,
                        versionBuffer, nullptr) != CL_SUCCESS)
                {
                    delete[] versionBuffer;
                    continue;
                }
                ::strcat(versionBuffer, " (clrx 0.0)");
                clrxPlatform.version = versionBuffer;
                clrxPlatform.versionSize = ::strlen(versionBuffer)+1;
                
                /* parse OpenCL version */
                cxuint openCLmajor = 0;
                cxuint openCLminor = 0;
                const char* verEnd = nullptr;
                if (::strncmp(versionBuffer, "OpenCL ", 7) == 0)
                {
                    openCLmajor = CLRX::cstrtoui(versionBuffer + 7, verEnd);
                    if (verEnd != nullptr && *verEnd == '.')
                    {
                        const char* verEnd2 = nullptr;
                        openCLminor = CLRX::cstrtoui(verEnd + 1, verEnd2);
                        if (openCLmajor > UINT16_MAX || openCLminor > UINT16_MAX)
                            openCLmajor = openCLminor = 0; // if failed
                    }
                    clrxPlatform.openCLVersionNum = getOpenCLVersionNum(
                                    openCLmajor, openCLminor);
                }
                /*std::cout << "Platform OCL version: " <<
                        openCLmajor << "." << openCLminor << std::endl;*/
                
                /* initialize extTable */
                clrxPlatform.extEntries = new CLRXExtensionEntry[clrxExtEntriesNum];
                std::copy(clrxExtensionsTable, clrxExtensionsTable + clrxExtEntriesNum,
                          clrxPlatform.extEntries);
                
                if (clrxPlatform.openCLVersionNum >= getOpenCLVersionNum(1, 2))
                {   /* update p->extEntries for platform */
                    for (size_t k = 0; k < clrxExtEntriesNum; k++)
                    {   // erase CLRX extension entry if not reflected in AMD extensions
                        CLRXExtensionEntry& extEntry = clrxPlatform.extEntries[k];
                        if (amdOclPlatform->dispatch->
                            clGetExtensionFunctionAddressForPlatform
                                    (amdOclPlatform, extEntry.funcname) == nullptr)
                            extEntry.address = nullptr;
                    }
                    /* end of p->extEntries for platform */
                }
                
                /* filtering for different OpenCL standards */
                size_t icdEntriesToKept = CLRXICD_ENTRIES_NUM;
                if (clrxPlatform.openCLVersionNum < getOpenCLVersionNum(1, 1))
                    // if earlier than OpenCL 1.1
                    icdEntriesToKept =
                            offsetof(CLRXIcdDispatch, clSetEventCallback)/sizeof(void*);
                else if (clrxPlatform.openCLVersionNum < getOpenCLVersionNum(1, 2))
                    // if earlier than OpenCL 1.2
                    icdEntriesToKept =
                            offsetof(CLRXIcdDispatch, clCreateSubDevices)/sizeof(void*);
                
                // zeroing unsupported entries for later version of OpenCL standard
                std::fill(clrxPlatform.dispatch->entries + icdEntriesToKept,
                          clrxPlatform.dispatch->entries + CLRXICD_ENTRIES_NUM, nullptr);
                
                for (size_t k = 0; k < icdEntriesToKept; k++)
                    /* disable function when not in original driver */
                    if (amdOclPlatform->dispatch->entries[k] == nullptr)
                        clrxPlatform.dispatch->entries[k] = nullptr;
            }
        }
        
        amdOclLibrary = tmpAmdOclLibrary;
        tmpAmdOclLibrary = nullptr;
        amdOclNumPlatforms = platformCount;
    }
    catch(const std::bad_alloc& ex)
    { 
        delete[] clrxPlatforms;
        clrxPlatforms = nullptr;
        delete amdOclLibrary;
        delete tmpAmdOclLibrary;
        amdOclNumPlatforms = 0;
        clrxWrapperInitStatus = CL_OUT_OF_HOST_MEMORY;
        return;
    }
    catch(...)
    { 
        delete[] clrxPlatforms;
        clrxPlatforms = nullptr;
        delete amdOclLibrary;
        delete tmpAmdOclLibrary;
        amdOclNumPlatforms = 0;
        throw; // fatal exception
    }
}

void clrxPlatformInitializeDevices(CLRXPlatform* platform)
{
    cl_int status = platform->amdOclPlatform->dispatch->clGetDeviceIDs(
            platform->amdOclPlatform, CL_DEVICE_TYPE_ALL, 0, nullptr,
            &platform->devicesNum);
    
    if (status != CL_SUCCESS && status != CL_DEVICE_NOT_FOUND)
    {
        platform->devicesNum = 0;
        platform->deviceInitStatus = status;
        return;
    }
    if (status == CL_DEVICE_NOT_FOUND)
        platform->devicesNum = 0; // reset
    
    /* check whether OpenCL 1.2 or later */
    bool isOCL12OrLater = platform->openCLVersionNum >= getOpenCLVersionNum(1, 2);
    //std::cout << "IsOCl12OrLater: " << isOCL12OrLater << std::endl;
    
    /* custom devices not listed in all devices */
    cl_uint customDevicesNum = 0;
    if (isOCL12OrLater)
    {
        status = platform->amdOclPlatform->dispatch->clGetDeviceIDs(
                platform->amdOclPlatform, CL_DEVICE_TYPE_CUSTOM, 0, nullptr,
                &customDevicesNum);
        
        if (status != CL_SUCCESS && status != CL_DEVICE_NOT_FOUND)
        {
            platform->devicesNum = 0;
            platform->deviceInitStatus = status;
            return;
        }
        
        if (status == CL_SUCCESS) // if some devices
            platform->devicesNum += customDevicesNum;
        else // status == CL_DEVICE_NOT_FOUND
            customDevicesNum = 0;
    }
    
    try
    {
        std::vector<cl_device_id> amdDevices(platform->devicesNum);
        platform->devicesArray = new CLRXDevice[platform->devicesNum];
    
        /* get amd devices */
        status = platform->amdOclPlatform->dispatch->clGetDeviceIDs(
                platform->amdOclPlatform, CL_DEVICE_TYPE_ALL,
                platform->devicesNum-customDevicesNum, amdDevices.data(), nullptr);
        if (status != CL_SUCCESS)
        {
            delete[] platform->devicesArray;
            platform->devicesNum = 0;
            platform->devicesArray = nullptr;
            platform->deviceInitStatus = status;
            return;
        }
        // custom devices
        if (customDevicesNum != 0)
        {
            status = platform->amdOclPlatform->dispatch->clGetDeviceIDs(
                    platform->amdOclPlatform, CL_DEVICE_TYPE_CUSTOM, customDevicesNum,
                    amdDevices.data()+platform->devicesNum-customDevicesNum, nullptr);
            if (status != CL_SUCCESS)
            {
                delete[] platform->devicesArray;
                platform->devicesNum = 0;
                platform->devicesArray = nullptr;
                platform->deviceInitStatus = status;
                return;
            }
        }
        
        for (cl_uint i = 0; i < platform->devicesNum; i++)
        {
            CLRXDevice& clrxDevice = platform->devicesArray[i];
            clrxDevice.dispatch = platform->dispatch;
            clrxDevice.amdOclDevice = amdDevices[i];
            clrxDevice.platform = platform;
            
            cl_device_type devType;
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i], CL_DEVICE_TYPE,
                        sizeof(cl_device_type), &devType, nullptr);
            if (status != CL_SUCCESS)
                break;
            
            if ((devType & CL_DEVICE_TYPE_GPU) == 0)
                continue; // do not change extensions if not gpu
            
            // add to extensions "cl_radeon_extender"
            size_t extsSize;
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i],
                      CL_DEVICE_EXTENSIONS, 0, nullptr, &extsSize);
            
            if (status != CL_SUCCESS)
                break;
            char* extsBuffer = new char[extsSize+19];
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i],
                  CL_DEVICE_EXTENSIONS, extsSize, extsBuffer, nullptr);
            if (status != CL_SUCCESS)
            {
                delete[] extsBuffer;
                break;
            }
            if (extsSize > 2 && extsBuffer[extsSize-2] != ' ')
                strcat(extsBuffer, " cl_radeon_extender");
            else
                strcat(extsBuffer, "cl_radeon_extender");
            clrxDevice.extensionsSize = ::strlen(extsBuffer)+1;
            clrxDevice.extensions = extsBuffer;
            
            // add to version " (clrx 0.0)"
            size_t versionSize;
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i],
                      CL_DEVICE_VERSION, 0, nullptr, &versionSize);
            if (status != CL_SUCCESS)
                break;
            char* versionBuffer = new char[versionSize+20];
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i],
                      CL_DEVICE_VERSION, versionSize, versionBuffer, nullptr);
            if (status != CL_SUCCESS)
            {
                delete[] versionBuffer;
                break;
            }
            ::strcat(versionBuffer, " (clrx 0.0)");
            clrxDevice.version = versionBuffer;
            clrxDevice.versionSize = ::strlen(versionBuffer)+1;
        }
        
        if (status != CL_SUCCESS)
        {
            delete[] platform->devicesArray;
            platform->devicesNum = 0;
            platform->devicesArray = nullptr;
            platform->deviceInitStatus = status;
            return;
        }
        // init device pointers
        platform->devicePtrs = new CLRXDevice*[platform->devicesNum];
        for (cl_uint i = 0; i < platform->devicesNum; i++)
            platform->devicePtrs[i] = platform->devicesArray + i;
    }
    catch(const std::bad_alloc& ex)
    {
        delete[] platform->devicesArray;
        delete[] platform->devicePtrs;
        platform->devicesNum = 0;
        platform->devicesArray = nullptr;
        platform->devicePtrs = nullptr;
        platform->deviceInitStatus = CL_OUT_OF_HOST_MEMORY;
        return;
    }
    catch(...)
    {
        delete[] platform->devicesArray;
        delete[] platform->devicePtrs;
        platform->devicesNum = 0;
        platform->devicesArray = nullptr;
        platform->devicePtrs = nullptr;
        throw;
    }
}

/*
 * clrx object utils
 */

static inline bool clrxDeviceCompareByAmdDevice(const CLRXDevice* l, const CLRXDevice* r)
{
    return ptrdiff_t(l->amdOclDevice) < ptrdiff_t(r->amdOclDevice);
}

void translateAMDDevicesIntoCLRXDevices(cl_uint allDevicesNum,
           const CLRXDevice** allDevices, cl_uint amdDevicesNum, cl_device_id* amdDevices)
{
    /* after it we replaces amdDevices into ours devices */
    if (allDevicesNum < 16)  //efficient for small
    {   // 
        for (cl_uint i = 0; i < amdDevicesNum; i++)
        {
            cl_uint j;
            for (j = 0; j < allDevicesNum; j++)
                if (amdDevices[i] == allDevices[j]->amdOclDevice)
                {
                    amdDevices[i] = const_cast<CLRXDevice**>(allDevices)[j];
                    break;
                }
                
            if (j == allDevicesNum)
            {
                std::cerr << "Fatal error at translating AMD devices" << std::endl;
                abort();
            }
        }
    }
    else if(amdDevicesNum != 0) // sorting
    {
        std::vector<const CLRXDevice*> sortedOriginal(allDevices,
                 allDevices + allDevicesNum);
        std::sort(sortedOriginal.begin(), sortedOriginal.end(),
                  clrxDeviceCompareByAmdDevice);
        auto newEnd = std::unique(sortedOriginal.begin(), sortedOriginal.end());
        
        CLRXDevice tmpDevice;
        for (cl_uint i = 0; i < amdDevicesNum; i++)
        {
            tmpDevice.amdOclDevice = amdDevices[i];
            const auto& found = std::lower_bound(sortedOriginal.begin(),
                     newEnd, &tmpDevice, clrxDeviceCompareByAmdDevice);
            if (found != newEnd)
                amdDevices[i] = (cl_device_id)(*found);
            else
            {
                std::cerr << "Fatal error at translating AMD devices" << std::endl;
                abort();
            }
        }
    }
}

/* called always on creating context */
cl_int clrxSetContextDevices(CLRXContext* c, const CLRXPlatform* platform)
{
    cl_uint amdDevicesNum;
    cl_device_id* amdDevices = nullptr;
    
    cl_int status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
        CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &amdDevicesNum, nullptr);
    if (status != CL_SUCCESS)
        return status;
    
    amdDevices = new cl_device_id[amdDevicesNum];
    status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
            CL_CONTEXT_DEVICES, sizeof(cl_device_id)*amdDevicesNum,
            amdDevices, nullptr);
    if (status == CL_OUT_OF_HOST_MEMORY)
    {
        delete[] amdDevices;
        return status;
    }
    if (status != CL_SUCCESS)
    {
        delete[] amdDevices;
        return status;
    }
    
    try
    {
        translateAMDDevicesIntoCLRXDevices(platform->devicesNum,
               (const CLRXDevice**)(platform->devicePtrs), amdDevicesNum, amdDevices);
        // now is ours devices
        c->devicesNum = amdDevicesNum;
        c->devices = reinterpret_cast<CLRXDevice**>(amdDevices);
    }
    catch(const std::bad_alloc& ex)
    {
        delete[] amdDevices;
        return CL_OUT_OF_HOST_MEMORY;
    }
    return CL_SUCCESS;
}

cl_int clrxSetContextDevices(CLRXContext* c, cl_uint inDevicesNum,
            const cl_device_id* inDevices)
{
    cl_uint amdDevicesNum;
    cl_device_id* amdDevices = nullptr;
    
    cl_int status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
        CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &amdDevicesNum, nullptr);
    if (status != CL_SUCCESS)
        return status;
    
    amdDevices = new cl_device_id[amdDevicesNum];
    status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
            CL_CONTEXT_DEVICES, sizeof(cl_device_id)*amdDevicesNum,
            amdDevices, nullptr);
    if (status == CL_OUT_OF_HOST_MEMORY)
    {
        delete[] amdDevices;
        return status;
    }
    if (status != CL_SUCCESS)
    {
        delete[] amdDevices;
        return status;
    }
    
    try
    {
        translateAMDDevicesIntoCLRXDevices(inDevicesNum, (const CLRXDevice**)inDevices,
                       amdDevicesNum, amdDevices);
        // now is ours devices
        c->devicesNum = amdDevicesNum;
        c->devices = reinterpret_cast<CLRXDevice**>(amdDevices);
    }
    catch(const std::bad_alloc& ex)
    {
        delete[] amdDevices;
        return CL_OUT_OF_HOST_MEMORY;
    }
    return CL_SUCCESS;
}

cl_int clrxUpdateProgramAssocDevices(CLRXProgram* p)
{
    size_t amdAssocDevicesNum;
    cl_device_id* amdAssocDevices = nullptr;
    try
    {
        cl_uint totalDevicesNum = (p->transDevicesMap != nullptr) ?
                    p->transDevicesMap->size() : p->context->devicesNum;
        
        amdAssocDevices = new cl_device_id[totalDevicesNum];
        // single OpenCL call should be atomic:
        // reason: can be called between clBuildProgram which changes associated devices
        const cl_int status = p->amdOclProgram->dispatch->clGetProgramInfo(
            p->amdOclProgram, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*totalDevicesNum,
                      amdAssocDevices, &amdAssocDevicesNum);
        
        if (status != CL_SUCCESS)
        {
            delete[] amdAssocDevices;
            return status;
        }
        
        amdAssocDevicesNum /= sizeof(cl_device_id); // number of amd devices
        
        if (totalDevicesNum != amdAssocDevicesNum)
        {   /* reallocate amdAssocDevices */
            cl_device_id* tmpAmdAssocDevices = new cl_device_id[amdAssocDevicesNum];
            std::copy(amdAssocDevices, amdAssocDevices+amdAssocDevicesNum,
                      tmpAmdAssocDevices);
            delete[] amdAssocDevices;
            amdAssocDevices = tmpAmdAssocDevices;
        }
        
        /* compare with previous assocDevices */
        if (p->assocDevicesNum == amdAssocDevicesNum && p->assocDevices != nullptr)
        {
            bool haveDiffs = false;
            for (cl_uint i = 0; i < amdAssocDevicesNum; i++)
                if (static_cast<CLRXDevice*>(p->assocDevices[i])->amdOclDevice !=
                    amdAssocDevices[i])
                {
                    haveDiffs = true;
                    break;
                }
            if (!haveDiffs)
            {   // no differences between calls
                delete[] amdAssocDevices;
                return CL_SUCCESS;
            }
        }
        
        if (p->transDevicesMap != nullptr)
            for (cl_uint i = 0; i < amdAssocDevicesNum; i++)
            {
                CLRXProgramDevicesMap::const_iterator found =
                        p->transDevicesMap->find(amdAssocDevices[i]);
                if (found == p->transDevicesMap->end())
                {
                    std::cerr << "Fatal error at translating AMD devices" << std::endl;
                    abort();
                }
                amdAssocDevices[i] = found->second;
            }
        else
            translateAMDDevicesIntoCLRXDevices(p->context->devicesNum,
                   const_cast<const CLRXDevice**>(p->context->devices),
                   amdAssocDevicesNum, static_cast<cl_device_id*>(amdAssocDevices));
        
        delete[] p->assocDevices;
        p->assocDevicesNum = amdAssocDevicesNum;
        p->assocDevices = reinterpret_cast<CLRXDevice**>(amdAssocDevices);
    }
    catch(const std::bad_alloc& ex)
    {
        delete[] amdAssocDevices;
        return CL_OUT_OF_HOST_MEMORY;
    }
    return CL_SUCCESS;
}

void clrxBuildProgramNotifyWrapper(cl_program program, void * user_data)
{
    CLRXBuildProgramUserData* wrappedDataPtr =
            static_cast<CLRXBuildProgramUserData*>(user_data);
    CLRXBuildProgramUserData wrappedData = *wrappedDataPtr;
    CLRXProgram* p = wrappedDataPtr->clrxProgram;
    try
    {
        std::lock_guard<std::mutex> l(p->mutex);
        if (!wrappedDataPtr->inClFunction)
        {   // do it if not done in clBuildProgram
            p->concurrentBuilds--; // after this building
            const cl_int newStatus = clrxUpdateProgramAssocDevices(p);
            if (newStatus != CL_SUCCESS)
            {
                std::cerr << "Fatal error: cant update programAssocDevices" << std::endl;
                abort();
            }
            if (p->concurrentBuilds == 0)
            {
                delete p->transDevicesMap;
                p->transDevicesMap = nullptr;
            }
        }
        wrappedDataPtr->callDone = true;
        if (wrappedDataPtr->inClFunction) // delete if done in clBuildProgram
        {
            //std::cout << "Delete WrappedData: " << wrappedDataPtr << std::endl;
            delete wrappedDataPtr;
        }
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal exception happened: " << ex.what() << std::endl;
        abort();
    }
    
    // must be called only once (freeing wrapped data)
    wrappedData.realNotify(wrappedData.clrxProgram, wrappedData.realUserData);
}

void clrxLinkProgramNotifyWrapper(cl_program program, void * user_data)
{
    CLRXLinkProgramUserData* wrappedDataPtr =
            static_cast<CLRXLinkProgramUserData*>(user_data);
    
    bool initializedByCallback = false;
    cl_program clrxProgram = nullptr;
    void* realUserData = nullptr;
    try
    {
        std::lock_guard<std::mutex> l(wrappedDataPtr->mutex);
        if (!wrappedDataPtr->clrxProgramFilled)
        {
            initializedByCallback = true;
            CLRXProgram* outProgram = nullptr;
            if (program != nullptr)
            {
                outProgram = new CLRXProgram;
                outProgram->dispatch = wrappedDataPtr->clrxContext->dispatch;
                outProgram->amdOclProgram = program;
                outProgram->context = wrappedDataPtr->clrxContext;
                outProgram->transDevicesMap = wrappedDataPtr->transDevicesMap;
                clrxUpdateProgramAssocDevices(outProgram);
                outProgram->transDevicesMap = nullptr;
                clrxRetainOnlyCLRXContext(wrappedDataPtr->clrxContext);
            }
            wrappedDataPtr->clrxProgram = outProgram;
            wrappedDataPtr->clrxProgramFilled = true;
        }
        clrxProgram = wrappedDataPtr->clrxProgram;
        realUserData = wrappedDataPtr->realUserData;
    }
    catch(std::bad_alloc& ex)
    {
        std::cerr << "Out of memory on link callback" << std::endl;
        abort();
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal exception happened: " << ex.what() << std::endl;
        abort();
    }
    
    void (*realNotify)(cl_program program, void * user_data) = wrappedDataPtr->realNotify;
    if (!initializedByCallback) // if not initialized by this callback to delete
    {
        delete wrappedDataPtr->transDevicesMap;
        delete wrappedDataPtr;
    }
    
    realNotify(clrxProgram, realUserData);
}

CLRXProgram* clrxCreateCLRXProgram(CLRXContext* c, cl_program amdProgram,
          cl_int* errcode_ret)
{
    CLRXProgram* outProgram = nullptr;
    cl_int error = CL_SUCCESS;
    try
    {
        outProgram = new CLRXProgram;
        outProgram->dispatch = c->dispatch;
        outProgram->amdOclProgram = amdProgram;
        outProgram->context = c;
        error = clrxUpdateProgramAssocDevices(outProgram);
    }
    catch(const std::bad_alloc& ex)
    { error = CL_OUT_OF_HOST_MEMORY; }
    
    if (error != CL_SUCCESS)
    {
        delete outProgram;
        if (c->amdOclContext->dispatch->clReleaseProgram(amdProgram) != CL_SUCCESS)
        {
            std::cerr << "Fatal Error at handling error at program creation!" << std::endl;
            abort();
        }
        if (errcode_ret != nullptr)
            *errcode_ret = error;
        return nullptr;
    }
    
    clrxRetainOnlyCLRXContext(c);
    return outProgram;
}

cl_int clrxApplyCLRXEvent(CLRXCommandQueue* q, cl_event* event,
             cl_event amdEvent, cl_int status)
{
    CLRXEvent* outEvent = nullptr;
    if (event != nullptr && amdEvent != nullptr)
    {  // create event
        try
        {
            outEvent = new CLRXEvent;
            outEvent->dispatch = q->dispatch;
            outEvent->amdOclEvent = amdEvent;
            outEvent->commandQueue = q;
            outEvent->context = q->context;
            *event = outEvent;
        }
        catch (const std::bad_alloc& ex)
        {
            if (q->amdOclCommandQueue->dispatch->clReleaseEvent(amdEvent) != CL_SUCCESS)
            {
                std::cerr <<
                    "Fatal Error at handling error at apply event!" << std::endl;
                abort();
            }
            return CL_OUT_OF_HOST_MEMORY;
        }
        clrxRetainOnlyCLRXContext(q->context);
        clrxRetainOnlyCLRXCommandQueue(q);
    }
    
    return status;
}

cl_int clrxCreateOutDevices(CLRXDevice* d, cl_uint devicesNum,
       cl_device_id* out_devices, cl_int (*AMDReleaseDevice)(cl_device_id),
       const char* fatalErrorMessage)
{
    cl_uint dp = 0;
    try
    {
        for (dp = 0; dp < devicesNum; dp++)
        {
            CLRXDevice* device = new CLRXDevice;
            device->dispatch = d->dispatch;
            device->amdOclDevice = out_devices[dp];
            device->platform = d->platform;
            device->parent = d;
            if (d->extensionsSize != 0)
            {
                device->extensionsSize = d->extensionsSize;
                device->extensions = new char[d->extensionsSize];
                ::memcpy((char*)device->extensions, d->extensions, d->extensionsSize);
            }
            if (d->versionSize != 0)
            {
                device->versionSize = d->versionSize;
                device->version = new char[d->versionSize];
                ::memcpy((char*)device->version, d->version, d->versionSize);
            }
            out_devices[dp] = device;
        }
    }
    catch(const std::bad_alloc& ex)
    {   // revert translation
        for (cl_uint i = 0; i < dp; i++)
        {
            CLRXDevice* d = static_cast<CLRXDevice*>(out_devices[i]);
            out_devices[i] = d->amdOclDevice;
            delete d;
        }
        // free all subdevices
        for (cl_uint i = 0; i < devicesNum; i++)
        {
            if (AMDReleaseDevice(out_devices[i]) != CL_SUCCESS)
            {
                std::cerr << fatalErrorMessage << std::endl;
                abort();
            }
        }
        return CL_OUT_OF_HOST_MEMORY;
    }
    return CL_SUCCESS;
}

void clrxEventCallbackWrapper(cl_event event, cl_int exec_status, void * user_data)
{
    CLRXEventCallbackUserData* wrappedDataPtr =
            static_cast<CLRXEventCallbackUserData*>(user_data);
    CLRXEventCallbackUserData wrappedData = *wrappedDataPtr;
    // must be called only once (freeing wrapped data)
    delete wrappedDataPtr;
    wrappedData.realNotify(wrappedData.clrxEvent, exec_status,
                wrappedData.realUserData);
}

void clrxMemDtorCallbackWrapper(cl_mem memobj, void * user_data)
{
    CLRXMemDtorCallbackUserData* wrappedDataPtr =
            static_cast<CLRXMemDtorCallbackUserData*>(user_data);
    CLRXMemDtorCallbackUserData wrappedData = *wrappedDataPtr;
    // must be called only once (freeing wrapped data)
    delete wrappedDataPtr;
    wrappedData.realNotify(wrappedData.clrxMemObject, wrappedData.realUserData);
}

cl_int clrxInitKernelArgFlagsMap(CLRXProgram* program)
{
    if (program->kernelArgFlagsInitialized)
        return CL_SUCCESS; // if already initialized
    // clear before set up
    program->kernelArgFlagsMap.clear();
    
    if (program->assocDevicesNum == 0)
        return CL_SUCCESS;
    
    cl_program_binary_type ptype;
    cl_int status = program->amdOclProgram->dispatch->clGetProgramBuildInfo(
        program->amdOclProgram, program->assocDevices[0]->amdOclDevice,
        CL_PROGRAM_BINARY_TYPE, sizeof(cl_program_binary_type), &ptype, nullptr);
    if (status != CL_SUCCESS)
    {
        std::cerr << "Cant get program binary type" << std::endl;
        abort();
    }
    
    if (ptype != CL_PROGRAM_BINARY_TYPE_EXECUTABLE)
        return CL_SUCCESS; // do nothing if not executable
    
    unsigned char** binaries = nullptr;
    try
    {
        std::vector<size_t> binarySizes(program->assocDevicesNum);
        
        status = program->amdOclProgram->dispatch->clGetProgramInfo(
                program->amdOclProgram, CL_PROGRAM_BINARY_SIZES,
                sizeof(size_t)*program->assocDevicesNum, binarySizes.data(), nullptr);
        if (status != CL_SUCCESS)
        {
            std::cerr << "Cant get program binary sizes!" << std::endl;
            abort();
        }
        
        binaries = new unsigned char*[program->assocDevicesNum];
        std::fill(binaries, binaries + program->assocDevicesNum, nullptr);
        
        for (cl_uint i = 0; i < program->assocDevicesNum; i++)
            if (binarySizes[i] != 0) // if available
                binaries[i] = new unsigned char[binarySizes[i]];
        
        status = program->amdOclProgram->dispatch->clGetProgramInfo(
                program->amdOclProgram, CL_PROGRAM_BINARIES,
                sizeof(char*)*program->assocDevicesNum, binaries, nullptr);
        if (status != CL_SUCCESS)
        {
            std::cerr << "Cant get program binaries!" << std::endl;
            abort();
        }
        /* get kernel arg info from all binaries */
        for (cl_uint i = 0; i < program->assocDevicesNum; i++)
        {
            if (binaries[i] == nullptr)
                continue; // skip if not built for this device
            
            std::unique_ptr<AmdMainBinaryBase> amdBin(
                createAmdBinaryFromCode(binarySizes[i], (char*)(binaries[i]),
                             AMDBIN_CREATE_KERNELINFO));
            size_t kernelsNum = amdBin->getKernelInfosNum();
            const KernelInfo* kernelInfos = amdBin->getKernelInfos();
            /* create kernel argsflags map (for setKernelArg) */
            for (size_t i = 0; i < kernelsNum; i++)
            {
                const KernelInfo& kernelInfo = kernelInfos[i];
                std::vector<bool> kernelFlags(kernelInfo.argsNum<<1);
                for (cxuint k = 0; k < kernelInfo.argsNum; k++)
                {
                    const KernelArg& karg = kernelInfo.argInfos[k];
                    // if mem object (image or
                    kernelFlags[k<<1] = ((karg.argType == KernelArgType::POINTER &&
                            (karg.ptrSpace == KernelPtrSpace::GLOBAL ||
                             karg.ptrSpace == KernelPtrSpace::CONSTANT)) ||
                             karg.argType == KernelArgType::IMAGE);
                    // if sampler
                    kernelFlags[(k<<1)+1] = (karg.argType == KernelArgType::SAMPLER);
                }
                
                auto insertInfo = program->kernelArgFlagsMap.insert(
                    std::make_pair(kernelInfo.kernelName, kernelFlags));
                
                if (!insertInfo.second)
                { // if not inserted (already exists)
                    if ((insertInfo.first)->second != kernelFlags)
                    {   /* if not match!!! */
                        for (cl_uint x = 0; x < program->assocDevicesNum; x++)
                            delete[] binaries[x];
                        delete[] binaries;
                        return CL_INVALID_KERNEL_DEFINITION;
                    }
                }
            }
        }
    }
    catch(const std::bad_alloc& ex)
    {
        if (binaries != nullptr)
        {
            for (cl_uint x = 0; x < program->assocDevicesNum; x++)
                delete[] binaries[x];
            delete[] binaries;
        }
        return CL_OUT_OF_HOST_MEMORY;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal error at kernelArgFlagsMap creation: " <<
                ex.what() << std::endl;
        abort();
    }
    
    if (binaries != nullptr)
    {
        for (cl_uint x = 0; x < program->assocDevicesNum; x++)
            delete[] binaries[x];
        delete[] binaries;
    }
    program->kernelArgFlagsInitialized = true;
    return CL_SUCCESS;
}
