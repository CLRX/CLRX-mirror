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
#include <string>
#include <cstring>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/AmdCL2Binaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/amdasm/Disassembler.h>
#include "../TestUtils.h"

using namespace CLRX;

static const cxbyte disasmInput1Global[14] =
{ 13, 56, 66, 213,
  55, 93, 123, 85,
  164, 234, 21, 37,
  44, 188 };

static const char* disasmInput1Kernel1Metadata =
";ARGSTART:__OpenCL_DCT_kernel\n"
";version:3:1:111\n"
";device:pitcairn\n"
";uniqueid:1024\n"
";memory:uavprivate:0\n"
";memory:hwlocal:0\n"
";memory:hwregion:0\n"
";pointer:output:float:1:1:0:uav:12:4:RW:0:0\n"
";pointer:input:float:1:1:16:uav:13:4:RO:0:0\n"
";pointer:dct8x8:float:1:1:32:uav:14:4:RO:0:0\n"
";pointer:inter:float:1:1:48:hl:1:4:RW:0:0\n"
";value:width:u32:1:1:64\n"
";value:blockWidth:u32:1:1:80\n"
";value:inverse:u32:1:1:96\n"
";function:1:1030\n"
";uavid:11\n"
";printfid:9\n"
";cbid:10\n"
";privateid:8\n"
";reflection:0:float*\n"
";reflection:1:float*\n"
";reflection:2:float*\n"
";reflection:3:float*\n"
";reflection:4:uint\n"
";reflection:5:uint\n"
";reflection:6:uint\n"
";ARGEND:__OpenCL_DCT_kernel\n"
"dessss9843re88888888888888888uuuuuuuufdd"
"dessss9843re88888888888888888uuuuuuuufdd"
"dessss9843re88888888888888888uuuuuuuufdd444444444444444444444444"
"\r\r\t\t\t\t\txx\f\f\f\3777x833334441\n";

static const cxbyte disasmInput1Kernel1Header[59] =
{ 1, 2, 0, 0, 0, 44, 0, 0,
  12, 3, 6, 3, 3, 2, 0, 0,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 18,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19
};

static const cxbyte disasmInput1Kernel1Data[40] =
{
    1, 4, 5, 6, 6, 6, 4, 3,
    3, 5, 65, 13, 5, 5, 11, 57,
    5, 65, 5, 0, 0, 0, 0, 5,
    1, 15, 5, 1, 56, 0, 0, 5,
    5, 65, 5, 0, 0, 78, 255, 5
};

static uint32_t disasmInput1Kernel1ProgInfo[13] =
{
    LEV(0xffcd44dcU), LEV(0x4543U),
    LEV(0x456U), LEV(0x5663677U),
    LEV(0x3cd90cU), LEV(0x3958a85U),
    LEV(0x458c98d9U), LEV(0x344dbd9U),
    LEV(0xd0d9d9dU), LEV(0x234455U),
    LEV(0x1U), LEV(0x55U), LEV(0x4565U)
};

static uint32_t disasmInput1Kernel1Inputs[20] =
{
    LEV(1113U), LEV(1113U), LEV(1113U), LEV(1113U),
    LEV(1113U), LEV(1113U), LEV(1113U), LEV(1113U),
    LEV(1113U), LEV(1113U), LEV(1113U), LEV(1114U),
    LEV(1113U), LEV(1113U), LEV(1113U), LEV(1113U),
    LEV(1113U), LEV(1113U), LEV(1113U), LEV(1113U)
};

static uint32_t disasmInput1Kernel1Outputs[17] =
{
    LEV(5U), LEV(6U), LEV(3U), LEV(13U), LEV(5U), LEV(117U), LEV(12U), LEV(3U),
    LEV(785U), LEV(46U), LEV(55U), LEV(5U), LEV(2U), LEV(3U), LEV(0U), LEV(0U), LEV(44U)
};

static uint32_t disasmInput1Kernel1EarlyExit[3] = { LEV(1355U), LEV(44U), LEV(444U) };

static uint32_t disasmInput1Kernel2EarlyExit[3] = { LEV(121U) };

static CALDataSegmentEntry disasmInput1Kernel1Float32Consts[6] =
{
    { LEV(0U), LEV(5U) }, { LEV(66U), LEV(57U) }, { LEV(67U), LEV(334U) },
    { LEV(1U), LEV(6U) }, { LEV(5U), LEV(86U) }, { LEV(2100U), LEV(466U) }
};

static CALSamplerMapEntry disasmInput1Kernel1InputSamplers[6] =
{
    { LEV(0U), LEV(5U) }, { LEV(66U), LEV(57U) }, { LEV(67U), LEV(334U) },
    { LEV(1U), LEV(6U) }, { LEV(5U), LEV(86U) }, { LEV(2100U), LEV(466U) }
};

static uint32_t disasmInput1Kernel1UavOpMask[1] = { LEV(4556U) };

