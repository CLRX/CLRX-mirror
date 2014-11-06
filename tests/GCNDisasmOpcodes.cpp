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
};

static void testDecGCNOpcodes(cxuint i, const GCNDisasmOpcodeCase& testCase)
{
    std::ostringstream disOss;
    DisasmInput input;
    input.deviceType = GPUDeviceType::PITCAIRN;
    input.is64BitMode = false;
    Disassembler disasm(&input, disOss, 0);
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
