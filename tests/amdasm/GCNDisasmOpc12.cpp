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

/* for Radeon RX3X0 series with GCN1.2 */
const GCNDisasmOpcodeCase decGCNOpcodeGCN12Cases[] =
{
    /* SOP2 encoding */
    { 0x80153d04U, 0, false, "        s_add_u32       s21, s4, s61\n" },
    /* extra registers */
    { 0x80663d04U, 0, false, "        s_add_u32       flat_scratch_lo, s4, s61\n" },
    { 0x80673d04U, 0, false, "        s_add_u32       flat_scratch_hi, s4, s61\n" },
    { 0x80683d04U, 0, false, "        s_add_u32       xnack_mask_lo, s4, s61\n" },
    { 0x80693d04U, 0, false, "        s_add_u32       xnack_mask_hi, s4, s61\n" },
    // 1/(2*pi)
    { 0x80693df8U, 0, false, "        s_add_u32       xnack_mask_hi, 0.15915494, s61\n" },
    { 0x81153d04U, 0, false, "        s_add_i32       s21, s4, s61\n" },
    { 0x81953d04U, 0, false, "        s_sub_i32       s21, s4, s61\n" },
    { 0x82153d04U, 0, false, "        s_addc_u32      s21, s4, s61\n" },
    { 0x82953d04U, 0, false, "        s_subb_u32      s21, s4, s61\n" },
    { 0x83153d04U, 0, false, "        s_min_i32       s21, s4, s61\n" },
    { 0x83953d04U, 0, false, "        s_min_u32       s21, s4, s61\n" },
    { 0x84153d04U, 0, false, "        s_max_i32       s21, s4, s61\n" },
    { 0x84953d04U, 0, false, "        s_max_u32       s21, s4, s61\n" },
    { 0x85153d04U, 0, false, "        s_cselect_b32   s21, s4, s61\n" },
    { 0x85953d04U, 0, false, "        s_cselect_b64   s[21:22], s[4:5], s[61:62]\n" },
    { 0x86153d04U, 0, false, "        s_and_b32       s21, s4, s61\n" },
    { 0x86953d04U, 0, false, "        s_and_b64       s[21:22], s[4:5], s[61:62]\n" },
    { 0x87153d04U, 0, false, "        s_or_b32        s21, s4, s61\n" },
    { 0x87953d04U, 0, false, "        s_or_b64        s[21:22], s[4:5], s[61:62]\n" },
    { 0x88153d04U, 0, false, "        s_xor_b32       s21, s4, s61\n" },
    { 0x88953d04U, 0, false, "        s_xor_b64       s[21:22], s[4:5], s[61:62]\n" },
    { 0x89153d04U, 0, false, "        s_andn2_b32     s21, s4, s61\n" },
    { 0x89953d04U, 0, false, "        s_andn2_b64     s[21:22], s[4:5], s[61:62]\n" },
    { 0x8a153d04U, 0, false, "        s_orn2_b32      s21, s4, s61\n" },
    { 0x8a953d04U, 0, false, "        s_orn2_b64      s[21:22], s[4:5], s[61:62]\n" },
    { 0x8b153d04U, 0, false, "        s_nand_b32      s21, s4, s61\n" },
    { 0x8b953d04U, 0, false, "        s_nand_b64      s[21:22], s[4:5], s[61:62]\n" },
    { 0x8c153d04U, 0, false, "        s_nor_b32       s21, s4, s61\n" },
    { 0x8c953d04U, 0, false, "        s_nor_b64       s[21:22], s[4:5], s[61:62]\n" },
    { 0x8d153d04U, 0, false, "        s_xnor_b32      s21, s4, s61\n" },
    { 0x8d953d04U, 0, false, "        s_xnor_b64      s[21:22], s[4:5], s[61:62]\n" },
    { 0x8e153d04U, 0, false, "        s_lshl_b32      s21, s4, s61\n" },
    { 0x8e953d04U, 0, false, "        s_lshl_b64      s[21:22], s[4:5], s61\n" },
    { 0x8f153d04U, 0, false, "        s_lshr_b32      s21, s4, s61\n" },
    { 0x8f953d04U, 0, false, "        s_lshr_b64      s[21:22], s[4:5], s61\n" },
    { 0x90153d04U, 0, false, "        s_ashr_i32      s21, s4, s61\n" },
    { 0x90953d04U, 0, false, "        s_ashr_i64      s[21:22], s[4:5], s61\n" },
    { 0x91153d04U, 0, false, "        s_bfm_b32       s21, s4, s61\n" },
    { 0x91953d04U, 0, false, "        s_bfm_b64       s[21:22], s4, s61\n" },
    { 0x92153d04U, 0, false, "        s_mul_i32       s21, s4, s61\n" },
    { 0x92953d04U, 0, false, "        s_bfe_u32       s21, s4, s61\n" },
    { 0x93153d04U, 0, false, "        s_bfe_i32       s21, s4, s61\n" },
    { 0x93953d04U, 0, false, "        s_bfe_u64       s[21:22], s[4:5], s61\n" },
    { 0x94153d04U, 0, false, "        s_bfe_i64       s[21:22], s[4:5], s61\n" },
    { 0x94953d04U, 0, false, "        s_cbranch_g_fork s[4:5], s[61:62] sdst=0x15\n" },
    { 0x94803d04U, 0, false, "        s_cbranch_g_fork s[4:5], s[61:62]\n" },
    { 0x95153d04U, 0, false, "        s_absdiff_i32   s21, s4, s61\n" },
    { 0x95953d04U, 0, false, "        s_rfe_restore_b64 s[4:5], s61 sdst=0x15\n" },
    { 0x95803d04U, 0, false, "        s_rfe_restore_b64 s[4:5], s61\n" },
    { 0x96153d04U, 0, false, "        SOP2_ill_44     s21, s4, s61\n" },
    { 0x96953d04U, 0, false, "        SOP2_ill_45     s21, s4, s61\n" },
    { 0x97153d04U, 0, false, "        SOP2_ill_46     s21, s4, s61\n" },
    { 0x97953d04U, 0, false, "        SOP2_ill_47     s21, s4, s61\n" },
    { 0x98153d04U, 0, false, "        SOP2_ill_48     s21, s4, s61\n" },
    /* SOPK encoding */
    { 0xb02b0000U, 0, false, "        s_movk_i32      s43, 0x0\n" },
    { 0xb02bd3b9U, 0, false, "        s_movk_i32      s43, 0xd3b9\n" },
    { 0xb0abd3b9U, 0, false, "        s_cmovk_i32     s43, 0xd3b9\n" },
    { 0xb12bd3b9U, 0, false, "        s_cmpk_eq_i32   s43, 0xd3b9\n" },
    { 0xb1abd3b9U, 0, false, "        s_cmpk_lg_i32   s43, 0xd3b9\n" },
    { 0xb22bd3b9U, 0, false, "        s_cmpk_gt_i32   s43, 0xd3b9\n" },
    { 0xb2abd3b9U, 0, false, "        s_cmpk_ge_i32   s43, 0xd3b9\n" },
    { 0xb32bd3b9U, 0, false, "        s_cmpk_lt_i32   s43, 0xd3b9\n" },
    { 0xb3abd3b9U, 0, false, "        s_cmpk_le_i32   s43, 0xd3b9\n" },
    { 0xb42bd3b9U, 0, false, "        s_cmpk_eq_u32   s43, 0xd3b9\n" },
    { 0xb4abd3b9U, 0, false, "        s_cmpk_lg_u32   s43, 0xd3b9\n" },
    { 0xb52bd3b9U, 0, false, "        s_cmpk_gt_u32   s43, 0xd3b9\n" },
    { 0xb5abd3b9U, 0, false, "        s_cmpk_ge_u32   s43, 0xd3b9\n" },
    { 0xb62bd3b9U, 0, false, "        s_cmpk_lt_u32   s43, 0xd3b9\n" },
    { 0xb6abd3b9U, 0, false, "        s_cmpk_le_u32   s43, 0xd3b9\n" },
    { 0xb72bd3b9U, 0, false, "        s_addk_i32      s43, 0xd3b9\n" },
    { 0xb7abd3b9U, 0, false, "        s_mulk_i32      s43, 0xd3b9\n" },
    { 0xb82b0000U, 0, false, "        s_cbranch_i_fork s[43:44], .L4_0\n" },
    { 0xb82bffffU, 0, false, "        s_cbranch_i_fork s[43:44], .L0_0\n" },
    { 0xb82b000aU, 0, false, "        s_cbranch_i_fork s[43:44], .L44_0\n" },
    { 0xb8ab0000u, 0, false, "        s_getreg_b32    s43, hwreg(@0, 0, 1)\n" },
    { 0xb8ab0001u, 0, false, "        s_getreg_b32    s43, hwreg(mode, 0, 1)\n" },
    { 0xb8ab0002u, 0, false, "        s_getreg_b32    s43, hwreg(status, 0, 1)\n" },
    { 0xb8ab0003u, 0, false, "        s_getreg_b32    s43, hwreg(trapsts, 0, 1)\n" },
    { 0xb8ab07c3u, 0, false, "        s_getreg_b32    s43, hwreg(trapsts, 31, 1)\n" },
    { 0xb8ab28c3u, 0, false, "        s_getreg_b32    s43, hwreg(trapsts, 3, 6)\n" },
    { 0xb8ab48c3u, 0, false, "        s_getreg_b32    s43, hwreg(trapsts, 3, 10)\n" },
    { 0xb92b48c3u, 0, false, "        s_setreg_b32    hwreg(trapsts, 3, 10), s43\n" },
    { 0xb9abf8c3u, 0, false, "        s_getreg_regrd_b32 s43, hwreg(trapsts, 3, 32)\n" },
    { 0xba2b48c3u, 0x45d2a, true, "        s_setreg_imm32_b32 hwreg(trapsts, 3, 10), "
                "0x45d2a sdst=0x2b\n" },
    { 0xba0048c3u, 0x45d2a, true, "        s_setreg_imm32_b32 hwreg(trapsts, 3, 10), "
                "0x45d2a\n" },
    { 0xb8ab000du, 0, false, "        s_getreg_b32    s43, hwreg(ib_dbg1, 0, 1)\n" },
    { 0xbaabd3b9u, 0, false, "        SOPK_ill_21     s43, 0xd3b9\n" },
    { 0xbb2bd3b9U, 0, false, "        SOPK_ill_22     s43, 0xd3b9\n" },
    { 0xbbabd3b9U, 0, false, "        SOPK_ill_23     s43, 0xd3b9\n" },
    { 0xbc2bd3b9U, 0, false, "        SOPK_ill_24     s43, 0xd3b9\n" },
    { 0xbcabd3b9U, 0, false, "        SOPK_ill_25     s43, 0xd3b9\n" },
    { 0xbd2bd3b9U, 0, false, "        SOPK_ill_26     s43, 0xd3b9\n" },
    { 0xbdabd3b9U, 0, false, "        SOPK_ill_27     s43, 0xd3b9\n" },
    /* SOP1 encoding */
    { 0xbed60014U, 0, false, "        s_mov_b32       s86, s20\n" },
    { 0xbed600ffU, 0xddbbaa11, true, "        s_mov_b32       s86, 0xddbbaa11\n" },
    { 0xbed60114U, 0, false, "        s_mov_b64       s[86:87], s[20:21]\n" },
    { 0xbed60214U, 0, false, "        s_cmov_b32      s86, s20\n" },
    { 0xbed60314U, 0, false, "        s_cmov_b64      s[86:87], s[20:21]\n" },
    { 0xbed60414U, 0, false, "        s_not_b32       s86, s20\n" },
    { 0xbed60514U, 0, false, "        s_not_b64       s[86:87], s[20:21]\n" },
    { 0xbed60614U, 0, false, "        s_wqm_b32       s86, s20\n" },
    { 0xbed60714U, 0, false, "        s_wqm_b64       s[86:87], s[20:21]\n" },
    { 0xbed60814U, 0, false, "        s_brev_b32      s86, s20\n" },
    { 0xbed60914U, 0, false, "        s_brev_b64      s[86:87], s[20:21]\n" },
    { 0xbed60a14U, 0, false, "        s_bcnt0_i32_b32 s86, s20\n" },
    { 0xbed60b14U, 0, false, "        s_bcnt0_i32_b64 s86, s[20:21]\n" },
    { 0xbed60c14U, 0, false, "        s_bcnt1_i32_b32 s86, s20\n" },
    { 0xbed60d14U, 0, false, "        s_bcnt1_i32_b64 s86, s[20:21]\n" },
    { 0xbed60e14U, 0, false, "        s_ff0_i32_b32   s86, s20\n" },
    { 0xbed60f14U, 0, false, "        s_ff0_i32_b64   s86, s[20:21]\n" },
    { 0xbed61014U, 0, false, "        s_ff1_i32_b32   s86, s20\n" },
    { 0xbed61114U, 0, false, "        s_ff1_i32_b64   s86, s[20:21]\n" },
    { 0xbed61214U, 0, false, "        s_flbit_i32_b32 s86, s20\n" },
    { 0xbed61314U, 0, false, "        s_flbit_i32_b64 s86, s[20:21]\n" },
    { 0xbed61414U, 0, false, "        s_flbit_i32     s86, s20\n" },
    { 0xbed61514U, 0, false, "        s_flbit_i32_i64 s86, s[20:21]\n" },
    { 0xbed61614U, 0, false, "        s_sext_i32_i8   s86, s20\n" },
    { 0xbed61714U, 0, false, "        s_sext_i32_i16  s86, s20\n" },
    { 0xbed61814U, 0, false, "        s_bitset0_b32   s86, s20\n" },
    { 0xbed61914U, 0, false, "        s_bitset0_b64   s[86:87], s20\n" },
    { 0xbed61a14U, 0, false, "        s_bitset1_b32   s86, s20\n" },
    { 0xbed61b14U, 0, false, "        s_bitset1_b64   s[86:87], s20\n" },
    { 0xbed61c00U, 0, false, "        s_getpc_b64     s[86:87]\n" },
    { 0xbed61c14U, 0, false, "        s_getpc_b64     s[86:87] ssrc=0x14\n" },
    { 0xbe801d14U, 0, false, "        s_setpc_b64     s[20:21]\n" },
    { 0xbed61d14U, 0, false, "        s_setpc_b64     s[20:21] sdst=0x56\n" },
    { 0xbed61e14U, 0, false, "        s_swappc_b64    s[86:87], s[20:21]\n" },
    { 0xbe801f14U, 0, false, "        s_rfe_b64       s[20:21]\n" },
    { 0xbed61f14U, 0, false, "        s_rfe_b64       s[20:21] sdst=0x56\n" },
    { 0xbed62014U, 0, false, "        s_and_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62114U, 0, false, "        s_or_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62214U, 0, false, "        s_xor_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62314U, 0, false, "        s_andn2_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62414U, 0, false, "        s_orn2_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62514U, 0, false, "        s_nand_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62614U, 0, false, "        s_nor_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62714U, 0, false, "        s_xnor_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62814U, 0, false, "        s_quadmask_b32  s86, s20\n" },
    { 0xbed62914U, 0, false, "        s_quadmask_b64  s[86:87], s[20:21]\n" },
    { 0xbed62a14U, 0, false, "        s_movrels_b32   s86, s20\n" },
    { 0xbed62b14U, 0, false, "        s_movrels_b64   s[86:87], s[20:21]\n" },
    { 0xbed62c14U, 0, false, "        s_movreld_b32   s86, s20\n" },
    { 0xbed62d14U, 0, false, "        s_movreld_b64   s[86:87], s[20:21]\n" },
    { 0xbe802e14U, 0, false, "        s_cbranch_join  s20\n" },
    { 0xbed62e14U, 0, false, "        s_cbranch_join  s20 sdst=0x56\n" },
    { 0xbed62f14U, 0, false, "        s_mov_regrd_b32 s86, s20\n" },
    { 0xbed63014U, 0, false, "        s_abs_i32       s86, s20\n" },
    { 0xbed63114U, 0, false, "        s_mov_fed_b32   s86, s20\n" },
    { 0xbed63214U, 0, false, "        s_set_gpr_idx_idx s20 sdst=0x56\n" },
    { 0xbe803214U, 0, false, "        s_set_gpr_idx_idx s20\n" },
    { 0xbed63314U, 0, false, "        SOP1_ill_51     s86, s20\n" },
    { 0xbed63414U, 0, false, "        SOP1_ill_52     s86, s20\n" },
    { 0xbed63514U, 0, false, "        SOP1_ill_53     s86, s20\n" },
    { 0xbed63614U, 0, false, "        SOP1_ill_54     s86, s20\n" },
    { 0xbed63714U, 0, false, "        SOP1_ill_55     s86, s20\n" },
    /* SOPC encoding */
    { 0xbf00451dU, 0, false, "        s_cmp_eq_i32    s29, s69\n" },
    { 0xbf0045ffU, 0x6d894, true, "        s_cmp_eq_i32    0x6d894, s69\n" },
    { 0xbf00ff1dU, 0x6d894, true, "        s_cmp_eq_i32    s29, 0x6d894\n" },
    { 0xbf01451dU, 0, false, "        s_cmp_lg_i32    s29, s69\n" },
    { 0xbf02451dU, 0, false, "        s_cmp_gt_i32    s29, s69\n" },
    { 0xbf03451dU, 0, false, "        s_cmp_ge_i32    s29, s69\n" },
    { 0xbf04451dU, 0, false, "        s_cmp_lt_i32    s29, s69\n" },
    { 0xbf05451dU, 0, false, "        s_cmp_le_i32    s29, s69\n" },
    { 0xbf06451dU, 0, false, "        s_cmp_eq_u32    s29, s69\n" },
    { 0xbf07451dU, 0, false, "        s_cmp_lg_u32    s29, s69\n" },
    { 0xbf08451dU, 0, false, "        s_cmp_gt_u32    s29, s69\n" },
    { 0xbf09451dU, 0, false, "        s_cmp_ge_u32    s29, s69\n" },
    { 0xbf0a451dU, 0, false, "        s_cmp_lt_u32    s29, s69\n" },
    { 0xbf0b451dU, 0, false, "        s_cmp_le_u32    s29, s69\n" },
    { 0xbf0c451dU, 0, false, "        s_bitcmp0_b32   s29, s69\n" },
    { 0xbf0d451dU, 0, false, "        s_bitcmp1_b32   s29, s69\n" },
    { 0xbf0e451dU, 0, false, "        s_bitcmp0_b64   s[29:30], s69\n" },
    { 0xbf0f451dU, 0, false, "        s_bitcmp1_b64   s[29:30], s69\n" },
    { 0xbf10451dU, 0, false, "        s_setvskip      s29, s69\n" },
    { 0xbf11001dU, 0, false, "        s_set_gpr_idx_on s29, 0x0\n" },
    { 0xbf11021dU, 0, false, "        s_set_gpr_idx_on s29, 0x2\n" },
    { 0xbf110a1dU, 0, false, "        s_set_gpr_idx_on s29, 0xa\n" },
    { 0xbf110d1dU, 0, false, "        s_set_gpr_idx_on s29, 0xd\n" },
    { 0xbf11451dU, 0, false, "        s_set_gpr_idx_on s29, 0x45\n" },
    { 0xbf11a51dU, 0, false, "        s_set_gpr_idx_on s29, 0xa5\n" },
    { 0xbf11be1dU, 0, false, "        s_set_gpr_idx_on s29, 0xbe\n" },
    { 0xbf11f51dU, 0, false, "        s_set_gpr_idx_on s29, 0xf5\n" },
    { 0xbf12451dU, 0, false, "        s_cmp_eq_u64    s[29:30], s[69:70]\n" },
    { 0xbf13451dU, 0, false, "        s_cmp_lg_u64    s[29:30], s[69:70]\n" },
    { 0xbf14451dU, 0, false, "        SOPC_ill_20     s29, s69\n" },
    { 0xbf15451dU, 0, false, "        SOPC_ill_21     s29, s69\n" },
    { 0xbf16451dU, 0, false, "        SOPC_ill_22     s29, s69\n" },
    { 0xbf17451dU, 0, false, "        SOPC_ill_23     s29, s69\n" },
    /* SOPP encoding */
    { 0xbf800000U, 0, false, "        s_nop           0x0\n" },
    { 0xbf800006U, 0, false, "        s_nop           0x6\n" },
    { 0xbf80cd26U, 0, false, "        s_nop           0xcd26\n" },
    { 0xbf810000U, 0, false, "        s_endpgm\n" },
    { 0xbf818d33U, 0, false, "        s_endpgm        0x8d33\n" },
    { 0xbf820029U, 0, false, "        s_branch        .L168_0\n" },
    { 0xbf82ffffU, 0, false, "        s_branch        .L0_0\n" },
    { 0xbf830000U, 0, false, "        s_wakeup\n" },
    { 0xbf838d33U, 0, false, "        s_wakeup        0x8d33\n" },
    { 0xbf840029U, 0, false, "        s_cbranch_scc0  .L168_0\n" },
    { 0xbf850029U, 0, false, "        s_cbranch_scc1  .L168_0\n" },
    { 0xbf860029U, 0, false, "        s_cbranch_vccz  .L168_0\n" },
    { 0xbf870029U, 0, false, "        s_cbranch_vccnz .L168_0\n" },
    { 0xbf880029U, 0, false, "        s_cbranch_execz .L168_0\n" },
    { 0xbf890029U, 0, false, "        s_cbranch_execnz .L168_0\n" },
    { 0xbf8a0000U, 0, false, "        s_barrier\n" },
    { 0xbf8b032bU, 0, false, "        s_setkill       0x32b\n" },
    { 0xbf8c0d36U, 0, false, "        s_waitcnt       "
        "vmcnt(6) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0d3fU, 0, false, "        s_waitcnt       expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0d3eU, 0, false, "        s_waitcnt       "
        "vmcnt(14) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0d7eU, 0, false, "        s_waitcnt       vmcnt(14) & lgkmcnt(13)\n" },
    { 0xbf8c0f7eU, 0, false, "        s_waitcnt       vmcnt(14)\n" },
    { 0xbf8c0f7fU, 0, false, "        s_waitcnt       "
        "vmcnt(15) & expcnt(7) & lgkmcnt(15)\n" },
    { 0xbf8cad36U, 0, false, "        s_waitcnt       "
        "vmcnt(6) & expcnt(3) & lgkmcnt(13) :0xad36\n" },
    { 0xbf8c0536U, 0, false, "        s_waitcnt       "
        "vmcnt(6) & expcnt(3) & lgkmcnt(5)\n" },
    { 0xbf8c0000U, 0, false, "        s_waitcnt       "
        "vmcnt(0) & expcnt(0) & lgkmcnt(0)\n" },
    { 0xbf8c0080U, 0, false, "        s_waitcnt       "
        "vmcnt(0) & expcnt(0) & lgkmcnt(0) :0x80\n" },
    { 0xbf8d032bU, 0, false, "        s_sethalt       0x32b\n" },
    { 0xbf8e032bU, 0, false, "        s_sleep         0x32b\n" },
    { 0xbf8f032bU, 0, false, "        s_setprio       0x32b\n" },
    { 0xbf90001bU, 0, false, "        s_sendmsg       sendmsg(@11, cut, 0)\n" },
    { 0xbf900014U, 0, false, "        s_sendmsg       sendmsg(savewave, cut, 0)\n" },
    { 0xbf900004U, 0, false, "        s_sendmsg       sendmsg(savewave)\n" },
    { 0xbf91001bU, 0, false, "        s_sendmsghalt   sendmsg(@11, cut, 0)\n" },
    { 0xbf92032bU, 0, false, "        s_trap          0x32b\n" },
    { 0xbf93032bU, 0, false, "        s_icache_inv    0x32b\n" },
    { 0xbf930000U, 0, false, "        s_icache_inv\n" },
    { 0xbf941234U, 0, false, "        s_incperflevel  0x1234\n" },
    { 0xbf951234U, 0, false, "        s_decperflevel  0x1234\n" },
    { 0xbf960000U, 0, false, "        s_ttracedata\n" },
    { 0xbf960dcaU, 0, false, "        s_ttracedata    0xdca\n" },
    { 0xbf970029U, 0, false, "        s_cbranch_cdbgsys .L168_0\n" },
    { 0xbf980029U, 0, false, "        s_cbranch_cdbguser .L168_0\n" },
    { 0xbf990029U, 0, false, "        s_cbranch_cdbgsys_or_user .L168_0\n" },
    { 0xbf9a0029U, 0, false, "        s_cbranch_cdbgsys_and_user .L168_0\n" },
    { 0xbf9b0000U, 0, false, "        s_endpgm_saved\n" },
    { 0xbf9b8d33U, 0, false, "        s_endpgm_saved  0x8d33\n" },
    { 0xbf9c0000U, 0, false, "        s_set_gpr_idx_off\n" },
    { 0xbf9c8d33U, 0, false, "        s_set_gpr_idx_off 0x8d33\n" },
    { 0xbf9d8d33U, 0, false, "        s_set_gpr_idx_mode 0x8d33\n" },
    { 0xbf9d0000U, 0, false, "        s_set_gpr_idx_mode 0x0\n" },
    { 0xbf9e0020U, 0, false, "        SOPP_ill_30     0x20\n" },
    { 0xbf9f0020U, 0, false, "        SOPP_ill_31     0x20\n" },
    { 0xbfa00020U, 0, false, "        SOPP_ill_32     0x20\n" },
    /* SMEM encoding */
    { 0xc0020c9dU, 0x5b, true, "        s_load_dword    s50, s[58:59], 0x5b\n" },
    { 0xc0020c9dU, 0x1345b, true, "        s_load_dword    s50, s[58:59], 0x1345b\n" },
    { 0xc0020c9dU, 0x1d1345b, true, "        s_load_dword    s50, s[58:59], 0x1345b\n" },
    { 0xc0000c9dU, 0x5b, true, "        s_load_dword    s50, s[58:59], s91\n" },
    { 0xc0000cddU, 0x5b, true, "        s_load_dword    s51, s[58:59], s91\n" },
    { 0xc0010cddU, 0x5b, true, "        s_load_dword    s51, s[58:59], s91 glc\n" },
    { 0xc0030c9dU, 0x5b, true, "        s_load_dword    s50, s[58:59], 0x5b glc\n" },
    { 0xc0060c9dU, 0x5b, true, "        s_load_dwordx2  s[50:51], s[58:59], 0x5b\n" },
    { 0xc00a0c9dU, 0x5b, true, "        s_load_dwordx4  s[50:53], s[58:59], 0x5b\n" },
    { 0xc00e0c9dU, 0x5b, true, "        s_load_dwordx8  s[50:57], s[58:59], 0x5b\n" },
    { 0xc0120c9dU, 0x5b, true, "        s_load_dwordx16 s[50:65], s[58:59], 0x5b\n" },
    { 0xc0160c9dU, 0x5b, true, "        SMEM_ill_5      s50, s[58:59], 0x5b\n" },
    { 0xc01a0c9dU, 0x5b, true, "        SMEM_ill_6      s50, s[58:59], 0x5b\n" },
    { 0xc01e0c9dU, 0x5b, true, "        SMEM_ill_7      s50, s[58:59], 0x5b\n" },
    { 0xc0220c9dU, 0x5b, true, "        s_buffer_load_dword s50, s[58:61], 0x5b\n" },
    { 0xc0260c9dU, 0x5b, true,
        "        s_buffer_load_dwordx2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc02a0c9dU, 0x5b, true,
        "        s_buffer_load_dwordx4 s[50:53], s[58:61], 0x5b\n" },
    { 0xc02e0c9dU, 0x5b, true,
        "        s_buffer_load_dwordx8 s[50:57], s[58:61], 0x5b\n" },
    { 0xc0320c9dU, 0x5b, true,
        "        s_buffer_load_dwordx16 s[50:65], s[58:61], 0x5b\n" },
    { 0xc0360c9dU, 0x5b, true, "        SMEM_ill_13     s50, s[58:59], 0x5b\n" },
    { 0xc03a0c9dU, 0x5b, true, "        SMEM_ill_14     s50, s[58:59], 0x5b\n" },
    { 0xc03e0c9dU, 0x5b, true, "        SMEM_ill_15     s50, s[58:59], 0x5b\n" },
    { 0xc0420c9dU, 0x5b, true, "        s_store_dword   s50, s[58:59], 0x5b\n" },
    { 0xc0460c9dU, 0x5b, true, "        s_store_dwordx2 s[50:51], s[58:59], 0x5b\n" },
    { 0xc04a0c9dU, 0x5b, true, "        s_store_dwordx4 s[50:53], s[58:59], 0x5b\n" },
    { 0xc04e0c9dU, 0x5b, true, "        SMEM_ill_19     s50, s[58:59], 0x5b\n" },
    { 0xc0520c9dU, 0x5b, true, "        SMEM_ill_20     s50, s[58:59], 0x5b\n" },
    { 0xc0560c9dU, 0x5b, true, "        SMEM_ill_21     s50, s[58:59], 0x5b\n" },
    { 0xc05a0c9dU, 0x5b, true, "        SMEM_ill_22     s50, s[58:59], 0x5b\n" },
    { 0xc05e0c9dU, 0x5b, true, "        SMEM_ill_23     s50, s[58:59], 0x5b\n" },
    { 0xc0620c9dU, 0x5b, true, "        s_buffer_store_dword s50, s[58:61], 0x5b\n" },
    { 0xc0660c9dU, 0x5b, true,
        "        s_buffer_store_dwordx2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc06a0c9dU, 0x5b, true,
        "        s_buffer_store_dwordx4 s[50:53], s[58:61], 0x5b\n" },
    { 0xc06e0c9dU, 0x5b, true, "        SMEM_ill_27     s50, s[58:59], 0x5b\n" },
    { 0xc0720c9dU, 0x5b, true, "        SMEM_ill_28     s50, s[58:59], 0x5b\n" },
    { 0xc0760c9dU, 0x5b, true, "        SMEM_ill_29     s50, s[58:59], 0x5b\n" },
    { 0xc07a0c9dU, 0x5b, true, "        SMEM_ill_30     s50, s[58:59], 0x5b\n" },
    { 0xc07e0c9dU, 0x5b, true, "        SMEM_ill_31     s50, s[58:59], 0x5b\n" },
    { 0xc0800000U, 0, true, "        s_dcache_inv\n" },
    { 0xc0810000U, 0, true, "        s_dcache_inv    glc\n" },
    { 0xc0820000U, 0, true, "        s_dcache_inv    imm=1\n" },
    { 0xc0800cb8U, 0x23, true,
        "        s_dcache_inv    sdata=0x32 sbase=0x38 offset=0x23\n" },
    { 0xc0801ff8U, 0x23, true,
        "        s_dcache_inv    sdata=0x7f sbase=0x38 offset=0x23\n" },
    { 0xc0811ff8U, 0x23, true,
        "        s_dcache_inv    glc sdata=0x7f sbase=0x38 offset=0x23\n" },
    { 0xc0820cb8U, 0, true, "        s_dcache_inv    sdata=0x32 sbase=0x38 imm=1\n" },
    { 0xc0800038U, 0, true, "        s_dcache_inv    sbase=0x38\n" },
    { 0xc0810038U, 0, true, "        s_dcache_inv    glc sbase=0x38\n" },
    { 0xc08000c0U, 0, true, "        s_dcache_inv    sdata=0x3\n" },
    { 0xc0840000U, 0, true, "        s_dcache_wb\n" },
    { 0xc0880000U, 0, true, "        s_dcache_inv_vol\n" },
    { 0xc08c0000U, 0, true, "        s_dcache_wb_vol\n" },
    { 0xc0900cc0U, 0, true, "        s_memtime       s[51:52]\n" },
    { 0xc0910cc0U, 0, true, "        s_memtime       s[51:52] glc\n" },
    { 0xc0900cc0U, 0x44, true, "        s_memtime       s[51:52] offset=0x44\n" },
    { 0xc0910cc0U, 0x44, true, "        s_memtime       s[51:52] glc offset=0x44\n" },
    { 0xc0920cc0U, 0x45, true, "        s_memtime       s[51:52] offset=0x45 imm=1\n" },
    { 0xc0930cc0U, 0x45, true,
        "        s_memtime       s[51:52] glc offset=0x45 imm=1\n" },
    { 0xc0940cc0U, 0, true, "        s_memrealtime   s[51:52]\n" },
    { 0xc09a0c9dU, 0xfff5b, true, "        s_atc_probe     0x32, s[58:59], 0xfff5b\n" },
    { 0xc09e0c9dU, 0xfff5b, true, "        s_atc_probe_buffer 0x32, s[58:61], 0xfff5b\n" },
    { 0xc0a20c9dU, 0x5b, true, "        SMEM_ill_40     s50, s[58:59], 0x5b\n" },
    { 0xc0a60c9dU, 0x5b, true, "        SMEM_ill_41     s50, s[58:59], 0x5b\n" },
    { 0xc0aa0c9dU, 0x5b, true, "        SMEM_ill_42     s50, s[58:59], 0x5b\n" },
    { 0xc0ae0c9dU, 0x5b, true, "        SMEM_ill_43     s50, s[58:59], 0x5b\n" },
    { 0xc1020c9dU, 0x5b, true, "        s_buffer_atomic_swap s50, s[58:61], 0x5b\n" },
    { 0xc1060c9dU, 0x5b, true,
        "        s_buffer_atomic_cmpswap s[50:51], s[58:61], 0x5b\n" },
    { 0xc10a0c9dU, 0x5b, true, "        s_buffer_atomic_add s50, s[58:61], 0x5b\n" },
    { 0xc10e0c9dU, 0x5b, true, "        s_buffer_atomic_sub s50, s[58:61], 0x5b\n" },
    { 0xc1120c9dU, 0x5b, true, "        s_buffer_atomic_smin s50, s[58:61], 0x5b\n" },
    { 0xc1160c9dU, 0x5b, true, "        s_buffer_atomic_umin s50, s[58:61], 0x5b\n" },
    { 0xc11a0c9dU, 0x5b, true, "        s_buffer_atomic_smax s50, s[58:61], 0x5b\n" },
    { 0xc11e0c9dU, 0x5b, true, "        s_buffer_atomic_umax s50, s[58:61], 0x5b\n" },
    { 0xc1220c9dU, 0x5b, true, "        s_buffer_atomic_and s50, s[58:61], 0x5b\n" },
    { 0xc1260c9dU, 0x5b, true, "        s_buffer_atomic_or s50, s[58:61], 0x5b\n" },
    { 0xc12a0c9dU, 0x5b, true, "        s_buffer_atomic_xor s50, s[58:61], 0x5b\n" },
    { 0xc12e0c9dU, 0x5b, true, "        s_buffer_atomic_inc s50, s[58:61], 0x5b\n" },
    { 0xc1320c9dU, 0x5b, true, "        s_buffer_atomic_dec s50, s[58:61], 0x5b\n" },
    { 0xc1820c9dU, 0x5b, true,
        "        s_buffer_atomic_swap_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1860d1dU, 0x5b, true,
        "        s_buffer_atomic_cmpswap_x2 s[52:55], s[58:61], 0x5b\n" },
    { 0xc18a0c9dU, 0x5b, true,
        "        s_buffer_atomic_add_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc18e0c9dU, 0x5b, true,
        "        s_buffer_atomic_sub_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1920c9dU, 0x5b, true,
        "        s_buffer_atomic_smin_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1960c9dU, 0x5b, true,
        "        s_buffer_atomic_umin_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc19a0c9dU, 0x5b, true,
        "        s_buffer_atomic_smax_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc19e0c9dU, 0x5b, true,
        "        s_buffer_atomic_umax_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1a20c9dU, 0x5b, true,
        "        s_buffer_atomic_and_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1a60c9dU, 0x5b, true,
        "        s_buffer_atomic_or_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1aa0c9dU, 0x5b, true,
        "        s_buffer_atomic_xor_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1ae0c9dU, 0x5b, true,
        "        s_buffer_atomic_inc_x2 s[50:51], s[58:61], 0x5b\n" },
    { 0xc1b20c9dU, 0x5b, true,
        "        s_buffer_atomic_dec_x2 s[50:51], s[58:61], 0x5b\n" },
    /* VOP2 encoding */
    { 0x0134d715U, 0, false, "        v_cndmask_b32   v154, v21, v107, vcc\n" },
    { 0x0134d6ffU, 0x445aa, true , "        v_cndmask_b32   v154, 0x445aa, v107, vcc\n" },
    /* VOP_SDWA */
    { 0x0134d6f9U, 0, true, "        v_cndmask_b32   v154, v0, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x3d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x13d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte1 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x23d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte2 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x33d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte3 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x43d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:word0 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x53d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:word1 src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x63d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x73d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:invalid src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x93d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte1 dst_unused:sext src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x113d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte1 dst_unused:preserve src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x193d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte1 dst_unused:invalid src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x393d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "clamp dst_sel:byte1 dst_unused:invalid src0_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x1003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte1 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x2003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte2 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x3003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte3 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x4003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:word0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x5003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:word1 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x6003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x7003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:invalid src1_sel:byte0\n" },
    { 0x0134d6f9U, 0xd003d, true, "        v_cndmask_b32   v154, sext(v61), v107, vcc "
        "dst_sel:byte0 src0_sel:word1 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x1d003d, true, "        v_cndmask_b32   v154, sext(-v61), v107, vcc "
        "dst_sel:byte0 src0_sel:word1 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x3d003d, true, "        v_cndmask_b32   v154, sext(-abs(v61)), "
        "v107, vcc dst_sel:byte0 src0_sel:word1 src1_sel:byte0\n" },
    { 0x0134d6f9U, 0x100003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte1\n" },
    { 0x0134d6f9U, 0x200003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte2\n" },
    { 0x0134d6f9U, 0x300003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:byte3\n" },
    { 0x0134d6f9U, 0x400003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:word0\n" },
    { 0x0134d6f9U, 0x500003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:word1\n" },
    { 0x0134d6f9U, 0x600003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0\n" },
    { 0x0134d6f9U, 0x700003d, true, "        v_cndmask_b32   v154, v61, v107, vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:invalid\n" },
    { 0x0134d6f9U, 0xf00003d, true, "        v_cndmask_b32   v154, v61, sext(v107), vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:invalid\n" },
    { 0x0134d6f9U, 0x1f00003d, true, "        v_cndmask_b32   v154, v61, sext(-v107), vcc "
        "dst_sel:byte0 src0_sel:byte0 src1_sel:invalid\n" },
    { 0x0134d6f9U, 0x3f00003d, true, "        v_cndmask_b32   v154, v61, "
        "sext(-abs(v107)), vcc dst_sel:byte0 src0_sel:byte0 src1_sel:invalid\n" },
    // SDWA word at end
    { 0x0134d6f9U, 0x06060600, true, "        v_cndmask_b32   v154, v0, v107, vcc sdwa\n" },
    { 0x0134d6f9U, 0x06160600, true,
        "        v_cndmask_b32   v154, -v0, v107, vcc sdwa\n" },
    { 0x0134d6f9U, 0x16160600, true,
        "        v_cndmask_b32   v154, -v0, -v107, vcc sdwa\n" },
    { 0x0134d6f9U, 0x26260600, true,
        "        v_cndmask_b32   v154, abs(v0), abs(v107), vcc sdwa\n" },
    { 0x0134d6f9U, 0x0e0e0600, true,
        "        v_cndmask_b32   v154, sext(v0), sext(v107), vcc\n" },
    /* VOP_DPP */
    { 0x0134d6faU, 0xbe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[0,0,0,0] bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x72be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[2,0,3,1] bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0xb4be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[0,1,3,2] bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x100be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x101be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:1 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x102be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:2 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x105be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:5 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x106be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:6 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x10abe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:10 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x10dbe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:13 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x10fbe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shl:15 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x110be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "dppctrl:0x110 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x11abe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_shr:10 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x120be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "dppctrl:0x120 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x12abe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_ror:10 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x12fbe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_ror:15 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x130be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "wave_shl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x131be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "dppctrl:0x131 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x134be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "wave_rol bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x136be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "dppctrl:0x136 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x138be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "wave_shr bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x13ebe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "dppctrl:0x13e bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x13cbe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "wave_ror bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x140be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_mirror bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x141be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_half_mirror bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x142be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_bcast15 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x143be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "row_bcast31 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x14dbe, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "dppctrl:0x14d bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x193be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "dppctrl:0x193 bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x872be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x872be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x1872be, true, "        v_cndmask_b32   v154, -v190, v107, vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x2872be, true, "        v_cndmask_b32   v154, abs(v190), v107, vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x3872be, true, "        v_cndmask_b32   v154, -abs(v190), v107, vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x4872be, true, "        v_cndmask_b32   v154, v190, -v107, vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x8872be, true, "        v_cndmask_b32   v154, v190, abs(v107), vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0xc872be, true, "        v_cndmask_b32   v154, v190, -abs(v107), vcc "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x0134d6faU, 0x10072be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[2,0,3,1] bank_mask:1 row_mask:0\n" },
    { 0x0134d6faU, 0xe0072be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[2,0,3,1] bank_mask:14 row_mask:0\n" },
    { 0x0134d6faU, 0x100072be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[2,0,3,1] bank_mask:0 row_mask:1\n" },
    { 0x0134d6faU, 0xd00072be, true, "        v_cndmask_b32   v154, v190, v107, vcc "
        "quad_perm:[2,0,3,1] bank_mask:0 row_mask:13\n" },
    /* VOP2 instructions */
    { 0x0334d715U, 0, false, "        v_add_f32       v154, v21, v107\n" },
    { 0x0334d6ffU, 0x40000000U, true, "        v_add_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0534d715U, 0, false, "        v_sub_f32       v154, v21, v107\n" },
    { 0x0534d6ffU, 0x40000000U, true, "        v_sub_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0734d715U, 0, false, "        v_subrev_f32    v154, v21, v107\n" },
    { 0x0734d6ffU, 0x40000000U, true, "        v_subrev_f32    v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0934d715U, 0, false, "        v_mul_legacy_f32 v154, v21, v107\n" },
    { 0x0934d6ffU, 0x40000000U, true, "        v_mul_legacy_f32 v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0b34d715U, 0, false, "        v_mul_f32       v154, v21, v107\n" },
    { 0x0b34d6ffU, 0x40000000U, true, "        v_mul_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0d34d715U, 0, false, "        v_mul_i32_i24   v154, v21, v107\n" },
    { 0x0f34d715U, 0, false, "        v_mul_hi_i32_i24 v154, v21, v107\n" },
    { 0x1134d715U, 0, false, "        v_mul_u32_u24   v154, v21, v107\n" },
    { 0x1334d715U, 0, false, "        v_mul_hi_u32_u24 v154, v21, v107\n" },
    { 0x1534d715U, 0, false, "        v_min_f32       v154, v21, v107\n" },
    { 0x1534d6ffU, 0x40000000U, true, "        v_min_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x1734d715U, 0, false, "        v_max_f32       v154, v21, v107\n" },
    { 0x1734d6ffU, 0x40000000U, true, "        v_max_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x1934d715U, 0, false, "        v_min_i32       v154, v21, v107\n" },
    { 0x1b34d715U, 0, false, "        v_max_i32       v154, v21, v107\n" },
    { 0x1d34d715U, 0, false, "        v_min_u32       v154, v21, v107\n" },
    { 0x1f34d715U, 0, false, "        v_max_u32       v154, v21, v107\n" },
    { 0x2134d715U, 0, false, "        v_lshrrev_b32   v154, v21, v107\n" },
    { 0x2334d715U, 0, false, "        v_ashrrev_i32   v154, v21, v107\n" },
    { 0x2534d715U, 0, false, "        v_lshlrev_b32   v154, v21, v107\n" },
    { 0x2734d715U, 0, false, "        v_and_b32       v154, v21, v107\n" },
    { 0x2934d715U, 0, false, "        v_or_b32        v154, v21, v107\n" },
    { 0x2b34d715U, 0, false, "        v_xor_b32       v154, v21, v107\n" },
    { 0x2d34d715U, 0, false, "        v_mac_f32       v154, v21, v107\n" },
    { 0x2d34d6ffU, 0x40000000U, true, "        v_mac_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x2f34d715U, 0x567d0700U, true, "        v_madmk_f32     "
            "v154, v21, 0x567d0700 /* 6.9551627e+13f */, v107\n" }, /* check floatLits */
    { 0x2f34d6ffU, 0x567d0700U, true, "        v_madmk_f32     "
            "v154, 0x567d0700 /* 6.9551627e+13f */, "
            "0x567d0700 /* 6.9551627e+13f */, v107\n" }, /* check floatLits */
    { 0x3134d715U, 0x567d0700U, true, "        v_madak_f32     "
            "v154, v21, v107, 0x567d0700 /* 6.9551627e+13f */\n" },  /* check floatLits */
    { 0x3134d6ffU, 0x567d0700U, true, "        v_madak_f32     "
            "v154, 0x567d0700 /* 6.9551627e+13f */, "
            "v107, 0x567d0700 /* 6.9551627e+13f */\n" },  /* check floatLits */
    { 0x3334d715U, 0, false, "        v_add_u32       v154, vcc, v21, v107\n" },
    { 0x3534d715U, 0, false, "        v_sub_u32       v154, vcc, v21, v107\n" },
    { 0x3734d715U, 0, false, "        v_subrev_u32    v154, vcc, v21, v107\n" },
    { 0x3934d715U, 0, false, "        v_addc_u32      v154, vcc, v21, v107, vcc\n" },
    { 0x3b34d715U, 0, false, "        v_subb_u32      v154, vcc, v21, v107, vcc\n" },
    { 0x3d34d715U, 0, false, "        v_subbrev_u32   v154, vcc, v21, v107, vcc\n" },
    { 0x3f34d715U, 0, false, "        v_add_f16       v154, v21, v107\n" },
    { 0x3f34d6ffU, 0x3d4c, true,
        "        v_add_f16       v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x4134d715U, 0, false, "        v_sub_f16       v154, v21, v107\n" },
    { 0x4134d6ffU, 0x3d4c, true,
        "        v_sub_f16       v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x4334d715U, 0, false, "        v_subrev_f16    v154, v21, v107\n" },
    { 0x4334d6ffU, 0x3d4c, true,
        "        v_subrev_f16    v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x4534d715U, 0, false, "        v_mul_f16       v154, v21, v107\n" },
    { 0x4534d6ffU, 0x3d4c, true,
        "        v_mul_f16       v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x4734d715U, 0, false, "        v_mac_f16       v154, v21, v107\n" },
    { 0x4734d6ffU, 0x3d4c, true,
        "        v_mac_f16       v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x4934d715U, 0x3d4c, true,
        "        v_madmk_f16     v154, v21, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x4b34d715U, 0x3d4c, true,
        "        v_madak_f16     v154, v21, v107, 0x3d4c /* 1.3242h */\n" },
    { 0x4d34d715U, 0, false, "        v_add_u16       v154, v21, v107\n" },
    { 0x4f34d715U, 0, false, "        v_sub_u16       v154, v21, v107\n" },
    { 0x5134d715U, 0, false, "        v_subrev_u16    v154, v21, v107\n" },
    { 0x5334d715U, 0, false, "        v_mul_lo_u16    v154, v21, v107\n" },
    { 0x5534d715U, 0, false, "        v_lshlrev_b16   v154, v21, v107\n" },
    { 0x5734d715U, 0, false, "        v_lshrrev_b16   v154, v21, v107\n" },
    { 0x5934d715U, 0, false, "        v_ashrrev_i16   v154, v21, v107\n" },
    { 0x5b34d715U, 0, false, "        v_max_f16       v154, v21, v107\n" },
    { 0x5b34d6ffU, 0x3d4c, true,
        "        v_max_f16       v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x5d34d715U, 0, false, "        v_min_f16       v154, v21, v107\n" },
    { 0x5d34d6ffU, 0x3d4c, true,
        "        v_min_f16       v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x5f34d715U, 0, false, "        v_max_u16       v154, v21, v107\n" },
    { 0x6134d715U, 0, false, "        v_max_i16       v154, v21, v107\n" },
    { 0x6334d715U, 0, false, "        v_min_u16       v154, v21, v107\n" },
    { 0x6534d715U, 0, false, "        v_min_i16       v154, v21, v107\n" },
    { 0x6734d6ffU, 0x3d4c, true,
        "        v_ldexp_f16     v154, 0x3d4c /* 1.3242h */, v107\n" },
    { 0x6934d715U, 0, false, "        VOP2_ill_52     v154, v21, v107\n" },
    { 0x6b34d715U, 0, false, "        VOP2_ill_53     v154, v21, v107\n" },
    { 0x6d34d715U, 0, false, "        VOP2_ill_54     v154, v21, v107\n" },
    { 0x6f34d715U, 0, false, "        VOP2_ill_55     v154, v21, v107\n" },
    /* VOP1 encoding */
    { 0x7f3c004fU, 0, false, "        v_nop           vdst=0x9e src0=0x4f\n" },
    { 0x7f3c0000U, 0, false, "        v_nop           vdst=0x9e\n" },
    { 0x7e00014fU, 0, false, "        v_nop           src0=0x14f\n" },
    { 0x7e000000U, 0, false, "        v_nop\n" },
    { 0x7e0000f9U, 0x3d003d, true, "        v_nop           "
        "src0=0xf9 dst_sel:byte0 sext0 neg0 abs0\n" },
    { 0x7e0000f9U, 0x3d00003d, true,"        v_nop           "
        "src0=0xf9 dst_sel:byte0 sext1 neg1 abs1\n" },
    { 0x7e0000faU, 0xc872be, true, "        v_nop           "
        "src0=0xfa quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0 neg1 abs1\n" },
    { 0x7e0000faU, 0x3872be, true, "        v_nop           "
        "src0=0xfa quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0 neg0 abs0\n" },
    { 0x7f3c024fU, 0, false, "        v_mov_b32       v158, s79\n" },
    { 0x7e3c044fU, 0, false, "        v_readfirstlane_b32 s30, s79\n" },
    { 0x7e3c054fU, 0, false, "        v_readfirstlane_b32 s30, v79\n" },
    { 0x7f3c074fU, 0, false, "        v_cvt_i32_f64   v158, v[79:80]\n" },
    { 0x7f3c094fU, 0, false, "        v_cvt_f64_i32   v[158:159], v79\n" },
    { 0x7f3c0b4fU, 0, false, "        v_cvt_f32_i32   v158, v79\n" },
    { 0x7f3c0affU, 0x4556fd, true, "        v_cvt_f32_i32   v158, 0x4556fd\n" },
    { 0x7f3c0d4fU, 0, false, "        v_cvt_f32_u32   v158, v79\n" },
    { 0x7f3c0cffU, 0x40000000U, true, "        v_cvt_f32_u32   v158, 0x40000000\n" },
    { 0x7f3c0f4fU, 0, false, "        v_cvt_u32_f32   v158, v79\n" },
    { 0x7f3c0effU, 0x40000000U, true, "        v_cvt_u32_f32   v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c114fU, 0, false, "        v_cvt_i32_f32   v158, v79\n" },
    { 0x7f3c10ffU, 0x40000000U, true, "        v_cvt_i32_f32   v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c134fU, 0, false, "        v_mov_fed_b32   v158, v79\n" },
    { 0x7f3c12ffU, 0x40000000U, true, "        v_mov_fed_b32   v158, "
                "0x40000000\n" },
    { 0x7f3c154fU, 0, false, "        v_cvt_f16_f32   v158, v79\n" },
    { 0x7f3c14ffU, 0x40000000U, true, "        v_cvt_f16_f32   v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c174fU, 0, false, "        v_cvt_f32_f16   v158, v79\n" },
    { 0x7f3c16ffU, 0x3d4cU, true, "        v_cvt_f32_f16   v158, 0x3d4c\n" },
    { 0x7f3c194fU, 0, false, "        v_cvt_rpi_i32_f32 v158, v79\n" },
    { 0x7f3c18ffU, 0x40000000U, true, "        v_cvt_rpi_i32_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c1b4fU, 0, false, "        v_cvt_flr_i32_f32 v158, v79\n" },
    { 0x7f3c1affU, 0x40000000U, true, "        v_cvt_flr_i32_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c1d4fU, 0, false, "        v_cvt_off_f32_i4 v158, v79\n" },
    { 0x7f3c1cffU, 0x40000000U, true, "        v_cvt_off_f32_i4 v158, 0x40000000\n" },
    { 0x7f3c1f4fU, 0, false, "        v_cvt_f32_f64   v158, v[79:80]\n" },
    { 0x7f3c214fU, 0, false, "        v_cvt_f64_f32   v[158:159], v79\n" },
    { 0x7f3c234fU, 0, false, "        v_cvt_f32_ubyte0 v158, v79\n" },
    { 0x7f3c254fU, 0, false, "        v_cvt_f32_ubyte1 v158, v79\n" },
    { 0x7f3c274fU, 0, false, "        v_cvt_f32_ubyte2 v158, v79\n" },
    { 0x7f3c294fU, 0, false, "        v_cvt_f32_ubyte3 v158, v79\n" },
    { 0x7f3c2b4fU, 0, false, "        v_cvt_u32_f64   v158, v[79:80]\n" },
    { 0x7f3c2d4fU, 0, false, "        v_cvt_f64_u32   v[158:159], v79\n" },
    { 0x7f3c2f4fU, 0, false, "        v_trunc_f64     v[158:159], v[79:80]\n" },
    { 0x7f3c314fU, 0, false, "        v_ceil_f64      v[158:159], v[79:80]\n" },
    { 0x7f3c334fU, 0, false, "        v_rndne_f64     v[158:159], v[79:80]\n" },
    { 0x7f3c354fU, 0, false, "        v_floor_f64     v[158:159], v[79:80]\n" },
    { 0x7f3c374fU, 0, false, "        v_fract_f32     v158, v79\n" },
    { 0x7f3c36ffU, 0x40000000U, true, "        v_fract_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c394fU, 0, false, "        v_trunc_f32     v158, v79\n" },
    { 0x7f3c38ffU, 0x40000000U, true, "        v_trunc_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c3b4fU, 0, false, "        v_ceil_f32      v158, v79\n" },
    { 0x7f3c3affU, 0x40000000U, true, "        v_ceil_f32      v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c3d4fU, 0, false, "        v_rndne_f32     v158, v79\n" },
    { 0x7f3c3cffU, 0x40000000U, true, "        v_rndne_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c3f4fU, 0, false, "        v_floor_f32     v158, v79\n" },
    { 0x7f3c3effU, 0x40000000U, true, "        v_floor_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c414fU, 0, false, "        v_exp_f32       v158, v79\n" },
    { 0x7f3c40ffU, 0x40000000U, true, "        v_exp_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c434fU, 0, false, "        v_log_f32       v158, v79\n" },
    { 0x7f3c42ffU, 0x40000000U, true, "        v_log_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c454fU, 0, false, "        v_rcp_f32       v158, v79\n" },
    { 0x7f3c44ffU, 0x40000000U, true, "        v_rcp_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c474fU, 0, false, "        v_rcp_iflag_f32 v158, v79\n" },
    { 0x7f3c46ffU, 0x40000000U, true, "        v_rcp_iflag_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c494fU, 0, false, "        v_rsq_f32       v158, v79\n" },
    { 0x7f3c48ffU, 0x40000000U, true, "        v_rsq_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c4b4fU, 0, false, "        v_rcp_f64       v[158:159], v[79:80]\n" },
    { 0x7f3c4affU, 0x40000000U, true, "        v_rcp_f64       v[158:159], "
                "0x40000000\n" },
    { 0x7f3c4d4fU, 0, false, "        v_rsq_f64       v[158:159], v[79:80]\n" },
    { 0x7f3c4f4fU, 0, false, "        v_sqrt_f32      v158, v79\n" },
    { 0x7f3c514fU, 0, false, "        v_sqrt_f64      v[158:159], v[79:80]\n" },
    { 0x7f3c534fU, 0, false, "        v_sin_f32       v158, v79\n" },
    { 0x7f3c554fU, 0, false, "        v_cos_f32       v158, v79\n" },
    { 0x7f3c574fU, 0, false, "        v_not_b32       v158, v79\n" },
    { 0x7f3c594fU, 0, false, "        v_bfrev_b32     v158, v79\n" },
    { 0x7f3c5b4fU, 0, false, "        v_ffbh_u32      v158, v79\n" },
    { 0x7f3c5d4fU, 0, false, "        v_ffbl_b32      v158, v79\n" },
    { 0x7f3c5f4fU, 0, false, "        v_ffbh_i32      v158, v79\n" },
    { 0x7f3c614fU, 0, false, "        v_frexp_exp_i32_f64 v158, v[79:80]\n" },
    { 0x7f3c634fU, 0, false, "        v_frexp_mant_f64 v[158:159], v[79:80]\n" },
    { 0x7f3c654fU, 0, false, "        v_fract_f64     v[158:159], v[79:80]\n" },
    { 0x7f3c674fU, 0, false, "        v_frexp_exp_i32_f32 v158, v79\n" },
    { 0x7f3c66ffU, 0x40000000U, true, "        v_frexp_exp_i32_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c694fU, 0, false, "        v_frexp_mant_f32 v158, v79\n" },
    { 0x7f3c68ffU, 0x40000000U, true, "        v_frexp_mant_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c6b4fU, 0, false, "        v_clrexcp       vdst=0x9e src0=0x14f\n" },
    { 0x7e006a00U, 0, false, "        v_clrexcp\n" },
    { 0x7f3c6d4fU, 0, false, "        v_movreld_b32   v158, v79\n" },
    { 0x7f3c6f4fU, 0, false, "        v_movrels_b32   v158, v79\n" },
    { 0x7f3c714fU, 0, false, "        v_movrelsd_b32  v158, v79\n" },
    { 0x7f3c734fU, 0, false, "        v_cvt_f16_u16   v158, v79\n" },
    { 0x7f3c754fU, 0, false, "        v_cvt_f16_i16   v158, v79\n" },
    { 0x7f3c774fU, 0, false, "        v_cvt_u16_f16   v158, v79\n" },
    { 0x7f3c76ffU, 0x3d4c, true, "        v_cvt_u16_f16   v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c794fU, 0, false, "        v_cvt_i16_f16   v158, v79\n" },
    { 0x7f3c78ffU, 0x3d4c, true, "        v_cvt_i16_f16   v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c7b4fU, 0, false, "        v_rcp_f16       v158, v79\n" },
    { 0x7f3c7affU, 0x3d4c, true, "        v_rcp_f16       v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c7d4fU, 0, false, "        v_sqrt_f16      v158, v79\n" },
    { 0x7f3c7cffU, 0x3d4c, true, "        v_sqrt_f16      v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c7f4fU, 0, false, "        v_rsq_f16       v158, v79\n" },
    { 0x7f3c7effU, 0x3d4c, true, "        v_rsq_f16       v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c814fU, 0, false, "        v_log_f16       v158, v79\n" },
    { 0x7f3c80ffU, 0x3d4c, true, "        v_log_f16       v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c834fU, 0, false, "        v_exp_f16       v158, v79\n" },
    { 0x7f3c82ffU, 0x3d4c, true, "        v_exp_f16       v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c854fU, 0, false, "        v_frexp_mant_f16 v158, v79\n" },
    { 0x7f3c84ffU, 0x3d4c, true, "        v_frexp_mant_f16 v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c874fU, 0, false, "        v_frexp_exp_i16_f16 v158, v79\n" },
    { 0x7f3c86ffU, 0x3d4c, true, "        v_frexp_exp_i16_f16 "
        "v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c894fU, 0, false, "        v_floor_f16     v158, v79\n" },
    { 0x7f3c88ffU, 0x3d4c, true, "        v_floor_f16     v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c8b4fU, 0, false, "        v_ceil_f16      v158, v79\n" },
    { 0x7f3c8affU, 0x3d4c, true, "        v_ceil_f16      v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c8d4fU, 0, false, "        v_trunc_f16     v158, v79\n" },
    { 0x7f3c8cffU, 0x3d4c, true, "        v_trunc_f16     v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c8f4fU, 0, false, "        v_rndne_f16     v158, v79\n" },
    { 0x7f3c8effU, 0x3d4c, true, "        v_rndne_f16     v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c914fU, 0, false, "        v_fract_f16     v158, v79\n" },
    { 0x7f3c90ffU, 0x3d4c, true, "        v_fract_f16     v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c934fU, 0, false, "        v_sin_f16       v158, v79\n" },
    { 0x7f3c92ffU, 0x3d4c, true, "        v_sin_f16       v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c954fU, 0, false, "        v_cos_f16       v158, v79\n" },
    { 0x7f3c94ffU, 0x3d4c, true, "        v_cos_f16       v158, 0x3d4c /* 1.3242h */\n" },
    { 0x7f3c974fU, 0, false, "        v_exp_legacy_f32 v158, v79\n" },
    { 0x7f3c96ffU, 0x40000000U, true, "        v_exp_legacy_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c994fU, 0, false, "        v_log_legacy_f32 v158, v79\n" },
    { 0x7f3c98ffU, 0x40000000U, true, "        v_log_legacy_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c9b4fU, 0, false, "        VOP1_ill_77     v158, v79\n" },
    { 0x7f3c9d4fU, 0, false, "        VOP1_ill_78     v158, v79\n" },
    { 0x7f3c9f4fU, 0, false, "        VOP1_ill_79     v158, v79\n" },
    /* VOPC encoding */
    { 0x7c01934fU, 0, false, "        VOPC_ill_0      vcc, v79, v201\n" },
    { 0x7c03934fU, 0, false, "        VOPC_ill_1      vcc, v79, v201\n" },
    { 0x7c05934fU, 0, false, "        VOPC_ill_2      vcc, v79, v201\n" },
    { 0x7c07934fU, 0, false, "        VOPC_ill_3      vcc, v79, v201\n" },
    { 0x7c09934fU, 0, false, "        VOPC_ill_4      vcc, v79, v201\n" },
    { 0x7c0b934fU, 0, false, "        VOPC_ill_5      vcc, v79, v201\n" },
    { 0x7c0d934fU, 0, false, "        VOPC_ill_6      vcc, v79, v201\n" },
    { 0x7c0f934fU, 0, false, "        VOPC_ill_7      vcc, v79, v201\n" },
    { 0x7c11934fU, 0, false, "        VOPC_ill_8      vcc, v79, v201\n" },
    { 0x7c13934fU, 0, false, "        VOPC_ill_9      vcc, v79, v201\n" },
    { 0x7c15934fU, 0, false, "        VOPC_ill_10     vcc, v79, v201\n" },
    { 0x7c17934fU, 0, false, "        VOPC_ill_11     vcc, v79, v201\n" },
    { 0x7c19934fU, 0, false, "        VOPC_ill_12     vcc, v79, v201\n" },
    { 0x7c1b934fU, 0, false, "        VOPC_ill_13     vcc, v79, v201\n" },
    { 0x7c1d934fU, 0, false, "        VOPC_ill_14     vcc, v79, v201\n" },
    { 0x7c1f934fU, 0, false, "        VOPC_ill_15     vcc, v79, v201\n" },
    { 0x7c21934fU, 0, false, "        v_cmp_class_f32 vcc, v79, v201\n" },
    { 0x7c2192ffU, 0x40000000U, true, "        v_cmp_class_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c23934fU, 0, false, "        v_cmpx_class_f32 vcc, v79, v201\n" },
    { 0x7c2392ffU, 0x40000000U, true, "        v_cmpx_class_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c25934fU, 0, false, "        v_cmp_class_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7c2592ffU, 0x40000000U, true, "        v_cmp_class_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c27934fU, 0, false, "        v_cmpx_class_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7c2792ffU, 0x40000000U, true, "        v_cmpx_class_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c29934fU, 0, false, "        v_cmp_class_f16 vcc, v79, v201\n" },
    { 0x7c2992ffU, 0x3d4c, true, "        v_cmp_class_f16 "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c2b934fU, 0, false, "        v_cmpx_class_f16 vcc, v79, v201\n" },
    { 0x7c2b92ffU, 0x3d4c, true, "        v_cmpx_class_f16 "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c2d934fU, 0, false, "        VOPC_ill_22     vcc, v79, v201\n" },
    { 0x7c2f934fU, 0, false, "        VOPC_ill_23     vcc, v79, v201\n" },
    { 0x7c31934fU, 0, false, "        VOPC_ill_24     vcc, v79, v201\n" },
    { 0x7c33934fU, 0, false, "        VOPC_ill_25     vcc, v79, v201\n" },
    { 0x7c35934fU, 0, false, "        VOPC_ill_26     vcc, v79, v201\n" },
    { 0x7c37934fU, 0, false, "        VOPC_ill_27     vcc, v79, v201\n" },
    { 0x7c39934fU, 0, false, "        VOPC_ill_28     vcc, v79, v201\n" },
    { 0x7c3b934fU, 0, false, "        VOPC_ill_29     vcc, v79, v201\n" },
    { 0x7c3d934fU, 0, false, "        VOPC_ill_30     vcc, v79, v201\n" },
    { 0x7c3f934fU, 0, false, "        VOPC_ill_31     vcc, v79, v201\n" },
    { 0x7c41934fU, 0, false, "        v_cmp_f_f16     vcc, v79, v201\n" },
    { 0x7c4192ffU, 0x3d4cU, true, "        v_cmp_f_f16     "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c43934fU, 0, false, "        v_cmp_lt_f16    vcc, v79, v201\n" },
    { 0x7c4392ffU, 0x3d4cU, true, "        v_cmp_lt_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c45934fU, 0, false, "        v_cmp_eq_f16    vcc, v79, v201\n" },
    { 0x7c4592ffU, 0x3d4cU, true, "        v_cmp_eq_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c47934fU, 0, false, "        v_cmp_le_f16    vcc, v79, v201\n" },
    { 0x7c4792ffU, 0x3d4cU, true, "        v_cmp_le_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c49934fU, 0, false, "        v_cmp_gt_f16    vcc, v79, v201\n" },
    { 0x7c4992ffU, 0x3d4cU, true, "        v_cmp_gt_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c4b934fU, 0, false, "        v_cmp_lg_f16    vcc, v79, v201\n" },
    { 0x7c4b92ffU, 0x3d4cU, true, "        v_cmp_lg_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c4d934fU, 0, false, "        v_cmp_ge_f16    vcc, v79, v201\n" },
    { 0x7c4d92ffU, 0x3d4cU, true, "        v_cmp_ge_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c4f934fU, 0, false, "        v_cmp_o_f16     vcc, v79, v201\n" },
    { 0x7c4f92ffU, 0x3d4cU, true, "        v_cmp_o_f16     "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c51934fU, 0, false, "        v_cmp_u_f16     vcc, v79, v201\n" },
    { 0x7c5192ffU, 0x3d4cU, true, "        v_cmp_u_f16     "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c53934fU, 0, false, "        v_cmp_nge_f16   vcc, v79, v201\n" },
    { 0x7c5392ffU, 0x3d4cU, true, "        v_cmp_nge_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c55934fU, 0, false, "        v_cmp_nlg_f16   vcc, v79, v201\n" },
    { 0x7c5592ffU, 0x3d4cU, true, "        v_cmp_nlg_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c57934fU, 0, false, "        v_cmp_ngt_f16   vcc, v79, v201\n" },
    { 0x7c5792ffU, 0x3d4cU, true, "        v_cmp_ngt_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c59934fU, 0, false, "        v_cmp_nle_f16   vcc, v79, v201\n" },
    { 0x7c5992ffU, 0x3d4cU, true, "        v_cmp_nle_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c5b934fU, 0, false, "        v_cmp_neq_f16   vcc, v79, v201\n" },
    { 0x7c5b92ffU, 0x3d4cU, true, "        v_cmp_neq_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c5d934fU, 0, false, "        v_cmp_nlt_f16   vcc, v79, v201\n" },
    { 0x7c5d92ffU, 0x3d4cU, true, "        v_cmp_nlt_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c5f934fU, 0, false, "        v_cmp_tru_f16   vcc, v79, v201\n" },
    { 0x7c5f92ffU, 0x3d4cU, true, "        v_cmp_tru_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    
    { 0x7c61934fU, 0, false, "        v_cmpx_f_f16    vcc, v79, v201\n" },
    { 0x7c6192ffU, 0x3d4cU, true, "        v_cmpx_f_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c63934fU, 0, false, "        v_cmpx_lt_f16   vcc, v79, v201\n" },
    { 0x7c6392ffU, 0x3d4cU, true, "        v_cmpx_lt_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c65934fU, 0, false, "        v_cmpx_eq_f16   vcc, v79, v201\n" },
    { 0x7c6592ffU, 0x3d4cU, true, "        v_cmpx_eq_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c67934fU, 0, false, "        v_cmpx_le_f16   vcc, v79, v201\n" },
    { 0x7c6792ffU, 0x3d4cU, true, "        v_cmpx_le_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c69934fU, 0, false, "        v_cmpx_gt_f16   vcc, v79, v201\n" },
    { 0x7c6992ffU, 0x3d4cU, true, "        v_cmpx_gt_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c6b934fU, 0, false, "        v_cmpx_lg_f16   vcc, v79, v201\n" },
    { 0x7c6b92ffU, 0x3d4cU, true, "        v_cmpx_lg_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c6d934fU, 0, false, "        v_cmpx_ge_f16   vcc, v79, v201\n" },
    { 0x7c6d92ffU, 0x3d4cU, true, "        v_cmpx_ge_f16   "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c6f934fU, 0, false, "        v_cmpx_o_f16    vcc, v79, v201\n" },
    { 0x7c6f92ffU, 0x3d4cU, true, "        v_cmpx_o_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c71934fU, 0, false, "        v_cmpx_u_f16    vcc, v79, v201\n" },
    { 0x7c7192ffU, 0x3d4cU, true, "        v_cmpx_u_f16    "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c73934fU, 0, false, "        v_cmpx_nge_f16  vcc, v79, v201\n" },
    { 0x7c7392ffU, 0x3d4cU, true, "        v_cmpx_nge_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c75934fU, 0, false, "        v_cmpx_nlg_f16  vcc, v79, v201\n" },
    { 0x7c7592ffU, 0x3d4cU, true, "        v_cmpx_nlg_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c77934fU, 0, false, "        v_cmpx_ngt_f16  vcc, v79, v201\n" },
    { 0x7c7792ffU, 0x3d4cU, true, "        v_cmpx_ngt_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c79934fU, 0, false, "        v_cmpx_nle_f16  vcc, v79, v201\n" },
    { 0x7c7992ffU, 0x3d4cU, true, "        v_cmpx_nle_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c7b934fU, 0, false, "        v_cmpx_neq_f16  vcc, v79, v201\n" },
    { 0x7c7b92ffU, 0x3d4cU, true, "        v_cmpx_neq_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c7d934fU, 0, false, "        v_cmpx_nlt_f16  vcc, v79, v201\n" },
    { 0x7c7d92ffU, 0x3d4cU, true, "        v_cmpx_nlt_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    { 0x7c7f934fU, 0, false, "        v_cmpx_tru_f16  vcc, v79, v201\n" },
    { 0x7c7f92ffU, 0x3d4cU, true, "        v_cmpx_tru_f16  "
                "vcc, 0x3d4c /* 1.3242h */, v201\n" },
    
    { 0x7c81934fU, 0, false, "        v_cmp_f_f32     vcc, v79, v201\n" },
    { 0x7c8192ffU, 0x40000000U, true, "        v_cmp_f_f32     "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c83934fU, 0, false, "        v_cmp_lt_f32    vcc, v79, v201\n" },
    { 0x7c8392ffU, 0x40000000U, true, "        v_cmp_lt_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c85934fU, 0, false, "        v_cmp_eq_f32    vcc, v79, v201\n" },
    { 0x7c8592ffU, 0x40000000U, true, "        v_cmp_eq_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c87934fU, 0, false, "        v_cmp_le_f32    vcc, v79, v201\n" },
    { 0x7c8792ffU, 0x40000000U, true, "        v_cmp_le_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c89934fU, 0, false, "        v_cmp_gt_f32    vcc, v79, v201\n" },
    { 0x7c8992ffU, 0x40000000U, true, "        v_cmp_gt_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c8b934fU, 0, false, "        v_cmp_lg_f32    vcc, v79, v201\n" },
    { 0x7c8b92ffU, 0x40000000U, true, "        v_cmp_lg_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c8d934fU, 0, false, "        v_cmp_ge_f32    vcc, v79, v201\n" },
    { 0x7c8d92ffU, 0x40000000U, true, "        v_cmp_ge_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c8f934fU, 0, false, "        v_cmp_o_f32     vcc, v79, v201\n" },
    { 0x7c8f92ffU, 0x40000000U, true, "        v_cmp_o_f32     "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c91934fU, 0, false, "        v_cmp_u_f32     vcc, v79, v201\n" },
    { 0x7c9192ffU, 0x40000000U, true, "        v_cmp_u_f32     "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c93934fU, 0, false, "        v_cmp_nge_f32   vcc, v79, v201\n" },
    { 0x7c9392ffU, 0x40000000U, true, "        v_cmp_nge_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c95934fU, 0, false, "        v_cmp_nlg_f32   vcc, v79, v201\n" },
    { 0x7c9592ffU, 0x40000000U, true, "        v_cmp_nlg_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c97934fU, 0, false, "        v_cmp_ngt_f32   vcc, v79, v201\n" },
    { 0x7c9792ffU, 0x40000000U, true, "        v_cmp_ngt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c99934fU, 0, false, "        v_cmp_nle_f32   vcc, v79, v201\n" },
    { 0x7c9992ffU, 0x40000000U, true, "        v_cmp_nle_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c9b934fU, 0, false, "        v_cmp_neq_f32   vcc, v79, v201\n" },
    { 0x7c9b92ffU, 0x40000000U, true, "        v_cmp_neq_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c9d934fU, 0, false, "        v_cmp_nlt_f32   vcc, v79, v201\n" },
    { 0x7c9d92ffU, 0x40000000U, true, "        v_cmp_nlt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c9f934fU, 0, false, "        v_cmp_tru_f32   vcc, v79, v201\n" },
    { 0x7c9f92ffU, 0x40000000U, true, "        v_cmp_tru_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca1934fU, 0, false, "        v_cmpx_f_f32    vcc, v79, v201\n" },
    { 0x7ca192ffU, 0x40000000U, true, "        v_cmpx_f_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca3934fU, 0, false, "        v_cmpx_lt_f32   vcc, v79, v201\n" },
    { 0x7ca392ffU, 0x40000000U, true, "        v_cmpx_lt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca5934fU, 0, false, "        v_cmpx_eq_f32   vcc, v79, v201\n" },
    { 0x7ca592ffU, 0x40000000U, true, "        v_cmpx_eq_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca7934fU, 0, false, "        v_cmpx_le_f32   vcc, v79, v201\n" },
    { 0x7ca792ffU, 0x40000000U, true, "        v_cmpx_le_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca9934fU, 0, false, "        v_cmpx_gt_f32   vcc, v79, v201\n" },
    { 0x7ca992ffU, 0x40000000U, true, "        v_cmpx_gt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cab934fU, 0, false, "        v_cmpx_lg_f32   vcc, v79, v201\n" },
    { 0x7cab92ffU, 0x40000000U, true, "        v_cmpx_lg_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cad934fU, 0, false, "        v_cmpx_ge_f32   vcc, v79, v201\n" },
    { 0x7cad92ffU, 0x40000000U, true, "        v_cmpx_ge_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7caf934fU, 0, false, "        v_cmpx_o_f32    vcc, v79, v201\n" },
    { 0x7caf92ffU, 0x40000000U, true, "        v_cmpx_o_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb1934fU, 0, false, "        v_cmpx_u_f32    vcc, v79, v201\n" },
    { 0x7cb192ffU, 0x40000000U, true, "        v_cmpx_u_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb3934fU, 0, false, "        v_cmpx_nge_f32  vcc, v79, v201\n" },
    { 0x7cb392ffU, 0x40000000U, true, "        v_cmpx_nge_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb5934fU, 0, false, "        v_cmpx_nlg_f32  vcc, v79, v201\n" },
    { 0x7cb592ffU, 0x40000000U, true, "        v_cmpx_nlg_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb7934fU, 0, false, "        v_cmpx_ngt_f32  vcc, v79, v201\n" },
    { 0x7cb792ffU, 0x40000000U, true, "        v_cmpx_ngt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb9934fU, 0, false, "        v_cmpx_nle_f32  vcc, v79, v201\n" },
    { 0x7cb992ffU, 0x40000000U, true, "        v_cmpx_nle_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cbb934fU, 0, false, "        v_cmpx_neq_f32  vcc, v79, v201\n" },
    { 0x7cbb92ffU, 0x40000000U, true, "        v_cmpx_neq_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cbd934fU, 0, false, "        v_cmpx_nlt_f32  vcc, v79, v201\n" },
    { 0x7cbd92ffU, 0x40000000U, true, "        v_cmpx_nlt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cbf934fU, 0, false, "        v_cmpx_tru_f32  vcc, v79, v201\n" },
    { 0x7cbf92ffU, 0x40000000U, true, "        v_cmpx_tru_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    
    { 0x7cc1934fU, 0, false, "        v_cmp_f_f64     vcc, v[79:80], v[201:202]\n" },
    { 0x7cc192ffU, 0x40000000U, true, "        v_cmp_f_f64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc3934fU, 0, false, "        v_cmp_lt_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cc392ffU, 0x40000000U, true, "        v_cmp_lt_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc5934fU, 0, false, "        v_cmp_eq_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cc592ffU, 0x40000000U, true, "        v_cmp_eq_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc7934fU, 0, false, "        v_cmp_le_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cc792ffU, 0x40000000U, true, "        v_cmp_le_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc9934fU, 0, false, "        v_cmp_gt_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cc992ffU, 0x40000000U, true, "        v_cmp_gt_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ccb934fU, 0, false, "        v_cmp_lg_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7ccb92ffU, 0x40000000U, true, "        v_cmp_lg_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ccd934fU, 0, false, "        v_cmp_ge_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7ccd92ffU, 0x40000000U, true, "        v_cmp_ge_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ccf934fU, 0, false, "        v_cmp_o_f64     vcc, v[79:80], v[201:202]\n" },
    { 0x7ccf92ffU, 0x40000000U, true, "        v_cmp_o_f64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd1934fU, 0, false, "        v_cmp_u_f64     vcc, v[79:80], v[201:202]\n" },
    { 0x7cd192ffU, 0x40000000U, true, "        v_cmp_u_f64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd3934fU, 0, false, "        v_cmp_nge_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cd392ffU, 0x40000000U, true, "        v_cmp_nge_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd5934fU, 0, false, "        v_cmp_nlg_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cd592ffU, 0x40000000U, true, "        v_cmp_nlg_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd7934fU, 0, false, "        v_cmp_ngt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cd792ffU, 0x40000000U, true, "        v_cmp_ngt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd9934fU, 0, false, "        v_cmp_nle_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cd992ffU, 0x40000000U, true, "        v_cmp_nle_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cdb934fU, 0, false, "        v_cmp_neq_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cdb92ffU, 0x40000000U, true, "        v_cmp_neq_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cdd934fU, 0, false, "        v_cmp_nlt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cdd92ffU, 0x40000000U, true, "        v_cmp_nlt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cdf934fU, 0, false, "        v_cmp_tru_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cdf92ffU, 0x40000000U, true, "        v_cmp_tru_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce1934fU, 0, false, "        v_cmpx_f_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7ce192ffU, 0x40000000U, true, "        v_cmpx_f_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce3934fU, 0, false, "        v_cmpx_lt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ce392ffU, 0x40000000U, true, "        v_cmpx_lt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce5934fU, 0, false, "        v_cmpx_eq_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ce592ffU, 0x40000000U, true, "        v_cmpx_eq_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce7934fU, 0, false, "        v_cmpx_le_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ce792ffU, 0x40000000U, true, "        v_cmpx_le_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce9934fU, 0, false, "        v_cmpx_gt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ce992ffU, 0x40000000U, true, "        v_cmpx_gt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ceb934fU, 0, false, "        v_cmpx_lg_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ceb92ffU, 0x40000000U, true, "        v_cmpx_lg_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ced934fU, 0, false, "        v_cmpx_ge_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ced92ffU, 0x40000000U, true, "        v_cmpx_ge_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cef934fU, 0, false, "        v_cmpx_o_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cef92ffU, 0x40000000U, true, "        v_cmpx_o_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf1934fU, 0, false, "        v_cmpx_u_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cf192ffU, 0x40000000U, true, "        v_cmpx_u_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf3934fU, 0, false, "        v_cmpx_nge_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cf392ffU, 0x40000000U, true, "        v_cmpx_nge_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf5934fU, 0, false, "        v_cmpx_nlg_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cf592ffU, 0x40000000U, true, "        v_cmpx_nlg_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf7934fU, 0, false, "        v_cmpx_ngt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cf792ffU, 0x40000000U, true, "        v_cmpx_ngt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf9934fU, 0, false, "        v_cmpx_nle_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cf992ffU, 0x40000000U, true, "        v_cmpx_nle_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cfb934fU, 0, false, "        v_cmpx_neq_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cfb92ffU, 0x40000000U, true, "        v_cmpx_neq_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cfd934fU, 0, false, "        v_cmpx_nlt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cfd92ffU, 0x40000000U, true, "        v_cmpx_nlt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cff934fU, 0, false, "        v_cmpx_tru_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cff92ffU, 0x40000000U, true, "        v_cmpx_tru_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    
    { 0x7d01934fU, 0, false, "        VOPC_ill_128    vcc, v79, v201\n" },
    { 0x7d41934fU, 0, false, "        v_cmp_f_i16     vcc, v79, v201\n" },
    { 0x7d4192ffU, 0x40000000U, true, "        v_cmp_f_i16     vcc, 0x40000000, v201\n" },
    { 0x7d43934fU, 0, false, "        v_cmp_lt_i16    vcc, v79, v201\n" },
    { 0x7d4392ffU, 0x40000000U, true, "        v_cmp_lt_i16    vcc, 0x40000000, v201\n" },
    { 0x7d45934fU, 0, false, "        v_cmp_eq_i16    vcc, v79, v201\n" },
    { 0x7d4592ffU, 0x40000000U, true, "        v_cmp_eq_i16    vcc, 0x40000000, v201\n" },
    { 0x7d47934fU, 0, false, "        v_cmp_le_i16    vcc, v79, v201\n" },
    { 0x7d4792ffU, 0x40000000U, true, "        v_cmp_le_i16    vcc, 0x40000000, v201\n" },
    { 0x7d49934fU, 0, false, "        v_cmp_gt_i16    vcc, v79, v201\n" },
    { 0x7d4992ffU, 0x40000000U, true, "        v_cmp_gt_i16    vcc, 0x40000000, v201\n" },
    { 0x7d4b934fU, 0, false, "        v_cmp_lg_i16    vcc, v79, v201\n" },
    { 0x7d4b92ffU, 0x40000000U, true, "        v_cmp_lg_i16    vcc, 0x40000000, v201\n" },
    { 0x7d4d934fU, 0, false, "        v_cmp_ge_i16    vcc, v79, v201\n" },
    { 0x7d4d92ffU, 0x40000000U, true, "        v_cmp_ge_i16    vcc, 0x40000000, v201\n" },
    { 0x7d4f934fU, 0, false, "        v_cmp_tru_i16   vcc, v79, v201\n" },
    { 0x7d4f92ffU, 0x40000000U, true, "        v_cmp_tru_i16   vcc, 0x40000000, v201\n" },
    { 0x7d51934fU, 0, false, "        v_cmp_f_u16     vcc, v79, v201\n" },
    { 0x7d5192ffU, 0x40000000U, true, "        v_cmp_f_u16     vcc, 0x40000000, v201\n" },
    { 0x7d53934fU, 0, false, "        v_cmp_lt_u16    vcc, v79, v201\n" },
    { 0x7d5392ffU, 0x40000000U, true, "        v_cmp_lt_u16    vcc, 0x40000000, v201\n" },
    { 0x7d55934fU, 0, false, "        v_cmp_eq_u16    vcc, v79, v201\n" },
    { 0x7d5592ffU, 0x40000000U, true, "        v_cmp_eq_u16    vcc, 0x40000000, v201\n" },
    { 0x7d57934fU, 0, false, "        v_cmp_le_u16    vcc, v79, v201\n" },
    { 0x7d5792ffU, 0x40000000U, true, "        v_cmp_le_u16    vcc, 0x40000000, v201\n" },
    { 0x7d59934fU, 0, false, "        v_cmp_gt_u16    vcc, v79, v201\n" },
    { 0x7d5992ffU, 0x40000000U, true, "        v_cmp_gt_u16    vcc, 0x40000000, v201\n" },
    { 0x7d5b934fU, 0, false, "        v_cmp_lg_u16    vcc, v79, v201\n" },
    { 0x7d5b92ffU, 0x40000000U, true, "        v_cmp_lg_u16    vcc, 0x40000000, v201\n" },
    { 0x7d5d934fU, 0, false, "        v_cmp_ge_u16    vcc, v79, v201\n" },
    { 0x7d5d92ffU, 0x40000000U, true, "        v_cmp_ge_u16    vcc, 0x40000000, v201\n" },
    { 0x7d5f934fU, 0, false, "        v_cmp_tru_u16   vcc, v79, v201\n" },
    { 0x7d5f92ffU, 0x40000000U, true, "        v_cmp_tru_u16   vcc, 0x40000000, v201\n" },
    { 0x7d61934fU, 0, false, "        v_cmpx_f_i16    vcc, v79, v201\n" },
    { 0x7d6192ffU, 0x40000000U, true, "        v_cmpx_f_i16    vcc, 0x40000000, v201\n" },
    { 0x7d63934fU, 0, false, "        v_cmpx_lt_i16   vcc, v79, v201\n" },
    { 0x7d6392ffU, 0x40000000U, true, "        v_cmpx_lt_i16   vcc, 0x40000000, v201\n" },
    { 0x7d65934fU, 0, false, "        v_cmpx_eq_i16   vcc, v79, v201\n" },
    { 0x7d6592ffU, 0x40000000U, true, "        v_cmpx_eq_i16   vcc, 0x40000000, v201\n" },
    { 0x7d67934fU, 0, false, "        v_cmpx_le_i16   vcc, v79, v201\n" },
    { 0x7d6792ffU, 0x40000000U, true, "        v_cmpx_le_i16   vcc, 0x40000000, v201\n" },
    { 0x7d69934fU, 0, false, "        v_cmpx_gt_i16   vcc, v79, v201\n" },
    { 0x7d6992ffU, 0x40000000U, true, "        v_cmpx_gt_i16   vcc, 0x40000000, v201\n" },
    { 0x7d6b934fU, 0, false, "        v_cmpx_lg_i16   vcc, v79, v201\n" },
    { 0x7d6b92ffU, 0x40000000U, true, "        v_cmpx_lg_i16   vcc, 0x40000000, v201\n" },
    { 0x7d6d934fU, 0, false, "        v_cmpx_ge_i16   vcc, v79, v201\n" },
    { 0x7d6d92ffU, 0x40000000U, true, "        v_cmpx_ge_i16   vcc, 0x40000000, v201\n" },
    { 0x7d6f934fU, 0, false, "        v_cmpx_tru_i16  vcc, v79, v201\n" },
    { 0x7d6f92ffU, 0x40000000U, true, "        v_cmpx_tru_i16  vcc, 0x40000000, v201\n" },
    { 0x7d71934fU, 0, false, "        v_cmpx_f_u16    vcc, v79, v201\n" },
    { 0x7d7192ffU, 0x40000000U, true, "        v_cmpx_f_u16    vcc, 0x40000000, v201\n" },
    { 0x7d73934fU, 0, false, "        v_cmpx_lt_u16   vcc, v79, v201\n" },
    { 0x7d7392ffU, 0x40000000U, true, "        v_cmpx_lt_u16   vcc, 0x40000000, v201\n" },
    { 0x7d75934fU, 0, false, "        v_cmpx_eq_u16   vcc, v79, v201\n" },
    { 0x7d7592ffU, 0x40000000U, true, "        v_cmpx_eq_u16   vcc, 0x40000000, v201\n" },
    { 0x7d77934fU, 0, false, "        v_cmpx_le_u16   vcc, v79, v201\n" },
    { 0x7d7792ffU, 0x40000000U, true, "        v_cmpx_le_u16   vcc, 0x40000000, v201\n" },
    { 0x7d79934fU, 0, false, "        v_cmpx_gt_u16   vcc, v79, v201\n" },
    { 0x7d7992ffU, 0x40000000U, true, "        v_cmpx_gt_u16   vcc, 0x40000000, v201\n" },
    { 0x7d7b934fU, 0, false, "        v_cmpx_lg_u16   vcc, v79, v201\n" },
    { 0x7d7b92ffU, 0x40000000U, true, "        v_cmpx_lg_u16   vcc, 0x40000000, v201\n" },
    { 0x7d7d934fU, 0, false, "        v_cmpx_ge_u16   vcc, v79, v201\n" },
    { 0x7d7d92ffU, 0x40000000U, true, "        v_cmpx_ge_u16   vcc, 0x40000000, v201\n" },
    { 0x7d7f934fU, 0, false, "        v_cmpx_tru_u16  vcc, v79, v201\n" },
    { 0x7d7f92ffU, 0x40000000U, true, "        v_cmpx_tru_u16  vcc, 0x40000000, v201\n" },    
    { 0x7d81934fU, 0, false, "        v_cmp_f_i32     vcc, v79, v201\n" },
    { 0x7d8192ffU, 0x40000000U, true, "        v_cmp_f_i32     vcc, 0x40000000, v201\n" },
    { 0x7d83934fU, 0, false, "        v_cmp_lt_i32    vcc, v79, v201\n" },
    { 0x7d8392ffU, 0x40000000U, true, "        v_cmp_lt_i32    vcc, 0x40000000, v201\n" },
    { 0x7d85934fU, 0, false, "        v_cmp_eq_i32    vcc, v79, v201\n" },
    { 0x7d8592ffU, 0x40000000U, true, "        v_cmp_eq_i32    vcc, 0x40000000, v201\n" },
    { 0x7d87934fU, 0, false, "        v_cmp_le_i32    vcc, v79, v201\n" },
    { 0x7d8792ffU, 0x40000000U, true, "        v_cmp_le_i32    vcc, 0x40000000, v201\n" },
    { 0x7d89934fU, 0, false, "        v_cmp_gt_i32    vcc, v79, v201\n" },
    { 0x7d8992ffU, 0x40000000U, true, "        v_cmp_gt_i32    vcc, 0x40000000, v201\n" },
    { 0x7d8b934fU, 0, false, "        v_cmp_lg_i32    vcc, v79, v201\n" },
    { 0x7d8b92ffU, 0x40000000U, true, "        v_cmp_lg_i32    vcc, 0x40000000, v201\n" },
    { 0x7d8d934fU, 0, false, "        v_cmp_ge_i32    vcc, v79, v201\n" },
    { 0x7d8d92ffU, 0x40000000U, true, "        v_cmp_ge_i32    vcc, 0x40000000, v201\n" },
    { 0x7d8f934fU, 0, false, "        v_cmp_tru_i32   vcc, v79, v201\n" },
    { 0x7d8f92ffU, 0x40000000U, true, "        v_cmp_tru_i32   vcc, 0x40000000, v201\n" },
    { 0x7d91934fU, 0, false, "        v_cmp_f_u32     vcc, v79, v201\n" },
    { 0x7d9192ffU, 0x40000000U, true, "        v_cmp_f_u32     vcc, 0x40000000, v201\n" },
    { 0x7d93934fU, 0, false, "        v_cmp_lt_u32    vcc, v79, v201\n" },
    { 0x7d9392ffU, 0x40000000U, true, "        v_cmp_lt_u32    vcc, 0x40000000, v201\n" },
    { 0x7d95934fU, 0, false, "        v_cmp_eq_u32    vcc, v79, v201\n" },
    { 0x7d9592ffU, 0x40000000U, true, "        v_cmp_eq_u32    vcc, 0x40000000, v201\n" },
    { 0x7d97934fU, 0, false, "        v_cmp_le_u32    vcc, v79, v201\n" },
    { 0x7d9792ffU, 0x40000000U, true, "        v_cmp_le_u32    vcc, 0x40000000, v201\n" },
    { 0x7d99934fU, 0, false, "        v_cmp_gt_u32    vcc, v79, v201\n" },
    { 0x7d9992ffU, 0x40000000U, true, "        v_cmp_gt_u32    vcc, 0x40000000, v201\n" },
    { 0x7d9b934fU, 0, false, "        v_cmp_lg_u32    vcc, v79, v201\n" },
    { 0x7d9b92ffU, 0x40000000U, true, "        v_cmp_lg_u32    vcc, 0x40000000, v201\n" },
    { 0x7d9d934fU, 0, false, "        v_cmp_ge_u32    vcc, v79, v201\n" },
    { 0x7d9d92ffU, 0x40000000U, true, "        v_cmp_ge_u32    vcc, 0x40000000, v201\n" },
    { 0x7d9f934fU, 0, false, "        v_cmp_tru_u32   vcc, v79, v201\n" },
    { 0x7d9f92ffU, 0x40000000U, true, "        v_cmp_tru_u32   vcc, 0x40000000, v201\n" },
    { 0x7da1934fU, 0, false, "        v_cmpx_f_i32    vcc, v79, v201\n" },
    { 0x7da192ffU, 0x40000000U, true, "        v_cmpx_f_i32    vcc, 0x40000000, v201\n" },
    { 0x7da3934fU, 0, false, "        v_cmpx_lt_i32   vcc, v79, v201\n" },
    { 0x7da392ffU, 0x40000000U, true, "        v_cmpx_lt_i32   vcc, 0x40000000, v201\n" },
    { 0x7da5934fU, 0, false, "        v_cmpx_eq_i32   vcc, v79, v201\n" },
    { 0x7da592ffU, 0x40000000U, true, "        v_cmpx_eq_i32   vcc, 0x40000000, v201\n" },
    { 0x7da7934fU, 0, false, "        v_cmpx_le_i32   vcc, v79, v201\n" },
    { 0x7da792ffU, 0x40000000U, true, "        v_cmpx_le_i32   vcc, 0x40000000, v201\n" },
    { 0x7da9934fU, 0, false, "        v_cmpx_gt_i32   vcc, v79, v201\n" },
    { 0x7da992ffU, 0x40000000U, true, "        v_cmpx_gt_i32   vcc, 0x40000000, v201\n" },
    { 0x7dab934fU, 0, false, "        v_cmpx_lg_i32   vcc, v79, v201\n" },
    { 0x7dab92ffU, 0x40000000U, true, "        v_cmpx_lg_i32   vcc, 0x40000000, v201\n" },
    { 0x7dad934fU, 0, false, "        v_cmpx_ge_i32   vcc, v79, v201\n" },
    { 0x7dad92ffU, 0x40000000U, true, "        v_cmpx_ge_i32   vcc, 0x40000000, v201\n" },
    { 0x7daf934fU, 0, false, "        v_cmpx_tru_i32  vcc, v79, v201\n" },
    { 0x7daf92ffU, 0x40000000U, true, "        v_cmpx_tru_i32  vcc, 0x40000000, v201\n" },
    { 0x7db1934fU, 0, false, "        v_cmpx_f_u32    vcc, v79, v201\n" },
    { 0x7db192ffU, 0x40000000U, true, "        v_cmpx_f_u32    vcc, 0x40000000, v201\n" },
    { 0x7db3934fU, 0, false, "        v_cmpx_lt_u32   vcc, v79, v201\n" },
    { 0x7db392ffU, 0x40000000U, true, "        v_cmpx_lt_u32   vcc, 0x40000000, v201\n" },
    { 0x7db5934fU, 0, false, "        v_cmpx_eq_u32   vcc, v79, v201\n" },
    { 0x7db592ffU, 0x40000000U, true, "        v_cmpx_eq_u32   vcc, 0x40000000, v201\n" },
    { 0x7db7934fU, 0, false, "        v_cmpx_le_u32   vcc, v79, v201\n" },
    { 0x7db792ffU, 0x40000000U, true, "        v_cmpx_le_u32   vcc, 0x40000000, v201\n" },
    { 0x7db9934fU, 0, false, "        v_cmpx_gt_u32   vcc, v79, v201\n" },
    { 0x7db992ffU, 0x40000000U, true, "        v_cmpx_gt_u32   vcc, 0x40000000, v201\n" },
    { 0x7dbb934fU, 0, false, "        v_cmpx_lg_u32   vcc, v79, v201\n" },
    { 0x7dbb92ffU, 0x40000000U, true, "        v_cmpx_lg_u32   vcc, 0x40000000, v201\n" },
    { 0x7dbd934fU, 0, false, "        v_cmpx_ge_u32   vcc, v79, v201\n" },
    { 0x7dbd92ffU, 0x40000000U, true, "        v_cmpx_ge_u32   vcc, 0x40000000, v201\n" },
    { 0x7dbf934fU, 0, false, "        v_cmpx_tru_u32  vcc, v79, v201\n" },
    { 0x7dbf92ffU, 0x40000000U, true, "        v_cmpx_tru_u32  vcc, 0x40000000, v201\n" },
    
    { 0x7dc1934fU, 0, false, "        v_cmp_f_i64     vcc, v[79:80], v[201:202]\n" },
    { 0x7dc192ffU, 0x40000000U, true, "        v_cmp_f_i64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc3934fU, 0, false, "        v_cmp_lt_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc392ffU, 0x40000000U, true, "        v_cmp_lt_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc5934fU, 0, false, "        v_cmp_eq_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc592ffU, 0x40000000U, true, "        v_cmp_eq_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc7934fU, 0, false, "        v_cmp_le_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc792ffU, 0x40000000U, true, "        v_cmp_le_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc9934fU, 0, false, "        v_cmp_gt_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc992ffU, 0x40000000U, true, "        v_cmp_gt_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dcb934fU, 0, false, "        v_cmp_lg_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dcb92ffU, 0x40000000U, true, "        v_cmp_lg_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dcd934fU, 0, false, "        v_cmp_ge_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dcd92ffU, 0x40000000U, true, "        v_cmp_ge_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dcf934fU, 0, false, "        v_cmp_tru_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7dcf92ffU, 0x40000000U, true, "        v_cmp_tru_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dd1934fU, 0, false, "        v_cmp_f_u64     vcc, v[79:80], v[201:202]\n" },
    { 0x7dd192ffU, 0x40000000U, true, "        v_cmp_f_u64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dd3934fU, 0, false, "        v_cmp_lt_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dd392ffU, 0x40000000U, true, "        v_cmp_lt_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dd5934fU, 0, false, "        v_cmp_eq_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dd592ffU, 0x40000000U, true, "        v_cmp_eq_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dd7934fU, 0, false, "        v_cmp_le_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dd792ffU, 0x40000000U, true, "        v_cmp_le_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dd9934fU, 0, false, "        v_cmp_gt_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dd992ffU, 0x40000000U, true, "        v_cmp_gt_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ddb934fU, 0, false, "        v_cmp_lg_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7ddb92ffU, 0x40000000U, true, "        v_cmp_lg_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ddd934fU, 0, false, "        v_cmp_ge_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7ddd92ffU, 0x40000000U, true, "        v_cmp_ge_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ddf934fU, 0, false, "        v_cmp_tru_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ddf92ffU, 0x40000000U, true, "        v_cmp_tru_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    
    { 0x7de1934fU, 0, false, "        v_cmpx_f_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7de192ffU, 0x40000000U, true, "        v_cmpx_f_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de3934fU, 0, false, "        v_cmpx_lt_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de392ffU, 0x40000000U, true, "        v_cmpx_lt_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de5934fU, 0, false, "        v_cmpx_eq_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de592ffU, 0x40000000U, true, "        v_cmpx_eq_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de7934fU, 0, false, "        v_cmpx_le_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de792ffU, 0x40000000U, true, "        v_cmpx_le_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de9934fU, 0, false, "        v_cmpx_gt_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de992ffU, 0x40000000U, true, "        v_cmpx_gt_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7deb934fU, 0, false, "        v_cmpx_lg_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7deb92ffU, 0x40000000U, true, "        v_cmpx_lg_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ded934fU, 0, false, "        v_cmpx_ge_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ded92ffU, 0x40000000U, true, "        v_cmpx_ge_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7def934fU, 0, false, "        v_cmpx_tru_i64  vcc, v[79:80], v[201:202]\n" },
    { 0x7def92ffU, 0x40000000U, true, "        v_cmpx_tru_i64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7df1934fU, 0, false, "        v_cmpx_f_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7df192ffU, 0x40000000U, true, "        v_cmpx_f_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7df3934fU, 0, false, "        v_cmpx_lt_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7df392ffU, 0x40000000U, true, "        v_cmpx_lt_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7df5934fU, 0, false, "        v_cmpx_eq_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7df592ffU, 0x40000000U, true, "        v_cmpx_eq_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7df7934fU, 0, false, "        v_cmpx_le_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7df792ffU, 0x40000000U, true, "        v_cmpx_le_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7df9934fU, 0, false, "        v_cmpx_gt_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7df992ffU, 0x40000000U, true, "        v_cmpx_gt_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dfb934fU, 0, false, "        v_cmpx_lg_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7dfb92ffU, 0x40000000U, true, "        v_cmpx_lg_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dfd934fU, 0, false, "        v_cmpx_ge_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7dfd92ffU, 0x40000000U, true, "        v_cmpx_ge_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dff934fU, 0, false, "        v_cmpx_tru_u64  vcc, v[79:80], v[201:202]\n" },
    { 0x7dff92ffU, 0x40000000U, true, "        v_cmpx_tru_u64  "
                "vcc, 0x40000000, v[201:202]\n" },
    /* VOPC SDWA */
    { 0x7dbf92f9U, 0x3d003d, true, "        v_cmpx_tru_u32  vcc, sext(-abs(v61)), v201 "
         "dst_sel:byte0 src0_sel:word1 src1_sel:byte0\n" },
    { 0x7dbf92f9U, 0x383d003d, true, "        v_cmpx_tru_u32  vcc, sext(-abs(v61)), "
        "sext(-abs(v201)) dst_sel:byte0 src0_sel:word1 src1_sel:byte0\n" },
    /* VOPC DPP */
    { 0x7dff92faU, 0x3872be, true,
        "        v_cmpx_tru_u64  vcc, -abs(v[190:191]), v[201:202] "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    { 0x7dff92faU, 0xc872be, true,
        "        v_cmpx_tru_u64  vcc, v[190:191], -abs(v[201:202]) "
        "quad_perm:[2,0,3,1] bound_ctrl bank_mask:0 row_mask:0\n" },
    /* VOP3 encoding */
    { 0xd040002aU, 0x0002d732U, true, "        v_cmp_f_f32     s[42:43], v50, v107\n" },
    { 0xd040032aU, 0x0002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107)\n" },
    { 0xd040832aU, 0x0002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107) clamp\n" },
    { 0xd040832aU, 0x0802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107) mul:2 clamp\n" },
    { 0xd040832aU, 0x1002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107) mul:4 clamp\n" },
    { 0xd040832aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -abs(v107) div:2 clamp neg2\n" },
    { 0xd040832aU, 0xd802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), -abs(v107) div:2 clamp neg2\n" },
    { 0xd040832aU, 0xb802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), abs(v107) div:2 clamp neg2\n" },
    { 0xd040822aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -v50, -abs(v107) div:2 clamp neg2\n" },
    { 0xd040812aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -v107 div:2 clamp neg2\n" },
    { 0xd040012aU, 0x7002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -v107 mul:4\n" },
    { 0xd040812aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -v107 div:2 clamp neg2\n" },
    /* vop3 */
    { 0xd040006aU, 0x0002d732U, true, "        v_cmp_f_f32     vcc, v50, v107 vop3\n" },
    { 0xd040006aU, 0x0002d632U, true, "        v_cmp_f_f32     vcc, s50, v107 vop3\n" },
    /* no vop3 */
    { 0xd040806aU, 0x0002d632U, true, "        v_cmp_f_f32     vcc, s50, v107 clamp\n" },
    { 0xd040006aU, 0x0000d632U, true, "        v_cmp_f_f32     vcc, s50, vcc_hi\n" },
    { 0xd040002aU, 0x00aed732U, true, "        v_cmp_f_f32     "
                "s[42:43], v50, v107 vsrc2=0x2b\n" },
    { 0xd040006aU, 0x00aed732U, true, "        v_cmp_f_f32     "
                "vcc, v50, v107 vsrc2=0x2b\n" },
    { 0xd040006aU, 0x1002d732U, true, "        v_cmp_f_f32     vcc, v50, v107 mul:4\n" },
    { 0xd040006aU, 0x8002d732U, true, "        v_cmp_f_f32     vcc, v50, v107 neg2\n" },
    { 0xd040006aU, 0x4002d732U, true, "        v_cmp_f_f32     vcc, v50, -v107\n" },
    { 0xd040016aU, 0x0002d732U, true, "        v_cmp_f_f32     vcc, abs(v50), v107\n" },
    /// ??????
    { 0xd040002aU, 0x0001feffU, true, "        v_cmp_f_f32     s[42:43], 0x0, 0x0\n" },
    { 0xd04000ffU, 0x0001feffU, true, "        v_cmp_f_f32     0x0, 0x0, 0x0\n" },
    /*** VOP3 comparisons ***/
    { 0xd000002aU, 0x0002d732U, true, "        VOP3A_ill_0     s[42:43], v50, v107\n" },
    { 0xd001002aU, 0x0002d732U, true, "        VOP3A_ill_1     s[42:43], v50, v107\n" },
    { 0xd002002aU, 0x0002d732U, true, "        VOP3A_ill_2     s[42:43], v50, v107\n" },
    { 0xd003002aU, 0x0002d732U, true, "        VOP3A_ill_3     s[42:43], v50, v107\n" },
    { 0xd004002aU, 0x0002d732U, true, "        VOP3A_ill_4     s[42:43], v50, v107\n" },
    { 0xd005002aU, 0x0002d732U, true, "        VOP3A_ill_5     s[42:43], v50, v107\n" },
    { 0xd006002aU, 0x0002d732U, true, "        VOP3A_ill_6     s[42:43], v50, v107\n" },
    { 0xd007002aU, 0x0002d732U, true, "        VOP3A_ill_7     s[42:43], v50, v107\n" },
    { 0xd008002aU, 0x0002d732U, true, "        VOP3A_ill_8     s[42:43], v50, v107\n" },
    { 0xd009002aU, 0x0002d732U, true, "        VOP3A_ill_9     s[42:43], v50, v107\n" },
    { 0xd00a002aU, 0x0002d732U, true, "        VOP3A_ill_10    s[42:43], v50, v107\n" },
    { 0xd00b002aU, 0x0002d732U, true, "        VOP3A_ill_11    s[42:43], v50, v107\n" },
    { 0xd00c002aU, 0x0002d732U, true, "        VOP3A_ill_12    s[42:43], v50, v107\n" },
    { 0xd00d002aU, 0x0002d732U, true, "        VOP3A_ill_13    s[42:43], v50, v107\n" },
    { 0xd00e002aU, 0x0002d732U, true, "        VOP3A_ill_14    s[42:43], v50, v107\n" },
    { 0xd00f002aU, 0x0002d732U, true, "        VOP3A_ill_15    s[42:43], v50, v107\n" },
    
    { 0xd010002aU, 0x0002d732U, true, "        v_cmp_class_f32 s[42:43], v50, v107\n" },
    { 0xd011002aU, 0x0002d732U, true, "        v_cmpx_class_f32 s[42:43], v50, v107\n" },
    { 0xd012002aU, 0x0002d732U, true,
        "        v_cmp_class_f64 s[42:43], v[50:51], v[107:108]\n" },
    { 0xd013002aU, 0x0002d732U, true,
        "        v_cmpx_class_f64 s[42:43], v[50:51], v[107:108]\n" },
    { 0xd014002aU, 0x0002d732U, true, "        v_cmp_class_f16 s[42:43], v50, v107\n" },
    { 0xd015002aU, 0x0002d732U, true, "        v_cmpx_class_f16 s[42:43], v50, v107\n" },
    { 0xd016002aU, 0x0002d732U, true, "        VOP3A_ill_22    s[42:43], v50, v107\n" },
    { 0xd017002aU, 0x0002d732U, true, "        VOP3A_ill_23    s[42:43], v50, v107\n" },
    { 0xd018002aU, 0x0002d732U, true, "        VOP3A_ill_24    s[42:43], v50, v107\n" },
    { 0xd019002aU, 0x0002d732U, true, "        VOP3A_ill_25    s[42:43], v50, v107\n" },
    { 0xd01a002aU, 0x0002d732U, true, "        VOP3A_ill_26    s[42:43], v50, v107\n" },
    { 0xd01b002aU, 0x0002d732U, true, "        VOP3A_ill_27    s[42:43], v50, v107\n" },
    { 0xd01c002aU, 0x0002d732U, true, "        VOP3A_ill_28    s[42:43], v50, v107\n" },
    { 0xd01d002aU, 0x0002d732U, true, "        VOP3A_ill_29    s[42:43], v50, v107\n" },
    { 0xd01e002aU, 0x0002d732U, true, "        VOP3A_ill_30    s[42:43], v50, v107\n" },
    { 0xd01f002aU, 0x0002d732U, true, "        VOP3A_ill_31    s[42:43], v50, v107\n" },
    { 0xd020002aU, 0x0002d732U, true, "        v_cmp_f_f16     s[42:43], v50, v107\n" },
    { 0xd021002aU, 0x0002d732U, true, "        v_cmp_lt_f16    s[42:43], v50, v107\n" },
    { 0xd022002aU, 0x0002d732U, true, "        v_cmp_eq_f16    s[42:43], v50, v107\n" },
    { 0xd023002aU, 0x0002d732U, true, "        v_cmp_le_f16    s[42:43], v50, v107\n" },
    { 0xd024002aU, 0x0002d732U, true, "        v_cmp_gt_f16    s[42:43], v50, v107\n" },
    { 0xd025002aU, 0x0002d732U, true, "        v_cmp_lg_f16    s[42:43], v50, v107\n" },
    { 0xd026002aU, 0x0002d732U, true, "        v_cmp_ge_f16    s[42:43], v50, v107\n" },
    { 0xd027002aU, 0x0002d732U, true, "        v_cmp_o_f16     s[42:43], v50, v107\n" },
    { 0xd028002aU, 0x0002d732U, true, "        v_cmp_u_f16     s[42:43], v50, v107\n" },
    { 0xd029002aU, 0x0002d732U, true, "        v_cmp_nge_f16   s[42:43], v50, v107\n" },
    { 0xd02a002aU, 0x0002d732U, true, "        v_cmp_nlg_f16   s[42:43], v50, v107\n" },
    { 0xd02b002aU, 0x0002d732U, true, "        v_cmp_ngt_f16   s[42:43], v50, v107\n" },
    { 0xd02c002aU, 0x0002d732U, true, "        v_cmp_nle_f16   s[42:43], v50, v107\n" },
    { 0xd02d002aU, 0x0002d732U, true, "        v_cmp_neq_f16   s[42:43], v50, v107\n" },
    { 0xd02e002aU, 0x0002d732U, true, "        v_cmp_nlt_f16   s[42:43], v50, v107\n" },
    { 0xd02f002aU, 0x0002d732U, true, "        v_cmp_tru_f16   s[42:43], v50, v107\n" },
    { 0xd030002aU, 0x0002d732U, true, "        v_cmpx_f_f16    s[42:43], v50, v107\n" },
    { 0xd031002aU, 0x0002d732U, true, "        v_cmpx_lt_f16   s[42:43], v50, v107\n" },
    { 0xd032002aU, 0x0002d732U, true, "        v_cmpx_eq_f16   s[42:43], v50, v107\n" },
    { 0xd033002aU, 0x0002d732U, true, "        v_cmpx_le_f16   s[42:43], v50, v107\n" },
    { 0xd034002aU, 0x0002d732U, true, "        v_cmpx_gt_f16   s[42:43], v50, v107\n" },
    { 0xd035002aU, 0x0002d732U, true, "        v_cmpx_lg_f16   s[42:43], v50, v107\n" },
    { 0xd036002aU, 0x0002d732U, true, "        v_cmpx_ge_f16   s[42:43], v50, v107\n" },
    { 0xd037002aU, 0x0002d732U, true, "        v_cmpx_o_f16    s[42:43], v50, v107\n" },
    { 0xd038002aU, 0x0002d732U, true, "        v_cmpx_u_f16    s[42:43], v50, v107\n" },
    { 0xd039002aU, 0x0002d732U, true, "        v_cmpx_nge_f16  s[42:43], v50, v107\n" },
    { 0xd03a002aU, 0x0002d732U, true, "        v_cmpx_nlg_f16  s[42:43], v50, v107\n" },
    { 0xd03b002aU, 0x0002d732U, true, "        v_cmpx_ngt_f16  s[42:43], v50, v107\n" },
    { 0xd03c002aU, 0x0002d732U, true, "        v_cmpx_nle_f16  s[42:43], v50, v107\n" },
    { 0xd03d002aU, 0x0002d732U, true, "        v_cmpx_neq_f16  s[42:43], v50, v107\n" },
    { 0xd03e002aU, 0x0002d732U, true, "        v_cmpx_nlt_f16  s[42:43], v50, v107\n" },
    { 0xd03f002aU, 0x0002d732U, true, "        v_cmpx_tru_f16  s[42:43], v50, v107\n" },
    { 0xd040002aU, 0x0002d732U, true, "        v_cmp_f_f32     s[42:43], v50, v107\n" },
    { 0xd041002aU, 0x0002d732U, true, "        v_cmp_lt_f32    s[42:43], v50, v107\n" },
    { 0xd042002aU, 0x0002d732U, true, "        v_cmp_eq_f32    s[42:43], v50, v107\n" },
    { 0xd043002aU, 0x0002d732U, true, "        v_cmp_le_f32    s[42:43], v50, v107\n" },
    { 0xd044002aU, 0x0002d732U, true, "        v_cmp_gt_f32    s[42:43], v50, v107\n" },
    { 0xd045002aU, 0x0002d732U, true, "        v_cmp_lg_f32    s[42:43], v50, v107\n" },
    { 0xd046002aU, 0x0002d732U, true, "        v_cmp_ge_f32    s[42:43], v50, v107\n" },
    { 0xd047002aU, 0x0002d732U, true, "        v_cmp_o_f32     s[42:43], v50, v107\n" },
    { 0xd048002aU, 0x0002d732U, true, "        v_cmp_u_f32     s[42:43], v50, v107\n" },
    { 0xd049002aU, 0x0002d732U, true, "        v_cmp_nge_f32   s[42:43], v50, v107\n" },
    { 0xd04a002aU, 0x0002d732U, true, "        v_cmp_nlg_f32   s[42:43], v50, v107\n" },
    { 0xd04b002aU, 0x0002d732U, true, "        v_cmp_ngt_f32   s[42:43], v50, v107\n" },
    { 0xd04c002aU, 0x0002d732U, true, "        v_cmp_nle_f32   s[42:43], v50, v107\n" },
    { 0xd04d002aU, 0x0002d732U, true, "        v_cmp_neq_f32   s[42:43], v50, v107\n" },
    { 0xd04e002aU, 0x0002d732U, true, "        v_cmp_nlt_f32   s[42:43], v50, v107\n" },
    { 0xd04f002aU, 0x0002d732U, true, "        v_cmp_tru_f32   s[42:43], v50, v107\n" },
    { 0xd050002aU, 0x0002d732U, true, "        v_cmpx_f_f32    s[42:43], v50, v107\n" },
    { 0xd051002aU, 0x0002d732U, true, "        v_cmpx_lt_f32   s[42:43], v50, v107\n" },
    { 0xd052002aU, 0x0002d732U, true, "        v_cmpx_eq_f32   s[42:43], v50, v107\n" },
    { 0xd053002aU, 0x0002d732U, true, "        v_cmpx_le_f32   s[42:43], v50, v107\n" },
    { 0xd054002aU, 0x0002d732U, true, "        v_cmpx_gt_f32   s[42:43], v50, v107\n" },
    { 0xd055002aU, 0x0002d732U, true, "        v_cmpx_lg_f32   s[42:43], v50, v107\n" },
    { 0xd056002aU, 0x0002d732U, true, "        v_cmpx_ge_f32   s[42:43], v50, v107\n" },
    { 0xd057002aU, 0x0002d732U, true, "        v_cmpx_o_f32    s[42:43], v50, v107\n" },
    { 0xd058002aU, 0x0002d732U, true, "        v_cmpx_u_f32    s[42:43], v50, v107\n" },
    { 0xd059002aU, 0x0002d732U, true, "        v_cmpx_nge_f32  s[42:43], v50, v107\n" },
    { 0xd05a002aU, 0x0002d732U, true, "        v_cmpx_nlg_f32  s[42:43], v50, v107\n" },
    { 0xd05b002aU, 0x0002d732U, true, "        v_cmpx_ngt_f32  s[42:43], v50, v107\n" },
    { 0xd05c002aU, 0x0002d732U, true, "        v_cmpx_nle_f32  s[42:43], v50, v107\n" },
    { 0xd05d002aU, 0x0002d732U, true, "        v_cmpx_neq_f32  s[42:43], v50, v107\n" },
    { 0xd05e002aU, 0x0002d732U, true, "        v_cmpx_nlt_f32  s[42:43], v50, v107\n" },
    { 0xd05f002aU, 0x0002d732U, true, "        v_cmpx_tru_f32  s[42:43], v50, v107\n" },
    { 0xd060002aU, 0x0002d732U, true,
        "        v_cmp_f_f64     s[42:43], v[50:51], v[107:108]\n" },
    { 0xd061002aU, 0x0002d732U, true,
        "        v_cmp_lt_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd062002aU, 0x0002d732U, true,
        "        v_cmp_eq_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd063002aU, 0x0002d732U, true,
        "        v_cmp_le_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd064002aU, 0x0002d732U, true,
        "        v_cmp_gt_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd065002aU, 0x0002d732U, true,
        "        v_cmp_lg_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd066002aU, 0x0002d732U, true,
        "        v_cmp_ge_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd067002aU, 0x0002d732U, true,
        "        v_cmp_o_f64     s[42:43], v[50:51], v[107:108]\n" },
    { 0xd068002aU, 0x0002d732U, true,
        "        v_cmp_u_f64     s[42:43], v[50:51], v[107:108]\n" },
    { 0xd069002aU, 0x0002d732U, true,
        "        v_cmp_nge_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06a002aU, 0x0002d732U, true,
        "        v_cmp_nlg_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06b002aU, 0x0002d732U, true,
        "        v_cmp_ngt_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06c002aU, 0x0002d732U, true,
        "        v_cmp_nle_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06d002aU, 0x0002d732U, true,
        "        v_cmp_neq_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06e002aU, 0x0002d732U, true,
        "        v_cmp_nlt_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06f002aU, 0x0002d732U, true,
        "        v_cmp_tru_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd070002aU, 0x0002d732U, true,
        "        v_cmpx_f_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd071002aU, 0x0002d732U, true,
        "        v_cmpx_lt_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd072002aU, 0x0002d732U, true,
        "        v_cmpx_eq_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd073002aU, 0x0002d732U, true,
        "        v_cmpx_le_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd074002aU, 0x0002d732U, true,
        "        v_cmpx_gt_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd075002aU, 0x0002d732U, true,
        "        v_cmpx_lg_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd076002aU, 0x0002d732U, true,
        "        v_cmpx_ge_f64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd077002aU, 0x0002d732U, true,
        "        v_cmpx_o_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd078002aU, 0x0002d732U, true,
        "        v_cmpx_u_f64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd079002aU, 0x0002d732U, true,
        "        v_cmpx_nge_f64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07a002aU, 0x0002d732U, true,
        "        v_cmpx_nlg_f64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07b002aU, 0x0002d732U, true,
        "        v_cmpx_ngt_f64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07c002aU, 0x0002d732U, true,
        "        v_cmpx_nle_f64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07d002aU, 0x0002d732U, true,
        "        v_cmpx_neq_f64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07e002aU, 0x0002d732U, true,
        "        v_cmpx_nlt_f64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07f002aU, 0x0002d732U, true,
        "        v_cmpx_tru_f64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd080002aU, 0x0002d732U, true, "        VOP3A_ill_128   s[42:43], v50, v107\n" },
    
    { 0xd0a0002aU, 0x0002d732U, true, "        v_cmp_f_i16     s[42:43], v50, v107\n" },
    { 0xd0a1002aU, 0x0002d732U, true, "        v_cmp_lt_i16    s[42:43], v50, v107\n" },
    { 0xd0a2002aU, 0x0002d732U, true, "        v_cmp_eq_i16    s[42:43], v50, v107\n" },
    { 0xd0a3002aU, 0x0002d732U, true, "        v_cmp_le_i16    s[42:43], v50, v107\n" },
    { 0xd0a4002aU, 0x0002d732U, true, "        v_cmp_gt_i16    s[42:43], v50, v107\n" },
    { 0xd0a5002aU, 0x0002d732U, true, "        v_cmp_lg_i16    s[42:43], v50, v107\n" },
    { 0xd0a6002aU, 0x0002d732U, true, "        v_cmp_ge_i16    s[42:43], v50, v107\n" },
    { 0xd0a7002aU, 0x0002d732U, true, "        v_cmp_tru_i16   s[42:43], v50, v107\n" },
    { 0xd0a8002aU, 0x0002d732U, true, "        v_cmp_f_u16     s[42:43], v50, v107\n" },
    { 0xd0a9002aU, 0x0002d732U, true, "        v_cmp_lt_u16    s[42:43], v50, v107\n" },
    { 0xd0aa002aU, 0x0002d732U, true, "        v_cmp_eq_u16    s[42:43], v50, v107\n" },
    { 0xd0ab002aU, 0x0002d732U, true, "        v_cmp_le_u16    s[42:43], v50, v107\n" },
    { 0xd0ac002aU, 0x0002d732U, true, "        v_cmp_gt_u16    s[42:43], v50, v107\n" },
    { 0xd0ad002aU, 0x0002d732U, true, "        v_cmp_lg_u16    s[42:43], v50, v107\n" },
    { 0xd0ae002aU, 0x0002d732U, true, "        v_cmp_ge_u16    s[42:43], v50, v107\n" },
    { 0xd0af002aU, 0x0002d732U, true, "        v_cmp_tru_u16   s[42:43], v50, v107\n" },
    { 0xd0b0002aU, 0x0002d732U, true, "        v_cmpx_f_i16    s[42:43], v50, v107\n" },
    { 0xd0b1002aU, 0x0002d732U, true, "        v_cmpx_lt_i16   s[42:43], v50, v107\n" },
    { 0xd0b2002aU, 0x0002d732U, true, "        v_cmpx_eq_i16   s[42:43], v50, v107\n" },
    { 0xd0b3002aU, 0x0002d732U, true, "        v_cmpx_le_i16   s[42:43], v50, v107\n" },
    { 0xd0b4002aU, 0x0002d732U, true, "        v_cmpx_gt_i16   s[42:43], v50, v107\n" },
    { 0xd0b5002aU, 0x0002d732U, true, "        v_cmpx_lg_i16   s[42:43], v50, v107\n" },
    { 0xd0b6002aU, 0x0002d732U, true, "        v_cmpx_ge_i16   s[42:43], v50, v107\n" },
    { 0xd0b7002aU, 0x0002d732U, true, "        v_cmpx_tru_i16  s[42:43], v50, v107\n" },
    { 0xd0b8002aU, 0x0002d732U, true, "        v_cmpx_f_u16    s[42:43], v50, v107\n" },
    { 0xd0b9002aU, 0x0002d732U, true, "        v_cmpx_lt_u16   s[42:43], v50, v107\n" },
    { 0xd0ba002aU, 0x0002d732U, true, "        v_cmpx_eq_u16   s[42:43], v50, v107\n" },
    { 0xd0bb002aU, 0x0002d732U, true, "        v_cmpx_le_u16   s[42:43], v50, v107\n" },
    { 0xd0bc002aU, 0x0002d732U, true, "        v_cmpx_gt_u16   s[42:43], v50, v107\n" },
    { 0xd0bd002aU, 0x0002d732U, true, "        v_cmpx_lg_u16   s[42:43], v50, v107\n" },
    { 0xd0be002aU, 0x0002d732U, true, "        v_cmpx_ge_u16   s[42:43], v50, v107\n" },
    { 0xd0bf002aU, 0x0002d732U, true, "        v_cmpx_tru_u16  s[42:43], v50, v107\n" },
    { 0xd0c0002aU, 0x0002d732U, true, "        v_cmp_f_i32     s[42:43], v50, v107\n" },
    { 0xd0c1002aU, 0x0002d732U, true, "        v_cmp_lt_i32    s[42:43], v50, v107\n" },
    { 0xd0c2002aU, 0x0002d732U, true, "        v_cmp_eq_i32    s[42:43], v50, v107\n" },
    { 0xd0c3002aU, 0x0002d732U, true, "        v_cmp_le_i32    s[42:43], v50, v107\n" },
    { 0xd0c4002aU, 0x0002d732U, true, "        v_cmp_gt_i32    s[42:43], v50, v107\n" },
    { 0xd0c5002aU, 0x0002d732U, true, "        v_cmp_lg_i32    s[42:43], v50, v107\n" },
    { 0xd0c6002aU, 0x0002d732U, true, "        v_cmp_ge_i32    s[42:43], v50, v107\n" },
    { 0xd0c7002aU, 0x0002d732U, true, "        v_cmp_tru_i32   s[42:43], v50, v107\n" },
    { 0xd0c8002aU, 0x0002d732U, true, "        v_cmp_f_u32     s[42:43], v50, v107\n" },
    { 0xd0c9002aU, 0x0002d732U, true, "        v_cmp_lt_u32    s[42:43], v50, v107\n" },
    { 0xd0ca002aU, 0x0002d732U, true, "        v_cmp_eq_u32    s[42:43], v50, v107\n" },
    { 0xd0cb002aU, 0x0002d732U, true, "        v_cmp_le_u32    s[42:43], v50, v107\n" },
    { 0xd0cc002aU, 0x0002d732U, true, "        v_cmp_gt_u32    s[42:43], v50, v107\n" },
    { 0xd0cd002aU, 0x0002d732U, true, "        v_cmp_lg_u32    s[42:43], v50, v107\n" },
    { 0xd0ce002aU, 0x0002d732U, true, "        v_cmp_ge_u32    s[42:43], v50, v107\n" },
    { 0xd0cf002aU, 0x0002d732U, true, "        v_cmp_tru_u32   s[42:43], v50, v107\n" },
    { 0xd0d0002aU, 0x0002d732U, true, "        v_cmpx_f_i32    s[42:43], v50, v107\n" },
    { 0xd0d1002aU, 0x0002d732U, true, "        v_cmpx_lt_i32   s[42:43], v50, v107\n" },
    { 0xd0d2002aU, 0x0002d732U, true, "        v_cmpx_eq_i32   s[42:43], v50, v107\n" },
    { 0xd0d3002aU, 0x0002d732U, true, "        v_cmpx_le_i32   s[42:43], v50, v107\n" },
    { 0xd0d4002aU, 0x0002d732U, true, "        v_cmpx_gt_i32   s[42:43], v50, v107\n" },
    { 0xd0d5002aU, 0x0002d732U, true, "        v_cmpx_lg_i32   s[42:43], v50, v107\n" },
    { 0xd0d6002aU, 0x0002d732U, true, "        v_cmpx_ge_i32   s[42:43], v50, v107\n" },
    { 0xd0d7002aU, 0x0002d732U, true, "        v_cmpx_tru_i32  s[42:43], v50, v107\n" },
    { 0xd0d8002aU, 0x0002d732U, true, "        v_cmpx_f_u32    s[42:43], v50, v107\n" },
    { 0xd0d9002aU, 0x0002d732U, true, "        v_cmpx_lt_u32   s[42:43], v50, v107\n" },
    { 0xd0da002aU, 0x0002d732U, true, "        v_cmpx_eq_u32   s[42:43], v50, v107\n" },
    { 0xd0db002aU, 0x0002d732U, true, "        v_cmpx_le_u32   s[42:43], v50, v107\n" },
    { 0xd0dc002aU, 0x0002d732U, true, "        v_cmpx_gt_u32   s[42:43], v50, v107\n" },
    { 0xd0dd002aU, 0x0002d732U, true, "        v_cmpx_lg_u32   s[42:43], v50, v107\n" },
    { 0xd0de002aU, 0x0002d732U, true, "        v_cmpx_ge_u32   s[42:43], v50, v107\n" },
    { 0xd0df002aU, 0x0002d732U, true, "        v_cmpx_tru_u32  s[42:43], v50, v107\n" },
    { 0xd0e0002aU, 0x0002d732U, true,
        "        v_cmp_f_i64     s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e1002aU, 0x0002d732U, true,
        "        v_cmp_lt_i64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e2002aU, 0x0002d732U, true,
        "        v_cmp_eq_i64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e3002aU, 0x0002d732U, true,
        "        v_cmp_le_i64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e4002aU, 0x0002d732U, true,
        "        v_cmp_gt_i64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e5002aU, 0x0002d732U, true,
        "        v_cmp_lg_i64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e6002aU, 0x0002d732U, true,
        "        v_cmp_ge_i64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e7002aU, 0x0002d732U, true,
        "        v_cmp_tru_i64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e8002aU, 0x0002d732U, true,
        "        v_cmp_f_u64     s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e9002aU, 0x0002d732U, true,
        "        v_cmp_lt_u64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ea002aU, 0x0002d732U, true,
        "        v_cmp_eq_u64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0eb002aU, 0x0002d732U, true,
        "        v_cmp_le_u64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ec002aU, 0x0002d732U, true,
        "        v_cmp_gt_u64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ed002aU, 0x0002d732U, true,
        "        v_cmp_lg_u64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ee002aU, 0x0002d732U, true,
        "        v_cmp_ge_u64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ef002aU, 0x0002d732U, true,
        "        v_cmp_tru_u64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f0002aU, 0x0002d732U, true,
        "        v_cmpx_f_i64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f1002aU, 0x0002d732U, true,
        "        v_cmpx_lt_i64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f2002aU, 0x0002d732U, true,
        "        v_cmpx_eq_i64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f3002aU, 0x0002d732U, true,
        "        v_cmpx_le_i64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f4002aU, 0x0002d732U, true,
        "        v_cmpx_gt_i64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f5002aU, 0x0002d732U, true,
        "        v_cmpx_lg_i64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f6002aU, 0x0002d732U, true,
        "        v_cmpx_ge_i64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f7002aU, 0x0002d732U, true,
        "        v_cmpx_tru_i64  s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f8002aU, 0x0002d732U, true,
        "        v_cmpx_f_u64    s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f9002aU, 0x0002d732U, true,
        "        v_cmpx_lt_u64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0fa002aU, 0x0002d732U, true,
        "        v_cmpx_eq_u64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0fb002aU, 0x0002d732U, true,
        "        v_cmpx_le_u64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0fc002aU, 0x0002d732U, true,
        "        v_cmpx_gt_u64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0fd002aU, 0x0002d732U, true,
        "        v_cmpx_lg_u64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0fe002aU, 0x0002d732U, true,
        "        v_cmpx_ge_u64   s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ff002aU, 0x0002d732U, true,
        "        v_cmpx_tru_u64  s[42:43], v[50:51], v[107:108]\n" },
    /* VOP2 instructions encoded to VOP3a/VOP3b */
    { 0xd100002aU, 0x0002d732U, true, "        v_cndmask_b32   v42, v50, v107, s[0:1]\n" },
    { 0xd100002aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd100002aU, 0x003ed732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, s[15:16]\n" },
    { 0xd100002aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd100042aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd100002aU, 0x81aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    /* no vop3 */
    { 0xd100022aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, abs(v107), vcc\n" },
    { 0xd100002aU, 0x41aad732U, true, "        v_cndmask_b32   v42, v50, -v107, vcc\n" },
    { 0xd100012aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, abs(v50), v107, vcc\n" },
    { 0xd100002aU, 0x21aad732U, true, "        v_cndmask_b32   v42, -v50, v107, vcc\n" },
    { 0xd100802aU, 0x81aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc clamp\n" },
    { 0xd100002aU, 0x81aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd100002aU, 0x81a8c932U, true, "        v_cndmask_b32   "
                "v42, v50, s100, vcc\n" },
    /* v_add_f32 */
    { 0xd1010037U, 0x0002b41bU, true, "        v_add_f32       v55, s27, v90 vop3\n" },
    { 0xd1010037U, 0x0000b41bU, true, "        v_add_f32       v55, s27, s90\n" },
    { 0xd1018037U, 0x0002b41bU, true, "        v_add_f32       v55, s27, v90 clamp\n" },
    { 0xd1010037U, 0x4002b41bU, true, "        v_add_f32       v55, s27, -v90\n" },
    { 0xd1010237U, 0x4002b41bU, true, "        v_add_f32       v55, s27, -abs(v90)\n" },
    { 0xd1010437U, 0x0002b41bU, true, "        v_add_f32       v55, s27, v90 abs2\n" },
    /* other opcodes */
    { 0xd1020037U, 0x0002b41bU, true, "        v_sub_f32       v55, s27, v90 vop3\n" },
    { 0xd1030037U, 0x0002b41bU, true, "        v_subrev_f32    v55, s27, v90 vop3\n" },
    { 0xd1040037U, 0x0002b41bU, true, "        v_mul_legacy_f32 v55, s27, v90 vop3\n" },
    { 0xd1050037U, 0x0002b41bU, true, "        v_mul_f32       v55, s27, v90 vop3\n" },
    { 0xd1060037U, 0x0002b41bU, true, "        v_mul_i32_i24   v55, s27, v90 vop3\n" },
    { 0xd1070037U, 0x0002b41bU, true, "        v_mul_hi_i32_i24 v55, s27, v90 vop3\n" },
    { 0xd1080037U, 0x0002b41bU, true, "        v_mul_u32_u24   v55, s27, v90 vop3\n" },
    { 0xd1090037U, 0x0002b41bU, true, "        v_mul_hi_u32_u24 v55, s27, v90 vop3\n" },
    { 0xd10a0037U, 0x0002b41bU, true, "        v_min_f32       v55, s27, v90 vop3\n" },
    { 0xd10b0037U, 0x0002b41bU, true, "        v_max_f32       v55, s27, v90 vop3\n" },
    { 0xd10c0037U, 0x0002b41bU, true, "        v_min_i32       v55, s27, v90 vop3\n" },
    { 0xd10d0037U, 0x0002b41bU, true, "        v_max_i32       v55, s27, v90 vop3\n" },
    { 0xd10e0037U, 0x0002b41bU, true, "        v_min_u32       v55, s27, v90 vop3\n" },
    { 0xd10f0037U, 0x0002b41bU, true, "        v_max_u32       v55, s27, v90 vop3\n" },
    { 0xd1100037U, 0x0002b41bU, true, "        v_lshrrev_b32   v55, s27, v90 vop3\n" },
    { 0xd1110037U, 0x0002b41bU, true, "        v_ashrrev_i32   v55, s27, v90 vop3\n" },
    { 0xd1120037U, 0x0002b41bU, true, "        v_lshlrev_b32   v55, s27, v90 vop3\n" },
    { 0xd1130037U, 0x0002b41bU, true, "        v_and_b32       v55, s27, v90 vop3\n" },
    { 0xd1140037U, 0x0002b41bU, true, "        v_or_b32        v55, s27, v90 vop3\n" },
    { 0xd1150037U, 0x0002b41bU, true, "        v_xor_b32       v55, s27, v90 vop3\n" },
    { 0xd1160037U, 0x0002b41bU, true, "        v_mac_f32       v55, s27, v90 vop3\n" },
    { 0xd1170037U, 0x0002b41bU, true, "        v_madmk_f32     v55, s27, v90, s0 vop3\n" },
    { 0xd1180037U, 0x0002b41bU, true, "        v_madak_f32     v55, s27, v90, s0 vop3\n" },
    { 0xd1191237U, 0x0002b41bU, true,
        "        v_add_u32       v55, s[18:19], s27, v90\n" },
    { 0xd1196a37U, 0x0002b41bU, true,
        "        v_add_u32       v55, vcc, s27, v90 vop3\n" },
    { 0xd11a1237U, 0x0002b41bU, true,
        "        v_sub_u32       v55, s[18:19], s27, v90\n" },
    { 0xd11a6a37U, 0x0002b41bU, true,
        "        v_sub_u32       v55, vcc, s27, v90 vop3\n" },
    { 0xd11b1237U, 0x0002b41bU, true,
        "        v_subrev_u32    v55, s[18:19], s27, v90\n" },
    { 0xd11b6a37U, 0x0002b41bU, true,
        "        v_subrev_u32    v55, vcc, s27, v90 vop3\n" },
    { 0xd11c0737U, 0x4066b41bU, true, "        v_addc_u32      "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd11c6a37U, 0x01aab41bU, true, "        v_addc_u32      "
                "v55, vcc, s27, v90, vcc vop3\n" },
    { 0xd11c6a37U, 0x41aab41bU, true, "        v_addc_u32      "
                "v55, vcc, s27, -v90, vcc\n" },
    { 0xd11d0737U, 0x4066b41bU, true, "        v_subb_u32      "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd11e0737U, 0x4066b41bU, true, "        v_subbrev_u32   "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd11d6a0bU, 0x00021480U, true,   /* no vop3 case! */
                "        v_subb_u32      v11, vcc, 0, v10, s[0:1]\n" },
    { 0xd11f0037U, 0x0002b41bU, true, "        v_add_f16       v55, s27, v90 vop3\n" },
    { 0xd1200037U, 0x0002b41bU, true, "        v_sub_f16       v55, s27, v90 vop3\n" },
    { 0xd1210037U, 0x0002b41bU, true, "        v_subrev_f16    v55, s27, v90 vop3\n" },
    { 0xd1220037U, 0x0002b41bU, true, "        v_mul_f16       v55, s27, v90 vop3\n" },
    { 0xd1230037U, 0x0002b41bU, true, "        v_mac_f16       v55, s27, v90 vop3\n" },
    { 0xd1240037U, 0x0002b41bU, true, "        v_madmk_f16     v55, s27, v90, s0 vop3\n" },
    { 0xd1250037U, 0x0002b41bU, true, "        v_madak_f16     v55, s27, v90, s0 vop3\n" },
    { 0xd1260037U, 0x0002b41bU, true, "        v_add_u16       v55, s27, v90 vop3\n" },
    { 0xd1270037U, 0x0002b41bU, true, "        v_sub_u16       v55, s27, v90 vop3\n" },
    { 0xd1280037U, 0x0002b41bU, true, "        v_subrev_u16    v55, s27, v90 vop3\n" },
    { 0xd1290037U, 0x0002b41bU, true, "        v_mul_lo_u16    v55, s27, v90 vop3\n" },
    { 0xd12a0037U, 0x0002b41bU, true, "        v_lshlrev_b16   v55, s27, v90 vop3\n" },
    { 0xd12b0037U, 0x0002b41bU, true, "        v_lshrrev_b16   v55, s27, v90 vop3\n" },
    { 0xd12c0037U, 0x0002b41bU, true, "        v_ashrrev_i16   v55, s27, v90 vop3\n" },
    { 0xd12d0037U, 0x0002b41bU, true, "        v_max_f16       v55, s27, v90 vop3\n" },
    { 0xd12e0037U, 0x0002b41bU, true, "        v_min_f16       v55, s27, v90 vop3\n" },
    { 0xd12f0037U, 0x0002b41bU, true, "        v_max_u16       v55, s27, v90 vop3\n" },
    { 0xd1300037U, 0x0002b41bU, true, "        v_max_i16       v55, s27, v90 vop3\n" },
    { 0xd1310037U, 0x0002b41bU, true, "        v_min_u16       v55, s27, v90 vop3\n" },
    { 0xd1320037U, 0x0002b41bU, true, "        v_min_i16       v55, s27, v90 vop3\n" },
    { 0xd1330037U, 0x0002b41bU, true, "        v_ldexp_f16     v55, s27, v90 vop3\n" },
    { 0xd1340037U, 0x0002b41bU, true, "        VOP3A_ill_308   v55, s27, v90, s0\n" },
    { 0xd1350037U, 0x0002b41bU, true, "        VOP3A_ill_309   v55, s27, v90, s0\n" },
    { 0xd1360037U, 0x0002b41bU, true, "        VOP3A_ill_310   v55, s27, v90, s0\n" },
    { 0xd1370037U, 0x0002b41bU, true, "        VOP3A_ill_311   v55, s27, v90, s0\n" },
    /* VOP1 instructions encoded to VOP3a/VOP3b */
    { 0xd1400037U, 0x4002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a neg1\n" },
    { 0xd1400000U, 0x00000000U, true, "        v_nop           vop3\n" },
    { 0xd1400237U, 0x4002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a abs1 neg1\n" },
    { 0xd1400137U, 0x4002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b abs0 vsrc1=0x15a neg1\n" },
    { 0xd1400237U, 0xc01eb41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a abs1 neg1 vsrc2=0x7 neg2\n" },
    { 0xd1400037U, 0x0002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a\n" },
    { 0xd1400037U, 0x0000001bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vop3\n" },
    { 0xd1408037U, 0x0000001bU, true, "        v_nop           "
                "clamp dst=0x37 src0=0x1b\n" },
    /* v_mov_b32 and v_readfirstlane_b32 */
    { 0xd1410037U, 0x0000001bU, true, "        v_mov_b32       v55, s27 vop3\n" },
    { 0xd1410037U, 0x0000011bU, true, "        v_mov_b32       v55, v27 vop3\n" },
    { 0xd1418037U, 0x0000001bU, true, "        v_mov_b32       v55, s27 clamp\n" },
    { 0xd1410037U, 0x00001e1bU, true, "        v_mov_b32       v55, s27 vsrc1=0xf\n" },
    { 0xd1410037U, 0x003c001bU, true, "        v_mov_b32       v55, s27 vsrc2=0xf\n" },
    { 0xd1410737U, 0x003c001bU, true, "        v_mov_b32       "
                "v55, abs(s27) abs1 vsrc2=0xf abs2\n" },
    { 0xd1410037U, 0x1800001bU, true, "        v_mov_b32       v55, s27 div:2\n" },
    { 0xd1410037U, 0xf800001bU, true, "        v_mov_b32       "
                "v55, -s27 div:2 neg1 neg2\n" },
    { 0xd1410037U, 0x2000011bU, true, "        v_mov_b32       v55, -v27\n" },
    { 0xd1420037U, 0x2000001bU, true, "        v_readfirstlane_b32 s55, -s27\n" },
    { 0xd1420037U, 0x2000011bU, true, "        v_readfirstlane_b32 s55, -v27\n" },
    { 0xd1420037U, 0x0000001bU, true, "        v_readfirstlane_b32 s55, s27 vop3\n" },
    { 0xd1420037U, 0x0000011bU, true, "        v_readfirstlane_b32 s55, v27 vop3\n" },
    { 0xd1428037U, 0x0000001bU, true, "        v_readfirstlane_b32 s55, s27 clamp\n" },
    /* others */
    { 0xd1430037U, 0x0000001bU, true, "        v_cvt_i32_f64   v55, s[27:28] vop3\n" },
    { 0xd1430137U, 0x0000001bU, true, "        v_cvt_i32_f64   v55, abs(s[27:28])\n" },
    { 0xd1430037U, 0x0000011bU, true, "        v_cvt_i32_f64   v55, v[27:28] vop3\n" },
    { 0xd1430137U, 0x0000011bU, true, "        v_cvt_i32_f64   v55, abs(v[27:28])\n" },
    { 0xd1440037U, 0x0000011bU, true, "        v_cvt_f64_i32   v[55:56], v27 vop3\n" },
    { 0xd1440137U, 0x0000011bU, true, "        v_cvt_f64_i32   v[55:56], abs(v27)\n" },
    { 0xd1450037U, 0x0000011bU, true, "        v_cvt_f32_i32   v55, v27 vop3\n" },
    { 0xd1460037U, 0x0000011bU, true, "        v_cvt_f32_u32   v55, v27 vop3\n" },
    { 0xd1470037U, 0x0000011bU, true, "        v_cvt_u32_f32   v55, v27 vop3\n" },
    { 0xd1480037U, 0x0000011bU, true, "        v_cvt_i32_f32   v55, v27 vop3\n" },
    { 0xd1490037U, 0x0000011bU, true, "        v_mov_fed_b32   v55, v27 vop3\n" },
    { 0xd14a0037U, 0x0000011bU, true, "        v_cvt_f16_f32   v55, v27 vop3\n" },
    { 0xd14b0037U, 0x0000011bU, true, "        v_cvt_f32_f16   v55, v27 vop3\n" },
    { 0xd14c0037U, 0x0000011bU, true, "        v_cvt_rpi_i32_f32 v55, v27 vop3\n" },
    { 0xd14d0037U, 0x0000011bU, true, "        v_cvt_flr_i32_f32 v55, v27 vop3\n" },
    { 0xd14e0037U, 0x0000011bU, true, "        v_cvt_off_f32_i4 v55, v27 vop3\n" },
    { 0xd14f0037U, 0x0000011bU, true, "        v_cvt_f32_f64   v55, v[27:28] vop3\n" },
    { 0xd1500037U, 0x0000011bU, true, "        v_cvt_f64_f32   v[55:56], v27 vop3\n" },
    { 0xd1510037U, 0x0000011bU, true, "        v_cvt_f32_ubyte0 v55, v27 vop3\n" },
    { 0xd1520037U, 0x0000011bU, true, "        v_cvt_f32_ubyte1 v55, v27 vop3\n" },
    { 0xd1530037U, 0x0000011bU, true, "        v_cvt_f32_ubyte2 v55, v27 vop3\n" },
    { 0xd1540037U, 0x0000011bU, true, "        v_cvt_f32_ubyte3 v55, v27 vop3\n" },
    { 0xd1550037U, 0x0000011bU, true, "        v_cvt_u32_f64   v55, v[27:28] vop3\n" },
    { 0xd1560037U, 0x0000011bU, true, "        v_cvt_f64_u32   v[55:56], v27 vop3\n" },
    { 0xd1570037U, 0x0000011bU, true, "        v_trunc_f64     v[55:56], v[27:28] vop3\n" },
    { 0xd1580037U, 0x0000011bU, true, "        v_ceil_f64      v[55:56], v[27:28] vop3\n" },
    { 0xd1590037U, 0x0000011bU, true, "        v_rndne_f64     v[55:56], v[27:28] vop3\n" },
    { 0xd15a0037U, 0x0000011bU, true, "        v_floor_f64     v[55:56], v[27:28] vop3\n" },
    { 0xd15b0037U, 0x0000011bU, true, "        v_fract_f32     v55, v27 vop3\n" },
    { 0xd15c0037U, 0x0000011bU, true, "        v_trunc_f32     v55, v27 vop3\n" },
    { 0xd15d0037U, 0x0000011bU, true, "        v_ceil_f32      v55, v27 vop3\n" },
    { 0xd15e0037U, 0x0000011bU, true, "        v_rndne_f32     v55, v27 vop3\n" },
    { 0xd15f0037U, 0x0000011bU, true, "        v_floor_f32     v55, v27 vop3\n" },
    { 0xd1600037U, 0x0000011bU, true, "        v_exp_f32       v55, v27 vop3\n" },
    { 0xd1610037U, 0x0000011bU, true, "        v_log_f32       v55, v27 vop3\n" },
    { 0xd1620037U, 0x0000011bU, true, "        v_rcp_f32       v55, v27 vop3\n" },
    { 0xd1630037U, 0x0000011bU, true, "        v_rcp_iflag_f32 v55, v27 vop3\n" },
    { 0xd1640037U, 0x0000011bU, true, "        v_rsq_f32       v55, v27 vop3\n" },
    { 0xd1650037U, 0x0000011bU, true, "        v_rcp_f64       v[55:56], v[27:28] vop3\n" },
    { 0xd1660037U, 0x0000011bU, true, "        v_rsq_f64       v[55:56], v[27:28] vop3\n" },
    { 0xd1670037U, 0x0000011bU, true, "        v_sqrt_f32      v55, v27 vop3\n" },
    { 0xd1680037U, 0x0000011bU, true, "        v_sqrt_f64      v[55:56], v[27:28] vop3\n" },
    { 0xd1690037U, 0x0000011bU, true, "        v_sin_f32       v55, v27 vop3\n" },
    { 0xd16a0037U, 0x0000011bU, true, "        v_cos_f32       v55, v27 vop3\n" },
    { 0xd16b0037U, 0x0000011bU, true, "        v_not_b32       v55, v27 vop3\n" },
    { 0xd16c0037U, 0x0000011bU, true, "        v_bfrev_b32     v55, v27 vop3\n" },
    { 0xd16d0037U, 0x0000011bU, true, "        v_ffbh_u32      v55, v27 vop3\n" },
    { 0xd16e0037U, 0x0000011bU, true, "        v_ffbl_b32      v55, v27 vop3\n" },
    { 0xd16f0037U, 0x0000011bU, true, "        v_ffbh_i32      v55, v27 vop3\n" },
    { 0xd1700037U, 0x0000011bU, true, "        v_frexp_exp_i32_f64 v55, v[27:28] vop3\n" },
    { 0xd1710037U, 0x0000011bU, true,
        "        v_frexp_mant_f64 v[55:56], v[27:28] vop3\n" },
    { 0xd1720037U, 0x0000011bU, true, "        v_fract_f64     v[55:56], v[27:28] vop3\n" },
    { 0xd1730037U, 0x0000011bU, true, "        v_frexp_exp_i32_f32 v55, v27 vop3\n" },
    { 0xd1740037U, 0x0000011bU, true, "        v_frexp_mant_f32 v55, v27 vop3\n" },
    { 0xd1750037U, 0x0000011bU, true, "        v_clrexcp       "
                "dst=0x37 src0=0x11b vop3\n" },
    { 0xd1750000U, 0x00000000U, true, "        v_clrexcp       vop3\n" },
    { 0xd1760037U, 0x0000011bU, true, "        v_movreld_b32   v55, v27 vop3\n" },
    { 0xd1770037U, 0x0000011bU, true, "        v_movrels_b32   v55, v27 vop3\n" },
    { 0xd1780037U, 0x0000011bU, true, "        v_movrelsd_b32  v55, v27 vop3\n" },
    { 0xd1790037U, 0x0000011bU, true, "        v_cvt_f16_u16   v55, v27 vop3\n" },
    { 0xd17a0037U, 0x0000011bU, true, "        v_cvt_f16_i16   v55, v27 vop3\n" },
    { 0xd17b0037U, 0x0000011bU, true, "        v_cvt_u16_f16   v55, v27 vop3\n" },
    { 0xd17c0037U, 0x0000011bU, true, "        v_cvt_i16_f16   v55, v27 vop3\n" },
    { 0xd17d0037U, 0x0000011bU, true, "        v_rcp_f16       v55, v27 vop3\n" },
    { 0xd17e0037U, 0x0000011bU, true, "        v_sqrt_f16      v55, v27 vop3\n" },
    { 0xd17f0037U, 0x0000011bU, true, "        v_rsq_f16       v55, v27 vop3\n" },
    { 0xd1800037U, 0x0000011bU, true, "        v_log_f16       v55, v27 vop3\n" },
    { 0xd1810037U, 0x0000011bU, true, "        v_exp_f16       v55, v27 vop3\n" },
    { 0xd1820037U, 0x0000011bU, true, "        v_frexp_mant_f16 v55, v27 vop3\n" },
    { 0xd1830037U, 0x0000011bU, true, "        v_frexp_exp_i16_f16 v55, v27 vop3\n" },
    { 0xd1840037U, 0x0000011bU, true, "        v_floor_f16     v55, v27 vop3\n" },
    { 0xd1850037U, 0x0000011bU, true, "        v_ceil_f16      v55, v27 vop3\n" },
    { 0xd1860037U, 0x0000011bU, true, "        v_trunc_f16     v55, v27 vop3\n" },
    { 0xd1870037U, 0x0000011bU, true, "        v_rndne_f16     v55, v27 vop3\n" },
    { 0xd1880037U, 0x0000011bU, true, "        v_fract_f16     v55, v27 vop3\n" },
    { 0xd1890037U, 0x0000011bU, true, "        v_sin_f16       v55, v27 vop3\n" },
    { 0xd18a0037U, 0x0000011bU, true, "        v_cos_f16       v55, v27 vop3\n" },
    { 0xd18b0037U, 0x0000011bU, true, "        v_exp_legacy_f32 v55, v27 vop3\n" },
    { 0xd18c0037U, 0x0000011bU, true, "        v_log_legacy_f32 v55, v27 vop3\n" },
    { 0xd18d0037U, 0x0000011bU, true, "        VOP3A_ill_397   v55, v27, s0, s0\n" },
    { 0xd18e0037U, 0x0000011bU, true, "        VOP3A_ill_398   v55, v27, s0, s0\n" },
    { 0xd18f0037U, 0x0000011bU, true, "        VOP3A_ill_399   v55, v27, s0, s0\n" },
    { 0xd1900037U, 0x0000011bU, true, "        VOP3A_ill_400   v55, v27, s0, s0\n" },
    { 0xd1910037U, 0x0000011bU, true, "        VOP3A_ill_401   v55, v27, s0, s0\n" },
    { 0xd1920037U, 0x0000011bU, true, "        VOP3A_ill_402   v55, v27, s0, s0\n" },
    /* VOP3 only encoding */
    { 0xd1c00037U, 0x07974d4fU, true, "        v_mad_legacy_f32 v55, v79, v166, v229\n" },
    { 0xd1c08437U, 0x07974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, v166, abs(v229) clamp\n" },
    { 0xd1c08437U, 0x87974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, v166, -abs(v229) clamp\n" },
    { 0xd1c08437U, 0x97974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, v166, -abs(v229) mul:4 clamp\n" },
    { 0xd1c08637U, 0x47974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, -abs(v166), abs(v229) clamp\n" },
    { 0xd1c08537U, 0xb7974d4fU, true, "        v_mad_legacy_f32 "
                "v55, -abs(v79), v166, -abs(v229) mul:4 clamp\n" },
    { 0xd1c08537U, 0xb7974d4fU, true, "        v_mad_legacy_f32 "
                "v55, -abs(v79), v166, -abs(v229) mul:4 clamp\n" },
    { 0xd1c00037U, 0x03954c4fU, true, "        v_mad_legacy_f32 v55, s79, 38, ill_229\n" },
    { 0xd1c10037U, 0x07974d4fU, true, "        v_mad_f32       v55, v79, v166, v229\n" },
    { 0xd1c20037U, 0x07974d4fU, true, "        v_mad_i32_i24   v55, v79, v166, v229\n" },
    { 0xd1c30037U, 0x07974d4fU, true, "        v_mad_u32_u24   v55, v79, v166, v229\n" },
    { 0xd1c40037U, 0x07974d4fU, true, "        v_cubeid_f32    v55, v79, v166, v229\n" },
    { 0xd1c50037U, 0x07974d4fU, true, "        v_cubesc_f32    v55, v79, v166, v229\n" },
    { 0xd1c60037U, 0x07974d4fU, true, "        v_cubetc_f32    v55, v79, v166, v229\n" },
    { 0xd1c70037U, 0x07974d4fU, true, "        v_cubema_f32    v55, v79, v166, v229\n" },
    { 0xd1c80037U, 0x07974d4fU, true, "        v_bfe_u32       v55, v79, v166, v229\n" },
    { 0xd1c90037U, 0x07974d4fU, true, "        v_bfe_i32       v55, v79, v166, v229\n" },
    { 0xd1ca0037U, 0x07974d4fU, true, "        v_bfi_b32       v55, v79, v166, v229\n" },
    { 0xd1cb0037U, 0x07974d4fU, true, "        v_fma_f32       v55, v79, v166, v229\n" },
    { 0xd1cc0037U, 0x07974d4fU, true, "        v_fma_f64       "
                "v[55:56], v[79:80], v[166:167], v[229:230]\n" },
    { 0xd1cd0037U, 0x07974d4fU, true, "        v_lerp_u8       v55, v79, v166, v229\n" },
    { 0xd1ce0037U, 0x07974d4fU, true, "        v_alignbit_b32  v55, v79, v166, v229\n" },
    { 0xd1cf0037U, 0x07974d4fU, true, "        v_alignbyte_b32 v55, v79, v166, v229\n" },
    { 0xd1d00037U, 0x07974d4fU, true, "        v_min3_f32      v55, v79, v166, v229\n" },
    { 0xd1d10037U, 0x07974d4fU, true, "        v_min3_i32      v55, v79, v166, v229\n" },
    { 0xd1d20037U, 0x07974d4fU, true, "        v_min3_u32      v55, v79, v166, v229\n" },
    { 0xd1d30037U, 0x07974d4fU, true, "        v_max3_f32      v55, v79, v166, v229\n" },
    { 0xd1d40037U, 0x07974d4fU, true, "        v_max3_i32      v55, v79, v166, v229\n" },
    { 0xd1d50037U, 0x07974d4fU, true, "        v_max3_u32      v55, v79, v166, v229\n" },
    { 0xd1d60037U, 0x07974d4fU, true, "        v_med3_f32      v55, v79, v166, v229\n" },
    { 0xd1d70037U, 0x07974d4fU, true, "        v_med3_i32      v55, v79, v166, v229\n" },
    { 0xd1d80037U, 0x07974d4fU, true, "        v_med3_u32      v55, v79, v166, v229\n" },
    { 0xd1d90037U, 0x07974d4fU, true, "        v_sad_u8        v55, v79, v166, v229\n" },
    { 0xd1da0037U, 0x07974d4fU, true, "        v_sad_hi_u8     v55, v79, v166, v229\n" },
    { 0xd1db0037U, 0x07974d4fU, true, "        v_sad_u16       v55, v79, v166, v229\n" },
    { 0xd1dc0037U, 0x07974d4fU, true, "        v_sad_u32       v55, v79, v166, v229\n" },
    { 0xd1dd0037U, 0x07974d4fU, true, "        v_cvt_pk_u8_f32 v55, v79, v166, v229\n" },
    { 0xd1de0037U, 0x07974d4fU, true, "        v_div_fixup_f32 v55, v79, v166, v229\n" },
    { 0xd1df0037U, 0x07974d4fU, true, "        v_div_fixup_f64 "
                "v[55:56], v[79:80], v[166:167], v[229:230]\n" },
    /* VOP3b */
    { 0xd1e02537U, 0x07974d4fU, true, "        v_div_scale_f32 "
                "v55, s[37:38], v79, v166, v229\n" },
    { 0xd1e06a37U, 0x00034d4fU, true, "        v_div_scale_f32 "
                "v55, vcc, v79, v166, s0\n" },
    { 0xd1e12537U, 0x07974d4fU, true, "        v_div_scale_f64 "
                "v[55:56], s[37:38], v[79:80], v[166:167], v[229:230]\n" },
    /* rest of VOP3a */
    { 0xd1e20037U, 0x07974d4fU, true, "        v_div_fmas_f32  v55, v79, v166, v229\n" },
    { 0xd1e30037U, 0x07974d4fU, true, "        v_div_fmas_f64  "
                "v[55:56], v[79:80], v[166:167], v[229:230]\n" },
    { 0xd1e40037U, 0x07974d4fU, true, "        v_msad_u8       v55, v79, v166, v229\n" },
    { 0xd1e50037U, 0x07974d4fU, true, "        v_qsad_pk_u16_u8 "
                "v[55:56], v[79:80], v166, v[229:230]\n" },
    { 0xd1e60037U, 0x07974d4fU, true, "        v_mqsad_pk_u16_u8 "
                "v[55:56], v[79:80], v166, v[229:230]\n" },
    { 0xd1e70037U, 0x07974d4fU, true, "        v_mqsad_u32_u8  "
                "v[55:58], v[79:80], v166, v[229:232]\n" },
    { 0xd1e82f37U, 0x07974d4fU, true, "        v_mad_u64_u32   "
                "v[55:56], s[47:48], v79, v166, v[229:230]\n" },
    { 0xd1e92f37U, 0x07974d4fU, true, "        v_mad_i64_i32   "
                "v[55:56], s[47:48], v79, v166, v[229:230]\n" },
    { 0xd1ea0037U, 0x07974d4fU, true, "        v_mad_f16       v55, v79, v166, v229\n" },
    { 0xd1eb0037U, 0x07974d4fU, true, "        v_mad_u16       v55, v79, v166, v229\n" },
    { 0xd1ec0037U, 0x07974d4fU, true, "        v_mad_i16       v55, v79, v166, v229\n" },
    { 0xd1ed0037U, 0x07974d4fU, true, "        v_perm_b32      v55, v79, v166, v229\n" },
    { 0xd1ee0037U, 0x07974d4fU, true, "        v_fma_f16       v55, v79, v166, v229\n" },
    { 0xd1ef0037U, 0x07974d4fU, true, "        v_div_fixup_f16 v55, v79, v166, v229\n" },
    { 0xd1f0002aU, 0x049ad732U, true,
        "        v_cvt_pkaccum_u8_f32 v42, v50, v107 vsrc2=0x126\n" },
    { 0xd1f10037U, 0x0002b41bU, true, "        VOP3A_ill_497   v55, s27, v90, s0\n" },
    { 0xd1f20037U, 0x0002b41bU, true, "        VOP3A_ill_498   v55, s27, v90, s0\n" },
    { 0xd1f30037U, 0x0002b41bU, true, "        VOP3A_ill_499   v55, s27, v90, s0\n" },
    { 0xd1f40037U, 0x0002b41bU, true, "        VOP3A_ill_500   v55, s27, v90, s0\n" },
    /* VOP3 VINTRP encoding */
    { 0xd270002aU, 0x000220a7U, true,
        "        v_interp_p1_f32 v42, v16, attr39.z vop3\n" },
    { 0xd270002aU, 0x000221a7U, true,
        "        v_interp_p1_f32 v42, v16, attr39.z high\n" },
    { 0xd270002aU, 0x000220f7U, true,
        "        v_interp_p1_f32 v42, v16, attr55.w vop3\n" },
    { 0xd270002aU, 0x00022017U, true,
        "        v_interp_p1_f32 v42, v16, attr23.x vop3\n" },
    { 0xd270805dU, 0x000220a7U, true,
        "        v_interp_p1_f32 v93, v16, attr39.z clamp\n" },
    { 0xd270805dU, 0x0002206dU, true,
        "        v_interp_p1_f32 v93, v16, attr45.y clamp\n" },
    { 0xd270005dU, 0x0802206dU, true,
        "        v_interp_p1_f32 v93, v16, attr45.y mul:2\n" },
    { 0xd270002aU, 0x002e20a7U, true,
        "        v_interp_p1_f32 v42, v16, attr39.z vsrc2=0xb\n" },
    { 0xd270002aU, 0x000026a7U, true, "        v_interp_p1_f32 v42, s19, attr39.z\n" },
    { 0xd270022aU, 0x400026a7U, true,
        "        v_interp_p1_f32 v42, -abs(s19), attr39.z\n" },
    { 0xd270022aU, 0x400220a7U, true,
        "        v_interp_p1_f32 v42, -abs(v16), attr39.z\n" },
    { 0xd270022aU, 0x000220a7U, true,
        "        v_interp_p1_f32 v42, abs(v16), attr39.z\n" },
    { 0xd270022aU, 0x400220a7U, true,
        "        v_interp_p1_f32 v42, -abs(v16), attr39.z\n" },
    /* other VINTRP opcodes */
    { 0xd271002aU, 0x00022ca7U, true,
        "        v_interp_p2_f32 v42, v22, attr39.z vop3\n" },
    { 0xd272002aU, 0x000000a7U, true,
        "        v_interp_mov_f32 v42, p10, attr39.z vop3\n" },
    { 0xd272002aU, 0x000002a7U, true,
        "        v_interp_mov_f32 v42, p20, attr39.z vop3\n" },
    { 0xd272002aU, 0x000004a7U, true,
        "        v_interp_mov_f32 v42, p0, attr39.z vop3\n" },
    { 0xd272002aU, 0x00022ca7U, true,
        "        v_interp_mov_f32 v42, invalid_278, attr39.z\n" },
    { 0xd2730037U, 0x0002b41bU, true, "        VOP3A_ill_627   v55, s27, v90, s0\n" },
    { 0xd274802aU, 0x00022ca7U, true,   /* no vop3 */
        "        v_interp_p1ll_f16 v42, v22, attr39.z clamp\n" },
    { 0xd274002aU, 0x000220a7U, true,   /* no vop3 */
        "        v_interp_p1ll_f16 v42, v16, attr39.z\n" },
    { 0xd275002aU, 0x007402a7, true,
        "        v_interp_p1lv_f16 v42, s1, attr39.z, s29\n" },
    { 0xd275002aU, 0x007403a7, true,
        "        v_interp_p1lv_f16 v42, s1, attr39.z, s29 high\n" },
    { 0xd276002aU, 0x007402a7, true,
        "        v_interp_p2_f16 v42, s1, attr39.z, s29\n" },
    { 0xd276002aU, 0x007403a7, true,
        "        v_interp_p2_f16 v42, s1, attr39.z, s29 high\n" },
    { 0xd275022aU, 0xc07402a7, true,
        "        v_interp_p1lv_f16 v42, -abs(s1), attr39.z, -s29\n" },
    /* others */
    { 0xd2800037U, 0x00034d4fU, true, "        v_add_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2810037U, 0x00034d4fU, true, "        v_mul_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2820037U, 0x00034d4fU, true, "        v_min_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2830037U, 0x00034d4fU, true, "        v_max_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2840037U, 0x00034d4fU, true, "        v_ldexp_f64     "
                "v[55:56], v[79:80], v166\n" },
    { 0xd2850037U, 0x00034d4fU, true, "        v_mul_lo_u32    v55, v79, v166\n" },
    { 0xd2860037U, 0x00034d4fU, true, "        v_mul_hi_u32    v55, v79, v166\n" },
    { 0xd2870037U, 0x00034d4fU, true, "        v_mul_hi_i32    v55, v79, v166\n" },
    { 0xd2880037U, 0x4002b41bU, true, "        v_ldexp_f32     v55, s27, -v90\n" },
    { 0xd2890037U, 0x0002b51bU, true, "        v_readlane_b32  s55, v27, v90\n" },
    { 0xd28a0037U, 0x0002b51bU, true, "        v_writelane_b32 v55, v27, v90\n" },
    { 0xd28a0037U, 0x0002b41bU, true, "        v_writelane_b32 v55, s27, v90\n" },
    { 0xd28b0037U, 0x4002b41bU, true, "        v_bcnt_u32_b32  v55, s27, -v90\n" },
    { 0xd28c0037U, 0x4002b41bU, true, "        v_mbcnt_lo_u32_b32 v55, s27, -v90\n" },
    { 0xd28d0037U, 0x4002b41bU, true, "        v_mbcnt_hi_u32_b32 v55, s27, -v90\n" },
    { 0xd28e0037U, 0x4002b41bU, true, "        v_mac_legacy_f32 v55, s27, -v90\n" },
    { 0xd28f0037U, 0x00034d4fU, true,
                "        v_lshlrev_b64   v[55:56], v79, v[166:167]\n" },
    { 0xd2900037U, 0x00034d4fU, true,
                "        v_lshrrev_b64   v[55:56], v79, v[166:167]\n" },
    { 0xd2910037U, 0x00034d4fU, true,
                "        v_ashrrev_i64   v[55:56], v79, v[166:167]\n" },
    { 0xd2920037U, 0x07974d4fU, true, "        v_trig_preop_f64 "
                "v[55:56], v[79:80], v166 vsrc2=0x1e5\n" },
    { 0xd2930037U, 0x4002b41bU, true, "        v_bfm_b32       v55, s27, -v90\n" },
    { 0xd2940037U, 0x4002b41bU, true, "        v_cvt_pknorm_i16_f32 v55, s27, -v90\n" },
    { 0xd2950037U, 0x4002b41bU, true, "        v_cvt_pknorm_u16_f32 v55, s27, -v90\n" },
    { 0xd2960037U, 0x4002b41bU, true, "        v_cvt_pkrtz_f16_f32 v55, s27, -v90\n" },
    { 0xd2970037U, 0x4002b41bU, true, "        v_cvt_pk_u16_u32 v55, s27, -v90\n" },
    { 0xd2980037U, 0x4002b41bU, true, "        v_cvt_pk_i16_i32 v55, s27, -v90\n" },
    { 0xd2990037U, 0x0002b41bU, true, "        VOP3A_ill_665   v55, s27, v90, s0\n" },
    { 0xd29a0037U, 0x0002b41bU, true, "        VOP3A_ill_666   v55, s27, v90, s0\n" },
    { 0xd29b0037U, 0x0002b41bU, true, "        VOP3A_ill_667   v55, s27, v90, s0\n" },
    { 0xd29c0037U, 0x0002b41bU, true, "        VOP3A_ill_668   v55, s27, v90, s0\n" },
    /* VINTRP */
    { 0xd5746bd3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.w\n" },
    { 0xd5746ad3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.z\n" },
    { 0xd57469d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.y\n" },
    { 0xd57468d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.x\n" },
    { 0xd57400d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr0.x\n" },
    { 0xd57428d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr10.x\n" },
    { 0xd5743cd3U, 0, false, "        v_interp_p1_f32 v93, v211, attr15.x\n" },
    { 0xd5756bd3U, 0, false, "        v_interp_p2_f32 v93, v211, attr26.w\n" },
    { 0xd5766b00U, 0, false, "        v_interp_mov_f32 v93, p10, attr26.w\n" },
    { 0xd5766b01U, 0, false, "        v_interp_mov_f32 v93, p20, attr26.w\n" },
    { 0xd5766b02U, 0, false, "        v_interp_mov_f32 v93, p0, attr26.w\n" },
    { 0xd5766bd3U, 0, false, "        v_interp_mov_f32 v93, invalid_211, attr26.w\n" },
    { 0xd574f4d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr61.x\n" },
    { 0xd5776bd3U, 0, false, "        VINTRP_ill_3    v93, v211, attr26.w\n" },
    /* DS encoding */
    { 0xd800cd67U, 0x8b27a947U, true, "        ds_add_u32      "
                "v71, v169 offset:52583 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8000000U, 0x8b27a947U, true, "        ds_add_u32      "
                "v71, v169 vdata1=0x27 vdst=0x8b\n" },
    { 0xd800cd67U, 0x0000a947U, true, "        ds_add_u32      v71, v169 offset:52583\n" },
    { 0xd801cd67U, 0x8b27a947U, true, "        ds_add_u32      "
                "v71, v169 offset:52583 gds vdata1=0x27 vdst=0x8b\n" },
    { 0xd801cd67U, 0x0000a947U, true, "        ds_add_u32      "
                "v71, v169 offset:52583 gds\n" },
    { 0xd802cd67U, 0x0000a947U, true, "        ds_sub_u32      v71, v169 offset:52583\n" },
    { 0xd804cd67U, 0x0000a947U, true, "        ds_rsub_u32     v71, v169 offset:52583\n" },
    { 0xd806cd67U, 0x0000a947U, true, "        ds_inc_u32      v71, v169 offset:52583\n" },
    { 0xd808cd67U, 0x0000a947U, true, "        ds_dec_u32      v71, v169 offset:52583\n" },
    { 0xd80acd67U, 0x0000a947U, true, "        ds_min_i32      v71, v169 offset:52583\n" },
    { 0xd80ccd67U, 0x0000a947U, true, "        ds_max_i32      v71, v169 offset:52583\n" },
    { 0xd80ecd67U, 0x0000a947U, true, "        ds_min_u32      v71, v169 offset:52583\n" },
    { 0xd810cd67U, 0x0000a947U, true, "        ds_max_u32      v71, v169 offset:52583\n" },
    { 0xd812cd67U, 0x0000a947U, true, "        ds_and_b32      v71, v169 offset:52583\n" },
    { 0xd814cd67U, 0x0000a947U, true, "        ds_or_b32       v71, v169 offset:52583\n" },
    { 0xd816cd67U, 0x0000a947U, true, "        ds_xor_b32      v71, v169 offset:52583\n" },
    { 0xd818cd67U, 0x0027a947U, true, "        ds_mskor_b32    "
                "v71, v169, v39 offset:52583\n" },
    { 0xd818cd67U, 0x8b27a947U, true, "        ds_mskor_b32    "
                "v71, v169, v39 offset:52583 vdst=0x8b\n" },
    { 0xd81acd67U, 0x0000a947U, true, "        ds_write_b32    v71, v169 offset:52583\n" },
    { 0xd81ccd67U, 0x0027a947U, true, "        ds_write2_b32   "
                "v71, v169, v39 offset0:103 offset1:205\n" },
    { 0xd81c0067U, 0x0027a947U, true, "        ds_write2_b32   "
                "v71, v169, v39 offset0:103\n" },
    { 0xd81c1100U, 0x0027a947U, true, "        ds_write2_b32   "
                "v71, v169, v39 offset1:17\n" },
    { 0xd81c0000U, 0x0027a947U, true, "        ds_write2_b32   v71, v169, v39\n" },
    { 0xd81ecd67U, 0x0027a947U, true, "        ds_write2st64_b32 "
                "v71, v169, v39 offset0:103 offset1:205\n" },
    { 0xd820cd67U, 0x0027a947U, true, "        ds_cmpst_b32    "
                "v71, v169, v39 offset:52583\n" },
    { 0xd822cd67U, 0x0027a947U, true, "        ds_cmpst_f32    "
                "v71, v169, v39 offset:52583\n" },
    { 0xd824cd67U, 0x0000a947U, true, "        ds_min_f32      v71, v169 offset:52583\n" }, 
    { 0xd826cd67U, 0x0000a947U, true, "        ds_max_f32      v71, v169 offset:52583\n" }, 
    { 0xd828cd67U, 0x8b27a947U, true, "        ds_nop          "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd82acd67U, 0x0000a947U, true, "        ds_add_f32      v71, v169 offset:52583\n" }, 
    { 0xd82ccd67U, 0x8b27a947U, true, "        DS_ill_22       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd82ecd67U, 0x8b27a947U, true, "        DS_ill_23       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd830cd67U, 0x8b27a947U, true, "        DS_ill_24       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd832cd67U, 0x8b27a947U, true, "        DS_ill_25       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd834cd67U, 0x8b27a947U, true, "        DS_ill_26       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd836cd67U, 0x8b27a947U, true, "        DS_ill_27       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd838cd67U, 0x8b27a947U, true, "        DS_ill_28       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd83acd67U, 0x8b27a947U, true, "        DS_ill_29       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd83ccd67U, 0x0000a947U, true, "        ds_write_b8     v71, v169 offset:52583\n" },
    { 0xd83ecd67U, 0x0000a947U, true, "        ds_write_b16    v71, v169 offset:52583\n" },
    { 0xd840cd67U, 0x9b00a947U, true, "        ds_add_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd840cd67U, 0x9b05a947U, true, "        ds_add_rtn_u32  "
                "v155, v71, v169 offset:52583 vdata1=0x5\n" },
    { 0xd842cd67U, 0x9b00a947U, true, "        ds_sub_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd844cd67U, 0x9b00a947U, true, "        ds_rsub_rtn_u32 "
                "v155, v71, v169 offset:52583\n" },
    { 0xd846cd67U, 0x9b00a947U, true, "        ds_inc_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd848cd67U, 0x9b00a947U, true, "        ds_dec_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd84acd67U, 0x9b00a947U, true, "        ds_min_rtn_i32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd84ccd67U, 0x9b00a947U, true, "        ds_max_rtn_i32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd84ecd67U, 0x9b00a947U, true, "        ds_min_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd850cd67U, 0x9b00a947U, true, "        ds_max_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd852cd67U, 0x9b00a947U, true, "        ds_and_rtn_b32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd854cd67U, 0x9b00a947U, true, "        ds_or_rtn_b32   "
                "v155, v71, v169 offset:52583\n" },
    { 0xd856cd67U, 0x9b00a947U, true, "        ds_xor_rtn_b32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd858cd67U, 0x9b1aa947U, true, "        ds_mskor_rtn_b32 "
                "v155, v71, v169, v26 offset:52583\n" },
    { 0xd85acd67U, 0x9b00a947U, true, "        ds_wrxchg_rtn_b32 "
                "v155, v71, v169 offset:52583\n" },
    { 0xd85ccd67U, 0x9b1aa947U, true, "        ds_wrxchg2_rtn_b32 "
                "v[155:156], v71, v169, v26 offset0:103 offset1:205\n" },
    { 0xd85ecd67U, 0x9b1aa947U, true, "        ds_wrxchg2st64_rtn_b32 "
                "v[155:156], v71, v169, v26 offset0:103 offset1:205\n" },
    { 0xd860cd67U, 0x9b1aa947U, true, "        ds_cmpst_rtn_b32 "
                "v155, v71, v169, v26 offset:52583\n" },
    { 0xd862cd67U, 0x9b1aa947U, true, "        ds_cmpst_rtn_f32 "
                "v155, v71, v169, v26 offset:52583\n" },
    { 0xd864cd67U, 0x9b00a947U, true, "        ds_min_rtn_f32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd866cd67U, 0x9b00a947U, true, "        ds_max_rtn_f32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd868cd67U, 0x9b56a947U, true, "        ds_wrap_rtn_b32 "
                "v155, v71, v169, v86 offset:52583\n" }, /* check */
    { 0xd86acd67U, 0x9b00a947U, true, "        ds_add_rtn_f32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd86ccd67U, 0x9b000047U, true, "        ds_read_b32     v155, v71 offset:52583\n" },
    { 0xd86ecd67U, 0x9b000047U, true, "        ds_read2_b32    "
                "v[155:156], v71 offset0:103 offset1:205\n" },
    { 0xd870cd67U, 0x9b000047U, true, "        ds_read2st64_b32 "
                "v[155:156], v71 offset0:103 offset1:205\n" },
    { 0xd872cd67U, 0x9b000047U, true, "        ds_read_i8      v155, v71 offset:52583\n" },
    { 0xd874cd67U, 0x9b000047U, true, "        ds_read_u8      v155, v71 offset:52583\n" },
    { 0xd876cd67U, 0x9b000047U, true, "        ds_read_i16     v155, v71 offset:52583\n" },
    { 0xd878cd67U, 0x9b000047U, true, "        ds_read_u16     v155, v71 offset:52583\n" },
    { 0xd87acd67U, 0x9b00a947U, true, "        ds_swizzle_b32  "
                "v155, v71 offset:52583 vdata0=0xa9\n" }, /* check */
    { 0xd87ccd67U, 0x9b00a947U, true, "        ds_permute_b32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd87ecd67U, 0x9b00a947U, true, "        ds_bpermute_b32 "
                "v155, v71, v169 offset:52583\n" },
    { 0xd880cd67U, 0x0000a947U, true, "        ds_add_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd882cd67U, 0x0000a947U, true, "        ds_sub_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd884cd67U, 0x0000a947U, true, "        ds_rsub_u64     "
                "v71, v[169:170] offset:52583\n" },
    { 0xd886cd67U, 0x0000a947U, true, "        ds_inc_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd888cd67U, 0x0000a947U, true, "        ds_dec_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd88acd67U, 0x0000a947U, true, "        ds_min_i64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd88ccd67U, 0x0000a947U, true, "        ds_max_i64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd88ecd67U, 0x0000a947U, true, "        ds_min_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd890cd67U, 0x0000a947U, true, "        ds_max_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd892cd67U, 0x0000a947U, true, "        ds_and_b64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd894cd67U, 0x0000a947U, true, "        ds_or_b64       "
                "v71, v[169:170] offset:52583\n" },
    { 0xd896cd67U, 0x0000a947U, true, "        ds_xor_b64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd898cd67U, 0x0017a947U, true, "        ds_mskor_b64    "
                "v71, v[169:170], v[23:24] offset:52583\n" },
    { 0xd89acd67U, 0x0000a947U, true, "        ds_write_b64    "
                "v71, v[169:170] offset:52583\n" },
    { 0xd89ccd67U, 0x0027a947U, true, "        ds_write2_b64   "
                "v71, v[169:170], v[39:40] offset0:103 offset1:205\n" },
    { 0xd89ecd67U, 0x0027a947U, true, "        ds_write2st64_b64 "
                "v71, v[169:170], v[39:40] offset0:103 offset1:205\n" },
    { 0xd8a0cd67U, 0x0027a947U, true, "        ds_cmpst_b64    "
                "v71, v[169:170], v[39:40] offset:52583\n" },
    { 0xd8a2cd67U, 0x0027a947U, true, "        ds_cmpst_f64    "
                "v71, v[169:170], v[39:40] offset:52583\n" },
    { 0xd8a4cd67U, 0x0000a947U, true, "        ds_min_f64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd8a6cd67U, 0x0000a947U, true, "        ds_max_f64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd8a8cd67U, 0x8b27a947U, true, "        DS_ill_84       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8aacd67U, 0x8b27a947U, true, "        DS_ill_85       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8accd67U, 0x8b27a947U, true, "        DS_ill_86       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8aecd67U, 0x8b27a947U, true, "        DS_ill_87       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8b0cd67U, 0x8b27a947U, true, "        DS_ill_88       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8b2cd67U, 0x8b27a947U, true, "        DS_ill_89       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8b4cd67U, 0x8b27a947U, true, "        DS_ill_90       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8b6cd67U, 0x8b27a947U, true, "        DS_ill_91       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8b8cd67U, 0x8b27a947U, true, "        DS_ill_92       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8bacd67U, 0x8b27a947U, true, "        DS_ill_93       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8bccd67U, 0x8b27a947U, true, "        DS_ill_94       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8becd67U, 0x8b27a947U, true, "        DS_ill_95       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8c0cd67U, 0x8b00a947U, true, "        ds_add_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8c0cd67U, 0x8b11a947U, true, "        ds_add_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583 vdata1=0x11\n" },
    { 0xd8c2cd67U, 0x8b00a947U, true, "        ds_sub_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8c4cd67U, 0x8b00a947U, true, "        ds_rsub_rtn_u64 "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8c6cd67U, 0x8b00a947U, true, "        ds_inc_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8c8cd67U, 0x8b00a947U, true, "        ds_dec_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8cacd67U, 0x8b00a947U, true, "        ds_min_rtn_i64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8cccd67U, 0x8b00a947U, true, "        ds_max_rtn_i64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8cecd67U, 0x8b00a947U, true, "        ds_min_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8d0cd67U, 0x8b00a947U, true, "        ds_max_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8d2cd67U, 0x8b00a947U, true, "        ds_and_rtn_b64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8d4cd67U, 0x8b00a947U, true, "        ds_or_rtn_b64   "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8d6cd67U, 0x8b00a947U, true, "        ds_xor_rtn_b64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8d8cd67U, 0x8b56a947U, true, "        ds_mskor_rtn_b64 "
                "v[139:140], v71, v[169:170], v[86:87] offset:52583\n" },
    { 0xd8dacd67U, 0x8b00a947U, true, "        ds_wrxchg_rtn_b64 "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8dccd67U, 0x8b56a947U, true, "        ds_wrxchg2_rtn_b64 "
                "v[139:142], v71, v[169:170], v[86:87] offset0:103 offset1:205\n" },
    { 0xd8decd67U, 0x8b56a947U, true, "        ds_wrxchg2st64_rtn_b64 "
                "v[139:142], v71, v[169:170], v[86:87] offset0:103 offset1:205\n" },
    { 0xd8e0cd67U, 0x8b56a947U, true, "        ds_cmpst_rtn_b64 "
                "v[139:140], v71, v[169:170], v[86:87] offset:52583\n" },
    { 0xd8e2cd67U, 0x8b56a947U, true, "        ds_cmpst_rtn_f64 "
                "v[139:140], v71, v[169:170], v[86:87] offset:52583\n" },
    { 0xd8e4cd67U, 0x8b00a947U, true, "        ds_min_rtn_f64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8e6cd67U, 0x8b00a947U, true, "        ds_max_rtn_f64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd8e8cd67U, 0x8b27a947U, true, "        DS_ill_116      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8eacd67U, 0x8b27a947U, true, "        DS_ill_117      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8eccd67U, 0x8b000047U, true, "        ds_read_b64     "
                "v[139:140], v71 offset:52583\n" },
    { 0xd8eecd67U, 0x8b000047U, true, "        ds_read2_b64    "
                "v[139:142], v71 offset0:103 offset1:205\n" },
    { 0xd8f0cd67U, 0x8b000047U, true, "        ds_read2st64_b64 "
                "v[139:142], v71 offset0:103 offset1:205\n" },
    { 0xd8f2cd67U, 0x8b27a947U, true, "        DS_ill_121      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8f4cd67U, 0x8b27a947U, true, "        DS_ill_122      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8f6cd67U, 0x8b27a947U, true, "        DS_ill_123      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8f8cd67U, 0x8b27a947U, true, "        DS_ill_124      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8facd67U, 0x8b27a947U, true, "        DS_ill_125      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8fccd67U, 0x9b56a947U, true, "        ds_condxchg32_rtn_b64 "
                "v[155:156], v71, v[169:170] offset:52583 vdata1=0x56\n" },
    { 0xd8fecd67U, 0x8b27a947U, true, "        DS_ill_127      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd900cd67U, 0x8b00a947U, true, "        ds_add_src2_u32 "
                "v71 offset:52583 vdata0=0xa9 vdst=0x8b\n" },
    { 0xd900cd67U, 0x00000047U, true, "        ds_add_src2_u32 v71 offset:52583\n" },
    { 0xd902cd67U, 0x00000047U, true, "        ds_sub_src2_u32 v71 offset:52583\n" },
    { 0xd904cd67U, 0x00000047U, true, "        ds_rsub_src2_u32 v71 offset:52583\n" },
    { 0xd906cd67U, 0x00000047U, true, "        ds_inc_src2_u32 v71 offset:52583\n" },
    { 0xd908cd67U, 0x00000047U, true, "        ds_dec_src2_u32 v71 offset:52583\n" },
    { 0xd90acd67U, 0x00000047U, true, "        ds_min_src2_i32 v71 offset:52583\n" },
    { 0xd90ccd67U, 0x00000047U, true, "        ds_max_src2_i32 v71 offset:52583\n" },
    { 0xd90ecd67U, 0x00000047U, true, "        ds_min_src2_u32 v71 offset:52583\n" },
    { 0xd910cd67U, 0x00000047U, true, "        ds_max_src2_u32 v71 offset:52583\n" },
    { 0xd912cd67U, 0x00000047U, true, "        ds_and_src2_b32 v71 offset:52583\n" },
    { 0xd914cd67U, 0x00000047U, true, "        ds_or_src2_b32  v71 offset:52583\n" },
    { 0xd916cd67U, 0x00000047U, true, "        ds_xor_src2_b32 v71 offset:52583\n" },
    { 0xd918cd67U, 0x8b27a947U, true, "        DS_ill_140      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd91acd67U, 0x00000047U, true, "        ds_write_src2_b32 v71 offset:52583\n" },
    { 0xd91ccd67U, 0x8b27a947U, true, "        DS_ill_142      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd91ecd67U, 0x8b27a947U, true, "        DS_ill_143      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd920cd67U, 0x8b27a947U, true, "        DS_ill_144      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd922cd67U, 0x8b27a947U, true, "        DS_ill_145      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd924cd67U, 0x00000047U, true, "        ds_min_src2_f32 v71 offset:52583\n" },
    { 0xd926cd67U, 0x00000047U, true, "        ds_max_src2_f32 v71 offset:52583\n" },
    { 0xd928cd67U, 0x8b27a947U, true, "        DS_ill_148      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd92acd67U, 0x00000047U, true, "        ds_add_src2_f32 v71 offset:52583\n" },
    { 0xd92ccd67U, 0x8b27a947U, true, "        DS_ill_150      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd92ecd67U, 0x8b27a947U, true, "        DS_ill_151      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd930cd67U, 0x8b27a947U, true, "        ds_gws_sema_release_all "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9330000U, 0x8b000047U, true, "        ds_gws_init     v71 gds vdst=0x8b\n" },
    { 0xd933cd67U, 0x00000047U, true, "        ds_gws_init     v71 offset:52583 gds\n" },
    { 0xd935cd67U, 0x00000047U, true, "        ds_gws_sema_v   v71 offset:52583 gds\n" },
    { 0xd937cd67U, 0x00000047U, true, "        ds_gws_sema_br  v71 offset:52583 gds\n" },
    { 0xd939cd67U, 0x00000047U, true, "        ds_gws_sema_p   v71 offset:52583 gds\n" },
    { 0xd93bcd67U, 0x00000047U, true, "        ds_gws_barrier  v71 offset:52583 gds\n" },
    { 0xd93ccd67U, 0x8b27a947U, true, "        DS_ill_158      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd93ecd67U, 0x8b27a947U, true, "        DS_ill_159      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd978cd67U, 0x8b27a947U, true, "        DS_ill_188      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd97acd67U, 0x9b000047U, true, "        ds_consume      "
                "v155 offset:52583 vaddr=0x47\n" },
    { 0xd97acd67U, 0x9b000000U, true, "        ds_consume      v155 offset:52583\n" },
    { 0xd97ccd67U, 0x9b000047U, true, "        ds_append       "
                "v155 offset:52583 vaddr=0x47\n" },
    { 0xd97ccd67U, 0x9b000000U, true, "        ds_append       v155 offset:52583\n" },
    { 0xd97ecd67U, 0x9b000047U, true, "        ds_ordered_count v155, v71 offset:52583\n" },
    { 0xd980cd67U, 0x00000047U, true, "        ds_add_src2_u64 v71 offset:52583\n" },
    { 0xd982cd67U, 0x00000047U, true, "        ds_sub_src2_u64 v71 offset:52583\n" },
    { 0xd984cd67U, 0x00000047U, true, "        ds_rsub_src2_u64 v71 offset:52583\n" },
    { 0xd986cd67U, 0x00000047U, true, "        ds_inc_src2_u64 v71 offset:52583\n" },
    { 0xd988cd67U, 0x00000047U, true, "        ds_dec_src2_u64 v71 offset:52583\n" },
    { 0xd98acd67U, 0x00000047U, true, "        ds_min_src2_i64 v71 offset:52583\n" },
    { 0xd98ccd67U, 0x00000047U, true, "        ds_max_src2_i64 v71 offset:52583\n" },
    { 0xd98ecd67U, 0x00000047U, true, "        ds_min_src2_u64 v71 offset:52583\n" },
    { 0xd990cd67U, 0x00000047U, true, "        ds_max_src2_u64 v71 offset:52583\n" },
    { 0xd992cd67U, 0x00000047U, true, "        ds_and_src2_b64 v71 offset:52583\n" },
    { 0xd994cd67U, 0x00000047U, true, "        ds_or_src2_b64  v71 offset:52583\n" },
    { 0xd996cd67U, 0x00000047U, true, "        ds_xor_src2_b64 v71 offset:52583\n" },
    { 0xd998cd67U, 0x8b27a947U, true, "        DS_ill_204      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd99acd67U, 0x00000047U, true, "        ds_write_src2_b64 v71 offset:52583\n" },
    { 0xd99ccd67U, 0x8b27a947U, true, "        DS_ill_206      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd99ecd67U, 0x8b27a947U, true, "        DS_ill_207      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9a0cd67U, 0x8b27a947U, true, "        DS_ill_208      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9a2cd67U, 0x8b27a947U, true, "        DS_ill_209      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9a4cd67U, 0x00000047U, true, "        ds_min_src2_f64 v71 offset:52583\n" },
    { 0xd9a6cd67U, 0x00000047U, true, "        ds_max_src2_f64 v71 offset:52583\n" },
    { 0xd9a8cd67U, 0x8b27a947U, true, "        DS_ill_212      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9aacd67U, 0x8b27a947U, true, "        DS_ill_213      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9accd67U, 0x8b27a947U, true, "        DS_ill_214      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9aecd67U, 0x8b27a947U, true, "        DS_ill_215      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9b0cd67U, 0x8b27a947U, true, "        DS_ill_216      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9b2cd67U, 0x8b27a947U, true, "        DS_ill_217      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9bccd67U, 0x0000a947U, true, "        ds_write_b96    "
                "v71, v[169:171] offset:52583\n" },
    { 0xd9becd67U, 0x0000a947U, true, "        ds_write_b128   "
                "v71, v[169:172] offset:52583\n" },
    { 0xd9facd67U, 0x9b56a947U, true, "        ds_condxchg32_rtn_b128 " /* is good??? */
                "v[155:158] offset:52583 vaddr=0x47 vdata0=0xa9 vdata1=0x56\n" },
    { 0xd9fccd67U, 0x9b56a947U, true, "        ds_read_b96     "
                "v[155:157], v71 offset:52583 vdata0=0xa9 vdata1=0x56\n" },
    { 0xd9fecd67U, 0x9b56a947U, true, "        ds_read_b128    "
                "v[155:158], v71 offset:52583 vdata0=0xa9 vdata1=0x56\n" },
    /* MUBUF encoding */
    { 0xe003725bU, 0x23b43d12U, true, "        buffer_load_format_x "
    "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds tfe\n" },
    { 0xe003f25bU, 0x23b43d12U, true, "        buffer_load_format_x "
    "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds tfe\n" },
    /* vaddr sizing */
    { 0xe003725bU, 0x23b43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds tfe\n" },
    { 0xe003425bU, 0x23b43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], s35 offset:603 glc slc lds tfe\n" },
    { 0xe003625bU, 0x23b43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], s35 idxen offset:603 glc slc lds tfe\n" },
    { 0xe003525bU, 0x23b43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], s35 offen offset:603 glc slc lds tfe\n" },
    /* flags */
    { 0xe003c25bU, 0xf0b43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], 0.5 offset:603 glc slc lds tfe\n" },
    { 0xe001c25bU, 0xf0b43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], 0.5 offset:603 glc lds tfe\n" },
    { 0xe003c25bU, 0x23bd3d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, ttmp[4:7], s35 offset:603 glc slc lds tfe\n" },
    { 0xe003c25bU, 0x23a93d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[36:39], s35 offset:603 glc slc lds tfe\n" },
    { 0xe003f000U, 0x23b43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen glc slc lds tfe\n" },
    /* MUBUF instructions */
    { 0xe000325bU, 0x23343d12U, true, "        buffer_load_format_x "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe004325bU, 0x23343d12U, true, "        buffer_load_format_xy "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe008325bU, 0x23343d12U, true, "        buffer_load_format_xyz "
                "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe00c325bU, 0x23343d12U, true, "        buffer_load_format_xyzw "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe010325bU, 0x23343d12U, true, "        buffer_store_format_x "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe014325bU, 0x23343d12U, true, "        buffer_store_format_xy "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe018325bU, 0x23343d12U, true, "        buffer_store_format_xyz "
                "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe01c325bU, 0x23343d12U, true, "        buffer_store_format_xyzw "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe020325bU, 0x23343d12U, true, "        buffer_load_format_d16_x "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe024325bU, 0x23343d12U, true, "        buffer_load_format_d16_xy "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe028325bU, 0x23343d12U, true, "        buffer_load_format_d16_xyz "
                "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe02c325bU, 0x23343d12U, true, "        buffer_load_format_d16_xyzw "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe030325bU, 0x23343d12U, true, "        buffer_store_format_d16_x "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe034325bU, 0x23343d12U, true, "        buffer_store_format_d16_xy "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe038325bU, 0x23343d12U, true, "        buffer_store_format_d16_xyz "
                "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe03c325bU, 0x23343d12U, true, "        buffer_store_format_d16_xyzw "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe040325bU, 0x23343d12U, true, "        buffer_load_ubyte "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe044325bU, 0x23343d12U, true, "        buffer_load_sbyte "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe048325bU, 0x23343d12U, true, "        buffer_load_ushort "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe04c325bU, 0x23343d12U, true, "        buffer_load_sshort "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe050325bU, 0x23343d12U, true, "        buffer_load_dword "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe054325bU, 0x23343d12U, true, "        buffer_load_dwordx2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe058325bU, 0x23343d12U, true, "        buffer_load_dwordx3 "
                "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe05c325bU, 0x23343d12U, true, "        buffer_load_dwordx4 "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe060325bU, 0x23343d12U, true, "        buffer_store_byte "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe064325bU, 0x23343d12U, true, "        MUBUF_ill_25    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe068325bU, 0x23343d12U, true, "        buffer_store_short "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe06c325bU, 0x23343d12U, true, "        MUBUF_ill_27    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe070325bU, 0x23343d12U, true, "        buffer_store_dword "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe074325bU, 0x23343d12U, true, "        buffer_store_dwordx2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe078325bU, 0x23343d12U, true, "        buffer_store_dwordx3 "
                "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe07c325bU, 0x23343d12U, true, "        buffer_store_dwordx4 "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe080325bU, 0x23343d12U, true, "        MUBUF_ill_32    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0f4325bU, 0x23343d12U, true, "        buffer_store_lds_dword "
                "s[80:83], s35 offen idxen offset:603 vaddr=0x12 vdata=0x3d\n" },
    { 0xe0f4325bU, 0x23340000U, true, "        buffer_store_lds_dword "
                "s[80:83], s35 offen idxen offset:603\n" },
    /* wbinv */
    { 0xe0f9325bU, 0x23343d12U, true, "        buffer_wbinvl1  "
        "offen idxen offset:603 lds vaddr=0x12 vdata=0x3d srsrc=0x14 soffset=0x23\n" },
    { 0xe0f9325bU, 0x00200000U, true, "        buffer_wbinvl1  "
        "offen idxen offset:603 lds\n" },
    { 0xe0fd325bU, 0x23343d12U, true, "        buffer_wbinvl1_vol "
        "offen idxen offset:603 lds vaddr=0x12 vdata=0x3d srsrc=0x14 soffset=0x23\n" },
    { 0xe0fd325bU, 0x00200000U, true, "        buffer_wbinvl1_vol "
        "offen idxen offset:603 lds\n" },
    { 0xe100325bU, 0x23343d12U, true, "        buffer_atomic_swap "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe104325bU, 0x23343d12U, true, "        buffer_atomic_cmpswap "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe108325bU, 0x23343d12U, true, "        buffer_atomic_add "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe10c325bU, 0x23343d12U, true, "        buffer_atomic_sub "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe110325bU, 0x23343d12U, true, "        buffer_atomic_smin "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe114325bU, 0x23343d12U, true, "        buffer_atomic_umin "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe118325bU, 0x23343d12U, true, "        buffer_atomic_smax "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe11c325bU, 0x23343d12U, true, "        buffer_atomic_umax "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe120325bU, 0x23343d12U, true, "        buffer_atomic_and "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe124325bU, 0x23343d12U, true, "        buffer_atomic_or "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe128325bU, 0x23343d12U, true, "        buffer_atomic_xor "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe12c325bU, 0x23343d12U, true, "        buffer_atomic_inc "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe130325bU, 0x23343d12U, true, "        buffer_atomic_dec "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe134325bU, 0x23343d12U, true, "        MUBUF_ill_77    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe138325bU, 0x23343d12U, true, "        MUBUF_ill_78    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe13c325bU, 0x23343d12U, true, "        MUBUF_ill_79    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe140325bU, 0x23343d12U, true, "        MUBUF_ill_80    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe180325bU, 0x23343d12U, true, "        buffer_atomic_swap_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe184325bU, 0x23343d12U, true, "        buffer_atomic_cmpswap_x2 "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe188325bU, 0x23343d12U, true, "        buffer_atomic_add_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe18c325bU, 0x23343d12U, true, "        buffer_atomic_sub_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe190325bU, 0x23343d12U, true, "        buffer_atomic_smin_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe194325bU, 0x23343d12U, true, "        buffer_atomic_umin_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe198325bU, 0x23343d12U, true, "        buffer_atomic_smax_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe19c325bU, 0x23343d12U, true, "        buffer_atomic_umax_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1a0325bU, 0x23343d12U, true, "        buffer_atomic_and_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1a4325bU, 0x23343d12U, true, "        buffer_atomic_or_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1a8325bU, 0x23343d12U, true, "        buffer_atomic_xor_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1ac325bU, 0x23343d12U, true, "        buffer_atomic_inc_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1b0325bU, 0x23343d12U, true, "        buffer_atomic_dec_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1b4325bU, 0x23343d12U, true, "        MUBUF_ill_109   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1b8325bU, 0x23343d12U, true, "        MUBUF_ill_110   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1bc325bU, 0x23343d12U, true, "        MUBUF_ill_111   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe1c0325bU, 0x23343d12U, true, "        MUBUF_ill_112   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    /* MTBUF encoding */
    { 0xea8877d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    /* formats */
    { 0xea8077d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[invalid,sint]\n" },
    { 0xea8877d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea9077d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[16,sint]\n" },
    /* instructions */
    { 0xea88f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_xy "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8977d4U, 0x23f43d12U, true, "        tbuffer_load_format_xyz "
        "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea89f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_xyzw "
        "v[61:65], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8a77d4U, 0x23f43d12U, true, "        tbuffer_store_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8af7d4U, 0x23f43d12U, true, "        tbuffer_store_format_xy "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8b77d4U, 0x23f43d12U, true, "        tbuffer_store_format_xyz "
        "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8bf7d4U, 0x23f43d12U, true, "        tbuffer_store_format_xyzw "
        "v[61:65], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8c77d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8cf7d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_xy "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8d77d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_xyz "
        "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8df7d4U, 0x23f43d12U, true, "        tbuffer_load_format_d16_xyzw "
        "v[61:65], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8e77d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8ef7d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_xy "
        "v[61:63], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8f77d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_xyz "
        "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    { 0xea8ff7d4U, 0x23f43d12U, true, "        tbuffer_store_format_d16_xyzw "
        "v[61:65], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
        "format:[sint]\n" },
    /* MIMG encoding */
    { 0xf203fb00U, 0x02d59d79U, true, "        image_load      v[157:160], "
        "v[121:124], s[84:87] dmask:11 unorm glc slc r128 tfe lwe da ssamp=0x16\n" },
    { 0xf203fb00U, 0x00159d79U, true, "        image_load      v[157:160], "
        "v[121:124], s[84:87] dmask:11 unorm glc slc r128 tfe lwe da\n" },
    /* dmasks */
    { 0xf202f000U, 0x02d59d79U, true, "        image_load      v157, "
        "v[121:124], s[84:87] dmask:0 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f100U, 0x02d59d79U, true, "        image_load      v157, "
        "v[121:124], s[84:87] unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f200U, 0x02d59d79U, true, "        image_load      v157, "
        "v[121:124], s[84:87] dmask:2 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f300U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:3 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f400U, 0x02d59d79U, true, "        image_load      v157, "
        "v[121:124], s[84:87] dmask:4 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f500U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:5 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f600U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:6 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f700U, 0x02d59d79U, true, "        image_load      v[157:159], "
        "v[121:124], s[84:87] dmask:7 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f800U, 0x02d59d79U, true, "        image_load      v157, "
        "v[121:124], s[84:87] dmask:8 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202f900U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:9 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202fa00U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:10 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202fb00U, 0x02d59d79U, true, "        image_load      v[157:159], "
        "v[121:124], s[84:87] dmask:11 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202fc00U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:12 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202fd00U, 0x02d59d79U, true, "        image_load      v[157:159], "
        "v[121:124], s[84:87] dmask:13 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202fe00U, 0x02d59d79U, true, "        image_load      v[157:159], "
        "v[121:124], s[84:87] dmask:14 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202ff00U, 0x02d59d79U, true, "        image_load      v[157:160], "
        "v[121:124], s[84:87] dmask:15 unorm glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf203f000U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:0 unorm glc slc r128 tfe lwe da ssamp=0x16\n" },
    { 0xf203f100U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] unorm glc slc r128 tfe lwe da ssamp=0x16\n" },
    { 0xf203f200U, 0x02d59d79U, true, "        image_load      v[157:158], "
        "v[121:124], s[84:87] dmask:2 unorm glc slc r128 tfe lwe da ssamp=0x16\n" },
    /* other masks */
    { 0xf202e100U, 0x02d59d79U, true, "        image_load      " /* no unorm */
            "v157, v[121:124], s[84:87] glc slc r128 lwe da ssamp=0x16\n" },
    { 0xf202d100U, 0x02d59d79U, true, "        image_load      " /* no glc */
            "v157, v[121:124], s[84:87] unorm slc r128 lwe da ssamp=0x16\n" },
    { 0xf202b100U, 0x02d59d79U, true, "        image_load      " /* no da */
            "v157, v[121:124], s[84:87] unorm glc slc r128 lwe ssamp=0x16\n" },
    { 0xf2027100U, 0x02d59d79U, true, "        image_load      " /* no r128 */
            "v157, v[121:124], s[84:91] unorm glc slc lwe da ssamp=0x16\n" },
    { 0xf002f100U, 0x02d59d79U, true, "        image_load      " /* no slc */
            "v157, v[121:124], s[84:87] unorm glc r128 lwe da ssamp=0x16\n" },
    { 0xf200f100U, 0x02d59d79U, true, "        image_load      " /* no lwe */
            "v157, v[121:124], s[84:87] unorm glc slc r128 da ssamp=0x16\n" },
    { 0xf0000100U, 0x02d59d79U, true, "        image_load      " /* no flags */
            "v157, v[121:124], s[84:91] ssamp=0x16\n" },
    { 0xf202e100U, 0x82d59d79U, true, "        image_load      " /* no unorm */
            "v157, v[121:124], s[84:87] glc slc r128 lwe da d16 ssamp=0x16\n" },
    /* MIMG instructions */
    { 0xf000fb00U, 0x00159d79U, true, "        image_load      "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf004fb00U, 0x00159d79U, true, "        image_load_mip  "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf008fb00U, 0x00159d79U, true, "        image_load_pck  "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf00cfb00U, 0x00159d79U, true, "        image_load_pck_sgn "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf010fb00U, 0x00159d79U, true, "        image_load_mip_pck "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf014fb00U, 0x00159d79U, true, "        image_load_mip_pck_sgn "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf018fb00U, 0x00159d79U, true, "        MIMG_ill_6      "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf01cfb00U, 0x00159d79U, true, "        MIMG_ill_7      "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf020fb00U, 0x00159d79U, true, "        image_store     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf024fb00U, 0x00159d79U, true, "        image_store_mip "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf028fb00U, 0x00159d79U, true, "        image_store_pck "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf02cfb00U, 0x00159d79U, true, "        image_store_mip_pck "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf030fb00U, 0x00159d79U, true, "        MIMG_ill_12     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf034fb00U, 0x00159d79U, true, "        MIMG_ill_13     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf038fb00U, 0x00159d79U, true, "        image_get_resinfo "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf03cfb00U, 0x00159d79U, true, "        MIMG_ill_15     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf040fb00U, 0x00159d79U, true, "        image_atomic_swap "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf044fb00U, 0x00159d79U, true, "        image_atomic_cmpswap "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf048fb00U, 0x00159d79U, true, "        image_atomic_add "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf04cfb00U, 0x00159d79U, true, "        image_atomic_sub "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf050fb00U, 0x00159d79U, true, "        image_atomic_smin "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf054fb00U, 0x00159d79U, true, "        image_atomic_umin "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf058fb00U, 0x00159d79U, true, "        image_atomic_smax "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf05cfb00U, 0x00159d79U, true, "        image_atomic_umax "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf060fb00U, 0x00159d79U, true, "        image_atomic_and "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf064fb00U, 0x00159d79U, true, "        image_atomic_or "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf068fb00U, 0x00159d79U, true, "        image_atomic_xor "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf06cfb00U, 0x00159d79U, true, "        image_atomic_inc "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf070fb00U, 0x00159d79U, true, "        image_atomic_dec "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf074fb00U, 0x00159d79U, true, "        MIMG_ill_29     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf078fb00U, 0x00159d79U, true, "        MIMG_ill_30     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf07cfb00U, 0x00159d79U, true, "        MIMG_ill_31     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf080fb00U, 0x02d59d79U, true, "        image_sample    "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf080fb00U, 0x00159d79U, true, "        image_sample    " /* with ssamp=0 */
        "v[157:159], v[121:124], s[84:87], s[0:3] dmask:11 unorm glc r128 da\n" },
    { 0xf084fb00U, 0x02d59d79U, true, "        image_sample_cl "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf088fb00U, 0x02d59d79U, true, "        image_sample_d  "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf08cfb00U, 0x02d59d79U, true, "        image_sample_d_cl "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf090fb00U, 0x02d59d79U, true, "        image_sample_l  "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf094fb00U, 0x02d59d79U, true, "        image_sample_b  "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf098fb00U, 0x02d59d79U, true, "        image_sample_b_cl "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf09cfb00U, 0x02d59d79U, true, "        image_sample_lz "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0a0fb00U, 0x02d59d79U, true, "        image_sample_c  "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0a4fb00U, 0x02d59d79U, true, "        image_sample_c_cl "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0a8fb00U, 0x02d59d79U, true, "        image_sample_c_d "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0acfb00U, 0x02d59d79U, true, "        image_sample_c_d_cl "
        "v[157:159], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0b0fb00U, 0x02d59d79U, true, "        image_sample_c_l "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0b4fb00U, 0x02d59d79U, true, "        image_sample_c_b "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0b8fb00U, 0x02d59d79U, true, "        image_sample_c_b_cl "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0bcfb00U, 0x02d59d79U, true, "        image_sample_c_lz "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0c0fb00U, 0x02d59d79U, true, "        image_sample_o  "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0c4fb00U, 0x02d59d79U, true, "        image_sample_cl_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0c8fb00U, 0x02d59d79U, true, "        image_sample_d_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0ccfb00U, 0x02d59d79U, true, "        image_sample_d_cl_o "
        "v[157:159], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0d0fb00U, 0x02d59d79U, true, "        image_sample_l_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0d4fb00U, 0x02d59d79U, true, "        image_sample_b_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0d8fb00U, 0x02d59d79U, true, "        image_sample_b_cl_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0dcfb00U, 0x02d59d79U, true, "        image_sample_lz_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0e0fb00U, 0x02d59d79U, true, "        image_sample_c_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0e4fb00U, 0x02d59d79U, true, "        image_sample_c_cl_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0e8fb00U, 0x02d59d79U, true, "        image_sample_c_d_o "
        "v[157:159], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0ecfb00U, 0x02d59d79U, true, "        image_sample_c_d_cl_o "
        "v[157:159], v[121:126], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0f0fb00U, 0x02d59d79U, true, "        image_sample_c_l_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0f4fb00U, 0x02d59d79U, true, "        image_sample_c_b_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0f8fb00U, 0x02d59d79U, true, "        image_sample_c_b_cl_o "
        "v[157:159], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf0fcfb00U, 0x02d59d79U, true, "        image_sample_c_lz_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf100fb00U, 0x02d59d79U, true, "        image_gather4   "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf104fb00U, 0x02d59d79U, true, "        image_gather4_cl "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf108fb00U, 0x00159d79U, true, "        MIMG_ill_66     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf10cfb00U, 0x00159d79U, true, "        MIMG_ill_67     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf110fb00U, 0x02d59d79U, true, "        image_gather4_l "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf114fb00U, 0x02d59d79U, true, "        image_gather4_b "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf118fb00U, 0x02d59d79U, true, "        image_gather4_b_cl "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf11cfb00U, 0x02d59d79U, true, "        image_gather4_lz "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf120fb00U, 0x02d59d79U, true, "        image_gather4_c "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf124fb00U, 0x02d59d79U, true, "        image_gather4_c_cl "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf128fb00U, 0x00159d79U, true, "        MIMG_ill_74     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf12cfb00U, 0x00159d79U, true, "        MIMG_ill_75     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf130fb00U, 0x02d59d79U, true, "        image_gather4_c_l "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf134fb00U, 0x02d59d79U, true, "        image_gather4_c_b "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf138fb00U, 0x02d59d79U, true, "        image_gather4_c_b_cl "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf13cfb00U, 0x02d59d79U, true, "        image_gather4_c_lz "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf140fb00U, 0x02d59d79U, true, "        image_gather4_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf144fb00U, 0x02d59d79U, true, "        image_gather4_cl_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf148fb00U, 0x00159d79U, true, "        MIMG_ill_82     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf14cfb00U, 0x00159d79U, true, "        MIMG_ill_83     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf150fb00U, 0x02d59d79U, true, "        image_gather4_l_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf154fb00U, 0x02d59d79U, true, "        image_gather4_b_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf158fb00U, 0x02d59d79U, true, "        image_gather4_b_cl_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf15cfb00U, 0x02d59d79U, true, "        image_gather4_lz_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf160fb00U, 0x02d59d79U, true, "        image_gather4_c_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf164fb00U, 0x02d59d79U, true, "        image_gather4_c_cl_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf168fb00U, 0x00159d79U, true, "        MIMG_ill_90     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf16cfb00U, 0x00159d79U, true, "        MIMG_ill_91     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf170fb00U, 0x02d59d79U, true, "        image_gather4_c_l_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf174fb00U, 0x02d59d79U, true, "        image_gather4_c_b_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf178fb00U, 0x02d59d79U, true, "        image_gather4_c_b_cl_o "
        "v[157:160], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf17cfb00U, 0x02d59d79U, true, "        image_gather4_c_lz_o "
        "v[157:160], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf180fb00U, 0x02d59d79U, true, "        image_get_lod   "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf184fb00U, 0x00159d79U, true, "        MIMG_ill_97     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf188fb00U, 0x00159d79U, true, "        MIMG_ill_98     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf18cfb00U, 0x00159d79U, true, "        MIMG_ill_99     "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf190fb00U, 0x00159d79U, true, "        MIMG_ill_100    "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf194fb00U, 0x00159d79U, true, "        MIMG_ill_101    "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf198fb00U, 0x00159d79U, true, "        MIMG_ill_102    "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf19cfb00U, 0x00159d79U, true, "        MIMG_ill_103    "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf1a0fb00U, 0x02d59d79U, true, "        image_sample_cd "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1a4fb00U, 0x02d59d79U, true, "        image_sample_cd_cl "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1a8fb00U, 0x02d59d79U, true, "        image_sample_c_cd "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1acfb00U, 0x02d59d79U, true, "        image_sample_c_cd_cl "
        "v[157:159], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1b0fb00U, 0x02d59d79U, true, "        image_sample_cd_o "
        "v[157:159], v[121:124], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1b4fb00U, 0x02d59d79U, true, "        image_sample_cd_cl_o "
        "v[157:159], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1b8fb00U, 0x02d59d79U, true, "        image_sample_c_cd_o "
        "v[157:159], v[121:125], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1bcfb00U, 0x02d59d79U, true, "        image_sample_c_cd_cl_o "
        "v[157:159], v[121:126], s[84:87], s[88:91] dmask:11 unorm glc r128 da\n" },
    { 0xf1c0fb00U, 0x00159d79U, true, "        MIMG_ill_112    "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    /* EXP encoding */
    { 0xc4001e57U, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, v116, v93, off done compr vm vsrc2=0x1b vsrc3=0x7c\n" },
    { 0xc4001a57U, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, v93, v27, off done vm vsrc3=0x7c\n" },
    { 0xc4001e5fU, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, v116, v93, v93 done compr vm vsrc2=0x1b vsrc3=0x7c\n" },
    { 0xc4001a5fU, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, v93, v27, v124 done vm\n" },
    /* EXP target */
    { 0xc4001f47U, 0x7c1b5d74U, true, "        exp             "
        "param20, v116, v116, v93, off done compr vm vsrc2=0x1b vsrc3=0x7c\n" },
    { 0xc4001fd7U, 0x7c1b5d74U, true, "        exp             "
        "param29, v116, v116, v93, off done compr vm vsrc2=0x1b vsrc3=0x7c\n" },
    /* flags */
    { 0xc400125fU, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, v93, v27, v124 vm\n" },
    { 0xc400025fU, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, v93, v27, v124\n" },
    /* en on/off (compr) */
    { 0xc4001e5cU, 0x7c1b5d74U, true, "        exp             "
        "param5, off, off, v93, v93 done compr vm vsrc0=0x74 vsrc2=0x1b vsrc3=0x7c\n" },
    { 0xc4001e5dU, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, off, v93, v93 done compr vm vsrc2=0x1b vsrc3=0x7c\n" },
    /* en on/off (compr) */
    { 0xc4001a55U, 0x7c1b5d74U, true, "        exp             "
        "param5, v116, off, v27, off done vm vsrc1=0x5d vsrc3=0x7c\n" },
    { 0xc4001a5eU, 0x7c1b5d74U, true, "        exp             "
        "param5, off, v93, v27, v124 done vm vsrc0=0x74\n" },
    /* FLAT encoding */
    { 0xdc030000U, 0x2f8041bbU, true, "        FLAT_ill_0      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xde030000U, 0x2f8041bbU, true, "        FLAT_ill_0      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc070000U, 0x2f8041bbU, true, "        FLAT_ill_1      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc0b0000U, 0x2f8041bbU, true, "        FLAT_ill_2      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc0f0000U, 0x2f8041bbU, true, "        FLAT_ill_3      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc130000U, 0x2f8041bbU, true, "        FLAT_ill_4      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc170000U, 0x2f8041bbU, true, "        FLAT_ill_5      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc1b0000U, 0x2f8041bbU, true, "        FLAT_ill_6      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc1f0000U, 0x2f8041bbU, true, "        FLAT_ill_7      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc230000U, 0x2f8041bbU, true, "        FLAT_ill_8      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc270000U, 0x2f8041bbU, true, "        FLAT_ill_9      "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc2b0000U, 0x2f8041bbU, true, "        FLAT_ill_10     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc2f0000U, 0x2f8041bbU, true, "        FLAT_ill_11     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc330000U, 0x2f8041bbU, true, "        FLAT_ill_12     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc370000U, 0x2f8041bbU, true, "        FLAT_ill_13     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc3b0000U, 0x2f8041bbU, true, "        FLAT_ill_14     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc3f0000U, 0x2f8041bbU, true, "        FLAT_ill_15     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc430000U, 0x2f8000bbU, true, "        flat_load_ubyte "
                "v[47:48], v[187:188] glc slc tfe\n" },
    { 0xdc430000U, 0x2f0000bbU, true, "        flat_load_ubyte "
                "v47, v[187:188] glc slc\n" },
    { 0xdc430000U, 0x2f0062bbU, true, "        flat_load_ubyte "
                "v47, v[187:188] glc slc vdata=0x62\n" },
    { 0xdc400000U, 0x2f8000bbU, true, "        flat_load_ubyte "
                "v[47:48], v[187:188] tfe\n" },
    { 0xdc410000U, 0x2f8000bbU, true, "        flat_load_ubyte "
                "v[47:48], v[187:188] glc tfe\n" },
    { 0xdc420000U, 0x2f8000bbU, true, "        flat_load_ubyte "
                "v[47:48], v[187:188] slc tfe\n" },
    { 0xdc420000U, 0x2f8000bfU, true, "        flat_load_ubyte "
                "v[47:48], v[191:192] slc tfe\n" },
    /* FLAT instructions */
    { 0xdc470000U, 0x2f8000bbU, true, "        flat_load_sbyte "
                "v[47:48], v[187:188] glc slc tfe\n" },
    { 0xde470000U, 0x2f8000bbU, true, "        flat_load_sbyte "
                "v[47:48], v[187:188] glc slc tfe\n" },
    { 0xdc4b0000U, 0x2f8000bbU, true, "        flat_load_ushort "
                "v[47:48], v[187:188] glc slc tfe\n" },
    { 0xdc4f0000U, 0x2f8000bbU, true, "        flat_load_sshort "
                "v[47:48], v[187:188] glc slc tfe\n" },
    { 0xdc530000U, 0x2f8000bbU, true, "        flat_load_dword "
                "v[47:48], v[187:188] glc slc tfe\n" },
    { 0xdc570000U, 0x2f8000bbU, true, "        flat_load_dwordx2 "
                "v[47:49], v[187:188] glc slc tfe\n" },
    { 0xdc5b0000U, 0x2f8000bbU, true, "        flat_load_dwordx3 "
                "v[47:50], v[187:188] glc slc tfe\n" },
    { 0xdc5f0000U, 0x2f8000bbU, true, "        flat_load_dwordx4 "
                "v[47:51], v[187:188] glc slc tfe\n" },
    { 0xdc630000U, 0x2f8054bfU, true, "        flat_store_byte "
                "v[191:192], v84 glc slc tfe vdst=0x2f\n" },
    { 0xdc630000U, 0x008054bfU, true, "        flat_store_byte "
                "v[191:192], v84 glc slc tfe\n" },
    { 0xdc670000U, 0x2f8041bbU, true, "        FLAT_ill_25     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc6b0000U, 0x008054bfU, true, "        flat_store_short "
                "v[191:192], v84 glc slc tfe\n" },
    { 0xdc6f0000U, 0x2f8041bbU, true, "        FLAT_ill_27     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc730000U, 0x008054bfU, true, "        flat_store_dword "
                "v[191:192], v84 glc slc tfe\n" },
    { 0xdc770000U, 0x008054bfU, true, "        flat_store_dwordx2 "
                "v[191:192], v[84:85] glc slc tfe\n" },
    { 0xdc7b0000U, 0x008054bfU, true, "        flat_store_dwordx3 "
                "v[191:192], v[84:86] glc slc tfe\n" },
    { 0xdc7f0000U, 0x008054bfU, true, "        flat_store_dwordx4 "
                "v[191:192], v[84:87] glc slc tfe\n" },
    { 0xdc830000U, 0x2f8041bbU, true, "        FLAT_ill_32     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc870000U, 0x2f8041bbU, true, "        FLAT_ill_33     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc8b0000U, 0x2f8041bbU, true, "        FLAT_ill_34     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc8f0000U, 0x2f8041bbU, true, "        FLAT_ill_35     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc930000U, 0x2f8041bbU, true, "        FLAT_ill_36     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc970000U, 0x2f8041bbU, true, "        FLAT_ill_37     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc9b0000U, 0x2f8041bbU, true, "        FLAT_ill_38     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdc9f0000U, 0x2f8041bbU, true, "        FLAT_ill_39     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdca30000U, 0x2f8041bbU, true, "        FLAT_ill_40     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdca70000U, 0x2f8041bbU, true, "        FLAT_ill_41     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdcab0000U, 0x2f8041bbU, true, "        FLAT_ill_42     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdcaf0000U, 0x2f8041bbU, true, "        FLAT_ill_43     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdcb30000U, 0x2f8041bbU, true, "        FLAT_ill_44     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdcb70000U, 0x2f8041bbU, true, "        FLAT_ill_45     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdcbb0000U, 0x2f8041bbU, true, "        FLAT_ill_46     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdcbf0000U, 0x2f8041bbU, true, "        FLAT_ill_47     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdcc30000U, 0x2f8041bbU, true, "        FLAT_ill_48     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdce30000U, 0x2f8041bbU, true, "        FLAT_ill_56     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd030000U, 0x2f8041bbU, true, "        flat_atomic_swap "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd030000U, 0x2f0041bbU, true, "        flat_atomic_swap "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd070000U, 0x2f0041bbU, true, "        flat_atomic_cmpswap "
                "v47, v[187:188], v[65:66] glc slc\n" },
    { 0xdd0b0000U, 0x2f0041bbU, true, "        flat_atomic_add "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd0f0000U, 0x2f0041bbU, true, "        flat_atomic_sub "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd130000U, 0x2f0041bbU, true, "        flat_atomic_smin "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd170000U, 0x2f0041bbU, true, "        flat_atomic_umin "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd1b0000U, 0x2f0041bbU, true, "        flat_atomic_smax "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd1f0000U, 0x2f0041bbU, true, "        flat_atomic_umax "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd230000U, 0x2f0041bbU, true, "        flat_atomic_and "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd270000U, 0x2f0041bbU, true, "        flat_atomic_or  "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd2b0000U, 0x2f0041bbU, true, "        flat_atomic_xor "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd2f0000U, 0x2f0041bbU, true, "        flat_atomic_inc "
                "v47, v[187:188], v65 glc slc\n" },
    { 0xdd330000U, 0x2f0041bbU, true, "        flat_atomic_dec "
                "v47, v[187:188], v65 glc slc\n" }, 
    { 0xdd430000U, 0x2f8041bbU, true, "        FLAT_ill_80     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd470000U, 0x2f8041bbU, true, "        FLAT_ill_81     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd4b0000U, 0x2f8041bbU, true, "        FLAT_ill_82     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd4f0000U, 0x2f8041bbU, true, "        FLAT_ill_83     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd530000U, 0x2f8041bbU, true, "        FLAT_ill_84     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd570000U, 0x2f8041bbU, true, "        FLAT_ill_85     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd5b0000U, 0x2f8041bbU, true, "        FLAT_ill_86     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd5f0000U, 0x2f8041bbU, true, "        FLAT_ill_87     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd630000U, 0x2f8041bbU, true, "        FLAT_ill_88     "
                "v[47:48], v[187:188], v65 glc slc tfe\n" },
    { 0xdd830000U, 0x2f0041bbU, true, "        flat_atomic_swap_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdd870000U, 0x2f0041bbU, true, "        flat_atomic_cmpswap_x2 "
                "v[47:48], v[187:188], v[65:68] glc slc\n" },
    { 0xdd8b0000U, 0x2f0041bbU, true, "        flat_atomic_add_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdd8f0000U, 0x2f0041bbU, true, "        flat_atomic_sub_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdd930000U, 0x2f0041bbU, true, "        flat_atomic_smin_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdd970000U, 0x2f0041bbU, true, "        flat_atomic_umin_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdd9b0000U, 0x2f0041bbU, true, "        flat_atomic_smax_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdd9f0000U, 0x2f0041bbU, true, "        flat_atomic_umax_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdda30000U, 0x2f0041bbU, true, "        flat_atomic_and_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xdda70000U, 0x2f0041bbU, true, "        flat_atomic_or_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xddab0000U, 0x2f0041bbU, true, "        flat_atomic_xor_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xddaf0000U, 0x2f0041bbU, true, "        flat_atomic_inc_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0xddb30000U, 0x2f0041bbU, true, "        flat_atomic_dec_x2 "
                "v[47:48], v[187:188], v[65:66] glc slc\n" },
    { 0, 0, false, nullptr }
};