/* AMD Input for disassembler for testing */
static AmdDisasmInput disasmInput1 =
{
    GPUDeviceType::SPOOKY,
    true,
    "This\nis my\300\x31 stupid\r\"driver", "-O ccc",
    sizeof(disasmInput1Global), disasmInput1Global,
    {
        { "kernelxVCR",
          ::strlen(disasmInput1Kernel1Metadata),
          disasmInput1Kernel1Metadata,
          sizeof(disasmInput1Kernel1Header), disasmInput1Kernel1Header,
          {
              { { 8, 52, CALNOTE_ATI_PROGINFO,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1ProgInfo) },
              { { 8, 79, CALNOTE_ATI_INPUTS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1Inputs) },
              { { 8, 67, CALNOTE_ATI_OUTPUTS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1Outputs) },
              { { 8, 12, CALNOTE_ATI_EARLYEXIT,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1EarlyExit) },
              { { 8, 4, CALNOTE_ATI_EARLYEXIT,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel2EarlyExit) },
              { { 8, 48, CALNOTE_ATI_FLOAT32CONSTS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1Float32Consts) },
              { { 8, 48, CALNOTE_ATI_INPUT_SAMPLERS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1InputSamplers) },
              { { 8, 48, CALNOTE_ATI_CONSTANT_BUFFERS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1InputSamplers) },
              { { 8, 48, 0x3d, { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1InputSamplers) },
              { { 8, 4, CALNOTE_ATI_UAV_OP_MASK,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1UavOpMask) },
          },
          sizeof(disasmInput1Kernel1Data), disasmInput1Kernel1Data,
          0, nullptr
        }
    }
};

static const cxbyte galliumInput1Global[14] =
{ 1,2,3,4,5,6,7,8,9,10,11,33,44,55 };

/* Gallium Input for disassembler for testing */
static const GalliumDisasmInput galliumDisasmData =
{
    GPUDeviceType::PITCAIRN, false, false, false, false,
    sizeof(galliumInput1Global), galliumInput1Global,
    {
        { "kernel1",
          { { 0xff1123, 0xcda1fa }, { 0x3bf1123, 0xfdca45 }, { 0x2dca6, 0xfbca168 } },
          0, {
              { GalliumArgType::SCALAR, false, GalliumArgSemantic::GENERAL, 4, 8, 12 },
              { GalliumArgType::CONSTANT, false, GalliumArgSemantic::GENERAL, 1, 8, 12 },
              { GalliumArgType::GLOBAL, false, GalliumArgSemantic::GENERAL, 4, 2, 12 },
              { GalliumArgType::IMAGE2D_RDONLY, false,
                  GalliumArgSemantic::GENERAL, 4, 7, 45 },
              { GalliumArgType::IMAGE2D_WRONLY, false,
                  GalliumArgSemantic::GENERAL, 4, 8, 45 },
              { GalliumArgType::IMAGE3D_RDONLY, false,
                  GalliumArgSemantic::GENERAL, 9, 7, 45 },
              { GalliumArgType::IMAGE3D_WRONLY, false,
                  GalliumArgSemantic::GENERAL, 12, 7, 45 },
              { GalliumArgType::SAMPLER, false, GalliumArgSemantic::GENERAL, 17, 16, 74 },
              { GalliumArgType::SCALAR, true, GalliumArgSemantic::GENERAL, 4, 8, 12 },
              { GalliumArgType::SCALAR, false,
                  GalliumArgSemantic::GRID_DIMENSION, 124, 8, 12 },
              { GalliumArgType::SCALAR, false,
                  GalliumArgSemantic::GRID_OFFSET, 4, 0, 32 }
          } },
        { "kernel2",
          { { 0x234423, 0xaaabba }, { 0x3bdd123, 0x132235 }, { 0x11122, 0xf3424dd } },
          0, {
              { GalliumArgType::CONSTANT, false, GalliumArgSemantic::GENERAL, 1, 8, 12 },
              { GalliumArgType::GLOBAL, false, GalliumArgSemantic::GENERAL, 4, 2, 12 },
              { GalliumArgType::SCALAR, false, GalliumArgSemantic::GENERAL, 4, 8, 12 },
              { GalliumArgType::IMAGE2D_RDONLY, false,
                  GalliumArgSemantic::GENERAL, 4, 9, 45 },
              { GalliumArgType::IMAGE2D_WRONLY, false,
                  GalliumArgSemantic::GENERAL, 4, 8, 51 },
              { GalliumArgType::IMAGE3D_RDONLY, false,
                  GalliumArgSemantic::GENERAL, 9, 7, 45 },
              { GalliumArgType::IMAGE3D_WRONLY, true,
                  GalliumArgSemantic::GENERAL, 12, 7, 45 },
              { GalliumArgType::SAMPLER, false, GalliumArgSemantic::GENERAL, 17, 16, 74 },
              { GalliumArgType::SCALAR, true, GalliumArgSemantic::GENERAL, 4, 8, 12 },
              { GalliumArgType::SCALAR, false,
                  GalliumArgSemantic::GRID_DIMENSION, 124, 8, 12 },
              { GalliumArgType::SCALAR, false,
                  GalliumArgSemantic::GRID_OFFSET, 4, 32, 32 }
          } }
    },
    0, nullptr
};

struct DisasmAmdTestCase
{
    const AmdDisasmInput* amdInput;
    const GalliumDisasmInput* galliumInput;
    const char* filename;
    const char* expectedString;
    bool config;
    bool hsaConfig;
    cxuint llvmVersion;
    const char* exceptionString;
};

// disasm testcases
static const DisasmAmdTestCase disasmDataTestCases[] =
{
    /* 0 - */
    { &disasmInput1, nullptr, nullptr,
        R"xxFxx(.amd
.gpu Spooky
.64bit
.compile_options "-O ccc"
.driver_info "This\nis my\3001 stupid\r\"driver"
.globaldata
    .byte 0x0d, 0x38, 0x42, 0xd5, 0x37, 0x5d, 0x7b, 0x55
    .byte 0xa4, 0xea, 0x15, 0x25, 0x2c, 0xbc
.kernel kernelxVCR
    .header
        .byte 0x01, 0x02, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00
        .byte 0x0c, 0x03, 0x06, 0x03, 0x03, 0x02, 0x00, 0x00
        .fill 16, 1, 0x13
        .byte 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x12
        .fill 19, 1, 0x13
    .metadata
        .ascii ";ARGSTART:__OpenCL_DCT_kernel\n"
        .ascii ";version:3:1:111\n"
        .ascii ";device:pitcairn\n"
        .ascii ";uniqueid:1024\n"
        .ascii ";memory:uavprivate:0\n"
        .ascii ";memory:hwlocal:0\n"
        .ascii ";memory:hwregion:0\n"
        .ascii ";pointer:output:float:1:1:0:uav:12:4:RW:0:0\n"
        .ascii ";pointer:input:float:1:1:16:uav:13:4:RO:0:0\n"
        .ascii ";pointer:dct8x8:float:1:1:32:uav:14:4:RO:0:0\n"
        .ascii ";pointer:inter:float:1:1:48:hl:1:4:RW:0:0\n"
        .ascii ";value:width:u32:1:1:64\n"
        .ascii ";value:blockWidth:u32:1:1:80\n"
        .ascii ";value:inverse:u32:1:1:96\n"
        .ascii ";function:1:1030\n"
        .ascii ";uavid:11\n"
        .ascii ";printfid:9\n"
        .ascii ";cbid:10\n"
        .ascii ";privateid:8\n"
        .ascii ";reflection:0:float*\n"
        .ascii ";reflection:1:float*\n"
        .ascii ";reflection:2:float*\n"
        .ascii ";reflection:3:float*\n"
        .ascii ";reflection:4:uint\n"
        .ascii ";reflection:5:uint\n"
        .ascii ";reflection:6:uint\n"
        .ascii ";ARGEND:__OpenCL_DCT_kernel\n"
        .ascii "dessss9843re88888888888888888uuuuuuuufdddessss9843re88888888888888888uuu"
        .ascii "uuuuufdddessss9843re88888888888888888uuuuuuuufdd444444444444444444444444"
        .ascii "\r\r\t\t\t\t\txx\f\f\f\3777x833334441\n"
    .data
        .byte 0x01, 0x04, 0x05, 0x06, 0x06, 0x06, 0x04, 0x03
        .byte 0x03, 0x05, 0x41, 0x0d, 0x05, 0x05, 0x0b, 0x39
        .byte 0x05, 0x41, 0x05, 0x00, 0x00, 0x00, 0x00, 0x05
        .byte 0x01, 0x0f, 0x05, 0x01, 0x38, 0x00, 0x00, 0x05
        .byte 0x05, 0x41, 0x05, 0x00, 0x00, 0x4e, 0xff, 0x05
    .proginfo
        .entry 0xffcd44dc, 0x00004543
        .entry 0x00000456, 0x05663677
        .entry 0x003cd90c, 0x03958a85
        .entry 0x458c98d9, 0x0344dbd9
        .entry 0x0d0d9d9d, 0x00234455
        .entry 0x00000001, 0x00000055
        .byte 0x65, 0x45, 0x00, 0x00
    .inputs
        .fill 8, 4, 0x00000459
        .int 0x00000459, 0x00000459, 0x00000459, 0x0000045a
        .fill 7, 4, 0x00000459
        .byte 0x59, 0x04, 0x00
    .outputs
        .int 0x00000005, 0x00000006, 0x00000003, 0x0000000d
        .int 0x00000005, 0x00000075, 0x0000000c, 0x00000003
        .int 0x00000311, 0x0000002e, 0x00000037, 0x00000005
        .int 0x00000002, 0x00000003, 0x00000000, 0x00000000
        .byte 0x2c, 0x00, 0x00
    .earlyexit
        .byte 0x4b, 0x05, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00
        .byte 0xbc, 0x01, 0x00, 0x00
    .earlyexit 121
    .floatconsts
        .segment 0, 5
        .segment 66, 57
        .segment 67, 334
        .segment 1, 6
        .segment 5, 86
        .segment 2100, 466
    .inputsamplers
        .sampler 0, 0x5
        .sampler 66, 0x39
        .sampler 67, 0x14e
        .sampler 1, 0x6
        .sampler 5, 0x56
        .sampler 2100, 0x1d2
    .constantbuffers
        .cbmask 0, 5
        .cbmask 66, 57
        .cbmask 67, 334
        .cbmask 1, 6
        .cbmask 5, 86
        .cbmask 2100, 466
    .calnote 0x3d
        .byte 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00
        .byte 0x42, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00
        .byte 0x43, 0x00, 0x00, 0x00, 0x4e, 0x01, 0x00, 0x00
        .byte 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00
        .byte 0x05, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00
        .byte 0x34, 0x08, 0x00, 0x00, 0xd2, 0x01, 0x00, 0x00
    .uavopmask 4556
)xxFxx", false, false
    },
    /* 1 - */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/samplekernels.clo",
        R"xxFxx(.amd
.gpu Pitcairn
.32bit
.compile_options ""
.driver_info "@(#) OpenCL 1.2 AMD-APP (1702.3).  Driver version: 1702.3 (VM)"
.kernel add
    .header
        .fill 16, 1, 0x00
        .byte 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        .fill 8, 1, 0x00
    .metadata
        .ascii ";ARGSTART:__OpenCL_add_kernel\n"
        .ascii ";version:3:1:111\n"
        .ascii ";device:pitcairn\n"
        .ascii ";uniqueid:1024\n"
        .ascii ";memory:uavprivate:0\n"
        .ascii ";memory:hwlocal:0\n"
        .ascii ";memory:hwregion:0\n"
        .ascii ";value:n:u32:1:1:0\n"
        .ascii ";pointer:adat:u32:1:1:16:uav:12:4:RO:0:0\n"
        .ascii ";constarg:1:adat\n"
        .ascii ";pointer:bdat:u32:1:1:32:uav:13:4:RO:0:0\n"
        .ascii ";constarg:2:bdat\n"
        .ascii ";pointer:cdat:u32:1:1:48:uav:14:4:RW:0:0\n"
        .ascii ";function:1:1028\n"
        .ascii ";uavid:11\n"
        .ascii ";printfid:9\n"
        .ascii ";cbid:10\n"
        .ascii ";privateid:8\n"
        .ascii ";reflection:0:uint\n"
        .ascii ";reflection:1:uint*\n"
        .ascii ";reflection:2:uint*\n"
        .ascii ";reflection:3:uint*\n"
        .ascii ";ARGEND:__OpenCL_add_kernel\n"
    .data
        .fill 4736, 1, 0x00
    .inputs
    .outputs
    .uav
        .entry 12, 4, 0, 5
        .entry 13, 4, 0, 5
        .entry 14, 4, 0, 5
        .entry 11, 4, 0, 5
    .condout 0
    .floatconsts
    .intconsts
    .boolconsts
    .earlyexit 0
    .globalbuffers
    .constantbuffers
        .cbmask 0, 0
        .cbmask 1, 0
    .inputsamplers
    .scratchbuffers
        .int 0x00000000
    .persistentbuffers
    .proginfo
        .entry 0x80001000, 0x00000003
        .entry 0x80001001, 0x00000017
        .entry 0x80001002, 0x00000000
        .entry 0x80001003, 0x00000002
        .entry 0x80001004, 0x00000002
        .entry 0x80001005, 0x00000002
        .entry 0x80001006, 0x00000000
        .entry 0x80001007, 0x00000004
        .entry 0x80001008, 0x00000004
        .entry 0x80001009, 0x00000002
        .entry 0x8000100a, 0x00000001
        .entry 0x8000100b, 0x00000008
        .entry 0x8000100c, 0x00000004
        .entry 0x80001041, 0x00000003
        .entry 0x80001042, 0x00000010
        .entry 0x80001863, 0x00000066
        .entry 0x80001864, 0x00000100
        .entry 0x80001043, 0x000000c0
        .entry 0x80001044, 0x00000000
        .entry 0x80001045, 0x00000000
        .entry 0x00002e13, 0x00000098
        .entry 0x8000001c, 0x00000100
        .entry 0x8000001d, 0x00000000
        .entry 0x8000001e, 0x00000000
        .entry 0x80001841, 0x00000000
        .entry 0x8000001f, 0x00007000
        .entry 0x80001843, 0x00007000
        .entry 0x80001844, 0x00000000
        .entry 0x80001845, 0x00000000
        .entry 0x80001846, 0x00000000
        .entry 0x80001847, 0x00000000
        .entry 0x80001848, 0x00000000
        .entry 0x80001849, 0x00000000
        .entry 0x8000184a, 0x00000000
        .entry 0x8000184b, 0x00000000
        .entry 0x8000184c, 0x00000000
        .entry 0x8000184d, 0x00000000
        .entry 0x8000184e, 0x00000000
        .entry 0x8000184f, 0x00000000
        .entry 0x80001850, 0x00000000
        .entry 0x80001851, 0x00000000
        .entry 0x80001852, 0x00000000
        .entry 0x80001853, 0x00000000
        .entry 0x80001854, 0x00000000
        .entry 0x80001855, 0x00000000
        .entry 0x80001856, 0x00000000
        .entry 0x80001857, 0x00000000
        .entry 0x80001858, 0x00000000
        .entry 0x80001859, 0x00000000
        .entry 0x8000185a, 0x00000000
        .entry 0x8000185b, 0x00000000
        .entry 0x8000185c, 0x00000000
        .entry 0x8000185d, 0x00000000
        .entry 0x8000185e, 0x00000000
        .entry 0x8000185f, 0x00000000
        .entry 0x80001860, 0x00000000
        .entry 0x80001861, 0x00000000
        .entry 0x80001862, 0x00000000
        .entry 0x8000000a, 0x00000001
        .entry 0x80000078, 0x00000040
        .entry 0x80000081, 0x00008000
        .entry 0x80000082, 0x00000000
    .subconstantbuffers
    .uavmailboxsize 0
    .uavopmask
        .byte 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 120, 1, 0x00
    .text
/*c2000504         */ s_buffer_load_dword s0, s[4:7], 0x4
/*c2008518         */ s_buffer_load_dword s1, s[4:7], 0x18
/*c2020900         */ s_buffer_load_dword s4, s[8:11], 0x0
/*c2028904         */ s_buffer_load_dword s5, s[8:11], 0x4
/*c2030908         */ s_buffer_load_dword s6, s[8:11], 0x8
/*c203890c         */ s_buffer_load_dword s7, s[8:11], 0xc
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8380ff00 0000ffff*/ s_min_u32       s0, s0, 0xffff
/*9300000c         */ s_mul_i32       s0, s12, s0
/*80000100         */ s_add_u32       s0, s0, s1
/*4a000000         */ v_add_i32       v0, vcc, s0, v0
/*7d880004         */ v_cmp_gt_u32    vcc, s4, v0
/*be80246a         */ s_and_saveexec_b64 s[0:1], vcc
/*bf880011         */ s_cbranch_execz .L128_0
/*c0840360         */ s_load_dwordx4  s[8:11], s[2:3], 0x60
/*c0860368         */ s_load_dwordx4  s[12:15], s[2:3], 0x68
/*34000082         */ v_lshlrev_b32   v0, 2, v0
/*4a020005         */ v_add_i32       v1, vcc, s5, v0
/*4a040006         */ v_add_i32       v2, vcc, s6, v0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*eba01000 80020101*/ tbuffer_load_format_x v1, v1, s[8:11], 0 offen format:[32,float]
/*eba01000 80030202*/ tbuffer_load_format_x v2, v2, s[12:15], 0 offen format:[32,float]
/*c0840370         */ s_load_dwordx4  s[8:11], s[2:3], 0x70
/*bf8c0f70         */ s_waitcnt       vmcnt(0)
/*4a020501         */ v_add_i32       v1, vcc, v1, v2
/*4a000007         */ v_add_i32       v0, vcc, s7, v0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*eba41000 80020100*/ tbuffer_store_format_x v1, v0, s[8:11], 0 offen format:[32,float]
.L128_0:
/*bf810000         */ s_endpgm
.kernel multiply
    .header
        .fill 16, 1, 0x00
        .byte 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        .fill 8, 1, 0x00
    .metadata
        .ascii ";ARGSTART:__OpenCL_multiply_kernel\n"
        .ascii ";version:3:1:111\n"
        .ascii ";device:pitcairn\n"
        .ascii ";uniqueid:1025\n"
        .ascii ";memory:uavprivate:0\n"
        .ascii ";memory:hwlocal:0\n"
        .ascii ";memory:hwregion:0\n"
        .ascii ";value:n:u32:1:1:0\n"
        .ascii ";pointer:adat:u32:1:1:16:uav:12:4:RO:0:0\n"
        .ascii ";constarg:1:adat\n"
        .ascii ";pointer:bdat:u32:1:1:32:uav:13:4:RO:0:0\n"
        .ascii ";constarg:2:bdat\n"
        .ascii ";pointer:cdat:u32:1:1:48:uav:14:4:RW:0:0\n"
        .ascii ";function:1:1029\n"
        .ascii ";uavid:11\n"
        .ascii ";printfid:9\n"
        .ascii ";cbid:10\n"
        .ascii ";privateid:8\n"
        .ascii ";reflection:0:uint\n"
        .ascii ";reflection:1:uint*\n"
        .ascii ";reflection:2:uint*\n"
        .ascii ";reflection:3:uint*\n"
        .ascii ";ARGEND:__OpenCL_multiply_kernel\n"
    .data
        .fill 4736, 1, 0x00
    .inputs
    .outputs
    .uav
        .entry 12, 4, 0, 5
        .entry 13, 4, 0, 5
        .entry 14, 4, 0, 5
        .entry 11, 4, 0, 5
    .condout 0
    .floatconsts
    .intconsts
    .boolconsts
    .earlyexit 0
    .globalbuffers
    .constantbuffers
        .cbmask 0, 0
        .cbmask 1, 0
    .inputsamplers
    .scratchbuffers
        .int 0x00000000
    .persistentbuffers
    .proginfo
        .entry 0x80001000, 0x00000003
        .entry 0x80001001, 0x00000017
        .entry 0x80001002, 0x00000000
        .entry 0x80001003, 0x00000002
        .entry 0x80001004, 0x00000002
        .entry 0x80001005, 0x00000002
        .entry 0x80001006, 0x00000000
        .entry 0x80001007, 0x00000004
        .entry 0x80001008, 0x00000004
        .entry 0x80001009, 0x00000002
        .entry 0x8000100a, 0x00000001
        .entry 0x8000100b, 0x00000008
        .entry 0x8000100c, 0x00000004
        .entry 0x80001041, 0x00000003
        .entry 0x80001042, 0x00000014
        .entry 0x80001863, 0x00000066
        .entry 0x80001864, 0x00000100
        .entry 0x80001043, 0x000000c0
        .entry 0x80001044, 0x00000000
        .entry 0x80001045, 0x00000000
        .entry 0x00002e13, 0x00000098
        .entry 0x8000001c, 0x00000100
        .entry 0x8000001d, 0x00000000
        .entry 0x8000001e, 0x00000000
        .entry 0x80001841, 0x00000000
        .entry 0x8000001f, 0x00007000
        .entry 0x80001843, 0x00007000
        .entry 0x80001844, 0x00000000
        .entry 0x80001845, 0x00000000
        .entry 0x80001846, 0x00000000
        .entry 0x80001847, 0x00000000
        .entry 0x80001848, 0x00000000
        .entry 0x80001849, 0x00000000
        .entry 0x8000184a, 0x00000000
        .entry 0x8000184b, 0x00000000
        .entry 0x8000184c, 0x00000000
        .entry 0x8000184d, 0x00000000
        .entry 0x8000184e, 0x00000000
        .entry 0x8000184f, 0x00000000
        .entry 0x80001850, 0x00000000
        .entry 0x80001851, 0x00000000
        .entry 0x80001852, 0x00000000
        .entry 0x80001853, 0x00000000
        .entry 0x80001854, 0x00000000
        .entry 0x80001855, 0x00000000
        .entry 0x80001856, 0x00000000
        .entry 0x80001857, 0x00000000
        .entry 0x80001858, 0x00000000
        .entry 0x80001859, 0x00000000
        .entry 0x8000185a, 0x00000000
        .entry 0x8000185b, 0x00000000
        .entry 0x8000185c, 0x00000000
        .entry 0x8000185d, 0x00000000
        .entry 0x8000185e, 0x00000000
        .entry 0x8000185f, 0x00000000
        .entry 0x80001860, 0x00000000
        .entry 0x80001861, 0x00000000
        .entry 0x80001862, 0x00000000
        .entry 0x8000000a, 0x00000001
        .entry 0x80000078, 0x00000040
        .entry 0x80000081, 0x00008000
        .entry 0x80000082, 0x00000000
    .subconstantbuffers
    .uavmailboxsize 0
    .uavopmask
        .byte 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 120, 1, 0x00
    .text
/*c2000504         */ s_buffer_load_dword s0, s[4:7], 0x4
/*c2008518         */ s_buffer_load_dword s1, s[4:7], 0x18
/*c2020900         */ s_buffer_load_dword s4, s[8:11], 0x0
/*c2028904         */ s_buffer_load_dword s5, s[8:11], 0x4
/*c2030908         */ s_buffer_load_dword s6, s[8:11], 0x8
/*c203890c         */ s_buffer_load_dword s7, s[8:11], 0xc
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8380ff00 0000ffff*/ s_min_u32       s0, s0, 0xffff
/*9300000c         */ s_mul_i32       s0, s12, s0
/*80000100         */ s_add_u32       s0, s0, s1
/*4a000000         */ v_add_i32       v0, vcc, s0, v0
/*7d880004         */ v_cmp_gt_u32    vcc, s4, v0
/*be80246a         */ s_and_saveexec_b64 s[0:1], vcc
/*bf880011         */ s_cbranch_execz .L128_1
/*c0840360         */ s_load_dwordx4  s[8:11], s[2:3], 0x60
/*c0860368         */ s_load_dwordx4  s[12:15], s[2:3], 0x68
/*c0880370         */ s_load_dwordx4  s[16:19], s[2:3], 0x70
/*34000082         */ v_lshlrev_b32   v0, 2, v0
/*4a020005         */ v_add_i32       v1, vcc, s5, v0
/*4a040006         */ v_add_i32       v2, vcc, s6, v0
/*4a000007         */ v_add_i32       v0, vcc, s7, v0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*eba01000 80020101*/ tbuffer_load_format_x v1, v1, s[8:11], 0 offen format:[32,float]
/*eba01000 80030202*/ tbuffer_load_format_x v2, v2, s[12:15], 0 offen format:[32,float]
/*bf8c0f70         */ s_waitcnt       vmcnt(0)
/*d2d60001 00020302*/ v_mul_lo_i32    v1, v2, v1
/*eba41000 80040100*/ tbuffer_store_format_x v1, v0, s[16:19], 0 offen format:[32,float]
.L128_1:
/*bf810000         */ s_endpgm
)xxFxx", false, false
    },
    /* 2 - */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/samplekernels_64.clo",
        R"xxFxx(.amd
