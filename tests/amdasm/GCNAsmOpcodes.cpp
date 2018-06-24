/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2018 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope thaimplied warranty of
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
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/MemAccess.h>
#include "GCNAsmOpc.h"
#include "../TestUtils.h"

using namespace CLRX;

static void testEncGCNOpcodes(cxuint i, const GCNAsmOpcodeCase& testCase,
                      GPUDeviceType deviceType)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    // create assembler with input stream (content is string
    Assembler assembler("test.s", input, ASM_ALL&~ASM_ALTMACRO,
                    BinaryFormat::GALLIUM, deviceType, errorStream);
    // try to assemble code
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << getGPUDeviceTypeName(deviceType) << " encGCNCase#" << i;
    const std::string testCaseName = oss.str();
    // check is good match in testcase
    assertValue<bool>("testEncGCNOpcodes", testCaseName+".good", testCase.good, good);
    if (assembler.getSections().size()<1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
            " encGCNCase#" << i;
        throw Exception(oss.str());
    }
    const AsmSection& section = assembler.getSections()[0];
    const size_t codeSize = section.content.size();
    const size_t expectedSize = (testCase.good) ? ((testCase.twoWords)?8:4) : 0;
    // check size of output content
    if (good && codeSize != expectedSize)
    {
        std::ostringstream oss;
        oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
            " encGCNCase#" << i << ". Wrong code size: " << expectedSize << "!=" <<
            codeSize;
        throw Exception(oss.str());
    }
    // check content
    if (expectedSize!=0)
    {
        // get expected words and result (output) words
        uint32_t expectedWord0 = testCase.expWord0;
        uint32_t expectedWord1 = testCase.expWord1;
        uint32_t resultWord0 = ULEV(*reinterpret_cast<const uint32_t*>(
                    section.content.data()));
        uint32_t resultWord1 = 0;
        if (expectedSize==8)
            resultWord1 = ULEV(*reinterpret_cast<const uint32_t*>(
                        section.content.data()+4));
        
        if (expectedWord0!=resultWord0 || (expectedSize==8 && expectedWord1!=resultWord1))
        {
            // if content doesn't match
            std::ostringstream oss;
            oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
                " encGCNCase#" << i << ". Content doesn't match: 0x" <<
                std::hex << expectedWord0 << "!=0x" << resultWord0 << std::dec;
            if (expectedSize==8)
                oss << ", 0x" << std::hex << expectedWord1 << "!=0x" <<
                            resultWord1 << std::dec;
            throw Exception(oss.str());
        }
    }
    // check error messages
    assertString("testEncGCNOpcodes", testCaseName+".errorMessages",
              testCase.errorMessages, errorStream.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; encGCNOpcodeCases[i].input!=nullptr; i++)
        try
        { testEncGCNOpcodes(i, encGCNOpcodeCases[i], GPUDeviceType::PITCAIRN); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; encGCN11OpcodeCases[i].input!=nullptr; i++)
        try
        { testEncGCNOpcodes(i, encGCN11OpcodeCases[i], GPUDeviceType::BONAIRE); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; encGCN12OpcodeCases[i].input!=nullptr; i++)
        try
        { testEncGCNOpcodes(i, encGCN12OpcodeCases[i], GPUDeviceType::TONGA); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; encGCN14OpcodeCases[i].input!=nullptr; i++)
        try
        { testEncGCNOpcodes(i, encGCN14OpcodeCases[i], GPUDeviceType::GFX900); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; encGCN141OpcodeCases[i].input!=nullptr; i++)
        try
        { testEncGCNOpcodes(i, encGCN141OpcodeCases[i], GPUDeviceType::GFX906); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
