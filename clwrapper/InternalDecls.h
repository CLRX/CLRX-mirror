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

#ifndef __CLRX_INTERNALDECLS_H__
#define __CLRX_INTERNALDECLS_H__

#ifndef CL_SUCCESS
#  error "This file must be included after CL/cl.h"
#endif

#if __GNUC__ >= 4 && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(__MINGW64__)
#  define CLRX_CL_INTERNAL_DECLSYM(NAME) extern decltype(NAME) clrx##NAME \
        __attribute__((visibility("hidden")));

#  define CLRX_CL_PUBLIC_SYM(NAME) decltype(NAME) NAME \
        __attribute__((alias("clrx" #NAME), visibility("default")));
#else
#  define CLRX_CL_INTERNAL_DECLSYM(NAME) extern decltype(NAME) clrx##NAME;
#  define CLRX_CL_PUBLIC_SYM(NAME)
#endif

CLRX_CL_INTERNAL_DECLSYM(clGetPlatformIDs)
CLRX_CL_INTERNAL_DECLSYM(clIcdGetPlatformIDsKHR)
CLRX_CL_INTERNAL_DECLSYM(clGetPlatformInfo)
CLRX_CL_INTERNAL_DECLSYM(clGetDeviceIDs)
CLRX_CL_INTERNAL_DECLSYM(clGetDeviceInfo)
CLRX_CL_INTERNAL_DECLSYM(clCreateContext)
CLRX_CL_INTERNAL_DECLSYM(clCreateContextFromType)
CLRX_CL_INTERNAL_DECLSYM(clRetainContext)
CLRX_CL_INTERNAL_DECLSYM(clReleaseContext)
CLRX_CL_INTERNAL_DECLSYM(clGetContextInfo)
CLRX_CL_INTERNAL_DECLSYM(clCreateCommandQueue)
CLRX_CL_INTERNAL_DECLSYM(clRetainCommandQueue)
CLRX_CL_INTERNAL_DECLSYM(clReleaseCommandQueue)
CLRX_CL_INTERNAL_DECLSYM(clGetCommandQueueInfo)
CLRX_CL_INTERNAL_DECLSYM(clSetCommandQueueProperty)
CLRX_CL_INTERNAL_DECLSYM(clCreateBuffer)
CLRX_CL_INTERNAL_DECLSYM(clCreateImage2D)
CLRX_CL_INTERNAL_DECLSYM(clCreateImage3D)
CLRX_CL_INTERNAL_DECLSYM(clRetainMemObject)
CLRX_CL_INTERNAL_DECLSYM(clReleaseMemObject)
CLRX_CL_INTERNAL_DECLSYM(clGetSupportedImageFormats)
CLRX_CL_INTERNAL_DECLSYM(clGetMemObjectInfo)
CLRX_CL_INTERNAL_DECLSYM(clGetImageInfo)
CLRX_CL_INTERNAL_DECLSYM(clCreateSampler)
CLRX_CL_INTERNAL_DECLSYM(clRetainSampler)
CLRX_CL_INTERNAL_DECLSYM(clReleaseSampler)
CLRX_CL_INTERNAL_DECLSYM(clGetSamplerInfo)
CLRX_CL_INTERNAL_DECLSYM(clCreateProgramWithSource)
CLRX_CL_INTERNAL_DECLSYM(clCreateProgramWithBinary)
CLRX_CL_INTERNAL_DECLSYM(clRetainProgram)
CLRX_CL_INTERNAL_DECLSYM(clReleaseProgram)
CLRX_CL_INTERNAL_DECLSYM(clBuildProgram)
CLRX_CL_INTERNAL_DECLSYM(clUnloadCompiler)
CLRX_CL_INTERNAL_DECLSYM(clGetProgramInfo)
CLRX_CL_INTERNAL_DECLSYM(clGetProgramBuildInfo)
CLRX_CL_INTERNAL_DECLSYM(clCreateKernel)
CLRX_CL_INTERNAL_DECLSYM(clCreateKernelsInProgram)
CLRX_CL_INTERNAL_DECLSYM(clRetainKernel)
CLRX_CL_INTERNAL_DECLSYM(clReleaseKernel)
CLRX_CL_INTERNAL_DECLSYM(clSetKernelArg)
CLRX_CL_INTERNAL_DECLSYM(clGetKernelInfo)
CLRX_CL_INTERNAL_DECLSYM(clGetKernelWorkGroupInfo)
CLRX_CL_INTERNAL_DECLSYM(clWaitForEvents)
CLRX_CL_INTERNAL_DECLSYM(clGetEventInfo)
CLRX_CL_INTERNAL_DECLSYM(clRetainEvent)
CLRX_CL_INTERNAL_DECLSYM(clReleaseEvent)
CLRX_CL_INTERNAL_DECLSYM(clGetEventProfilingInfo)
CLRX_CL_INTERNAL_DECLSYM(clFlush)
CLRX_CL_INTERNAL_DECLSYM(clFinish)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueReadBuffer)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueWriteBuffer)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueCopyBuffer)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueReadImage)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueWriteImage)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueCopyImage)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueCopyImageToBuffer)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueCopyBufferToImage)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueMapBuffer)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueMapImage)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueUnmapMemObject)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueNDRangeKernel)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueTask)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueNativeKernel)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueMarker)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueWaitForEvents)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueBarrier)
CLRX_CL_INTERNAL_DECLSYM(clGetExtensionFunctionAddress)
#ifdef HAVE_OPENGL
CLRX_CL_INTERNAL_DECLSYM(clCreateFromGLBuffer)
CLRX_CL_INTERNAL_DECLSYM(clCreateFromGLTexture2D)
CLRX_CL_INTERNAL_DECLSYM(clCreateFromGLTexture3D)
CLRX_CL_INTERNAL_DECLSYM(clCreateFromGLRenderbuffer)
CLRX_CL_INTERNAL_DECLSYM(clGetGLObjectInfo)
CLRX_CL_INTERNAL_DECLSYM(clGetGLTextureInfo)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueAcquireGLObjects)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueReleaseGLObjects)
CLRX_CL_INTERNAL_DECLSYM(clGetGLContextInfoKHR)
#endif
CLRX_CL_INTERNAL_DECLSYM(clSetEventCallback)
CLRX_CL_INTERNAL_DECLSYM(clCreateSubBuffer)
CLRX_CL_INTERNAL_DECLSYM(clSetMemObjectDestructorCallback)
CLRX_CL_INTERNAL_DECLSYM(clCreateUserEvent)
CLRX_CL_INTERNAL_DECLSYM(clSetUserEventStatus)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueReadBufferRect)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueWriteBufferRect)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueCopyBufferRect)
CLRX_CL_INTERNAL_DECLSYM(clCreateSubDevicesEXT)
CLRX_CL_INTERNAL_DECLSYM(clRetainDeviceEXT)
CLRX_CL_INTERNAL_DECLSYM(clReleaseDeviceEXT)
#ifdef HAVE_OPENGL
CLRX_CL_INTERNAL_DECLSYM(clCreateEventFromGLsyncKHR)
#endif
#ifdef CL_VERSION_1_2
CLRX_CL_INTERNAL_DECLSYM(clCreateSubDevices)
CLRX_CL_INTERNAL_DECLSYM(clRetainDevice)
CLRX_CL_INTERNAL_DECLSYM(clReleaseDevice)
CLRX_CL_INTERNAL_DECLSYM(clCreateImage)
CLRX_CL_INTERNAL_DECLSYM(clCreateProgramWithBuiltInKernels)
CLRX_CL_INTERNAL_DECLSYM(clCompileProgram)
CLRX_CL_INTERNAL_DECLSYM(clLinkProgram)
CLRX_CL_INTERNAL_DECLSYM(clUnloadPlatformCompiler)
CLRX_CL_INTERNAL_DECLSYM(clGetKernelArgInfo)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueFillBuffer)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueFillImage)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueMigrateMemObjects)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueMarkerWithWaitList)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueBarrierWithWaitList)
CLRX_CL_INTERNAL_DECLSYM(clGetExtensionFunctionAddressForPlatform)
#ifdef HAVE_OPENGL
CLRX_CL_INTERNAL_DECLSYM(clCreateFromGLTexture)
#endif
CLRX_CL_INTERNAL_DECLSYM(clEnqueueWaitSignalAMD)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueWriteSignalAMD)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueMakeBuffersResidentAMD)
#endif
#ifdef CL_VERSION_2_0
CLRX_CL_INTERNAL_DECLSYM(clCreateCommandQueueWithProperties)
CLRX_CL_INTERNAL_DECLSYM(clCreatePipe)
CLRX_CL_INTERNAL_DECLSYM(clGetPipeInfo)
CLRX_CL_INTERNAL_DECLSYM(clSVMAlloc)
CLRX_CL_INTERNAL_DECLSYM(clSVMFree)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueSVMFree)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueSVMMemcpy)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueSVMMemFill)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueSVMMap)
CLRX_CL_INTERNAL_DECLSYM(clEnqueueSVMUnmap)
CLRX_CL_INTERNAL_DECLSYM(clCreateSamplerWithProperties)
CLRX_CL_INTERNAL_DECLSYM(clSetKernelArgSVMPointer)
CLRX_CL_INTERNAL_DECLSYM(clSetKernelExecInfo)
#endif

#endif
