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
#include "GCNAsmOpc.h"

const GCNAsmOpcodeCase encGCN14OpcodeCases[] =
{
    /* extra scalar registers */
    { "s_add_u32 s21, shared_base, s61\n", 0x80153debU, 0, false, true, "" },
    { "s_add_u32 s21, src_shared_base, s61\n", 0x80153debU, 0, false, true, "" },
    { "s_add_u32 s21, shared_limit, s61\n", 0x80153decU, 0, false, true, "" },
    { "s_add_u32 s21, src_shared_limit, s61\n", 0x80153decU, 0, false, true, "" },
    { "s_add_u32 s21, private_base, s61\n", 0x80153dedU, 0, false, true, "" },
    { "s_add_u32 s21, src_private_base, s61\n", 0x80153dedU, 0, false, true, "" },
    { "s_add_u32 s21, private_limit, s61\n", 0x80153deeU, 0, false, true, "" },
    { "s_add_u32 s21, src_private_limit, s61\n", 0x80153deeU, 0, false, true, "" },
    { "s_add_u32 s21, pops_exiting_wave_id, s61\n", 0x80153defU, 0, false, true, "" },
    { "s_add_u32 s21, src_pops_exiting_wave_id, s61\n", 0x80153defU, 0, false, true, "" },
    { "s_add_u32 s21, execz, s61\n", 0x80153dfcU, 0, false, true, "" },
    { "s_add_u32 s21, src_execz, s61\n", 0x80153dfcU, 0, false, true, "" },
    { "s_add_u32 s21, vccz, s61\n", 0x80153dfbU, 0, false, true, "" },
    { "s_add_u32 s21, src_vccz, s61\n", 0x80153dfbU, 0, false, true, "" },
    { "s_add_u32 s21, scc, s61\n", 0x80153dfdU, 0, false, true, "" },
    { "s_add_u32 s21, src_scc, s61\n", 0x80153dfdU, 0, false, true, "" },
    { "s_add_u32 ttmp0, s4, s61", 0x806c3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp1, s4, s61", 0x806d3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp2, s4, s61", 0x806e3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp3, s4, s61", 0x806f3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp4, s4, s61", 0x80703d04U, 0, false, true, "" },
    { "s_add_u32 ttmp5, s4, s61", 0x80713d04U, 0, false, true, "" },
    { "s_add_u32 ttmp14, s4, s61", 0x807a3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp15, s4, s61", 0x807b3d04U, 0, false, true, "" },
    { "s_add_u32 ttmp16, s4, s61", 0x807b3d04U, 0, false, false,
        "test.s:1:11: Error: TTMPRegister number out of range (0-15)\n" },
    { "s_add_u32 ttmp[5], s4, s61", 0x80713d04U, 0, false, true, "" },
    { "s_add_u32 ttmp[5:5], s4, s61", 0x80713d04U, 0, false, true, "" },
    { "    s_add_u32  tma_lo, s4, s61", 0x806e3d04U, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    { "    s_add_u32  tba_lo, s4, s61", 0x806e3d04U, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    /* SOP2 instructions */
    { "        s_mul_hi_u32    s21, s4, s61\n", 0x96153d04U, 0, false, true, "" },
    { "        s_mul_hi_i32    s21, s4, s61\n", 0x96953d04U, 0, false, true, "" },
    { "        s_lshl1_add_u32 s21, s4, s61\n", 0x97153d04U, 0, false, true, "" },
    { "        s_lshl2_add_u32 s21, s4, s61\n", 0x97953d04U, 0, false, true, "" },
    { "        s_lshl3_add_u32 s21, s4, s61\n", 0x98153d04U, 0, false, true, "" },
    { "        s_lshl4_add_u32 s21, s4, s61\n", 0x98953d04U, 0, false, true, "" },
    { "        s_pack_ll_b32_b16 s21, s4, s61\n", 0x99153d04U, 0, false, true, "" },
    { "        s_pack_lh_b32_b16 s21, s4, s61\n", 0x99953d04U, 0, false, true, "" },
    { "        s_pack_hh_b32_b16 s21, s4, s61\n", 0x9a153d04U, 0, false, true, "" },
    /* SOP1 instructions */
    { "s_andn1_saveexec_b64 s[86:87], s[20:21]\n", 0xbed63314U, 0, false, true, "" },
    { "s_orn1_saveexec_b64 s[86:87], s[20:21]\n", 0xbed63414U, 0, false, true, "" },
    { "s_andn1_wrexec_b64 s[86:87], s[20:21]\n", 0xbed63514U, 0, false, true, "" },
    { "s_andn2_wrexec_b64 s[86:87], s[20:21]\n", 0xbed63614U, 0, false, true, "" },
    { "s_bitreplicate_b64_b32 s[86:87], s20\n", 0xbed63714U, 0, false, true, "" },
    /* SOPK instructions */
    { "s_call_b64 s[44:45], xxxx+8\nxxxx:", 0xbaac0002U, 0, false, true, "" },
    /* SOPP instructions */
    { "        s_endpgm_ordered_ps_done\n", 0xbf9e0000U, 0, false, true, "" },
    { nullptr, 0, 0, false, false, 0 }
};
