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
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <memory>
#include <CLRX/amdasm/Assembler.h>
#include "../TestUtils.h"

using namespace CLRX;

static const char* galliumArgTypeTbl[] =
{
    "scalar", "constant", "global", "local", "image2dro", "image2dwr",
    "image3dro", "image3dwr", "sampler"
};

static const char* galliumArgSemanticTbl[] =
{   "general", "griddim", "gridoffset" };

static std::string confValueToString(uint32_t val)
{
    if (val == BINGEN_DEFAULT)
        return "default";
    if (val == BINGEN_NOTSUPPLIED)
        return "notsup";
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

static void printHexData(std::ostream& os, cxuint indentLevel, size_t size,
             const cxbyte* data)
{
    if (data==nullptr)
    {
        for (cxuint j = 0; j < indentLevel; j++)
            os << "  ";
        os << "nullptr\n";
        return;
    }
    for (size_t i = 0; i < size; i++)
    {
        if ((i&31)==0)
            for (cxuint j = 0; j < indentLevel; j++)
                os << "  ";
        char buf[10];
        snprintf(buf, 10, "%02x", cxuint(data[i]));
        os << buf;
        if ((i&31)==31 || i+1 == size)
            os << '\n';
    }
}

static void printGalliumOutput(std::ostream& os, const GalliumInput* output)
{
    os << "GalliumBinDump:" << std::endl;
    for (const GalliumKernelInput& kernel: output->kernels)
    {
        os << "  Kernel: name=" << kernel.kernelName << ", " <<
                "offset=" << kernel.offset << "\n";
        if (!kernel.useConfig)
        {
            os << "    ProgInfo: ";
            for (cxuint i = 0; i < 3; i++)
                os << "0x" << std::hex << kernel.progInfo[i].address <<
                        "=0x" << kernel.progInfo[i].value << ((i==2)?"\n":", ");
            os << std::dec;
        }
        else
        {
            const GalliumKernelConfig& config = kernel.config;
            os << "    Config:\n";
            os << "      dims=" << confValueToString(config.dimMask) << ", "
                    "SGPRS=" << confValueToString(config.usedSGPRsNum) << ", "
                    "VGPRS=" << confValueToString(config.usedVGPRsNum) << ", "
                    "pgmRSRC2=" << std::hex << "0x" << config.pgmRSRC2 << ", "
                    "ieeeMode=0x" << cxuint(config.ieeeMode) << "\n      "
                    "floatMode=0x" << cxuint(config.floatMode) << std::dec << ", "
                    "priority=" << cxuint(config.priority) << ", "
                    "localSize=" << config.localSize << ", "
                    "scratchBuffer=" << config.scratchBufferSize << std::endl;
        }
        for (const GalliumArgInfo& arg: kernel.argInfos)
        {
            os << "    Arg: " << galliumArgTypeTbl[cxuint(arg.type)] << ", " <<
                    ((arg.signExtended) ? "true" : "false") << ", " <<
                    galliumArgSemanticTbl[cxuint(arg.semantic)] << ", " <<
                    "size=" << arg.size << ", tgtSize=" << arg.targetSize << ", " <<
                    "tgtAlign=" << arg.targetAlign << "\n";
        }
        os.flush();
    }
    os << "  Comment:\n";
    printHexData(os, 1, output->commentSize, (const cxbyte*)output->comment);
    os << "  GlobalData:\n";
    printHexData(os, 1, output->globalDataSize, output->globalData);
    os << "  Code:\n";
    printHexData(os, 1, output->codeSize, output->code);
    
    for (BinSection section: output->extraSections)
    {
        os << "  Section " << section.name << ", type=" << section.type <<
                        ", flags=" << section.flags << ":\n";
        printHexData(os, 1, section.size, section.data);
    }
    for (BinSymbol symbol: output->extraSymbols)
        os << "  Symbol: name=" << symbol.name << ", value=" << symbol.value <<
                ", size=" << symbol.size << ", section=" << symbol.sectionId << "\n";
    os.flush();
}

struct AsmTestCase
{
    const char* input;
    const char* dump;
    const char* errors;
    bool good;
};

static const AsmTestCase asmTestCases1Tbl[] =
{
    /* 0 - gallium  */
    {
        R"ffDXD(            .gallium
            .kernel firstKernel
            .args
            .arg scalar,8
            .arg local,4
            .arg constant,4
            .arg global,4
            .arg image3d_rd,4
            .arg image2d_rd,4
            .arg image3d_wr,4
            .arg image2d_wr,4
            .arg sampler,8
            .arg scalar,8, , ,sext
            .arg scalar,8, , ,sext,general
            .arg scalar,8, 18,2
            .arg scalar,20, 18,2
            .arg scalar,8, , , ,griddim
            .arg griddim,4
            .arg gridoffset,4
            .arg scalar, 11111111111, 22222222222222, 4
            .section .comment
            .ascii "nocomments"
            .globaldata
            .byte 0xf0,0xfd,0x3d,0x44
            .kernel secondKernel
            .proginfo
            .entry 12,22
            .entry 14,288
            .entry 16,160
            .args
            .arg scalar,4
            .arg scalar,4
            .arg griddim,4
            .arg gridoffset,4
            .kernel thirdKernel
            .proginfo
            .entry 0xfffffaaaaa, 0x12233
            .entry 0xff, 0x111223030
            .entry 1,2
            .text
firstKernel: .byte 1,22,3,4
            .p2align 4
secondKernel:.byte 77,76,75,90,11
thirdKernel:
            .section .info1
            .ascii "noinfo"
            .section .infox
            .ascii "refer to some link"
            .section .softX ,"awx",@nobits
            .section .softy ,"x",@note
            .section .softz ,"a",@progbits
            .section .softX ,"a",@nobits)ffDXD",
        /* dump */
        R"ffDXD(GalliumBinDump:
  Kernel: name=firstKernel, offset=0
    ProgInfo: 0xb848=0xc0000, 0xb84c=0x1788, 0xb860=0x0
    Arg: scalar, false, general, size=8, tgtSize=8, tgtAlign=8
    Arg: local, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: constant, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: global, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: image3dro, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: image2dro, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: image3dwr, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: image2dwr, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: sampler, false, general, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, true, general, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, true, general, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, false, general, size=8, tgtSize=18, tgtAlign=2
    Arg: scalar, false, general, size=20, tgtSize=18, tgtAlign=2
    Arg: scalar, false, griddim, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, false, griddim, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, gridoffset, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, general, size=2521176519, tgtSize=61432718, tgtAlign=4
  Kernel: name=secondKernel, offset=16
    ProgInfo: 0xc=0x16, 0xe=0x120, 0x10=0xa0
    Arg: scalar, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, general, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, griddim, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, gridoffset, size=4, tgtSize=4, tgtAlign=4
  Kernel: name=thirdKernel, offset=21
    ProgInfo: 0xfffaaaaa=0x12233, 0xff=0x11223030, 0x1=0x2
  Comment:
  6e6f636f6d6d656e7473
  GlobalData:
  f0fd3d44
  Code:
  01160304000080bf000080bf000080bf4d4c4b5a0b
  Section .info1, type=1, flags=0:
  6e6f696e666f
  Section .infox, type=1, flags=0:
  726566657220746f20736f6d65206c696e6b
  Section .softX, type=8, flags=7:
  Section .softy, type=7, flags=4:
  Section .softz, type=1, flags=2:
)ffDXD",
        "test.s:20:26: Warning: Size of argument out of range\n"
        "test.s:20:39: Warning: Target size of argument out of range\n"
        "test.s:37:20: Warning: Value 0xfffffaaaaa truncated to 0xfffaaaaa\n"
        "test.s:38:26: Warning: Value 0x111223030 truncated to 0x11223030\n"
        "test.s:52:13: Warning: Section type, flags and alignment was ignored\n", true
    },
    /* 1 - gallium (configured proginfo) */
    { R"ffDXD(            .gallium
            .kernel aa22
            .args
            .arg scalar, 8,,,SEXT,griddim
            .config
            .priority 1
            .floatmode 43
            .ieeemode
            .sgprsnum 36
            .vgprsnum 139
            .pgmrsrc2 523243
            .scratchbuffer 230
            .kernel aa23
            .args
            .arg scalar, 8,,,SEXT,griddim
            .config
            .dims yz
            .priority 3
            .ieeemode
            .pgmrsrc2 0
.text
aa22:
aa23:)ffDXD",
       R"ffDXD(GalliumBinDump:
  Kernel: name=aa22, offset=0
    Config:
      dims=default, SGPRS=36, VGPRS=139, pgmRSRC2=0x7fbeb, ieeeMode=0x1
      floatMode=0x2b, priority=1, localSize=0, scratchBuffer=230
    Arg: scalar, true, griddim, size=8, tgtSize=8, tgtAlign=8
  Kernel: name=aa23, offset=0
    Config:
      dims=6, SGPRS=6, VGPRS=3, pgmRSRC2=0x0, ieeeMode=0x1
      floatMode=0xc0, priority=3, localSize=0, scratchBuffer=0
    Arg: scalar, true, griddim, size=8, tgtSize=8, tgtAlign=8
  Comment:
  nullptr
  GlobalData:
  nullptr
  Code:
)ffDXD", "", true
    },
    /* 2 - gallium (errors) */
    {
        R"ffDXD(            .gallium
            .kernel firstKernel
            .entry 111,1
            .arg scalar,8
            .arg local,4
            .arg constant,4
            .args
            .arg sclar,5
            .arg scalar
            .arg image3d_rd,0,0,1
            .arg sampler,4,4,4     , zxx,    ssds
            .arg scalar,8, 18,2,zext,general ,
            .proginfo
            .entry ,
            .entry  ,66
            .entry 66,
            .kernel secondKernel
            .proginfo
            .entry 1,2
            .entry 2,3
            .entry 3,4
            .entry 5,6
            .args
            .proginfo
            .entry 7,8
            .section .txt3, "a  ", @xxx
            .section .txt3, "ax", x
            .section .txt3, "a vcxs", @  )ffDXD",
        /* dump */
        "",
        /* errors */
        R"ffDXD(test.s:3:13: Error: ProgInfo entry definition outside ProgInfo
