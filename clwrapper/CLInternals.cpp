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
#include <cstdio>
#include <vector>
#include <utility>
#include <thread>
#include <mutex>
#include <cstring>
#include <string>
#include <climits>
#include <cstdint>
#include <cstddef>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/utils/InputOutput.h>
#include <CLRX/utils/GPUId.h>
#include "CLWrapper.h"

using namespace CLRX;

OnceFlag clrxOnceFlag;
bool useCLRXWrapper = true;
/* use pure pointer - AMDOCL library must be available to end of program,
 * even after main routine and within atexit callback */
static DynLibrary* amdOclLibrary = nullptr;
cl_uint amdOclNumPlatforms = 0;
CLRXpfn_clGetPlatformIDs amdOclGetPlatformIDs = nullptr;
CLRXpfn_clUnloadCompiler amdOclUnloadCompiler = nullptr;
cl_int clrxWrapperInitStatus = CL_SUCCESS;

/* use pure pointer - all datas must be available to end of program,
 * even after main routine and within atexit callback */
CLRXPlatform* clrxPlatforms = nullptr;

#ifdef CL_VERSION_1_2
clEnqueueWaitSignalAMD_fn amdOclEnqueueWaitSignalAMD = nullptr;
clEnqueueWriteSignalAMD_fn amdOclEnqueueWriteSignalAMD = nullptr;
clEnqueueMakeBuffersResidentAMD_fn amdOclEnqueueMakeBuffersResidentAMD = nullptr;
#endif
CLRXpfn_clGetExtensionFunctionAddress amdOclGetExtensionFunctionAddress = nullptr;

/* extensions table - entries are sorted in function name's order */
CLRXExtensionEntry clrxExtensionsTable[18] =
{
#ifdef HAVE_OPENGL
    { "clCreateEventFromGLsyncKHR", (void*)clrxclCreateEventFromGLsyncKHR },
    { "clCreateFromGLBuffer", (void*)clrxclCreateFromGLBuffer },
    { "clCreateFromGLRenderbuffer", (void*)clrxclCreateFromGLRenderbuffer },
#else
    { "clCreateEventFromGLsyncKHR", nullptr },
    { "clCreateFromGLBuffer", nullptr },
    { "clCreateFromGLRenderbuffer", nullptr },
#endif
#if defined(CL_VERSION_1_2) && defined(HAVE_OPENGL)
    { "clCreateFromGLTexture", (void*)clrxclCreateFromGLTexture },
#else
    { "clCreateFromGLTexture", nullptr },
#endif
#ifdef HAVE_OPENGL
    { "clCreateFromGLTexture2D", (void*)clrxclCreateFromGLTexture2D },
    { "clCreateFromGLTexture3D", (void*)clrxclCreateFromGLTexture3D },
#else
    { "clCreateFromGLTexture2D", nullptr },
    { "clCreateFromGLTexture3D", nullptr },
#endif
    { "clCreateSubDevicesEXT", (void*)clrxclCreateSubDevicesEXT },
#ifdef HAVE_OPENGL
    { "clEnqueueAcquireGLObjects", (void*)clrxclEnqueueAcquireGLObjects },
#else
    { "clEnqueueAcquireGLObjects", nullptr },
#endif
#ifdef CL_VERSION_1_2
    { "clEnqueueMakeBuffersResidentAMD", (void*)clrxclEnqueueMakeBuffersResidentAMD },
#else
    { "clEnqueueMakeBuffersResidentAMD", nullptr },
#endif
#ifdef HAVE_OPENGL
    { "clEnqueueReleaseGLObjects", (void*)clrxclEnqueueReleaseGLObjects },
#else
    { "clEnqueueReleaseGLObjects", nullptr },
#endif
#ifdef CL_VERSION_1_2
    { "clEnqueueWaitSignalAMD", (void*)clrxclEnqueueWaitSignalAMD },
    { "clEnqueueWriteSignalAMD", (void*)clrxclEnqueueWriteSignalAMD },
#else
    { "clEnqueueWaitSignalAMD", nullptr },
    { "clEnqueueWriteSignalAMD", nullptr },
#endif
#ifdef HAVE_OPENGL
    { "clGetGLContextInfoKHR", (void*)clrxclGetGLContextInfoKHR },
    { "clGetGLObjectInfo", (void*)clrxclGetGLObjectInfo },
    { "clGetGLTextureInfo", (void*)clrxclGetGLTextureInfo },
#else
    { "clGetGLContextInfoKHR", nullptr },
    { "clGetGLObjectInfo", nullptr },
    { "clGetGLTextureInfo", nullptr },
#endif
    { "clIcdGetPlatformIDsKHR", (void*)clrxclIcdGetPlatformIDsKHR },
    { "clReleaseDeviceEXT", (void*)clrxclReleaseDeviceEXT },
    { "clRetainDeviceEXT", (void*)clrxclRetainDeviceEXT }
};

/* dispatch structure */
const CLRXIcdDispatch clrxDispatchRecord =
{ {
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
#ifdef HAVE_OPENGL
    clrxclCreateFromGLBuffer,
    clrxclCreateFromGLTexture2D,
    clrxclCreateFromGLTexture3D,
    clrxclCreateFromGLRenderbuffer,
    clrxclGetGLObjectInfo,
    clrxclGetGLTextureInfo,
    clrxclEnqueueAcquireGLObjects,
    clrxclEnqueueReleaseGLObjects,
    clrxclGetGLContextInfoKHR,
#else
    nullptr, // clrxclCreateFromGLBuffer,
    nullptr, // clrxclCreateFromGLTexture2D,
    nullptr, // clrxclCreateFromGLTexture3D,
    nullptr, // clrxclCreateFromGLRenderbuffer,
    nullptr, // clrxclGetGLObjectInfo,
    nullptr, // clrxclGetGLTextureInfo,
    nullptr, // clrxclEnqueueAcquireGLObjects,
    nullptr, // clrxclEnqueueReleaseGLObjects,
    nullptr, // clrxclGetGLContextInfoKHR,
#endif

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

#ifdef HAVE_OPENGL
    clrxclCreateEventFromGLsyncKHR
#else
    nullptr
#endif

#ifdef CL_VERSION_1_2
    ,
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
#ifdef HAVE_OPENGL
    clrxclCreateFromGLTexture,
#else
    nullptr,
#endif

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
#endif
#ifdef CL_VERSION_2_0
    ,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    clrxclCreateCommandQueueWithProperties,
    clrxclCreatePipe,
    clrxclGetPipeInfo,
    clrxclSVMAlloc,
    clrxclSVMFree,
    clrxclEnqueueSVMFree,
    clrxclEnqueueSVMMemcpy,
    clrxclEnqueueSVMMemFill,
    clrxclEnqueueSVMMap,
    clrxclEnqueueSVMUnmap,
    clrxclCreateSamplerWithProperties,
    clrxclSetKernelArgSVMPointer,
    clrxclSetKernelExecInfo
#endif
} };

void clrxAbort(const char* abortStr)
{
    std::cerr << abortStr << std::endl;
    abort();
}

CLRX_INTERNAL void clrxAbort(const char* abortStr, const char* exStr)
{
    std::cerr << abortStr << exStr << std::endl;
    abort();
}

void clrxReleaseOnlyCLRXDevice(CLRXDevice* device)
{
    if (device->parent != nullptr)
        if (device->refCount.fetch_sub(1) == 1)
        {
            // amdOclDevice has been already released, we release only our device
            clrxReleaseOnlyCLRXDevice(device->parent);
            delete device;
        }
}

void clrxReleaseOnlyCLRXContext(CLRXContext* context)
{
    if (context->refCount.fetch_sub(1) == 1)
    {
        // amdOclContext has been already released, we release only our context
        for (cl_uint i = 0; i < context->devicesNum; i++)
            clrxReleaseOnlyCLRXDevice(context->devices[i]);
        delete context;
    }
}

