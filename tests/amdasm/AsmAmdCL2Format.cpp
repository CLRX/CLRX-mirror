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
#include <CLRX/amdbin/AmdCL2BinGen.h>
#include <CLRX/amdasm/Assembler.h>
#include <CLRX/amdasm/AsmFormats.h>
#include "../TestUtils.h"

using namespace CLRX;

static const char* argTypeNameTbl[] =
{
    "void", "uchar", "char", "ushort", "short", "uint", "int",
    "ulong", "long", "float", "double", "pointer",
    "image", "image1d", "image1d_array", "image1d_buffer",
    "image2d", "image2d_array", "image3d",
    "uchar2", "uchar3", "uchar4", "uchar8", "uchar16",
    "char2", "char3", "char4", "char8", "char16",
    "ushort2", "ushort3", "ushort4", "ushort8", "ushort16",
    "short2", "short3", "short4", "short8", "short16",
    "uint2", "uint3", "uint4", "uint8", "uint16",
    "int2", "int3", "int4", "int8", "int16",
    "ulong2", "ulong3", "ulong4", "ulong8", "ulong16",
    "long2", "long3", "long4", "long8", "long16",
    "float2", "float3", "float4", "float8", "float16",
    "double2", "double3", "double4", "double8", "double16",
    "sampler", "structure", "counter32", "counter64",
    "pipe", "cmdqueue", "clkevent"
};

static const char* ptrSpaceNameTbl[] =
{ "none", "local", "constant", "global" };

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

// helper for printing value from kernel config (print default and notsupplied)
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


