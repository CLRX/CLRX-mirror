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
#include <CLRX/amdbin/ROCmBinaries.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
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

static const char* rocmValueKindNames[] =
{
    "value", "globalbuf", "dynshptr", "sampler", "image", "pipe", "queue",
    "gox", "goy", "goz", "none", "printfbuf", "defqueue", "complact"
};

static const char* rocmValueTypeNames[] =
{ "struct", "i8", "u8", "i16", "u16", "f16", "i32", "u32", "f32", "i64", "u64", "f64" };

static const char* rocmAddressSpaces[] =
{ "none", "private", "global", "constant", "local", "generic", "region" };

static const char* rocmAccessQuals[] =
{ "default", "read_only", "write_only", "read_write" };

// print dump of ROCm output to stream for comparing with testcase
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
        
        // print kernel configuration
        os << "    Config:\n"
            "      amdCodeVersion=" << ULEV(config.amdCodeVersionMajor) << "." <<
                ULEV(config.amdCodeVersionMinor) << "\n"
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
    // print comment and code
    os << "  Comment:\n";
    printHexData(os, 1, output->commentSize, (const cxbyte*)output->comment);
    os << "  Code:\n";
    printHexData(os, 1, output->codeSize, output->code);
    if (output->globalData != nullptr)
    {
        os << "  GlobalData:\n";
        printHexData(os, 1, output->globalDataSize, output->globalData);
    }
    
    if (output->metadata != nullptr)
        os << "  Metadata:\n" << std::string(output->metadata,
                            output->metadataSize) << "\n";
    
    // dump ROCm metadata
    if (output->useMetadataInfo)
    {
        const ROCmMetadata& metadata = output->metadataInfo;
        os << "  MetadataInfo:\n"
            "    Version: " << metadata.version[0] << "." << metadata.version[1] << "\n";
        // dump printf info
        for (const ROCmPrintfInfo& printfInfo: metadata.printfInfos)
        {
            os << "    Printf: " << printfInfo.id;
            for (size_t argSize: printfInfo.argSizes)
                os << ", " << argSize;
            os << "; \"" << printfInfo.format << "\"\n";
        }
        // dump kernel metadata
        for (const ROCmKernelMetadata& kernel: metadata.kernels)
        {
            os << "    Kernel: " << kernel.name << "\n"
                "      SymName=" << kernel.symbolName << "\n"
                "      Language=" << kernel.language << " " <<
                        kernel.langVersion[0] << "." << kernel.langVersion[1] << "\n"
                "      ReqdWorkGroupSize=" << kernel.reqdWorkGroupSize[0] << " " <<
                        kernel.reqdWorkGroupSize[1] << " " <<
                        kernel.reqdWorkGroupSize[2] << "\n"
                "      WorkGroupSizeHint=" << kernel.workGroupSizeHint[0] << " " <<
                        kernel.workGroupSizeHint[1] << " " <<
                        kernel.workGroupSizeHint[2] << "\n"
                "      VecTypeHint=" << kernel.vecTypeHint << "\n"
                "      RuntimeHandle=" << kernel.runtimeHandle << "\n"
                "      KernargSegmentSize=" << kernel.kernargSegmentSize << "\n"
                "      KernargSegmentAlign=" << kernel.kernargSegmentAlign << "\n"
                "      GroupSegmentFixedSize=" << kernel.groupSegmentFixedSize<< "\n"
                "      PrivateSegmentFixedSize=" << kernel.privateSegmentFixedSize<< "\n"
                "      WaveFrontSize=" << kernel.wavefrontSize << "\n"
                "      SgprsNum=" << kernel.sgprsNum << "\n"
                "      VgprsNum=" << kernel.vgprsNum << "\n"
                "      SpilledSgprs=" << kernel.spilledSgprs << "\n"
                "      SpilledVgprs=" << kernel.spilledVgprs << "\n"
                "      MaxFlatWorkGroupSize=" << kernel.maxFlatWorkGroupSize << "\n"
                "      FixedWorkGroupSize=" << kernel.fixedWorkGroupSize[0] << " " <<
                        kernel.fixedWorkGroupSize[1] << " " <<
                        kernel.fixedWorkGroupSize[2] << "\n";
            
            // dump kernel arguments
            for (const ROCmKernelArgInfo& argInfo: kernel.argInfos)
                os << "      Arg name=" << argInfo.name << ", type=" << argInfo.typeName <<
                    ", size=" << argInfo.size << ", align=" << argInfo.align << "\n"
                    "        valuekind=" <<
                            rocmValueKindNames[cxuint(argInfo.valueKind)] <<
                    ", valuetype=" << rocmValueTypeNames[cxuint(argInfo.valueType)] <<
                    ", pointeeAlign=" << argInfo.pointeeAlign << "\n"
                    "        addrSpace=" <<
                            rocmAddressSpaces[cxuint(argInfo.addressSpace)] <<
                    ", accQual=" << rocmAccessQuals[cxuint(argInfo.accessQual)] <<
                    ", actAccQual=" <<
                            rocmAccessQuals[cxuint(argInfo.actualAccessQual)] << "\n"
                    "        Flags=" <<
                    (argInfo.isConst ? " const" : "") <<
                    (argInfo.isRestrict ? " restrict" : "") <<
                    (argInfo.isVolatile ? " volatile" : "") <<
                    (argInfo.isPipe ? " pipe" : "") << "\n";
        }
    }
    
    if (!output->target.empty())
        os << "  Target=" << output->target << "\n";
    if (output->eflags != BINGEN_DEFAULT)
        os << "  EFlags=" << output->eflags << std::endl;
    
    if (output->newBinFormat)
        os << "  NewBinFormat\n";
    
    if (!output->gotSymbols.empty())
    {
        // print got symbol indices with symbol names
        os << "  GotSymbols:\n";
        for (size_t index: output->gotSymbols)
        {
            const CString name = (index < output->symbols.size()) ?
                    output->symbols[index].symbolName :
                    output->extraSymbols[index-output->symbols.size()].name;
            os << "    Sym: " << name << "\n";
        }
    }
    
    // print extra sections if supplied
    for (BinSection section: output->extraSections)
    {
        os << "  Section " << section.name << ", type=" << section.type <<
                        ", flags=" << section.flags << ":\n";
        printHexData(os, 1, section.size, section.data);
    }
    // print extra symbols if supplied
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
    {   // 0
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
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0x3c0040
      computePgmRsrc2=0xa008081
      enableSgprRegisterFlags=0x0
      enableFeatureFlags=0x6
      workitemPrivateSegmentSize=111
      workgroupGroupSegmentSize=22
      gdsSegmentSize=100
      kernargSegmentSize=0
      workgroupFbarrierCount=3324
      wavefrontSgprCount=10
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
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xc0000
      computePgmRsrc2=0x84
      enableSgprRegisterFlags=0x8
      enableFeatureFlags=0x0
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=0
      kernargSegmentSize=0
      workgroupFbarrierCount=0
      wavefrontSgprCount=5
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
  0000000000000000000000000000000040003c008180000a000006006f000000
  16000000640000000000000000000000fc0c00000a00010007000b0009000400
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
  0000000000000000000000000000000000000000050001000000000000000000
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
    {   // 1
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
      amdCodeVersion=1.0
      amdMachine=8:0:1:2
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=1002
      kernelCodePrefetchSize=13431
      maxScrachBackingMemorySize=4212
      computePgmRsrc1=0xa00c3ab4
      computePgmRsrc2=0x3ed09291
      enableSgprRegisterFlags=0x2a1
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
    {   // 2
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
    {   // 3
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
        "", R"ffDXD(test.s:6:28: Error: Wrong register range
test.s:7:28: Error: Wrong register range
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
    },
    {   // 4 - different eflags
        R"ffDXD(.rocm
        .gpu Fiji
        .eflags 3
.kernel kxx1
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
.text
kxx1:
        .skip 256
        s_mov_b32 s7, 0
        s_endpgm
)ffDXD",
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=kxx1, offset=0, size=0, type=kernel
    Config:
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0x3c0040
      computePgmRsrc2=0xa008081
      enableSgprRegisterFlags=0x0
      enableFeatureFlags=0x6
      workitemPrivateSegmentSize=77222
      workgroupGroupSegmentSize=22
      gdsSegmentSize=100
      kernargSegmentSize=0
      workgroupFbarrierCount=3324
      wavefrontSgprCount=10
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
  Comment:
  nullptr
  Code:
  0100000000000000010008000000030000010000000000000000000000000000
  0000000000000000000000000000000040003c008180000a00000600a62d0100
  16000000640000000000000000000000fc0c00000a00010007000b0009000400
  6000620005040706ac4d03000000000000000000000000003a8bc94d00000000
  0100000002000000030000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  800087be000081bf
  EFlags=3
)ffDXD", "", true
    },
    {   // 5 - metadata and others
        R"ffDXD(.rocm
        .gpu Fiji
        .eflags 3
        .newbinfmt
.metadata
        .ascii "sometext in this place\n"
        .ascii "maybe not unrecognizable by parser but it is understandable by human\n"
.globaldata
        .byte 1,2,3,4,5,5,6,33
.kernel kxx1
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
.text
kxx1:
        .skip 256
        s_mov_b32 s7, 0
        s_endpgm
)ffDXD",
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=kxx1, offset=0, size=0, type=kernel
    Config:
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0x3c0040
      computePgmRsrc2=0xa000081
      enableSgprRegisterFlags=0x0
      enableFeatureFlags=0x6
      workitemPrivateSegmentSize=77222
      workgroupGroupSegmentSize=22
      gdsSegmentSize=100
      kernargSegmentSize=0
      workgroupFbarrierCount=3324
      wavefrontSgprCount=10
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
  Comment:
  nullptr
  Code:
  0100000000000000010008000000030000010000000000000000000000000000
  0000000000000000000000000000000040003c008100000a00000600a62d0100
  16000000640000000000000000000000fc0c00000a00010007000b0009000400
  6000620005040706ac4d03000000000000000000000000003a8bc94d00000000
  0100000002000000030000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  800087be000081bf
  GlobalData:
  0102030405050621
  Metadata:
sometext in this place
maybe not unrecognizable by parser but it is understandable by human

  EFlags=3
  NewBinFormat
)ffDXD", "", true
    },
    {   // 6 - metadata info
        R"ffDXD(.rocm
        .gpu Fiji
        .eflags 3
        .newbinfmt
        .md_version 3 , 5
        .printf 1 ,5 ,7 , 2,  11, "sometext %d %e %f"
        .printf 2 ,"sometext"
        .printf  , 16 ,8 , 2,  4, "sometext %d %e %f"
.kernel kxx1
    .config
        .dims x
        .codeversion 1,0
        .call_convention 0x34dac
        .debug_private_segment_buffer_sgpr 98
        .debug_wavefront_private_segment_offset_sgpr 96
        .gds_segment_size 100
        .kernarg_segment_align 32
    # metadata
        .md_symname "kxx1@kd"
        .md_language "Poliglot", 3, 1
        .reqd_work_group_size 6,2,4
        .work_group_size_hint 5,7,2
        .vectypehint float16
        .spilledsgprs 11
        .spilledvgprs 52
        .md_kernarg_segment_size 64
        .md_kernarg_segment_align 8
        .md_group_segment_fixed_size 0
        .md_private_segment_fixed_size 0
        .md_wavefront_size 64
        .md_sgprsnum 14
        .md_vgprsnum 11
        .max_flat_work_group_size 256
        .arg n, "uint", 4, , value, u32
        .arg n2, "uint", 12, , value, u32
        .arg x0, "char", 1, 16, value, char
        .arg x1, "int8", 1, 16, value, i8
        .arg x2, "short", 2, 16, value, short
        .arg x3, "int16", 2, 16, value, i16
        .arg x4, "int", 4, 16, value, int
        .arg x5, "int32", 4, 16, value, i32
        .arg x6, "long", 8, 16, value, long
        .arg x7, "int64", 8, 16, value, i64
        .arg x8, "uchar", 1, 16, value, uchar
        .arg x9, "uint8", 1, 16, value, u8
        .arg x10, "ushort", 2, 16, value, ushort
        .arg x11, "uint16", 2, 16, value, u16
        .arg x12, "uint", 4, 16, value, uint
        .arg x13, "uint32", 4, 16, value, u32
        .arg x14, "ulong", 8, 16, value, ulong
        .arg x15, "uint64", 8, 16, value, u64
        .arg x16, "half", 2, 16, value, half
        .arg x17, "fp16", 2, 16, value, f16
        .arg x18, "float", 4, 16, value, float
        .arg x19, "fp32", 4, 16, value, f32
        .arg x20, "double", 8, 16, value, double
        .arg x21, "fp64", 8, 16, value, f64
        .arg a, "float*", 8, 8, globalbuf, f32, global, default const volatile
        .arg abuf, "float*", 8, 8, globalbuf, f32, constant, default
        .arg abuf2, "float*", 8, 8, dynshptr, f32, 1, local
        .arg abuf3, "float*", 8, 8, globalbuf, f32, generic, default
        .arg abuf4, "float*", 8, 8, globalbuf, f32, region, default
        .arg abuf5, "float*", 8, 8, dynshptr, f32, 1, private
        .arg bbuf, "float*", 8, 8, globalbuf, f32, global, read_only
        .arg bbuf2, "float*", 8, 8, globalbuf, f32, global, write_only
        .arg bbuf3, "float*", 8, 8, globalbuf, f32, global, read_write
        .arg img1, "image1d_t", 8, 8, image, struct, rdonly, default
        .arg img2, "image1d_t", 8, 8, image, struct, wronly, default
        .arg img3, "image1d_t", 8, 8, image, struct, rdwr, default
        .arg , "", 8, 8, gox, i64
        .arg , "", 8, 8, goy, i64
        .arg , "", 8, 8, goz, i64
        .arg , "", 8, 8, globaloffsetx, i64
        .arg , "", 8, 8, globaloffsety, i64
        .arg , "", 8, 8, globaloffsetz, i64
.text
kxx1:   .skip 256
        s_mov_b32 s7, 0
        s_endpgm
)ffDXD",
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=kxx1, offset=0, size=0, type=kernel
    Config:
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xc0040
      computePgmRsrc2=0x80
      enableSgprRegisterFlags=0x0
      enableFeatureFlags=0x0
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=100
      kernargSegmentSize=528
      workgroupFbarrierCount=0
      wavefrontSgprCount=10
      workitemVgprCount=1
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
  Comment:
  nullptr
  Code:
  0100000000000000010008000000030000010000000000000000000000000000
  0000000000000000000000000000000040000c00800000000000000000000000
  00000000640000001002000000000000000000000a0001000000000000000000
  6000620005040406ac4d03000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  800087be000081bf
  MetadataInfo:
    Version: 3.5
    Printf: 1, 5, 7, 2, 11; "sometext %d %e %f"
    Printf: 2; "sometext"
    Printf: 4294967295, 16, 8, 2, 4; "sometext %d %e %f"
    Kernel: kxx1
      SymName=kxx1@kd
      Language=Poliglot 3.1
      ReqdWorkGroupSize=6 2 4
      WorkGroupSizeHint=5 7 2
      VecTypeHint=float16
      RuntimeHandle=
      KernargSegmentSize=64
      KernargSegmentAlign=8
      GroupSegmentFixedSize=0
      PrivateSegmentFixedSize=0
      WaveFrontSize=64
      SgprsNum=14
      VgprsNum=11
      SpilledSgprs=11
      SpilledVgprs=52
      MaxFlatWorkGroupSize=256
      FixedWorkGroupSize=0 0 0
      Arg name=n, type=uint, size=4, align=4
        valuekind=value, valuetype=u32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=n2, type=uint, size=12, align=16
        valuekind=value, valuetype=u32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x0, type=char, size=1, align=16
        valuekind=value, valuetype=i8, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x1, type=int8, size=1, align=16
        valuekind=value, valuetype=i8, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x2, type=short, size=2, align=16
        valuekind=value, valuetype=i16, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x3, type=int16, size=2, align=16
        valuekind=value, valuetype=i16, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x4, type=int, size=4, align=16
        valuekind=value, valuetype=i32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x5, type=int32, size=4, align=16
        valuekind=value, valuetype=i32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x6, type=long, size=8, align=16
        valuekind=value, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x7, type=int64, size=8, align=16
        valuekind=value, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x8, type=uchar, size=1, align=16
        valuekind=value, valuetype=u8, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x9, type=uint8, size=1, align=16
        valuekind=value, valuetype=u8, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x10, type=ushort, size=2, align=16
        valuekind=value, valuetype=i16, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x11, type=uint16, size=2, align=16
        valuekind=value, valuetype=u16, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x12, type=uint, size=4, align=16
        valuekind=value, valuetype=u32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x13, type=uint32, size=4, align=16
        valuekind=value, valuetype=u32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x14, type=ulong, size=8, align=16
        valuekind=value, valuetype=u64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x15, type=uint64, size=8, align=16
        valuekind=value, valuetype=u64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x16, type=half, size=2, align=16
        valuekind=value, valuetype=f16, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x17, type=fp16, size=2, align=16
        valuekind=value, valuetype=f16, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x18, type=float, size=4, align=16
        valuekind=value, valuetype=f32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x19, type=fp32, size=4, align=16
        valuekind=value, valuetype=f32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x20, type=double, size=8, align=16
        valuekind=value, valuetype=f64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=x21, type=fp64, size=8, align=16
        valuekind=value, valuetype=f64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=a, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= const volatile
      Arg name=abuf, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=constant, accQual=default, actAccQual=default
        Flags=
      Arg name=abuf2, type=float*, size=8, align=8
        valuekind=dynshptr, valuetype=f32, pointeeAlign=1
        addrSpace=local, accQual=default, actAccQual=default
        Flags=
      Arg name=abuf3, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=generic, accQual=default, actAccQual=default
        Flags=
      Arg name=abuf4, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=region, accQual=default, actAccQual=default
        Flags=
      Arg name=abuf5, type=float*, size=8, align=8
        valuekind=dynshptr, valuetype=f32, pointeeAlign=1
        addrSpace=private, accQual=default, actAccQual=default
        Flags=
      Arg name=bbuf, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=read_only
        Flags=
      Arg name=bbuf2, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=write_only
        Flags=
      Arg name=bbuf3, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=read_write
        Flags=
      Arg name=img1, type=image1d_t, size=8, align=8
        valuekind=image, valuetype=struct, pointeeAlign=0
        addrSpace=none, accQual=read_only, actAccQual=default
        Flags=
      Arg name=img2, type=image1d_t, size=8, align=8
        valuekind=image, valuetype=struct, pointeeAlign=0
        addrSpace=none, accQual=write_only, actAccQual=default
        Flags=
      Arg name=img3, type=image1d_t, size=8, align=8
        valuekind=image, valuetype=struct, pointeeAlign=0
        addrSpace=none, accQual=read_write, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=gox, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=goy, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=goz, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=gox, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=gox, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=gox, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
  EFlags=3
  NewBinFormat
)ffDXD",
        "", true
    },
    {   //  6 - next metadata info example
        R"ffDXD(.rocm
        .gpu Fiji
        .eflags 3
        .newbinfmt
        .md_version 3 , 5
.kernel kxx1
    .config
        .dims x
        .codeversion 1,0
        .call_convention 0x34dac
        .debug_private_segment_buffer_sgpr 98
        .debug_wavefront_private_segment_offset_sgpr 96
        .gds_segment_size 100
        .kernarg_segment_align 32
    # metadata
        .md_language "jezorx"
        .reqd_work_group_size 6,
        .work_group_size_hint 5,7
        .fixed_work_group_size 3,,71
        .md_kernarg_segment_size 64
        .md_kernarg_segment_align 32
        .md_group_segment_fixed_size 1121
        .md_private_segment_fixed_size 6632
        .md_wavefront_size 64
        .md_sgprsnum 14
        .md_vgprsnum 11
        .runtime_handle "SomeCodeToExec"
        # arg infos
        .arg , "", 8, 8, none, i64
        .arg , "", 8, 8, complact, i64
        .arg , "", 8, 8, printfbuf, i64
        .arg , "", 8, 8, defqueue, i64
        .arg pipe0, "pipe_t", 8, 8, pipe, struct, read_write, default pipe
        .arg qx01, "queue_t", 8, 8, queue, struct
        .arg masksamp, "sampler_t", 8, 8, sampler, struct
        .arg vxx1, "void*", 8, 8, globalbuf, i8, global, default const
        .arg vx1, "void*", 8, 8, globalbuf, i8, global, default volatile
        .arg dx3, "void*", 8, 8, globalbuf, i8, global, default restrict
        .arg ex6, "void*", 8, 8, globalbuf, i8, global, default pipe
        .arg fx9, "void*", 8, 8, globalbuf, i8, global, default volatile const restrict
.text
kxx1:   .skip 256
        s_mov_b32 s7, 0
        s_endpgm
)ffDXD",
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=kxx1, offset=0, size=0, type=kernel
    Config:
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xc0040
      computePgmRsrc2=0x80
      enableSgprRegisterFlags=0x0
      enableFeatureFlags=0x0
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=100
      kernargSegmentSize=96
      workgroupFbarrierCount=0
      wavefrontSgprCount=10
      workitemVgprCount=1
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
  Comment:
  nullptr
  Code:
  0100000000000000010008000000030000010000000000000000000000000000
  0000000000000000000000000000000040000c00800000000000000000000000
  00000000640000006000000000000000000000000a0001000000000000000000
  6000620005040406ac4d03000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  800087be000081bf
  MetadataInfo:
    Version: 3.5
    Kernel: kxx1
      SymName=
      Language=jezorx 0.0
      ReqdWorkGroupSize=6 1 1
      WorkGroupSizeHint=5 7 1
      VecTypeHint=
      RuntimeHandle=SomeCodeToExec
      KernargSegmentSize=64
      KernargSegmentAlign=32
      GroupSegmentFixedSize=1121
      PrivateSegmentFixedSize=6632
      WaveFrontSize=64
      SgprsNum=14
      VgprsNum=11
      SpilledSgprs=4294967294
      SpilledVgprs=4294967294
      MaxFlatWorkGroupSize=18446744073709551614
      FixedWorkGroupSize=3 1 71
      Arg name=, type=, size=8, align=8
        valuekind=none, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=complact, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=printfbuf, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=defqueue, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=pipe0, type=pipe_t, size=8, align=8
        valuekind=pipe, valuetype=struct, pointeeAlign=0
        addrSpace=none, accQual=read_write, actAccQual=default
        Flags= pipe
      Arg name=qx01, type=queue_t, size=8, align=8
        valuekind=queue, valuetype=struct, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=masksamp, type=sampler_t, size=8, align=8
        valuekind=sampler, valuetype=struct, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=vxx1, type=void*, size=8, align=8
        valuekind=globalbuf, valuetype=i8, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= const
      Arg name=vx1, type=void*, size=8, align=8
        valuekind=globalbuf, valuetype=i8, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= volatile
      Arg name=dx3, type=void*, size=8, align=8
        valuekind=globalbuf, valuetype=i8, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= restrict
      Arg name=ex6, type=void*, size=8, align=8
        valuekind=globalbuf, valuetype=i8, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= pipe
      Arg name=fx9, type=void*, size=8, align=8
        valuekind=globalbuf, valuetype=i8, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= const restrict volatile
  EFlags=3
  NewBinFormat
)ffDXD",
        "", true
    },
    {   // next metadata info (errors)
        R"ffDXD(.rocm
        .gpu Fiji
        .eflags 3
        .newbinfmt
        .md_version 3 , 5
.kernel kxx1
    .config
        .dims x
        .codeversion 1,0
        .call_convention 0x34dac
        .debug_private_segment_buffer_sgpr 98
        .debug_wavefront_private_segment_offset_sgpr 96
        .gds_segment_size 100
        .kernarg_segment_align 32
    # metadata
    .text
        .md_language "jezorx"
        .reqd_work_group_size 6,
        .work_group_size_hint 5,7
        .fixed_work_group_size 3,,71
        .md_kernarg_segment_size 64
        .md_kernarg_segment_align 32
        .md_group_segment_fixed_size 1121
        .md_private_segment_fixed_size 6632
        .md_wavefront_size 64
        .md_sgprsnum 14
        .md_vgprsnum 11
        .spilledsgprs 34
        .spilledvgprs 42
        .runtime_handle "SomeCodeToExec"
        # arg infos
        .arg , "", 8, 8, none, i64
        .arg , "", 8, 8, complact, i64
.kernel kxx1
        .arg , "", 8, 8, printfbuxf, i64
        .arg vx1, "void*", 8, 8, globalbuf, xi8, global, default volatile
        .arg vx1, "void*", 8, 8, globalbuf, xi8, global, default volxx
        .arg vx1, "void*", 8, 8, globalbuf, i8, global, volatile
        .arg vx1, "void*", 8, 8, globalbuf, i8, global :: xx
        .md_language "jezorx", ,
        .md_vgprsnum  
        .runtime_handle 144
.main
        .printf 22,,,"aa"
.text
kxx1:   .skip 256
        s_mov_b32 s7, 0
        s_endpgm
)ffDXD",
        "",
        R"ffDXD(test.s:17:9: Error: Illegal place of configuration pseudo-op