void clrxReleaseOnlyCLRXMemObject(CLRXMemObject* memObject)
{
    if (memObject->refCount.fetch_sub(1) == 1)
    {
        // amdOclContext has been already released, we release only our context
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
    std::unique_ptr<DynLibrary> tmpAmdOclLibrary = nullptr;
    try
    {
        useCLRXWrapper = !parseEnvVariable<bool>("CLRX_FORCE_ORIGINAL_AMDOCL", false);
        std::string amdOclPath = findAmdOCL();
        /// set temporary amd ocl library
        tmpAmdOclLibrary.reset(new DynLibrary(amdOclPath.c_str(), DYNLIB_NOW));
        // get relevant OpenCL function pointers
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
#ifdef CL_VERSION_1_2
        amdOclEnqueueWaitSignalAMD = (clEnqueueWaitSignalAMD_fn)
            amdOclGetExtensionFunctionAddress("clEnqueueWaitSignalAMD");
        amdOclEnqueueWriteSignalAMD = (clEnqueueWriteSignalAMD_fn)
            amdOclGetExtensionFunctionAddress("clEnqueueWriteSignalAMD");
        amdOclEnqueueMakeBuffersResidentAMD = (clEnqueueMakeBuffersResidentAMD_fn)
            amdOclGetExtensionFunctionAddress("clEnqueueMakeBuffersResidentAMD");
#endif
        
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
            return;
        }
        
        if (platformCount == 0)
            return;
        
        std::vector<cl_platform_id> amdOclPlatforms(platformCount, nullptr);
        status = pgetPlatformIDs(platformCount, amdOclPlatforms.data(), nullptr);
        if (status != CL_SUCCESS)
        {
            clrxWrapperInitStatus = status;
            return;
        }
        
        if (useCLRXWrapper)
        {
            const cxuint clrxExtEntriesNum =
                    sizeof(clrxExtensionsTable)/sizeof(CLRXExtensionEntry);
            
            std::unique_ptr<CLRXPlatform[]> tmpClrxPlatforms(
                                new CLRXPlatform[platformCount]);
            for (cl_uint i = 0; i < platformCount; i++)
            {
                if (amdOclPlatforms[i] == nullptr)
                    continue;
                
                const cl_platform_id amdOclPlatform = amdOclPlatforms[i];
                CLRXPlatform& clrxPlatform = tmpClrxPlatforms[i];
                
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
                std::unique_ptr<char[]> extsBuffer(new char[extsSize + 19]);
                if (amdOclPlatform->dispatch->clGetPlatformInfo(amdOclPlatform,
                        CL_PLATFORM_EXTENSIONS, extsSize, extsBuffer.get(),
                        nullptr) != CL_SUCCESS)
                    continue;
                if (extsSize > 2 && extsBuffer[extsSize-2] != ' ')
                    ::strcat(extsBuffer.get(), " cl_radeon_extender");
                else
                    ::strcat(extsBuffer.get(), "cl_radeon_extender");
                clrxPlatform.extensionsSize = ::strlen(extsBuffer.get())+1;
                clrxPlatform.extensions = std::move(extsBuffer);
                
                // add to version " (clrx x.y.z)"
                size_t versionSize;
                if (amdOclPlatform->dispatch->clGetPlatformInfo(amdOclPlatform,
                            CL_PLATFORM_VERSION, 0, nullptr, &versionSize) != CL_SUCCESS)
                    continue;
                std::unique_ptr<char[]> versionBuffer(new char[versionSize+30]);
                if (amdOclPlatform->dispatch->clGetPlatformInfo(amdOclPlatform,
                        CL_PLATFORM_VERSION, versionSize,
                        versionBuffer.get(), nullptr) != CL_SUCCESS)
                    continue;
                {
                    char verBuf[30];
                    snprintf(verBuf, 30, " (clrx %s)", CLRX_VERSION);
                    ::strcat(versionBuffer.get(), verBuf);
                }
                clrxPlatform.versionSize = ::strlen(versionBuffer.get())+1;
                clrxPlatform.version = std::move(versionBuffer);
                
                /* parse OpenCL version */
                cxuint openCLmajor = 0;
                cxuint openCLminor = 0;
                const char* verEnd = nullptr;
                if (::strncmp(clrxPlatform.version.get(), "OpenCL ", 7) == 0)
                {
                    try
                    {
                        openCLmajor = CLRX::cstrtoui(
                                clrxPlatform.version.get() + 7, nullptr, verEnd);
                        if (verEnd != nullptr && *verEnd == '.')
                        {
                            const char* verEnd2 = nullptr;
                            openCLminor = CLRX::cstrtoui(verEnd + 1, nullptr, verEnd2);
                            if (openCLmajor > UINT16_MAX || openCLminor > UINT16_MAX)
                                openCLmajor = openCLminor = 0; // if failed
                        }
                    }
                    catch(const ParseException& ex)
                    { /* ignore parse exception */ }
                    clrxPlatform.openCLVersionNum = getOpenCLVersionNum(
                                    openCLmajor, openCLminor);
                }
                /*std::cout << "Platform OCL version: " <<
                        openCLmajor << "." << openCLminor << std::endl;*/
                
                /* initialize extTable */
                clrxPlatform.extEntries.reset(new CLRXExtensionEntry[clrxExtEntriesNum]);
                std::copy(clrxExtensionsTable, clrxExtensionsTable + clrxExtEntriesNum,
                          clrxPlatform.extEntries.get());
                
                if (clrxPlatform.openCLVersionNum >= getOpenCLVersionNum(1, 2))
                {
                    /* update p->extEntries for platform */
                    for (size_t k = 0; k < clrxExtEntriesNum; k++)
                    {
                        // erase CLRX extension entry if not reflected in AMD extensions
                        CLRXExtensionEntry& extEntry = clrxPlatform.extEntries[k];
#ifdef CL_VERSION_1_2
                        if (amdOclPlatform->dispatch->
                            clGetExtensionFunctionAddressForPlatform
                                    (amdOclPlatform, extEntry.funcname) == nullptr)
                            extEntry.address = nullptr;
#else
                        if (amdOclGetExtensionFunctionAddress(extEntry.funcname) == nullptr)
                            extEntry.address = nullptr;
#endif
                    }
                    /* end of p->extEntries for platform */
                }
                
                /* filtering for different OpenCL standards */
                size_t icdEntriesToKept = CLRXICD_ENTRIES_NUM;
                if (clrxPlatform.openCLVersionNum < getOpenCLVersionNum(1, 1))
                    // if earlier than OpenCL 1.1
                    icdEntriesToKept =
                            offsetof(CLRXIcdDispatch, clSetEventCallback)/sizeof(void*);
#ifdef CL_VERSION_1_2
                else if (clrxPlatform.openCLVersionNum < getOpenCLVersionNum(1, 2))
                    // if earlier than OpenCL 1.2
                    icdEntriesToKept =
                            offsetof(CLRXIcdDispatch, clCreateSubDevices)/sizeof(void*);
#endif
#ifdef CL_VERSION_2_0
                else if (clrxPlatform.openCLVersionNum < getOpenCLVersionNum(2, 0))
                    // if earlier than OpenCL 2.0
                    icdEntriesToKept = offsetof(CLRXIcdDispatch,
                                clCreateFromEGLImageKHR)/sizeof(void*);
#endif
                
                // zeroing unsupported entries for later version of OpenCL standard
                std::fill(clrxPlatform.dispatch->entries + icdEntriesToKept,
                          clrxPlatform.dispatch->entries + CLRXICD_ENTRIES_NUM, nullptr);
                
                for (size_t k = 0; k < icdEntriesToKept; k++)
                    /* disable function when not in original driver */
                    if (amdOclPlatform->dispatch->entries[k] == nullptr)
                        clrxPlatform.dispatch->entries[k] = nullptr;
            }
            clrxPlatforms = tmpClrxPlatforms.release();
        }
        
        amdOclLibrary = tmpAmdOclLibrary.release();
        amdOclNumPlatforms = platformCount;
    }
    catch(const std::bad_alloc& ex)
    {
        clrxWrapperInitStatus = CL_OUT_OF_HOST_MEMORY;
        return;
    }
}

struct ManCLContext
{
    cl_context clContext;
    
