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

#ifndef __CLRX_DISPATCHSTRUCT_H__
#define __CLRX_DISPATCHSTRUCT_H__

#ifndef CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#endif

#ifndef CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#endif

#ifndef CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#endif

#ifndef CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#undef NOMINMAX
#endif

#ifdef __APPLE__
#  include <OpenCL/cl.h>
#  ifdef HAVE_OPENGL
#    include <OpenGL/gl.h>
#    include <OpenCL/cl_gl.h>
#    include <OpenCL/cl_gl_ext.h>
#  endif
#  include <OpenCL/cl_ext.h>
#else
#  include <CL/cl.h>
#  ifdef HAVE_OPENGL
#    include <GL/gl.h>
#    include <CL/cl_gl.h>
#    include <CL/cl_gl_ext.h>
#  endif
#  include <CL/cl_ext.h>
#endif

#ifndef CL_VERSION_1_2
#define CL_EXT_PREFIX__VERSION_1_1_DEPRECATED
#define CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetPlatformIDs)(
        cl_uint, cl_platform_id*, cl_uint*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetPlatformInfo)(
        cl_platform_id, cl_platform_info, size_t, void*, size_t*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetDeviceIDs)(
        cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetDeviceInfo)(
        cl_device_id, cl_device_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clCreateSubDevices)(
        cl_device_id, const cl_device_partition_property*, cl_uint, cl_device_id*, cl_uint*)
         CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL * CLRXpfn_clRetainDevice)(
        cl_device_id) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL * CLRXpfn_clReleaseDevice)(
        cl_device_id) CL_API_SUFFIX__VERSION_1_2;
#endif

typedef CL_API_ENTRY cl_context (CL_API_CALL *CLRXpfn_clCreateContext)(
        const cl_context_properties*, cl_uint, const cl_device_id*,
        void (CL_CALLBACK*)(const char*, const void*, size_t, void*), void*, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_context (CL_API_CALL *CLRXpfn_clCreateContextFromType)(
        const cl_context_properties*, cl_device_type,
        void (CL_CALLBACK*)(const char*, const void*, size_t, void*), void*, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clRetainContext)(
        cl_context) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clReleaseContext)(
        cl_context) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetContextInfo)(
        cl_context, cl_context_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_command_queue (CL_API_CALL *CLRXpfn_clCreateCommandQueue)(
        cl_context, cl_device_id, cl_command_queue_properties, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_2_0
typedef CL_API_ENTRY cl_command_queue
    (CL_API_CALL *CLRXpfn_clCreateCommandQueueWithProperties)(cl_context, cl_device_id,
        const cl_queue_properties *, cl_int *) CL_API_SUFFIX__VERSION_2_0;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clRetainCommandQueue)(
        cl_command_queue) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clReleaseCommandQueue)(
        cl_command_queue) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetCommandQueueInfo)(
        cl_command_queue, cl_command_queue_info, size_t, void*, size_t*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateBuffer)(
        cl_context, cl_mem_flags, size_t, void*, cl_int*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateImage)(
        cl_context, cl_mem_flags, const cl_image_format*, const cl_image_desc*, void*,
        cl_int*) CL_API_SUFFIX__VERSION_1_2;
#endif
#ifdef CL_VERSION_2_0
typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreatePipe)(cl_context, cl_mem_flags,
        cl_uint, cl_uint, const cl_pipe_properties *, cl_int *) CL_API_SUFFIX__VERSION_2_0;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clRetainMemObject)(cl_mem)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clReleaseMemObject)(cl_mem)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetSupportedImageFormats)(
        cl_context, cl_mem_flags, cl_mem_object_type, cl_uint, cl_image_format*, cl_uint*) 
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetMemObjectInfo)(
        cl_mem, cl_mem_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetImageInfo)(
        cl_mem, cl_image_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_2_0
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetPipeInfo)(cl_mem, cl_pipe_info,
        size_t, void *, size_t *) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY void * (CL_API_CALL *CLRXpfn_clSVMAlloc)(cl_context, cl_svm_mem_flags,
        size_t, cl_uint) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY void (CL_API_CALL *CLRXpfn_clSVMFree)(cl_context, void *)
        CL_API_SUFFIX__VERSION_2_0;
    
