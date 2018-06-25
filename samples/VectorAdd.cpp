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

static const char* vectorAddSource = R"ffDXD(# VectorAdd example
SMUL = 1
GCN1_2_4 = 0
.ifarch GCN1.2
    GCN1_2_4 = 1
    SMUL = 4
.elseifarch GCN1.4
    GCN1_2_4 = 1
    SMUL = 4
.endif

.ifarch GCN1.4
    # helper macros for integer add/sub instructions
    .macro VADD_U32 dest, cdest, src0, src1, mods:vararg
        v_add_co_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VADDC_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_addc_co_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUB_U32 dest, cdest, src0, src1, mods:vararg
        v_sub_co_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBB_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subb_co_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUBREV_U32 dest, cdest, src0, src1, mods:vararg
        v_subrev_co_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBBREV_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subbrev_co_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
.else
    # helper macros for integer add/sub instructions
    .macro VADD_U32 dest, cdest, src0, src1, mods:vararg
        v_add_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VADDC_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_addc_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUB_U32 dest, cdest, src0, src1, mods:vararg
        v_sub_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBB_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subb_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
    .macro VSUBREV_U32 dest, cdest, src0, src1, mods:vararg
        v_subrev_u32 \dest, \cdest, \src0, \src1 \mods
    .endm
    .macro VSUBBREV_U32 dest, cdest, src0, src1, csrc, mods:vararg
        v_subbrev_u32 \dest, \cdest, \src0, \src1, \csrc \mods
    .endm
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
    GCN12And64Bit = 0
    .if GCN1_2_4
        .if64
            GCN12And64Bit = 1
        .endif
    .endif
    
    .ifeq GCN12And64Bit # if not GCN 1.2 or not 64-bit
        s_buffer_load_dword s0, s[4:7], 4*SMUL       # s0 - local_size(0)
        s_buffer_load_dword s1, s[4:7], 24*SMUL      # s1 - global_offset(0)
        s_buffer_load_dword s4, s[8:11], 0      # s4 - n
    .if32
        # 32-bit addressing
        s_buffer_load_dword s5, s[8:11], 4*SMUL      # s5 - aBuffer offset
        s_buffer_load_dword s6, s[8:11], 8*SMUL      # s6 - bBuffer offset
        s_buffer_load_dword s7, s[8:11], 12*SMUL     # s7 - cBuffer offset
    .else
        # 64-bit addressing
        s_buffer_load_dwordx2 s[16:17], s[8:11], 4*SMUL      # s[16:17] - aBuffer offset
        s_buffer_load_dwordx2 s[18:19], s[8:11], 8*SMUL      # s[18:19] - bBuffer offset
        s_buffer_load_dwordx2 s[6:7], s[8:11], 12*SMUL       # s[6:7] - cBuffer offset
    .endif
        s_waitcnt lgkmcnt(0)                    # wait for results
        s_mul_i32 s0, s0, s12            # s0 - local_size(0)*group_id(0) (s12)
        s_add_u32 s0, s0, s1             # s0 - local_size(0)*group_id(0)+global_offset(0)
        VADD_U32 v0, vcc, s0, v0        # v0 - s0+local_id(0) -> global_id(0)
        v_cmp_gt_u32 vcc, s4, v0         # global_id(0) < n
        s_and_saveexec_b64 s[0:1], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        s_load_dwordx4 s[8:11], s[2:3], 12*8*SMUL    # s[8:11] - aBuffer descriptor
        s_load_dwordx4 s[12:15], s[2:3], 13*8*SMUL   # s[12:15] - bBuffer descriptor
    .if32
        v_lshlrev_b32 v0, 2, v0                 # v0 - global_id(0)*4
        s_waitcnt lgkmcnt(0)                    # wait for results
        # load dword from aBuf - v1, and bBuf - v2
        buffer_load_dword v1, v0, s[8:11], s5 offen        # value from aBuf
        buffer_load_dword v2, v0, s[12:15], s6 offen       # value from bBuf
    .else # 64bit
        v_lshrrev_b32 v1, 30, v0
        v_lshlrev_b32 v0, 2, v0                 # v[0:1] - global_id(0)*4
        VADD_U32 v2,vcc,s16,v0                 # v[2:3] - abuf offset + global_id(0)*2
        v_mov_b32 v3, s17                       # move to vector reg
        VADDC_U32 v3,vcc,v3,v1,vcc             # VADDC_U32 with only vector regs
        VADD_U32 v4,vcc,s18,v0                 # v[4:5] - bbuf offset + global_id(0)*2
        v_mov_b32 v5, s19                       # move to vector reg
        VADDC_U32 v5,vcc,v5,v1,vcc             # VADDC_U32 with only vector regs
        s_waitcnt lgkmcnt(0)                    # wait for results
        buffer_load_dword v2, v[2:3], s[8:11], 0 addr64     # value from aBuf
        buffer_load_dword v3, v[4:5], s[12:15], 0 addr64    # value from bBuf
    .endif
        s_load_dwordx4 s[8:11], s[2:3], 14*8*SMUL     # s[8:11] - cBuffer descriptor
        s_waitcnt vmcnt(0)
    .if32
        v_add_f32 v1, v1, v2        # add two values
        s_waitcnt lgkmcnt(0)         # wait for cBuf descriptor and offset
        buffer_store_dword v1, v0, s[8:11], s7 offen       # write value to cBuf
    .else # 64bit
        v_add_f32 v2, v2, v3        # add two values
        VADD_U32 v0,vcc,s6,v0                  # v[0:1] - cbuf offset + global_id(0)*2
        v_mov_b32 v3, s7                        # move to vector reg
        VADDC_U32 v1,vcc,v3,v1,vcc             # VADDC_U32 with only vector regs
        s_waitcnt lgkmcnt(0)         # wait for cBuf descriptor and offset
        buffer_store_dword v2, v[0:1], s[8:11], 0 addr64    # value to cBuf
    .endif
    
    .else # GCN 1.2 and 64-bit (use FLAT model)
        s_buffer_load_dword s0, s[4:7], 4*SMUL       # s0 - local_size(0)
        s_buffer_load_dword s1, s[4:7], 24*SMUL      # s1 - global_offset(0)
        s_buffer_load_dword s4, s[8:11], 0      # s4 - n
        s_buffer_load_dwordx2 s[16:17], s[8:11], 4*SMUL      # s[16:17] - aBuffer offset
        s_buffer_load_dwordx2 s[18:19], s[8:11], 8*SMUL      # s[18:19] - bBuffer offset
        s_buffer_load_dwordx2 s[6:7], s[8:11], 12*SMUL       # s[6:7] - cBuffer offset
        s_waitcnt lgkmcnt(0)                    # wait for results
        
        s_mul_i32 s0, s0, s12            # s0 - local_size(0)*group_id(0) (s12)
        s_add_u32 s0, s0, s1             # s0 - local_size(0)*group_id(0)+global_offset(0)
        VADD_U32 v0, vcc, s0, v0        # v0 - s0+local_id(0) -> global_id(0)
        v_cmp_gt_u32 vcc, s4, v0         # global_id(0) < n
        s_and_saveexec_b64 s[0:1], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        s_load_dwordx4 s[8:11], s[2:3], 12*8*SMUL    # s[8:11] - aBuffer descriptor
        s_load_dwordx4 s[12:15], s[2:3], 13*8*SMUL   # s[12:15] - bBuffer descriptor
        v_lshrrev_b32 v1, 30, v0
        v_lshlrev_b32 v0, 2, v0                 # v[0:1] - global_id(0)*4
        s_waitcnt lgkmcnt(0)                    # wait for results
        s_and_b32 s9, s9, 0xffff        # extract 48-bit address from buffer desc
        s_and_b32 s13, s13, 0xffff        # extract 48-bit address from buffer desc
        s_add_u32 s8, s8, s16       # s[8:9] - aBuffer base + aBuffer offset
        s_addc_u32 s9, s9, s17
        v_mov_b32 v3, s9            # v[2:3] - s[8:9] + global_id(0)*4
        VADD_U32 v2, vcc, s8, v0
        VADDC_U32 v3, vcc, v3, v1, vcc
        s_add_u32 s12, s12, s18       # s[12:13] - bBuffer base + bBuffer offset
        s_addc_u32 s13, s13, s19
        v_mov_b32 v5, s13            # v[4:5] - s[12:13] + global_id(0)*4
        VADD_U32 v4, vcc, s12, v0
        VADDC_U32 v5, vcc, v5, v1, vcc
        flat_load_dword v6, v[2:3]      # load a[i]
        flat_load_dword v7, v[4:5]      # load b[i]
        s_waitcnt vmcnt(0)                      # wait for data
        
        v_add_f32 v6, v6, v7                # add values
        
        s_load_dwordx4 s[8:11], s[2:3], 14*8*SMUL    # s[8:11] - cBuffer descriptor
        s_waitcnt lgkmcnt(0)                    # wait for results
        s_add_u32 s8, s8, s6         # s[12:13] - cBuffer base + cBuffer offset
        s_addc_u32 s9, s9, s7
        v_mov_b32 v3, s9             # v[4:5] - s[12:13] + global_id(0)*4
        VADD_U32 v2, vcc, s8, v0
        VADDC_U32 v3, vcc, v3, v1, vcc
        flat_store_dword v[2:3], v6
    .endif
