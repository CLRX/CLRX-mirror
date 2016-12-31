/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
#include <CL/cl.h>

/// error class based on std::exception
class CLError: public std::exception
{
private:
    cl_int error;
    std::string description;
public:
    /// empty constructor
    CLError() : error(0)
    { }
    explicit CLError(const char* _description) : error(0), description(_description)
    { }
    CLError(cl_int _error, const char* _description) : error(_error)
    {
        char buf[20];
        ::snprintf(buf, 20, "%d", _error);
        description = "Error code: ";
        description += buf;
        description += ", Desc: ";
        description += _description;
    }
    virtual ~CLError() noexcept
    { }
    const char* what() const noexcept
    { return (!description.empty()) ? description.c_str() : "No error!"; }
    int code() const
    { return error; }
};

class CLFacade
{
protected:
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
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
                  const char** argv, cl_uint& deviceIndex, bool& useCL2);
    
    explicit CLFacade(cl_uint deviceIndex, const char* sourceCode,
                      const char* kernelNames = nullptr, bool useCL2 = false);
    ~CLFacade();
};

#endif
