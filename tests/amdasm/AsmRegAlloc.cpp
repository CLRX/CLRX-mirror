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
#include <vector>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/Containers.h>
#include "../TestUtils.h"

using namespace CLRX;

typedef AsmRegAllocator::CodeBlock CodeBlock;
typedef AsmRegAllocator::NextBlock NextBlock;

struct CCodeBlock
{
    size_t start, end; // place in code
    Array<NextBlock> nexts; ///< nexts blocks
    bool haveCalls;
    bool haveReturn;
    bool haveEnd;
};

struct AsmCodeStructCase
{
    const char* input;
    Array<CCodeBlock> codeBlocks;
    bool good;
    const char* errorMessages;
};

static const AsmCodeStructCase codeStructTestCases1Tbl[] =
{
    {   // 0
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n",
        {
            { 0, 8, { }, false, false, true }
        },
        true, ""
    },
    {   // 1
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        ".cf_end\n"
        ".cf_start\n"
        "v_mov_b32 v4, v2\n"
        "v_mov_b32 v8, v3\n"
        "v_mov_b32 v8, v3\n"
        ".cf_end\n",
        {
            { 0, 8, { }, false, false, true },
            { 8, 20, { }, false, false, true }
        },
        true, ""
    },
    {   // 2 - ignore cf_start to cf_end (except first)
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        ".cf_start\n"
        "v_mov_b32 v4, v2\n"
        "v_mov_b32 v8, v3\n"
        ".cf_start\n"
        "v_mov_b32 v8, v3\n"
        ".cf_end\n"
        ".cf_start\n"
        "v_and_b32 v6, v1, v2\n"
        ".cf_start\n"
        "v_and_b32 v9, v1, v3\n"
        ".cf_end\n",
        {
            { 0, 20, { }, false, false, true },
            { 20, 28, { }, false, false, true }
        },
        true, ""
    },
    {   // 3 - ignore cf_end
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        ".cf_end\n"
        "v_mov_b32 v4, v2\n"
        "v_mov_b32 v8, v3\n"
        "v_mov_b32 v8, v3\n"
        ".cf_end\n"
        ".cf_start\n"
        "v_and_b32 v9, v1, v3\n"
        ".cf_end\n",
        {
            { 0, 8, { }, false, false, true },
            { 20, 24, { }, false, false, true }
        },
        true, ""
    },
    {   /* 4 - cond jump */
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        "label2:\n"
        "v_mov_b32 v1, v3\n"
        "v_mov_b32 v1, v3\n"
        "s_cbranch_vccz label2\n"
        "s_xor_b32 s3, s5, s8\n"
        "s_endpgm\n",
        {
            { 0, 8, { }, false, false, false },
            { 8, 20,
              { { 1, false }, { 2, false } },
              false, false, false },
            { 20, 28, { }, false, false, true }
        },
        true, ""
    },
    {   /* 5 - jump */
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        "label2:\n"
        "v_mov_b32 v1, v3\n"
        "v_mov_b32 v1, v3\n"
        "s_branch label2\n"
        "s_xor_b32 s3, s5, s8\n"
        "s_endpgm\n",
        {
            { 0, 8, { }, false, false, false },
            { 8, 20,
              { { 1, false } },
              false, false, true }
        },
        true, ""
    },
    {
        /* 6 - jump */
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n"
        "lstart:\n"
        ".cf_jump l0, l1, l2\n"
        "s_setpc_b64 s[0:1]\n"
        "l0:\n"
        "v_add_f32 v1, v2, v3\n"
        "s_branch lend\n"
        "l1:\n"
        "v_add_f32 v1, v2, v3\n"
        "s_branch lend\n"
        "l2:\n"
        "v_add_f32 v1, v2, v3\n"
        "s_branch lend\n"
        "lend:\n"
        "s_cbranch_vccz lstart\n"
        "s_endpgm\n",
        {
            { 0, 8, { }, false, false, false },
            { 8, 12,
              { { 2, false }, { 3, false }, { 4, false } },
              false, false, true },
            { 12, 20,
              { { 5, false } },
              false, false, true },
            { 20, 28,
              { { 5, false } },
              false, false, true },
            { 28, 36,
              { { 5, false } },
              false, false, true },
            { 36, 40,
              { { 1, false }, { 6, false } },
              false, false, false },
            { 40, 44, { }, false, false, true }
        },
        true, ""
    },
    {   /* 7 - subroutines */
        R"ffDXD(
        v_mov_b32 v1, v2
        v_mov_b32 v1, v3
        .cf_call l0, l1, l2
        s_setpc_b64 s[0:1]
        v_sub_f32 v1, v3, v6    # 12
        s_branch j0
b0:     v_add_u32 v4, vcc, v6, v11  # 20
        v_mac_f32 v6, v6, v6
        s_endpgm
l0:     # 32
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
l1:     # 44
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
l2:     # 60
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        v_add_f32 v1, v2, v3
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
j0:     # 84
        v_lshrrev_b32 v4, 3, v2
        v_lshrrev_b32 v4, 3, v2
        s_branch b0
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 3, true }, { 4, true }, { 5, true } },
                true, false, false },
            { 12, 20, { { 6, false } }, false, false, true },
            { 20, 32, { }, false, false, true },
            { 32, 44, { }, false, true, true },
            { 44, 60, { }, false, true, true },
            { 60, 84, { }, false, true, true },
            { 84, 96, { { 2, false } }, false, false, true }
        },
        true, ""
    },
    {   /* 8 - subroutines 2 - more complex */
        R"ffDXD(
        v_mov_b32 v1, v2    # 0
        v_mov_b32 v1, v3
loop:   s_mov_b32 s5, s0    # 8
        .cf_call c1,c2,c3
        s_swappc_b64 s[0:1], s[0:1]
        v_and_b32 v4, v5, v4    # 16
        v_and_b32 v4, v5, v3
        v_and_b32 v4, v5, v4
        .cf_call c5,c6
        s_swappc_b64 s[0:1], s[0:1]
        v_and_b32 v4, v5, v3    # 32
        v_and_b32 v4, v5, v9
        s_add_i32 s3, s3, s1
        s_cbranch_vccnz loop
        v_mac_f32 v4, v6, v7    # 48
        s_endpgm
.p2align 4
c1:     v_xor_b32 v3, v4, v3    # 64
        v_xor_b32 v3, v4, v7
        v_xor_b32 v3, v4, v8
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
.p2align 4
c2:     v_xor_b32 v3, v4, v3    # 80
        v_xor_b32 v3, v4, v7
c5:     v_xor_b32 v3, v4, v8    # 88
        .cf_call c4
        s_swappc_b64 s[0:1], s[0:1]
        v_or_b32 v3, v11, v9    # 96
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
.p2align 4
c3:     v_xor_b32 v3, v4, v3    # 112
        v_xor_b32 v3, v4, v8
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
.p2align 4
c4:     v_xor_b32 v3, v4, v3    # 128
        v_xor_b32 v3, v9, v8
c6: loop2:
        v_xor_b32 v3, v9, v8    # 136
        v_xor_b32 v3, v9, v8
        s_add_i32 s4, 1, s34
        s_cbranch_vccz loop2
        v_lshlrev_b32 v3, 4, v2 # 152
        .cf_ret
        s_swappc_b64 s[0:1], s[0:1]
)ffDXD",
        {
            { 0, 8, { }, false, false, false },
            { 8, 16,
                { { 2, false }, { 5, true }, { 6, true }, { 9, true } },
                true, false, false },
            // 2
            { 16, 32,
                { { 3, false }, { 7, true }, { 11, true } },
                true, false, false },
            { 32, 48,
                { { 1, false }, { 4, false } },
                false, false, false },
            // 4
            { 48, 56, { }, false, false, true },
            // 5 - c1 subroutine
            { 64, 80, { }, false, true, true },
            // 6 - c2 subroutine
            { 80, 88, { }, false, false, false },
            { 88, 96, { { 8, false }, { 10, true } },
                true, false, false },
            { 96, 104, { }, false, true, true },
            // 9 - c3 subroutine
            { 112, 124, { }, false, true, true },
            // 10 - c4 subroutine
            { 128, 136, { }, false, false, false },
            { 136, 152,
                { { 11, false }, { 12, false } },
                false, false, false },
            { 152, 160, { }, false, true, true }
        },
        true, ""
    },
    {   // 9 - switch
        R"ffDXD(
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            .cf_jump j1, j2, j3, j4
            s_setpc_b64 s[0:1]
            
j1:         v_xor_b32 v3, v9, v8    # 12
            v_xor_b32 v3, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j2:         v_xor_b32 v3, v9, v8    # 28
            v_xor_b32 v1, v5, v8
            v_and_b32 v2, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j3:         v_xor_b32 v3, v9, v8    # 48
            v_xor_b32 v1, v5, v8
            s_branch b0

j4:         v_xor_b32 v3, v9, v8    # 60
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
b0:         s_sub_u32 s0, s1, s2    # 72
            s_endpgm
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 2, false }, { 3, false }, { 4, false } },
                false, false, true },
            { 12, 28, { { 5, false } }, false, false, true },
            { 28, 48, { { 5, false } }, false, false, true },
            { 48, 60, { { 5, false } }, false, false, true },
            { 60, 72, { }, false, false, false },
            { 72, 80, { }, false, false, true }
        },
        true, ""
    },
    {   // 10 - multiple kernels
        R"ffDXD(
            .cf_start
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            .cf_end
.p2align 6
            .cf_start
            v_xor_b32 v3, v9, v8
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
            s_endpgm
.p2align 6
            .cf_start
            v_xor_b32 v3, v9, v8
            v_xor_b32 v1, v5, v8
            v_xor_b32 v3, v9, v8
            v_or_b32 v3, v9, v8
            v_or_b32 v3, v9, v8
            s_endpgm
)ffDXD",
        {
            { 0, 8, { }, false, false, true },
            { 64, 84, { }, false, false, true },
            { 128, 152, { }, false, false, true }
        },
        true, ""
    }
};

