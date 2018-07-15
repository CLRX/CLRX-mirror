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
#include <iostream>
#include <string>
#include <unordered_map>
#include <CLRX/utils/CString.h>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/AsmSource.h>
#include "../TestUtils.h"

using namespace CLRX;

struct SourcePosEntry
{
    size_t offset;
    const char* macroName;
    const char* sourceName;
    LineNo lineNo;
    ColNo colNo;
};

typedef Array<SourcePosEntry> SourcePosHandlerTestCase;

// testcases
static const SourcePosHandlerTestCase sourcePoHandlerTestTbl[] =
{
    // 0 - first testcase
    {
        { 8, "", "file0.s", 1, 9 },
        { 12, "", "file0.s", 2, 9 },
        { 17, "", "file0.s", 3, 4 },
        { 19, "", "file0.s", 4, 1 },
        { 119, "", "file0.s", 7, 1 }
    },
    // 1 - bigger lineNo
    {
        { 8, "", "file0.s", 32768U, 9 },
        { 12, "", "file0.s", 32768U+1, 9 },
        { 17, "", "file0.s", 32768U+2, 4 }
    },
    // 2 - bigger lineNo
    {
        { 8, "", "file0.s", 121768U, 9 },
        { 12, "", "file0.s", 121768U+1, 9 },
        { 17, "", "file0.s", 121768U+2, 4 }
    },
    // 3 - bigger lineNo
    {
        { 8, "", "file0.s", 0x40000000U, 9 },
        { 12, "", "file0.s", 0x40000000U+1, 9 },
        { 17, "", "file0.s", 0x40000000U+2, 4 }
    },
    // 4 - bigger lineNo
    {
        { 8, "", "file0.s", 0x3541fa711ULL, 9 },
        { 12, "", "file0.s", 0x3541fa711ULL+1, 9 },
        { 17, "", "file0.s", 0x3541fa711ULL+2, 4 }
    },
    // 5 - bigger lineNo
    {
        { 8, "", "file0.s", 0x200000000000ULL, 9 },
        { 12, "", "file0.s", 0x200000000000ULL+1, 9 },
        { 17, "", "file0.s", 0x200000000000ULL+2, 4 }
    },
    // 6 - bigger lineNo
    {
        { 8, "", "file0.s", 0x42063ca43c9a1acULL, 9 },
        { 12, "", "file0.s", 0x42063ca43c9a1acULL+1, 9 },
        { 17, "", "file0.s", 0x42063ca43c9a1acULL+2, 4 }
    },
    // 7 - bigger colNo
    {
        { 8, "", "file0.s", 1, 128 },
        { 12, "", "file0.s", 2, 128 },
        { 17, "", "file0.s", 3, 4 }
    },
    // 8 - bigger colNo
    {
        { 8, "", "file0.s", 1, 1271 },
        { 12, "", "file0.s", 2, 1271 },
        { 17, "", "file0.s", 3, 4 }
    },
    // 9 - bigger colNo
    {
        { 8, "", "file0.s", 1, 0x4000U },
        { 12, "", "file0.s", 2, 0x4000U },
        { 17, "", "file0.s", 3, 4 }
    },
    // 10 - bigger colNo
    {
        { 8, "", "file0.s", 1, 0x1cafaU },
        { 12, "", "file0.s", 2, 0x1cafaU },
        { 17, "", "file0.s", 3, 4 }
    },
    // 11 - bigger colNo
    {
        { 8, "", "file0.s", 1, 0x200000U },
        { 12, "", "file0.s", 2, 0x200000U },
        { 17, "", "file0.s", 3, 4 }
    },
    // 12 - bigger colNo
    {
        { 8, "", "file0.s", 1, 0xaf29c2bU },
        { 12, "", "file0.s", 2, 0xaf29c2bU },
        { 17, "", "file0.s", 3, 4 }
    },
    // 13 - bigger colNo
    {
        { 8, "", "file0.s", 1, 0x10000000U },
        { 12, "", "file0.s", 2, 0x10000000U },
        { 17, "", "file0.s", 3, 4 }
    },
    // 14 - bigger colNo
    {
        { 8, "", "file0.s", 1, 0xffaabcadU },
        { 12, "", "file0.s", 2, 0xffaabcadU },
        { 17, "", "file0.s", 3, 4 }
    },
    // 15 - with zero offset
    {
        { 0, "", "file0.s", 1, 9 },
        { 4, "", "file0.s", 2, 9 },
        { 9, "", "file0.s", 3, 4 }
    },
    // 16 -
    {
        { 8, "", "file0.s", 1, 9 },
        { 12, "", "file0.s", 12, 9 },
        { 17, "", "file0.s", 24, 5 }
    },
    // 17 -
    {
        { 8, "", "file0.s", 1, 9 },
        { 12, "", "file0.s", 65, 9 }, // 64linediff
        { 17, "", "file0.s", 130, 5 },
        { 20, "", "file0.s", 742, 5 },
        { 24, "", "file0.s", 807, 9 } // 65linediff
    },
    // 18 -
    {
        { 8, "", "file0.s", 1, 9 },
        { 12, "", "file0.s", 65, 9 },
        { 17, "", "file0.s", 130, 1 },
        { 20, "", "file0.s", 742, 1 }
    },
    // 19 -
    {
        { 8, "", "file0.s", 1, 9 },
        { 12, "", "file0.s", 64, 9 },
        { 17, "", "file0.s", 128, 5 }
    },
    // 20 -
    {
        { 8, "", "file0.s", 1, 9 },
        { 12, "", "file0.s", 12, 65 },
        { 17, "", "file0.s", 23, 129 },
        { 20, "", "file0.s", 24, 66 },
        { 24, "", "file0.s", 25, 64 }
    },
    // 21 - many entries for single line
    {
        { 0, "", "file0.s", 1, 9 },
        { 4, "", "file0.s", 1, 20 },
        { 9, "", "file0.s", 1, 53 },
        { 16, "", "file0.s", 2, 5 },
        { 24, "", "file0.s", 2, 47 }
    },
    // 22 - many entries for single line 2
    {
        { 0, "", "file0.s", 1, 9 },
        { 4, "", "file0.s", 1, 123 },
        { 9, "", "file0.s", 1, 370 },
        { 16, "", "file0.s", 2, 5 },
        { 24, "", "file0.s", 2, 70 },
        { 28, "", "file0.s", 2, 134 }
    },
    // 23 - testing one encoding
    {
        { 0, "", "file0.s", 1, 9 },
        { 8, "", "file0.s", 2, 9 },
        { 16, "", "file0.s", 9, 9 },
        { 24, "", "file0.s", 9, 9 },
        { 24, "", "file0.s", 15, 9 },
        { 33, "", "file0.s", 16, 9 },
        { 40, "", "file0.s", 24, 9 }
    },
    // 24 - many sources and macros
    {
        { 0, "", "file0.s", 1, 9 },
        { 4, "", "file1.s", 2, 10 },
        { 8, "", "file2.s", 3, 12 },
        { 12, "", "file0.s", 2, 5 },
        { 16, "macro0", "macro0.s", 1, 7 },
        { 20, "macro0", "macro0.s", 2, 7 },
        { 24, "macro0", "macro0.s", 4, 7 },
        { 32, "", "file0.s", 3, 6 },
        { 34, "maxx", "file0.s", 4, 6 },
        { 36, "maxx", "file0.s", 5, 6 },
        { 38, "maxx", "file1.s", 10, 6 },
        { 42, "maxx", "file0.s", 6, 6 },
        { 46, "", "file0.s", 8, 6 }
    },
    // 25 - many sources and macros
    {
        { 0, "", "file0.s", 1001, 9 },
        { 4, "", "file1.s", 2002, 10 },
        { 8, "", "file2.s", 4003, 12 },
        { 12, "", "file0.s", 1002, 5 },
        { 16, "macro0", "macro0.s", 77001, 1007 },
        { 20, "macro0", "macro0.s", 77002, 1007 },
        { 24, "macro0", "macro0.s", 77004, 1007 },
        { 32, "", "file0.s", 1003, 6 },
        { 34, "maxx", "file0.s", 1004, 6 },
        { 36, "maxx", "file0.s", 1055, 6 },
        { 38, "maxx", "file1.s", 4210, 6 },
        { 42, "maxx", "file0.s", 1066, 6 },
        { 46, "", "file0.s", 1288, 6 }
    },
    // 26 - big diffs in offset
    {
        { 100, "", "file0.s", 1, 9 },
        { 164, "", "file0.s", 2, 9 },
        { 229, "", "file0.s", 3, 4 },
        { 292, "", "file0.s", 4, 5 },
        { 24292, "", "file0.s", 6, 5 },
        { 24307, "", "file0.s", 8, 5 },
        { 24627, "", "file0.s", 11, 9 }
    },
    // 27 - big diffs in offset
    {
        { 100, "", "file0.s", 1, 9 },
        { 164, "", "file0.s", 1, 9 },
        { 228, "", "file0.s", 1, 9 }
    },
    // 28 - big diffs in offset 2
    {
        { 100, "", "file0.s", 1, 9 },
        { 164, "", "file0.s", 2, 9 },
        { 229, "", "file0.s", 3, 4 },
        { 292, "", "file0.s", 4, 5 },
        { 124292, "", "file0.s", 6, 5 },
        { 524307, "", "file0.s", 8, 5 },
        { 594627, "", "file0.s", 11, 9 },
        { 791112, "", "file0.s", 12, 9 },
        { 1472678, "", "file0.s", 14, 9 }
    },
    // 29 - bigger colNo
    {
        { 8, "", "file0.s", 1, 2331 },
        { 12, "", "file0.s", 2, 552567 },
        { 18, "", "file0.s", 2, 578567 },
        { 23, "", "file0.s", 3, 773215 },
        { 28, "", "file0.s", 3, 7 }
    }
};

