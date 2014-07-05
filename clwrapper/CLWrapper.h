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

#ifndef __CLRX_CLWRAPPER_H__
#define __CLRX_CLWRAPPER_H__

// force C interpretation
extern "C"
{

#include "DispatchStruct.h"

typedef CL_API_ENTRY cl_int (CL_API_CALL *pfn_clIcdGetPlatformIDs)(
    cl_uint num_entries, 
    cl_platform_id *platforms, 
    cl_uint *num_platforms) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL clIcdGetPlatformIDsKHR(
    cl_uint num_entries, 
    cl_platform_id *platforms, 
    cl_uint *num_platforms) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL clSetCommandQueueProperty(
    cl_command_queue              command_queue,
    cl_command_queue_properties   properties, 
    cl_bool                       enable,
    cl_command_queue_properties * old_properties) CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED;

extern CL_API_ENTRY cl_int CL_API_CALL clEnqueueWaitSignalAMD(
                           cl_command_queue command_queue,
                           cl_mem mem_object,
                           cl_uint value,
                           cl_uint num_events,
                           const cl_event * event_wait_list,
                           cl_event * event) CL_EXT_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL clEnqueueWriteSignalAMD(
                        cl_command_queue command_queue,
                        cl_mem mem_object,
                        cl_uint value,
                        cl_ulong offset,
                        cl_uint num_events,
                        const cl_event * event_list,
                        cl_event * event) CL_EXT_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL clEnqueueMakeBuffersResidentAMD(
                         cl_command_queue command_queue,
                         cl_uint num_mem_objs,
                         cl_mem * mem_objects,
                         cl_bool blocking_make_resident,
                         cl_bus_address_amd * bus_addresses,
                         cl_uint num_events,
                         const cl_event * event_list,
                         cl_event * event) CL_EXT_SUFFIX__VERSION_1_2;

#include "InternalDecls.h"

}

#include <mutex>
#include <atomic>
#include <vector>
#include <memory>
#include <unordered_map>
#include <CLRX/Utilities.h>

struct CLRXVendorRecord
{
    char *suffix;
    CLRXpfn_clGetExtensionFunctionAddress clGetExtensionFunctionAddress;
    cl_platform_id platform;
    
    CLRXVendorRecord() { suffix = nullptr; }
    ~CLRXVendorRecord()
    { delete[] suffix; }
};

struct CLRXDevice;

struct CLRX_INTERNAL CLRXPlatform: _cl_platform_id
{
    cl_platform_id amdOclPlatform;
    std::once_flag onceFlag; // for synchronization
    const char* extensions;
    size_t extensionsSize;
    const char* version;
    size_t versionSize;
    
    cl_uint devicesNum;
    CLRXDevice* devices;
    cl_int deviceStatusInit;
    
    CLRXPlatform();
    ~CLRXPlatform();
};

struct CLRX_INTERNAL CLRXDevice: _cl_device_id
{
    std::atomic<size_t> refCount;
    cl_device_id amdOclDevice;
    CLRXPlatform* platform;
    cl_device_type type;
    CLRXDevice* parent;
    const char* extensions;
    size_t extensionsSize;
    
    CLRXDevice();
    ~CLRXDevice();
};

struct CLRX_INTERNAL CLRXContext: _cl_context
{
    std::atomic<size_t> refCount;
    cl_context amdOclContext;
    cl_uint devicesNum;
    CLRXDevice** devices;
    size_t propertiesNum;
    cl_context_properties* properties;
    
    CLRXContext();
    ~CLRXContext();
};

struct CLRX_INTERNAL CLRXCommandQueue: _cl_command_queue
{
    std::atomic<size_t> refCount;
    cl_command_queue amdOclCommandQueue;
    CLRXContext* context;
    CLRXDevice* device;
    
    CLRXCommandQueue();
    ~CLRXCommandQueue();
};

struct CLRX_INTERNAL CLRXMemObject: _cl_mem
{
    std::atomic<size_t> refCount;
    cl_mem amdOclMemObject;
    CLRXContext* context;
    CLRXMemObject* parent;
    
    CLRXMemObject();
    ~CLRXMemObject();
};

struct CLRX_INTERNAL CLRXMemDtorCallbackUserData
{
    CLRXMemObject* clrxMemObject;
    void (*realNotify)(cl_mem memobj, void * user_data);
    void* realUserData;
};

