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

const AsmSSADataCase ssaDataTestCases3Tbl[] =
{
    {   // 0 - routine calls inside routine
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        s_mov_b32 sa[6], s8
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_add_u32 sa[5], sa[5], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm
        
routine:
        s_add_u32 sa[6], sa[6], sa[0]
        .cf_call routine1,routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        .cf_jump a1, a2, a3
        s_setpc_b64 s[0:1]
        
a1:     s_add_u32 sa[4], sa[4], sa[0]
        .cf_call routine1,routine3
        s_swappc_b64 s[0:1], s[2:3]
        .cf_ret
        s_setpc_b64 s[0:1]
        
a2:     s_add_u32 sa[4], sa[4], sa[0]
        .cf_call routine2,routine3
        s_swappc_b64 s[0:1], s[2:3]
        
e0:     s_add_u32 sa[7], sa[4], sa[0]
        s_add_u32 sa[7], sa[4], sa[0]
        
        .cf_call routine5
        s_swappc_b64 s[0:1], s[2:3]
        .cf_ret
        s_setpc_b64 s[0:1]
        
a3:     s_add_u32 sa[5], sa[5], sa[0]
        .cf_call routine3, routine4
        s_swappc_b64 s[0:1], s[2:3]
        s_branch e0

        # subroutines
routine1:
        s_add_u32 sa[2], sa[2], sa[0]
        s_cbranch_vccz r1a1
        
        s_add_u32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
r1a1:
        s_add_u32 sa[4], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

routine2:
        s_add_u32 sa[2], sa[2], sa[0]
        s_cbranch_vccz r2a1
        
        s_add_u32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
r2a1:
        s_add_u32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

routine3:
        s_add_u32 sa[3], sa[3], sa[0]
        s_cbranch_vccz r3a1
        
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
r3a1:
        s_add_u32 sa[3], sa[5], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine4:
        s_add_u32 sa[4], sa[4], sa[0]
        s_cbranch_vccz r4a1
        
        s_add_u32 sa[4], sa[4], sa[0]
        s_add_u32 sa[5], sa[5], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
r4a1:
        s_add_u32 sa[5], sa[5], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine5:
        s_add_u32 sa[4], sa[4], sa[0]
        s_cbranch_vccz r5a1
        
        s_add_u32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
r5a1:
        s_add_u32 sa[5], sa[5], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 8 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                24, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 9, 9, 9, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 56,
                { { 11, true }, { 14, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, true, false, false },
            {   // block 3 - jump a1,a2,a3
                56, 60,
                { { 4, false }, { 6, false }, { 9, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, false, true },
            {   // block 4 - a1
                60, 68,
                { { 11, true }, { 17, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, true, false, false },
            {   // block 5 - a1 end
                68, 72,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, true, true },
            {   // block 6 - a2
                72, 80,
                { { 14, true }, { 17, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 5, 5, 5, 1, true) }
                }, true, false, false },
            {   // block 7 - e0
                80, 92,
                { { 23, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(3, SIZE_MAX, 6, SIZE_MAX, 0, true) },
                    { { "sa", 7 }, SSAInfo(0, 1, 1, 2, 2, false) }
                }, true, false, false },
            {   // block 8 - a2 end
                92, 96,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, true, true },
            {   // block 9 - a3
                96, 104,
                { { 17, true }, { 20, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, true, false, false },
            {   // block 10 - a3 end
                104, 108,
                { { 7, false } },
                { },
                false, false, true },
            {   // block 11 - routine1
                108, 116,
                { { 12, false }, { 13, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 12 - ret1
                116, 124,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 13 - ret2
                124, 132,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, SIZE_MAX, 4, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, false) }
                }, false, true, true },
            {   // block 14 - routine2
                132, 140,
                { { 15, false }, { 16, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, false, false },
            {   // block 15 - ret1
                140, 148,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 16 - ret2
                148, 156,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 17 - routine3
                156, 164,
                { { 18, false }, { 19, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 18 - ret1
                164, 176,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(3, 4, 4, 4, 1, false) }
                }, false, true, true },
            {   // block 19 - ret2
                176, 184,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, false) },
                    { { "sa", 5 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, true, true },
            {   // block 20 - routine4
                184, 192,
                { { 21, false }, { 22, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 7, 7, 7, 1, true) }
                }, false, false, false },
            {   // block 21 - ret1
                192, 204,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(7, 8, 8, 8, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 22 - ret2
                204, 212,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 23 - routine5
                212, 220,
                { { 24, false }, { 25, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(3, 6, 6, 6, 1, true) }
                }, false, false, false },
            {   // block 24 - ret1
                220, 228,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 6, 6, 6, 1, true) }
                }, false, true, true },
            {   // block 25 - ret2
                228, 236,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 } } },
            { { "sa", 3 }, { { 2, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 } } },
            /*{ { "sa", 4 }, { { 2, 1 }, { 4, 3 }, { 3, 1 }, { 5, 1 },
                        { 6, 1 }, { 8, 3 }, { 7, 3 } } },*/
            { { "sa", 4 }, { { 2, 1 }, { 4, 3 }, { 3, 1 },
                        { 6, 1 }, { 8, 3 }, { 7, 3 } } },
            { { "sa", 5 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 } } }
        },
        true, ""
    },
    {   // 1 - retssa tests 2
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[6], s7
        s_mov_b32 sa[7], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[6], sa[3], 4
        s_lshl_b32 sa[6], sa[4], 4
        s_cbranch_execz aa3
        
aa2:    s_lshr_b32 sa[7], sa[1], 4
        s_endpgm
        
aa3:    s_ashr_i32 sa[7], sa[7], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        .cf_cjump bb1, bb2
        s_setpc_b64 s[0:1]
        
bb0:    s_min_u32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

bb2:    s_and_b32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 7 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after call
                24, 36,
                { { 2, false }, { 3, false } },
                {
                    { { "sa", 3 }, SSAInfo(3, SIZE_MAX, 6, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, SIZE_MAX, 5, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 5, 5, 6, 2, false) }
                }, false, false, false },
            {   // block 2 - aa2
                36, 44,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 7 }, SSAInfo(1, 5, 5, 5, 1, false) }
                }, false, false, true },
            {   // block 3 - aa3
                44, 52,
                { },
                {
                    { { "sa", 7 }, SSAInfo(2, 6, 6, 6, 1, true) }
                }, false, false, true
            },
            {   // block 4 - routine
                52, 64,
                { { 5, false }, { 6, false }, { 7, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - bb0
                64, 84,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 6 - bb1
                84, 104,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 7 - bb2
                104, 124,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 3 }, { { 4, 3 }, { 5, 3 } } },
            { { "sa", 4 }, { { 3, 2 }, { 4, 2 } } },
            { { "sa", 7 }, { { 3, 2 }, { 4, 2 } } }
        },
        true, ""
    },
    {   // 2 - routine with loop and jumps to mid in loop
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s6
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_add_u32 sa[5], sa[5], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_execnz b0
        
loop:   s_xor_b32 sa[3], sa[3], sa[1]
l0:     s_xor_b32 sa[4], sa[4], sa[0]
l1:     s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_vccz loop
        
        .cf_ret
        s_setpc_b64 s[0:1]

b0:     s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_vccz l0
        s_xor_b32 sa[6], sa[6], sa[1]
        s_branch l1
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                24, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 60,
                { { 3, false }, { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - loop
                60, 64,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - l0
                64, 68,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - l1
                68, 76,
                { { 3, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 6 - ret
                76, 80,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                },
                false, true, true },
            {   // block 7 - b0, to l0
                80, 88,
                { { 4, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 8 - jump to l1
                88, 96,
                { { 5, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 } } },
            { { "sa", 3 }, { { 3, 2 } } },
            { { "sa", 4 }, { { 2, 1 } } },
            { { "sa", 5 }, { { 2, 1 } } },
            { { "sa", 6 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 3 - routine with loop and jumps to mid in loop
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s6
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_add_u32 sa[5], sa[5], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_execnz b0
        
loop:   s_xor_b32 sa[3], sa[3], sa[1]
        
l0:     s_xor_b32 sa[4], sa[4], sa[0]
        s_cbranch_scc0 end1
        
l1:     s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_scc0 end2
        
        s_branch loop
        
end1:   s_xor_b32 sa[5], sa[5], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end2:   s_xor_b32 sa[6], sa[0], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]

b0:     s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_vccz l0
        s_xor_b32 sa[6], sa[6], sa[1]
        s_branch l1
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                24, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 60,
                { { 3, false }, { 9, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - loop
                60, 64,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - l0
                64, 72,
                { { 5, false }, { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - l1
                72, 80,
                { { 6, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 6 - loop end
                80, 84,
                { { 3, false } },
                { },
                false, false, true },
            {   // block 7 - end1
                84, 92,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 8 - end2
                92, 100,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, false) }
                }, false, true, true },
            {   // block 9 - b0
                100, 108,
                { { 4, false }, { 10, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 10 - to l1
                108, 116,
                { { 5, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 } } },
            { { "sa", 3 }, { { 3, 2 } } },
            { { "sa", 4 }, { { 2, 1 } } },
            { { "sa", 5 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 } } }
        },
        true, ""
    },
    {   // 4 - routine with loop and jumps to mid in loop
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s6
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_add_u32 sa[5], sa[5], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm
        
end1:   s_xor_b32 sa[5], sa[5], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end2:   s_xor_b32 sa[6], sa[0], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_execnz b0
        
loop:   s_xor_b32 sa[3], sa[3], sa[1]
        
l0:     s_xor_b32 sa[4], sa[4], sa[0]
        s_cbranch_scc0 end1
        
l1:     s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_scc0 end2
        
        s_branch loop
        
b0:     s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_vccz l0
        s_xor_b32 sa[6], sa[6], sa[1]
        s_branch l1
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                24, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 2 - end1
                48, 56,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 3 - end2
                56, 64,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, false) }
                }, false, true, true },
            {   // block 4 - routine
                64, 76,
                { { 5, false }, { 9, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - loop
                76, 80,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 6 - l0
                80, 88,
                { { 2, false }, { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 7 - l1
                88, 96,
                { { 3, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 8 - loop end
                96, 100,
                { { 5, false } },
                { },
                false, false, true },
            {   // block 9 - b0
                100, 108,
                { { 6, false }, { 10, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 10 - to l1
                108, 116,
                { { 7, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 } } },
            { { "sa", 3 }, { { 3, 2 } } },
            { { "sa", 4 }, { { 2, 1 } } },
            { { "sa", 5 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 } } }
        },
        true, ""
    },
    {   // 5 - routine with routine with regvar unused in the first routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_vccnz b0
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        .cf_ret
        s_setpc_b64 s[0:1]
        
b0:     s_xor_b32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

routine2:
        s_and_b32 sa[6], sa[0], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                16, 32,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                32, 44,
                { { 3, false }, { 5, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - call routine
                44, 48,
                { { 6, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 4 - routine ret
                48, 52,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                }, false, true, true },
            {   // block 5 - b0
                52, 60,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 6 - routine2
                60, 68,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, false) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 3 }, { { 3, 2 } } },
            { { "sa", 6 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 6 - routine with routine with regvar unused in the first routine
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm

b0:     s_xor_b32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_vccnz b0
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]

        .cf_ret
        s_setpc_b64 s[0:1]
        
routine2:
        s_and_b32 sa[6], sa[0], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                16, 32,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 2 - b0
                32, 40,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 3 - routine
                40, 52,
                { { 2, false }, { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 4 - call routine
                52, 56,
                { { 6, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 5 - routine ret
                56, 60,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                }, false, true, true },
            {   // block 6 - routine2
                60, 68,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, false) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 3 }, { { 3, 2 } } },
            { { "sa", 6 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 7 - first recursion testcase
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_vccnz b0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[3], sa[3], sa[1]
        s_xor_b32 sa[6], sa[6], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
b0:     s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                16, 32,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                32, 44,
                { { 3, false }, { 5, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - recur call
                44, 48,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 4 - routine end
                48, 60,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 5 - routine end 2
                60, 76,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 }, { 2, 1 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 4, 3 }, { 2, 1 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 }, { 3, 2 } } }
        },
        true, ""
    },
    {   // 8 - second recursion testcase
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[4], s5
        s_mov_b32 sa[5], s6
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[1]
        s_add_u32 sa[5], sa[5], sa[1]
        s_add_u32 sa[6], sa[6], sa[1]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_vccnz b0
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[3], sa[3], sa[1]
        s_xor_b32 sa[6], sa[6], sa[1]
        s_xor_b32 sa[5], sa[5], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
b0:     s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine2:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_vccnz b1
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[3], sa[3], sa[1]
        s_xor_b32 sa[6], sa[6], sa[1]
        s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
b1:     s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                24, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 8, 8, 8, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(4, 6, 6, 6, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 60,
                { { 3, false }, { 5, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - call routine2
                60, 64,
                { { 6, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 4 - routine end
                64, 80,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(4, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 5 - routine end2
                80, 96,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 6 - routine2
                96, 108,
                { { 7, false }, { 9, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 7 - call routine
                108, 112,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 8 - routine2 end
                112, 128,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 9 - routine2 end2
                128, 144,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 5, 3 }, { 3, 1 } } },
            /*{ { "sa", 3 }, { { 5, 4 }, { 4, 3 }, { 6, 3 }, { 7, 3 }, { 4, 2 },
                        { 5, 2 }, { 6, 4 }, { 7, 4 }, { 3, 1 } } },*/
            { { "sa", 3 }, { { 5, 4 }, { 6, 3 }, { 7, 3 }, { 4, 2 }, { 5, 2 },
                        { 7, 6 }, { 3, 1 } } },
            { { "sa", 4 }, { { 2, 1 } } },
            { { "sa", 5 }, { { 2, 1 } } },
            /*{ { "sa", 6 }, { { 3, 2 }, { 2, 1 }, { 4, 1 }, { 5, 1 }, { 3, 1 },
                        { 4, 2 }, { 5, 2 } } }*/
            { { "sa", 6 }, { { 3, 2 }, { 4, 1 }, { 5, 1 }, { 2, 1 }, { 3, 1 },
                        { 5, 4 } } }
        },
        true, ""
    },
    // TODO: update ssaIds for regvar that has been returned and changed
    // by recursion callee, after that callee
    {   // 9 - double recursion
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[6], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[6], sa[6], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_vccnz b0
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[3], sa[3], sa[1]
        s_xor_b32 sa[6], sa[6], sa[1]
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
b0:     s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                 0, 16,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                16, 32,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                32, 44,
                { { 3, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - call recur 1
                44, 48,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 4 - call recur 2
                48, 60,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, true, false, false },
            {   // block 5 - routine end 1
                60, 68,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true ,true },
            {   // block 6 - routine end 2
                68, 84,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces (good ???)
            { { "sa", 2 }, { { 3, 2 }, { 4, 2 }, { 3, 1 }, { 4, 1 }, { 2, 1 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 2, 1 }, { 3, 1 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 } } }
        },
        true, ""
    },
    {   // 10 - routine with program end
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[4], s4
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_endpgm
        
routine:
        s_add_u32 sa[2], sa[2], sa[0]
        s_cbranch_execz b0
        
        s_add_u32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
b0:     s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        
        s_endpgm
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                16, 32,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                32, 40,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - routine end 1
                40, 48,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 4 - routine end 2
                48, 64,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
        },
        true, ""
    },
    {   // 11 - routine without return but with program ends
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[4], s4
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_endpgm
        
routine:
        s_add_u32 sa[2], sa[2], sa[0]
        s_cbranch_execz b0
        
        s_add_u32 sa[3], sa[3], sa[0]
        s_endpgm
        
b0:     s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        
        s_endpgm
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                16, 32,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                32, 40,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - routine end 1
                40, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 4 - routine end 2
                48, 64,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
        },
        true, ""
    },
    {   // 12 - routine with program end, and jump to subroutine without end
        // checking skipping joinning retSSAIds while joining subroutine 
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s4
        s_mov_b32 sa[4], s4
        s_mov_b32 sa[5], s5
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_add_u32 sa[5], sa[5], sa[0]
        s_endpgm
        
routine:
        s_add_u32 sa[2], sa[2], sa[0]
        .cf_cjump b0, b1, b2
        s_setpc_b64 s[0:1]
        
        s_add_u32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
b0:     s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[0]
        s_add_u32 sa[4], sa[4], sa[0]
        s_endpgm
        
b1:     s_add_u32 sa[4], sa[4], sa[0]
        s_branch b0
        
b2:     s_add_u32 sa[5], sa[5], sa[0]
        s_branch b0
)ffDXD",
        {
            {   // block 0 - start
                0, 20,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                20, 40,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                40, 48,
                { { 3, false }, { 4, false }, { 5, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - routine end 1
                48, 56,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 4 - routine end 2
                56, 72,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 5 - b1
                72, 80,
                { { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 6 - b2
                80, 88,
                { { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
        },
        {   // SSA replaces
            { { "sa", 4 }, { { 3, 1 } } }
        },
        true, ""
    },
    {   // 13 - simple routine with loop (first jump to end, second to loop)
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_endpgm
        
rend:   s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        
loop0:  s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        .cf_jump loop0, rend
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after routine call
                16, 32,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, true, false, false },
            {   // block 2 - end
                32, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 3 - routine end
                48, 60,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 4 - routine
                60, 68,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - loop0
                68, 84,
                { { 3, false }, { 5, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 }, { 5, 1 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 4, 1 } } }
        },
        true, ""
    },
    {   // 14 - simple routine with loop (without end)
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        
loop0:  s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc0 loop0
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[2], sa[2], sa[0]
        s_endpgm
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after routine call
                16, 32,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, true, false, false },
            {   // block 2 - end
                32, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 3 - routine
                48, 56,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 4 - loop0
                56, 72,
                { { 4, false }, { 5, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - routine end
                72, 84,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 }, { 5, 1 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 4, 1 } } }
        },
        true, ""
    },
    {   // 15 - one routine as part of other routine
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_endpgm
        
routine2:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_scc1 b0
        .cf_ret
        s_setpc_b64 s[0:1]
        
b0:     s_xor_b32 sa[3], sa[3], sa[1]
        s_xor_b32 sa[2], sa[2], sa[1]

routine:
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 6, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after call routine
                16, 32,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, true, false, false },
            {   // block 2 - end
                32, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 3 - routine2
                48, 60,
                { { 4, false }, { 5, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, false },
            {   // block 4 - end1
                60, 64,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, true, true },
            {   // block 5 - b0
                64, 72,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, false, false },
            {   // block 6 - routine
                72, 84,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 } } },
            { { "sa", 3 }, { { 4, 2 }, { 5, 1 } } },
            { { "sa", 4 }, { { 3, 2 }, { 3, 1 } } }
        },
        true, ""
    },
    {   // 16 - routine with loop with call to other routine
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        s_mov_b32 sa[6], s8
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_vccz  subr1
        
loop0:  s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_scc0 end1
        
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_scc0 end2
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc0 end3
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[5], sa[5], sa[1]
        s_branch loop0
        
end1:   s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

end2:   s_xor_b32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

end3:   s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

subr1:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_cbranch_execnz loop0

subr2:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine2:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[2], sa[1]
        s_cbranch_scc1 r2b0
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[5], sa[5], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
r2b0:
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 2, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 8 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                24, 48,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 9, 9, 9, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 9, 9, 9, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 7, 7, 7, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 64,
                { { 3, false }, { 11, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - loop0
                64, 72,
                { { 4, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - to end2
                72, 80,
                { { 5, false }, { 9, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 5 - to end3
                80, 88,
                { { 6, false }, { 10, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 6 - call routine
                88, 92,
                { { 13, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 7 - loop end
                92, 100,
                { { 3, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 8 - end1
                100, 108,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) }
                }, false, true, true },
            {   // block 9 - end2
                108, 116,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 6, 6, 6, 1, true) }
                }, false, true, true },
            {   // block 10 - end3
                116, 124,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 11 - subr1
                124, 148,
                { { 3, false }, { 12, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 12 - subr2
                148, 172,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(7, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(7, 8, 8, 8, 1, true) },
                    { { "sa", 4 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 13 - routine2
                172, 184,
                { { 14, false }, { 15, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, false) }
                }, false, false, false },
            {   // block 14 - end1
                184, 196,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 15 - end2
                196, 208,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 6, 3 }, { 8, 3 }, { 5, 2 }, { 4, 2 }, { 7, 2 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 },
                        { 7, 2 }, { 8, 2 } } },
            { { "sa", 4 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 } } },
            { { "sa", 5 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 } } }
        },
        true, ""
    },
    {   // 17 - routine with loop with call to other routine
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_cbranch_scc1 a1
        
a0:     s_xor_b32 sa[2], sa[2], sa[0]
        s_endpgm
        
a1:     .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        s_branch a0
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_execz b0
        
        s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
b0:     s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 8,
                { { 1, false }, { 2, false } },
                {
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            {   // block 1 - a0
                8, 16,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 2 - a1 (routine call)
                16, 20,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 3 - jump to a0
                20, 24,
                { { 1, false } },
                { }, false, false, true },
            {   // block 4 - routine
                24, 32,
                { { 5, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 5 - routine end 1
                32, 40,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 6 - routine end 2
                40, 48,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 1 }, { 5, 1 } } }
        },
        true, ""
    },
    {   // 18 - simple routine with loop (no var init)
        R"ffDXD(.regvar sa:s:10, va:v:8
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_endpgm
        
routine:
loop0:  s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc0 loop0
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 4,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 1 - after routine call
                4, 20,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, true, false, false },
            {   // block 2 - end
                20, 36,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 3 - routine/loop
                36, 52,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, true) }
                }, false, false, false },
            {   // block 4 - routine end
                52, 64,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 1, 0 }, { 3, 0 } } },
            { { "sa", 3 }, { { 1, 0 }, { 2, 0 } } },
            { { "sa", 4 }, { { 1, 0 }, { 3, 0 } } }
        },
        true, ""
    },
    {   // 19 - complex condition/loop testcase
        R"ffDXD(.regvar sa:s:12, va:v:8
        s_mov_b32 sa[2], s2
        s_mov_b32 sa[3], s3
        s_mov_b32 sa[4], s4
        s_mov_b32 sa[5], s5
        s_mov_b32 sa[6], s6
        s_mov_b32 sa[7], s7
        s_mov_b32 sa[8], s8
        s_mov_b32 sa[9], s9
        s_mov_b32 sa[10], s10
        s_mov_b32 sa[11], s11
        
        .cf_jump a0,a1,a2,a3
        s_setpc_b64 s[0:1]
        
end:    s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[1]
        s_xor_b32 sa[8], sa[8], sa[0]
        s_xor_b32 sa[9], sa[9], sa[1]
        s_xor_b32 sa[10], sa[10], sa[0]
        s_xor_b32 sa[11], sa[11], sa[1]
        s_endpgm
        
        # jumps
a0:     s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        
        .cf_cjump a00,a01,a02
        s_setpc_b64 s[0:1]
a0end:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[4], sa[3], sa[1]
        s_branch end
        
a00:    s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        s_branch a0end
        
a01:    s_xor_b32 sa[2], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc1 a0end
        
a02:    s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        .cf_jump a00, a01
        s_setpc_b64 s[0:1]
        
        # loops
a1:     s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[5], sa[5], sa[1]
        
a1loop0:
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_and_b32 sa[5], sa[5], sa[3]
        s_cbranch_execz a1end0
a1loop1:
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[3], sa[1]
        s_cbranch_execz a1end1
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_xor_b32 sa[2], sa[3], sa[5]
        s_cbranch_vccnz a1loop1
        
        s_xor_b32 sa[5], sa[5], sa[1]
        s_cbranch_vccnz a1loop0
        
        s_xor_b32 sa[3], sa[3], sa[1]
        s_branch end
        
a1end0:
        s_xor_b32 sa[3], sa[3], sa[0]
        s_branch end

a1end1:
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_scc0 a1loop0
        s_branch end
        
        # loops2
a2:     s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[1]
a2loop0:
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[1]
a2loop1:
        s_xor_b32 sa[8], sa[8], sa[0]
        s_xor_b32 sa[9], sa[9], sa[1]
        s_cbranch_scc1 a2loop1
        
a2loop2:
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_scc1 a2loop2
        
        s_xor_b32 sa[7], sa[7], sa[1]
        s_cbranch_scc1 a2loop0
        
a2loop3:
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[1]
a2loop4:
        s_xor_b32 sa[8], sa[8], sa[0]
        s_xor_b32 sa[9], sa[8], sa[1]
        s_cbranch_vccz a2loop0
        s_cbranch_scc1 a2loop4
        
        s_xor_b32 sa[7], sa[9], sa[1]
        s_cbranch_scc1 a2loop3
        
        s_branch end
        
        # stupid loop
a3:
        s_xor_b32 sa[10], sa[10], sa[0]
        s_xor_b32 sa[11], sa[11], sa[1]
a3loop:
        s_xor_b32 sa[10], sa[10], sa[0]
        s_xor_b32 sa[11], sa[11], sa[1]
        s_branch a3loop
)ffDXD",
        {
            {   // block 0 - start
                0, 44,
                { { 2, false }, { 7, false }, { 16, false }, { 26, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 8 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 9 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 10 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 11 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 7 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 8 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 9 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 10 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 11 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            {   // block 1 - end
                44, 88,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 8 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 9 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 10 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 11 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 2 - a0
                88, 100,
                { { 3, false }, { 4, false }, { 5, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - a0end
                100, 112,
                { { 1, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, false) }
                }, false, false, true },
            {   // block 4 - a00
                112, 124,
                { { 3, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 5 - a01
                124, 136,
                { { 3, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 6, 6, 6, 1, false) },
                    { { "sa", 3 }, SSAInfo(1, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 6, 6, 6, 1, true) }
                }, false, false, false },
            {   // block 6 - a02
                136, 148,
                { { 4, false }, { 5, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(6, 7, 7, 7, 1, true) }
                }, false, false, true },
            {   // block 7 - a1
                148, 156,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 8 - a1loop0
                156, 172,
                { { 9, false }, { 13, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 4, 4, 5, 2, true) }
                }, false, false, false },
            {   // block 9 - a1loop1
                172, 184,
                { { 10, false }, { 14, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 8, 8, 8, 1, false) }
                }, false, false, false },
            {   // block 10 - after jump a1end1
                184, 200,
                { { 9, false }, { 11, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 8, 8, 8, 1, false) },
                    { { "sa", 3 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(5, 6, 6, 6, 1, true) }
                }, false, false, false },
            {   // block 11 - a1loop0 end
                200, 208,
                { { 8, false }, { 12, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(6, 7, 7, 7, 1, true) }
                }, false, false, false },
            {   // block 12 - a1 end
                208, 216,
                { { 1, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) }
                }, false, false, true },
            {   // block 13 - a1end0
                216, 224,
                { { 1, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(4, 8, 8, 8, 1, true) }
                }, false, false, true },
            {   // block 14 - a1end1
                224, 232,
                { { 8, false }, { 15, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(5, 8, 8, 8, 1, true) }
                }, false, false, false },
            {   // block 15 - jump to end
                232, 236,
                { { 1, false } },
                { }, false, false, true },
            {   // block 16 - a2
                236, 244,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 17 - a2loop0
                244, 252,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 7 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, false },
            {   // block 18 - a2loop1
                252, 264,
                { { 18, false }, { 19, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 8 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 9 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 19 - a2loop2
                264, 272,
                { { 19, false }, { 20, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 9, 9, 9, 1, true) }
                }, false, false, false },
            {   // block 20 - a2loop0 end
                272, 280,
                { { 17, false }, { 21, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 7 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, false, false },
            {   // block 21 - a2loop3
                280, 288,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 7 }, SSAInfo(5, 6, 6, 6, 1, true) }
                }, false, false, false },
            {   // block 22 - a2loop4
                288, 300,
                { { 17, false }, { 23, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 8 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 9 }, SSAInfo(3, 4, 4, 4, 1, false) }
                }, false, false, false },
            {   // block 23 - after jump a2loop0
                300, 304,
                { { 22, false }, { 24, false } },
                { }, false, false, false },
            {   // block 24 - a2loop3 end
                304, 312,
                { { 21, false }, { 25, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 7 }, SSAInfo(6, 7, 7, 7, 1, false) },
                    { { "sa", 9 }, SSAInfo(4, SIZE_MAX, 5, SIZE_MAX, 0, true) }
                }, false, false, false },
            {   // block 25 - to end
                312, 316,
                { { 1, false } },
                { }, false, false, true },
            {   // block 26 - a3
                316, 324,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 10 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 11 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 27 - a3loop
                324, 336,
                { { 27, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 10 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 11 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 5, 2 }, { 6, 2 }, { 7, 2 }, { 8, 3 }, { 3, 1 } } },
            { { "sa", 3 }, { { 6, 4 }, { 6, 3 }, { 7, 1 }, { 5, 3 },
                        { 5, 1 }, { 8, 1 } } },
            { { "sa", 4 }, { { 7, 2 }, { 6, 2 }, { 8, 3 }, { 3, 1 } } },
            { { "sa", 5 }, { { 6, 5 }, { 7, 3 }, { 7, 1 }, { 8, 3 },
                        { 8, 1 }, { 5, 1 }, { 9, 1 } } },
            { { "sa", 6 }, { { 4, 3 }, { 5, 3 }, { 5, 4 }, { 5, 1 } } },
            { { "sa", 7 }, { { 5, 3 }, { 6, 3 }, { 7, 5 }, { 7, 1 } } },
            { { "sa", 8 }, { { 3, 1 }, { 4, 1 }, { 4, 3 } } },
            { { "sa", 9 }, { { 3, 1 }, { 4, 1 } } },
            { { "sa", 10 }, { { 4, 3 } } },
            { { "sa", 11 }, { { 4, 3 } } }
        },
        true, ""
    },
    {   // 20 - simple call, more complex routine (many changes at end)
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
        s_and_b32 sa[2], sa[2], sa[4]
        s_and_b32 sa[3], sa[3], sa[4]
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
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 6, 6, 6, 1, true) }
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
            { 68, 88,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 5, 2, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 5, 2, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 5, 3 } } },
            { { "sa", 3 }, { { 5, 3 } } }
        },
        true, ""
    },
    {   // 21 - simple join from 3 ways
        R"ffDXD(.regvar sa:s:12, va:v:8
        s_mov_b32 sa[2], s2
        .cf_jump a1,a2,a3
        s_setpc_b64 s[0:1]
        
a1:     s_add_u32 sa[2], sa[2], sa[3]
        s_add_u32 sa[2], sa[2], sa[3]
        s_branch end
a2:     s_add_u32 sa[2], sa[2], sa[4]
        s_add_u32 sa[2], sa[2], sa[4]
        s_branch end
a3:     s_add_u32 sa[2], sa[2], sa[5]
        s_add_u32 sa[2], sa[2], sa[5]
        s_branch end

end:    s_xor_b32 sa[2], sa[2], s3
        s_endpgm
)ffDXD",
        {
            {   // block 0 - start
                0, 8,
                { { 1, false }, { 2, false }, { 3, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            {   // block 1 - a1
                8, 20,
                { { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 3, 2, true) },
                    { { "sa", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            {   // block 2 - a2
                20, 32,
                { { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 6, 2, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            {   // block 3 - a3
                32, 44,
                { { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 7, 7, 8, 2, true) },
                    { { "sa", 5 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true
            },
            {   // block 4 - end
                44, 52,
                { },
                {
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 6, 3 }, { 8, 3 } } }
        },
        true, ""
    },
    {   // 22 - same registers, no regvars
        R"ffDXD(
        s_mov_b32 s2, s10
        s_mov_b32 s3, s11
        
loop:   s_xor_b32 s2, s2, s4
        s_cbranch_scc0 end
        
        s_xor_b32 s3, s2, s4
        s_cbranch_scc0 loop
        
end:    s_xor_b32 s3, s3, s4
        s_endpgm
)ffDXD",
        {
            // block 0 - start
            { 0, 8,
                { },
                {
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 10 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 11 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, false, false },
            // block 1 - loop
            { 8, 16,
                { { 2, false }, { 3, false } },
                {
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, false, false },
            // block 2 - loop part 2
            { 16, 24,
                { { 1, false }, { 3, false } },
                {
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, false, false },
            // block 3 - end
            { 24, 32,
                { },
                {
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, false, true }
        },
        { },
        true, ""
    },
    { nullptr }
};