end:
        s_endpgm
)ffDXD"  // MSVC fix: string too big
R"ffDXD(.elseiffmt amdcl2  # AMD OpenCL 2.0 code
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
    .ifarch GCN1.4
        GID = %s10
    .else
        GID = %s8
    .endif
    .if32 # 32-bit
        s_load_dwordx2 s[0:1], s[6:7], 7*SMUL   # get aBuf and bBuf pointers
        s_load_dword s4, s[4:5], 1*SMUL         # get local info dword
        s_load_dword s2, s[6:7], 0              # get global offset (32-bit)
        s_load_dword s3, s[6:7], 6*SMUL         # get n - number of elems
        s_waitcnt lgkmcnt(0)                    # wait for data
        s_and_b32 s4, s4, 0xffff            # only first localsize(0)
        s_mul_i32 s4, GID, s4                # localsize*groupId
        s_add_u32 s4, s2, s4                # localsize*groupId+offset
        VADD_U32 v0, vcc, s4, v0           # final global_id
        v_cmp_gt_u32 vcc, s3, v0            # global_id(0) < n
        s_and_saveexec_b64 s[4:5], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        
        s_movk_i32 s8, 0                        # memory buffer (base=0)
        s_movk_i32 s9, 0
        s_movk_i32 s10, -1                  # infinite number of records ((1<<32)-1)
        s_mov_b32 s11, 0x8027fac                # set dstsel, nfmt and dfmt
        
        v_lshlrev_b32 v0, 2, v0                 # v0 - global_id(0)*4
        VADD_U32 v1, vcc, s0, v0               # aBuf+get_global_id(0)
        VADD_U32 v2, vcc, s1, v0               # bBuf+get_global_id(0)
        buffer_load_dword v1, v1, s[8:11], 0 offen # load value from aBuf
        buffer_load_dword v2, v2, s[8:11], 0 offen # load value from bBuf
        s_waitcnt vmcnt(0)                      # wait for data
        v_add_f32 v1, v1, v2                    # add values
        s_load_dword s0, s[6:7], 9*SMUL         # get cBuf pointer
        s_waitcnt lgkmcnt(0)                    # wait for data
        VADD_U32 v2, vcc, s0, v0               # cBuf+get_global_id(0)
        buffer_store_dword v1, v2, s[8:11], 0 offen # store value to cBuf
        
    .else # 64-bit
        s_load_dwordx4 s[0:3], s[6:7], 14*SMUL  # get aBuf and bBuf pointers
        s_load_dword s4, s[4:5], 1*SMUL         # get local info dword
        s_load_dword s9, s[6:7], 0              # get global offset (32-bit)
        s_load_dword s5, s[6:7], 12*SMUL        # get n - number of elems
        s_waitcnt lgkmcnt(0)                    # wait for data
        s_and_b32 s4, s4, 0xffff            # only first localsize(0)
        s_mul_i32 s4, GID, s4                # localsize*groupId
        s_add_u32 s4, s9, s4                # localsize*groupId+offset
        VADD_U32 v0, vcc, s4, v0           # final global_id
        v_cmp_gt_u32 vcc, s5, v0            # global_id(0) < n
        s_and_saveexec_b64 s[4:5], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
        
        v_lshrrev_b32 v1, 30, v0
        v_lshlrev_b32 v0, 2, v0             # v[0:1] - global_id(0)*4
        VADD_U32 v2, vcc, s0, v0           # aBuf+get_global_id(0)
        v_mov_b32 v3, s1
        VADDC_U32 v3, vcc, 0, v3, vcc      # aBuf+get_global_id(0) - higher part
        VADD_U32 v4, vcc, s2, v0           # bBuf+get_global_id(0)
        v_mov_b32 v5, s3
        VADDC_U32 v5, vcc, 0, v5, vcc      # bBuf+get_global_id(0) - higher part
        flat_load_dword v2, v[2:3]          # load value from aBuf
        flat_load_dword v4, v[4:5]          # load value from bBuf
        s_waitcnt vmcnt(0) & lgkmcnt(0)     # wait for data
        v_add_f32 v2, v2, v4                # add values
        
        s_load_dwordx2 s[0:1], s[6:7], 18*SMUL  # get cBuf pointer
        s_waitcnt lgkmcnt(0)
        VADD_U32 v0, vcc, s0, v0           # cBuf+get_global_id(0)
        v_mov_b32 v3, s1
        VADDC_U32 v1, vcc, 0, v3, vcc      # cBuf+get_global_id(0) - higher part
        flat_store_dword v[0:1], v2         # store value to cBuf
    .endif
