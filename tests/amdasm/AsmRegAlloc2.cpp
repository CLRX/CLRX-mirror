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
#include "AsmRegAlloc.h"

using namespace CLRX;

struct AsmApplySSAReplacesCase
{
    Array<CCodeBlock2> inCodeBlocks;
    Array<std::pair<TestSingleVReg2, Array<SSAReplace> > > inSSAReplaces;
    Array<CCodeBlock2> codeBlocks; // expected code blocks
};

static const AsmApplySSAReplacesCase ssaApplyReplacesCasesTbl[] =
{
    {   // 0 - first testcase
        // 16 - trick - SSA replaces beyond visited point
        {
            // block 0 - start
            { 0, 8,
                { },
                {
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - loop
            { 8, 16,
                { { 2, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 2 - loop part 2
            { 16, 24,
                { { 1, false }, { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(2, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - end 2
            { 24, 28,
                { },
                { }, false, false, true },
            // block 4 - end
            { 28, 36,
                { },
                {
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {
            { { "sa", 2 }, { { 2, 1 } } },
            // must be
            { { "sa", 3 }, { { 2, 1 } } }
        },
        // expected code blocks
        {
            // block 0 - start
            { 0, 8,
                { },
                {
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - loop
            { 8, 16,
                { { 2, false }, { 4, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 1, 2, 1, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 2 - loop part 2
            { 16, 24,
                { { 1, false }, { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, SIZE_MAX, 3, SIZE_MAX, 0, true) },
                    { { "sa", 3 }, SSAInfo(1, 1, 2, 1, 1, false) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - end 2
            { 24, 28,
                { },
                { }, false, false, true },
            // block 4 - end
            { 28, 36,
                { },
                {
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        }
    },
    {   // 1 - second testcase
        // 13 - yet another branch example
        {
            // block 0
            { 0, 12,
                { { 1, false }, { 2, false }, { 4, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - b0
            { 12, 24,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 2 - b1
            { 24, 32,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - b1x
            { 32, 40,
                { { 1, false } },
                {
                    { { "sa", 2 }, SSAInfo(3, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 4 - b2
            { 40, 48,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 4, 4, 4, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 5 - b2x
            { 48, 56,
                { { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(5, 6, 6, 6, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 6 - b3
            { 56, 68,
                { { 5, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 7, 7, 7, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        },
        {   // SSA replaces
            { { "sa", 2 }, { { 4, 1 }, { 6, 3 }, { 7, 5 } } },
            { { "sa", 3 }, { { 3, 1 }, { 4, 1 }, { 5, 1 } } }
        },
        // expected blocks
        {
            // block 0
            { 0, 12,
                { { 1, false }, { 2, false }, { 4, false }, { 6, false } },
                {
                    { { "", 0 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 1 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 4 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "", 5 }, SSAInfo(0, 0, 0, 0, 0, true) },
                    { { "sa", 2 }, SSAInfo(0, 1, 1, 1, 1, false) },
                    { { "sa", 3 }, SSAInfo(0, 1, 1, 1, 1, false) }
                }, false, false, false },
            // block 1 - b0
            { 12, 24,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 2, 2, 2, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 2 - b1
            { 24, 32,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 3, 3, 3, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 1, 3, 1, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 3 - b1x
            { 32, 40,
                { { 1, false } },
                {
                    { { "sa", 2 }, SSAInfo(3, 1, 4, 1, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 4 - b2
            { 40, 48,
                { },
                {
                    { { "sa", 2 }, SSAInfo(1, 5, 5, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 1, 4, 1, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, false },
            // block 5 - b2x
            { 48, 56,
                { { 3, false } },
                {
                    { { "sa", 2 }, SSAInfo(5, 3, 6, 3, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true },
            // block 6 - b3
            { 56, 68,
                { { 5, false } },
                {
                    { { "sa", 2 }, SSAInfo(1, 5, 7, 5, 1, true) },
                    { { "sa", 3 }, SSAInfo(1, 1, 5, 1, 1, true) },
                    { { "sa", 4 }, SSAInfo(0, SIZE_MAX, 1, SIZE_MAX, 0, true) }
                }, false, false, true }
        }
    }
};

std::ostream& operator<<(std::ostream& os, const TestSingleVReg& vreg)
{
    if (!vreg.name.empty())
        os << vreg.name << "[" << vreg.index << "]";
    else
        os << "@REG[" << vreg.index << "]";
    return os;
}

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

static void testApplySSAReplaces(cxuint i, const AsmApplySSAReplacesCase& testCase)
{
    std::istringstream input("");
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE);
    
    std::unordered_map<CString, AsmRegVar> regVars;
    std::vector<CodeBlock> inCodeBlocks;
    // generate input code blocks
    for (size_t i = 0; i < testCase.inCodeBlocks.size(); i++)
    {
        const CCodeBlock2& cc = testCase.inCodeBlocks[i];
        CodeBlock inCodeBlock{ cc.start, cc.end,
            std::vector<AsmRegAllocator::NextBlock>(cc.nexts.begin(), cc.nexts.end()),
            cc.haveCalls, cc.haveReturn, cc.haveEnd };
        for (const auto& v: cc.ssaInfos)
        {
            AsmSingleVReg vreg{ nullptr, v.first.index };
            if (v.first.name != nullptr)
            {
                auto res = regVars.insert({ v.first.name , AsmRegVar{ 0, 100 } });
                vreg.regVar = &res.first->second;
            }
            inCodeBlock.ssaInfoMap.insert({ vreg, v.second });
        }
        inCodeBlocks.push_back(inCodeBlock);
    }
    SSAReplacesMap inSSAReplaces;
    // generate SSA replaces
    for (const auto& replace: testCase.inSSAReplaces)
    {
        AsmSingleVReg vreg{ nullptr, replace.first.index };
        if (replace.first.name != nullptr)
        {
            auto res = regVars.insert({ replace.first.name , AsmRegVar{ 0, 100 } });
            vreg.regVar = &res.first->second;
        }
        inSSAReplaces.insert({ vreg, VectorSet<SSAReplace>(
                    replace.second.begin(), replace.second.end()) });
    }
    
    AsmRegAllocator regAlloc(assembler, inCodeBlocks, inSSAReplaces);
    regAlloc.applySSAReplaces();
    
    const std::vector<CodeBlock>& resCodeBlocks = regAlloc.getCodeBlocks();
    std::ostringstream oss;
    oss << " testAsmSSAReplace case#" << i;
    const std::string testCaseName = oss.str();
    
    assertValue("testAsmSSAReplace", testCaseName+".codeBlocks.size",
                testCase.codeBlocks.size(), resCodeBlocks.size());
    
    std::unordered_map<const AsmRegVar*, CString> regVarNamesMap;
    for (const auto& rvEntry: regVars)
        regVarNamesMap.insert(std::make_pair(&rvEntry.second, rvEntry.first));
    
    for (size_t j = 0; j < resCodeBlocks.size(); j++)
    {
        const CCodeBlock2& expCBlock = testCase.codeBlocks[j];
        const CodeBlock& resCBlock = resCodeBlocks[j];
        std::ostringstream codeBlockOss;
        codeBlockOss << ".codeBlock#" << j << ".";
        codeBlockOss.flush();
        std::string cbname(codeBlockOss.str());
        assertValue("testAsmSSAReplace", testCaseName + cbname + "start",
                    expCBlock.start, resCBlock.start);
        assertValue("testAsmSSAReplace", testCaseName + cbname + "end",
                    expCBlock.end, resCBlock.end);
        assertValue("testAsmSSAReplace", testCaseName + cbname + "nextsSize",
                    expCBlock.nexts.size(), resCBlock.nexts.size());
        
        for (size_t k = 0; k < expCBlock.nexts.size(); k++)
        {
            std::ostringstream nextOss;
            nextOss << "next#" << k << ".";
            nextOss.flush();
            std::string nname(nextOss.str());
            
            assertValue("testAsmSSAReplace", testCaseName + cbname + nname + "block",
                    expCBlock.nexts[k].block, resCBlock.nexts[k].block);
            assertValue("testAsmSSAReplace", testCaseName + cbname + nname + "isCall",
                    int(expCBlock.nexts[k].isCall), int(resCBlock.nexts[k].isCall));
        }
        
        assertValue("testAsmSSAReplace", testCaseName + cbname + "ssaInfoSize",
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
            
            const TestSingleVReg expVReg = { expCBlock.ssaInfos[k].first.name,
                    expCBlock.ssaInfos[k].first.index };
            
            assertValue("testAsmSSAReplace", testCaseName + cbname + ssaName + "svreg",
                        expVReg, resSSAInfos[k].first);
            const SSAInfo& expSInfo = expCBlock.ssaInfos[k].second;
            const SSAInfo& resSInfo = resSSAInfos[k].second;
            assertValue("testAsmSSAReplace",
                        testCaseName + cbname + ssaName + "ssaIdBefore",
                        expSInfo.ssaIdBefore, resSInfo.ssaIdBefore);
            assertValue("testAsmSSAReplace", testCaseName + cbname + ssaName + "ssaId",
                        expSInfo.ssaId, resSInfo.ssaId);
            assertValue("testAsmSSAReplace",
                        testCaseName + cbname + ssaName + "ssaIdChange",
                        expSInfo.ssaIdChange, resSInfo.ssaIdChange);
            assertValue("testAsmSSAReplace",
                        testCaseName + cbname + ssaName + "ssaIdFirst",
                        expSInfo.ssaIdFirst, resSInfo.ssaIdFirst);
            assertValue("testAsmSSAReplace",
                        testCaseName + cbname + ssaName + "ssaIdLast",
                        expSInfo.ssaIdLast, resSInfo.ssaIdLast);
            assertValue("testAsmSSAReplace", testCaseName + cbname + ssaName +
                        "readBeforeWrite", int(expSInfo.readBeforeWrite),
                        int(resSInfo.readBeforeWrite));
        }
        
        assertValue("testAsmSSAReplace", testCaseName + cbname + "haveCalls",
                    int(expCBlock.haveCalls), int(resCBlock.haveCalls));
        assertValue("testAsmSSAReplace", testCaseName + cbname + "haveReturn",
                    int(expCBlock.haveReturn), int(resCBlock.haveReturn));
        assertValue("testAsmSSAReplace", testCaseName + cbname + "haveEnd",
                    int(expCBlock.haveEnd), int(resCBlock.haveEnd));
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(ssaApplyReplacesCasesTbl)/
                        sizeof(AsmApplySSAReplacesCase); i++)
        try
        { testApplySSAReplaces(i, ssaApplyReplacesCasesTbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
