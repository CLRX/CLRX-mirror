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
#include <cstdio>
#include <sstream>
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "../TestUtils.h"
#include "AsmBasics.h"

using namespace CLRX;

typedef std::pair<CString, const AsmSymbol*> AsmSymbolEntryC;

// recursive function to push symbols from scope adding to name scope prefixes
static void pushSymbolsFromScopes(const AsmScope& scope,
            std::vector<AsmSymbolEntryC>& symEntries, const std::string& prefix)
{
    // push symbol in this scope adding to its name a prefix
    for (const AsmSymbolEntry& symEntry: scope.symbolMap)
    {
        std::string symName = prefix+symEntry.first.c_str();
        symEntries.push_back(AsmSymbolEntryC(CString(symName.c_str()), &symEntry.second));
    }
    // traverse through scopes
    for (const auto& scopeEntry: scope.scopeMap)
    {
        std::string newPrefix = prefix+scopeEntry.first.c_str()+"::";
        pushSymbolsFromScopes(*scopeEntry.second, symEntries, newPrefix);
    }
}

static void testAssembler(cxuint testSuiteId, cxuint testId, const AsmTestCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    std::ostringstream printStream;
    
    // create assembler with testcase input
    // enable ASM_TESTRUN (needed while testing)
    Assembler assembler("test.s", input, (ASM_ALL|ASM_TESTRUN)&~ASM_ALTMACRO,
            BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, errorStream, printStream);
    // include include dirs from testcase
    for (const char* incDir: testCase.includeDirs)
        assembler.addIncludeDir(incDir);
    // just assemble
    bool good = assembler.assemble();
    /* compare results */
    char testName[30];
    snprintf(testName, 30, "Test%u #%u", testSuiteId, testId);
    
    // check whether good, format, device, bitness is match
    assertValue(testName, "good", int(testCase.good), int(good));
    assertValue(testName, "format", int(testCase.format),
                int(assembler.getBinaryFormat()));
    assertValue(testName, "deviceType", int(testCase.deviceType),
                int(assembler.getDeviceType()));
    assertValue(testName, "64bit", int(testCase.is64Bit), int(assembler.is64Bit()));
    
    // check kernels
    const std::vector<AsmKernel>& resKernels = assembler.getKernels();
    assertValue(testName, "kernels.length", testCase.kernelNames.size(), resKernels.size());
    for (size_t i = 0; i < testCase.kernelNames.size(); i++)
    {
        std::ostringstream caseOss;
        caseOss << "Kernel#" << i << ".";
        caseOss.flush();
        std::string caseName(caseOss.str());
        assertString(testName, caseName+"name", testCase.kernelNames[i],
                     resKernels[i].name);
    }
    
    // check sections
    const std::vector<AsmSection>& resSections = assembler.getSections();
    assertValue(testName, "sections.length", testCase.sections.size(), resSections.size());
    for (size_t i = 0; i < testCase.sections.size(); i++)
    {
        std::ostringstream caseOss;
        caseOss << "Section#" << i << ".";
        caseOss.flush();
        std::string caseName(caseOss.str());
        
        const AsmSection& resSection = resSections[i];
        const Section& expSection = testCase.sections[i];
        assertString(testName, caseName+"name", expSection.name, resSection.name);
        assertValue(testName, caseName+"kernelId", expSection.kernelId,
                    resSection.kernelId);
        assertValue(testName, caseName+"type", int(expSection.type), int(resSection.type));
        assertArray<cxbyte>(testName, caseName+"content", expSection.content,
                    resSection.content);
    }
    // check symbols
    std::vector<AsmSymbolEntryC> symEntries;
    // push symbols recursive traversing through scopes (begins from global)
    pushSymbolsFromScopes(assembler.getGlobalScope(), symEntries, "");
    std::sort(symEntries.begin(), symEntries.end(),
                [](const AsmSymbolEntryC& s1, const AsmSymbolEntryC& s2)
                { return s1.first < s2.first; });
    
    assertValue(testName, "symbols.length", testCase.symbols.size(), symEntries.size());
    
    for (cxuint i = 0; i < testCase.symbols.size(); i++)
    {
        std::ostringstream caseOss;
        char buf[32];
        snprintf(buf, 32, "Symbol#%u.", i);
        std::string caseName(buf);
        
        // compare expected symbols with result symbols
        const AsmSymbolEntryC& resSymbol = symEntries[i];
        const SymEntry& expSymbol = testCase.symbols[i];
        assertString(testName,caseName+"name", expSymbol.name, resSymbol.first);
        assertValue(testName,caseName+"value", expSymbol.value, resSymbol.second->value);
        assertValue(testName,caseName+"sectId", expSymbol.sectionId,
                     resSymbol.second->sectionId);
        assertValue(testName,caseName+"size", expSymbol.size, resSymbol.second->size);
        assertValue(testName,caseName+"isDefined", int(expSymbol.hasValue),
                    int(resSymbol.second->hasValue));
        assertValue(testName,caseName+"onceDefined", int(expSymbol.onceDefined),
                    int(resSymbol.second->onceDefined));
        assertValue(testName,caseName+"base", int(expSymbol.base),
                    int(resSymbol.second->base));
        assertValue(testName,caseName+"info", int(expSymbol.info),
                    int(resSymbol.second->info));
        assertValue(testName,caseName+"other", int(expSymbol.other),
                    int(resSymbol.second->other));
        assertValue(testName,caseName+"regRange", int(expSymbol.regRange),
                    int(resSymbol.second->regRange));
    }
    errorStream.flush();
    printStream.flush();
    const std::string errorMsgs = errorStream.str();
    const std::string printMsgs = printStream.str();
    assertString(testName, "errorMessages", testCase.errorMessages, errorMsgs);
    assertString(testName, "printMessages", testCase.printMessages, printMsgs);
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; asmTestCases1Tbl[i].input != nullptr; i++)
        try
        { testAssembler(0, i, asmTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (size_t i = 0; asmTestCases2Tbl[i].input != nullptr; i++)
        try
        { testAssembler(1, i, asmTestCases2Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