// print dump of AMD OpenCL2.0 output to stream for comparing with testcase
static void printAmdCL2Output(std::ostream& os, const AmdCL2Input* output)
{
    os << "AmdCL2BinDump:" << std::endl;
    os << "  devType=" << getGPUDeviceTypeName(output->deviceType) << ", "
            "aclVersion=" << output->aclVersion << ", "
            "drvVersion=" << output->driverVersion << ", "
            "compileOptions=\"" << output->compileOptions << "\"\n";
    
    for (const AmdCL2KernelInput& kernel: output->kernels)
    {
        os << "  Kernel: " << kernel.kernelName << "\n";
        if (kernel.code != nullptr)
        {
            os << "    Code:\n";
            printHexData(os, 2, kernel.codeSize, kernel.code);
        }
        if (!kernel.useConfig)
        {
            // print metadata, setup and stub (if no config)
            os << "    Metadata:\n";
            printHexData(os, 2, kernel.metadataSize, (const cxbyte*)kernel.metadata);
            os << "    Setup:\n";
            printHexData(os, 2, kernel.setupSize, (const cxbyte*)kernel.setup);
            if (kernel.stubSize!=0)
            {
                os << "    Stub:\n";
                printHexData(os, 2, kernel.stubSize, (const cxbyte*)kernel.stub);
            }
            if (kernel.isaMetadataSize!=0)
            {
                os << "    ISAMetadata:\n";
                printHexData(os, 2, kernel.metadataSize, (const cxbyte*)kernel.metadata);
            }
        }
        else
        {
            // when config
            const AmdCL2KernelConfig& config = kernel.config;
            if (!kernel.hsaConfig)
                os << "    Config:\n";
            else
                os << "    HSAConfig:\n";
            for (AmdKernelArgInput arg: config.args)
                os << "      Arg: \"" << arg.argName << "\", \"" <<
                        arg.typeName << "\", " <<
                        argTypeNameTbl[cxuint(arg.argType)] << ", " <<
                        argTypeNameTbl[cxuint(arg.pointerType)] << ", " <<
                        ptrSpaceNameTbl[cxuint(arg.ptrSpace)] << ", " <<
                        cxuint(arg.ptrAccess) << ", " << arg.structSize << ", " <<
                        arg.constSpaceSize << ", " <<
                        confValueToString(arg.resId) << ", " <<
                        cxuint(arg.used) << "\n";
            if (!config.samplers.empty())
            {
                os << "      Sampler:";
                for (cxuint sampler: config.samplers)
                    os << " " << sampler;
                os << '\n';
            }
            if (!kernel.hsaConfig)
            {
                // print kernel config in old style
                os << "      dims=" << confDimMaskToString(config.dimMask) << ", "
                        "cws=" << config.reqdWorkGroupSize[0] << " " <<
                            config.reqdWorkGroupSize[1] << " " <<
                            config.reqdWorkGroupSize[2] << ", "
                        "SGPRS=" << config.usedSGPRsNum << ", "
                        "VGPRS=" << config.usedVGPRsNum << "\n      "
                        "pgmRSRC1=" << std::hex << "0x" << config.pgmRSRC1 << ", "
                        "pgmRSRC2=0x" << config.pgmRSRC2 << ", "
                        "ieeeMode=0x" << std::dec << config.ieeeMode << std::hex << ", "
                        "floatMode=0x" << config.floatMode<< std::dec << "\n      "
                        "priority=" << config.priority << ", "
                        "exceptions=" << cxuint(config.exceptions) << ", "
                        "localSize=" << config.localSize << ", "
                        "scratchBuffer=" << config.scratchBufferSize << "\n      " <<
                        (config.tgSize?"tgsize ":"") <<
                        (config.debugMode?"debug ":"") <<
                        (config.privilegedMode?"priv ":"") <<
                        (config.dx10Clamp?"dx10clamp ":"") <<
                        (config.useSetup?"useSetup ":"") <<
                        (config.useArgs?"useArgs ":"") <<
                        (config.useEnqueue?"useEnqueue ":"") <<
                        (config.useGeneric?"useGeneric ":"") << "\n";
                if (!config.vecTypeHint.empty() ||
                    config.workGroupSizeHint[0] != 0 ||
                    config.workGroupSizeHint[1] != 0 ||
                    config.workGroupSizeHint[1] != 0)
                {
                    os << "      vectypehint=" << config.vecTypeHint.c_str() << ", "
                        "workGroupSizeHint=" << config.workGroupSizeHint[0] << " " <<
                        config.workGroupSizeHint[1] << " " <<
                        config.workGroupSizeHint[2] << "\n";
                }
            }
            else
            {
                // print AMD HSA config
                const AmdHsaKernelConfig& config =
                        *reinterpret_cast<const AmdHsaKernelConfig*>(kernel.setup);
                os <<
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
                    "      computePgmRsrc1=0x" <<
                        std::hex << ULEV(config.computePgmRsrc1) << "\n"
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
                    "      callConvention=0x" <<
                        std::hex << ULEV(config.callConvention) << "\n"
                    "      runtimeLoaderKernelSymbol=0x" <<
                        ULEV(config.runtimeLoaderKernelSymbol) << std::dec << "\n";
                os << "      ControlDirective:\n";
                printHexData(os, 3, 128, config.controlDirective);
            }
        }
        // sort relocation before printing
        std::vector<AmdCL2RelInput> relocs(kernel.relocations.begin(),
                            kernel.relocations.end());
        std::sort(relocs.begin(), relocs.end(),
                  [] (const AmdCL2RelInput& a, const AmdCL2RelInput& b)
                  { return a.offset < b.offset; });
        // print relocations
        for (const AmdCL2RelInput& rel: relocs)
            os << "    Rel: offset=" << rel.offset << ", type=" << rel.type << ", "
                "symbol=" << rel.symbol << ", addend=" << rel.addend << "\n";
    }
    // print code
    if (output->code != nullptr)
    {
        os << "  Code:\n";
        printHexData(os,  1, output->codeSize, output->code);
    }
    {
        // sort relocation before printing
        std::vector<AmdCL2RelInput> relocs(output->relocations.begin(),
                            output->relocations.end());
        std::sort(relocs.begin(), relocs.end(),
                  [] (const AmdCL2RelInput& a, const AmdCL2RelInput& b)
                  { return a.offset < b.offset; });
        // print relocations
        for (const AmdCL2RelInput& rel: relocs)
            os << "  Rel: offset=" << rel.offset << ", type=" << rel.type << ", "
                "symbol=" << rel.symbol << ", addend=" << rel.addend << "\n";
    }
    // print globaldata, rwdata, bss
    os << "  GlobalData:\n";
    printHexData(os,  1, output->globalDataSize, output->globalData);
    os << "  RwData:\n";
    printHexData(os,  1, output->rwDataSize, output->rwData);
    os << "  Bss size: " << output->bssSize << ", bssAlign: " <<
                output->bssAlignment << "\n";
    os << "  SamplerInit:\n";
    printHexData(os,  1, output->samplerInitSize, output->samplerInit);
    
    // print extra sections from inner binary if supplied
    for (BinSection section: output->innerExtraSections)
    {
        os << "    ISection " << section.name << ", type=" << section.type <<
                        ", flags=" << section.flags << ":\n";
        printHexData(os, 2, section.size, section.data);
    }
    // print extra symbols from inner binary if supplied
    for (BinSymbol symbol: output->innerExtraSymbols)
        os << "    ISymbol: name=" << symbol.name << ", value=" << symbol.value <<
                ", size=" << symbol.size << ", section=" << symbol.sectionId << "\n";
    
    // print extra sections from main binary if supplied
    for (BinSection section: output->extraSections)
    {
        os << "  Section " << section.name << ", type=" << section.type <<
                        ", flags=" << section.flags << ":\n";
        printHexData(os, 1, section.size, section.data);
    }
    // print extra symbols from main binary if supplied
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
    /* AMD CL2 (no config) */
    {
R"ffDXD(            .amdcl2
            .64bit
            .gpu Bonaire
            .driver_version 191205
            .kernel aaa1
                .metadata
                    .byte 1,2,3,4,44
                .setup
                    .byte 0,6,0,3,4

            .kernel aaa2
                .setup
                    .byte 0,6,0,3,4
                .metadata
                    .byte 1,2,3,4,44
            .kernel aaa1
                    .byte 0xc1,0xc4
            .main
            .globaldata
                .byte 44,55,66
            .bssdata align=8
                .skip 50
            .main
            .section .ala
                .string "ala"
            .inner
            .section .beta
                .string "beta"
            .main
                .byte 0xa,0xba
            .inner
                .byte 0xff,0xfd
            .main
            .bssdata
                .skip 10
            .main
            .section .xx
                .byte 1,23
            .inner
            .section .xx
                .byte 12,13
            .kernel vu
                .section aaaxx
                .text
                    s_endpgm
            .inner
                .section aaaxx
                .byte 0xcd,0xdc)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    Metadata:
    010203042c
    Setup:
    0006000304c1c4
  Kernel: aaa2
    Code:
    Metadata:
    010203042c
    Setup:
    0006000304
  Kernel: vu
    Code:
    000081bf
    Metadata:
    nullptr
    Setup:
    nullptr
  GlobalData:
  2c3742
  RwData:
  nullptr
  Bss size: 60, bssAlign: 8
  SamplerInit:
  nullptr
    ISection .beta, type=1, flags=0:
    6265746100fffd
    ISection .xx, type=1, flags=0:
    0c0d
    ISection aaaxx, type=1, flags=0:
    cddc
  Section .ala, type=1, flags=0:
  616c61000aba
  Section .xx, type=1, flags=0:
  0117
)ffDXD", "", true
    },
    /* AMD CL2 (no config) (no main before rodata or bss) */
    {
R"ffDXD(            .amdcl2
            .64bit
            .gpu Bonaire
            .driver_version 191205
            .kernel aaa1
                .metadata
                    .byte 1,2,3,4,44
                .setup
                    .byte 0,6,0,3,4

            .kernel aaa2
                .setup
                    .byte 0,6,0,3,4
                .metadata
                    .byte 1,2,3,4,44
            .kernel aaa1
                    .byte 0xc1,0xc4
            .section .rodata
                .byte 44,55,66
            .bssdata align=8
                .skip 50
            .main
            .section .ala
                .string "ala"
            .inner
            .section .beta
                .string "beta"
            .main
                .byte 0xa,0xba
            .inner
                .byte 0xff,0xfd
            .bssdata
                .skip 10
            .main
            .section .xx
                .byte 1,23
            .inner
            .section .xx
                .byte 12,13
            .kernel vu
                .section aaaxx
                .text
                    s_endpgm
            .inner
                .section aaaxx
                .byte 0xcd,0xdc)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    Metadata:
    010203042c
    Setup:
    0006000304c1c4
  Kernel: aaa2
    Code:
    Metadata:
    010203042c
    Setup:
    0006000304
  Kernel: vu
    Code:
    000081bf
    Metadata:
    nullptr
    Setup:
    nullptr
  GlobalData:
  2c3742
  RwData:
  nullptr
  Bss size: 60, bssAlign: 8
  SamplerInit:
  nullptr
    ISection .beta, type=1, flags=0:
    6265746100fffd
    ISection .xx, type=1, flags=0:
    0c0d
    ISection aaaxx, type=1, flags=0:
    cddc
  Section .ala, type=1, flags=0:
  616c61000aba
  Section .xx, type=1, flags=0:
  0117
)ffDXD", "", true
    },
    /* AMD CL2 (old format, no config) */
    {
        R"ffDXD(.amdcl2
.64bit
.gpu Bonaire
.driver_version 180005
.kernel aaa1
    .metadata
        .byte 1,2,3,4,44
    .setup
        .byte 0,6,0,3,4
    .stub
.kernel aaa2
    .stub
        .byte 0xf0,0xef,0xe1
    .setup
        .byte 0,6,0,3,4
    .metadata
        .byte 1,2,3,4,44
    .isametadata
        .byte 0xd0,0xd1,0xd4
.kernel aaa1
        .byte 0xc1,0xc4
.main
    .section .cc
        .short 12,34,261)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=180005, compileOptions=""
  Kernel: aaa1
    Code:
    Metadata:
    010203042c
    Setup:
    0006000304
    Stub:
    c1c4
  Kernel: aaa2
    Code:
    Metadata:
    010203042c
    Setup:
    0006000304
    Stub:
    f0efe1
    ISAMetadata:
    010203042c
  GlobalData:
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
  Section .cc, type=1, flags=0:
  0c0022000501
)ffDXD", "", true
    },
    /* AMDCL2 with configs */
    {
        R"ffDXD(.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.kernel aaa1
    .config
        .dims x
        .setupargs
        .arg n,uint
        .arg in,uint*,global,const
        .arg out,uint*,global
        .ieeemode
        .floatmode 0xda
        .localsize 1000
        .useargs
    .text
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
.kernel aaa2
    .config
        .dims x
        .setupargs
        .arg n,uint
        .arg in,float*,global,const
        .arg out,float*,global
        .arg q,queue
        .arg piper,pipe
        .arg ce,clkevent
        .scratchbuffer 2342
        .ieeemode
        .floatmode 0xda
        .localsize 1000
        .useargs
        .priority 2
        .privmode
        .debugmode
        .dx10clamp
        .exceptions 0x12
        .useenqueue
    .text
        s_and_b32 s9,s5,44
        s_endpgm
.kernel aaa1
        s_and_b32 s11,s5,5
        s_endpgm
.kernel gfd12
    .config
        .dims xyz
        .cws 8,5,2
        .sgprsnum 25
        .vgprsnum 78
        .setupargs
        .arg n,uint
        .arg in,double*,global,const
        .arg out,double*,global
        .arg v,uchar,rdonly
        .arg v2,uchar,wronly
        .localsize 656
        .pgmrsrc1 0x1234000
        .pgmrsrc2 0xfcdefd0
        .useargs
        .usesetup
        .usegeneric
    .text
        v_mov_b32 v1,v2
        v_add_i32 v1,vcc,55,v3
        s_endpgm
)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    05ac098705850a8705850b87000081bf
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "in", "uint*", pointer, uint, global, 4, 0, 0, default, 3
      Arg: "out", "uint*", pointer, uint, global, 0, 0, 0, default, 3
      dims=9, cws=0 0 0, SGPRS=12, VGPRS=1
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x1, floatMode=0xda
      priority=0, exceptions=0, localSize=1000, scratchBuffer=0
      useArgs 
  Kernel: aaa2
    Code:
    05ac0987000081bf
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "in", "float*", pointer, float, global, 4, 0, 0, default, 3
      Arg: "out", "float*", pointer, float, global, 0, 0, 0, default, 3
      Arg: "q", "queue_t", cmdqueue, void, none, 0, 0, 0, default, 3
      Arg: "piper", "pipe", pipe, void, none, 0, 0, 0, default, 3
      Arg: "ce", "clk_event_t", clkevent, void, none, 0, 0, 0, default, 3
      dims=9, cws=0 0 0, SGPRS=12, VGPRS=1
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x1, floatMode=0xda
      priority=2, exceptions=18, localSize=1000, scratchBuffer=2342
      debug priv dx10clamp useArgs useEnqueue 
  Kernel: gfd12
    Code:
    0203027eb706024a000081bf
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "in", "double*", pointer, double, global, 4, 0, 0, default, 3
      Arg: "out", "double*", pointer, double, global, 0, 0, 0, default, 3
      Arg: "v", "uchar", uchar, void, none, 0, 0, 0, default, 1
      Arg: "v2", "uchar", uchar, void, none, 0, 0, 0, default, 2
      dims=63, cws=8 5 2, SGPRS=25, VGPRS=78
      pgmRSRC1=0x1234000, pgmRSRC2=0xfcdefd0, ieeeMode=0x0, floatMode=0xc0
      priority=0, exceptions=0, localSize=656, scratchBuffer=0
      useSetup useArgs useGeneric 
  GlobalData:
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD", "", true
    },
    /* AMDCL2 - relocations */
    {
        R"ffDXD(.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.globaldata
gstart:
        .int 1,2,3,4,5,6
gstart2:
gstartx = .   # replaced later after reloc
.rwdata
        .int 4,5
rwdat1:
.bssdata
        .skip 10
bsslabel:
.kernel aaa1
    .config
        .dims x
        .setupargs
        .arg n,uint
        .arg in,uint*,global,const
        .arg out,uint*,global
        .ieeemode
        .useargs
cc=gstart+10+x
    .text
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
        s_mov_b32 s1, gstart+77
        s_mov_b32 s1, gstart+7*3
        s_mov_b32 s1, (gstart+7*3)&(1<<32-1)
        s_mov_b32 s1, ((gstart+7*3)*2-gstart)&(4096*4096*256-1)
        s_mov_b32 s1, (-gstart+2*(gstart+7*3))%(4096*4096*256)
        s_mov_b32 s1, (-gstart+2*(gstart2+7*3))%%(4096*4096*256)
        s_mov_b32 s1, (-gstart+2*(gstart2+7*5))%%(4096*4096*256*9)
        s_mov_b32 s1, (-gstart+2*(gstart2+7*5))%(4096*4096*256*9)
        s_mov_b32 s1, (gstart+7*3)>>(31-5+6)
        s_mov_b32 s1, (gstart+7*3)>>(234%101)
        s_mov_b32 s1, (gstart+7*3)>>(235%101-1)
        s_mov_b32 s1, (gstart+7*3)/(4096*4096*256)
        s_mov_b32 s1, (gstart+7*3)//(4096*4096*256)
        s_mov_b32 s1, aa>>32
        s_mov_b32 s1, (aa-10)>>32
        s_mov_b32 s1, (bb-10)>>32
        s_mov_b32 s1, (cc+1)>>32
        s_mov_b32 s1, gstart2+77
        s_mov_b32 s1, rwdat1+33
        s_mov_b32 s1, bsslabel+33
        .byte 12
        .int (gstart+22)>>32
        .int (gstartx+22)>>32
        s_endpgm
aa = gstart2 + 100
bb = aa + 3
x=3*6
gstartx = 11)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    05ac098705850a87ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be555555550c5555555555555555000081bf
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "in", "uint*", pointer, uint, global, 4, 0, 0, default, 3
      Arg: "out", "uint*", pointer, uint, global, 0, 0, 0, default, 3
      dims=9, cws=0 0 0, SGPRS=11, VGPRS=1
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x1, floatMode=0xc0
      priority=0, exceptions=0, localSize=0, scratchBuffer=0
      useArgs 
    Rel: offset=12, type=1, symbol=0, addend=77
    Rel: offset=20, type=1, symbol=0, addend=21
    Rel: offset=28, type=1, symbol=0, addend=21
    Rel: offset=36, type=1, symbol=0, addend=42
    Rel: offset=44, type=1, symbol=0, addend=42
    Rel: offset=52, type=1, symbol=0, addend=90
    Rel: offset=60, type=1, symbol=0, addend=118
    Rel: offset=68, type=1, symbol=0, addend=118
    Rel: offset=76, type=2, symbol=0, addend=21
    Rel: offset=84, type=2, symbol=0, addend=21
    Rel: offset=92, type=2, symbol=0, addend=21
    Rel: offset=100, type=2, symbol=0, addend=21
    Rel: offset=108, type=2, symbol=0, addend=21
    Rel: offset=116, type=2, symbol=0, addend=124
    Rel: offset=124, type=2, symbol=0, addend=114
    Rel: offset=132, type=2, symbol=0, addend=117
    Rel: offset=140, type=2, symbol=0, addend=29
    Rel: offset=148, type=1, symbol=0, addend=101
    Rel: offset=156, type=1, symbol=1, addend=41
    Rel: offset=164, type=1, symbol=2, addend=43
    Rel: offset=169, type=2, symbol=0, addend=22
    Rel: offset=173, type=2, symbol=0, addend=46
  GlobalData:
  010000000200000003000000040000000500000006000000
  RwData:
  0400000005000000
  Bss size: 10, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD", "", true
    },
    /* AMDCL2 - errors */
    {
        R"ffDXD(.amdcl2
.gpu Bonaire
.driver_version 191205
.kernel aaa1
    .config
        .dims xyd
        .setupargs
        .cws  
        .arg n,uint
        .arg in,uint*,global,const
        .arg out,uint*,global
        .ieeemode
        .floatmode 0x111da
        .localsize 103000
        .priority 6
        .exceptions 0xd9
        .sgprsnum 231
        .arg vx,uint,dxdd
        .useargs
    .text
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
        .dims x
        .setupargs
        .arg n,uint
        .arg in,float*,global,const
        .arg out,float*,global
        .arg q,queue
        .arg piper,pipe
        .arg ce,clkevent
        .scratchbuffer 2342
        .ieeemode
        .floatmode 0xda
        .localsize 1000
        .useargs
        .priority 2
        .privmode
        .debugmode
        .dx10clamp
        .exceptions 0x12
        .useenqueue
.kernel aaa2
    .metadata
        .byte 1,234,4
    .config)ffDXD",
        "",
        R"ffDXD(test.s:6:15: Error: Unknown dimension type
