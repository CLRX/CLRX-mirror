/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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

static const char* vectorAddSource = R"ffDXD(# VectorAdd example
.ifarch gcn1.2
    .error "Unsupported GCN1.2 architecture"
.endif
.iffmt amd    # if AMD Catalyst
.kernel vectorAdd
    .config
        .dims x     # required, this kernel uses only one dimension
        .uavid 11   # force to UAVID=12 for first buffer
        .arg n, uint                        # argument uint n
        .arg aBuf, float*, global, const    # argument const float* aBuf, uavid=12
        .arg bBuf, float*, global, const    # argument const float* bBuf, uavid=13
        .arg cBuf, float*, global           # argument float* cBuf      , uavid=14
        # argument offset in dwords:
        # 0 - n, 4 - aBuf, 8 - bBuf, 12 - cBuf
        .userdata ptr_uav_table, 0, 2, 2        # s[2:3] - buffer's table
        .userdata imm_const_buffer, 0, 4, 4     # s[4:7] - kernel setup const buffer
        .userdata imm_const_buffer, 1, 8, 4     # s[8:11] - put kernel argument const buffer
    .text
        s_buffer_load_dword s0, s[4:7], 4       # s0 - local_size(0)
        s_buffer_load_dword s1, s[4:7], 24      # s1 - global_offset(0)
        s_buffer_load_dword s4, s[8:11], 0      # s4 - n
    .if32
        # 32-bit addressing
        s_buffer_load_dword s5, s[8:11], 4      # s5 - aBuffer offset
        s_buffer_load_dword s6, s[8:11], 8      # s6 - bBuffer offset
        s_buffer_load_dword s7, s[8:11], 12     # s7 - cBuffer offset
    .else
        # 64-bit addressing
        s_buffer_load_dwordx2 s[16:17], s[8:11], 4      # s[16:17] - aBuffer offset
        s_buffer_load_dwordx2 s[18:19], s[8:11], 8      # s[18:19] - bBuffer offset
        s_buffer_load_dwordx2 s[6:7], s[8:11], 12       # s[6:7] - cBuffer offset
    .endif
        s_waitcnt lgkmcnt(0)                    # wait for results
        s_mul_i32 s0, s0, s12            # s0 - local_size(0)*group_id(0) (s12)
        s_add_u32 s0, s0, s1             # s0 - local_size(0)*group_id(0)+global_offset(0)
        v_add_i32 v0, vcc, s0, v0        # v0 - s0+local_id(0) -> global_id(0)
        v_cmp_gt_u32 vcc, s4, v0         # global_id(0) < n
        s_and_saveexec_b64 s[0:1], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        s_load_dwordx4 s[8:11], s[2:3], 12*8    # s[8:11] - aBuffer descriptor
        s_load_dwordx4 s[12:15], s[2:3], 13*8   # s[4:7] - bBuffer descriptor
    .if32
        v_lshlrev_b32 v0, 2, v0                 # v0 - global_id(0)*4
        s_waitcnt lgkmcnt(0)                    # wait for results
        # load dword from aBuf - v1, and bBuf - v2
        buffer_load_dword v1, v0, s[8:11], s5 offen        # value from aBuf
        buffer_load_dword v2, v0, s[12:15], s6 offen       # value from bBuf
    .else # 64bit
        v_lshrrev_b32 v1, 30, v0
        v_lshlrev_b32 v0, 2, v0                 # v[0:1] - global_id(0)*4
        v_add_i32 v2,vcc,s16,v0                 # v[2:3] - abuf offset + global_id(0)*2
        v_mov_b32 v3, s17                       # move to vector reg
        v_addc_u32 v3,vcc,v3,v1,vcc             # v_addc_u32 with only vector regs
        v_add_i32 v4,vcc,s18,v0                 # v[4:5] - bbuf offset + global_id(0)*2
        v_mov_b32 v5, s19                       # move to vector reg
        v_addc_u32 v5,vcc,v5,v1,vcc             # v_addc_u32 with only vector regs
        s_waitcnt lgkmcnt(0)                    # wait for results
        buffer_load_dword v2, v[2:3], s[8:11], 0 addr64     # value from aBuf
        buffer_load_dword v3, v[4:5], s[12:15], 0 addr64    # value from bBuf
    .endif
        s_load_dwordx4 s[8:11], s[2:3], 14*8     # s[4:7] - cBuffer descriptor
        s_waitcnt vmcnt(0)
    .if32
        v_add_f32 v1, v1, v2        # add two values
        s_waitcnt lgkmcnt(0)         # wait for cBuf descriptor and offset
        buffer_store_dword v1, v0, s[8:11], s7 offen       # write value to cBuf
    .else # 64bit
        v_add_f32 v2, v2, v3        # add two values
        v_add_i32 v0,vcc,s6,v0                  # v[0:1] - cbuf offset + global_id(0)*2
        v_mov_b32 v3, s7                        # move to vector reg
        v_addc_u32 v1,vcc,v3,v1,vcc             # v_addc_u32 with only vector regs
        s_waitcnt lgkmcnt(0)         # wait for cBuf descriptor and offset
        buffer_store_dword v2, v[0:1], s[8:11], 0 addr64    # value to cBuf
    .endif
