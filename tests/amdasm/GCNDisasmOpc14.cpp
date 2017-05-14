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

#include <CLRX/Config.h>
#include "GCNDisasmOpc.h"

/* for Radeon RX VEGA series with GCN1.4 */
const GCNDisasmOpcodeCase decGCNOpcodeGCN14Cases[] =
{   /* extra scalar registers */
    { 0x80153debU, 0, false, "        s_add_u32       s21, shared_base, s61\n" },
    { 0x8015eb04U, 0, false, "        s_add_u32       s21, s4, shared_base\n" },
    { 0x80153decU, 0, false, "        s_add_u32       s21, shared_limit, s61\n" },
    { 0x8015ec04U, 0, false, "        s_add_u32       s21, s4, shared_limit\n" },
    { 0x80153dedU, 0, false, "        s_add_u32       s21, private_base, s61\n" },
    { 0x8015ed04U, 0, false, "        s_add_u32       s21, s4, private_base\n" },
    { 0x80153deeU, 0, false, "        s_add_u32       s21, private_limit, s61\n" },
    { 0x80153defU, 0, false, "        s_add_u32       s21, pops_exiting_wave_id, s61\n" },
    { 0x80153d6cU, 0, false, "        s_add_u32       s21, ttmp0, s61\n" },
    { 0x80156c04U, 0, false, "        s_add_u32       s21, s4, ttmp0\n" },
    { 0x80153d6dU, 0, false, "        s_add_u32       s21, ttmp1, s61\n" },
    { 0x80156d04U, 0, false, "        s_add_u32       s21, s4, ttmp1\n" },
    { 0x80153d6eU, 0, false, "        s_add_u32       s21, ttmp2, s61\n" },
    { 0x80153d6fU, 0, false, "        s_add_u32       s21, ttmp3, s61\n" },
    { 0x80153d70U, 0, false, "        s_add_u32       s21, ttmp4, s61\n" },
    { 0x80153d71U, 0, false, "        s_add_u32       s21, ttmp5, s61\n" },
    { 0x80153d72U, 0, false, "        s_add_u32       s21, ttmp6, s61\n" },
    { 0x80153d73U, 0, false, "        s_add_u32       s21, ttmp7, s61\n" },
    { 0x80153d74U, 0, false, "        s_add_u32       s21, ttmp8, s61\n" },
    { 0x80153d75U, 0, false, "        s_add_u32       s21, ttmp9, s61\n" },
    { 0x80153d76U, 0, false, "        s_add_u32       s21, ttmp10, s61\n" },
    { 0x80153d77U, 0, false, "        s_add_u32       s21, ttmp11, s61\n" },
    { 0x80153d78U, 0, false, "        s_add_u32       s21, ttmp12, s61\n" },
    { 0x80153d79U, 0, false, "        s_add_u32       s21, ttmp13, s61\n" },
    { 0x80153d7aU, 0, false, "        s_add_u32       s21, ttmp14, s61\n" },
    { 0x80153d7bU, 0, false, "        s_add_u32       s21, ttmp15, s61\n" },
    { 0x80157b04U, 0, false, "        s_add_u32       s21, s4, ttmp15\n" },
    /* SOP2 instructions */
    { 0x96153d04U, 0, false, "        s_mul_hi_u32    s21, s4, s61\n" },
    { 0x96953d04U, 0, false, "        s_mul_hi_i32    s21, s4, s61\n" },
    { 0x97153d04U, 0, false, "        s_lshl1_add_u32 s21, s4, s61\n" },
    { 0x97953d04U, 0, false, "        s_lshl2_add_u32 s21, s4, s61\n" },
    { 0x98153d04U, 0, false, "        s_lshl3_add_u32 s21, s4, s61\n" },
    { 0x98953d04U, 0, false, "        s_lshl4_add_u32 s21, s4, s61\n" },
    { 0x99153d04U, 0, false, "        s_pack_ll_b32_b16 s21, s4, s61\n" },
    { 0x99953d04U, 0, false, "        s_pack_lh_b32_b16 s21, s4, s61\n" },
    { 0x9a153d04U, 0, false, "        s_pack_hh_b32_b16 s21, s4, s61\n" },
    /* SOP1 instructions */
    { 0xbed63314U, 0, false, "        s_andn1_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed63414U, 0, false, "        s_orn1_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed63514U, 0, false, "        s_andn1_wrexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed63614U, 0, false, "        s_andn2_wrexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed63714U, 0, false, "        s_bitreplicate_b64_b32 s[86:87], s20\n" },
    /* SOPK instructions */
    { 0xbaab0000U, 0, false, "        s_call_b64      s[43:44], .L4_0\n" },
    { 0xbaab000aU, 0, false, "        s_call_b64      s[43:44], .L44_0\n" },
    /* SOPP instructions */
    { 0xbf9e0000U, 0, false, "        s_endpgm_ordered_ps_done\n" },
    /* hwregisters */
    { 0xb8ab000eu, 0, false, "        s_getreg_b32    s43, hwreg(flush_ib, 0, 1)\n" },
    { 0xb8ab000fu, 0, false, "        s_getreg_b32    s43, hwreg(sh_mem_bases, 0, 1)\n" },
    { 0xb8ab0010u, 0, false, "        s_getreg_b32    "
                "s43, hwreg(sq_shader_tba_lo, 0, 1)\n" },
    { 0xb8ab0011u, 0, false, "        s_getreg_b32    "
                "s43, hwreg(sq_shader_tba_hi, 0, 1)\n" },
    { 0xb8ab0012u, 0, false, "        s_getreg_b32    "
                "s43, hwreg(sq_shader_tma_lo, 0, 1)\n" },
    { 0xb8ab0013u, 0, false, "        s_getreg_b32    "
                "s43, hwreg(sq_shader_tma_hi, 0, 1)\n" },
    /* new msg types */
    { 0xbf900015U, 0, false, "        s_sendmsg       sendmsg(stall_wave_gen, cut, 0)\n" },
    { 0xbf900016U, 0, false, "        s_sendmsg       sendmsg(halt_waves, cut, 0)\n" },
    { 0xbf900017U, 0, false, "        s_sendmsg       "
                "sendmsg(ordered_ps_done, cut, 0)\n" },
    { 0xbf900018U, 0, false, "        s_sendmsg       "
                "sendmsg(early_prim_dealloc, cut, 0)\n" },
    { 0xbf900019U, 0, false, "        s_sendmsg       sendmsg(gs_alloc_req, cut, 0)\n" },
    { 0xbf90001aU, 0, false, "        s_sendmsg       sendmsg(get_doorbell, cut, 0)\n" },
    /* waitcnts */
    { 0xbf8c0d36U, 0, false, "        s_waitcnt       "
        "vmcnt(6) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c4d36U, 0, false, "        s_waitcnt       "
        "vmcnt(22) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c8d36U, 0, false, "        s_waitcnt       "
        "vmcnt(38) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8ccd36U, 0, false, "        s_waitcnt       "
        "vmcnt(54) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0d3fU, 0, false, "        s_waitcnt       "
        "vmcnt(15) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c4d3fU, 0, false, "        s_waitcnt       "
        "vmcnt(31) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c8d3fU, 0, false, "        s_waitcnt       "
        "vmcnt(47) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8ccd3fU, 0, false, "        s_waitcnt       "
        "expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0000U, 0, false, "        s_waitcnt       "
        "vmcnt(0) & expcnt(0) & lgkmcnt(0)\n" },
    { 0xbf8ccf7fU, 0, false, "        s_waitcnt       " /* good ??? */
        "vmcnt(63) & expcnt(7) & lgkmcnt(15)\n" },
    { 0xbf8cfd36U, 0, false, "        s_waitcnt       "
        "vmcnt(54) & expcnt(3) & lgkmcnt(13) :0xfd36\n" },
    /* SMEM encoding */
    { 0xc0020c9dU, 0x5b, true, "        s_load_dword    s50, s[58:59], 0x5b\n" },
    { 0xc0020c9dU, 0x13da5b, true, "        s_load_dword    s50, s[58:59], 0x13da5b\n" },
    { 0xc0028c9dU, 0x5b, true, "        s_load_dword    s50, s[58:59], 0x5b nv\n" },
    { 0xc0024c9dU, 0x3200005b, true, "        s_load_dword    "
        "s50, s[58:59], 0x32 offset:0x5b\n" },
    { 0xc002cc9dU, 0x3200005b, true, "        s_load_dword    "
        "s50, s[58:59], 0x32 nv offset:0x5b\n" },
    { 0xc0004c9dU, 0xb6000000U, true, "        s_load_dword    s50, s[58:59], s91\n" },
    /* SMEM instructions */
    { 0xc0140c9d, 0x5b, true, "        s_scratch_load_dword s50, s[58:59], s91\n" },
    { 0xc0180c9d, 0x5b, true, "        s_scratch_load_dwordx2 s[50:51], s[58:59], s91\n" },
    { 0xc01c0c9d, 0x5b, true, "        s_scratch_load_dwordx4 s[50:53], s[58:59], s91\n" },
    { 0xc0540c9d, 0x5b, true, "        s_scratch_store_dword s50, s[58:59], s91\n" },
    { 0xc0580c9d, 0x5b, true,
        "        s_scratch_store_dwordx2 s[50:51], s[58:59], s91\n" },
    { 0xc05c0c9d, 0x5b, true,
        "        s_scratch_store_dwordx4 s[50:53], s[58:59], s91\n" },
    { 0xc0a00c9dU, 0x5b, true, "        s_dcache_discard s[58:59], s91 sdata=0x32\n" },
    { 0xc0a0001dU, 0x5b, true, "        s_dcache_discard s[58:59], s91\n" },
    { 0xc0a4001dU, 0x5b, true, "        s_dcache_discard_x2 s[58:59], s91\n" },
    { 0xc2020c9dU, 0x5b, true, "        s_atomic_swap   s50, s[58:59], 0x5b\n" },
    { 0xc2060c9dU, 0x5b, true, "        s_atomic_cmpswap s[50:51], s[58:59], 0x5b\n" },
    { 0xc20a0c9dU, 0x5b, true, "        s_atomic_add    s50, s[58:59], 0x5b\n" },
    { 0xc20e0c9dU, 0x5b, true, "        s_atomic_sub    s50, s[58:59], 0x5b\n" },
    { 0xc2120c9dU, 0x5b, true, "        s_atomic_smin   s50, s[58:59], 0x5b\n" },
    { 0xc2160c9dU, 0x5b, true, "        s_atomic_umin   s50, s[58:59], 0x5b\n" },
    { 0xc21a0c9dU, 0x5b, true, "        s_atomic_smax   s50, s[58:59], 0x5b\n" },
    { 0xc21e0c9dU, 0x5b, true, "        s_atomic_umax   s50, s[58:59], 0x5b\n" },
    { 0xc2220c9dU, 0x5b, true, "        s_atomic_and    s50, s[58:59], 0x5b\n" },
    { 0xc2260c9dU, 0x5b, true, "        s_atomic_or     s50, s[58:59], 0x5b\n" },
    { 0xc22a0c9dU, 0x5b, true, "        s_atomic_xor    s50, s[58:59], 0x5b\n" },
    { 0xc22e0c9dU, 0x5b, true, "        s_atomic_inc    s50, s[58:59], 0x5b\n" },
    { 0xc2320c9dU, 0x5b, true, "        s_atomic_dec    s50, s[58:59], 0x5b\n" },
    { 0xc2820c9dU, 0x5b, true, "        s_atomic_swap_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc2860c9dU, 0x5b, true, "        s_atomic_cmpswap_x2 s[50:53], s[58:59], 0x5b\n" },
    { 0xc28a0c9dU, 0x5b, true, "        s_atomic_add_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc28e0c9dU, 0x5b, true, "        s_atomic_sub_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc2920c9dU, 0x5b, true, "        s_atomic_smin_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc2960c9dU, 0x5b, true, "        s_atomic_umin_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc29a0c9dU, 0x5b, true, "        s_atomic_smax_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc29e0c9dU, 0x5b, true, "        s_atomic_umax_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc2a20c9dU, 0x5b, true, "        s_atomic_and_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc2a60c9dU, 0x5b, true, "        s_atomic_or_x2  s[50:51], s[58:59], 0x5b\n" },
    { 0xc2aa0c9dU, 0x5b, true, "        s_atomic_xor_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc2ae0c9dU, 0x5b, true, "        s_atomic_inc_x2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc2b20c9dU, 0x5b, true, "        s_atomic_dec_x2 s[50:51], s[58:59], 0x5b\n" },
    /* VOP DPP */
    { 0x0134d6faU, 0x101be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:1 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x102be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:2 bank_mask:0 row_mask:0\n" },
    /* VOP SDWA */
    { 0x0134d6f9U, 0x3d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x13d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte1 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x403d, true, "        v_cndmask_b32   v154, v61, v107, vcc mul:2 "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x803d, true, "        v_cndmask_b32   v154, v61, v107, vcc mul:4 "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0xc03d, true, "        v_cndmask_b32   v154, v61, v107, vcc div:2 "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x80003d, true, "        v_cndmask_b32   v154, s61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x8000003d, true, "        v_cndmask_b32   v154, v61, vcc_hi, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x8080003d, true, "        v_cndmask_b32   v154, s61, vcc_hi, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0, 0, false, nullptr }
};
