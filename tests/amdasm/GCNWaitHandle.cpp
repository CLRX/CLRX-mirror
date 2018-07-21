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
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/GCNDefs.h>
#include "../TestUtils.h"

using namespace CLRX;

struct AsmDelayedOpData
{
    size_t offset;
    const char* regVarName;
    uint16_t rstart;
    uint16_t rend;
    cxbyte count;
    cxbyte delayedOpType;
    cxbyte delayedOpType2;
    cxbyte rwFlags;
    cxbyte rwFlags2;
};

struct AsmWaitHandlerCase
{
    const char* input;
    Array<AsmWaitInstr> waitInstrs;
    Array<AsmDelayedOpData> delayedOps;
    bool good;
    const char* errorMessages;
};

static const AsmWaitHandlerCase waitHandlerTestCases[] =
{
    {   /* 0 - first test - empty */
        R"ffDXD(
            .regvar bax:s, dbx:v, dcx:s:8
            s_mov_b32 bax, s11
            s_mov_b32 bax, dcx[1]
            s_branch aa0
            s_endpgm
aa0:        s_add_u32 bax, dcx[1], dcx[2]
            s_endpgm
)ffDXD",
        { }, { }, true, ""
    },
    {   /* 1 - SMRD instr */
        R"ffDXD(
            .regvar bax:s, dbx:v, dcx:s:8
            s_mov_b32 bax, s11
            s_load_dword dcx[2], s[10:11], 4
            s_mov_b32 bax, dcx[1]
            s_waitcnt lgkmcnt(0)
            s_branch aa0
            s_endpgm
aa0:        s_add_u32 bax, dcx[1], dcx[2]
            s_endpgm
)ffDXD",
        {
            // s_waitcnt lgkmcnt(0)
            { 12U, { 15, 0, 7, 0 } },
        },
        {
            // s_load_dword dcx[2], s[10:11], 4
            { 4U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 2 - s_waitcnt tests */
        R"ffDXD(.gpu Bonaire
            s_waitcnt vmcnt(3) & lgkmcnt(5) & expcnt(4)
            s_waitcnt vmcnt(3) & lgkmcnt(5)
            s_waitcnt vmcnt(3)
            s_waitcnt expcnt(3)
            s_waitcnt lgkmcnt(3)
            s_waitcnt vmcnt(3) & expcnt(4)
            s_waitcnt lgkmcnt(3) & expcnt(4)
            s_waitcnt vmcnt(15) lgkmcnt(15) expcnt(7)
            s_waitcnt vmcnt(15) lgkmcnt(11) expcnt(7)
)ffDXD",
        {
            { 0U, { 3, 5, 4, 0 } },
            { 4U, { 3, 5, 7, 0 } },
            { 8U, { 3, 15, 7, 0 } },
            { 12U, { 15, 15, 3, 0 } },
            { 16U, { 15, 3, 7, 0 } },
            { 20U, { 3, 15, 4, 0 } },
            { 24U, { 15, 3, 4, 0 } },
            { 28U, { 15, 15, 7, 0 } },
            { 32U, { 15, 11, 7, 0 } }
        },
        { }, true, ""
    },
    {   /* 3 - s_waitcnt tests (GCN 1.0) */
        R"ffDXD(
            s_waitcnt vmcnt(3) & lgkmcnt(5) & expcnt(4)
            s_waitcnt vmcnt(3) & lgkmcnt(5)
            s_waitcnt vmcnt(3)
            s_waitcnt expcnt(3)
            s_waitcnt lgkmcnt(3)
            s_waitcnt vmcnt(3) & expcnt(4)
            s_waitcnt lgkmcnt(3) & expcnt(4)
            s_waitcnt vmcnt(15) lgkmcnt(15) expcnt(7)
            s_waitcnt vmcnt(15) lgkmcnt(11) expcnt(7)
)ffDXD",
        {
            { 0U, { 3, 5, 4, 0 } },
            { 4U, { 3, 5, 7, 0 } },
            { 8U, { 3, 7, 7, 0 } },
            { 12U, { 15, 7, 3, 0 } },
            { 16U, { 15, 3, 7, 0 } },
            { 20U, { 3, 7, 4, 0 } },
            { 24U, { 15, 3, 4, 0 } },
            { 28U, { 15, 7, 7, 0 } },
            { 32U, { 15, 7, 7, 0 } }
        },
        { }, true, ""
    },
    {   /* 4 - s_waitcnt tests (GFX9) */
        R"ffDXD(.arch GFX9
            s_waitcnt vmcnt(47) & lgkmcnt(5) & expcnt(4)
            s_waitcnt lgkmcnt(5)
)ffDXD",
        {
            { 0U, { 47, 5, 4, 0 } },
            { 4U, { 63, 5, 7, 0 } }
        },
        { }, true, ""
    },
    {   /* 5 - SMRD */
         R"ffDXD(
            .regvar bax:s, dbx:v, dcx:s:8
            .regvar bb:s:28
            s_load_dword dcx[2], s[10:11], 4
            s_load_dwordx2 dcx[4:5], s[10:11], 4
            s_load_dwordx4 bb[4:7], s[10:11], 4
            s_load_dwordx8 bb[16:23], s[10:11], 4
            s_load_dwordx16 bb[12:27], s[10:11], 4
            s_buffer_load_dword dcx[2], s[12:15], 4
            s_buffer_load_dwordx2 dcx[4:5], s[12:15], 4
            s_buffer_load_dwordx4 bb[4:7], s[12:15], 4
            s_buffer_load_dwordx8 bb[16:23], s[12:15], 4
            s_buffer_load_dwordx16 bb[12:27], s[12:15], 4
            s_memtime s[4:5]
            s_memtime dcx[5:6]
            s_buffer_load_dwordx2 vcc, s[12:15], 4
)ffDXD",
        { },
        {
            { 0U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 4U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 8U, "bb", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 12U, "bb", 16, 24, 8, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 16U, "bb", 12, 28, 16, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 20U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 24U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 28U, "bb", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 32U, "bb", 16, 24, 8, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 36U, "bb", 12, 28, 16, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 40U, nullptr, 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 44U, "dcx", 5, 7, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 48U, nullptr, 106, 108, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 6 - SMEM */
         R"ffDXD(.gpu Fiji
            .regvar bax:s, dbx:v, dcx:s:8
            .regvar bb:s:28
            s_load_dword dcx[2], s[10:11], 4
            s_load_dwordx2 dcx[4:5], s[10:11], 4
            s_load_dwordx4 bb[4:7], s[10:11], 4
            s_load_dwordx8 bb[16:23], s[10:11], 4
            s_load_dwordx16 bb[12:27], s[10:11], 4
            s_buffer_load_dword dcx[2], s[12:15], 4
            s_buffer_load_dwordx2 dcx[4:5], s[12:15], 4
            s_buffer_load_dwordx4 bb[4:7], s[12:15], 4
            s_buffer_load_dwordx8 bb[16:23], s[12:15], 4
            s_buffer_load_dwordx16 bb[12:27], s[12:15], 4
            s_memtime s[4:5]
            s_memtime dcx[5:6]
            s_store_dword dcx[2], s[10:11], 4
            s_store_dwordx2 dcx[4:5], s[10:11], 4
            s_store_dwordx4 bb[4:7], s[10:11], 4
            s_buffer_store_dword dcx[2], s[20:23], 4
            s_buffer_store_dwordx2 dcx[4:5], s[20:23], 4
            s_buffer_store_dwordx4 bb[4:7], s[20:23], 4
)ffDXD",
        { },
        {
            { 0U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 8U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 16U, "bb", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 24U, "bb", 16, 24, 8, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 32U, "bb", 12, 28, 16, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 40U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 48U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 56U, "bb", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 64U, "bb", 16, 24, 8, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 72U, "bb", 12, 28, 16, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 80U, nullptr, 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 88U, "dcx", 5, 7, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 96U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 104U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 112U, "bb", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 120U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 128U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 136U, "bb", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ }
        }, true, ""
    },
    {   /* 7 - SMEM (GFX9) */
         R"ffDXD(.gpu GFX900
            .regvar bax:s, dbx:v, dcx:s:8
            s_atomic_swap bax, s[14:15], 18
            s_atomic_cmpswap dcx[2:3], s[14:15], 18
            s_atomic_umin bax, s[14:15], 20
            s_atomic_swap_x2 dcx[2:3], s[14:15], 18
            s_atomic_cmpswap_x2 dcx[4:7], s[14:15], 18
            s_atomic_umin_x2 dcx[2:3], s[14:15], 20
            s_atomic_swap bax, s[14:15], 18 glc
            s_atomic_cmpswap dcx[2:3], s[14:15], 18 glc
            s_atomic_umin bax, s[14:15], 20 glc
            s_atomic_swap_x2 dcx[2:3], s[14:15], 18 glc
            s_atomic_cmpswap_x2 dcx[4:7], s[14:15], 18 glc
            s_atomic_umin_x2 dcx[2:3], s[14:15], 20 glc
            
            s_buffer_atomic_swap bax, s[16:19], 18
            s_buffer_atomic_cmpswap dcx[2:3], s[16:19], 18
            s_buffer_atomic_umin bax, s[16:19], 20
            s_buffer_atomic_swap_x2 dcx[2:3], s[16:19], 18
            s_buffer_atomic_cmpswap_x2 dcx[4:7], s[16:19], 18
            s_buffer_atomic_umin_x2 dcx[2:3], s[16:19], 20
            s_buffer_atomic_swap bax, s[16:19], 18 glc
            s_buffer_atomic_cmpswap dcx[2:3], s[16:19], 18 glc
            s_buffer_atomic_umin bax, s[16:19], 20 glc
            s_buffer_atomic_swap_x2 dcx[2:3], s[16:19], 18 glc
            s_buffer_atomic_cmpswap_x2 dcx[4:7], s[16:19], 18 glc
            s_buffer_atomic_umin_x2 dcx[2:3], s[16:19], 20 glc
)ffDXD",
        { },
        {
            // S_ATOMIC without GLC
            { 0U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 8U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 16U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 24U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 32U, "dcx", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 40U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            // S_ATOMIC with GLC
            { 48U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 56U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 56U, "dcx", 3, 4, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 64U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 72U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 80U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 80U, "dcx", 6, 8, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 88U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            // S_BUFFER_ATOMIC without GLC
            { 96U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 104U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 112U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 120U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 128U, "dcx", 4, 8, 4, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 136U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            // S_BUFFER_ATOMIC with GLC
            { 144U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 152U, "dcx", 2, 3, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 152U, "dcx", 3, 4, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 160U, "bax", 0, 1, 1, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 168U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 176U, "dcx", 4, 6, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 176U, "dcx", 6, 8, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 184U, "dcx", 2, 4, 2, GCNDELOP_SMEMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 8 - S_SENDMSG */
        "s_mov_b32 s1, s2\n"
        "s_sendmsg sendmsg(gs_done, nop)",
        { },
        {
            { 4U, nullptr, 0, 0, 1, GCNDELOP_SENDMSG, ASMDELOP_NONE, 0 }
        }, true, ""
    },
    {   /* 9 - DS encoding */
        R"ffDXD(.arch gcn1.1
            .regvar bax:v, dbx:v:8, dcx:v:8
            ds_read_b32 v1, v2 offset:12
            ds_read_b32 bax, v2 offset:12
            ds_write_b32 v2, v1 offset:12
            ds_write_b32 v2, bax offset:12
            ds_read_b64 v[1:2], v2 offset:112
            ds_read_b64 dbx[5:6], v2 offset:112
            ds_write_b64 v2, v[2:3] offset:112
            ds_write_b64 v2, dbx[6:7] offset:112
            ds_read_b128 v[1:4], v2 offset:38
            ds_read_b128 dbx[3:6], v2 offset:38
            ds_write_b128 v2, v[7:10] offset:38
            ds_write_b128 v2, dbx[2:5] offset:38
            
            ds_sub_u32 bax, dcx[3] offset:4
            ds_sub_rtn_u32 dbx[5], bax, dcx[3] offset:4
            ds_sub_src2_u32 bax offset:4
            ds_add_u64 bax, dcx[5:6] offset:4
            ds_add_rtn_u64 dbx[3:4], bax, dcx[1:2] offset:4
            ds_add_src2_u64 bax offset:4
            
            ds_cmpst_b32 bax, dbx[4], dcx[1]
            ds_cmpst_b64 bax, dbx[4:5], dcx[3:4]
            ds_cmpst_rtn_b32 dcx[5], bax, dbx[4], dcx[1]
            ds_cmpst_rtn_b64 dcx[0:1], bax, dbx[4:5], dcx[3:4]
            
            # GDS
            ds_read_b32 v1, v2 offset:12 gds
            ds_read_b32 bax, v2 offset:12 gds
            ds_write_b32 v2, v1 offset:12 gds
            ds_write_b32 v2, bax offset:12 gds
            ds_read_b64 v[1:2], v2 offset:112 gds
            ds_read_b64 dbx[5:6], v2 offset:112 gds
            ds_write_b64 v2, v[2:3] offset:112 gds
            ds_write_b64 v2, dbx[6:7] offset:112 gds
            ds_read_b128 v[1:4], v2 offset:38 gds
            ds_read_b128 dbx[3:6], v2 offset:38 gds
            ds_write_b128 v2, v[7:10] offset:38 gds
            ds_write_b128 v2, dbx[2:5] offset:38 gds
            
            ds_sub_u32 bax, dcx[3] offset:4 gds
            ds_sub_rtn_u32 dbx[5], bax, dcx[3] offset:4 gds
            ds_sub_src2_u32 bax offset:4 gds
            ds_add_u64 bax, dcx[5:6] offset:4 gds
            ds_add_rtn_u64 dbx[3:4], bax, dcx[1:2] offset:4 gds
            ds_add_src2_u64 bax offset:4 gds
            
            ds_cmpst_b32 bax, dbx[4], dcx[1] gds
            ds_cmpst_b64 bax, dbx[4:5], dcx[3:4] gds
            ds_cmpst_rtn_b32 dcx[5], bax, dbx[4], dcx[1] gds
            ds_cmpst_rtn_b64 dcx[0:1], bax, dbx[4:5], dcx[3:4] gds
            
            # DS_SWIZZLE
            ds_swizzle_b32 v4, v6 offset:0xfdac
)ffDXD",
        { },
        {
            { 0U, nullptr, 256+1, 256+2, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 8U, "bax", 0, 1, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 16U, nullptr, 256+1, 256+2, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 24U, "bax", 0, 1, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            // read/write 64-bit
            { 32U, nullptr, 256+1, 256+3, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 40U, "dbx", 5, 7, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 48U, nullptr, 256+2, 256+4, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 56U, "dbx", 6, 8, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            // read/write 128-bit
            { 64U, nullptr, 256+1, 256+5, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 72U, "dbx", 3, 7, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 80U, nullptr, 256+7, 256+11, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 88U, "dbx", 2, 6, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            /* atomics 32-bit */
            { 96U, "dcx", 3, 4, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 104U, "dbx", 5, 6, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 104U, "dcx", 3, 4, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 112U, nullptr, 0, 0, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, 0 },
            /* atomics 64-bit */
            { 120U, "dcx", 5, 7, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 128U, "dbx", 3, 5, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 128U, "dcx", 1, 3, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 136U, nullptr, 0, 0, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, 0 },
            /* ds_cmpst_* */
            { 144U, "dbx", 4, 5, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 144U, "dcx", 1, 2, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 152U, "dbx", 4, 6, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 152U, "dcx", 3, 5, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 160U, "dcx", 5, 6, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 160U, "dbx", 4, 5, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 160U, "dcx", 1, 2, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 168U, "dcx", 0, 2, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 168U, "dbx", 4, 6, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            { 168U, "dcx", 3, 5, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_READ },
            /* GDS -- */
            { 176U, nullptr, 256+1, 256+2, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 184U, "bax", 0, 1, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 192U, nullptr, 256+1, 256+2, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 200U, "bax", 0, 1, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            // read/write 64-bit
            { 208U, nullptr, 256+1, 256+3, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 216U, "dbx", 5, 7, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 224U, nullptr, 256+2, 256+4, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 232U, "dbx", 6, 8, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            // read/write 128-bit
            { 240U, nullptr, 256+1, 256+5, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 248U, "dbx", 3, 7, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 256U, nullptr, 256+7, 256+11, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 264U, "dbx", 2, 6, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            /* atomics 32-bit */
            { 272U, "dcx", 3, 4, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 280U, "dbx", 5, 6, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 280U, "dcx", 3, 4, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 288U, nullptr, 0, 0, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT, 0 },
            /* atomics 64-bit */
            { 296U, "dcx", 5, 7, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 304U, "dbx", 3, 5, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 304U, "dcx", 1, 3, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 312U, nullptr, 0, 0, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT, 0 },
            /* ds_cmpst_* */
            { 320U, "dbx", 4, 5, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 320U, "dcx", 1, 2, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 328U, "dbx", 4, 6, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 328U, "dcx", 3, 5, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 336U, "dcx", 5, 6, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 336U, "dbx", 4, 5, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 336U, "dcx", 1, 2, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 344U, "dcx", 0, 2, 1, GCNDELOP_GDSOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 344U, "dbx", 4, 6, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            { 344U, "dcx", 3, 5, 1, GCNDELOP_GDSOP, GCNDELOP_EXPORT,
                ASMRVU_READ, ASMRVU_READ },
            /* DS_SWIZZLE */
            { 352U, nullptr, 256+4, 256+5, 1, GCNDELOP_LDSOP, ASMDELOP_NONE, ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 10 - MTBUF encoding */
        R"ffDXD(
            .regvar bax:v, dbx:v:8, dcx:v:8, sr:s:8
            tbuffer_load_format_x dbx[4], bax, sr[0:3], 0 offen format:[32,uint]
            tbuffer_load_format_xy dbx[2:3], bax, sr[0:3], 0 offen format:[32_32,uint]
            tbuffer_load_format_xyz dcx[5:7], bax, sr[0:3], 0 \
                            offen format:[32_32_32,uint]
            tbuffer_load_format_xyzw dcx[2:5], bax, sr[0:3], 0 \
                            offen format:[32_32_32_32,uint]
            
            tbuffer_store_format_x dbx[7], bax, sr[0:3], 0 offen format:[32,uint]
            tbuffer_store_format_xy dbx[2:3], bax, sr[0:3], 0 offen format:[32_32,uint]
            tbuffer_store_format_xyz dbx[5:7], bax, sr[0:3], 0 \
                            offen format:[32_32_32,uint]
            tbuffer_store_format_xyzw dbx[2:5], bax, sr[0:3], 0 \
                            offen format:[32_32_32_32,uint]
            # GLC
            tbuffer_load_format_x dbx[4], bax, sr[0:3], 0 offen format:[32,uint] glc
            tbuffer_store_format_x dbx[7], bax, sr[0:3], 0 offen format:[32,uint] glc
            # TFE
            tbuffer_load_format_x dbx[4:5], bax, sr[0:3], 0 offen format:[32,uint] tfe
            tbuffer_load_format_xy dbx[2:4], bax, sr[0:3], 0 offen format:[32,uint] tfe
            tbuffer_store_format_xy dbx[2:4], bax, sr[0:3], 0 offen format:[32,uint] tfe
)ffDXD",
        { },
        {
            // tbuffer_load_format_*
            { 0U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 8U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 16U, "dcx", 5, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 24U, "dcx", 2, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            // tbuffer_store_format_*
            { 32U, "dbx", 7, 8, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 40U, "dbx", 2, 4, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 48U, "dbx", 5, 8, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 56U, "dbx", 2, 6, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            /* glc */
            { 64U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 72U, "dbx", 7, 8, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            /* tfe */
            { 80U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 80U, "dbx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 88U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 88U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 96U, "dbx", 2, 4, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 96U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 11 - MTBUF encoding (GCN1.1-1.4) */
        R"ffDXD(.arch gcn1.1
            .regvar bax:v, dbx:v:8, dcx:v:8, sr:s:8
            tbuffer_load_format_x dbx[4], bax, sr[0:3], 0 offen format:[32,uint]
            
            tbuffer_store_format_x dbx[7], bax, sr[0:3], 0 offen format:[32,uint]
            tbuffer_store_format_xy dbx[2:3], bax, sr[0:3], 0 offen format:[32_32,uint]
            tbuffer_store_format_xyz dbx[5:7], bax, sr[0:3], 0 \
                            offen format:[32_32_32,uint]
            tbuffer_store_format_xyzw dbx[2:5], bax, sr[0:3], 0 \
                            offen format:[32_32_32_32,uint]
            # GLC
            tbuffer_store_format_x dbx[7], bax, sr[0:3], 0 offen format:[32,uint] glc
            # TFE
            tbuffer_store_format_xy dbx[2:4], bax, sr[0:3], 0 offen format:[32,uint] tfe
)ffDXD",
        { },
        {
            // tbuffer_load_format_*
            { 0U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            // tbuffer_store_format_*
            { 8U, "dbx", 7, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 16U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 24U, "dbx", 5, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 32U, "dbx", 2, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* glc */
            { 40U, "dbx", 7, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* tfe */
            { 48U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 48U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 12 - MUBUF encoding */
        R"ffDXD(
            .regvar bax:v, dbx:v:8, dcx:v:8, sr:s:8
            buffer_load_format_x dbx[4], bax, sr[0:3], 0 offen
            buffer_load_format_xy dbx[2:3], bax, sr[0:3], 0 offen
            buffer_load_format_xyz dcx[5:7], bax, sr[0:3], 0 offen
            buffer_load_format_xyzw dcx[2:5], bax, sr[0:3], 0 offen
            
            buffer_store_format_x dbx[7], bax, sr[0:3], 0 offen
            buffer_store_format_xy dbx[2:3], bax, sr[0:3], 0 offen
            buffer_store_format_xyz dbx[5:7], bax, sr[0:3], 0 offen
            buffer_store_format_xyzw dbx[2:5], bax, sr[0:3], 0 offen
            # GLC
            buffer_load_format_x dbx[4], bax, sr[0:3], 0 offen glc
            buffer_store_format_x dbx[7], bax, sr[0:3], 0 offen glc
            # TFE
            buffer_load_format_x dbx[4:5], bax, sr[0:3], 0 offen tfe
            buffer_load_format_xy dbx[2:4], bax, sr[0:3], 0 offen tfe
            buffer_store_format_xy dbx[2:4], bax, sr[0:3], 0 offen tfe
            # DWORD
            buffer_load_dword dcx[4], bax, sr[0:3], 0 offen
            buffer_load_dwordx2 dcx[2:3], bax, sr[0:3], 0 offen
            # LDS
            buffer_load_format_x dbx[4], bax, sr[0:3], 0 offen lds
            buffer_store_format_x dbx[4], bax, sr[0:3], 0 offen lds
            # ATOMIC
            buffer_atomic_add dbx[6], bax, sr[0:3], 0 offen
            buffer_atomic_add dbx[6], bax, sr[0:3], 0 offen glc
            buffer_atomic_cmpswap dbx[0:1], bax, sr[0:3], 0 offen
            buffer_atomic_cmpswap dbx[0:1], bax, sr[0:3], 0 offen glc
            # ATOMIC_X2
            buffer_atomic_add_x2 dbx[4:5], bax, sr[0:3], 0 offen
            buffer_atomic_add_x2 dbx[4:5], bax, sr[0:3], 0 offen glc
            buffer_atomic_cmpswap_x2 dbx[1:4], bax, sr[0:3], 0 offen
            buffer_atomic_cmpswap_x2 dbx[1:4], bax, sr[0:3], 0 offen glc
            # ATOMIC_X2 TFE
            buffer_atomic_add_x2 dbx[4:6], bax, sr[0:3], 0 offen tfe
            buffer_atomic_add_x2 dbx[4:6], bax, sr[0:3], 0 offen glc tfe
            buffer_atomic_cmpswap_x2 dbx[1:5], bax, sr[0:3], 0 offen tfe
            buffer_atomic_cmpswap_x2 dbx[1:5], bax, sr[0:3], 0 offen glc tfe
)ffDXD",
        { },
        {
            // tbuffer_load_format_*
            { 0U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 8U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 16U, "dcx", 5, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 24U, "dcx", 2, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            // tbuffer_store_format_*
            { 32U, "dbx", 7, 8, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 40U, "dbx", 2, 4, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 48U, "dbx", 5, 8, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 56U, "dbx", 2, 6, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            /* glc */
            { 64U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 72U, "dbx", 7, 8, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            /* tfe */
            { 80U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 80U, "dbx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 88U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 88U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 96U, "dbx", 2, 4, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 96U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            /* DWORD */
            { 104U, "dcx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 112U, "dcx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            /* LDS */
            { 120U, nullptr, 0, 0, 1, GCNDELOP_VMOP, ASMDELOP_NONE, 0 },
            { 128U, nullptr, 0, 0, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE, 0, 0 },
            /* ATOMIC */
            { 136U, "dbx", 6, 7, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 144U, "dbx", 6, 7, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            /* CMPSWAP */
            { 152U, "dbx", 0, 2, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 160U, "dbx", 0, 1, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            { 160U, "dbx", 1, 2, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* ATOMIC_X2 */
            { 168U, "dbx", 4, 6, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 176U, "dbx", 4, 6, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            /* CMPSWAP_X2 */
            { 184U, "dbx", 1, 5, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 192U, "dbx", 1, 3, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            { 192U, "dbx", 3, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* ATOMIC_X2 TFE */
            { 200U, "dbx", 4, 6, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 200U, "dbx", 6, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            /* TODO: why dbx[4-6] ???: check */
            { 208U, "dbx", 4, 7, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            // ????
            { 208U, "dbx", 6, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            /* CMPSWAP_X2 TFE */
            { 216U, "dbx", 1, 5, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 216U, "dbx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 224U, "dbx", 1, 3, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            { 224U, "dbx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 224U, "dbx", 3, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ }
        }, true, ""
    },
    {   /* 13 - MUBUF encoding (GCN1.1) */
        R"ffDXD(.arch gcn1.1
            .regvar bax:v, dbx:v:8, dcx:v:8, sr:s:8
            buffer_load_format_x dbx[4], bax, sr[0:3], 0 offen
            buffer_load_format_xy dbx[2:3], bax, sr[0:3], 0 offen
            buffer_load_format_xyz dcx[5:7], bax, sr[0:3], 0 offen
            buffer_load_format_xyzw dcx[2:5], bax, sr[0:3], 0 offen
            
            buffer_store_format_x dbx[7], bax, sr[0:3], 0 offen
            buffer_store_format_xy dbx[2:3], bax, sr[0:3], 0 offen
            buffer_store_format_xyz dbx[5:7], bax, sr[0:3], 0 offen
            buffer_store_format_xyzw dbx[2:5], bax, sr[0:3], 0 offen
            # GLC
            buffer_load_format_x dbx[4], bax, sr[0:3], 0 offen glc
            buffer_store_format_x dbx[7], bax, sr[0:3], 0 offen glc
            # TFE
            buffer_load_format_x dbx[4:5], bax, sr[0:3], 0 offen tfe
            buffer_load_format_xy dbx[2:4], bax, sr[0:3], 0 offen tfe
            buffer_store_format_xy dbx[2:4], bax, sr[0:3], 0 offen tfe
            # DWORD
            buffer_load_dword dcx[4], bax, sr[0:3], 0 offen
            buffer_load_dwordx2 dcx[2:3], bax, sr[0:3], 0 offen
            # LDS
            buffer_load_format_x dbx[4], bax, sr[0:3], 0 offen lds
            buffer_store_format_x dbx[4], bax, sr[0:3], 0 offen lds
            # ATOMIC
            buffer_atomic_add dbx[6], bax, sr[0:3], 0 offen
            buffer_atomic_add dbx[6], bax, sr[0:3], 0 offen glc
            buffer_atomic_cmpswap dbx[0:1], bax, sr[0:3], 0 offen
            buffer_atomic_cmpswap dbx[0:1], bax, sr[0:3], 0 offen glc
            # ATOMIC_X2
            buffer_atomic_add_x2 dbx[4:5], bax, sr[0:3], 0 offen
            buffer_atomic_add_x2 dbx[4:5], bax, sr[0:3], 0 offen glc
            buffer_atomic_cmpswap_x2 dbx[1:4], bax, sr[0:3], 0 offen
            buffer_atomic_cmpswap_x2 dbx[1:4], bax, sr[0:3], 0 offen glc
            # ATOMIC_X2 TFE
            buffer_atomic_add_x2 dbx[4:6], bax, sr[0:3], 0 offen tfe
            buffer_atomic_add_x2 dbx[4:6], bax, sr[0:3], 0 offen glc tfe
            buffer_atomic_cmpswap_x2 dbx[1:5], bax, sr[0:3], 0 offen tfe
            buffer_atomic_cmpswap_x2 dbx[1:5], bax, sr[0:3], 0 offen glc tfe
)ffDXD",
        { },
        {
            // buffer_load_format_*
            { 0U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 8U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 16U, "dcx", 5, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 24U, "dcx", 2, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            // buffer_store_format_*
            { 32U, "dbx", 7, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 40U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 48U, "dbx", 5, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 56U, "dbx", 2, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* glc */
            { 64U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 72U, "dbx", 7, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* tfe */
            { 80U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 80U, "dbx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 88U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 88U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 96U, "dbx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 96U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            /* DWORD */
            { 104U, "dcx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 112U, "dcx", 2, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            /* LDS */
            { 120U, nullptr, 0, 0, 1, GCNDELOP_VMOP, ASMDELOP_NONE, 0 },
            { 128U, nullptr, 0, 0, 1, GCNDELOP_VMOP, ASMDELOP_NONE, 0 },
            /* ATOMIC */
            { 136U, "dbx", 6, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 144U, "dbx", 6, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            /* CMPSWAP */
            { 152U, "dbx", 0, 2, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 160U, "dbx", 0, 1, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 160U, "dbx", 1, 2, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* ATOMIC_X2 */
            { 168U, "dbx", 4, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 176U, "dbx", 4, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            /* CMPSWAP_X2 */
            { 184U, "dbx", 1, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 192U, "dbx", 1, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 192U, "dbx", 3, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            /* ATOMIC_X2 TFE */
            { 200U, "dbx", 4, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 200U, "dbx", 6, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 208U, "dbx", 4, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            // ????
            { 208U, "dbx", 6, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            /* CMPSWAP_X2 TFE */
            { 216U, "dbx", 1, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 216U, "dbx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 224U, "dbx", 1, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 224U, "dbx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 224U, "dbx", 3, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ }
        }, true, ""
    },
    {   /* 14 - MIMG encoding */
        R"ffDXD(
            .regvar bax:v:8, dbx:v:8, dcx:v:8, sr:s:8
            # load
            image_load dcx[5], bax[1:4], sr[0:3] dmask:1 unorm r128
            image_load dcx[5:7], bax[1:4], sr[0:3] dmask:13 unorm r128
            # store
            image_store dbx[4], bax[1:4], sr[0:3] dmask:1 unorm r128
            image_store dbx[3:5], bax[1:4], sr[0:3] dmask:13 unorm r128
            # load tfe
            image_load dcx[4:5], bax[1:4], sr[0:3] dmask:1 unorm r128 tfe
            image_load dcx[4:7], bax[1:4], sr[0:3] dmask:13 unorm r128 tfe
            # ATOMIC
            image_atomic_inc dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128
            image_atomic_inc dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128 glc
            # CMPSWAP
            image_atomic_cmpswap dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128
            image_atomic_cmpswap dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128 glc
            # image_lod
            image_get_lod dbx[3], bax[1:4], sr[0:3], sr[4:7] dmask:1 unorm r128
            # ATOMIC TFE
            image_atomic_inc dbx[1:3], bax[1:4], sr[0:3] dmask:5 unorm r128 tfe
            image_atomic_inc dbx[1:3], bax[1:4], sr[0:3] dmask:5 unorm r128 glc tfe
)ffDXD",
        { },
        {
            // LOAD
            { 0U, "dcx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 8U, "dcx", 5, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            // STORE
            { 16U, "dbx", 4, 5, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 24U, "dbx", 3, 6, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            // LOAD TFE
            { 32U, "dcx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 32U, "dcx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 40U, "dcx", 4, 7, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            { 40U, "dcx", 7, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            // ATOMIC
            { 48U, "dbx", 1, 3, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 56U, "dbx", 1, 3, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            // ATOMIC CMPSWAP
            { 64U, "dbx", 1, 3, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 72U, "dbx", 1, 2, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            { 72U, "dbx", 2, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            // IMAGE_GET_LOD
            { 80U, "dbx", 3, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            // ATOMIC TFE
            { 88U, "dbx", 1, 3, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ, ASMRVU_READ },
            { 88U, "dbx", 3, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 96U, "dbx", 1, 4, 1, GCNDELOP_VMOP, GCNDELOP_EXPVMWRITE,
                ASMRVU_READ|ASMRVU_WRITE, ASMRVU_READ },
            { 96U, "dbx", 3, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 15 - MIMG encoding (GCN 1.1) */
        R"ffDXD(.arch gcn1.1
            .regvar bax:v:8, dbx:v:8, dcx:v:8, sr:s:8
            # load
            image_load dcx[5], bax[1:4], sr[0:3] dmask:1 unorm r128
            # store
            image_store dbx[4], bax[1:4], sr[0:3] dmask:1 unorm r128
            image_store dbx[3:5], bax[1:4], sr[0:3] dmask:13 unorm r128
            # ATOMIC
            image_atomic_inc dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128
            image_atomic_inc dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128 glc
            # CMPSWAP
            image_atomic_cmpswap dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128
            image_atomic_cmpswap dbx[1:2], bax[1:4], sr[0:3] dmask:5 unorm r128 glc
            # ATOMIC TFE
            image_atomic_inc dbx[1:3], bax[1:4], sr[0:3] dmask:5 unorm r128 tfe
            image_atomic_inc dbx[1:3], bax[1:4], sr[0:3] dmask:5 unorm r128 glc tfe
)ffDXD",
        { },
        {
            // LOAD
            { 0U, "dcx", 5, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_WRITE },
            // STORE
            { 8U, "dbx", 4, 5, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 16U, "dbx", 3, 6, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            // ATOMIC
            { 24U, "dbx", 1, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 32U, "dbx", 1, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            // ATOMIC CMPSWAP
            { 40U, "dbx", 1, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 48U, "dbx", 1, 2, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 48U, "dbx", 2, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            // ATOMIC TFE
            { 56U, "dbx", 1, 3, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ },
            { 56U, "dbx", 3, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 64U, "dbx", 1, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE },
            { 64U, "dbx", 3, 4, 1, GCNDELOP_VMOP, ASMDELOP_NONE, ASMRVU_READ|ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 16 - EXP encoding */
        R"ffDXD(
            .regvar fax:v, fbx:v, fcx:v, fex:v, fbx4:v:8, fcx4:v:12
            exp  param5, fax, fbx, fcx, fbx4[5] done vm
            exp  param5, off, fcx4[2], off, fbx4[6] done vm
            exp  param5, v54, v28, v83, v161 done vm
            exp  param5, off, v42, off, v97 done vm
)ffDXD",
        { },
        {
            { 0U, "fax", 0, 1, 1, GCNDELOP_EXPORT, ASMDELOP_NONE, ASMRVU_READ },
            { 0U, "fbx", 0, 1, 1, GCNDELOP_EXPORT, ASMDELOP_NONE, ASMRVU_READ },
            { 0U, "fcx", 0, 1, 1, GCNDELOP_EXPORT, ASMDELOP_NONE, ASMRVU_READ },
            { 0U, "fbx4", 5, 6, 1, GCNDELOP_EXPORT, ASMDELOP_NONE, ASMRVU_READ },
            { 8U, "fcx4", 2, 3, 1, GCNDELOP_EXPORT, ASMDELOP_NONE, ASMRVU_READ },
            { 8U, "fbx4", 6, 7, 1, GCNDELOP_EXPORT, ASMDELOP_NONE, ASMRVU_READ },
            { 16U, nullptr, 256+54, 256+55, 1, GCNDELOP_EXPORT, ASMDELOP_NONE,
                ASMRVU_READ },
            { 16U, nullptr, 256+28, 256+29, 1, GCNDELOP_EXPORT, ASMDELOP_NONE,
                ASMRVU_READ },
            { 16U, nullptr, 256+83, 256+84, 1, GCNDELOP_EXPORT, ASMDELOP_NONE,
                ASMRVU_READ },
            { 16U, nullptr, 256+161, 256+162, 1, GCNDELOP_EXPORT, ASMDELOP_NONE,
                ASMRVU_READ },
            { 24U, nullptr, 256+42, 256+43, 1, GCNDELOP_EXPORT, ASMDELOP_NONE,
                ASMRVU_READ },
            { 24U, nullptr, 256+97, 256+98, 1, GCNDELOP_EXPORT, ASMDELOP_NONE,
                ASMRVU_READ }
        }, true, ""
    },
    {   /* 17 - FLAT encoding */
        R"ffDXD(.arch gcn1.1
            .regvar bax:v, dbx:v:8, dcx:v:8, vr:v:8
            flat_load_dword dbx[6], vr[1:2]
            flat_load_dwordx2 dbx[6:7], vr[1:2]
            flat_load_dwordx3 dbx[5:7], vr[1:2]
            flat_load_dwordx4 dbx[3:6], vr[1:2]
            
            flat_store_dword vr[1:2], dcx[6]
            flat_store_dwordx2 vr[1:2], dcx[2:3]
            flat_store_dwordx3 vr[1:2], dcx[2:4]
            flat_store_dwordx4 vr[1:2], dcx[4:7]
            # GLC
            flat_load_dwordx4 dbx[3:6], vr[1:2] glc
            flat_store_dwordx3 vr[1:2], dcx[2:4] glc
            # TFE
            flat_load_dwordx4 dbx[3:7], vr[1:2] tfe
            flat_store_dwordx3 vr[1:2], dbx[0:2] tfe
            # ATOMIC
            flat_atomic_smax dbx[6], vr[1:2], dcx[1]
            flat_atomic_smax_x2 dbx[6:7], vr[1:2], dcx[1:2]
            flat_atomic_cmpswap_x2 dbx[6:7], vr[1:2], dcx[1:4]
            # ATOMIC GLC
            flat_atomic_smax dbx[6], vr[1:2], dcx[1] glc
            flat_atomic_cmpswap_x2 dbx[6:7], vr[1:2], dcx[1:4] glc
            # ATOMIC TFE
            flat_atomic_smax dbx[6:7], vr[1:2], dcx[1] tfe
            flat_atomic_smax dbx[6:7], vr[1:2], dcx[1] tfe glc
)ffDXD",
        { },
        {
            // FLAT_LOAD
            { 0U, "dbx", 6, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 8U, "dbx", 6, 8, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 16U, "dbx", 5, 8, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 24U, "dbx", 3, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            // FLAT_STORE
            { 32U, "dcx", 6, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            { 40U, "dcx", 2, 4, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            { 48U, "dcx", 2, 5, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            { 56U, "dcx", 4, 8, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            // GLC
            { 64U, "dbx", 3, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 72U, "dcx", 2, 5, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            // TFE
            { 80U, "dbx", 3, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 80U, "dbx", 7, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 88U, "dbx", 0, 3, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            // ATOMIC
            { 96U, "dcx", 1, 2, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            // ATOMIC_X2
            { 104U, "dcx", 1, 3, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            { 112U, "dcx", 1, 5, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            // ATOMIC GLC
            { 120U, "dbx", 6, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 120U, "dcx", 1, 2, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            { 128U, "dbx", 6, 8, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 128U, "dcx", 1, 5, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            // ATOMIC TFE
            { 136U, "dcx", 1, 2, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ },
            { 144U, "dbx", 6, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
            { 144U, "dbx", 7, 8, 1, GCNDELOP_VMOP, ASMDELOP_NONE,
                ASMRVU_READ|ASMRVU_WRITE },
            { 144U, "dcx", 1, 2, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_READ, ASMRVU_READ }
        }, true, ""
    },
    {   /* 18 - FLAT encoding (GCN 1.4, GFX9) */
        R"ffDXD(.arch gcn1.4
            .regvar bax:v, dbx:v:8, dcx:v:8, vr:v:8
            global_load_dword dbx[6], vr[1:2], off lds
            global_load_dwordx2 dbx[6:7], vr[1:2], off lds
            global_load_dwordx3 dbx[5:7], vr[1:2], off lds
            global_load_dwordx4 dbx[3:6], vr[1:2], off lds
            flat_load_dwordx4 dbx[3:6], vr[1:2]
)ffDXD",
        { },
        {
            { 0U, nullptr, 0, 0, 1, GCNDELOP_VMOP, ASMDELOP_NONE, 0 },
            { 8U, nullptr, 0, 0, 1, GCNDELOP_VMOP, ASMDELOP_NONE, 0 },
            { 16U, nullptr, 0, 0, 1, GCNDELOP_VMOP, ASMDELOP_NONE, 0 },
            { 24U, nullptr, 0, 0, 1, GCNDELOP_VMOP, ASMDELOP_NONE, 0 },
            { 32U, "dbx", 3, 7, 1, GCNDELOP_VMOP, GCNDELOP_LDSOP,
                ASMRVU_WRITE, ASMRVU_WRITE },
        }, true, ""
    }
};

static void pushRegVarsFromScopes(const AsmScope& scope,
            std::unordered_map<const AsmRegVar*, CString>& rvMap,
            const std::string& prefix)
{
    for (const auto& rvEntry: scope.regVarMap)
    {
        std::string rvName = prefix+rvEntry.first.c_str();
        rvMap.insert(std::make_pair(&rvEntry.second, rvName));
    }
    
    for (const auto& scopeEntry: scope.scopeMap)
    {
        std::string newPrefix = prefix+scopeEntry.first.c_str()+"::";
        pushRegVarsFromScopes(*scopeEntry.second, rvMap, newPrefix);
    }
}

static void testWaitHandlerCase(cxuint i, const AsmWaitHandlerCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input,
                    (ASM_ALL&~ASM_ALTMACRO) | ASM_TESTRUN | ASM_TESTRESOLVE,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << " testWaitHandlerCase#" << i;
    oss.flush();
    const std::string testCaseName = oss.str();
    assertValue<bool>("testWaitHandle", testCaseName+".good",
                      testCase.good, good);
    assertString("testWaitHandle", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
    
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testWaitHandlerCase#" << i;
        throw Exception(oss.str());
    }
    
    std::unordered_map<const AsmRegVar*, CString> regVarNamesMap;
    pushRegVarsFromScopes(assembler.getGlobalScope(), regVarNamesMap, "");
    ISAWaitHandler* waitHandler = assembler.getSections()[0].waitHandler.get();
    ISAWaitHandler::ReadPos waitPos{ 0, 0 };
    size_t j, k;
    for (j = k = 0; waitHandler->hasNext(waitPos);)
    {
        AsmWaitInstr waitInstr{};
        AsmDelayedOp delayedOp{};
        std::ostringstream koss;
        if (waitHandler->nextInstr(waitPos, delayedOp, waitInstr))
        {
            /// check Asm wait instruction
            assertTrue("testWaitHandle", testCaseName+".wlength",
                    j < testCase.waitInstrs.size());
            koss << testCaseName << ".waitInstr#" << j;
            std::string wiStr = koss.str();
            
            const AsmWaitInstr& expWaitInstr = testCase.waitInstrs[j++];
            assertValue("testWaitHandle", wiStr+".offset", expWaitInstr.offset,
                        waitInstr.offset);
            assertArray("testWaitHandle", wiStr+".waits",
                        Array<uint16_t>(expWaitInstr.waits, expWaitInstr.waits+4),
                        4, waitInstr.waits);
        }
        else
        {
            // test asm delayed op
            assertTrue("testWaitHandle", testCaseName+".dolength",
                    k < testCase.delayedOps.size());
            koss << testCaseName << ".delayedOp#" << k;
            std::string doStr = koss.str();
            
            const AsmDelayedOpData& expDelayedOp = testCase.delayedOps[k++];
            assertValue("testWaitHandle", doStr+".offset", expDelayedOp.offset,
                        delayedOp.offset);
            if (expDelayedOp.regVarName==nullptr)
                assertTrue("testWaitHandle", doStr+".regVar", delayedOp.regVar==nullptr);
            else // otherwise
            {
                assertTrue("testWaitHandle", doStr+".regVar", delayedOp.regVar!=nullptr);
                assertString("testWaitHandle", doStr+".regVarName",
                        expDelayedOp.regVarName,
                        regVarNamesMap.find(delayedOp.regVar)->second);
            }
            assertValue("testWaitHandle", doStr+".rstart", expDelayedOp.rstart,
                        delayedOp.rstart);
            assertValue("testWaitHandle", doStr+".rend", expDelayedOp.rend,
                        delayedOp.rend);
            assertValue("testWaitHandle", doStr+".count",
                        cxuint(expDelayedOp.count), cxuint(delayedOp.count));
            assertValue("testWaitHandle", doStr+".delayedOpType",
                        cxuint(expDelayedOp.delayedOpType),
                        cxuint(delayedOp.delayedOpType));
            assertValue("testWaitHandle", doStr+".delayedOpType2",
                        cxuint(expDelayedOp.delayedOpType2),
                        cxuint(delayedOp.delayedOpType2));
            assertValue("testWaitHandle", doStr+".rwFlags",
                        cxuint(expDelayedOp.rwFlags), cxuint(delayedOp.rwFlags));
            assertValue("testWaitHandle", doStr+".rwFlags2",
                        cxuint(expDelayedOp.rwFlags2), cxuint(delayedOp.rwFlags2));
        }
    }
    assertTrue("testWaitHandle", testCaseName+".wlength",
                   j == testCase.waitInstrs.size());
    assertTrue("testWaitHandle", testCaseName+".dolength",
                   k == testCase.delayedOps.size());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(waitHandlerTestCases) / sizeof(AsmWaitHandlerCase); i++)
        try
        { testWaitHandlerCase(i, waitHandlerTestCases[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
