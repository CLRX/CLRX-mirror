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
                    { { "sa", 2 }, SSAInfo(2, 6, 6, 6, 1, true) },
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
            /*{ { "sa", 2 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 5, 1 }, { 4, 1 } } },
            { { "sa", 3 }, { { 2, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 4, 3 }, { 3, 1 }, { 4, 1 },
                        { 5, 1 }, { 6, 1 } } },
            { { "sa", 5 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 } } }*/
        },
        true, ""
    },
    { nullptr }
};