typedef std::unordered_map<std::string, std::vector<bool> > CLRXKernelArgFlagMap;

struct CLRX_INTERNAL CLRXProgram: _cl_program
{
    std::atomic<size_t> refCount;
    std::mutex mutex; // for thread-safe updating assoc devices
    cl_program amdOclProgram;
    CLRXContext* context;
    cl_uint assocDevicesNum;
    CLRXDevice** assocDevices;
    cl_uint origAssocDevicesNum;
    CLRXDevice** origAssocDevices;
    cl_ulong concurrentBuilds;
    bool kernelArgFlagsInitialized;
    bool kernelsAttached;
    CLRXKernelArgFlagMap kernelArgFlagsMap;
    
    CLRXProgram();
    ~CLRXProgram();
};

struct CLRX_INTERNAL CLRXBuildProgramUserData
{
    CLRXProgram* clrxProgram;
    void (*realNotify)(cl_program program, void * user_data);
    void* realUserData;
};

struct CLRX_INTERNAL CLRXLinkProgramUserData
{
    std::mutex mutex;
    bool clrxProgramFilled;
    CLRXContext* clrxContext;
    CLRXProgram* clrxProgram;
    void (*realNotify)(cl_program program, void * user_data);
    void* realUserData;
    bool toDeleteByCallback;
};

struct CLRX_INTERNAL CLRXKernel: _cl_kernel
{
    std::atomic<size_t> refCount;
    cl_kernel amdOclKernel;
    CLRXProgram* program;
    const std::vector<bool>& argTypes;
    
    CLRXKernel(const std::vector<bool>& argTypes);
    ~CLRXKernel();
};

struct CLRX_INTERNAL CLRXEvent: _cl_event
{
    std::atomic<size_t> refCount;
    cl_event amdOclEvent;
    CLRXContext* context;
    CLRXCommandQueue* commandQueue;
    
     CLRXEvent();
     ~CLRXEvent();
};

struct CLRX_INTERNAL CLRXEventCallbackUserData
{
    CLRXEvent* clrxEvent;
    void (*realNotify)(cl_event event, cl_int exec_status, void * user_data);
    void* realUserData;
};

struct CLRX_INTERNAL CLRXSampler: _cl_sampler
{
    std::atomic<size_t> refCount;
    cl_sampler amdOclSampler;
    CLRXContext* context;
    
    CLRXSampler();
    ~CLRXSampler();
};

/* internals */

struct CLRXExtensionEntry
{
    const char* funcname;
    void* address;
};

CLRX_INTERNAL extern std::once_flag clrxOnceFlag;
CLRX_INTERNAL extern bool useCLRXWrapper;
CLRX_INTERNAL extern std::shared_ptr<CLRX::DynLibrary> amdOclLibrary;
CLRX_INTERNAL extern cl_uint amdOclNumPlatforms;
CLRX_INTERNAL extern CLRXVendorRecord* amdOclVendors;
CLRX_INTERNAL extern CLRXpfn_clGetPlatformIDs amdOclGetPlatformIDs;
CLRX_INTERNAL extern CLRXpfn_clUnloadCompiler amdOclUnloadCompiler;
CLRX_INTERNAL extern cl_int clrxWrapperInitStatus;

CLRX_INTERNAL extern CLRXPlatform* clrxPlatforms;

CLRX_INTERNAL extern clEnqueueWaitSignalAMD_fn amdOclEnqueueWaitSignalAMD;
CLRX_INTERNAL extern clEnqueueWriteSignalAMD_fn amdOclEnqueueWriteSignalAMD;
CLRX_INTERNAL extern clEnqueueMakeBuffersResidentAMD_fn amdOclEnqueueMakeBuffersResidentAMD;
CLRX_INTERNAL extern CLRXpfn_clGetExtensionFunctionAddress
        amdOclGetExtensionFunctionAddress;

CLRX_INTERNAL extern const CLRXIcdDispatch clrxDispatchRecord;
CLRX_INTERNAL extern const CLRXExtensionEntry clrxExtensionsTable[7];

/* internal routines */

CLRX_INTERNAL void clrxWrapperInitialize();
CLRX_INTERNAL void clrxPlatformInitializeDevices(CLRXPlatform* platform);
CLRX_INTERNAL void translateAMDDevicesIntoCLRXDevices(cl_uint allDevicesNum,
       CLRXDevice** allDevices, cl_uint amdDevicesNum, cl_device_id* amdDevices);
