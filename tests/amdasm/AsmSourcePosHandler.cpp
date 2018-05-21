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
        { 17, "", "file0.s", 3, 4 }
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
    sourcePosHandler.rewind();
    for (size_t j = 0; j < testCase.size(); j++)
    {
        const SourcePosEntry& entry = testCase[j];
        std::ostringstream eOss;
        eOss << "entry#" << j;
        eOss.flush();
        const std::string eName = eOss.str();
        
        if (!sourcePosHandler.hasNext())
            assertTrue(testCaseName, eName+".size match", sourcePosHandler.hasNext());
        
        const std::pair<size_t, AsmSourcePos> result = sourcePosHandler.nextSourcePos();
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
    assertTrue(testCaseName, "noNext", !sourcePosHandler.hasNext());
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
