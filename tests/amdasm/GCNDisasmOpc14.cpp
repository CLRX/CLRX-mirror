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

/* for Radeon RX3X0 series with GCN1.2 */
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
    { 0, 0, false, nullptr }
};