end:
        s_endpgm
)ffDXD" // MSVC fix: string too big
R"ffDXD(.elseiffmt gallium   # GalliumCompute code
.get_llvm_version LLVM_VERSION
.kernel vectorAdd
    .args
        .arg scalar,4       # uint n
        .arg global,8       # const global float* abuf
        .arg global,8       # const global float* bbuf
        .arg global,8       # global float* cbuf
        .arg griddim,4
        .arg gridoffset,4
    .config
    .if LLVM_VERSION>=40000
        .dims x
        .dx10clamp
        .ieeemode
        .default_hsa_features
    .else
        .dims xyz   # gallium set always three dimensions by Gallium
        .tgsize     # TG_SIZE_EN is always enabled by Gallium
        # arg offset in dwords:
        # 9 - n, 11 - abuf, 13 - bbuf, 15 - cbuf, 17 - griddim, 18 - gridoffset
    .endif
.text
vectorAdd:
.if LLVM_VERSION>=40000
        .skip 256
        s_load_dword s0, s[4:5], 1*SMUL        # s0 - local size X
        s_load_dword s1, s[6:7], 9*SMUL        # s1 - global offset
        s_load_dword s2, s[6:7], 0             # s2 - n
        s_waitcnt lgkmcnt(0)
        s_and_b32 s0, s0, 0xffff           # only local size X
        s_mul_i32 s0, s0, s8               # s0 - localSize*groupId
        s_add_i32 s0, s0, s1               # s0 - localSize*groupId + offset
        VADD_U32 v0, vcc, s0, v0          # v0 - localId + localSize*groupId + offset
        v_cmp_gt_u32 vcc, s2, v0           # compare n > id
        s_and_saveexec_b64 s[0:1], vcc
        s_cbranch_execz end             # end
        v_mov_b32 v1, 0
    .ifeq GCN1_2_4  # GCN 1.0/1.1
        s_load_dwordx2  s[0:1], s[6:7], 2*SMUL      # prepare Aptr buffer
        s_mov_b32       s2, 0
        s_mov_b32       s3, 0xf000
        s_load_dwordx2  s[8:9], s[6:7], 4*SMUL      # prepare Bptr buffer
        s_mov_b64 s[10:11], s[2:3]
        s_load_dwordx2  s[12:13], s[6:7], 6*SMUL    # prepare Cptr buffer
        s_mov_b64 s[14:15], s[2:3]
        v_lshl_b64 v[0:1], v[0:1], 2          # make vector offset
        s_waitcnt lgkmcnt(0)
        buffer_load_dword v2, v[0:1], s[0:3], 0 addr64      # load a value
        buffer_load_dword v3, v[0:1], s[8:11], 0 addr64     # load b value
        s_waitcnt vmcnt(0)
        v_add_f32 v2, v3, v2
        buffer_store_dword v2, v[0:1], s[12:15], 0 addr64
    .else   # GCN 1.2
        s_load_dwordx4  s[0:3], s[6:7], 2*SMUL      # load Aptr and Bptr
        s_load_dwordx2  s[4:5], s[6:7], 6*SMUL      # load Cptr
        s_waitcnt lgkmcnt(0)
        v_lshlrev_b64 v[0:1], 2, v[0:1]     # make vector offset
        VADD_U32 v2, vcc, s0, v0           # Aptr + offset
        v_mov_b32 v3, s1
        VADDC_U32 v3, vcc, v3, v1, vcc
        VADD_U32 v4, vcc, s2, v0           # Bptr + offset
        v_mov_b32 v5, s3
        VADDC_U32 v5, vcc, v5, v1, vcc
        flat_load_dword v6, v[2:3]          # load A value
        flat_load_dword v7, v[4:5]          # load B value
        s_waitcnt vmcnt(0)          # wait for values
        VADD_U32 v0, vcc, s4, v0           # Cptr + offset
        v_mov_b32 v2, s5
        VADDC_U32 v1, vcc, v2, v1, vcc
        v_add_f32 v6, v7, v6            # add values
        flat_store_dword v[0:1], v6
    .endif
