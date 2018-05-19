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
#include <string>
#include <algorithm>
#include <memory>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include "../TestUtils.h"

using namespace CLRX;

static const char* galliumArgTypeTbl[] =
{
    "scalar", "constant", "global", "local", "image2dro", "image2dwr",
    "image3dro", "image3dwr", "sampler"
};

static const char* galliumArgSemanticTbl[] =
{   "general", "griddim", "gridoffset" };

// helper for printing value from kernel config (print default and notsupplied)
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

static std::string confDimMaskToString(uint32_t val)
{
    if (val == BINGEN_DEFAULT)
        return "default";
    if (val == BINGEN_NOTSUPPLIED)
        return "notsup";
    std::ostringstream oss;
    oss << (val&0xff);
    return oss.str();
}

// print hex data or nullptr
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

// print dump of gallium output to stream for comparing with testcase
static void printGalliumOutput(std::ostream& os, const GalliumInput* output, bool amdHsa)
{
    os << "GalliumBinDump:" << std::endl;
    for (const GalliumKernelInput& kernel: output->kernels)
    {
        os << "  Kernel: name=" << kernel.kernelName << ", " <<
                "offset=" << kernel.offset << "\n";
        if (!kernel.useConfig)
        {
            // if not configuration, then print same prog info entries (only old gallium)
            os << "    ProgInfo: ";
            for (cxuint i = 0; i < 3; i++)
                os << "0x" << std::hex << kernel.progInfo[i].address <<
                        "=0x" << kernel.progInfo[i].value << ((i==2)?"\n":", ");
            os << std::dec;
        }
        else
        {
            // print kernel config
            const GalliumKernelConfig& config = kernel.config;
            os << "    Config:\n";
            os << "      dims=" << confDimMaskToString(config.dimMask) << ", "
                    "SGPRS=" << confValueToString(config.usedSGPRsNum) << ", "
                    "VGPRS=" << confValueToString(config.usedVGPRsNum) << ", "
                    "pgmRSRC2=" << std::hex << "0x" << config.pgmRSRC2 << ", "
                    "ieeeMode=0x" << cxuint(config.ieeeMode) << "\n      "
                    "floatMode=0x" << cxuint(config.floatMode) << std::dec << ", "
                    "priority=" << cxuint(config.priority) << ", "
                    "localSize=" << config.localSize << ", "
                    "scratchBuffer=" << config.scratchBufferSize << std::endl;
            if (amdHsa)
            {
                // print also AMD HSA configuration
                const AmdHsaKernelConfig& config =
                    *reinterpret_cast<const AmdHsaKernelConfig*>(
                                output->code + kernel.offset);
                os << "    AMD HSA Config:\n"
                    "      amdCodeVersion=" << ULEV(config.amdCodeVersionMajor) << "." <<
                        ULEV(config.amdCodeVersionMinor) << "\n"
                    "      amdMachine=" << ULEV(config.amdMachineKind) << ":" <<
                        ULEV(config.amdMachineMajor) << ":" <<
                        ULEV(config.amdMachineMinor) << ":" <<
                        ULEV(config.amdMachineStepping) << "\n"
                    "      kernelCodeEntryOffset=" <<
                        ULEV(config.kernelCodeEntryOffset) << "\n"
                    "      kernelCodePrefetchOffset=" <<
                        ULEV(config.kernelCodePrefetchOffset) << "\n"
                    "      kernelCodePrefetchSize=" <<
                            ULEV(config.kernelCodePrefetchSize) << "\n"
                    "      maxScrachBackingMemorySize=" <<
                        ULEV(config.maxScrachBackingMemorySize) << "\n"
                    "      computePgmRsrc1=0x" << std::hex <<
                            ULEV(config.computePgmRsrc1) << "\n"
                    "      computePgmRsrc2=0x" << ULEV(config.computePgmRsrc2) << "\n"
                    "      enableSgprRegisterFlags=0x" <<
                        ULEV(config.enableSgprRegisterFlags) << "\n"
                    "      enableFeatureFlags=0x" <<
                        ULEV(config.enableFeatureFlags) << std::dec << "\n"
                    "      workitemPrivateSegmentSize=" <<
                        ULEV(config.workitemPrivateSegmentSize) << "\n"
                    "      workgroupGroupSegmentSize=" <<
                        ULEV(config.workgroupGroupSegmentSize) << "\n"
                    "      gdsSegmentSize=" << ULEV(config.gdsSegmentSize) << "\n"
                    "      kernargSegmentSize=" << ULEV(config.kernargSegmentSize) << "\n"
                    "      workgroupFbarrierCount=" <<
                            ULEV(config.workgroupFbarrierCount) << "\n"
                    "      wavefrontSgprCount=" << ULEV(config.wavefrontSgprCount) << "\n"
                    "      workitemVgprCount=" << ULEV(config.workitemVgprCount) << "\n"
                    "      reservedVgprFirst=" << ULEV(config.reservedVgprFirst) << "\n"
                    "      reservedVgprCount=" << ULEV(config.reservedVgprCount) << "\n"
                    "      reservedSgprFirst=" << ULEV(config.reservedSgprFirst) << "\n"
                    "      reservedSgprCount=" << ULEV(config.reservedSgprCount) << "\n"
                    "      debugWavefrontPrivateSegmentOffsetSgpr=" <<
                        ULEV(config.debugWavefrontPrivateSegmentOffsetSgpr) << "\n"
                    "      debugPrivateSegmentBufferSgpr=" <<
                        ULEV(config.debugPrivateSegmentBufferSgpr) << "\n"
                    "      kernargSegmentAlignment=" << 
                        cxuint(config.kernargSegmentAlignment) << "\n"
                    "      groupSegmentAlignment=" <<
                        cxuint(config.groupSegmentAlignment) << "\n"
                    "      privateSegmentAlignment=" <<
                        cxuint(config.privateSegmentAlignment) << "\n"
                    "      wavefrontSize=" << cxuint(config.wavefrontSize) << "\n"
                    "      callConvention=0x" << std::hex <<
                        ULEV(config.callConvention) << "\n"
                    "      runtimeLoaderKernelSymbol=0x" <<
                        ULEV(config.runtimeLoaderKernelSymbol) << std::dec << "\n";
                os << "      ControlDirective:\n";
                printHexData(os, 3, 128, config.controlDirective);
            }
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
    // scratch relocations
    if (!output->scratchRelocs.empty())
    {
        os << "  Scratch relocations:\n";
        for (const GalliumScratchReloc& rel: output->scratchRelocs)
            os << "    Rel: offset=" << rel.offset << ", type: " << rel.type << "\n";
    }
    // other data from output
    os << "  Comment:\n";
    printHexData(os, 1, output->commentSize, (const cxbyte*)output->comment);
    os << "  GlobalData:\n";
    printHexData(os, 1, output->globalDataSize, output->globalData);
    os << "  Code:\n";
    printHexData(os, 1, output->codeSize, output->code);
    
    // print extra sections when supplied
    for (BinSection section: output->extraSections)
    {
        os << "  Section " << section.name << ", type=" << section.type <<
                        ", flags=" << section.flags << ":\n";
        printHexData(os, 1, section.size, section.data);
    }
    // print extra symbols when supplied
    Array<BinSymbol> extraSymbols(output->extraSymbols.begin(), output->extraSymbols.end());
    std::sort(extraSymbols.begin(), extraSymbols.end(),
              [](const BinSymbol& s1, const BinSymbol& s2)
              { return s1.name < s2.name; });
    for (BinSymbol symbol: extraSymbols)
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
      dims=54, SGPRS=8, VGPRS=3, pgmRSRC2=0x0, ieeeMode=0x1
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
    /* AMD HSA */
    /* 3 - gallium (configured proginfo and AMDHSA) */
    { R"ffDXD(            .gallium
        .llvm_version 40000
            .kernel aa22
            .args
            .arg scalar, 8,,,SEXT
            .arg griddim,4
            .arg gridoffset,4
            .config
            .priority 1
            .floatmode 43
            .ieeemode
            .sgprsnum 36
            .vgprsnum 139
            .pgmrsrc2 523243
            .scratchbuffer 230
            .default_hsa_features
            
            .call_convention 0x34dac
            .debug_private_segment_buffer_sgpr 98
            .debug_wavefront_private_segment_offset_sgpr 96
            .gds_segment_size 100
            .kernarg_segment_align 32
            .workgroup_group_segment_size 22
            .workgroup_fbarrier_count 3324
            .hsa_sgprsnum 79
        .control_directive
        .int 1,2,4
        
            .kernel aa23
            .args
            .arg scalar, 8,,,SEXT
            .arg griddim,4
            .arg gridoffset,4
            .config
            .dims yz
            .priority 3
            .ieeemode
            .pgmrsrc2 0
            .default_hsa_features
            .group_segment_align 128
            .kernarg_segment_align 64
            .kernarg_segment_size 228
            .kernel_code_entry_offset 256
            .kernel_code_prefetch_offset 1002
            .kernel_code_prefetch_size 13431
            .max_scratch_backing_memory 4212
            .reserved_sgprs 12,19
            .reserved_vgprs 26,48
.text
aa22:
    .skip 256
aa23:
    .skip 256
    .kernel aa22
    .control_directive
        .fill 116,1,0
)ffDXD",
       R"ffDXD(GalliumBinDump:
  Kernel: name=aa22, offset=0
    Config:
      dims=default, SGPRS=36, VGPRS=139, pgmRSRC2=0x7fbeb, ieeeMode=0x1
      floatMode=0x2b, priority=1, localSize=0, scratchBuffer=230
    AMD HSA Config:
      amdCodeVersion=1.0
      amdMachine=1:0:0:0
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0x8eb662
      computePgmRsrc2=0x7fbd1
      enableSgprRegisterFlags=0xb
      enableFeatureFlags=0xa
      workitemPrivateSegmentSize=230
      workgroupGroupSegmentSize=22
      gdsSegmentSize=100
      kernargSegmentSize=24
      workgroupFbarrierCount=3324
      wavefrontSgprCount=79
      workitemVgprCount=139
      reservedVgprFirst=0
      reservedVgprCount=0
      reservedSgprFirst=0
      reservedSgprCount=0
      debugWavefrontPrivateSegmentOffsetSgpr=96
      debugPrivateSegmentBufferSgpr=98
      kernargSegmentAlignment=5
      groupSegmentAlignment=4
      privateSegmentAlignment=4
      wavefrontSize=6
      callConvention=0x34dac
      runtimeLoaderKernelSymbol=0x0
      ControlDirective:
      0100000002000000040000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
    Arg: scalar, true, general, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, false, griddim, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, gridoffset, size=4, tgtSize=4, tgtAlign=4
  Kernel: name=aa23, offset=256
    Config:
      dims=54, SGPRS=12, VGPRS=3, pgmRSRC2=0x0, ieeeMode=0x1
      floatMode=0xc0, priority=3, localSize=0, scratchBuffer=0
    AMD HSA Config:
      amdCodeVersion=1.0
      amdMachine=1:0:0:0
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=1002
      kernelCodePrefetchSize=13431
      maxScrachBackingMemorySize=4212
      computePgmRsrc1=0x8c0c40
      computePgmRsrc2=0x1310
      enableSgprRegisterFlags=0xb
      enableFeatureFlags=0xa
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=0
      kernargSegmentSize=228
      workgroupFbarrierCount=0
      wavefrontSgprCount=12
      workitemVgprCount=3
      reservedVgprFirst=26
      reservedVgprCount=23
      reservedSgprFirst=12
      reservedSgprCount=8
      debugWavefrontPrivateSegmentOffsetSgpr=0
      debugPrivateSegmentBufferSgpr=0
      kernargSegmentAlignment=6
      groupSegmentAlignment=7
      privateSegmentAlignment=4
      wavefrontSize=6
      callConvention=0x0
      runtimeLoaderKernelSymbol=0x0
      ControlDirective:
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
    Arg: scalar, true, general, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, false, griddim, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, gridoffset, size=4, tgtSize=4, tgtAlign=4
  Comment:
  nullptr
  GlobalData:
  nullptr
  Code:
  0100000000000000010000000000000000010000000000000000000000000000
  0000000000000000000000000000000062b68e00d1fb07000b000a00e6000000
  16000000640000001800000000000000fc0c00004f008b000000000000000000
  6000620005040406ac4d03000000000000000000000000000000000000000000
  0100000002000000040000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  010000000000000001000000000000000001000000000000ea03000000000000
  77340000000000007410000000000000400c8c00101300000b000a0000000000
  0000000000000000e400000000000000000000000c0003001a0017000c000800
  0000000006070406000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
)ffDXD", "", true
    },
    /* 3 - gallium - alloc reg flags (extra SGPR registers) */
    { R"ffDXD(            .gallium
        .gpu Fiji
        .llvm_version 40000
            .kernel aa22
            .args
            .arg scalar, 8,,,SEXT
            .arg griddim,4
            .arg gridoffset,4
            .config
            .priority 1
            .floatmode 43
            .ieeemode
            .vgprsnum 139
            .pgmrsrc2 523243
            .scratchbuffer 230
            .use_flat_scratch_init
            
            .call_convention 0x34dac
            .debug_private_segment_buffer_sgpr 98
            .debug_wavefront_private_segment_offset_sgpr 96
            .gds_segment_size 100
            .kernarg_segment_align 32
            .workgroup_group_segment_size 22
            .workgroup_fbarrier_count 3324
    .text
aa22:
    .skip 256
    s_mov_b32 s54, 455
)ffDXD", R"ffDXD(GalliumBinDump:
  Kernel: name=aa22, offset=0
    Config:
      dims=default, SGPRS=61, VGPRS=139, pgmRSRC2=0x7fbeb, ieeeMode=0x1
      floatMode=0x2b, priority=1, localSize=0, scratchBuffer=230
    AMD HSA Config:
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0x8eb5e2
      computePgmRsrc2=0x7fbc5
      enableSgprRegisterFlags=0x20
      enableFeatureFlags=0x0
      workitemPrivateSegmentSize=230
      workgroupGroupSegmentSize=22
      gdsSegmentSize=100
      kernargSegmentSize=24
      workgroupFbarrierCount=3324
      wavefrontSgprCount=61
      workitemVgprCount=139
      reservedVgprFirst=0
      reservedVgprCount=0
      reservedSgprFirst=0
      reservedSgprCount=0
      debugWavefrontPrivateSegmentOffsetSgpr=96
      debugPrivateSegmentBufferSgpr=98
      kernargSegmentAlignment=5
      groupSegmentAlignment=4
      privateSegmentAlignment=4
      wavefrontSize=6
      callConvention=0x34dac
      runtimeLoaderKernelSymbol=0x0
      ControlDirective:
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
    Arg: scalar, true, general, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, false, griddim, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, gridoffset, size=4, tgtSize=4, tgtAlign=4
  Comment:
  nullptr
  GlobalData:
  nullptr
  Code:
  0100000000000000010008000000030000010000000000000000000000000000
  00000000000000000000000000000000e2b58e00c5fb070020000000e6000000
  16000000640000001800000000000000fc0c00003d008b000000000000000000
  6000620005040406ac4d03000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  ff00b6bec7010000
)ffDXD", "", true
    },
    /* 3 - gallium (configured proginfo and AMDHSA) */
    { R"ffDXD(            .gallium
        .llvm_version 40000
            .kernel aa22
            .args
            .arg scalar, 8,,,SEXT
            .arg griddim,4
            .arg gridoffset,4
            .config
            .priority 1
            .floatmode 0x12
            .ieeemode
            .sgprsnum 36
            .vgprsnum 139
            .pgmrsrc2 523243
            .scratchbuffer 230
            .default_hsa_features
            .dims x
            .hsa_dims xy
            .hsa_priority 2
            .call_convention 0x34dac
            .debug_wavefront_private_segment_offset_sgpr 96
            .gds_segment_size 100
            .kernarg_segment_align 32
            .workgroup_group_segment_size 22
            .localsize 23
            .workgroup_fbarrier_count 3324
            .hsa_sgprsnum 79
            .hsa_vgprsnum 167
            .hsa_scratchbuffer 786
            .hsa_floatmode 0xdd
        .control_directive
        .int 1,2,4
.text
aa22:
    .skip 256
    .kernel aa22
    .control_directive
        .fill 116,1,0
)ffDXD", R"ffDXD(GalliumBinDump:
  Kernel: name=aa22, offset=0
    Config:
      dims=9, SGPRS=36, VGPRS=139, pgmRSRC2=0x7fbeb, ieeeMode=0x1
      floatMode=0x12, priority=1, localSize=23, scratchBuffer=230
    AMD HSA Config:
      amdCodeVersion=1.0
      amdMachine=1:0:0:0
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0x8dfa69
      computePgmRsrc2=0x7e9d1
      enableSgprRegisterFlags=0xb
      enableFeatureFlags=0xa
      workitemPrivateSegmentSize=786
      workgroupGroupSegmentSize=22
      gdsSegmentSize=100
      kernargSegmentSize=24
      workgroupFbarrierCount=3324
      wavefrontSgprCount=79
      workitemVgprCount=167
      reservedVgprFirst=0
      reservedVgprCount=0
      reservedSgprFirst=0
      reservedSgprCount=0
      debugWavefrontPrivateSegmentOffsetSgpr=96
      debugPrivateSegmentBufferSgpr=0
      kernargSegmentAlignment=5
      groupSegmentAlignment=4
      privateSegmentAlignment=4
      wavefrontSize=6
      callConvention=0x34dac
      runtimeLoaderKernelSymbol=0x0
      ControlDirective:
      0100000002000000040000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
    Arg: scalar, true, general, size=8, tgtSize=8, tgtAlign=8
    Arg: scalar, false, griddim, size=4, tgtSize=4, tgtAlign=4
    Arg: scalar, false, gridoffset, size=4, tgtSize=4, tgtAlign=4
  Comment:
  nullptr
  GlobalData:
  nullptr
  Code:
  0100000000000000010000000000000000010000000000000000000000000000
  0000000000000000000000000000000069fa8d00d1e907000b000a0012030000
  16000000640000001800000000000000fc0c00004f00a7000000000000000000
  6000000005040406ac4d03000000000000000000000000000000000000000000
  0100000002000000040000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
)ffDXD", "", true
    },
    /* scratch relocations */
    /* 1 - gallium scratch relocation */
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
        .scratchsym scratch
