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
#include <iostream>
#include <sstream>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/MemAccess.h>

using namespace CLRX;

struct GCNDisasmLabelCase
{
    size_t wordsNum;
    const uint32_t* words;
    const char* expected;
};

static const uint32_t code1tbl[] = { 0xd8dc2625U, 0x37000006U, 0xbf82fffeU };
static const uint32_t code2tbl[] = { 0x7c6b92ffU };
static const uint32_t code3tbl[] = { 0xd8dc2625U, 0x37000006U, 0xbf82fffeU, 0xbf820002U,
    0xea88f7d4U, 0x23f43d12U, 0xd25a0037U, 0x4002b41bU
};
static const uint32_t code4tbl[] = { 0xbf820243U, 0xbf820106U, 0xbf820105U };
/* testing labels symbols */
static const uint32_t code5tbl[] = { 0xbf820001U, 0xb1abd3b9U, 0xbf82fffeU };
static const uint32_t code6tbl[] = { 0xbf820001U, 0x81953d04U, 0xbf82fffeU };
static const uint32_t code7tbl[] = { 0xbf820001U, 0xbed60414U, 0xbf82fffeU };
static const uint32_t code8tbl[] = { 0xbf820001U, 0xbf06451dU, 0xbf82fffeU };
static const uint32_t code9tbl[] = { 0xbf820001U, 0xbf8c0f7eU, 0xbf82fffeU };
static const uint32_t code10tbl[] = { 0xbf820002U, 0xba8048c3u, 0x45d2aU, 0xbf82fffdU };
static const uint32_t code11tbl[] = { 0xbf820002U, 0xbed603ffU, 0xddbbaa11U, 0xbf82fffdU };
static const uint32_t code12tbl[] = { 0xbf820002U, 0xbf0045ffU, 0x6d894U, 0xbf82fffdU };
static const uint32_t code38tbl[] = { 0xbf820002U, 0xbf00ff45U, 0x6d894U, 0xbf82fffdU };
static const uint32_t code13tbl[] = { 0xbf820002U, 0x807fff05U, 0xd3abc5fU, 0xbf82fffdU };
static const uint32_t code14tbl[] = { 0xbf820002U, 0x807f3dffU, 0xd3abc5fU, 0xbf82fffdU };
static const uint32_t code15tbl[] = { 0xbf820001U, 0xc7998000U, 0xbf82fffeU };
static const uint32_t code16tbl[] = { 0xbf820001U, 0x0134d715U, 0xbf82fffeU };
static const uint32_t code17tbl[] = { 0xbf820002U, 0x0134d6ffU, 0x445aaU, 0xbf82fffdU };
static const uint32_t code18tbl[] = { 0xbf820002U, 0x4134d715U, 0x567d0700U, 0xbf82fffdU };
static const uint32_t code19tbl[] = { 0xbf820002U, 0x4334d715U, 0x567d0700U, 0xbf82fffdU };
static const uint32_t code20tbl[] = { 0xbf820001U, 0x7f3c024fU, 0xbf82fffeU };
static const uint32_t code21tbl[] = { 0xbf820002U, 0x7f3c0affU, 0x4556fdU, 0xbf82fffdU };
static const uint32_t code22tbl[] = { 0xbf820001U, 0x7c03934fU, 0xbf82fffeU };
static const uint32_t code23tbl[] = { 0xbf820002U, 0x7c0392ffU, 0x40000000U, 0xbf82fffdU };
static const uint32_t code24tbl[] = { 0xbf820002U, 0xd22e0037U, 0x4002b41bU, 0xbf82fffdU };
static const uint32_t code25tbl[] = { 0xbf820002U, 0xd814cd67U, 0x0000a947U, 0xbf82fffdU };
static const uint32_t code26tbl[] = { 0xbf820002U, 0xe000325bU, 0x23343d12U, 0xbf82fffdU };
static const uint32_t code27tbl[] = { 0xbf820002U, 0xea8877d4U, 0x23f43d12U, 0xbf82fffdU };
static const uint32_t code28tbl[] = { 0xbf820002U, 0xf203fb00U, 0x00159d79U, 0xbf82fffdU };
static const uint32_t code29tbl[] = { 0xbf820002U, 0xf8001a5fU, 0x7c1b5d74U, 0xbf82fffdU };
static const uint32_t code30tbl[] = { 0xbf820001U, 0xdc270000U, 0xbf82fffeU };
static const uint32_t code58tbl[] = { 0xbf820001U, 0xc97400d3U, 0xbf82fffeU };