.gpu Pitcairn
.64bit
.compile_options ""
.driver_info "@(#) OpenCL 1.2 AMD-APP (1702.3).  Driver version: 1702.3 (VM)"
.kernel add
    .header
        .fill 16, 1, 0x00
        .byte 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        .fill 8, 1, 0x00
    .metadata
        .ascii ";ARGSTART:__OpenCL_add_kernel\n"
        .ascii ";version:3:1:111\n"
        .ascii ";device:pitcairn\n"
        .ascii ";uniqueid:1024\n"
        .ascii ";memory:uavprivate:0\n"
        .ascii ";memory:hwlocal:0\n"
        .ascii ";memory:hwregion:0\n"
        .ascii ";value:n:u32:1:1:0\n"
        .ascii ";pointer:adat:u32:1:1:16:uav:12:4:RO:0:0\n"
        .ascii ";constarg:1:adat\n"
        .ascii ";pointer:bdat:u32:1:1:32:uav:13:4:RO:0:0\n"
        .ascii ";constarg:2:bdat\n"
        .ascii ";pointer:cdat:u32:1:1:48:uav:14:4:RW:0:0\n"
        .ascii ";function:1:1028\n"
        .ascii ";memory:64bitABI\n"
        .ascii ";uavid:11\n"
        .ascii ";printfid:9\n"
        .ascii ";cbid:10\n"
        .ascii ";privateid:8\n"
        .ascii ";reflection:0:uint\n"
        .ascii ";reflection:1:uint*\n"
        .ascii ";reflection:2:uint*\n"
        .ascii ";reflection:3:uint*\n"
        .ascii ";ARGEND:__OpenCL_add_kernel\n"
    .data
        .fill 4736, 1, 0x00
    .inputs
    .outputs
    .uav
        .entry 12, 4, 0, 5
        .entry 13, 4, 0, 5
        .entry 14, 4, 0, 5
        .entry 11, 4, 0, 5
    .condout 0
    .floatconsts
    .intconsts
    .boolconsts
    .earlyexit 0
    .globalbuffers
    .constantbuffers
        .cbmask 0, 0
        .cbmask 1, 0
    .inputsamplers
    .scratchbuffers
        .int 0x00000000
    .persistentbuffers
    .proginfo
        .entry 0x80001000, 0x00000003
        .entry 0x80001001, 0x00000017
        .entry 0x80001002, 0x00000000
        .entry 0x80001003, 0x00000002
        .entry 0x80001004, 0x00000002
        .entry 0x80001005, 0x00000002
        .entry 0x80001006, 0x00000000
        .entry 0x80001007, 0x00000004
        .entry 0x80001008, 0x00000004
        .entry 0x80001009, 0x00000002
        .entry 0x8000100a, 0x00000001
        .entry 0x8000100b, 0x00000008
        .entry 0x8000100c, 0x00000004
        .entry 0x80001041, 0x00000008
        .entry 0x80001042, 0x00000014
        .entry 0x80001863, 0x00000066
        .entry 0x80001864, 0x00000100
        .entry 0x80001043, 0x000000c0
        .entry 0x80001044, 0x00000000
        .entry 0x80001045, 0x00000000
        .entry 0x00002e13, 0x00000098
        .entry 0x8000001c, 0x00000100
        .entry 0x8000001d, 0x00000000
        .entry 0x8000001e, 0x00000000
        .entry 0x80001841, 0x00000000
        .entry 0x8000001f, 0x00007000
        .entry 0x80001843, 0x00007000
        .entry 0x80001844, 0x00000000
        .entry 0x80001845, 0x00000000
        .entry 0x80001846, 0x00000000
        .entry 0x80001847, 0x00000000
        .entry 0x80001848, 0x00000000
        .entry 0x80001849, 0x00000000
        .entry 0x8000184a, 0x00000000
        .entry 0x8000184b, 0x00000000
        .entry 0x8000184c, 0x00000000
        .entry 0x8000184d, 0x00000000
        .entry 0x8000184e, 0x00000000
        .entry 0x8000184f, 0x00000000
        .entry 0x80001850, 0x00000000
        .entry 0x80001851, 0x00000000
        .entry 0x80001852, 0x00000000
        .entry 0x80001853, 0x00000000
        .entry 0x80001854, 0x00000000
        .entry 0x80001855, 0x00000000
        .entry 0x80001856, 0x00000000
        .entry 0x80001857, 0x00000000
        .entry 0x80001858, 0x00000000
        .entry 0x80001859, 0x00000000
        .entry 0x8000185a, 0x00000000
        .entry 0x8000185b, 0x00000000
        .entry 0x8000185c, 0x00000000
        .entry 0x8000185d, 0x00000000
        .entry 0x8000185e, 0x00000000
        .entry 0x8000185f, 0x00000000
        .entry 0x80001860, 0x00000000
        .entry 0x80001861, 0x00000000
        .entry 0x80001862, 0x00000000
        .entry 0x8000000a, 0x00000001
        .entry 0x80000078, 0x00000040
        .entry 0x80000081, 0x00008000
        .entry 0x80000082, 0x00000000
    .subconstantbuffers
    .uavmailboxsize 0
    .uavopmask
        .byte 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 120, 1, 0x00
    .text