    ManCLContext() : clContext(nullptr) { }
    ~ManCLContext()
    {
        if (clContext != nullptr)
            if (clContext->dispatch->clReleaseContext(clContext) != CL_SUCCESS)
                clrxAbort("Can't release amdOfflineContext!");
    }
    cl_context operator()()
    { return clContext; }
    
    ManCLContext& operator=(cl_context c)
    {
        clContext = c;
        return *this;
    }
};

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
#ifdef CL_VERSION_1_2
    bool isOCL12OrLater = platform->openCLVersionNum >= getOpenCLVersionNum(1, 2);
    //std::cout << "IsOCl12OrLater: " << isOCL12OrLater << std::endl;
    
    ManCLContext offlineContext;
    cl_uint amdOfflineDevicesNum = 0;
    cl_uint offlineContextDevicesNum = 0;
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
        
        if (::strstr(platform->extensions.get(), "cl_amd_offline_devices") != nullptr)
        {
            // if amd offline devices is available
            cl_context_properties clctxprops[5];
            clctxprops[0] = CL_CONTEXT_PLATFORM;
            clctxprops[1] = (cl_context_properties)platform->amdOclPlatform;
            clctxprops[2] = CL_CONTEXT_OFFLINE_DEVICES_AMD;
            clctxprops[3] = (cl_context_properties)1;
            clctxprops[4] = 0;
            offlineContext = platform->amdOclPlatform->dispatch->
                    clCreateContextFromType(clctxprops, CL_DEVICE_TYPE_ALL,
                                NULL, NULL, &status);
            
            if (offlineContext() == nullptr)
            {
                platform->devicesNum = 0;
                platform->deviceInitStatus = status;
                return;
            }
            
            status = offlineContext()->dispatch->clGetContextInfo(offlineContext(),
                    CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint),
                    &offlineContextDevicesNum, nullptr);
            
            if (status == CL_SUCCESS &&
                offlineContextDevicesNum > (platform->devicesNum - customDevicesNum))
            {
                amdOfflineDevicesNum = offlineContextDevicesNum -
                        (platform->devicesNum - customDevicesNum);
                platform->devicesNum += amdOfflineDevicesNum;
            }
            else // no additional offline devices
                amdOfflineDevicesNum = 0;
        }
    }
#else
    cl_uint customDevicesNum = 0;
    cl_uint amdOfflineDevicesNum = 0;
#endif
    
    try
    {
        std::vector<cl_device_id> amdDevices(platform->devicesNum);
        platform->devicesArray = new CLRXDevice[platform->devicesNum];
    
        /* get AMD devices */
        status = platform->amdOclPlatform->dispatch->clGetDeviceIDs(
                platform->amdOclPlatform, CL_DEVICE_TYPE_ALL,
                platform->devicesNum-customDevicesNum-amdOfflineDevicesNum,
                amdDevices.data(), nullptr);
        if (status != CL_SUCCESS)
        {
            delete[] platform->devicesArray;
            platform->devicesNum = 0;
            platform->devicesArray = nullptr;
            platform->deviceInitStatus = status;
            return;
        }
#ifdef CL_VERSION_1_2
        // custom devices
        if (customDevicesNum != 0)
        {
            status = platform->amdOclPlatform->dispatch->clGetDeviceIDs(
                    platform->amdOclPlatform, CL_DEVICE_TYPE_CUSTOM, customDevicesNum,
                    amdDevices.data() + platform->devicesNum-
                        customDevicesNum-amdOfflineDevicesNum, nullptr);
            if (status != CL_SUCCESS)
            {
                delete[] platform->devicesArray;
                platform->devicesNum = 0;
                platform->devicesArray = nullptr;
                platform->deviceInitStatus = status;
                return;
            }
        }
        /// get all AMD offline devices for enumerating and finding devices
        if (amdOfflineDevicesNum != 0)
        {
            std::vector<cl_device_id> offlineDevices(offlineContextDevicesNum);
            status = offlineContext()->dispatch->clGetContextInfo(offlineContext(),
                    CL_CONTEXT_DEVICES, sizeof(cl_device_id)*offlineContextDevicesNum,
                    offlineDevices.data(), nullptr);
            
            if (status != CL_SUCCESS)
            {
                delete[] platform->devicesArray;
                platform->devicesNum = 0;
                platform->devicesArray = nullptr;
                platform->deviceInitStatus = status;
                return;
            }
            /* filter: only put unavailable devices (offline) */
            cl_uint k = platform->devicesNum-amdOfflineDevicesNum;
            /* using std::vector, some strange fails on Catalyst 15.7 when
             * NBody tries to dump kernel code */
            std::vector<cl_device_id> normalDevices(
                        amdDevices.begin(), amdDevices.begin()+k);
            std::sort(normalDevices.begin(), normalDevices.end());
            for (cl_device_id deviceId: offlineDevices)
            {
                /* broken CL_DEVICE_AVAILABLE in latest driver (Catalyst 15.7) */
                //if (!available)
                if (binaryFind(normalDevices.begin(), normalDevices.end(), deviceId)
                            == normalDevices.end())
                    amdDevices[k++] = deviceId;
            }
        }
#endif
        
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
            std::unique_ptr<char[]> extsBuffer(new char[extsSize+19]);
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i],
                  CL_DEVICE_EXTENSIONS, extsSize, extsBuffer.get(), nullptr);
            if (status != CL_SUCCESS)
                break;
            if (extsSize > 2 && extsBuffer[extsSize-2] != ' ')
                strcat(extsBuffer.get(), " cl_radeon_extender");
            else
                strcat(extsBuffer.get(), "cl_radeon_extender");
            clrxDevice.extensionsSize = ::strlen(extsBuffer.get())+1;
            clrxDevice.extensions = std::move(extsBuffer);
            
            // add to version " (clrx x.y.z)"
            size_t versionSize;
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i],
                      CL_DEVICE_VERSION, 0, nullptr, &versionSize);
            if (status != CL_SUCCESS)
                break;
            std::unique_ptr<char[]> versionBuffer(new char[versionSize+30]);
            status = amdDevices[i]->dispatch->clGetDeviceInfo(amdDevices[i],
                      CL_DEVICE_VERSION, versionSize, versionBuffer.get(), nullptr);
            if (status != CL_SUCCESS)
                break;
            {
                char verBuf[30];
                snprintf(verBuf, 30, " (clrx %s)", CLRX_VERSION);
                ::strcat(versionBuffer.get(), verBuf);
            }
            clrxDevice.versionSize = ::strlen(versionBuffer.get())+1;
            clrxDevice.version = std::move(versionBuffer);
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
        platform->devicePtrs.reset(new CLRXDevice*[platform->devicesNum]);
        for (cl_uint i = 0; i < platform->devicesNum; i++)
            platform->devicePtrs[i] = platform->devicesArray + i;
    }
    catch(const std::bad_alloc& ex)
    {
        // error at memory allocation
        delete[] platform->devicesArray;
        platform->devicesNum = 0;
        platform->devicesArray = nullptr;
        platform->deviceInitStatus = CL_OUT_OF_HOST_MEMORY;
        return;
    }
    catch(...)
    {
        delete[] platform->devicesArray;
        platform->devicesNum = 0;
        platform->devicesArray = nullptr;
        throw;
    }
}

/*
 * clrx object utils
 */

static inline bool clrxDeviceCompareByAmdDevice(const CLRXDevice* l, const CLRXDevice* r)
{
    return l->amdOclDevice < r->amdOclDevice;
}

void translateAMDDevicesIntoCLRXDevices(cl_uint allDevicesNum,
           const CLRXDevice** allDevices, cl_uint amdDevicesNum, cl_device_id* amdDevices)
{
    /* after it we replaces amdDevices into ours devices */
    if (allDevicesNum < 16)  //efficient for small
    {  
        // if smaller number of devices (sorting is not needed)
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
                clrxAbort("Fatal error at translating AMD devices");
        }
    }
    else if(amdDevicesNum != 0) // sorting (for more devices)
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
            const auto& found = binaryFind(sortedOriginal.begin(),
                     newEnd, &tmpDevice, clrxDeviceCompareByAmdDevice);
            if (found != newEnd)
                amdDevices[i] = (cl_device_id)(*found);
            else
                clrxAbort("Fatal error at translating AMD devices");
        }
    }
}