static void testAsmCodeStructure(cxuint i, const AsmCodeStructCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testAsmCodeStructCase#" << i;
        throw Exception(oss.str());
    }
    const AsmSection& section = assembler.getSections()[0];
    
    AsmRegAllocator regAlloc(assembler);
    
    regAlloc.createCodeStructure(section.codeFlow, section.getSize(),
                            section.content.data());
    const std::vector<CodeBlock>& resCodeBlocks = regAlloc.getCodeBlocks();
    std::ostringstream oss;
    oss << " testAsmCodeStructCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testAsmCodeStruct", testCaseName+".good",
                      testCase.good, good);
    assertString("testAsmCodeStruct", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
    
    assertValue("testAsmCodeStruct", testCaseName+".codeBlocks.size",
                testCase.codeBlocks.size(), resCodeBlocks.size());
    
    for (size_t j = 0; j < resCodeBlocks.size(); j++)
    {
        const CCodeBlock& expCBlock = testCase.codeBlocks[j];
        const CodeBlock& resCBlock = resCodeBlocks[j];
        std::ostringstream codeBlockOss;
        codeBlockOss << ".codeBlock#" << j << ".";
        codeBlockOss.flush();
        std::string cbname(codeBlockOss.str());
        assertValue("testAsmCodeStruct", testCaseName + cbname + "start",
                    expCBlock.start, resCBlock.start);
        assertValue("testAsmCodeStruct", testCaseName + cbname + "end",
                    expCBlock.end, resCBlock.end);
        assertValue("testAsmCodeStruct", testCaseName + cbname + "nextsSize",
                    expCBlock.nexts.size(), resCBlock.nexts.size());
        
        for (size_t k = 0; k < expCBlock.nexts.size(); k++)
        {
            std::ostringstream nextOss;
            nextOss << "next#" << k << ".";
            nextOss.flush();
            std::string nname(nextOss.str());
            
            assertValue("testAsmCodeStruct", testCaseName + cbname + nname + "block",
                    expCBlock.nexts[k].block, resCBlock.nexts[k].block);
            assertValue("testAsmCodeStruct", testCaseName + cbname + nname + "isCall",
                    int(expCBlock.nexts[k].isCall), int(resCBlock.nexts[k].isCall));
        }
        
        assertValue("testAsmCodeStruct", testCaseName + cbname + "haveCalls",
                    int(expCBlock.haveCalls), int(resCBlock.haveCalls));
        assertValue("testAsmCodeStruct", testCaseName + cbname + "haveReturn",
                    int(expCBlock.haveReturn), int(resCBlock.haveReturn));
        assertValue("testAsmCodeStruct", testCaseName + cbname + "haveEnd",
                    int(expCBlock.haveEnd), int(resCBlock.haveEnd));
    }
}


int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(codeStructTestCases1Tbl)/sizeof(AsmCodeStructCase); i++)
        try
        { testAsmCodeStructure(i, codeStructTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