test.s:4:13: Error: Argument definition outside arguments list
test.s:5:13: Error: Argument definition outside arguments list
test.s:6:13: Error: Argument definition outside arguments list
test.s:8:18: Error: Unknown argument type
test.s:9:24: Error: Expected ',' before argument
test.s:10:29: Warning: Size of argument out of range
test.s:10:31: Warning: Target size of argument out of range
test.s:11:38: Error: Unknown numeric extension
test.s:11:46: Error: Unknown argument semantic
test.s:12:46: Error: Garbages at end of line
test.s:14:20: Error: Expected expression
test.s:14:21: Error: Expected expression
test.s:15:21: Error: Expected expression
test.s:16:23: Error: Expected expression
test.s:22:13: Error: Maximum 3 entries can be in ProgInfo
test.s:25:13: Error: Maximum 3 entries can be in ProgInfo
test.s:26:29: Error: Only 'a', 'w', 'x' is accepted in flags string
test.s:26:36: Error: Unknown section type
test.s:27:35: Error: Section type was not preceded by '@'
test.s:28:29: Error: Only 'a', 'w', 'x' is accepted in flags string
test.s:28:39: Error: Section type was not preceded by '@'
)ffDXD", false
    },
    {
        R"ffDXD(            .gallium
            .kernel aa22
            .config
            .proginfo
            .kernel av77
            .proginfo
            .config
            .kernel aa22
            .args
            .arg scalar, 8,,,SEXT,griddim
            .config
            .priority 7
            .floatmode 343
            .ieeemode
            .sgprsnum 136
            .vgprsnum 339
)ffDXD", "",
R"ffDXD(test.s:4:13: Error: ProgInfo can't be defined if configuration was exists
test.s:7:13: Error: Configuration can't be defined if progInfo was defined
test.s:12:23: Warning: Value 0x7 truncated to 0x3
test.s:13:24: Warning: Value 0x157 truncated to 0x57
test.s:15:23: Error: Used SGPRs number out of range (0-104)
test.s:16:23: Error: Used VGPRs number out of range (0-256)
)ffDXD", false
    },
};

static void testAssembler(cxuint testId, const AsmTestCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    std::ostringstream printStream;
    
    Assembler assembler("test.s", input, (ASM_ALL|ASM_TESTRUN)&~ASM_ALTMACRO,
            BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, errorStream, printStream);
    bool good = assembler.assemble();
    
    std::ostringstream dumpOss;
    if (good && assembler.getFormatHandler()!=nullptr)
        // get format handler and their output
        printGalliumOutput(dumpOss, static_cast<const AsmGalliumHandler*>(
                    assembler.getFormatHandler())->getOutput());
    /* compare results */
    char testName[30];
    snprintf(testName, 30, "Test #%u", testId);
    
    assertValue(testName, "good", int(testCase.good), int(good));
    assertString(testName, "dump", testCase.dump, dumpOss.str());
    assertString(testName, "errorMessages", testCase.errors, errorStream.str());
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (size_t i = 0; i < sizeof(asmTestCases1Tbl)/sizeof(AsmTestCase); i++)
        try
        { testAssembler(i, asmTestCases1Tbl[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