end:
        s_endpgm
.elseiffmt amdcl2  # AMD OpenCL 2.0 code
.kernel vectorAdd
    .config
        .dims x
        .useargs
        .usesetup
        .setupargs
        .arg n, uint                        # argument uint n
        .arg aBuf, float*, global, const    # argument const float* aBuf
        .arg bBuf, float*, global, const    # argument const float* bBuf
        .arg cBuf, float*, global           # argument float* cBuf
    .text
    .if32 # 32-bit
        s_load_dwordx2 s[0:1], s[6:7], 7        # get aBuf and bBuf pointers
        s_load_dword s4, s[4:5], 1              # get local info dword
        s_load_dword s2, s[6:7], 0              # get global offset (32-bit)
        s_load_dword s3, s[6:7], 6              # get n - number of elems
        s_waitcnt lgkmcnt(0)                    # wait for data
        s_and_b32 s4, s4, 0xffff            # only first localsize(0)
        s_mul_i32 s4, s8, s4                # localsize*groupId
        s_add_u32 s4, s2, s4                # localsize*groupId+offset
        v_add_i32 v0, vcc, s4, v0           # final global_id
        v_cmp_gt_u32 vcc, s3, v0            # global_id(0) < n
        s_and_saveexec_b64 s[4:5], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        
        s_movk_i32 s8, 0                        # memory buffer (base=0)
        s_movk_i32 s9, 0
        s_movk_i32 s10, 0xffff                  # infinite number of records
        s_mov_b32 s11, 0x8027fac                # set dstsel, nfmt and dfmt
        
        v_lshlrev_b32 v0, 2, v0                 # v0 - global_id(0)*4
        v_add_i32 v1, vcc, s0, v0               # aBuf+get_global_id(0)
        v_add_i32 v2, vcc, s1, v0               # bBuf+get_global_id(0)
        buffer_load_dword v1, v1, s[8:11], 0 offen # load value from aBuf
        buffer_load_dword v2, v2, s[8:11], 0 offen # load value from bBuf
        s_waitcnt vmcnt(0)                      # wait for data
        v_add_f32 v1, v1, v2                    # add values
        s_load_dword s0, s[6:7], 9              # get cBuf pointer
        s_waitcnt lgkmcnt(0)                    # wait for data
        v_add_i32 v2, vcc, s0, v0               # cBuf+get_global_id(0)
        buffer_store_dword v1, v2, s[8:11], 0 offen # store value to cBuf
        
    .else # 64-bit
        s_load_dwordx4 s[0:3], s[6:7], 14       # get aBuf and bBuf pointers
        s_load_dword s4, s[4:5], 1              # get local info dword
        s_load_dword s9, s[6:7], 0              # get global offset (32-bit)
        s_load_dword s5, s[6:7], 12             # get n - number of elems
        s_waitcnt lgkmcnt(0)                    # wait for data
        s_and_b32 s4, s4, 0xffff            # only first localsize(0)
        s_mul_i32 s4, s8, s4                # localsize*groupId
        s_add_u32 s4, s9, s4                # localsize*groupId+offset
        v_add_i32 v0, vcc, s4, v0           # final global_id
        v_cmp_gt_u32 vcc, s5, v0            # global_id(0) < n
        s_and_saveexec_b64 s[4:5], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        
        v_lshrrev_b32 v1, 30, v0
        v_lshlrev_b32 v0, 2, v0             # v[0:1] - global_id(0)*4
        v_add_i32 v2, vcc, s0, v0           # aBuf+get_global_id(0)
        v_mov_b32 v3, s1
        v_addc_u32 v3, vcc, 0, v3, vcc      # aBuf+get_global_id(0) - higher part
        v_add_i32 v4, vcc, s2, v0           # bBuf+get_global_id(0)
        v_mov_b32 v5, s3
        v_addc_u32 v5, vcc, 0, v5, vcc      # bBuf+get_global_id(0) - higher part
        flat_load_dword v2, v[2:3]          # load value from aBuf
        flat_load_dword v4, v[4:5]          # load value from bBuf
        s_waitcnt vmcnt(0) & lgkmcnt(0)     # wait for data
        v_add_f32 v2, v2, v4                # add values
        
        s_load_dwordx2 s[0:1], s[6:7], 18       # get cBuf pointer
        s_waitcnt lgkmcnt(0)
        v_add_i32 v0, vcc, s0, v0           # cBuf+get_global_id(0)
        v_mov_b32 v3, s1
        v_addc_u32 v1, vcc, 0, v3, vcc      # cBuf+get_global_id(0) - higher part
        flat_store_dword v[0:1], v2         # store value to cBuf
    .endif