typedef CL_API_ENTRY cl_sampler (CL_API_CALL *CLRXpfn_clCreateSamplerWithProperties)(
        cl_context, const cl_sampler_properties *, cl_int *) CL_API_SUFFIX__VERSION_2_0;
#endif

typedef CL_API_ENTRY cl_sampler (CL_API_CALL *CLRXpfn_clCreateSampler)(
        cl_context, cl_bool, cl_addressing_mode, cl_filter_mode, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clRetainSampler)(cl_sampler)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clReleaseSampler)(cl_sampler)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetSamplerInfo)(
        cl_sampler, cl_sampler_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_program (CL_API_CALL *CLRXpfn_clCreateProgramWithSource)(
        cl_context, cl_uint, const char**, const size_t*, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_program (CL_API_CALL *CLRXpfn_clCreateProgramWithBinary)(
        cl_context, cl_uint, const cl_device_id*, const size_t*, const unsigned char**,
        cl_int*, cl_int*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_program (CL_API_CALL *CLRXpfn_clCreateProgramWithBuiltInKernels)(
        cl_context, cl_uint, const cl_device_id*, const char*, cl_int*)
        CL_API_SUFFIX__VERSION_1_2;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clRetainProgram)(cl_program)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clReleaseProgram)(cl_program)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clBuildProgram)(
        cl_program, cl_uint, const cl_device_id*, const char*,
        void (CL_CALLBACK*)(cl_program, void*), void*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clCompileProgram)(
        cl_program, cl_uint, const cl_device_id*, const char*, cl_uint, const cl_program*,
        const char**, void (CL_CALLBACK*)(cl_program, void*), void*)
        CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_program (CL_API_CALL *CLRXpfn_clLinkProgram)(
        cl_context, cl_uint, const cl_device_id*, const char*, cl_uint, const cl_program*,
        void (CL_CALLBACK*)(cl_program, void*), void*, cl_int*)
        CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clUnloadPlatformCompiler)(
        cl_platform_id) CL_API_SUFFIX__VERSION_1_2;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetProgramInfo)(
        cl_program, cl_program_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetProgramBuildInfo)(
        cl_program, cl_device_id, cl_program_build_info, size_t, void*, size_t*)
        CL_API_SUFFIX__VERSION_1_0;

