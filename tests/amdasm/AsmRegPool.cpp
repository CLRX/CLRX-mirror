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
{   /* gcn asm test cases */
    { ".amd;.kernel xx;.config;.text;s_add_u32 s5,s0,s1", { { "xx", 6, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_and_b64 s[6:7],s[0:1],s[2:3]", { { "xx", 8, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_cmpk_lt_i32 s14,43", { { "xx", 15, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_brev_b32 s17,s2", { { "xx", 18, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_brev_b64 s[18:19],s[2:3]", { { "xx", 20, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_cmp_lg_i32 s11,s0", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_load_dword s11,s[0:1],s5", { { "xx", 12, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_load_dwordx4 s[20:23],s[0:1],s5",
        { { "xx", 24, 0 } } },
    { ".amd;.kernel xx;.config;.text;s_load_dwordx16 s[20:35],s[0:1],s5",
        { { "xx", 36, 0 } } },
    { ".amd;.kernel xx;.config;.text;v_mul_f32 v36,v1,v2", { { "xx", 1, 37 } } },
    { ".amd;.kernel xx;.config;.text;v_fract_f32 v22,v1", { { "xx", 1, 23 } } },
    { ".amd;.kernel xx;.config;.text;v_cmp_gt_f32 vcc,v71,v7", { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.text;v_cmp_gt_f32 s[18:19],v71,v7", { { "xx", 20, 0 } } },
    { ".amd;.kernel xx;.config;.text;v_min3_f32 v22,v1,v5,v7", { { "xx", 1, 23 } } },
    { ".amd;.kernel xx;.config;.text;v_min3_f32 v22,v1,v5,v7", { { "xx", 1, 23 } } },
    { ".amd;.kernel xx;.config;.text;v_readlane_b32 s43,v4,s5", { { "xx", 44, 0 } } },
    { ".amd;.kernel xx;.config;.text;v_interp_p1_f32 v93, v211, attr26.x",
        { { "xx", 1, 94 } } },
    { ".amd;.kernel xx;.config;.text;ds_write_b32 v71, v169 offset:40",
        { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.text;ds_write_b32 v71, v169 offset:40",
        { { "xx", 1, 0 } } },
    { ".amd;.kernel xx;.config;.text;ds_read_b32 v155, v71", { { "xx", 1, 156 } } },
    { ".amd;.kernel xx;.config;.text;ds_max_rtn_i64  v[139:140], "
        "v71, v[169:170] offset:84", { { "xx", 1, 141 } } },
    { ".amd;.kernel xx;.config;.text;buffer_load_format_xyzw v[61:64], v18, s[80:83], "
        "s35 idxen offset:603", { { "xx", 1, 65 } } },
};

static void testAsmRegPoolTestCase(cxuint testId, const AsmRegPoolTestCase& testCase)
{
    char testName[30];
    snprintf(testName, 30, "Test #%u", testId);
    
    std::istringstream input(testCase.input);
    Assembler assembler("test.s", input, ASM_ALL, BinaryFormat::AMD,
            GPUDeviceType::CAPE_VERDE);
    assertTrue(testName, "good", assembler.assemble());
    Array<KernelRegPool> resRegPools;
    // retrieve data
    if (assembler.getBinaryFormat()==BinaryFormat::AMD)
    {   // Catalyst
        const AmdInput* input = static_cast<const AsmAmdHandler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->kernels.size());
        
        char buf[32];
        for (cxuint i = 0; i < input->kernels.size(); i++)
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
    if (assembler.getBinaryFormat()==BinaryFormat::GALLIUM)
    {   // GalliumCompute
        const GalliumInput* input = static_cast<const AsmGalliumHandler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->kernels.size());
        char buf[32];
        for (cxuint i = 0; i < input->kernels.size(); i++)
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
