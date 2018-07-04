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
#include <CLRX/amdasm/GCNDefs.h>
#include "../TestUtils.h"

using namespace CLRX;

struct AsmDelayedOpData
{
    size_t offset;
    const char* regVarName;
    uint16_t rstart;
    uint16_t rend;
    cxbyte delayInstrType;
    cxbyte rwFlags;
};

struct AsmWaitHandlerCase
{
    const char* input;
    Array<AsmWaitInstr> waitInstrs;
    Array<AsmDelayedOpData> delayedOps;
    bool good;
    const char* errorMessages;
};

static const AsmWaitHandlerCase waitHandlerTestCases[] =
{
    {   /* 0 - first test - empty */
        R"ffDXD(
            .regvar bax:s, dbx:v, dcx:s:8
            s_mov_b32 bax, s11
            s_mov_b32 bax, dcx[1]
            s_branch aa0
            s_endpgm
aa0:        s_add_u32 bax, dcx[1], dcx[2]
            s_endpgm
)ffDXD",
        { }, { }, true, ""
    },
    {   /* 1 - SMRD instr */
        R"ffDXD(
            .regvar bax:s, dbx:v, dcx:s:8
            s_mov_b32 bax, s11
            s_load_dword dcx[2], s[10:11], 4
            s_mov_b32 bax, dcx[1]
            s_waitcnt lgkmcnt(0)
            s_branch aa0
            s_endpgm
aa0:        s_add_u32 bax, dcx[1], dcx[2]
            s_endpgm
)ffDXD",
        {
            // s_waitcnt lgkmcnt(0)
            { 12U, { 15, 0, 7, 0 } },
        },
        {
            // s_load_dword dcx[2], s[10:11], 4
            { 4U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 2 - s_waitcnt tests */
        R"ffDXD(
            s_waitcnt vmcnt(3) & lgkmcnt(5) & expcnt(4)
            s_waitcnt vmcnt(3) & lgkmcnt(5)
            s_waitcnt vmcnt(3)
            s_waitcnt expcnt(3)
            s_waitcnt lgkmcnt(3)
            s_waitcnt vmcnt(3) & expcnt(4)
            s_waitcnt lgkmcnt(3) & expcnt(4)
            s_waitcnt vmcnt(15) lgkmcnt(15) expcnt(7)
)ffDXD",
        {
            { 0U, { 3, 5, 4, 0 } },
            { 4U, { 3, 5, 7, 0 } },
            { 8U, { 3, 7, 7, 0 } },
            { 12U, { 15, 7, 3, 0 } },
            { 16U, { 15, 3, 7, 0 } },
            { 20U, { 3, 7, 4, 0 } },
            { 24U, { 15, 3, 4, 0 } },
            { 28U, { 15, 7, 7, 0 } }
        },
        { }, true, ""
    },
    {   /* 3 - s_waitcnt tests (GFX9) */
        R"ffDXD(.arch GFX9
            s_waitcnt vmcnt(47) & lgkmcnt(5) & expcnt(4)
            s_waitcnt lgkmcnt(5)
)ffDXD",
        {
            { 0U, { 47, 5, 4, 0 } },
            { 4U, { 63, 5, 7, 0 } }
        },
        { }, true, ""
    },
    {   /* 4 - SMRD */
         R"ffDXD(
            .regvar bax:s, dbx:v, dcx:s:8
            .regvar bb:s:28
            s_load_dword dcx[2], s[10:11], 4
            s_load_dwordx2 dcx[4:5], s[10:11], 4
            s_load_dwordx4 bb[4:7], s[10:11], 4
            s_load_dwordx8 bb[16:23], s[10:11], 4
            s_load_dwordx16 bb[12:27], s[10:11], 4
            s_buffer_load_dword dcx[2], s[12:15], 4
            s_buffer_load_dwordx2 dcx[4:5], s[12:15], 4
            s_buffer_load_dwordx4 bb[4:7], s[12:15], 4
            s_buffer_load_dwordx8 bb[16:23], s[12:15], 4
            s_buffer_load_dwordx16 bb[12:27], s[12:15], 4
            s_memtime s[4:5]
            s_memtime dcx[5:6]
)ffDXD",
        { },
        {
            { 0U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 4U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 8U, "bb", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 12U, "bb", 16, 24, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 16U, "bb", 12, 28, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 20U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 24U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 28U, "bb", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 32U, "bb", 16, 24, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 36U, "bb", 12, 28, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 40U, nullptr, 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 44U, "dcx", 5, 7, GCNDELINSTR_SMINSTR, ASMRVU_WRITE }
        }, true, ""
    },
    {   /* 5 - SMEM */
         R"ffDXD(.gpu Fiji
            .regvar bax:s, dbx:v, dcx:s:8
            .regvar bb:s:28
            s_load_dword dcx[2], s[10:11], 4
            s_load_dwordx2 dcx[4:5], s[10:11], 4
            s_load_dwordx4 bb[4:7], s[10:11], 4
            s_load_dwordx8 bb[16:23], s[10:11], 4
            s_load_dwordx16 bb[12:27], s[10:11], 4
            s_buffer_load_dword dcx[2], s[12:15], 4
            s_buffer_load_dwordx2 dcx[4:5], s[12:15], 4
            s_buffer_load_dwordx4 bb[4:7], s[12:15], 4
            s_buffer_load_dwordx8 bb[16:23], s[12:15], 4
            s_buffer_load_dwordx16 bb[12:27], s[12:15], 4
            s_memtime s[4:5]
            s_memtime dcx[5:6]
            s_store_dword dcx[2], s[10:11], 4
            s_store_dwordx2 dcx[4:5], s[10:11], 4
            s_store_dwordx4 bb[4:7], s[10:11], 4
            s_buffer_store_dword dcx[2], s[20:23], 4
            s_buffer_store_dwordx2 dcx[4:5], s[20:23], 4
            s_buffer_store_dwordx4 bb[4:7], s[20:23], 4
)ffDXD",
        { },
        {
            { 0U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 8U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 16U, "bb", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 24U, "bb", 16, 24, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 32U, "bb", 12, 28, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 40U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 48U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 56U, "bb", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 64U, "bb", 16, 24, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 72U, "bb", 12, 28, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 80U, nullptr, 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 88U, "dcx", 5, 7, GCNDELINSTR_SMINSTR, ASMRVU_WRITE },
            { 96U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 104U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 112U, "bb", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 120U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 128U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 136U, "bb", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_READ }
        }, true, ""
    },
    {   /* 6 - SMEM (GFX9) */
         R"ffDXD(.gpu GFX900
            .regvar bax:s, dbx:v, dcx:s:8
            s_atomic_swap bax, s[14:15], 18
            s_atomic_cmpswap dcx[2:3], s[14:15], 18
            s_atomic_umin bax, s[14:15], 20
            s_atomic_swap_x2 dcx[2:3], s[14:15], 18
            s_atomic_cmpswap_x2 dcx[4:7], s[14:15], 18
            s_atomic_umin_x2 dcx[2:3], s[14:15], 20
            s_atomic_swap bax, s[14:15], 18 glc
            s_atomic_cmpswap dcx[2:3], s[14:15], 18 glc
            s_atomic_umin bax, s[14:15], 20 glc
            s_atomic_swap_x2 dcx[2:3], s[14:15], 18 glc
            s_atomic_cmpswap_x2 dcx[4:7], s[14:15], 18 glc
            s_atomic_umin_x2 dcx[2:3], s[14:15], 20 glc
            
            s_buffer_atomic_swap bax, s[16:19], 18
            s_buffer_atomic_cmpswap dcx[2:3], s[16:19], 18
            s_buffer_atomic_umin bax, s[16:19], 20
            s_buffer_atomic_swap_x2 dcx[2:3], s[16:19], 18
            s_buffer_atomic_cmpswap_x2 dcx[4:7], s[16:19], 18
            s_buffer_atomic_umin_x2 dcx[2:3], s[16:19], 20
            s_buffer_atomic_swap bax, s[16:19], 18 glc
            s_buffer_atomic_cmpswap dcx[2:3], s[16:19], 18 glc
            s_buffer_atomic_umin bax, s[16:19], 20 glc
            s_buffer_atomic_swap_x2 dcx[2:3], s[16:19], 18 glc
            s_buffer_atomic_cmpswap_x2 dcx[4:7], s[16:19], 18 glc
            s_buffer_atomic_umin_x2 dcx[2:3], s[16:19], 20 glc
)ffDXD",
        { },
        {
            // S_ATOMIC without GLC
            { 0U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 8U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 16U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 24U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 32U, "dcx", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 40U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            // S_ATOMIC with GLC
            { 48U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 56U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 56U, "dcx", 3, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 64U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 72U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 80U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 80U, "dcx", 6, 8, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 88U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            // S_BUFFER_ATOMIC without GLC
            { 96U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 104U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 112U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 120U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 128U, "dcx", 4, 8, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 136U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            // S_BUFFER_ATOMIC with GLC
            { 144U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 152U, "dcx", 2, 3, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 152U, "dcx", 3, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 160U, "bax", 0, 1, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 168U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 176U, "dcx", 4, 6, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE },
            { 176U, "dcx", 6, 8, GCNDELINSTR_SMINSTR, ASMRVU_READ },
            { 184U, "dcx", 2, 4, GCNDELINSTR_SMINSTR, ASMRVU_READ|ASMRVU_WRITE }
        }, true, ""
    }
};

static void pushRegVarsFromScopes(const AsmScope& scope,
            std::unordered_map<const AsmRegVar*, CString>& rvMap,
            const std::string& prefix)
{
    for (const auto& rvEntry: scope.regVarMap)
    {
        std::string rvName = prefix+rvEntry.first.c_str();
        rvMap.insert(std::make_pair(&rvEntry.second, rvName));
    }
    
    for (const auto& scopeEntry: scope.scopeMap)
    {
        std::string newPrefix = prefix+scopeEntry.first.c_str()+"::";
        pushRegVarsFromScopes(*scopeEntry.second, rvMap, newPrefix);
    }
}

static void testWaitHandlerCase(cxuint i, const AsmWaitHandlerCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input,
                    (ASM_ALL&~ASM_ALTMACRO) | ASM_TESTRUN | ASM_TESTRESOLVE,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << " testWaitHandlerCase#" << i;
    oss.flush();
    const std::string testCaseName = oss.str();
    assertValue<bool>("testWaitHandle", testCaseName+".good",
                      testCase.good, good);
    assertString("testWaitHandle", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
    
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testWaitHandlerCase#" << i;
        throw Exception(oss.str());
    }
    
    std::unordered_map<const AsmRegVar*, CString> regVarNamesMap;
    pushRegVarsFromScopes(assembler.getGlobalScope(), regVarNamesMap, "");
    ISAWaitHandler* waitHandler = assembler.getSections()[0].waitHandler.get();
    waitHandler->rewind();
    size_t j, k;
    for (j = k = 0; waitHandler->hasNext();)
    {
        AsmWaitInstr waitInstr{};
        AsmDelayedOp delayedOp{};
        std::ostringstream koss;
        if (waitHandler->nextInstr(delayedOp, waitInstr))
        {
            /// check Asm wait instruction
            assertTrue("testWaitHandle", testCaseName+".wlength",
                    j < testCase.waitInstrs.size());
            koss << testCaseName << ".waitInstr#" << j;
            std::string wiStr = koss.str();
            
            const AsmWaitInstr& expWaitInstr = testCase.waitInstrs[j++];
            assertValue("testWaitHandle", wiStr+".offset", expWaitInstr.offset,
                        waitInstr.offset);
            assertArray("testWaitHandle", wiStr+".waits",
                        Array<uint16_t>(expWaitInstr.waits, expWaitInstr.waits+4),
                        4, waitInstr.waits);
        }
        else
        {
            // test asm delayed op
            assertTrue("testWaitHandle", testCaseName+".dolength",
                    k < testCase.delayedOps.size());
            koss << testCaseName << ".delayedOp#" << k;
            std::string doStr = koss.str();
            
            const AsmDelayedOpData& expDelayedOp = testCase.delayedOps[k++];
            assertValue("testWaitHandle", doStr+".offset", expDelayedOp.offset,
                        delayedOp.offset);
            if (expDelayedOp.regVarName==nullptr)
                assertTrue("testWaitHandle", doStr+".regVar", delayedOp.regVar==nullptr);
            else // otherwise
            {
                assertTrue("testWaitHandle", doStr+".regVar", delayedOp.regVar!=nullptr);
                assertString("testWaitHandle", doStr+".regVarName",
                        expDelayedOp.regVarName,
                        regVarNamesMap.find(delayedOp.regVar)->second);
            }
            assertValue("testWaitHandle", doStr+".rstart", expDelayedOp.rstart,
                        delayedOp.rstart);
            assertValue("testWaitHandle", doStr+".rend", expDelayedOp.rend,
                        delayedOp.rend);
            assertValue("testWaitHandle", doStr+".delayedInstrType",
                        cxuint(expDelayedOp.delayInstrType),
                        cxuint(delayedOp.delayInstrType));
            assertValue("testWaitHandle", doStr+".rwFlags",
                        cxuint(expDelayedOp.rwFlags), cxuint(delayedOp.rwFlags));
        }
    }
    assertTrue("testWaitHandle", testCaseName+".wlength",
                   j == testCase.waitInstrs.size());
    assertTrue("testWaitHandle", testCaseName+".dolength",
                   k == testCase.delayedOps.size());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(waitHandlerTestCases) / sizeof(AsmWaitHandlerCase); i++)
        try
        { testWaitHandlerCase(i, waitHandlerTestCases[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