/* called always on creating context (clCreateContextFromType) */
cl_int clrxSetContextDevices(CLRXContext* c, const CLRXPlatform* platform)
{
    cl_uint amdDevicesNum;
    cl_int status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
        CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &amdDevicesNum, nullptr);
    if (status != CL_SUCCESS)
        return status;
    
    std::unique_ptr<cl_device_id[]> amdDevices(new cl_device_id[amdDevicesNum]);
    status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
            CL_CONTEXT_DEVICES, sizeof(cl_device_id)*amdDevicesNum,
            amdDevices.get(), nullptr);
    if (status != CL_SUCCESS)
        return status;

    try
    {
        translateAMDDevicesIntoCLRXDevices(platform->devicesNum,
               (const CLRXDevice**)(platform->devicePtrs.get()), amdDevicesNum,
               amdDevices.get());
        // now is ours devices
        c->devicesNum = amdDevicesNum;
        c->devices.reset(reinterpret_cast<CLRXDevice**>(amdDevices.release()));
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    return CL_SUCCESS;
}

/* called always on creating context (clCreateContext) */
cl_int clrxSetContextDevices(CLRXContext* c, cl_uint inDevicesNum,
            const cl_device_id* inDevices)
{
    cl_uint amdDevicesNum;
    
    cl_int status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
        CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &amdDevicesNum, nullptr);
    if (status != CL_SUCCESS)
        return status;
    
    std::unique_ptr<cl_device_id> amdDevices(new cl_device_id[amdDevicesNum]);
    status = c->amdOclContext->dispatch->clGetContextInfo(c->amdOclContext,
            CL_CONTEXT_DEVICES, sizeof(cl_device_id)*amdDevicesNum,
            amdDevices.get(), nullptr);
    if (status != CL_SUCCESS)
        return status;
    
    try
    {
        translateAMDDevicesIntoCLRXDevices(inDevicesNum, (const CLRXDevice**)inDevices,
                       amdDevicesNum, amdDevices.get());
        // now is ours devices
        c->devicesNum = amdDevicesNum;
        c->devices.reset(reinterpret_cast<CLRXDevice**>(amdDevices.release()));
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    return CL_SUCCESS;
}

/* update associates devices for program (called when concurrent building working or
 * after program building */
cl_int clrxUpdateProgramAssocDevices(CLRXProgram* p)
{
    size_t amdAssocDevicesNum;
    try
    {
        cl_uint totalDevicesNum = (p->transDevicesMap != nullptr) ?
                    p->transDevicesMap->size() : p->context->devicesNum;
        
        std::unique_ptr<cl_device_id[]> amdAssocDevices(new cl_device_id[totalDevicesNum]);
        cl_program amdProg = (p->amdOclAsmProgram!=nullptr) ? p->amdOclAsmProgram :
                p->amdOclProgram;
        
        // single OpenCL call should be atomic:
        // reason: can be called between clBuildProgram which changes associated devices
        const cl_int status = p->amdOclProgram->dispatch->clGetProgramInfo(
            amdProg, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*totalDevicesNum,
                      amdAssocDevices.get(), &amdAssocDevicesNum);
        
        if (status != CL_SUCCESS)
            return status;
        
        amdAssocDevicesNum /= sizeof(cl_device_id); // number of amd devices
        
        if (totalDevicesNum != amdAssocDevicesNum)
        {
            /* reallocate amdAssocDevices */
            std::unique_ptr<cl_device_id[]> tmpAmdAssocDevices(
                        new cl_device_id[amdAssocDevicesNum]);
            std::copy(amdAssocDevices.get(), amdAssocDevices.get()+amdAssocDevicesNum,
                      tmpAmdAssocDevices.get());
            amdAssocDevices = std::move(tmpAmdAssocDevices);
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
            if (!haveDiffs) // no differences between calls
                return CL_SUCCESS;
        }
        
        if (p->transDevicesMap != nullptr)
            for (cl_uint i = 0; i < amdAssocDevicesNum; i++)
            {
                // translate AMD device to CLRX device
                CLRXProgramDevicesMap::const_iterator found =
                        p->transDevicesMap->find(amdAssocDevices[i]);
                if (found == p->transDevicesMap->end())
                    clrxAbort("Fatal error at translating AMD devices");
                amdAssocDevices[i] = found->second;
            }
        else
            translateAMDDevicesIntoCLRXDevices(p->context->devicesNum,
                   const_cast<const CLRXDevice**>(p->context->devices.get()),
                   amdAssocDevicesNum, static_cast<cl_device_id*>(amdAssocDevices.get()));
        
        p->assocDevicesNum = amdAssocDevicesNum;
        p->assocDevices.reset(reinterpret_cast<CLRXDevice**>(amdAssocDevices.release()));
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    return CL_SUCCESS;
}

/* this wrapper packages program_notify, it passes ours program to real notify */
void CL_CALLBACK clrxBuildProgramNotifyWrapper(cl_program program, void * user_data)
{
    CLRXBuildProgramUserData* wrappedDataPtr =
            static_cast<CLRXBuildProgramUserData*>(user_data);
    CLRXBuildProgramUserData wrappedData = *wrappedDataPtr;
    CLRXProgram* p = wrappedDataPtr->clrxProgram;
    try
    {
        std::lock_guard<std::mutex> l(p->mutex);
        if (!wrappedDataPtr->inClFunction)
        {
            // do it if not done in clBuildProgram
            const cl_int newStatus = clrxUpdateProgramAssocDevices(p);
            if (newStatus != CL_SUCCESS)
                clrxAbort("Fatal error: cant update programAssocDevices");
            clrxReleaseConcurrentBuild(p);
        }
        wrappedDataPtr->callDone = true;
        if (wrappedDataPtr->inClFunction) // delete if done in clBuildProgram
        {
            //std::cout << "Delete WrappedData: " << wrappedDataPtr << std::endl;
            delete wrappedDataPtr;
        }
    }
    catch(const std::exception& ex)
    { clrxAbort("Fatal exception happened: ", ex.what()); }
    
    // must be called only once (freeing wrapped data)
    wrappedData.realNotify(wrappedData.clrxProgram, wrappedData.realUserData);
}

/* this wrapper packages program_notify, it prepare new output program with
 * translated devices to (from its device to ours devices) */
void CL_CALLBACK clrxLinkProgramNotifyWrapper(cl_program program, void * user_data)
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
    { clrxAbort("Out of memory on link callback"); }
    catch(const std::exception& ex)
    { clrxAbort("Fatal exception happened: ", ex.what()); }
    
    void (CL_CALLBACK *realNotify)(cl_program program, void * user_data) =
            wrappedDataPtr->realNotify;
    if (!initializedByCallback) // if not initialized by this callback to delete
    {
        delete wrappedDataPtr->transDevicesMap;
        delete wrappedDataPtr;
    }
    
    realNotify(clrxProgram, realUserData);
}

// helper for clCreateProgramXXX
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
            clrxAbort("Fatal Error at handling error at program creation!");
        if (errcode_ret != nullptr)
            *errcode_ret = error;
        return nullptr;
    }
    
    clrxRetainOnlyCLRXContext(c);
    return outProgram;
}

/// helper called while creating command event
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
                clrxAbort("Fatal Error at handling error at apply event!");
            return CL_OUT_OF_HOST_MEMORY;
        }
        clrxRetainOnlyCLRXContext(q->context);
        clrxRetainOnlyCLRXCommandQueue(q);
    }
    
    return status;
}

/* helper for clCreateSubDevices - creates CLRX devices, set extension and version info
 * from parent CLRX device. includes exception handling */
