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
#include <CLRX/utils/Containers.h>
#include "CLWrapper.h"

extern "C"
{

/* public API definitions */
#ifdef _MSC_VER
#  ifdef HAVE_32BIT
#    pragma comment(linker,"/export:clSetEventCallback=_clrxclSetEventCallback@16")
#    pragma comment(linker,"/export:clCreateSubBuffer=_clrxclCreateSubBuffer@24")
#    pragma comment(linker,"/export:clSetMemObjectDestructorCallback=_clrxclSetMemObjectDestructorCallback@12")
#    pragma comment(linker,"/export:clCreateUserEvent=_clrxclCreateUserEvent@8")
#    pragma comment(linker,"/export:clSetUserEventStatus=_clrxclSetUserEventStatus@8")
#    pragma comment(linker,"/export:clEnqueueReadBufferRect=_clrxclEnqueueReadBufferRect@56")
#    pragma comment(linker,"/export:clEnqueueWriteBufferRect=_clrxclEnqueueWriteBufferRect@56")
#    pragma comment(linker,"/export:clEnqueueCopyBufferRect=_clrxclEnqueueCopyBufferRect@52")
#    pragma comment(linker,"/export:clCreateSubDevicesEXT=_clrxclCreateSubDevicesEXT@20")
#    pragma comment(linker,"/export:clRetainDeviceEXT=_clrxclRetainDeviceEXT@4")
#    pragma comment(linker,"/export:clReleaseDeviceEXT=_clrxclReleaseDeviceEXT@4")
#    ifdef HAVE_OPENGL
#    pragma comment(linker,"/export:clCreateEventFromGLsyncKHR=_clrxclCreateEventFromGLsyncKHR@12")
#    endif
#    ifdef CL_VERSION_1_2
#    pragma comment(linker,"/export:clCreateSubDevices=_clrxclCreateSubDevices@20")
#    pragma comment(linker,"/export:clRetainDevice=_clrxclRetainDevice@4")
#    pragma comment(linker,"/export:clReleaseDevice=_clrxclReleaseDevice@4")
#    pragma comment(linker,"/export:clCreateImage=_clrxclCreateImage@28")
#    pragma comment(linker,"/export:clCreateProgramWithBuiltInKernels=_clrxclCreateProgramWithBuiltInKernels@20")
#    pragma comment(linker,"/export:clCompileProgram=_clrxclCompileProgram@36")
#    pragma comment(linker,"/export:clLinkProgram=_clrxclLinkProgram@36")
#    pragma comment(linker,"/export:clUnloadPlatformCompiler=_clrxclUnloadPlatformCompiler@4")
#    pragma comment(linker,"/export:clGetKernelArgInfo=_clrxclGetKernelArgInfo@24")
#    pragma comment(linker,"/export:clEnqueueFillBuffer=_clrxclEnqueueFillBuffer@36")
#    pragma comment(linker,"/export:clEnqueueFillImage=_clrxclEnqueueFillImage@32")
#    pragma comment(linker,"/export:clEnqueueMigrateMemObjects=_clrxclEnqueueMigrateMemObjects@32")
#    pragma comment(linker,"/export:clEnqueueMarkerWithWaitList=_clrxclEnqueueMarkerWithWaitList@16")
#    pragma comment(linker,"/export:clEnqueueBarrierWithWaitList=_clrxclEnqueueBarrierWithWaitList@16")
#    pragma comment(linker,"/export:clGetExtensionFunctionAddressForPlatform=_clrxclGetExtensionFunctionAddressForPlatform@8")
#    ifdef HAVE_OPENGL
#    pragma comment(linker,"/export:clCreateFromGLTexture=_clrxclCreateFromGLTexture@28")
#    endif
#    pragma comment(linker,"/export:clEnqueueWaitSignalAMD=_clrxclEnqueueWaitSignalAMD@24")
#    pragma comment(linker,"/export:clEnqueueWriteSignalAMD=_clrxclEnqueueWriteSignalAMD@32")
#    pragma comment(linker,"/export:clEnqueueMakeBuffersResidentAMD=_clrxclEnqueueMakeBuffersResidentAMD@32")
#    endif
#    ifdef CL_VERSION_2_0
#    pragma comment(linker,"/export:clCreateCommandQueueWithProperties=_clrxclCreateCommandQueueWithProperties@16")
#    pragma comment(linker,"/export:clCreatePipe=_clrxclCreatePipe@28")
#    pragma comment(linker,"/export:clGetPipeInfo=_clrxclGetPipeInfo@20")
#    pragma comment(linker,"/export:clSVMAlloc=_clrxclSVMAlloc@20")
#    pragma comment(linker,"/export:clSVMFree=_clrxclSVMFree@8")
#    pragma comment(linker,"/export:clEnqueueSVMFree=_clrxclEnqueueSVMFree@32")
#    pragma comment(linker,"/export:clEnqueueSVMMemcpy=_clrxclEnqueueSVMMemcpy@32")
#    pragma comment(linker,"/export:clEnqueueSVMMemFill=_clrxclEnqueueSVMMemFill@32")
#    pragma comment(linker,"/export:clEnqueueSVMMap=_clrxclEnqueueSVMMap@36")
#    pragma comment(linker,"/export:clEnqueueSVMUnmap=_clrxclEnqueueSVMUnmap@20")
#    pragma comment(linker,"/export:clCreateSamplerWithProperties=_clrxclCreateSamplerWithProperties@12")
#    pragma comment(linker,"/export:clSetKernelArgSVMPointer=_clrxclSetKernelArgSVMPointer@12")
#    pragma comment(linker,"/export:clSetKernelExecInfo=_clrxclSetKernelExecInfo@16")
#    endif
#  else
#    pragma comment(linker,"/export:clSetEventCallback=clrxclSetEventCallback")
#    pragma comment(linker,"/export:clCreateSubBuffer=clrxclCreateSubBuffer")
#    pragma comment(linker,"/export:clSetMemObjectDestructorCallback=clrxclSetMemObjectDestructorCallback")
#    pragma comment(linker,"/export:clCreateUserEvent=clrxclCreateUserEvent")
#    pragma comment(linker,"/export:clSetUserEventStatus=clrxclSetUserEventStatus")
#    pragma comment(linker,"/export:clEnqueueReadBufferRect=clrxclEnqueueReadBufferRect")
#    pragma comment(linker,"/export:clEnqueueWriteBufferRect=clrxclEnqueueWriteBufferRect")
#    pragma comment(linker,"/export:clEnqueueCopyBufferRect=clrxclEnqueueCopyBufferRect")
#    pragma comment(linker,"/export:clCreateSubDevicesEXT=clrxclCreateSubDevicesEXT")
#    pragma comment(linker,"/export:clRetainDeviceEXT=clrxclRetainDeviceEXT")
#    pragma comment(linker,"/export:clReleaseDeviceEXT=clrxclReleaseDeviceEXT")
#    ifdef HAVE_OPENGL
#    pragma comment(linker,"/export:clCreateEventFromGLsyncKHR=clrxclCreateEventFromGLsyncKHR")
#    endif
#    ifdef CL_VERSION_1_2
#    pragma comment(linker,"/export:clCreateSubDevices=clrxclCreateSubDevices")
#    pragma comment(linker,"/export:clRetainDevice=clrxclRetainDevice")
#    pragma comment(linker,"/export:clReleaseDevice=clrxclReleaseDevice")
#    pragma comment(linker,"/export:clCreateImage=clrxclCreateImage")
#    pragma comment(linker,"/export:clCreateProgramWithBuiltInKernels=clrxclCreateProgramWithBuiltInKernels")
#    pragma comment(linker,"/export:clCompileProgram=clrxclCompileProgram")
#    pragma comment(linker,"/export:clLinkProgram=clrxclLinkProgram")
#    pragma comment(linker,"/export:clUnloadPlatformCompiler=clrxclUnloadPlatformCompiler")
#    pragma comment(linker,"/export:clGetKernelArgInfo=clrxclGetKernelArgInfo")
#    pragma comment(linker,"/export:clEnqueueFillBuffer=clrxclEnqueueFillBuffer")
#    pragma comment(linker,"/export:clEnqueueFillImage=clrxclEnqueueFillImage")
#    pragma comment(linker,"/export:clEnqueueMigrateMemObjects=clrxclEnqueueMigrateMemObjects")
#    pragma comment(linker,"/export:clEnqueueMarkerWithWaitList=clrxclEnqueueMarkerWithWaitList")
#    pragma comment(linker,"/export:clEnqueueBarrierWithWaitList=clrxclEnqueueBarrierWithWaitList")
#    pragma comment(linker,"/export:clGetExtensionFunctionAddressForPlatform=clrxclGetExtensionFunctionAddressForPlatform")
#    ifdef HAVE_OPENGL
#    pragma comment(linker,"/export:clCreateFromGLTexture=clrxclCreateFromGLTexture")
#    endif
#    pragma comment(linker,"/export:clEnqueueWaitSignalAMD=clrxclEnqueueWaitSignalAMD")
#    pragma comment(linker,"/export:clEnqueueWriteSignalAMD=clrxclEnqueueWriteSignalAMD")
#    pragma comment(linker,"/export:clEnqueueMakeBuffersResidentAMD=clrxclEnqueueMakeBuffersResidentAMD")
#    endif
#    ifdef CL_VERSION_2_0
#    pragma comment(linker,"/export:clCreateCommandQueueWithProperties=clrxclCreateCommandQueueWithProperties")
#    pragma comment(linker,"/export:clCreatePipe=clrxclCreatePipe")
#    pragma comment(linker,"/export:clGetPipeInfo=clrxclGetPipeInfo")
#    pragma comment(linker,"/export:clSVMAlloc=clrxclSVMAlloc")
#    pragma comment(linker,"/export:clSVMFree=clrxclSVMFree")
#    pragma comment(linker,"/export:clEnqueueSVMFree=clrxclEnqueueSVMFree")
#    pragma comment(linker,"/export:clEnqueueSVMMemcpy=clrxclEnqueueSVMMemcpy")
#    pragma comment(linker,"/export:clEnqueueSVMMemFill=clrxclEnqueueSVMMemFill")
#    pragma comment(linker,"/export:clEnqueueSVMMap=clrxclEnqueueSVMMap")
#    pragma comment(linker,"/export:clEnqueueSVMUnmap=clrxclEnqueueSVMUnmap")
#    pragma comment(linker,"/export:clCreateSamplerWithProperties=clrxclCreateSamplerWithProperties")
#    pragma comment(linker,"/export:clSetKernelArgSVMPointer=clrxclSetKernelArgSVMPointer")
#    pragma comment(linker,"/export:clSetKernelExecInfo=clrxclSetKernelExecInfo")
#    endif
#  endif
#else
CLRX_CL_PUBLIC_SYM(clSetEventCallback)
CLRX_CL_PUBLIC_SYM(clCreateSubBuffer)
CLRX_CL_PUBLIC_SYM(clSetMemObjectDestructorCallback)
CLRX_CL_PUBLIC_SYM(clCreateUserEvent)
CLRX_CL_PUBLIC_SYM(clSetUserEventStatus)
CLRX_CL_PUBLIC_SYM(clEnqueueReadBufferRect)
CLRX_CL_PUBLIC_SYM(clEnqueueWriteBufferRect)
CLRX_CL_PUBLIC_SYM(clEnqueueCopyBufferRect)
CLRX_CL_PUBLIC_SYM(clCreateSubDevicesEXT)
CLRX_CL_PUBLIC_SYM(clRetainDeviceEXT)
CLRX_CL_PUBLIC_SYM(clReleaseDeviceEXT)
#ifdef HAVE_OPENGL
CLRX_CL_PUBLIC_SYM(clCreateEventFromGLsyncKHR)
#endif
#ifdef CL_VERSION_1_2
CLRX_CL_PUBLIC_SYM(clCreateSubDevices)
CLRX_CL_PUBLIC_SYM(clRetainDevice)
CLRX_CL_PUBLIC_SYM(clReleaseDevice)
CLRX_CL_PUBLIC_SYM(clCreateImage)
CLRX_CL_PUBLIC_SYM(clCreateProgramWithBuiltInKernels)
CLRX_CL_PUBLIC_SYM(clCompileProgram)
CLRX_CL_PUBLIC_SYM(clLinkProgram)
CLRX_CL_PUBLIC_SYM(clUnloadPlatformCompiler)
CLRX_CL_PUBLIC_SYM(clGetKernelArgInfo)
CLRX_CL_PUBLIC_SYM(clEnqueueFillBuffer)
CLRX_CL_PUBLIC_SYM(clEnqueueFillImage)
CLRX_CL_PUBLIC_SYM(clEnqueueMigrateMemObjects)
CLRX_CL_PUBLIC_SYM(clEnqueueMarkerWithWaitList)
CLRX_CL_PUBLIC_SYM(clEnqueueBarrierWithWaitList)
CLRX_CL_PUBLIC_SYM(clGetExtensionFunctionAddressForPlatform)
#ifdef HAVE_OPENGL
CLRX_CL_PUBLIC_SYM(clCreateFromGLTexture)
#endif
CLRX_CL_PUBLIC_SYM(clEnqueueWaitSignalAMD)
CLRX_CL_PUBLIC_SYM(clEnqueueWriteSignalAMD)
CLRX_CL_PUBLIC_SYM(clEnqueueMakeBuffersResidentAMD)
#endif
#ifdef CL_VERSION_2_0
CLRX_CL_PUBLIC_SYM(clCreateCommandQueueWithProperties)
CLRX_CL_PUBLIC_SYM(clCreatePipe)
CLRX_CL_PUBLIC_SYM(clGetPipeInfo)
CLRX_CL_PUBLIC_SYM(clSVMAlloc)
CLRX_CL_PUBLIC_SYM(clSVMFree)
CLRX_CL_PUBLIC_SYM(clEnqueueSVMFree)
CLRX_CL_PUBLIC_SYM(clEnqueueSVMMemcpy)
CLRX_CL_PUBLIC_SYM(clEnqueueSVMMemFill)
CLRX_CL_PUBLIC_SYM(clEnqueueSVMMap)
CLRX_CL_PUBLIC_SYM(clEnqueueSVMUnmap)
CLRX_CL_PUBLIC_SYM(clCreateSamplerWithProperties)
CLRX_CL_PUBLIC_SYM(clSetKernelArgSVMPointer)
CLRX_CL_PUBLIC_SYM(clSetKernelExecInfo)
#endif
#endif

/* end of public API definitions */

#define CLRX_CLCOMMAND_PREFIX q->amdOclCommandQueue->dispatch->

CL_API_ENTRY cl_int CL_API_CALL
clrxclSetEventCallback( cl_event    event,
                    cl_int      command_exec_callback_type,
                    void (CL_CALLBACK * pfn_notify)(cl_event, cl_int, void *),
                    void *      user_data) CL_API_SUFFIX__VERSION_1_1
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    if (pfn_notify == nullptr)
        return CL_INVALID_VALUE;
    CLRXEvent* e = static_cast<CLRXEvent*>(event);
    
    cl_int status = CL_SUCCESS;
    try
    {
        CLRXEventCallbackUserData* wrappedData = new CLRXEventCallbackUserData;
        wrappedData->clrxEvent = e;
        wrappedData->realNotify = pfn_notify;
        wrappedData->realUserData = user_data;
        
        status = e->amdOclEvent->dispatch->clSetEventCallback(e->amdOclEvent,
                 command_exec_callback_type, clrxEventCallbackWrapper, wrappedData);
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    return status;
}

CL_API_ENTRY cl_mem CL_API_CALL
clrxclCreateSubBuffer(cl_mem                   buffer,
                  cl_mem_flags             flags,
                  cl_buffer_create_type    buffer_create_type,
                  const void *             buffer_create_info,
                  cl_int *                 errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
    if (buffer == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_MEM_OBJECT;
        return nullptr;
    }
    
    CLRXMemObject* b = static_cast<CLRXMemObject*>(buffer);
    const cl_mem amdBuffer = b->amdOclMemObject->dispatch->clCreateSubBuffer(
                b->amdOclMemObject, flags, buffer_create_type, buffer_create_info,
                errcode_ret);
    
    if (amdBuffer == nullptr)
        return nullptr;
    
    CLRXContext* c = static_cast<CLRXContext*>(b->context);
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdBuffer,
              clReleaseMemObject,
              "Fatal Error at handling error at subbufer creation!")
    
    outObject->parent = b;
    
    clrxRetainOnlyCLRXMemObject(b);
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclSetMemObjectDestructorCallback(  cl_mem memobj, 
                void (CL_CALLBACK * pfn_notify)( cl_mem, void* ), 
                void * user_data)             CL_API_SUFFIX__VERSION_1_1
{
    if (memobj == nullptr)
        return CL_INVALID_MEM_OBJECT;
    if (pfn_notify == nullptr)
        return CL_INVALID_VALUE;
    CLRXMemObject* m = static_cast<CLRXMemObject*>(memobj);
    
    cl_int status = CL_SUCCESS;
    try
    {
        CLRXMemDtorCallbackUserData* wrappedData = new CLRXMemDtorCallbackUserData;
        wrappedData->clrxMemObject = m;
        wrappedData->realNotify = pfn_notify;
        wrappedData->realUserData = user_data;
        
        status = m->amdOclMemObject->dispatch->clSetMemObjectDestructorCallback(
                m->amdOclMemObject, clrxMemDtorCallbackWrapper, wrappedData);
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    return status;
}

CL_API_ENTRY cl_event CL_API_CALL
clrxclCreateUserEvent(cl_context    context,
                  cl_int *      errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_event amdEvent = c->amdOclContext->dispatch->clCreateUserEvent(
            c->amdOclContext, errcode_ret);
    
    if (amdEvent == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXEvent, amdOclEvent, amdEvent,
              clReleaseEvent,
              "Fatal Error at handling error at userEvent creation!")
    
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclSetUserEventStatus(cl_event   event,
                     cl_int     execution_status) CL_API_SUFFIX__VERSION_1_1
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    
    const CLRXEvent* e = static_cast<const CLRXEvent*>(event);
    return e->amdOclEvent->dispatch->clSetUserEventStatus(e->amdOclEvent,
                      execution_status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueReadBufferRect(cl_command_queue    command_queue,
                        cl_mem              buffer,
                        cl_bool             blocking_read,
                        const size_t *      buffer_origin,
                        const size_t *      host_origin,
                        const size_t *      region,
                        size_t              buffer_row_pitch,
                        size_t              buffer_slice_pitch,
                        size_t              host_row_pitch,
                        size_t              host_slice_pitch,
                        void *              ptr,
                        cl_uint             num_events_in_wait_list,
                        const cl_event *    event_wait_list,
                        cl_event *          event) CL_API_SUFFIX__VERSION_1_1
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_int status = CL_SUCCESS;
    
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueReadBufferRect(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_read, buffer_origin, host_origin, region, \
            buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueWriteBufferRect(cl_command_queue    command_queue,
                         cl_mem              buffer,
                         cl_bool             blocking_write,
                         const size_t *      buffer_origin,
                         const size_t *      host_origin, 
                         const size_t *      region,
                         size_t              buffer_row_pitch,
                         size_t              buffer_slice_pitch,
                         size_t              host_row_pitch,
                         size_t              host_slice_pitch,
                         const void *        ptr,
                         cl_uint             num_events_in_wait_list,
                         const cl_event *    event_wait_list,
                         cl_event *          event) CL_API_SUFFIX__VERSION_1_1
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_int status = CL_SUCCESS;
    
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueWriteBufferRect(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_write, buffer_origin, host_origin, region, \
            buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueCopyBufferRect(cl_command_queue    command_queue,
                        cl_mem              src_buffer,
                        cl_mem              dst_buffer, 
                        const size_t *      src_origin,
                        const size_t *      dst_origin,
                        const size_t *      region,
                        size_t              src_row_pitch,
                        size_t              src_slice_pitch,
                        size_t              dst_row_pitch,
                        size_t              dst_slice_pitch,
                        cl_uint             num_events_in_wait_list,
                        const cl_event *    event_wait_list,
                        cl_event *          event) CL_API_SUFFIX__VERSION_1_1
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (src_buffer == nullptr || dst_buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* sb = static_cast<const CLRXMemObject*>(src_buffer);
    const CLRXMemObject* db = static_cast<const CLRXMemObject*>(dst_buffer);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueCopyBufferRect(q->amdOclCommandQueue, \
            sb->amdOclMemObject, db->amdOclMemObject, src_origin, dst_origin, region, \
            src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL clrxclCreateSubDevicesEXT(cl_device_id in_device,
                            const cl_device_partition_property_ext * properties,
                            cl_uint num_entries,
                            cl_device_id * out_devices,
                            cl_uint * num_devices) CL_EXT_SUFFIX__VERSION_1_1
{
    if (in_device == nullptr)
        return CL_INVALID_DEVICE;
    if ((num_entries == 0 && out_devices != nullptr) ||
        (out_devices == nullptr && num_devices == nullptr))
        return CL_INVALID_VALUE;
    
    CLRXDevice* d = static_cast<CLRXDevice*>(in_device);
    cl_uint devicesNum = 0;
    cl_int status = d->amdOclDevice->dispatch->clCreateSubDevicesEXT(
            d->amdOclDevice, properties, num_entries, out_devices, &devicesNum);
    if (status != CL_SUCCESS)
        return status;
    
    if (out_devices != nullptr)
    {
        const cl_int curSubDevicesNum = std::min(devicesNum, num_entries);
        status = clrxCreateOutDevices(d, curSubDevicesNum, out_devices,
              d->amdOclDevice->dispatch->clReleaseDeviceEXT,
              "Fatal error at handling error for clCreateSubDevicesEXT");
        if (status != CL_SUCCESS)
            return status;
        clrxRetainOnlyCLRXDeviceNTimes(d, curSubDevicesNum);
    }
    
    if (num_devices != nullptr)
        *num_devices = devicesNum;
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainDeviceEXT(cl_device_id device) CL_EXT_SUFFIX__VERSION_1_1
{
    if (device == nullptr)
        return CL_INVALID_DEVICE;
    
    CLRXDevice* d = static_cast<CLRXDevice*>(device);
    const cl_uint status = d->amdOclDevice->dispatch->clRetainDeviceEXT(d->amdOclDevice);
    if (status == CL_SUCCESS)
        if (d->parent != nullptr)
            d->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseDeviceEXT(cl_device_id device) CL_EXT_SUFFIX__VERSION_1_1
{
    if (device == nullptr)
        return CL_INVALID_DEVICE;
    
    CLRXDevice* d = static_cast<CLRXDevice*>(device);
    const cl_uint status = d->amdOclDevice->dispatch->clReleaseDeviceEXT(d->amdOclDevice);
    if (status == CL_SUCCESS && d->parent != nullptr)
        clrxReleaseOnlyCLRXDevice(d);
    return status;
}

#ifdef HAVE_OPENGL
CL_API_ENTRY cl_event CL_API_CALL
clrxclCreateEventFromGLsyncKHR(cl_context           context,
                   cl_GLsync            cl_GLsync,
                   cl_int *             errcode_ret) CL_EXT_SUFFIX__VERSION_1_1
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_event amdEvent = c->amdOclContext->dispatch->clCreateEventFromGLsyncKHR(
            c->amdOclContext, cl_GLsync, errcode_ret);
    
    if (amdEvent == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXEvent, amdOclEvent, amdEvent,
              clReleaseEvent,
              "Fatal Error at handling error at userEvent creation!")
    
    return outObject;
}
#endif

#ifdef CL_VERSION_1_2
CL_API_ENTRY cl_int CL_API_CALL
clrxclCreateSubDevices(cl_device_id  in_device,
               const cl_device_partition_property * properties,
               cl_uint           num_devices,
               cl_device_id *    out_devices,
               cl_uint *         num_devices_ret) CL_API_SUFFIX__VERSION_1_2
{
    if (in_device == nullptr)
        return CL_INVALID_DEVICE;
    if (num_devices == 0 && out_devices != nullptr)
        return CL_INVALID_VALUE;
    
    CLRXDevice* d = static_cast<CLRXDevice*>(in_device);
    cl_uint devicesNum = 0;
    cl_int status = d->amdOclDevice->dispatch->clCreateSubDevices(
            d->amdOclDevice, properties, num_devices, out_devices, &devicesNum);
    if (status != CL_SUCCESS)
        return status;
    
    if (out_devices != nullptr)
    {
        status = clrxCreateOutDevices(d, devicesNum, out_devices,
              d->amdOclDevice->dispatch->clReleaseDevice,
              "Fatal error at handling error for clCreateSubDevices");
        if (status != CL_SUCCESS)
            return status;
        clrxRetainOnlyCLRXDeviceNTimes(d, devicesNum);
    }
    
    if (num_devices_ret != nullptr)
        *num_devices_ret = devicesNum;
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclRetainDevice(cl_device_id device) CL_API_SUFFIX__VERSION_1_2
{
    if (device == nullptr)
        return CL_INVALID_DEVICE;
    
    CLRXDevice* d = static_cast<CLRXDevice*>(device);
    const cl_uint status = d->amdOclDevice->dispatch->clRetainDevice(d->amdOclDevice);
    if (status == CL_SUCCESS)
        if (d->parent != nullptr)
            d->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseDevice(cl_device_id device) CL_API_SUFFIX__VERSION_1_2
{
    if (device == nullptr)
        return CL_INVALID_DEVICE;
    
    CLRXDevice* d = static_cast<CLRXDevice*>(device);
    const cl_uint status = d->amdOclDevice->dispatch->clReleaseDevice(d->amdOclDevice);
    if (status == CL_SUCCESS && d->parent != nullptr)
        clrxReleaseOnlyCLRXDevice(d);
    return status;
}

CL_API_ENTRY cl_mem CL_API_CALL
clrxclCreateImage(cl_context              context,
              cl_mem_flags            flags,
              const cl_image_format * image_format,
              const cl_image_desc *   image_desc, 
              void *                  host_ptr,
              cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    
    cl_image_desc imageDesc;
    bool processedBufferField = false;
    if (image_desc != nullptr)
    {
        imageDesc = *image_desc;
        
        if ((imageDesc.image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER ||
                imageDesc.image_type == CL_MEM_OBJECT_IMAGE2D) &&
                imageDesc.buffer != nullptr) // check buffer field
        {
            /* do it if buffer not null and type is CL_MEM_OBJECT_IMAGE1D_BUFFER or
             * CL_MEM_OBJECT_IMAGE2D, otherwise we ignore buffer field */
            imageDesc.buffer =
                static_cast<const CLRXMemObject*>(imageDesc.buffer)->amdOclMemObject;
            processedBufferField = true;
        }
    }
    
    const cl_mem amdImage = c->amdOclContext->dispatch->clCreateImage(c->amdOclContext,
              flags, image_format, &imageDesc, host_ptr, errcode_ret);
    
    if (amdImage == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdImage,
              clReleaseMemObject,
              "Fatal Error at handling error at image creation!")
    
    if (processedBufferField)
    {
        outObject->buffer = (CLRXMemObject*)image_desc->buffer;
        clrxRetainOnlyCLRXMemObject(outObject->buffer);
    }
    
    return outObject;
}

CL_API_ENTRY cl_program CL_API_CALL
clrxclCreateProgramWithBuiltInKernels(cl_context            context,
                  cl_uint               num_devices,
                  const cl_device_id *  device_list,
                  const char *          kernel_names,
                  cl_int *              errcode_ret) CL_API_SUFFIX__VERSION_1_2
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
        // get original AMD devices before calling
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
        
        amdProgram = c->amdOclContext->dispatch->clCreateProgramWithBuiltInKernels(
                c->amdOclContext, num_devices, amdDevices.data(),
                kernel_names, errcode_ret);
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

extern CL_API_ENTRY cl_int CL_API_CALL
clrxclCompileProgram(cl_program           program,
                 cl_uint              num_devices,
                 const cl_device_id * device_list,
                 const char *         options,
                 cl_uint              num_input_headers,
                 const cl_program *   input_headers,
                 const char **        header_include_names,
                 void (CL_CALLBACK *  pfn_notify)(cl_program program, void * user_data),
                 void *               user_data) CL_API_SUFFIX__VERSION_1_2
{
    if (program == nullptr)
        return CL_INVALID_PROGRAM;
    
    if ((num_devices != 0 && device_list == nullptr) ||
        (num_devices == 0 && device_list != nullptr))
        return CL_INVALID_VALUE;
    if ((num_input_headers != 0 && input_headers == nullptr) ||
        (num_input_headers == 0 && input_headers != nullptr))
        return CL_INVALID_VALUE;
    if ((num_input_headers != 0 && header_include_names == nullptr) ||
        (num_input_headers == 0 && header_include_names != nullptr))
        return CL_INVALID_VALUE;
    
    bool doDeleteWrappedData = true;
    void* destUserData = user_data;
    CLRXBuildProgramUserData* wrappedData = nullptr;
    void (CL_CALLBACK *  notifyToCall)(cl_program, void *) =  nullptr;
    
    try
    {
    CLRXProgram* p = static_cast<CLRXProgram*>(program);
    
    if (pfn_notify != nullptr)
    {
        wrappedData = new CLRXBuildProgramUserData;
        wrappedData->realNotify = pfn_notify;
        wrappedData->clrxProgram = p;
        wrappedData->realUserData = user_data;
        wrappedData->callDone = false;
        wrappedData->inClFunction = false;
        destUserData = wrappedData;
        notifyToCall = clrxBuildProgramNotifyWrapper;
    }
    
    {
        std::lock_guard<std::mutex> lock(p->mutex);
        clrxClearProgramAsmState(p);
        
        if (p->kernelsAttached != 0) // if kernels attached
            return CL_INVALID_OPERATION;
        p->kernelArgFlagsInitialized = false;
        p->concurrentBuilds++;
    }
    
    cl_int status;
    try
    {
        /* translate into AMD program headers */
        std::vector<cl_program> amdHeaders(num_input_headers);
        for (cl_uint i = 0; i < num_input_headers; i++)
        {
            if (input_headers[i] == nullptr)
            {
                // bad header
                std::lock_guard<std::mutex> lock(p->mutex);
                clrxReleaseConcurrentBuild(p);
                delete wrappedData;
                return CL_INVALID_PROGRAM;
            }
            amdHeaders[i] = static_cast<CLRXProgram*>(input_headers[i])->amdOclProgram;
        }
        cl_program* amdHeadersPtr = (input_headers!=nullptr) ? amdHeaders.data() : nullptr;
    
        if (num_devices == 0)
            status = p->amdOclProgram->dispatch->clCompileProgram(
                p->amdOclProgram, 0, nullptr, options, num_input_headers, amdHeadersPtr,
                header_include_names, notifyToCall, destUserData);
        else
        {
            std::vector<cl_device_id> amdDevices(num_devices);
            // get original AMD devices before calling
            for (cl_uint i = 0; i < num_devices; i++)
            {
                if (device_list[i] == nullptr)
                {
                    // bad device
                    std::lock_guard<std::mutex> lock(p->mutex);
                    clrxReleaseConcurrentBuild(p);
                    delete wrappedData;
                    return CL_INVALID_DEVICE;
                }
                amdDevices[i] =
                    static_cast<const CLRXDevice*>(device_list[i])->amdOclDevice;
            }
            // intialize trans device map for new CLRX program
            clrxInitProgramTransDevicesMap(p, num_devices, device_list, amdDevices);
            
            status = p->amdOclProgram->dispatch->clCompileProgram(
                    p->amdOclProgram, num_devices, amdDevices.data(),
                    options, num_input_headers, amdHeadersPtr, header_include_names,
                    notifyToCall, destUserData);
        }
    }
    catch(const std::bad_alloc& ex)
    {
        // if allocation failed
        std::lock_guard<std::mutex> lock(p->mutex);
        clrxReleaseConcurrentBuild(p);
        delete wrappedData;
        return CL_OUT_OF_HOST_MEMORY;
    }
    /* after this we update associated devices and decrease concurrentBuilds in
     * in this place or in pfn_notify call */
    
    std::lock_guard<std::mutex> lock(p->mutex);
    // determine whether wrapped must deleted when out of memory happened
    // delete wrappedData if clCompileProgram not finished successfully
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
        clrxReleaseConcurrentBuild(p);
        if (wrappedData != nullptr)
            wrappedData->inClFunction = true;
    }
    // delete wrappedData if clCompileProgram not finished successfully
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
        return 1;
    }
}

CL_API_ENTRY cl_program CL_API_CALL
clrxclLinkProgram(cl_context           context,
              cl_uint              num_devices,
              const cl_device_id * device_list,
              const char *         options,
              cl_uint              num_input_programs,
              const cl_program *   input_programs,
              void (CL_CALLBACK *  pfn_notify)(cl_program program, void * user_data),
              void *               user_data,
              cl_int *             errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    if ((num_devices != 0 && device_list == nullptr) ||
        (num_devices == 0 && device_list != nullptr) ||
        (num_input_programs == 0 || input_programs == nullptr))
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_VALUE;
        return nullptr;
    }
    
    void* destUserData = user_data;
    CLRXLinkProgramUserData* wrappedData = nullptr;
    void (CL_CALLBACK *  notifyToCall)(cl_program, void *) =  nullptr;
    CLRXContext* c = static_cast<CLRXContext*>(context);
    
    if (pfn_notify != nullptr)
    {
        // prepare wrapper for notify (will be called by original OpenCL call)
        wrappedData = new CLRXLinkProgramUserData;
        wrappedData->realNotify = pfn_notify;
        wrappedData->clrxProgramFilled = false;
        wrappedData->clrxProgram = nullptr;
        wrappedData->clrxContext = c;
        wrappedData->realUserData = user_data;
        wrappedData->transDevicesMap = nullptr;
        destUserData = wrappedData;
        notifyToCall = clrxLinkProgramNotifyWrapper;
    }
    
    cl_program amdProgram = nullptr;
    CLRXProgram* outProgram = nullptr;
    CLRXProgramDevicesMap* transDevicesMap = nullptr;
    try
    {
        // get original AMDOCL programs to call
        std::vector<cl_program> amdInputPrograms(num_input_programs);
        for (cl_uint i = 0; i < num_input_programs; i++)
        {
            if (input_programs[i] == nullptr)
            {
                delete wrappedData;
                if (errcode_ret != nullptr)
                    *errcode_ret = CL_INVALID_PROGRAM;
                return nullptr;
            }
            amdInputPrograms[i] = static_cast<CLRXProgram*>(
                    input_programs[i])->amdOclProgram;
        }
        
        if (num_devices != 0)
        {
            // get original AMDOCL devices to call and to translating devices
            std::vector<cl_device_id> amdDevices(num_devices);
            for (cl_uint i = 0; i < num_devices; i++)
            {
                if (device_list[i] == nullptr)
                {
                    delete wrappedData;
                    if (errcode_ret != nullptr)
                        *errcode_ret = CL_INVALID_DEVICE;
                    return nullptr;
                }
                amdDevices[i] = static_cast<CLRXDevice*>(device_list[i])->amdOclDevice;
            }
            /* create transDevicesMap if needed (if device_list present) */
            transDevicesMap = new CLRXProgramDevicesMap;
            for (cl_uint i = 0; i < c->devicesNum; i++)
            {
                CLRXDevice* device = c->devices[i];
                transDevicesMap->insert(std::make_pair(device->amdOclDevice, device));
            }
            
            // add device_list into translate device map
            for (cl_uint i = 0; i < num_devices; i++)
                transDevicesMap->insert(std::make_pair(amdDevices[i],
                      device_list[i]));
            if (wrappedData != nullptr)
                wrappedData->transDevicesMap = transDevicesMap;
            
            amdProgram = c->amdOclContext->dispatch->clLinkProgram(c->amdOclContext,
                    num_devices, amdDevices.data(), options, num_input_programs, 
                    amdInputPrograms.data(), notifyToCall, destUserData, errcode_ret);
        }
        else // no device_list
            amdProgram = c->amdOclContext->dispatch->clLinkProgram(c->amdOclContext,
                    0, nullptr, options, num_input_programs, 
                    amdInputPrograms.data(), notifyToCall, destUserData, errcode_ret);
        
        if (wrappedData != nullptr)
        {
            bool initializedByCLCall = false;
            {
                std::lock_guard<std::mutex> l(wrappedData->mutex);
                if (!wrappedData->clrxProgramFilled)
                {
                    if (amdProgram != nullptr)
                    {
                        // we fill outProgram if already not filled by notify callback
                        outProgram = new CLRXProgram;
                        outProgram->dispatch = c->dispatch;
                        outProgram->amdOclProgram = amdProgram;
                        outProgram->context = c;
                        outProgram->transDevicesMap = transDevicesMap;
                        clrxUpdateProgramAssocDevices(outProgram);
                        outProgram->transDevicesMap = nullptr;
                        clrxRetainOnlyCLRXContext(c);
                        
                        wrappedData->clrxProgramFilled = true;
                        wrappedData->clrxProgram = outProgram;
                        initializedByCLCall = true; // force skip deletion of wrappedData
                    }
                    // error occurred and no callback called, delete wrappedData
                }
                else
                {
                    // get from wrapped data our program
                    if (amdProgram != nullptr) // only if returned not null program
                        outProgram = static_cast<CLRXProgram*>(wrappedData->clrxProgram);
                    /* otherwise we do free own CLRXProgram and delete wrappedData if
                     * initialized by callback */
                    else
                        clrxReleaseOnlyCLRXProgram(wrappedData->clrxProgram);
                }
            }
            
            if (!initializedByCLCall)
            {
                /* delete if initialized by callback */
                delete transDevicesMap;
                delete wrappedData;
                wrappedData = nullptr;
            }
        }
        else if (amdProgram != nullptr)
        {
            outProgram = new CLRXProgram;
            outProgram->dispatch = c->dispatch;
            outProgram->amdOclProgram = amdProgram;
            outProgram->context = c;
            outProgram->transDevicesMap = transDevicesMap;
            clrxUpdateProgramAssocDevices(outProgram);
            outProgram->transDevicesMap = nullptr;
            clrxRetainOnlyCLRXContext(c);
            delete transDevicesMap;
        }
    }
    catch(const std::bad_alloc& ex)
    {
        if (amdProgram != nullptr)
        {
            if (c->amdOclContext->dispatch->clReleaseProgram(amdProgram) != CL_SUCCESS)
                clrxAbort("Fatal Error at handling error at program linkage!");
        }
        delete outProgram;
        delete wrappedData;
        delete transDevicesMap;
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    catch(const std::exception& ex)
    { clrxAbort("Fatal exception happened: ", ex.what()); }
    
    return outProgram;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclUnloadPlatformCompiler(cl_platform_id platform) CL_API_SUFFIX__VERSION_1_2
{
    if (platform == nullptr)
        return CL_INVALID_PLATFORM;
    const CLRXPlatform* p = static_cast<const CLRXPlatform*>(platform);
    return p->amdOclPlatform->dispatch->clUnloadPlatformCompiler(p->amdOclPlatform);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetKernelArgInfo(cl_kernel       kernel,
                   cl_uint         arg_indx,
                   cl_kernel_arg_info  param_name,
                   size_t          param_value_size,
                   void *          param_value,
                   size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_2
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    return k->amdOclKernel->dispatch->clGetKernelArgInfo(k->amdOclKernel,
             arg_indx, param_name, param_value_size, param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueFillBuffer(cl_command_queue    command_queue,
                    cl_mem             buffer, 
                    const void *       pattern,
                    size_t             pattern_size,
                    size_t             offset,
                    size_t             size,
                    cl_uint            num_events_in_wait_list,
                    const cl_event *   event_wait_list,
                    cl_event *         event) CL_API_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueFillBuffer(q->amdOclCommandQueue, \
            b->amdOclMemObject, pattern, pattern_size, offset, size
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueFillImage(cl_command_queue   command_queue,
                   cl_mem             image,
                   const void *       fill_color,
                   const size_t *     origin, 
                   const size_t *     region,
                   cl_uint            num_events_in_wait_list,
                   const cl_event *   event_wait_list,
                   cl_event *         event) CL_API_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (image == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(image);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueFillImage(q->amdOclCommandQueue, \
            b->amdOclMemObject, fill_color, origin, region
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueMigrateMemObjects(cl_command_queue       command_queue,
                           cl_uint                num_mem_objects,
                           const cl_mem *         mem_objects,
                           cl_mem_migration_flags flags,
                           cl_uint                num_events_in_wait_list,
                           const cl_event *       event_wait_list,
                           cl_event *             event) CL_API_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (num_mem_objects == 0 || mem_objects == nullptr)
        return CL_INVALID_VALUE;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    try
    {
        std::vector<cl_mem> amdMemObjects(num_mem_objects);
        for (cl_uint i = 0; i < num_mem_objects; i++)
        {
            if (mem_objects[i] == nullptr)
                return CL_INVALID_MEM_OBJECT;
            amdMemObjects[i] = static_cast<CLRXMemObject*>(mem_objects[i])->amdOclMemObject;
        }
        
        cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueMigrateMemObjects(q->amdOclCommandQueue, \
            num_mem_objects, amdMemObjects.data(), flags
        CLRX_CALL_QUEUE_COMMAND
    
        return clrxApplyCLRXEvent(q, event, amdEvent, status);
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueMarkerWithWaitList(cl_command_queue command_queue,
                            cl_uint           num_events_in_wait_list,
                            const cl_event *  event_wait_list,
                            cl_event *        event) CL_API_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueMarkerWithWaitList(q->amdOclCommandQueue
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueBarrierWithWaitList(cl_command_queue command_queue,
                             cl_uint           num_events_in_wait_list,
                             const cl_event *  event_wait_list,
                             cl_event *        event) CL_API_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueBarrierWithWaitList(q->amdOclCommandQueue
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY void * CL_API_CALL 
clrxclGetExtensionFunctionAddressForPlatform(cl_platform_id platform,
             const char *   func_name) CL_API_SUFFIX__VERSION_1_2
{
    CLRX_INITIALIZE_VOIDPTR
    
    if (platform == nullptr)
        return nullptr;
    
    const CLRXPlatform* p = static_cast<CLRXPlatform*>(platform);
    const CLRXExtensionEntry tmp = {func_name, nullptr};
    const size_t length = sizeof(clrxExtensionsTable)/sizeof(CLRXExtensionEntry);
    const CLRXExtensionEntry* entry = CLRX::binaryFind(p->extEntries.get(),
           p->extEntries.get() + length,
           tmp, [](const CLRXExtensionEntry& l, const CLRXExtensionEntry& r) -> bool
           { return ::strcmp(l.funcname, r.funcname)<0; });
    
    if (entry == p->extEntries.get() + length)
        return nullptr;
    return entry->address;
}

#ifdef HAVE_OPENGL
CL_API_ENTRY cl_mem CL_API_CALL
clrxclCreateFromGLTexture(cl_context      context,
                      cl_mem_flags    flags,
                      cl_GLenum       target,
                      cl_GLint        miplevel,
                      cl_GLuint       texture,
                      cl_int *        errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdBuffer = c->amdOclContext->dispatch->clCreateFromGLTexture(
                c->amdOclContext, flags, target, miplevel, texture, errcode_ret);
    
    if (amdBuffer == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdBuffer,
              clReleaseMemObject,
              "Fatal Error at handling error at fromGLTexture creation!")
    
    return outObject;
}
#endif

/*
 * AMD extensions
 */

#undef CLRX_CLCOMMAND_PREFIX
#define CLRX_CLCOMMAND_PREFIX

CL_API_ENTRY cl_int CL_API_CALL clrxclEnqueueWaitSignalAMD(
                           cl_command_queue command_queue,
                           cl_mem mem_object,
                           cl_uint value,
                           cl_uint num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event * event) CL_EXT_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (mem_object == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(mem_object);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND amdOclEnqueueWaitSignalAMD(q->amdOclCommandQueue, \
            m->amdOclMemObject, value
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL clrxclEnqueueWriteSignalAMD(
                        cl_command_queue command_queue,
                        cl_mem mem_object,
                        cl_uint value,
                        cl_ulong offset,
                        cl_uint num_events_in_wait_list,
                        const cl_event * event_wait_list,
                        cl_event * event) CL_EXT_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (mem_object == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(mem_object);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND amdOclEnqueueWriteSignalAMD(q->amdOclCommandQueue, \
            m->amdOclMemObject, value, offset
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL clrxclEnqueueMakeBuffersResidentAMD(
                         cl_command_queue command_queue,
                         cl_uint num_mem_objects,
                         cl_mem * mem_objects,
                         cl_bool blocking_make_resident,
                         cl_bus_address_amd * bus_addresses,
                         cl_uint num_events_in_wait_list,
                         const cl_event * event_wait_list,
                         cl_event * event) CL_EXT_SUFFIX__VERSION_1_2
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (num_mem_objects == 0 || mem_objects == nullptr)
        return CL_INVALID_VALUE;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    try
    {
        // use vector to store original AMDOCL memobjects
        std::vector<cl_mem> amdMemObjects(num_mem_objects);
        for (cl_uint i = 0; i < num_mem_objects; i++)
        {
            if (mem_objects[i] == nullptr)
                return CL_INVALID_MEM_OBJECT;
            amdMemObjects[i] = static_cast<CLRXMemObject*>(mem_objects[i])->amdOclMemObject;
        }
        
        cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND amdOclEnqueueMakeBuffersResidentAMD(q->amdOclCommandQueue, \
            num_mem_objects, amdMemObjects.data(), blocking_make_resident, bus_addresses
        CLRX_CALL_QUEUE_COMMAND
    
        return clrxApplyCLRXEvent(q, event, amdEvent, status);
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
}

#endif /* CL_VERSION_1_2 */

#ifdef CL_VERSION_2_0
CL_API_ENTRY cl_command_queue CL_API_CALL
clrxclCreateCommandQueueWithProperties(
                cl_context context,
                cl_device_id device,
                const cl_queue_properties * properties,
                cl_int * errcode_ret) CL_API_SUFFIX__VERSION_2_0
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
    const cl_command_queue amdCmdQueue =
            c->amdOclContext->dispatch->clCreateCommandQueueWithProperties(
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

CL_API_ENTRY cl_mem CL_API_CALL
clrxclCreatePipe(cl_context                 context,
             cl_mem_flags               flags,
             cl_uint                    pipe_packet_size,
             cl_uint                    pipe_max_packets,
             const cl_pipe_properties * properties,
             cl_int *                   errcode_ret) CL_API_SUFFIX__VERSION_2_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdPipe = c->amdOclContext->dispatch->clCreatePipe(c->amdOclContext,
              flags, pipe_packet_size, pipe_max_packets, properties, errcode_ret);
    
    if (amdPipe == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdPipe,
              clReleaseMemObject,
              "Fatal Error at handling error at pipe creation!")
    
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetPipeInfo(cl_mem           pipe,
               cl_pipe_info     param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (pipe == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(pipe);
    return m->amdOclMemObject->dispatch->clGetPipeInfo(m->amdOclMemObject, param_name,
            param_value_size, param_value, param_value_size_ret);
}

CL_API_ENTRY void * CL_API_CALL
clrxclSVMAlloc(cl_context       context,
           cl_svm_mem_flags flags,
           size_t           size,
           cl_uint          alignment) CL_API_SUFFIX__VERSION_2_0
{
    if (context == nullptr)
        return nullptr;
    
    const CLRXContext* c = static_cast<const CLRXContext*>(context);
    return c->amdOclContext->dispatch->clSVMAlloc(c->amdOclContext,
                              flags, size, alignment);
}

CL_API_ENTRY void CL_API_CALL
clrxclSVMFree(cl_context        context,
          void *            svm_pointer) CL_API_SUFFIX__VERSION_2_0
{
    if (context == nullptr)
        return;
    
    const CLRXContext* c = static_cast<const CLRXContext*>(context);
    return c->amdOclContext->dispatch->clSVMFree(c->amdOclContext, svm_pointer);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueSVMFree(cl_command_queue  command_queue,
                 cl_uint           num_svm_pointers,
                 void **           svm_pointers,
         void (CL_CALLBACK * pfn_free_func)(cl_command_queue, cl_uint, void **, void *),
                 void *            user_data,
                 cl_uint           num_events_in_wait_list,
                 const cl_event *  event_wait_list,
                 cl_event *        event) CL_API_SUFFIX__VERSION_2_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    cl_event amdEvent = nullptr;
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr;
    CLRXSVMFreeCallbackUserData* wrappedData = nullptr;
    void (CL_CALLBACK * clrxFreeFunc)(cl_command_queue, cl_uint, void *[], void *) =
            nullptr;
    cl_int status = CL_SUCCESS;
    try
    {
    if (pfn_free_func != nullptr)
    {
        wrappedData = new CLRXSVMFreeCallbackUserData;
        wrappedData->clrxCommandQueue = q;
        wrappedData->realNotify = pfn_free_func;
        wrappedData->realUserData = user_data;
        clrxFreeFunc = clrxSVMFreeCallbackWrapper;
    }
        
    if (event_wait_list != nullptr)
    {
        if (num_events_in_wait_list <= maxLocalEventsNum)
        {
            // store original events in array
            cl_event amdWaitList[maxLocalEventsNum];
            for (cl_uint i = 0; i < num_events_in_wait_list; i++)
            {
                if (event_wait_list[i] == nullptr)
                    return CL_INVALID_EVENT_WAIT_LIST;
                amdWaitList[i] =
                    static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
            }
            
            status = q->amdOclCommandQueue->dispatch->clEnqueueSVMFree(
                    q->amdOclCommandQueue, num_svm_pointers, svm_pointers, clrxFreeFunc,
                    wrappedData, num_events_in_wait_list, amdWaitList, amdEventPtr);
        }
        else
            // if events too much use vector to store original AMDOCL events
            try
            {
                std::vector<cl_event> amdWaitList(num_events_in_wait_list);
                for (cl_uint i = 0; i < num_events_in_wait_list; i++)
                {
                    if (event_wait_list[i] == nullptr)
                        return CL_INVALID_EVENT_WAIT_LIST;
                    amdWaitList[i] =
                        static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
                }
                
                status = q->amdOclCommandQueue->dispatch->clEnqueueSVMFree(
                    q->amdOclCommandQueue, num_svm_pointers, svm_pointers, clrxFreeFunc,
                    wrappedData, num_events_in_wait_list, amdWaitList.data(), amdEventPtr);
            }
            catch (const std::bad_alloc& ex)
            {
                delete wrappedData;
                return CL_OUT_OF_HOST_MEMORY;
            }
    }
    else
        status = q->amdOclCommandQueue->dispatch->clEnqueueSVMFree(
                q->amdOclCommandQueue, num_svm_pointers, svm_pointers, clrxFreeFunc,
                wrappedData, 0, nullptr, amdEventPtr);
    }
    catch (const std::bad_alloc& ex)
    {
        delete wrappedData;
        return CL_OUT_OF_HOST_MEMORY;
    }
    if (status != CL_SUCCESS)
        delete wrappedData;
    return status;
}

#undef CLRX_CLCOMMAND_PREFIX
#define CLRX_CLCOMMAND_PREFIX q->amdOclCommandQueue->dispatch->

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueSVMMemcpy(cl_command_queue  command_queue,
                   cl_bool           blocking_copy,
                   void *            dst_ptr,
                   const void *      src_ptr,
                   size_t            size,
                   cl_uint           num_events_in_wait_list,
                   const cl_event *  event_wait_list,
                   cl_event *        event) CL_API_SUFFIX__VERSION_2_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueSVMMemcpy(q->amdOclCommandQueue, \
            blocking_copy, dst_ptr, src_ptr, size
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueSVMMemFill(cl_command_queue  command_queue,
                    void *            svm_ptr,
                    const void *      pattern,
                    size_t            pattern_size,
                    size_t            size,
                    cl_uint           num_events_in_wait_list,
                    const cl_event *  event_wait_list,
                    cl_event *        event) CL_API_SUFFIX__VERSION_2_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueSVMMemFill(q->amdOclCommandQueue, \
            svm_ptr, pattern, pattern_size, size
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueSVMMap(cl_command_queue  command_queue,
                cl_bool           blocking_map,
                cl_map_flags      flags,
                void *            svm_ptr,
                size_t            size,
                cl_uint           num_events_in_wait_list,
                const cl_event *  event_wait_list,
                cl_event *        event) CL_API_SUFFIX__VERSION_2_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueSVMMap(q->amdOclCommandQueue, \
            blocking_map, flags, svm_ptr, size
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueSVMUnmap(cl_command_queue  command_queue,
                  void *            svm_ptr,
                  cl_uint           num_events_in_wait_list,
                  const cl_event *   event_wait_list,
                  cl_event *        event) CL_API_SUFFIX__VERSION_2_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueSVMUnmap(q->amdOclCommandQueue, svm_ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_sampler CL_API_CALL
clrxclCreateSamplerWithProperties(cl_context                     context,
              const cl_sampler_properties *  properties,
              cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_2_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_sampler amdSampler = c->amdOclContext->dispatch->clCreateSamplerWithProperties(
            c->amdOclContext, properties, errcode_ret);
    
    if (amdSampler == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXSampler, amdOclSampler, amdSampler,
              clReleaseSampler,
              "Fatal Error at handling error at sampler creation!")
    
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclSetKernelArgSVMPointer(cl_kernel    kernel,
                         cl_uint      arg_index,
                         const void * arg_value) CL_API_SUFFIX__VERSION_2_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    if (arg_index >= (k->argTypes.size()>>1))
        return CL_INVALID_ARG_INDEX;
    return k->amdOclKernel->dispatch->clSetKernelArgSVMPointer(k->amdOclKernel,
               arg_index, arg_value);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclSetKernelExecInfo(cl_kernel            kernel,
                    cl_kernel_exec_info  param_name,
                    size_t               param_value_size,
                    const void *         param_value) CL_API_SUFFIX__VERSION_2_0
{
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    return k->amdOclKernel->dispatch->clSetKernelExecInfo(k->amdOclKernel, param_name,
              param_value_size, param_value);
}
#endif /* CL_VERSION_2_0 */

} /* extern "C" */
