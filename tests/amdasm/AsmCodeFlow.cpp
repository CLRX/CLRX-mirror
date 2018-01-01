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
#include <sstream>
#include <string>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"

using namespace CLRX;

struct AsmCodeFlowCase
{
    const char* input;
    Array<AsmCodeFlowEntry> codeFlow;
    bool good;
    const char* errorMessages;
};

static const AsmCodeFlowCase codeFlowTestCases1Tbl[] =
{
    {
        R"ffDXD(.text
        v_mov_b32 v3, v1
        v_mov_b32 v3, v2
label1:
        v_mov_b32 v3, v6
label2:
        v_mov_b32 v3, v2
        v_mov_b32 v3, v2
        .cf_jump label1, label2, label3
        v_nop
        v_add_f32 v54, v3, v21
        v_add_f32 v54, v3, v21
        .cf_call label4, label2, label3
        v_mul_f32 v54, v3, v21
        .cf_cjump label1, label4, label3
label3: s_nop 3
label4: v_nop
        .cf_ret
        v_nop; v_nop
        .cf_start
        v_nop; v_nop; v_nop
        .cf_end)ffDXD",
        {
            { 20U, 8U, AsmCodeFlowType::JUMP },
            { 20U, 12U, AsmCodeFlowType::JUMP },
            { 20U, 36U, AsmCodeFlowType::JUMP },
            { 32U, 12U, AsmCodeFlowType::CALL },
            { 32U, 36U, AsmCodeFlowType::CALL },
            { 32U, 40U, AsmCodeFlowType::CALL },
            { 36U, 8U, AsmCodeFlowType::CJUMP },
            { 36U, 36U, AsmCodeFlowType::CJUMP },
            { 36U, 40U, AsmCodeFlowType::CJUMP },
            { 44U, 0U, AsmCodeFlowType::RETURN },
            { 52U, 0U, AsmCodeFlowType::START },
            { 64U, 0U, AsmCodeFlowType::END }
        }, true, ""
    },
    {
        R"ffDXD(.text
        v_mov_b32 v3, v1
        v_mov_b32 v3, v2
label1:
        v_mov_b32 v3, v6
label2:
        v_mov_b32 v3, v2
        v_mov_b32 v3, v2
        s_branch label1
        s_cbranch_vccz label1
        s_cbranch_i_fork s[4:5], label1
        s_nop 5
        s_branch label2
        s_cbranch_vccz label2
        s_cbranch_i_fork s[6:7], label2
        v_mov_b32 v5, v3
        v_sub_f32 v6, v2, v1
        s_branch label3
        s_cbranch_vccz label3
        s_cbranch_i_fork s[6:7], label3
        v_nop; v_nop; v_nop
label3: v_madak_f32 v3, v4, v6, 1.453
)ffDXD",
        {
            { 20U, 8U, AsmCodeFlowType::JUMP },
            { 24U, 8U, AsmCodeFlowType::CJUMP },
            { 28U, 8U, AsmCodeFlowType::CJUMP },
            { 36U, 12U, AsmCodeFlowType::JUMP },
            { 40U, 12U, AsmCodeFlowType::CJUMP },
            { 44U, 12U, AsmCodeFlowType::CJUMP },
            { 56U, 80U, AsmCodeFlowType::JUMP },
            { 60U, 80U, AsmCodeFlowType::CJUMP },
            { 64U, 80U, AsmCodeFlowType::CJUMP },
        }, true, ""
    },
    // RX VEGA (CALL)
    {
        R"ffDXD(.gpu gfx900
        .text
        v_mov_b32 v3, v1
        v_mov_b32 v3, v2
label1:
        v_mov_b32 v3, v6
label2:
        v_mov_b32 v3, v2
        v_mov_b32 v3, v2
        s_branch label1
        s_cbranch_vccz label1
        s_cbranch_i_fork s[4:5], label1
        s_nop 5
        s_branch label2
        s_cbranch_vccz label2
        s_cbranch_i_fork s[6:7], label2
        v_mov_b32 v5, v3
        v_sub_f32 v6, v2, v1
        s_call_b64 s[10:11], label2
        s_call_b64 s[10:11], label3
        s_cbranch_i_fork s[6:7], label3
        v_nop; v_nop; v_nop
label3: v_madak_f32 v3, v4, v6, 1.453
)ffDXD",
        {
            { 20U, 8U, AsmCodeFlowType::JUMP },
            { 24U, 8U, AsmCodeFlowType::CJUMP },
            { 28U, 8U, AsmCodeFlowType::CJUMP },
            { 36U, 12U, AsmCodeFlowType::JUMP },
            { 40U, 12U, AsmCodeFlowType::CJUMP },
            { 44U, 12U, AsmCodeFlowType::CJUMP },
            { 56U, 12U, AsmCodeFlowType::CALL },
            { 60U, 80U, AsmCodeFlowType::CALL },
            { 64U, 80U, AsmCodeFlowType::CJUMP },
        }, true, ""
    },
};

