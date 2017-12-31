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

#ifndef __CLRX_CLWRAPPER_H__
#define __CLRX_CLWRAPPER_H__

#include <CLRX/Config.h>

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

#ifdef CL_VERSION_1_2

#ifndef CL_MEM_BUS_ADDRESSABLE_AMD
    /********************************
    * cl_amd_bus_addressable_memory *
    ********************************/

    /* cl_mem flag - bitfield */
    #define CL_MEM_BUS_ADDRESSABLE_AMD               (1<<30)
    #define CL_MEM_EXTERNAL_PHYSICAL_AMD             (1<<31)

    #define CL_COMMAND_WAIT_SIGNAL_AMD                0x4080
    #define CL_COMMAND_WRITE_SIGNAL_AMD               0x4081
    #define CL_COMMAND_MAKE_BUFFERS_RESIDENT_AMD      0x4082

    typedef struct
    {
        cl_ulong surface_bus_address;
        cl_ulong marker_bus_address;
    } cl_bus_address_amd;
    
    typedef CL_API_ENTRY cl_int
    (CL_API_CALL * clEnqueueWaitSignalAMD_fn)( cl_command_queue /*command_queue*/,
                                   cl_mem /*mem_object*/,
                                   cl_uint /*value*/,
                                   cl_uint /*num_events*/,
                                   const cl_event * /*event_wait_list*/,
                                   cl_event * /*event*/) CL_EXT_SUFFIX__VERSION_1_2;

    typedef CL_API_ENTRY cl_int
    (CL_API_CALL * clEnqueueWriteSignalAMD_fn)( cl_command_queue /*command_queue*/,
                                    cl_mem /*mem_object*/,
                                    cl_uint /*value*/,
                                    cl_ulong /*offset*/,
                                    cl_uint /*num_events*/,
                                    const cl_event * /*event_list*/,
                                    cl_event * /*event*/) CL_EXT_SUFFIX__VERSION_1_2;

    typedef CL_API_ENTRY cl_int
    (CL_API_CALL * clEnqueueMakeBuffersResidentAMD_fn)( cl_command_queue /*command_queue*/,
                                 cl_uint /*num_mem_objs*/,
                                 cl_mem * /*mem_objects*/,
                                 cl_bool /*blocking_make_resident*/,
                                 cl_bus_address_amd * /*bus_addresses*/,
                                 cl_uint /*num_events*/,
                                 const cl_event * /*event_list*/,
                                 cl_event * /*event*/) CL_EXT_SUFFIX__VERSION_1_2;

#endif

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
#endif

#include "InternalDecls.h"


#ifndef CL_CONTEXT_OFFLINE_DEVICES_AMD
#define CL_CONTEXT_OFFLINE_DEVICES_AMD              0x403F
#endif
}

#include <cstdint>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <CLRX/utils/Utilities.h>
#include <CLRX/utils/Containers.h>

struct CLRXExtensionEntry
{
    const char* funcname;
    void* address;
};

CLRX_INTERNAL extern CLRXExtensionEntry clrxExtensionsTable[18];

struct CLRXPlatform;

struct CLRX_INTERNAL CLRXDevice: _cl_device_id, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    cl_device_id amdOclDevice;
    CLRXPlatform* platform;
    CLRXDevice* parent;
    std::unique_ptr<char[]> extensions;
    size_t extensionsSize;
    std::unique_ptr<char[]> version;
    size_t versionSize;
    
    CLRXDevice() : refCount(1)
    {
        platform = nullptr;
        extensionsSize = 0;
        versionSize = 0;
        parent = nullptr;
    }
};

static inline uint32_t getOpenCLVersionNum(uint16_t major, uint16_t minor)
{ return (cxuint(major)<<16) | minor; }

struct CLRX_INTERNAL CLRXPlatform: _cl_platform_id, CLRX::NonCopyableAndNonMovable
{
    cl_platform_id amdOclPlatform;
    CLRX::OnceFlag onceFlag; // for synchronization
    std::unique_ptr<char[]> extensions;
    size_t extensionsSize;
    std::unique_ptr<char[]> version;
    size_t versionSize;
    
    cl_uint devicesNum;
    CLRXDevice* devicesArray;
    std::unique_ptr<CLRXDevice*[]> devicePtrs;
    std::unique_ptr<CLRXExtensionEntry[]> extEntries;
    uint32_t openCLVersionNum; /* major - upper half, minor - lower half */
    cl_int deviceInitStatus;
    
