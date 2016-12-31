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

static const char* rocmRegionTypeNames[3] =
{ "data", "fkernel", "kernel" };

static void printROCmOutput(std::ostream& os, const ROCmInput* output)
{
    os << "ROCmBinDump:" << std::endl;
    for (const ROCmSymbolInput& symbol: output->symbols)
    {
        os << "  ROCmSymbol: name=" << symbol.symbolName << ", " <<
                "offset=" << symbol.offset << ", size=" << symbol.size << ", type=" <<
                rocmRegionTypeNames[cxuint(symbol.type)] << "\n";
        if (symbol.type == ROCmRegionType::DATA)
            continue;
        if (symbol.offset+sizeof(ROCmKernelConfig) > output->codeSize)
            continue;
        const ROCmKernelConfig& config = *reinterpret_cast<const ROCmKernelConfig*>(
                            output->code + symbol.offset);
        os << "    Config:\n"
            "      amdCodeVersion=" << ULEV(config.amdCodeVersionMajor) << "." <<
                ULEV(config.amdCodeVersionMajor) << "\n"
            "      amdMachine=" << ULEV(config.amdMachineKind) << ":" <<
                ULEV(config.amdMachineMajor) << ":" <<
                ULEV(config.amdMachineMinor) << ":" <<
                ULEV(config.amdMachineStepping) << "\n"
            "      kernelCodeEntryOffset=" << ULEV(config.kernelCodeEntryOffset) << "\n"
            "      kernelCodePrefetchOffset=" <<
                ULEV(config.kernelCodePrefetchOffset) << "\n"
            "      kernelCodePrefetchSize=" << ULEV(config.kernelCodePrefetchSize) << "\n"
            "      maxScrachBackingMemorySize=" <<
                ULEV(config.maxScrachBackingMemorySize) << "\n"
            "      computePgmRsrc1=0x" << std::hex << ULEV(config.computePgmRsrc1) << "\n"
            "      computePgmRsrc2=0x" << ULEV(config.computePgmRsrc2) << "\n"
            "      enableSpgrRegisterFlags=0x" <<
                ULEV(config.enableSpgrRegisterFlags) << "\n"
            "      enableFeatureFlags=0x" <<
                ULEV(config.enableFeatureFlags) << std::dec << "\n"
            "      workitemPrivateSegmentSize=" <<
                ULEV(config.workitemPrivateSegmentSize) << "\n"
            "      workgroupGroupSegmentSize=" <<
                ULEV(config.workgroupGroupSegmentSize) << "\n"
            "      gdsSegmentSize=" << ULEV(config.gdsSegmentSize) << "\n"
            "      kernargSegmentSize=" << ULEV(config.kernargSegmentSize) << "\n"
            "      workgroupFbarrierCount=" << ULEV(config.workgroupFbarrierCount) << "\n"
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
            "      callConvention=0x" << std::hex << ULEV(config.callConvention) << "\n"
            "      runtimeLoaderKernelSymbol=0x" <<
                ULEV(config.runtimeLoaderKernelSymbol) << std::dec << "\n";
        os << "      ControlDirective:\n";
        printHexData(os, 3, 128, config.controlDirective);
    }
    os << "  Comment:\n";
    printHexData(os, 1, output->commentSize, (const cxbyte*)output->comment);
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
    {
        R"ffDXD(        .rocm
        .gpu Fiji
.kernel kxx1
    .fkernel
    .config
        .dims x
        .codeversion 1,0
        .call_convention 0x34dac
        .debug_private_segment_buffer_sgpr 98
        .debug_wavefront_private_segment_offset_sgpr 96
        .gds_segment_size 100
        .kernarg_segment_align 32
        .workgroup_group_segment_size 22
        .workgroup_fbarrier_count 3324
        .dx10clamp
        .exceptions 10
        .private_segment_align 128
        .privmode
        .reserved_sgprs 5,14
        .runtime_loader_kernel_symbol 0x4dc98b3a
        .scratchbuffer 77222
        .reserved_sgprs 9,12
        .reserved_vgprs 7,17
        .private_elem_size 16
    .control_directive
        .int 1,2,3
        .fill 116,1,0
.kernel kxx2
    .config
        .dims x
        .codeversion 1,0
        .call_convention 0x112223
.kernel kxx1
    .config
        .scratchbuffer 111
.text
kxx1:
        .skip 256
        s_mov_b32 s7, 0
        s_endpgm
        
.align 256
kxx2:
        .skip 256
        s_endpgm
.section .comment
        .ascii "some comment for you"
.kernel kxx2
    .control_directive
        .fill 124,1,0xde
    .config
        .use_kernarg_segment_ptr
    .control_directive
        .int 0xaadd66cc
    .config
.kernel kxx1
.kernel kxx2
        .call_convention 0x1112234
        
)ffDXD",
        /* dump */
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=kxx1, offset=0, size=0, type=fkernel
    Config:
      amdCodeVersion=1.1
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0x3c0000
      computePgmRsrc2=0xa008081
      enableSpgrRegisterFlags=0x0
      enableFeatureFlags=0x6
      workitemPrivateSegmentSize=111
      workgroupGroupSegmentSize=22
      gdsSegmentSize=100
      kernargSegmentSize=0
      workgroupFbarrierCount=3324
      wavefrontSgprCount=8
      workitemVgprCount=1
      reservedVgprFirst=7
      reservedVgprCount=11
      reservedSgprFirst=9
      reservedSgprCount=4
      debugWavefrontPrivateSegmentOffsetSgpr=96
      debugPrivateSegmentBufferSgpr=98
      kernargSegmentAlignment=5
      groupSegmentAlignment=4
      privateSegmentAlignment=7
      wavefrontSize=6
      callConvention=0x34dac
      runtimeLoaderKernelSymbol=0x4dc98b3a
      ControlDirective:
      0100000002000000030000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
  ROCmSymbol: name=kxx2, offset=512, size=0, type=kernel
    Config:
      amdCodeVersion=1.1
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xc0000
      computePgmRsrc2=0x84
      enableSpgrRegisterFlags=0x8
      enableFeatureFlags=0x0
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=0
      kernargSegmentSize=0
      workgroupFbarrierCount=0
      wavefrontSgprCount=3
      workitemVgprCount=1
      reservedVgprFirst=0
      reservedVgprCount=0
      reservedSgprFirst=0
      reservedSgprCount=0
      debugWavefrontPrivateSegmentOffsetSgpr=0
      debugPrivateSegmentBufferSgpr=0
      kernargSegmentAlignment=4
      groupSegmentAlignment=4
      privateSegmentAlignment=4
      wavefrontSize=6
      callConvention=0x1112234
      runtimeLoaderKernelSymbol=0x0
      ControlDirective:
      dededededededededededededededededededededededededededededededede
      dededededededededededededededededededededededededededededededede
      dededededededededededededededededededededededededededededededede
      dedededededededededededededededededededededededededededecc66ddaa
  Comment:
  736f6d6520636f6d6d656e7420666f7220796f75
  Code:
  0100000000000000010008000000030000010000000000000000000000000000
  0000000000000000000000000000000000003c008180000a000006006f000000
  16000000640000000000000000000000fc0c00000800010007000b0009000400
  6000620005040706ac4d03000000000000000000000000003a8bc94d00000000
  0100000002000000030000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  800087be000081bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  0100000000000000010008000000030000010000000000000000000000000000
  0000000000000000000000000000000000000c00840000000800000000000000
  0000000000000000000000000000000000000000030001000000000000000000
  0000000004040406342211010000000000000000000000000000000000000000
  dededededededededededededededededededededededededededededededede
  dededededededededededededededededededededededededededededededede
  dededededededededededededededededededededededededededededededede
  dedededededededededededededededededededededededededededecc66ddaa
  000081bf
)ffDXD",
        /* warning/errors */
        "",
        true
    },
    {
        R"ffDXD(        .rocm
        .gpu Fiji
.kernel someKernelX
    .config
        .dims xz
        .call_convention 331
        .codeversion 1,0
        .machine 8,0,1,2
        .debug_private_segment_buffer_sgpr 10
        .debug_wavefront_private_segment_offset_sgpr 31
        .exceptions 0x3e
        .floatmode 0xc3
        .gds_segment_size 105
        .group_segment_align 128
        .kernarg_segment_align 64
        .kernarg_segment_size 228
        .kernel_code_entry_offset 256
        .kernel_code_prefetch_offset 1002
        .kernel_code_prefetch_size 13431
        .max_scratch_backing_memory 4212
        .pgmrsrc1 0xa0000000
        .pgmrsrc2 0xd00000
        .priority 2
        .private_elem_size 8
        .private_segment_align 32
        .reserved_sgprs 12,19
        .reserved_vgprs 26,48
        .runtime_loader_kernel_symbol 0x3eda1
        .scratchbuffer 2330
        .use_debug_enabled
        .use_flat_scratch_init
        .use_grid_workgroup_count xz
        .use_private_segment_buffer
        .use_ptr64
        .use_xnack_enabled
        .wavefront_size 256
        .workgroup_fbarrier_count 69
        .workgroup_group_segment_size 324
        .workitem_private_segment_size 33
        .vgprsnum 211
        .sgprsnum 85
.text
someKernelX:
        .skip 256
        s_endpgm)ffDXD",
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=someKernelX, offset=0, size=0, type=kernel
    Config:
      amdCodeVersion=1.1
      amdMachine=8:0:1:2
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=1002
      kernelCodePrefetchSize=13431
      maxScrachBackingMemorySize=4212
      computePgmRsrc1=0xa00c3ab4
      computePgmRsrc2=0x3ed09291
      enableSpgrRegisterFlags=0x2a1
      enableFeatureFlags=0x6c
      workitemPrivateSegmentSize=33
      workgroupGroupSegmentSize=324
      gdsSegmentSize=105
      kernargSegmentSize=228
      workgroupFbarrierCount=69
      wavefrontSgprCount=85
      workitemVgprCount=211
      reservedVgprFirst=26
      reservedVgprCount=23
      reservedSgprFirst=12
      reservedSgprCount=8
      debugWavefrontPrivateSegmentOffsetSgpr=31
      debugPrivateSegmentBufferSgpr=10
      kernargSegmentAlignment=6
      groupSegmentAlignment=7
      privateSegmentAlignment=5
      wavefrontSize=8
      callConvention=0x14b
      runtimeLoaderKernelSymbol=0x3eda1
      ControlDirective:
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
  Comment:
  nullptr
  Code:
  010000000000000008000000010002000001000000000000ea03000000000000
  77340000000000007410000000000000b43a0ca09192d03ea1026c0021000000
  4401000069000000e400000000000000450000005500d3001a0017000c000800
  1f000a00060705084b010000000000000000000000000000a1ed030000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  000081bf
)ffDXD",
        /* warning/errors */
        "",
        true
    },
    {
        R"ffDXD(        .rocm
        .gpu Fiji
.kernel someKernelX
    .config
        .dims xz
        .reserved_vgprs 0, 11
.text
someKernelX:
        s_endpgm)ffDXD",
        "", "test.s:3:1: Error: "
        "Code for kernel 'someKernelX' is too small for configuration\n", false
    },
    {
        R"ffDXD(        .rocm
        .gpu Fiji
.kernel someKernelX
    .config
        .dims xz
        .reserved_vgprs 12,11
        .reserved_sgprs 17,11
        .reserved_vgprs 256,257
        .reserved_sgprs 112,113
        .debug_private_segment_buffer_sgpr 123
        .debug_wavefront_private_segment_offset_sgpr 108
        .private_elem_size 6
        .private_elem_size 1
        .private_elem_size 32
        .kernarg_segment_align 56
        .kernarg_segment_align 8
        .private_segment_align 56
        .private_segment_align 8
        .wavefront_size 157
        .wavefront_size 512
        .pgmrsrc2 0xaa1fd3da2313
.text
someKernelX:
        .skip 256
        s_endpgm)ffDXD",
        "", R"ffDXD(test.s:6:28: Error: Wrong regsister range
test.s:7:28: Error: Wrong regsister range
test.s:8:25: Error: First reserved VGPR register out of range (0-255)
test.s:8:29: Error: Last reserved VGPR register out of range (0-255)
test.s:9:25: Error: First reserved SGPR register out of range (0-101)
test.s:9:29: Error: Last reserved SGPR register out of range (0-101)
test.s:10:44: Error: SGPR register out of range
test.s:11:54: Error: SGPR register out of range
test.s:12:28: Error: Private element size must be power of two
test.s:13:28: Error: Private element size out of range
test.s:14:28: Error: Private element size out of range
test.s:15:32: Error: Alignment must be power of two
test.s:16:32: Error: Alignment must be not smaller than 16
test.s:17:32: Error: Alignment must be power of two
test.s:18:32: Error: Alignment must be not smaller than 16
test.s:19:25: Error: Wavefront size must be power of two
test.s:20:25: Error: Wavefront size must be not greater than 256
test.s:21:19: Warning: Value 0xaa1fd3da2313 truncated to 0xd3da2313
)ffDXD", false
    }
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
        printROCmOutput(dumpOss, static_cast<const AsmROCmHandler*>(
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
