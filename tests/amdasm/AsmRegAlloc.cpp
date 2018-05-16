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

std::ostream& operator<<(std::ostream& os, const TestSingleVReg& vreg)
{
    if (!vreg.name.empty())
        os << vreg.name << "[" << vreg.index << "]";
    else
        os << "@REG[" << vreg.index << "]";
    return os;
}

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

static void testCreateSSAData(cxuint testSuiteId, cxuint i, const AsmSSADataCase& testCase)
{
    std::cout << "-----------------------------------------------\n"
    "           Test " << testSuiteId << " " << i << "\n"
                "------------------------------------------------\n";
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input,
                    (ASM_ALL&~ASM_ALTMACRO) | ASM_TESTRUN | ASM_TESTRESOLVE,
                    BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, errorStream);
    bool good = assembler.assemble();
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << " testAsmCodeSSAData#" << i;
        throw Exception(oss.str());
    }
    const AsmSection& section = assembler.getSections()[0];
    
    AsmRegAllocator regAlloc(assembler);
    
    regAlloc.createCodeStructure(section.codeFlow, section.getSize(),
                            section.content.data());
    regAlloc.createSSAData(*section.usageHandler, *section.linearDepHandler);
    
    const std::vector<CodeBlock>& resCodeBlocks = regAlloc.getCodeBlocks();
    std::ostringstream oss;
    oss << " testAsmSSAData" << testSuiteId << " case#" << i;
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
            
            const TestSingleVReg expVReg = { expCBlock.ssaInfos[k].first.name,
                    expCBlock.ssaInfos[k].first.index };
            
            assertValue("testAsmSSAData", testCaseName + cbname + ssaName + "svreg",
                        expVReg, resSSAInfos[k].first);
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
    Array<std::pair<TestSingleVReg, Array<SSAReplace> > >
                        resSSAReplaces(ssaReplacesMap.size());
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
        
        const TestSingleVReg expVReg = { expEntry.first.name, expEntry.first.index };
        
        assertValue("testAsmSSAData", testCaseName + ssaName + "svreg",
                        expVReg, resEntry.first);
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
    for (size_t i = 0; codeStructTestCases1Tbl[i].input!=nullptr; i++)
        try
        { testCreateCodeStructure(i, codeStructTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (size_t i = 0; ssaDataTestCases1Tbl[i].input!=nullptr; i++)
        try
        { testCreateSSAData(0, i, ssaDataTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (size_t i = 0; ssaDataTestCases2Tbl[i].input!=nullptr; i++)
        try
        { testCreateSSAData(1, i, ssaDataTestCases2Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (size_t i = 0; ssaDataTestCases3Tbl[i].input!=nullptr; i++)
        try
        { testCreateSSAData(2, i, ssaDataTestCases3Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