.else
        s_load_dword s2, s[0:1], 6*SMUL         # s2 - local_size(0)
        s_load_dword s3, s[0:1], 9*SMUL         # s3 - n
        s_load_dword s1, s[0:1], 18*SMUL        # s1 - global_offset(0)
        s_load_dwordx2 s[8:9], s[0:1], 11*SMUL       # s[8:9] - aBuffer pointer
        s_load_dwordx2 s[12:13], s[0:1], 13*SMUL     # s[12:13] - bBuffer pointer
        s_load_dwordx2 s[6:7], s[0:1], 15*SMUL       # s[6:7] - cBuffer pointer
        s_waitcnt lgkmcnt(0)            # wait for results
        s_mul_i32 s0, s2, s4            # s0 - local_size(0)*group_id(0)
        s_add_u32 s0, s0, s1            # s0 - local_size(0)*group_id(0)+global_offset(0)
        VADD_U32 v0, vcc, s0, v0       # v0 - s0+local_id(0) -> global_id(0)
        v_cmp_gt_u32 vcc, s3, v0                # global_id(0) < n
        s_and_saveexec_b64 s[0:1], vcc          # lock all threads with id>=n
        s_cbranch_execz end                     # no active threads, we jump to end
    .ifeq GCN1_2_4    # GCN 1.0/1.1
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
    .else # GCN 1.2
        v_lshrrev_b32 v1, 30, v0    # v[0:1] - global_id(0)*2
        v_lshlrev_b32 v0, 2, v0
        v_mov_b32 v3, s9
        VADD_U32 v2, vcc, s8, v0           # add gid*4 to aBuffer pointer
        VADDC_U32 v3, vcc, v3, v1, vcc
        v_mov_b32 v5, s13
        VADD_U32 v4, vcc, s12, v0           # add gid*4 to bBuffer pointer
        VADDC_U32 v5, vcc, v5, v1, vcc
        flat_load_dword v6, v[2:3]          # load value from aBuffer
        flat_load_dword v7, v[4:5]          # load value from bBuffer
        s_waitcnt vmcnt(0)
        v_add_f32 v6, v6, v7                # add values
        v_mov_b32 v3, s7
        VADD_U32 v2, vcc, s6, v0           # add gid*4 to cBuffer pointer
        VADDC_U32 v3, vcc, v3, v1, vcc
        flat_store_dword v[2:3], v6
    .endif
