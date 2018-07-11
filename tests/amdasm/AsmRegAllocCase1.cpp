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
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"
#include "AsmRegAlloc.h"

const AsmCodeStructCase codeStructTestCases1Tbl[] =
{
    {   // 0
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n",
        {
            { 0, 8, { }, false, false, true }
        },
        true, ""
    },
    {   // 1
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        ".cf_end\n"
        ".cf_start\n"
        "v_mov_b32 v4, v2\n"
        "v_mov_b32 v8, v3\n"
        "v_mov_b32 v8, v3\n"
        ".cf_end\n",
        {
            { 0, 8, { }, false, false, true },
            { 8, 20, { }, false, false, true }
        },
        true, ""
    },
    {   // 2 - ignore cf_start to cf_end (except first)
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        ".cf_start\n"
        "v_mov_b32 v4, v2\n"
        "v_mov_b32 v8, v3\n"
        ".cf_start\n"
        "v_mov_b32 v8, v3\n"
        ".cf_end\n"
        ".cf_start\n"
        "v_and_b32 v6, v1, v2\n"
        ".cf_start\n"
        "v_and_b32 v9, v1, v3\n"
        ".cf_end\n",
        {
            { 0, 20, { }, false, false, true },
            { 20, 28, { }, false, false, true }
        },
        true, ""
    },
    {   // 3 - ignore cf_end
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        ".cf_end\n"
        "v_mov_b32 v4, v2\n"
        "v_mov_b32 v8, v3\n"
        "v_mov_b32 v8, v3\n"
        ".cf_end\n"
        ".cf_start\n"
        "v_and_b32 v9, v1, v3\n"
        ".cf_end\n",
        {
            { 0, 8, { }, false, false, true },
            { 20, 24, { }, false, false, true }
        },
        true, ""
    },
    {   /* 4 - cond jump */
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        "label2:\n"
        "v_mov_b32 v1, v3\n"
        "v_mov_b32 v1, v3\n"
        "s_cbranch_vccz label2\n"
        "s_xor_b32 s3, s5, s8\n"
        "s_endpgm\n",
        {
            { 0, 8, { }, false, false, false },
            { 8, 20,
              { { 1, false }, { 2, false } },
              false, false, false },
            { 20, 28, { }, false, false, true }
        },
        true, ""
    },
    {   /* 5 - jump */
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        "label2:\n"
        "v_mov_b32 v1, v3\n"
        "v_mov_b32 v1, v3\n"
        "s_branch label2\n"
        "s_xor_b32 s3, s5, s8\n"
        "s_endpgm\n",
        {
            { 0, 8, { }, false, false, false },
            { 8, 20,
              { { 1, false } },
              false, false, true }
        },
        true, ""
    },
    {
        /* 6 - jump */
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        "lstart:\n"
        ".cf_jump l0, l1, l2\n"
        "s_setpc_b64 s[0:1]\n"
        "l0:\n"
        "v_add_f32 v1, v2, v3\n"
        "s_branch lend\n"
        "l1:\n"
        "v_add_f32 v1, v2, v3\n"
        "s_branch lend\n"
        "l2:\n"
        "v_add_f32 v1, v2, v3\n"
        "s_branch lend\n"
        "lend:\n"
        "s_cbranch_vccz lstart\n"
        "s_endpgm\n",
        {
            { 0, 8, { }, false, false, false },
            { 8, 12,
              { { 2, false }, { 3, false }, { 4, false } },
              false, false, true },
            { 12, 20,
              { { 5, false } },
              false, false, true },
            { 20, 28,
              { { 5, false } },
              false, false, true },
            { 28, 36,
              { { 5, false } },
              false, false, true },
            { 36, 40,
              { { 1, false }, { 6, false } },
              false, false, false },
            { 40, 44, { }, false, false, true }
        },
        true, ""
    },
    {   /* 7 - subroutines */
        R"ffDXD(
        v_mov_b32 v1, v2
        v_mov_b32 v1, v3
        .cf_call l0, l1, l2
        s_setpc_b64 s[0:1]
        v_sub_f32 v1, v3, v6    # 12
        s_branch j0
b0:     v_add_u32 v4, vcc, v6, v11  # 20
        v_mac_f32 v6, v6, v6
        s_endpgm
l0:     # 32
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
l1:     # 44
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
l2:     # 60
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
j0:     # 84
        v_lshrrev_b32 v4, 3, v2
        v_lshrrev_b32 v4, 3, v2
        s_branch b0
)ffDXD",
        {
            { 0, 12,
                { { 3, true }, { 4, true }, { 5, true } },
                true, false, false },
            { 12, 20, { { 6, false } }, false, false, true },
            { 20, 32, { }, false, false, true },
            { 32, 44, { }, false, true, true },
            { 44, 60, { }, false, true, true },
            { 60, 84, { }, false, true, true },
            { 84, 96, { { 2, false } }, false, false, true }
        },
        true, ""
    },
    {   /* 8 - subroutines 2 - more complex */
        R"ffDXD(
        v_mov_b32 v1, v2    # 0
        v_mov_b32 v1, v3
loop:   s_mov_b32 s5, s0    # 8
        .cf_call c1,c2,c3
        s_swappc_b64 s[0:1], s[0:1]
        v_and_b32 v4, v5, v4    # 16
        v_and_b32 v4, v5, v3
        v_and_b32 v4, v5, v4
        .cf_call c5,c6
        s_swappc_b64 s[0:1], s[0:1]
        v_and_b32 v4, v5, v3    # 32
        v_and_b32 v4, v5, v9
        s_add_i32 s3, s3, s1
        s_cbranch_vccnz loop
        v_mac_f32 v4, v6, v7    # 48
        s_endpgm
.p2align 4
c1:     v_xor_b32 v3, v4, v3    # 64
        v_xor_b32 v3, v4, v7
        v_xor_b32 v3, v4, v8
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
.p2align 4
c2:     v_xor_b32 v3, v4, v3    # 80
        v_xor_b32 v3, v4, v7
c5:     v_xor_b32 v3, v4, v8    # 88
        .cf_call c4
        s_swappc_b64 s[0:1], s[0:1]
        v_or_b32 v3, v11, v9    # 96
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
.p2align 4
c3:     v_xor_b32 v3, v4, v3    # 112
        v_xor_b32 v3, v4, v8
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
.p2align 4
c4:     v_xor_b32 v3, v4, v3    # 128
        v_xor_b32 v3, v9, v8
c6: loop2:
        v_xor_b32 v3, v9, v8    # 136
        v_xor_b32 v3, v9, v8
        s_add_i32 s4, 1, s34
        s_cbranch_vccz loop2
        v_lshlrev_b32 v3, 4, v2 # 152
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
)ffDXD",
        {
            { 0, 8, { }, false, false, false },
            { 8, 16,
                { { 5, true }, { 6, true }, { 9, true } },
                true, false, false },
            // 2
            { 16, 32,
                { { 7, true }, { 11, true } },
                true, false, false },
            { 32, 48,
                { { 1, false }, { 4, false } },
                false, false, false },
            // 4
            { 48, 56, { }, false, false, true },
            // 5 - c1 subroutine
            { 64, 80, { }, false, true, true },
            // 6 - c2 subroutine
            { 80, 88, { }, false, false, false },
            { 88, 96, { { 10, true } },
                true, false, false },
            { 96, 104, { }, false, true, true },
            // 9 - c3 subroutine
            { 112, 124, { }, false, true, true },
            // 10 - c4 subroutine
            { 128, 136, { }, false, false, false },
            { 136, 152,
                { { 11, false }, { 12, false } },
                false, false, false },
            { 152, 160, { }, false, true, true }
        },
        true, ""
    },
    {   // 9 - switch
        R"ffDXD(
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            .cf_jump j1, j2, j3, j4
            s_setpc_b64 s[0:1]
            
j1:         v_xor_b32 v3, v9, v8    # 12
            v_xor_b32 v3, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j2:         v_xor_b32 v3, v9, v8    # 28
            v_xor_b32 v1, v5, v8
            v_and_b32 v2, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j3:         v_xor_b32 v3, v9, v8    # 48
            v_xor_b32 v1, v5, v8
            s_branch b0

j4:         v_xor_b32 v3, v9, v8    # 60
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
b0:         s_sub_u32 s0, s1, s2    # 72
            s_endpgm
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 2, false }, { 3, false }, { 4, false } },
                false, false, true },
            { 12, 28, { { 5, false } }, false, false, true },
            { 28, 48, { { 5, false } }, false, false, true },
            { 48, 60, { { 5, false } }, false, false, true },
            { 60, 72, { }, false, false, false },
            { 72, 80, { }, false, false, true }
        },
        true, ""
    },
    {   // 10 - multiple kernels
        R"ffDXD(
            .cf_start
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            .cf_end
.p2align 6
            .cf_start
            v_xor_b32 v3, v9, v8
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
            s_endpgm
.p2align 6
            .cf_start       # empty block, ignored
            .cf_end
            .cf_start
            v_xor_b32 v3, v9, v8
            v_xor_b32 v1, v5, v8
            v_xor_b32 v3, v9, v8
            v_or_b32 v3, v9, v8
            v_or_b32 v3, v9, v8
            s_endpgm
            .cf_end
            .cf_start
            v_or_b32 v3, v9, v8
            .cf_end
            .cf_start   # empty block, ignored
            .cf_end
)ffDXD",
        {
            { 0, 8, { }, false, false, true },
            { 64, 84, { }, false, false, true },
            { 128, 152, { }, false, false, true },
            { 152, 156, { }, false, false, true }
        },
        true, ""
    },
    {   // 11 - different type of jumps
        R"ffDXD(
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            .cf_jump j1, j4
            .cf_cjump j2, j3
            s_setpc_b64 s[0:1]
            
            v_mov_b32 v5, v3
            .cf_jump j1, j4
            .cf_cjump j2, j3
            s_setpc_b64 s[0:1]
            .cf_end
            
j1:         v_xor_b32 v3, v9, v8    # 20
            v_xor_b32 v3, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j2:         v_xor_b32 v3, v9, v8    # 36
            v_xor_b32 v1, v5, v8
            v_and_b32 v2, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j3:         v_xor_b32 v3, v9, v8    # 56
            v_xor_b32 v1, v5, v8
            s_branch b0

j4:         v_xor_b32 v3, v9, v8    # 68
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
b0:         s_sub_u32 s0, s1, s2    # 80
            s_endpgm
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 2, false }, { 3, false },
                  { 4, false }, { 5, false } },
                false, false, false },
            { 12, 20,
                { { 2, false }, { 3, false }, { 4, false }, { 5, false } },
                false, false, true }, // have cf_end, force haveEnd
            { 20, 36, { { 6, false } }, false, false, true },
            { 36, 56, { { 6, false } }, false, false, true },
            { 56, 68, { { 6, false } }, false, false, true },
            { 68, 80, { }, false, false, false },
            { 80, 88, { }, false, false, true }
        },
        true, ""
    },
    {   // 12 - many jumps to same place
        R"ffDXD(
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            s_cbranch_vccz j1
            v_mov_b32 v4, v3        # 12
            v_mov_b32 v4, v3
            s_cbranch_execz j1
            v_mov_b32 v4, v3        # 24
            v_mov_b32 v4, v3
            s_cbranch_execnz b0
            s_branch j2             # 36
b0:         v_mov_b32 v4, v3        # 40
            v_mov_b32 v5, v3
            v_mov_b32 v5, v4
            s_branch j2
j1:         v_max_u32 v4, v1, v6    # 56
            s_endpgm
j2:         v_min_i32 v4, v1, v6    # 64
            v_min_i32 v3, v1, v6
            .cf_call c1
            s_swappc_b64 s[0:1], s[0:1]
            v_min_i32 v3, v1, v6    # 76
            .cf_call c1
            s_swappc_b64 s[0:1], s[0:1]
            v_min_i32 v3, v1, v6    # 84
            s_endpgm
c1:         v_add_f32 v6, v1, v4    # 92
            v_add_f32 v6, v1, v4
            .cf_ret
            s_swappc_b64 s[0:1], s[0:1]
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 5, false } },
                false, false, false },
            { 12, 24,
                { { 2, false }, { 5, false } },
                false, false, false },
            { 24, 36,
                { { 3, false }, { 4, false } },
                false, false, false },
            { 36, 40, { { 6, false } }, false, false, true },
            { 40, 56, { { 6, false } }, false, false, true },
            // 5
            { 56, 64, { }, false, false, true },
            { 64, 76,
                { { 9, true } },
                true, false, false },
            { 76, 84,
                { { 9, true } },
                true, false, false },
            { 84, 92, { }, false, false, true },
            // 9 - subroutine
            { 92, 104, { }, false, true, true }
        },
        true, ""
    },
    {   /* 13 - subroutines 2 (one is jump) */
        R"ffDXD(
        v_mov_b32 v1, v2
        v_mov_b32 v1, v3
        .cf_call l0, l1
        .cf_jump l2
        s_setpc_b64 s[0:1]
        v_sub_f32 v1, v3, v6    # 12
        s_branch j0
b0:     v_add_u32 v4, vcc, v6, v11  # 20
        v_mac_f32 v6, v6, v6
        s_endpgm
l0:     # 32
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
l1:     # 44
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
l2:     # 60
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
j0:     # 84
        v_lshrrev_b32 v4, 3, v2
        v_lshrrev_b32 v4, 3, v2
        s_branch b0
)ffDXD",
        {
            { 0, 12,
                { { 5, false }, { 3, true }, { 4, true } },
                true, false, true },
            { 12, 20, { { 6, false } }, false, false, true },
            { 20, 32, { }, false, false, true },
            { 32, 44, { }, false, true, true },
            { 44, 60, { }, false, true, true },
            { 60, 84, { }, false, true, true },
            { 84, 96, { { 2, false } }, false, false, true }
        },
        true, ""
    },
    {   /* 14 - empty */
        ".regvar sa:s:8, va:v:12, vb:v:10\n",
        { },
        true, ""
    },
    {   // 15 - switch (empty block at end)
        R"ffDXD(
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            .cf_jump j1, j2, j3, j4
            s_setpc_b64 s[0:1]
            
j1:         v_xor_b32 v3, v9, v8    # 12
            v_xor_b32 v3, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j2:         v_xor_b32 v3, v9, v8    # 28
            v_xor_b32 v1, v5, v8
            v_and_b32 v2, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j3:         v_xor_b32 v3, v9, v8    # 48
            v_xor_b32 v1, v5, v8
            s_branch b0

j4:         v_xor_b32 v3, v9, v8    # 60
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
b0:         # 72
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 2, false }, { 3, false }, { 4, false } },
                false, false, true },
            { 12, 28, { { 5, false } }, false, false, true },
            { 28, 48, { { 5, false } }, false, false, true },
            { 48, 60, { { 5, false } }, false, false, true },
            { 60, 72, { }, false, false, false },
            { 72, 72, { }, false, false, true }
        },
        true, ""
    },
    { nullptr }
};