cl_int clrxCreateOutDevices(CLRXDevice* d, cl_uint devicesNum,
       cl_device_id* out_devices, cl_int (CL_API_CALL *AMDReleaseDevice)(cl_device_id),
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
            // copy ours extensions and versions strings to subdevice
            if (d->extensionsSize != 0)
            {
                device->extensionsSize = d->extensionsSize;
                device->extensions.reset(new char[d->extensionsSize]);
                ::memcpy(device->extensions.get(), d->extensions.get(), d->extensionsSize);
            }
            if (d->versionSize != 0)
            {
                device->versionSize = d->versionSize;
                device->version.reset(new char[d->versionSize]);
                ::memcpy(device->version.get(), d->version.get(), d->versionSize);
            }
            out_devices[dp] = device;
        }
    }
    catch(const std::bad_alloc& ex)
    {
        // revert translation
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
                clrxAbort(fatalErrorMessage);
        }
        return CL_OUT_OF_HOST_MEMORY;
    }
    return CL_SUCCESS;
}

void CL_CALLBACK clrxEventCallbackWrapper(cl_event event, cl_int exec_status,
                          void * user_data)
{
    CLRXEventCallbackUserData* wrappedDataPtr =
            static_cast<CLRXEventCallbackUserData*>(user_data);
    CLRXEventCallbackUserData wrappedData = *wrappedDataPtr;
    // must be called only once (freeing wrapped data)
    delete wrappedDataPtr;
    wrappedData.realNotify(wrappedData.clrxEvent, exec_status, wrappedData.realUserData);
}

void CL_CALLBACK clrxMemDtorCallbackWrapper(cl_mem memobj, void * user_data)
{
    CLRXMemDtorCallbackUserData* wrappedDataPtr =
            static_cast<CLRXMemDtorCallbackUserData*>(user_data);
    CLRXMemDtorCallbackUserData wrappedData = *wrappedDataPtr;
    // must be called only once (freeing wrapped data)
    delete wrappedDataPtr;
    wrappedData.realNotify(wrappedData.clrxMemObject, wrappedData.realUserData);
}

#ifdef CL_VERSION_2_0
void CL_CALLBACK clrxSVMFreeCallbackWrapper(cl_command_queue queue,
      cl_uint num_svm_pointers, void** svm_pointers, void* user_data)
{
    CLRXSVMFreeCallbackUserData* wrappedDataPtr =
            static_cast<CLRXSVMFreeCallbackUserData*>(user_data);
    CLRXSVMFreeCallbackUserData wrappedData = *wrappedDataPtr;
    delete wrappedDataPtr;
    wrappedData.realNotify(wrappedData.clrxCommandQueue, num_svm_pointers, svm_pointers,
           wrappedData.realUserData);
}
#endif