/*c2000504         */ s_buffer_load_dword s0, s[4:7], 0x4
/*c2008518         */ s_buffer_load_dword s1, s[4:7], 0x18
/*c2420904         */ s_buffer_load_dwordx2 s[4:5], s[8:11], 0x4
/*c2430908         */ s_buffer_load_dwordx2 s[6:7], s[8:11], 0x8
/*c247090c         */ s_buffer_load_dwordx2 s[14:15], s[8:11], 0xc
/*c2040900         */ s_buffer_load_dword s8, s[8:11], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8380ff00 0000ffff*/ s_min_u32       s0, s0, 0xffff
/*9300000c         */ s_mul_i32       s0, s12, s0
/*80000100         */ s_add_u32       s0, s0, s1
/*4a000000         */ v_add_i32       v0, vcc, s0, v0
/*7e020280         */ v_mov_b32       v1, 0
/*7e040208         */ v_mov_b32       v2, s8
/*7e060280         */ v_mov_b32       v3, 0
/*7dc20500         */ v_cmp_lt_u64    vcc, v[0:1], v[2:3]
/*be80246a         */ s_and_saveexec_b64 s[0:1], vcc
/*bf880018         */ s_cbranch_execz .L168_0
/*c0840360         */ s_load_dwordx4  s[8:11], s[2:3], 0x60
/*c0880368         */ s_load_dwordx4  s[16:19], s[2:3], 0x68
/*d2c20000 00010500*/ v_lshl_b64      v[0:1], v[0:1], 2
/*4a040004         */ v_add_i32       v2, vcc, s4, v0
/*7e060205         */ v_mov_b32       v3, s5
/*50060303         */ v_addc_u32      v3, vcc, v3, v1, vcc
/*4a0c0006         */ v_add_i32       v6, vcc, s6, v0
/*7e0a0207         */ v_mov_b32       v5, s7
/*500e0305         */ v_addc_u32      v7, vcc, v5, v1, vcc
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*eba08000 80020202*/ tbuffer_load_format_x v2, v[2:3], s[8:11], 0 addr64 format:[32,float]
/*eba08000 80040306*/ tbuffer_load_format_x v3, v[6:7], s[16:19], 0 addr64 format:[32,float]
/*c0820370         */ s_load_dwordx4  s[4:7], s[2:3], 0x70
/*bf8c0f70         */ s_waitcnt       vmcnt(0)
/*4a040702         */ v_add_i32       v2, vcc, v2, v3
/*4a00000e         */ v_add_i32       v0, vcc, s14, v0
/*7e06020f         */ v_mov_b32       v3, s15
/*50020303         */ v_addc_u32      v1, vcc, v3, v1, vcc
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*eba48000 80010200*/ tbuffer_store_format_x v2, v[0:1], s[4:7], 0 addr64 format:[32,float]
.L168_0:
/*bf810000         */ s_endpgm
.kernel multiply
    .header
        .fill 16, 1, 0x00
        .byte 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
        .fill 8, 1, 0x00
    .metadata
        .ascii ";ARGSTART:__OpenCL_multiply_kernel\n"
        .ascii ";version:3:1:111\n"
        .ascii ";device:pitcairn\n"
        .ascii ";uniqueid:1025\n"
        .ascii ";memory:uavprivate:0\n"
        .ascii ";memory:hwlocal:0\n"
        .ascii ";memory:hwregion:0\n"
        .ascii ";value:n:u32:1:1:0\n"
        .ascii ";pointer:adat:u32:1:1:16:uav:12:4:RO:0:0\n"
        .ascii ";constarg:1:adat\n"
        .ascii ";pointer:bdat:u32:1:1:32:uav:13:4:RO:0:0\n"
        .ascii ";constarg:2:bdat\n"
        .ascii ";pointer:cdat:u32:1:1:48:uav:14:4:RW:0:0\n"
        .ascii ";function:1:1029\n"
        .ascii ";memory:64bitABI\n"
        .ascii ";uavid:11\n"
        .ascii ";printfid:9\n"
        .ascii ";cbid:10\n"
        .ascii ";privateid:8\n"
        .ascii ";reflection:0:uint\n"
        .ascii ";reflection:1:uint*\n"
        .ascii ";reflection:2:uint*\n"
        .ascii ";reflection:3:uint*\n"
        .ascii ";ARGEND:__OpenCL_multiply_kernel\n"
    .data
        .fill 4736, 1, 0x00
    .inputs
    .outputs
    .uav
        .entry 12, 4, 0, 5
        .entry 13, 4, 0, 5
        .entry 14, 4, 0, 5
        .entry 11, 4, 0, 5
    .condout 0
    .floatconsts
    .intconsts
    .boolconsts
    .earlyexit 0
    .globalbuffers
    .constantbuffers
        .cbmask 0, 0
        .cbmask 1, 0
    .inputsamplers
    .scratchbuffers
        .int 0x00000000
    .persistentbuffers
    .proginfo
        .entry 0x80001000, 0x00000003
        .entry 0x80001001, 0x00000017
        .entry 0x80001002, 0x00000000
        .entry 0x80001003, 0x00000002
        .entry 0x80001004, 0x00000002
        .entry 0x80001005, 0x00000002
        .entry 0x80001006, 0x00000000
        .entry 0x80001007, 0x00000004
        .entry 0x80001008, 0x00000004
        .entry 0x80001009, 0x00000002
        .entry 0x8000100a, 0x00000001
        .entry 0x8000100b, 0x00000008
        .entry 0x8000100c, 0x00000004
        .entry 0x80001041, 0x00000008
        .entry 0x80001042, 0x00000014
        .entry 0x80001863, 0x00000066
        .entry 0x80001864, 0x00000100
        .entry 0x80001043, 0x000000c0
        .entry 0x80001044, 0x00000000
        .entry 0x80001045, 0x00000000
        .entry 0x00002e13, 0x00000098
        .entry 0x8000001c, 0x00000100
        .entry 0x8000001d, 0x00000000
        .entry 0x8000001e, 0x00000000
        .entry 0x80001841, 0x00000000
        .entry 0x8000001f, 0x00007000
        .entry 0x80001843, 0x00007000
        .entry 0x80001844, 0x00000000
        .entry 0x80001845, 0x00000000
        .entry 0x80001846, 0x00000000
        .entry 0x80001847, 0x00000000
        .entry 0x80001848, 0x00000000
        .entry 0x80001849, 0x00000000
        .entry 0x8000184a, 0x00000000
        .entry 0x8000184b, 0x00000000
        .entry 0x8000184c, 0x00000000
        .entry 0x8000184d, 0x00000000
        .entry 0x8000184e, 0x00000000
        .entry 0x8000184f, 0x00000000
        .entry 0x80001850, 0x00000000
        .entry 0x80001851, 0x00000000
        .entry 0x80001852, 0x00000000
        .entry 0x80001853, 0x00000000
        .entry 0x80001854, 0x00000000
        .entry 0x80001855, 0x00000000
        .entry 0x80001856, 0x00000000
        .entry 0x80001857, 0x00000000
        .entry 0x80001858, 0x00000000
        .entry 0x80001859, 0x00000000
        .entry 0x8000185a, 0x00000000
        .entry 0x8000185b, 0x00000000
        .entry 0x8000185c, 0x00000000
        .entry 0x8000185d, 0x00000000
        .entry 0x8000185e, 0x00000000
        .entry 0x8000185f, 0x00000000
        .entry 0x80001860, 0x00000000
        .entry 0x80001861, 0x00000000
        .entry 0x80001862, 0x00000000
        .entry 0x8000000a, 0x00000001
        .entry 0x80000078, 0x00000040
        .entry 0x80000081, 0x00008000
        .entry 0x80000082, 0x00000000
    .subconstantbuffers
    .uavmailboxsize 0
    .uavopmask
        .byte 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        .fill 120, 1, 0x00
    .text