const AsmSSADataCase ssaDataTestCases1Tbl[] =
{
    {   /* 0 - simple */
        ".regvar sa:s:8, va:v:10\n"
        "s_mov_b32 sa[4], sa[2]\n"
        "s_add_u32 sa[4], sa[2], s3\n"
        "ds_read_b64 va[4:5], v0\n"
        "v_add_f64 va[0:1], va[4:5], va[2:3]\n"
        "v_mac_f32 va[0], va[4], va[5]\n"  // ignore this write, because also read
        "v_mul_f32 va[1], va[4], va[5]\n"
        "ds_read_b32 v10, v0\n"
        "v_mul_lo_u32 v10, va[2], va[3]\n"
        "v_mul_lo_u32 v10, va[2], va[3]\n",
        {
            { 0, 56, { },
                {
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 266 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 2, 2, false) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 2, 2, false) },
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true }
        },
        { },
        true, ""
    },
    {   /* 1 - tree */
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[2], s3
        ds_read_b64 va[4:5], v0
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        .cf_jump j1,j2,j3
        s_setpc_b64 s[0:1]
        # 44
j1:     v_add_f32 va[5], va[2], va[3]
        v_mov_b32 va[6], va[2]
        s_endpgm
j2:     v_add_f32 va[3], va[2], va[5]
        v_add_f32 va[6], va[2], va[3]
        s_endpgm
j3:     v_add_f32 va[2], va[5], va[3]
        s_endpgm
)ffDXD",
        {
            { 0, 44,
                { { 1, false }, { 2, false }, { 3, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 2, 2, false) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 3, 3, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 3, 3, false) },
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 44, 56, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "va", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 56, 68, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(1, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "va", 6 }, SSAInfo(0, 2, 2, 2, 1, false) }
                }, false, false, true },
            { 68, 76, { },
                {
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(1, SIZE_MAX, 3, SIZE_MAX, 0, true) }
                }, false, false, true },
        },
        { },
        true, ""
    },
    {   /* 2 - tree (align) */
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[2], s3
        ds_read_b64 va[4:5], v0
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        .cf_jump j1,j2,j3
        s_setpc_b64 s[0:1]
.p2align 6
        # 64
j1:     v_add_f32 va[5], va[2], va[3]
        v_mov_b32 va[6], va[2]
        s_endpgm
j2:     v_add_f32 va[3], va[2], va[5]
        v_add_f32 va[6], va[2], va[3]
        s_endpgm
j3:     v_add_f32 va[2], va[5], va[3]
        s_endpgm
)ffDXD",
        {
            { 0, 44,
                { { 1, false }, { 2, false }, { 3, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 2, 2, false) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 3, 3, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 3, 3, false) },
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 64, 76, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "va", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 76, 88, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(1, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "va", 6 }, SSAInfo(0, 2, 2, 2, 1, false) }
                }, false, false, true },
            { 88, 96, { },
                {
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(1, SIZE_MAX, 3, SIZE_MAX, 0, true) }
                }, false, false, true },
        },
        { },
        true, ""
    },
    {   /* 3 - longer tree (more blocks) */
        R"ffDXD(.regvar sa:s:8, va:v:12, vb:v:10
        # 0
        ds_read_b64 v[2:3], v0
        ds_read_b64 v[4:5], v0
        v_mov_b32 va[0], v1
        v_mul_f32 va[1], v0, v1
        .cf_cjump tx1, tx2, tx3
        s_setpc_b64 s[0:1]
        # 28
        v_mul_lo_u32 va[2], va[0], va[1]
        v_mul_hi_u32 va[3], va[0], va[1]
        v_lshrrev_b32 va[4], v2, va[5]
        .cf_cjump ux1, ux2, ux3
        s_setpc_b64 s[0:1]
        # 52
        v_mul_u32_u24 va[2], va[0], va[1]
        v_mul_hi_u32_u24 va[3], va[0], va[1]
        v_lshlrev_b32 va[5], v3, va[4]
        .cf_cjump vx1, vx2, vx3
        s_setpc_b64 s[0:1]
        # 68
        v_mul_lo_i32 va[2], va[0], va[1]
        v_mul_hi_i32 va[3], va[0], va[1]
        v_ashrrev_i32 va[6], v4, va[4]
        .cf_jump wx1, wx2, wx3
        s_setpc_b64 s[0:1]
        
.p2align 4
        # 96
tx1:    v_min_f32 vb[0], va[0], va[1]
        v_madak_f32 vb[5], vb[0], va[1], 2.5
        s_endpgm
ux1:    v_nop
        s_endpgm
vx1:    v_add_f32 va[9], v11, vb[4]
        s_endpgm
wx1:    v_add_f32 va[10], v11, vb[4]
        s_endpgm
.p2align 4
tx2:    v_max_f32 vb[1], va[0], va[1]
        v_madak_f32 va[3], va[0], va[1], 2.5
        s_endpgm
ux2:    v_nop
        s_nop 7
        s_endpgm
vx2:    v_nop
        s_endpgm
wx2:    v_add_f32 va[8], v19, vb[5]
        s_endpgm
.p2align 4
tx3:    v_max_u32 vb[2], va[0], va[1]
        v_madmk_f32 vb[3], va[0], 1.23, vb[1]
        s_endpgm
ux3:    v_add_f32 va[7], v11, vb[5]
        s_endpgm
vx3:    v_add_f32 va[6], v13, vb[5]
        s_endpgm
wx3:    v_nop
        s_endpgm
)ffDXD",
        {
            { 0, 28,
                { { 1, false }, { 4, false }, { 8, false }, { 12, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 256+3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 256+4 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 256+5 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            { 28, 52,
                { { 2, false }, { 5, false }, { 9, false }, { 13, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            { 52, 68,
                { { 3, false }, { 6, false }, { 10, false }, { 14, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+3, }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "va", 3 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "va", 4 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            { 68, 92,
                { { 7, false }, { 11, false }, { 15, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(2, 3, 3, 3, 1, false) },
                    { { "va", 3 }, SSAInfo(2, 3, 3, 3, 1, false) },
                    { { "va", 4 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            // 4-7:
            { 96, 112,
                { },
                {
                    { { "va", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "vb", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "vb", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 112, 120,
                { },
                { }, false, false, true },
            { 120, 128,
                { },
                {
                    { { "", 256+11 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 9 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "vb", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            { 128, 136,
                { },
                {
                    { { "", 256+11 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 10 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "vb", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // 8-11
            { 144, 160,
                { },
                {
                    { { "va", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, 4, 4, 4, 1, false) },
                    { { "vb", 1 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 160, 172,
                { },
                { }, false, false, true },
            { 172, 180,
                { },
                { }, false, false, true },
            { 180, 188,
                { },
                {
                    { { "", 256+19 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 8 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "vb", 5 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // 12-15
            { 192, 208,
                { },
                {
                    { { "va", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "vb", 1 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "vb", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "vb", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 208, 216,
                { },
                {
                    { { "", 256+11 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 7 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "vb", 5 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            { 216, 224,
                { },
                {
                    { { "", 256+13 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 6 }, SSAInfo(0, 2, 2, 2, 1, false) },
                    { { "vb", 5 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            { 224, 232,
                { },
                { }, false, false, true }
        },
        { },
        true, ""
    },
    {   /* 4 - empty! */
        ".regvar sa:s:8, va:v:12, vb:v:10\n",
        { },
        { },
        true, ""
    },
    {   /* 5 - longer blocks (tree) */
        R"ffDXD(.regvar sa:s:8, va:v:12, vb:v:10
        # block 0, offset - 0 
        s_mov_b32 s0, 0x11
        s_mov_b32 s1, 0x11
        s_mov_b64 s[2:3], 3344
        v_mov_b32 v1, v0
        v_add_f32 v2, v1, v0
        v_and_b32 v1, v1, v2
        
        s_mov_b32 sa[0], 1
        s_mov_b32 sa[1], 1
        s_mov_b32 sa[2], 1
        s_mov_b32 sa[3], 1
        
.rept 4
        s_xor_b32 sa[0], sa[1], sa[3]
        s_and_b32 sa[1], sa[2], sa[3]
        s_xor_b32 sa[3], sa[0], sa[2]
        s_xor_b32 sa[2], sa[1], sa[3]
.endr
        s_and_b32 sa[1], sa[2], sa[3]
        s_xor_b32 sa[2], sa[1], sa[3]
        s_xor_b32 sa[2], sa[1], sa[3]
        
        .cf_jump b1, b2
        s_setpc_b64 s[4:5]
        
        # block 1, offset - 124
b1:     s_add_u32 s4, s1, sa[2]
        s_add_u32 sa[4], s2, sa[3]
        
        s_mov_b32 sa[6], s[4]
.rept 3
        s_xor_b32 sa[0], sa[6], sa[3]
        s_and_b32 sa[5], sa[2], sa[3]
        s_and_b32 sa[6], sa[0], sa[5]
.endr
        .cf_jump b11, b12
        s_setpc_b64 s[6:7]
        
        # block 2, offset - 176
b2:     v_mul_lo_u32 va[1], va[1], va[4]
        v_mul_lo_u32 va[2], sa[1], va[1]
        v_mov_b32 va[3], 2343
.rept 3
        v_mul_lo_u32 va[3], sa[1], va[3]
        v_mul_lo_u32 va[1], sa[0], va[4]
        v_mul_lo_u32 va[2], sa[2], va[1]
.endr
.rept 4
        v_mul_lo_u32 va[1], sa[0], va[4]
.endr
.rept 2
        v_mul_lo_u32 va[2], v1, v4
.endr
        v_and_b32 v3, v3, v2
        s_endpgm
        
        # block 2, offset - xx
b11:    v_mov_b32 va[0], 122
        v_mov_b32 va[1], 122
        v_mov_b32 va[2], 122
.rept 4
        v_xor_b32 va[0], va[1], v2
        v_xor_b32 va[1], va[1], v1
        v_xor_b32 va[2], sa[2], va[1]
.endr
        v_xor_b32 va[1], sa[1], v0
        s_xor_b32 sa[1], sa[2], s3
        s_endpgm
        
        # block 3, offset - xx
b12:    v_mov_b32 va[4], 122
        v_xor_b32 va[1], 112, va[1]
        v_xor_b32 va[0], v2, va[0]
.rept 5
        v_xor_b32 va[0], va[1], v2
        v_xor_b32 va[1], va[0], v0
        v_xor_b32 va[4], sa[3], va[4]
.endr
        v_xor_b32 va[4], sa[2], v0
        s_xor_b32 sa[1], sa[3], s3
        s_endpgm
)ffDXD",
        {
            { 0, 124,
                { { 1, false }, { 2, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 256+2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 5, 5, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 6, 6, false) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 7, 7, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 5, 5, false) }
                }, false, false, true },
            // block 1
            { 124, 176,
                { { 3, false }, { 4, false } },
                {
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(5, 6, 6, 8, 3, false) },
                    { { "sa", 2 }, SSAInfo(7, SIZE_MAX, 8, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(5, SIZE_MAX, 6, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 3, 3, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 4, 4, false) }
                }, false, false, true },
            // block 2
            { 176, 328,
                { },
                {
                    { { "", 256+1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(5, SIZE_MAX, 9, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(6, SIZE_MAX, 9, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(7, SIZE_MAX, 8, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(0, 13, 13, 20, 8, true) },
                    { { "va", 2 }, SSAInfo(0, 6, 6, 11, 6, false) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 4, 4, false) },
                    { { "va", 4 }, SSAInfo(0, SIZE_MAX, 8, SIZE_MAX, 0, true) },
                }, false, false, true },
            // block 3
            { 328, 412,
                { },
                {
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 2 }, SSAInfo(7, SIZE_MAX, 8, SIZE_MAX, 0, true) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 5, 5, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 6, 6, false) },
                    { { "va", 2 }, SSAInfo(0, 1, 1, 5, 5, false) }
                }, false, false, true },
            // block 4
            { 412, 504,
                { },
                {
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(6, 8, 8, 8, 1, false) },
                    { { "sa", 2 }, SSAInfo(7, SIZE_MAX, 8, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(5, SIZE_MAX, 6, SIZE_MAX, 0, true) },
                    { { "va", 0 }, SSAInfo(0, 6, 6, 11, 6, true) },
                    { { "va", 1 }, SSAInfo(0, 7, 7, 12, 6, true) },
                    { { "va", 4 }, SSAInfo(0, 1, 1, 7, 7, false) }
                }, false, false, true }
        },
        { },
        true, ""
    },
    {   // 6 - simple loop
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[0], 0
        s_mov_b32 sa[1], s10
        v_mov_b32 va[0], v0
        v_mov_b32 va[1], v1
        v_mov_b32 va[3], 0
        v_mov_b32 va[4], 11
loop:
        ds_read_b32 va[2], va[3]
        v_xor_b32 va[0], va[1], va[2]
        v_not_b32 va[0], va[0]
        v_xor_b32 va[0], 0xfff, va[0]
        v_xor_b32 va[4], va[4], va[2]
        v_not_b32 va[4], va[0]
        v_xor_b32 va[4], 0xfff, va[0]
        v_add_u32 va[1], vcc, 1001, va[1]
        
        s_add_u32 sa[0], sa[0], 1
        s_cmp_lt_u32 sa[0], sa[1]
        s_cbranch_scc1 loop
        
        v_xor_b32 va[0], 33, va[0]
        s_add_u32 sa[0], 14, sa[0]
        s_endpgm
)ffDXD",
        {
            { 0, 24,
                { },
                {
                    { { "", 10 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            { 24, 84,
                { { 1, false }, { 2, false } },
                {
                    { { "", 106 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 107 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 0 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 0 }, SSAInfo(1, 2, 2, 4, 3, false) },
                    { { "va", 1 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 4 }, SSAInfo(1, 2, 2, 4, 3, true) }
                }, false, false, false },
            { 84, 96,
                { },
                {
                    { { "sa", 0 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "va", 0 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, false, true, }
        },
        {   // SSA replaces
            { { "sa", 0 }, { { 2, 1 } } },
            { { "va", 1 }, { { 2, 1 } } },
            { { "va", 4 }, { { 4, 1 } } }
        },
        true, ""
    },
    {   // 7 - branches
        R"ffDXD(.regvar sa:s:8, va:v:8
        # block 0
        s_mov_b32 sa[0], s0
        s_mov_b32 sa[1], s1
        s_mov_b32 sa[2], s2
        s_add_u32 sa[0], sa[1], sa[2]
        s_mov_b32 sa[3], 31
        s_mov_b32 sa[4], 11
        s_cbranch_scc0 pp0
        
        # block 1
bb0:    s_xor_b32 sa[1], sa[1], sa[0]
        s_xor_b32 sa[1], 122, sa[1]
        s_xor_b32 sa[2], sa[1], sa[0]
        s_cbranch_scc0 pp1
        
        # block 2
        s_xor_b32 sa[0], 122, sa[0]
        s_xor_b32 sa[2], sa[0], sa[2]
        s_bfe_u32 sa[3], sa[2], sa[0]
        s_endpgm
        
        # block 3
pp1:    s_xor_b32 sa[1], sa[0], sa[1]
        s_and_b32 sa[0], sa[0], sa[1]
        s_xor_b32 sa[2], sa[0], sa[1]
        s_bfe_u32 sa[4], sa[4], sa[0]
        s_endpgm
        
        # block 4
pp0:    s_xor_b32 sa[1], sa[1], sa[0]
        s_sub_u32 sa[0], sa[1], sa[0]
        s_xor_b32 sa[1], sa[1], sa[0]
        s_xor_b32 sa[1], sa[1], sa[0]
        s_add_u32 sa[0], sa[1], sa[0]
        s_xor_b32 sa[2], sa[1], sa[2]
        s_add_u32 sa[2], sa[0], sa[0]
        s_cbranch_scc0 pp2
        
        # block 5
        s_xor_b32 sa[2], sa[1], sa[2]
        s_add_u32 sa[2], sa[0], sa[0]
        s_xor_b32 sa[2], sa[1], sa[2]
        s_sub_u32 sa[4], sa[4], sa[0]
        s_sub_u32 sa[4], sa[4], sa[2]
        s_branch bb0
        
        # block 6
pp2:    s_xor_b32 sa[1], sa[1], sa[0]
        s_xor_b32 sa[0], sa[1], sa[2]
        s_add_u32 sa[0], sa[1], sa[2]
        s_and_b32 sa[3], sa[3], sa[0]
        s_bfe_u32 sa[3], sa[3], sa[4]
        s_bfe_u32 sa[3], sa[3], sa[1]
        s_branch bb0
)ffDXD",
        {
            // block 0
            { 0, 28,
                { { 1, false }, { 4, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 2, 2, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1
            { 28, 48,
                { { 2, false }, { 3, false } },
                {
                    { { "sa", 0 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(1, 2, 2, 3, 2, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, false) },
                }, false, false, false },
            // block 2
            { 48, 68,
                { },
                {
                    { { "sa", 0 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, false) }
                }, false, false, true },
            // block 3
            { 68, 88,
                { },
                {
                    { { "sa", 0 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 1 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, false) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            // block 4
            { 88, 120,
                { { 5, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(2, 5, 5, 6, 2, true) },
                    { { "sa", 1 }, SSAInfo(1, 5, 5, 7, 3, true) },
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 6, 2, true) }
                }, false, false, false },
            // block 5
            { 120, 144,
                { { 1, false } },
                {
                    { { "sa", 0 }, SSAInfo(6, SIZE_MAX, 7, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(7, SIZE_MAX, 8, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 9, 3, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 4, 2, true) }
                }, false, false, true },
            // block 6
            { 144, 172,
                { { 1, false } },
                {
                    { { "sa", 0 }, SSAInfo(6, 7, 7, 8, 2, true) },
                    { { "sa", 1 }, SSAInfo(7, 8, 8, 8, 1, true) },
                    { { "sa", 2 }, SSAInfo(6, SIZE_MAX, 10, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 5, 3, true) },
                    { { "sa", 4 }, SSAInfo(1, SIZE_MAX, 5, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {
            { { "sa", 0 }, { { 6, 2 }, { 8, 2 } } },
            { { "sa", 1 }, { { 7, 1 }, { 8, 1 } } },
            { { "sa", 4 }, { { 4, 1 } } }
        },
        true, ""
    },
    {   // 8 - two loops, one nested
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b64 sa[0:1], 0
        v_mov_b32 va[0], v1
        v_mov_b32 va[1], v2
        v_mov_b32 va[2], v3
        
loop0:  v_lshrrev_b32 va[0], va[2], va[0]
        v_xor_b32 va[0], va[1], va[0]
        
        s_xor_b32 sa[3], sa[0], 0x5
        s_cmp_eq_u32 sa[3], 9
        s_cbranch_scc1 bb0
        
        v_and_b32 va[0], -15, va[0]
        v_sub_u32 va[0], vcc, -13, va[0]
        s_branch loop1start
        
bb0:    v_xor_b32 va[0], 15, va[0]
        v_add_u32 va[0], vcc, 17, va[0]
loop1start:
        s_mov_b32 sa[1], sa[0]
        
loop1:  v_add_u32 va[2], vcc, va[2], va[1]
        v_xor_b32 va[2], 0xffaaaa, va[0]
        
        s_xor_b32 sa[2], sa[1], 0x5
        s_cmp_eq_u32 sa[2], 7
        s_cbranch_scc1 bb1
        
        v_sub_u32 va[1], vcc, 5, va[1]
        v_sub_u32 va[2], vcc, 7, va[2]
        s_branch loop1end
        
bb1:    v_xor_b32 va[1], 15, va[1]
        v_xor_b32 va[2], 17, va[2]
loop1end:
        
        s_add_u32 sa[1], sa[1], 1
        s_cmp_lt_u32 sa[1], 52
        s_cbranch_scc1 loop1
        
        v_xor_b32 va[0], va[1], va[0]
        v_xor_b32 va[0], va[2], va[0]
        v_xor_b32 va[0], sa[0], va[0]
        
        s_add_u32 sa[0], sa[0], 1
        s_cmp_lt_u32 sa[0], 33
        s_cbranch_scc1 loop0
        
        s_endpgm
)ffDXD",
        {
            // block 0
            { 0, 16,
                { },
                {
                    { { "", 256+1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 (loop0)
            { 16, 36,
                { { 2, false }, { 3, false } },
                {
                    { { "sa", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 0 }, SSAInfo(1, 2, 2, 3, 2, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 2 (to bb0)
            { 36, 48,
                { { 4, false } },
                {
                    { { "", 106 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 107 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "va", 0 }, SSAInfo(3, 4, 4, 5, 2, true) }
                }, false, false, true },
            // block 3 (bb0)
            { 48, 56,
                { },
                {
                    { { "", 106 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 107 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "va", 0 }, SSAInfo(3, 9, 9, 10, 2, true) }
                }, false, false, false },
            // block 4 (loop1start)
            { 56, 60,
                { },
                {
                    { { "sa", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(1, 2, 2, 2, 1, false) }
                }, false, false, false },
            // block 5 (loop1)
            { 60, 84,
                { { 6, false }, { 7, false } },
                {
                    { { "", 106 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 107 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 1 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 0 }, SSAInfo(5, SIZE_MAX, 6, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(1, 2, 2, 3, 2, true) }
                }, false, false, false },
            // block 6 (to bb1)
            { 84, 96,
                { { 8, false } },
                {
                    { { "", 106 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 107 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "va", 1 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "va", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, true },
            // block 7 (b11)
            { 96, 104,
                { },
                {
                    { { "va", 1 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "va", 2 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, false, false },
            // block 8 (loop1end)
            { 104, 116,
                { { 5, false }, { 9, false } },
                {
                    { { "sa", 1 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            // block 9 (loop0end)
            { 116, 140,
                { { 1, false }, { 10, false } },
                {
                    { { "sa", 0 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "va", 0 }, SSAInfo(5, 6, 6, 8, 3, true) },
                    { { "va", 1 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(4, SIZE_MAX, 5, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 10 (to bb0)
            { 140, 144,
                { },
                { }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 0 }, { { 2, 1 } } },
            { { "sa", 1 }, { { 3, 2 } } },
            { { "va", 0 }, { { 8, 1 }, { 10, 5 } } },
            { { "va", 1 }, { { 2, 1 }, { 3, 1 }, { 3, 2 } } },
            { { "va", 2 }, { { 4, 1 }, { 5, 1 }, { 5, 4 } } }
        },
        true, ""
    },
    {   // 9 - two branches
        R"ffDXD(.regvar sa:s:8, va:v:8
        # block 0
        s_mov_b64 sa[0:1], 321
        s_mov_b32 sa[2], s1
        s_mov_b32 sa[3], s2
        s_cmpk_eq_u32 s1, 321
        s_cbranch_scc1 bb10
        
        # block 1
        s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        s_cbranch_scc1 bb01
        
        # block 2
        s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        s_branch bb0e
        
        # block 3
bb01:   s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        
        # block 4
bb0e:   s_add_u32 sa[0], sa[0], sa[2]
        s_endpgm
        
        # block 5
bb10:   s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        s_add_u32 sa[0], sa[0], sa[1]
        s_cbranch_scc1 bb11
        
        # block 6
        s_add_u32 sa[0], sa[0], sa[2]
        s_add_u32 sa[0], sa[0], sa[2]
        s_branch bb1e
        
        # block 7
bb11:   s_add_u32 sa[0], sa[0], sa[3]
        s_add_u32 sa[0], sa[0], sa[3]
        
        # block 8
bb1e:   s_add_u32 sa[0], sa[0], sa[1]
        s_endpgm
)ffDXD",
        {
            // block 0 - start
            { 0, 24,
                { { 1, false }, { 5, false } },
                {
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - bb00
            { 24, 40,
                { { 2, false }, { 3, false } },
                {
                    { { "sa", 0 }, SSAInfo(1, 2, 2, 4, 3, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 2 - bb00 second
            { 40, 52,
                { { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(4, 5, 5, 6, 2, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 3 - bb01
            { 52, 60,
                { },
                {
                    { { "sa", 0 }, SSAInfo(4, 8, 8, 9, 2, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 4 - bb0e
            { 60, 68,
                { },
                {
                    { { "sa", 0 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 2 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 5 - bb10
            { 68, 92,
                { { 6, false }, { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(1, 10, 10, 14, 5, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 6 - bb10 (second)
            { 92, 104,
                { { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(14, 15, 15, 16, 2, true) },
                    { { "sa", 2 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 7 - bb11
            { 104, 112,
                { },
                {
                    { { "sa", 0 }, SSAInfo(14, 18, 18, 19, 2, true) },
                    { { "sa", 3 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 8 - bb1e
            { 112, 120,
                { },
                {
                    { { "sa", 0 }, SSAInfo(16, 17, 17, 17, 1, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 0 }, { { 9, 6 }, { 19, 16 } } }
        },
        true, ""
    },
    {   // 10 - two branches with various SSAIds
        R"ffDXD(.regvar sa:s:8, va:v:8
        # block 0
        s_mov_b32 sa[0], s2
        s_mov_b32 sa[1], s3
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_cmp_lt_u32 s0, s1
        s_cbranch_scc1 bb1
        
        # block 1, bb0
bb0:    s_add_u32 sa[3], sa[3], s1   # replace from this
        s_add_u32 sa[3], sa[3], s2
        s_cbranch_scc1 bb01
        
        # block 2, bb00
        s_add_u32 sa[2], sa[2], sa[1]   # replace from this
        s_add_u32 sa[3], sa[3], sa[2]
        s_endpgm

bb01:   s_add_u32 sa[2], sa[2], sa[1]
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[2]
        s_endpgm
        
bb1:    s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_branch bb0
)ffDXD",
        {
            // block 0 - start
            { 0, 24,
                { { 1, false }, { 4, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - bb0
            { 24, 36,
                { { 2, false }, { 3, false } },
                {
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 3, 2, true) }
                }, false, false, false },
            // block 2 - bb00
            { 36, 48,
                { },
                {
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, true },
            // block 3 - bb01
            { 48, 64,
                { },
                {
                    { { "sa", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 4, 2, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, false, true },
            // block 4 - bb1
            { 64, 76,
                { { 1, false } },
                {
                    { { "sa", 0 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 6, 6, 6, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 5, 1 } } },
            { { "sa", 3 }, { { 6, 1 } } }
        },
        true, ""
    },
    {   // 11 - with trap: first branch have unhandled SSA replace
        R"ffDXD(.regvar sa:s:8, va:v:8
        # block 0
        s_mov_b32 sa[0], s2
        s_mov_b32 sa[1], s3
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_cmp_lt_u32 s0, s1
        s_cbranch_scc1 bb1
        
        # bb0
bb0:    s_add_u32 sa[3], sa[3], s1      # SSAID replace
        s_add_u32 sa[3], sa[3], s2
        s_endpgm
        
bb3:    s_add_u32 sa[3], sa[3], s2      # no SSAID replace (this same SSAID)
        s_endpgm
        
bb1:    s_add_u32 sa[3], sa[3], s2
bb1x:   .cf_jump bb3, bb2, bb4
        s_setpc_b64 s[0:1]

bb2:    s_branch bb0
        
bb4:    s_branch bb1x       # jump to point to resolve conflict (bb1x)
)ffDXD",
        {
            // block 0
            { 0, 24,
                { { 1, false }, { 3, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1
            { 24, 36,
                { },
                {
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 3, 2, true) }
                }, false, false, true },
            // block 2
            { 36, 44,
                { },
                {
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, false, true },
            // block 3 - bb1 - main part
            { 44, 48,
                { },
                {
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, false, false },
            // block 4 - bb1x - jmp part
            { 48, 52,
                { { 2, false }, { 5, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, false, true },
            // block 5
            { 52, 56,
                { { 1, false } },
                { }, false, false, true },
            // block 6
            { 56, 60,
                { { 4, false } },
                { }, false, false, true }
        },
        {
            { { "sa", 3 }, { { 4, 1 } } }
        },
        true, ""
    },
    {   // 12 - with trap: first branch have higher SSAId
        R"ffDXD(.regvar sa:s:8, va:v:8
        # block 0
        s_mov_b32 sa[0], s2
        s_mov_b32 sa[1], s3
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_cmp_lt_u32 s0, s1
        .cf_jump bb1, bbx, bb3, bb2
        s_setpc_b64 s[0:1]
        
bb1:    s_add_u32 sa[3], sa[3], sa[2]
        s_endpgm
        
bbx:    s_add_u32 sa[3], sa[3], sa[2]
        s_cbranch_scc1 bb1_

bb0:    s_add_u32 sa[3], sa[3], sa[2]
        s_endpgm
        
bb1_:   s_branch bb1

bb3:    .cf_jump bb0, bb1_
        s_setpc_b64 s[0:1]
                
bb2:    s_add_u32 sa[3], sa[3], sa[2]
        s_branch bb3
)ffDXD",
        {
            // block 0
            { 0, 24,
                { { 1, false }, { 2, false }, { 5, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            // block 1 - bb1
            { 24, 32,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            // block 2 - bbx
            { 32, 40,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            // block 3 - bb0
            { 40, 48,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, true },
            // block 4 - bb1_
            { 48, 52,
                { { 1, false } },
                { }, false, false, true },
            // block 5
            { 52, 56,
                { { 3, false }, { 4, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, false, true },
            // block 5
            { 56, 64,
                { { 5, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 5, 5, 5, 1, true) }
                }, false, false, true }
        },
        {
            { { "sa", 3 }, { { 3, 1 }, { 5, 3 }, { 5, 1 } } }
        },
        true, ""
    },
    {   // 13 - yet another branch example
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        .cf_cjump b1, b2, b3
        s_setpc_b64 s[0:1]

b0:     s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_endpgm
        
b1:     s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
b1x:    s_xor_b32 sa[2], sa[2], sa[4]
        s_branch b0
        
b2:     s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
b2x:    s_xor_b32 sa[2], sa[2], sa[4]
        s_branch b1x
        
b3:     s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_branch b2x
)ffDXD",
        {
            // block 0
            { 0, 12,
                { { 1, false }, { 2, false }, { 4, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - b0
            { 12, 24,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 2 - b1
            { 24, 32,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - b1x
            { 32, 40,
                { { 1, false } },
                {
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 4 - b2
            { 40, 48,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 5 - b2x
            { 48, 56,
                { { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 6 - b3
            { 56, 68,
                { { 5, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 1 }, { 6, 3 }, { 7, 5 } } },
            { { "sa", 3 }, { { 3, 1 }, { 4, 1 }, { 5, 1 } } }
        },
        true, ""
    },
    {   // 14 -
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        .cf_jump b1, b2
        s_setpc_b64 s[0:1]
        
b11:    s_xor_b32 sa[2], sa[2], sa[3]
        s_endpgm
        
b1:     s_xor_b32 sa[2], sa[2], sa[3]
        s_branch b11
        
b2:     s_branch b11
)ffDXD",
        {
            // block 0 - start
            { 0, 12,
                { { 2, false }, { 3, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            // block 1 - b11
            { 12, 20,
                { },
                {
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 2 - b1
            { 20, 28,
                { { 1, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 3 - b2
            { 28, 32,
                { { 1, false } },
                { }, false, false, true }
        },
        {
            { { "sa", 2 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 15 - trick - SSA replaces beyond visited point
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
loop:   s_xor_b32 sa[2], sa[2], sa[4]
        s_cbranch_scc0 end
        
        s_xor_b32 sa[3], sa[2], sa[4]
        s_cbranch_scc0 loop
        
        s_endpgm
        
end:    s_xor_b32 sa[3], sa[3], sa[4]
        s_endpgm
)ffDXD",
        {
            // block 0 - start
            { 0, 8,
                { },
                {
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - loop
            { 8, 16,
                { { 2, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 2 - loop part 2
            { 16, 24,
                { { 1, false }, { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - end 2
            { 24, 28,
                { },
                { }, false, false, true },
            // block 4 - end
            { 28, 36,
                { },
                {
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {
            { { "sa", 2 }, { { 2, 1 } } },
            // must be
            { { "sa", 3 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 16 - trick - SSA replaces beyond visited point
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
loop:   s_xor_b32 sa[2], sa[2], sa[4]
        s_cbranch_scc0 end
        
        s_xor_b32 sa[3], sa[2], sa[4]
        s_cbranch_scc0 loop
        
end:    s_xor_b32 sa[3], sa[3], sa[4]
        s_endpgm
)ffDXD",
        {
            // block 0 - start
            { 0, 8,
                { },
                {
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - loop
            { 8, 16,
                { { 2, false }, { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 2 - loop part 2
            { 16, 24,
                { { 1, false }, { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - end
            { 24, 32,
                { },
                {
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {
            { { "sa", 2 }, { { 2, 1 } } },
            // must be
            { { "sa", 3 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 17 - simple call
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            { 0, 32,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            { 32, 44,
                { },
                {
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, true },
            { 44, 56,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        { },
        true, ""
    },
    {   // 18 - simple call, more complex routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[4]
        s_min_u32 sa[3], sa[3], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]
        s_and_b32 sa[3], sa[3], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            { 0, 32,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1 - after call
            { 32, 44,
                { },
                {
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, false, true },
            // block 2 - routine
            { 44, 56,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - first return
            { 56, 68,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 4 - second return
            { 68, 80,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 } } },
            { { "sa", 3 }, { { 4, 3 } } }
        },
        true, ""
    },
    {   // 19 - simple call, more complex routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            { 0, 32,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1 - after call
            { 32, 44,
                { },
                {
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            // block 2 - routine
            { 44, 52,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - first return
            { 52, 64,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 4 - second return
            { 64, 72,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 } } },
            { { "sa", 3 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 20 - simple call, more complex routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            { 0, 32,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1 - after call
            { 32, 44,
                { },
                {
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, true },
            // block 2 - routine
            { 44, 56,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - first return
            { 56, 64,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 4 - second return
            { 64, 72,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 } } }
        },
        true, ""
    },
    {   // 21 - simple call, many deep returns
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        s_lshl_b32 sa[4], sa[4], 5
        s_lshl_b32 sa[5], sa[5], 5
        s_lshl_b32 sa[6], sa[6], 5
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[6], sa[3], sa[0]
        s_cbranch_scc1 bb1
        
b0:     s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_scc1 bb01
        
bb00:   s_xor_b32 sa[2], sa[2], sa[0]
        s_branch bb00_
        
bb00_:  s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb01:   s_xor_b32 sa[2], sa[2], sa[0]
        s_branch bb01_
        
bb01_:  s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_scc1 bb11
        
bb10:   s_xor_b32 sa[2], sa[2], sa[0]
        s_branch bb10_
        
bb10_:  s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[5], sa[3], sa[2]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb11:   s_xor_b32 sa[2], sa[2], sa[0]
        s_branch bb11_
        
bb11_:  s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            // block 0
            { 0, 32,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1
            { 32, 56,
                { },
                {
                    { { "sa", 2 }, SSAInfo(5, 13, 13, 13, 1, true) },
                    { { "sa", 3 }, SSAInfo(4, 9, 9, 9, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(0, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            // block 2 - routine
            { 56, 72,
                { { 3, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 3 - bb0
            { 72, 84,
                { { 4, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            // block 4 - bb00
            { 84, 92,
                { { 5, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, true },
            // block 5 - bb00_
            { 92, 104,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            // block 6 - bb01
            { 104, 112,
                { { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) }
                }, false, false, true },
            // block 7 - bb01_
            { 112, 128,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, true) }
                }, false, true, true },
            // block 8 - bb1
            { 128, 140,
                { { 9, false }, { 11, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 6, 6, 6, 1, true) }
                }, false, false, false },
            // block 9 - bb10
            { 140, 148,
                { { 10, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(8, 9, 9, 9, 1, true) }
                }, false, false, true },
            // block 10 - bb10_
            { 148, 164,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(9, 10, 10, 10, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, true, true },
            // block 11
            { 164, 172,
                { { 12, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(8, 11, 11, 11, 1, true) }
                }, false, false, true },
            // block 12
            { 172, 188,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(11, 12, 12, 12, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 8, 8, 8, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, 2, 2, 2, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 7, 5 }, { 10, 5 }, { 12, 5 } } },
            { { "sa", 3 }, { { 5, 4 }, { 7, 4 }, { 8, 4 } } },
            { { "sa", 4 }, { { 1, 0 }, { 2, 0 } } },
            { { "sa", 5 }, { { 1, 0 } } }
        },
        true, ""
    },
    {   // 22 - multiple call of routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_ashr_i32 sa[2], sa[2], 3
        s_ashr_i32 sa[2], sa[2], 3
        s_ashr_i32 sa[3], sa[3], 4
        s_ashr_i32 sa[3], sa[3], 4
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_ashr_i32 sa[2], sa[2], 3
        s_ashr_i32 sa[3], sa[3], 3
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            // block 0
            { 0, 32,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1
            { 32, 64,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, true, false, false },
            // block 2
            { 64, 104,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 7, 2, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 5, 2, true) }
                }, true, false, false },
            // block 3
            { 104, 116,
                { },
                {
                    { { "sa", 2 }, SSAInfo(3, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 6, 6, 6, 1, true) }
                }, false, false, true},
            // block 4 - routine
            { 116, 128,
                { { 5, false }, { 6, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 5
            { 128, 136,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 6
            { 136, 144,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 5, 1 }, { 7, 1 } } },
            { { "sa", 3 }, { { 3, 1 }, { 5, 1 } } }
        },
        true, ""
    },
    {   // 23 - simple call, more complex routine (no use return)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[3], sa[3], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            { 0, 32,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1 - after call
            { 32, 40,
                { },
                {
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, true },
            // block 2 - routine
            { 40, 52,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - first return
            { 52, 60,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 4 - second return
            { 60, 68,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        { },
        true, ""
    },
    {   // 24 - simple call, more complex routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb2

        s_min_u32 sa[3], sa[3], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb2:    s_min_u32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            { 0, 32,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1 - after call
            { 32, 44,
                { },
                {
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, false, true },
            // block 2 - routine
            { 44, 56,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - first return
            { 56, 64,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 4 - second return
            { 64, 72,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 } } },
            { { "sa", 3 }, { { 3, 2 } } }
        },
        true, ""
    },
    {   // 25 - nested calls
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 4
        s_lshl_b32 sa[3], sa[3], 4
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine2-.
        s_add_u32 s3, s3, routine2-.+4
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshr_b32 sa[2], sa[2], 2
        s_min_u32 sa[2], sa[2], sa[3]
        s_lshr_b32 sa[3], sa[3], 2
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[4]
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine2-.
        s_add_u32 s3, s3, routine2-.+4
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]

routine2:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb2

        s_min_u32 sa[3], sa[3], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb2:    s_min_u32 sa[2], sa[2], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            // block 0 - start
            { 0, 32,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1
            { 32, 64,
                { { 7, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 2 }, SSAInfo(4, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) }
                }, true, false, false },
            // block 2
            { 64, 80,
                { },
                {
                    { { "sa", 2 }, SSAInfo(4, 8, 8, 9, 2, true) },
                    { { "sa", 3 }, SSAInfo(3, 6, 6, 6, 1, true) }
                }, false, false, true },
            // block 3 - routine
            { 80, 92,
                { { 4, false }, { 6, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 4 - routine - first way bb0
            { 92, 120,
                { { 7, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, true, false, false },
            // block 5 - routine - return 0
            { 120, 124,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, true, true },
            // block 6 - routine - bb1
            { 124, 132,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 7
            { 132, 144,
                { { 8, false }, { 9, false } },
                {
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 8
            { 144, 152,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 9
            { 152, 160,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 5, 4 }, { 6, 4 }, { 7, 3 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 4, 3 }, { 5, 2 } } }
        },
        true, ""
    },
    {   // 26 - many routines in single calls
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine, routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 6
        s_lshl_b32 sa[3], sa[3], 6
        s_lshl_b32 sa[4], sa[4], 6
        s_lshl_b32 sa[5], sa[5], 6
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine2-.
        s_add_u32 s3, s3, routine2-.+4
        .cf_call routine2, routine3
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 4
        s_lshl_b32 sa[3], sa[3], 4
        s_lshl_b32 sa[4], sa[4], 4
        s_lshl_b32 sa[5], sa[5], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[6]
        s_xor_b32 sa[3], sa[3], sa[6]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]

routine2:
        s_xor_b32 sa[2], sa[2], sa[6]
        s_xor_b32 sa[4], sa[4], sa[6]
        s_cbranch_scc1 bb2
        
        s_min_u32 sa[2], sa[2], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb2:    s_and_b32 sa[2], sa[2], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]

routine3:
        s_xor_b32 sa[3], sa[3], sa[6]
        s_xor_b32 sa[5], sa[5], sa[6]
        s_cbranch_scc1 bb3
        
        s_min_u32 sa[3], sa[3], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb3:    s_and_b32 sa[5], sa[5], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            // block 0 - start
            { 0, 40,
                { { 3, true }, { 6, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1 - after first call
            { 40, 80,
                { { 6, true }, { 9, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 2 }, SSAInfo(3, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, true, false, false },
            // block 2 - end
            { 80, 100,
                { },
                {
                    { { "sa", 2 }, SSAInfo(3, 9, 9, 9, 1, true) },
                    { { "sa", 3 }, SSAInfo(4, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, false, true },
            // block 3 - routine
            { 100, 112,
                { { 4, false }, { 5, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 4 - routine way 0
            { 112, 120,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 5 - routine way 1
            { 120, 128,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 6 - routine 2
            { 128, 140,
                { { 7, false }, { 8, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 7 - routine 2 way 0
            { 140, 148,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 8 - routine 2 way 1
            { 148, 156,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 7, 7, 7, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 9 - routine 3
            { 156, 168,
                { { 10, false }, { 11, false } },
                {
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 10 - routine 3 way 0
            { 168, 176,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 11 - routine 3 way 1
            { 176, 184,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 5 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true  }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 6, 3 }, { 7, 3 }, { 8, 1 } } },
            { { "sa", 3 }, { { 5, 4 } } },
            { { "sa", 4 }, { { 3, 1 } } },
            { { "sa", 5 }, { { 4, 3 } } }
        },
        true, ""
    },
    {   // 27 - simple loop with fork (regvar used only in these forks)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
loop:   s_cbranch_scc1 b1
        
b0:     s_add_u32 sa[2], sa[2], sa[0]
        s_branch loopend
b1:     s_add_u32 sa[2], sa[2], sa[1]
loopend:
        s_cbranch_scc0 loop
        s_endpgm
)ffDXD",
        {
            {   // block 0 - start
                0, 4,
                { },
                {
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            {   // block 1 - loop
                4, 8,
                { { 2, false }, { 3, false } },
                {
                }, false, false, false },
            {   // block 2 - b0
                8, 16,
                { { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 3 - b1
                16, 20,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - loopend
                20, 24,
                { { 1, false }, { 5, false } },
                {
                }, false, false, false },
            {   // block 5 - en
                24, 28,
                { },
                { }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 2, 1 }, { 3, 1 } } }
        },
        true, ""
    },
    { nullptr }
};