test.s:13:20: Warning: Value 0x111da truncated to 0xda
test.s:14:20: Error: LocalSize out of range (0-32768)
test.s:15:19: Warning: Value 0x6 truncated to 0x2
test.s:16:21: Warning: Value 0xd9 truncated to 0x59
test.s:17:19: Error: Used SGPRs number out of range (0-104)
test.s:18:22: Error: This is not 'unused' specifier
test.s:23:9: Error: Illegal place of configuration pseudo-op
test.s:24:9: Error: Illegal place of kernel argument
test.s:25:9: Error: Illegal place of kernel argument
test.s:26:9: Error: Illegal place of kernel argument
test.s:27:9: Error: Illegal place of kernel argument
test.s:28:9: Error: Illegal place of kernel argument
test.s:29:9: Error: Illegal place of kernel argument
test.s:30:9: Error: Illegal place of kernel argument
test.s:31:9: Error: Illegal place of configuration pseudo-op
test.s:32:9: Error: Illegal place of configuration pseudo-op
test.s:33:9: Error: Illegal place of configuration pseudo-op
test.s:34:9: Error: Illegal place of configuration pseudo-op
test.s:35:9: Error: Illegal place of configuration pseudo-op
test.s:36:9: Error: Illegal place of configuration pseudo-op
test.s:37:9: Error: Illegal place of configuration pseudo-op
test.s:38:9: Error: Illegal place of configuration pseudo-op
test.s:39:9: Error: Illegal place of configuration pseudo-op
test.s:40:9: Error: Illegal place of configuration pseudo-op
test.s:41:9: Error: Illegal place of configuration pseudo-op
test.s:45:5: Error: Config can't be defined if metadata,header,setup,stub section exists
)ffDXD", false
    },
    /* AMD HSA config */
    {
        R"ffDXD(.amdcl2
.gpu Bonaire
.64bit
.arch_minor 0
.arch_stepping 0
.driver_version 234800
.kernel GenerateScramblerKernel
    .hsaconfig
        .dims xyz
        .sgprsnum 22
        .vgprsnum 16
        .dx10clamp
        .ieeemode
        .floatmode 0xc0
        .priority 0
        .userdatanum 6
        .codeversion 1, 1
        .machine 1, 0, 0, 0
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .byte 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 120, 1, 0x00
    .hsaconfig
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg d_wiring, "Wiring*", structure*, 1024, constant, const, rdonly
        .arg d_key, "Key*", structure*, 64, constant, const, rdonly
        .arg thblockShift, "uint", uint
        .arg localShift, "uint", uint
        .arg scramblerDataPitch, "uint", uint
        .arg scramblerData, "int8_t*", char*, global, 
    .text
        s_load_dwordx2  s[0:1], s[4:5], 0x10
        s_waitcnt       lgkmcnt(0)
        s_sub_u32       s1, s1, s0
        v_bfe_u32       v1, v0, 0, s1
        v_cmp_ge_u32    vcc, 25, v1
        s_and_saveexec_b64 s[2:3], vcc
        v_lshrrev_b32   v0, s1, v0
        s_endpgm
)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=234800, compileOptions=""
  Kernel: GenerateScramblerKernel
    Code:
    100540c07f008cbf01008180010090d20001050099028c7d6a2482be0100002c
    000081bf
    HSAConfig:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 0, 0, default, 1
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "d_wiring", "Wiring*", pointer, structure, constant, 4, 1024, 0, default, 1
      Arg: "d_key", "Key*", pointer, structure, constant, 4, 64, 0, default, 1
      Arg: "thblockShift", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "localShift", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "scramblerDataPitch", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "scramblerData", "int8_t*", pointer, char, global, 0, 0, 0, default, 3
      amdCodeVersion=1.1
      amdMachine=1:0:0:0
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xac0083
      computePgmRsrc2=0x138c
      enableSgprRegisterFlags=0x9
      enableFeatureFlags=0xa
      workitemPrivateSegmentSize=0
      workgroupGroupSegmentSize=0
      gdsSegmentSize=0
      kernargSegmentSize=96
      workgroupFbarrierCount=0
      wavefrontSgprCount=22
      workitemVgprCount=16
      reservedVgprFirst=16
      reservedVgprCount=0
      reservedSgprFirst=20
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
      0001000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
  GlobalData:
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD", "", true
    },
    {
        R"ffDXD(.amdcl2
.gpu Bonaire
.64bit
.arch_minor 0
.arch_stepping 0
.driver_version 234800
.kernel GenerateScramblerKernel
    .hsaconfig
        .dims xyz
        .dx10clamp
        .ieeemode
        .floatmode 0xc0
        .priority 0
        .userdatanum 6
        .codeversion 1, 1
        .machine 1, 4, 6, 7
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .call_convention 0x0
        .workgroup_fbarrier_count 3324
        .runtime_loader_kernel_symbol 0x4dc98b3a
        .scratchbuffer 77222
        .localsize 413
    .control_directive
        .byte 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .hsaconfig
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg d_wiring, "Wiring*", structure*, 1024, constant, const, rdonly
        .arg d_key, "Key*", structure*, 64, constant, const, rdonly
        .arg thblockShift, "uint", uint
        .arg localShift, "uint", uint
        .arg scramblerDataPitch, "uint", uint
        .arg scramblerData, "int8_t*", char*, global, 
    .control_directive
        .fill 116, 1, 0x00
    .text
        s_mov_b32 s32, s14
        v_mov_b32 v42, v11
        s_endpgm
    .control_directive
        .int 2132
)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=234800, compileOptions=""
  Kernel: GenerateScramblerKernel
    Code:
    0e03a0be0b03547e000081bf
    HSAConfig:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 0, 0, default, 1
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, default, 3
      Arg: "d_wiring", "Wiring*", pointer, structure, constant, 4, 1024, 0, default, 1
      Arg: "d_key", "Key*", pointer, structure, constant, 4, 64, 0, default, 1
      Arg: "thblockShift", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "localShift", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "scramblerDataPitch", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "scramblerData", "int8_t*", pointer, char, global, 0, 0, 0, default, 3
      amdCodeVersion=1.1
      amdMachine=1:4:6:7
      kernelCodeEntryOffset=256
      kernelCodePrefetchOffset=0
      kernelCodePrefetchSize=0
      maxScrachBackingMemorySize=0
      computePgmRsrc1=0xac010a
      computePgmRsrc2=0x138d
      enableSgprRegisterFlags=0x9
      enableFeatureFlags=0xa
      workitemPrivateSegmentSize=77222
      workgroupGroupSegmentSize=413
      gdsSegmentSize=0
      kernargSegmentSize=96
      workgroupFbarrierCount=3324
      wavefrontSgprCount=35
      workitemVgprCount=43
      reservedVgprFirst=43
      reservedVgprCount=0
      reservedSgprFirst=33
      reservedSgprCount=0
      debugWavefrontPrivateSegmentOffsetSgpr=0
      debugPrivateSegmentBufferSgpr=0
      kernargSegmentAlignment=4
      groupSegmentAlignment=4
      privateSegmentAlignment=4
      wavefrontSize=6
      callConvention=0x0
      runtimeLoaderKernelSymbol=0x4dc98b3a
      ControlDirective:
      0001000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000000000000
      0000000000000000000000000000000000000000000000000000000054080000
  GlobalData:
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD", "", true
    },
    {   // with vectypehint and work_group_size_hint
        R"ffDXD(.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.kernel aaa1
    .config
        .dims x
        .setupargs
        .vectypehint float8
        .work_group_size_hint 44,112,5
        .arg n,uint
        .arg in,uint*,global,const
        .arg out,uint*,global
        .ieeemode
        .floatmode 0xda
        .localsize 1000
        .useargs
    .text
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    05ac098705850a87
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "in", "uint*", pointer, uint, global, 4, 0, 0, default, 3
      Arg: "out", "uint*", pointer, uint, global, 0, 0, 0, default, 3
      dims=9, cws=0 0 0, SGPRS=11, VGPRS=1
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x1, floatMode=0xda
      priority=0, exceptions=0, localSize=1000, scratchBuffer=0
      useArgs 
      vectypehint=float8, workGroupSizeHint=44 112 5
  GlobalData:
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD", "", true
    },
    {   // with vectypehint and work_group_size_hint
        // cws and work_group_size_hint defaults
        R"ffDXD(.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.kernel aaa1
    .config
        .dims x
        .setupargs
        .vectypehint float8
        .cws 144,11
        .work_group_size_hint 44
        .arg n,uint
        .arg in,uint*,global,const
        .arg out,uint*,global
        .ieeemode
        .floatmode 0xda
        .localsize 1000
        .useargs
    .text
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    05ac098705850a87
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "in", "uint*", pointer, uint, global, 4, 0, 0, default, 3
      Arg: "out", "uint*", pointer, uint, global, 0, 0, 0, default, 3
      dims=9, cws=144 11 1, SGPRS=11, VGPRS=1
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x1, floatMode=0xda
      priority=0, exceptions=0, localSize=1000, scratchBuffer=0
      useArgs 
      vectypehint=float8, workGroupSizeHint=44 1 1
  GlobalData:
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD", "", true
    },
    {   // with different group_id and local_id dimensions
        R"ffDXD(.amdcl2
.64bit
.gpu Bonaire
.driver_version 191205
.kernel aaa1
    .config
        .dims x, xy
        .setupargs
        .vectypehint float8
        .work_group_size_hint 44,112,5
        .arg n,uint
        .arg in,uint*,global,const
        .arg out,uint*,global
        .ieeemode
        .floatmode 0xda
        .localsize 1000
        .useargs
    .text
        s_and_b32 s9,s5,44
        s_and_b32 s10,s5,5
)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    05ac098705850a87
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "uint", uint, void, none, 0, 0, 0, default, 3
      Arg: "in", "uint*", pointer, uint, global, 4, 0, 0, default, 3
      Arg: "out", "uint*", pointer, uint, global, 0, 0, 0, default, 3
      dims=25, cws=0 0 0, SGPRS=11, VGPRS=2
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x1, floatMode=0xda
      priority=0, exceptions=0, localSize=1000, scratchBuffer=0
      useArgs 
      vectypehint=float8, workGroupSizeHint=44 112 5
  GlobalData:
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD", "", true
    },
    /* AMDCL2 with HSA layout */
    {
        R"ffDXD(.64bit
.amdcl2
.driver_version 240000
.gpu Bonaire
.hsalayout
.globaldata
dat0: .int 1,2,3
dat1: .int 6,7,11
        
.kernel aaa1
    .config
        .dims x
        .useargs
        .usesetup
        .setupargs
        .arg n, int
        .arg a, int*, global, const
        .arg b, int*, global, const
        .arg c, int*, global
.kernel aaa2
    .config
        .dims x
        .localsize 1000
        .useargs
        .usesetup
        .setupargs
        .arg n, int
        .arg a, int*, global, const
        .arg b, int*, global, const
        .arg c, int*, global
.text
aaa2:
.skip 256
        s_mov_b32 s27, s1
        v_mov_b32 v16, v1
        v_mov_b32 v11, dat1&0xffffffff
        s_nop 4
        v_mov_b32 v12, dat0>>32
        s_endpgm
.p2align 8
aaa1:
.skip 256
        s_mov_b32 s14, s1
        v_mov_b32 v26, v1
        v_mov_b32 v11, dat0&0xffffffff
        v_mov_b32 v12, dat1>>32
        s_endpgm
)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=240000, compileOptions=""
  Kernel: aaa1
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "int", int, void, none, 0, 0, 0, default, 3
      Arg: "a", "int*", pointer, int, global, 4, 0, 0, default, 3
      Arg: "b", "int*", pointer, int, global, 4, 0, 0, default, 3
      Arg: "c", "int*", pointer, int, global, 0, 0, 0, default, 3
      dims=9, cws=0 0 0, SGPRS=15, VGPRS=27
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x0, floatMode=0xc0
      priority=0, exceptions=0, localSize=0, scratchBuffer=0
      useSetup useArgs 
  Kernel: aaa2
    Config:
      Arg: "_.global_offset_0", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_1", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.global_offset_2", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.printf_buffer", "size_t", pointer, void, global, 0, 3, 0, 0, 0
      Arg: "_.vqueue_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "_.aqlwrap_pointer", "size_t", long, void, none, 0, 0, 0, 0, 0
      Arg: "n", "int", int, void, none, 0, 0, 0, default, 3
      Arg: "a", "int*", pointer, int, global, 4, 0, 0, default, 3
      Arg: "b", "int*", pointer, int, global, 4, 0, 0, default, 3
      Arg: "c", "int*", pointer, int, global, 0, 0, 0, default, 3
      dims=9, cws=0 0 0, SGPRS=28, VGPRS=17
      pgmRSRC1=0x0, pgmRSRC2=0x0, ieeeMode=0x0, floatMode=0xc0
      priority=0, exceptions=0, localSize=1000, scratchBuffer=0
      useSetup useArgs 
  Code:
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  01039bbe0103207eff02167e55555555040080bfff02187e55555555000081bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  000080bf000080bf000080bf000080bf000080bf000080bf000080bf000080bf
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  0000000000000000000000000000000000000000000000000000000000000000
  01038ebe0103347eff02167e55555555ff02187e55555555000081bf
  Rel: offset=268, type=1, symbol=0, addend=12
  Rel: offset=280, type=2, symbol=0, addend=0
  Rel: offset=780, type=1, symbol=0, addend=0
  Rel: offset=788, type=2, symbol=0, addend=12
  GlobalData:
  01000000020000000300000006000000070000000b000000
  RwData:
  nullptr
  Bss size: 0, bssAlign: 0
  SamplerInit:
  nullptr
)ffDXD",
        "", true
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
        printAmdCL2Output(dumpOss, static_cast<const AsmAmdCL2Handler*>(
                        assembler.getFormatHandler())->getOutput());
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