static const uint32_t code31tbl[] = { 0xbf820002U, 0xdc370000U, 0x2f8000bbU, 0xbf82fffdU };

static const uint32_t code32tbl[] = { 0xbf820001U, 0xb12bd3b9U, 0xbf82fffeU };
static const uint32_t code33tbl[] = { 0xbf820002U, 0xba0048c3u, 0x45d2aU, 0xbf82fffdU };
static const uint32_t code34tbl[] = { 0xbf820001U, 0xbed60114U, 0xbf82fffeU };
static const uint32_t code35tbl[] = { 0xbf820002U, 0xbed600ffU, 0xddbbaa11U, 0xbf82fffdU };
static const uint32_t code36tbl[] = { 0xbf820001U, 0xbf00451dU, 0xbf82fffeU };
static const uint32_t code37tbl[] = { 0xbf820002U, 0xbf0045ffU, 0x6d894U, 0xbf82fffdU };
static const uint32_t code39tbl[] = { 0xbf820002U, 0xbf00ff45U, 0x6d894U, 0xbf82fffdU };
static const uint32_t code40tbl[] = { 0xbf820001U, 0xbf90001bU, 0xbf82fffeU };
static const uint32_t code41tbl[] = { 0xbf820002U, 0xc0020c9dU, 0x1d1345bU, 0xbf82fffdU };
static const uint32_t code42tbl[] = { 0xbf820001U, 0x92153d04U, 0xbf82fffeU };
static const uint32_t code43tbl[] = { 0xbf820002U, 0x92153dffU, 0x12345U, 0xbf82fffdU };
static const uint32_t code44tbl[] = { 0xbf820002U, 0x9215ff04U, 0x12345U, 0xbf82fffdU };
static const uint32_t code45tbl[] = { 0xbf820001U, 0x1f34d715U, 0xbf82fffeU };
static const uint32_t code46tbl[] = { 0xbf820002U, 0x0134d6f9U, 0x63dU, 0xbf82fffdU };
static const uint32_t code47tbl[] = { 0xbf820002U, 0x0134d6faU, 0x11abeU, 0xbf82fffdU };
static const uint32_t code48tbl[] = { 0xbf820002U, 0x0d34d6ffU, 0xa2346U, 0xbf82fffdU };
static const uint32_t code49tbl[] = { 0xbf820002U, 0x2f34d715U, 0x567d0700U, 0xbf82fffdU };
static const uint32_t code50tbl[] = { 0xbf820002U, 0x3134d715U, 0x567d0700U, 0xbf82fffdU };
static const uint32_t code51tbl[] = { 0xbf820002U, 0x4934d715U, 0x3d4cU, 0xbf82fffdU };
static const uint32_t code52tbl[] = { 0xbf820002U, 0x4b34d715U, 0x3d4cU, 0xbf82fffdU };
static const uint32_t code53tbl[] = { 0xbf820001U, 0x7f3c0d4fU, 0xbf82fffeU };
static const uint32_t code54tbl[] = { 0xbf820002U, 0x7f3c0cffU, 0x40000000U, 0xbf82fffdU };
static const uint32_t code55tbl[] = { 0xbf820001U, 0x7c41934fU, 0xbf82fffeU };
static const uint32_t code56tbl[] = { 0xbf820002U, 0x7c4192ffU, 0x3d4cU, 0xbf82fffdU };
static const uint32_t code57tbl[] = { 0xbf820002U, 0xd1d10037U, 0x07974d4fU, 0xbf82fffdU };
static const uint32_t code59tbl[] = { 0xbf820001U, 0xd57400d3U, 0xbf82fffeU };
static const uint32_t code60tbl[] = { 0xbf820002U, 0xd846cd67U, 0x9b00a947U, 0xbf82fffdU };
static const uint32_t code61tbl[] = { 0xbf820002U, 0xe003f25bU, 0x23b43d12U, 0xbf82fffdU };
static const uint32_t code62tbl[] = { 0xbf820002U, 0xea89f7d4U, 0x23f43d12U, 0xbf82fffdU };
static const uint32_t code63tbl[] = { 0xbf820002U, 0xf024fb00U, 0x00159d79U, 0xbf82fffdU };
static const uint32_t code64tbl[] = { 0xbf820002U, 0xc4001a5fU, 0x7c1b5d74U, 0xbf82fffdU };
static const uint32_t code65tbl[] = { 0xbf820002U, 0xdc730000U, 0x008054bfU, 0xbf82fffdU };

