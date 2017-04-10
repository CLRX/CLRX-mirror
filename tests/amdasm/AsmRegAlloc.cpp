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
    {
        "v_mov_b32 v1, v2\n"
        "v_mov_b32 v1, v3\n",
        {
            { 0, 8, { }, false, false, true }
        },
        true, ""
    },
    {
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
    {   // ignore cf_start to cf_end (except first)
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
    {   // ignore cf_end
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
