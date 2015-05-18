/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
    { 0xb82b0000U, 0, false, "        s_cbranch_i_fork s[43:44], .L4\n" },
    { 0xb82bffffU, 0, false, "        s_cbranch_i_fork s[43:44], .L0\n" },
    { 0xb82b000aU, 0, false, "        s_cbranch_i_fork s[43:44], .L44\n" },
    { 0xb8ab0000u, 0, false, "        s_getreg_b32    s43, hwreg(0, 0, 1)\n" },
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
    { 0xbf820029U, 0, false, "        s_branch        .L168\n" },
    { 0xbf82ffffU, 0, false, "        s_branch        .L0\n" },
    { 0xbf830000U, 0, false, "        s_wakeup\n" },
    { 0xbf838d33U, 0, false, "        s_wakeup        0x8d33\n" },
    { 0xbf840029U, 0, false, "        s_cbranch_scc0  .L168\n" },
    { 0xbf850029U, 0, false, "        s_cbranch_scc1  .L168\n" },
    { 0xbf860029U, 0, false, "        s_cbranch_vccz  .L168\n" },
    { 0xbf870029U, 0, false, "        s_cbranch_vccnz .L168\n" },
    { 0xbf880029U, 0, false, "        s_cbranch_execz .L168\n" },
    { 0xbf890029U, 0, false, "        s_cbranch_execnz .L168\n" },
    { 0xbf8a0000U, 0, false, "        s_barrier\n" },
    { 0xbf8b032bU, 0, false, "        s_setkill       0x32b\n" },
    { 0xbf8c0d36U, 0, false, "        s_waitcnt       "
        "vmcnt(6) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0d3fU, 0, false, "        s_waitcnt       expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0d3eU, 0, false, "        s_waitcnt       "
        "vmcnt(14) & expcnt(3) & lgkmcnt(13)\n" },
    { 0xbf8c0d7eU, 0, false, "        s_waitcnt       vmcnt(14) & lgkmcnt(13)\n" },
    { 0xbf8c0f7eU, 0, false, "        s_waitcnt       vmcnt(14)\n" },
    { 0xbf8c0f7fU, 0, false, "        s_waitcnt       0xf7f\n" },
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
    { 0xbf90001bU, 0, false, "        s_sendmsg       sendmsg(11, cut, 0)\n" },
    { 0xbf91001bU, 0, false, "        s_sendmsghalt   sendmsg(11, cut, 0)\n" },
    { 0xbf92032bU, 0, false, "        s_trap          0x32b\n" },
    { 0xbf93032bU, 0, false, "        s_icache_inv    0x32b\n" },
    { 0xbf930000U, 0, false, "        s_icache_inv\n" },
    { 0xbf941234U, 0, false, "        s_incperflevel  0x1234\n" },
    { 0xbf951234U, 0, false, "        s_decperflevel  0x1234\n" },
    { 0xbf960000U, 0, false, "        s_ttracedata\n" },
    { 0xbf960dcaU, 0, false, "        s_ttracedata    0xdca\n" },
    { 0xbf970029U, 0, false, "        s_cbranch_cdbgsys .L168\n" },
    { 0xbf980029U, 0, false, "        s_cbranch_cdbguser .L168\n" },
    { 0xbf990029U, 0, false, "        s_cbranch_cdbgsys_or_user .L168\n" },
    { 0xbf9a0029U, 0, false, "        s_cbranch_cdbgsys_and_user .L168\n" },
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
    { 0x5534d715U, 0, false, "        v_lshlrev_u16   v154, v21, v107\n" },
    { 0x5734d715U, 0, false, "        v_lshrrev_u16   v154, v21, v107\n" },
    { 0x5934d715U, 0, false, "        v_ashrrev_u16   v154, v21, v107\n" },
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
    { 0x7e0000f9U, 0x3d003d, true,"        v_nop           "
        "src0=0xf9 dst_sel:byte0 src0_sel:word1 src1_sel:byte0 sext0 neg0 abs0\n" },
    { 0x7e0000f9U, 0x3d00003d, true,"        v_nop           "
        "src0=0xf9 dst_sel:byte0 src0_sel:byte0 src1_sel:word1 sext1 neg1 abs1\n" },
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
    { 0x7f3c16ffU, 0x3d4cU, true, "        v_cvt_f32_f16   v158, "
                "0x3d4c /* 1.3242h */\n" },
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
    
    { 0, 0, false, nullptr }
};
