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
#    pragma comment(linker,"/export:clWaitForEvents=_clrxclWaitForEvents@8")
#    pragma comment(linker,"/export:clGetEventInfo=_clrxclGetEventInfo@20")
#    pragma comment(linker,"/export:clRetainEvent=_clrxclRetainEvent@4")
#    pragma comment(linker,"/export:clReleaseEvent=_clrxclReleaseEvent@4")
#    pragma comment(linker,"/export:clGetEventProfilingInfo=_clrxclGetEventProfilingInfo@20")
#    pragma comment(linker,"/export:clFlush=_clrxclFlush@4")
#    pragma comment(linker,"/export:clFinish=_clrxclFinish@4")
#    pragma comment(linker,"/export:clEnqueueReadBuffer=_clrxclEnqueueReadBuffer@36")
#    pragma comment(linker,"/export:clEnqueueWriteBuffer=_clrxclEnqueueWriteBuffer@36")
#    pragma comment(linker,"/export:clEnqueueCopyBuffer=_clrxclEnqueueCopyBuffer@36")
#    pragma comment(linker,"/export:clEnqueueReadImage=_clrxclEnqueueReadImage@44")
#    pragma comment(linker,"/export:clEnqueueWriteImage=_clrxclEnqueueWriteImage@44")
#    pragma comment(linker,"/export:clEnqueueCopyImage=_clrxclEnqueueCopyImage@36")
#    pragma comment(linker,"/export:clEnqueueCopyImageToBuffer=_clrxclEnqueueCopyImageToBuffer@36")
#    pragma comment(linker,"/export:clEnqueueCopyBufferToImage=_clrxclEnqueueCopyBufferToImage@36")
#    pragma comment(linker,"/export:clEnqueueMapBuffer=_clrxclEnqueueMapBuffer@44")
#    pragma comment(linker,"/export:clEnqueueMapImage=_clrxclEnqueueMapImage@52")
#    pragma comment(linker,"/export:clEnqueueUnmapMemObject=_clrxclEnqueueUnmapMemObject@24")
#    pragma comment(linker,"/export:clEnqueueNDRangeKernel=_clrxclEnqueueNDRangeKernel@36")
#    pragma comment(linker,"/export:clEnqueueTask=_clrxclEnqueueTask@20")
#    pragma comment(linker,"/export:clEnqueueNativeKernel=_clrxclEnqueueNativeKernel@40")
#    pragma comment(linker,"/export:clEnqueueMarker=_clrxclEnqueueMarker@8")
#    pragma comment(linker,"/export:clEnqueueWaitForEvents=_clrxclEnqueueWaitForEvents@12")
#    pragma comment(linker,"/export:clEnqueueBarrier=_clrxclEnqueueBarrier@4")
#    pragma comment(linker,"/export:clGetExtensionFunctionAddress=_clrxclGetExtensionFunctionAddress@4")
#    ifdef HAVE_OPENGL
#    pragma comment(linker,"/export:clCreateFromGLBuffer=_clrxclCreateFromGLBuffer@20")
#    pragma comment(linker,"/export:clCreateFromGLTexture2D=_clrxclCreateFromGLTexture2D@28")
#    pragma comment(linker,"/export:clCreateFromGLTexture3D=_clrxclCreateFromGLTexture3D@28")
#    pragma comment(linker,"/export:clCreateFromGLRenderbuffer=_clrxclCreateFromGLRenderbuffer@20")
#    pragma comment(linker,"/export:clGetGLObjectInfo=_clrxclGetGLObjectInfo@12")
#    pragma comment(linker,"/export:clGetGLTextureInfo=_clrxclGetGLTextureInfo@20")
#    pragma comment(linker,"/export:clEnqueueAcquireGLObjects=_clrxclEnqueueAcquireGLObjects@24")
#    pragma comment(linker,"/export:clEnqueueReleaseGLObjects=_clrxclEnqueueReleaseGLObjects@24")
#    pragma comment(linker,"/export:clGetGLContextInfoKHR=_clrxclGetGLContextInfoKHR@20")
#    endif
#  else
#    pragma comment(linker,"/export:clWaitForEvents=clrxclWaitForEvents")
#    pragma comment(linker,"/export:clGetEventInfo=clrxclGetEventInfo")
#    pragma comment(linker,"/export:clRetainEvent=clrxclRetainEvent")
#    pragma comment(linker,"/export:clReleaseEvent=clrxclReleaseEvent")
#    pragma comment(linker,"/export:clGetEventProfilingInfo=clrxclGetEventProfilingInfo")
#    pragma comment(linker,"/export:clFlush=clrxclFlush")
#    pragma comment(linker,"/export:clFinish=clrxclFinish")
#    pragma comment(linker,"/export:clEnqueueReadBuffer=clrxclEnqueueReadBuffer")
#    pragma comment(linker,"/export:clEnqueueWriteBuffer=clrxclEnqueueWriteBuffer")
#    pragma comment(linker,"/export:clEnqueueCopyBuffer=clrxclEnqueueCopyBuffer")
#    pragma comment(linker,"/export:clEnqueueReadImage=clrxclEnqueueReadImage")
#    pragma comment(linker,"/export:clEnqueueWriteImage=clrxclEnqueueWriteImage")
#    pragma comment(linker,"/export:clEnqueueCopyImage=clrxclEnqueueCopyImage")
#    pragma comment(linker,"/export:clEnqueueCopyImageToBuffer=clrxclEnqueueCopyImageToBuffer")
#    pragma comment(linker,"/export:clEnqueueCopyBufferToImage=clrxclEnqueueCopyBufferToImage")
#    pragma comment(linker,"/export:clEnqueueMapBuffer=clrxclEnqueueMapBuffer")
#    pragma comment(linker,"/export:clEnqueueMapImage=clrxclEnqueueMapImage")
#    pragma comment(linker,"/export:clEnqueueUnmapMemObject=clrxclEnqueueUnmapMemObject")
#    pragma comment(linker,"/export:clEnqueueNDRangeKernel=clrxclEnqueueNDRangeKernel")
#    pragma comment(linker,"/export:clEnqueueTask=clrxclEnqueueTask")
#    pragma comment(linker,"/export:clEnqueueNativeKernel=clrxclEnqueueNativeKernel")
#    pragma comment(linker,"/export:clEnqueueMarker=clrxclEnqueueMarker")
#    pragma comment(linker,"/export:clEnqueueWaitForEvents=clrxclEnqueueWaitForEvents")
#    pragma comment(linker,"/export:clEnqueueBarrier=clrxclEnqueueBarrier")
#    pragma comment(linker,"/export:clGetExtensionFunctionAddress=clrxclGetExtensionFunctionAddress")
#    ifdef HAVE_OPENGL
#    pragma comment(linker,"/export:clCreateFromGLBuffer=clrxclCreateFromGLBuffer")
#    pragma comment(linker,"/export:clCreateFromGLTexture2D=clrxclCreateFromGLTexture2D")
#    pragma comment(linker,"/export:clCreateFromGLTexture3D=clrxclCreateFromGLTexture3D")
#    pragma comment(linker,"/export:clCreateFromGLRenderbuffer=clrxclCreateFromGLRenderbuffer")
#    pragma comment(linker,"/export:clGetGLObjectInfo=clrxclGetGLObjectInfo")
#    pragma comment(linker,"/export:clGetGLTextureInfo=clrxclGetGLTextureInfo")
#    pragma comment(linker,"/export:clEnqueueAcquireGLObjects=clrxclEnqueueAcquireGLObjects")
#    pragma comment(linker,"/export:clEnqueueReleaseGLObjects=clrxclEnqueueReleaseGLObjects")
#    pragma comment(linker,"/export:clGetGLContextInfoKHR=clrxclGetGLContextInfoKHR")
#    endif
#  endif
#else
CLRX_CL_PUBLIC_SYM(clWaitForEvents)
CLRX_CL_PUBLIC_SYM(clGetEventInfo)
CLRX_CL_PUBLIC_SYM(clRetainEvent)
CLRX_CL_PUBLIC_SYM(clReleaseEvent)
CLRX_CL_PUBLIC_SYM(clGetEventProfilingInfo)
CLRX_CL_PUBLIC_SYM(clFlush)
CLRX_CL_PUBLIC_SYM(clFinish)
CLRX_CL_PUBLIC_SYM(clEnqueueReadBuffer)
CLRX_CL_PUBLIC_SYM(clEnqueueWriteBuffer)
CLRX_CL_PUBLIC_SYM(clEnqueueCopyBuffer)
CLRX_CL_PUBLIC_SYM(clEnqueueReadImage)
CLRX_CL_PUBLIC_SYM(clEnqueueWriteImage)
CLRX_CL_PUBLIC_SYM(clEnqueueCopyImage)
CLRX_CL_PUBLIC_SYM(clEnqueueCopyImageToBuffer)
CLRX_CL_PUBLIC_SYM(clEnqueueCopyBufferToImage)
CLRX_CL_PUBLIC_SYM(clEnqueueMapBuffer)
CLRX_CL_PUBLIC_SYM(clEnqueueMapImage)
CLRX_CL_PUBLIC_SYM(clEnqueueUnmapMemObject)
CLRX_CL_PUBLIC_SYM(clEnqueueNDRangeKernel)
CLRX_CL_PUBLIC_SYM(clEnqueueTask)
CLRX_CL_PUBLIC_SYM(clEnqueueNativeKernel)
CLRX_CL_PUBLIC_SYM(clEnqueueMarker)
CLRX_CL_PUBLIC_SYM(clEnqueueWaitForEvents)
CLRX_CL_PUBLIC_SYM(clEnqueueBarrier)
CLRX_CL_PUBLIC_SYM(clGetExtensionFunctionAddress)
#  ifdef HAVE_OPENGL
CLRX_CL_PUBLIC_SYM(clCreateFromGLBuffer)
CLRX_CL_PUBLIC_SYM(clCreateFromGLTexture2D)
CLRX_CL_PUBLIC_SYM(clCreateFromGLTexture3D)
CLRX_CL_PUBLIC_SYM(clCreateFromGLRenderbuffer)
CLRX_CL_PUBLIC_SYM(clGetGLObjectInfo)
CLRX_CL_PUBLIC_SYM(clGetGLTextureInfo)
CLRX_CL_PUBLIC_SYM(clEnqueueAcquireGLObjects)
CLRX_CL_PUBLIC_SYM(clEnqueueReleaseGLObjects)
CLRX_CL_PUBLIC_SYM(clGetGLContextInfoKHR)
#  endif
#endif

