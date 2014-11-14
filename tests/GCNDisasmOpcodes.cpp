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
#include <iostream>
#include <sstream>
#include <CLRX/Utilities.h>
#include <CLRX/Assembler.h>

using namespace CLRX;

struct GCNDisasmOpcodeCase
{
    uint32_t word0, word1;
    bool twoWords;
    const char* expected;
};

static const GCNDisasmOpcodeCase decGCNOpcodeCases[] =
{   /* SOP2 encoding */
    { 0x80153d04U, 0, false, "        s_add_u32       s21, s4, s61\n" },
    { 0x807f3d04U, 0, false, "        s_add_u32       exec_hi, s4, s61\n" },
    { 0x807f3df1U, 0, false, "        s_add_u32       exec_hi, -0.5, s61\n" },
    { 0x807f3dffU, 0xd3abc5f, true, "        s_add_u32       exec_hi, 0xd3abc5f, s61\n" },
    { 0x807fff05U, 0xd3abc5f, true, "        s_add_u32       exec_hi, s5, 0xd3abc5f\n" },
    { 0x80ff3df1U, 0, false, "        s_sub_u32       exec_hi, -0.5, s61\n" },
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
    { 0x86153d04U, 0, false, "        SOP2_ill_12     s21, s4, s61\n" },
    { 0x86953d04U, 0, false, "        SOP2_ill_13     s21, s4, s61\n" },
    { 0x87153d04U, 0, false, "        s_and_b32       s21, s4, s61\n" },
    { 0x87953d04U, 0, false, "        s_and_b64       s[21:22], s[4:5], s[61:62]\n" },
    { 0x88153d04U, 0, false, "        s_or_b32        s21, s4, s61\n" },
    { 0x88953d04U, 0, false, "        s_or_b64        s[21:22], s[4:5], s[61:62]\n" },
    { 0x89153d04U, 0, false, "        s_xor_b32       s21, s4, s61\n" },
    { 0x89953d04U, 0, false, "        s_xor_b64       s[21:22], s[4:5], s[61:62]\n" },
    { 0x8a153d04U, 0, false, "        s_andn2_b32     s21, s4, s61\n" },
    { 0x8a953d04U, 0, false, "        s_andn2_b64     s[21:22], s[4:5], s[61:62]\n" },
    { 0x8b153d04U, 0, false, "        s_orn2_b32      s21, s4, s61\n" },
    { 0x8b953d04U, 0, false, "        s_orn2_b64      s[21:22], s[4:5], s[61:62]\n" },
    { 0x8c153d04U, 0, false, "        s_nand_b32      s21, s4, s61\n" },
    { 0x8c953d04U, 0, false, "        s_nand_b64      s[21:22], s[4:5], s[61:62]\n" },
    { 0x8d153d04U, 0, false, "        s_nor_b32       s21, s4, s61\n" },
    { 0x8d953d04U, 0, false, "        s_nor_b64       s[21:22], s[4:5], s[61:62]\n" },
    { 0x8e153d04U, 0, false, "        s_xnor_b32      s21, s4, s61\n" },
    { 0x8e953d04U, 0, false, "        s_xnor_b64      s[21:22], s[4:5], s[61:62]\n" },
    { 0x8f153d04U, 0, false, "        s_lshl_b32      s21, s4, s61\n" },
    { 0x8f953d04U, 0, false, "        s_lshl_b64      s[21:22], s[4:5], s61\n" },
    { 0x90153d04U, 0, false, "        s_lshr_b32      s21, s4, s61\n" },
    { 0x90953d04U, 0, false, "        s_lshr_b64      s[21:22], s[4:5], s61\n" },
    { 0x91153d04U, 0, false, "        s_ashr_i32      s21, s4, s61\n" },
    { 0x91953d04U, 0, false, "        s_ashr_i64      s[21:22], s[4:5], s61\n" },
    { 0x92153d04U, 0, false, "        s_bfm_b32       s21, s4, s61\n" },
    { 0x92953d04U, 0, false, "        s_bfm_b64       s[21:22], s4, s61\n" },
    { 0x93153d04U, 0, false, "        s_mul_i32       s21, s4, s61\n" },
    { 0x93953d04U, 0, false, "        s_bfe_u32       s21, s4, s61\n" },
    { 0x94153d04U, 0, false, "        s_bfe_i32       s21, s4, s61\n" },
    { 0x94953d04U, 0, false, "        s_bfe_u64       s[21:22], s[4:5], s61\n" },
    { 0x95153d04U, 0, false, "        s_bfe_i64       s[21:22], s[4:5], s61\n" },
    { 0x95953d04U, 0, false, "        s_cbranch_g_fork s[4:5], s[61:62] sdst=0x15\n" },
    { 0x95803d04U, 0, false, "        s_cbranch_g_fork s[4:5], s[61:62]\n" },
    { 0x96153d04U, 0, false, "        s_absdiff_i32   s21, s4, s61\n" },
    { 0x96953d04U, 0, false, "        SOP2_ill_45     s21, s4, s61\n" },
    { 0x97153d04U, 0, false, "        SOP2_ill_46     s21, s4, s61\n" },
    { 0x97953d04U, 0, false, "        SOP2_ill_47     s21, s4, s61\n" },
    { 0x98153d04U, 0, false, "        SOP2_ill_48     s21, s4, s61\n" },
    /* SOPK encoding */
    { 0xb02b0000U, 0, false, "        s_movk_i32      s43, 0x0\n" },
    { 0xb02bd3b9U, 0, false, "        s_movk_i32      s43, 0xd3b9\n" },
    { 0xb0abd3b9U, 0, false, "        SOPK_ill_1      s43, 0xd3b9\n" },
    { 0xb12bd3b9U, 0, false, "        s_cmovk_i32     s43, 0xd3b9\n" },
    { 0xb1abd3b9U, 0, false, "        s_cmpk_eq_i32   s43, 0xd3b9\n" },
    { 0xb22bd3b9U, 0, false, "        s_cmpk_lg_i32   s43, 0xd3b9\n" },
    { 0xb2abd3b9U, 0, false, "        s_cmpk_gt_i32   s43, 0xd3b9\n" },
    { 0xb32bd3b9U, 0, false, "        s_cmpk_ge_i32   s43, 0xd3b9\n" },
    { 0xb3abd3b9U, 0, false, "        s_cmpk_lt_i32   s43, 0xd3b9\n" },
    { 0xb42bd3b9U, 0, false, "        s_cmpk_le_i32   s43, 0xd3b9\n" },
    { 0xb4abd3b9U, 0, false, "        s_cmpk_eq_u32   s43, 0xd3b9\n" },
    { 0xb52bd3b9U, 0, false, "        s_cmpk_lg_u32   s43, 0xd3b9\n" },
    { 0xb5abd3b9U, 0, false, "        s_cmpk_gt_u32   s43, 0xd3b9\n" },
    { 0xb62bd3b9U, 0, false, "        s_cmpk_ge_u32   s43, 0xd3b9\n" },
    { 0xb6abd3b9U, 0, false, "        s_cmpk_lt_u32   s43, 0xd3b9\n" },
    { 0xb72bd3b9U, 0, false, "        s_cmpk_le_u32   s43, 0xd3b9\n" },
    { 0xb7abd3b9U, 0, false, "        s_addk_i32      s43, 0xd3b9\n" },
    { 0xb82bd3b9U, 0, false, "        s_mulk_i32      s43, 0xd3b9\n" },
    { 0xb8ab0000U, 0, false, "        s_cbranch_i_fork s[43:44], L1\n" },
    { 0xb8abffffU, 0, false, "        s_cbranch_i_fork s[43:44], L0\n" },
    { 0xb8ab000aU, 0, false, "        s_cbranch_i_fork s[43:44], L11\n" },
    { 0xb92b0000U, 0, false, "        s_getreg_b32    s43, hwreg(0, 0, 1)\n" },
    { 0xb92b0001U, 0, false, "        s_getreg_b32    s43, hwreg(MODE, 0, 1)\n" },
    { 0xb92b0002U, 0, false, "        s_getreg_b32    s43, hwreg(STATUS, 0, 1)\n" },
    { 0xb92b0003U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 0, 1)\n" },
    { 0xb92b0004U, 0, false, "        s_getreg_b32    s43, hwreg(HW_ID, 0, 1)\n" },
    { 0xb92b0005U, 0, false, "        s_getreg_b32    s43, hwreg(GPR_ALLOC, 0, 1)\n" },
    { 0xb92b0006U, 0, false, "        s_getreg_b32    s43, hwreg(LDS_ALLOC, 0, 1)\n" },
    { 0xb92b0007U, 0, false, "        s_getreg_b32    s43, hwreg(IB_STS, 0, 1)\n" },
    { 0xb92b0008U, 0, false, "        s_getreg_b32    s43, hwreg(PC_LO, 0, 1)\n" },
    { 0xb92b0009U, 0, false, "        s_getreg_b32    s43, hwreg(PC_HI, 0, 1)\n" },
    { 0xb92b000aU, 0, false, "        s_getreg_b32    s43, hwreg(INST_DW0, 0, 1)\n" },
    { 0xb92b000bU, 0, false, "        s_getreg_b32    s43, hwreg(INST_DW1, 0, 1)\n" },
    { 0xb92b000cU, 0, false, "        s_getreg_b32    s43, hwreg(IB_DBG0, 0, 1)\n" },
    { 0xb92b000dU, 0, false, "        s_getreg_b32    s43, hwreg(13, 0, 1)\n" },
    { 0xb92b000eU, 0, false, "        s_getreg_b32    s43, hwreg(14, 0, 1)\n" },
    { 0xb92b0014U, 0, false, "        s_getreg_b32    s43, hwreg(20, 0, 1)\n" },
    { 0xb92b001eU, 0, false, "        s_getreg_b32    s43, hwreg(30, 0, 1)\n" },
    { 0xb92b003fU, 0, false, "        s_getreg_b32    s43, hwreg(63, 0, 1)\n" },
    { 0xb92b00c3U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 3, 1)\n" },
    { 0xb92b03c3U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 15, 1)\n" },
    { 0xb92b0283U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 10, 1)\n" },
    { 0xb92b0583U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 22, 1)\n" },
    { 0xb92b07c3U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 31, 1)\n" },
    { 0xb92b28c3U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 3, 6)\n" },
    { 0xb92b48c3U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 3, 10)\n" },
    { 0xb92bf8c3U, 0, false, "        s_getreg_b32    s43, hwreg(TRAPSTS, 3, 32)\n" },
    { 0xb9ab48c3U, 0, false, "        s_setreg_b32    hwreg(TRAPSTS, 3, 10), s43\n" },
    { 0xba2bf8c3U, 0, false, "        s_getreg_regrd_b32 s43, hwreg(TRAPSTS, 3, 32)\n" },
    { 0xbaab48c3U, 0x45d2a, true, "        s_setreg_imm32_b32 hwreg(TRAPSTS, 3, 10), "
                "0x45d2a sdst=0x2b\n" },
    { 0xba8048c3U, 0x45d2a, true, "        s_setreg_imm32_b32 hwreg(TRAPSTS, 3, 10), "
                "0x45d2a\n" },
    { 0xbb2bd3b9U, 0, false, "        SOPK_ill_22     s43, 0xd3b9\n" },
    { 0xbbabd3b9U, 0, false, "        SOPK_ill_23     s43, 0xd3b9\n" },
    { 0xbc2bd3b9U, 0, false, "        SOPK_ill_24     s43, 0xd3b9\n" },
    { 0xbcabd3b9U, 0, false, "        SOPK_ill_25     s43, 0xd3b9\n" },
    { 0xbd2bd3b9U, 0, false, "        SOPK_ill_26     s43, 0xd3b9\n" },
    { 0xbdabd3b9U, 0, false, "        SOPK_ill_27     s43, 0xd3b9\n" },
    /* SOP1 encoding */
    { 0xbed60014U, 0, false, "        SOP1_ill_0      s86, s20\n" },
    { 0xbed60114U, 0, false, "        SOP1_ill_1      s86, s20\n" },
    { 0xbed60214U, 0, false, "        SOP1_ill_2      s86, s20\n" },
    { 0xbed60314U, 0, false, "        s_mov_b32       s86, s20\n" },
    { 0xbed603ffU, 0xddbbaa11, true, "        s_mov_b32       s86, 0xddbbaa11\n" },
    { 0xbed60414U, 0, false, "        s_mov_b64       s[86:87], s[20:21]\n" },
    { 0xbed60514U, 0, false, "        s_cmov_b32      s86, s20\n" },
    { 0xbed60614U, 0, false, "        s_cmov_b64      s[86:87], s[20:21]\n" },
    { 0xbed60714U, 0, false, "        s_not_b32       s86, s20\n" },
    { 0xbed60814U, 0, false, "        s_not_b64       s[86:87], s[20:21]\n" },
    { 0xbed60914U, 0, false, "        s_wqm_b32       s86, s20\n" },
    { 0xbed60a14U, 0, false, "        s_wqm_b64       s[86:87], s[20:21]\n" },
    { 0xbed60b14U, 0, false, "        s_brev_b32      s86, s20\n" },
    { 0xbed60c14U, 0, false, "        s_brev_b64      s[86:87], s[20:21]\n" },
    { 0xbed60d14U, 0, false, "        s_bcnt0_i32_b32 s86, s20\n" },
    { 0xbed60e14U, 0, false, "        s_bcnt0_i32_b64 s86, s[20:21]\n" },
    { 0xbed60f14U, 0, false, "        s_bcnt1_i32_b32 s86, s20\n" },
    { 0xbed61014U, 0, false, "        s_bcnt1_i32_b64 s86, s[20:21]\n" },
    { 0xbed61114U, 0, false, "        s_ff0_i32_b32   s86, s20\n" },
    { 0xbed61214U, 0, false, "        s_ff0_i32_b64   s86, s[20:21]\n" },
    { 0xbed61314U, 0, false, "        s_ff1_i32_b32   s86, s20\n" },
    { 0xbed61414U, 0, false, "        s_ff1_i32_b64   s86, s[20:21]\n" },
    { 0xbed61514U, 0, false, "        s_flbit_i32_b32 s86, s20\n" },
    { 0xbed61614U, 0, false, "        s_flbit_i32_b64 s86, s[20:21]\n" },
    { 0xbed61714U, 0, false, "        s_flbit_i32     s86, s20\n" },
    { 0xbed61814U, 0, false, "        s_flbit_i32_i64 s86, s[20:21]\n" },
    { 0xbed61914U, 0, false, "        s_sext_i32_i8   s86, s20\n" },
    { 0xbed61a14U, 0, false, "        s_sext_i32_i16  s86, s20\n" },
    { 0xbed61b14U, 0, false, "        s_bitset0_b32   s86, s20\n" },
    { 0xbed61c14U, 0, false, "        s_bitset0_b64   s[86:87], s20\n" },
    { 0xbed61d14U, 0, false, "        s_bitset1_b32   s86, s20\n" },
    { 0xbed61e14U, 0, false, "        s_bitset1_b64   s[86:87], s20\n" },
    { 0xbed61f00U, 0, false, "        s_getpc_b64     s[86:87]\n" },
    { 0xbed61f14U, 0, false, "        s_getpc_b64     s[86:87] ssrc=0x14\n" },
    { 0xbe802014U, 0, false, "        s_setpc_b64     s[20:21]\n" },
    { 0xbed62014U, 0, false, "        s_setpc_b64     s[20:21] sdst=0x56\n" },
    { 0xbed62114U, 0, false, "        s_swappc_b64    s[86:87], s[20:21]\n" },
    { 0xbe802214U, 0, false, "        s_rfe_b64       s[20:21]\n" },
    { 0xbed62214U, 0, false, "        s_rfe_b64       s[20:21] sdst=0x56\n" },
    { 0xbed62314U, 0, false, "        SOP1_ill_35     s86, s20\n" },
    { 0xbed62414U, 0, false, "        s_and_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62514U, 0, false, "        s_or_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62614U, 0, false, "        s_xor_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62714U, 0, false, "        s_andn2_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62814U, 0, false, "        s_orn2_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62914U, 0, false, "        s_nand_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62a14U, 0, false, "        s_nor_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62b14U, 0, false, "        s_xnor_saveexec_b64 s[86:87], s[20:21]\n" },
    { 0xbed62c14U, 0, false, "        s_quadmask_b32  s86, s20\n" },
    { 0xbed62d14U, 0, false, "        s_quadmask_b64  s[86:87], s[20:21]\n" },
    { 0xbed62e14U, 0, false, "        s_movrels_b32   s86, s20\n" },
    { 0xbed62f14U, 0, false, "        s_movrels_b64   s[86:87], s[20:21]\n" },
    { 0xbed63014U, 0, false, "        s_movreld_b32   s86, s20\n" },
    { 0xbed63114U, 0, false, "        s_movreld_b64   s[86:87], s[20:21]\n" },
    { 0xbe803214U, 0, false, "        s_cbranch_join  s20\n" },
    { 0xbed63214U, 0, false, "        s_cbranch_join  s20 sdst=0x56\n" },
    { 0xbed63314U, 0, false, "        s_mov_regrd_b32 s86, s20\n" },
    { 0xbed63414U, 0, false, "        s_abs_i32       s86, s20\n" },
    { 0xbed63514U, 0, false, "        s_mov_fed_b32   s86, s20\n" },
    { 0xbed63614U, 0, false, "        SOP1_ill_54     s86, s20\n" },
    { 0xbed63714U, 0, false, "        SOP1_ill_55     s86, s20\n" },
    { 0xbed63814U, 0, false, "        SOP1_ill_56     s86, s20\n" },
    { 0xbed63914U, 0, false, "        SOP1_ill_57     s86, s20\n" },
    { 0xbed63a14U, 0, false, "        SOP1_ill_58     s86, s20\n" },
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
    { 0xbf11451dU, 0, false, "        SOPC_ill_17     s29, s69\n" },
    { 0xbf12451dU, 0, false, "        SOPC_ill_18     s29, s69\n" },
    { 0xbf13451dU, 0, false, "        SOPC_ill_19     s29, s69\n" },
    { 0xbf14451dU, 0, false, "        SOPC_ill_20     s29, s69\n" },
    /* SOPP encoding */
    { 0xbf800000U, 0, false, "        s_nop           0x0\n" },
    { 0xbf800006U, 0, false, "        s_nop           0x6\n" },
    { 0xbf80cd26U, 0, false, "        s_nop           0xcd26\n" },
    { 0xbf810000U, 0, false, "        s_endpgm\n" },
    { 0xbf818d33U, 0, false, "        s_endpgm        0x8d33\n" },
    { 0xbf820029U, 0, false, "        s_branch        L42\n" },
    { 0xbf82ffffU, 0, false, "        s_branch        L0\n" },
    { 0xbf830020U, 0, false, "        SOPP_ill_3      0x20\n" },
    { 0xbf840029U, 0, false, "        s_cbranch_scc0  L42\n" },
    { 0xbf850029U, 0, false, "        s_cbranch_scc1  L42\n" },
    { 0xbf860029U, 0, false, "        s_cbranch_vccz  L42\n" },
    { 0xbf870029U, 0, false, "        s_cbranch_vccnz L42\n" },
    { 0xbf880029U, 0, false, "        s_cbranch_execz L42\n" },
    { 0xbf890029U, 0, false, "        s_cbranch_execnz L42\n" },
    { 0xbf8a0000U, 0, false, "        s_barrier\n" },
    { 0xbf8b0020U, 0, false, "        SOPP_ill_11     0x20\n" },
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
    { 0xbf90000bU, 0, false, "        s_sendmsg       sendmsg(11, nop, 0)\n" },
    { 0xbf90000fU, 0, false, "        s_sendmsg       sendmsg(system)\n" },
    { 0xbf90021fU, 0, false, "        s_sendmsg       sendmsg(system, cut, 2)\n" },
    { 0xbf900a1fU, 0, false, "        s_sendmsg       sendmsg(system, cut, 2) :0xa1f\n" },
    { 0xbf900002U, 0, false, "        s_sendmsg       sendmsg(gs, nop, 0)\n" },
    { 0xbf900003U, 0, false, "        s_sendmsg       sendmsg(gs_done, nop, 0)\n" },
    { 0xbf900022U, 0, false, "        s_sendmsg       sendmsg(gs, emit, 0)\n" },
    { 0xbf900032U, 0, false, "        s_sendmsg       sendmsg(gs, emit-cut, 0)\n" },
    { 0xbf900322U, 0, false, "        s_sendmsg       sendmsg(gs, emit, 3)\n" },
    { 0xbf900332U, 0, false, "        s_sendmsg       sendmsg(gs, emit-cut, 3)\n" },
    { 0xbf91001bU, 0, false, "        s_sendmsghalt   sendmsg(11, cut, 0)\n" },
    { 0xbf92032bU, 0, false, "        s_trap          0x32b\n" },
    { 0xbf93032bU, 0, false, "        s_icache_inv    0x32b\n" },
    { 0xbf930000U, 0, false, "        s_icache_inv\n" },
    { 0xbf941234U, 0, false, "        s_incperflevel  0x1234\n" },
    { 0xbf951234U, 0, false, "        s_decperflevel  0x1234\n" },
    { 0xbf960000U, 0, false, "        s_ttracedata\n" },
    { 0xbf960dcaU, 0, false, "        s_ttracedata    0xdca\n" },
    { 0xbf970020U, 0, false, "        SOPP_ill_23     0x20\n" },
    { 0xbf980020U, 0, false, "        SOPP_ill_24     0x20\n" },
    { 0xbf990020U, 0, false, "        SOPP_ill_25     0x20\n" },
    { 0xbf9a0020U, 0, false, "        SOPP_ill_26     0x20\n" },
    { 0xbf9b0020U, 0, false, "        SOPP_ill_27     0x20\n" },
    { 0xbf9c0020U, 0, false, "        SOPP_ill_28     0x20\n" },
    /* SMRD encoding */
    { 0xc0193b5bU, 0, false, "        s_load_dword    s50, s[58:59], 0x5b\n" },
    { 0xc0193a5bU, 0, false, "        s_load_dword    s50, s[58:59], s91\n" },
    { 0xc019ba5bU, 0, false, "        s_load_dword    s51, s[58:59], s91\n" },
    { 0xc059ba5bU, 0, false, "        s_load_dwordx2  s[51:52], s[58:59], s91\n" },
    { 0xc099ba5bU, 0, false, "        s_load_dwordx4  s[51:54], s[58:59], s91\n" },
    { 0xc0d9ba5bU, 0, false, "        s_load_dwordx8  s[51:58], s[58:59], s91\n" },
    { 0xc119ba5bU, 0, false, "        s_load_dwordx16 s[51:66], s[58:59], s91\n" },
    { 0xc1593b5bU, 0, false, "        SMRD_ill_5      s50, s[58:59], 0x5b\n" },
    { 0xc1993b5bU, 0, false, "        SMRD_ill_6      s50, s[58:59], 0x5b\n" },
    { 0xc1d93b5bU, 0, false, "        SMRD_ill_7      s50, s[58:59], 0x5b\n" },
    { 0xc219ba5bU, 0, false, "        s_buffer_load_dword s51, s[58:61], s91\n" },
    { 0xc219bb5bU, 0, false, "        s_buffer_load_dword s51, s[58:61], 0x5b\n" },
    { 0xc259ba5bU, 0, false, "        s_buffer_load_dwordx2 s[51:52], s[58:61], s91\n" },
    { 0xc299ba5bU, 0, false, "        s_buffer_load_dwordx4 s[51:54], s[58:61], s91\n" },
    { 0xc2d9ba5bU, 0, false, "        s_buffer_load_dwordx8 s[51:58], s[58:61], s91\n" },
    { 0xc319ba5bU, 0, false, "        s_buffer_load_dwordx16 s[51:66], s[58:61], s91\n" },
    { 0xc359bb5bU, 0, false, "        SMRD_ill_13     s51, s[58:59], 0x5b\n" },
    { 0xc399bb5bU, 0, false, "        SMRD_ill_14     s51, s[58:59], 0x5b\n" },
    { 0xc3d9bb5bU, 0, false, "        SMRD_ill_15     s51, s[58:59], 0x5b\n" },
    { 0xc7998000U, 0, false, "        s_memtime       s[51:52]\n" },
    { 0xc7998044U, 0, false, "        s_memtime       s[51:52] offset=0x44\n" },
    { 0xc7998144U, 0, false, "        s_memtime       s[51:52] offset=0x44 imm=1\n" },
    { 0xc7c00000U, 0, false, "        s_dcache_inv\n" },
    { 0xc7c00100U, 0, false, "        s_dcache_inv    imm=1\n" },
    { 0xc7fff023U, 0, false, "        s_dcache_inv    sdst=0x7f sbase=0x38 offset=0x23\n" },
    { 0xc7fff000U, 0, false, "        s_dcache_inv    sdst=0x7f sbase=0x38\n" },
    { 0xc7c07000U, 0, false, "        s_dcache_inv    sbase=0x38\n" },
    /* VOP2 encoding */
    { 0x0134d715U, 0, false, "        v_cndmask_b32   v154, v21, v107, vcc\n" },
    { 0x0134d6ffU, 0x445aa, true , "        v_cndmask_b32   v154, 0x445aa, v107, vcc\n" },
    { 0x0234bd15U, 0, false, "        v_readlane_b32  s26, v21, s94\n" },
    { 0x0434bc15U, 0, false, "        v_writelane_b32 v26, s21, s94\n" },
    { 0x0434bd15U, 0, false, "        v_writelane_b32 v26, v21, s94\n" },
    { 0x0734d715U, 0, false, "        v_add_f32       v154, v21, v107\n" },
    { 0x0734d6ffU, 0x40000000U, true, "        v_add_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0934d715U, 0, false, "        v_sub_f32       v154, v21, v107\n" },
    { 0x0934d6ffU, 0x40000000U, true, "        v_sub_f32       v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0b34d715U, 0, false, "        v_subrev_f32    v154, v21, v107\n" },
    { 0x0b34d6ffU, 0x40000000U, true, "        v_subrev_f32    v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0d34d715U, 0, false, "        v_mac_legacy_f32 v154, v21, v107\n" },
    { 0x0d34d6ffU, 0x40000000U, true, "        v_mac_legacy_f32 v154, "
            "0x40000000 /* 2f */, v107\n" },
    { 0x0f34d715U, 0, false, "        v_mul_legacy_f32 v154, v21, v107\n" },
    { 0x0f34d6ffU, 0x40000000U, true, "        v_mul_legacy_f32 v154, "
        "0x40000000 /* 2f */, v107\n" },
    { 0x1134d715U, 0, false, "        v_mul_f32       v154, v21, v107\n" },
    { 0x1134d6ffU, 0x40000000U, true, "        v_mul_f32       v154, "
        "0x40000000 /* 2f */, v107\n" },
    { 0x1334d715U, 0, false, "        v_mul_i32_i24   v154, v21, v107\n" },
    { 0x1534d715U, 0, false, "        v_mul_hi_i32_i24 v154, v21, v107\n" },
    { 0x1734d715U, 0, false, "        v_mul_u32_u24   v154, v21, v107\n" },
    { 0x1934d715U, 0, false, "        v_mul_hi_u32_u24 v154, v21, v107\n" },
    { 0x1b34d715U, 0, false, "        v_min_legacy_f32 v154, v21, v107\n" },
    { 0x1d34d715U, 0, false, "        v_max_legacy_f32 v154, v21, v107\n" },
    { 0x1f34d715U, 0, false, "        v_min_f32       v154, v21, v107\n" },
    { 0x2134d715U, 0, false, "        v_max_f32       v154, v21, v107\n" },
    { 0x2334d715U, 0, false, "        v_min_i32       v154, v21, v107\n" },
    { 0x2534d715U, 0, false, "        v_max_i32       v154, v21, v107\n" },
    { 0x2734d715U, 0, false, "        v_min_u32       v154, v21, v107\n" },
    { 0x2934d715U, 0, false, "        v_max_u32       v154, v21, v107\n" },
    { 0x2b34d715U, 0, false, "        v_lshr_b32      v154, v21, v107\n" },
    { 0x2d34d715U, 0, false, "        v_lshrrev_b32   v154, v21, v107\n" },
    { 0x2f34d715U, 0, false, "        v_ashr_i32      v154, v21, v107\n" },
    { 0x3134d715U, 0, false, "        v_ashrrev_i32   v154, v21, v107\n" },
    { 0x3334d715U, 0, false, "        v_lshl_b32      v154, v21, v107\n" },
    { 0x3534d715U, 0, false, "        v_lshlrev_b32   v154, v21, v107\n" },
    { 0x3734d715U, 0, false, "        v_and_b32       v154, v21, v107\n" },
    { 0x3934d715U, 0, false, "        v_or_b32        v154, v21, v107\n" },
    { 0x3b34d715U, 0, false, "        v_xor_b32       v154, v21, v107\n" },
    { 0x3d34d715U, 0, false, "        v_bfm_b32       v154, v21, v107\n" },
    { 0x3f34d715U, 0, false, "        v_mac_f32       v154, v21, v107\n" },
    { 0x4134d715U, 0x567d0700U, true, "        v_madmk_f32     "
            "v154, v21, 0x567d0700 /* 6.9551627e+13f */, v107\n" }, /* check floatLits */
    { 0x4134d6ffU, 0x567d0700U, true, "        v_madmk_f32     "
            "v154, 0x567d0700 /* 6.9551627e+13f */, "
            "0x567d0700 /* 6.9551627e+13f */, v107\n" }, /* check floatLits */
    { 0x4334d715U, 0x567d0700U, true, "        v_madak_f32     "
            "v154, v21, v107, 0x567d0700 /* 6.9551627e+13f */\n" },  /* check floatLits */
    { 0x4334d6ffU, 0x567d0700U, true, "        v_madak_f32     "
            "v154, 0x567d0700 /* 6.9551627e+13f */, "
            "v107, 0x567d0700 /* 6.9551627e+13f */\n" },  /* check floatLits */
    { 0x4534d715U, 0, false, "        v_bcnt_u32_b32  v154, v21, v107\n" },
    { 0x4734d715U, 0, false, "        v_mbcnt_lo_u32_b32 v154, v21, v107\n" },
    { 0x4934d715U, 0, false, "        v_mbcnt_hi_u32_b32 v154, v21, v107\n" },
    { 0x4b34d715U, 0, false, "        v_add_i32       v154, vcc, v21, v107\n" },
    { 0x4d34d715U, 0, false, "        v_sub_i32       v154, vcc, v21, v107\n" },
    { 0x4f34d715U, 0, false, "        v_subrev_i32    v154, vcc, v21, v107\n" },
    { 0x5134d715U, 0, false, "        v_addc_u32      v154, vcc, v21, v107, vcc\n" },
    { 0x5334d715U, 0, false, "        v_subb_u32      v154, vcc, v21, v107, vcc\n" },
    { 0x5534d715U, 0, false, "        v_subbrev_u32   v154, vcc, v21, v107, vcc\n" },
    { 0x5734d715U, 0, false, "        v_ldexp_f32     v154, v21, v107\n" },
    { 0x5934d715U, 0, false, "        v_cvt_pkaccum_u8_f32 v154, v21, v107\n" },
    { 0x5b34d715U, 0, false, "        v_cvt_pknorm_i16_f32 v154, v21, v107\n" },
    { 0x5d34d715U, 0, false, "        v_cvt_pknorm_u16_f32 v154, v21, v107\n" },
    { 0x5f34d715U, 0, false, "        v_cvt_pkrtz_f16_f32 v154, v21, v107\n" },
    { 0x6134d715U, 0, false, "        v_cvt_pk_u16_u32 v154, v21, v107\n" },
    { 0x6334d715U, 0, false, "        v_cvt_pk_i16_i32 v154, v21, v107\n" },
    { 0x6534d715U, 0, false, "        VOP2_ill_50     v154, v21, v107\n" },
    { 0x6734d715U, 0, false, "        VOP2_ill_51     v154, v21, v107\n" },
    { 0x6934d715U, 0, false, "        VOP2_ill_52     v154, v21, v107\n" },
    { 0x6b34d715U, 0, false, "        VOP2_ill_53     v154, v21, v107\n" },
    { 0x6d34d715U, 0, false, "        VOP2_ill_54     v154, v21, v107\n" },
    /* VOP1 encoding */
    { 0x7f3c004fU, 0, false, "        v_nop           vdst=0x9e vsrc=0x4f\n" },
    { 0x7f3c0000U, 0, false, "        v_nop           vdst=0x9e\n" },
    { 0x7e00014fU, 0, false, "        v_nop           vsrc=0x14f\n" },
    { 0x7e000000U, 0, false, "        v_nop\n" },
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
    { 0x7f3c16ffU, 0x40000000U, true, "        v_cvt_f32_f16   v158, "
                "0x40000000\n" },
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
    { 0x7f3c2f4fU, 0, false, "        VOP1_ill_23     v158, v79\n" },
    { 0x7f3c314fU, 0, false, "        VOP1_ill_24     v158, v79\n" },
    { 0x7f3c334fU, 0, false, "        VOP1_ill_25     v158, v79\n" },
    { 0x7f3c354fU, 0, false, "        VOP1_ill_26     v158, v79\n" },
    { 0x7f3c374fU, 0, false, "        VOP1_ill_27     v158, v79\n" },
    { 0x7f3c394fU, 0, false, "        VOP1_ill_28     v158, v79\n" },
    { 0x7f3c3b4fU, 0, false, "        VOP1_ill_29     v158, v79\n" },
    { 0x7f3c3d4fU, 0, false, "        VOP1_ill_30     v158, v79\n" },
    { 0x7f3c3f4fU, 0, false, "        VOP1_ill_31     v158, v79\n" },
    { 0x7f3c414fU, 0, false, "        v_fract_f32     v158, v79\n" },
    { 0x7f3c40ffU, 0x40000000U, true, "        v_fract_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c434fU, 0, false, "        v_trunc_f32     v158, v79\n" },
    { 0x7f3c42ffU, 0x40000000U, true, "        v_trunc_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c454fU, 0, false, "        v_ceil_f32      v158, v79\n" },
    { 0x7f3c44ffU, 0x40000000U, true, "        v_ceil_f32      v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c474fU, 0, false, "        v_rndne_f32     v158, v79\n" },
    { 0x7f3c46ffU, 0x40000000U, true, "        v_rndne_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c494fU, 0, false, "        v_floor_f32     v158, v79\n" },
    { 0x7f3c48ffU, 0x40000000U, true, "        v_floor_f32     v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c4b4fU, 0, false, "        v_exp_f32       v158, v79\n" },
    { 0x7f3c4affU, 0x40000000U, true, "        v_exp_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c4d4fU, 0, false, "        v_log_clamp_f32 v158, v79\n" },
    { 0x7f3c4cffU, 0x40000000U, true, "        v_log_clamp_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c4f4fU, 0, false, "        v_log_f32       v158, v79\n" },
    { 0x7f3c4effU, 0x40000000U, true, "        v_log_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c514fU, 0, false, "        v_rcp_clamp_f32 v158, v79\n" },
    { 0x7f3c50ffU, 0x40000000U, true, "        v_rcp_clamp_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c534fU, 0, false, "        v_rcp_legacy_f32 v158, v79\n" },
    { 0x7f3c52ffU, 0x40000000U, true, "        v_rcp_legacy_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c554fU, 0, false, "        v_rcp_f32       v158, v79\n" },
    { 0x7f3c54ffU, 0x40000000U, true, "        v_rcp_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c574fU, 0, false, "        v_rcp_iflag_f32 v158, v79\n" },
    { 0x7f3c56ffU, 0x40000000U, true, "        v_rcp_iflag_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c594fU, 0, false, "        v_rsq_clamp_f32 v158, v79\n" },
    { 0x7f3c58ffU, 0x40000000U, true, "        v_rsq_clamp_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c5b4fU, 0, false, "        v_rsq_legacy_f32 v158, v79\n" },
    { 0x7f3c5affU, 0x40000000U, true, "        v_rsq_legacy_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c5d4fU, 0, false, "        v_rsq_f32       v158, v79\n" },
    { 0x7f3c5cffU, 0x40000000U, true, "        v_rsq_f32       v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c5f4fU, 0, false, "        v_rcp_f64       v[158:159], v[79:80]\n" },
    { 0x7f3c5effU, 0x40000000U, true, "        v_rcp_f64       v[158:159], "
                "0x40000000\n" },
    { 0x7f3c614fU, 0, false, "        v_rcp_clamp_f64 v[158:159], v[79:80]\n" },
    { 0x7f3c634fU, 0, false, "        v_rsq_f64       v[158:159], v[79:80]\n" },
    { 0x7f3c654fU, 0, false, "        v_rsq_clamp_f64 v[158:159], v[79:80]\n" },
    { 0x7f3c674fU, 0, false, "        v_sqrt_f32      v158, v79\n" },
    { 0x7f3c694fU, 0, false, "        v_sqrt_f64      v[158:159], v[79:80]\n" },
    { 0x7f3c6b4fU, 0, false, "        v_sin_f32       v158, v79\n" },
    { 0x7f3c6d4fU, 0, false, "        v_cos_f32       v158, v79\n" },
    { 0x7f3c6f4fU, 0, false, "        v_not_b32       v158, v79\n" },
    { 0x7f3c714fU, 0, false, "        v_bfrev_b32     v158, v79\n" },
    { 0x7f3c734fU, 0, false, "        v_ffbh_u32      v158, v79\n" },
    { 0x7f3c754fU, 0, false, "        v_ffbl_b32      v158, v79\n" },
    { 0x7f3c774fU, 0, false, "        v_ffbh_i32      v158, v79\n" },
    { 0x7f3c794fU, 0, false, "        v_frexp_exp_i32_f64 v158, v[79:80]\n" },
    { 0x7f3c7b4fU, 0, false, "        v_frexp_mant_f64 v[158:159], v[79:80]\n" },
    { 0x7f3c7d4fU, 0, false, "        v_fract_f64     v[158:159], v[79:80]\n" },
    { 0x7f3c7f4fU, 0, false, "        v_frexp_exp_i32_f32 v158, v79\n" },
    { 0x7f3c7effU, 0x40000000U, true, "        v_frexp_exp_i32_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c814fU, 0, false, "        v_frexp_mant_f32 v158, v79\n" },
    { 0x7f3c80ffU, 0x40000000U, true, "        v_frexp_mant_f32 v158, "
                "0x40000000 /* 2f */\n" },
    { 0x7f3c834fU, 0, false, "        v_clrexcp       vdst=0x9e vsrc=0x14f\n" },
    { 0x7e008200U, 0, false, "        v_clrexcp\n" },
    { 0x7f3c854fU, 0, false, "        v_movreld_b32   v158, v79\n" },
    { 0x7f3c874fU, 0, false, "        v_movrels_b32   v158, v79\n" },
    { 0x7f3c894fU, 0, false, "        v_movrelsd_b32  v158, v79\n" },
    { 0x7f3c8b4fU, 0, false, "        VOP1_ill_69     v158, v79\n" },
    { 0x7f3c8d4fU, 0, false, "        VOP1_ill_70     v158, v79\n" },
    { 0x7f3c8f4fU, 0, false, "        VOP1_ill_71     v158, v79\n" },
    { 0x7f3c914fU, 0, false, "        VOP1_ill_72     v158, v79\n" },
    { 0x7f3c934fU, 0, false, "        VOP1_ill_73     v158, v79\n" },
    /* VOPC encoding */
    { 0x7c01934fU, 0, false, "        v_cmp_f_f32     vcc, v79, v201\n" },
    { 0x7c0192ffU, 0x40000000U, true, "        v_cmp_f_f32     "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c03934fU, 0, false, "        v_cmp_lt_f32    vcc, v79, v201\n" },
    { 0x7c0392ffU, 0x40000000U, true, "        v_cmp_lt_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c05934fU, 0, false, "        v_cmp_eq_f32    vcc, v79, v201\n" },
    { 0x7c0592ffU, 0x40000000U, true, "        v_cmp_eq_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c07934fU, 0, false, "        v_cmp_le_f32    vcc, v79, v201\n" },
    { 0x7c0792ffU, 0x40000000U, true, "        v_cmp_le_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c09934fU, 0, false, "        v_cmp_gt_f32    vcc, v79, v201\n" },
    { 0x7c0992ffU, 0x40000000U, true, "        v_cmp_gt_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c0b934fU, 0, false, "        v_cmp_lg_f32    vcc, v79, v201\n" },
    { 0x7c0b92ffU, 0x40000000U, true, "        v_cmp_lg_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c0d934fU, 0, false, "        v_cmp_ge_f32    vcc, v79, v201\n" },
    { 0x7c0d92ffU, 0x40000000U, true, "        v_cmp_ge_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c0f934fU, 0, false, "        v_cmp_o_f32     vcc, v79, v201\n" },
    { 0x7c0f92ffU, 0x40000000U, true, "        v_cmp_o_f32     "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c11934fU, 0, false, "        v_cmp_u_f32     vcc, v79, v201\n" },
    { 0x7c1192ffU, 0x40000000U, true, "        v_cmp_u_f32     "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c13934fU, 0, false, "        v_cmp_nge_f32   vcc, v79, v201\n" },
    { 0x7c1392ffU, 0x40000000U, true, "        v_cmp_nge_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c15934fU, 0, false, "        v_cmp_nlg_f32   vcc, v79, v201\n" },
    { 0x7c1592ffU, 0x40000000U, true, "        v_cmp_nlg_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c17934fU, 0, false, "        v_cmp_ngt_f32   vcc, v79, v201\n" },
    { 0x7c1792ffU, 0x40000000U, true, "        v_cmp_ngt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c19934fU, 0, false, "        v_cmp_nle_f32   vcc, v79, v201\n" },
    { 0x7c1992ffU, 0x40000000U, true, "        v_cmp_nle_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c1b934fU, 0, false, "        v_cmp_neq_f32   vcc, v79, v201\n" },
    { 0x7c1b92ffU, 0x40000000U, true, "        v_cmp_neq_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c1d934fU, 0, false, "        v_cmp_nlt_f32   vcc, v79, v201\n" },
    { 0x7c1d92ffU, 0x40000000U, true, "        v_cmp_nlt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c1f934fU, 0, false, "        v_cmp_tru_f32   vcc, v79, v201\n" },
    { 0x7c1f92ffU, 0x40000000U, true, "        v_cmp_tru_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c21934fU, 0, false, "        v_cmpx_f_f32    vcc, v79, v201\n" },
    { 0x7c2192ffU, 0x40000000U, true, "        v_cmpx_f_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c23934fU, 0, false, "        v_cmpx_lt_f32   vcc, v79, v201\n" },
    { 0x7c2392ffU, 0x40000000U, true, "        v_cmpx_lt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c25934fU, 0, false, "        v_cmpx_eq_f32   vcc, v79, v201\n" },
    { 0x7c2592ffU, 0x40000000U, true, "        v_cmpx_eq_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c27934fU, 0, false, "        v_cmpx_le_f32   vcc, v79, v201\n" },
    { 0x7c2792ffU, 0x40000000U, true, "        v_cmpx_le_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c29934fU, 0, false, "        v_cmpx_gt_f32   vcc, v79, v201\n" },
    { 0x7c2992ffU, 0x40000000U, true, "        v_cmpx_gt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c2b934fU, 0, false, "        v_cmpx_lg_f32   vcc, v79, v201\n" },
    { 0x7c2b92ffU, 0x40000000U, true, "        v_cmpx_lg_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c2d934fU, 0, false, "        v_cmpx_ge_f32   vcc, v79, v201\n" },
    { 0x7c2d92ffU, 0x40000000U, true, "        v_cmpx_ge_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c2f934fU, 0, false, "        v_cmpx_o_f32    vcc, v79, v201\n" },
    { 0x7c2f92ffU, 0x40000000U, true, "        v_cmpx_o_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c31934fU, 0, false, "        v_cmpx_u_f32    vcc, v79, v201\n" },
    { 0x7c3192ffU, 0x40000000U, true, "        v_cmpx_u_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c33934fU, 0, false, "        v_cmpx_nge_f32  vcc, v79, v201\n" },
    { 0x7c3392ffU, 0x40000000U, true, "        v_cmpx_nge_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c35934fU, 0, false, "        v_cmpx_nlg_f32  vcc, v79, v201\n" },
    { 0x7c3592ffU, 0x40000000U, true, "        v_cmpx_nlg_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c37934fU, 0, false, "        v_cmpx_ngt_f32  vcc, v79, v201\n" },
    { 0x7c3792ffU, 0x40000000U, true, "        v_cmpx_ngt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c39934fU, 0, false, "        v_cmpx_nle_f32  vcc, v79, v201\n" },
    { 0x7c3992ffU, 0x40000000U, true, "        v_cmpx_nle_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c3b934fU, 0, false, "        v_cmpx_neq_f32  vcc, v79, v201\n" },
    { 0x7c3b92ffU, 0x40000000U, true, "        v_cmpx_neq_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c3d934fU, 0, false, "        v_cmpx_nlt_f32  vcc, v79, v201\n" },
    { 0x7c3d92ffU, 0x40000000U, true, "        v_cmpx_nlt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c3f934fU, 0, false, "        v_cmpx_tru_f32  vcc, v79, v201\n" },
    { 0x7c3f92ffU, 0x40000000U, true, "        v_cmpx_tru_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    
    { 0x7c41934fU, 0, false, "        v_cmp_f_f64     vcc, v[79:80], v[201:202]\n" },
    { 0x7c4192ffU, 0x40000000U, true, "        v_cmp_f_f64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c43934fU, 0, false, "        v_cmp_lt_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c4392ffU, 0x40000000U, true, "        v_cmp_lt_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c45934fU, 0, false, "        v_cmp_eq_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c4592ffU, 0x40000000U, true, "        v_cmp_eq_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c47934fU, 0, false, "        v_cmp_le_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c4792ffU, 0x40000000U, true, "        v_cmp_le_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c49934fU, 0, false, "        v_cmp_gt_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c4992ffU, 0x40000000U, true, "        v_cmp_gt_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c4b934fU, 0, false, "        v_cmp_lg_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c4b92ffU, 0x40000000U, true, "        v_cmp_lg_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c4d934fU, 0, false, "        v_cmp_ge_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c4d92ffU, 0x40000000U, true, "        v_cmp_ge_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c4f934fU, 0, false, "        v_cmp_o_f64     vcc, v[79:80], v[201:202]\n" },
    { 0x7c4f92ffU, 0x40000000U, true, "        v_cmp_o_f64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c51934fU, 0, false, "        v_cmp_u_f64     vcc, v[79:80], v[201:202]\n" },
    { 0x7c5192ffU, 0x40000000U, true, "        v_cmp_u_f64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c53934fU, 0, false, "        v_cmp_nge_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c5392ffU, 0x40000000U, true, "        v_cmp_nge_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c55934fU, 0, false, "        v_cmp_nlg_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c5592ffU, 0x40000000U, true, "        v_cmp_nlg_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c57934fU, 0, false, "        v_cmp_ngt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c5792ffU, 0x40000000U, true, "        v_cmp_ngt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c59934fU, 0, false, "        v_cmp_nle_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c5992ffU, 0x40000000U, true, "        v_cmp_nle_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c5b934fU, 0, false, "        v_cmp_neq_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c5b92ffU, 0x40000000U, true, "        v_cmp_neq_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c5d934fU, 0, false, "        v_cmp_nlt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c5d92ffU, 0x40000000U, true, "        v_cmp_nlt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c5f934fU, 0, false, "        v_cmp_tru_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c5f92ffU, 0x40000000U, true, "        v_cmp_tru_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c61934fU, 0, false, "        v_cmpx_f_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c6192ffU, 0x40000000U, true, "        v_cmpx_f_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c63934fU, 0, false, "        v_cmpx_lt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c6392ffU, 0x40000000U, true, "        v_cmpx_lt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c65934fU, 0, false, "        v_cmpx_eq_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c6592ffU, 0x40000000U, true, "        v_cmpx_eq_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c67934fU, 0, false, "        v_cmpx_le_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c6792ffU, 0x40000000U, true, "        v_cmpx_le_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c69934fU, 0, false, "        v_cmpx_gt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c6992ffU, 0x40000000U, true, "        v_cmpx_gt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c6b934fU, 0, false, "        v_cmpx_lg_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c6b92ffU, 0x40000000U, true, "        v_cmpx_lg_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c6d934fU, 0, false, "        v_cmpx_ge_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7c6d92ffU, 0x40000000U, true, "        v_cmpx_ge_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c6f934fU, 0, false, "        v_cmpx_o_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c6f92ffU, 0x40000000U, true, "        v_cmpx_o_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c71934fU, 0, false, "        v_cmpx_u_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7c7192ffU, 0x40000000U, true, "        v_cmpx_u_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c73934fU, 0, false, "        v_cmpx_nge_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7c7392ffU, 0x40000000U, true, "        v_cmpx_nge_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c75934fU, 0, false, "        v_cmpx_nlg_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7c7592ffU, 0x40000000U, true, "        v_cmpx_nlg_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c77934fU, 0, false, "        v_cmpx_ngt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7c7792ffU, 0x40000000U, true, "        v_cmpx_ngt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c79934fU, 0, false, "        v_cmpx_nle_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7c7992ffU, 0x40000000U, true, "        v_cmpx_nle_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c7b934fU, 0, false, "        v_cmpx_neq_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7c7b92ffU, 0x40000000U, true, "        v_cmpx_neq_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c7d934fU, 0, false, "        v_cmpx_nlt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7c7d92ffU, 0x40000000U, true, "        v_cmpx_nlt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7c7f934fU, 0, false, "        v_cmpx_tru_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7c7f92ffU, 0x40000000U, true, "        v_cmpx_tru_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    
    /**** v_cmpsx_* ****/
    { 0x7c81934fU, 0, false, "        v_cmps_f_f32    vcc, v79, v201\n" },
    { 0x7c8192ffU, 0x40000000U, true, "        v_cmps_f_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c83934fU, 0, false, "        v_cmps_lt_f32   vcc, v79, v201\n" },
    { 0x7c8392ffU, 0x40000000U, true, "        v_cmps_lt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c85934fU, 0, false, "        v_cmps_eq_f32   vcc, v79, v201\n" },
    { 0x7c8592ffU, 0x40000000U, true, "        v_cmps_eq_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c87934fU, 0, false, "        v_cmps_le_f32   vcc, v79, v201\n" },
    { 0x7c8792ffU, 0x40000000U, true, "        v_cmps_le_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c89934fU, 0, false, "        v_cmps_gt_f32   vcc, v79, v201\n" },
    { 0x7c8992ffU, 0x40000000U, true, "        v_cmps_gt_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c8b934fU, 0, false, "        v_cmps_lg_f32   vcc, v79, v201\n" },
    { 0x7c8b92ffU, 0x40000000U, true, "        v_cmps_lg_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c8d934fU, 0, false, "        v_cmps_ge_f32   vcc, v79, v201\n" },
    { 0x7c8d92ffU, 0x40000000U, true, "        v_cmps_ge_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c8f934fU, 0, false, "        v_cmps_o_f32    vcc, v79, v201\n" },
    { 0x7c8f92ffU, 0x40000000U, true, "        v_cmps_o_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c91934fU, 0, false, "        v_cmps_u_f32    vcc, v79, v201\n" },
    { 0x7c9192ffU, 0x40000000U, true, "        v_cmps_u_f32    "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c93934fU, 0, false, "        v_cmps_nge_f32  vcc, v79, v201\n" },
    { 0x7c9392ffU, 0x40000000U, true, "        v_cmps_nge_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c95934fU, 0, false, "        v_cmps_nlg_f32  vcc, v79, v201\n" },
    { 0x7c9592ffU, 0x40000000U, true, "        v_cmps_nlg_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c97934fU, 0, false, "        v_cmps_ngt_f32  vcc, v79, v201\n" },
    { 0x7c9792ffU, 0x40000000U, true, "        v_cmps_ngt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c99934fU, 0, false, "        v_cmps_nle_f32  vcc, v79, v201\n" },
    { 0x7c9992ffU, 0x40000000U, true, "        v_cmps_nle_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c9b934fU, 0, false, "        v_cmps_neq_f32  vcc, v79, v201\n" },
    { 0x7c9b92ffU, 0x40000000U, true, "        v_cmps_neq_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c9d934fU, 0, false, "        v_cmps_nlt_f32  vcc, v79, v201\n" },
    { 0x7c9d92ffU, 0x40000000U, true, "        v_cmps_nlt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7c9f934fU, 0, false, "        v_cmps_tru_f32  vcc, v79, v201\n" },
    { 0x7c9f92ffU, 0x40000000U, true, "        v_cmps_tru_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca1934fU, 0, false, "        v_cmpsx_f_f32   vcc, v79, v201\n" },
    { 0x7ca192ffU, 0x40000000U, true, "        v_cmpsx_f_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca3934fU, 0, false, "        v_cmpsx_lt_f32  vcc, v79, v201\n" },
    { 0x7ca392ffU, 0x40000000U, true, "        v_cmpsx_lt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca5934fU, 0, false, "        v_cmpsx_eq_f32  vcc, v79, v201\n" },
    { 0x7ca592ffU, 0x40000000U, true, "        v_cmpsx_eq_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca7934fU, 0, false, "        v_cmpsx_le_f32  vcc, v79, v201\n" },
    { 0x7ca792ffU, 0x40000000U, true, "        v_cmpsx_le_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7ca9934fU, 0, false, "        v_cmpsx_gt_f32  vcc, v79, v201\n" },
    { 0x7ca992ffU, 0x40000000U, true, "        v_cmpsx_gt_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cab934fU, 0, false, "        v_cmpsx_lg_f32  vcc, v79, v201\n" },
    { 0x7cab92ffU, 0x40000000U, true, "        v_cmpsx_lg_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cad934fU, 0, false, "        v_cmpsx_ge_f32  vcc, v79, v201\n" },
    { 0x7cad92ffU, 0x40000000U, true, "        v_cmpsx_ge_f32  "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7caf934fU, 0, false, "        v_cmpsx_o_f32   vcc, v79, v201\n" },
    { 0x7caf92ffU, 0x40000000U, true, "        v_cmpsx_o_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb1934fU, 0, false, "        v_cmpsx_u_f32   vcc, v79, v201\n" },
    { 0x7cb192ffU, 0x40000000U, true, "        v_cmpsx_u_f32   "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb3934fU, 0, false, "        v_cmpsx_nge_f32 vcc, v79, v201\n" },
    { 0x7cb392ffU, 0x40000000U, true, "        v_cmpsx_nge_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb5934fU, 0, false, "        v_cmpsx_nlg_f32 vcc, v79, v201\n" },
    { 0x7cb592ffU, 0x40000000U, true, "        v_cmpsx_nlg_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb7934fU, 0, false, "        v_cmpsx_ngt_f32 vcc, v79, v201\n" },
    { 0x7cb792ffU, 0x40000000U, true, "        v_cmpsx_ngt_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cb9934fU, 0, false, "        v_cmpsx_nle_f32 vcc, v79, v201\n" },
    { 0x7cb992ffU, 0x40000000U, true, "        v_cmpsx_nle_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cbb934fU, 0, false, "        v_cmpsx_neq_f32 vcc, v79, v201\n" },
    { 0x7cbb92ffU, 0x40000000U, true, "        v_cmpsx_neq_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cbd934fU, 0, false, "        v_cmpsx_nlt_f32 vcc, v79, v201\n" },
    { 0x7cbd92ffU, 0x40000000U, true, "        v_cmpsx_nlt_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7cbf934fU, 0, false, "        v_cmpsx_tru_f32 vcc, v79, v201\n" },
    { 0x7cbf92ffU, 0x40000000U, true, "        v_cmpsx_tru_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    
    { 0x7cc1934fU, 0, false, "        v_cmps_f_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cc192ffU, 0x40000000U, true, "        v_cmps_f_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc3934fU, 0, false, "        v_cmps_lt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cc392ffU, 0x40000000U, true, "        v_cmps_lt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc5934fU, 0, false, "        v_cmps_eq_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cc592ffU, 0x40000000U, true, "        v_cmps_eq_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc7934fU, 0, false, "        v_cmps_le_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cc792ffU, 0x40000000U, true, "        v_cmps_le_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cc9934fU, 0, false, "        v_cmps_gt_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cc992ffU, 0x40000000U, true, "        v_cmps_gt_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ccb934fU, 0, false, "        v_cmps_lg_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ccb92ffU, 0x40000000U, true, "        v_cmps_lg_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ccd934fU, 0, false, "        v_cmps_ge_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ccd92ffU, 0x40000000U, true, "        v_cmps_ge_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ccf934fU, 0, false, "        v_cmps_o_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7ccf92ffU, 0x40000000U, true, "        v_cmps_o_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd1934fU, 0, false, "        v_cmps_u_f64    vcc, v[79:80], v[201:202]\n" },
    { 0x7cd192ffU, 0x40000000U, true, "        v_cmps_u_f64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd3934fU, 0, false, "        v_cmps_nge_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cd392ffU, 0x40000000U, true, "        v_cmps_nge_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd5934fU, 0, false, "        v_cmps_nlg_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cd592ffU, 0x40000000U, true, "        v_cmps_nlg_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd7934fU, 0, false, "        v_cmps_ngt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cd792ffU, 0x40000000U, true, "        v_cmps_ngt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cd9934fU, 0, false, "        v_cmps_nle_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cd992ffU, 0x40000000U, true, "        v_cmps_nle_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cdb934fU, 0, false, "        v_cmps_neq_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cdb92ffU, 0x40000000U, true, "        v_cmps_neq_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cdd934fU, 0, false, "        v_cmps_nlt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cdd92ffU, 0x40000000U, true, "        v_cmps_nlt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cdf934fU, 0, false, "        v_cmps_tru_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7cdf92ffU, 0x40000000U, true, "        v_cmps_tru_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce1934fU, 0, false, "        v_cmpsx_f_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ce192ffU, 0x40000000U, true, "        v_cmpsx_f_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce3934fU, 0, false, "        v_cmpsx_lt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7ce392ffU, 0x40000000U, true, "        v_cmpsx_lt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce5934fU, 0, false, "        v_cmpsx_eq_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7ce592ffU, 0x40000000U, true, "        v_cmpsx_eq_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce7934fU, 0, false, "        v_cmpsx_le_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7ce792ffU, 0x40000000U, true, "        v_cmpsx_le_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ce9934fU, 0, false, "        v_cmpsx_gt_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7ce992ffU, 0x40000000U, true, "        v_cmpsx_gt_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ceb934fU, 0, false, "        v_cmpsx_lg_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7ceb92ffU, 0x40000000U, true, "        v_cmpsx_lg_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ced934fU, 0, false, "        v_cmpsx_ge_f64  vcc, v[79:80], v[201:202]\n" },
    { 0x7ced92ffU, 0x40000000U, true, "        v_cmpsx_ge_f64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cef934fU, 0, false, "        v_cmpsx_o_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cef92ffU, 0x40000000U, true, "        v_cmpsx_o_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf1934fU, 0, false, "        v_cmpsx_u_f64   vcc, v[79:80], v[201:202]\n" },
    { 0x7cf192ffU, 0x40000000U, true, "        v_cmpsx_u_f64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf3934fU, 0, false, "        v_cmpsx_nge_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7cf392ffU, 0x40000000U, true, "        v_cmpsx_nge_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf5934fU, 0, false, "        v_cmpsx_nlg_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7cf592ffU, 0x40000000U, true, "        v_cmpsx_nlg_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf7934fU, 0, false, "        v_cmpsx_ngt_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7cf792ffU, 0x40000000U, true, "        v_cmpsx_ngt_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cf9934fU, 0, false, "        v_cmpsx_nle_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7cf992ffU, 0x40000000U, true, "        v_cmpsx_nle_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cfb934fU, 0, false, "        v_cmpsx_neq_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7cfb92ffU, 0x40000000U, true, "        v_cmpsx_neq_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cfd934fU, 0, false, "        v_cmpsx_nlt_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7cfd92ffU, 0x40000000U, true, "        v_cmpsx_nlt_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7cff934fU, 0, false, "        v_cmpsx_tru_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7cff92ffU, 0x40000000U, true, "        v_cmpsx_tru_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
                
    /*** integer comparisons and classes ***/
    { 0x7d01934fU, 0, false, "        v_cmp_f_i32     vcc, v79, v201\n" },
    { 0x7d0192ffU, 0x40000000U, true, "        v_cmp_f_i32     vcc, 0x40000000, v201\n" },
    { 0x7d03934fU, 0, false, "        v_cmp_lt_i32    vcc, v79, v201\n" },
    { 0x7d0392ffU, 0x40000000U, true, "        v_cmp_lt_i32    vcc, 0x40000000, v201\n" },
    { 0x7d05934fU, 0, false, "        v_cmp_eq_i32    vcc, v79, v201\n" },
    { 0x7d0592ffU, 0x40000000U, true, "        v_cmp_eq_i32    vcc, 0x40000000, v201\n" },
    { 0x7d07934fU, 0, false, "        v_cmp_le_i32    vcc, v79, v201\n" },
    { 0x7d0792ffU, 0x40000000U, true, "        v_cmp_le_i32    vcc, 0x40000000, v201\n" },
    { 0x7d09934fU, 0, false, "        v_cmp_gt_i32    vcc, v79, v201\n" },
    { 0x7d0992ffU, 0x40000000U, true, "        v_cmp_gt_i32    vcc, 0x40000000, v201\n" },
    { 0x7d0b934fU, 0, false, "        v_cmp_lg_i32    vcc, v79, v201\n" },
    { 0x7d0b92ffU, 0x40000000U, true, "        v_cmp_lg_i32    vcc, 0x40000000, v201\n" },
    { 0x7d0d934fU, 0, false, "        v_cmp_ge_i32    vcc, v79, v201\n" },
    { 0x7d0d92ffU, 0x40000000U, true, "        v_cmp_ge_i32    vcc, 0x40000000, v201\n" },
    { 0x7d0f934fU, 0, false, "        v_cmp_tru_i32   vcc, v79, v201\n" },
    { 0x7d0f92ffU, 0x40000000U, true, "        v_cmp_tru_i32   vcc, 0x40000000, v201\n" },
    { 0x7d11934fU, 0, false, "        v_cmp_class_f32 vcc, v79, v201\n" },
    { 0x7d1192ffU, 0x40000000U, true, "        v_cmp_class_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7d13934fU, 0, false, "        VOPC_ill_137    vcc, v79, v201\n" },
    { 0x7d15934fU, 0, false, "        VOPC_ill_138    vcc, v79, v201\n" },
    { 0x7d17934fU, 0, false, "        VOPC_ill_139    vcc, v79, v201\n" },
    { 0x7d19934fU, 0, false, "        VOPC_ill_140    vcc, v79, v201\n" },
    { 0x7d1b934fU, 0, false, "        VOPC_ill_141    vcc, v79, v201\n" },
    { 0x7d1d934fU, 0, false, "        VOPC_ill_142    vcc, v79, v201\n" },
    { 0x7d1f934fU, 0, false, "        VOPC_ill_143    vcc, v79, v201\n" },
    { 0x7d21934fU, 0, false, "        v_cmpx_f_i32    vcc, v79, v201\n" },
    { 0x7d2192ffU, 0x40000000U, true, "        v_cmpx_f_i32    vcc, 0x40000000, v201\n" },
    { 0x7d23934fU, 0, false, "        v_cmpx_lt_i32   vcc, v79, v201\n" },
    { 0x7d2392ffU, 0x40000000U, true, "        v_cmpx_lt_i32   vcc, 0x40000000, v201\n" },
    { 0x7d25934fU, 0, false, "        v_cmpx_eq_i32   vcc, v79, v201\n" },
    { 0x7d2592ffU, 0x40000000U, true, "        v_cmpx_eq_i32   vcc, 0x40000000, v201\n" },
    { 0x7d27934fU, 0, false, "        v_cmpx_le_i32   vcc, v79, v201\n" },
    { 0x7d2792ffU, 0x40000000U, true, "        v_cmpx_le_i32   vcc, 0x40000000, v201\n" },
    { 0x7d29934fU, 0, false, "        v_cmpx_gt_i32   vcc, v79, v201\n" },
    { 0x7d2992ffU, 0x40000000U, true, "        v_cmpx_gt_i32   vcc, 0x40000000, v201\n" },
    { 0x7d2b934fU, 0, false, "        v_cmpx_lg_i32   vcc, v79, v201\n" },
    { 0x7d2b92ffU, 0x40000000U, true, "        v_cmpx_lg_i32   vcc, 0x40000000, v201\n" },
    { 0x7d2d934fU, 0, false, "        v_cmpx_ge_i32   vcc, v79, v201\n" },
    { 0x7d2d92ffU, 0x40000000U, true, "        v_cmpx_ge_i32   vcc, 0x40000000, v201\n" },
    { 0x7d2f934fU, 0, false, "        v_cmpx_tru_i32  vcc, v79, v201\n" },
    { 0x7d2f92ffU, 0x40000000U, true, "        v_cmpx_tru_i32  vcc, 0x40000000, v201\n" },
    { 0x7d31934fU, 0, false, "        v_cmpx_class_f32 vcc, v79, v201\n" },
    { 0x7d3192ffU, 0x40000000U, true, "        v_cmpx_class_f32 "
                "vcc, 0x40000000 /* 2f */, v201\n" },
    { 0x7d33934fU, 0, false, "        VOPC_ill_153    vcc, v79, v201\n" },
    { 0x7d35934fU, 0, false, "        VOPC_ill_154    vcc, v79, v201\n" },
    { 0x7d37934fU, 0, false, "        VOPC_ill_155    vcc, v79, v201\n" },
    { 0x7d39934fU, 0, false, "        VOPC_ill_156    vcc, v79, v201\n" },
    { 0x7d3b934fU, 0, false, "        VOPC_ill_157    vcc, v79, v201\n" },
    { 0x7d3d934fU, 0, false, "        VOPC_ill_158    vcc, v79, v201\n" },
    { 0x7d3f934fU, 0, false, "        VOPC_ill_159    vcc, v79, v201\n" },
    
    { 0x7d41934fU, 0, false, "        v_cmp_f_i64     vcc, v[79:80], v[201:202]\n" },
    { 0x7d4192ffU, 0x40000000U, true, "        v_cmp_f_i64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d43934fU, 0, false, "        v_cmp_lt_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7d4392ffU, 0x40000000U, true, "        v_cmp_lt_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d45934fU, 0, false, "        v_cmp_eq_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7d4592ffU, 0x40000000U, true, "        v_cmp_eq_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d47934fU, 0, false, "        v_cmp_le_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7d4792ffU, 0x40000000U, true, "        v_cmp_le_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d49934fU, 0, false, "        v_cmp_gt_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7d4992ffU, 0x40000000U, true, "        v_cmp_gt_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d4b934fU, 0, false, "        v_cmp_lg_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7d4b92ffU, 0x40000000U, true, "        v_cmp_lg_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d4d934fU, 0, false, "        v_cmp_ge_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7d4d92ffU, 0x40000000U, true, "        v_cmp_ge_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d4f934fU, 0, false, "        v_cmp_tru_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7d4f92ffU, 0x40000000U, true, "        v_cmp_tru_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d51934fU, 0, false, "        v_cmp_class_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7d5192ffU, 0x40000000U, true, "        v_cmp_class_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d53934fU, 0, false, "        VOPC_ill_169    vcc, v79, v201\n" },
    { 0x7d55934fU, 0, false, "        VOPC_ill_170    vcc, v79, v201\n" },
    { 0x7d57934fU, 0, false, "        VOPC_ill_171    vcc, v79, v201\n" },
    { 0x7d59934fU, 0, false, "        VOPC_ill_172    vcc, v79, v201\n" },
    { 0x7d5b934fU, 0, false, "        VOPC_ill_173    vcc, v79, v201\n" },
    { 0x7d5d934fU, 0, false, "        VOPC_ill_174    vcc, v79, v201\n" },
    { 0x7d5f934fU, 0, false, "        VOPC_ill_175    vcc, v79, v201\n" },
    { 0x7d61934fU, 0, false, "        v_cmpx_f_i64    vcc, v[79:80], v[201:202]\n" },
    { 0x7d6192ffU, 0x40000000U, true, "        v_cmpx_f_i64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d63934fU, 0, false, "        v_cmpx_lt_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7d6392ffU, 0x40000000U, true, "        v_cmpx_lt_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d65934fU, 0, false, "        v_cmpx_eq_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7d6592ffU, 0x40000000U, true, "        v_cmpx_eq_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d67934fU, 0, false, "        v_cmpx_le_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7d6792ffU, 0x40000000U, true, "        v_cmpx_le_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d69934fU, 0, false, "        v_cmpx_gt_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7d6992ffU, 0x40000000U, true, "        v_cmpx_gt_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d6b934fU, 0, false, "        v_cmpx_lg_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7d6b92ffU, 0x40000000U, true, "        v_cmpx_lg_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d6d934fU, 0, false, "        v_cmpx_ge_i64   vcc, v[79:80], v[201:202]\n" },
    { 0x7d6d92ffU, 0x40000000U, true, "        v_cmpx_ge_i64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d6f934fU, 0, false, "        v_cmpx_tru_i64  vcc, v[79:80], v[201:202]\n" },
    { 0x7d6f92ffU, 0x40000000U, true, "        v_cmpx_tru_i64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d71934fU, 0, false, "        v_cmpx_class_f64 vcc, v[79:80], v[201:202]\n" },
    { 0x7d7192ffU, 0x40000000U, true, "        v_cmpx_class_f64 "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7d73934fU, 0, false, "        VOPC_ill_185    vcc, v79, v201\n" },
    { 0x7d75934fU, 0, false, "        VOPC_ill_186    vcc, v79, v201\n" },
    { 0x7d77934fU, 0, false, "        VOPC_ill_187    vcc, v79, v201\n" },
    { 0x7d79934fU, 0, false, "        VOPC_ill_188    vcc, v79, v201\n" },
    { 0x7d7b934fU, 0, false, "        VOPC_ill_189    vcc, v79, v201\n" },
    { 0x7d7d934fU, 0, false, "        VOPC_ill_190    vcc, v79, v201\n" },
    { 0x7d7f934fU, 0, false, "        VOPC_ill_191    vcc, v79, v201\n" },
    
    /* unsigned integer comparisons */
    { 0x7d81934fU, 0, false, "        v_cmp_f_u32     vcc, v79, v201\n" },
    { 0x7d8192ffU, 0x40000000U, true, "        v_cmp_f_u32     vcc, 0x40000000, v201\n" },
    { 0x7d83934fU, 0, false, "        v_cmp_lt_u32    vcc, v79, v201\n" },
    { 0x7d8392ffU, 0x40000000U, true, "        v_cmp_lt_u32    vcc, 0x40000000, v201\n" },
    { 0x7d85934fU, 0, false, "        v_cmp_eq_u32    vcc, v79, v201\n" },
    { 0x7d8592ffU, 0x40000000U, true, "        v_cmp_eq_u32    vcc, 0x40000000, v201\n" },
    { 0x7d87934fU, 0, false, "        v_cmp_le_u32    vcc, v79, v201\n" },
    { 0x7d8792ffU, 0x40000000U, true, "        v_cmp_le_u32    vcc, 0x40000000, v201\n" },
    { 0x7d89934fU, 0, false, "        v_cmp_gt_u32    vcc, v79, v201\n" },
    { 0x7d8992ffU, 0x40000000U, true, "        v_cmp_gt_u32    vcc, 0x40000000, v201\n" },
    { 0x7d8b934fU, 0, false, "        v_cmp_lg_u32    vcc, v79, v201\n" },
    { 0x7d8b92ffU, 0x40000000U, true, "        v_cmp_lg_u32    vcc, 0x40000000, v201\n" },
    { 0x7d8d934fU, 0, false, "        v_cmp_ge_u32    vcc, v79, v201\n" },
    { 0x7d8d92ffU, 0x40000000U, true, "        v_cmp_ge_u32    vcc, 0x40000000, v201\n" },
    { 0x7d8f934fU, 0, false, "        v_cmp_tru_u32   vcc, v79, v201\n" },
    { 0x7d8f92ffU, 0x40000000U, true, "        v_cmp_tru_u32   vcc, 0x40000000, v201\n" },
    { 0x7d91934fU, 0, false, "        VOPC_ill_200    vcc, v79, v201\n" },
    { 0x7d93934fU, 0, false, "        VOPC_ill_201    vcc, v79, v201\n" },
    { 0x7d95934fU, 0, false, "        VOPC_ill_202    vcc, v79, v201\n" },
    { 0x7d97934fU, 0, false, "        VOPC_ill_203    vcc, v79, v201\n" },
    { 0x7d99934fU, 0, false, "        VOPC_ill_204    vcc, v79, v201\n" },
    { 0x7d9b934fU, 0, false, "        VOPC_ill_205    vcc, v79, v201\n" },
    { 0x7d9d934fU, 0, false, "        VOPC_ill_206    vcc, v79, v201\n" },
    { 0x7d9f934fU, 0, false, "        VOPC_ill_207    vcc, v79, v201\n" },
    { 0x7da1934fU, 0, false, "        v_cmpx_f_u32    vcc, v79, v201\n" },
    { 0x7da192ffU, 0x40000000U, true, "        v_cmpx_f_u32    vcc, 0x40000000, v201\n" },
    { 0x7da3934fU, 0, false, "        v_cmpx_lt_u32   vcc, v79, v201\n" },
    { 0x7da392ffU, 0x40000000U, true, "        v_cmpx_lt_u32   vcc, 0x40000000, v201\n" },
    { 0x7da5934fU, 0, false, "        v_cmpx_eq_u32   vcc, v79, v201\n" },
    { 0x7da592ffU, 0x40000000U, true, "        v_cmpx_eq_u32   vcc, 0x40000000, v201\n" },
    { 0x7da7934fU, 0, false, "        v_cmpx_le_u32   vcc, v79, v201\n" },
    { 0x7da792ffU, 0x40000000U, true, "        v_cmpx_le_u32   vcc, 0x40000000, v201\n" },
    { 0x7da9934fU, 0, false, "        v_cmpx_gt_u32   vcc, v79, v201\n" },
    { 0x7da992ffU, 0x40000000U, true, "        v_cmpx_gt_u32   vcc, 0x40000000, v201\n" },
    { 0x7dab934fU, 0, false, "        v_cmpx_lg_u32   vcc, v79, v201\n" },
    { 0x7dab92ffU, 0x40000000U, true, "        v_cmpx_lg_u32   vcc, 0x40000000, v201\n" },
    { 0x7dad934fU, 0, false, "        v_cmpx_ge_u32   vcc, v79, v201\n" },
    { 0x7dad92ffU, 0x40000000U, true, "        v_cmpx_ge_u32   vcc, 0x40000000, v201\n" },
    { 0x7daf934fU, 0, false, "        v_cmpx_tru_u32  vcc, v79, v201\n" },
    { 0x7daf92ffU, 0x40000000U, true, "        v_cmpx_tru_u32  vcc, 0x40000000, v201\n" },
    { 0x7db1934fU, 0, false, "        VOPC_ill_216    vcc, v79, v201\n" },
    { 0x7db3934fU, 0, false, "        VOPC_ill_217    vcc, v79, v201\n" },
    { 0x7db5934fU, 0, false, "        VOPC_ill_218    vcc, v79, v201\n" },
    { 0x7db7934fU, 0, false, "        VOPC_ill_219    vcc, v79, v201\n" },
    { 0x7db9934fU, 0, false, "        VOPC_ill_220    vcc, v79, v201\n" },
    { 0x7dbb934fU, 0, false, "        VOPC_ill_221    vcc, v79, v201\n" },
    { 0x7dbd934fU, 0, false, "        VOPC_ill_222    vcc, v79, v201\n" },
    { 0x7dbf934fU, 0, false, "        VOPC_ill_223    vcc, v79, v201\n" },
    
    { 0x7dc1934fU, 0, false, "        v_cmp_f_u64     vcc, v[79:80], v[201:202]\n" },
    { 0x7dc192ffU, 0x40000000U, true, "        v_cmp_f_u64     "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc3934fU, 0, false, "        v_cmp_lt_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc392ffU, 0x40000000U, true, "        v_cmp_lt_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc5934fU, 0, false, "        v_cmp_eq_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc592ffU, 0x40000000U, true, "        v_cmp_eq_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc7934fU, 0, false, "        v_cmp_le_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc792ffU, 0x40000000U, true, "        v_cmp_le_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dc9934fU, 0, false, "        v_cmp_gt_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dc992ffU, 0x40000000U, true, "        v_cmp_gt_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dcb934fU, 0, false, "        v_cmp_lg_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dcb92ffU, 0x40000000U, true, "        v_cmp_lg_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dcd934fU, 0, false, "        v_cmp_ge_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7dcd92ffU, 0x40000000U, true, "        v_cmp_ge_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dcf934fU, 0, false, "        v_cmp_tru_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7dcf92ffU, 0x40000000U, true, "        v_cmp_tru_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7dd1934fU, 0, false, "        VOPC_ill_232    vcc, v79, v201\n" },
    { 0x7dd3934fU, 0, false, "        VOPC_ill_233    vcc, v79, v201\n" },
    { 0x7dd5934fU, 0, false, "        VOPC_ill_234    vcc, v79, v201\n" },
    { 0x7dd7934fU, 0, false, "        VOPC_ill_235    vcc, v79, v201\n" },
    { 0x7dd9934fU, 0, false, "        VOPC_ill_236    vcc, v79, v201\n" },
    { 0x7ddb934fU, 0, false, "        VOPC_ill_237    vcc, v79, v201\n" },
    { 0x7ddd934fU, 0, false, "        VOPC_ill_238    vcc, v79, v201\n" },
    { 0x7ddf934fU, 0, false, "        VOPC_ill_239    vcc, v79, v201\n" },
    { 0x7de1934fU, 0, false, "        v_cmpx_f_u64    vcc, v[79:80], v[201:202]\n" },
    { 0x7de192ffU, 0x40000000U, true, "        v_cmpx_f_u64    "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de3934fU, 0, false, "        v_cmpx_lt_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de392ffU, 0x40000000U, true, "        v_cmpx_lt_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de5934fU, 0, false, "        v_cmpx_eq_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de592ffU, 0x40000000U, true, "        v_cmpx_eq_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de7934fU, 0, false, "        v_cmpx_le_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de792ffU, 0x40000000U, true, "        v_cmpx_le_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7de9934fU, 0, false, "        v_cmpx_gt_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7de992ffU, 0x40000000U, true, "        v_cmpx_gt_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7deb934fU, 0, false, "        v_cmpx_lg_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7deb92ffU, 0x40000000U, true, "        v_cmpx_lg_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7ded934fU, 0, false, "        v_cmpx_ge_u64   vcc, v[79:80], v[201:202]\n" },
    { 0x7ded92ffU, 0x40000000U, true, "        v_cmpx_ge_u64   "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7def934fU, 0, false, "        v_cmpx_tru_u64  vcc, v[79:80], v[201:202]\n" },
    { 0x7def92ffU, 0x40000000U, true, "        v_cmpx_tru_u64  "
                "vcc, 0x40000000, v[201:202]\n" },
    { 0x7df1934fU, 0, false, "        VOPC_ill_248    vcc, v79, v201\n" },
    { 0x7df3934fU, 0, false, "        VOPC_ill_249    vcc, v79, v201\n" },
    { 0x7df5934fU, 0, false, "        VOPC_ill_250    vcc, v79, v201\n" },
    { 0x7df7934fU, 0, false, "        VOPC_ill_251    vcc, v79, v201\n" },
    { 0x7df9934fU, 0, false, "        VOPC_ill_252    vcc, v79, v201\n" },
    { 0x7dfb934fU, 0, false, "        VOPC_ill_253    vcc, v79, v201\n" },
    { 0x7dfd934fU, 0, false, "        VOPC_ill_254    vcc, v79, v201\n" },
    { 0x7dff934fU, 0, false, "        VOPC_ill_255    vcc, v79, v201\n" },
    
    /* VOP3 encoding */
    { 0xd001002aU, 0x0002d732U, true, "        v_cmp_f_f32     s[42:43], v50, v107\n" },
    { 0xd001032aU, 0x0002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107)\n" },
    { 0xd0010b2aU, 0x0002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107) clamp\n" },
    { 0xd0010b2aU, 0x0802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107) mul:2 clamp\n" },
    { 0xd0010b2aU, 0x1002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), abs(v107) mul:4 clamp\n" },
    { 0xd0010b2aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -abs(v107) div:2 clamp neg2\n" },
    { 0xd0010b2aU, 0xd802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], abs(v50), -abs(v107) div:2 clamp neg2\n" },
    { 0xd0010b2aU, 0xb802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), abs(v107) div:2 clamp neg2\n" },
    { 0xd0010a2aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -v50, -abs(v107) div:2 clamp neg2\n" },
    { 0xd001092aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -v107 div:2 clamp neg2\n" },
    { 0xd001012aU, 0x7002d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -v107 mul:4\n" },
    { 0xd001092aU, 0xf802d732U, true, "        v_cmp_f_f32     "
                "s[42:43], -abs(v50), -v107 div:2 clamp neg2\n" },
    /* vop3 */
    { 0xd001006aU, 0x0002d732U, true, "        v_cmp_f_f32     vcc, v50, v107 vop3\n" },
    { 0xd001006aU, 0x0002d632U, true, "        v_cmp_f_f32     vcc, s50, v107 vop3\n" },
    /* no vop3 */
    { 0xd001086aU, 0x0002d632U, true, "        v_cmp_f_f32     vcc, s50, v107 clamp\n" },
    { 0xd001006aU, 0x0000d632U, true, "        v_cmp_f_f32     vcc, s50, vcc_hi\n" },
    { 0xd001002aU, 0x00aed732U, true, "        v_cmp_f_f32     "
                "s[42:43], v50, v107 vsrc2=0x2b\n" },
    { 0xd001006aU, 0x00aed732U, true, "        v_cmp_f_f32     "
                "vcc, v50, v107 vsrc2=0x2b\n" },
    { 0xd001006aU, 0x1002d732U, true, "        v_cmp_f_f32     vcc, v50, v107 mul:4\n" },
    { 0xd001006aU, 0x8002d732U, true, "        v_cmp_f_f32     vcc, v50, v107 neg2\n" },
    { 0xd001006aU, 0x4002d732U, true, "        v_cmp_f_f32     vcc, v50, -v107\n" },
    { 0xd001016aU, 0x0002d732U, true, "        v_cmp_f_f32     vcc, abs(v50), v107\n" },
    /// ??????
    { 0xd001002aU, 0x0001feffU, true, "        v_cmp_f_f32     s[42:43], 0x0, 0x0\n" },
    { 0xd00100ffU, 0x0001feffU, true, "        v_cmp_f_f32     0x0, 0x0, 0x0\n" },
    /*** VOP3 comparisons ***/
    { 0xd001002aU, 0x0002d732U, true, "        v_cmp_f_f32     s[42:43], v50, v107\n" },
    { 0xd003002aU, 0x0002d732U, true, "        v_cmp_lt_f32    s[42:43], v50, v107\n" },
    { 0xd005002aU, 0x0002d732U, true, "        v_cmp_eq_f32    s[42:43], v50, v107\n" },
    { 0xd007002aU, 0x0002d732U, true, "        v_cmp_le_f32    s[42:43], v50, v107\n" },
    { 0xd009002aU, 0x0002d732U, true, "        v_cmp_gt_f32    s[42:43], v50, v107\n" },
    { 0xd00b002aU, 0x0002d732U, true, "        v_cmp_lg_f32    s[42:43], v50, v107\n" },
    { 0xd00d002aU, 0x0002d732U, true, "        v_cmp_ge_f32    s[42:43], v50, v107\n" },
    { 0xd00f002aU, 0x0002d732U, true, "        v_cmp_o_f32     s[42:43], v50, v107\n" },
    { 0xd011002aU, 0x0002d732U, true, "        v_cmp_u_f32     s[42:43], v50, v107\n" },
    { 0xd013002aU, 0x0002d732U, true, "        v_cmp_nge_f32   s[42:43], v50, v107\n" },
    { 0xd015002aU, 0x0002d732U, true, "        v_cmp_nlg_f32   s[42:43], v50, v107\n" },
    { 0xd017002aU, 0x0002d732U, true, "        v_cmp_ngt_f32   s[42:43], v50, v107\n" },
    { 0xd019002aU, 0x0002d732U, true, "        v_cmp_nle_f32   s[42:43], v50, v107\n" },
    { 0xd01b002aU, 0x0002d732U, true, "        v_cmp_neq_f32   s[42:43], v50, v107\n" },
    { 0xd01d002aU, 0x0002d732U, true, "        v_cmp_nlt_f32   s[42:43], v50, v107\n" },
    { 0xd01f002aU, 0x0002d732U, true, "        v_cmp_tru_f32   s[42:43], v50, v107\n" },
    { 0xd021002aU, 0x0002d732U, true, "        v_cmpx_f_f32    s[42:43], v50, v107\n" },
    { 0xd023002aU, 0x0002d732U, true, "        v_cmpx_lt_f32   s[42:43], v50, v107\n" },
    { 0xd025002aU, 0x0002d732U, true, "        v_cmpx_eq_f32   s[42:43], v50, v107\n" },
    { 0xd027002aU, 0x0002d732U, true, "        v_cmpx_le_f32   s[42:43], v50, v107\n" },
    { 0xd029002aU, 0x0002d732U, true, "        v_cmpx_gt_f32   s[42:43], v50, v107\n" },
    { 0xd02b002aU, 0x0002d732U, true, "        v_cmpx_lg_f32   s[42:43], v50, v107\n" },
    { 0xd02d002aU, 0x0002d732U, true, "        v_cmpx_ge_f32   s[42:43], v50, v107\n" },
    { 0xd02f002aU, 0x0002d732U, true, "        v_cmpx_o_f32    s[42:43], v50, v107\n" },
    { 0xd031002aU, 0x0002d732U, true, "        v_cmpx_u_f32    s[42:43], v50, v107\n" },
    { 0xd033002aU, 0x0002d732U, true, "        v_cmpx_nge_f32  s[42:43], v50, v107\n" },
    { 0xd035002aU, 0x0002d732U, true, "        v_cmpx_nlg_f32  s[42:43], v50, v107\n" },
    { 0xd037002aU, 0x0002d732U, true, "        v_cmpx_ngt_f32  s[42:43], v50, v107\n" },
    { 0xd039002aU, 0x0002d732U, true, "        v_cmpx_nle_f32  s[42:43], v50, v107\n" },
    { 0xd03b002aU, 0x0002d732U, true, "        v_cmpx_neq_f32  s[42:43], v50, v107\n" },
    { 0xd03d002aU, 0x0002d732U, true, "        v_cmpx_nlt_f32  s[42:43], v50, v107\n" },
    { 0xd03f002aU, 0x0002d732U, true, "        v_cmpx_tru_f32  s[42:43], v50, v107\n" },
    
    { 0xd041002aU, 0x0002d732U, true, "        v_cmp_f_f64     "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd043002aU, 0x0002d732U, true, "        v_cmp_lt_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd045002aU, 0x0002d732U, true, "        v_cmp_eq_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd047002aU, 0x0002d732U, true, "        v_cmp_le_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd049002aU, 0x0002d732U, true, "        v_cmp_gt_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd04b002aU, 0x0002d732U, true, "        v_cmp_lg_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd04d002aU, 0x0002d732U, true, "        v_cmp_ge_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd04f002aU, 0x0002d732U, true, "        v_cmp_o_f64     "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd051002aU, 0x0002d732U, true, "        v_cmp_u_f64     "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd053002aU, 0x0002d732U, true, "        v_cmp_nge_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd055002aU, 0x0002d732U, true, "        v_cmp_nlg_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd057002aU, 0x0002d732U, true, "        v_cmp_ngt_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd059002aU, 0x0002d732U, true, "        v_cmp_nle_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd05b002aU, 0x0002d732U, true, "        v_cmp_neq_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd05d002aU, 0x0002d732U, true, "        v_cmp_nlt_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd05f002aU, 0x0002d732U, true, "        v_cmp_tru_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd061002aU, 0x0002d732U, true, "        v_cmpx_f_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd063002aU, 0x0002d732U, true, "        v_cmpx_lt_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd065002aU, 0x0002d732U, true, "        v_cmpx_eq_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd067002aU, 0x0002d732U, true, "        v_cmpx_le_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd069002aU, 0x0002d732U, true, "        v_cmpx_gt_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06b002aU, 0x0002d732U, true, "        v_cmpx_lg_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06d002aU, 0x0002d732U, true, "        v_cmpx_ge_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd06f002aU, 0x0002d732U, true, "        v_cmpx_o_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd071002aU, 0x0002d732U, true, "        v_cmpx_u_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd073002aU, 0x0002d732U, true, "        v_cmpx_nge_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd075002aU, 0x0002d732U, true, "        v_cmpx_nlg_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd077002aU, 0x0002d732U, true, "        v_cmpx_ngt_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd079002aU, 0x0002d732U, true, "        v_cmpx_nle_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07b002aU, 0x0002d732U, true, "        v_cmpx_neq_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07d002aU, 0x0002d732U, true, "        v_cmpx_nlt_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd07f002aU, 0x0002d732U, true, "        v_cmpx_tru_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    /* VOP3 v_cmps*** */
    { 0xd081002aU, 0x0002d732U, true, "        v_cmps_f_f32    s[42:43], v50, v107\n" },
    { 0xd083002aU, 0x0002d732U, true, "        v_cmps_lt_f32   s[42:43], v50, v107\n" },
    { 0xd085002aU, 0x0002d732U, true, "        v_cmps_eq_f32   s[42:43], v50, v107\n" },
    { 0xd087002aU, 0x0002d732U, true, "        v_cmps_le_f32   s[42:43], v50, v107\n" },
    { 0xd089002aU, 0x0002d732U, true, "        v_cmps_gt_f32   s[42:43], v50, v107\n" },
    { 0xd08b002aU, 0x0002d732U, true, "        v_cmps_lg_f32   s[42:43], v50, v107\n" },
    { 0xd08d002aU, 0x0002d732U, true, "        v_cmps_ge_f32   s[42:43], v50, v107\n" },
    { 0xd08f002aU, 0x0002d732U, true, "        v_cmps_o_f32    s[42:43], v50, v107\n" },
    { 0xd091002aU, 0x0002d732U, true, "        v_cmps_u_f32    s[42:43], v50, v107\n" },
    { 0xd093002aU, 0x0002d732U, true, "        v_cmps_nge_f32  s[42:43], v50, v107\n" },
    { 0xd095002aU, 0x0002d732U, true, "        v_cmps_nlg_f32  s[42:43], v50, v107\n" },
    { 0xd097002aU, 0x0002d732U, true, "        v_cmps_ngt_f32  s[42:43], v50, v107\n" },
    { 0xd099002aU, 0x0002d732U, true, "        v_cmps_nle_f32  s[42:43], v50, v107\n" },
    { 0xd09b002aU, 0x0002d732U, true, "        v_cmps_neq_f32  s[42:43], v50, v107\n" },
    { 0xd09d002aU, 0x0002d732U, true, "        v_cmps_nlt_f32  s[42:43], v50, v107\n" },
    { 0xd09f002aU, 0x0002d732U, true, "        v_cmps_tru_f32  s[42:43], v50, v107\n" },
    { 0xd0a1002aU, 0x0002d732U, true, "        v_cmpsx_f_f32   s[42:43], v50, v107\n" },
    { 0xd0a3002aU, 0x0002d732U, true, "        v_cmpsx_lt_f32  s[42:43], v50, v107\n" },
    { 0xd0a5002aU, 0x0002d732U, true, "        v_cmpsx_eq_f32  s[42:43], v50, v107\n" },
    { 0xd0a7002aU, 0x0002d732U, true, "        v_cmpsx_le_f32  s[42:43], v50, v107\n" },
    { 0xd0a9002aU, 0x0002d732U, true, "        v_cmpsx_gt_f32  s[42:43], v50, v107\n" },
    { 0xd0ab002aU, 0x0002d732U, true, "        v_cmpsx_lg_f32  s[42:43], v50, v107\n" },
    { 0xd0ad002aU, 0x0002d732U, true, "        v_cmpsx_ge_f32  s[42:43], v50, v107\n" },
    { 0xd0af002aU, 0x0002d732U, true, "        v_cmpsx_o_f32   s[42:43], v50, v107\n" },
    { 0xd0b1002aU, 0x0002d732U, true, "        v_cmpsx_u_f32   s[42:43], v50, v107\n" },
    { 0xd0b3002aU, 0x0002d732U, true, "        v_cmpsx_nge_f32 s[42:43], v50, v107\n" },
    { 0xd0b5002aU, 0x0002d732U, true, "        v_cmpsx_nlg_f32 s[42:43], v50, v107\n" },
    { 0xd0b7002aU, 0x0002d732U, true, "        v_cmpsx_ngt_f32 s[42:43], v50, v107\n" },
    { 0xd0b9002aU, 0x0002d732U, true, "        v_cmpsx_nle_f32 s[42:43], v50, v107\n" },
    { 0xd0bb002aU, 0x0002d732U, true, "        v_cmpsx_neq_f32 s[42:43], v50, v107\n" },
    { 0xd0bd002aU, 0x0002d732U, true, "        v_cmpsx_nlt_f32 s[42:43], v50, v107\n" },
    { 0xd0bf002aU, 0x0002d732U, true, "        v_cmpsx_tru_f32 s[42:43], v50, v107\n" },
    
    { 0xd0c1002aU, 0x0002d732U, true, "        v_cmps_f_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0c3002aU, 0x0002d732U, true, "        v_cmps_lt_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0c5002aU, 0x0002d732U, true, "        v_cmps_eq_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0c7002aU, 0x0002d732U, true, "        v_cmps_le_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0c9002aU, 0x0002d732U, true, "        v_cmps_gt_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0cb002aU, 0x0002d732U, true, "        v_cmps_lg_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0cd002aU, 0x0002d732U, true, "        v_cmps_ge_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0cf002aU, 0x0002d732U, true, "        v_cmps_o_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0d1002aU, 0x0002d732U, true, "        v_cmps_u_f64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0d3002aU, 0x0002d732U, true, "        v_cmps_nge_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0d5002aU, 0x0002d732U, true, "        v_cmps_nlg_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0d7002aU, 0x0002d732U, true, "        v_cmps_ngt_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0d9002aU, 0x0002d732U, true, "        v_cmps_nle_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0db002aU, 0x0002d732U, true, "        v_cmps_neq_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0dd002aU, 0x0002d732U, true, "        v_cmps_nlt_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0df002aU, 0x0002d732U, true, "        v_cmps_tru_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e1002aU, 0x0002d732U, true, "        v_cmpsx_f_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e3002aU, 0x0002d732U, true, "        v_cmpsx_lt_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e5002aU, 0x0002d732U, true, "        v_cmpsx_eq_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e7002aU, 0x0002d732U, true, "        v_cmpsx_le_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0e9002aU, 0x0002d732U, true, "        v_cmpsx_gt_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0eb002aU, 0x0002d732U, true, "        v_cmpsx_lg_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ed002aU, 0x0002d732U, true, "        v_cmpsx_ge_f64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ef002aU, 0x0002d732U, true, "        v_cmpsx_o_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f1002aU, 0x0002d732U, true, "        v_cmpsx_u_f64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f3002aU, 0x0002d732U, true, "        v_cmpsx_nge_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f5002aU, 0x0002d732U, true, "        v_cmpsx_nlg_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f7002aU, 0x0002d732U, true, "        v_cmpsx_ngt_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0f9002aU, 0x0002d732U, true, "        v_cmpsx_nle_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0fb002aU, 0x0002d732U, true, "        v_cmpsx_neq_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0fd002aU, 0x0002d732U, true, "        v_cmpsx_nlt_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd0ff002aU, 0x0002d732U, true, "        v_cmpsx_tru_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    
    /* VOP3 integer comparisons */
    { 0xd101002aU, 0x0002d732U, true, "        v_cmp_f_i32     s[42:43], v50, v107\n" },
    { 0xd103002aU, 0x0002d732U, true, "        v_cmp_lt_i32    s[42:43], v50, v107\n" },
    { 0xd105002aU, 0x0002d732U, true, "        v_cmp_eq_i32    s[42:43], v50, v107\n" },
    { 0xd107002aU, 0x0002d732U, true, "        v_cmp_le_i32    s[42:43], v50, v107\n" },
    { 0xd109002aU, 0x0002d732U, true, "        v_cmp_gt_i32    s[42:43], v50, v107\n" },
    { 0xd10b002aU, 0x0002d732U, true, "        v_cmp_lg_i32    s[42:43], v50, v107\n" },
    { 0xd10d002aU, 0x0002d732U, true, "        v_cmp_ge_i32    s[42:43], v50, v107\n" },
    { 0xd10f002aU, 0x0002d732U, true, "        v_cmp_tru_i32   s[42:43], v50, v107\n" },
    { 0xd111002aU, 0x0002d732U, true, "        v_cmp_class_f32 s[42:43], v50, v107\n" },
    { 0xd113002aU, 0x0002d732U, true, "        VOP3A_ill_137   s[42:43], v50, v107\n" },
    { 0xd115002aU, 0x0002d732U, true, "        VOP3A_ill_138   s[42:43], v50, v107\n" },
    { 0xd117002aU, 0x0002d732U, true, "        VOP3A_ill_139   s[42:43], v50, v107\n" },
    { 0xd119002aU, 0x0002d732U, true, "        VOP3A_ill_140   s[42:43], v50, v107\n" },
    { 0xd11b002aU, 0x0002d732U, true, "        VOP3A_ill_141   s[42:43], v50, v107\n" },
    { 0xd11d002aU, 0x0002d732U, true, "        VOP3A_ill_142   s[42:43], v50, v107\n" },
    { 0xd11f002aU, 0x0002d732U, true, "        VOP3A_ill_143   s[42:43], v50, v107\n" },
    { 0xd121002aU, 0x0002d732U, true, "        v_cmpx_f_i32    s[42:43], v50, v107\n" },
    { 0xd123002aU, 0x0002d732U, true, "        v_cmpx_lt_i32   s[42:43], v50, v107\n" },
    { 0xd125002aU, 0x0002d732U, true, "        v_cmpx_eq_i32   s[42:43], v50, v107\n" },
    { 0xd127002aU, 0x0002d732U, true, "        v_cmpx_le_i32   s[42:43], v50, v107\n" },
    { 0xd129002aU, 0x0002d732U, true, "        v_cmpx_gt_i32   s[42:43], v50, v107\n" },
    { 0xd12b002aU, 0x0002d732U, true, "        v_cmpx_lg_i32   s[42:43], v50, v107\n" },
    { 0xd12d002aU, 0x0002d732U, true, "        v_cmpx_ge_i32   s[42:43], v50, v107\n" },
    { 0xd12f002aU, 0x0002d732U, true, "        v_cmpx_tru_i32  s[42:43], v50, v107\n" },
    { 0xd131002aU, 0x0002d732U, true, "        v_cmpx_class_f32 s[42:43], v50, v107\n" },
    { 0xd133002aU, 0x0002d732U, true, "        VOP3A_ill_153   s[42:43], v50, v107\n" },
    { 0xd135002aU, 0x0002d732U, true, "        VOP3A_ill_154   s[42:43], v50, v107\n" },
    { 0xd137002aU, 0x0002d732U, true, "        VOP3A_ill_155   s[42:43], v50, v107\n" },
    { 0xd139002aU, 0x0002d732U, true, "        VOP3A_ill_156   s[42:43], v50, v107\n" },
    { 0xd13b002aU, 0x0002d732U, true, "        VOP3A_ill_157   s[42:43], v50, v107\n" },
    { 0xd13d002aU, 0x0002d732U, true, "        VOP3A_ill_158   s[42:43], v50, v107\n" },
    { 0xd13f002aU, 0x0002d732U, true, "        VOP3A_ill_159   s[42:43], v50, v107\n" },
    /* 64-bit integers */
    { 0xd141002aU, 0x0002d732U, true, "        v_cmp_f_i64     "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd143002aU, 0x0002d732U, true, "        v_cmp_lt_i64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd145002aU, 0x0002d732U, true, "        v_cmp_eq_i64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd147002aU, 0x0002d732U, true, "        v_cmp_le_i64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd149002aU, 0x0002d732U, true, "        v_cmp_gt_i64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd14b002aU, 0x0002d732U, true, "        v_cmp_lg_i64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd14d002aU, 0x0002d732U, true, "        v_cmp_ge_i64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd14f002aU, 0x0002d732U, true, "        v_cmp_tru_i64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd151002aU, 0x0002d732U, true, "        v_cmp_class_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd153002aU, 0x0002d732U, true, "        VOP3A_ill_169   s[42:43], v50, v107\n" },
    { 0xd155002aU, 0x0002d732U, true, "        VOP3A_ill_170   s[42:43], v50, v107\n" },
    { 0xd157002aU, 0x0002d732U, true, "        VOP3A_ill_171   s[42:43], v50, v107\n" },
    { 0xd159002aU, 0x0002d732U, true, "        VOP3A_ill_172   s[42:43], v50, v107\n" },
    { 0xd15b002aU, 0x0002d732U, true, "        VOP3A_ill_173   s[42:43], v50, v107\n" },
    { 0xd15d002aU, 0x0002d732U, true, "        VOP3A_ill_174   s[42:43], v50, v107\n" },
    { 0xd15f002aU, 0x0002d732U, true, "        VOP3A_ill_175   s[42:43], v50, v107\n" },
    { 0xd161002aU, 0x0002d732U, true, "        v_cmpx_f_i64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd163002aU, 0x0002d732U, true, "        v_cmpx_lt_i64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd165002aU, 0x0002d732U, true, "        v_cmpx_eq_i64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd167002aU, 0x0002d732U, true, "        v_cmpx_le_i64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd169002aU, 0x0002d732U, true, "        v_cmpx_gt_i64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd16b002aU, 0x0002d732U, true, "        v_cmpx_lg_i64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd16d002aU, 0x0002d732U, true, "        v_cmpx_ge_i64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd16f002aU, 0x0002d732U, true, "        v_cmpx_tru_i64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd171002aU, 0x0002d732U, true, "        v_cmpx_class_f64 "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd173002aU, 0x0002d732U, true, "        VOP3A_ill_185   s[42:43], v50, v107\n" },
    { 0xd175002aU, 0x0002d732U, true, "        VOP3A_ill_186   s[42:43], v50, v107\n" },
    { 0xd177002aU, 0x0002d732U, true, "        VOP3A_ill_187   s[42:43], v50, v107\n" },
    { 0xd179002aU, 0x0002d732U, true, "        VOP3A_ill_188   s[42:43], v50, v107\n" },
    { 0xd17b002aU, 0x0002d732U, true, "        VOP3A_ill_189   s[42:43], v50, v107\n" },
    { 0xd17d002aU, 0x0002d732U, true, "        VOP3A_ill_190   s[42:43], v50, v107\n" },
    { 0xd17f002aU, 0x0002d732U, true, "        VOP3A_ill_191   s[42:43], v50, v107\n" },
    
    /* VOP3 unsigned integer comparisons */
    { 0xd181002aU, 0x0002d732U, true, "        v_cmp_f_u32     s[42:43], v50, v107\n" },
    { 0xd183002aU, 0x0002d732U, true, "        v_cmp_lt_u32    s[42:43], v50, v107\n" },
    { 0xd185002aU, 0x0002d732U, true, "        v_cmp_eq_u32    s[42:43], v50, v107\n" },
    { 0xd187002aU, 0x0002d732U, true, "        v_cmp_le_u32    s[42:43], v50, v107\n" },
    { 0xd189002aU, 0x0002d732U, true, "        v_cmp_gt_u32    s[42:43], v50, v107\n" },
    { 0xd18b002aU, 0x0002d732U, true, "        v_cmp_lg_u32    s[42:43], v50, v107\n" },
    { 0xd18d002aU, 0x0002d732U, true, "        v_cmp_ge_u32    s[42:43], v50, v107\n" },
    { 0xd18f002aU, 0x0002d732U, true, "        v_cmp_tru_u32   s[42:43], v50, v107\n" },
    { 0xd191002aU, 0x0002d732U, true, "        VOP3A_ill_200   s[42:43], v50, v107\n" },
    { 0xd193002aU, 0x0002d732U, true, "        VOP3A_ill_201   s[42:43], v50, v107\n" },
    { 0xd195002aU, 0x0002d732U, true, "        VOP3A_ill_202   s[42:43], v50, v107\n" },
    { 0xd197002aU, 0x0002d732U, true, "        VOP3A_ill_203   s[42:43], v50, v107\n" },
    { 0xd199002aU, 0x0002d732U, true, "        VOP3A_ill_204   s[42:43], v50, v107\n" },
    { 0xd19b002aU, 0x0002d732U, true, "        VOP3A_ill_205   s[42:43], v50, v107\n" },
    { 0xd19d002aU, 0x0002d732U, true, "        VOP3A_ill_206   s[42:43], v50, v107\n" },
    { 0xd19f002aU, 0x0002d732U, true, "        VOP3A_ill_207   s[42:43], v50, v107\n" },
    { 0xd1a1002aU, 0x0002d732U, true, "        v_cmpx_f_u32    s[42:43], v50, v107\n" },
    { 0xd1a3002aU, 0x0002d732U, true, "        v_cmpx_lt_u32   s[42:43], v50, v107\n" },
    { 0xd1a5002aU, 0x0002d732U, true, "        v_cmpx_eq_u32   s[42:43], v50, v107\n" },
    { 0xd1a7002aU, 0x0002d732U, true, "        v_cmpx_le_u32   s[42:43], v50, v107\n" },
    { 0xd1a9002aU, 0x0002d732U, true, "        v_cmpx_gt_u32   s[42:43], v50, v107\n" },
    { 0xd1ab002aU, 0x0002d732U, true, "        v_cmpx_lg_u32   s[42:43], v50, v107\n" },
    { 0xd1ad002aU, 0x0002d732U, true, "        v_cmpx_ge_u32   s[42:43], v50, v107\n" },
    { 0xd1af002aU, 0x0002d732U, true, "        v_cmpx_tru_u32  s[42:43], v50, v107\n" },
    { 0xd1b1002aU, 0x0002d732U, true, "        VOP3A_ill_216   s[42:43], v50, v107\n" },
    { 0xd1b3002aU, 0x0002d732U, true, "        VOP3A_ill_217   s[42:43], v50, v107\n" },
    { 0xd1b5002aU, 0x0002d732U, true, "        VOP3A_ill_218   s[42:43], v50, v107\n" },
    { 0xd1b7002aU, 0x0002d732U, true, "        VOP3A_ill_219   s[42:43], v50, v107\n" },
    { 0xd1b9002aU, 0x0002d732U, true, "        VOP3A_ill_220   s[42:43], v50, v107\n" },
    { 0xd1bb002aU, 0x0002d732U, true, "        VOP3A_ill_221   s[42:43], v50, v107\n" },
    { 0xd1bd002aU, 0x0002d732U, true, "        VOP3A_ill_222   s[42:43], v50, v107\n" },
    { 0xd1bf002aU, 0x0002d732U, true, "        VOP3A_ill_223   s[42:43], v50, v107\n" },
    /* 64-bit integers */
    { 0xd1c1002aU, 0x0002d732U, true, "        v_cmp_f_u64     "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1c3002aU, 0x0002d732U, true, "        v_cmp_lt_u64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1c5002aU, 0x0002d732U, true, "        v_cmp_eq_u64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1c7002aU, 0x0002d732U, true, "        v_cmp_le_u64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1c9002aU, 0x0002d732U, true, "        v_cmp_gt_u64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1cb002aU, 0x0002d732U, true, "        v_cmp_lg_u64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1cd002aU, 0x0002d732U, true, "        v_cmp_ge_u64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1cf002aU, 0x0002d732U, true, "        v_cmp_tru_u64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1d1002aU, 0x0002d732U, true, "        VOP3A_ill_232   s[42:43], v50, v107\n" },
    { 0xd1d3002aU, 0x0002d732U, true, "        VOP3A_ill_233   s[42:43], v50, v107\n" },
    { 0xd1d5002aU, 0x0002d732U, true, "        VOP3A_ill_234   s[42:43], v50, v107\n" },
    { 0xd1d7002aU, 0x0002d732U, true, "        VOP3A_ill_235   s[42:43], v50, v107\n" },
    { 0xd1d9002aU, 0x0002d732U, true, "        VOP3A_ill_236   s[42:43], v50, v107\n" },
    { 0xd1db002aU, 0x0002d732U, true, "        VOP3A_ill_237   s[42:43], v50, v107\n" },
    { 0xd1dd002aU, 0x0002d732U, true, "        VOP3A_ill_238   s[42:43], v50, v107\n" },
    { 0xd1df002aU, 0x0002d732U, true, "        VOP3A_ill_239   s[42:43], v50, v107\n" },
    { 0xd1e1002aU, 0x0002d732U, true, "        v_cmpx_f_u64    "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1e3002aU, 0x0002d732U, true, "        v_cmpx_lt_u64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1e5002aU, 0x0002d732U, true, "        v_cmpx_eq_u64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1e7002aU, 0x0002d732U, true, "        v_cmpx_le_u64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1e9002aU, 0x0002d732U, true, "        v_cmpx_gt_u64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1eb002aU, 0x0002d732U, true, "        v_cmpx_lg_u64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1ed002aU, 0x0002d732U, true, "        v_cmpx_ge_u64   "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1ef002aU, 0x0002d732U, true, "        v_cmpx_tru_u64  "
                "s[42:43], v[50:51], v[107:108]\n" },
    { 0xd1f1002aU, 0x0002d732U, true, "        VOP3A_ill_248   s[42:43], v50, v107\n" },
    { 0xd1f3002aU, 0x0002d732U, true, "        VOP3A_ill_249   s[42:43], v50, v107\n" },
    { 0xd1f5002aU, 0x0002d732U, true, "        VOP3A_ill_250   s[42:43], v50, v107\n" },
    { 0xd1f7002aU, 0x0002d732U, true, "        VOP3A_ill_251   s[42:43], v50, v107\n" },
    { 0xd1f9002aU, 0x0002d732U, true, "        VOP3A_ill_252   s[42:43], v50, v107\n" },
    { 0xd1fb002aU, 0x0002d732U, true, "        VOP3A_ill_253   s[42:43], v50, v107\n" },
    { 0xd1fd002aU, 0x0002d732U, true, "        VOP3A_ill_254   s[42:43], v50, v107\n" },
    { 0xd1ff002aU, 0x0002d732U, true, "        VOP3A_ill_255   s[42:43], v50, v107\n" },
    
    /* VOP2 instructions encoded to VOP3a/VOP3b */
    { 0xd200002aU, 0x0002d732U, true, "        v_cndmask_b32   v42, v50, v107, s[0:1]\n" },
    { 0xd200002aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd200002aU, 0x003ed732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, s[15:16]\n" },
    { 0xd200002aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd200042aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd200002aU, 0x81aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    /* no vop3 */
    { 0xd200022aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, v50, abs(v107), vcc\n" },
    { 0xd200002aU, 0x41aad732U, true, "        v_cndmask_b32   v42, v50, -v107, vcc\n" },
    { 0xd200012aU, 0x01aad732U, true, "        v_cndmask_b32   "
                "v42, abs(v50), v107, vcc\n" },
    { 0xd200002aU, 0x21aad732U, true, "        v_cndmask_b32   v42, -v50, v107, vcc\n" },
    { 0xd200082aU, 0x81aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc clamp\n" },
    { 0xd200002aU, 0x81aad732U, true, "        v_cndmask_b32   "
                "v42, v50, v107, vcc vop3\n" },
    { 0xd200002aU, 0x81a8c932U, true, "        v_cndmask_b32   "
                "v42, v50, s100, vcc\n" },
    /* VOP2 in VOP3 */
    /** ??? */
    { 0xd2020037U, 0x0002b51bU, true, "        v_readlane_b32  s55, v27, v90\n" },
    { 0xd2040037U, 0x0002b51bU, true, "        v_writelane_b32 v55, v27, v90\n" },
    { 0xd2040037U, 0x0002b41bU, true, "        v_writelane_b32 v55, s27, v90\n" },
    { 0xd2020037U, 0x0000b51bU, true, "        v_readlane_b32  s55, v27, s90 vop3\n" },
    { 0xd2040037U, 0x0000b51bU, true, "        v_writelane_b32 v55, v27, s90 vop3\n" },
    { 0xd2040037U, 0x0000b41bU, true, "        v_writelane_b32 v55, s27, s90 vop3\n" },
    { 0xd2040037U, 0x00ceb41bU, true, "        v_writelane_b32 v55, s27, v90 vsrc2=0x33\n" },
    /* vadd */
    { 0xd2060037U, 0x0002b41bU, true, "        v_add_f32       v55, s27, v90 vop3\n" },
    { 0xd2060037U, 0x0000b41bU, true, "        v_add_f32       v55, s27, s90\n" },
    { 0xd2060837U, 0x0002b41bU, true, "        v_add_f32       v55, s27, v90 clamp\n" },
    { 0xd2060037U, 0x4002b41bU, true, "        v_add_f32       v55, s27, -v90\n" },
    { 0xd2060237U, 0x4002b41bU, true, "        v_add_f32       v55, s27, -abs(v90)\n" },
    { 0xd2060437U, 0x0002b41bU, true, "        v_add_f32       v55, s27, v90 abs2\n" },
    /* other opcodes */
    { 0xd2080037U, 0x4002b41bU, true, "        v_sub_f32       v55, s27, -v90\n" },
    { 0xd20a0037U, 0x4002b41bU, true, "        v_subrev_f32    v55, s27, -v90\n" },
    { 0xd20c0037U, 0x4002b41bU, true, "        v_mac_legacy_f32 v55, s27, -v90\n" },
    { 0xd20e0037U, 0x4002b41bU, true, "        v_mul_legacy_f32 v55, s27, -v90\n" },
    { 0xd2100037U, 0x4002b41bU, true, "        v_mul_f32       v55, s27, -v90\n" },
    { 0xd2120037U, 0x4002b41bU, true, "        v_mul_i32_i24   v55, s27, -v90\n" },
    { 0xd2140037U, 0x4002b41bU, true, "        v_mul_hi_i32_i24 v55, s27, -v90\n" },
    { 0xd2160037U, 0x4002b41bU, true, "        v_mul_u32_u24   v55, s27, -v90\n" },
    { 0xd2180037U, 0x4002b41bU, true, "        v_mul_hi_u32_u24 v55, s27, -v90\n" },
    { 0xd21a0037U, 0x4002b41bU, true, "        v_min_legacy_f32 v55, s27, -v90\n" },
    { 0xd21c0037U, 0x4002b41bU, true, "        v_max_legacy_f32 v55, s27, -v90\n" },
    { 0xd21e0037U, 0x4002b41bU, true, "        v_min_f32       v55, s27, -v90\n" },
    { 0xd2200037U, 0x4002b41bU, true, "        v_max_f32       v55, s27, -v90\n" },
    { 0xd2220037U, 0x4002b41bU, true, "        v_min_i32       v55, s27, -v90\n" },
    { 0xd2240037U, 0x4002b41bU, true, "        v_max_i32       v55, s27, -v90\n" },
    { 0xd2260037U, 0x4002b41bU, true, "        v_min_u32       v55, s27, -v90\n" },
    { 0xd2280037U, 0x4002b41bU, true, "        v_max_u32       v55, s27, -v90\n" },
    { 0xd22a0037U, 0x4002b41bU, true, "        v_lshr_b32      v55, s27, -v90\n" },
    { 0xd22c0037U, 0x4002b41bU, true, "        v_lshrrev_b32   v55, s27, -v90\n" },
    { 0xd22e0037U, 0x4002b41bU, true, "        v_ashr_i32      v55, s27, -v90\n" },
    { 0xd2300037U, 0x4002b41bU, true, "        v_ashrrev_i32   v55, s27, -v90\n" },
    { 0xd2320037U, 0x4002b41bU, true, "        v_lshl_b32      v55, s27, -v90\n" },
    { 0xd2340037U, 0x4002b41bU, true, "        v_lshlrev_b32   v55, s27, -v90\n" },
    { 0xd2360037U, 0x4002b41bU, true, "        v_and_b32       v55, s27, -v90\n" },
    { 0xd2380037U, 0x4002b41bU, true, "        v_or_b32        v55, s27, -v90\n" },
    { 0xd23a0037U, 0x4002b41bU, true, "        v_xor_b32       v55, s27, -v90\n" },
    { 0xd23c0037U, 0x4002b41bU, true, "        v_bfm_b32       v55, s27, -v90\n" },
    { 0xd23e0037U, 0x4002b41bU, true, "        v_mac_f32       v55, s27, -v90\n" },
    /* what is syntax for v_madmk_f32 and v_madak_f32 in vop3 encoding? I don't know */
    { 0xd2400037U, 0x405ab41bU, true, "        v_madmk_f32     v55, s27, -v90, s22 vop3\n" },
    { 0xd2400037U, 0x0002b41bU, true, "        v_madmk_f32     v55, s27, v90, s0 vop3\n" },
    { 0xd2400037U, 0x445ab41bU, true, "        v_madmk_f32     v55, s27, -v90, v22 vop3\n" },
    { 0xd2420037U, 0x405ab41bU, true, "        v_madak_f32     v55, s27, -v90, s22 vop3\n" },
    { 0xd2420037U, 0x0002b41bU, true, "        v_madak_f32     v55, s27, v90, s0 vop3\n" },
    /* further instructions */
    { 0xd2440037U, 0x4002b41bU, true, "        v_bcnt_u32_b32  v55, s27, -v90\n" },
    { 0xd2460037U, 0x4002b41bU, true, "        v_mbcnt_lo_u32_b32 v55, s27, -v90\n" },
    { 0xd2480037U, 0x4002b41bU, true, "        v_mbcnt_hi_u32_b32 v55, s27, -v90\n" },
    // v_add_i32 - and others */
    { 0xd24a0737U, 0x4002b41bU, true, "        v_add_i32       v55, s[7:8], s27, -v90\n" },
    { 0xd24a6a37U, 0x0002b41bU, true, "        v_add_i32       v55, vcc, s27, v90 vop3\n" },
    { 0xd24a6a37U, 0x2002b41bU, true, "        v_add_i32       v55, vcc, -s27, v90\n" },
    { 0xd24c0737U, 0x4002b41bU, true, "        v_sub_i32       v55, s[7:8], s27, -v90\n" },
    { 0xd24e0737U, 0x4002b41bU, true, "        v_subrev_i32    v55, s[7:8], s27, -v90\n" },
    /* v_addcs */
    { 0xd2500737U, 0x4066b41bU, true, "        v_addc_u32      "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd2506a37U, 0x01aab41bU, true, "        v_addc_u32      "
                "v55, vcc, s27, v90, vcc vop3\n" },
    { 0xd2506a37U, 0x41aab41bU, true, "        v_addc_u32      "
                "v55, vcc, s27, -v90, vcc\n" },
    { 0xd2520737U, 0x4066b41bU, true, "        v_subb_u32      "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    { 0xd2540737U, 0x4066b41bU, true, "        v_subbrev_u32   "
                "v55, s[7:8], s27, -v90, s[25:26]\n" },
    /* others */
    { 0xd2560037U, 0x4002b41bU, true, "        v_ldexp_f32     v55, s27, -v90\n" },
    { 0xd2580037U, 0x4002b41bU, true, "        v_cvt_pkaccum_u8_f32 v55, s27, -v90\n" },
    { 0xd25a0037U, 0x4002b41bU, true, "        v_cvt_pknorm_i16_f32 v55, s27, -v90\n" },
    { 0xd25c0037U, 0x4002b41bU, true, "        v_cvt_pknorm_u16_f32 v55, s27, -v90\n" },
    { 0xd25e0037U, 0x4002b41bU, true, "        v_cvt_pkrtz_f16_f32 v55, s27, -v90\n" },
    { 0xd2600037U, 0x4002b41bU, true, "        v_cvt_pk_u16_u32 v55, s27, -v90\n" },
    { 0xd2620037U, 0x4002b41bU, true, "        v_cvt_pk_i16_i32 v55, s27, -v90\n" },
    { 0xd2640037U, 0x4002b41bU, true, "        VOP3A_ill_306   v55, s27, -v90, s0\n" },
    { 0xd2660037U, 0x4002b41bU, true, "        VOP3A_ill_307   v55, s27, -v90, s0\n" },
    { 0xd2680037U, 0x4002b41bU, true, "        VOP3A_ill_308   v55, s27, -v90, s0\n" },
    { 0xd26a0037U, 0x4002b41bU, true, "        VOP3A_ill_309   v55, s27, -v90, s0\n" },
    /* VOP1 instruction as VOP3 */
    { 0xd3000037U, 0x4002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a neg1\n" },
    { 0xd3000000U, 0x00000000U, true, "        v_nop           vop3\n" },
    { 0xd3000237U, 0x4002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a abs1 neg1\n" },
    { 0xd3000137U, 0x4002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b abs0 vsrc1=0x15a neg1\n" },
    { 0xd3000237U, 0xc01eb41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a abs1 neg1 vsrc2=0x7 neg2\n" },
    { 0xd3000037U, 0x0002b41bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vsrc1=0x15a\n" },
    { 0xd3000037U, 0x0000001bU, true, "        v_nop           "
                "dst=0x37 src0=0x1b vop3\n" },
    { 0xd3000837U, 0x0000001bU, true, "        v_nop           "
                "clamp dst=0x37 src0=0x1b\n" },
    /* v_mov_b32 and v_readfirstlane_b32 */
    { 0xd3020037U, 0x0000001bU, true, "        v_mov_b32       v55, s27 vop3\n" },
    { 0xd3020037U, 0x0000011bU, true, "        v_mov_b32       v55, v27 vop3\n" },
    { 0xd3020837U, 0x0000001bU, true, "        v_mov_b32       v55, s27 clamp\n" },
    { 0xd3020037U, 0x00001e1bU, true, "        v_mov_b32       v55, s27 vsrc1=0xf\n" },
    { 0xd3020037U, 0x003c001bU, true, "        v_mov_b32       v55, s27 vsrc2=0xf\n" },
    { 0xd3020737U, 0x003c001bU, true, "        v_mov_b32       "
                "v55, abs(s27) abs1 vsrc2=0xf abs2\n" },
    { 0xd3020037U, 0x1800001bU, true, "        v_mov_b32       v55, s27 div:2\n" },
    { 0xd3020037U, 0xf800001bU, true, "        v_mov_b32       "
                "v55, -s27 div:2 neg1 neg2\n" },
    { 0xd3020037U, 0x2000011bU, true, "        v_mov_b32       v55, -v27\n" },
    { 0xd3040037U, 0x2000001bU, true, "        v_readfirstlane_b32 s55, -s27\n" },
    { 0xd3040037U, 0x2000011bU, true, "        v_readfirstlane_b32 s55, -v27\n" },
    { 0xd3040037U, 0x0000001bU, true, "        v_readfirstlane_b32 s55, s27 vop3\n" },
    { 0xd3040037U, 0x0000011bU, true, "        v_readfirstlane_b32 s55, v27 vop3\n" },
    { 0xd3040837U, 0x0000001bU, true, "        v_readfirstlane_b32 s55, s27 clamp\n" },
    /* others */
    { 0xd3060037U, 0x0000001bU, true, "        v_cvt_i32_f64   v55, s[27:28] vop3\n" },
    { 0xd3060137U, 0x0000001bU, true, "        v_cvt_i32_f64   v55, abs(s[27:28])\n" },
    { 0xd3060037U, 0x0000011bU, true, "        v_cvt_i32_f64   v55, v[27:28] vop3\n" },
    { 0xd3060137U, 0x0000011bU, true, "        v_cvt_i32_f64   v55, abs(v[27:28])\n" },
    { 0xd3080037U, 0x0000011bU, true, "        v_cvt_f64_i32   v[55:56], v27 vop3\n" },
    { 0xd3080137U, 0x0000011bU, true, "        v_cvt_f64_i32   v[55:56], abs(v27)\n" },
    { 0xd30a0037U, 0x0000011bU, true, "        v_cvt_f32_i32   v55, v27 vop3\n" },
    { 0xd30c0037U, 0x0000011bU, true, "        v_cvt_f32_u32   v55, v27 vop3\n" },
    { 0xd30e0037U, 0x0000011bU, true, "        v_cvt_u32_f32   v55, v27 vop3\n" },
    { 0xd3100037U, 0x0000011bU, true, "        v_cvt_i32_f32   v55, v27 vop3\n" },
    { 0xd3120037U, 0x0000011bU, true, "        v_mov_fed_b32   v55, v27 vop3\n" },
    { 0xd3140037U, 0x0000011bU, true, "        v_cvt_f16_f32   v55, v27 vop3\n" },
    { 0xd3160037U, 0x0000011bU, true, "        v_cvt_f32_f16   v55, v27 vop3\n" },
    { 0xd3180037U, 0x0000011bU, true, "        v_cvt_rpi_i32_f32 v55, v27 vop3\n" },
    { 0xd31a0037U, 0x0000011bU, true, "        v_cvt_flr_i32_f32 v55, v27 vop3\n" },
    { 0xd31c0037U, 0x0000011bU, true, "        v_cvt_off_f32_i4 v55, v27 vop3\n" },
    { 0xd31e0037U, 0x0000011bU, true, "        v_cvt_f32_f64   v55, v[27:28] vop3\n" },
    { 0xd3200037U, 0x0000011bU, true, "        v_cvt_f64_f32   v[55:56], v27 vop3\n" },
    { 0xd3220037U, 0x0000011bU, true, "        v_cvt_f32_ubyte0 v55, v27 vop3\n" },
    { 0xd3240037U, 0x0000011bU, true, "        v_cvt_f32_ubyte1 v55, v27 vop3\n" },
    { 0xd3260037U, 0x0000011bU, true, "        v_cvt_f32_ubyte2 v55, v27 vop3\n" },
    { 0xd3280037U, 0x0000011bU, true, "        v_cvt_f32_ubyte3 v55, v27 vop3\n" },
    { 0xd32a0037U, 0x0000011bU, true, "        v_cvt_u32_f64   v55, v[27:28] vop3\n" },
    { 0xd32c0037U, 0x0000011bU, true, "        v_cvt_f64_u32   v[55:56], v27 vop3\n" },
    { 0xd32e0037U, 0x0000011bU, true, "        VOP3A_ill_407   v55, v27, s0, s0\n" },
    { 0xd3300037U, 0x0000011bU, true, "        VOP3A_ill_408   v55, v27, s0, s0\n" },
    { 0xd3320037U, 0x0000011bU, true, "        VOP3A_ill_409   v55, v27, s0, s0\n" },
    { 0xd3340037U, 0x0000011bU, true, "        VOP3A_ill_410   v55, v27, s0, s0\n" },
    { 0xd3360037U, 0x0000011bU, true, "        VOP3A_ill_411   v55, v27, s0, s0\n" },
    { 0xd3380037U, 0x0000011bU, true, "        VOP3A_ill_412   v55, v27, s0, s0\n" },
    { 0xd33a0037U, 0x0000011bU, true, "        VOP3A_ill_413   v55, v27, s0, s0\n" },
    { 0xd33c0037U, 0x0000011bU, true, "        VOP3A_ill_414   v55, v27, s0, s0\n" },
    { 0xd33e0037U, 0x0000011bU, true, "        VOP3A_ill_415   v55, v27, s0, s0\n" },
    { 0xd3400037U, 0x0000011bU, true, "        v_fract_f32     v55, v27 vop3\n" },
    { 0xd3420037U, 0x0000011bU, true, "        v_trunc_f32     v55, v27 vop3\n" },
    { 0xd3440037U, 0x0000011bU, true, "        v_ceil_f32      v55, v27 vop3\n" },
    { 0xd3460037U, 0x0000011bU, true, "        v_rndne_f32     v55, v27 vop3\n" },
    { 0xd3480037U, 0x0000011bU, true, "        v_floor_f32     v55, v27 vop3\n" },
    { 0xd34a0037U, 0x0000011bU, true, "        v_exp_f32       v55, v27 vop3\n" },
    { 0xd34c0037U, 0x0000011bU, true, "        v_log_clamp_f32 v55, v27 vop3\n" },
    { 0xd34e0037U, 0x0000011bU, true, "        v_log_f32       v55, v27 vop3\n" },
    { 0xd3500037U, 0x0000011bU, true, "        v_rcp_clamp_f32 v55, v27 vop3\n" },
    { 0xd3520037U, 0x0000011bU, true, "        v_rcp_legacy_f32 v55, v27 vop3\n" },
    { 0xd3540037U, 0x0000011bU, true, "        v_rcp_f32       v55, v27 vop3\n" },
    { 0xd3560037U, 0x0000011bU, true, "        v_rcp_iflag_f32 v55, v27 vop3\n" },
    { 0xd3580037U, 0x0000011bU, true, "        v_rsq_clamp_f32 v55, v27 vop3\n" },
    { 0xd35a0037U, 0x0000011bU, true, "        v_rsq_legacy_f32 v55, v27 vop3\n" },
    { 0xd35c0037U, 0x0000011bU, true, "        v_rsq_f32       v55, v27 vop3\n" },
    { 0xd35e0037U, 0x0000011bU, true, "        v_rcp_f64       v[55:56], v[27:28] vop3\n" },
    { 0xd3600037U, 0x0000011bU, true, "        v_rcp_clamp_f64 v[55:56], v[27:28] vop3\n" },
    { 0xd3620037U, 0x0000011bU, true, "        v_rsq_f64       v[55:56], v[27:28] vop3\n" },
    { 0xd3640037U, 0x0000011bU, true, "        v_rsq_clamp_f64 v[55:56], v[27:28] vop3\n" },
    { 0xd3660037U, 0x0000011bU, true, "        v_sqrt_f32      v55, v27 vop3\n" },
    { 0xd3680037U, 0x0000011bU, true, "        v_sqrt_f64      v[55:56], v[27:28] vop3\n" },
    { 0xd36a0037U, 0x0000011bU, true, "        v_sin_f32       v55, v27 vop3\n" },
    { 0xd36c0037U, 0x0000011bU, true, "        v_cos_f32       v55, v27 vop3\n" },
    { 0xd36e0037U, 0x0000011bU, true, "        v_not_b32       v55, v27 vop3\n" },
    { 0xd3700037U, 0x0000011bU, true, "        v_bfrev_b32     v55, v27 vop3\n" },
    { 0xd3720037U, 0x0000011bU, true, "        v_ffbh_u32      v55, v27 vop3\n" },
    { 0xd3740037U, 0x0000011bU, true, "        v_ffbl_b32      v55, v27 vop3\n" },
    { 0xd3760037U, 0x0000011bU, true, "        v_ffbh_i32      v55, v27 vop3\n" },
    { 0xd3780037U, 0x0000011bU, true, "        v_frexp_exp_i32_f64 v55, v[27:28] vop3\n" },
    { 0xd37a0037U, 0x0000011bU, true, "        v_frexp_mant_f64 v[55:56], v[27:28] vop3\n" },
    { 0xd37c0037U, 0x0000011bU, true, "        v_fract_f64     v[55:56], v[27:28] vop3\n" },
    { 0xd37e0037U, 0x0000011bU, true, "        v_frexp_exp_i32_f32 v55, v27 vop3\n" },
    { 0xd3800037U, 0x0000011bU, true, "        v_frexp_mant_f32 v55, v27 vop3\n" },
    { 0xd3820037U, 0x0000011bU, true, "        v_clrexcp       "
                "dst=0x37 src0=0x11b vop3\n" },
    { 0xd3820000U, 0x00000000U, true, "        v_clrexcp       vop3\n" },
    { 0xd3840037U, 0x0000011bU, true, "        v_movreld_b32   v55, v27 vop3\n" },
    { 0xd3860037U, 0x0000011bU, true, "        v_movrels_b32   v55, v27 vop3\n" },
    { 0xd3880037U, 0x0000011bU, true, "        v_movrelsd_b32  v55, v27 vop3\n" },
    { 0xd38a0037U, 0x0000011bU, true, "        VOP3A_ill_453   v55, v27, s0, s0\n" },
    { 0xd38c0037U, 0x0000011bU, true, "        VOP3A_ill_454   v55, v27, s0, s0\n" },
    { 0xd38e0037U, 0x0000011bU, true, "        VOP3A_ill_455   v55, v27, s0, s0\n" },
    { 0xd3900037U, 0x0000011bU, true, "        VOP3A_ill_456   v55, v27, s0, s0\n" },
    { 0xd3920037U, 0x0000011bU, true, "        VOP3A_ill_457   v55, v27, s0, s0\n" },
    { 0xd3940037U, 0x0000011bU, true, "        VOP3A_ill_458   v55, v27, s0, s0\n" },
    { 0xd3960037U, 0x0000011bU, true, "        VOP3A_ill_459   v55, v27, s0, s0\n" },
    /**** VOP3a instructions ***/
    { 0xd2800037U, 0x07974d4fU, true, "        v_mad_legacy_f32 v55, v79, v166, v229\n" },
    { 0xd2800c37U, 0x07974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, v166, abs(v229) clamp\n" },
    { 0xd2800c37U, 0x87974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, v166, -abs(v229) clamp\n" },
    { 0xd2800c37U, 0x97974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, v166, -abs(v229) mul:4 clamp\n" },
    { 0xd2800e37U, 0x47974d4fU, true, "        v_mad_legacy_f32 "
                "v55, v79, -abs(v166), abs(v229) clamp\n" },
    { 0xd2800d37U, 0xb7974d4fU, true, "        v_mad_legacy_f32 "
                "v55, -abs(v79), v166, -abs(v229) mul:4 clamp\n" },
    { 0xd2800d37U, 0xb7974d4fU, true, "        v_mad_legacy_f32 "
                "v55, -abs(v79), v166, -abs(v229) mul:4 clamp\n" },
    { 0xd2800037U, 0x03954c4fU, true, "        v_mad_legacy_f32 v55, s79, 38, ill_229\n" },
    { 0xd2820037U, 0x07974d4fU, true, "        v_mad_f32       v55, v79, v166, v229\n" },
    { 0xd2840037U, 0x07974d4fU, true, "        v_mad_i32_i24   v55, v79, v166, v229\n" },
    { 0xd2860037U, 0x07974d4fU, true, "        v_mad_u32_u24   v55, v79, v166, v229\n" },
    { 0xd2880037U, 0x07974d4fU, true, "        v_cubeid_f32    v55, v79, v166, v229\n" },
    { 0xd28a0037U, 0x07974d4fU, true, "        v_cubesc_f32    v55, v79, v166, v229\n" },
    { 0xd28c0037U, 0x07974d4fU, true, "        v_cubetc_f32    v55, v79, v166, v229\n" },
    { 0xd28e0037U, 0x07974d4fU, true, "        v_cubema_f32    v55, v79, v166, v229\n" },
    { 0xd2900037U, 0x07974d4fU, true, "        v_bfe_u32       v55, v79, v166, v229\n" },
    { 0xd2920037U, 0x07974d4fU, true, "        v_bfe_i32       v55, v79, v166, v229\n" },
    { 0xd2940037U, 0x07974d4fU, true, "        v_bfi_b32       v55, v79, v166, v229\n" },
    { 0xd2960037U, 0x07974d4fU, true, "        v_fma_f32       v55, v79, v166, v229\n" },
    { 0xd2980037U, 0x07974d4fU, true, "        v_fma_f64       "
                "v[55:56], v[79:80], v[166:167], v[229:230]\n" },
    { 0xd29a0037U, 0x07974d4fU, true, "        v_lerp_u8       v55, v79, v166, v229\n" },
    { 0xd29c0037U, 0x07974d4fU, true, "        v_alignbit_b32  v55, v79, v166, v229\n" },
    { 0xd29e0037U, 0x07974d4fU, true, "        v_alignbyte_b32 v55, v79, v166, v229\n" },
    { 0xd2a00037U, 0x07974d4fU, true, "        v_mullit_f32    "
                "v55, v79, v166 vsrc2=0x1e5\n" },
    { 0xd2a00037U, 0x00034d4fU, true, "        v_mullit_f32    "
                "v55, v79, v166\n" },
    { 0xd2a20037U, 0x07974d4fU, true, "        v_min3_f32      v55, v79, v166, v229\n" },
    { 0xd2a40037U, 0x07974d4fU, true, "        v_min3_i32      v55, v79, v166, v229\n" },
    { 0xd2a60037U, 0x07974d4fU, true, "        v_min3_u32      v55, v79, v166, v229\n" },
    { 0xd2a80037U, 0x07974d4fU, true, "        v_max3_f32      v55, v79, v166, v229\n" },
    { 0xd2aa0037U, 0x07974d4fU, true, "        v_max3_i32      v55, v79, v166, v229\n" },
    { 0xd2ac0037U, 0x07974d4fU, true, "        v_max3_u32      v55, v79, v166, v229\n" },
    { 0xd2ae0037U, 0x07974d4fU, true, "        v_med3_f32      v55, v79, v166, v229\n" },
    { 0xd2b00037U, 0x07974d4fU, true, "        v_med3_i32      v55, v79, v166, v229\n" },
    { 0xd2b20037U, 0x07974d4fU, true, "        v_med3_u32      v55, v79, v166, v229\n" },
    { 0xd2b40037U, 0x07974d4fU, true, "        v_sad_u8        v55, v79, v166, v229\n" },
    { 0xd2b60037U, 0x07974d4fU, true, "        v_sad_hi_u8     v55, v79, v166, v229\n" },
    { 0xd2b80037U, 0x07974d4fU, true, "        v_sad_u16       v55, v79, v166, v229\n" },
    { 0xd2ba0037U, 0x07974d4fU, true, "        v_sad_u32       v55, v79, v166, v229\n" },
    { 0xd2bc0037U, 0x07974d4fU, true, "        v_cvt_pk_u8_f32 v55, v79, v166, v229\n" },
    { 0xd2be0037U, 0x07974d4fU, true, "        v_div_fixup_f32 v55, v79, v166, v229\n" },
    { 0xd2c00037U, 0x07974d4fU, true, "        v_div_fixup_f64 "
                "v[55:56], v[79:80], v[166:167], v[229:230]\n" },
    { 0xd2c20037U, 0x07974d4fU, true, "        v_lshl_b64      "
                "v[55:56], v[79:80], v166 vsrc2=0x1e5\n" },
    { 0xd2c20037U, 0x00034d4fU, true, "        v_lshl_b64      "
                "v[55:56], v[79:80], v166\n" },
    { 0xd2c40037U, 0x07974d4fU, true, "        v_lshr_b64      "
                "v[55:56], v[79:80], v166 vsrc2=0x1e5\n" },
    { 0xd2c40037U, 0x00034d4fU, true, "        v_lshr_b64      "
                "v[55:56], v[79:80], v166\n" },
    { 0xd2c60037U, 0x07974d4fU, true, "        v_ashr_i64      "
                "v[55:56], v[79:80], v166 vsrc2=0x1e5\n" },
    { 0xd2c60037U, 0x00034d4fU, true, "        v_ashr_i64      "
                "v[55:56], v[79:80], v166\n" },
    { 0xd2c80037U, 0x00034d4fU, true, "        v_add_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2ca0037U, 0x00034d4fU, true, "        v_mul_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2cc0037U, 0x00034d4fU, true, "        v_min_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2ce0037U, 0x00034d4fU, true, "        v_max_f64       "
                "v[55:56], v[79:80], v[166:167]\n" },
    { 0xd2d00037U, 0x00034d4fU, true, "        v_ldexp_f64     "
                "v[55:56], v[79:80], v166\n" },
    { 0xd2d20037U, 0x00034d4fU, true, "        v_mul_lo_u32    v55, v79, v166\n" },
    { 0xd2d40037U, 0x00034d4fU, true, "        v_mul_hi_u32    v55, v79, v166\n" },
    { 0xd2d60037U, 0x00034d4fU, true, "        v_mul_lo_i32    v55, v79, v166\n" },
    { 0xd2d80037U, 0x00034d4fU, true, "        v_mul_hi_i32    v55, v79, v166\n" },
    /* VOP3b */
    { 0xd2da2537U, 0x07974d4fU, true, "        v_div_scale_f32 "
                "v55, s[37:38], v79, v166, v229\n" },
    { 0xd2da6a37U, 0x00034d4fU, true, "        v_div_scale_f32 "
                "v55, vcc, v79, v166, s0\n" },
    { 0xd2dc2537U, 0x07974d4fU, true, "        v_div_scale_f64 "
                "v[55:56], s[37:38], v[79:80], v[166:167], v[229:230]\n" },
    /* rest of VOP3a */
    { 0xd2de0037U, 0x07974d4fU, true, "        v_div_fmas_f32  v55, v79, v166, v229\n" },
    { 0xd2e00037U, 0x07974d4fU, true, "        v_div_fmas_f64  "
                "v[55:56], v[79:80], v[166:167], v[229:230]\n" },
    { 0xd2e20037U, 0x07974d4fU, true, "        v_msad_u8       v55, v79, v166, v229\n" },
    { 0xd2e40037U, 0x07974d4fU, true, "        v_qsad_u8       "
                "v[55:56], v[79:80], v166, v[229:230]\n" },
    { 0xd2e60037U, 0x07974d4fU, true, "        v_mqsad_u8      "
                "v[55:56], v[79:80], v166, v[229:230]\n" },
    { 0xd2e80037U, 0x07974d4fU, true, "        v_trig_preop_f64 "
                "v[55:56], v[79:80], v166 vsrc2=0x1e5\n" },
    { 0xd2ea0037U, 0x0000011bU, true, "        VOP3A_ill_373   v55, v27, s0, s0\n" },
    { 0xd2ec0037U, 0x0000011bU, true, "        VOP3A_ill_374   v55, v27, s0, s0\n" },
    { 0xd2ee0037U, 0x0000011bU, true, "        VOP3A_ill_375   v55, v27, s0, s0\n" },
    /* VINTRP */
    { 0xc9746bd3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.w\n" },
    { 0xc9746ad3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.z\n" },
    { 0xc97469d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.y\n" },
    { 0xc97468d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr26.x\n" },
    { 0xc97400d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr0.x\n" },
    { 0xc97428d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr10.x\n" },
    { 0xc9743cd3U, 0, false, "        v_interp_p1_f32 v93, v211, attr15.x\n" },
    { 0xc9756bd3U, 0, false, "        v_interp_p2_f32 v93, v211, attr26.w\n" },
    { 0xc9766bd3U, 0, false, "        v_interp_mov_f32 v93, v211, attr26.w\n" },
    { 0xc974f4d3U, 0, false, "        v_interp_p1_f32 v93, v211, attr61.x\n" },
    { 0xc9776bd3U, 0, false, "        VINTRP_ill_3    v93, v211, attr26.w\n" },
    /* DS encoding */
    { 0xd800cd67U, 0x8b27a947U, true, "        ds_add_u32      "
                "v71, v169 offset:52583 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8000000U, 0x8b27a947U, true, "        ds_add_u32      "
                "v71, v169 vdata1=0x27 vdst=0x8b\n" },
    { 0xd800cd67U, 0x0000a947U, true, "        ds_add_u32      v71, v169 offset:52583\n" },
    { 0xd802cd67U, 0x8b27a947U, true, "        ds_add_u32      "
                "v71, v169 offset:52583 gds vdata1=0x27 vdst=0x8b\n" },
    { 0xd802cd67U, 0x0000a947U, true, "        ds_add_u32      "
                "v71, v169 offset:52583 gds\n" },
    { 0xd804cd67U, 0x0000a947U, true, "        ds_sub_u32      v71, v169 offset:52583\n" },
    { 0xd808cd67U, 0x0000a947U, true, "        ds_rsub_u32     v71, v169 offset:52583\n" },
    { 0xd80ccd67U, 0x0000a947U, true, "        ds_inc_u32      v71, v169 offset:52583\n" },
    { 0xd810cd67U, 0x0000a947U, true, "        ds_dec_u32      v71, v169 offset:52583\n" },
    { 0xd814cd67U, 0x0000a947U, true, "        ds_min_i32      v71, v169 offset:52583\n" },
    { 0xd818cd67U, 0x0000a947U, true, "        ds_max_i32      v71, v169 offset:52583\n" },
    { 0xd81ccd67U, 0x0000a947U, true, "        ds_min_u32      v71, v169 offset:52583\n" },
    { 0xd820cd67U, 0x0000a947U, true, "        ds_max_u32      v71, v169 offset:52583\n" },
    { 0xd824cd67U, 0x0000a947U, true, "        ds_and_b32      v71, v169 offset:52583\n" },
    { 0xd828cd67U, 0x0000a947U, true, "        ds_or_b32       v71, v169 offset:52583\n" },
    { 0xd82ccd67U, 0x0000a947U, true, "        ds_xor_b32      v71, v169 offset:52583\n" },
    { 0xd830cd67U, 0x0027a947U, true, "        ds_mskor_b32    "
                "v71, v169, v39 offset:52583\n" },
    { 0xd830cd67U, 0x8b27a947U, true, "        ds_mskor_b32    "
                "v71, v169, v39 offset:52583 vdst=0x8b\n" },
    { 0xd834cd67U, 0x0000a947U, true, "        ds_write_b32    v71, v169 offset:52583\n" },
    { 0xd838cd67U, 0x0027a947U, true, "        ds_write2_b32   "
                "v71, v169, v39 offset0:103 offset1:205\n" },
    { 0xd8380067U, 0x0027a947U, true, "        ds_write2_b32   "
                "v71, v169, v39 offset0:103\n" },
    { 0xd8381100U, 0x0027a947U, true, "        ds_write2_b32   "
                "v71, v169, v39 offset1:17\n" },
    { 0xd8380000U, 0x0027a947U, true, "        ds_write2_b32   v71, v169, v39\n" },
    { 0xd83ccd67U, 0x0027a947U, true, "        ds_write2st64_b32 "
                "v71, v169, v39 offset0:103 offset1:205\n" },
    { 0xd840cd67U, 0x0027a947U, true, "        ds_cmpst_b32    "
                "v71, v169, v39 offset:52583\n" },
    { 0xd844cd67U, 0x0027a947U, true, "        ds_cmpst_f32    "
                "v71, v169, v39 offset:52583\n" },
    { 0xd848cd67U, 0x0000a947U, true, "        ds_min_f32      v71, v169 offset:52583\n" }, 
    { 0xd84ccd67U, 0x0000a947U, true, "        ds_max_f32      v71, v169 offset:52583\n" }, 
    { 0xd850cd67U, 0x8b27a947U, true, "        DS_ill_20       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd854cd67U, 0x8b27a947U, true, "        DS_ill_21       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd858cd67U, 0x8b27a947U, true, "        DS_ill_22       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd85ccd67U, 0x8b27a947U, true, "        DS_ill_23       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd860cd67U, 0x8b27a947U, true, "        DS_ill_24       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd864cd67U, 0x8b27a947U, true, "        ds_gws_init     "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd864cd67U, 0x8b000047U, true, "        ds_gws_init     "
                "v71 offset:52583 vdst=0x8b\n" },
    { 0xd866cd67U, 0x8b000047U, true, "        ds_gws_init     "
                "v71 offset:52583 gds vdst=0x8b\n" },
    { 0xd8660000U, 0x8b000047U, true, "        ds_gws_init     v71 gds vdst=0x8b\n" },
    { 0xd866cd67U, 0x00000047U, true, "        ds_gws_init     v71 offset:52583 gds\n" },
    { 0xd86acd67U, 0x00000047U, true, "        ds_gws_sema_v   v71 offset:52583 gds\n" },
    { 0xd86ecd67U, 0x00000047U, true, "        ds_gws_sema_br  v71 offset:52583 gds\n" },
    { 0xd872cd67U, 0x00000047U, true, "        ds_gws_sema_p   v71 offset:52583 gds\n" },
    { 0xd876cd67U, 0x00000047U, true, "        ds_gws_barrier  v71 offset:52583 gds\n" },
    { 0xd878cd67U, 0x0000a947U, true, "        ds_write_b8     v71, v169 offset:52583\n" },
    { 0xd87ccd67U, 0x0000a947U, true, "        ds_write_b16    v71, v169 offset:52583\n" },
    { 0xd880cd67U, 0x9b00a947U, true, "        ds_add_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd880cd67U, 0x9b05a947U, true, "        ds_add_rtn_u32  "
                "v155, v71, v169 offset:52583 vdata1=0x5\n" },
    { 0xd884cd67U, 0x9b00a947U, true, "        ds_sub_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd888cd67U, 0x9b00a947U, true, "        ds_rsub_rtn_u32 "
                "v155, v71, v169 offset:52583\n" },
    { 0xd88ccd67U, 0x9b00a947U, true, "        ds_inc_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd890cd67U, 0x9b00a947U, true, "        ds_dec_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd894cd67U, 0x9b00a947U, true, "        ds_min_rtn_i32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd898cd67U, 0x9b00a947U, true, "        ds_max_rtn_i32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd89ccd67U, 0x9b00a947U, true, "        ds_min_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8a0cd67U, 0x9b00a947U, true, "        ds_max_rtn_u32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8a4cd67U, 0x9b00a947U, true, "        ds_and_rtn_b32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8a8cd67U, 0x9b00a947U, true, "        ds_or_rtn_b32   "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8accd67U, 0x9b00a947U, true, "        ds_xor_rtn_b32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8b0cd67U, 0x9b1aa947U, true, "        ds_mskor_rtn_b32 "
                "v155, v71, v169, v26 offset:52583\n" },
    { 0xd8b4cd67U, 0x9b00a947U, true, "        ds_wrxchg_rtn_b32 "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8b8cd67U, 0x9b1aa947U, true, "        ds_wrxchg2_rtn_b32 "
                "v[155:156], v71, v169, v26 offset0:103 offset1:205\n" },
    { 0xd8bccd67U, 0x9b1aa947U, true, "        ds_wrxchg2st64_rtn_b32 "
                "v[155:156], v71, v169, v26 offset0:103 offset1:205\n" },
    { 0xd8c0cd67U, 0x9b1aa947U, true, "        ds_cmpst_rtn_b32 "
                "v155, v71, v169, v26 offset:52583\n" },
    { 0xd8c4cd67U, 0x9b1aa947U, true, "        ds_cmpst_rtn_f32 "
                "v155, v71, v169, v26 offset:52583\n" },
    { 0xd8c8cd67U, 0x9b00a947U, true, "        ds_min_rtn_f32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8cccd67U, 0x9b00a947U, true, "        ds_max_rtn_f32  "
                "v155, v71, v169 offset:52583\n" },
    { 0xd8d0cd67U, 0x8b27a947U, true, "        DS_ill_52       "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd8d4cd67U, 0x9b00a947U, true, "        ds_swizzle_b32  "
                "v155, v71 offset:52583 vdata0=0xa9\n" }, /* check */
    { 0xd8d4cd67U, 0x9b000047U, true, "        ds_swizzle_b32  "
                "v155, v71 offset:52583\n" }, /* check */
    { 0xd8d8cd67U, 0x9b000047U, true, "        ds_read_b32     v155, v71 offset:52583\n" },
    { 0xd8dccd67U, 0x9b000047U, true, "        ds_read2_b32    "
                "v[155:156], v71 offset0:103 offset1:205\n" },
    { 0xd8e0cd67U, 0x9b000047U, true, "        ds_read2st64_b32 "
                "v[155:156], v71 offset0:103 offset1:205\n" },
    { 0xd8e4cd67U, 0x9b000047U, true, "        ds_read_i8      v155, v71 offset:52583\n" },
    { 0xd8e8cd67U, 0x9b000047U, true, "        ds_read_u8      v155, v71 offset:52583\n" },
    { 0xd8eccd67U, 0x9b000047U, true, "        ds_read_i16     v155, v71 offset:52583\n" },
    { 0xd8f0cd67U, 0x9b000047U, true, "        ds_read_u16     v155, v71 offset:52583\n" },
    { 0xd8f4cd67U, 0x9b000047U, true, "        ds_consume      "
                "v155 offset:52583 vaddr=0x47\n" },
    { 0xd8f4cd67U, 0x9b000000U, true, "        ds_consume      v155 offset:52583\n" },
    { 0xd8f8cd67U, 0x9b000047U, true, "        ds_append       "
                "v155 offset:52583 vaddr=0x47\n" },
    { 0xd8f8cd67U, 0x9b000000U, true, "        ds_append       v155 offset:52583\n" },
    { 0xd8fccd67U, 0x9b000047U, true, "        ds_ordered_count v155, v71 offset:52583\n" },
    { 0xd900cd67U, 0x0000a947U, true, "        ds_add_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd904cd67U, 0x0000a947U, true, "        ds_sub_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd908cd67U, 0x0000a947U, true, "        ds_rsub_u64     "
                "v71, v[169:170] offset:52583\n" },
    { 0xd90ccd67U, 0x0000a947U, true, "        ds_inc_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd910cd67U, 0x0000a947U, true, "        ds_dec_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd914cd67U, 0x0000a947U, true, "        ds_min_i64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd918cd67U, 0x0000a947U, true, "        ds_max_i64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd91ccd67U, 0x0000a947U, true, "        ds_min_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd920cd67U, 0x0000a947U, true, "        ds_max_u64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd924cd67U, 0x0000a947U, true, "        ds_and_b64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd928cd67U, 0x0000a947U, true, "        ds_or_b64       "
                "v71, v[169:170] offset:52583\n" },
    { 0xd92ccd67U, 0x0000a947U, true, "        ds_xor_b64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd930cd67U, 0x0017a947U, true, "        ds_mskor_b64    "
                "v71, v[169:170], v[23:24] offset:52583\n" },
    { 0xd934cd67U, 0x0000a947U, true, "        ds_write_b64    "
                "v71, v[169:170] offset:52583\n" },
    { 0xd938cd67U, 0x0027a947U, true, "        ds_write2_b64   "
                "v71, v[169:170], v[39:40] offset0:103 offset1:205\n" },
    { 0xd93ccd67U, 0x0027a947U, true, "        ds_write2st64_b64 "
                "v71, v[169:170], v[39:40] offset0:103 offset1:205\n" },
    { 0xd940cd67U, 0x0027a947U, true, "        ds_cmpst_b64    "
                "v71, v[169:170], v[39:40] offset:52583\n" },
    { 0xd944cd67U, 0x0027a947U, true, "        ds_cmpst_f64    "
                "v71, v[169:170], v[39:40] offset:52583\n" },
    { 0xd948cd67U, 0x0000a947U, true, "        ds_min_f64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd94ccd67U, 0x0000a947U, true, "        ds_max_f64      "
                "v71, v[169:170] offset:52583\n" },
    { 0xd950cd67U, 0x0000a947U, true, "        DS_ill_84       "
                "v71 offset:52583 vdata0=0xa9\n" },
    { 0xd954cd67U, 0x0000a947U, true, "        DS_ill_85       "
                "v71 offset:52583 vdata0=0xa9\n" },
    { 0xd958cd67U, 0x0000a947U, true, "        DS_ill_86       "
                "v71 offset:52583 vdata0=0xa9\n" },
    { 0xd95ccd67U, 0x0000a947U, true, "        DS_ill_87       "
                "v71 offset:52583 vdata0=0xa9\n" },
    { 0xd960cd67U, 0x0000a947U, true, "        DS_ill_88       "
                "v71 offset:52583 vdata0=0xa9\n" },
    { 0xd964cd67U, 0x0000a947U, true, "        DS_ill_89       "
                "v71 offset:52583 vdata0=0xa9\n" },
    { 0xd980cd67U, 0x8b00a947U, true, "        ds_add_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd980cd67U, 0x8b11a947U, true, "        ds_add_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583 vdata1=0x11\n" },
    { 0xd984cd67U, 0x8b00a947U, true, "        ds_sub_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd988cd67U, 0x8b00a947U, true, "        ds_rsub_rtn_u64 "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd98ccd67U, 0x8b00a947U, true, "        ds_inc_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd990cd67U, 0x8b00a947U, true, "        ds_dec_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd994cd67U, 0x8b00a947U, true, "        ds_min_rtn_i64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd998cd67U, 0x8b00a947U, true, "        ds_max_rtn_i64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd99ccd67U, 0x8b00a947U, true, "        ds_min_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9a0cd67U, 0x8b00a947U, true, "        ds_max_rtn_u64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9a4cd67U, 0x8b00a947U, true, "        ds_and_rtn_b64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9a8cd67U, 0x8b00a947U, true, "        ds_or_rtn_b64   "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9accd67U, 0x8b00a947U, true, "        ds_xor_rtn_b64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9b0cd67U, 0x8b56a947U, true, "        ds_mskor_rtn_b64 "
                "v[139:140], v71, v[169:170], v[86:87] offset:52583\n" },
    { 0xd9b4cd67U, 0x8b00a947U, true, "        ds_wrxchg_rtn_b64 "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9b8cd67U, 0x8b56a947U, true, "        ds_wrxchg2_rtn_b64 "
                "v[139:140], v71, v[169:170], v[86:87] offset0:103 offset1:205\n" },
    { 0xd9bccd67U, 0x8b56a947U, true, "        ds_wrxchg2st64_rtn_b64 "
                "v[139:140], v71, v[169:170], v[86:87] offset0:103 offset1:205\n" },
    { 0xd9c0cd67U, 0x8b56a947U, true, "        ds_cmpst_rtn_b64 "
                "v[139:140], v71, v[169:170], v[86:87] offset:52583\n" },
    { 0xd9c4cd67U, 0x8b56a947U, true, "        ds_cmpst_rtn_f64 "
                "v[139:140], v71, v[169:170], v[86:87] offset:52583\n" },
    { 0xd9c8cd67U, 0x8b00a947U, true, "        ds_min_rtn_f64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9cccd67U, 0x8b00a947U, true, "        ds_max_rtn_f64  "
                "v[139:140], v71, v[169:170] offset:52583\n" },
    { 0xd9d0cd67U, 0x8b27a947U, true, "        DS_ill_116      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9d4cd67U, 0x8b27a947U, true, "        DS_ill_117      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9d8cd67U, 0x8b000047U, true, "        ds_read_b64     "
                "v[139:140], v71 offset:52583\n" },
    { 0xd9dccd67U, 0x8b000047U, true, "        ds_read2_b64    "
                "v[139:140], v71 offset0:103 offset1:205\n" },
    { 0xd9e0cd67U, 0x8b000047U, true, "        ds_read2st64_b64 "
                "v[139:140], v71 offset0:103 offset1:205\n" },
    { 0xd9e4cd67U, 0x8b27a947U, true, "        DS_ill_121      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9e8cd67U, 0x8b27a947U, true, "        DS_ill_122      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9eccd67U, 0x8b27a947U, true, "        DS_ill_123      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9f0cd67U, 0x8b27a947U, true, "        DS_ill_124      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9f4cd67U, 0x8b27a947U, true, "        DS_ill_125      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9f8cd67U, 0x8b27a947U, true, "        DS_ill_126      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xd9fccd67U, 0x8b27a947U, true, "        DS_ill_127      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda00cd67U, 0x8b00a947U, true, "        ds_add_src2_u32 "
                "v71 offset:52583 vdata0=0xa9 vdst=0x8b\n" },
    { 0xda00cd67U, 0x00000047U, true, "        ds_add_src2_u32 v71 offset:52583\n" },
    { 0xda04cd67U, 0x00000047U, true, "        ds_sub_src2_u32 v71 offset:52583\n" },
    { 0xda08cd67U, 0x00000047U, true, "        ds_rsub_src2_u32 v71 offset:52583\n" },
    { 0xda0ccd67U, 0x00000047U, true, "        ds_inc_src2_u32 v71 offset:52583\n" },
    { 0xda10cd67U, 0x00000047U, true, "        ds_dec_src2_u32 v71 offset:52583\n" },
    { 0xda14cd67U, 0x00000047U, true, "        ds_min_src2_i32 v71 offset:52583\n" },
    { 0xda18cd67U, 0x00000047U, true, "        ds_max_src2_i32 v71 offset:52583\n" },
    { 0xda1ccd67U, 0x00000047U, true, "        ds_min_src2_u32 v71 offset:52583\n" },
    { 0xda20cd67U, 0x00000047U, true, "        ds_max_src2_u32 v71 offset:52583\n" },
    { 0xda24cd67U, 0x00000047U, true, "        ds_and_src2_b32 v71 offset:52583\n" },
    { 0xda28cd67U, 0x00000047U, true, "        ds_or_src2_b32  v71 offset:52583\n" },
    { 0xda2ccd67U, 0x00000047U, true, "        ds_xor_src2_b32 v71 offset:52583\n" },
    { 0xda30cd67U, 0x00000047U, true, "        ds_write_src2_b32 v71 offset:52583\n" },
    { 0xda34cd67U, 0x8b27a947U, true, "        DS_ill_141      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda38cd67U, 0x8b27a947U, true, "        DS_ill_142      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda3ccd67U, 0x8b27a947U, true, "        DS_ill_143      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda40cd67U, 0x8b27a947U, true, "        DS_ill_144      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda44cd67U, 0x8b27a947U, true, "        DS_ill_145      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda48cd67U, 0x00000047U, true, "        ds_min_src2_f32 v71 offset:52583\n" },
    { 0xda4ccd67U, 0x00000047U, true, "        ds_max_src2_f32 v71 offset:52583\n" },
    { 0xda50cd67U, 0x8b27a947U, true, "        DS_ill_148      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda54cd67U, 0x8b27a947U, true, "        DS_ill_149      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda58cd67U, 0x8b27a947U, true, "        DS_ill_150      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xda5ccd67U, 0x8b27a947U, true, "        DS_ill_151      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb00cd67U, 0x00000047U, true, "        ds_add_src2_u64 v71 offset:52583\n" },
    { 0xdb04cd67U, 0x00000047U, true, "        ds_sub_src2_u64 v71 offset:52583\n" },
    { 0xdb08cd67U, 0x00000047U, true, "        ds_rsub_src2_u64 v71 offset:52583\n" },
    { 0xdb0ccd67U, 0x00000047U, true, "        ds_inc_src2_u64 v71 offset:52583\n" },
    { 0xdb10cd67U, 0x00000047U, true, "        ds_dec_src2_u64 v71 offset:52583\n" },
    { 0xdb14cd67U, 0x00000047U, true, "        ds_min_src2_i64 v71 offset:52583\n" },
    { 0xdb18cd67U, 0x00000047U, true, "        ds_max_src2_i64 v71 offset:52583\n" },
    { 0xdb1ccd67U, 0x00000047U, true, "        ds_min_src2_u64 v71 offset:52583\n" },
    { 0xdb20cd67U, 0x00000047U, true, "        ds_max_src2_u64 v71 offset:52583\n" },
    { 0xdb24cd67U, 0x00000047U, true, "        ds_and_src2_b64 v71 offset:52583\n" },
    { 0xdb28cd67U, 0x00000047U, true, "        ds_or_src2_b64  v71 offset:52583\n" },
    { 0xdb2ccd67U, 0x00000047U, true, "        ds_xor_src2_b64 v71 offset:52583\n" },
    { 0xdb30cd67U, 0x00000047U, true, "        ds_write_src2_b64 v71 offset:52583\n" },
    { 0xdb34cd67U, 0x8b27a947U, true, "        DS_ill_205      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb38cd67U, 0x8b27a947U, true, "        DS_ill_206      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb3ccd67U, 0x8b27a947U, true, "        DS_ill_207      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb40cd67U, 0x8b27a947U, true, "        DS_ill_208      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb44cd67U, 0x8b27a947U, true, "        DS_ill_209      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb48cd67U, 0x00000047U, true, "        ds_min_src2_f64 v71 offset:52583\n" },
    { 0xdb4ccd67U, 0x00000047U, true, "        ds_max_src2_f64 v71 offset:52583\n" },
    { 0xdb50cd67U, 0x8b27a947U, true, "        DS_ill_212      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb54cd67U, 0x8b27a947U, true, "        DS_ill_213      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb58cd67U, 0x8b27a947U, true, "        DS_ill_214      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb5ccd67U, 0x8b27a947U, true, "        DS_ill_215      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb60cd67U, 0x8b27a947U, true, "        DS_ill_216      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb64cd67U, 0x8b27a947U, true, "        DS_ill_217      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb68cd67U, 0x8b27a947U, true, "        DS_ill_218      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    { 0xdb6ccd67U, 0x8b27a947U, true, "        DS_ill_219      "
                "v71 offset:52583 vdata0=0xa9 vdata1=0x27 vdst=0x8b\n" },
    /* MUBUF encoding */
    { 0xe001f25bU, 0x23f43d12U, true, "        buffer_load_format_x "
    "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:603 glc slc addr64 lds tfe\n" },
    /* vaddr sizing */
    { 0xe001c25bU, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offset:603 glc slc addr64 lds tfe\n" },
    { 0xe001e25bU, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 idxen offset:603 glc slc addr64 lds tfe\n" },
    { 0xe001d25bU, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen offset:603 glc slc addr64 lds tfe\n" },
    { 0xe001725bU, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds tfe\n" },
    { 0xe001425bU, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], s35 offset:603 glc slc lds tfe\n" },
    { 0xe001625bU, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], s35 idxen offset:603 glc slc lds tfe\n" },
    { 0xe001525bU, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v18, s[80:83], s35 offen offset:603 glc slc lds tfe\n" },
    /* flags */
    { 0xe001b25bU, 0x23f43d12U, true, "        buffer_load_format_x " /* no glc */
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:603 slc addr64 lds tfe\n" },
    { 0xe000f25bU, 0x23f43d12U, true, "        buffer_load_format_x " /* no lds */
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:603 glc slc addr64 tfe\n" },
    { 0xe001f25bU, 0x23743d12U, true, "        buffer_load_format_x " /* no tfe */
        "v61, v[18:21], s[80:83], s35 offen idxen offset:603 glc slc addr64 lds\n" },
    { 0xe001f25bU, 0x23b43d12U, true, "        buffer_load_format_x " /* no slc */
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:603 glc addr64 lds tfe\n" },
    { 0xe000025bU, 0x23343d12U, true, "        buffer_load_format_x " /* no flags */
        "v61, v18, s[80:83], s35 offset:603\n" },
    /* fields */
    { 0xe001c25bU, 0xf0f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[80:83], 0.5 offset:603 glc slc addr64 lds tfe\n" },
    { 0xe001c25bU, 0x23fd3d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], ttmp[4:7], s35 offset:603 glc slc addr64 lds tfe\n" },
    { 0xe001c25bU, 0x23e93d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:19], s[36:39], s35 offset:603 glc slc addr64 lds tfe\n" },
    { 0xe001f000U, 0x23f43d12U, true, "        buffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen glc slc addr64 lds tfe\n" },
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
    { 0xe020325bU, 0x23343d12U, true, "        buffer_load_ubyte "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe024325bU, 0x23343d12U, true, "        buffer_load_sbyte "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe028325bU, 0x23343d12U, true, "        buffer_load_ushort "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe02c325bU, 0x23343d12U, true, "        buffer_load_sshort "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe030325bU, 0x23343d12U, true, "        buffer_load_dword "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe034325bU, 0x23343d12U, true, "        buffer_load_dwordx2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe038325bU, 0x23343d12U, true, "        buffer_load_dwordx4 "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe03c325bU, 0x23343d12U, true, "        MUBUF_ill_15    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe040325bU, 0x23343d12U, true, "        MUBUF_ill_16    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe044325bU, 0x23343d12U, true, "        MUBUF_ill_17    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe048325bU, 0x23343d12U, true, "        MUBUF_ill_18    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe04c325bU, 0x23343d12U, true, "        MUBUF_ill_19    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe050325bU, 0x23343d12U, true, "        MUBUF_ill_20    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe054325bU, 0x23343d12U, true, "        MUBUF_ill_21    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe058325bU, 0x23343d12U, true, "        MUBUF_ill_22    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe05c325bU, 0x23343d12U, true, "        MUBUF_ill_23    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
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
    { 0xe078325bU, 0x23343d12U, true, "        buffer_store_dwordx4 "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe07c325bU, 0x23343d12U, true, "        MUBUF_ill_31    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe080325bU, 0x23343d12U, true, "        MUBUF_ill_32    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe084325bU, 0x23343d12U, true, "        MUBUF_ill_33    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe088325bU, 0x23343d12U, true, "        MUBUF_ill_34    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe08c325bU, 0x23343d12U, true, "        MUBUF_ill_35    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe090325bU, 0x23343d12U, true, "        MUBUF_ill_36    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe094325bU, 0x23343d12U, true, "        MUBUF_ill_37    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe098325bU, 0x23343d12U, true, "        MUBUF_ill_38    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe09c325bU, 0x23343d12U, true, "        MUBUF_ill_39    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0a0325bU, 0x23343d12U, true, "        MUBUF_ill_40    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0c0325bU, 0x23343d12U, true, "        buffer_atomic_swap "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0c4325bU, 0x23343d12U, true, "        buffer_atomic_cmpswap "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0c8325bU, 0x23343d12U, true, "        buffer_atomic_add "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0cc325bU, 0x23343d12U, true, "        buffer_atomic_sub "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0d0325bU, 0x23343d12U, true, "        buffer_atomic_rsub "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0d4325bU, 0x23343d12U, true, "        buffer_atomic_smin "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0d8325bU, 0x23343d12U, true, "        buffer_atomic_umin "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0dc325bU, 0x23343d12U, true, "        buffer_atomic_smax "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0e0325bU, 0x23343d12U, true, "        buffer_atomic_umax "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0e4325bU, 0x23343d12U, true, "        buffer_atomic_and "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0e8325bU, 0x23343d12U, true, "        buffer_atomic_or "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0ec325bU, 0x23343d12U, true, "        buffer_atomic_xor "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0f0325bU, 0x23343d12U, true, "        buffer_atomic_inc "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0f4325bU, 0x23343d12U, true, "        buffer_atomic_dec "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0f8325bU, 0x23343d12U, true, "        buffer_atomic_fcmpswap "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe0fc325bU, 0x23343d12U, true, "        buffer_atomic_fmin "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe100325bU, 0x23343d12U, true, "        buffer_atomic_fmax "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe104325bU, 0x23343d12U, true, "        MUBUF_ill_65    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe108325bU, 0x23343d12U, true, "        MUBUF_ill_66    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe10c325bU, 0x23343d12U, true, "        MUBUF_ill_67    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe110325bU, 0x23343d12U, true, "        MUBUF_ill_68    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe114325bU, 0x23343d12U, true, "        MUBUF_ill_69    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe118325bU, 0x23343d12U, true, "        MUBUF_ill_70    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe11c325bU, 0x23343d12U, true, "        MUBUF_ill_71    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe120325bU, 0x23343d12U, true, "        MUBUF_ill_72    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe124325bU, 0x23343d12U, true, "        MUBUF_ill_73    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe140325bU, 0x23343d12U, true, "        buffer_atomic_swap_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe144325bU, 0x23343d12U, true, "        buffer_atomic_cmpswap_x2 "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe148325bU, 0x23343d12U, true, "        buffer_atomic_add_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe14c325bU, 0x23343d12U, true, "        buffer_atomic_sub_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe150325bU, 0x23343d12U, true, "        buffer_atomic_rsub_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe154325bU, 0x23343d12U, true, "        buffer_atomic_smin_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe158325bU, 0x23343d12U, true, "        buffer_atomic_umin_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe15c325bU, 0x23343d12U, true, "        buffer_atomic_smax_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe160325bU, 0x23343d12U, true, "        buffer_atomic_umax_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe164325bU, 0x23343d12U, true, "        buffer_atomic_and_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe168325bU, 0x23343d12U, true, "        buffer_atomic_or_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe16c325bU, 0x23343d12U, true, "        buffer_atomic_xor_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe170325bU, 0x23343d12U, true, "        buffer_atomic_inc_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe174325bU, 0x23343d12U, true, "        buffer_atomic_dec_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe178325bU, 0x23343d12U, true, "        buffer_atomic_fcmpswap_x2 "
                "v[61:64], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe17c325bU, 0x23343d12U, true, "        buffer_atomic_fmin_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe180325bU, 0x23343d12U, true, "        buffer_atomic_fmax_x2 "
                "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe184325bU, 0x23343d12U, true, "        MUBUF_ill_97    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe188325bU, 0x23343d12U, true, "        MUBUF_ill_98    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe18c325bU, 0x23343d12U, true, "        MUBUF_ill_99    "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe190325bU, 0x23343d12U, true, "        MUBUF_ill_100   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe194325bU, 0x23343d12U, true, "        MUBUF_ill_101   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe198325bU, 0x23343d12U, true, "        MUBUF_ill_102   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    { 0xe19c325bU, 0x23343d12U, true, "        MUBUF_ill_103   "
                "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n" },
    /* wbinv */
    { 0xe1c1325bU, 0x23343d12U, true, "        buffer_wbinvl1_sc "
        "offen idxen offset:603 lds vaddr=0x12 vdata=0x3d srsrc=0x14 soffset=0x23\n" },
    { 0xe1c1325bU, 0x00200000U, true, "        buffer_wbinvl1_sc "
        "offen idxen offset:603 lds\n" },
    { 0xe1c5325bU, 0x23343d12U, true, "        buffer_wbinvl1  "
        "offen idxen offset:603 lds vaddr=0x12 vdata=0x3d srsrc=0x14 soffset=0x23\n" },
    { 0xe1c5325bU, 0x00200000U, true, "        buffer_wbinvl1  "
        "offen idxen offset:603 lds\n" },
    { 0xe1c1f25bU, 0x23f43d12U, true, "        buffer_wbinvl1_sc "
        "offen idxen offset:603 glc slc addr64 lds tfe "
        "vaddr=0x12 vdata=0x3d srsrc=0x14 soffset=0x23\n" },
    /* MTBUF encoding */
    { 0xea88f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    /* formats */
    { 0xea80f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[invalid,sint]\n" },
    { 0xea88f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xea90f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[16,sint]\n" },
    { 0xea98f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8_8,sint]\n" },
    { 0xeaa0f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[32,sint]\n" },
    { 0xeaa8f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[16_16,sint]\n" },
    { 0xeab0f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[10_11_11,sint]\n" },
    { 0xeab8f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[11_11_10,sint]\n" },
    { 0xeac0f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[10_10_10_2,sint]\n" },
    { 0xeac8f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[2_10_10_10,sint]\n" },
    { 0xead0f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8_8_8_8,sint]\n" },
    { 0xead8f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[32_32,sint]\n" },
    { 0xeae0f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[16_16_16_16,sint]\n" },
    { 0xeae8f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[32_32_32,sint]\n" },
    { 0xeaf0f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[32_32_32_32,sint]\n" },
    { 0xeaf8f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[reserved,sint]\n" },
    /* nfmt */
    { 0xe808f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,unorm]\n" },
    { 0xe888f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,snorm]\n" },
    { 0xe908f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,uscaled]\n" },
    { 0xe988f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sscaled]\n" },
    { 0xea08f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,uint]\n" },
    { 0xea88f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xeb08f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,snorm_ogl]\n" },
    { 0xeb88f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,float]\n" },
    /* instructions */
    { 0xea89f7d4U, 0x23f43d12U, true, "        tbuffer_load_format_xy "
        "v[61:63], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xea8af7d4U, 0x23f43d12U, true, "        tbuffer_load_format_xyz "
        "v[61:64], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xea8bf7d4U, 0x23f43d12U, true, "        tbuffer_load_format_xyzw "
        "v[61:65], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xea8cf7d4U, 0x23f43d12U, true, "        tbuffer_store_format_x "
        "v[61:62], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xea8df7d4U, 0x23f43d12U, true, "        tbuffer_store_format_xy "
        "v[61:63], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xea8ef7d4U, 0x23f43d12U, true, "        tbuffer_store_format_xyz "
        "v[61:64], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
    { 0xea8ff7d4U, 0x23f43d12U, true, "        tbuffer_store_format_xyzw "
        "v[61:65], v[18:21], s[80:83], s35 offen idxen offset:2004 glc slc addr64 tfe "
        "format:[8,sint]\n" },
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
    { 0xf03cfb00U, 0x00159d79U, true, "        image_atomic_swap "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf040fb00U, 0x00159d79U, true, "        image_atomic_cmpswap "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
    { 0xf044fb00U, 0x00159d79U, true, "        image_atomic_add "
        "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n" },
};

static void testDecGCNOpcodes(cxuint i, const GCNDisasmOpcodeCase& testCase)
{
    std::ostringstream disOss;
    DisasmInput input;
    input.deviceType = GPUDeviceType::PITCAIRN;
    input.is64BitMode = false;
    Disassembler disasm(&input, disOss, DISASM_FLOATLITS);
    GCNDisassembler gcnDisasm(disasm);
    uint32_t inputCode[2] = { testCase.word0, testCase.word1 };
    gcnDisasm.setInput(testCase.twoWords?8:4, reinterpret_cast<cxbyte*>(inputCode));
    gcnDisasm.disassemble();
    std::string outStr = disOss.str();
    if (outStr != testCase.expected)
    {
        std::ostringstream oss;
        oss << "FAILED for decGCNCase#" << i << ": size=" << (testCase.twoWords?2:1) <<
            ", word0=0x" << std::hex << testCase.word0 << std::dec;
        if (testCase.twoWords)
            oss << ", word1=0x" << std::hex << testCase.word1 << std::dec;
        oss << "\nExpected: " << testCase.expected << ", Result: " << outStr;
        throw Exception(oss.str());
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(decGCNOpcodeCases)/sizeof(GCNDisasmOpcodeCase); i++)
        try
        { testDecGCNOpcodes(i, decGCNOpcodeCases[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
