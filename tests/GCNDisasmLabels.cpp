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
#include <sstream>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/utils/MemAccess.h>

using namespace CLRX;

struct GCNDisasmLabelCase
{
    size_t wordsNum;
    const uint32_t* words;
    const char* expected;
};

static const uint32_t code1tbl[] = { 0xd8dc2625U, 0x37000006U, 0xbf82fffeU };
static const uint32_t code2tbl[] = { 0x7c6b92ffU };
static const uint32_t code3tbl[] = { 0xd8dc2625U, 0x37000006U, 0xbf82fffeU, 0xbf820002U,
    0xea88f7d4U, 0x23f43d12U, 0xd25a0037U, 0x4002b41bU
};
static const uint32_t code4tbl[] = { 0xbf820243U, 0xbf820106U, 0xbf820105U };

static const GCNDisasmLabelCase decGCNLabelCases[] =
{
    {
        3, code1tbl,
        "        ds_read2_b32    v[55:56], v6 offset0:37 offset1:38\n"
        ".org .-4\n.L1:\n.org .+4\n        s_branch        .L1\n"
    },
    {
        1, code2tbl,
        "        /* WARNING: Unfinished instruction at end! */\n"
        "        v_cmpx_lg_f64   vcc, 0x0, v[201:202]\n"
    },
    {
        8, code3tbl,
        "        ds_read2_b32    v[55:56], v6 offset0:37 offset1:38\n"
        ".org .-4\n.L1:\n.org .+4\n        s_branch        .L1\n"
        "        s_branch        .L6\n"
        "        tbuffer_load_format_x v[61:62], v[18:19], s[80:83], s35"
        " offen idxen offset:2004 glc slc addr64 tfe format:[8,sint]\n"
        ".L6:\n        v_cvt_pknorm_i16_f32 v55, s27, -v90\n"
    },
    {
        3, code4tbl, "        s_branch        .L580\n        s_branch        .L264\n"
        "        s_branch        .L264\n.org 0x108\n.L264:\n.org 0x244\n.L580:\n"
    }
};

static void testDecGCNLabels(cxuint i, const GCNDisasmLabelCase& testCase,
                      GPUDeviceType deviceType)
{
    std::ostringstream disOss;
    AmdDisasmInput input;
    input.deviceType = deviceType;
    input.is64BitMode = false;
    Disassembler disasm(&input, disOss, DISASM_FLOATLITS);
    GCNDisassembler gcnDisasm(disasm);
    uint32_t* code = nullptr;
    try
    {
        code = new uint32_t[testCase.wordsNum];
        for (cxuint i = 0; i < testCase.wordsNum; i++)
            code[i] = LEV(testCase.words[i]);
        
        gcnDisasm.setInput(testCase.wordsNum<<2,
               reinterpret_cast<const cxbyte*>(code));
        gcnDisasm.beforeDisassemble();
        gcnDisasm.disassemble();
        std::string outStr = disOss.str();
        if (outStr != testCase.expected)
        {
            std::ostringstream oss;
            oss << "FAILED for " <<
                (deviceType==GPUDeviceType::HAWAII?"Hawaii":"Pitcairn") <<
                " decGCNCase#" << i << ": size=" << (testCase.wordsNum) << std::endl;
            oss << "\nExpected: " << testCase.expected << ", Result: " << outStr;
            throw Exception(oss.str());
        }
    }
    catch(...)
    {
        delete[] code;
        throw;
    }
    delete[] code;
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(decGCNLabelCases)/sizeof(GCNDisasmLabelCase); i++)
        try
        { testDecGCNLabels(i, decGCNLabelCases[i], GPUDeviceType::PITCAIRN); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