/*c2000504         */ s_buffer_load_dword s0, s[4:7], 0x4
/*c2008518         */ s_buffer_load_dword s1, s[4:7], 0x18
/*c2420904         */ s_buffer_load_dwordx2 s[4:5], s[8:11], 0x4
/*c2430908         */ s_buffer_load_dwordx2 s[6:7], s[8:11], 0x8
/*c247090c         */ s_buffer_load_dwordx2 s[14:15], s[8:11], 0xc
/*c2040900         */ s_buffer_load_dword s8, s[8:11], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8380ff00 0000ffff*/ s_min_u32       s0, s0, 0xffff
/*9300000c         */ s_mul_i32       s0, s12, s0
/*80000100         */ s_add_u32       s0, s0, s1
/*4a000000         */ v_add_i32       v0, vcc, s0, v0
/*7e020280         */ v_mov_b32       v1, 0
/*7e040208         */ v_mov_b32       v2, s8
/*7e060280         */ v_mov_b32       v3, 0
/*7dc20500         */ v_cmp_lt_u64    vcc, v[0:1], v[2:3]
/*be80246a         */ s_and_saveexec_b64 s[0:1], vcc
/*bf880019         */ s_cbranch_execz .L172_1
/*c0840360         */ s_load_dwordx4  s[8:11], s[2:3], 0x60
/*c0880368         */ s_load_dwordx4  s[16:19], s[2:3], 0x68
/*d2c20000 00010500*/ v_lshl_b64      v[0:1], v[0:1], 2
/*4a040004         */ v_add_i32       v2, vcc, s4, v0
/*7e060205         */ v_mov_b32       v3, s5
/*50060303         */ v_addc_u32      v3, vcc, v3, v1, vcc
/*4a0c0006         */ v_add_i32       v6, vcc, s6, v0
/*7e0a0207         */ v_mov_b32       v5, s7
/*500e0305         */ v_addc_u32      v7, vcc, v5, v1, vcc
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*eba08000 80020202*/ tbuffer_load_format_x v2, v[2:3], s[8:11], 0 addr64 format:[32,float]
/*eba08000 80040306*/ tbuffer_load_format_x v3, v[6:7], s[16:19], 0 addr64 format:[32,float]
/*c0820370         */ s_load_dwordx4  s[4:7], s[2:3], 0x70
/*bf8c0f70         */ s_waitcnt       vmcnt(0)
/*d2d60002 00020503*/ v_mul_lo_i32    v2, v3, v2
/*4a00000e         */ v_add_i32       v0, vcc, s14, v0
/*7e06020f         */ v_mov_b32       v3, s15
/*50020303         */ v_addc_u32      v1, vcc, v3, v1, vcc
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*eba48000 80010200*/ tbuffer_store_format_x v2, v[0:1], s[4:7], 0 addr64 format:[32,float]
.L172_1:
/*bf810000         */ s_endpgm
)xxFxx", false, false
    },
    /* 3 - */
    { nullptr, &galliumDisasmData, nullptr,
        R"fxDfx(.gallium
.gpu Pitcairn
.32bit
.rodata
    .byte 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    .byte 0x09, 0x0a, 0x0b, 0x21, 0x2c, 0x37
.kernel kernel1
    .args
        .arg scalar, 4, 8, 12, zext, general
        .arg constant, 1, 8, 12, zext, general
        .arg global, 4, 2, 12, zext, general
        .arg image2d_rd, 4, 7, 45, zext, general
        .arg image2d_wr, 4, 8, 45, zext, general
        .arg image3d_rd, 9, 7, 45, zext, general
        .arg image3d_wr, 12, 7, 45, zext, general
        .arg sampler, 17, 16, 74, zext, general
        .arg scalar, 4, 8, 12, sext, general
        .arg scalar, 124, 8, 12, zext, griddim
        .arg scalar, 4, 0, 32, zext, gridoffset
    .proginfo
        .entry 0x00ff1123, 0x00cda1fa
        .entry 0x03bf1123, 0x00fdca45
        .entry 0x0002dca6, 0x0fbca168
.kernel kernel2
    .args
        .arg constant, 1, 8, 12, zext, general
        .arg global, 4, 2, 12, zext, general
        .arg scalar, 4, 8, 12, zext, general
        .arg image2d_rd, 4, 9, 45, zext, general
        .arg image2d_wr, 4, 8, 51, zext, general
        .arg image3d_rd, 9, 7, 45, zext, general
        .arg image3d_wr, 12, 7, 45, sext, general
        .arg sampler, 17, 16, 74, zext, general
        .arg scalar, 4, 8, 12, sext, general
        .arg scalar, 124, 8, 12, zext, griddim
        .arg scalar, 4, 32, 32, zext, gridoffset
    .proginfo
        .entry 0x00234423, 0x00aaabba
        .entry 0x03bdd123, 0x00132235
        .entry 0x00011122, 0x0f3424dd
)fxDfx", false, false, 0
    },
    /* 4 - configuration dump test cases */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/amd1.clo",
        R"ffDXD(.amd
.gpu Bonaire
.32bit
.compile_options "-O"
.driver_info "@(#) OpenCL 1.2 AMD-APP (1912.5).  Driver version: 1912.5 (VM)"
.kernel xT1
    .config
        .dims xyz
        .cws 2, 7, 1
        .sgprsnum 18
        .vgprsnum 222
        .floatmode 0xc0
        .scratchbuffer 108
        .uavid 11
        .uavprivate 314
        .printfid 9
        .privateid 8
        .cbid 10
        .earlyexit 1
        .condout 5
        .pgmrsrc2 0x7520139d
        .useprintf
        .exceptions 0x75
        .userdata ptr_uav_table, 0, 2, 2
        .userdata imm_const_buffer, 0, 4, 4
        .userdata imm_const_buffer, 7, 12, 2
        .arg x, "float", float
        .arg xff, "SP", float
        .arg x4, "float4", float
        .arg aaa, "SP4", float
        .arg vv, "double3", double
        .arg vv3, "SP4", double
        .arg sampler, "sampler_t", sampler
        .arg structor, "TypeX", structure, 24
        .arg structor2, "TypeX", structure, 24
        .arg img1, "IMG2D", image2d, read_only, 0
        .arg img1a, "IMG2DA", image2d_array, read_only, 1
        .arg img2, "TDIMG", image3d, write_only, 0
        .arg buf1, "uint*", uint*, global, const, 12
        .arg buf2, "ColorData", structure*, 52, global, const, 13
        .arg const2, "ColorData", structure*, 52, constant, volatile, 0, 14
        .arg dblloc, "double*", double*, local, 
        .arg counterx, "counter32_t", counter32, 0
        .arg sampler2, "sampler_t", sampler
        .arg countery, "counter32_t", counter32, 1
    .text
/*bf810000         */ s_endpgm
/*4a040501         */ v_add_i32       v2, vcc, v1, v2
)ffDXD", true, false
    },
    /* 5 - configuration dump test cases (diff dimensions in local_id and group_id */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/amd1-diffdims.clo",
        R"ffDXD(.amd
.gpu Bonaire
.32bit
.compile_options "-O"
.driver_info "@(#) OpenCL 1.2 AMD-APP (1912.5).  Driver version: 1912.5 (VM)"
.kernel xT1
    .config
        .dims xyz, xy
        .cws 2, 7, 1
        .sgprsnum 18
        .vgprsnum 222
        .floatmode 0xc0
        .scratchbuffer 108
        .uavid 11
        .uavprivate 314
        .printfid 9
        .privateid 8
        .cbid 10
        .earlyexit 1
        .condout 5
        .pgmrsrc2 0x75200b9d
        .useprintf
        .exceptions 0x75
        .userdata ptr_uav_table, 0, 2, 2
        .userdata imm_const_buffer, 0, 4, 4
        .userdata imm_const_buffer, 7, 12, 2
        .arg x, "float", float
        .arg xff, "SP", float
        .arg x4, "float4", float
        .arg aaa, "SP4", float
        .arg vv, "double3", double
        .arg vv3, "SP4", double
        .arg sampler, "sampler_t", sampler
        .arg structor, "TypeX", structure, 24
        .arg structor2, "TypeX", structure, 24
        .arg img1, "IMG2D", image2d, read_only, 0
        .arg img1a, "IMG2DA", image2d_array, read_only, 1
        .arg img2, "TDIMG", image3d, write_only, 0
        .arg buf1, "uint*", uint*, global, const, 12
        .arg buf2, "ColorData", structure*, 52, global, const, 13
        .arg const2, "ColorData", structure*, 52, constant, volatile, 0, 14
        .arg dblloc, "double*", double*, local, 
        .arg counterx, "counter32_t", counter32, 0
        .arg sampler2, "sampler_t", sampler
        .arg countery, "counter32_t", counter32, 1
    .text
/*bf810000         */ s_endpgm
/*4a040501         */ v_add_i32       v2, vcc, v1, v2
)ffDXD", true, false
    },
    /* 6 - amdcl2 config */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/amdcl2.clo",
        R"ffDXD(.amdcl2
.gpu Bonaire
.64bit
.arch_minor 0
.arch_stepping 0
.driver_version 191205
.compile_options ""
.acl_version "AMD-COMP-LIB-v0.8 (0.0.SC_BUILD_NUMBER)"
.kernel aaa1
    .config
        .dims x
        .sgprsnum 12
        .vgprsnum 1
        .localsize 1000
        .floatmode 0xda
        .pgmrsrc1 0x00ada040
        .pgmrsrc2 0x0000008c
        .dx10clamp
        .ieeemode
        .useargs
        .priority 0
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg n, "uint", uint
        .arg in, "uint*", uint*, global, const
        .arg out, "uint*", uint*, global, 
    .text
/*8709ac05         */ s_and_b32       s9, s5, 44
/*870a8505         */ s_and_b32       s10, s5, 5
/*870b8505         */ s_and_b32       s11, s5, 5
/*bf810000         */ s_endpgm
.kernel aaa2
    .config
        .dims x, xyz
        .sgprsnum 12
        .vgprsnum 1
        .localsize 1000
        .floatmode 0xda
        .scratchbuffer 2342
        .pgmrsrc1 0x00fda840
        .pgmrsrc2 0x12001095
        .privmode
        .debugmode
        .dx10clamp
        .ieeemode
        .exceptions 0x12
        .useargs
        .usesetup
        .useenqueue
        .priority 2
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg n, "uint", uint
        .arg in, "float*", float*, global, const
        .arg out, "float*", float*, global, 
        .arg q, "queue_t", queue
        .arg piper, "pipe", pipe, rdonly
        .arg ce, "clk_event_t", clkevent
    .text
/*8709ac05         */ s_and_b32       s9, s5, 44
/*bf810000         */ s_endpgm
.kernel gfd12
    .config
        .dims xyz
        .cws 8, 5, 2
        .sgprsnum 25
        .vgprsnum 78
        .localsize 656
        .floatmode 0xf4
        .pgmrsrc1 0x012f40d3
        .pgmrsrc2 0x0fcdf7d8
        .dx10clamp
        .tgsize
        .exceptions 0x0f
        .useargs
        .usesetup
        .usegeneric
        .priority 0
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg n, "uint", uint
        .arg in, "double*", double*, global, const
        .arg out, "double*", double*, global, 
        .arg v, "uchar", uchar
        .arg v2, "uchar", uchar
    .text