struct AsmKernelData
{
    const char* name;
    Array<std::pair<size_t, size_t> > codeRegions;
};

struct AsmKernelRegionsCase
{
    const char* input;
    Array<AsmKernelData> kernels;
    bool good;
    const char* errorMessages;
};

static const AsmKernelRegionsCase kernelRegionsTestCases1Tbl[] =
{
    {   /* simple */
        R"ffDXD(.gallium
.kernel ala
.kernel beata
.kernel celina
.text
ala:
        s_mov_b32 s5, s1
.p2align 8
beata:
        s_mov_b32 s5, s1
.p2align 8
celina:
        s_mov_b32 s5, s1
)ffDXD",
        {
            { "ala",
                { { 0, 256 }
                } },
            { "beata",
                { { 256, 512 }
                } },
            { "celina",
                { { 512, 516 }
                } }
        }, true, ""
    },
    {   /* kcode */
        R"ffDXD(.gallium
.kernel ala
.kernel beata
.kernel celina
.text
ala:
        s_mov_b32 s5, s1
.p2align 8
beata:
        s_mov_b32 s5, s1
.p2align 8
celina:
        s_mov_b32 s5, s1
.p2align 8
.kcode ala, beata
        v_mov_b32 v3, v4
.kcode -ala
        v_mov_b32 v3, v4
.kcode celina
        v_mov_b32 v3, v5
.kcodeend
        s_mov_b32 s0, 0
.kcodeend
        s_mov_b32 s0, 0
.kcodeend
)ffDXD",
        {
            { "ala",
                { { 0, 256 }, { 768, 772 }, { 784, 788 }
                } },
            { "beata",
                { { 256, 512 }, { 768, 788 }
                } },
            { "celina",
                { { 512, 768 }, { 776, 780 }
                } }
        }, true, ""
    },
    {   /* kcode 2 */
        R"ffDXD(.gallium
.kernel ala
.kernel beata
.kernel celina
.kernel dorota
.text
ala:
        s_mov_b32 s5, s1
.p2align 8
beata:
        s_mov_b32 s5, s1

.kcode ala,celina
        s_mov_b32 s5, s56
        s_mov_b32 s15, s6
.kcodeend
        s_mov_b32 s5, s1
.kcode ala,celina
        s_mov_b32 s5, s56
    .kcode -celina,ala,dorota
        v_add_f32 v5,v1,v3
        .kcode -dorota,-ala,-celina
        v_add_f32 v5,v1,v6
        .kcodeend
        v_add_f32 v5,v1,v8
    .kcodeend
        s_mov_b32 s15, s6
.kcodeend

.p2align 8
celina:
        s_mov_b32 s5, s1
.p2align 8
dorota:
        s_mov_b32 s5, s1
.p2align 8
.kcode ala, beata
        v_mov_b32 v3, v4
.kcode -ala
        v_mov_b32 v3, v4
.kcode celina
        v_mov_b32 v3, v5
.kcodeend
        s_mov_b32 s0, 0
.kcodeend
        s_mov_b32 s0, 0
.kcodeend
)ffDXD",
        {
            { "ala",
                { { 0, 256 }, { 260, 268 }, { 272, 280 }, { 284, 292 },
                  { 1024, 1028 }, { 1040, 1044 }
                } },
            { "beata",
                { { 256, 260 }, { 268, 272 }, { 292, 512 }, { 1024, 1044 }
                } },
            { "celina",
                { { 260, 268 }, { 272, 276 }, { 288, 292 }, { 512, 768 }, { 1032, 1036 }
                } },
            { "dorota",
                { { 276, 280 }, { 284, 288 }, { 768, 1024 }
                } }
        }, true, ""
    }
};