static void testAsmSourcePosHandler(cxuint i, const SourcePosHandlerTestCase& testCase)
{
    AsmSourcePosHandler sourcePosHandler;
    std::unordered_map<CString, RefPtr<const AsmSource> > sourceMap;
    std::unordered_map<CString, RefPtr<const AsmMacroSubst> > macroMap;
    std::unordered_map<const AsmSource*, CString> revSourceMap;
    std::unordered_map<const AsmMacroSubst*, CString> revMacroMap;
    
    // prepping sourceMap and macroMap
    for (const SourcePosEntry& entry: testCase)
    {
        if (entry.sourceName[0] != 0) // if not empty sourceName
        {
            // insert if not exists
            auto res = sourceMap.insert({ entry.sourceName, RefPtr<const AsmSource>(
                new AsmFile(RefPtr<const AsmSource>(), 1, 2, entry.sourceName)) });
            if (res.second)
                revSourceMap.insert({ res.first->second.get(), entry.sourceName });
        }
        if (entry.macroName[0] != 0) // if not empty macroName
        {
            auto res = macroMap.insert({ entry.macroName, RefPtr<const AsmMacroSubst>(
                new AsmMacroSubst(RefPtr<const AsmSource>(), 1, 2)) });
            if (res.second)
                revMacroMap.insert({ res.first->second.get(), entry.macroName });
        }
    }
    
    // pushing entries
    for (const SourcePosEntry& entry: testCase)
    {
        RefPtr<const AsmSource> source;
        RefPtr<const AsmMacroSubst> macro;
        if (entry.sourceName[0] != 0)
            source = sourceMap.find(entry.sourceName)->second;
        if (entry.macroName[0] != 0)
            macro = macroMap.find(entry.macroName)->second;
        
        sourcePosHandler.pushSourcePos(entry.offset, AsmSourcePos{ macro, source,
                    entry.lineNo, entry.colNo, nullptr });
    }
    
    std::ostringstream oss;
    oss << "TestCase#" << i;
    const std::string testCaseName = oss.str();
    
    // testing reading
    AsmSourcePosHandler::ReadPos sourcePosPos{ 0, 0 };
    for (size_t j = 0; j < testCase.size(); j++)
    {
        const SourcePosEntry& entry = testCase[j];
        std::ostringstream eOss;
        eOss << "entry#" << j;
        eOss.flush();
        const std::string eName = eOss.str();
        
        if (!sourcePosHandler.hasNext(sourcePosPos))
            assertTrue(testCaseName, eName+".size match",
                       sourcePosHandler.hasNext(sourcePosPos));
        
        const std::pair<size_t, AsmSourcePos> result = sourcePosHandler.nextSourcePos(
                    sourcePosPos);
        CString resultSourceName;
        CString resultMacroName;
        auto sit = revSourceMap.find(result.second.source.get());
        if (sit != revSourceMap.end())
            resultSourceName = sit->second;
        
        auto mit = revMacroMap.find(result.second.macro.get());
        if (mit != revMacroMap.end())
            resultMacroName = mit->second;
        
        assertValue(testCaseName, eName+".offset", entry.offset, result.first);
        assertString(testCaseName, eName+".macro", entry.macroName, resultMacroName);
        assertString(testCaseName, eName+".source", entry.sourceName, resultSourceName);
        assertValue(testCaseName, eName+".lineNo", entry.lineNo, result.second.lineNo);
        assertValue(testCaseName, eName+".colNo", entry.colNo, result.second.colNo);
    }
    assertTrue(testCaseName, "noNext", !sourcePosHandler.hasNext(sourcePosPos));
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(sourcePoHandlerTestTbl)/
                    sizeof(SourcePosHandlerTestCase); i++)
        try
        { testAsmSourcePosHandler(i, sourcePoHandlerTestTbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
