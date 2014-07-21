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
CLRX_CL_PUBLIC_SYM(clCreateEventFromGLsyncKHR)
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
CLRX_CL_PUBLIC_SYM(clCreateFromGLTexture)
CLRX_CL_PUBLIC_SYM(clEnqueueWaitSignalAMD)
CLRX_CL_PUBLIC_SYM(clEnqueueWriteSignalAMD)
CLRX_CL_PUBLIC_SYM(clEnqueueMakeBuffersResidentAMD)

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
    if (image_desc != nullptr)
    {
        imageDesc = *image_desc;
        if (imageDesc.buffer != nullptr)
        {
            if (imageDesc.image_type != CL_MEM_OBJECT_IMAGE1D_BUFFER)
            {
                if (errcode_ret != nullptr)
                    *errcode_ret = CL_INVALID_IMAGE_DESCRIPTOR;
                return nullptr;
            }
            imageDesc.buffer =
                static_cast<const CLRXMemObject*>(imageDesc.buffer)->amdOclMemObject;
        }
    }
    
    const cl_mem amdImage = c->amdOclContext->dispatch->clCreateImage(c->amdOclContext,
              flags, image_format, &imageDesc, host_ptr, errcode_ret);
    
    if (amdImage == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdImage,
              clReleaseMemObject,
              "Fatal Error at handling error at image creation!")
    
    if (image_desc != nullptr && image_desc->buffer != nullptr)
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
            {   // bad header
                std::lock_guard<std::mutex> lock(p->mutex);
                p->concurrentBuilds--; // after this building
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
            
            for (cl_uint i = 0; i < num_devices; i++)
            {
                if (device_list[i] == nullptr)
                {   // bad device
                    std::lock_guard<std::mutex> lock(p->mutex);
                    p->concurrentBuilds--; // after this building
                    delete wrappedData;
                    return CL_INVALID_DEVICE;
                }
                amdDevices[i] =
                    static_cast<const CLRXDevice*>(device_list[i])->amdOclDevice;
            }
            
            status = p->amdOclProgram->dispatch->clCompileProgram(
                    p->amdOclProgram, num_devices, amdDevices.data(),
                    options, num_input_headers, amdHeadersPtr, header_include_names,
                    notifyToCall, destUserData);
        }
    }
    catch(const std::bad_alloc& ex)
    {   // if allocation failed
        std::lock_guard<std::mutex> lock(p->mutex);
        p->concurrentBuilds--; // after this building
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
    {   // do it if callback not called
        p->concurrentBuilds--; // after this building
        if (status != CL_INVALID_DEVICE)
        {
            const cl_int newStatus = clrxUpdateProgramAssocDevices(p);
            if (newStatus != CL_SUCCESS)
                status = newStatus;
        }
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
        std::cerr << "Fatal exception happened: " << ex.what() << std::endl;
        abort();
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
        wrappedData = new CLRXLinkProgramUserData;
        wrappedData->realNotify = pfn_notify;
        wrappedData->clrxProgramFilled = false;
        wrappedData->clrxProgram = nullptr;
        wrappedData->clrxContext = c;
        wrappedData->realUserData = user_data;
        destUserData = wrappedData;
        notifyToCall = clrxLinkProgramNotifyWrapper;
    }
    
    cl_program amdProgram = nullptr;
    CLRXProgram* outProgram = nullptr;
    try
    {
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
                        outProgram = new CLRXProgram;
                        outProgram->dispatch = const_cast<CLRXIcdDispatch*>
                                    (&clrxDispatchRecord);
                        outProgram->amdOclProgram = amdProgram;
                        outProgram->context = c;
                        clrxUpdateProgramAssocDevices(outProgram);
                        clrxRetainOnlyCLRXContext(c);
                        
                        wrappedData->clrxProgramFilled = true;
                        wrappedData->clrxProgram = outProgram;
                        initializedByCLCall = true; // force skip deletion of wrappedData
                    }
                    // error occurred and no callback called, delete wrappedData
                }
                else // get from wrapped data our program
                {
                    if (amdProgram != nullptr) // only if returned not null program
                        outProgram = static_cast<CLRXProgram*>(wrappedData->clrxProgram);
                    /* otherwise we do free own CLRXProgram and delete wrappedData if
                     * initialized by callback */
                    else
                        clrxReleaseOnlyCLRXProgram(wrappedData->clrxProgram);
                }
            }
            
            if (!initializedByCLCall)
            {   /* delete if initialized by callback */
                delete wrappedData;
                wrappedData = nullptr;
            }
        }
        else if (amdProgram != nullptr)
        {
            outProgram = new CLRXProgram;
            outProgram->dispatch = const_cast<CLRXIcdDispatch*>(&clrxDispatchRecord);
            outProgram->amdOclProgram = amdProgram;
            outProgram->context = c;
            clrxUpdateProgramAssocDevices(outProgram);
            clrxRetainOnlyCLRXContext(c);
        }
    }
    catch(const std::bad_alloc& ex)
    {
        if (amdProgram != nullptr)
        {
            if (c->amdOclContext->dispatch->clReleaseProgram(amdProgram) != CL_SUCCESS)
            {
                std::cerr <<
                    "Fatal Error at handling error at program linkage!" << std::endl;
                abort();
            }
        }
        delete outProgram;
        delete wrappedData;
        if (errcode_ret != nullptr)
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
        return nullptr;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Fatal exception happened: " << ex.what() << std::endl;
        abort();
    }
    
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
    const size_t length = sizeof(p->extEntries)/sizeof(CLRXExtensionEntry);
    const CLRXExtensionEntry* entry = std::lower_bound(p->extEntries,
           p->extEntries + length,
           tmp, [](const CLRXExtensionEntry& l, const CLRXExtensionEntry& r) -> bool
           { return ::strcmp(l.funcname, r.funcname)<0; });
    
    if (entry == p->extEntries + length)
        return nullptr;
    if (::strcmp(func_name, entry->funcname)!=0)
        return nullptr;
    return entry->address;
}

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

} /* extern "C" */