static void testAsmCodeFlow(cxuint i, const AsmCodeFlowCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    
    std::ostringstream oss;
    oss << " testAsmCodeFlowCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testAsmCodeFlow", testCaseName+".good",
                      testCase.good, good);
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testAsmCodeFlowCase#" << i;
        throw Exception(oss.str());
    }
    std::vector<AsmCodeFlowEntry> resultCFlow = assembler.getSections()[0].codeFlow;
    /* sort codeflow by value (first in offset) */
    std::sort(resultCFlow.begin(), resultCFlow.end(),
              [](const AsmCodeFlowEntry& e1, const AsmCodeFlowEntry& e2)
              { return e1.offset < e2.offset || (e1.offset==e2.offset &&
                          (e1.target<e2.target ||
                              (e1.target==e2.target && e1.type<e2.type))); });
    
    assertValue("testAsmCodeFlow", testCaseName+".codeFlowSize",
                testCase.codeFlow.size(), resultCFlow.size());
    
    for (size_t j = 0; j < testCase.codeFlow.size(); j++)
    {
        const AsmCodeFlowEntry& expEntry = testCase.codeFlow[j];
        const AsmCodeFlowEntry& resultEntry = resultCFlow[j];
        std::ostringstream cfOss;
        cfOss << ".cflow#" << j << ".";
        cfOss.flush();
        std::string cflowName(cfOss.str());
        assertValue("testAsmCodeFlow", testCaseName+cflowName+"offset",
                    expEntry.offset, resultEntry.offset);
        assertValue("testAsmCodeFlow", testCaseName+cflowName+"target",
                    expEntry.target, resultEntry.target);
        assertValue("testAsmCodeFlow", testCaseName+cflowName+"type",
                    cxuint(expEntry.type), cxuint(resultEntry.type));
    }
    assertString("testAsmCodeFlow", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
}

static void testAsmKernelRegions(cxuint i, const AsmKernelRegionsCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    assembler.setLLVMVersion(1); // force old llvm version
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << " testAsmKernelRegionsCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testAsmKernelRegions", testCaseName+".good",
                      testCase.good, good);
    const std::vector<AsmKernel>& resultKernels = assembler.getKernels();
    assertValue("testAsmKernelRegions", testCaseName+".codeFlowSize",
                testCase.kernels.size(), resultKernels.size());
    
    for (size_t j = 0; j < testCase.kernels.size(); j++)
    {
        std::ostringstream kernelOss;
        kernelOss << ".kernel#" << j << ".";
        kernelOss.flush();
        std::string kname(kernelOss.str());
        const AsmKernelData& expKernel = testCase.kernels[j];
        const AsmKernel& resultKernel = resultKernels[j];
        assertString("testAsmKernelRegions", testCaseName+kname+"name",
                     expKernel.name, resultKernel.name);
        
        assertValue("testAsmKernelRegions", testCaseName+kname+"regionsSize",
                     expKernel.codeRegions.size(), resultKernel.codeRegions.size());
        
        for (size_t k = 0; k < expKernel.codeRegions.size(); k++)
        {
            std::ostringstream regionOss;
            regionOss << "region#" << k << ".";
            regionOss.flush();
            std::string rname(regionOss.str());
            std::pair<size_t, size_t> expRegion = expKernel.codeRegions[k];
            std::pair<size_t, size_t> resRegion = resultKernel.codeRegions[k];
            assertValue("testAsmKernelRegions", testCaseName+kname+rname+"first",
                     expRegion.first, resRegion.first);
            assertValue("testAsmKernelRegions", testCaseName+kname+rname+"second",
                     expRegion.second, resRegion.second);
        }
    }
    
    assertString("testAsmKernelRegions", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(codeFlowTestCases1Tbl)/sizeof(AsmCodeFlowCase); i++)
        try
        { testAsmCodeFlow(i, codeFlowTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (size_t i = 0; i < sizeof(kernelRegionsTestCases1Tbl) /
                sizeof(AsmKernelRegionsCase); i++)
        try
        { testAsmKernelRegions(i, kernelRegionsTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
