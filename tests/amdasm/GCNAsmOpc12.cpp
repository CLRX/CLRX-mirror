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
#include "GCNAsmOpc.h"

const GCNAsmOpcodeCase encGCN12OpcodeCases[] =
{
    /* SOPK */
    { "    s_add_u32  flat_scratch_lo, s4, s61", 0x80663d04U, 0, false, true, "" },
    { "    s_add_u32  flat_scratch_hi, s4, s61", 0x80673d04U, 0, false, true, "" },
    { "    s_add_u32  xnack_mask_lo, s4, s61", 0x80683d04U, 0, false, true, "" },
    { "    s_add_u32  xnack_mask_hi, s4, s61", 0x80693d04U, 0, false, true, "" },
    { "    s_add_u32  xnack_mask_hi, 0.15915494, s61", 0x80693df8U, 0, false, true, "" },
    { "    s_add_u32  xnack_mask_hi, 15.915494e-2, s61", 0x80693df8U, 0, false, true, "" },
    // SOP2 instructions
    { "    s_addc_u32  s21, s4, s61", 0x82153d04U, 0, false, true, "" },
    { "    s_and_b32  s21, s4, s61", 0x86153d04U, 0, false, true, "" },
    { "    s_and_b64  s[20:21], s[4:5], s[62:63]", 0x86943e04U, 0, false, true, "" },
    { "    s_or_b32  s21, s4, s61", 0x87153d04U, 0, false, true, "" },
    { "    s_or_b64  s[20:21], s[4:5], s[62:63]", 0x87943e04U, 0, false, true, "" },
    { "    s_xor_b32  s21, s4, s61", 0x88153d04U, 0, false, true, "" },
    { "    s_xor_b64  s[20:21], s[4:5], s[62:63]", 0x88943e04U, 0, false, true, "" },
    { "    s_andn2_b32  s21, s4, s61", 0x89153d04U, 0, false, true, "" },
    { "    s_andn2_b64  s[20:21], s[4:5], s[62:63]", 0x89943e04U, 0, false, true, "" },
    { "    s_orn2_b32  s21, s4, s61", 0x8a153d04U, 0, false, true, "" },
    { "    s_orn2_b64  s[20:21], s[4:5], s[62:63]", 0x8a943e04U, 0, false, true, "" },
    { "    s_nand_b32  s21, s4, s61", 0x8b153d04U, 0, false, true, "" },
    { "    s_nand_b64  s[20:21], s[4:5], s[62:63]", 0x8b943e04U, 0, false, true, "" },
    { "    s_nor_b32  s21, s4, s61", 0x8c153d04U, 0, false, true, "" },
    { "    s_nor_b64  s[20:21], s[4:5], s[62:63]", 0x8c943e04U, 0, false, true, "" },
    { "    s_xnor_b32  s21, s4, s61", 0x8d153d04U, 0, false, true, "" },
    { "    s_xnor_b64  s[20:21], s[4:5], s[62:63]", 0x8d943e04U, 0, false, true, "" },
    { "    s_lshl_b32  s21, s4, s61", 0x8e153d04U, 0, false, true, "" },
    { "    s_lshl_b64  s[20:21], s[4:5], s61", 0x8e943d04U, 0, false, true, "" },
    { "    s_lshr_b32  s21, s4, s61", 0x8f153d04U, 0, false, true, "" },
    { "    s_lshr_b64  s[20:21], s[4:5], s61", 0x8f943d04U, 0, false, true, "" },
    { "    s_ashr_i32  s21, s4, s61", 0x90153d04U, 0, false, true, "" },
    { "    s_ashr_i64  s[20:21], s[4:5], s61", 0x90943d04U, 0, false, true, "" },
    { "    s_bfm_b32  s21, s4, s61", 0x91153d04U, 0, false, true, "" },
    { "    s_bfm_b64  s[20:21], s4, s62", 0x91943e04U, 0, false, true, "" },
    { "    s_mul_i32  s21, s4, s61", 0x92153d04U, 0, false, true, "" },
    { "    s_bfe_u32  s21, s4, s61", 0x92953d04U, 0, false, true, "" },
    { "    s_bfe_i32  s21, s4, s61", 0x93153d04U, 0, false, true, "" },
    { "    s_bfe_u64  s[20:21], s[4:5], s61", 0x93943d04U, 0, false, true, "" },
    { "    s_bfe_i64  s[20:21], s[4:5], s61", 0x94143d04U, 0, false, true, "" },
    { "    s_cbranch_g_fork  s[4:5], s[62:63]", 0x94803e04U, 0, false, true, "" },
    { "    s_absdiff_i32  s21, s4, s61", 0x95153d04U, 0, false, true, "" },
    { "    s_rfe_restore_b64 s[4:5], s61", 0x95803d04U, 0, false, true, "" },
    /* SOP1 encoding */
    { "    s_mov_b32  s86, s20", 0xbed60014U, 0, false, true, "" },
    { "    s_mov_b64  s[86:87], s[20:21]", 0xbed60114U, 0, false, true, "" },
    { "    s_cmov_b32  s86, s20", 0xbed60214U, 0, false, true, "" },
    { "    s_cmov_b64  s[86:87], s[20:21]", 0xbed60314U, 0, false, true, "" },
    { "    s_not_b32  s86, s20", 0xbed60414U, 0, false, true, "" },
    { "    s_not_b64  s[86:87], s[20:21]", 0xbed60514U, 0, false, true, "" },
    { "    s_wqm_b32  s86, s20", 0xbed60614U, 0, false, true, "" },
    { "    s_wqm_b64  s[86:87], s[20:21]", 0xbed60714U, 0, false, true, "" },
    { "    s_brev_b32  s86, s20", 0xbed60814U, 0, false, true, "" },
    { "    s_brev_b64  s[86:87], s[20:21]", 0xbed60914U, 0, false, true, "" },
    { "    s_bcnt0_i32_b32  s86, s20", 0xbed60a14U, 0, false, true, "" },
    { "    s_bcnt0_i32_b64  s86, s[20:21]", 0xbed60b14U, 0, false, true, "" },
    { "    s_bcnt1_i32_b32  s86, s20", 0xbed60c14U, 0, false, true, "" },
    { "    s_bcnt1_i32_b64  s86, s[20:21]", 0xbed60d14U, 0, false, true, "" },
    { "    s_ff0_i32_b32  s86, s20", 0xbed60e14U, 0, false, true, "" },
    { "    s_ff0_i32_b64  s86, s[20:21]", 0xbed60f14U, 0, false, true, "" },
    { "    s_ff1_i32_b32  s86, s20", 0xbed61014U, 0, false, true, "" },
    { "    s_ff1_i32_b64  s86, s[20:21]", 0xbed61114U, 0, false, true, "" },
    { "    s_flbit_i32_b32  s86, s20", 0xbed61214U, 0, false, true, "" },
    { "    s_flbit_i32_b64  s86, s[20:21]", 0xbed61314U, 0, false, true, "" },
    { "    s_flbit_i32  s86, s20", 0xbed61414U, 0, false, true, "" },
    { "    s_flbit_i32_i64  s86, s[20:21]", 0xbed61514U, 0, false, true, "" },
    { "    s_sext_i32_i8  s86, s20", 0xbed61614U, 0, false, true, "" },
    { "    s_sext_i32_i16  s86, s20", 0xbed61714U, 0, false, true, "" },
    { "    s_bitset0_b32  s86, s20", 0xbed61814U, 0, false, true, "" },
    { "    s_bitset0_b64  s[86:87], s20", 0xbed61914U, 0, false, true, "" },
    { "    s_bitset1_b32  s86, s20", 0xbed61a14U, 0, false, true, "" },
    { "    s_bitset1_b64  s[86:87], s20", 0xbed61b14U, 0, false, true, "" },
    { "    s_getpc_b64  s[86:87]", 0xbed61c00U, 0, false, true, "" },
    { "    s_setpc_b64  s[20:21]", 0xbe801d14U, 0, false, true, "" },
    { "    s_swappc_b64  s[86:87], s[20:21]", 0xbed61e14U, 0, false, true, "" },
    { "    s_rfe_b64  s[20:21]", 0xbe801f14U, 0, false, true, "" },
    { "    s_and_saveexec_b64 s[86:87], s[20:21]", 0xbed62014U, 0, false, true, "" },
    { "    s_or_saveexec_b64 s[86:87], s[20:21]", 0xbed62114U, 0, false, true, "" },
    { "    s_xor_saveexec_b64 s[86:87], s[20:21]", 0xbed62214U, 0, false, true, "" },
    { "    s_andn2_saveexec_b64 s[86:87], s[20:21]", 0xbed62314U, 0, false, true, "" },
    { "    s_orn2_saveexec_b64 s[86:87], s[20:21]", 0xbed62414U, 0, false, true, "" },
    { "    s_nand_saveexec_b64 s[86:87], s[20:21]", 0xbed62514U, 0, false, true, "" },
    { "    s_nor_saveexec_b64 s[86:87], s[20:21]", 0xbed62614U, 0, false, true, "" },
    { "    s_xnor_saveexec_b64 s[86:87], s[20:21]", 0xbed62714U, 0, false, true, "" },
    { "    s_quadmask_b32  s86, s20",  0xbed62814U, 0, false, true, "" },
    { "    s_quadmask_b64  s[86:87], s[20:21]",  0xbed62914U, 0, false, true, "" },
    { "    s_movrels_b32  s86, s20",  0xbed62a14U, 0, false, true, "" },
    { "    s_movrels_b64  s[86:87], s[20:21]",  0xbed62b14U, 0, false, true, "" },
    { "    s_movreld_b32  s86, s20",  0xbed62c14U, 0, false, true, "" },
    { "    s_movreld_b64  s[86:87], s[20:21]",  0xbed62d14U, 0, false, true, "" },
    { "    s_cbranch_join  s20", 0xbe802e14U, 0, false, true, "" },
    { "    s_mov_regrd_b32 s86, s20", 0xbed62f14U, 0, false, true, "" },
    { "    s_abs_i32  s86, s20", 0xbed63014U, 0, false, true, "" },
    { "    s_mov_fed_b32  s86, s20", 0xbed63114U, 0, false, true, "" },
    { "    s_set_gpr_idx_idx s20", 0xbe803214U, 0, false, true, "" },
    /* SOPC encoding */
    { "    s_cmp_lt_i32  s29, s69", 0xbf04451dU, 0, false, true, "" },
    { "    s_bitcmp1_b32  s29, s69", 0xbf0d451dU, 0, false, true, "" },
    { "    s_bitcmp0_b64  s[28:29], s69", 0xbf0e451cU, 0, false, true, "" },
    { "    s_setvskip  s29, s69", 0xbf10451dU, 0, false, true, "" },
    /* SOPC new instructions */
    { "    s_set_gpr_idx_on s29, 0x45", 0xbf11451dU, 0, false, true, "" },
    { "xd=43; s_set_gpr_idx_on s29, 4+xd", 0xbf112f1dU, 0, false, true, "" },
    { "s_set_gpr_idx_on s29, 4+xd;xd=43", 0xbf112f1dU, 0, false, true, "" },
    { "    s_cmp_eq_u64  s[28:29], s[68:69]", 0xbf12441cU, 0, false, true, "" },
    { "    s_cmp_lg_u64  s[28:29], s[68:69]", 0xbf13441cU, 0, false, true, "" },
    { "    s_cmp_ne_u64  s[28:29], s[68:69]", 0xbf13441cU, 0, false, true, "" },
    /* SOPK encoding */
    { "    s_movk_i32      s43, 0xd3b9", 0xb02bd3b9U, 0, false, true, "" },
    { "    s_cmovk_i32  s43, 0xd3b9", 0xb0abd3b9U, 0, false, true, "" },
    { "    s_cmpk_eq_i32  s43, 0xd3b9", 0xb12bd3b9U, 0, false, true, "" },
    { "    s_cmpk_lg_i32  s43, 0xd3b9", 0xb1abd3b9U, 0, false, true, "" },
    { "    s_cmpk_gt_i32  s43, 0xd3b9", 0xb22bd3b9U, 0, false, true, "" },
    { "    s_cmpk_ge_i32  s43, 0xd3b9", 0xb2abd3b9U, 0, false, true, "" },
    { "    s_cmpk_lt_i32  s43, 0xd3b9", 0xb32bd3b9U, 0, false, true, "" },
    { "    s_cmpk_le_i32  s43, 0xd3b9", 0xb3abd3b9U, 0, false, true, "" },
    { "    s_cmpk_eq_u32  s43, 0xd3b9", 0xb42bd3b9U, 0, false, true, "" },
    { "    s_cmpk_lg_u32  s43, 0xd3b9", 0xb4abd3b9U, 0, false, true, "" },
    { "    s_cmpk_gt_u32  s43, 0xd3b9", 0xb52bd3b9U, 0, false, true, "" },
    { "    s_cmpk_ge_u32  s43, 0xd3b9", 0xb5abd3b9U, 0, false, true, "" },
    { "    s_cmpk_lt_u32  s43, 0xd3b9", 0xb62bd3b9U, 0, false, true, "" },
    { "    s_cmpk_le_u32  s43, 0xd3b9", 0xb6abd3b9U, 0, false, true, "" },
    { "    s_addk_i32  s43, 0xd3b9", 0xb72bd3b9U, 0, false, true, "" },
    { "    s_mulk_i32  s43, 0xd3b9", 0xb7abd3b9U, 0, false, true, "" },
    { "    s_cbranch_i_fork s[44:45], xxxx-8\nxxxx:\n", 0xb82cfffeU, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(mode, 0, 1)", 0xb8ab0001U, 0, false, true, "" },
    { "    s_setreg_b32  hwreg(trapsts, 3, 10), s43", 0xb92b48c3u, 0, false, true, "" },
    { "    s_setreg_imm32_b32 hwreg(trapsts, 3, 10), 0x24da4f",
                    0xba0048c3u, 0x24da4fU, true, true, "" },
    /* SOPP encoding */
    { nullptr, 0, 0, false, false, 0 }
};
