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
        os << "    Code:\n";
        printHexData(os, 2, kernel.codeSize, kernel.code);
        if (!kernel.useConfig)
        {
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
        {   // when config
            const AmdCL2KernelConfig& config = kernel.config;
            os << "    Config:\n";
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
            os << "      dims=" << confValueToString(config.dimMask) << ", "
                    "cws=" << config.reqdWorkGroupSize[0] << " " <<
                    config.reqdWorkGroupSize[1] << " " << config.reqdWorkGroupSize[2] << ", "
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
        }
        std::vector<AmdCL2RelInput> relocs(kernel.relocations.begin(),
                            kernel.relocations.end());
        std::sort(relocs.begin(), relocs.end(),
                  [] (const AmdCL2RelInput& a, const AmdCL2RelInput& b)
                  { return a.offset < b.offset; });
        for (const AmdCL2RelInput& rel: relocs)
            os << "    Rel: offset=" << rel.offset << ", type=" << rel.type << ", "
                "symbol=" << rel.symbol << ", addend=" << rel.addend << "\n";
    }
    
    os << "  GlobalData:\n";
    printHexData(os,  1, output->globalDataSize, output->globalData);
    os << "  RwData:\n";
    printHexData(os,  1, output->rwDataSize, output->rwData);
    os << "  Bss size: " << output->bssSize << ", bssAlign: " <<
                output->bssAlignment << "\n";
    os << "  SamplerInit:\n";
    printHexData(os,  1, output->samplerInitSize, output->samplerInit);
    
    for (BinSection section: output->innerExtraSections)
    {
        os << "    ISection " << section.name << ", type=" << section.type <<
                        ", flags=" << section.flags << ":\n";
        printHexData(os, 2, section.size, section.data);
    }
    for (BinSymbol symbol: output->innerExtraSymbols)
        os << "    ISymbol: name=" << symbol.name << ", value=" << symbol.value <<
                ", size=" << symbol.size << ", section=" << symbol.sectionId << "\n";
    
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
      dims=1, cws=0 0 0, SGPRS=12, VGPRS=1
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
      dims=1, cws=0 0 0, SGPRS=12, VGPRS=1
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
      dims=7, cws=8 5 2, SGPRS=25, VGPRS=78
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
        s_endpgm
aa = gstart2 + 100
bb = aa + 3
x=3*6)ffDXD",
        R"ffDXD(AmdCL2BinDump:
  devType=Bonaire, aclVersion=, drvVersion=191205, compileOptions=""
  Kernel: aaa1
    Code:
    05ac098705850a87ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be55555555ff0381be55555555ff0381be55555555ff0381be55555555
    ff0381be555555550c55555555000081bf
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
      dims=1, cws=0 0 0, SGPRS=11, VGPRS=1
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
test.s:8:15: Error: Expected expression
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
        printAmdCL2Output(dumpOss, static_cast<const AsmAmdCL2Handler*>(
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
