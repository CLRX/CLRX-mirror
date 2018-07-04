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

struct AsmWaitHandlerCase
{
    const char* input;
    Array<AsmWaitInstr> waitInstrs;
    Array<AsmDelayedOp> delayedResults;
    bool good;
    const char* errorMessages;
};

static const AsmWaitHandlerCase waitHandlerTestCases[] =
{
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
    /*size_t j, k;
    for (j = k = 0; waitHandler->hasNext();)
    {
        AsmWaitInstr waitInstr;
        AsmDelayedResult delayedResult;
        assertTrue("testWaitHandle", testCaseName+".length",
                   j < testCase.waitInstrs.size());
    }*/
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    return retVal;
}
