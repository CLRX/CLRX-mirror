/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
#include <cstdio>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "../TestUtils.h"

using namespace CLRX;

struct KernelRegPool
{
    const char* kernelName;
    cxuint sgprsNum;
    cxuint vgprsNum;
};

struct AsmRegPoolTestCase
{
    const char* input;
    const Array<KernelRegPool> regPools;
};

static const AsmRegPoolTestCase regPoolTestCasesTbl[] =
{   /* gcn asm test cases */
    { ".amd;.kernel xx;.config;.text;s_add_u32 s5,s0,s1", { { "xx", 6, 0 } } }
};

static void testAsmRegPoolTestCase(cxuint testId, const AsmRegPoolTestCase& testCase)
{
    char testName[30];
    snprintf(testName, 30, "Test #%u", testId);
    
    std::istringstream input(testCase.input);
    Assembler assembler("test.s", input, ASM_ALL, BinaryFormat::AMD,
            GPUDeviceType::CAPE_VERDE);
    assertTrue(testName, "good", assembler.assemble());
    Array<KernelRegPool> resRegPools;
    // retrieve data
    if (assembler.getBinaryFormat()==BinaryFormat::AMD)
    {   // Catalyst
        const AmdInput* input = static_cast<const AsmAmdHandler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->kernels.size());
        
        char buf[32];
        for (cxuint i = 0; i < input->kernels.size(); i++)
        {
            const AmdKernelInput& kinput = input->kernels[i];
            const KernelRegPool& regPool = testCase.regPools[i];
            snprintf(buf, 32, "Kernel=%u.", i);
            std::string caseName(buf);
            assertString(testName, caseName+"name", regPool.kernelName, kinput.kernelName);
            assertTrue(testName, caseName+"useConfig", kinput.useConfig);
            assertValue(testName, caseName+"sgprsNum", regPool.sgprsNum,
                        kinput.config.usedSGPRsNum);
            assertValue(testName, caseName+"vgprsNum", regPool.vgprsNum,
                        kinput.config.usedVGPRsNum);
        }
    }
    if (assembler.getBinaryFormat()==BinaryFormat::GALLIUM)
    {   // GalliumCompute
        const GalliumInput* input = static_cast<const AsmGalliumHandler*>(
                    assembler.getFormatHandler())->getOutput();
        assertTrue(testName, "input!=nullptr", input!=nullptr);
        assertValue(testName, "kernels.length", testCase.regPools.size(),
                    input->kernels.size());
        char buf[32];
        for (cxuint i = 0; i < input->kernels.size(); i++)
        {
            const GalliumKernelInput& kinput = input->kernels[i];
            const KernelRegPool& regPool = testCase.regPools[i];
            snprintf(buf, 32, "Kernel=%u.", i);
            std::string caseName(buf);
            assertString(testName, caseName+"name", regPool.kernelName, kinput.kernelName);
            assertTrue(testName, caseName+"useConfig", kinput.useConfig);
            assertValue(testName, caseName+"sgprsNum", regPool.sgprsNum,
                        kinput.config.usedSGPRsNum);
            assertValue(testName, caseName+"vgprsNum", regPool.vgprsNum,
                        kinput.config.usedVGPRsNum);
        }
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(regPoolTestCasesTbl)/sizeof(AsmRegPoolTestCase); i++)
        try
        { testAsmRegPoolTestCase(i, regPoolTestCasesTbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
