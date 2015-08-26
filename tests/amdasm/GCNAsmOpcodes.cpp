/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope thaimplied warranty of
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
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/MemAccess.h>
#include "../TestUtils.h"

using namespace CLRX;

struct GCNAsmOpcodeCase
{
    const char* input;
    uint32_t expWord0, expWord1;
    bool twoWords;
    bool good;
    const char* errorMessages;
};

static const GCNAsmOpcodeCase encGCNOpcodeCases[] =
{
    { "    s_add_u32  s21, s4, s61", 0x80153d04U, 0, false, true, "" },
    /* registers and literals */
    { "    s_add_u32  vcc_lo, s4, s61", 0x806a3d04U, 0, false, true, "" },
    { "    s_add_u32  vcc_hi, s4, s61", 0x806b3d04U, 0, false, true, "" },
    { "    s_add_u32  tba_lo, s4, s61", 0x806c3d04U, 0, false, true, "" },
    { "    s_add_u32  tba_hi, s4, s61", 0x806d3d04U, 0, false, true, "" },
    { "    s_add_u32  tma_lo, s4, s61", 0x806e3d04U, 0, false, true, "" },
    { "    s_add_u32  tma_hi, s4, s61", 0x806f3d04U, 0, false, true, "" },
    { "    s_add_u32  m0, s4, s61", 0x807c3d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp0, s4, s61", 0x80703d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp1, s4, s61", 0x80713d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp[2:2], s4, s61", 0x80723d04U, 0, false, true, "" },
    { "    s_add_u32  ttmp[2], s4, s61", 0x80723d04U, 0, false, true, "" },
    { "    s_add_u32  s[21:21], s4, s61", 0x80153d04U, 0, false, true, "" },
    { "    s_add_u32  s[21], s[4], s[61]", 0x80153d04U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 0", 0x80158004U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 1", 0x80158104U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 0x2a", 0x8015aa04U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -7", 0x8015c704U, 0, false, true, "" },
    { "    s_add_u32  s21, scc, s61", 0x80153dfdU, 0, false, true, "" },
    { "    s_add_u32  s21, execz, s61", 0x80153dfcU, 0, false, true, "" },
    { "    s_add_u32  s21, vccz, s61", 0x80153dfbU, 0, false, true, "" },
    { "t=0x2b;    s_add_u32  s21, s4, t", 0x8015ab04U, 0, false, true, "" },
    { "s0=0x2b;    s_add_u32  s21, s4, @s0", 0x8015ab04U, 0, false, true, "" },
    { "s0=0x2c;    s_add_u32  s21, s4, @s0-1", 0x8015ab04U, 0, false, true, "" },
    { "s_add_u32  s21, s4, @s0-1; s0=600", 0x8015ff04U, 599, true, true, "" },
    { "s_add_u32  s21, s4, @s0-1; s0=40", 0x8015ff04U, 39, true, true, "" },
    { "s_add_u32  s21, s4, 3254500000000", 0x8015ff04U, 0xbf510100U, true, true,
        "test.s:1:21: Warning: Value 0x2f5bf510100 truncated to 0xbf510100\n" },
    { "s_add_u32  s21, s4, xx\nxx=3254500000000", 0x8015ff04U, 0xbf510100U, true, true,
        "test.s:1:21: Warning: Value 0x2f5bf510100 truncated to 0xbf510100\n" },
    /* parse second source as expression ('@' force that) */
    { ".5=0x2b;    s_add_u32       s21, s4, @.5", 0x8015ab04U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, .5", 0x8015f004U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -.5", 0x8015f104U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 1.", 0x8015f204U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -1.", 0x8015f304U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 2.", 0x8015f404U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -2.", 0x8015f504U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 4.", 0x8015f604U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, -4.", 0x8015f704U, 0, false, true, "" },
    { "    s_add_u32  s21, s4, 1234", 0x8015ff04U, 1234, true, true, "" },
    { "    s_add_u32  s21, 1234, s4", 0x801504ffU, 1234, true, true, "" },
    { "    s_add_u32  s21, s4, -4.5", 0x8015ff04U, 0xc0900000U, true, true, "" },
    { "    s_add_u32  s21, s4, 3e-7", 0x8015ff04U, 0x34a10fb0U, true, true, "" },
    /* 64-bit registers and literals */
    { "        s_xor_b64 s[22:23], s[4:5], s[62:63]\n", 0x89963e04U, 0, false, true, "" },
    { "        s_xor_b64 vcc, s[4:5], s[62:63]\n", 0x89ea3e04U, 0, false, true, "" },
    { "        s_xor_b64 tba, s[4:5], s[62:63]\n", 0x89ec3e04U, 0, false, true, "" },
    { "        s_xor_b64 tma, s[4:5], s[62:63]\n", 0x89ee3e04U, 0, false, true, "" },
    { "        s_xor_b64 ttmp[4:5], s[4:5], s[62:63]\n", 0x89f43e04U, 0, false, true, "" },
    { "        s_xor_b64 exec, s[4:5], s[62:63]\n", 0x89fe3e04U, 0, false, true, "" },
    { "        s_xor_b64 exec, s[4:5], 4000\n", 0x89feff04U, 4000, true, true, "" },
    { "        s_xor_b64 s[22:23], 0x2e, s[62:63]\n", 0x89963eaeU, 0, false, true, "" },
    { "        s_xor_b64 s[22:23], -12, s[62:63]\n", 0x89963eccU, 0, false, true, "" },
    { "        s_xor_b64 s[22:23], 1.0, s[62:63]\n", 0x89963ef2U, 0, false, true, "" },
    { "        s_xor_b64 s[22:23], vccz, s[62:63]\n", 0x89963efbU, 0, false, true, "" },
    { "        s_xor_b64 s[22:23], execz, s[62:63]\n", 0x89963efcU, 0, false, true, "" },
    { "        s_xor_b64 s[22:23], scc, s[62:63]\n", 0x89963efdU, 0, false, true, "" },
    /* errors */
    { "    s_add_u32  xx, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n" },
    { "    s_add_u32  s[2:3], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Required 1 scalar register\n" },
    { "    s_add_u32  ttmp[2:3], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Required 1 scalar register\n" },
    { "    s_add_u32  s[3:4], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Required 1 scalar register\n" },
    { "    s_add_u32  ttmp[3:4], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Required 1 scalar register\n" },
    { "    s_add_u32  s104, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Scalar register number of out range\n" },
    { "    s_add_u32  ttmp12, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: TTMPRegister number out of range (0-11)\n" },
    { "    s_add_u32  s[104:105], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Some scalar register number out of range\n" },
    { "    s_add_u32  ttmp[12:13], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Some TTMPRegister number out of range (0-11)\n" },
    { "    s_add_u32  s[44:42], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Illegal scalar register range\n" },
    { "    s_add_u32  ttmp[10:8], s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Illegal TTMPRegister range\n" },
    { "    s_add_u32  s[z], s4, s61", 0, 0, false, false,
        "test.s:1:18: Error: Missing number\n"
        "test.s:1:18: Error: Expected ',' before argument\n" },
    { "    s_add_u32  ttmp[z], s4, s61", 0, 0, false, false,
        "test.s:1:21: Error: Missing number\n"
        "test.s:1:21: Error: Expected ',' before argument\n" },
    { "    s_add_u32  sxzz, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n"
        "test.s:1:17: Error: Expected ',' before argument\n" },
    { "    s_add_u32  ttmpxzz, s4, s61", 0, 0, false, false,
        "test.s:1:16: Error: Expected 1 scalar register\n"
        "test.s:1:20: Error: Expected ',' before argument\n" },
    { "    s_add_u32  s30, s[4, s[61", 0, 0, false, false,
        "test.s:1:21: Error: Unterminated scalar register range\n"
        "test.s:1:26: Error: Unterminated scalar register range\n" },
    { "    s_xor_b64  s[30:31], s[3:4], s[15:16]", 0, 0, false, false,
        "test.s:1:26: Error: Unaligned scalar register range\n"
        "test.s:1:34: Error: Unaligned scalar register range\n" },
    { "    s_xor_b64  s[30:31], vcc_lo, s[14:15]", 0, 0, false, false,
        "test.s:1:26: Error: Required 2 scalar registers\n" },
    { "    s_xor_b64  s[30:31], vcc_hi, s[14:15]", 0, 0, false, false,
        "test.s:1:26: Error: Required 2 scalar registers\n" },
    { "    s_add_u32  s10, xfdd+a*, s61", 0, 0, false, false,
        "test.s:1:28: Error: Unterminated expression\n" },
    { "    s_add_u32  s10, , ", 0, 0, false, false,
        "test.s:1:21: Error: Expected instruction operand\n"
        "test.s:1:23: Error: Expected instruction operand\n" },
    { "  s_add_u32  s35, 400000, 1111116", 0, 0, false, false,
        "test.s:1:27: Error: Only one literal constant can be used in instruction\n" },
    /* SOP2 encodings */
    { "    s_sub_u32  s21, s4, s61", 0x80953d04U, 0, false, true, "" },
    { "    s_add_i32  s21, s4, s61", 0x81153d04U, 0, false, true, "" },
    { "    s_sub_i32  s21, s4, s61", 0x81953d04U, 0, false, true, "" },
    { "    s_addc_u32  s21, s4, s61", 0x82153d04U, 0, false, true, "" },
    { "    s_subb_u32  s21, s4, s61", 0x82953d04U, 0, false, true, "" },
    { "    s_min_i32  s21, s4, s61", 0x83153d04U, 0, false, true, "" },
    { "    s_min_u32  s21, s4, s61", 0x83953d04U, 0, false, true, "" },
    { "    s_max_i32  s21, s4, s61", 0x84153d04U, 0, false, true, "" },
    { "    s_max_u32  s21, s4, s61", 0x84953d04U, 0, false, true, "" },
    { "    s_cselect_b32  s21, s4, s61", 0x85153d04U, 0, false, true, "" },
    { "    s_cselect_b64  s[20:21], s[4:5], s[62:63]", 0x85943e04U, 0, false, true, "" },
    { "    s_and_b32  s21, s4, s61", 0x87153d04U, 0, false, true, "" },
    { "    s_and_b64  s[20:21], s[4:5], s[62:63]", 0x87943e04U, 0, false, true, "" },
    { "    s_or_b32  s21, s4, s61", 0x88153d04U, 0, false, true, "" },
    { "    s_or_b64  s[20:21], s[4:5], s[62:63]", 0x88943e04U, 0, false, true, "" },
    { "    s_xor_b32  s21, s4, s61", 0x89153d04U, 0, false, true, "" },
    { "    s_xor_b64  s[20:21], s[4:5], s[62:63]", 0x89943e04U, 0, false, true, "" },
    { "    s_andn2_b32  s21, s4, s61", 0x8a153d04U, 0, false, true, "" },
    { "    s_andn2_b64  s[20:21], s[4:5], s[62:63]", 0x8a943e04U, 0, false, true, "" },
    { "    s_orn2_b32  s21, s4, s61", 0x8b153d04U, 0, false, true, "" },
    { "    s_orn2_b64  s[20:21], s[4:5], s[62:63]", 0x8b943e04U, 0, false, true, "" },
    { "    s_nand_b32  s21, s4, s61", 0x8c153d04U, 0, false, true, "" },
    { "    s_nand_b64  s[20:21], s[4:5], s[62:63]", 0x8c943e04U, 0, false, true, "" },
    { "    s_nor_b32  s21, s4, s61", 0x8d153d04U, 0, false, true, "" },
    { "    s_nor_b64  s[20:21], s[4:5], s[62:63]", 0x8d943e04U, 0, false, true, "" },
    { "    s_xnor_b32  s21, s4, s61", 0x8e153d04U, 0, false, true, "" },
    { "    s_xnor_b64  s[20:21], s[4:5], s[62:63]", 0x8e943e04U, 0, false, true, "" },
    { "    s_lshl_b32  s21, s4, s61", 0x8f153d04U, 0, false, true, "" },
    { "    s_lshl_b64  s[20:21], s[4:5], s61", 0x8f943d04U, 0, false, true, "" },
    { "    s_lshr_b32  s21, s4, s61", 0x90153d04U, 0, false, true, "" },
    { "    s_lshr_b64  s[20:21], s[4:5], s61", 0x90943d04U, 0, false, true, "" },
    { "    s_ashr_i32  s21, s4, s61", 0x91153d04U, 0, false, true, "" },
    { "    s_ashr_i64  s[20:21], s[4:5], s61", 0x91943d04U, 0, false, true, "" },
    { "    s_bfm_b32  s21, s4, s61", 0x92153d04U, 0, false, true, "" },
    { "    s_bfm_b64  s[20:21], s4, s62", 0x92943e04U, 0, false, true, "" },
    { "    s_mul_i32  s21, s4, s61", 0x93153d04U, 0, false, true, "" },
    { "    s_bfe_u32  s21, s4, s61", 0x93953d04U, 0, false, true, "" },
    { "    s_bfe_i32  s21, s4, s61", 0x94153d04U, 0, false, true, "" },
    { "    s_bfe_u64  s[20:21], s[4:5], s61", 0x94943d04U, 0, false, true, "" },
    { "    s_bfe_i64  s[20:21], s[4:5], s61", 0x95143d04U, 0, false, true, "" },
    { "    s_cbranch_g_fork  s[4:5], s[62:63]", 0x95803e04U, 0, false, true, "" },
    { "    s_absdiff_i32  s21, s4, s61", 0x96153d04U, 0, false, true, "" },
    /* SOP1 */
    { "    s_mov_b32  s86, s20", 0xbed60314U, 0, false, true, "" },
    { "    s_mov_b32  s86, 0xadbc", 0xbed603ff, 0xadbc, true, true, "" },
    { "    s_mov_b32  s86, xx; xx=0xadbc", 0xbed603ff, 0xadbc, true, true, "" },
    { "    s_mov_b64  s[86:87], s[20:21]", 0xbed60414U, 0, false, true, "" },
    { "    s_cmov_b32  s86, s20", 0xbed60514U, 0, false, true, "" },
    { "    s_cmov_b64  s[86:87], s[20:21]", 0xbed60614U, 0, false, true, "" },
    { "    s_not_b32  s86, s20", 0xbed60714U, 0, false, true, "" },
    { "    s_not_b64  s[86:87], s[20:21]", 0xbed60814U, 0, false, true, "" },
    { "    s_wqm_b32  s86, s20", 0xbed60914U, 0, false, true, "" },
    { "    s_wqm_b64  s[86:87], s[20:21]", 0xbed60a14U, 0, false, true, "" },
    { "    s_brev_b32  s86, s20", 0xbed60b14U, 0, false, true, "" },
    { "    s_brev_b64  s[86:87], s[20:21]", 0xbed60c14U, 0, false, true, "" },
    { "    s_bcnt0_i32_b32  s86, s20", 0xbed60d14U, 0, false, true, "" },
    { "    s_bcnt0_i32_b64  s86, s[20:21]", 0xbed60e14U, 0, false, true, "" },
    { "    s_bcnt1_i32_b32  s86, s20", 0xbed60f14U, 0, false, true, "" },
    { "    s_bcnt1_i32_b64  s86, s[20:21]", 0xbed61014U, 0, false, true, "" },
    { "    s_ff0_i32_b32  s86, s20", 0xbed61114U, 0, false, true, "" },
    { "    s_ff0_i32_b64  s86, s[20:21]", 0xbed61214U, 0, false, true, "" },
    { "    s_ff1_i32_b32  s86, s20", 0xbed61314U, 0, false, true, "" },
    { "    s_ff1_i32_b64  s86, s[20:21]", 0xbed61414U, 0, false, true, "" },
    { "    s_flbit_i32_b32  s86, s20", 0xbed61514U, 0, false, true, "" },
    { "    s_flbit_i32_b64  s86, s[20:21]", 0xbed61614U, 0, false, true, "" },
    { "    s_flbit_i32  s86, s20", 0xbed61714U, 0, false, true, "" },
    { "    s_flbit_i32_i64  s86, s[20:21]", 0xbed61814U, 0, false, true, "" },
    { "    s_sext_i32_i8  s86, s20", 0xbed61914U, 0, false, true, "" },
    { "    s_sext_i32_i16  s86, s20", 0xbed61a14U, 0, false, true, "" },
    { "    s_bitset0_b32  s86, s20", 0xbed61b14U, 0, false, true, "" },
    { "    s_bitset0_b64  s[86:87], s20", 0xbed61c14U, 0, false, true, "" },
    { "    s_bitset1_b32  s86, s20", 0xbed61d14U, 0, false, true, "" },
    { "    s_bitset1_b64  s[86:87], s20", 0xbed61e14U, 0, false, true, "" },
    { "    s_getpc_b64  s[86:87]", 0xbed61f00U, 0, false, true, "" },
    { "    s_setpc_b64  s[20:21]", 0xbe802014U, 0, false, true, "" },
    { "    s_swappc_b64  s[86:87], s[20:21]", 0xbed62114U, 0, false, true, "" },
    { "    s_rfe_b64  s[20:21]", 0xbe802214U, 0, false, true, "" },
    { "    s_and_saveexec_b64 s[86:87], s[20:21]", 0xbed62414U, 0, false, true, "" },
    { "    s_or_saveexec_b64 s[86:87], s[20:21]", 0xbed62514U, 0, false, true, "" },
    { "    s_xor_saveexec_b64 s[86:87], s[20:21]", 0xbed62614U, 0, false, true, "" },
    { "    s_andn2_saveexec_b64 s[86:87], s[20:21]", 0xbed62714U, 0, false, true, "" },
    { "    s_orn2_saveexec_b64 s[86:87], s[20:21]", 0xbed62814U, 0, false, true, "" },
    { "    s_nand_saveexec_b64 s[86:87], s[20:21]", 0xbed62914U, 0, false, true, "" },
    { "    s_nor_saveexec_b64 s[86:87], s[20:21]", 0xbed62a14U, 0, false, true, "" },
    { "    s_xnor_saveexec_b64 s[86:87], s[20:21]", 0xbed62b14U, 0, false, true, "" },
    { "    s_quadmask_b32  s86, s20",  0xbed62c14U, 0, false, true, "" },
    { "    s_quadmask_b64  s[86:87], s[20:21]",  0xbed62d14U, 0, false, true, "" },
    { "    s_movrels_b32  s86, s20",  0xbed62e14U, 0, false, true, "" },
    { "    s_movrels_b64  s[86:87], s[20:21]",  0xbed62f14U, 0, false, true, "" },
    { "    s_movreld_b32  s86, s20",  0xbed63014U, 0, false, true, "" },
    { "    s_movreld_b64  s[86:87], s[20:21]",  0xbed63114U, 0, false, true, "" },
    { "    s_cbranch_join  s20", 0xbe803214U, 0, false, true, "" },
    { "    s_mov_regrd_b32 s86, s20", 0xbed63314U, 0, false, true, "" },
    { "    s_abs_i32  s86, s20", 0xbed63414U, 0, false, true, "" },
    { "    s_mov_fed_b32  s86, s20", 0xbed63514U, 0, false, true, "" },
    /* SOPC */
    { "    s_cmp_eq_i32  s29, s69", 0xbf00451dU, 0, false, true, "" },
    { "    s_cmp_eq_i32  12222, s69", 0xbf0045ffU, 12222, true, true, "" },
    { "    s_cmp_eq_i32  xx, s69; xx=12222", 0xbf0045ffU, 12222, true, true, "" },
    { "    s_cmp_eq_i32  s29, 32545", 0xbf00ff1dU, 32545, true, true, "" },
    { "    s_cmp_eq_i32  s29, xx; xx=32545", 0xbf00ff1dU, 32545, true, true, "" },
    { "    s_cmp_lg_i32  s29, s69", 0xbf01451dU, 0, false, true, "" },
    { "    s_cmp_gt_i32  s29, s69", 0xbf02451dU, 0, false, true, "" },
    { "    s_cmp_ge_i32  s29, s69", 0xbf03451dU, 0, false, true, "" },
    { "    s_cmp_lt_i32  s29, s69", 0xbf04451dU, 0, false, true, "" },
    { "    s_cmp_le_i32  s29, s69", 0xbf05451dU, 0, false, true, "" },
    { "    s_cmp_eq_u32  s29, s69", 0xbf06451dU, 0, false, true, "" },
    { "    s_cmp_lg_u32  s29, s69", 0xbf07451dU, 0, false, true, "" },
    { "    s_cmp_gt_u32  s29, s69", 0xbf08451dU, 0, false, true, "" },
    { "    s_cmp_ge_u32  s29, s69", 0xbf09451dU, 0, false, true, "" },
    { "    s_cmp_lt_u32  s29, s69", 0xbf0a451dU, 0, false, true, "" },
    { "    s_cmp_le_u32  s29, s69", 0xbf0b451dU, 0, false, true, "" },
    { "    s_bitcmp0_b32  s29, s69", 0xbf0c451dU, 0, false, true, "" },
    { "    s_bitcmp1_b32  s29, s69", 0xbf0d451dU, 0, false, true, "" },
    { "    s_bitcmp0_b64  s[28:29], s69", 0xbf0e451cU, 0, false, true, "" },
    { "    s_bitcmp1_b64  s[28:29], s69", 0xbf0f451cU, 0, false, true, "" },
    { "    s_setvskip  s29, s69", 0xbf10451dU, 0, false, true, "" },
    /* SOPK */
    { "    s_movk_i32  s43, 0xd3b9", 0xb02bd3b9U, 0, false, true, "" },
    { "xc = 0xd4ba\n    s_movk_i32  s43, xc", 0xb02bd4baU, 0, false, true, "" },
    { "    s_movk_i32  s43, xc; xc = 0xd4ba", 0xb02bd4baU, 0, false, true, "" },
    { "xc = 0x11d4ba\n    s_movk_i32  s43, xc", 0xb02bd4baU, 0, false, true,
        "test.s:2:22: Warning: Value 0x11d4ba truncated to 0xd4ba\n" },
    { "    s_movk_i32  s43, xc; xc = 0x22d4ba", 0xb02bd4baU, 0, false, true,
        "test.s:1:22: Warning: Value 0x22d4ba truncated to 0xd4ba\n" },
    { "    s_cmovk_i32  s43, 0xd3b9", 0xb12bd3b9U, 0, false, true, "" },
    { "    s_cmpk_eq_i32  s43, 0xd3b9", 0xb1abd3b9U, 0, false, true, "" },
    { "    s_cmpk_lg_i32  s43, 0xd3b9", 0xb22bd3b9U, 0, false, true, "" },
    { "    s_cmpk_gt_i32  s43, 0xd3b9", 0xb2abd3b9U, 0, false, true, "" },
    { "    s_cmpk_ge_i32  s43, 0xd3b9", 0xb32bd3b9U, 0, false, true, "" },
    { "    s_cmpk_lt_i32  s43, 0xd3b9", 0xb3abd3b9U, 0, false, true, "" },
    { "    s_cmpk_le_i32  s43, 0xd3b9", 0xb42bd3b9U, 0, false, true, "" },
    { "    s_cmpk_eq_u32  s43, 0xd3b9", 0xb4abd3b9U, 0, false, true, "" },
    { "    s_cmpk_lg_u32  s43, 0xd3b9", 0xb52bd3b9U, 0, false, true, "" },
    { "    s_cmpk_gt_u32  s43, 0xd3b9", 0xb5abd3b9U, 0, false, true, "" },
    { "    s_cmpk_ge_u32  s43, 0xd3b9", 0xb62bd3b9U, 0, false, true, "" },
    { "    s_cmpk_lt_u32  s43, 0xd3b9", 0xb6abd3b9U, 0, false, true, "" },
    { "    s_cmpk_le_u32  s43, 0xd3b9", 0xb72bd3b9U, 0, false, true, "" },
    { "    s_addk_i32  s43, 0xd3b9", 0xb7abd3b9U, 0, false, true, "" },
    { "    s_mulk_i32  s43, 0xd3b9", 0xb82bd3b9U, 0, false, true, "" },
    { "    s_cbranch_i_fork s[44:45], xxxx+8\nxxxx:\n", 0xb8ac0002U, 0, false, true, "" },
    { "xxxx:    s_cbranch_i_fork s[44:45], xxxx+16\n", 0xb8ac0003U, 0, false, true, "" },
    { "    s_cbranch_i_fork s[44:45], xxxx+9\nxxxx:\n", 0, 0, false, false,
        "test.s:1:32: Error: Jump is not aligned to word!\n" },
    { "    s_cbranch_i_fork s[44:45], xxxx+10\nxxxx:\n", 0, 0, false, false,
        "test.s:1:32: Error: Jump is not aligned to word!\n" },
    { "xxxx:    s_cbranch_i_fork s[44:45], xxxx+17\n", 0, 0, false, false,
        "test.s:1:44: Error: Jump is not aligned to word!\n" },
    { "    s_cbranch_i_fork s[44:45], xxxx+8\n.rodata\nxxxx:\n", 0, 0, false, false,
        "test.s:1:32: Error: Jump over current section!\n" },
    { ".rodata\nxxxx:.text\n    s_cbranch_i_fork s[44:45], xxxx+8", 0, 0, false, false,
        "test.s:3:32: Error: Jump over current section!\n" },
    /* hwregs */
    { "    s_getreg_b32    s43, hwreg(mode, 0, 1)", 0xb92b0001U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg  (mode, 0, 1)", 0xb92b0001U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg  (mode  ,   0  , 1  )",
                    0xb92b0001U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_MODE, 0, 1)", 0xb92b0001U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(status, 0, 1)", 0xb92b0002U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_STATUS, 0, 1)", 0xb92b0002U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(trapsts, 0, 1)", 0xb92b0003U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_TRAPSTS, 0, 1)",
                    0xb92b0003U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(hw_id, 0, 1)", 0xb92b0004U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_HW_ID, 0, 1)",
                    0xb92b0004U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(gpr_alloc, 0, 1)", 0xb92b0005U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_GPR_ALLOC, 0, 1)",
                    0xb92b0005U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(lds_alloc, 0, 1)", 0xb92b0006U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_LDS_ALLOC, 0, 1)",
                    0xb92b0006U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(ib_sts, 0, 1)", 0xb92b0007U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_IB_STS, 0, 1)",
                    0xb92b0007U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(pc_lo, 0, 1)", 0xb92b0008U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_PC_LO, 0, 1)",
                    0xb92b0008U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(pc_hi, 0, 1)", 0xb92b0009U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_PC_HI, 0, 1)",
                    0xb92b0009U, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(inst_dw0, 0, 1)", 0xb92b000aU, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_INST_DW0, 0, 1)",
                    0xb92b000aU, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(inst_dw1, 0, 1)", 0xb92b000bU, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_INST_DW1, 0, 1)",
                    0xb92b000bU, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(ib_dbg0, 0, 1)", 0xb92b000cU, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(HWREG_IB_DBG0, 0, 1)",
                    0xb92b000cU, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(trapsts, 10, 1)", 0xb92b0283u, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(trapsts, 3, 10)", 0xb92b48c3u, 0, false, true, "" },
    { "    s_getreg_b32    s43, hwreg(trapsts, 3, 32)", 0xb92bf8c3u, 0, false, true, "" },
    { "    s_setreg_imm32_b32 hwreg(trapsts, 3, 10), 0x24da4f",
                    0xba8048c3u, 0x24da4fU, true, true, "" },
    { "    s_setreg_imm32_b32 hwreg(trapsts, 3, 10), xx; xx=0x24da4f",
                    0xba8048c3u, 0x24da4fU, true, true, "" },
    { "xx=0x24da4e;    s_setreg_imm32_b32 hwreg(trapsts, 3, 10), xx",
                    0xba8048c3u, 0x24da4eU, true, true, "" },
    { "     s_setreg_b32  hwreg(trapsts, 3, 10), s43", 0xb9ab48c3u, 0, false, true, "" },
    /* hw regs errors */
    { "    s_getreg_b32  s43, hwreg   (HWREG_MODE, 0, 2", 0, 0, false, false,
        "test.s:1:49: Error: Unterminated hwreg function call\n" },
    { "    s_getreg_b32  s43, hwrX(mode, 0, 1)", 0, 0, false, false,
        "test.s:1:24: Error: Expected hwreg function\n" },
    { "    s_getreg_b32  s43, hwreg(mode, , )", 0, 0, false, false,
        "test.s:1:36: Error: Expected expression\n"
        "test.s:1:38: Error: Expected operator or value or symbol\n"
        "test.s:1:39: Error: Unterminated hwreg function call\n" },
    { "    s_getreg_b32  s43, hwreg(XXX,6,8)", 0, 0, false, false,
        "test.s:1:30: Error: Unrecognized HWRegister\n" },
    /// s_getreg_b32  s43, hwreg(HW_REG_IB_DBG0, 0, 1)
    { nullptr, 0, 0, false, false, 0 }
};