cl_int clrxInitKernelArgFlagsMap(CLRXProgram* program)
{
    if (program->kernelArgFlagsInitialized)
        return CL_SUCCESS; // if already initialized
    // clear before set up
    program->kernelArgFlagsMap.clear();
    
    if (program->assocDevicesNum == 0)
        return CL_SUCCESS;
    
    cl_program amdProg = (program->asmState.load() != CLRXAsmState::NONE) ?
            program->amdOclAsmProgram : program->amdOclProgram;
#ifdef CL_VERSION_1_2
    cl_program_binary_type ptype;
    cl_int status = program->amdOclProgram->dispatch->clGetProgramBuildInfo(
        amdProg, program->assocDevices[0]->amdOclDevice,
        CL_PROGRAM_BINARY_TYPE, sizeof(cl_program_binary_type), &ptype, nullptr);
    if (status != CL_SUCCESS)
        clrxAbort("Can't get program binary type");
    
    if (ptype != CL_PROGRAM_BINARY_TYPE_EXECUTABLE)
        return CL_SUCCESS; // do nothing if not executable
#else
    cl_int status = CL_SUCCESS;
#endif
    
    std::unique_ptr<std::unique_ptr<unsigned char[]>[]> binaries = nullptr;
    try
    {
        std::vector<size_t> binarySizes(program->assocDevicesNum);
        
        status = program->amdOclProgram->dispatch->clGetProgramInfo(amdProg,
                CL_PROGRAM_BINARY_SIZES, sizeof(size_t)*program->assocDevicesNum,
                binarySizes.data(), nullptr);
        if (status != CL_SUCCESS)
            clrxAbort("Can't get program binary sizes!");
        
        binaries.reset(new std::unique_ptr<unsigned char[]>[program->assocDevicesNum]);
        
        for (cl_uint i = 0; i < program->assocDevicesNum; i++)
            if (binarySizes[i] != 0) // if available
                binaries[i].reset(new unsigned char[binarySizes[i]]);
        
        status = program->amdOclProgram->dispatch->clGetProgramInfo(amdProg,
                CL_PROGRAM_BINARIES, sizeof(char*)*program->assocDevicesNum,
                (unsigned char**)binaries.get(), nullptr);
        if (status != CL_SUCCESS)
            clrxAbort("Can't get program binaries!");
        /* get kernel arg info from all binaries */
        for (cl_uint i = 0; i < program->assocDevicesNum; i++)
        {
            if (binaries[i] == nullptr)
                continue; // skip if not built for this device
            
            std::unique_ptr<AmdMainBinaryBase> amdBin;
            bool binCL20 = false;
            if (isAmdCL2Binary(binarySizes[i], binaries[i].get()))
            {
                amdBin.reset(createAmdCL2BinaryFromCode(binarySizes[i], binaries[i].get(),
                             AMDBIN_CREATE_KERNELINFO));
                binCL20 = true;
            }
            else /* CL1.2 binary format */
                amdBin.reset(createAmdBinaryFromCode(binarySizes[i], binaries[i].get(),
                             AMDBIN_CREATE_KERNELINFO));
            
            size_t kernelsNum = amdBin->getKernelInfosNum();
            const KernelInfo* kernelInfos = amdBin->getKernelInfos();
            /* create kernel argsflags map (for setKernelArg) */
            const size_t oldKernelMapSize = program->kernelArgFlagsMap.size();
            program->kernelArgFlagsMap.resize(oldKernelMapSize+kernelsNum);
            for (size_t i = 0; i < kernelsNum; i++)
            {
                const KernelInfo& kernelInfo = kernelInfos[i];
                const cxuint kStart = binCL20 ? 6 : 0;
                if (binCL20 && kernelInfo.argInfos.size() < 6)
                    throw Exception("OpenCL2.0 kernel must have 6 setup arguments!");
                std::vector<bool> kernelFlags((kernelInfo.argInfos.size()-kStart)<<1);
                 /* for CL2 binformat: 6 args is kernel setup */
                for (cxuint k = 0; k < kernelInfo.argInfos.size()-kStart; k++)
                {
                    const AmdKernelArg& karg = kernelInfo.argInfos[k+kStart];
                    // if mem object (image, buffer or counter32)
                    kernelFlags[k<<1] = ((karg.argType == KernelArgType::POINTER &&
                            (karg.ptrSpace == KernelPtrSpace::GLOBAL ||
                             karg.ptrSpace == KernelPtrSpace::CONSTANT)) ||
                             isKernelArgImage(karg.argType) ||
                             karg.argType == KernelArgType::PIPE ||
                             karg.argType == KernelArgType::COUNTER32 ||
                             karg.argType == KernelArgType::COUNTER64);
                    // if sampler
                    kernelFlags[(k<<1)+1] = (karg.argType == KernelArgType::SAMPLER);
                    if (karg.argType == KernelArgType::CMDQUEUE)
                        // if command queue
                        kernelFlags[k<<1] = kernelFlags[(k<<1)+1] = true;
                }
                
                program->kernelArgFlagsMap[oldKernelMapSize+i] =
                        std::make_pair(kernelInfo.kernelName, kernelFlags);
            }
            CLRX::mapSort(program->kernelArgFlagsMap.begin(),
                      program->kernelArgFlagsMap.end());
            size_t j = 1;
            for (size_t k = 1; k < program->kernelArgFlagsMap.size(); k++)
                if (program->kernelArgFlagsMap[k].first ==
                    program->kernelArgFlagsMap[k-1].first)
                {
                    if (program->kernelArgFlagsMap[k].second !=
                        program->kernelArgFlagsMap[k-1].second) /* if not match!!! */
                        return CL_INVALID_KERNEL_DEFINITION;
                    continue;
                }
                else // copy to new place
                    program->kernelArgFlagsMap[j++] = program->kernelArgFlagsMap[k];
            program->kernelArgFlagsMap.resize(j);
        }
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    catch(const std::exception& ex)
    { clrxAbort("Fatal error at kernelArgFlagsMap creation: ", ex.what()); }
    
    program->kernelArgFlagsInitialized = true;
    return CL_SUCCESS;
}


void clrxInitProgramTransDevicesMap(CLRXProgram* program,
            cl_uint num_devices, const cl_device_id* device_list,
            const std::vector<cl_device_id>& amdDevices)
{
    /* initialize transDevicesMap if not needed */
    if (program->transDevicesMap == nullptr) // if not initialized
    {
        // initialize transDevicesMap
        program->transDevicesMap = new CLRXProgramDevicesMap;
        for (cl_uint i = 0; i < program->context->devicesNum; i++)
        {
            CLRXDevice* device = program->context->devices[i];
            program->transDevicesMap->insert(std::make_pair(device->amdOclDevice, device));
        }
    }
    // add device_list into translate device map
    for (cl_uint i = 0; i < num_devices; i++)
        program->transDevicesMap->insert(std::make_pair(amdDevices[i], device_list[i]));
}

void clrxReleaseConcurrentBuild(CLRXProgram* program)
{
    program->concurrentBuilds--; // after this building
    if (program->concurrentBuilds == 0)
    {
        delete program->transDevicesMap;
        program->transDevicesMap = nullptr;
    }
}

void clrxReleaseOnlyCLRXProgram(CLRXProgram* program)
{
    if (program->refCount.fetch_sub(1) == 1)
    {
        // amdOclProgram has been already released, we release only our program
        clrxReleaseOnlyCLRXContext(program->context);
        if (program->amdOclAsmProgram!=nullptr)
            if (program->amdOclAsmProgram->dispatch->clReleaseProgram(
                        program->amdOclAsmProgram) != CL_SUCCESS)
                clrxAbort("Fatal error on clReleaseProgram(amdProg)");
        delete program;
    }
}

void clrxClearProgramAsmState(CLRXProgram* p)
{
    p->asmState.store(CLRXAsmState::NONE);
    if (p->amdOclAsmProgram != nullptr)
    {
        if (p->amdOclProgram->dispatch->clReleaseProgram(
            p->amdOclAsmProgram) != CL_SUCCESS)
            clrxAbort("Fatal error on clReleaseProgram(amdProg)");
    }
    p->amdOclAsmProgram = nullptr;
    p->asmProgEntries.reset();
    p->asmOptions.clear();
}

/* Bridge between Assembler and OpenCL wrapper */

// check whether if option to call CLRX assembler (-xasm)
bool detectCLRXCompilerCall(const char* compilerOptions)
{
    bool isAsm = false;
    const char* co = compilerOptions;
    while (*co!=0)
    {
        while (*co!=0 && (*co==' ' || *co=='\t')) co++;
        if (::strncmp(co, "-x", 2)==0)
        {
            co+=2;
            while (*co!=0 && (*co==' ' || *co=='\t')) co++;
            if (::strncmp(co, "asm", 3)==0 && (co[3]==0 || co[3]==' ' || co[3]=='\t'))
            {
                isAsm = true;
                co+=3;
                continue;
            }
            else
                isAsm = false;
        }
        while (*co!=0 && *co!=' ' && *co!='\t') co++;
    }
    return isAsm;
}

// if symbol name is correct
static bool verifySymbolName(const CString& symbolName)
{
    if (symbolName.empty())
        return false;
    auto c = symbolName.begin();
    if (isAlpha(*c) || *c=='.' || *c=='_' || *c=='$')
        while (isAlnum(*c) || *c=='.' || *c=='_' || *c=='$') c++;
    return *c==0;
}

static std::pair<CString, uint64_t> getDefSym(const CString& word)
{
    std::pair<CString, uint64_t> defSym = { "", 0 };
    size_t pos = word.find('=');
    if (pos != CString::npos)
    {
        const char* outEnd;
        defSym.second = cstrtovCStyle<uint64_t>(word.c_str()+pos+1, nullptr, outEnd);
        if (*outEnd!=0)
            throw Exception("Garbages at value");
        defSym.first = word.substr(0, pos);
    }
    else
        defSym.first = word;
    if (!verifySymbolName(defSym.first))
        throw Exception("Invalid symbol name");
    return defSym;
}

struct CLRX_INTERNAL CLProgBinEntry: public FastRefCountable
{
    Array<cxbyte> binary;
    CLProgBinEntry() { }
    CLProgBinEntry(const Array<cxbyte>& _binary) : binary(_binary) { }
    CLProgBinEntry(Array<cxbyte>&& _binary) noexcept
            : binary(std::move(_binary)) { }
};

// generate table with order of associated devices (from program)
static cl_int genDeviceOrder(cl_uint devicesNum, const cl_device_id* devices,
               cl_uint assocDevicesNum, const cl_device_id* assocDevices,
               cxuint* devAssocOrders)
{
    if (assocDevicesNum < 16)
    {
        // small amount of devices
        for (cxuint i = 0; i < devicesNum; i++)
        {
            auto it = std::find(assocDevices, assocDevices+assocDevicesNum, devices[i]);
            if (it != assocDevices+assocDevicesNum)
                devAssocOrders[i] = it-assocDevices;
            else // error
                return CL_INVALID_DEVICE;
        }
    }
    else
    {
        // more devices
        std::unique_ptr<std::pair<cl_device_id, cxuint>[]> sortedAssocDevs(
                new std::pair<cl_device_id, cxuint>[assocDevicesNum]);
        for (cxuint i = 0; i < assocDevicesNum; i++)
            sortedAssocDevs[i] = std::make_pair(assocDevices[i], i);
        mapSort(sortedAssocDevs.get(), sortedAssocDevs.get()+assocDevicesNum);
        for (cxuint i = 0; i < devicesNum; i++)
        {
            auto it = binaryMapFind(sortedAssocDevs.get(),
                            sortedAssocDevs.get()+assocDevicesNum, devices[i]);
            if (it != sortedAssocDevs.get()+assocDevicesNum)
                devAssocOrders[i] = it->second;
            else // error
                return CL_INVALID_DEVICE;
        }
    }
    return CL_SUCCESS;
}

static const char* stripCString(char* str)
{
    while (*str==' ') str++;
    char* last = str+::strlen(str);
    while (last!=str && (*last==0||*last==' '))
        last--;
    if (*last!=0) last[1] = 0;
    return str;
}

cl_int clrxCompilerCall(CLRXProgram* program, const char* compilerOptions,
            cl_uint devicesNum, CLRXDevice* const* devices)
try
{
    std::lock_guard<std::mutex> lock(program->asmMutex);
    if (devices==nullptr)
    {
        devicesNum = program->assocDevicesNum;
        devices = program->assocDevices.get();
    }
    /* get source code */
    size_t sourceCodeSize;
    std::unique_ptr<char[]> sourceCode;
    const cl_program amdp = program->amdOclProgram;
    {
        std::lock_guard<std::mutex> clock(program->mutex);
        program->asmState.store(CLRXAsmState::IN_PROGRESS);
        program->asmProgEntries.reset();
    }
    
    cl_int error = amdp->dispatch->clGetProgramInfo(amdp, CL_PROGRAM_SOURCE,
                    0, nullptr, &sourceCodeSize);
    if (error!=CL_SUCCESS)
        clrxAbort("Fatal error from clGetProgramInfo in clrxCompilerCall");
    if (sourceCodeSize==0)
    {
        program->asmState.store(CLRXAsmState::FAILED);
        return CL_INVALID_OPERATION;
    }
    
    sourceCode.reset(new char[sourceCodeSize]);
    error = amdp->dispatch->clGetProgramInfo(amdp, CL_PROGRAM_SOURCE, sourceCodeSize,
                             sourceCode.get(), nullptr);
    if (error!=CL_SUCCESS)
        clrxAbort("Fatal error from clGetProgramInfo in clrxCompilerCall");
    
    Flags asmFlags = ASM_WARNINGS;
    // parsing compile options
    const char* co = compilerOptions;
    std::vector<CString> includePaths;
    std::vector<std::pair<CString, uint64_t> > defSyms;
    bool nextIsIncludePath = false;
    bool nextIsDefSym = false;
    bool nextIsLang = false;
    bool useCL20Std = false;
    bool useLegacy = false;
    // drivers since 200406 version uses AmdCL2 binary format by default for >=GCN1.1
    bool useCL2StdForGCN11 = detectAmdDriverVersion() >= 200406;
    bool havePolicy = false;
    cxuint policyVersion = 0;
    
    try
    {
    while(*co!=0)
    {
        while (*co!=0 && (*co==' ' || *co=='\t')) co++;
        if (*co==0)
            break;
        const char* wordStart = co;
        while (*co!=0 && *co!=' ' && *co!='\t') co++;
        const CString word(wordStart, co);
        
        if (nextIsIncludePath) // otherwise
        {
            nextIsIncludePath = false;
            includePaths.push_back(word);
        }
        else if (nextIsDefSym) // otherwise
        {
            nextIsDefSym = false;
            defSyms.push_back(getDefSym(word));
        }
        else if (nextIsLang) // otherwise
            nextIsLang = false;
        else if (word[0] == '-')
        {
            // if option
            if (word == "-w")
                asmFlags &= ~ASM_WARNINGS;
            else if (word == "-noMacroCase")
                asmFlags |= ASM_MACRONOCASE;
            else if (word == "-legacy")
            {
                useCL2StdForGCN11 = false;
                useLegacy = true;
            }
            else if (word == "-forceAddSymbols")
                asmFlags |= ASM_FORCE_ADD_SYMBOLS;
            else if (word == "-buggyFPLit")
                asmFlags |= ASM_BUGGYFPLIT;
            else if (word == "-oldModParam")
                asmFlags |= ASM_OLDMODPARAM;
            else if (word == "-I" || word == "-includePath")
                nextIsIncludePath = true;
            else if (word.compare(0, 2, "-I")==0)
                includePaths.push_back(word.substr(2, word.size()-2));
            else if (word.compare(0, 13, "-includePath=")==0)
                includePaths.push_back(word.substr(13, word.size()-13));
            else if (word == "-D" || word == "-defsym")
                nextIsDefSym = true;
            else if (word.compare(0, 2, "-D")==0)
                defSyms.push_back(getDefSym(word.substr(2, word.size()-2)));
            else if (word.compare(0, 8, "-defsym=")==0)
                defSyms.push_back(getDefSym(word.substr(8, word.size()-8)));
            else if (word.compare(0, 8, "-cl-std=")==0)
            {
                const CString stdName = word.substr(8, word.size()-8);
                if (stdName=="CL2.0")
                    useCL20Std = true;
                else if (stdName!="CL1.1" && stdName!="CL1.1" && stdName!="CL1.2")
                {
                    program->asmState.store(CLRXAsmState::FAILED);
                    return CL_INVALID_BUILD_OPTIONS;
                }
            }
            else if (word == "-policy=")
            {
                const CString policyVersionStr = word.substr(8, word.size()-8);
                const char* str = policyVersionStr.c_str();
                const char* outStr;
                policyVersion = cstrtoui(str, nullptr, outStr);
                havePolicy = true;
            }
            else if (word == "-x" )
                nextIsLang = true;
            else if (word != "-xasm")
            {
                // if not language selection to asm
                program->asmState.store(CLRXAsmState::FAILED);
                return CL_INVALID_BUILD_OPTIONS;
            }
        }
        else
        {
            program->asmState.store(CLRXAsmState::FAILED);
            return CL_INVALID_BUILD_OPTIONS;
        }
    }
    if (nextIsDefSym || nextIsIncludePath || nextIsLang)
    {
        program->asmState.store(CLRXAsmState::FAILED);
        return CL_INVALID_BUILD_OPTIONS;
    }
    } // error
    catch(const Exception& ex)
    {
        program->asmState.store(CLRXAsmState::FAILED);
        return CL_INVALID_BUILD_OPTIONS;
    }
    
    /* compiling programs */
    struct OutDevEntry {
        cl_device_id first, second;
        CString devName;
    };
    Array<OutDevEntry> outDeviceIndexMap(devicesNum);
    
    for (cxuint i = 0; i < devicesNum; i++)
    {
        size_t devNameSize;
        std::unique_ptr<char[]> devName;
        error = amdp->dispatch->clGetDeviceInfo(devices[i]->amdOclDevice, CL_DEVICE_NAME,
                        0, nullptr, &devNameSize);
        if (error!=CL_SUCCESS)
            clrxAbort("Fatal error at clCompilerCall (clGetDeviceInfo)");
        devName.reset(new char[devNameSize]);
        error = amdp->dispatch->clGetDeviceInfo(devices[i]->amdOclDevice, CL_DEVICE_NAME,
                                  devNameSize, devName.get(), nullptr);
        if (error!=CL_SUCCESS)
            clrxAbort("Fatal error at clCompilerCall (clGetDeviceInfo)");
        const char* sdevName = stripCString(devName.get());
        outDeviceIndexMap[i] = { devices[i], devices[i]->amdOclDevice, sdevName };
    }
    /// sort devices by name and cl_device_id
    std::sort(outDeviceIndexMap.begin(), outDeviceIndexMap.end(),
            [](const OutDevEntry& a, const OutDevEntry& b)
            { 
                int ret = a.devName.compare(b.devName);
                if (ret<0) return true;
                if (ret>0) return false;
                return (a.first<b.first);
            });
    /// remove obsolete duplicates of devices
    devicesNum = std::unique(outDeviceIndexMap.begin(), outDeviceIndexMap.end(),
                [](const OutDevEntry& a, const OutDevEntry& b)
                { return a.first==b.first; }) - outDeviceIndexMap.begin();
    outDeviceIndexMap.resize(devicesNum); // resize to final size
    
    std::unique_ptr<ProgDeviceEntry[]> progDeviceEntries(new ProgDeviceEntry[devicesNum]);
    Array<RefPtr<CLProgBinEntry> > compiledProgBins(devicesNum);
    /// sorted devices
    std::unique_ptr<CLRXDevice*[]> sortedDevs(new CLRXDevice*[devicesNum]);
    for (cxuint i = 0; i < devicesNum; i++)
        sortedDevs[i] = (CLRXDevice*)(outDeviceIndexMap[i].first);
    // initialize build state to in_progress
    for (cxuint i = 0; i < devicesNum; i++)
        progDeviceEntries[i].status = CL_BUILD_IN_PROGRESS;
    
    bool asmFailure = false;
    bool asmNotAvailable = false;
    cxuint prevDeviceType = -1;
    for (cxuint i = 0; i < devicesNum; i++)
    {
        const auto& entry = outDeviceIndexMap[i];
        ProgDeviceEntry& progDevEntry = progDeviceEntries[i];
        cxuint devType = -1;
        try
        { devType = cxuint(getGPUDeviceTypeFromName(entry.devName.c_str())); }
        catch(const Exception& ex)
        {
            // if assembler not available for this device
            progDevEntry.status = CL_BUILD_ERROR;
            asmNotAvailable = true;
            prevDeviceType = devType;
            continue;
        }
        // make duplicate only if not first entry
        if (i!=0 && devType == prevDeviceType)
        {
            // copy from previous device (if this same device type)
            compiledProgBins[i] = compiledProgBins[i-1];
            progDevEntry = progDeviceEntries[i-1];
            continue; // skip if this same architecture
        }
        prevDeviceType = devType;
        // assemble it
        ArrayIStream astream(sourceCodeSize-1, sourceCode.get());
        std::string msgString;
        StringOStream msgStream(msgString);
        /// determine whether use useCL20StdByDev
        bool useCL20StdByDev = (useCL20Std || (useCL2StdForGCN11 &&
                getGPUArchitectureFromDeviceType(GPUDeviceType(devType))
                        >=GPUArchitecture::GCN1_1));
        Assembler assembler("", astream, asmFlags,
                    (useCL20StdByDev) ? BinaryFormat::AMDCL2 : BinaryFormat::AMD,
                    GPUDeviceType(devType), msgStream);
        
        // get address bit - for bitness
        cl_uint addressBits;
        error = amdp->dispatch->clGetDeviceInfo(entry.second,
                    CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint), &addressBits, nullptr);
        if (error != CL_SUCCESS)
            clrxAbort("Fatal error at clCompilerCall (clGetDeviceInfo)");
        assembler.set64Bit(addressBits==64);
        
        for (const CString& incPath: includePaths)
            assembler.addIncludeDir(incPath);
        for (const auto& defSym: defSyms)
            assembler.addInitialDefSym(defSym.first, defSym.second);
        if (havePolicy)
            assembler.setPolicyVersion(policyVersion);
        
        /// call main assembler routine
        bool good = false;
        try
        { good = assembler.assemble(); }
        catch(...)
        {
            // if failed
            progDevEntry.log = RefPtr<CLProgLogEntry>(
                            new CLProgLogEntry(std::move(msgString)));
            progDevEntry.status = CL_BUILD_ERROR;
            asmFailure = true;
            continue;
        }
        /// set up logs
        progDevEntry.log = RefPtr<CLProgLogEntry>(
                            new CLProgLogEntry(std::move(msgString)));
        if (good)
        {
            // try to write binary and keep it in compiled program binaries
            try
            {
                progDevEntry.status = CL_BUILD_SUCCESS;
                Array<cxbyte> output;
                assembler.writeBinary(output);
                compiledProgBins[i] = RefPtr<CLProgBinEntry>(
                            new CLProgBinEntry(std::move(output)));
            }
            catch(const Exception& ex)
            {
                // if exception during writing binary
                compiledProgBins[i].reset();
                msgString.append(ex.what());
                progDevEntry.log = RefPtr<CLProgLogEntry>(
                            new CLProgLogEntry(std::move(msgString)));
                progDevEntry.status = CL_BUILD_ERROR;
                asmFailure = true;
            }
        }
        else // error
        {
            progDevEntry.status = CL_BUILD_ERROR;
            asmFailure = true;
        }
    }
    /* set program binaries in order of original devices list */
    std::unique_ptr<size_t[]> programBinSizes(new size_t[devicesNum]);
    std::unique_ptr<cxbyte*[]> programBinaries(new cxbyte*[devicesNum]);
    
    cxuint compiledNum = 0;
    std::unique_ptr<cl_device_id[]> amdDevices(new cl_device_id[devicesNum]);
    for (cxuint i = 0; i < devicesNum; i++)
        if (compiledProgBins[i])
        {
            // set program binaries for creating and building assembler program
            amdDevices[compiledNum] = sortedDevs[i]->amdOclDevice;
            programBinSizes[compiledNum] = compiledProgBins[i]->binary.size();
            programBinaries[compiledNum++] = compiledProgBins[i]->binary.data();
        }
    std::unique_ptr<CLRXDevice*[]> failedDevices(new CLRXDevice*[devicesNum-compiledNum]);
    cxuint j = 0;
    for (cxuint i = 0; i < devicesNum; i++)
        if (!compiledProgBins[i])   /// creating failed devices table
            failedDevices[j++] = sortedDevs[i];
    
    cl_program newAmdAsmP = nullptr;
    cl_int errorLast = CL_SUCCESS;
    if (compiledNum != 0)
    {
        // if compilation is not zero
        // (use int32_t instead cl_int) - GCC 5 internal error!!!
        /// just create new amdAsmProgram
        newAmdAsmP = amdp->dispatch->clCreateProgramWithBinary(
                    program->context->amdOclContext, compiledNum, amdDevices.get(),
                    programBinSizes.get(), (const cxbyte**)programBinaries.get(),
                    nullptr, &error);
        if (newAmdAsmP==nullptr)
        {
            // return error
            program->asmState.store(CLRXAsmState::FAILED);
            return error;
        }
        /// and build (errorLast holds last error to be returned)
        errorLast = amdp->dispatch->clBuildProgram(newAmdAsmP, compiledNum,
              amdDevices.get(), (useCL20Std) ? "-cl-std=CL2.0" :
                      ((useLegacy) ? "-legacy" : ""), nullptr, nullptr);
    }
    
    if (errorLast == CL_SUCCESS)
    {
        // resolve errorLast if build program succeeded
        if (asmFailure)
            errorLast = CL_BUILD_PROGRAM_FAILURE;
        else if (asmNotAvailable)
            errorLast = CL_COMPILER_NOT_AVAILABLE;
    }
    
    std::lock_guard<std::mutex> clock(program->mutex);
    if (program->amdOclAsmProgram!=nullptr)
    {
        // release old asm program
        if (amdp->dispatch->clReleaseProgram(program->amdOclAsmProgram) != CL_SUCCESS)
            clrxAbort("Fatal error on clReleaseProgram(amdProg)");
        program->amdOclAsmProgram = nullptr;
    }
    
    program->amdOclAsmProgram = newAmdAsmP;
    if (compiledNum!=0) // new amdAsm is set if least one device will have built program
        clrxUpdateProgramAssocDevices(program); /// update associated devices
    else
    {
        // no new binaries no good (successful) associated devices
        program->assocDevicesNum = 0;
        program->assocDevices.reset();
    }
    if (compiledNum!=program->assocDevicesNum)
        clrxAbort("Fatal error: compiledNum!=program->assocDevicesNum");
    
    if (compiledNum!=devicesNum)
    {
        // and add extra devices (failed) to list
        std::unique_ptr<CLRXDevice*[]> newAssocDevices(new CLRXDevice*[devicesNum]);
        std::copy(program->assocDevices.get(), program->assocDevices.get()+compiledNum,
                  newAssocDevices.get());
        std::copy(failedDevices.get(), failedDevices.get()+devicesNum-compiledNum,
                newAssocDevices.get()+compiledNum);
        program->assocDevices = std::move(newAssocDevices);
        program->assocDevicesNum = devicesNum;
    }
    /// create order of devices in associated devices list (for indexing assoc devices)
    std::unique_ptr<cxuint[]> asmDevOrders(new cxuint[devicesNum]);
    if (genDeviceOrder(devicesNum, (const cl_device_id*)sortedDevs.get(),
            program->assocDevicesNum, (const cl_device_id*)program->assocDevices.get(),
            asmDevOrders.get()) != CL_SUCCESS)
        clrxAbort("Fatal error at genDeviceOrder at clrxCompilerCall");
    /// set up progDevice entry (contains log and build_status)
    program->asmProgEntries.reset(new ProgDeviceMapEntry[program->assocDevicesNum]);
    /* move logs and build statuses to CLRX program structure */
    for (cxuint i = 0; i < devicesNum; i++)
    {
        program->asmProgEntries[asmDevOrders[i]].first =
                        program->assocDevices[asmDevOrders[i]];
        ProgDeviceEntry& progDevEntry = program->asmProgEntries[asmDevOrders[i]].second;
        progDevEntry = std::move(progDeviceEntries[i]);
        if (progDevEntry.status == CL_BUILD_SUCCESS)
        {
            // get real device status from original implementation
            if (amdp->dispatch->clGetProgramBuildInfo(newAmdAsmP,
                    sortedDevs[i]->amdOclDevice, CL_PROGRAM_BUILD_STATUS,
                    sizeof(cl_build_status), &progDevEntry.status, nullptr) != CL_SUCCESS)
                clrxAbort("Fatal error at clGetProgramBuildInfo at clrxCompilerCall");
        }
    }
    mapSort(program->asmProgEntries.get(), program->asmProgEntries.get() +
                program->assocDevicesNum);
    
    program->asmOptions = compilerOptions;
    program->asmState.store((errorLast!=CL_SUCCESS) ?
                CLRXAsmState::FAILED : CLRXAsmState::SUCCESS);
    return errorLast;
}
catch(const std::bad_alloc& ex)
{
    program->asmState.store(CLRXAsmState::FAILED);
    return CL_OUT_OF_HOST_MEMORY;
}
catch(const std::exception& ex)
{
    clrxAbort("Fatal error at CLRX compiler call:", ex.what());
    return -1;
}
