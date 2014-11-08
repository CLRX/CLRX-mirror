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
    /* VOP2 encoding */
    { 0x0134d715U, 0, false, "        v_cndmask_b32   v154, v21, v107, vcc\n" },
    { 0x0134d6ffU, 0x445aa, true , "        v_cndmask_b32   v154, 0x445aa, v107, vcc\n" },
    { 0x0234bd15U, 0, false, "        v_readlane_b32  s26, v21, s94\n" },
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