static void testEncGCNOpcodes(cxuint i, const GCNAsmOpcodeCase& testCase,
                      GPUDeviceType deviceType)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL, BinaryFormat::GALLIUM, deviceType,
                    errorStream);
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << getGPUDeviceTypeName(deviceType) << " encGCNCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testEncGCNOpcodes", testCaseName+".good", testCase.good, good);
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
            " encGCNCase#" << i;
        throw Exception(oss.str());
    }
    const AsmSection& section = assembler.getSections()[0];
    const size_t codeSize = section.content.size();
    const size_t expectedSize = (testCase.good) ? ((testCase.twoWords)?8:4) : 0;
    if (good && codeSize != expectedSize)
    {
        std::ostringstream oss;
        oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
            " encGCNCase#" << i << ". Wrong code size: " << expectedSize << "!=" <<
            codeSize;
        throw Exception(oss.str());
    }
    // check content
    if (expectedSize!=0)
    {
        uint32_t expectedWord0 = testCase.expWord0;
        uint32_t expectedWord1 = testCase.expWord1;
        uint32_t resultWord0 = ULEV(*reinterpret_cast<const uint32_t*>(
                    section.content.data()));
        uint32_t resultWord1 = 0;
        if (expectedSize==8)
            resultWord1 = ULEV(*reinterpret_cast<const uint32_t*>(
                        section.content.data()+4));
        
        if (expectedWord0!=resultWord0 || (expectedSize==8 && expectedWord1!=resultWord1))
        {
            std::ostringstream oss;
            oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
                " encGCNCase#" << i << ". Content doesn't match: 0x" <<
                std::hex << expectedWord0 << "!=0x" << resultWord0 << std::dec;
            if (expectedSize==8)
                oss << ", 0x" << std::hex << expectedWord1 << "!=0x" <<
                            resultWord1 << std::dec;
            throw Exception(oss.str());
        }
    }
    assertString("testEncGCNOpcodes", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; encGCNOpcodeCases[i].input!=nullptr; i++)
        try
        { testEncGCNOpcodes(i, encGCNOpcodeCases[i], GPUDeviceType::PITCAIRN); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    
    return retVal;
}