/*7e020302         */ v_mov_b32       v1, v2
/*4a0206b7         */ v_add_i32       v1, vcc, 55, v3
/*bf810000         */ s_endpgm
)ffDXD", true, false
    },
    /* 7 - amdcl2 config */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/amdcl2.clo",
        R"ffDXD(.amdcl2
.gpu Bonaire
.64bit
.arch_minor 0
.arch_stepping 0
.driver_version 191205
.compile_options ""
.acl_version "AMD-COMP-LIB-v0.8 (0.0.SC_BUILD_NUMBER)"
.kernel aaa1
    .hsaconfig
        .dims x
        .sgprsnum 16
        .vgprsnum 4
        .dx10clamp
        .ieeemode
        .floatmode 0xda
        .priority 0
        .userdatanum 6
        .pgmrsrc1 0x00ada040
        .pgmrsrc2 0x0000008c
        .codeversion 1, 1
        .machine 1, 0, 0, 0
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .workgroup_group_segment_size 1000
        .kernarg_segment_size 80
        .wavefront_sgpr_count 14
        .workitem_vgpr_count 1
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
    .hsaconfig
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg n, "uint", uint
        .arg in, "uint*", uint*, global, const
        .arg out, "uint*", uint*, global, 
    .text
/*8709ac05         */ s_and_b32       s9, s5, 44
/*870a8505         */ s_and_b32       s10, s5, 5
/*870b8505         */ s_and_b32       s11, s5, 5
/*bf810000         */ s_endpgm
.kernel aaa2
    .hsaconfig
        .dims x, xyz
        .sgprsnum 16
        .vgprsnum 4
        .privmode
        .debugmode
        .dx10clamp
        .ieeemode
        .floatmode 0xda
        .priority 2
        .userdatanum 10
        .pgmrsrc1 0x00fda840
        .pgmrsrc2 0x12001095
        .codeversion 1, 1
        .machine 1, 0, 0, 0
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .use_flat_scratch_init
        .private_elem_size 4
        .use_ptr64
        .workitem_private_segment_size 2342
        .workgroup_group_segment_size 1000
        .kernarg_segment_size 96
        .wavefront_sgpr_count 16
        .workitem_vgpr_count 1
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
    .hsaconfig
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg n, "uint", uint
        .arg in, "float*", float*, global, const
        .arg out, "float*", float*, global, 
        .arg q, "queue_t", queue
        .arg piper, "pipe", pipe, rdonly
        .arg ce, "clk_event_t", clkevent
    .text
/*8709ac05         */ s_and_b32       s9, s5, 44
/*bf810000         */ s_endpgm
.kernel gfd12
    .hsaconfig
        .cws 8, 5, 2
        .dims xyz
        .sgprsnum 32
        .vgprsnum 80
        .dx10clamp
        .tgsize
        .floatmode 0xf4
        .priority 0
        .exceptions 0x01
        .localsize 210432
        .userdatanum 12
        .pgmrsrc1 0x012f40d3
        .pgmrsrc2 0x0fcdf7d8
        .codeversion 1, 1
        .machine 1, 0, 0, 0
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_queue_ptr
        .use_kernarg_segment_ptr
        .use_flat_scratch_init
        .private_elem_size 4
        .use_ptr64
        .workgroup_group_segment_size 656
        .kernarg_segment_size 80
        .wavefront_sgpr_count 29
        .workitem_vgpr_count 78
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
    .hsaconfig
        .arg _.global_offset_0, "size_t", long
        .arg _.global_offset_1, "size_t", long
        .arg _.global_offset_2, "size_t", long
        .arg _.printf_buffer, "size_t", void*, global, , rdonly
        .arg _.vqueue_pointer, "size_t", long
        .arg _.aqlwrap_pointer, "size_t", long
        .arg n, "uint", uint
        .arg in, "double*", double*, global, const
        .arg out, "double*", double*, global, 
        .arg v, "uchar", uchar
        .arg v2, "uchar", uchar
    .text
/*7e020302         */ v_mov_b32       v1, v2
/*4a0206b7         */ v_add_i32       v1, vcc, 55, v3
/*bf810000         */ s_endpgm
)ffDXD", true, true
    },
    /* 8 - gallium config */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/gallium1.clo",
        R"ffDXD(.gallium
.gpu CapeVerde
.32bit
.rodata
    .byte 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00
    .byte 0x03, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00
    .byte 0x2d, 0x00, 0x00, 0x00
.kernel one1
    .args
        .arg scalar, 4, 8, 4, sext, general
        .arg scalar, 16, 16, 32, sext, general
        .arg scalar, 4, 4, 4, zext, imgformat
        .arg global, 8, 8, 8, zext, general
        .arg scalar, 4, 4, 4, zext, griddim
        .arg scalar, 4, 4, 4, zext, gridoffset
    .config
        .dims x
        .sgprsnum 104
        .vgprsnum 12
        .privmode
        .debugmode
        .ieeemode
        .tgsize
        .floatmode 0xfe
        .priority 3
        .exceptions 0x7f
        .localsize 32768
        .userdatanum 16
        .scratchbuffer 48
        .pgmrsrc1 0xffdfef02
        .pgmrsrc2 0x004004a1
.kernel secondx
    .args
    .config
        .dims xyz
        .sgprsnum 8
        .vgprsnum 56
        .tgsize
        .floatmode 0xd0
        .priority 3
        .localsize 256
        .userdatanum 16
        .pgmrsrc1 0x000d0c0d
        .pgmrsrc2 0x3d0097a0
.text
secondx:
/*3a040480         */ v_xor_b32       v2, 0, v2
/*bf810000         */ s_endpgm
.fill 62, 4, 0
one1:
/*7e000301         */ v_mov_b32       v0, v1
/*7e000303         */ v_mov_b32       v0, v3
)ffDXD", true, false, 0
    },
    /* 9 - gallium config - different dimensions for group_ids and local_ids */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/gallium1-diffdims.clo",
        R"ffDXD(.gallium
.gpu CapeVerde
.32bit
.rodata
    .byte 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00
    .byte 0x03, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00
    .byte 0x2d, 0x00, 0x00, 0x00
.kernel one1
    .args
        .arg scalar, 4, 8, 4, sext, general
        .arg scalar, 16, 16, 32, sext, general
        .arg scalar, 4, 4, 4, zext, imgformat
        .arg global, 8, 8, 8, zext, general
        .arg scalar, 4, 4, 4, zext, griddim
        .arg scalar, 4, 4, 4, zext, gridoffset
    .config
        .dims x
        .sgprsnum 104
        .vgprsnum 12
        .privmode
        .debugmode
        .ieeemode
        .tgsize
        .floatmode 0xfe
        .priority 3
        .exceptions 0x7f
        .localsize 32768
        .userdatanum 16
        .scratchbuffer 48
        .pgmrsrc1 0xffdfef02
        .pgmrsrc2 0x004004a1
.kernel secondx
    .args
    .config
        .dims yz, xyz
        .sgprsnum 8
        .vgprsnum 56
        .tgsize
        .floatmode 0xd0
        .priority 3
        .localsize 256
        .userdatanum 16
        .pgmrsrc1 0x000d0c0d
        .pgmrsrc2 0x3d009720
.text
secondx:
/*3a040480         */ v_xor_b32       v2, 0, v2
/*bf810000         */ s_endpgm
.fill 62, 4, 0
one1:
/*7e000301         */ v_mov_b32       v0, v1
/*7e000303         */ v_mov_b32       v0, v3
)ffDXD", true, false, 0
    },
    /* 10 - gallium config */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/new-gallium-llvm40.clo",
        R"ffDXD(.gallium
.gpu CapeVerde
.64bit
.driver_version 170000
.llvm_version 40000
.kernel vectorAdd
    .args
        .arg scalar, 4, 4, 4, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg scalar, 4, 4, 4, zext, griddim
        .arg scalar, 4, 4, 4, zext, gridoffset
    .config
        .dims x
        .sgprsnum 24
        .vgprsnum 4
        .dx10clamp
        .ieeemode
        .floatmode 0xc0
        .priority 0
        .userdatanum 8
        .pgmrsrc1 0x00ac0080
        .pgmrsrc2 0x00000090
        .spilledsgprs 0
        .spilledvgprs 0
        .hsa_dims x
        .hsa_sgprsnum 24
        .hsa_vgprsnum 4
        .hsa_dx10clamp
        .hsa_ieeemode
        .hsa_floatmode 0xc0
        .hsa_priority 0
        .hsa_userdatanum 8
        .hsa_pgmrsrc1 0x00ac0080
        .hsa_pgmrsrc2 0x00000090
        .codeversion 1, 0
        .machine 1, 0, 0, 0
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_size 48
        .wavefront_sgpr_count 18
        .workitem_vgpr_count 4
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
.text
vectorAdd:
.skip 256
/*c0000501         */ s_load_dword    s0, s[4:5], 0x1
/*c0008709         */ s_load_dword    s1, s[6:7], 0x9
/*c0010700         */ s_load_dword    s2, s[6:7], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8700ff00 0000ffff*/ s_and_b32       s0, s0, 0xffff
/*93000800         */ s_mul_i32       s0, s0, s8
/*81000100         */ s_add_i32       s0, s0, s1
/*4a000000         */ v_add_i32       v0, vcc, s0, v0
/*7d880002         */ v_cmp_gt_u32    vcc, s2, v0
/*be80246a         */ s_and_saveexec_b64 s[0:1], vcc
/*bf880014         */ s_cbranch_execz .L384_0
/*7e000280         */ v_mov_b32       v0, 0
/*c0400702         */ s_load_dwordx2  s[0:1], s[6:7], 0x2
/*be820380         */ s_mov_b32       s2, 0
/*be8303ff 0000f000*/ s_mov_b32       s3, 0xf000
/*c0440704         */ s_load_dwordx2  s[8:9], s[6:7], 0x4
/*be8a0402         */ s_mov_b64       s[10:11], s[2:3]
/*c0460706         */ s_load_dwordx2  s[12:13], s[6:7], 0x6
/*be8e0402         */ s_mov_b64       s[14:15], s[2:3]
/*d2c20000 00010500*/ v_lshl_b64      v[0:1], v[0:1], 2
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*e0308000 80000200*/ buffer_load_dword v2, v[0:1], s[0:3], 0 addr64
/*e0308000 80020300*/ buffer_load_dword v3, v[0:1], s[8:11], 0 addr64
/*bf8c0f70         */ s_waitcnt       vmcnt(0)
/*06040503         */ v_add_f32       v2, v3, v2
/*e0708000 80000200*/ buffer_store_dword v2, v[0:1], s[0:3], 0 addr64
.L384_0:
/*bf810000         */ s_endpgm
)ffDXD", true, false, 40000U
    },
    /* 11 - gallium config - different dimensions for group_ids and local_ids */
    { nullptr, nullptr, CLRX_SOURCE_DIR
        "/tests/amdasm/amdbins/new-gallium-llvm40-diffdims.clo",
        R"ffDXD(.gallium
.gpu CapeVerde
.64bit
.driver_version 170000
.llvm_version 40000
.kernel vectorAdd
    .args
        .arg scalar, 4, 4, 4, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg global, 8, 8, 8, zext, general
        .arg scalar, 4, 4, 4, zext, griddim
        .arg scalar, 4, 4, 4, zext, gridoffset
    .config
        .dims x, xy
        .sgprsnum 24
        .vgprsnum 4
        .dx10clamp
        .ieeemode
        .floatmode 0xc0
        .priority 0
        .userdatanum 8
        .pgmrsrc1 0x00ac0080
        .pgmrsrc2 0x00000890
        .spilledsgprs 0
        .spilledvgprs 0
        .hsa_dims x, xy
        .hsa_sgprsnum 24
        .hsa_vgprsnum 4
        .hsa_dx10clamp
        .hsa_ieeemode
        .hsa_floatmode 0xc0
        .hsa_priority 0
        .hsa_userdatanum 8
        .hsa_pgmrsrc1 0x00ac0080
        .hsa_pgmrsrc2 0x00000890
        .codeversion 1, 0
        .machine 1, 0, 0, 0
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_size 48
        .wavefront_sgpr_count 18
        .workitem_vgpr_count 4
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
.text
vectorAdd:
.skip 256
/*c0000501         */ s_load_dword    s0, s[4:5], 0x1
/*c0008709         */ s_load_dword    s1, s[6:7], 0x9
/*c0010700         */ s_load_dword    s2, s[6:7], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8700ff00 0000ffff*/ s_and_b32       s0, s0, 0xffff
/*93000800         */ s_mul_i32       s0, s0, s8
/*81000100         */ s_add_i32       s0, s0, s1
/*4a000000         */ v_add_i32       v0, vcc, s0, v0
/*7d880002         */ v_cmp_gt_u32    vcc, s2, v0
/*be80246a         */ s_and_saveexec_b64 s[0:1], vcc
/*bf880014         */ s_cbranch_execz .L384_0
/*7e000280         */ v_mov_b32       v0, 0
/*c0400702         */ s_load_dwordx2  s[0:1], s[6:7], 0x2
/*be820380         */ s_mov_b32       s2, 0
/*be8303ff 0000f000*/ s_mov_b32       s3, 0xf000
/*c0440704         */ s_load_dwordx2  s[8:9], s[6:7], 0x4
/*be8a0402         */ s_mov_b64       s[10:11], s[2:3]
/*c0460706         */ s_load_dwordx2  s[12:13], s[6:7], 0x6
/*be8e0402         */ s_mov_b64       s[14:15], s[2:3]
/*d2c20000 00010500*/ v_lshl_b64      v[0:1], v[0:1], 2
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*e0308000 80000200*/ buffer_load_dword v2, v[0:1], s[0:3], 0 addr64
/*e0308000 80020300*/ buffer_load_dword v3, v[0:1], s[8:11], 0 addr64
/*bf8c0f70         */ s_waitcnt       vmcnt(0)
/*06040503         */ v_add_f32       v2, v3, v2
/*e0708000 80000200*/ buffer_store_dword v2, v[0:1], s[0:3], 0 addr64
.L384_0:
/*bf810000         */ s_endpgm
)ffDXD", true, false, 40000U
    },
    /* 12 - rocm config */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/rocm-fiji.hsaco",
        R"ffDXD(.rocm
