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
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
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
            .cf_start       # empty block, ignored
            .cf_end
            .cf_start
            v_xor_b32 v3, v9, v8
            v_xor_b32 v1, v5, v8
            v_xor_b32 v3, v9, v8
            v_or_b32 v3, v9, v8
            v_or_b32 v3, v9, v8
            s_endpgm
            .cf_end
            .cf_start
            v_or_b32 v3, v9, v8
            .cf_end
            .cf_start   # empty block, ignored
            .cf_end
)ffDXD",
        {
            { 0, 8, { }, false, false, true },
            { 64, 84, { }, false, false, true },
            { 128, 152, { }, false, false, true },
            { 152, 156, { }, false, false, true }
        },
        true, ""
    },
    {   // 11 - different type of jumps
        R"ffDXD(
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            .cf_jump j1, j4
            .cf_cjump j2, j3
            s_setpc_b64 s[0:1]
            
            v_mov_b32 v5, v3
            .cf_jump j1, j4
            .cf_cjump j2, j3
            s_setpc_b64 s[0:1]
            .cf_end
            
j1:         v_xor_b32 v3, v9, v8    # 20
            v_xor_b32 v3, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j2:         v_xor_b32 v3, v9, v8    # 36
            v_xor_b32 v1, v5, v8
            v_and_b32 v2, v9, v8
            v_xor_b32 v3, v9, v8
            s_branch b0

j3:         v_xor_b32 v3, v9, v8    # 56
            v_xor_b32 v1, v5, v8
            s_branch b0

j4:         v_xor_b32 v3, v9, v8    # 68
            v_xor_b32 v1, v5, v8
            v_xor_b32 v1, v5, v8
b0:         s_sub_u32 s0, s1, s2    # 80
            s_endpgm
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 2, false }, { 3, false },
                  { 4, false }, { 5, false } },
                false, false, false },
            { 12, 20,
                { { 2, false }, { 3, false }, { 4, false }, { 5, false } },
                false, false, true }, // have cf_end, force haveEnd
            { 20, 36, { { 6, false } }, false, false, true },
            { 36, 56, { { 6, false } }, false, false, true },
            { 56, 68, { { 6, false } }, false, false, true },
            { 68, 80, { }, false, false, false },
            { 80, 88, { }, false, false, true }
        },
        true, ""
    },
    {   // 12 - many jumps to same place
        R"ffDXD(
            v_mac_f32 v6, v9, v8    # 0
            v_xor_b32 v3, v9, v8
            s_cbranch_vccz j1
            v_mov_b32 v4, v3        # 12
            v_mov_b32 v4, v3
            s_cbranch_execz j1
            v_mov_b32 v4, v3        # 24
            v_mov_b32 v4, v3
            s_cbranch_execnz b0
            s_branch j2             # 36
b0:         v_mov_b32 v4, v3        # 40
            v_mov_b32 v5, v3
            v_mov_b32 v5, v4
            s_branch j2
j1:         v_max_u32 v4, v1, v6    # 56
            s_endpgm
j2:         v_min_i32 v4, v1, v6    # 64
            v_min_i32 v3, v1, v6
            .cf_call c1
            s_swappc_b64 s[0:1], s[0:1]
            v_min_i32 v3, v1, v6    # 76
            .cf_call c1
            s_swappc_b64 s[0:1], s[0:1]
            v_min_i32 v3, v1, v6    # 84
            s_endpgm
c1:         v_add_f32 v6, v1, v4    # 92
            v_add_f32 v6, v1, v4
            .cf_ret
            s_swappc_b64 s[0:1], s[0:1]
)ffDXD",
        {
            { 0, 12,
                { { 1, false }, { 5, false } },
                false, false, false },
            { 12, 24,
                { { 2, false }, { 5, false } },
                false, false, false },
            { 24, 36,
                { { 3, false }, { 4, false } },
                false, false, false },
            { 36, 40, { { 6, false } }, false, false, true },
            { 40, 56, { { 6, false } }, false, false, true },
            // 5
            { 56, 64, { }, false, false, true },
            { 64, 76,
                { { 7, false }, { 9, true } },
                true, false, false },
            { 76, 84,
                { { 8, false }, { 9, true } },
                true, false, false },
            { 84, 92, { }, false, false, true },
            // 9 - subroutine
            { 92, 104, { }, false, true, true }
        },
        true, ""
    },
    {   /* 13 - subroutines 2 (one is jump) */
        R"ffDXD(
        v_mov_b32 v1, v2
        v_mov_b32 v1, v3
        .cf_call l0, l1
        .cf_jump l2
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
                { { 1, false }, { 5, false }, { 3, true }, { 4, true } },
                true, false, true },
            { 12, 20, { { 6, false } }, false, false, true },
            { 20, 32, { }, false, false, true },
            { 32, 44, { }, false, true, true },
            { 44, 60, { }, false, true, true },
            { 60, 84, { }, false, true, true },
            { 84, 96, { { 2, false } }, false, false, true }
        },
        true, ""
    },
};

