/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2017 Mateusz Szpakowski
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
            .cf_end
            )ffDXD",
        {
            { 20U, 8U, AsmCodeFlowType::JUMP },
            { 20U, 12U, AsmCodeFlowType::JUMP },
            { 20U, 36U, AsmCodeFlowType::JUMP },
            { 32U, 40U, AsmCodeFlowType::CALL },
            { 32U, 12U, AsmCodeFlowType::CALL },
            { 32U, 36U, AsmCodeFlowType::CALL },
            { 36U, 8U, AsmCodeFlowType::CJUMP },
            { 36U, 40U, AsmCodeFlowType::CJUMP },
            { 36U, 36U, AsmCodeFlowType::CJUMP },
            { 44U, 0U, AsmCodeFlowType::RETURN },
            { 52U, 0U, AsmCodeFlowType::START },
            { 64U, 0U, AsmCodeFlowType::END }
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
    assertValue<bool>("testAsmCodeFlowCase", testCaseName+".good",
                      testCase.good, good);
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testAsmCodeFlowCase#" << i;
        throw Exception(oss.str());
    }
    const std::vector<AsmCodeFlowEntry>& resultCFlow = assembler.getSections()[0].codeFlow;
    assertValue("testAsmCodeFlowCase", testCaseName+".codeFlowSize",
                testCase.codeFlow.size(), resultCFlow.size());
    for (size_t j = 0; j < testCase.codeFlow.size(); j++)
    {
        const AsmCodeFlowEntry& expEntry = testCase.codeFlow[j];
        const AsmCodeFlowEntry& resultEntry = resultCFlow[j];
        std::ostringstream cfOss;
        cfOss << ".cflow#" << j << ".";
        cfOss.flush();
        std::string cflowName(cfOss.str());
        assertValue("testAsmCodeFlowCase", testCaseName+cflowName+"offset",
                    expEntry.offset, resultEntry.offset);
        assertValue("testAsmCodeFlowCase", testCaseName+cflowName+"target",
                    expEntry.target, resultEntry.target);
        assertValue("testAsmCodeFlowCase", testCaseName+cflowName+"type",
                    cxuint(expEntry.type), cxuint(resultEntry.type));
    }
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
    return retVal;
}