static const GCNDisasmLabelCase decGCNLabelCases[] =
{
    {
        3, code1tbl,
        "        ds_read2_b32    v[55:56], v6 offset0:37 offset1:38\n"
        ".L4=.-4\n        s_branch        .L4\n"
    },
    {
        1, code2tbl,
        "        /* WARNING: Unfinished instruction at end! */\n"
        "        v_cmpx_lg_f64   vcc, 0x0, v[201:202]\n"
    },
    {
        8, code3tbl,
        "        ds_read2_b32    v[55:56], v6 offset0:37 offset1:38\n"
        ".L4=.-4\n        s_branch        .L4\n"
        "        s_branch        .L24\n"
        "        tbuffer_load_format_x v[61:62], v[18:19], s[80:83], s35"
        " offen idxen offset:2004 glc slc addr64 tfe format:[8,sint]\n"
        ".L24:\n        v_cvt_pknorm_i16_f32 v55, s27, -v90\n"
    },
    {
        3, code4tbl, "        s_branch        .L2320\n        s_branch        .L1056\n"
        "        s_branch        .L1056\n.org 0x420\n.L1056:\n.org 0x910\n.L2320:\n"
    },
    /* testing label symbols */
    { 3, code5tbl,  /* SOPK */
      "        s_branch        .L8\n.L4:\n        s_cmpk_eq_i32   s43, 0xd3b9\n.L8:\n"
      "        s_branch        .L4\n" },
    { 3, code6tbl,  /* SOP2 */
      "        s_branch        .L8\n.L4:\n        s_sub_i32       s21, s4, s61\n.L8:\n"
      "        s_branch        .L4\n" },
    { 3, code7tbl, /* SOP1 */
      "        s_branch        .L8\n.L4:\n        s_mov_b64       s[86:87], s[20:21]\n"
      ".L8:\n        s_branch        .L4\n" },
    { 3, code8tbl, /* SOPC */
      "        s_branch        .L8\n.L4:\n        s_cmp_eq_u32    s29, s69\n.L8:\n"
      "        s_branch        .L4\n" },
    { 3, code9tbl, /* SOPP */
      "        s_branch        .L8\n.L4:\n        s_waitcnt       vmcnt(14)\n.L8:\n"
      "        s_branch        .L4\n" },
    { 4, code10tbl, /* SOPK with second IMM */
      "        s_branch        .L12\n.L4:\n        s_setreg_imm32_b32 hwreg(trapsts, 3, 10), "
      "0x45d2a\n.L12:\n        s_branch        .L4\n" },
    { 4, code11tbl, /* SOP1 with literal */
      "        s_branch        .L12\n.L4:\n        s_mov_b32       s86, 0xddbbaa11\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code12tbl, /* SOPC with literal */
      "        s_branch        .L12\n.L4:\n        s_cmp_eq_i32    0x6d894, s69\n.L12:\n"
      "        s_branch        .L4\n" },
    { 4, code38tbl, /* SOPC with literal */
      "        s_branch        .L12\n.L4:\n        s_cmp_eq_i32    s69, 0x6d894\n.L12:\n"
      "        s_branch        .L4\n" },
    { 4, code13tbl, /* SOP2 with literal */
      "        s_branch        .L12\n.L4:\n        s_add_u32       "
      "exec_hi, s5, 0xd3abc5f\n.L12:\n        s_branch        .L4\n" },
    { 4, code14tbl, /* SOP2 with literal 2 */
      "        s_branch        .L12\n.L4:\n        s_add_u32       "
      "exec_hi, 0xd3abc5f, s61\n.L12:\n        s_branch        .L4\n" },
    { 3, code15tbl, /* SMRD */
      "        s_branch        .L8\n.L4:\n        s_memtime       s[51:52]\n.L8:\n"
      "        s_branch        .L4\n" },
    { 3, code16tbl, /* VOP2 */
      "        s_branch        .L8\n.L4:\n        v_cndmask_b32   v154, v21, v107, vcc\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code17tbl, /* VOP2 with literal */
      "        s_branch        .L12\n.L4:\n        v_cndmask_b32   "
      "v154, 0x445aa, v107, vcc\n.L12:\n        s_branch        .L4\n" },
    { 4, code18tbl, /* VOP2 v_madmk */
      "        s_branch        .L12\n.L4:\n        v_madmk_f32     "
      "v154, v21, 0x567d0700 /* 6.9551627e+13f */, v107\n.L12:\n"
      "        s_branch        .L4\n" },
    { 4, code19tbl, /* VOP2 v_madak */
      "        s_branch        .L12\n.L4:\n        v_madak_f32     "
      "v154, v21, v107, 0x567d0700 /* 6.9551627e+13f */\n.L12:\n"
      "        s_branch        .L4\n" },
    { 3, code20tbl, /* VOP1 */
      "        s_branch        .L8\n.L4:\n        v_mov_b32       v158, s79\n.L8:\n"
      "        s_branch        .L4\n" },
    { 4, code21tbl, /* VOP1 with literal */
      "        s_branch        .L12\n.L4:\n        v_cvt_f32_i32   v158, 0x4556fd\n.L12:\n"
      "        s_branch        .L4\n" },
    { 3, code22tbl, /* VOPC */
      "        s_branch        .L8\n.L4:\n        v_cmp_lt_f32    vcc, v79, v201\n.L8:\n"
      "        s_branch        .L4\n" },
    { 4, code23tbl, /* VOPC with literal */
      "        s_branch        .L12\n.L4:\n        v_cmp_lt_f32    "
      "vcc, 0x40000000 /* 2f */, v201\n.L12:\n        s_branch        .L4\n" },
    { 3, code58tbl, /* VINTRP */
      "        s_branch        .L8\n.L4:\n        v_interp_p1_f32 v93, v211, attr0.x\n.L8:\n"
      "        s_branch        .L4\n" },
    { 4, code24tbl, /* VOP3 */
      "        s_branch        .L12\n.L4:\n        v_ashr_i32      v55, s27, -v90\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code25tbl, /* DS */
      "        s_branch        .L12\n.L4:\n        ds_min_i32      "
      "v71, v169 offset:52583\n.L12:\n        s_branch        .L4\n" },
    { 4, code26tbl, /* MUBUF */
      "        s_branch        .L12\n.L4:\n        buffer_load_format_x "
      "v61, v[18:19], s[80:83], s35 offen idxen offset:603\n.L12:\n"
      "        s_branch        .L4\n" },
    { 4, code27tbl, /* MTBUF */
      "        s_branch        .L12\n.L4:\n        tbuffer_load_format_x "
      "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
      "format:[8,sint]\n.L12:\n        s_branch        .L4\n" },
    { 4, code28tbl, /* MIMG */
      "        s_branch        .L12\n.L4:\n        image_load      v[157:160], "
      "v[121:124], s[84:87] dmask:11 unorm glc slc r128 tfe lwe da\n.L12:\n"
      "        s_branch        .L4\n" },
    { 4, code29tbl, /* EXP */
      "        s_branch        .L12\n.L4:\n        exp             "
      "param5, v116, v93, v27, v124 done vm\n.L12:\n        s_branch        .L4\n" },
    { 3, code30tbl,  /* illegal encoding */
      "        s_branch        .L8\n.L4:\n        .int 0xdc270000\n.L8:\n"
      "        s_branch        .L4\n" }
};