    CLRXPlatform()
    {
        extensionsSize = 0;
        versionSize = 0;
        devicesNum = 0;
        devicesArray = nullptr;
        deviceInitStatus = CL_SUCCESS;
        openCLVersionNum = 0;
    }
    
    ~CLRXPlatform()
    { 
        delete[] devicesArray;
        delete dispatch; // allocated in CLRXPlatform
    }
};

struct CLRX_INTERNAL CLRXContext: _cl_context, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    cl_context amdOclContext;
    cl_uint devicesNum;
    std::unique_ptr<CLRXDevice*[]> devices;
    CLRX::Array<cl_context_properties> properties;
    uint32_t openCLVersionNum;
    
    CLRXContext() : refCount(1)
    {
        devicesNum = 0;
        openCLVersionNum = 0;
    }
};

struct CLRX_INTERNAL CLRXCommandQueue: _cl_command_queue, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    cl_command_queue amdOclCommandQueue;
    CLRXContext* context;
    CLRXDevice* device;

    CLRXCommandQueue() : refCount(1)
    {
        context = nullptr;
        device = nullptr;
    }
};

struct CLRX_INTERNAL CLRXMemObject: _cl_mem, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    cl_mem amdOclMemObject;
    CLRXContext* context;
    CLRXMemObject* parent;
    CLRXMemObject* buffer; // for buffer

    CLRXMemObject() : refCount(1)
    {
        context = nullptr;
        parent = nullptr;
        buffer = nullptr; // for image
    }
};

struct CLRX_INTERNAL CLRXMemDtorCallbackUserData
{
    CLRXMemObject* clrxMemObject;
    void (CL_CALLBACK *realNotify)(cl_mem memobj, void * user_data);
    void* realUserData;
};

typedef std::map<cl_device_id, cl_device_id> CLRXProgramDevicesMap;

typedef CLRX::Array<std::pair<CLRX::CString, std::vector<bool> > > CLRXKernelArgFlagMap;

struct CLRX_INTERNAL CLProgLogEntry: public CLRX::FastRefCountable
{
    std::string log;
    CLProgLogEntry() { }
    CLProgLogEntry(const std::string& _log) : log(_log) { }
    CLProgLogEntry(std::string&& _log) noexcept
            : log(std::move(_log)) { }
};

struct CLRX_INTERNAL ProgDeviceEntry
{
    CLRX::RefPtr<CLProgLogEntry> log;
    cl_build_status status;
    ProgDeviceEntry() : status(CL_BUILD_NONE) { }
};

// state assembler program
enum class CLRXAsmState
{
    NONE = 0,   // no assembler program
    IN_PROGRESS,    // build in progress
    FAILED,     // failed
    SUCCESS     // built successfully
};

typedef std::pair<cl_device_id, ProgDeviceEntry> ProgDeviceMapEntry;

struct CLRX_INTERNAL CLRXProgram: _cl_program, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    std::mutex mutex; // for thread-safe updating assoc devices
    cl_program amdOclProgram;
    CLRXContext* context;
    cl_uint assocDevicesNum;
    std::unique_ptr<CLRXDevice*[]> assocDevices;
    cl_ulong concurrentBuilds;
    CLRXProgramDevicesMap* transDevicesMap;
    size_t kernelsAttached;
    bool kernelArgFlagsInitialized;
    CLRXKernelArgFlagMap kernelArgFlagsMap;
    std::mutex asmMutex;
    cl_program amdOclAsmProgram;
    std::unique_ptr<ProgDeviceMapEntry[]> asmProgEntries;
    std::string asmOptions;
    std::atomic<CLRXAsmState> asmState;
    
    CLRXProgram() : refCount(1)
    {
        kernelArgFlagsInitialized = false;
        kernelsAttached = 0;
        context = nullptr;
        assocDevicesNum = 0;
        assocDevices = nullptr;
        concurrentBuilds = 0;
        transDevicesMap = nullptr;
        amdOclAsmProgram = nullptr;
        asmState.store(CLRXAsmState::NONE);
    }
};

struct CLRX_INTERNAL CLRXBuildProgramUserData
{
    CLRXProgram* clrxProgram;
    bool callDone;
    bool inClFunction;
    void (CL_CALLBACK *realNotify)(cl_program program, void * user_data);
    void* realUserData;
};

