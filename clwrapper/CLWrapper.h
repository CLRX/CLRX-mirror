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

}

#include <mutex>
#include <atomic>
#include <vector>
#include <memory>
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
    cl_platform_id platform;
    cl_device_type type;
    cl_device_id parent;
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
    cl_device_id* devices;
    size_t propertiesNum;
    cl_context_properties* properties;
    
    CLRXContext();
    ~CLRXContext();
};

struct CLRX_INTERNAL CLRXCommandQueue: _cl_command_queue
{
    std::atomic<size_t> refCount;
    cl_command_queue amdOclCommandQueue;
    cl_context context;
    cl_device_id device;
    
    CLRXCommandQueue();
    ~CLRXCommandQueue();
};

struct CLRXMemDtorCallbackUserData
{
    cl_mem clrxMemObject;
    void (*realNotify)(cl_mem memobj, void * user_data);
    void* realUserData;
};

struct CLRX_INTERNAL CLRXMemObject: _cl_mem
{
    std::atomic<size_t> refCount;
    cl_mem amdOclMemObject;
    cl_context context;
    
    CLRXMemObject();
    ~CLRXMemObject();
};

struct CLRXBuildProgramUserData
{
    cl_program clrxProgram;
    void (*realNotify)(cl_program program, void * user_data);
    void* realUserData;
};

struct CLRXLinkProgramUserData
{
    std::mutex mutex;
    bool clrxProgramFilled;
    cl_context clrxContext;
    cl_program clrxProgram;
    void (*realNotify)(cl_program program, void * user_data);
    void* realUserData;
    bool toDeleteByCallback;
};

struct CLRX_INTERNAL CLRXProgram: _cl_program
{
    std::atomic<size_t> refCount;
    std::mutex mutex; // for thread-safe updating assoc devices
    cl_program amdOclProgram;
    cl_context context;
    cl_uint assocDevicesNum;
    cl_device_id* assocDevices;
    cl_uint origAssocDevicesNum;
    cl_device_id* origAssocDevices;
    cl_ulong concurrentBuilds;
    
    CLRXProgram();
    ~CLRXProgram();
};

struct CLRX_INTERNAL CLRXKernel: _cl_kernel
{
    std::atomic<size_t> refCount;
    cl_kernel amdOclKernel;
    cl_program program;
    std::vector<bool> argTypes;
    
    CLRXKernel();
    ~CLRXKernel();
};

struct CLRXEventCallbackUserData
{
    cl_event clrxEvent;
    void (*realNotify)(cl_event event, cl_int exec_status, void * user_data);
    void* realUserData;
};

struct CLRX_INTERNAL CLRXEvent: _cl_event
{
    std::atomic<size_t> refCount;
    cl_event amdOclEvent;
    cl_context context;
    cl_command_queue commandQueue;
    
     CLRXEvent();
     ~CLRXEvent();
};

struct CLRX_INTERNAL CLRXSampler: _cl_sampler
{
    std::atomic<size_t> refCount;
    cl_sampler amdOclSampler;
    cl_context context;
    
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

CLRX_INTERNAL extern const CLRXIcdDispatch clrxDispatchRecord;
CLRX_INTERNAL extern const CLRXExtensionEntry clrxExtensionsTable[7];

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
CLRX_INTERNAL cl_int clrxCreateOutDevices(const CLRXDevice* d, cl_uint devicesNum,
       cl_device_id* out_devices, cl_int (*AMDReleaseDevice)(cl_device_id),
       const char* fatalErrorMessage);
CLRX_INTERNAL void clrxEventCallbackWrapper(cl_event event, cl_int exec_status,
        void * user_data);
CLRX_INTERNAL void clrxMemDtorCallbackWrapper(cl_mem memobj, void * user_data);

/* internal routines */

static inline void clrxRetainOnlyCLRXDevice(cl_device_id device)
{
    CLRXDevice* d = static_cast<CLRXDevice*>(device);
    if (d->parent != nullptr)
        d->refCount.fetch_add(1);
}

static inline void clrxReleaseOnlyCLRXDevice(cl_device_id device)
{
    CLRXDevice* d = static_cast<CLRXDevice*>(device);
    if (d->parent != nullptr)
        if (d->refCount.fetch_sub(1) == 1)
        {   // amdOclDevice has been already released, we release only our device
            delete d;
        }
}

static inline void clrxRetainOnlyCLRXContext(cl_context context)
{
    CLRXContext* c = static_cast<CLRXContext*>(context);
    c->refCount.fetch_add(1);
}

static inline void clrxReleaseOnlyCLRXContext(cl_context context)
{
    CLRXContext* c = static_cast<CLRXContext*>(context);
    if (c->refCount.fetch_sub(1) == 1)
    {   // amdOclContext has been already released, we release only our context
        for (cl_uint i = 0; i < c->devicesNum; i++)
            clrxReleaseOnlyCLRXDevice(c->devices[i]);
        delete c;
    }
}

static inline void clrxRetainOnlyCLRXProgram(cl_program program)
{
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    p->refCount.fetch_add(1);
}

static inline void clrxRetainOnlyCLRXProgramNTimes(cl_program program, cl_uint ntimes)
{
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    p->refCount.fetch_add(ntimes);
}

static inline void clrxReleaseOnlyCLRXProgram(cl_program program)
{
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    if (p->refCount.fetch_sub(1) == 1)
    {   // amdOclProgram has been already released, we release only our program
        clrxReleaseOnlyCLRXContext(p->context);
        delete p;
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
            std::cerr << \
                FATALERROR << std::endl; \
            abort(); \
        } \
        if (errcode_ret != nullptr) \
            *errcode_ret = CL_OUT_OF_HOST_MEMORY; \
        return nullptr; \
    } \
    \
    outObject->dispatch = const_cast<CLRXIcdDispatch*>(&clrxDispatchRecord); \
    outObject->AMDOBJECTMEMBER = AMDOBJECT; \
    outObject->context = context; \
    \
    clrxRetainOnlyCLRXContext(context);

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
