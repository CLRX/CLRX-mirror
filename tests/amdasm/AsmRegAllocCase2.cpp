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

const AsmSSADataCase ssaDataTestCases2Tbl[] =
{
    {   // 0 - conflicts inside routines
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_cbranch_scc1 aa0
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine1-.
        s_add_u32 s3, s3, routine1-.+4
        .cf_call routine1
        s_swappc_b64 s[0:1], s[2:3]
        
        s_and_b32 sa[5], sa[5], s0
        
aa0:    s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine2-.
        s_add_u32 s3, s3, routine2-.+4
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshr_b32 sa[2], sa[2], 2
        s_min_u32 sa[2], sa[2], sa[3]
        s_lshr_b32 sa[3], sa[3], 2
        s_endpgm
        
routine1:
        s_xor_b32 sa[2], sa[2], sa[6]
        s_xor_b32 sa[3], sa[3], sa[6]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[6]
        s_and_b32 sa[4], sa[4], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[6]
        s_and_b32 sa[4], sa[4], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine2:
        s_xor_b32 sa[2], sa[2], sa[6]
        s_xor_b32 sa[3], sa[3], sa[6]
        s_cbranch_scc1 bb2
        
        s_min_u32 sa[2], sa[2], sa[6]
        s_max_u32 sa[4], sa[4], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb2:    s_and_b32 sa[3], sa[3], sa[6]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            // block 0 - start
            { 0, 16,
                { { 1, false }, { 3, false } },
                {
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - call routine1
            { 16, 40,
                { { 5, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) }
                }, true, false, false },
            // block 2 - before aa0
            { 40, 44,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, true) }
                }, false, false, false },
            // block 3 - aa0
            { 44, 68,
                { { 8, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, false) }
                }, true, false, false },
            // block 4 - end
            { 68, 84,
                { },
                {
                    { { "sa", 2 }, SSAInfo(5, 7, 7, 8, 2, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, false, true },
            // block 5 - routine1
            { 84, 96,
                { { 6, false }, { 7, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 6 - routine1 way 1
            { 96, 108,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 7 - routine1 way 2
            { 108, 120,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 8 - routine 2
            { 120, 132,
                { { 9, false }, { 10, false } },
                {
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 9 - routine 2 way 0
            { 132, 144,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true },
            // block 10 - routine 2 way 1
            { 144, 152,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 6, 5 }, { 3, 1 } } },
            { { "sa", 3 }, { { 4, 3 }, { 2, 1 } } },
            { { "sa", 4 }, { { 3, 2 }, { 2, 1 } } }
        },
        true, ""
    },
    {  // 1 - res second point cache test 1
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        .cf_cjump aa0, bb0, cc0
        s_setpc_b64 s[0:1]
        
        s_mov_b32 sa[3], s3
        s_mov_b32 sa[6], s3
        s_branch mainx
        
aa0:    s_mov_b32 sa[2], s2
        s_mov_b32 sa[6], s2
        s_cbranch_scc0 mainx
        
        s_mov_b32 sa[4], s5
        s_branch mainx

bb0:    v_mov_b32 va[3], 4
        v_mov_b32 va[2], v6
        s_cbranch_scc0 mainy
        
        v_mov_b32 va[0], v6
        v_mov_b32 va[4], v6
        s_branch mainy
        
cc0:    s_mov_b32 xa[3], 4
        s_mov_b32 xa[4], 4
        s_cbranch_scc0 mainz
        
        s_mov_b32 xa[0], s6
        s_mov_b32 xa[3], s6
        s_cbranch_scc0 mainz
        
        s_mov_b32 xa[0], s6
        
mainx:
        s_add_u32 sa[2], sa[2], sa[7]
        s_add_u32 sa[3], sa[3], sa[7]
        s_sub_u32 sa[4], sa[4], sa[7]
        s_add_u32 sa[5], sa[5], sa[7]
        
        v_xor_b32 va[2], va[2], va[1]
        s_add_u32 xa[0], xa[0], xa[7]
        s_cbranch_scc0 mainz
        
mainy:
        v_xor_b32 va[0], va[0], va[1]
        v_xor_b32 va[2], va[2], va[1]
        v_xor_b32 va[3], va[3], va[1]
        v_xor_b32 va[4], va[4], va[1]
        s_endpgm
        
mainz:
        s_add_u32 xa[0], xa[0], xa[7]
        s_add_u32 xa[3], xa[3], xa[7]
        s_sub_u32 xa[4], xa[4], xa[7]
        s_add_u32 xa[5], xa[5], xa[7]
        s_add_u32 sa[6], sa[6], sa[7]
        s_add_u32 sa[3], sa[3], sa[7]
        s_sub_u32 sa[4], sa[4], sa[7]
        s_add_u32 sa[5], sa[5], sa[7]
        s_endpgm
)ffDXD",
        {
            {   // block 0 - start
                0, 16,
                { { 1, false }, { 2, false }, { 4, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            {   // block 1 - before aa0
                16, 28,
                { { 9, false } },
                {
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, true },
            {   // block 2 - aa0
                28, 40,
                { { 3, false }, { 9, false } },
                {
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 3, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 3, 3, 3, 1, false) }
                }, false, false, false },
            {   // block 3 - after aa0
                40, 48,
                { { 9, false } },
                {
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, false) }
                }, false, false, true },
            {   // block 4 - bb0
                48, 60,
                { { 5, false }, { 10, false } },
                {
                    { { "", 256+6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 2 }, SSAInfo(0, 3, 3, 3, 1, false) },
                    { { "va", 3 }, SSAInfo(0, 2, 2, 2, 1, false) }
                }, false, false, false },
            {   // block 5 - after bb0
                60, 72,
                { { 10, false } },
                {
                    { { "", 256+6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "va", 0 }, SSAInfo(0, 2, 2, 2, 1, false) },
                    { { "va", 4 }, SSAInfo(0, 2, 2, 2, 1, false) }
                }, false, false, true },
            {   // block 6 - cc0
                72, 84,
                { { 7, false }, { 11, false } },
                {
                    { { "xa", 3 }, SSAInfo(0, 2, 2, 2, 1, false) },
                    { { "xa", 4 }, SSAInfo(0, 2, 2, 2, 1, false) }
                }, false, false, false },
            {   // block 7 - after cc0
                84, 96,
                { { 8, false }, { 11, false } },
                {
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "xa", 0 }, SSAInfo(0, 3, 3, 3, 1, false) },
                    { { "xa", 3 }, SSAInfo(2, 3, 3, 3, 1, false) }
                }, false, false, false },
            {   // block 8 - before mainx
                96, 100,
                { },
                {
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "xa", 0 }, SSAInfo(3, 4, 4, 4, 1, false) }
                }, false, false, false },
            {   // block 9 - mainx
                100, 128,
                { { 10, false }, { 11, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "xa", 0 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            {   // block 10 - mainy
                128, 148,
                { },
                {
                    { { "va", 0 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "va", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "va", 4 }, SSAInfo(0, 1, 1, 1, 1, true) }
                }, false, false, true },
            {   // block 11 - mainz
                148, 184,
                { },
                {
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "xa", 3 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "xa", 4 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "xa", 5 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                }, false, false, true },
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 1 } } },
            { { "sa", 3 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 4 }, { { 4, 1 }, { 2, 1 } } },
            { { "sa", 6 }, { { 3, 1 }, } },
            { { "va", 0 }, { { 2, 0 } } },
            { { "va", 2 }, { { 3, 1 }, } },
            { { "va", 3 }, { { 2, 0 } } },
            { { "va", 4 }, { { 2, 0 } } },
            { { "xa", 0 }, { { 4, 0 }, { 3, 1 } } },
            { { "xa", 3 }, { { 3, 0 }, { 2, 0 } } },
            { { "xa", 4 }, { { 2, 0 } } }
        },
        true, ""
    },
    {   // 2 - cache test 2: loop
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        s_mov_b32 xa[0], s4
        .cf_cjump aa0, aa1, aa2, aa3
        s_setpc_b64 s[0:1]
        
loop:   s_xor_b32 sa[2], sa[2], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]
        
l1:     s_xor_b32 sa[3], sa[3], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]
        
