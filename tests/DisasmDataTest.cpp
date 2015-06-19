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
#include <string>
#include <cstring>
#include <CLRX/amdasm/Disassembler.h>

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
    0xffcd44dc, 0x4543,
    0x456, 0x5663677,
    0x3cd90c, 0x3958a85,
    0x458c98d9, 0x344dbd9,
    0xd0d9d9d, 0x234455,
    0x1, 0x55, 0x4565
};

static uint32_t disasmInput1Kernel1Inputs[20] =
{
    1113, 1113, 1113, 1113, 1113, 1113, 1113, 1113,
    1113, 1113, 1113, 1114, 1113, 1113, 1113, 1113,
    1113, 1113, 1113, 1113
};

static uint32_t disasmInput1Kernel1Outputs[17] =
{
    5, 6, 3, 13, 5, 117, 12, 3,
    785, 46, 55, 5, 2, 3, 0, 0, 44
};

static uint32_t disasmInput1Kernel1EarlyExit[3] = { 1355, 44, 444 };

static uint32_t disasmInput1Kernel2EarlyExit[3] = { 121 };

static CALDataSegmentEntry disasmInput1Kernel1Float32Consts[6] =
{
    { 0, 5 }, { 66, 57 }, { 67, 334 }, { 1, 6 }, { 5, 86 }, { 2100, 466 }
};

static CALSamplerMapEntry disasmInput1Kernel1InputSamplers[6] =
{
    { 0, 5 }, { 66, 57 }, { 67, 334 }, { 1, 6 }, { 5, 86 }, { 2100, 466 }
};

static uint32_t disasmInput1Kernel1UavOpMask[1] = { 4556 };

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

static const GalliumDisasmInput galliumDisasmData =
{
    GPUDeviceType::PITCAIRN,
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
};

static const DisasmAmdTestCase disasmDataTestCases[] =
{
    { &disasmInput1, nullptr, nullptr,
        R"xxFxx(.amd
.gpu Spooky
.64bit
.compile_options "-O ccc"
.driver_info "This\nis my\3001 stupid\r\"driver"
.data
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
    .kerneldata
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
)xxFxx"
    },
    { nullptr, nullptr, CLRX_SOURCE_DIR "/tests/amdbins/samplekernels.clo",
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
    .kerneldata
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
/*bf880011         */ s_cbranch_execz .L128
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
.L128:
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
    .kerneldata
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
/*bf880011         */ s_cbranch_execz .L128
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
.L128:
/*bf810000         */ s_endpgm
)xxFxx"
    },
    { nullptr, &galliumDisasmData, nullptr,
        R"fxDfx(.gallium
.gpu Pitcairn
.data
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
)fxDfx"
    }
};

static void testDisasmData(cxuint testId, const DisasmAmdTestCase& testCase)
{
    std::ostringstream disasmOss;
    std::string resultStr;
    if (testCase.filename == nullptr)
    {
        if (testCase.amdInput != nullptr)
        {
            Disassembler disasm(testCase.amdInput, disasmOss, DISASM_ALL);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
        else if (testCase.galliumInput != nullptr)
        {
            Disassembler disasm(testCase.galliumInput, disasmOss, DISASM_ALL);
            disasm.disassemble();
            resultStr = disasmOss.str();
        }
    }
    else
    {
        Array<cxbyte> binaryData = loadDataFromFile(testCase.filename);
        std::unique_ptr<AmdMainBinaryBase> base(createAmdBinaryFromCode(
                binaryData.size(), binaryData.data(),
                AMDBIN_CREATE_KERNELINFO | AMDBIN_CREATE_KERNELINFOMAP |
                AMDBIN_CREATE_INNERBINMAP | AMDBIN_CREATE_KERNELHEADERS |
                AMDBIN_CREATE_KERNELHEADERMAP | AMDBIN_INNER_CREATE_CALNOTES |
                AMDBIN_CREATE_INFOSTRINGS));
        AmdMainGPUBinary32* amdGpuBin = static_cast<AmdMainGPUBinary32*>(base.get());
        Disassembler disasm(*amdGpuBin, disasmOss, DISASM_ALL);
        disasm.disassemble();
        resultStr = disasmOss.str();
    }
    
    if (::strcmp(testCase.expectedString, resultStr.c_str()) != 0)
    {   // print error
        std::ostringstream oss;
        oss << "Failed for #" << testId << std::endl;
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
