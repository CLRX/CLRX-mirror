/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <iostream>
#include <memory>
#include <cmath>
#include <CLRX/utils/Utilities.h>
#include "CLUtils.h"

using namespace CLRX;

static const char* genericPtrSource = R"ffDXD(
.ifarch gcn1.0
    .error "Unsupported GCN1.0 architecture"
.endif
SMUL = 1
.ifarch gcn1.2
    SMUL = 4
.elseifarch gcn1.4
    SMUL = 4
.endif
.ifnfmt amdcl2  # AMD OpenCL 2.0 code
    .error "Only AMD OpenCL 2.0 binary format is supported"
.endif
.kernel genericPtr
    .config
        .dims x
        .setupargs
        .arg out,uint*,global
        .useargs
        .usegeneric         # enable generic pointers
        .localsize 400      # local size
        .scratchbuffer 180   # define scratch buffer
    .text
    .if32
        .error "this example doesn't work in 32-bit OpenCL 2.0!"
    .endif
        # initialize flat_scratch
    .ifarch GCN1.4
        s_add_u32 flat_scratch_lo, s10, s13
        s_addc_u32 flat_scratch_hi, s11, 0
    .else
        s_add_u32 s5, s10, s13      # flat scratch offset
        s_lshr_b32 s5, s5, 8        # for bitfield
        s_mov_b32 s10, s11          # flat scratch size
        s_mov_b32 s11, s5
        s_mov_b64 flat_scratch, s[10:11]    # store to flat_scratch
    .endif
        
        s_load_dwordx2 s[10:11], s[6:7], 16*SMUL # load localptr base and scratchptr base
    .if32
        s_load_dword s8, s[8:9], 6*SMUL   # load output buffer pointer
    .else
        s_load_dwordx2 s[8:9], s[8:9], 12*SMUL   # load output buffer pointer
    .endif
        s_waitcnt lgkmcnt(0)        # wait for data
        # write to LDS
        s_mov_b32 m0, 0x8000            # needed by local access
        v_mov_b32 v1, 36  # address
        v_mov_b32 v2, 24411  # value
        ds_write_b32 v1, v2         # store this value to LDS
        s_waitcnt lgkmcnt(0)        # wait
        # write to scratch buffer
        v_mov_b32 v3, 15321         # value to write
        buffer_store_dword v3, v0, s[0:3], s13 offset:76    # store to scratch buffer
        s_waitcnt vmcnt(0)
    .if32
        s_movk_i32 s12, 0                        # memory buffer (base=0)
        s_movk_i32 s13, 0
        s_movk_i32 s14, 0xffff                  # infinite number of records
        s_mov_b32 s15, 0x8027fac                # set dstsel, nfmt and dfmt
    .endif
        
        # load valuefrom LDS via flat
        v_mov_b32 v1, s10       # local ptr base
        v_mov_b32 v0, 36        # LDS offset
        flat_load_dword v2, v[0:1]
        s_waitcnt lgkmcnt(0) & vmcnt(0)     # wait
        # store to entry in output buffer
    .if32
        buffer_store_dword v2, v0, s[12:15], s8         # store value
    .else
        v_mov_b32 v3, s8    # output buffer pointer
        v_mov_b32 v4, s9
        flat_store_dword v[3:4], v2     # store value
    .endif
        
        # load value from scratch via flat
        v_mov_b32 v1, s11       # scratch ptr base
        v_mov_b32 v0, 76
        flat_load_dword v2, v[0:1]      # load
        s_waitcnt lgkmcnt(0) & vmcnt(0) #wait
        # store to next entry in output buffer
    .if32
        buffer_store_dword v2, v0, s[12:15], s8 offset:4 # store value
    .else
    .ifarch GCN1.4
        v_add_co_u32 v3, vcc, 4, v3        # next entry in output buffer
        v_addc_co_u32 v4, vcc, 0, v4, vcc
    .else
        v_add_i32 v3, vcc, 4, v3        # next entry in output buffer
        v_addc_u32 v4, vcc, 0, v4, vcc
    .endif
        flat_store_dword v[3:4], v2     # store value
    .endif
        s_endpgm
)ffDXD";

class GenericPtr: public CLFacade
{
private:
    cl_mem outBuffer;
public:
    explicit GenericPtr(cl_uint deviceIndex);
    ~GenericPtr() = default;
    
    void run();
};

GenericPtr::GenericPtr(cl_uint deviceIndex)
            : CLFacade(deviceIndex, genericPtrSource, "genericPtr", 2)
{
    // creating buffers: two for read-only, one for output
    cl_int error;
    outBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint)*2,
                        nullptr, &error);
    if (error != CL_SUCCESS)
        throw CLError(error, "clCreateBuffer");
    memObjects.push_back(outBuffer);
}

static const cl_uint twoZeroes[2] = { 0, 0 };

void GenericPtr::run()
{
    cl_int error;
    error = clEnqueueWriteBuffer(queue, outBuffer, CL_TRUE, 0, sizeof(cl_uint)*2,
                          twoZeroes, 0, nullptr, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueWriteBuffer");
    // kernel args
    clSetKernelArg(kernels[0], 0, sizeof(cl_mem), &outBuffer);
    
    size_t workSize = 1;
    size_t localSize = 1;
    callNDRangeKernel(kernels[0], 1, nullptr, &workSize, &localSize);
    
    cl_uint outData[2];
    /// read output buffer
    error = clEnqueueReadBuffer(queue, outBuffer, CL_TRUE, 0, sizeof(cl_uint)*2,
                          outData, 0, nullptr, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueReadBuffer");
    
    if (outData[0] != 24411 || outData[1] != 15321)
    {
        std::cerr << outData[0] << "," << outData[1] << "!=24411,15321" << std::endl;
        throw Exception("CData mismatch!");
    }
    std::cout << "Results is OK" << std::endl;
}

int main(int argc, const char** argv)
try
{
    cl_uint deviceIndex = 0;
    cxuint useCL = 0;
    if (CLFacade::parseArgs("GenericPtr", "", argc, argv, deviceIndex, useCL))
        return 0;
    GenericPtr genericPtr(deviceIndex);
    genericPtr.run();
    return 0;
}
catch(const std::exception& ex)
{
    std::cerr << ex.what() << std::endl;
    return 1;
}
catch(...)
{
    std::cerr << "unknown exception!" << std::endl;
    return 1;
}
