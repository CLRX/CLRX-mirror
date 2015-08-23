/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
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
#include "../TestUtils.h"

using namespace CLRX;

struct GCNAsmOpcodeCase
{
    const char* input;
    uint32_t expWord0, expWord1;
    bool twoWords;
    bool good;
    const char* errorMessages;
};

static const GCNAsmOpcodeCase encGCNOpcodeCases[] =
{
    { "    s_add_u32       s21, s4, s61", 0x80153d04U, 0, false, true, "" },
    /* registers and literals */
    { "    s_add_u32       vcc_lo, s4, s61", 0x806a3d04U, 0, false, true, "" },
    { "    s_add_u32       vcc_hi, s4, s61", 0x806b3d04U, 0, false, true, "" },
    { "    s_add_u32       tba_lo, s4, s61", 0x806c3d04U, 0, false, true, "" },
    { "    s_add_u32       tba_hi, s4, s61", 0x806d3d04U, 0, false, true, "" },
    { "    s_add_u32       tma_lo, s4, s61", 0x806e3d04U, 0, false, true, "" },
    { "    s_add_u32       tma_hi, s4, s61", 0x806f3d04U, 0, false, true, "" },
    { "    s_add_u32       ttmp0, s4, s61", 0x80703d04U, 0, false, true, "" },
    { "    s_add_u32       ttmp1, s4, s61", 0x80713d04U, 0, false, true, "" },
    { "    s_add_u32       ttmp[2:2], s4, s61", 0x80723d04U, 0, false, true, "" },
    { "    s_add_u32       ttmp[2], s4, s61", 0x80723d04U, 0, false, true, "" },
    { "    s_add_u32       s[21:21], s4, s61", 0x80153d04U, 0, false, true, "" },
    { "    s_add_u32       s[21], s[4], s[61]", 0x80153d04U, 0, false, true, "" },
    { "    s_add_u32       s21, s4, 1234", 0x8015ff04U, 1234, true, true, "" },
    { "    s_add_u32       s21, 1234, s4", 0x801504ffU, 1234, true, true, "" },
    /* 64-bit registers and literals */
    { "        s_xor_b64       s[22:23], s[4:5], s[62:63]\n",
        0x89963e04U, 0, false, true, "" },
    { "        s_xor_b64       vcc, s[4:5], s[62:63]\n",
        0x89ea3e04U, 0, false, true, "" },
    { "        s_xor_b64       tba, s[4:5], s[62:63]\n",
        0x89ec3e04U, 0, false, true, "" },
    { "        s_xor_b64       tma, s[4:5], s[62:63]\n",
        0x89ee3e04U, 0, false, true, "" },
    { "        s_xor_b64       ttmp[4:5], s[4:5], s[62:63]\n",
        0x89f43e04U, 0, false, true, "" },
    { "        s_xor_b64       exec, s[4:5], s[62:63]\n",
        0x89fe3e04U, 0, false, true, "" },
    { nullptr, 0, 0, false, false, 0 }
};

static void testEncGCNOpcodes(cxuint i, const GCNAsmOpcodeCase& testCase,
                      GPUDeviceType deviceType)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    
    Assembler assembler("test.s", input, ASM_ALL, BinaryFormat::GALLIUM, deviceType,
                    errorStream);
    bool good = assembler.assemble();
    std::ostringstream oss;
    oss << getGPUDeviceTypeName(deviceType) << " encGCNCase#" << i;
    const std::string testCaseName = oss.str();
    assertValue<bool>("testEncGCNOpcodes", testCaseName+".good", testCase.good, good);
    if (assembler.getSections().size()!=1)
    {
        std::ostringstream oss;
        oss << "FAILED for " << getGPUDeviceTypeName(deviceType) <<
            " encGCNCase#" << i;
        throw Exception(oss.str());
    }
    const AsmSection& section = assembler.getSections()[0];
    const size_t codeSize = section.content.size();
    const size_t expectedSize = (testCase.good) ? ((testCase.twoWords)?8:4) : 0;
    if (codeSize != expectedSize)
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
        uint32_t expectedWord0 = testCase.expWord0;
        uint32_t expectedWord1 = testCase.expWord1;
        uint32_t resultWord0 = ULEV(*reinterpret_cast<const uint32_t*>(
                    section.content.data()));
        uint32_t resultWord1;
        if (expectedSize==8)
            resultWord1 = ULEV(*reinterpret_cast<const uint32_t*>(
                        section.content.data()+4));
        
        if (expectedWord0!=resultWord0 || (expectedSize==8 && expectedWord1!=resultWord1))
        {
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
    
    return retVal;
}
