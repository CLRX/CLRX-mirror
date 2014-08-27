/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
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
#include "AsmInternals.h"

using namespace CLRX;

static const GCNInstruction gcnInstrTables[] =
{
    { GCNEncoding::SOP2, "s_add_u32", REG_ALL_32, 0, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_sub_u32", REG_ALL_32, 1, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_add_i32", REG_ALL_32, 2, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_sub_i32", REG_ALL_32, 3, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_addc_u32", REG_ALL_32, 4, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_subb_u32", REG_ALL_32, 5, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_min_i32", REG_ALL_32, 6, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_min_u32", REG_ALL_32, 7, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_max_i32", REG_ALL_32, 8, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_max_u32", REG_ALL_32, 9, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_cselect_b32", REG_ALL_32, 10, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_cselect_b64", REG_ALL_64, 11, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_and_b32", REG_ALL_32, 14, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_and_b64", REG_ALL_64, 15, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_or_b32", REG_ALL_32, 16, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_or_b64", REG_ALL_64, 17, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_xor_b32", REG_ALL_32, 18, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_xor_b64", REG_ALL_64, 19, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_andn2_b32", REG_ALL_32, 20, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_andn2_b64", REG_ALL_64, 21, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_orn2_b32", REG_ALL_32, 22, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_orn2_b64", REG_ALL_64, 23, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_nand_b32", REG_ALL_32, 24, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_nand_b64", REG_ALL_64, 25, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_nor_b32", REG_ALL_32, 26, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_nor_b64", REG_ALL_64, 27, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_xnor_b32", REG_ALL_32, 28, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_xnor_b64", REG_ALL_64, 29, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_lshl_b32", REG_ALL_32, 30, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_lshl_b64", REG_DS0_64, 31, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_lshr_b32", REG_ALL_32, 32, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_lshr_b64", REG_DS0_64, 33, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_ashr_i32", REG_ALL_32, 34, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_ashr_i64", REG_DS0_64, 35, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_bfm_b32", REG_ALL_32, 36, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_bfm_b64", REG_DST_64, 37, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_mul_i32", REG_ALL_32, 38, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_bfe_u32", REG_ALL_32, 39, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_bfe_i32", REG_ALL_32, 40, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_bfe_u64", REG_DST_64, 41, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_bfe_i64", REG_DST_64, 42, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_cbranch_g_fork", REG_DS0_64, 43, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP2, "s_absdiff_i32", REG_ALL_32, 44, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_movk_i32", REG_ALL_32, 0, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmovk_i32", REG_ALL_32, 2, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_eq_i32", REG_ALL_32, 3, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_lg_i32", REG_ALL_32, 4, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_gt_i32", REG_ALL_32, 5, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_ge_i32", REG_ALL_32, 6, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_lt_i32", REG_ALL_32, 7, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_le_i32", REG_ALL_32, 8, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_eq_u32", REG_ALL_32, 9, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_lg_u32", REG_ALL_32, 10, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_gt_u32", REG_ALL_32, 11, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_ge_u32", REG_ALL_32, 12, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_lt_u32", REG_ALL_32, 13, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cmp_le_u32", REG_ALL_32, 14, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_addk_i32", REG_ALL_32, 15, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_mulk_i32", REG_ALL_32, 16, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_cbranch_i_fork", REG_DST_64, 17, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_getreg_b32", REG_ALL_32, 18, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_setreg_b32", REG_ALL_32, 19, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOPK, "s_setreg_imm32_b32", REG_ALL_32, 21, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_mov_b32", REG_ALL_32, 3, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_mov_b64", REG_ALL_64, 4, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_cmov_b32", REG_ALL_32, 5, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_cmov_b64", REG_ALL_64, 6, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_not_b32", REG_ALL_32, 7, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_not_b64", REG_ALL_64, 8, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_wqm_b32", REG_ALL_32, 9, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_wqm_b64", REG_ALL_64, 10, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_brev_b32", REG_ALL_32, 11, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_brev_b64", REG_ALL_64, 12, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bcnt0_i32_b32", REG_ALL_32, 13, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bcnt0_i32_b64", REG_ALL_64, 14, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bcnt1_i32_b32", REG_ALL_32, 15, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bcnt1_i32_b64", REG_ALL_64, 16, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_ff0_i32_b32", REG_ALL_32, 17, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_ff0_i32_b64", REG_ALL_64, 18, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_ff1_i32_b32", REG_ALL_32, 19, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_ff1_i32_b64", REG_ALL_64, 20, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_flbit_i32_b32", REG_ALL_32, 21, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_flbit_i32_b64", REG_ALL_64, 22, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_flbit_i32", REG_ALL_32, 23, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_flbit_i32_i64", REG_ALL_64, 24, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_sext_32_i8", REG_ALL_32, 25, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_sext_32_i16", REG_ALL_32, 26, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bitset0_b32", REG_ALL_32, 27, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bitset0_b64", REG_ALL_64, 28, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bitset1_b32", REG_ALL_32, 29, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_bitset1_b64", REG_ALL_64, 30, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_getpc_b64", REG_ALL_64, 31, ARCH_HD7X00|ARCH_RX2X0 },
    { GCNEncoding::SOP1, "s_setpc_b64", REG_ALL_64, 32, ARCH_HD7X00|ARCH_RX2X0 },
};
