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
            koss << testCaseName << ".waitIstr#" << j;
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