static void testCreateCodeStructure(cxuint i, const AsmCodeStructCase& testCase)
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

typedef AsmRegAllocator::SSAInfo SSAInfo;
typedef AsmRegAllocator::SSAReplace SSAReplace;
typedef AsmRegAllocator::SSAReplacesMap SSAReplacesMap;

struct TestSingleVReg
{
    CString name;
    uint16_t index;
    
    bool operator==(const TestSingleVReg& b) const
    { return name==b.name && index==b.index; }
    bool operator!=(const TestSingleVReg& b) const
    { return name!=b.name || index!=b.index; }
    bool operator<(const TestSingleVReg& b) const
    { return name<b.name || (name==b.name && index<b.index); }
};

std::ostream& operator<<(std::ostream& os, const TestSingleVReg& vreg)
{
    if (!vreg.name.empty())
        os << vreg.name << "[" << vreg.index << "]";
    else
        os << "@REG[" << vreg.index << "]";
    return os;
}

struct CCodeBlock2
{
    size_t start, end; // place in code
    Array<NextBlock> nexts; ///< nexts blocks
    Array<std::pair<TestSingleVReg, SSAInfo> > ssaInfos;
    bool haveCalls;
    bool haveReturn;
    bool haveEnd;
};

struct AsmSSADataCase
{
    const char* input;
    Array<CCodeBlock2> codeBlocks;
    Array<std::pair<TestSingleVReg, Array<SSAReplace> > > ssaReplaces;
    bool good;
    const char* errorMessages;
};

static const AsmSSADataCase ssaDataTestCases1Tbl[] =
{
    {   /* 0 - simple */
        ".regvar sa:s:8, va:v:10\n"
        "s_mov_b32 sa[4], sa[2]\n"
        "s_add_u32 sa[4], sa[2], s3\n"
        "ds_read_b64 va[4:5], v0\n"
        "v_add_f64 va[0:1], va[4:5], va[2:3]\n"
        "v_mac_f32 va[0], va[4], va[5]\n"  // ignore this write, because also read
        "v_mul_f32 va[1], va[4], va[5]\n"
        "ds_read_b32 v10, v0\n"
        "v_mul_lo_u32 v10, va[2], va[3]\n"
        "v_mul_lo_u32 v10, va[2], va[3]\n",
        {
            { 0, 56, { },
                {
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 266 }, SSAInfo(0, 0, 0, 0, 0, false) },
                    { { "sa", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(SIZE_MAX, 0, 0, 1, 2, false) },
                    { { "va", 0 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 1 }, SSAInfo(SIZE_MAX, 0, 0, 1, 2, false) },
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 4 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 5 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) }
                }, false, false, true }
        },
        { },
        true, ""
    },
    {   /* 1 - tree */
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[2], s3
        ds_read_b64 va[4:5], v0
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        .cf_jump j1,j2,j3
        s_setpc_b64 s[0:1]
        # 44
j1:     v_add_f32 va[5], va[2], va[3]
        v_mov_b32 va[6], va[2]
        s_endpgm
j2:     v_add_f32 va[3], va[2], va[5]
        v_add_f32 va[6], va[2], va[3]
        s_endpgm
j3:     v_add_f32 va[2], va[5], va[3]
        s_endpgm
)ffDXD",
        {
            { 0, 44,
                { { 1, false }, { 2, false }, { 3, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(SIZE_MAX, 0, 0, 1, 2, false) },
                    { { "va", 0 }, SSAInfo(SIZE_MAX, 0, 0, 2, 3, false) },
                    { { "va", 1 }, SSAInfo(SIZE_MAX, 0, 0, 2, 3, false) },
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 4 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 5 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) }
                }, false, false, true },
            { 44, 56, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 6 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) }
                }, false, false, true },
            { 56, 68, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 6 }, SSAInfo(SIZE_MAX, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 68, 76, { },
                {
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true },
        },
        { },
        true, ""
    },
    {   /* 2 - tree (align) */
        R"ffDXD(.regvar sa:s:8, va:v:10
        s_mov_b32 sa[4], sa[2]  # 0
        s_add_u32 sa[4], sa[2], s3
        ds_read_b64 va[4:5], v0
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        v_add_f64 va[0:1], va[4:5], va[2:3]
        .cf_jump j1,j2,j3
        s_setpc_b64 s[0:1]
.p2align 6
        # 64
j1:     v_add_f32 va[5], va[2], va[3]
        v_mov_b32 va[6], va[2]
        s_endpgm
j2:     v_add_f32 va[3], va[2], va[5]
        v_add_f32 va[6], va[2], va[3]
        s_endpgm
j3:     v_add_f32 va[2], va[5], va[3]
        s_endpgm
)ffDXD",
        {
            { 0, 44,
                { { 1, false }, { 2, false }, { 3, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 3 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 256 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "sa", 4 }, SSAInfo(SIZE_MAX, 0, 0, 1, 2, false) },
                    { { "va", 0 }, SSAInfo(SIZE_MAX, 0, 0, 2, 3, false) },
                    { { "va", 1 }, SSAInfo(SIZE_MAX, 0, 0, 2, 3, false) },
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 4 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 5 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) }
                }, false, false, true },
            { 64, 76, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 6 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) }
                }, false, false, true },
            { 76, 88, { },
                {
                    { { "va", 2 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 5 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 6 }, SSAInfo(SIZE_MAX, 1, 1, 1, 1, false) }
                }, false, false, true },
            { 88, 96, { },
                {
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(0, SIZE_MAX, 2, SIZE_MAX, 0, true) }
                }, false, false, true },
        },
        { },
        true, ""
    }