/* end of public API definitions */

#define CLRX_CLCOMMAND_PREFIX q->amdOclCommandQueue->dispatch->

CL_API_ENTRY cl_int CL_API_CALL
clrxclWaitForEvents(cl_uint             num_events,
                const cl_event *    event_list) CL_API_SUFFIX__VERSION_1_0
{
    if (num_events == 0 || event_list == nullptr)
        return CL_INVALID_VALUE;
    
    const CLRXEvent* e = static_cast<const CLRXEvent*>(event_list[0]);
    if (num_events <= maxLocalEventsNum)
    {
        /* for static allocation */
        cl_event amdEvents[maxLocalEventsNum];
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
clrxclGetEventInfo(cl_event         event,
               cl_event_info    param_name,
               size_t           param_value_size,
               void *           param_value,
               size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    const CLRXEvent* e = static_cast<const CLRXEvent*>(event);
    
    switch (param_name)
    {
        case CL_EVENT_CONTEXT:
            if (e->context->openCLVersionNum >= getOpenCLVersionNum(1, 1))
            {
                if (param_value != nullptr)
                {
                    if (param_value_size < sizeof(cl_context))
                        return CL_INVALID_VALUE;
                    *static_cast<cl_context*>(param_value) = e->context;
                }
                if (param_value_size_ret != nullptr)
                    *param_value_size_ret = sizeof(cl_context);
            }
            else // if OpenCL 1.0
                return CL_INVALID_VALUE;
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
clrxclRetainEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    
    CLRXEvent* e = static_cast<CLRXEvent*>(event);
    const cl_int status = e->amdOclEvent->dispatch->clRetainEvent(e->amdOclEvent);
    if (status == CL_SUCCESS)
        e->refCount.fetch_add(1);
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclReleaseEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0
{
    if (event == nullptr)
        return CL_INVALID_EVENT;
    CLRXEvent* e = static_cast<CLRXEvent*>(event);
    
    const cl_int status = e->amdOclEvent->dispatch->clReleaseEvent(e->amdOclEvent);
    if (status == CL_SUCCESS)
        if (e->refCount.fetch_sub(1) == 1)
        {
            if (e->commandQueue != nullptr)
                clrxReleaseOnlyCLRXCommandQueue(e->commandQueue);
            clrxReleaseOnlyCLRXContext(e->context);
            delete e;
        }
    return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclGetEventProfilingInfo(cl_event            event,
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
clrxclFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    return q->amdOclCommandQueue->dispatch->clFlush(q->amdOclCommandQueue);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    return q->amdOclCommandQueue->dispatch->clFinish(q->amdOclCommandQueue);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueReadBuffer(cl_command_queue    command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_int status = CL_SUCCESS;
    
#define CLRX_ORIG_CLCOMMAND clEnqueueReadBuffer(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_read, offset, size, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueWriteBuffer(cl_command_queue   command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueWriteBuffer(q->amdOclCommandQueue, \
            b->amdOclMemObject, blocking_write, offset, size, ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueCopyBuffer(cl_command_queue    command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
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
clrxclEnqueueReadImage(cl_command_queue     command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
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
clrxclEnqueueWriteImage(cl_command_queue    command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
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
clrxclEnqueueCopyImage(cl_command_queue     command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
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
clrxclEnqueueCopyImageToBuffer(cl_command_queue command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
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
clrxclEnqueueCopyBufferToImage(cl_command_queue command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
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
clrxclEnqueueMapBuffer(cl_command_queue command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(buffer);
    
    cl_event amdEvent = nullptr;
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr;
    void* output = nullptr;
    
    std::unique_ptr<CLRXEvent> outEvent;
    if (event != nullptr)
        try // allocate our event object
        {
            outEvent.reset(new CLRXEvent);
            outEvent->dispatch = q->dispatch;
            outEvent->commandQueue = q;
            outEvent->context = q->context;
        }
        catch(const std::bad_alloc& ex)
        {
            if (errcode_ret != nullptr)
                *errcode_ret = CL_OUT_OF_HOST_MEMORY;
            return nullptr;
        }
    
    if (event_wait_list != nullptr)
    {
        if (num_events_in_wait_list <= maxLocalEventsNum)
        {
            // holds original events in array if number of events is small
            cl_event amdWaitList[maxLocalEventsNum];
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
            // get original events and store them into vector
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
    
    if (amdEvent != nullptr)
    {
        // if amd event has been set
        outEvent->amdOclEvent = amdEvent;
        *event = outEvent.release();
        clrxRetainOnlyCLRXContext(q->context);
        clrxRetainOnlyCLRXCommandQueue(q);
    }
    return output;
}

CL_API_ENTRY void * CL_API_CALL
clrxclEnqueueMapImage(cl_command_queue  command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* b = static_cast<const CLRXMemObject*>(image);
    
    cl_event amdEvent = nullptr;
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr;
    void* output = nullptr;
    
    std::unique_ptr<CLRXEvent> outEvent;
    if (event != nullptr)
        try // allocate our event object
        {
            outEvent.reset(new CLRXEvent);
            outEvent->dispatch = q->dispatch;
            outEvent->commandQueue = q;
            outEvent->context = q->context;
        }
        catch(const std::bad_alloc& ex)
        {
            if (errcode_ret != nullptr)
                *errcode_ret = CL_OUT_OF_HOST_MEMORY;
            return nullptr;
        }
    
    if (event_wait_list != nullptr)
    {
        if (num_events_in_wait_list <= maxLocalEventsNum)
        {
            // holds original events in array if number of events is small
            cl_event amdWaitList[maxLocalEventsNum];
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
            // get original events and store them into vector
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
    
    if (amdEvent != nullptr)
    {
        // if amd event has been set
        outEvent->amdOclEvent = amdEvent;
        *event = outEvent.release();
        clrxRetainOnlyCLRXContext(q->context);
        clrxRetainOnlyCLRXCommandQueue(q);
    }
    return output;
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueUnmapMemObject(cl_command_queue command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXMemObject* m = static_cast<const CLRXMemObject*>(memobj);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueUnmapMemObject(q->amdOclCommandQueue, \
            m->amdOclMemObject, mapped_ptr
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueNDRangeKernel(cl_command_queue command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
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
clrxclEnqueueTask(cl_command_queue  command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    const CLRXKernel* k = static_cast<const CLRXKernel*>(kernel);
    
    cl_int status = CL_SUCCESS;
#undef CLRX_ORIG_CLCOMMAND
#define CLRX_ORIG_CLCOMMAND clEnqueueTask(q->amdOclCommandQueue, \
            k->amdOclKernel
    CLRX_CALL_QUEUE_COMMAND
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY cl_int CL_API_CALL
clrxclEnqueueNativeKernel(cl_command_queue  command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    // real call
    cl_int status = CL_SUCCESS;
    cl_event amdEvent = nullptr;
    cl_event* amdEventPtr = (event != nullptr) ? &amdEvent : nullptr;
    
    try
    {
        std::vector<cl_mem> amdMemList(num_mem_objects);
        /* get AMD mem object from memList */
        for (cl_uint i = 0; i < num_mem_objects; i++)
        {
            if (mem_list[i] == nullptr)
                return CL_INVALID_MEM_OBJECT;
            amdMemList[i] =
                static_cast<const CLRXMemObject*>(mem_list[i])->amdOclMemObject;
        }
        
        const cl_mem* amdMemListPtr = (num_mem_objects!=0) ? amdMemList.data() : nullptr;
        
        if (event_wait_list != nullptr)
        {
            if (num_events_in_wait_list <= maxLocalEventsNum)
            {
                // get original events and store into array
                cl_event amdWaitList[maxLocalEventsNum];
                for (cl_uint i = 0; i < num_events_in_wait_list; i++)
                {
                    if (event_wait_list[i] == nullptr)
                        return CL_INVALID_EVENT_WAIT_LIST;
                    amdWaitList[i] =
                        static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
                }
                
                status = q->amdOclCommandQueue->dispatch->clEnqueueNativeKernel(
                        q->amdOclCommandQueue, user_func, args, cb_args,
                        num_mem_objects, amdMemListPtr, args_mem_loc,
                        num_events_in_wait_list, amdWaitList, amdEventPtr);
            }
            else
            {
                // get original events and store them into vector
                std::vector<cl_event> amdWaitList(num_events_in_wait_list);
                for (cl_uint i = 0; i < num_events_in_wait_list; i++)
                {
                    if (event_wait_list[i] == nullptr)
                        return CL_INVALID_EVENT_WAIT_LIST;
                    amdWaitList[i] =
                        static_cast<CLRXEvent*>(event_wait_list[i])->amdOclEvent;
                }
                
                status = q->amdOclCommandQueue->dispatch->clEnqueueNativeKernel(
                        q->amdOclCommandQueue, user_func, args, cb_args,
                        num_mem_objects, amdMemListPtr, args_mem_loc,
                        num_events_in_wait_list, amdWaitList.data(), amdEventPtr);
            }
        }
        else
            status = q->amdOclCommandQueue->dispatch->clEnqueueNativeKernel(
                        q->amdOclCommandQueue, user_func, args, cb_args,
                        num_mem_objects, amdMemListPtr, args_mem_loc,
                        0, nullptr, amdEventPtr);
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
        
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clrxclEnqueueMarker(cl_command_queue    command_queue,
                cl_event *          event) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if (event == nullptr)
        return CL_INVALID_VALUE;
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    cl_event amdEvent = nullptr;
    const cl_int status = q->amdOclCommandQueue->dispatch->clEnqueueMarker(
            q->amdOclCommandQueue, &amdEvent);
    
    return clrxApplyCLRXEvent(q, event, amdEvent, status);
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED cl_int CL_API_CALL
clrxclEnqueueWaitForEvents(cl_command_queue command_queue,
            cl_uint          num_events,
            const cl_event * event_list) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    if (num_events == 0 || event_list == nullptr)
        return CL_INVALID_VALUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    if (num_events <= maxLocalEventsNum)
    {
        /* for static allocation */
        cl_event amdEvents[maxLocalEventsNum];
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
        // use vector to hold oroginal AMDOCL events
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
clrxclEnqueueBarrier(cl_command_queue command_queue) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    if (command_queue == nullptr)
        return CL_INVALID_COMMAND_QUEUE;
    
    const CLRXCommandQueue* q = static_cast<const CLRXCommandQueue*>(command_queue);
    return q->amdOclCommandQueue->dispatch->clEnqueueBarrier(q->amdOclCommandQueue);
}

CL_API_ENTRY CL_EXT_PREFIX__VERSION_1_1_DEPRECATED void * CL_API_CALL
clrxclGetExtensionFunctionAddress(const char * func_name)
        CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
    CLRX_INITIALIZE_VOIDPTR
    
    if (!useCLRXWrapper) // call original amdocl function
        return amdOclGetExtensionFunctionAddress(func_name);
    
    const CLRXExtensionEntry tmp = {func_name, nullptr};
    const size_t length = sizeof(clrxExtensionsTable)/sizeof(CLRXExtensionEntry);
    const CLRXExtensionEntry* entry = CLRX::binaryFind(clrxExtensionsTable,
           clrxExtensionsTable + length,
           tmp, [](const CLRXExtensionEntry& l, const CLRXExtensionEntry& r) -> bool
           { return ::strcmp(l.funcname, r.funcname)<0; });
    
    if (entry == clrxExtensionsTable + length)
        return nullptr;
    return entry->address;
}

#ifdef HAVE_OPENGL

CL_API_ENTRY cl_mem CL_API_CALL
clrxclCreateFromGLBuffer(cl_context     context,
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
clrxclCreateFromGLTexture2D(cl_context      context,
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
clrxclCreateFromGLTexture3D(cl_context      context,
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
clrxclCreateFromGLRenderbuffer(cl_context   context,
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
clrxclGetGLObjectInfo(cl_mem                memobj,
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
clrxclGetGLTextureInfo(cl_mem               memobj,
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
clrxclEnqueueAcquireGLObjects(cl_command_queue      command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    try
    {
        // use vector to store original AMDOCL memobjects
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
clrxclEnqueueReleaseGLObjects(cl_command_queue      command_queue,
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
    
    CLRXCommandQueue* q = static_cast<CLRXCommandQueue*>(command_queue);
    
    try
    {
        // use vector to store original AMDOCL memobjects
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
clrxclGetGLContextInfoKHR(const cl_context_properties * properties,
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
                // translate platforms in CL_CONTEXT_PLATOFRM properties
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
        
        try
        { callOnce(platform->onceFlag, clrxPlatformInitializeDevices, platform); }
        catch(const std::exception& ex)
        { clrxAbort("Fatal error at device initialization: ", ex.what()); }
        catch(...)
        { clrxAbort("Fatal and unknown error at device initialization"); }
        if (platform->deviceInitStatus != CL_SUCCESS)
            return platform->deviceInitStatus;
        
        /* convert from original devices into own devices */
        if (param_value != nullptr &&
            (param_name == CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR ||
            param_name == CL_DEVICES_FOR_GL_CONTEXT_KHR))
            translateAMDDevicesIntoCLRXDevices(platform->devicesNum,
                   (const CLRXDevice**)(platform->devicePtrs.get()),
                   myParamValueSize/sizeof(cl_device_id), (cl_device_id*)param_value);
        
        if (param_value_size_ret != nullptr)
            *param_value_size_ret = myParamValueSize;
    }
    catch(const std::bad_alloc& ex)
    { return CL_OUT_OF_HOST_MEMORY; }
    
    return CL_SUCCESS;
}

#endif

} /* extern "C" */