test.s:18:9: Error: Illegal place of configuration pseudo-op
test.s:19:9: Error: Illegal place of configuration pseudo-op
test.s:20:9: Error: Illegal place of configuration pseudo-op
test.s:21:9: Error: Illegal place of configuration pseudo-op
test.s:22:9: Error: Illegal place of configuration pseudo-op
test.s:23:9: Error: Illegal place of configuration pseudo-op
test.s:24:9: Error: Illegal place of configuration pseudo-op
test.s:25:9: Error: Illegal place of configuration pseudo-op
test.s:26:9: Error: Illegal place of configuration pseudo-op
test.s:27:9: Error: Illegal place of configuration pseudo-op
test.s:28:9: Error: Illegal place of configuration pseudo-op
test.s:29:9: Error: Illegal place of configuration pseudo-op
test.s:30:9: Error: Illegal place of configuration pseudo-op
test.s:32:9: Error: Illegal place of configuration pseudo-op
test.s:33:9: Error: Illegal place of configuration pseudo-op
test.s:35:26: Error: Unknown value kind
test.s:36:45: Error: Unknown value type
test.s:37:45: Error: Unknown value type
test.s:37:66: Error: Unknown argument flag
test.s:38:57: Error: Unknown access qualifier
test.s:39:56: Error: Some garbages at argument flag place
test.s:40:32: Error: Expected expression
test.s:40:33: Error: Expected expression
test.s:41:23: Error: Expected expression
test.s:42:25: Error: Expected string
test.s:44:20: Error: Expected expression
test.s:44:21: Error: Expected expression
)ffDXD", false
    },
    {   // with got symbols
        R"ffDXD(.rocm
.gpu Iceland
.arch_minor 0
.arch_stepping 0
.eflags 2
.newbinfmt
.target "amdgcn-amd-amdhsa-amdgizcl-gfx800"
.md_version 1, 0
.globaldata
.gotsym gdata2
gdata1:
.global gdata1
.int 1,2,3,4,6
gdata2:
.global gdata2
.int 11,21,13,14,61
.gotsym gdata1
.gotsym datav, datav.GOT
.gotsym globalValX, globalValX.GOT
.global datav.GOT
.global globalValX.GOT
.kernel vectorAdd
    .config
        .dims x
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .md_language "OpenCL", 1, 2
        .arg n, "uint", 4, 4, value, u32
        .arg a, "float*", 8, 8, globalbuf, f32, global, default const
        .arg b, "float*", 8, 8, globalbuf, f32, global, default const
        .arg c, "float*", 8, 8, globalbuf, f32, global, default
        .arg , "", 8, 8, gox, i64
        .arg , "", 8, 8, goy, i64
        .arg , "", 8, 8, goz, i64
.text
vectorAdd:
.skip 256
        s_mov_b32   s1, .-gdata2
.global datav
datav:
.global globalValX
globalValX = 334
)ffDXD",
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=datav, offset=264, size=0, type=data
  ROCmSymbol: name=vectorAdd, offset=0, size=0, type=kernel
    Config:
      amdCodeVersion=1.1
      amdMachine=1:8:0:0
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xc0040
      computePgmRsrc2=0x90
      enableSgprRegisterFlags=0xb
      enableFeatureFlags=0xa
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=0
      kernargSegmentSize=64
      workgroupFbarrierCount=0
      wavefrontSgprCount=11
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
      callConvention=0x0
      runtimeLoaderKernelSymbol=0x0
      ControlDirective:
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
  Comment:
  nullptr
  Code:
  0100000001000000010008000000000000010000000000000000000000000000
  0000000000000000000000000000000040000c00900000000b000a0000000000
  00000000000000004000000000000000000000000b0001000000000000000000
  0000000004040406000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  ff0081be50070000
  GlobalData:
  01000000020000000300000004000000060000000b000000150000000d000000
  0e0000003d000000
  MetadataInfo:
    Version: 1.0
    Kernel: vectorAdd
      SymName=
      Language=OpenCL 1.2
      ReqdWorkGroupSize=0 0 0
      WorkGroupSizeHint=0 0 0
      VecTypeHint=
      RuntimeHandle=
      KernargSegmentSize=18446744073709551614
      KernargSegmentAlign=18446744073709551614
      GroupSegmentFixedSize=18446744073709551614
      PrivateSegmentFixedSize=18446744073709551614
      WaveFrontSize=4294967294
      SgprsNum=4294967294
      VgprsNum=4294967294
      SpilledSgprs=4294967294
      SpilledVgprs=4294967294
      MaxFlatWorkGroupSize=18446744073709551614
      FixedWorkGroupSize=0 0 0
      Arg name=n, type=uint, size=4, align=4
        valuekind=value, valuetype=u32, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=a, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= const
      Arg name=b, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags= const
      Arg name=c, type=float*, size=8, align=8
        valuekind=globalbuf, valuetype=f32, pointeeAlign=0
        addrSpace=global, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=gox, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=goy, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
      Arg name=, type=, size=8, align=8
        valuekind=goz, valuetype=i64, pointeeAlign=0
        addrSpace=none, accQual=default, actAccQual=default
        Flags=
  Target=amdgcn-amd-amdhsa-amdgizcl-gfx800
  EFlags=2
  NewBinFormat
  GotSymbols:
    Sym: gdata2
    Sym: gdata1
    Sym: datav
    Sym: globalValX
  Symbol: name=datav.GOT, value=16, size=0, section=4294967055
  Symbol: name=gdata1, value=0, size=0, section=4294967046
  Symbol: name=gdata2, value=20, size=0, section=4294967046
  Symbol: name=globalValX, value=334, size=0, section=4294967294
  Symbol: name=globalValX.GOT, value=24, size=0, section=4294967055
)ffDXD",
        "", true
    },
    {   // with different dims for local_ids and group_ids
        R"ffDXD(        .rocm
        .gpu Fiji
.kernel someKernelX
    .config
        .dims xy,xz
        .reserved_vgprs 0, 11
.text
someKernelX:
        .skip 256
        s_endpgm)ffDXD",
        R"ffDXD(ROCmBinDump:
  ROCmSymbol: name=someKernelX, offset=0, size=0, type=kernel
    Config:
      amdCodeVersion=1.0
      amdMachine=1:8:0:3
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xc0000
      computePgmRsrc2=0x1180
      enableSgprRegisterFlags=0x0
      enableFeatureFlags=0x0
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=0
      kernargSegmentSize=0
      workgroupFbarrierCount=0
      wavefrontSgprCount=4
      workitemVgprCount=3
      reservedVgprFirst=0
      reservedVgprCount=12
      reservedSgprFirst=0
      reservedSgprCount=0
      debugWavefrontPrivateSegmentOffsetSgpr=0
      debugPrivateSegmentBufferSgpr=0
      kernargSegmentAlignment=4
      groupSegmentAlignment=4
      privateSegmentAlignment=4
      wavefrontSize=6
      callConvention=0x0
      runtimeLoaderKernelSymbol=0x0
      ControlDirective:
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
  Comment:
  nullptr
  Code:
  0100000000000000010008000000030000010000000000000000000000000000
  0000000000000000000000000000000000000c00801100000000000000000000
  00000000000000000000000000000000000000000400030000000c0000000000
  0000000004040406000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  000081bf
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
    bool good = assembler.assemble();
    
    std::ostringstream dumpOss;
    if (good && assembler.getFormatHandler()!=nullptr)
        // get format handler and their output
        printROCmOutput(dumpOss, static_cast<const AsmROCmHandler*>(
                    assembler.getFormatHandler())->getOutput());
    /* compare results dump with expected dump */
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