.endif
end:
        s_endpgm

.elseiffmt rocm   # ROCm code
.newbinfmt
.tripple "amdgcn-amd-amdhsa-amdgizcl"
.kernel vectorAdd
    .config
        .dims x
        .dx10clamp
        .ieeemode
        .codeversion 1, 1
        .default_hsa_features
        .call_convention 0xffffffff
        .pgmrsrc2 0x40      # trap present
        # ROCm metadata
        .md_language "OpenCL C", 1, 2
        .md_kernarg_segment_align 8
        .arg n, "uint", 4, 4, value, u32
        .arg a, "float*", 8, 8, globalbuf, f32, global, default const
        .arg b, "float*", 8, 8, globalbuf, f32, global, default const
        .arg c, "float*", 8, 8, globalbuf, f32, global, default
        .arg , "", 8, 8, gox, i64
        .arg , "", 8, 8, goy, i64
        .arg , "", 8, 8, goz, i64
.text
vectorAdd:
.skip 256
# code from CLANG OpenCL compiler
        s_load_dword    s2, s[4:5], 0x1*SMUL
        s_load_dword    s3, s[6:7], 0x0
        s_load_dwordx2  s[0:1], s[6:7], 0x8*SMUL
        s_waitcnt       lgkmcnt(0)
        s_and_b32       s2, s2, 0xffff
        s_mul_i32       s8, s8, s2
        VADD_U32        v0, vcc, s8, v0
        v_mov_b32       v1, s1
        VADD_u32        v0, vcc, s0, v0
        VADDC_U32       v1, vcc, 0, v1, vcc
        v_cmp_gt_u32    vcc, s3, v0
        s_and_saveexec_b64 s[0:1], vcc
        s_cbranch_execz end
        s_load_dwordx2  s[0:1], s[6:7], 0x2*SMUL
        s_load_dwordx2  s[2:3], s[6:7], 0x4*SMUL