// Kernel Object APIs
typedef CL_API_ENTRY cl_kernel (CL_API_CALL *CLRXpfn_clCreateKernel)(
        cl_program, const char*, cl_int*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clCreateKernelsInProgram)(
        cl_program, cl_uint, cl_kernel*, cl_uint*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clRetainKernel)(cl_kernel)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clReleaseKernel)(cl_kernel)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clSetKernelArg)(
        cl_kernel, cl_uint, size_t, const void*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_2_0
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clSetKernelArgSVMPointer)(cl_kernel,
        cl_uint, const void *) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clSetKernelExecInfo)(cl_kernel,
        cl_kernel_exec_info, size_t, const void *) CL_API_SUFFIX__VERSION_2_0;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetKernelInfo)(
        cl_kernel, cl_kernel_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetKernelArgInfo)(
        cl_kernel, cl_uint, cl_kernel_arg_info, size_t, void*, size_t*)
        CL_API_SUFFIX__VERSION_1_2;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetKernelWorkGroupInfo)(
        cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void*, size_t*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clWaitForEvents)(
        cl_uint, const cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetEventInfo)(
        cl_event, cl_event_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clRetainEvent)(cl_event)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clReleaseEvent)(cl_event)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetEventProfilingInfo)(
        cl_event, cl_profiling_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clFlush)(cl_command_queue)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clFinish)(cl_command_queue)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueReadBuffer)(
        cl_command_queue, cl_mem, cl_bool, size_t, size_t, void*, cl_uint, const cl_event*,
        cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueReadBufferRect)(
        cl_command_queue, cl_mem, cl_bool, const size_t*, const size_t*, const size_t*,
        size_t, size_t, size_t, size_t, void*, cl_uint, const cl_event*, cl_event*)
        CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueWriteBuffer)(
        cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void*, cl_uint, 
        const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueWriteBufferRect)(
        cl_command_queue, cl_mem, cl_bool, const size_t*, const size_t*, const size_t*,
        size_t, size_t, size_t, size_t,    const void*, cl_uint, const cl_event*,
        cl_event*) CL_API_SUFFIX__VERSION_1_1;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueFillBuffer)(
        cl_command_queue, cl_mem, const void*, size_t, size_t, size_t, cl_uint,
        const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_2;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueCopyBuffer)(
        cl_command_queue, cl_mem, cl_mem, size_t, size_t, size_t, cl_uint,
        const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueCopyBufferRect)(
        cl_command_queue, cl_mem, cl_mem, const size_t*, const size_t*, const size_t*,
        size_t, size_t, size_t, size_t, cl_uint, const cl_event*, cl_event*)
        CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueReadImage)(
        cl_command_queue, cl_mem, cl_bool, const size_t*, const size_t*, size_t, size_t, 
        void*, cl_uint, const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueWriteImage)(
        cl_command_queue, cl_mem, cl_bool, const size_t*, const size_t*, size_t, size_t, 
        const void*, cl_uint, const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueFillImage)(
        cl_command_queue, cl_mem, const void*, const size_t*, const size_t*, cl_uint,
        const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_2;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueCopyImage)(
        cl_command_queue, cl_mem, cl_mem, const size_t*, const size_t*, const size_t*, 
        cl_uint, const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueCopyImageToBuffer)(
        cl_command_queue, cl_mem, cl_mem, const size_t*, const size_t*, size_t, cl_uint,
        const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueCopyBufferToImage)(
        cl_command_queue, cl_mem, cl_mem, size_t, const size_t*, const size_t*, cl_uint,
        const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY void * (CL_API_CALL *CLRXpfn_clEnqueueMapBuffer)(
        cl_command_queue, cl_mem, cl_bool, cl_map_flags, size_t, size_t, cl_uint,
        const cl_event*, cl_event*, cl_int*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY void * (CL_API_CALL *CLRXpfn_clEnqueueMapImage)(
        cl_command_queue, cl_mem, cl_bool, cl_map_flags, const size_t*, const size_t*,
        size_t*, size_t*, cl_uint, const cl_event*, cl_event*, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueUnmapMemObject)(
        cl_command_queue, cl_mem, void*, cl_uint, const cl_event*, cl_event*)
        CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueMigrateMemObjects)(
        cl_command_queue, cl_uint, const cl_mem*, cl_mem_migration_flags, cl_uint,
        const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_2;
#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueNDRangeKernel)(
        cl_command_queue, cl_kernel, cl_uint, const size_t*, const size_t*, const size_t*,
        cl_uint, const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueTask)(
        cl_command_queue, cl_kernel, cl_uint, const cl_event*, cl_event*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueNativeKernel)(
        cl_command_queue, void (CL_CALLBACK*)(void *), void*, size_t, cl_uint,
        const cl_mem*, const void**, cl_uint, const cl_event*, cl_event*)
        CL_API_SUFFIX__VERSION_1_0;

#ifdef CL_VERSION_1_2
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueMarkerWithWaitList)(
        cl_command_queue, cl_uint, const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueBarrierWithWaitList)(
        cl_command_queue, cl_uint, const cl_event*, cl_event*) CL_API_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY void * (CL_API_CALL *CLRXpfn_clGetExtensionFunctionAddressForPlatform)(
        cl_platform_id, const char*) CL_API_SUFFIX__VERSION_1_2;
#endif

#ifdef CL_VERSION_2_0
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueSVMFree)(cl_command_queue,
        cl_uint, void *[],
        void (CL_CALLBACK *)(cl_command_queue, cl_uint, void *[], void *),
        void *, cl_uint, const cl_event *, cl_event *) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueSVMMemcpy)(cl_command_queue,
        cl_bool, void *, const void *, size_t, cl_uint, const cl_event *,
        cl_event *) CL_API_SUFFIX__VERSION_2_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueSVMMemFill)(cl_command_queue,
        void *, const void *, size_t, size_t, cl_uint, const cl_event *,
        cl_event *) CL_API_SUFFIX__VERSION_2_0;
    
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueSVMMap)(cl_command_queue,
        cl_bool, cl_map_flags, void *, size_t, cl_uint, const cl_event *, cl_event *)
        CL_API_SUFFIX__VERSION_2_0;
    
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueSVMUnmap)(cl_command_queue,
        void *, cl_uint, const cl_event *, cl_event *) CL_API_SUFFIX__VERSION_2_0;