.text
aa22:
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
        s_mov_b32 s1, scratch
        s_mov_b32 s1, scratch+7*3-21
        s_mov_b32 s1, (scratch+7*3-21)&(1<<32-1)
        s_mov_b32 s1, ((scratch)*2-scratch)&(4096*4096*256-1)
        s_mov_b32 s1, (-scratch+2*(scratch))%(4096*4096*256)
        s_mov_b32 s1, (-scratch+2*(scratch))%%(4096*4096*256)
        s_mov_b32 s1, (-scratch+2*(scratch))%%(4096*4096*256*9)
        s_mov_b32 s1, (-scratch+2*(scratch))%(4096*4096*256*9)
        s_mov_b32 s1, (scratch+6-6)>>(31-5+6)
        s_mov_b32 s1, (scratch+6-3*2)>>(234%101)
        s_mov_b32 s1, (scratch)>>(235%101-1)
        s_mov_b32 s1, (scratch)/(4096*4096*256)
        s_mov_b32 s1, (scratch+6-3*2)//(4096*4096*256)
        s_mov_b32 s1, scratch>>32
        s_mov_b32 s1, scratch&0xffffffff
)ffDXD",
       R"ffDXD(GalliumBinDump:
  Kernel: name=aa22, offset=0
    Config:
      dims=default, SGPRS=36, VGPRS=139, pgmRSRC2=0x7fbeb, ieeeMode=0x1
      floatMode=0x2b, priority=1, localSize=0, scratchBuffer=230
    Arg: scalar, true, griddim, size=8, tgtSize=8, tgtAlign=8
  Scratch relocations:
    Rel: offset=12, type: 1
    Rel: offset=20, type: 1
    Rel: offset=28, type: 1
    Rel: offset=36, type: 1
    Rel: offset=44, type: 1
    Rel: offset=52, type: 1
    Rel: offset=60, type: 1
    Rel: offset=68, type: 1
    Rel: offset=76, type: 2
    Rel: offset=84, type: 2
    Rel: offset=92, type: 2
    Rel: offset=100, type: 2
    Rel: offset=108, type: 2
    Rel: offset=116, type: 2
    Rel: offset=124, type: 1
  Comment:
  nullptr
  GlobalData:
  nullptr
  Code:
  05ac098705850a87ff0381be04000000ff0381be04000000ff0381be04000000
  ff0381be04000000ff0381be04000000ff0381be04000000ff0381be04000000
  ff0381be04000000ff0381be04000000ff0381be04000000ff0381be04000000
  ff0381be04000000ff0381be04000000ff0381be04000000ff0381be04000000
)ffDXD", "", true
    },
    { R"ffDXD(            .gallium
            .kernel aa22
            .args
            .arg scalar, 8,,,SEXT,griddim
            .config
            .priority 1
            .scratchbuffer 230
        .scratchsym scratch
.text
aa22:
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
        s_mov_b32 s1, (scratch+6)&0xffffffff
)ffDXD", "", "test.s:13:23: Error: Expression must point to start of section\n", false
    },
    /* gallium (configured proginfo) - different group_id and local_id dimensions */
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
            .dims xy,xyz
            .kernel aa23
            .args
            .arg scalar, 8,,,SEXT,griddim
            .config
            .dims yz, xy
            .priority 3
            .ieeemode
            .pgmrsrc2 0
