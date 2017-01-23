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
    {
        ".regvar rax:s, rbx:s\n"
        ".regvar rax4:s:4, rbx5:s:4\n"
        "s_mov_b32 rax,rbx\n"
        "s_mov_b32 rax4[2],rbx4[1]\n",
        {
            { 0, "rax", 0, 1, GCNFIELD_SDST, ASMVARUS_WRITE, 1 },
            { 0, "rbx", 0, 1, GCNFIELD_SSRC0, ASMVARUS_READ, 1 },
            { 4, "rax4", 2, 3, GCNFIELD_SDST, ASMVARUS_WRITE, 1 },
            { 4, "rbx4", 1, 2, GCNFIELD_SSRC0, ASMVARUS_READ, 1 }
        },
        true,
        ""
    }
};

static void testEncGCNRegVarUsages(cxuint i, const GCNRegVarUsageCase& testCase)
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
    const AsmSection& section = assembler.getSections()[0];
    assertValue("testGCNRegVarUsages", testCaseName+".size",
                    testCase.regVarUsages.size(), section.regVarUsages.size());
    for (size_t j = 0; j < section.regVarUsages.size(); j++)
    {
        std::ostringstream rvuOss;
        rvuOss << ".regVarUsage#" << j << ".";
        rvuOss.flush();
        std::string rvuName(rvuOss.str());
        const AsmRegVarUsageData& expectedRvu = testCase.regVarUsages[j];
        const AsmRegVarUsage& resultRvu = section.regVarUsages[j];
        assertValue("testGCNRegVarUsages", testCaseName+rvuName+"offset",
                    expectedRvu.offset, resultRvu.offset);
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
        { testEncGCNRegVarUsages(i, gcnRvuTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