#endif

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clSetCommandQueueProperty)(
        cl_command_queue, cl_command_queue_properties, cl_bool,
        cl_command_queue_properties*) CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateImage2D)(
        cl_context, cl_mem_flags, const cl_image_format*, size_t, size_t, size_t, void*,
        cl_int*) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateImage3D)(
        cl_context, cl_mem_flags, const cl_image_format*, size_t, size_t, size_t, size_t, 
        size_t, void*, cl_int*) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clUnloadCompiler)(void)
        CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueMarker)(
        cl_command_queue, cl_event*) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueWaitForEvents)(
        cl_command_queue, cl_uint, const cl_event*) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueBarrier)(
        cl_command_queue) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

typedef CL_API_ENTRY void * (CL_API_CALL *CLRXpfn_clGetExtensionFunctionAddress)(
        const char*) CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED;

#ifdef HAVE_OPENGL
typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateFromGLBuffer)(
        cl_context, cl_mem_flags, GLuint, int*) CL_API_SUFFIX__VERSION_1_0;
#endif

#if defined(CL_VERSION_1_2) && defined(HAVE_OPENGL)
typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateFromGLTexture)(
        cl_context, cl_mem_flags, cl_GLenum, cl_GLint, cl_GLuint, cl_int*)
        CL_API_SUFFIX__VERSION_1_2;
#endif

#ifdef HAVE_OPENGL
typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateFromGLTexture2D)(
        cl_context, cl_mem_flags, GLenum, GLint, GLuint, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateFromGLTexture3D)(
        cl_context, cl_mem_flags, GLenum, GLint, GLuint, cl_int*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateFromGLRenderbuffer)(
        cl_context, cl_mem_flags, GLuint, cl_int*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetGLObjectInfo)(
        cl_mem, cl_gl_object_type*, GLuint*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetGLTextureInfo)(
        cl_mem, cl_gl_texture_info, size_t, void*, size_t*) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueAcquireGLObjects)(
        cl_command_queue, cl_uint, const cl_mem*, cl_uint, const cl_event*, cl_event*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clEnqueueReleaseGLObjects)(
        cl_command_queue, cl_uint, const cl_mem*, cl_uint, const cl_event*, cl_event*)
        CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clGetGLContextInfoKHR)(
        const cl_context_properties*, cl_gl_context_info, size_t, void*, size_t*);

typedef CL_API_ENTRY cl_event (CL_API_CALL *CLRXpfn_clCreateEventFromGLsyncKHR)(
        cl_context, cl_GLsync, cl_int*);
#endif
typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clSetEventCallback)(
        cl_event, cl_int, void(CL_CALLBACK*)(cl_event, cl_int, void*), void*)
        CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *CLRXpfn_clCreateSubBuffer)(
        cl_mem, cl_mem_flags, cl_buffer_create_type, const void*, cl_int*)
        CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clSetMemObjectDestructorCallback)(
        cl_mem, void (CL_CALLBACK*)(cl_mem, void*), void*) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_event (CL_API_CALL *CLRXpfn_clCreateUserEvent)(
        cl_context, cl_int*) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clSetUserEventStatus)(
        cl_event, cl_int) CL_API_SUFFIX__VERSION_1_1;

typedef CL_API_ENTRY cl_int (CL_API_CALL *CLRXpfn_clCreateSubDevicesEXT)(
        cl_device_id, const cl_device_partition_property_ext*, cl_uint, cl_device_id*,
        cl_uint*);

typedef CL_API_ENTRY cl_int (CL_API_CALL * CLRXpfn_clRetainDeviceEXT)(
        cl_device_id) CL_API_SUFFIX__VERSION_1_0;

typedef CL_API_ENTRY cl_int (CL_API_CALL * CLRXpfn_clReleaseDeviceEXT)(
        cl_device_id) CL_API_SUFFIX__VERSION_1_0;

typedef void *CLRXpfn_emptyFunction;

#define CLRXICD_ENTRIES_NUM (136U)

