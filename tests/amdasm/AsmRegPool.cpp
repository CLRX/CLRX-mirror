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
#include <iostream>
#include <string>
#include <cstdio>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "../TestUtils.h"

using namespace CLRX;

struct KernelRegPool
{
    const char* kernelName;
    cxuint sgprsNum;
    cxuint vgprsNum;
};

struct AsmRegPoolTestCase
{
    const char* input;
    const Array<KernelRegPool> regPools;
};

static const AsmRegPoolTestCase regPoolTestCasesTbl[] =
{
    /* gcn asm test cases */
    { ".amd;.kernel xx;.config;.tgsize;.text;s_add_u32 s5,s0,s1", { { "xx", 6, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_and_b64 s[6:7],s[0:1],s[2:3]",
        { { "xx", 8, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_movk_i32 s14,43", { { "xx", 15, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_cmpk_lt_i32 s14,43", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_brev_b32 s17,s2", { { "xx", 18, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_brev_b64 s[18:19],s[2:3]",
        { { "xx", 20, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_cmp_lg_i32 s11,s0", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_load_dword s11,s[0:1],s5",
        { { "xx", 12, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_load_dwordx4 s[20:23],s[0:1],s5",
        { { "xx", 24, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;s_load_dwordx16 s[20:35],s[0:1],s5",
        { { "xx", 36, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_mul_f32 v36,v1,v2", { { "xx", 1, 37 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_fract_f32 v22,v1", { { "xx", 1, 23 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_cmp_gt_f32 vcc,v71,v7",
        { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_cmp_gt_f32 s[18:19],v71,v7",
        { { "xx", 20, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_min3_f32 v22,v1,v5,v7",
        { { "xx", 1, 23 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_min3_f32 v22,v1,v5,v7",
        { { "xx", 1, 23 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_readlane_b32 s43,v4,s5",
        { { "xx", 44, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;v_interp_p1_f32 v93, v211, attr26.x",
        { { "xx", 1, 94 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;ds_write_b32 v71, v169 offset:40",
        { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;ds_write_b32 v71, v169 offset:40",
        { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;ds_read_b32 v155, v71",
        { { "xx", 1, 156 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;ds_max_rtn_i64  v[139:140], "
        "v71, v[169:170] offset:84", { { "xx", 1, 141 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;buffer_load_format_xyzw v[61:64], "
        "v18, s[80:83], s35 idxen offset:603", { { "xx", 1, 65 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;buffer_store_format_xyzw v[61:64], "
        "v18, s[80:83], s35 idxen offset:603", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;buffer_atomic_sub_x2 v[61:62], "
        "v18, s[80:83], s35 idxen offset:603", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;buffer_atomic_sub_x2 v[61:62], "
        "v18, s[80:83], s35 idxen offset:603 glc", { { "xx", 1, 63 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;image_load_mip  v[157:159], v[121:124], "
        "s[84:87] dmask:11 unorm glc r128 da", { { "xx", 1, 160 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;image_store  v[157:159], v[121:124], "
        "s[84:87] dmask:11 unorm glc r128 da", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;image_atomic_smax  v[157:159], v[121:124], "
        "s[84:87] dmask:11 unorm r128 da", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;image_atomic_smax  v[157:159], v[121:124], "
        "s[84:87] dmask:11 unorm r128 da glc", { { "xx", 1, 160 } } },
    { ".amd;.kernel xx;.config;.tgsize;.text;exp  param5, v116, v93, v27, v124 done",
        { { "xx", 1, 0 } } },
    /* gcn1.1 asm test cases */
    { ".amd;.arch gcn1.1;.kernel xx;.config;.tgsize;.text;flat_load_dwordx2  v[47:48], "
        "v[187:188]", { { "xx", 1, 49 } } },
    { ".amd;.arch gcn1.1;.kernel xx;.config;.tgsize;.text;flat_store_dwordx2 v[47:48], "
        "v[187:188]", { { "xx", 1, 0 } } },
    { ".amd;.arch gcn1.1;.kernel xx;.config;.tgsize;.text;flat_atomic_inc v47, "
        "v[187:188], v65 glc", { { "xx", 1, 48 } } }, // ???? only if glc
    /* gcn1.2 asm test cases - vop3 vintrp */
    { ".amd;.arch gcn1.2;.kernel xx;.config;.tgsize;.text;v_interp_p1_f32 v42, v16, "
        "attr39.z vop3", { { "xx", 1, 43 } } },
    /* regflags test */
    { ".gallium;.kernel xx;.config;.text;xx:s_xor_b64 "
        "s[10:11], s[4:5], s[62:63]", { { "xx", 14, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_xor_b64 s[10:11], s[4:5], vcc",
        { { "xx", 14, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_xor_b64 s[10:11], vcc, s[6:7]",
        { { "xx", 14, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_xor_b64 s[10:11], s[0:1], s[6:7];"
        "s_xor_b64 vcc, s[0:1], s[6:7]", { { "xx", 14, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_wqm_b32 s86, vcc_lo",
        { { "xx", 89, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_wqm_b32 s86, s3; s_not_b32 vcc_lo, s5",
        { { "xx", 89, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_cmp_lg_u32 vcc_lo,s3;s_not_b32 s15, s1",
        { { "xx", 18, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_cmp_lg_u32 s6,vcc_hi;s_not_b32 s15, s1",
        { { "xx", 18, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_movk_i32 vcc_lo,0xd3b9;"
        "s_not_b32 s15, s1", { { "xx", 18, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_load_dwordx2  s[2:3], s[8:9], vcc_lo;"
        "s_not_b32 s15, s1", { { "xx", 18, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:s_load_dwordx2  vcc, s[8:9], s5;"
        "s_not_b32 s15, s1", { { "xx", 18, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_sub_f32  v154, vcc_lo, v54;"
        "s_not_b32 s15, s1", { { "xx", 18, 155 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_addc_u32  v31, vcc, v21, v54, vcc;"
        "s_not_b32 s15, s1", { { "xx", 18, 32 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_addc_u32  v31, s[6:7], v21, v54, vcc;",
        { { "xx", 10, 32 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_readlane_b32  s21, vcc_lo, m0;",
        { { "xx", 24, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_sub_f32  v154, v54, vcc_hi;"
        "s_not_b32 s15, s1", { { "xx", 18, 155 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_cvt_i32_f32  v154, vcc_lo;"
        "s_not_b32 s15, s1", { { "xx", 18, 155 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_readfirstlane_b32  s56, vcc_lo;"
        "s_not_b32 s15, s1", { { "xx", 59, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_cmp_ge_f32  vcc, v32, s54;"
        "s_not_b32 s15, s1", { { "xx", 18, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_cmp_ge_f32  s[22:23], v32, vcc_lo",
        { { "xx", 26, 1 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_mad_f32 v23, v4, v5, vcc_hi;"
        "s_not_b32 s15, s1", { { "xx", 18, 24 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_mad_f32 v23, v4, vcc_lo, v5;"
        "s_not_b32 s15, s1", { { "xx", 18, 24 } } },
    { ".gallium;.kernel xx;.config;.text;xx:v_mad_f32 v23, vcc_hi, v4, v5;"
        "s_not_b32 s15, s1", { { "xx", 18, 24 } } },
    /* gallium kcode test */
    {
        R"ffDXD(            .gallium; .gpu pitcairn
            .kernel kx0
            .config
            .kernel kx1
            .config
            .kernel kx2
            .config
            .kernel kx3
            .config
            .kernel kx4
            .config
            .kernel kx5
            .config
            .text
.p2align 2
kx0: s_mov_b32 s10, s0
.p2align 2
kx1: s_mov_b32 s14, s0
.p2align 2
kx2: s_mov_b32 s19, s0
.p2align 2
kx3: s_mov_b32 s16, s0
.p2align 2
kx4: s_mov_b32 s8, s0
.p2align 2
kx5: s_mov_b32 s11, s0
.kcode + kx1 , + kx3
            v_sub_f32 v4,v1,v2
    .kcode kx4
            v_sub_f32 v6,v1,v2
        .kcode kx5
            v_sub_f32 v7,v1,v2
        .kcodeend
    .kcodeend
            v_sub_f32 v7,v1,v2
    .kcode + kx0
            v_sub_f32 v4,v1,v2
    .kcodeend
    .kcode -kx3
            v_sub_f32 v14,v1,v2
    .kcodeend
.kcodeend)ffDXD",
        { { "kx0", 13, 5 }, { "kx1", 17, 15 }, { "kx2", 22, 1 },
            { "kx3", 19, 8 }, { "kx4", 11, 8 }, { "kx5", 14, 8 } }
    },
    {
        R"ffDXD(            .gallium; .gpu pitcairn
            .kernel kx0
            .config
            .kernel kx1
            .config
            .kernel kx2
            .config
            .kernel kx3
            .config
            .kernel kx4
            .config
            .kernel kx5
            .config
            .text
.p2align 2
kx0:
            s_mov_b32 s10, s0
.p2align 2
kx1:
            s_mov_b32 s14, s0
.p2align 2
kx2:
            s_mov_b32 s19, s0
.p2align 2
kx3:
            s_mov_b32 s16, s0
.p2align 2
kx4:
            s_mov_b32 s8, s0
.p2align 2
kx5:
            s_mov_b32 s11, s0
.kcode +
            v_sub_f32 v4,v1,v2
.kcodeend)ffDXD",
        { { "kx0", 13, 5 }, { "kx1", 17, 5 }, { "kx2", 22, 5 },
            { "kx3", 19, 5 }, { "kx4", 11, 5 }, { "kx5", 14, 5 } }
    },
    {
        R"ffDXD(            .gallium; .gpu pitcairn
            .kernel kx0
            .config
            .kernel kx1
            .config
            .kernel kx2
            .config
            .kernel kx3
            .config
            .kernel kx4
            .config
            .kernel kx5
            .config
            .kernel kx6
            .config
            .kernel kx7
            .config
            .text
.p2align 2
kx0: s_mov_b32 s10, s0
.p2align 2
kx1: s_mov_b32 s14, s0
.p2align 2
kx2: s_mov_b32 s19, s0
.p2align 2
kx3: s_mov_b32 s16, s0
.p2align 2
kx4: s_mov_b32 s8, s0
.p2align 2
kx5: s_mov_b32 s11, s0
.p2align 2
kx6: s_mov_b32 s21, s0
.p2align 2
kx7: s_mov_b32 s23, s0

.kcode kx1,kx3
            v_and_b32 v41,v2,v1
    .kcode kx5
            v_and_b32 v22,v2,v1
        .kcode kx0
            v_and_b32 v24,v2,v1
        .kcodeend
    .kcodeend
    .kcode kx4
            v_and_b32 v18,v2,v1
        .kcode kx2
            v_and_b32 v15,v2,v1
        .kcodeend
    .kcodeend
    .kcode kx7
            v_and_b32 v33,v2,v1
        .kcode kx6
            v_and_b32 v17,v2,v1
        .kcodeend
            v_and_b32 v34,v2,v1
    .kcodeend
.kcodeend)ffDXD",
        { { "kx0", 13, 25 }, { "kx1", 17, 42 }, { "kx2", 22, 16 }, { "kx3", 19, 42 },
          { "kx4", 11, 19 }, { "kx5", 14, 25 }, { "kx6", 24, 18 }, { "kx7", 26, 35 } }
    },
    
    /* rocm kcode test */
    {
        R"ffDXD(            .rocm; .gpu pitcairn
            .kernel kx0
            .config
            .kernel kx1
            .config
            .kernel kx2
            .config
            .kernel kx3
            .config
            .kernel kx4
            .config
            .kernel kx5
            .config
            .kernel kx6
            .config
            .kernel kx7
            .config
            .text
.p2align 8
kx0:
.skip 256
s_mov_b32 s10, s0
.p2align 8
kx1:
.skip 256
s_mov_b32 s14, s0
.p2align 8
kx2:
.skip 256
s_mov_b32 s19, s0
.p2align 8
kx3:
.skip 256
s_mov_b32 s16, s0
.p2align 8
kx4:
.skip 256
s_mov_b32 s8, s0
.p2align 8
kx5:
.skip 256
s_mov_b32 s11, s0
.p2align 8
kx6:
.skip 256
s_mov_b32 s21, s0
.p2align 8
kx7:
.skip 256
s_mov_b32 s23, s0

.kcode kx1,kx3
            v_and_b32 v41,v2,v1
    .kcode kx5
            v_and_b32 v22,v2,v1
        .kcode kx0
            v_and_b32 v24,v2,v1
        .kcodeend
    .kcodeend
    .kcode kx4
            v_and_b32 v18,v2,v1
        .kcode kx2
            v_and_b32 v15,v2,v1
        .kcodeend
    .kcodeend
    .kcode kx7
            v_and_b32 v33,v2,v1
        .kcode kx6
            v_and_b32 v17,v2,v1
        .kcodeend
            v_and_b32 v34,v2,v1
    .kcodeend
.kcodeend)ffDXD",
        { { "kx0", 13, 25 }, { "kx1", 17, 42 }, { "kx2", 22, 16 }, { "kx3", 19, 42 },
          { "kx4", 11, 19 }, { "kx5", 14, 25 }, { "kx6", 24, 18 }, { "kx7", 26, 35 } }
    },
    /* amdcl2 kcode test */
    {
        R"ffDXD(            .amdcl2; .gpu bonaire;
        .driver_version 240000
        .hsalayout
            .kernel kx0
            .config
            .kernel kx1
            .config
            .kernel kx2
            .config
            .kernel kx3
            .config
            .kernel kx4
            .config
            .kernel kx5
            .config
            .text
.p2align 8
kx0: .skip 256;s_mov_b32 s10, s0
.p2align 8
kx1: .skip 256; s_mov_b32 s14, s0
.p2align 8
kx2: .skip 256; s_mov_b32 s19, s0
.p2align 8
kx3: .skip 256; s_mov_b32 s16, s0
.p2align 8
kx4: .skip 256; s_mov_b32 s8, s0
.p2align 8
kx5: .skip 256; s_mov_b32 s11, s0
.kcode + kx1 , + kx3
            v_sub_f32 v4,v1,v2
    .kcode kx4
            v_sub_f32 v6,v1,v2
        .kcode kx5
            v_sub_f32 v7,v1,v2
        .kcodeend
    .kcodeend
            v_sub_f32 v7,v1,v2
    .kcode + kx0
            v_sub_f32 v4,v1,v2
    .kcodeend
    .kcode -kx3
            v_sub_f32 v14,v1,v2
    .kcodeend
.kcodeend)ffDXD",
        // sgprs without VCC
        { { "kx0", 11, 5 }, { "kx1", 15, 15 }, { "kx2", 20, 1 },
            { "kx3", 17, 8 }, { "kx4", 9, 8 }, { "kx5", 12, 8 } }
    },
};

static void testAsmRegPoolTestCase(cxuint testId, const AsmRegPoolTestCase& testCase)
{
    char testName[30];
    snprintf(testName, 30, "Test #%u", testId);
    
    std::istringstream input(testCase.input);
    Assembler assembler("test.s", input, (ASM_ALL|ASM_TESTRUN)&~ASM_ALTMACRO,
            BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE);
    assembler.setLLVMVersion(1);
    assertTrue(testName, "good", assembler.assemble());
    // retrieve data
    if (assembler.getBinaryFormat()==BinaryFormat::AMD)
    {
        // for AMD Catalyst binaries
        const AmdInput* input = static_cast<const AsmAmdHandler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->kernels.size());
        
        char buf[32];
        // compare register counting for kernels
        for (AsmKernelId i = 0; i < input->kernels.size(); i++)
        {
            const AmdKernelInput& kinput = input->kernels[i];
            const KernelRegPool& regPool = testCase.regPools[i];
            snprintf(buf, 32, "Kernel=%u.", i);
            std::string caseName(buf);
            assertString(testName, caseName+"name", regPool.kernelName, kinput.kernelName);
            assertTrue(testName, caseName+"useConfig", kinput.useConfig);
            assertValue(testName, caseName+"sgprsNum", regPool.sgprsNum,
                        kinput.config.usedSGPRsNum);
            assertValue(testName, caseName+"vgprsNum", regPool.vgprsNum,
                        kinput.config.usedVGPRsNum);
        }
    }
    else if (assembler.getBinaryFormat()==BinaryFormat::GALLIUM)
    {
        // GalliumCompute
        const GalliumInput* input = static_cast<const AsmGalliumHandler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->kernels.size());
        char buf[32];
        // compare register counting for kernels
        for (AsmKernelId i = 0; i < input->kernels.size(); i++)
        {
            const GalliumKernelInput& kinput = input->kernels[i];
            const KernelRegPool& regPool = testCase.regPools[i];
            snprintf(buf, 32, "Kernel=%u.", i);
            std::string caseName(buf);
            assertString(testName, caseName+"name", regPool.kernelName, kinput.kernelName);
            assertTrue(testName, caseName+"useConfig", kinput.useConfig);
            assertValue(testName, caseName+"sgprsNum", regPool.sgprsNum,
                        kinput.config.usedSGPRsNum);
            assertValue(testName, caseName+"vgprsNum", regPool.vgprsNum,
                        kinput.config.usedVGPRsNum);
        }
    }
    else if (assembler.getBinaryFormat()==BinaryFormat::ROCM)
    {
        // ROCmCompute
        const ROCmInput* input = static_cast<const AsmROCmHandler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->symbols.size());
        char buf[32];
        // compare register counting for kernels
        for (AsmKernelId i = 0; i < input->symbols.size(); i++)
        {
            const ROCmSymbolInput& kinput = input->symbols[i];
            const KernelRegPool& regPool = testCase.regPools[i];
            snprintf(buf, 32, "Kernel=%u.", i);
            std::string caseName(buf);
            assertString(testName, caseName+"name", regPool.kernelName, kinput.symbolName);
            assertTrue(testName, caseName+"useConfig", true);
            const ROCmKernelConfig* config = reinterpret_cast<const ROCmKernelConfig*>(
                            input->code + kinput.offset);
            assertValue(testName, caseName+"sgprsNum", regPool.sgprsNum,
                        cxuint(ULEV(config->wavefrontSgprCount)));
            assertValue(testName, caseName+"vgprsNum", regPool.vgprsNum,
                        cxuint(ULEV(config->workitemVgprCount)));
        }
    }
    else if (assembler.getBinaryFormat()==BinaryFormat::AMDCL2)
    {
        // AmdCL2
        const AmdCL2Input* input = static_cast<const AsmAmdCL2Handler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->kernels.size());
        char buf[32];
        // compare register counting for kernels
        for (AsmKernelId i = 0; i < input->kernels.size(); i++)
        {
            const AmdCL2KernelInput& kinput = input->kernels[i];
            const KernelRegPool& regPool = testCase.regPools[i];
            snprintf(buf, 32, "Kernel=%u.", i);
            std::string caseName(buf);
            assertString(testName, caseName+"name", regPool.kernelName, kinput.kernelName);
            assertTrue(testName, caseName+"useConfig", kinput.useConfig);
            assertValue(testName, caseName+"sgprsNum", regPool.sgprsNum,
                        kinput.config.usedSGPRsNum);
            assertValue(testName, caseName+"vgprsNum", regPool.vgprsNum,
                        kinput.config.usedVGPRsNum);
        }
    }
    else
        throw Exception("Unsupported binary format");
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(regPoolTestCasesTbl)/sizeof(AsmRegPoolTestCase); i++)
        try
        { testAsmRegPoolTestCase(i, regPoolTestCasesTbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