.text
aa22:
aa23:)ffDXD",
       R"ffDXD(GalliumBinDump:
  Kernel: name=aa22, offset=0
    Config:
      dims=59, SGPRS=36, VGPRS=139, pgmRSRC2=0x7fbeb, ieeeMode=0x1
      floatMode=0x2b, priority=1, localSize=0, scratchBuffer=230
    Arg: scalar, true, griddim, size=8, tgtSize=8, tgtAlign=8
  Kernel: name=aa23, offset=0
    Config:
      dims=30, SGPRS=8, VGPRS=2, pgmRSRC2=0x0, ieeeMode=0x1
      floatMode=0xc0, priority=3, localSize=0, scratchBuffer=0
    Arg: scalar, true, griddim, size=8, tgtSize=8, tgtAlign=8
  Comment:
  nullptr
  GlobalData:
  nullptr
  Code:
)ffDXD", "", true
    }
};

static void testAssembler(cxuint testId, const AsmTestCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    std::ostringstream printStream;
    
    // create assembler with testcase's input and with ASM_TESTRUN flag
    Assembler assembler("test.s", input, (ASM_ALL|ASM_TESTRUN)&~ASM_ALTMACRO,
            BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, errorStream, printStream);
    assembler.setLLVMVersion(1);
    bool good = assembler.assemble();
    
    std::ostringstream dumpOss;
    if (good && assembler.getFormatHandler()!=nullptr)
        // get format handler and their output
        printGalliumOutput(dumpOss, static_cast<const AsmGalliumHandler*>(
                    assembler.getFormatHandler())->getOutput(),
                    assembler.getLLVMVersion() >= 40000U);
    /* compare result dump with expected dump */
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