CLRX_INTERNAL cl_int clrxSetContextDevices(CLRXContext* c, const CLRXPlatform* platform);
CLRX_INTERNAL cl_int clrxSetContextDevices(CLRXContext* c, cl_uint inDevicesNum,
            const cl_device_id* inDevices);
CLRX_INTERNAL cl_int clrxSetProgramOrigDevices(CLRXProgram* p);
CLRX_INTERNAL cl_int clrxUpdateProgramAssocDevices(CLRXProgram* p);
CLRX_INTERNAL void clrxBuildProgramNotifyWrapper(cl_program program, void * user_data);
CLRX_INTERNAL void clrxLinkProgramNotifyWrapper(cl_program program, void * user_data);
CLRX_INTERNAL CLRXProgram* clrxCreateCLRXProgram(CLRXContext* c, cl_program amdProgram,
          cl_int* errcode_ret);
CLRX_INTERNAL cl_int clrxApplyCLRXEvent(const CLRXCommandQueue* q, cl_event* event,
             cl_event amdEvent, cl_int status);
CLRX_INTERNAL cl_int clrxCreateOutDevices(CLRXDevice* d, cl_uint devicesNum,
       cl_device_id* out_devices, cl_int (*AMDReleaseDevice)(cl_device_id),
       const char* fatalErrorMessage);
CLRX_INTERNAL void clrxEventCallbackWrapper(cl_event event, cl_int exec_status,
        void * user_data);
CLRX_INTERNAL void clrxMemDtorCallbackWrapper(cl_mem memobj, void * user_data);

CLRX_INTERNAL cl_int clrxInitKernelArgFlagsMap(CLRXProgram* program);


static inline void clrxRetainOnlyCLRXDevice(CLRXDevice* device)
{
    if (device->parent != nullptr)
        device->refCount.fetch_add(1);
}

static inline void clrxRetainOnlyCLRXDeviceNTimes(CLRXDevice* device, cl_uint ntimes)
{
    if (device->parent != nullptr)
        device->refCount.fetch_add(ntimes);
}

static inline void clrxReleaseOnlyCLRXDevice(CLRXDevice* device)
{
    if (device->parent != nullptr)
        if (device->refCount.fetch_sub(1) == 1)
        {   // amdOclDevice has been already released, we release only our device
            clrxReleaseOnlyCLRXDevice(device->parent);
            delete device;
        }
}

static inline void clrxRetainOnlyCLRXContext(CLRXContext* context)
{
    context->refCount.fetch_add(1);
}

static inline void clrxReleaseOnlyCLRXContext(CLRXContext* context)
{
    if (context->refCount.fetch_sub(1) == 1)
    {   // amdOclContext has been already released, we release only our context
        for (cl_uint i = 0; i < context->devicesNum; i++)
            clrxReleaseOnlyCLRXDevice(context->devices[i]);
        delete context;
    }
}

static inline void clrxRetainOnlyCLRXMemObject(CLRXMemObject* memObject)
{
    memObject->refCount.fetch_add(1);
}

static inline void clrxReleaseOnlyCLRXMemObject(CLRXMemObject* memObject)
{
    if (memObject->refCount.fetch_sub(1) == 1)
    {   // amdOclContext has been already released, we release only our context
        if (memObject->parent != nullptr)
            clrxReleaseOnlyCLRXMemObject(memObject->parent);
        delete memObject;
    }
}

static inline void clrxRetainOnlyCLRXProgram(CLRXProgram* program)
{
    program->refCount.fetch_add(1);
}

static inline void clrxRetainOnlyCLRXProgramNTimes(CLRXProgram* program, cl_uint ntimes)
{
    program->refCount.fetch_add(ntimes);
}

static inline void clrxReleaseOnlyCLRXProgram(CLRXProgram* program)
{
    if (program->refCount.fetch_sub(1) == 1)
    {   // amdOclProgram has been already released, we release only our program
        clrxReleaseOnlyCLRXContext(program->context);
        delete program;
    }
}