#if 0
    ,
    {   /* 3 - longer tree (more blocks) */
        R"ffDXD(.regvar sa:s:8, va:v:12, vb:v:10
        # 0
        ds_read_b64 v[2:3], v0
        ds_read_b64 v[4:5], v0
        v_mov_b32 va[0], v1
        v_mul_f32 va[1], v0, v1
        .cf_cjump tx1, tx2, tx3
        s_setpc_b64 s[0:1]
        # 28
        v_mul_lo_u32 va[2], va[0], va[1]
        v_mul_hi_u32 va[3], va[0], va[1]
        v_lshrrev_b32 va[4], v2, va[5]
        .cf_cjump ux1, ux2, ux3
        s_setpc_b64 s[0:1]
        # 52
        v_mul_u32_u24 va[2], va[0], va[1]
        v_mul_hi_u32_u24 va[3], va[0], va[1]
        v_lshlrev_b32 va[5], v3, va[4]
        .cf_cjump vx1, vx2, vx3
        s_setpc_b64 s[0:1]
        # 68
        v_mul_lo_i32 va[2], va[0], va[1]
        v_mul_hi_i32 va[3], va[0], va[1]
        v_ashrrev_i32 va[6], v4, va[4]
        .cf_jump wx1, wx2, wx3
        s_setpc_b64 s[0:1]
        
.p2align 4
        # 96
tx1:    v_min_f32 vb[0], va[0], va[1]
        v_madak_f32 vb[5], vb[0], va[1], 2.5
ux1:    v_nop
vx1:    v_add_f32 va[9], v11, vb[4]
wx1:    v_add_f32 va[10], v11, vb[4]
        s_endpgm
.p2align 4
tx2:    v_max_f32 vb[1], va[0], va[1]
        v_madak_f32 va[3], va[0], va[1], 2.5
ux2:    v_nop
        s_nop 7
vx2:    v_nop
wx2:    v_add_f32 va[8], v19, vb[5]
        s_endpgm
.p2align 4
tx3:    v_max_u32 vb[2], va[0], va[1]
        v_madmk_f32 vb[3], va[0], 1.23, vb[1]
ux3:    v_add_f32 va[7], v11, vb[5]
vx3:    v_add_f32 va[6], v13, vb[5]
wx3:    v_nop
        s_endpgm
)ffDXD",
        {
            { 0, 28,
                { { 1, false }, { 4, false }, { 8, false }, { 12, false } },
                {
                    { { "", 0 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 1 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 256 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 256+1 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 256+2 }, SSAInfo(SIZE_MAX, 0, 0, SIZE_MAX, 0, false) },
                    { { "", 256+3 }, SSAInfo(SIZE_MAX, 0, 0, SIZE_MAX, 0, false) },
                    { { "", 256+4 }, SSAInfo(SIZE_MAX, 0, 0, SIZE_MAX, 0, false) },
                    { { "", 256+5 }, SSAInfo(SIZE_MAX, 0, 0, SIZE_MAX, 0, false) },
                    { { "va", 0 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 1 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                }, false, false, false },
            { 28, 52,
                { { 2, false }, { 5, false }, { 9, false }, { 13, false } },
                {
                    { { "", 0 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 1 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 256+2 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "va", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 3 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 4 }, SSAInfo(SIZE_MAX, 0, 0, 0, 1, false) },
                    { { "va", 5 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            { 52, 68,
                { { 3, false }, { 6, false }, { 10, false }, { 14, false } },
                {
                    { { "", 0 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 1 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 256+3, }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "va", 0 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 1 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 3 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "va", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) },
                    { { "va", 5 }, SSAInfo(0, 1, 1, 1, 1, false) },
                }, false, false, false },
            { 68, 92,
                { { 7, false }, { 11, false }, { 15, false } },
                {
                    { { "", 0 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                    { { "", 1 }, SSAInfo(0, SIZE_MAX, 0, SIZE_MAX, 0, true) },
                }, false, false, false },
            // 4-7:
            { 96, 108,
                { },
                { }, false, false, false },
            { 108, 112,
                { },
                { }, false, false, false },
            { 112, 116,
                { },
                { }, false, false, false },
            { 116, 124,
                { },
                { }, false, false, true },
            // 8-11
            { 128, 140,
                { },
                { }, false, false, false },
            { 140, 148,
                { },
                { }, false, false, false },
            { 148, 152,
                { },
                { }, false, false, false },
            { 152, 160,
                { },
                { }, false, false, true },
            // 12-15
            { 160, 172,
                { },
                { }, false, false, false },
            { 172, 176,
                { },
                { }, false, false, false },
            { 176, 180,
                { },
                { }, false, false, false },
            { 180, 188,
                { },
                { }, false, false, true }
        },
        { },
        true, ""
    }
#endif
};

static TestSingleVReg getTestSingleVReg(const AsmSingleVReg& vr,
        const std::unordered_map<const AsmRegVar*, CString>& rvMap)
{
    if (vr.regVar == nullptr)
        return { "", vr.index };
    
    auto it = rvMap.find(vr.regVar);
    if (it == rvMap.end())
        throw Exception("getTestSingleVReg: RegVar not found!!");
    return { it->second, vr.index };
}

static void testCreateSSAData(cxuint i, const AsmSSADataCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, (ASM_ALL&~ASM_ALTMACRO) | ASM_TESTRUN,
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
    regAlloc.createSSAData(*section.usageHandler);
    const std::vector<CodeBlock>& resCodeBlocks = regAlloc.getCodeBlocks();
    std::ostringstream oss;
    oss << " testAsmSSADataCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testAsmSSAData", testCaseName+".good",
                      testCase.good, good);
    assertString("testAsmSSAData", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
    
    assertValue("testAsmSSAData", testCaseName+".codeBlocks.size",
                testCase.codeBlocks.size(), resCodeBlocks.size());
    
    std::unordered_map<const AsmRegVar*, CString> regVarNamesMap;
    for (const auto& rvEntry: assembler.getRegVarMap())
        regVarNamesMap.insert(std::make_pair(&rvEntry.second, rvEntry.first));
    
    for (size_t j = 0; j < resCodeBlocks.size(); j++)
    {
        const CCodeBlock2& expCBlock = testCase.codeBlocks[j];
        const CodeBlock& resCBlock = resCodeBlocks[j];
        std::ostringstream codeBlockOss;
        codeBlockOss << ".codeBlock#" << j << ".";
        codeBlockOss.flush();
        std::string cbname(codeBlockOss.str());
        assertValue("testAsmSSAData", testCaseName + cbname + "start",
                    expCBlock.start, resCBlock.start);
        assertValue("testAsmSSAData", testCaseName + cbname + "end",
                    expCBlock.end, resCBlock.end);
        assertValue("testAsmSSAData", testCaseName + cbname + "nextsSize",
                    expCBlock.nexts.size(), resCBlock.nexts.size());
        
        for (size_t k = 0; k < expCBlock.nexts.size(); k++)
        {
            std::ostringstream nextOss;
            nextOss << "next#" << k << ".";
            nextOss.flush();
            std::string nname(nextOss.str());
            
            assertValue("testAsmSSAData", testCaseName + cbname + nname + "block",
                    expCBlock.nexts[k].block, resCBlock.nexts[k].block);
            assertValue("testAsmSSAData", testCaseName + cbname + nname + "isCall",
                    int(expCBlock.nexts[k].isCall), int(resCBlock.nexts[k].isCall));
        }
        
        assertValue("testAsmSSAData", testCaseName + cbname + "ssaInfoSize",
                    expCBlock.ssaInfos.size(), resCBlock.ssaInfoMap.size());
        
        Array<std::pair<TestSingleVReg, SSAInfo> > resSSAInfos(
                        resCBlock.ssaInfoMap.size());
        std::transform(resCBlock.ssaInfoMap.begin(), resCBlock.ssaInfoMap.end(),
            resSSAInfos.begin(),
            [&regVarNamesMap](const std::pair<AsmSingleVReg, SSAInfo>& a)
            -> std::pair<TestSingleVReg, SSAInfo>
            { return { getTestSingleVReg(a.first, regVarNamesMap), a.second }; });
        mapSort(resSSAInfos.begin(), resSSAInfos.end());
        
        for (size_t k = 0; k < expCBlock.ssaInfos.size(); k++)
        {
            std::ostringstream ssaOss;
            ssaOss << "ssaInfo#" << k << ".";
            ssaOss.flush();
            std::string ssaName(ssaOss.str());
            
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName + "svreg",
                        expCBlock.ssaInfos[k].first, resSSAInfos[k].first);
            const SSAInfo& expSInfo = expCBlock.ssaInfos[k].second;
            const SSAInfo& resSInfo = resSSAInfos[k].second;
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName + "ssaIdBefore",
                        expSInfo.ssaIdBefore, resSInfo.ssaIdBefore);
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName + "ssaId",
                        expSInfo.ssaId, resSInfo.ssaId);
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName + "ssaIdChange",
                        expSInfo.ssaIdChange, resSInfo.ssaIdChange);
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName + "ssaIdFirst",
                        expSInfo.ssaIdFirst, resSInfo.ssaIdFirst);
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName + "ssaIdLast",
                        expSInfo.ssaIdLast, resSInfo.ssaIdLast);
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName +
                        "readBeforeWrite", int(expSInfo.readBeforeWrite),
                        int(resSInfo.readBeforeWrite));
        }
        
        assertValue("testAsmSSAData", testCaseName + cbname + "haveCalls",
                    int(expCBlock.haveCalls), int(resCBlock.haveCalls));
        assertValue("testAsmSSAData", testCaseName + cbname + "haveReturn",
                    int(expCBlock.haveReturn), int(resCBlock.haveReturn));
        assertValue("testAsmSSAData", testCaseName + cbname + "haveEnd",
                    int(expCBlock.haveEnd), int(resCBlock.haveEnd));
    }
    
    const SSAReplacesMap& ssaReplacesMap = regAlloc.getSSAReplacesMap();
    assertValue("testAsmSSAData", testCaseName + "ssaReplacesSize",
                    testCase.ssaReplaces.size(), ssaReplacesMap.size());
    Array<std::pair<TestSingleVReg, Array<SSAReplace> > > resSSAReplaces;
    std::transform(ssaReplacesMap.begin(), ssaReplacesMap.end(),
            resSSAReplaces.begin(),
            [&regVarNamesMap](const std::pair<AsmSingleVReg, std::vector<SSAReplace> >& a)
            -> std::pair<TestSingleVReg, Array<SSAReplace> >
            { return { getTestSingleVReg(a.first, regVarNamesMap), 
                Array<SSAReplace>(a.second.begin(), a.second.end()) }; });
    mapSort(resSSAReplaces.begin(), resSSAReplaces.end());
    
    for (size_t j = 0; j < testCase.ssaReplaces.size(); j++)
    {
        std::ostringstream ssaOss;
        ssaOss << "ssas" << j << ".";
        ssaOss.flush();
        std::string ssaName(ssaOss.str());
        
        const auto& expEntry = testCase.ssaReplaces[j];
        const auto& resEntry = resSSAReplaces[j];
        
        assertValue("testAsmSSAData", testCaseName + ssaName + "svreg",
                        expEntry.first, resEntry.first);
        assertValue("testAsmSSAData", testCaseName + ssaName + "replacesSize",
                        expEntry.second.size(), resEntry.second.size());
        for (size_t k = 0; k < expEntry.second.size(); k++)
        {
            std::ostringstream repOss;
            repOss << "replace#" << k << ".";
            repOss.flush();
            std::string repName(repOss.str());
            
            assertValue("testAsmSSAData", testCaseName + ssaName + repName + "first",
                        expEntry.second[k].first, resEntry.second[k].first);
            assertValue("testAsmSSAData", testCaseName + ssaName + repName + "second",
                        expEntry.second[k].second, resEntry.second[k].second);
        }
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(codeStructTestCases1Tbl)/sizeof(AsmCodeStructCase); i++)
        try
        { testCreateCodeStructure(i, codeStructTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (size_t i = 0; i < sizeof(ssaDataTestCases1Tbl)/sizeof(AsmSSADataCase); i++)
        try
        { testCreateSSAData(i, ssaDataTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