l2:     s_xor_b32 sa[4], sa[4], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]

l3:     s_xor_b32 sa[5], sa[5], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]
        s_cbranch_scc0 loop
        
        s_endpgm
        
aa0:    s_xor_b32 sa[5], sa[5], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]
        s_branch loop

aa1:    s_xor_b32 sa[2], sa[2], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]
        s_branch l1

aa2:    s_xor_b32 sa[3], sa[3], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]
        s_branch l2

aa3:    s_xor_b32 sa[4], sa[4], sa[7]
        s_xor_b32 xa[0], xa[0], xa[7]
        s_branch l3
)ffDXD",
        {
            {   // block 0 - start
                0, 24,
                { { 1, false }, { 6, false }, { 7, false }, { 8, false }, { 9, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 7 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "xa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            {   // block 1 - loop
                24, 32,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            {   // block 2 - l1
                32, 40,
                { },
                {
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            {   // block 3 - l2
                40, 48,
                { },
                {
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            {   // block 4 - l3
                48, 60,
                { { 1, false }, { 5, false } },
                {
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            {   // block 5 - end
                60, 64,
                { },
                { }, false, false, true },
            {   // block 6 - aa0
                64, 76,
                { { 1, false } },
                {
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(1, 6, 6, 6, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            {   // block 7 - aa1
                76, 88,
                { { 2, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(1, 7, 7, 7, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            {   // block 9 - aa2
                88, 100,
                { { 3, false } },
                {
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(1, 8, 8, 8, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            {   // block 10 - aa3
                100, 112,
                { { 4, false } },
                {
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "xa", 0 }, SSAInfo(1, 9, 9, 9, 1, true) },
                    { { "xa", 7 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 3 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 5 }, { { 2, 1 }, { 3, 1 } } },
            { { "xa", 0 }, { { 5, 1 }, { 6, 1 }, { 7, 2 }, { 8, 3 }, { 9, 4 } } }
        },
        true, ""
    },
    {   // 3 - routine with loop
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_endpgm
        
routine:
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[1]
        
loop0:
        s_add_u32 sa[4], sa[3], sa[0]
        s_add_u32 sa[5], sa[3], sa[5]
        s_cbranch_scc1 j1
        
j0:     s_add_u32 sa[2], sa[3], sa[0]
        s_branch loopend

j1:     s_add_u32 sa[5], sa[2], sa[0]

loopend:
        s_add_u32 sa[5], sa[5], sa[1]
        s_cbranch_scc0 loop0
        
        s_add_u32 sa[5], sa[5], sa[0]
        s_add_u32 sa[3], sa[3], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 20,
                { { 3, true } },
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
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - before second call
                20, 32,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(4, 6, 6, 6, 1, true) }
                }, true, false, false },
            {   // block 2 - end
                32, 36,
                { },
                { }, false, false, true },
            {   // block 3 - routine
                36, 44,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 4 - loop
                44, 56,
                { { 5, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                },
                false, false, false },
            {   // block 5 - j0
                56, 64,
                { { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, false) },
                    { { "sa", 3 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) }
                }, false, false, true },
            {   // block 6 - j1
                64, 68,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, SIZE_MAX, 4, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 5, 5, 5, 1, false) }
                }, false, false, false },
            {   // block 7 - loopend
                68, 76,
                { { 4, false }, { 8, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 8 - routine end
                76, 88,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 }, { 4, 1 } } },
            { { "sa", 3 }, { { 3, 1 } } },
            { { "sa", 5 }, { { 3, 1 }, { 5, 2 }, { 6, 1 } } },
        },
        true, ""
    },
    {   // 4 - routine with condition
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_endpgm
        
routine:
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[1]
        
        s_add_u32 sa[4], sa[3], sa[0]
        s_add_u32 sa[5], sa[3], sa[5]
        s_cbranch_scc1 j1
        
j0:     s_add_u32 sa[2], sa[3], sa[0]
        s_add_u32 sa[4], sa[3], sa[1]
        s_branch rend

j1:     s_add_u32 sa[5], sa[2], sa[0]
        s_add_u32 sa[4], sa[3], sa[1]

rend:
        s_add_u32 sa[5], sa[5], sa[0]
        s_add_u32 sa[3], sa[3], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 20,
                { { 3, true } },
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
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - before second call
                20, 32,
                { { 3, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, true, false, false },
            {   // block 2 - end
                32, 36,
                { },
                { }, false, false, true },
            {   // block 3 - routine
                36, 56,
                { { 4, false }, { 5, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 4 - j0
                56, 68,
                { { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, false) },
                    { { "sa", 3 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, false) }
                },
                false, false, true },
            {   // block 5 - j1
                68, 76,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, SIZE_MAX, 4, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, SIZE_MAX, 4, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, false) },
                    { { "sa", 5 }, SSAInfo(2, 4, 4, 4, 1, false) }
                }, false, false, false },
            {   // block 6 - end
                76, 88,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 }, { 4, 1 } } },
            { { "sa", 3 }, { { 3, 1 } } },
            { { "sa", 5 }, { { 4, 2 }, { 5, 1 } } }
        },
        true, ""
    },
    {   // 5 - two routines with common code
        R"ffDXD(.regvar sa:s:8, va:v:8, xa:s:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        s_mov_b32 sa[6], s8
        s_mov_b32 xa[0], s9
        s_mov_b32 xa[1], s10
        
        .cf_call routine1
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 xa[0], xa[0], sa[0]
        s_xor_b32 xa[1], xa[1], sa[0]
        s_endpgm
        
routine1:
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[1]
        
        s_add_u32 sa[4], sa[3], sa[0]
        s_add_u32 sa[5], sa[3], sa[5]
        s_add_u32 sa[6], sa[6], sa[1]
        
        s_xor_b32 xa[1], xa[1], sa[0]
        
common:
        s_add_u32 sa[5], sa[5], sa[0]
        s_add_u32 sa[3], sa[3], sa[1]
        s_xor_b32 sa[4], sa[4], sa[2]
        .cf_ret
        s_setpc_b64 s[0:1]

routine2:
        s_add_u32 sa[2], sa[2], sa[0]
        s_add_u32 sa[3], sa[3], sa[1]
        
        s_add_u32 sa[4], sa[1], sa[2]
        s_add_u32 sa[5], sa[3], sa[5]
        
        s_xor_b32 xa[0], xa[0], sa[0]
        s_branch common
)ffDXD",
        {
            {   // block 0 - start
                0, 32,
                { { 3, true } },
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
                    { { "", 9 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 10 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "xa", 0 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "xa", 1 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after routine1 call
                32, 44,
                { { 5, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, true, false, false },
            {   // block 2 - after routine2 call
                44, 68,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "xa", 0 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "xa", 1 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 3 - routine1
                68, 92,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "xa", 1 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 4 - common part
                92, 108,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 5 - routine2
                108, 132,
                { { 4, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, false) },
                    { { "sa", 5 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "xa", 0 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 2 } } },
            { { "sa", 3 }, { { 4, 2 } } },
            { { "sa", 4 }, { { 4, 3 }, { 4, 2 } } },
            { { "sa", 5 }, { { 5, 2 } } }
        },
        true, ""
    },
    {   // 6 - simple call, more complex routine (reduce with reads)
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[5], s6
        s_mov_b32 sa[6], s7
        
        s_getpc_b64 s[2:3]
        s_add_u32 s2, s2, routine-.
        s_add_u32 s3, s3, routine-.+4
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[3], 4
        s_lshl_b32 sa[5], sa[5], 4
        s_lshl_b32 sa[6], sa[6], 4
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[4]
        s_xor_b32 sa[3], sa[3], sa[4]
        s_cbranch_scc1 bb1
        
        s_min_u32 sa[2], sa[2], sa[4]
        s_min_u32 sa[3], sa[3], sa[4]
        s_xor_b32 sa[5], sa[5], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[4]
        s_and_b32 sa[3], sa[3], sa[4]
        s_xor_b32 sa[6], sa[6], sa[4]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            { 0, 40,
                { { 2, true } },
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
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            // block 1 - after call
            { 40, 60,
                { },
                {
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, false, true },
            // block 2 - routine
            { 60, 72,
                { { 3, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - first return
            { 72, 88,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            // block 4 - second return
            { 88, 104,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 } } },
            { { "sa", 3 }, { { 4, 3 } } },
            { { "sa", 5 }, { { 2, 1 } } },
            { { "sa", 6 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 7 - retssa tests
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s6
        s_mov_b32 sa[6], s7
        s_mov_b32 sa[7], s7
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[1], 4
        s_cbranch_scc1 aa1
        
aa0:    s_lshl_b32 sa[5], sa[1], 4
        s_lshl_b32 sa[4], sa[1], 4
        s_branch ca0
        
aa1:    s_lshl_b32 sa[6], sa[1], 4
        s_lshl_b32 sa[3], sa[3], 4
        s_lshl_b32 sa[5], sa[1], 4
        
ca0:    .cf_call routine
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
        
bb0:    s_min_u32 sa[2], sa[2], sa[0]
        s_min_u32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[0]
        s_and_b32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

bb2:    s_and_b32 sa[2], sa[2], sa[0]
        s_and_b32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 28,
                { { 8, true } },
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
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 7 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after call
                28, 40,
                { { 2, false }, { 3, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 6, 6, 6, 1, false) }
                }, false, false, false },
            {   // block 2 - aa0
                40, 52,
                { { 4, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 5, 5, 5, 1, false) },
                    { { "sa", 5 }, SSAInfo(1, 5, 5, 5, 1, false) }
                }, false, false, true },
            {   // block 3 - aa1
                52, 64,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 6, 6, 6, 1, false) },
                    { { "sa", 6 }, SSAInfo(1, 7, 7, 7, 1, false) }
                }, false, false, false },
            {   // block 4 - ca0
                64, 68,
                { { 8, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 5 - last cond
                68, 80,
                { { 6, false }, { 7, false } },
                {
                    { { "sa", 3 }, SSAInfo(3, SIZE_MAX, 7, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, SIZE_MAX, 6, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 5, 5, 6, 2, false) }
                }, false, false, false },
            {   // block 6 - end1
                80, 88,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 7 }, SSAInfo(1, 5, 5, 5, 1, false) }
                }, false, false, true },
            {   // block 7 - end2
                88, 96,
                { },
                {
                    { { "sa", 7 }, SSAInfo(1, 6, 6, 6, 1, true) }
                }, false, false, true },
            {   // block 8 - routine
                96, 108,
                { { 9, false }, { 10, false }, { 11, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 9 - bb0
                108, 136,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 10 - bb1
                136, 164,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 11 - bb2
                164, 192,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 5, 3 }, { 6, 1 } } },
            { { "sa", 3 }, { { 4, 3 }, { 5, 3 }, { 6, 1 }, { 7, 1 } } },
            { { "sa", 4 }, { { 3, 2 }, { 4, 2 }, { 5, 1 }, { 2, 1 } } },
            { { "sa", 5 }, { { 5, 1 }, { 6, 1 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 7, 1 } } },
            //{ { "sa", 7 }, { { 3, 2 }, { 4, 2 }, { 2, 1 } } }
            { { "sa", 7 }, { { 2, 1 }, { 3, 1 }, { 4, 1 } } }
        },
        true, ""
    },
    {   // 8 - retssa tests
        R"ffDXD(.regvar sa:s:8, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s5
        s_mov_b32 sa[6], s6
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[3], sa[1], 4
        s_cbranch_scc1 aa1
        
aa0:    s_lshl_b32 sa[2], sa[2], 3
        s_lshl_b32 sa[4], sa[3], 3
        s_lshl_b32 sa[6], sa[5], 3
        s_endpgm

aa1:    s_lshl_b32 sa[2], sa[1], 3
        s_lshl_b32 sa[2], sa[3], 3
        s_lshl_b32 sa[4], sa[4], 3
        s_lshl_b32 sa[6], sa[6], 3
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        .cf_cjump bb1, bb2
        s_setpc_b64 s[0:1]
        
bb0:    s_min_u32 sa[2], sa[2], sa[0]
        s_min_u32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[0]
        s_and_b32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]

bb2:    s_and_b32 sa[2], sa[2], sa[0]
        s_and_b32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
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
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after call
                24, 36,
                { { 2, false }, { 3, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 6, 6, 6, 1, false) }
                }, false, false, false },
            {   // block 2 - aa0
                36, 52,
                { },
                {
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, SIZE_MAX, 7, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 5, 5, 5, 1, false) },
                    { { "sa", 5 }, SSAInfo(2, SIZE_MAX, 5, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 5, 5, 5, 1, false) }
                }, false, false, true },
            {   // block 3 - aa1
                52, 72,
                { },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 8, 8, 9, 2, false) },
                    { { "sa", 3 }, SSAInfo(6, SIZE_MAX, 7, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 6, 6, 6, 1, true) }
                }, false, false, true },
            {   // block 4 - routine
                72, 84,
                { { 5, false }, { 6, false }, { 7, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - bb0
                84, 108,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 6 - bb1
                108, 132,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 7 - bb2
                132, 156,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 5, 3 } } },
            { { "sa", 4 }, { { 3, 2 }, { 4, 2 } } },
            { { "sa", 5 }, { { 3, 2 }, { 4, 2 } } },
            { { "sa", 6 }, { { 3, 2 }, { 4, 2 } } }
        },
        true, ""
    },
    {   // 9 - retssa tests
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s5
        v_mov_b32 va[1], v0
        v_mov_b32 va[2], v1
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        s_cbranch_vccnz aa1
aa0:    s_xor_b32 sa[2], sa[1], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_cbranch_vccnz aa3
aa2:    s_xor_b32 sa[6], sa[1], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        v_xor_b32 va[1], va[0], va[0]
        v_xor_b32 va[2], va[0], va[0]
        s_endpgm
        
aa3:    s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        s_xor_b32 sa[8], sa[8], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        v_xor_b32 va[1], va[1], va[0]
        v_xor_b32 va[0], va[2], va[0]
        s_endpgm
        
aa1:    s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[2], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_execz aa5
        
aa4:    s_xor_b32 sa[4], sa[4], sa[0]
        v_xor_b32 va[1], va[1], va[0]
        s_endpgm
        
aa5:    s_xor_b32 sa[4], sa[4], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_vccz bb1
        
bb0:    s_min_u32 sa[2], sa[2], sa[0]
        s_min_u32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        v_xor_b32 va[1], va[1], va[0]
        v_xor_b32 va[2], va[2], va[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[0]
        s_and_b32 sa[3], sa[3], sa[0]
        s_max_u32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        v_xor_b32 va[1], va[1], va[0]
        v_xor_b32 va[2], va[2], va[0]
        .cf_ret
        s_setpc_b64 s[0:1]

routine2:
        s_xor_b32 sa[6], sa[6], sa[1]
        s_xor_b32 sa[7], sa[7], sa[1]
        s_cbranch_vccz bb3
        
bb2:    s_min_u32 sa[6], sa[6], sa[1]
        s_min_u32 sa[7], sa[7], sa[1]
        s_max_u32 sa[8], sa[8], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb3:    s_and_b32 sa[6], sa[6], sa[1]
        s_and_b32 sa[7], sa[7], sa[1]
        s_max_u32 sa[8], sa[8], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 28,
                { { 9, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 6 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256+1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 1 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - branch aa0-aa1
                28, 32,
                { { 2, false }, { 6, false } },
                {
                }, false, false, false },
            {   // block 2 - aa0
                32, 44,
                { { 12, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, false) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, true, false, false },
            {   // block 3 - branch aa2-aa3
                44, 48,
                { { 4, false }, { 5, false } },
                {
                }, false, false, false },
            {   // block 4 - aa2
                48, 68,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(0, 4, 4, 4, 1, false) },
                    { { "sa", 7 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "va", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, 4, 4, 4, 1, false) },
                    { { "va", 2 }, SSAInfo(1, 4, 4, 4, 1, false) }
                }, false, false, true },
            {   // block 5 - aa3
                68, 96,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(4, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 7 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 8 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "va", 0 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "va", 1 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "va", 2 }, SSAInfo(2, SIZE_MAX, 5, SIZE_MAX, 0, true) }
                }, false, false, true },
            {   // block 6 - aa1
                96, 116,
                { { 7, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 7, 2, true) },
                    { { "sa", 3 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, SIZE_MAX, 4, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 7, 7, 7, 1, true) }
                }, false, false, false },
            {   // block 7 - aa4
                116, 128,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "va", 0 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(2, 6, 6, 6, 1, true) }
                }, false, false, true },
            {   // block 8 - aa5
                128, 136,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 9 - routine
                136, 148,
                { { 10, false }, { 11, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 10 - bb0
                148, 176,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "va", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "va", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, true, true },
            {   // block 11 - bb1
                176, 204,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "va", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "va", 2 }, SSAInfo(1, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 12 - routine2
                204, 216,
                { { 13, false }, { 14, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, true) },
                    { { "sa", 7 }, SSAInfo(0, 1, 1, 1, 1, true) }
                }, false, false, false },
            {   // block 13 - bb2
                216, 236,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 8 }, SSAInfo(0, 1, 1, 1, 1, true) }
                }, false, true, true },
            {   // block 14 - bb3
                236, 256,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 8 }, SSAInfo(0, 2, 2, 2, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 } } },
            { { "sa", 3 }, { { 4, 3 } } },
            { { "sa", 4 }, { { 3, 2 } } },
            { { "sa", 5 }, { { 3, 2 }, { 5, 4 } } },
            { { "sa", 6 }, { { 3, 2 } } },
            { { "sa", 7 }, { { 3, 2 } } },
            { { "sa", 8 }, { { 2, 1 } } },
            { { "va", 1 }, { { 3, 2 } } },
            { { "va", 2 }, { { 3, 2 } } }
        },
        true, ""
    },
    {   // 10 - retssa tests 2
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_execz aa1
        
aa0:    .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        s_and_b32 sa[3], sa[3], sa[0]
        s_endpgm
        
aa1:    s_xor_b32 sa[4], sa[4], sa[1]
        s_and_b32 sa[2], sa[2], sa[0]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_vccz bb1
        
bb0:    s_min_u32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
bb1:    s_and_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine2:
        s_xor_b32 sa[2], sa[1], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_vccz cc1
        
cc0:    s_min_u32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[2], sa[1], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
cc1:    s_and_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[2], sa[1], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 12,
                { { 5, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after routine call
                12, 20,
                { { 2, false }, { 4, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(0, 1, 1, 1, 1, true) }
                }, false, false, false },
            {   // block 2 - aa0
                20, 24,
                { { 8, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 3 - after routine2 call
                24, 32,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 4 - aa1
                32, 44,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 8, 8, 8, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 5 - routine
                44, 52,
                { { 6, false }, { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 6 - bb0
                52, 60,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 7 - bb1
                60, 68,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 8 - routine2
                68, 80,
                { { 9, false }, { 10, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, false) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 9 - cc0
                80, 92,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, false) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 10 - cc1
                92, 104,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 7, 7, 7, 1, false) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 } } },
            { { "sa", 3 }, { { 4, 3 } } }
        },
        true, ""
    },
    {   // 11 - simple routine with loop
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
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 }, { 5, 1 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 4, 1 } } }
        },
        true, ""
    },
    {   // 12
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        .cf_call routine2
        s_swappc_b64 s[0:1], s[2:3]
        
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        s_endpgm
        
routine:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_scc1 aa0
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
aa0:    s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
routine2:
        s_and_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_scc1 aa1
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_and_b32 sa[3], sa[3], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
        
aa1:    s_and_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[1]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 12,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - after routine call
                12, 24,
                { { 7, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, true, false, false },
            {   // block 2 - after routine2 call
                24, 28,
                { { 4, true } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "", 2 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, true, false, false },
            {   // block 3 - end
                28, 32,
                { },
                { },
                false, false, true },
            {   // block 4 - routine
                32, 44,
                { { 5, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - first fork
                44, 56,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 6 - aa0 - second fork
                56, 68,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 7 - routine2
                68, 80,
                { { 8, false }, { 9, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(5, 6, 6, 6, 1, true) }
                }, false, false, false },
            {   // block 8 - first fork
                80, 92,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) }
                }, false, true, true },
            {   // block 9 - aa1 - second fork
                92, 104,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 8, 8, 8, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 7, 1 }, { 8, 1 } } },
            { { "sa", 3 }, { { 4, 3 }, { 7, 1 }, { 8, 1 } } }
        },
        true, ""
    },
    {   // 13 - routine with complex loop used twice
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
                    { { "sa", 2 }, SSAInfo(3, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 4, 4, 4, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 64,
                { { 3, false }, { 10, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - loop0
                64, 72,
                { { 4, false }, { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - to end2
                72, 80,
                { { 5, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 5 - to end3
                80, 88,
                { { 6, false }, { 9, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 6 - loop end
                88, 96,
                { { 3, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, true },
            {   // block 7 - end1
                96, 104,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 8 - end2
                104, 112,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 9 - end3
                112, 120,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 10 - subr1
                120, 144,
                { { 3, false }, { 11, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 11 - subr2
                144, 168,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 5 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 6, 3 }, { 3, 2 }, { 5, 2 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 } } },
            { { "sa", 4 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 } } },
            { { "sa", 5 }, { { 3, 2 }, { 4, 2 }, { 5, 2 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 } } }
        },
        true, ""
    },
    {   // 14 - routine with double complex loop used four times
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        s_mov_b32 sa[6], s8
        s_mov_b32 sa[7], s8
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        s_endpgm
        
routine:
loop0:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_vccz  subr1
        
loop1:  s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_scc0 end1
        
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_scc0 end2
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc0 end3
        
        s_xor_b32 sa[5], sa[5], sa[1]
        s_cbranch_vccnz loop1
        
        s_xor_b32 sa[6], sa[6], sa[1]
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
        s_xor_b32 sa[7], sa[7], sa[0]
        s_cbranch_execnz loop0
        
subr2:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        s_cbranch_execnz loop1
        
subr3:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 28,
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
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 7 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                28, 56,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 8, 8, 8, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 7, 7, 7, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 6, 6, 6, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 2 - routine/loop0
                56, 72,
                { { 3, false }, { 11, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - routine loop1
                72, 80,
                { { 4, false }, { 8, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - to end2
                80, 88,
                { { 5, false }, { 9, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 5 - to end3
                88, 96,
                { { 6, false }, { 10, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 6 - loop1 end
                96, 104,
                { { 3, false }, { 7, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 7 - loop0 end
                104, 112,
                { { 2, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 8 - end1
                112, 120,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 9 - end2
                120, 128,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 10 - end3
                128, 136,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 11 - subr1
                136, 164,
                { { 2, false }, { 12, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 12 - subr2
                164, 192,
                { { 3, false }, { 13, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 5 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 7 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 13 - subr3
                192, 220,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 7 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 7, 3 }, { 3, 2 }, { 3, 1 }, { 5, 1 }, { 6, 2 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 6, 2 }, { 7, 2 }, { 3, 1 }, { 5, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 } } },
            { { "sa", 5 }, { { 3, 2 }, { 5, 2 }, { 6, 2 }, { 3, 1 }, { 4, 1 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 } } },
            { { "sa", 7 }, { { 3, 1 }, { 4, 1 }, { 2, 1 } } }
        },
        true, ""
    },
    {   // 15 - routine with double complex loop used four times
        R"ffDXD(.regvar sa:s:10, va:v:8
        s_mov_b32 sa[2], s4
        s_mov_b32 sa[3], s5
        s_mov_b32 sa[4], s6
        s_mov_b32 sa[5], s7
        s_mov_b32 sa[6], s8
        s_mov_b32 sa[7], s8
        .cf_call routine
        s_swappc_b64 s[0:1], s[2:3]
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        s_endpgm
        
routine:
        s_nop 7
loop0:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_vccz  subr1
        
loop1:  s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_scc0 end1
        
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_scc0 end2
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc0 end3
        
        s_xor_b32 sa[5], sa[5], sa[1]
        s_cbranch_vccnz loop1
        
        s_xor_b32 sa[6], sa[6], sa[1]
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
        s_xor_b32 sa[7], sa[7], sa[0]
        s_cbranch_execnz loop0
        
subr2:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        s_cbranch_execnz loop1
        
subr3:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_xor_b32 sa[7], sa[7], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
)ffDXD",
        {
            {   // block 0 - start
                0, 28,
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
                    { { "sa", 6 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 7 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, true, false, false },
            {   // block 1 - end
                28, 56,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 8, 8, 8, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 7, 7, 7, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 6, 6, 6, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 2 - s_nop
                56, 60,
                { },
                { }, false, false, false },
            {   // block 3 - routine/loop0
                60, 76,
                { { 4, false }, { 12, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 4 - routine loop1
                76, 84,
                { { 5, false }, { 9, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 5 - to end2
                84, 92,
                { { 6, false }, { 10, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 6 - to end3
                92, 100,
                { { 7, false }, { 11, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 7 - loop1 end
                100, 108,
                { { 4, false }, { 8, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 8 - loop0 end
                108, 116,
                { { 3, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 9 - end1
                116, 124,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 10 - end2
                124, 132,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 11 - end3
                132, 140,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 12 - subr1
                140, 168,
                { { 3, false }, { 13, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 7 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 13 - subr2
                168, 196,
                { { 4, false }, { 14, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 5 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 6 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 7 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 13 - subr3
                196, 224,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 7 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 7, 3 }, { 3, 2 }, { 3, 1 }, { 5, 1 }, { 6, 2 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 6, 2 }, { 7, 2 }, { 3, 1 }, { 5, 1 } } },
            //{ { "sa", 4 }, { { 2, 1 }, { 3, 1 }, { 5, 1 }, { 6, 1 }, { 4, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 } } },
            { { "sa", 5 }, { { 3, 2 }, { 5, 2 }, { 6, 2 }, { 3, 1 }, { 4, 1 } } },
            //{ { "sa", 6 }, { { 2, 1 }, { 4, 1 }, { 5, 1 }, { 3, 1 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 } } },
            { { "sa", 7 }, { { 3, 1 }, { 4, 1 }, { 2, 1 } } }
        },
        true, ""
    },
    {   // 16 - routine with complex loop at start
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
loop0:  s_xor_b32 sa[2], sa[2], sa[0]
        s_cbranch_scc0 end1
        
        s_xor_b32 sa[3], sa[3], sa[0]
        s_cbranch_scc0 end2
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc0 end3
        
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
                    { { "sa", 2 }, SSAInfo(2, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 2 - routine, loop
                48, 56,
                { { 3, false }, { 6, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - to end2
                56, 64,
                { { 4, false }, { 7, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 4 - to end3
                64, 72,
                { { 5, false }, { 8, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 5 - loop end
                72, 80,
                { { 2, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, true },
            {   // block 6 - end1
                80, 88,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 7 - end2
                88, 96,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true },
            {   // block 8 - end3
                96, 104,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 3, 2 }, { 2, 1 } } },
            { { "sa", 3 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 4 }, { { 2, 1 }, { 3, 1 } } },
            { { "sa", 5 }, { { 2, 1 } } }
        },
        true, ""
    },
    {   // 17 - routine with two consecutive complex loops
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
        
        s_xor_b32 sa[5], sa[5], sa[1]
        s_cbranch_vccnz loop0
        
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_cbranch_vccz  subr2

loop1:  s_xor_b32 sa[4], sa[4], sa[0]
        s_cbranch_scc0 end4
        
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_scc0 end5
        
        s_xor_b32 sa[6], sa[6], sa[1]
        s_cbranch_scc0 end6
        
        s_xor_b32 sa[3], sa[3], sa[1]
        s_branch loop1
        
end1:   s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end2:   s_xor_b32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end3:   s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end4:   s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end5:   s_xor_b32 sa[5], sa[5], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end6:   s_xor_b32 sa[6], sa[6], sa[0]
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
        s_cbranch_execnz loop1
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[1]
        s_xor_b32 sa[5], sa[5], sa[1]
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
                    { { "sa", 2 }, SSAInfo(3, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 10, 10, 10, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 10, 10, 10, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 9, 9, 9, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 8, 8, 8, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 64,
                { { 3, false }, { 18, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - loop0
                64, 72,
                { { 4, false }, { 12, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - to end2
                72, 80,
                { { 5, false }, { 13, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 5 - to end3
                80, 88,
                { { 6, false }, { 14, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 6 - loop0 end
                88, 96,
                { { 3, false }, { 7, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 7 - after loop0
                96, 112,
                { { 8, false }, { 19, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 8 - loop1
                112, 120,
                { { 9, false }, { 15, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, false },
            {   // block 9 - to end5
                120, 128,
                { { 10, false }, { 16, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, false },
            {   // block 10 - to end6
                128, 136,
                { { 11, false }, { 17, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 11 - loop1 end
                136, 144,
                { { 8, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, false, true },
            {   // block 12 - end1
                144, 152,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 6, 6, 6, 1, true) }
                }, false, true, true },
            {   // block 13 - end2
                152, 160,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 8, 8, 8, 1, true) }
                }, false, true, true },
            {   // block 14 - end3
                160, 168,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 8, 8, 8, 1, true) }
                }, false, true, true },
            {   // block 15 - end4
                168, 176,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 16 - end5
                176, 184,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(4, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 17 - end6
                184, 192,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 18 - subr1
                192, 216,
                { { 3, false }, { 19, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 9, 9, 9, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 9, 9, 9, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 8, 8, 8, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 7, 7, 7, 1, true) }
                }, false, false, false },
            {   // block 19 - subr2
                216, 240,
                { { 8, false }, { 20, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 3 }, SSAInfo(4, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(3, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(2, 5, 5, 5, 1, true) }
                }, false, false, false },
            {   // block 20 - after subr2
                240, 264,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(4, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 5 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 6 }, SSAInfo(5, 6, 6, 6, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 5, 3 }, { 6, 3 }, { 7, 3 }, { 3, 2 }, { 7, 2 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 }, { 7, 2 }, { 8, 2 },
                        { 9, 2 }, { 5, 4 }, { 6, 4 }, { 9, 4 } } },
            { { "sa", 4 }, { { 2, 1 }, { 4, 1 }, { 5, 1 }, { 7, 1 }, { 8, 1 }, { 9, 1 },
                        { 4, 3 }, { 6, 3 }, { 9, 3 } } },
            { { "sa", 5 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 }, { 7, 2 }, { 8, 2 },
                        { 4, 3 }, { 6, 3 }, { 8, 3 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 }, { 7, 1 },
                        { 3, 2 }, { 5, 2 }, { 7, 2 } } }
        },
        true, ""
    },
    {   // 18 - routine with nested loops
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
        
        s_xor_b32 sa[6], sa[6], sa[0]
        
loop1:  s_xor_b32 sa[4], sa[4], sa[0]
        s_cbranch_scc0 end4
        
        s_xor_b32 sa[5], sa[5], sa[0]
        s_cbranch_scc0 end5
        
        s_xor_b32 sa[6], sa[6], sa[1]
        s_cbranch_scc0 end6
        
        s_xor_b32 sa[3], sa[3], sa[1]
        s_cbranch_execnz loop1
        
        s_xor_b32 sa[4], sa[4], sa[1]
        s_cbranch_scc0 end3
        
        s_xor_b32 sa[5], sa[5], sa[1]
        s_cbranch_vccnz loop0
        
        .cf_ret
        s_setpc_b64 s[0:1]
        
end1:   s_xor_b32 sa[2], sa[2], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end2:   s_xor_b32 sa[3], sa[3], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end3:   s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end4:   s_xor_b32 sa[4], sa[4], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end5:   s_xor_b32 sa[5], sa[5], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
end6:   s_xor_b32 sa[6], sa[6], sa[0]
        .cf_ret
        s_setpc_b64 s[0:1]
        
subr1:
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_cbranch_execnz loop0
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[1]
        s_xor_b32 sa[6], sa[6], sa[0]
        s_cbranch_execnz loop1
        
        s_xor_b32 sa[2], sa[2], sa[0]
        s_xor_b32 sa[3], sa[3], sa[0]
        s_xor_b32 sa[4], sa[4], sa[0]
        s_xor_b32 sa[5], sa[5], sa[1]
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
                    { { "sa", 2 }, SSAInfo(3, 8, 8, 8, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 9, 9, 9, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 9, 9, 9, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 9, 9, 9, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 8, 8, 8, 1, true) }
                }, false, false, true },
            {   // block 2 - routine
                48, 64,
                { { 3, false }, { 19, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 5 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 3 - loop0
                64, 72,
                { { 4, false }, { 13, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 4 - to end2
                72, 80,
                { { 5, false }, { 14, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 5 - before loop1
                80, 84,
                { },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 6 - loop1
                84, 92,
                { { 7, false }, { 16, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(1, 2, 2, 2, 1, true) }
                }, false, false, false },
            {   // block 7 - to end5
                92, 100,
                { { 8, false }, { 17, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 8 - to end6
                100, 108,
                { { 9, false }, { 18, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 9 - loop1 end
                108, 116,
                { { 6, false }, { 10, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, false },
            {   // block 10 - to end3
                116, 124,
                { { 11, false }, { 15, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 3, 3, 3, 1, true) }
                }, false, false, false },
            {   // block 11 - loop0 end
                124, 132,
                { { 3, false }, { 12, false } },
                {
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, false, false },
            {   // block 12 - routine end
                132, 136,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) }
                }, false, true, true },
            {   // block 13 - end1
                136, 144,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 14 - end2
                144, 152,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 15 - end3
                152, 160,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 16 - end4
                160, 168,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(2, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 17 - end5
                168, 176,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 5 }, SSAInfo(3, 5, 5, 5, 1, true) }
                }, false, true, true },
            {   // block 18 - end6
                176, 184,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 6 }, SSAInfo(3, 4, 4, 4, 1, true) }
                }, false, true, true },
            {   // block 19 - subr1
                184, 208,
                { { 3, false }, { 20, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(2, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(2, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(1, 6, 6, 6, 1, true) },
                    { { "sa", 5 }, SSAInfo(2, 6, 6, 6, 1, true) },
                    { { "sa", 6 }, SSAInfo(1, 5, 5, 5, 1, true) }
                }, false, false, false },
            {   // block 20 - subr2
                208, 232,
                { { 6, false }, { 21, false } },
                {
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 3 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 4 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 5 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 6 }, SSAInfo(5, 6, 6, 6, 1, true) }
                }, false, false, false },
            {   // block 21 - after subr2
                232, 256,
                { },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 2 }, SSAInfo(6, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(7, 8, 8, 8, 1, true) },
                    { { "sa", 4 }, SSAInfo(7, 8, 8, 8, 1, true) },
                    { { "sa", 5 }, SSAInfo(7, 8, 8, 8, 1, true) },
                    { { "sa", 6 }, SSAInfo(6, 7, 7, 7, 1, true) }
                }, false, true, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 3 }, { 6, 3 }, { 7, 3 }, { 3, 2 },
                        { 5, 2 }, { 6,  2 } } },
            { { "sa", 3 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 }, { 7, 2 },
                        { 8, 2 }, { 4, 3 }, { 7, 3 } } },
            /*{ { "sa", 4 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 },
                        { 6, 1 }, { 8, 1 }, { 7, 1 } } },*/
            { { "sa", 4 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 },
                        { 6, 1 }, { 7, 1 }, { 8, 1 } } },
            { { "sa", 5 }, { { 3, 2 }, { 4, 2 }, { 5, 2 }, { 6, 2 },
                        { 7, 2 }, { 8, 2 } } },
            { { "sa", 6 }, { { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 },
                        { 6, 1 }, { 7, 1 }, { 3, 2 }, { 6, 2 } } }
        },
        true, ""
    },
    { nullptr }
};
