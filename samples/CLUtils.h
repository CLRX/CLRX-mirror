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

#ifndef __CLRXSAMPLES_CLUTILS_H__
#define __CLRXSAMPLES_CLUTILS_H__

#include <cstdio>
#include <string>
#include <vector>
#include <exception>
#include <CLRX/clhelper/CLHelper.h>
#ifdef __APPLE__
#  include <OpenCL/cl.h>
#else
#  include <CL/cl.h>
#endif

class CLFacade
{
protected:
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    // mem object to retain and release while destructing
    std::vector<cl_mem> memObjects;
    std::vector<cl_kernel> kernels;
    
    cl_uint computeUnits;
    size_t maxWorkGroupSize;
    
    void getKernelInfo(cl_kernel kernel, size_t& workGroupSize,
               size_t& workGroupSizeMultiple);
    
    void callNDRangeKernel(cl_kernel kernel, cl_uint workDim, const size_t* offset,
               const size_t* workSize, const size_t* localSize);
public:
    // return true if application should immediately return
    static bool parseArgs(const char* progName, const char* usagePart, int argc,
                  const char** argv, cl_uint& deviceIndex, cxuint& useCL);
    
    explicit CLFacade(cl_uint deviceIndex, const char* sourceCode,
                      const char* kernelNames = nullptr, cxuint useCL = 0);
    ~CLFacade();
};

#endif