.gpu Fiji
.arch_minor 0
.arch_stepping 3
.kernel test1
    .config
        .dims x
        .sgprsnum 16
        .vgprsnum 8
        .dx10clamp
        .floatmode 0xc0
        .priority 0
        .userdatanum 8
        .pgmrsrc1 0x002c0041
        .pgmrsrc2 0x00000090
        .codeversion 1, 0
        .machine 1, 8, 0, 3
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_size 8
        .wavefront_sgpr_count 15
        .workitem_vgpr_count 7
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
.kernel test2
    .config
        .dims x
        .sgprsnum 16
        .vgprsnum 8
        .dx10clamp
        .floatmode 0xc0
        .priority 0
        .userdatanum 8
        .pgmrsrc1 0x002c0041
        .pgmrsrc2 0x00000090
        .codeversion 1, 0
        .machine 1, 8, 0, 3
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_size 8
        .wavefront_sgpr_count 15
        .workitem_vgpr_count 7
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
.text
test1:
.skip 256
/*c0020082 00000004*/ s_load_dword    s2, s[4:5], 0x4
/*c0060003 00000000*/ s_load_dwordx2  s[0:1], s[6:7], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8602ff02 0000ffff*/ s_and_b32       s2, s2, 0xffff
/*92020802         */ s_mul_i32       s2, s2, s8
/*32000002         */ v_add_u32       v0, vcc, s2, v0
/*2202009f         */ v_ashrrev_i32   v1, 31, v0
/*d28f0001 00020082*/ v_lshlrev_b64   v[1:2], 2, v[0:1]
/*32060200         */ v_add_u32       v3, vcc, s0, v1
/*7e020201         */ v_mov_b32       v1, s1
/*38080302         */ v_addc_u32      v4, vcc, v2, v1, vcc
/*2600008f         */ v_and_b32       v0, 15, v0
/*7e020280         */ v_mov_b32       v1, 0
/*dc500000 02000003*/ flat_load_dword v2, v[3:4]
/*d28f0000 00020082*/ v_lshlrev_b64   v[0:1], 2, v[0:1]
/*be801c00         */ s_getpc_b64     s[0:1]
/*8000ff00 00000234*/ s_add_u32       s0, s0, 0x234
/*82018001         */ s_addc_u32      s1, s1, 0
/*320a0000         */ v_add_u32       v5, vcc, s0, v0
/*7e000201         */ v_mov_b32       v0, s1
/*380c0101         */ v_addc_u32      v6, vcc, v1, v0, vcc
/*dc500000 00000005*/ flat_load_dword v0, v[5:6]
/*bf8c0070         */ s_waitcnt       vmcnt(0) & lgkmcnt(0)
/*0a000500         */ v_mul_f32       v0, v0, v2
/*dc700000 00000003*/ flat_store_dword v[3:4], v0
/*bf810000         */ s_endpgm
.fill 29, 4, 0
test2:
.skip 256
/*c0020082 00000004*/ s_load_dword    s2, s[4:5], 0x4
/*c0060003 00000000*/ s_load_dwordx2  s[0:1], s[6:7], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8602ff02 0000ffff*/ s_and_b32       s2, s2, 0xffff
/*92020802         */ s_mul_i32       s2, s2, s8
/*32000002         */ v_add_u32       v0, vcc, s2, v0
/*2202009f         */ v_ashrrev_i32   v1, 31, v0
/*d28f0001 00020082*/ v_lshlrev_b64   v[1:2], 2, v[0:1]
/*32060200         */ v_add_u32       v3, vcc, s0, v1
/*7e020201         */ v_mov_b32       v1, s1
/*38080302         */ v_addc_u32      v4, vcc, v2, v1, vcc
/*2600008f         */ v_and_b32       v0, 15, v0
/*7e020280         */ v_mov_b32       v1, 0
/*dc500000 02000003*/ flat_load_dword v2, v[3:4]
/*d28f0000 00020082*/ v_lshlrev_b64   v[0:1], 2, v[0:1]
/*be801c00         */ s_getpc_b64     s[0:1]
/*8000ff00 00000074*/ s_add_u32       s0, s0, 0x74
/*82018001         */ s_addc_u32      s1, s1, 0
/*320a0000         */ v_add_u32       v5, vcc, s0, v0
/*7e000201         */ v_mov_b32       v0, s1
/*380c0101         */ v_addc_u32      v6, vcc, v1, v0, vcc
/*dc500000 00000005*/ flat_load_dword v0, v[5:6]
/*bf8c0070         */ s_waitcnt       vmcnt(0) & lgkmcnt(0)
/*02000500         */ v_add_f32       v0, v0, v2
/*dc700000 00000003*/ flat_store_dword v[3:4], v0
/*bf810000         */ s_endpgm
data1:
.global data1
        .byte 0xcd, 0xcc, 0x8c, 0x3f, 0x33, 0x33, 0x13, 0x40
        .byte 0x33, 0x33, 0x93, 0x40, 0x33, 0x33, 0xa3, 0x40
        .byte 0x85, 0xeb, 0x91, 0xbf, 0x3d, 0x0a, 0x27, 0xc0
        .byte 0x0a, 0xd7, 0x93, 0xc0, 0xb8, 0x1e, 0xa5, 0xc0
        .byte 0x9a, 0x99, 0x49, 0x41, 0xcd, 0xcc, 0xac, 0x40
        .byte 0x0a, 0xd7, 0x13, 0x40, 0xd7, 0xa3, 0x98, 0x40
        .byte 0x9a, 0x99, 0x49, 0xc1, 0xcd, 0xcc, 0xac, 0xc0
        .byte 0x0a, 0xd7, 0x13, 0xc0, 0xd7, 0xa3, 0x98, 0xc0
data2:
.global data2
        .byte 0xcd, 0x8c, 0xaa, 0x43, 0x33, 0x33, 0x13, 0x40
        .byte 0x33, 0x33, 0x93, 0x40, 0x52, 0xb8, 0xe6, 0x40
        .byte 0xb8, 0x1e, 0x81, 0xc1, 0x3d, 0x0a, 0x27, 0xc0
        .byte 0x85, 0xeb, 0x09, 0xc1, 0xb8, 0x1e, 0xa5, 0xc0
        .byte 0x9a, 0x99, 0x49, 0x41, 0xcd, 0xcc, 0xaa, 0x42
        .byte 0x0a, 0xd7, 0x33, 0x40, 0xd7, 0xa3, 0x98, 0x40
        .byte 0xcd, 0xcc, 0x94, 0xc1, 0x33, 0x33, 0xb3, 0xc0
        .byte 0x0a, 0xd7, 0x13, 0xc0, 0xd7, 0xa3, 0x98, 0xc0
)ffDXD", true, false
    },
    /* 13 - rocm config - different dimensions for group_ids and local_ids */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/rocm-fiji-diffdims.hsaco",
        R"ffDXD(.rocm
.gpu Fiji
.arch_minor 0
.arch_stepping 3
.kernel test1
    .config
        .dims x, xy
        .sgprsnum 16
        .vgprsnum 8
        .dx10clamp
        .floatmode 0xc0
        .priority 0
        .userdatanum 8
        .pgmrsrc1 0x002c0041
        .pgmrsrc2 0x00000890
        .codeversion 1, 0
        .machine 1, 8, 0, 3
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_size 8
        .wavefront_sgpr_count 15
        .workitem_vgpr_count 7
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
.kernel test2
    .config
        .dims x
        .sgprsnum 16
        .vgprsnum 8
        .dx10clamp
        .floatmode 0xc0
        .priority 0
        .userdatanum 8
        .pgmrsrc1 0x002c0041
        .pgmrsrc2 0x00000090
        .codeversion 1, 0
        .machine 1, 8, 0, 3
        .kernel_code_entry_offset 0x100
        .use_private_segment_buffer
        .use_dispatch_ptr
        .use_kernarg_segment_ptr
        .private_elem_size 4
        .use_ptr64
        .kernarg_segment_size 8
        .wavefront_sgpr_count 15
        .workitem_vgpr_count 7
        .kernarg_segment_align 16
        .group_segment_align 16
        .private_segment_align 16
        .wavefront_size 64
        .call_convention 0x0
    .control_directive
        .fill 128, 1, 0x00