/* internal macros */
#define CLRX_INITIALIZE \
    { \
        try \
        { std::call_once(clrxOnceFlag, clrxWrapperInitialize); } \
        catch(const std::exception& ex) \
        { \
            std::cerr << "Fatal error at wrapper initialization: " << \
                    ex.what() << std::endl; \
            abort(); \
        } \
        catch(...) \
        { \
            std::cerr << \
                "Fatal and unknown error at wrapper initialization: " << std::endl; \
            abort(); \
        } \
        if (clrxWrapperInitStatus != CL_SUCCESS) \
            return clrxWrapperInitStatus; \
    }

#define CLRX_INITIALIZE_OBJ \
    { \
        try \
        { std::call_once(clrxOnceFlag, clrxWrapperInitialize); } \
        catch(const std::exception& ex) \
        { \
            std::cerr << "Fatal error at wrapper initialization: " << \
                    ex.what() << std::endl; \
            abort(); \
        } \
        catch(...) \
        { \
            std::cerr << \
                "Fatal and unknown error at wrapper initialization: " << std::endl; \
            abort(); \
        } \
        if (clrxWrapperInitStatus != CL_SUCCESS) \
        { \
            if (errcode_ret != nullptr) \
                *errcode_ret = clrxWrapperInitStatus; \
            return nullptr; \
        } \
    }

#define CLRX_INITIALIZE_VOIDPTR \
    { \
        try \
        { std::call_once(clrxOnceFlag, clrxWrapperInitialize); } \
        catch(const std::exception& ex) \
        { \
            std::cerr << "Fatal error at wrapper initialization: " << \
                    ex.what() << std::endl; \
            abort(); \
        } \
        catch(...) \
        { \
            std::cerr << \
                "Fatal and unknown error at wrapper initialization: " << std::endl; \
            abort(); \
        } \
        if (clrxWrapperInitStatus != CL_SUCCESS) \
            return nullptr; \
    }

#define CREATE_CLRXCONTEXT_OBJECT(CLRXTYPE, AMDOBJECTMEMBER, \
        AMDOBJECT, CLRELEASECALL, FATALERROR) \
    CLRXTYPE* outObject = nullptr; \
    try \
    { outObject = new CLRXTYPE; } \
    catch(const std::bad_alloc& ex) \
    { \
        if (c->amdOclContext->dispatch->CLRELEASECALL(AMDOBJECT) != CL_SUCCESS) \
        { \
            std::cerr << FATALERROR << std::endl; \
            abort(); \
        } \
        if (errcode_ret != nullptr) \
            *errcode_ret = CL_OUT_OF_HOST_MEMORY; \
        return nullptr; \
    } \
    \
    outObject->dispatch = const_cast<CLRXIcdDispatch*>(&clrxDispatchRecord); \
    outObject->AMDOBJECTMEMBER = AMDOBJECT; \
    outObject->context = c; \
    \
    clrxRetainOnlyCLRXContext(c);

#define CLRX_CALL_QUEUE_COMMAND \
    cl_event amdEvent = nullptr; \
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr; \
    if (event_wait_list != nullptr) \
    { \
        if (num_events_in_wait_list <= 100) \
        { \
            cl_event amdWaitList[100]; \
            for (cl_uint i = 0; i < num_events_in_wait_list; i++) \
            { \
                if (event_wait_list[i] == nullptr) \
                    return CL_INVALID_EVENT_WAIT_LIST; \
                amdWaitList[i] = \
                    static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent; \
            } \
            \
            status = CLRX_CLCOMMAND_PREFIX CLRX_ORIG_CLCOMMAND, \
                num_events_in_wait_list, amdWaitList, amdEventPtr); \
        } \
        else \
            try \
            { \
                std::vector<cl_event> amdWaitList(num_events_in_wait_list); \
                for (cl_uint i = 0; i < num_events_in_wait_list; i++) \
                { \
                    if (event_wait_list[i] == nullptr) \
                        return CL_INVALID_EVENT_WAIT_LIST; \
                    amdWaitList[i] = \
                        static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent; \
                } \
                \
                status = CLRX_CLCOMMAND_PREFIX CLRX_ORIG_CLCOMMAND, \
                    num_events_in_wait_list, amdWaitList.data(), amdEventPtr); \
            } \
            catch (const std::bad_alloc& ex) \
            { return CL_OUT_OF_HOST_MEMORY; } \
    } \
    else \
        status = CLRX_CLCOMMAND_PREFIX CLRX_ORIG_CLCOMMAND, \
            0, nullptr, amdEventPtr);
    
#endif
