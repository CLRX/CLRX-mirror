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

#define CLRX_CLCOMMAND_PREFIX q->amdOclCommandQueue->dispatch->
    
CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(cl_uint             num_events,
                const cl_event *    event_list) CL_API_SUFFIX__VERSION_1_0
{
    if (num_events == 0 || event_list == nullptr)
        return CL_INVALID_VALUE;
    
    const CLRXEvent* e = static_cast<const CLRXEvent*>(event_list[0]);
    if (num_events <= 100)
    {   /* for static allocation */
        cl_event amdEvents[100];
        for (cl_uint i = 0; i < num_events; i++)
        {
            if (event_list[i] == nullptr)
                return CL_INVALID_EVENT;
            amdEvents[i] = static_cast<const CLRXEvent*>(event_list[i])->amdOclEvent;
        }
        return e->amdOclEvent->dispatch->clWaitForEvents(num_events, amdEvents);
    }
    try
    {
        std::vector<cl_event> amdEvents(num_events);
        for (cl_uint i = 0; i < num_events; i++)
        {
            if (event_list[i] == nullptr)
                return CL_INVALID_EVENT;
            amdEvents[i] = static_cast<const CLRXEvent*>(event_list[i])->amdOclEvent;
        }
        
        return e->amdOclEvent->dispatch->clWaitForEvents(num_events, amdEvents.data());
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(cl_event         event,
               cl_event_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    const CLRXEvent* e = static_cast<CLRXEvent*>(event);
    
    switch (param_name)
    {
        case CL_EVENT_CONTEXT:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_context))
                    return CL_INVALID_VALUE;
                *static_cast<cl_context*>(param_value) = e->context;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_context);
            break;
        case CL_EVENT_COMMAND_QUEUE:
            if (param_value != nullptr)
            {
                if (param_value_size < sizeof(cl_command_queue))
                    return CL_INVALID_VALUE;
                *static_cast<cl_command_queue*>(param_value) = e->commandQueue;
            }
            if (param_value_size_ret != nullptr)
                *param_value_size_ret = sizeof(cl_command_queue);
            break;
        default:
            return e->amdOclEvent->dispatch->clGetEventInfo(e->amdOclEvent, param_name,
                    param_value_size, param_value, param_value_size_ret);
    }
    return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    
    CLRXEvent* e = static_cast<CLRXEvent*>(event);
    const cl_int status = e->dispatch->clRetainEvent(e->amdOclEvent);
    if (status == CL_SUCCESS)
        e->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    CLRXEvent* e = static_cast<CLRXEvent*>(event);
    
    const cl_int status = e->amdOclEvent->dispatch->clReleaseEvent(e->amdOclEvent);
    if (status == CL_SUCCESS)
        if (e->refCount.fetch_sub(1) == 1)
        {
            clrxReleaseOnlyCLRXContext(e->context);
            delete e;
        }
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(cl_event            event,
                cl_profiling_info   param_name,
                size_t              param_value_size,
                void *              param_value,
                size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    
    const CLRXEvent* e = static_cast<const CLRXEvent*>(event);
    return e->amdOclEvent->dispatch->clGetEventProfilingInfo(e->amdOclEvent, param_name,
                 param_value_size, param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    return q->amdOclCommandQueue->dispatch->clFlush(q->amdOclCommandQueue);
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    return q->amdOclCommandQueue->dispatch->clFinish(q->amdOclCommandQueue);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(cl_command_queue    command_queue,
                    cl_mem              buffer,
                    cl_bool             blocking_read,
                    size_t              offset,
                    size_t              size,
                    void *              ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_int status = CL_SUCCESS;
    
#define CLRX_ORIG_CLCOMMAND clEnqueueReadBuffer(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_read, offset, size, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(cl_command_queue   command_queue,
                     cl_mem             buffer,
                     cl_bool            blocking_write,
                     size_t             offset,
                     size_t             size,
                     const void *       ptr,
                     cl_uint            num_events_in_wait_list,
                     const cl_event *   event_wait_list,
                     cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueWriteBuffer(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_write, offset, size, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(cl_command_queue    command_queue,
                    cl_mem              src_buffer,
                    cl_mem              dst_buffer,
                    size_t              src_offset,
                    size_t              dst_offset,
                    size_t              size,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (src_buffer == nullptr || dst_buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* sb = static_cast<const CLRXMemObject*>(src_buffer);
    const CLRXMemObject* db = static_cast<const CLRXMemObject*>(dst_buffer);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueCopyBuffer(q->amdOclCommandQueue, \
            sb->amdOclMemObject, db->amdOclMemObject, src_offset, dst_offset, size
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage(cl_command_queue     command_queue,
                   cl_mem               image,
                   cl_bool              blocking_read,
                   const size_t *       origin,
                   const size_t *       region,
                   size_t               row_pitch,
                   size_t               slice_pitch,
                   void *               ptr,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (image == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(image);
    
    cl_int status = CL_SUCCESS;

#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueReadImage(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_read, origin, region, \
            row_pitch, slice_pitch, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage(cl_command_queue    command_queue,
                    cl_mem              image,
                    cl_bool             blocking_write,
                    const size_t *      origin,
                    const size_t *      region,
                    size_t              input_row_pitch,
                    size_t              input_slice_pitch,
                    const void *        ptr,
                    cl_uint             num_events_in_wait_list,
                    const cl_event *    event_wait_list,
                    cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (image == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(image);
    
    cl_int status = CL_SUCCESS;

#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueWriteImage(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_write, origin, region, \
            input_row_pitch, input_slice_pitch, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage(cl_command_queue     command_queue,
                   cl_mem               src_image,
                   cl_mem               dst_image, 
                   const size_t *       src_origin,
                   const size_t *       dst_origin,
                   const size_t *       region,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (src_image == nullptr || dst_image == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* sb = static_cast<const CLRXMemObject*>(src_image);
    const CLRXMemObject* db = static_cast<const CLRXMemObject*>(dst_image);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueCopyImage(q->amdOclCommandQueue, \
            sb->amdOclMemObject, db->amdOclMemObject, src_origin, dst_origin, region
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                           cl_mem           src_image,
                           cl_mem           dst_buffer, 
                           const size_t *   src_origin,
                           const size_t *   region, 
                           size_t           dst_offset,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (src_image == nullptr || dst_buffer == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* sb = static_cast<const CLRXMemObject*>(src_image);
    const CLRXMemObject* db = static_cast<const CLRXMemObject*>(dst_buffer);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueCopyImageToBuffer(q->amdOclCommandQueue, \
            sb->amdOclMemObject, db->amdOclMemObject, src_origin, region, dst_offset
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage(cl_command_queue command_queue,
                           cl_mem           src_buffer,
                           cl_mem           dst_image,
                           size_t           src_offset,
                           const size_t *   dst_origin,
                           const size_t *   region,
                           cl_uint          num_events_in_wait_list,
                           const cl_event * event_wait_list,
                           cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (src_buffer == nullptr || dst_image == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* sb = static_cast<const CLRXMemObject*>(src_buffer);
    const CLRXMemObject* db = static_cast<const CLRXMemObject*>(dst_image);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueCopyBufferToImage(q->amdOclCommandQueue, \
            sb->amdOclMemObject, db->amdOclMemObject, src_offset, dst_origin, region
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(cl_command_queue command_queue,
                   cl_mem           buffer,
                   cl_bool          blocking_map,
                   cl_map_flags     map_flags,
                   size_t           offset,
                   size_t           size,
                   cl_uint          num_events_in_wait_list,
                   const cl_event * event_wait_list,
                   cl_event *       event,
                   cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_COMMAND_QUEUE;
        return nullptr;
    }
    if (buffer == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_MEM_OBJECT;
        return nullptr;
    }
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
        return nullptr;
    }
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_event amdEvent = nullptr;
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr;
    void* output = nullptr;
    if (event_wait_list != nullptr)
    {
        if (num_events_in_wait_list <= 100)
        {
            cl_event amdWaitList[100];
            for (cl_uint i = 0; i < num_events_in_wait_list; i++)
            {
                if (event_wait_list[i] == nullptr)
                {
                    if (errcode_ret != nullptr)
                        *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
                    return nullptr;
                }
                amdWaitList[i] =
                    static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
            }
            
            output = q->amdOclCommandQueue->dispatch->clEnqueueMapBuffer(
                q->amdOclCommandQueue, b->amdOclMemObject, blocking_map, map_flags,
                offset, size, num_events_in_wait_list, amdWaitList, amdEventPtr,
                errcode_ret);
        }
        else
            try
            {
                std::vector<cl_event> amdWaitList(num_events_in_wait_list);
                for (cl_uint i = 0; i < num_events_in_wait_list; i++)
                {
                    if (event_wait_list[i] == nullptr)
                    {
                        if (errcode_ret != nullptr)
                            *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
                        return nullptr;
                    }
                    amdWaitList[i] =
                        static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
                }
                
                output = q->amdOclCommandQueue->dispatch->clEnqueueMapBuffer(
                    q->amdOclCommandQueue, b->amdOclMemObject, blocking_map, map_flags,
                    offset, size, num_events_in_wait_list, amdWaitList.data(), amdEventPtr,
                    errcode_ret);
            }
            catch (const std::bad_alloc& ex)
            { 
                if (errcode_ret != nullptr)
                    *errcode_ret = CL_OUT_OF_HOST_MEMORY;
                return nullptr;
            }
    }
    else
        output = q->amdOclCommandQueue->dispatch->clEnqueueMapBuffer(
                q->amdOclCommandQueue, b->amdOclMemObject, blocking_map, map_flags,
                offset, size, 0, nullptr, amdEventPtr, errcode_ret);
    
    cl_int status = CL_SUCCESS;
    if (errcode_ret != nullptr)
        status = *errcode_ret;
    
    status = clrxApplyCLRXEvent(q, event, amdEvent, status);
    if (errcode_ret != nullptr)
        *errcode_ret = status;
    return output;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapImage(cl_command_queue  command_queue,
                  cl_mem            image,
                  cl_bool           blocking_map,
                  cl_map_flags      map_flags,
                  const size_t *    origin,
                  const size_t *    region,
                  size_t *          image_row_pitch,
                  size_t *          image_slice_pitch,
                  cl_uint           num_events_in_wait_list,
                  const cl_event *  event_wait_list,
                  cl_event *        event,
                  cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_COMMAND_QUEUE;
        return nullptr;
    }
    if (image == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_MEM_OBJECT;
        return nullptr;
    }
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
        return nullptr;
    }
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(image);
    
    cl_event amdEvent = nullptr;
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr;
    void* output = nullptr;
    if (event_wait_list != nullptr)
    {
        if (num_events_in_wait_list <= 100)
        {
            cl_event amdWaitList[100];
            for (cl_uint i = 0; i < num_events_in_wait_list; i++)
            {
                if (event_wait_list[i] == nullptr)
                {
                    if (errcode_ret != nullptr)
                        *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
                    return nullptr;
                }
                amdWaitList[i] =
                    static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
            }
            
            output = q->amdOclCommandQueue->dispatch->clEnqueueMapImage(
                q->amdOclCommandQueue, b->amdOclMemObject, blocking_map, map_flags,
                origin, region, image_row_pitch, image_slice_pitch,
                num_events_in_wait_list, amdWaitList, amdEventPtr, errcode_ret);
        }
        else
            try
            {
                std::vector<cl_event> amdWaitList(num_events_in_wait_list);
                for (cl_uint i = 0; i < num_events_in_wait_list; i++)
                {
                    if (event_wait_list[i] == nullptr)
                    {
                        if (errcode_ret != nullptr)
                            *errcode_ret = CL_INVALID_EVENT_WAIT_LIST;
                        return nullptr;
                    }
                    amdWaitList[i] =
                        static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
                }
                
                output = q->amdOclCommandQueue->dispatch->clEnqueueMapImage(
                    q->amdOclCommandQueue, b->amdOclMemObject, blocking_map, map_flags,
                    origin, region, image_row_pitch, image_slice_pitch,
                    num_events_in_wait_list, amdWaitList.data(), amdEventPtr, errcode_ret);
            }
            catch (const std::bad_alloc& ex)
            { 
                if (errcode_ret != nullptr)
                    *errcode_ret = CL_OUT_OF_HOST_MEMORY;
                return nullptr;
            }
    }
    else
        output = q->amdOclCommandQueue->dispatch->clEnqueueMapImage(
                q->amdOclCommandQueue, b->amdOclMemObject, blocking_map, map_flags,
                origin, region, image_row_pitch, image_slice_pitch,
                0, nullptr, amdEventPtr, errcode_ret);
    
    cl_int status = CL_SUCCESS;
    if (errcode_ret != nullptr)
        status = *errcode_ret;
    
    status = clrxApplyCLRXEvent(q, event, amdEvent, status);
    if (errcode_ret != nullptr)
        *errcode_ret = status;
    return output;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(cl_command_queue command_queue,
                        cl_mem           memobj,
                        void *           mapped_ptr,
                        cl_uint          num_events_in_wait_list,
                        const cl_event *  event_wait_list,
                        cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (memobj == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(memobj);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueUnmapMemObject(q->amdOclCommandQueue, \
            m->amdOclMemObject, mapped_ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(cl_command_queue command_queue,
                       cl_kernel        kernel,
                       cl_uint          work_dim,
                       const size_t *   global_work_offset,
                       const size_t *   global_work_size,
                       const size_t *   local_work_size,
                       cl_uint          num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event *       event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueNDRangeKernel(q->amdOclCommandQueue, \
            k->amdOclKernel, work_dim, global_work_offset, \
            global_work_size, local_work_size
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(cl_command_queue  command_queue,
              cl_kernel         kernel,
              cl_uint           num_events_in_wait_list,
              const cl_event *  event_wait_list,
              cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    if (kernel == nullptr)
        return CL_INVALID_KERNEL;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueTask(q->amdOclCommandQueue, \
            k->amdOclKernel
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(cl_command_queue  command_queue,
                      void (CL_CALLBACK * user_func)(void *), 
                      void *            args,
                      size_t            cb_args, 
                      cl_uint           num_mem_objects,
                      const cl_mem *    mem_list,
                      const void **     args_mem_loc,
                      cl_uint           num_events_in_wait_list,
                      const cl_event *  event_wait_list,
                      cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if (user_func == nullptr || (args == nullptr && cb_args != 0) ||
        (args == nullptr && num_mem_objects != 0) || (args != nullptr && cb_args == 0))
        return CL_INVALID_VALUE;
    
    if ((num_mem_objects != 0 && (mem_list == nullptr || args_mem_loc == nullptr)) ||
        (num_mem_objects == 0 && (mem_list != nullptr || args_mem_loc != nullptr)))
        return CL_INVALID_VALUE;
    
    void* amdArgs = args; // copy for original call
    if (num_mem_objects != 0)
    {   // create copy AMD callback
        amdArgs = malloc(cb_args);
        if (amdArgs == nullptr)
            return CL_OUT_OF_HOST_MEMORY;
        memcpy(amdArgs, args, cb_args);
        /* copy our memobject to new args */
        for (cl_uint i = 0; i < num_mem_objects; i++)
        {
            if (mem_list[i] == nullptr)
                return CL_INVALID_VALUE;
            *(cl_mem*)((char*)amdArgs + ((ptrdiff_t)args_mem_loc-(ptrdiff_t)args)) = 
                    mem_list[i];
        }
    }
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    // real call
    cl_int status = CL_SUCCESS;
    cl_event amdEvent = nullptr;
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr;
    if (event_wait_list != nullptr)
    {
        if (num_events_in_wait_list <= 100)
        {
            cl_event amdWaitList[100];
            for (cl_uint i = 0; i < num_events_in_wait_list; i++)
            {
                if (event_wait_list[i] == nullptr)
                    return CL_INVALID_EVENT_WAIT_LIST;
                amdWaitList[i] =
                    static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
            }
            
            status = q->amdOclCommandQueue->dispatch->clEnqueueNativeKernel(
                    q->amdOclCommandQueue, user_func, amdArgs, cb_args,
                    num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list,
                    amdWaitList, amdEventPtr);
        }
        else
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
                
                status = q->amdOclCommandQueue->dispatch->clEnqueueNativeKernel(
                        q->amdOclCommandQueue, user_func, amdArgs, cb_args,
                        num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list,
                        amdWaitList.data(), amdEventPtr);
            }
            catch (const std::bad_alloc& ex)
            {
                if (num_mem_objects != 0)
                    free(amdArgs); // free amd args
                return CL_OUT_OF_HOST_MEMORY;
            }
    }
    else
        status = q->amdOclCommandQueue->dispatch->clEnqueueNativeKernel(
                    q->amdOclCommandQueue, user_func, amdArgs, cb_args,
                    num_mem_objects, mem_list, args_mem_loc, 0, nullptr, amdEventPtr);
    
    if (num_mem_objects != 0)
        free(amdArgs);
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clEnqueueMarker(cl_command_queue    command_queue,
                cl_event *          event) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if (event == nullptr)
        return CL_INVALID_VALUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);

    cl_event amdEvent = nullptr;
    const cl_int status = q->amdOclCommandQueue->dispatch->clEnqueueMarker(
            q->amdOclCommandQueue, &amdEvent);
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clEnqueueWaitForEvents(cl_command_queue command_queue,
            cl_uint          num_events,
            const cl_event * event_list) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if (num_events == 0 || event_list == nullptr)
        return CL_INVALID_VALUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    if (num_events <= 100)
    {   /* for static allocation */
        cl_event amdEvents[100];
        for (cl_uint i = 0; i < num_events; i++)
        {
            if (event_list[i] == nullptr)
                return CL_INVALID_EVENT;
            amdEvents[i] = static_cast<const CLRXEvent*>(event_list[i])->amdOclEvent;
        }
        return q->amdOclCommandQueue->dispatch->clEnqueueWaitForEvents(
                q->amdOclCommandQueue, num_events, amdEvents);
    }
    try
    {
        std::vector<cl_event> amdEvents(num_events);
        for (cl_uint i = 0; i < num_events; i++)
        {
            if (event_list[i] == nullptr)
                return CL_INVALID_EVENT;
            amdEvents[i] = static_cast<const CLRXEvent*>(event_list[i])->amdOclEvent;
        }
        
        return q->amdOclCommandQueue->dispatch->clEnqueueWaitForEvents(
                    q->amdOclCommandQueue, num_events, amdEvents.data());
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clEnqueueBarrier(cl_command_queue command_queue) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    return q->amdOclCommandQueue->dispatch->clEnqueueBarrier(q->amdOclCommandQueue);
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED void * CL_API_CALL
clGetExtensionFunctionAddress(const char * func_name) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    CLRX_INITIALIZE_VOIDPTR
    
    const CLRXExtensionEntry tmp{func_name};
    const size_t length = sizeof(clrxExtensionsTable)/sizeof(CLRXExtensionEntry);
    const CLRXExtensionEntry* entry = std::lower_bound(clrxExtensionsTable,
           clrxExtensionsTable + length,
           tmp, [](const CLRXExtensionEntry& l, const CLRXExtensionEntry& r) -> bool
           { return strcmp(l.funcname, r.funcname)<0; });
    
    if (entry == clrxExtensionsTable + length)
        return nullptr;
    if (strcmp(func_name, entry->funcname)!=0)
        return nullptr;
    return entry->address;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLBuffer(cl_context     context,
                     cl_mem_flags   flags,
                     cl_GLuint      bufobj,
                     int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdBuffer = c->amdOclContext->dispatch->clCreateFromGLBuffer(
                c->amdOclContext, flags, bufobj, errcode_ret);
    
    if (amdBuffer == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdBuffer,
              clReleaseMemObject,
              "Fatal Error at handling error at buffer from GL creation!")
    
    return outObject;
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_mem CL_API_CALL
clCreateFromGLTexture2D(cl_context      context,
                cl_mem_flags    flags,
                cl_GLenum       target,
                cl_GLint        miplevel,
                cl_GLuint       texture,
                cl_int *        errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdBuffer = c->amdOclContext->dispatch->clCreateFromGLTexture2D(
                c->amdOclContext, flags, target, miplevel, texture, errcode_ret);
    
    if (amdBuffer == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdBuffer,
              clReleaseMemObject,
              "Fatal Error at handling error at fromGLTexture2D creation!")
    
    return outObject;
}
    
CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_mem CL_API_CALL
clCreateFromGLTexture3D(cl_context      context,
                cl_mem_flags    flags,
                cl_GLenum       target,
                cl_GLint        miplevel,
                cl_GLuint       texture,
                cl_int *        errcode_ret) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdBuffer = c->amdOclContext->dispatch->clCreateFromGLTexture3D(
                c->amdOclContext, flags, target, miplevel, texture, errcode_ret);
    
    if (amdBuffer == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdBuffer,
              clReleaseMemObject,
              "Fatal Error at handling error at fromGLTexture3D creation!")
    
    return outObject;
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateFromGLRenderbuffer(cl_context   context,
                           cl_mem_flags flags,
                           cl_GLuint    renderbuffer,
                           cl_int *     errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (context == nullptr)
    {
        if (errcode_ret != nullptr)
            *errcode_ret = CL_INVALID_CONTEXT;
        return nullptr;
    }
    
    CLRXContext* c = static_cast<CLRXContext*>(context);
    const cl_mem amdBuffer = c->amdOclContext->dispatch->clCreateFromGLRenderbuffer(
                c->amdOclContext, flags, renderbuffer, errcode_ret);
    
    if (amdBuffer == nullptr)
        return nullptr;
    
    CREATE_CLRXCONTEXT_OBJECT(CLRXMemObject, amdOclMemObject, amdBuffer,
              clReleaseMemObject,
              "Fatal Error at handling error at fromGLRenderbuffer creation!")
    
    return outObject;
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLObjectInfo(cl_mem                memobj,
                  cl_gl_object_type *   gl_object_type,
                  cl_GLuint *           gl_object_name) CL_API_SUFFIX__VERSION_1_0
{
    if (memobj == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(memobj);
    return m->amdOclMemObject->dispatch->clGetGLObjectInfo(m->amdOclMemObject,
                           gl_object_type, gl_object_name);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLTextureInfo(cl_mem               memobj,
               cl_gl_texture_info   param_name,
               size_t               param_value_size,
               void *               param_value,
               size_t *             param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (memobj == nullptr)
        return CL_INVALID_MEM_OBJECT;
    
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(memobj);
    return m->amdOclMemObject->dispatch->clGetGLTextureInfo(m->amdOclMemObject,
            param_name, param_value_size, param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAcquireGLObjects(cl_command_queue      command_queue,
                          cl_uint               num_objects,
                          const cl_mem *        mem_objects,
                          cl_uint               num_events_in_wait_list,
                          const cl_event *      event_wait_list,
                          cl_event *            event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    if ((num_objects == 0 && mem_objects != nullptr) ||
        (num_objects != 0 && mem_objects == nullptr))
        return CL_INVALID_VALUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    
    try
    {
        std::vector<cl_mem> amdMemObjects(num_objects);
        for (cl_uint i = 0; i < num_objects; i++)
        {
            if (mem_objects[i] == nullptr)
                return CL_INVALID_MEM_OBJECT;
            amdMemObjects[i] = static_cast<CLRXMemObject*>(mem_objects[i])->amdOclMemObject;
        }
        
        cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueAcquireGLObjects(q->amdOclCommandQueue, \
                num_objects, amdMemObjects.data()
        CLRX_CALL_QUEUE_COMMAND
        
        return clrxApplyCLRXEvent(q, event, amdEvent, status);
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReleaseGLObjects(cl_command_queue      command_queue,
                          cl_uint               num_objects,
                          const cl_mem *        mem_objects,
                          cl_uint               num_events_in_wait_list,
                          const cl_event *      event_wait_list,
                          cl_event *            event) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if ((num_events_in_wait_list == 0 && event_wait_list != nullptr) ||
        (num_events_in_wait_list != 0 && event_wait_list == nullptr))
        return CL_INVALID_EVENT_WAIT_LIST;
    
    if ((num_objects == 0 && mem_objects != nullptr) ||
        (num_objects != 0 && mem_objects == nullptr))
        return CL_INVALID_VALUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    
    try
    {
        std::vector<cl_mem> amdMemObjects(num_objects);
        for (cl_uint i = 0; i < num_objects; i++)
        {
            if (mem_objects[i] == nullptr)
                return CL_INVALID_MEM_OBJECT;
            amdMemObjects[i] = static_cast<CLRXMemObject*>(mem_objects[i])->amdOclMemObject;
        }
        
        cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueReleaseGLObjects(q->amdOclCommandQueue, \
                num_objects, amdMemObjects.data()
        CLRX_CALL_QUEUE_COMMAND
        
        return clrxApplyCLRXEvent(q, event, amdEvent, status);
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
}

CL_API_ENTRY cl_int CL_API_CALL
clGetGLContextInfoKHR(const cl_context_properties * properties,
      cl_gl_context_info            param_name,
      size_t                        param_value_size,
      void *                        param_value,
      size_t *                      param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    CLRX_INITIALIZE
    
    CLRXPlatform* platform = clrxPlatforms; // choose first AMD platform
    bool doTranslateProps = false;
    size_t propNums = 0;
    
    cl_int status = CL_SUCCESS;
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
        
        size_t myParamValueSize = 0;
        status = platform->amdOclPlatform->dispatch->clGetGLContextInfoKHR(amdPropsPtr,
                   param_name, param_value_size, param_value, &myParamValueSize);
        
        if (status != CL_SUCCESS)
            return status;
        
        /* convert from original devices into own devices */
        if (param_value != nullptr &&
            (param_name == CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR ||
            param_name == CL_DEVICES_FOR_GL_CONTEXT_KHR))
        {
            std::vector<cl_device_id> platformDevices(platform->devicesNum);
            for (cl_uint i = 0; i < platform->devicesNum; i++)
                platformDevices[i] = platform->devices + i;
            
            translateAMDDevicesIntoCLRXDevices(platform->devicesNum,
                   (CLRXDevice**)(platformDevices.data()),
                   myParamValueSize/sizeof(cl_device_id), (cl_device_id*)param_value);
        }
        if (param_value_size_ret != nullptr)
            *param_value_size_ret = myParamValueSize;
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    
    return CL_SUCCESS;
}

} /* extern "C" */