.text
test1:
.skip 256
/*c0020082 00000004*/ s_load_dword    s2, s[4:5], 0x4
/*c0060003 00000000*/ s_load_dwordx2  s[0:1], s[6:7], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8602ff02 0000ffff*/ s_and_b32       s2, s2, 0xffff
/*92020802         */ s_mul_i32       s2, s2, s8
/*32000002         */ v_add_u32       v0, vcc, s2, v0
/*2202009f         */ v_ashrrev_i32   v1, 31, v0
/*d28f0001 00020082*/ v_lshlrev_b64   v[1:2], 2, v[0:1]
/*32060200         */ v_add_u32       v3, vcc, s0, v1
/*7e020201         */ v_mov_b32       v1, s1
/*38080302         */ v_addc_u32      v4, vcc, v2, v1, vcc
/*2600008f         */ v_and_b32       v0, 15, v0
/*7e020280         */ v_mov_b32       v1, 0
/*dc500000 02000003*/ flat_load_dword v2, v[3:4]
/*d28f0000 00020082*/ v_lshlrev_b64   v[0:1], 2, v[0:1]
/*be801c00         */ s_getpc_b64     s[0:1]
/*8000ff00 00000234*/ s_add_u32       s0, s0, 0x234
/*82018001         */ s_addc_u32      s1, s1, 0
/*320a0000         */ v_add_u32       v5, vcc, s0, v0
/*7e000201         */ v_mov_b32       v0, s1
/*380c0101         */ v_addc_u32      v6, vcc, v1, v0, vcc
/*dc500000 00000005*/ flat_load_dword v0, v[5:6]
/*bf8c0070         */ s_waitcnt       vmcnt(0) & lgkmcnt(0)
/*0a000500         */ v_mul_f32       v0, v0, v2
/*dc700000 00000003*/ flat_store_dword v[3:4], v0
/*bf810000         */ s_endpgm
.fill 29, 4, 0
test2:
.skip 256
/*c0020082 00000004*/ s_load_dword    s2, s[4:5], 0x4
/*c0060003 00000000*/ s_load_dwordx2  s[0:1], s[6:7], 0x0
/*bf8c007f         */ s_waitcnt       lgkmcnt(0)
/*8602ff02 0000ffff*/ s_and_b32       s2, s2, 0xffff
/*92020802         */ s_mul_i32       s2, s2, s8
/*32000002         */ v_add_u32       v0, vcc, s2, v0
/*2202009f         */ v_ashrrev_i32   v1, 31, v0
/*d28f0001 00020082*/ v_lshlrev_b64   v[1:2], 2, v[0:1]
/*32060200         */ v_add_u32       v3, vcc, s0, v1
/*7e020201         */ v_mov_b32       v1, s1
/*38080302         */ v_addc_u32      v4, vcc, v2, v1, vcc
/*2600008f         */ v_and_b32       v0, 15, v0
/*7e020280         */ v_mov_b32       v1, 0
/*dc500000 02000003*/ flat_load_dword v2, v[3:4]
/*d28f0000 00020082*/ v_lshlrev_b64   v[0:1], 2, v[0:1]
/*be801c00         */ s_getpc_b64     s[0:1]
/*8000ff00 00000074*/ s_add_u32       s0, s0, 0x74
/*82018001         */ s_addc_u32      s1, s1, 0
/*320a0000         */ v_add_u32       v5, vcc, s0, v0
/*7e000201         */ v_mov_b32       v0, s1
/*380c0101         */ v_addc_u32      v6, vcc, v1, v0, vcc
/*dc500000 00000005*/ flat_load_dword v0, v[5:6]
/*bf8c0070         */ s_waitcnt       vmcnt(0) & lgkmcnt(0)
/*02000500         */ v_add_f32       v0, v0, v2
/*dc700000 00000003*/ flat_store_dword v[3:4], v0
/*bf810000         */ s_endpgm
data1:
.global data1
        .byte 0xcd, 0xcc, 0x8c, 0x3f, 0x33, 0x33, 0x13, 0x40
        .byte 0x33, 0x33, 0x93, 0x40, 0x33, 0x33, 0xa3, 0x40
        .byte 0x85, 0xeb, 0x91, 0xbf, 0x3d, 0x0a, 0x27, 0xc0
        .byte 0x0a, 0xd7, 0x93, 0xc0, 0xb8, 0x1e, 0xa5, 0xc0
        .byte 0x9a, 0x99, 0x49, 0x41, 0xcd, 0xcc, 0xac, 0x40
        .byte 0x0a, 0xd7, 0x13, 0x40, 0xd7, 0xa3, 0x98, 0x40
        .byte 0x9a, 0x99, 0x49, 0xc1, 0xcd, 0xcc, 0xac, 0xc0
        .byte 0x0a, 0xd7, 0x13, 0xc0, 0xd7, 0xa3, 0x98, 0xc0
data2:
.global data2
        .byte 0xcd, 0x8c, 0xaa, 0x43, 0x33, 0x33, 0x13, 0x40
        .byte 0x33, 0x33, 0x93, 0x40, 0x52, 0xb8, 0xe6, 0x40
        .byte 0xb8, 0x1e, 0x81, 0xc1, 0x3d, 0x0a, 0x27, 0xc0
        .byte 0x85, 0xeb, 0x09, 0xc1, 0xb8, 0x1e, 0xa5, 0xc0
        .byte 0x9a, 0x99, 0x49, 0x41, 0xcd, 0xcc, 0xaa, 0x42
        .byte 0x0a, 0xd7, 0x33, 0x40, 0xd7, 0xa3, 0x98, 0x40
        .byte 0xcd, 0xcc, 0x94, 0xc1, 0x33, 0x33, 0xb3, 0xc0
        .byte 0x0a, 0xd7, 0x13, 0xc0, 0xd7, 0xa3, 0x98, 0xc0
)ffDXD", true, false
    },
    /* 14 - gallium old bin format disassembled as new binformat - error */
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdasm/amdbins/gallium1.clo",
        "", true, false, 40000U,
        "Gallium kernel region is too small" // error
    },
};

static void testDisasmData(cxuint testId, const DisasmAmdTestCase& testCase)
{
    std::ostringstream disasmOss;
    std::string resultStr;
    Flags disasmFlags = DISASM_ALL&~DISASM_CODEPOS;
    if (testCase.config)
        disasmFlags |= DISASM_CONFIG;
    if (testCase.hsaConfig)
        disasmFlags |= DISASM_HSACONFIG;
    
    bool haveException = false;
    std::string resExceptionStr;
    try
    {
    if (testCase.filename == nullptr)
    {
        // disassemble input provided in testcase
        if (testCase.amdInput != nullptr)
        {
            Disassembler disasm(testCase.amdInput, disasmOss, disasmFlags);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
        else if (testCase.galliumInput != nullptr)
        {
            Disassembler disasm(testCase.galliumInput, disasmOss, disasmFlags);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
    }
    else
    {
        // disassemble file that filename provided by testcase
        Array<cxbyte> binaryData = loadDataFromFile(testCase.filename);
        if (isAmdBinary(binaryData.size(), binaryData.data()))
        {
            // if AMD OpenCL binary
            std::unique_ptr<AmdMainBinaryBase> base(createAmdBinaryFromCode(
                    binaryData.size(), binaryData.data(),
                    AMDBIN_CREATE_KERNELINFO | AMDBIN_CREATE_KERNELINFOMAP |
                    AMDBIN_CREATE_INNERBINMAP | AMDBIN_CREATE_KERNELHEADERS |
                    AMDBIN_CREATE_KERNELHEADERMAP | AMDBIN_INNER_CREATE_CALNOTES |
                    AMDBIN_CREATE_INFOSTRINGS));
            AmdMainGPUBinary32* amdGpuBin = static_cast<AmdMainGPUBinary32*>(base.get());
            Disassembler disasm(*amdGpuBin, disasmOss, disasmFlags);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
        else if (isAmdCL2Binary(binaryData.size(), binaryData.data()))
        {
            // if AMD OpenCL 2.0 binary
            AmdCL2MainGPUBinary64 amdBin(binaryData.size(), binaryData.data(),
                AMDBIN_CREATE_KERNELINFO | AMDBIN_CREATE_KERNELINFOMAP |
                AMDBIN_CREATE_INNERBINMAP | AMDBIN_CREATE_KERNELHEADERS |
                AMDBIN_CREATE_KERNELHEADERMAP | AMDBIN_INNER_CREATE_CALNOTES |
                AMDBIN_CREATE_INFOSTRINGS | AMDCL2BIN_INNER_CREATE_KERNELDATA |
                AMDCL2BIN_INNER_CREATE_KERNELDATAMAP | AMDCL2BIN_INNER_CREATE_KERNELSTUBS);
            Disassembler disasm(amdBin, disasmOss, disasmFlags);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
        else if (isROCmBinary(binaryData.size(), binaryData.data()))
        {
            // if ROCm (HSACO) binary
            ROCmBinary rocmBin(binaryData.size(), binaryData.data(), 0);
            Disassembler disasm(rocmBin, disasmOss, disasmFlags);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
        else
        {   /* gallium */
            GalliumBinary galliumBin(binaryData.size(),binaryData.data(), 0);
            Disassembler disasm(GPUDeviceType::CAPE_VERDE, galliumBin,
                            disasmOss, disasmFlags, testCase.llvmVersion);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
    }
    }
    catch(const DisasmException& ex)
    {
        resExceptionStr = ex.what();
        haveException = true;
    }
    
    std::string caseName;
    {
        std::ostringstream oss;
        oss << "DisasmCase#" << testId;
        oss.flush();
        caseName = oss.str();
    }
    
    if (testCase.exceptionString == nullptr)
        assertTrue("DisasmData", caseName + ".noexception", !haveException);
    else
    {
        assertTrue("DisasmData", caseName + ".exception", haveException);
        assertString("DisasmData", caseName + ".exceptionString",
                     testCase.exceptionString, resExceptionStr);
    }
    
    // compare output with expected string
    if (::strcmp(testCase.expectedString, resultStr.c_str()) != 0)
    {
        // print error
        std::ostringstream oss;
        oss << "Failed for #" << testId << std::endl;
        oss << resultStr << std::endl;
        oss.flush();
        throw Exception(oss.str());
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(disasmDataTestCases)/sizeof(DisasmAmdTestCase); i++)
        try
        { testDisasmData(i, disasmDataTestCases[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