static const GCNDisasmLabelCase decGCN11LabelCases[] =
{
    { 4, code31tbl, /* FLAT */
      "        s_branch        .L12\n.L4:\n        flat_load_dwordx2 "
      "v[47:49], v[187:188] glc slc tfe\n.L12:\n        s_branch        .L4\n" }
};

static const GCNDisasmLabelCase decGCN12LabelCases[] =
{
    { 3, code32tbl,  /* SOPK */
      "        s_branch        .L8\n.L4:\n        s_cmpk_eq_i32   s43, 0xd3b9\n.L8:\n"
      "        s_branch        .L4\n" },
    { 4, code33tbl, /* SOPK with second IMM */
      "        s_branch        .L12\n.L4:\n        s_setreg_imm32_b32 hwreg(trapsts, 3, 10), "
      "0x45d2a\n.L12:\n        s_branch        .L4\n" },
    { 3, code34tbl,  /* SOP1 */
      "        s_branch        .L8\n.L4:\n        s_mov_b64       s[86:87], s[20:21]\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code35tbl, /* SOP1 with literal */
      "        s_branch        .L12\n.L4:\n        s_mov_b32       s86, 0xddbbaa11\n"
      ".L12:\n        s_branch        .L4\n" },
    { 3, code36tbl,  /* SOPC */
      "        s_branch        .L8\n.L4:\n        s_cmp_eq_i32    s29, s69\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code37tbl, /* SOPC with literal */
      "        s_branch        .L12\n.L4:\n        s_cmp_eq_i32    0x6d894, s69\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code39tbl, /* SOPC with literal */
      "        s_branch        .L12\n.L4:\n        s_cmp_eq_i32    s69, 0x6d894\n"
      ".L12:\n        s_branch        .L4\n" },
    { 3, code40tbl,  /* SOPP */
      "        s_branch        .L8\n.L4:\n        s_sendmsg       sendmsg(11, cut, 0)\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code41tbl, /* SMEM */
      "        s_branch        .L12\n.L4:\n        s_load_dword    s50, s[58:59], 0x1345b\n"
      ".L12:\n        s_branch        .L4\n" },
    { 3, code42tbl,  /* SOP2 */
      "        s_branch        .L8\n.L4:\n        s_mul_i32       s21, s4, s61\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code43tbl,  /* SOP2 with literal */
      "        s_branch        .L12\n.L4:\n        s_mul_i32       s21, 0x12345, s61\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code44tbl,  /* SOP2 with literal 2 */
      "        s_branch        .L12\n.L4:\n        s_mul_i32       s21, s4, 0x12345\n"
      ".L12:\n        s_branch        .L4\n" },
    { 3, code45tbl,  /* VOP2 */
      "        s_branch        .L8\n.L4:\n        v_max_u32       v154, v21, v107\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code46tbl,  /* VOP2 SDWA */
      "        s_branch        .L12\n.L4:\n        v_cndmask_b32   v154, v61, v107, vcc "
      "src0_sel:byte0 src1_sel:byte0\n.L12:\n        s_branch        .L4\n" },
    { 4, code47tbl,  /* VOP2 DPP */
      "        s_branch        .L12\n.L4:\n        v_cndmask_b32   v154, v190, v107, vcc "
      "row_shr:10 bank_mask:0 row_mask:0\n.L12:\n        s_branch        .L4\n" },
    { 4, code48tbl,  /* VOP2 with literal */
      "        s_branch        .L12\n.L4:\n        v_mul_i32_i24   v154, 0xa2346, v107\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code49tbl,  /* VOP2 : V_VMADMK_F32 */
      "        s_branch        .L12\n.L4:\n        v_madmk_f32     "
      "v154, v21, 0x567d0700 /* 6.9551627e+13f */, v107\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code50tbl,  /* VOP2 : V_VMADAK_F32 */
      "        s_branch        .L12\n.L4:\n        v_madak_f32     "
      "v154, v21, v107, 0x567d0700 /* 6.9551627e+13f */\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code51tbl,  /* VOP2 : V_VMADAK_F16 */
      "        s_branch        .L12\n.L4:\n        v_madmk_f16     "
      "v154, v21, 0x3d4c /* 1.3242h */, v107\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code52tbl,  /* VOP2 : V_VMADAK_F16 */
      "        s_branch        .L12\n.L4:\n        v_madak_f16     "
      "v154, v21, v107, 0x3d4c /* 1.3242h */\n"
      ".L12:\n        s_branch        .L4\n" },
    { 3, code53tbl,  /* VOP1 */
      "        s_branch        .L8\n.L4:\n        v_cvt_f32_u32   v158, v79\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code54tbl,  /* VOP2 with literal */
      "        s_branch        .L12\n.L4:\n        v_cvt_f32_u32   v158, 0x40000000\n"
      ".L12:\n        s_branch        .L4\n" },
    { 3, code55tbl,  /* VOPC */
      "        s_branch        .L8\n.L4:\n        v_cmp_f_f16     vcc, v79, v201\n"
      ".L8:\n        s_branch        .L4\n" },
    { 4, code56tbl,  /* VOPC with literal */
      "        s_branch        .L12\n.L4:\n        v_cmp_f_f16     "
      "vcc, 0x3d4c /* 1.3242h */, v201\n.L12:\n        s_branch        .L4\n" },
    { 4, code57tbl,  /* VOP3 */
      "        s_branch        .L12\n.L4:\n        v_min3_i32      v55, v79, v166, v229\n"
      ".L12:\n        s_branch        .L4\n" },
    { 3, code59tbl, /* VINTRP */
      "        s_branch        .L8\n.L4:\n        v_interp_p1_f32 v93, v211, attr0.x\n.L8:\n"
      "        s_branch        .L4\n" },
    { 4, code60tbl,  /* DS */
      "        s_branch        .L12\n.L4:\n        ds_inc_rtn_u32  "
      "v155, v71, v169 offset:52583\n.L12:\n        s_branch        .L4\n" },
    { 4, code61tbl,  /* MUBUF */
      "        s_branch        .L12\n.L4:\n        buffer_load_format_x "
      "v[61:62], v[18:19], s[80:83], s35 offen idxen offset:603 glc slc lds tfe\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code62tbl,  /* MTBUF */
      "        s_branch        .L12\n.L4:\n        tbuffer_load_format_xyzw "
      "v[61:65], v[18:19], s[80:83], s35 offen idxen offset:2004 glc slc tfe "
      "format:[8,sint]\n.L12:\n        s_branch        .L4\n" },
    { 4, code63tbl,  /* MIMG */
      "        s_branch        .L12\n.L4:\n        image_store_mip "
      "v[157:159], v[121:124], s[84:87] dmask:11 unorm glc r128 da\n"
      ".L12:\n        s_branch        .L4\n" },
    { 4, code64tbl,  /* EXP */
      "        s_branch        .L12\n.L4:\n        exp             "
      "param5, v116, v93, v27, v124 done vm\n.L12:\n        s_branch        .L4\n" },
    { 4, code65tbl,  /* FLAT */
      "        s_branch        .L12\n.L4:\n        flat_store_dword "
      "v[191:192], v84 glc slc tfe\n.L12:\n        s_branch        .L4\n" },
};