.if GCN1_2_4
        v_lshlrev_b64   v[0:1], 2, v[0:1]
.else
        v_lshl_b64      v[0:1], v[0:1], 2
.endif
        s_load_dwordx2  s[4:5], s[6:7], 0x6*SMUL
        v_and_b32       v5, 3, v1
        s_waitcnt       lgkmcnt(0)
        v_mov_b32       v2, s1
        VADD_U32        v1, vcc, s0, v0
        VADDC_U32       v2, vcc, v2, v5, vcc
        v_mov_b32       v4, s3
        VADD_U32        v3, vcc, s2, v0
        VADDC_U32       v4, vcc, v4, v5, vcc
        flat_load_dword v3, v[3:4]
        flat_load_dword v2, v[1:2]
        v_mov_b32       v1, s5
        VADD_U32        v0, vcc, s4, v0
        VADDC_U32       v1, vcc, v1, v5, vcc
        s_waitcnt       vmcnt(0) & lgkmcnt(0)
        v_add_f32       v2, v2, v3
        flat_store_dword v[0:1], v2
end:
        s_endpgm
.else
        .error "Unsupported binary format"
.endif
)ffDXD";

class VectorAdd: public CLFacade
{
private:
    size_t elemsNum;
    cl_mem aBuffer, bBuffer, cBuffer;
public:
    explicit VectorAdd(cl_uint deviceIndex, cxuint useCL, size_t elemsNum);
    ~VectorAdd() = default;
    
    void run();
};

VectorAdd::VectorAdd(cl_uint deviceIndex, cxuint useCL, size_t _elemsNum)
            : CLFacade(deviceIndex, vectorAddSource, "vectorAdd", useCL),
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
    
    // checking results (use threshold 5.0e-7)
    for (size_t i = 0; i < elemsNum; i++)
    {
        const float expected = aData[i]+bData[i];
        if (::fabs(expected-cData[i]) >= 5.0e-7)
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
    cxuint useCL = 0;
    if (CLFacade::parseArgs("VectorAdd", "[ELEMSNUM]", argc, argv, deviceIndex, useCL))
        return 0;
    size_t elemsNum = 100;
    const char* end;
    if (argc >= 3)
        elemsNum = cstrtovCStyle<size_t>(argv[2], nullptr, end);
    
    VectorAdd vectorAdd(deviceIndex, useCL, elemsNum);
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