typedef union _CLRXIcdDispatch
{
    struct
    {
        CLRXpfn_clGetPlatformIDs clGetPlatformIDs;
        CLRXpfn_clGetPlatformInfo clGetPlatformInfo;
        CLRXpfn_clGetDeviceIDs clGetDeviceIDs;
        CLRXpfn_clGetDeviceInfo clGetDeviceInfo;
        CLRXpfn_clCreateContext clCreateContext;
        CLRXpfn_clCreateContextFromType clCreateContextFromType;
        CLRXpfn_clRetainContext clRetainContext;
        CLRXpfn_clReleaseContext clReleaseContext;
        CLRXpfn_clGetContextInfo clGetContextInfo;
        CLRXpfn_clCreateCommandQueue clCreateCommandQueue;
        CLRXpfn_clRetainCommandQueue clRetainCommandQueue;
        CLRXpfn_clReleaseCommandQueue clReleaseCommandQueue;
        CLRXpfn_clGetCommandQueueInfo clGetCommandQueueInfo;
        CLRXpfn_clSetCommandQueueProperty clSetCommandQueueProperty;
        CLRXpfn_clCreateBuffer clCreateBuffer;
        CLRXpfn_clCreateImage2D clCreateImage2D;
        CLRXpfn_clCreateImage3D clCreateImage3D;
        CLRXpfn_clRetainMemObject clRetainMemObject;
        CLRXpfn_clReleaseMemObject clReleaseMemObject;
        CLRXpfn_clGetSupportedImageFormats clGetSupportedImageFormats;
        CLRXpfn_clGetMemObjectInfo clGetMemObjectInfo;
        CLRXpfn_clGetImageInfo clGetImageInfo;
        CLRXpfn_clCreateSampler clCreateSampler;
        CLRXpfn_clRetainSampler clRetainSampler;
        CLRXpfn_clReleaseSampler clReleaseSampler;
        CLRXpfn_clGetSamplerInfo clGetSamplerInfo;
        CLRXpfn_clCreateProgramWithSource clCreateProgramWithSource;
        CLRXpfn_clCreateProgramWithBinary clCreateProgramWithBinary;
        CLRXpfn_clRetainProgram clRetainProgram;
        CLRXpfn_clReleaseProgram clReleaseProgram;
        CLRXpfn_clBuildProgram clBuildProgram;
        CLRXpfn_clUnloadCompiler clUnloadCompiler;
        CLRXpfn_clGetProgramInfo clGetProgramInfo;
        CLRXpfn_clGetProgramBuildInfo clGetProgramBuildInfo;
        CLRXpfn_clCreateKernel clCreateKernel;
        CLRXpfn_clCreateKernelsInProgram clCreateKernelsInProgram;
        CLRXpfn_clRetainKernel clRetainKernel;
        CLRXpfn_clReleaseKernel clReleaseKernel;
        CLRXpfn_clSetKernelArg clSetKernelArg;
        CLRXpfn_clGetKernelInfo clGetKernelInfo;
        CLRXpfn_clGetKernelWorkGroupInfo clGetKernelWorkGroupInfo;
        CLRXpfn_clWaitForEvents clWaitForEvents;
        CLRXpfn_clGetEventInfo clGetEventInfo;
        CLRXpfn_clRetainEvent clRetainEvent;
        CLRXpfn_clReleaseEvent clReleaseEvent;
        CLRXpfn_clGetEventProfilingInfo clGetEventProfilingInfo;
        CLRXpfn_clFlush clFlush;
        CLRXpfn_clFinish clFinish;
        CLRXpfn_clEnqueueReadBuffer clEnqueueReadBuffer;
        CLRXpfn_clEnqueueWriteBuffer clEnqueueWriteBuffer;
        CLRXpfn_clEnqueueCopyBuffer clEnqueueCopyBuffer;
        CLRXpfn_clEnqueueReadImage clEnqueueReadImage;
        CLRXpfn_clEnqueueWriteImage clEnqueueWriteImage;
        CLRXpfn_clEnqueueCopyImage clEnqueueCopyImage;
        CLRXpfn_clEnqueueCopyImageToBuffer clEnqueueCopyImageToBuffer;
        CLRXpfn_clEnqueueCopyBufferToImage clEnqueueCopyBufferToImage;
        CLRXpfn_clEnqueueMapBuffer clEnqueueMapBuffer;
        CLRXpfn_clEnqueueMapImage clEnqueueMapImage;
        CLRXpfn_clEnqueueUnmapMemObject clEnqueueUnmapMemObject;
        CLRXpfn_clEnqueueNDRangeKernel clEnqueueNDRangeKernel;
        CLRXpfn_clEnqueueTask clEnqueueTask;
        CLRXpfn_clEnqueueNativeKernel clEnqueueNativeKernel;
        CLRXpfn_clEnqueueMarker clEnqueueMarker;
        CLRXpfn_clEnqueueWaitForEvents clEnqueueWaitForEvents;
        CLRXpfn_clEnqueueBarrier clEnqueueBarrier;
        CLRXpfn_clGetExtensionFunctionAddress clGetExtensionFunctionAddress;
#ifdef HAVE_OPENGL
        CLRXpfn_clCreateFromGLBuffer clCreateFromGLBuffer;
        CLRXpfn_clCreateFromGLTexture2D clCreateFromGLTexture2D;
        CLRXpfn_clCreateFromGLTexture3D clCreateFromGLTexture3D;
        CLRXpfn_clCreateFromGLRenderbuffer clCreateFromGLRenderbuffer;
        CLRXpfn_clGetGLObjectInfo clGetGLObjectInfo;
        CLRXpfn_clGetGLTextureInfo clGetGLTextureInfo;
        CLRXpfn_clEnqueueAcquireGLObjects clEnqueueAcquireGLObjects;
        CLRXpfn_clEnqueueReleaseGLObjects clEnqueueReleaseGLObjects;
        CLRXpfn_clGetGLContextInfoKHR clGetGLContextInfoKHR;
#else
        CLRXpfn_emptyFunction clCreateFromGLBuffer;
        CLRXpfn_emptyFunction clCreateFromGLTexture2D;
        CLRXpfn_emptyFunction clCreateFromGLTexture3D;
        CLRXpfn_emptyFunction clCreateFromGLRenderbuffer;
        CLRXpfn_emptyFunction clGetGLObjectInfo;
        CLRXpfn_emptyFunction clGetGLTextureInfo;
        CLRXpfn_emptyFunction clEnqueueAcquireGLObjects;
        CLRXpfn_emptyFunction clEnqueueReleaseGLObjects;
        CLRXpfn_emptyFunction clGetGLContextInfoKHR;
#endif
        /* Direct3D */
        CLRXpfn_emptyFunction clGetDeviceIDsFromD3D10KHR;
        CLRXpfn_emptyFunction clCreateFromD3D10BufferKHR;
        CLRXpfn_emptyFunction clCreateFromD3D10Texture2DKHR;
        CLRXpfn_emptyFunction clCreateFromD3D10Texture3DKHR;
        CLRXpfn_emptyFunction clEnqueueAcquireD3D10ObjectsKHR;
        CLRXpfn_emptyFunction clEnqueueReleaseD3D10ObjectsKHR;
        /* opencl 1.1 */
        CLRXpfn_clSetEventCallback clSetEventCallback;
        CLRXpfn_clCreateSubBuffer clCreateSubBuffer;
        CLRXpfn_clSetMemObjectDestructorCallback clSetMemObjectDestructorCallback;
        CLRXpfn_clCreateUserEvent clCreateUserEvent;
        CLRXpfn_clSetUserEventStatus clSetUserEventStatus;
        CLRXpfn_clEnqueueReadBufferRect clEnqueueReadBufferRect;
        CLRXpfn_clEnqueueWriteBufferRect clEnqueueWriteBufferRect;
        CLRXpfn_clEnqueueCopyBufferRect clEnqueueCopyBufferRect;

        CLRXpfn_clCreateSubDevicesEXT clCreateSubDevicesEXT;
        CLRXpfn_clRetainDeviceEXT clRetainDeviceEXT;
        CLRXpfn_clReleaseDeviceEXT clReleaseDeviceEXT;

#ifdef HAVE_OPENGL
        CLRXpfn_clCreateEventFromGLsyncKHR clCreateEventFromGLsyncKHR;
#else
        CLRXpfn_emptyFunction clCreateEventFromGLsyncKHR;
#endif
#ifdef CL_VERSION_1_2
        CLRXpfn_clCreateSubDevices clCreateSubDevices;
        CLRXpfn_clRetainDevice clRetainDevice;
        CLRXpfn_clReleaseDevice clReleaseDevice;
        CLRXpfn_clCreateImage clCreateImage;
        CLRXpfn_clCreateProgramWithBuiltInKernels clCreateProgramWithBuiltInKernels;
        CLRXpfn_clCompileProgram clCompileProgram;
        CLRXpfn_clLinkProgram clLinkProgram;
        CLRXpfn_clUnloadPlatformCompiler clUnloadPlatformCompiler;
        CLRXpfn_clGetKernelArgInfo clGetKernelArgInfo;
        CLRXpfn_clEnqueueFillBuffer clEnqueueFillBuffer;
        CLRXpfn_clEnqueueFillImage clEnqueueFillImage;
        CLRXpfn_clEnqueueMigrateMemObjects clEnqueueMigrateMemObjects;
        CLRXpfn_clEnqueueMarkerWithWaitList clEnqueueMarkerWithWaitList;
        CLRXpfn_clEnqueueBarrierWithWaitList clEnqueueBarrierWithWaitList;
        CLRXpfn_clGetExtensionFunctionAddressForPlatform
                clGetExtensionFunctionAddressForPlatform;
#ifdef HAVE_OPENGL
        CLRXpfn_clCreateFromGLTexture clCreateFromGLTexture;
#else
        CLRXpfn_emptyFunction clCreateFromGLTexture;
#endif

        CLRXpfn_emptyFunction clGetDeviceIDsFromD3D11KHR;
        CLRXpfn_emptyFunction clCreateFromD3D11BufferKHR;
        CLRXpfn_emptyFunction clCreateFromD3D11Texture2DKHR;
        CLRXpfn_emptyFunction clCreateFromD3D11Texture3DKHR;
        CLRXpfn_emptyFunction clCreateFromDX9MediaSurfaceKHR;
        CLRXpfn_emptyFunction clEnqueueAcquireD3D11ObjectsKHR;
        CLRXpfn_emptyFunction clEnqueueReleaseD3D11ObjectsKHR;
        CLRXpfn_emptyFunction clGetDeviceIDsFromDX9MediaAdapterKHR;
        CLRXpfn_emptyFunction clEnqueueAcquireDX9MediaSurfacesKHR;
        CLRXpfn_emptyFunction clEnqueueReleaseDX9MediaSurfacesKHR;
#endif
#ifdef CL_VERSION_2_0
        CLRXpfn_emptyFunction clCreateFromEGLImageKHR;
        CLRXpfn_emptyFunction clEnqueueAcquireEGLObjectsKHR;
        CLRXpfn_emptyFunction clEnqueueReleaseEGLObjectsKHR;
        CLRXpfn_emptyFunction clCreateEventFromEGLSyncKHR;
        CLRXpfn_clCreateCommandQueueWithProperties clCreateCommandQueueWithProperties;
        CLRXpfn_clCreatePipe clCreatePipe;
        CLRXpfn_clGetPipeInfo clGetPipeInfo;
        CLRXpfn_clSVMAlloc clSVMAlloc;
        CLRXpfn_clSVMFree clSVMFree;
        CLRXpfn_clEnqueueSVMFree clEnqueueSVMFree;
        CLRXpfn_clEnqueueSVMMemcpy clEnqueueSVMMemcpy;
        CLRXpfn_clEnqueueSVMMemFill clEnqueueSVMMemFill;
        CLRXpfn_clEnqueueSVMMap clEnqueueSVMMap;
        CLRXpfn_clEnqueueSVMUnmap clEnqueueSVMUnmap;
        CLRXpfn_clCreateSamplerWithProperties clCreateSamplerWithProperties;
        CLRXpfn_clSetKernelArgSVMPointer clSetKernelArgSVMPointer;
        CLRXpfn_clSetKernelExecInfo clSetKernelExecInfo;
#endif
    };
    void* entries[CLRXICD_ENTRIES_NUM];
} CLRXIcdDispatch;

struct _cl_platform_id
{ CLRXIcdDispatch *dispatch; };

struct _cl_device_id
{ CLRXIcdDispatch *dispatch; };

struct _cl_context
{ CLRXIcdDispatch *dispatch; };

struct _cl_command_queue
{ CLRXIcdDispatch *dispatch; };

struct _cl_mem
{ CLRXIcdDispatch *dispatch; };

struct _cl_program
{ CLRXIcdDispatch *dispatch; };

struct _cl_kernel
{ CLRXIcdDispatch *dispatch; };

struct _cl_event
{ CLRXIcdDispatch *dispatch; };

struct _cl_sampler
{ CLRXIcdDispatch *dispatch; };

#endif