static void testDecGCNLabels(cxuint i, const GCNDisasmLabelCase& testCase,
                      GPUDeviceType deviceType)
{
    std::ostringstream disOss;
    AmdDisasmInput input;
    input.deviceType = deviceType;
    input.is64BitMode = false;
    Disassembler disasm(&input, disOss, DISASM_FLOATLITS);
    GCNDisassembler gcnDisasm(disasm);
    Array<uint32_t> code(testCase.wordsNum);
    for (cxuint i = 0; i < testCase.wordsNum; i++)
        code[i] = LEV(testCase.words[i]);
    
    gcnDisasm.setInput(testCase.wordsNum<<2,
           reinterpret_cast<const cxbyte*>(code.data()));
    gcnDisasm.beforeDisassemble();
    gcnDisasm.disassemble();
    std::string outStr = disOss.str();
    if (outStr != testCase.expected)
    {
        std::ostringstream oss;
        oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
            " decGCNCase#" << i << ": size=" << (testCase.wordsNum) << std::endl;
        oss << "\nExpected: " << testCase.expected << ", Result: " << outStr;
        throw Exception(oss.str());
    }
}

static const uint32_t unalignedNamedLabelCode[] =
{
    LEV(0x90153d04U),
    LEV(0x0934d6ffU), LEV(0x11110000U),
    LEV(0x90153d02U),
    LEV(0xbf82fffcU)
};

