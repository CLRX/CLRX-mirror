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
#include <sstream>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Disassembler.h>
#include <CLRX/utils/MemAccess.h>
#include "GCNDisasmOpc.h"

using namespace CLRX;

static void testDecGCNOpcodes(cxuint i, const GCNDisasmOpcodeCase& testCase,
                      GPUDeviceType deviceType)
{
    std::ostringstream disOss;
    AmdDisasmInput input;
    // set device type
    input.deviceType = deviceType;
    input.is64BitMode = false;
    // set up GCN disassembler
    Disassembler disasm(&input, disOss, DISASM_FLOATLITS);
    GCNDisassembler gcnDisasm(disasm);
    // create input code
    uint32_t inputCode[2] = { LEV(testCase.word0), LEV(testCase.word1) };
    gcnDisasm.setInput(testCase.twoWords?8:4, reinterpret_cast<cxbyte*>(inputCode));
    // disassemble
    gcnDisasm.disassemble();
    std::string outStr = disOss.str();
    // compare output
    if (outStr != testCase.expected)
    {
        // throw exception with detailed info
        std::ostringstream oss;
        oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
            " decGCNCase#" << i << ": size=" << (testCase.twoWords?2:1) <<
            ", word0=0x" << std::hex << testCase.word0 << std::dec;
        if (testCase.twoWords)
            oss << ", word1=0x" << std::hex << testCase.word1 << std::dec;
        oss << "\nExpected: " << testCase.expected << ", Result: " << outStr;
        throw Exception(oss.str());
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; decGCNOpcodeCases[i].expected!=nullptr; i++)
        try
        { testDecGCNOpcodes(i, decGCNOpcodeCases[i], GPUDeviceType::PITCAIRN); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; decGCNOpcodeGCN11Cases[i].expected!=nullptr; i++)
        try
        { testDecGCNOpcodes(i, decGCNOpcodeGCN11Cases[i], GPUDeviceType::HAWAII); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; decGCNOpcodeGCN12Cases[i].expected!=nullptr; i++)
        try
        { testDecGCNOpcodes(i, decGCNOpcodeGCN12Cases[i], GPUDeviceType::TONGA); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; decGCNOpcodeGCN14Cases[i].expected!=nullptr; i++)
        try
        { testDecGCNOpcodes(i, decGCNOpcodeGCN14Cases[i], GPUDeviceType::GFX900); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    for (cxuint i = 0; decGCNOpcodeGCN141Cases[i].expected!=nullptr; i++)
        try
        { testDecGCNOpcodes(i, decGCNOpcodeGCN141Cases[i], GPUDeviceType::GFX906); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
