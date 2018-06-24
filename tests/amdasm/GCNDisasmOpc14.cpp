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
        "s50, s[58:59], s25 offset:0x5b\n" },
    { 0xc002cc9dU, 0x3200005b, true, "        s_load_dword    "
        "s50, s[58:59], s25 nv offset:0x5b\n" },
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
    { 0x7c2192f9U, 0x0404004eU, true, "        v_cmp_class_f32 vcc, v78, v201 "
            "src0_sel:word0 src1_sel:word0\n" },
    { 0x7c2192f9U, 0x0404924eU, true, "        v_cmp_class_f32 s[18:19], v78, v201 "
            "src0_sel:word0 src1_sel:word0\n" },
    { 0x7c2192f9U, 0x0404424eU, true, "        v_cmp_class_f32 vcc, v78, v201 "
            "src0_sel:word0 src1_sel:word0\n" },
    /* VOP2 instructions */
    { 0x3334d715U, 0, false, "        v_add_co_u32    v154, vcc, v21, v107\n" },
    { 0x3534d715U, 0, false, "        v_sub_co_u32    v154, vcc, v21, v107\n" },
    { 0x3734d715U, 0, false, "        v_subrev_co_u32 v154, vcc, v21, v107\n" },
    { 0x3934d715U, 0, false, "        v_addc_co_u32   v154, vcc, v21, v107, vcc\n" },
    { 0x3b34d715U, 0, false, "        v_subb_co_u32   v154, vcc, v21, v107, vcc\n" },
    { 0x3d34d715U, 0, false, "        v_subbrev_co_u32 v154, vcc, v21, v107, vcc\n" },
    { 0xd1191237U, 0x0002b41bU, true,
        "        v_add_co_u32    v55, s[18:19], s27, v90\n" },
    { 0xd1196a37U, 0x0002b41bU, true,
        "        v_add_co_u32    v55, vcc, s27, v90 vop3\n" },
    { 0xd11a1237U, 0x0002b41bU, true,
        "        v_sub_co_u32    v55, s[18:19], s27, v90\n" },
    { 0xd11a6a37U, 0x0002b41bU, true,
        "        v_sub_co_u32    v55, vcc, s27, v90 vop3\n" },
    { 0xd11b1237U, 0x0002b41bU, true,
        "        v_subrev_co_u32 v55, s[18:19], s27, v90\n" },
    { 0xd11b6a37U, 0x0002b41bU, true,
        "        v_subrev_co_u32 v55, vcc, s27, v90 vop3\n" },
    { 0xd11c0737U, 0x4066b41bU, true, "        v_addc_co_u32   "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd11c6a37U, 0x01aab41bU, true, "        v_addc_co_u32   "
                "v55, vcc, s27, v90, vcc vop3\n" },
    { 0xd11c6a37U, 0x41aab41bU, true, "        v_addc_co_u32   "
                "v55, vcc, s27, -v90, vcc\n" },
    { 0xd11d0737U, 0x4066b41bU, true, "        v_subb_co_u32   "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd11e0737U, 0x4066b41bU, true, "        v_subbrev_co_u32 "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd11d6a0bU, 0x00021480U, true,   /* no vop3 case! */
                "        v_subb_co_u32   v11, vcc, 0, v10, s[0:1]\n" },
    { 0x6934d715U, 0, false, "        v_add_u32       v154, v21, v107\n" },
    { 0x6b34d715U, 0, false, "        v_sub_u32       v154, v21, v107\n" },
    { 0x6d34d715U, 0, false, "        v_subrev_u32    v154, v21, v107\n" },
    { 0xd1340037U, 0x0002b41bU, true, "        v_add_u32       v55, s27, v90 vop3\n" },
    { 0xd1350037U, 0x0002b41bU, true, "        v_sub_u32       v55, s27, v90 vop3\n" },
    { 0xd1360037U, 0x0002b41bU, true, "        v_subrev_u32    v55, s27, v90 vop3\n" },
    /* opsel */
    { 0xd1014837U, 0x0002b41bU, true,
        "        v_add_f32       v55, s27, v90 op_sel:[1,0,1]\n" },
    { 0xd1011837U, 0x0002b41bU, true,
        "        v_add_f32       v55, s27, v90 op_sel:[1,1,0]\n" },
    /* VOP1 instructions */
    { 0x7f3c6d4fU, 0, false, "        v_mov_prsv_b32  v158, v79\n" },
    { 0x7f3c6f4fU, 0, false, "        v_screen_partition_4se_b32 v158, v79\n" },
    { 0x7f3c714fU, 0, false, "        VOP1_ill_56     v158, v79\n" },
    { 0xd1760037U, 0x0000011bU, true, "        v_mov_prsv_b32  v55, v27 vop3\n" },
    { 0xd1770037U, 0x0000011bU, true,
        "        v_screen_partition_4se_b32 v55, v27 vop3\n" },
    { 0xd1780037U, 0x0000011bU, true, "        VOP3A_ill_376   v55, v27, s0, s0\n" },
    { 0x7f3c9b4fU, 0, false, "        v_cvt_norm_i16_f16 v158, v79\n" },
    { 0x7f3c9affU, 0x3d4c, true, "        v_cvt_norm_i16_f16 "
            "v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c9d4fU, 0, false, "        v_cvt_norm_u16_f16 v158, v79\n" },
    { 0x7f3c9cffU, 0x3d4c, true, "        v_cvt_norm_u16_f16 "
            "v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c9f4fU, 0, false, "        v_sat_pk_u8_i16 v158, v79\n" },
    { 0x7f3c9effU, 0x3d4c, true, "        v_sat_pk_u8_i16 v158, 0x3d4c\n" },
    { 0x7f3ca14fU, 0, false, "        v_writelane_regwr_b32 v158, v79\n" },
    { 0x7f3ca34fU, 0, false, "        v_swap_b32      v158, v79\n" },
    { 0xd18d0037U, 0x0000011bU, true, "        v_cvt_norm_i16_f16 v55, v27 vop3\n" },
    { 0xd18e0037U, 0x0000011bU, true, "        v_cvt_norm_u16_f16 v55, v27 vop3\n" },
    { 0xd18f0037U, 0x0000011bU, true, "        v_sat_pk_u8_i16 v55, v27 vop3\n" },
    { 0xd1900037U, 0x0000011bU, true, "        v_writelane_regwr_b32 v55, v27 vop3\n" },
    { 0xd1910037U, 0x0000011bU, true, "        v_swap_b32      v55, v27 vop3\n" },
    /* opsel */
    { 0xd14a0837U, 0x0000011bU, true, "        v_cvt_f16_f32   v55, v27 op_sel:[1,0]\n" },
    { 0xd14a4037U, 0x0000011bU, true, "        v_cvt_f16_f32   v55, v27 op_sel:[0,1]\n" },
    /* VOPC opsel */
    { 0xd0cc182aU, 0x0002d732U, true,
            "        v_cmp_gt_u32    s[42:43], v50, v107 op_sel:[1,1,0]\n" },
    { 0xd0cc402aU, 0x0002d732U, true,
            "        v_cmp_gt_u32    s[42:43], v50, v107 op_sel:[0,0,1]\n" },
    /* VOP3 instructions */
    { 0xd1f10037U, 0x07974d4fU, true, "        v_mad_u32_u16   v55, v79, v166, v229\n" },
    { 0xd1f20037U, 0x07974d4fU, true, "        v_mad_i32_i16   v55, v79, v166, v229\n" },
    { 0xd1f30037U, 0x07974d4fU, true, "        v_xad_u32       v55, v79, v166, v229\n" },
    { 0xd1f40037U, 0x07974d4fU, true, "        v_min3_f16      v55, v79, v166, v229\n" },
    { 0xd1f50037U, 0x07974d4fU, true, "        v_min3_i16      v55, v79, v166, v229\n" },
    { 0xd1f60037U, 0x07974d4fU, true, "        v_min3_u16      v55, v79, v166, v229\n" },
    { 0xd1f70037U, 0x07974d4fU, true, "        v_max3_f16      v55, v79, v166, v229\n" },
    { 0xd1f80037U, 0x07974d4fU, true, "        v_max3_i16      v55, v79, v166, v229\n" },
    { 0xd1f90037U, 0x07974d4fU, true, "        v_max3_u16      v55, v79, v166, v229\n" },
    { 0xd1fa0037U, 0x07974d4fU, true, "        v_med3_f16      v55, v79, v166, v229\n" },
    { 0xd1fb0037U, 0x07974d4fU, true, "        v_med3_i16      v55, v79, v166, v229\n" },
    { 0xd1fc0037U, 0x07974d4fU, true, "        v_med3_u16      v55, v79, v166, v229\n" },
    { 0xd1fd0037U, 0x07974d4fU, true, "        v_lshl_add_u32  v55, v79, v166, v229\n" },
    { 0xd1fe0037U, 0x07974d4fU, true, "        v_add_lshl_u32  v55, v79, v166, v229\n" },
    { 0xd1ff0037U, 0x07974d4fU, true, "        v_add3_u32      v55, v79, v166, v229\n" },
    { 0xd2000037U, 0x07974d4fU, true, "        v_lshl_or_b32   v55, v79, v166, v229\n" },
    { 0xd2010037U, 0x07974d4fU, true, "        v_and_or_b32    v55, v79, v166, v229\n" },
    { 0xd2020037U, 0x07974d4fU, true, "        v_or3_b32       v55, v79, v166, v229\n" },
    { 0xd2030037U, 0x07974d4fU, true, "        v_mad_f16       v55, v79, v166, v229\n" },
    { 0xd2040037U, 0x07974d4fU, true, "        v_mad_u16       v55, v79, v166, v229\n" },
    { 0xd2050037U, 0x07974d4fU, true, "        v_mad_i16       v55, v79, v166, v229\n" },
    { 0xd2060037U, 0x07974d4fU, true, "        v_fma_f16       v55, v79, v166, v229\n" },
    { 0xd2070037U, 0x07974d4fU, true, "        v_div_fixup_f16 v55, v79, v166, v229\n" },
    { 0xd1ea0037U, 0x07974d4fU, true, "        v_mad_legacy_f16 v55, v79, v166, v229\n" },
    { 0xd1eb0037U, 0x07974d4fU, true, "        v_mad_legacy_u16 v55, v79, v166, v229\n" },
    { 0xd1ec0037U, 0x07974d4fU, true, "        v_mad_legacy_i16 v55, v79, v166, v229\n" },
    { 0xd1ee0037U, 0x07974d4fU, true, "        v_fma_legacy_f16 v55, v79, v166, v229\n" },
    { 0xd1ef0037U, 0x07974d4fU, true,
            "        v_div_fixup_legacy_f16 v55, v79, v166, v229\n" },
    { 0xd276002aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29\n" },
    { 0xd277002aU, 0x007402a7, true, "        v_interp_p2_f16 v42, s1, attr39.z, s29\n" },
    { 0xd2990037U, 0x0002b51bU, true, "        v_cvt_pknorm_i16_f16 v55, v27, v90\n" },
    { 0xd29a0037U, 0x0002b51bU, true, "        v_cvt_pknorm_u16_f16 v55, v27, v90\n" },
    { 0xd29b0037U, 0x0002b51bU, true, "        v_readlane_regrd_b32 s55, v27, v90\n" },
    { 0xd29c0037U, 0x0002b51bU, true, "        v_add_i32       v55, v27, v90\n" },
    { 0xd29d0037U, 0x0002b51bU, true, "        v_sub_i32       v55, v27, v90\n" },
    { 0xd29e0037U, 0x0002b51bU, true, "        v_add_i16       v55, v27, v90\n" },
    { 0xd29f0037U, 0x0002b51bU, true, "        v_sub_i16       v55, v27, v90\n" },
    { 0xd2a00037U, 0x0002b51bU, true, "        v_pack_b32_f16  v55, v27, v90\n" },
    /* VOP3 - op_sel */
    { 0xd276082aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,0,0,0]\n" },
    { 0xd276102aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[0,1,0,0]\n" },
    { 0xd276182aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,1,0,0]\n" },
    { 0xd276202aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[0,0,1,0]\n" },
    { 0xd276282aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,0,1,0]\n" },
    { 0xd276302aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[0,1,1,0]\n" },
    { 0xd276382aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,1,1,0]\n" },
    { 0xd276402aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[0,0,0,1]\n" },
    { 0xd276482aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,0,0,1]\n" },
    { 0xd276502aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[0,1,0,1]\n" },
    { 0xd276582aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,1,0,1]\n" },
    { 0xd276602aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[0,0,1,1]\n" },
    { 0xd276682aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,0,1,1]\n" },
    { 0xd276702aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[0,1,1,1]\n" },
    { 0xd276782aU, 0x007402a7, true,
        "        v_interp_p2_legacy_f16 v42, s1, attr39.z, s29 op_sel:[1,1,1,1]\n" },
    /* VOP3 - op_sel in instructions */
    { 0xd1ce5837U, 0x07974d4fU, true,
        "        v_alignbit_b32  v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1cf5837U, 0x07974d4fU, true,
        "        v_alignbyte_b32 v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1ea5837U, 0x07974d4fU, true,
        "        v_mad_legacy_f16 v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1eb5837U, 0x07974d4fU, true,
        "        v_mad_legacy_u16 v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1ec5837U, 0x07974d4fU, true,
        "        v_mad_legacy_i16 v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1ee5837U, 0x07974d4fU, true,
        "        v_fma_legacy_f16 v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1ef5837U, 0x07974d4fU, true,
        "        v_div_fixup_legacy_f16 v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1f45837U, 0x07974d4fU, true,
        "        v_min3_f16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1f55837U, 0x07974d4fU, true,
        "        v_min3_i16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1f65837U, 0x07974d4fU, true,
        "        v_min3_u16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1f75837U, 0x07974d4fU, true,
        "        v_max3_f16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1f85837U, 0x07974d4fU, true,
        "        v_max3_i16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1f95837U, 0x07974d4fU, true,
        "        v_max3_u16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1fa5837U, 0x07974d4fU, true,
        "        v_med3_f16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1fb5837U, 0x07974d4fU, true,
        "        v_med3_i16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd1fc5837U, 0x07974d4fU, true,
        "        v_med3_u16      v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    { 0xd2991837U, 0x0002b51bU, true,
        "        v_cvt_pknorm_i16_f16 v55, v27, v90 op_sel:[1,1,0]\n" },
    { 0xd29a1837U, 0x0002b51bU, true,
        "        v_cvt_pknorm_u16_f16 v55, v27, v90 op_sel:[1,1,0]\n" },
    { 0xd2995837U, 0x0002b51bU, true,
        "        v_cvt_pknorm_i16_f16 v55, v27, v90 op_sel:[1,1,1]\n" },
    { 0xd29a5837U, 0x0002b51bU, true,
        "        v_cvt_pknorm_u16_f16 v55, v27, v90 op_sel:[1,1,1]\n" },
    { 0xd29e1837U, 0x0002b51bU, true,
        "        v_add_i16       v55, v27, v90 op_sel:[1,1,0]\n" },
    { 0xd29f1837U, 0x0002b51bU, true,
        "        v_sub_i16       v55, v27, v90 op_sel:[1,1,0]\n" },
    { 0xd2a01837U, 0x0002b51bU, true,
        "        v_pack_b32_f16  v55, v27, v90 op_sel:[1,1,0]\n" },
    { 0xd29e5837U, 0x0002b51bU, true,
        "        v_add_i16       v55, v27, v90 op_sel:[1,1,1]\n" },
    { 0xd29f5837U, 0x0002b51bU, true,
        "        v_sub_i16       v55, v27, v90 op_sel:[1,1,1]\n" },
    { 0xd2a05837U, 0x0002b51bU, true,
        "        v_pack_b32_f16  v55, v27, v90 op_sel:[1,1,1]\n" },
    { 0xd1db5837U, 0x07974d4fU, true,
        "        v_sad_u16       v55, v79, v166, v229 op_sel:[1,1,0,1]\n" },
    // VOP3P encoding
    { 0xd3800037U, 0x07974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 op_sel_hi:[0,0,0]\n" },
    { 0xd3804037U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229\n" },
    { 0xd3800037U, 0x0f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 op_sel_hi:[1,0,0]\n" },
    { 0xd3800037U, 0x17974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 op_sel_hi:[0,1,0]\n" },
    { 0xd3824037U, 0x10034d4fU, true,
        "        v_pk_add_i16    v55, v79, v166 op_sel_hi:[0,1]\n" },
    { 0xd3804037U, 0x07974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 op_sel_hi:[0,0,1]\n" },
    { 0xd3800837U, 0x07974d4fU, true, "        v_pk_mad_i16    "
            "v55, v79, v166, v229 op_sel:[1,0,0] op_sel_hi:[0,0,0]\n" },
    { 0xd3801037U, 0x07974d4fU, true, "        v_pk_mad_i16    "
            "v55, v79, v166, v229 op_sel:[0,1,0] op_sel_hi:[0,0,0]\n" },
    { 0xd3802037U, 0x07974d4fU, true, "        v_pk_mad_i16    "
            "v55, v79, v166, v229 op_sel:[0,0,1] op_sel_hi:[0,0,0]\n" },
    { 0xd3825837U, 0x10034d4fU, true,
        "        v_pk_add_i16    v55, v79, v166 op_sel:[1,1] op_sel_hi:[0,1]\n" },
    { 0xd3804137U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_hi:[1,0,0]\n" },
    { 0xd3804237U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_hi:[0,1,0]\n" },
    { 0xd3804337U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_hi:[1,1,0]\n" },
    { 0xd3804437U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_hi:[0,0,1]\n" },
    { 0xd3804537U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_hi:[1,0,1]\n" },
    { 0xd3804637U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_hi:[0,1,1]\n" },
    { 0xd3804737U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_hi:[1,1,1]\n" },
    { 0xd3824337U, 0x18034d4fU, true,
        "        v_pk_add_i16    v55, v79, v166 neg_hi:[1,1]\n" },
    { 0xd3804037U, 0x3f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_lo:[1,0,0]\n" },
    { 0xd3804037U, 0x5f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_lo:[0,1,0]\n" },
    { 0xd3804037U, 0x7f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_lo:[1,1,0]\n" },
    { 0xd3804037U, 0x9f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_lo:[0,0,1]\n" },
    { 0xd3804037U, 0xbf974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_lo:[1,0,1]\n" },
    { 0xd3804037U, 0xdf974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_lo:[0,1,1]\n" },
    { 0xd3804037U, 0xff974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 neg_lo:[1,1,1]\n" },
    { 0xd3824037U, 0x78034d4fU, true,
        "        v_pk_add_i16    v55, v79, v166 neg_lo:[1,1]\n" },
    /* VOP3P instructions */
    { 0xd3814037U, 0x18034d4fU, true, "        v_pk_mul_lo_u16 v55, v79, v166\n" },
    { 0xd3824037U, 0x18034d4fU, true, "        v_pk_add_i16    v55, v79, v166\n" },
    { 0xd3834037U, 0x18034d4fU, true, "        v_pk_sub_i16    v55, v79, v166\n" },
    { 0xd3844037U, 0x18034d4fU, true, "        v_pk_lshlrev_b16 v55, v79, v166\n" },
    { 0xd3854037U, 0x18034d4fU, true, "        v_pk_lshrrev_b16 v55, v79, v166\n" },
    { 0xd3864037U, 0x18034d4fU, true, "        v_pk_ashrrev_i16 v55, v79, v166\n" },
    { 0xd3874037U, 0x18034d4fU, true, "        v_pk_max_i16    v55, v79, v166\n" },
    { 0xd3884037U, 0x18034d4fU, true, "        v_pk_min_i16    v55, v79, v166\n" },
    { 0xd3894037U, 0x1f974d4fU, true, "        v_pk_mad_u16    v55, v79, v166, v229\n" },
    { 0xd38a4037U, 0x18034d4fU, true, "        v_pk_add_u16    v55, v79, v166\n" },
    { 0xd38b4037U, 0x18034d4fU, true, "        v_pk_sub_u16    v55, v79, v166\n" },
    { 0xd38c4037U, 0x18034d4fU, true, "        v_pk_max_u16    v55, v79, v166\n" },
    { 0xd38d4037U, 0x18034d4fU, true, "        v_pk_min_u16    v55, v79, v166\n" },
    { 0xd38e4037U, 0x1f974d4fU, true, "        v_pk_fma_f16    v55, v79, v166, v229\n" },
    { 0xd38f4037U, 0x18034d4fU, true, "        v_pk_add_f16    v55, v79, v166\n" },
    { 0xd3904037U, 0x18034d4fU, true, "        v_pk_mul_f16    v55, v79, v166\n" },
    { 0xd3914037U, 0x18034d4fU, true, "        v_pk_min_f16    v55, v79, v166\n" },
    { 0xd3924037U, 0x18034d4fU, true, "        v_pk_max_f16    v55, v79, v166\n" },
    { 0xd3a04037U, 0x1f974d4fU, true, "        v_mad_mix_f32   v55, v79, v166, v229\n" },
    { 0xd3a14037U, 0x1f974d4fU, true, "        v_mad_mixlo_f16 v55, v79, v166, v229\n" },
    { 0xd3a24037U, 0x1f974d4fU, true, "        v_mad_mixhi_f16 v55, v79, v166, v229\n" },
    /* DS instructions */
    { 0xd83acd67U, 0x00004700U, true, "        ds_write_addtid_b32 v71 offset:52583\n" },
    { 0xd8a8cd67U, 0x0000a947U, true,
        "        ds_write_b8_d16_hi v71, v169 offset:52583\n" },
    { 0xd8aacd67U, 0x0000a947U, true,
        "        ds_write_b16_d16_hi v71, v169 offset:52583\n" },
    { 0xd8accd67U, 0x8b000047U, true, "        ds_read_u8_d16  v139, v71 offset:52583\n" },
    { 0xd8aecd67U, 0x8b000047U, true,
        "        ds_read_u8_d16_hi v139, v71 offset:52583\n" },
    { 0xd8b0cd67U, 0x8b000047U, true, "        ds_read_i8_d16  v139, v71 offset:52583\n" },
    { 0xd8b2cd67U, 0x8b000047U, true,
        "        ds_read_i8_d16_hi v139, v71 offset:52583\n" },
    { 0xd8b4cd67U, 0x8b000047U, true, "        ds_read_u16_d16 v139, v71 offset:52583\n" },
    { 0xd96ccd67U, 0x8b000000U, true, "        ds_read_addtid_b32 v139 offset:52583\n" },
    /* MUBUF instructions */
    { 0xe067725bU, 0x23343d12U, true, "        buffer_store_byte_d16_hi "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe06f725bU, 0x23343d12U, true, "        buffer_store_short_d16_hi "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe083725bU, 0x23343d12U, true, "        buffer_load_ubyte_d16 "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe087725bU, 0x23343d12U, true, "        buffer_load_ubyte_d16_hi "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe08b725bU, 0x23343d12U, true, "        buffer_load_sbyte_d16 "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe08f725bU, 0x23343d12U, true, "        buffer_load_sbyte_d16_hi "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe093725bU, 0x23343d12U, true, "        buffer_load_short_d16 "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe097725bU, 0x23343d12U, true, "        buffer_load_short_d16_hi "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe09b725bU, 0x23343d12U, true, "        buffer_load_format_d16_hi_x "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    { 0xe09f725bU, 0x23343d12U, true, "        buffer_store_format_d16_hi_x "
        "v61, v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds\n" },
    /* MUBUF D16 FORMAT instrs */
    { 0xe020325bU, 0x23343d12U, true, "        buffer_load_format_d16_x "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe024325bU, 0x23343d12U, true, "        buffer_load_format_d16_xy "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe028325bU, 0x23343d12U, true, "        buffer_load_format_d16_xyz "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe02c325bU, 0x23343d12U, true, "        buffer_load_format_d16_xyzw "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe030325bU, 0x23343d12U, true, "        buffer_store_format_d16_x "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe034325bU, 0x23343d12U, true, "        buffer_store_format_d16_xy "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe038325bU, 0x23343d12U, true, "        buffer_store_format_d16_xyz "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe03c325bU, 0x23343d12U, true, "        buffer_store_format_d16_xyzw "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    /* MTBUF D16 FORMAT instrs */
    { 0xea8c77d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8cf7d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_xy "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8d77d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_xyz "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8df7d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_xyzw "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8e77d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8ef7d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_xy "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8f77d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_xyz "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8ff7d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_xyzw "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    /* MIMG instructions */
    /* TODO: check register ranges */
    { 0xf108fb00U, 0x00159d79U, true, "        image_gather4h  "
            "v[157:160], v[121:124], s[84:91], s[0:3] dmask:11 unorm glc a16 da\n" },
    { 0xf1087b00U, 0x00159d79U, true, "        image_gather4h  "
            "v[157:160], v[121:124], s[84:91], s[0:3] dmask:11 unorm glc da\n" },
    { 0xf128fb00U, 0x00159d79U, true, "        image_gather4h_pck "
            "v[157:160], v[121:124], s[84:91], s[0:3] dmask:11 unorm glc a16 da\n" },
    { 0xf12cfb00U, 0x00159d79U, true, "        image_gather8h_pck "
            "v[157:160], v[121:124], s[84:91], s[0:3] dmask:11 unorm glc a16 da\n" },
    /* FLAT encoding */
    { 0xdc400000U, 0x2f8000bbU, true, "        flat_load_ubyte v47, v[187:188] nv\n" },
    { 0xdc400000U, 0x2f0000bbU, true, "        flat_load_ubyte v47, v[187:188]\n" },
    { 0xdc402000U, 0x2f0000bbU, true, "        flat_load_ubyte v47, v[187:188] lds\n" },
    { 0xdc400211U, 0x2f0000bbU, true,
        "        flat_load_ubyte v47, v[187:188] inst_offset:529\n" },
    /* FLAT instructions */
    { 0xdc670000U, 0x008041bbU, true, "        flat_store_byte_d16_hi "
        "v[187:188], v65 glc slc nv\n" },
    { 0xdc6f0000U, 0x008041bbU, true, "        flat_store_short_d16_hi "
        "v[187:188], v65 glc slc nv\n" },
    { 0xdc830000U, 0x2f8000bbU, true, "        flat_load_ubyte_d16 "
        "v47, v[187:188] glc slc nv\n" },
    { 0xdc870000U, 0x2f8000bbU, true, "        flat_load_ubyte_d16_hi "
        "v47, v[187:188] glc slc nv\n" },
    { 0xdc8b0000U, 0x2f8000bbU, true, "        flat_load_sbyte_d16 "
        "v47, v[187:188] glc slc nv\n" },
    { 0xdc8f0000U, 0x2f8000bbU, true, "        flat_load_sbyte_d16_hi "
        "v47, v[187:188] glc slc nv\n" },
    { 0xdc930000U, 0x2f8000bbU, true, "        flat_load_short_d16 "
        "v47, v[187:188] glc slc nv\n" },
    { 0xdc970000U, 0x2f8000bbU, true, "        flat_load_short_d16_hi "
        "v47, v[187:188] glc slc nv\n" },
    /* FLAT SCRATCH encoding */
    { 0xdc434000U, 0x2f3100bbU, true,
        "        scratch_load_ubyte v47, off, s49 glc slc\n" },
    { 0xdc434000U, 0x2fff00bbU, true,
        "        scratch_load_ubyte v47, v187, off glc slc nv\n" },
    { 0xdc434000U, 0x2f7f00bbU, true,
        "        scratch_load_ubyte v47, v187, off glc slc\n" },
    { 0xdc434413U, 0x2fff00bbU, true,
        "        scratch_load_ubyte v47, v187, off inst_offset:1043 glc slc nv\n" },
    { 0xdc435413U, 0x2fff00bbU, true,
        "        scratch_load_ubyte v47, v187, off inst_offset:-3053 glc slc nv\n" },
    { 0xdc435000U, 0x2fff00bbU, true,
        "        scratch_load_ubyte v47, v187, off inst_offset:-4096 glc slc nv\n" },
    /* FLAT SCRATCH instructions */
    { 0xdc474000U, 0x2f3100bbU, true,
        "        scratch_load_sbyte v47, off, s49 glc slc\n" },
    { 0xdc4b4000U, 0x2f3100bbU, true,
        "        scratch_load_ushort v47, off, s49 glc slc\n" },
    { 0xdc4f4000U, 0x2f3100bbU, true,
        "        scratch_load_sshort v47, off, s49 glc slc\n" },
    { 0xdc534000U, 0x2f3100bbU, true,
        "        scratch_load_dword v47, off, s49 glc slc\n" },
    { 0xdc574000U, 0x2f3100bbU, true,
        "        scratch_load_dwordx2 v[47:48], off, s49 glc slc\n" },
    { 0xdc5b4000U, 0x2f3100bbU, true,
        "        scratch_load_dwordx3 v[47:49], off, s49 glc slc\n" },
    { 0xdc5f4000U, 0x2f3100bbU, true,
        "        scratch_load_dwordx4 v[47:50], off, s49 glc slc\n" },
    { 0xdc634000U, 0x003141bbU, true,
        "        scratch_store_byte off, v65, s49 glc slc\n" },
    { 0xdc674000U, 0x003141bbU, true,
        "        scratch_store_byte_d16_hi off, v65, s49 glc slc\n" },
    { 0xdc6b4000U, 0x003141bbU, true,
        "        scratch_store_short off, v65, s49 glc slc\n" },
    { 0xdc6f4000U, 0x003141bbU, true,
        "        scratch_store_short_d16_hi off, v65, s49 glc slc\n" },
    { 0xdc734000U, 0x003141bbU, true,
        "        scratch_store_dword off, v65, s49 glc slc\n" },
    { 0xdc774000U, 0x003141bbU, true,
        "        scratch_store_dwordx2 off, v[65:66], s49 glc slc\n" },
    { 0xdc7b4000U, 0x003141bbU, true,
        "        scratch_store_dwordx3 off, v[65:67], s49 glc slc\n" },
    { 0xdc7f4000U, 0x003141bbU, true,
        "        scratch_store_dwordx4 off, v[65:68], s49 glc slc\n" },
    { 0xdc834000U, 0x2f3100bbU, true,
        "        scratch_load_ubyte_d16 v47, off, s49 glc slc\n" },
    { 0xdc874000U, 0x2f3100bbU, true,
        "        scratch_load_ubyte_d16_hi v47, off, s49 glc slc\n" },
    { 0xdc8b4000U, 0x2f3100bbU, true,
        "        scratch_load_sbyte_d16 v47, off, s49 glc slc\n" },
    { 0xdc8f4000U, 0x2f3100bbU, true,
        "        scratch_load_sbyte_d16_hi v47, off, s49 glc slc\n" },
    { 0xdc934000U, 0x2f3100bbU, true,
        "        scratch_load_short_d16 v47, off, s49 glc slc\n" },
    { 0xdc974000U, 0x2f3100bbU, true,
        "        scratch_load_short_d16_hi v47, off, s49 glc slc\n" },
    /* FLAT GLOBAL encoding */
    { 0xdc438000U, 0x2f3100bbU, true,
        "        global_load_ubyte v47, v187, s[49:50] glc slc\n" },
    { 0xdc438000U, 0x2fff00bbU, true,
        "        global_load_ubyte v47, v[187:188], off glc slc nv\n" },
    { 0xdc438000U, 0x2f7f00bbU, true,
        "        global_load_ubyte v47, v[187:188], off glc slc\n" },
    { 0xdc438413U, 0x2f3100bbU, true, "        global_load_ubyte "
        "v47, v187, s[49:50] inst_offset:1043 glc slc\n" },
    { 0xdc439413U, 0x2f3100bbU, true, "        global_load_ubyte "
        "v47, v187, s[49:50] inst_offset:-3053 glc slc\n" },
    /* FLAT GLOBAL instructions */
    { 0xdc478000U, 0x2f3100bbU, true,
        "        global_load_sbyte v47, v187, s[49:50] glc slc\n" },
    { 0xdc4b8000U, 0x2f3100bbU, true,
        "        global_load_ushort v47, v187, s[49:50] glc slc\n" },
    { 0xdc4f8000U, 0x2f3100bbU, true,
        "        global_load_sshort v47, v187, s[49:50] glc slc\n" },
    { 0xdc538000U, 0x2f3100bbU, true,
        "        global_load_dword v47, v187, s[49:50] glc slc\n" },
    { 0xdc578000U, 0x2f3100bbU, true,
        "        global_load_dwordx2 v[47:48], v187, s[49:50] glc slc\n" },
    { 0xdc5b8000U, 0x2f3100bbU, true,
        "        global_load_dwordx3 v[47:49], v187, s[49:50] glc slc\n" },
    { 0xdc5f8000U, 0x2f3100bbU, true,
        "        global_load_dwordx4 v[47:50], v187, s[49:50] glc slc\n" },
    { 0xdc638000U, 0x003141bbU, true,
        "        global_store_byte v187, v65, s[49:50] glc slc\n" },
    { 0xdc678000U, 0x003141bbU, true,
        "        global_store_byte_d16_hi v187, v65, s[49:50] glc slc\n" },
    { 0xdc6b8000U, 0x003141bbU, true,
        "        global_store_short v187, v65, s[49:50] glc slc\n" },
    { 0xdc6f8000U, 0x003141bbU, true,
        "        global_store_short_d16_hi v187, v65, s[49:50] glc slc\n" },
    { 0xdc738000U, 0x003141bbU, true,
        "        global_store_dword v187, v65, s[49:50] glc slc\n" },
    { 0xdc778000U, 0x003141bbU, true,
        "        global_store_dwordx2 v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdc7b8000U, 0x003141bbU, true,
        "        global_store_dwordx3 v187, v[65:67], s[49:50] glc slc\n" },
    { 0xdc7f8000U, 0x003141bbU, true,
        "        global_store_dwordx4 v187, v[65:68], s[49:50] glc slc\n" },
    { 0xdc838000U, 0x2f3100bbU, true,
        "        global_load_ubyte_d16 v47, v187, s[49:50] glc slc\n" },
    { 0xdc878000U, 0x2f3100bbU, true,
        "        global_load_ubyte_d16_hi v47, v187, s[49:50] glc slc\n" },
    { 0xdc8b8000U, 0x2f3100bbU, true,
        "        global_load_sbyte_d16 v47, v187, s[49:50] glc slc\n" },
    { 0xdc8f8000U, 0x2f3100bbU, true,
        "        global_load_sbyte_d16_hi v47, v187, s[49:50] glc slc\n" },
    { 0xdc938000U, 0x2f3100bbU, true,
        "        global_load_short_d16 v47, v187, s[49:50] glc slc\n" },
    { 0xdc978000U, 0x2f3100bbU, true,
        "        global_load_short_d16_hi v47, v187, s[49:50] glc slc\n" },
    { 0xdd038000U, 0x2f3141bbU, true,
        "        global_atomic_swap v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd078000U, 0x2f3141bbU, true, "        global_atomic_cmpswap "
        "v47, v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdd0b8000U, 0x2f3141bbU, true,
        "        global_atomic_add v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd0f8000U, 0x2f3141bbU, true,
        "        global_atomic_sub v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd138000U, 0x2f3141bbU, true,
        "        global_atomic_smin v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd178000U, 0x2f3141bbU, true,
        "        global_atomic_umin v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd1b8000U, 0x2f3141bbU, true,
        "        global_atomic_smax v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd1f8000U, 0x2f3141bbU, true,
        "        global_atomic_umax v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd238000, 0x2f3141bbU, true,
        "        global_atomic_and v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd278000, 0x2f3141bbU, true,
        "        global_atomic_or v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd2b8000, 0x2f3141bbU, true,
        "        global_atomic_xor v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd2f8000, 0x2f3141bbU, true,
        "        global_atomic_inc v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd338000, 0x2f3141bbU, true,
        "        global_atomic_dec v47, v187, v65, s[49:50] glc slc\n" },
    { 0xdd838000U, 0x2f3141bbU, true, "        global_atomic_swap_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdd878000U, 0x2f3141bbU, true, "        global_atomic_cmpswap_x2 "
        "v[47:48], v187, v[65:68], s[49:50] glc slc\n" },
    { 0xdd8b8000U, 0x2f3141bbU, true, "        global_atomic_add_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdd8f8000U, 0x2f3141bbU, true, "        global_atomic_sub_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdd938000U, 0x2f3141bbU, true, "        global_atomic_smin_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdd978000U, 0x2f3141bbU, true, "        global_atomic_umin_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdd9b8000U, 0x2f3141bbU, true, "        global_atomic_smax_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdd9f8000U, 0x2f3141bbU, true, "        global_atomic_umax_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdda38000, 0x2f3141bbU, true, "        global_atomic_and_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xdda78000, 0x2f3141bbU, true, "        global_atomic_or_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xddab8000, 0x2f3141bbU, true, "        global_atomic_xor_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xddaf8000, 0x2f3141bbU, true, "        global_atomic_inc_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    { 0xddb38000, 0x2f3141bbU, true, "        global_atomic_dec_x2 "
        "v[47:48], v187, v[65:66], s[49:50] glc slc\n" },
    // VOP3 CMP
    { 0x7c7f92ffU, 0x3d4cU, true, "        v_cmpx_tru_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0, 0, false, nullptr }
};

/* for VEGA 20 series with GCN1.4.1 */
const GCNDisasmOpcodeCase decGCNOpcodeGCN141Cases[] =
{
    { 0xd3a04037U, 0x1f974d4fU, true, "        v_fma_mix_f32   v55, v79, v166, v229\n" },
    { 0xd3a14037U, 0x1f974d4fU, true, "        v_fma_mixlo_f16 v55, v79, v166, v229\n" },
    { 0xd3a24037U, 0x1f974d4fU, true, "        v_fma_mixhi_f16 v55, v79, v166, v229\n" },
    { 0xd3a34037U, 0x1f974d4fU, true, "        v_dot2_f32_f16  v55, v79, v166, v229\n" },
    { 0xd3a64037U, 0x1f974d4fU, true, "        v_dot2_i32_i16  v55, v79, v166, v229\n" },
    { 0xd3a74037U, 0x1f974d4fU, true, "        v_dot2_u32_u16  v55, v79, v166, v229\n" },
    { 0xd3a84037U, 0x1f974d4fU, true, "        v_dot4_i32_i8   v55, v79, v166, v229\n" },
    { 0xd3a94037U, 0x1f974d4fU, true, "        v_dot4_u32_u8   v55, v79, v166, v229\n" },
    { 0xd3aa4037U, 0x1f974d4fU, true, "        v_dot8_i32_i4   v55, v79, v166, v229\n" },
    { 0xd3ab4037U, 0x1f974d4fU, true, "        v_dot8_u32_u4   v55, v79, v166, v229\n" },
    { 0x7734d715U, 0, false, "        v_fmac_f32      v154, v21, v107\n" },
    { 0xd13b0037U, 0x0002b41bU, true, "        v_fmac_f32      v55, s27, v90 vop3\n" },
    { 0x7b34d715U, 0, false, "        v_xnor_b32      v154, v21, v107\n" },
    { 0xd13d0037U, 0x0002b41bU, true, "        v_xnor_b32      v55, s27, v90 vop3\n" },
    // VOP3P from GCN1.4
    { 0xd3800037U, 0x07974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 op_sel_hi:[0,0,0]\n" },
    { 0xd3804037U, 0x1f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229\n" },
    { 0xd3800037U, 0x0f974d4fU, true,
        "        v_pk_mad_i16    v55, v79, v166, v229 op_sel_hi:[1,0,0]\n" },
    { 0x3334d715U, 0, false, "        v_add_co_u32    v154, vcc, v21, v107\n" },
    { 0x3534d715U, 0, false, "        v_sub_co_u32    v154, vcc, v21, v107\n" },
    // test for other opcodes (mapping of RX VEGA VOP3 code table
    // VOP3 CMP
    { 0x7c7f92ffU, 0x3d4cU, true, "        v_cmpx_tru_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0xd1760037U, 0x0000011bU, true, "        v_mov_prsv_b32  v55, v27 vop3\n" },
    { 0xd1770037U, 0x0000011bU, true,
        "        v_screen_partition_4se_b32 v55, v27 vop3\n" },
    { 0, 0, false, nullptr }
};