static void testDecGCNNamedLabels()
{
    std::ostringstream disOss;
    AmdDisasmInput input;
    input.deviceType = GPUDeviceType::PITCAIRN;
    input.is64BitMode = false;
    Disassembler disasm(&input, disOss, DISASM_FLOATLITS);
    GCNDisassembler gcnDisasm(disasm);
    gcnDisasm.setInput(sizeof(unalignedNamedLabelCode),
                   reinterpret_cast<const cxbyte*>(unalignedNamedLabelCode));
    gcnDisasm.addNamedLabel(1, "buru");
    gcnDisasm.addNamedLabel(2, "buru2");
    gcnDisasm.addNamedLabel(2, "buru2tto");
    gcnDisasm.addNamedLabel(3, "testLabel1");
    gcnDisasm.addNamedLabel(4, "nextInstr");
    gcnDisasm.beforeDisassemble();
    gcnDisasm.disassemble();
    if (disOss.str() !=
        "        s_lshr_b32      s21, s4, s61\n"
        "buru=.-3\n"
        "buru2=.-2\n"
        "buru2tto=.-2\n"
        "testLabel1=.-1\n"
        ".L4:\n"
        "nextInstr:\n"
        "        v_sub_f32       v154, 0x11110000 /* 1.14384831e-28f */, v107\n"
        "        s_lshr_b32      s21, s2, s61\n"
        "        s_branch        nextInstr\n")
        throw Exception("FAILED namedLabelsTest: result: "+disOss.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(decGCNLabelCases)/sizeof(GCNDisasmLabelCase); i++)
        try
        { testDecGCNLabels(i, decGCNLabelCases[i], GPUDeviceType::PITCAIRN); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; i < sizeof(decGCN11LabelCases)/sizeof(GCNDisasmLabelCase); i++)
        try
        { testDecGCNLabels(i, decGCN11LabelCases[i], GPUDeviceType::HAWAII); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; i < sizeof(decGCN12LabelCases)/sizeof(GCNDisasmLabelCase); i++)
        try
        { testDecGCNLabels(i, decGCN12LabelCases[i], GPUDeviceType::TONGA); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    
    try
    { testDecGCNNamedLabels(); }
    catch(const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        retVal = 1;
    }
    return retVal;
}