struct CLRX_INTERNAL CLRXLinkProgramUserData
{
    std::mutex mutex;
    bool clrxProgramFilled;
    CLRXContext* clrxContext;
    CLRXProgram* clrxProgram;
    CLRXProgramDevicesMap* transDevicesMap;
    void (CL_CALLBACK *realNotify)(cl_program program, void * user_data);
    void* realUserData;
};

struct CLRX_INTERNAL CLRXKernel: _cl_kernel, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    cl_kernel amdOclKernel;
    CLRXProgram* program;
    const std::vector<bool>& argTypes;
    bool fromAsm;
    
    CLRXKernel(const std::vector<bool>& _argTypes) : refCount(1),
            argTypes(_argTypes)
    { 
        program = nullptr;
        fromAsm = false;
    }
};

struct CLRX_INTERNAL CLRXEvent: _cl_event, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    cl_event amdOclEvent;
    CLRXContext* context;
    CLRXCommandQueue* commandQueue;

    CLRXEvent() : refCount(1)
    {
        context = nullptr;
        commandQueue = nullptr;
    }
};

struct CLRX_INTERNAL CLRXEventCallbackUserData
{
    CLRXEvent* clrxEvent;
    void (CL_CALLBACK *realNotify)(cl_event event, cl_int exec_status, void * user_data);
    void* realUserData;
};

struct CLRX_INTERNAL CLRXSampler: _cl_sampler, CLRX::NonCopyableAndNonMovable
{
    std::atomic<size_t> refCount;
    cl_sampler amdOclSampler;
    CLRXContext* context;
    
    CLRXSampler() : refCount(1)
    { context = nullptr; }
};

#ifdef CL_VERSION_2_0
struct CLRX_INTERNAL CLRXSVMFreeCallbackUserData
{
    CLRXCommandQueue* clrxCommandQueue;
    void (CL_CALLBACK *realNotify)(cl_command_queue queue, cl_uint num_svm_pointers,
             void** svm_pointers, void* user_data);
    void* realUserData;
};
#endif

/* internals */
CLRX_INTERNAL extern CLRX::OnceFlag clrxOnceFlag;
CLRX_INTERNAL extern bool useCLRXWrapper;
CLRX_INTERNAL extern cl_uint amdOclNumPlatforms;
CLRX_INTERNAL extern CLRXpfn_clGetPlatformIDs amdOclGetPlatformIDs;
CLRX_INTERNAL extern CLRXpfn_clUnloadCompiler amdOclUnloadCompiler;
CLRX_INTERNAL extern cl_int clrxWrapperInitStatus;

CLRX_INTERNAL extern CLRXPlatform* clrxPlatforms;

#ifdef CL_VERSION_1_2
CLRX_INTERNAL extern clEnqueueWaitSignalAMD_fn amdOclEnqueueWaitSignalAMD;
CLRX_INTERNAL extern clEnqueueWriteSignalAMD_fn amdOclEnqueueWriteSignalAMD;
CLRX_INTERNAL extern clEnqueueMakeBuffersResidentAMD_fn amdOclEnqueueMakeBuffersResidentAMD;
#endif
CLRX_INTERNAL extern CLRXpfn_clGetExtensionFunctionAddress
        amdOclGetExtensionFunctionAddress;

CLRX_INTERNAL extern const CLRXIcdDispatch clrxDispatchRecord;

/* internal routines */

CLRX_INTERNAL void clrxWrapperInitialize();
CLRX_INTERNAL void clrxPlatformInitializeDevices(CLRXPlatform* platform);
CLRX_INTERNAL void translateAMDDevicesIntoCLRXDevices(cl_uint allDevicesNum,
       const CLRXDevice** allDevices, cl_uint amdDevicesNum, cl_device_id* amdDevices);
CLRX_INTERNAL cl_int clrxSetContextDevices(CLRXContext* c, const CLRXPlatform* platform);
CLRX_INTERNAL cl_int clrxSetContextDevices(CLRXContext* c, cl_uint inDevicesNum,
            const cl_device_id* inDevices);
CLRX_INTERNAL cl_int clrxUpdateProgramAssocDevices(CLRXProgram* p);
CLRX_INTERNAL void CL_CALLBACK clrxBuildProgramNotifyWrapper(cl_program program,
             void * user_data);
CLRX_INTERNAL void CL_CALLBACK clrxLinkProgramNotifyWrapper(cl_program program,
             void * user_data);
CLRX_INTERNAL CLRXProgram* clrxCreateCLRXProgram(CLRXContext* c, cl_program amdProgram,
          cl_int* errcode_ret);