end:
        s_endpgm
.else   # GalliumCompute code
.kernel vectorAdd
    .args
        .arg scalar,4       # uint n
        .arg global,8       # const global float* abuf
        .arg global,8       # const global float* bbuf
        .arg global,8       # global float* cbuf
        .arg griddim,4
        .arg gridoffset,4
    .config
        .dims xyz   # gallium set always three dimensions by Gallium
        .tgsize     # TG_SIZE_EN is always enabled by Gallium
        # arg offset in dwords:
        # 9 - n, 11 - abuf, 13 - bbuf, 15 - cbuf, 17 - griddim, 18 - gridoffset
.text
vectorAdd:
        s_load_dword s2, s[0:1], 6              # s2 - local_size(0)
        s_load_dword s3, s[0:1], 9              # s3 - n
        s_load_dword s1, s[0:1], 18             # s1 - global_offset(0)
        s_load_dwordx2 s[8:9], s[0:1], 11       # s[8:9] - aBuffer pointer
        s_load_dwordx2 s[12:13], s[0:1], 13     # s[12:13] - bBuffer pointer
        s_load_dwordx2 s[6:7], s[0:1], 15       # s[6:7] - cBuffer pointer
        s_waitcnt lgkmcnt(0)            # wait for results
        s_mul_i32 s0, s2, s4            # s0 - local_size(0)*group_id(0)
        s_add_u32 s0, s0, s1            # s0 - local_size(0)*group_id(0)+global_offset(0)
        v_add_i32 v0, vcc, s0, v0       # v0 - s0+local_id(0) -> global_id(0)
        v_cmp_gt_u32 vcc, s3, v0                # global_id(0) < n
        s_and_saveexec_b64 s[0:1], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        # generate buffer descriptors
        s_mov_b32 s4, s6            # cbuffer - s[4:7]
        s_mov_b32 s5, s7
        s_mov_b32 s6, -1            # no limit
        s_mov_b32 s7, 0xf000        # default num_format
        s_mov_b32 s10, s6           # abuffer - s[8:11]
        s_mov_b32 s11, s7
        s_mov_b32 s14, s6           # bbuffer - s[12:15]
        s_mov_b32 s15, s7
        v_lshrrev_b32 v1, 30, v0    # v[0:1] - global_id(0)*2
        v_lshlrev_b32 v0, 2, v0
        buffer_load_dword v2, v[0:1], s[8:11], 0 addr64     # load float from aBuffer
        buffer_load_dword v3, v[0:1], s[12:15], 0 addr64    # load float from aBuffer
        s_waitcnt vmcnt(0)          # wait for data from aBuffer and bBuffer
        v_add_f32 v2, v2, v3        # add two floats
        buffer_store_dword v2, v[0:1], s[4:7], 0 addr64     # store result
end:
        s_endpgm
.endif
)ffDXD";

class VectorAdd: public CLFacade
{
private:
    size_t elemsNum;
    cl_mem aBuffer, bBuffer, cBuffer;
public:
    explicit VectorAdd(cl_uint deviceIndex, bool useCL2, size_t elemsNum);
    ~VectorAdd() = default;
    
    void run();
};

