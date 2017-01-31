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

struct AsmRegVarUsageData
{
    size_t offset;
    const char* regVarName;
    uint16_t rstart, rend;
    AsmRegField regField;
    cxbyte rwFlags;
    cxbyte align;
};

struct GCNRegVarUsageCase
{
    const char* input;
    Array<AsmRegVarUsageData> regVarUsages;
    bool good;
    const char* errorMessages;
};

static const GCNRegVarUsageCase gcnRvuTestCases1Tbl[] =
{
    {   /* SOP1 encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        "s_mov_b32 rax,rbx\n"
        ".space 12\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        ".space 134\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n",
        {
            // s_mov_b32 rax,rbx
            { 0, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 16, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 16, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 154, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 154, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
        },
        true, ""
    },
    {   /* SOP1 encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        ".space 200\n"
        "s_mov_b32 rax,rbx\n"
        ".space 12\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        ".space 134\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n",
        {
            // s_mov_b32 rax,rbx
            { 200, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 200, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 216, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 216, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 354, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 354, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
        },
        true, ""
    },
    {   /* SOP1 encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:6, rbx5:s:8\n"
        "s_mov_b32 rax,rbx\n"
        "s_mov_b32 rax4[2],rbx5[1]\n"
        "s_mov_b64 rax4[2:3],rbx5[1:2]\n"
        "s_ff1_i32_b64 rbx, rbx5[1:2]\n"
        "s_bitset0_b64 rbx5[3:4],rax\n"
        "s_getpc_b64 rax4[0:1]\n"
        "s_setpc_b64 rax4[2:3]\n"
        "s_cbranch_join rax4[2]\n"
        "s_movrels_b32 rax,rbx\n",
        {
            // s_mov_b32 rax,rbx
            { 0, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b32 rax4[2],rbx5[1]
            { 4, "rax4", 2, 3, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 4, "rbx5", 1, 2, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_mov_b64 rax4[2:3],rbx5[1:2]
            { 8, "rax4", 2, 4, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_ff1_i32_b64 rbx, rbx5[1:2]
            { 12, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 12, "rbx5", 1, 3, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_bitset0_b64 rbx5[3:4],rax
            { 16, "rbx5", 3, 5, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 16, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_getpc_b64 rax4[0:1]
            { 20, "rax4", 0, 2, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            // s_setpc_b64 rax4[2:3]
            { 24, "rax4", 2, 4, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            // s_cbranch_join rax4[2]
            { 28, "rax4", 2, 3, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            // s_movrels_b32 rax,rbx
            { 32, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 32, "rbx", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 }
        },
        true, ""
    },
    {   /* SOP2 encoding */
        ".regvar rax:s, rbx:s, rdx:s\n"
        ".regvar rax4:s:8, rbx5:s:8, rcx3:s:6\n"
        "s_and_b32 rdx, rax, rbx\n"
        "s_or_b32 rdx, s11, rbx\n"
        "s_xor_b64 rcx3[4:5], rax4[0:1], rbx5[2:3]\n"
        "s_cbranch_g_fork  rcx3[0:1], rax4[2:3]\n",
        {
            // s_and_b32 rdx, rax, rbx
            { 0, "rdx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_or_b32 rdx, s11, rbx
            { 4, "rdx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            { 4, nullptr, 11, 12, GCNFIELD_SSRC0, ASMRVU_READ, 0 },
            { 4, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_xor_b64 rcx3[4:5], rax4[0:1], rbx5[2:3]
            { 8, "rcx3", 4, 6, GCNFIELD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rax4", 0, 2, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 8, "rbx5", 2, 4, GCNFIELD_SSRC1, ASMRVU_READ, 2 },
            // s_cbranch_g_fork  rcx3[0:1], rax4[2:3]
            { 12, "rcx3", 0, 2, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 12, "rax4", 2, 4, GCNFIELD_SSRC1, ASMRVU_READ, 2 }
        },
        true, ""
    },
    {   /* SOPC encoding */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:4, rbx5:s:4\n"
        "s_cmp_ge_i32  rax, rbx\n"
        "s_bitcmp0_b64  rbx5[2:3], rax4[3]\n"
        "s_setvskip  rax, rbx5[2]\n",
        {
            // s_cmp_ge_i32  rax, rbx
            { 0, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_bitcmp0_b64  rbx5[2:3], rax[3]
            { 4, "rbx5", 2, 4, GCNFIELD_SSRC0, ASMRVU_READ, 2 },
            { 4, "rax4", 3, 4, GCNFIELD_SSRC1, ASMRVU_READ, 1 },
            // s_set_vskip  rax, rbx5[2]
            { 8, "rax", 0, 1, GCNFIELD_SSRC0, ASMRVU_READ, 1 },
            { 8, "rbx5", 2, 3, GCNFIELD_SSRC1, ASMRVU_READ, 1 }
        },
        true, ""
    },
    {   /* SOPK */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:4, rbx5:s:4\n"
        "s_cmpk_eq_i32  rbx, 0xd3b9\n"
        "s_addk_i32  rax, 0xd3b9\n"
        "s_cbranch_i_fork rbx5[2:3], xxxx-8\nxxxx:\n"
        "s_getreg_b32 rbx, hwreg(trapsts, 0, 1)\n"
        "s_setreg_b32  hwreg(trapsts, 3, 10), rax\n",
        {
            // s_cmpk_eq_i32  rbx, 0xd3b9
            { 0, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_READ, 1 },
            // s_addk_i32  rax, 0xd3b9
            { 4, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            // s_cbranch_i_fork rbx5[2:3], xxxx-8
            { 8, "rbx5", 2, 4, GCNFIELD_SDST, ASMRVU_READ, 2 },
            // s_getreg_b32 rbx, hwreg(trapsts, 0, 1)
            { 12, "rbx", 0, 1, GCNFIELD_SDST, ASMRVU_WRITE, 1 },
            // s_setreg_b32  hwreg(trapsts, 3, 10), rax
            { 16, "rax", 0, 1, GCNFIELD_SDST, ASMRVU_READ, 1 }
        },
        true, ""
    },
    {   /* SMRD */
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:20, rbx5:s:16\n"
        "s_load_dword rbx, rbx5[2:3], 0x5b\n"
        "s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b\n"
        "s_load_dwordx4 rax4[0:3], rbx5[6:7], 0x5b\n"
        "s_load_dwordx8 rax4[0:7], rbx5[8:9], 0x5b\n"
        "s_load_dwordx16 rax4[4:19], rbx5[10:11], 0x5b\n"
        "s_load_dword rbx, rbx5[2:3], rbx5[6]\n"
        "s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b\n"
        "s_memtime  rax4[2:3]\n"
        "s_dcache_inv\n",
        {
            // s_load_dword rbx, rbx5[2:3], 0x5b
            { 0, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b
            { 4, "rax4", 0, 2, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            { 4, "rbx5", 4, 6, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx4 rax4[0:3], rbx5[4:5], 0x5b
            { 8, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 8, "rbx5", 6, 8, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx8 rax4[0:7], rbx5[4:5], 0x5b
            { 12, "rax4", 0, 8, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 12, "rbx5", 8, 10, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx16 rax4[4:19], rbx5[4:5], 0x5b
            { 16, "rax4", 4, 20, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 16, "rbx5", 10, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dword rbx, rbx5[2:3], rbx5[6]
            { 20, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 20, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            { 20, "rbx5", 6, 7, GCNFIELD_SMRD_SOFFSET, ASMRVU_READ, 1 },
            // s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b
            { 24, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 24, "rbx5", 8, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_memtime  rax4[2:3]
            { 28, "rax4", 2, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 }
        },
        true, ""
    },
    {   /* SMEM */
        ".gpu Fiji\n"
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:20, rbx5:s:16\n"
        "s_load_dword rbx, rbx5[2:3], 0x5b\n"
        "s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b\n"
        "s_load_dwordx4 rax4[0:3], rbx5[6:7], 0x5b\n"
        "s_load_dwordx8 rax4[0:7], rbx5[8:9], 0x5b\n"
        "s_load_dwordx16 rax4[4:19], rbx5[10:11], 0x5b\n"
        "s_load_dword rbx, rbx5[2:3], rbx5[6]\n"
        "s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b\n"
        "s_memtime  rax4[2:3]\n"
        "s_dcache_inv\n"
        "s_store_dword rbx, rbx5[2:3], 0x5b\n"
        "s_atc_probe  0x32, rax4[12:13], 0xfff5b\n"
        "s_atc_probe_buffer  0x32, rax4[12:15], 0xfff5b\n",
        {
            // s_load_dword rbx, rbx5[2:3], 0x5b
            { 0, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 0, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx2 rax4[0:1], rbx5[4:5], 0x5b
            { 8, "rax4", 0, 2, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            { 8, "rbx5", 4, 6, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx4 rax4[0:3], rbx5[4:5], 0x5b
            { 16, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 16, "rbx5", 6, 8, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx8 rax4[0:7], rbx5[4:5], 0x5b
            { 24, "rax4", 0, 8, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 24, "rbx5", 8, 10, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dwordx16 rax4[4:19], rbx5[4:5], 0x5b
            { 32, "rax4", 4, 20, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 32, "rbx5", 10, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_load_dword rbx, rbx5[2:3], rbx5[6]
            { 40, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 1 },
            { 40, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            { 40, "rbx5", 6, 7, GCNFIELD_SMRD_SOFFSET, ASMRVU_READ, 1 },
            // s_buffer_load_dwordx4 rax4[0:3], rbx5[8:11], 0x5b
            { 48, "rax4", 0, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 4 },
            { 48, "rbx5", 8, 12, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 },
            // s_memtime  rax4[2:3]
            { 56, "rax4", 2, 4, GCNFIELD_SMRD_SDST, ASMRVU_WRITE, 2 },
            // s_store_dword rbx, rbx5[2:3], 0x5b\n
            { 72, "rbx", 0, 1, GCNFIELD_SMRD_SDST, ASMRVU_READ, 1 },
            { 72, "rbx5", 2, 4, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_atc_probe  0x32, rax4[12:13], 0xfff5b
            { 80, "rax4", 12, 14, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 2 },
            // s_atc_probe_buffer 0x32, rax4[12:13], 0xfff5b
            { 88, "rax4", 12, 16, GCNFIELD_SMRD_SBASE, ASMRVU_READ, 4 }
        },
        true, ""
    },
    {   /* VOP2 */
        ".regvar rax:v, rbx:v, rex:v\n"
        ".regvar rax2:v:8, rbx4:v:8, rex5:v:10\n"
        ".regvar srex:s, srdx3:s:6\n"
        "v_sub_f32  rex, rax, rbx\n"
        "v_sub_f32  rex, srex, rbx\n",
        {
            // v_sub_f32  rex, rax, rbx
            { 0, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 0, "rax", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_VOP_SRC1, ASMRVU_READ, 1 },
            // v_sub_f32  rex, srex, rbx
            { 4, "rex", 0, 1, GCNFIELD_VOP_VDST, ASMRVU_WRITE, 1 },
            { 4, "srex", 0, 1, GCNFIELD_VOP_SRC0, ASMRVU_READ, 1 },
            { 4, "rbx", 0, 1, GCNFIELD_VOP_SRC1, ASMRVU_READ, 1 },
        },
        true, ""
    }
};

static void testGCNRegVarUsages(cxuint i, const GCNRegVarUsageCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << " regVarUsageGCNCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testGCNRegVarUsages", testCaseName+".good",
                      testCase.good, good);
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " regVarUsageGCNCase#" << i;
        throw Exception(oss.str());
    }
    /*assertValue("testGCNRegVarUsages", testCaseName+".size",
                    testCase.regVarUsages.size(), section.regVarUsages.size());*/
    ISAUsageHandler* usageHandler = assembler.getSections()[0].usageHandler.get();
    usageHandler->rewind();
    for (size_t j = 0; usageHandler->hasNext(); j++)
    {
        const AsmRegVarUsage resultRvu = usageHandler->nextUsage();
        std::ostringstream rvuOss;
        rvuOss << ".regVarUsage#" << j << ".";
        rvuOss.flush();
        std::string rvuName(rvuOss.str());
        const AsmRegVarUsageData& expectedRvu = testCase.regVarUsages[j];
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"offset",
                    expectedRvu.offset, resultRvu.offset);
        if (expectedRvu.regVarName==nullptr)
            assertTrue("testGCNRegVarUsages", testCaseName+rvuName+"regVarName",
                       resultRvu.regVar==nullptr);
        else // otherwise
            assertString("testGCNRegVarUsages", testCaseName+rvuName+"regVarName",
                        expectedRvu.regVarName, resultRvu.regVar->first);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rstart",
                    expectedRvu.rstart, resultRvu.rstart);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rend",
                    expectedRvu.rend, resultRvu.rend);
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"regField",
                    cxuint(expectedRvu.regField), cxuint(resultRvu.regField));
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"rwFlags",
                    cxuint(expectedRvu.rwFlags), cxuint(resultRvu.rwFlags));
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"align",
                    cxuint(expectedRvu.align), cxuint(resultRvu.align));
    }
    assertString("testGCNRegVarUsages", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(gcnRvuTestCases1Tbl)/sizeof(GCNRegVarUsageCase); i++)
        try
        { testGCNRegVarUsages(i, gcnRvuTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