CLRX_INTERNAL cl_int clrxApplyCLRXEvent(CLRXCommandQueue* q, cl_event* event,
             cl_event amdEvent, cl_int status);
CLRX_INTERNAL cl_int clrxCreateOutDevices(CLRXDevice* d, cl_uint devicesNum,
       cl_device_id* out_devices, cl_int (CL_API_CALL *AMDReleaseDevice)(cl_device_id),
       const char* fatalErrorMessage);
CLRX_INTERNAL void CL_CALLBACK clrxEventCallbackWrapper(cl_event event, cl_int exec_status,
        void * user_data);
CLRX_INTERNAL void CL_CALLBACK clrxMemDtorCallbackWrapper(cl_mem memobj, void * user_data);
CLRX_INTERNAL void CL_CALLBACK clrxSVMFreeCallbackWrapper(cl_command_queue queue,
      cl_uint num_svm_pointers, void** svm_pointers, void* user_data);

CLRX_INTERNAL cl_int clrxInitKernelArgFlagsMap(CLRXProgram* program);

CLRX_INTERNAL void clrxInitProgramTransDevicesMap(CLRXProgram* program,
              cl_uint devices_num, const cl_device_id* device_list,
              const std::vector<cl_device_id>& amdDevices);
CLRX_INTERNAL void clrxReleaseConcurrentBuild(CLRXProgram* program);

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

CLRX_INTERNAL extern void clrxReleaseOnlyCLRXDevice(CLRXDevice* device);

static inline void clrxRetainOnlyCLRXContext(CLRXContext* context)
{
    context->refCount.fetch_add(1);
}

CLRX_INTERNAL extern void clrxReleaseOnlyCLRXContext(CLRXContext* context);

static inline void clrxRetainOnlyCLRXCommandQueue(CLRXCommandQueue* cmdQueue)
{
    cmdQueue->refCount.fetch_add(1);
}

static inline void clrxReleaseOnlyCLRXCommandQueue(CLRXCommandQueue* cmdQueue)
{
    if (cmdQueue->refCount.fetch_sub(1) == 1)
    {
        clrxReleaseOnlyCLRXDevice(cmdQueue->device);
        clrxReleaseOnlyCLRXContext(cmdQueue->context);
        delete cmdQueue;
    }
}

static inline void clrxRetainOnlyCLRXMemObject(CLRXMemObject* memObject)
{
    memObject->refCount.fetch_add(1);
}

CLRX_INTERNAL extern void clrxReleaseOnlyCLRXMemObject(CLRXMemObject* memObject);

static inline void clrxRetainOnlyCLRXProgram(CLRXProgram* program)
{
    program->refCount.fetch_add(1);
}

static inline void clrxRetainOnlyCLRXProgramNTimes(CLRXProgram* program, cl_uint ntimes)
{
    program->refCount.fetch_add(ntimes);
}

CLRX_INTERNAL void clrxReleaseOnlyCLRXProgram(CLRXProgram* program);

CLRX_INTERNAL void clrxClearProgramAsmState(CLRXProgram* p);

/* main compiler options */
CLRX_INTERNAL bool detectCLRXCompilerCall(const char* compilerOptions);

CLRX_INTERNAL cl_int clrxCompilerCall(CLRXProgram* program, const char* compilerOptions,
            cl_uint devicesNum, CLRXDevice* const* devices);

CLRX_INTERNAL void clrxAbort(const char* abortStr);
CLRX_INTERNAL void clrxAbort(const char* abortStr, const char* exStr);

/* internal macros */
#define CLRX_INITIALIZE \
    { \
        try \
        { CLRX::callOnce(clrxOnceFlag, clrxWrapperInitialize); } \
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
        { CLRX::callOnce(clrxOnceFlag, clrxWrapperInitialize); } \
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
        { CLRX::callOnce(clrxOnceFlag, clrxWrapperInitialize); } \
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
    outObject->dispatch = c->dispatch; \
    outObject->AMDOBJECTMEMBER = AMDOBJECT; \
    outObject->context = c; \
    \
    clrxRetainOnlyCLRXContext(c);

#define CLRX_CALL_QUEUE_COMMAND \
    cl_event amdEvent = nullptr; \
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr; \
    if (event_wait_list != nullptr) \
    { \
        if (num_events_in_wait_list <= maxLocalEventsNum) \
        { \
            cl_event amdWaitList[maxLocalEventsNum]; \
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

static const cxuint maxLocalEventsNum = 50;

#endif