VectorAdd::VectorAdd(cl_uint deviceIndex, bool useCL2, size_t _elemsNum)
            : CLFacade(deviceIndex, vectorAddSource, "vectorAdd", useCL2),
              elemsNum(_elemsNum)
{   // creating buffers: two for read-only, one for output
    cl_int error;
    aBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float)*elemsNum,
                        nullptr, &error);
    if (error != CL_SUCCESS)
        throw CLError(error, "clCreateBuffer");
    memObjects.push_back(aBuffer);
    bBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float)*elemsNum,
                        nullptr, &error);
    if (error != CL_SUCCESS)
        throw CLError(error, "clCreateBuffer");
    memObjects.push_back(bBuffer);
    cBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*elemsNum,
                        nullptr, &error);
    if (error != CL_SUCCESS)
        throw CLError(error, "clCreateBuffer");
    memObjects.push_back(cBuffer);
}

void VectorAdd::run()
{
    cl_int error;
    std::unique_ptr<float[]> aData(new float[elemsNum]);
    std::unique_ptr<float[]> bData(new float[elemsNum]);
    std::unique_ptr<float[]> cData(new float[elemsNum]);
    
    /// generate input data
    float preva = 1.0;
    float prevb = 1.0;
    for (size_t i = 0; i < elemsNum; i++)
    {
        aData[i] = preva + float(i)*0.5f/elemsNum;
        bData[i] = prevb + float(i)*float(i)*0.6f/(float(elemsNum)*elemsNum);
        cData[i] = 0.0f; // zeroing output
    }
    /// write to buffers
    error = clEnqueueWriteBuffer(queue, aBuffer, CL_TRUE, 0, sizeof(float)*elemsNum,
                          aData.get(), 0, nullptr, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueWriteBuffer");
    error = clEnqueueWriteBuffer(queue, bBuffer, CL_TRUE, 0, sizeof(float)*elemsNum,
                          bData.get(), 0, nullptr, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueWriteBuffer");
    // zeroing content of cBuffer
    error = clEnqueueWriteBuffer(queue, cBuffer, CL_TRUE, 0, sizeof(float)*elemsNum,
                          cData.get(), 0, nullptr, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueWriteBuffer");
    
    cl_uint argElemsNum = elemsNum;
    clSetKernelArg(kernels[0], 0, sizeof(cl_uint), &argElemsNum);
    clSetKernelArg(kernels[0], 1, sizeof(cl_mem), &aBuffer);
    clSetKernelArg(kernels[0], 2, sizeof(cl_mem), &bBuffer);
    clSetKernelArg(kernels[0], 3, sizeof(cl_mem), &cBuffer);
    /// execute kernel
    size_t workGroupSize, workGroupSizeMul;
    getKernelInfo(kernels[0], workGroupSize, workGroupSizeMul);
    
    size_t workSize = (elemsNum+workGroupSize-1)/workGroupSize*workGroupSize;
    std::cout << "WorkSize: " << workSize << ", LocalSize: " << workGroupSize << std::endl;
    callNDRangeKernel(kernels[0], 1, nullptr, &workSize, &workGroupSize);
    
    /// read output buffer
    error = clEnqueueReadBuffer(queue, cBuffer, CL_TRUE, 0, sizeof(float)*elemsNum,
                          cData.get(), 0, nullptr, nullptr);
    if (error != CL_SUCCESS)
        throw CLError(error, "clEnqueueReadBuffer");
    
    for (size_t i = 0; i < elemsNum; i++)
    {
        const float expected = aData[i]+bData[i];
        if (::fabs(expected-cData[i]) >= 1.0e-7)
        {
            std::cerr << i << ": " << aData[i] << " + " << bData[i] <<
                ": " << expected << "!=" << cData[i] << "\n";
            throw Exception("CData mismatch!");
        }
    }
    // print result
    for (size_t i = 0; i < elemsNum; i++)
        std::cout << i << ": " << aData[i] << " + " << bData[i] <<
                " = " << cData[i] << "\n";
    std::cout.flush();
}

int main(int argc, const char** argv)
try
{
    cl_uint deviceIndex = 0;
    bool useCL2 = false;
    if (CLFacade::parseArgs("VectorAdd", "[ELEMSNUM]", argc, argv, deviceIndex, useCL2))
        return 0;
    size_t elemsNum = 100;
    const char* end;
    if (argc >= 3)
        elemsNum = cstrtovCStyle<size_t>(argv[2], nullptr, end);
    
    VectorAdd vectorAdd(deviceIndex, useCL2, elemsNum);
    vectorAdd.run();
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
