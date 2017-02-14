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
#include <sstream>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "../TestUtils.h"
#include "AsmBasics.h"

using namespace CLRX;

const AsmTestCase asmTestCases2Tbl[] =
{
    /* 66 - undef test */
    {   R"ffDXD(.eqv xz, a*b
        .int xz+xz*7
        .eqv ulu,xz*xz
        .int ulu
        .undef xz
        .int xz
        a = 3; b = 8
        
        xz1=a*b
        .int xz1
        .undef xz1
        .int xz1
        
        xz2=a1*b1
        .int xz2
        .undef xz2
        .int xz2
        
        xz2=a1*b1
        .undef xz2
        .undef xz2)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0xc0, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 28U, 0, 0U, true, false, false, 0, 0 },
            { "a", 3U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "a1", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "b", 8U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "b1", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "ulu", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "xz", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "xz1", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "xz2", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 }
        },
        true, "test.s:21:16: Warning: Symbol 'xz2' already doesn't exist\n", ""
    },
    /* 67 - include test 1 */
    {   R"ffDXD(            .include "inc1.s"
            .include "inc2.s"
            .include "inc3.s")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 11,22,44,55, 11,22,44,58, 31,23,44,55 } } },
        { { ".", 12U, 0, 0U, true, false, false, 0, 0 } },
        true, "", "",
        { CLRX_SOURCE_DIR "/tests/amdasm/incdir0", CLRX_SOURCE_DIR "/tests/amdasm/incdir1" }
    },
    /* 68 - include test 2 */
    {   R"ffDXD(            .include "incdir0\\inc1.s"
            .include "incdir0/inc2.s"
            .include "incdir1\\inc3.s")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 11,22,44,55, 11,22,44,58, 31,23,44,55 } } },
        { { ".", 12U, 0, 0U, true, false, false, 0, 0 } },
        true, "", "",
        { CLRX_SOURCE_DIR "/tests/amdasm" }
    },
    /* 69 - failed include */
    {   R"ffDXD(            .include "incdir0\\incx.s"
            .include "xxxx.s"
            .include "xxxa.s")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:1:22: Error: Include file "
        "'incdir0\\incx.s' not found or unavailable in any directory\n"
        "test.s:2:22: Error: Include file 'xxxx.s' "
        "not found or unavailable in any directory\n"
        "test.s:3:22: Error: Include file 'xxxa.s' "
        "not found or unavailable in any directory\n", "",
        { CLRX_SOURCE_DIR "/tests/amdasm" }
    },
    /* 70 - incbin */
    {   R"ffDXD(            .incbin "incbin1"
            .incbin "incbin2"
            .incbin "incbin3")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x12, 0x87, 0x17, 0x87, 0x8C, 0xD8, 0xA8, 0x93,
                0x89, 0xA9, 0x81, 0xA8, 0x94, 0xD3, 0x89, 0xC8,
                0x9A, 0x13, 0x89, 0x89, 0xDD, 0x12, 0x31, 0x12,
                0x1D, 0xCD, 0xAF, 0x12, 0xCD, 0xCD, 0x33, 0x81,
                0xA8, 0x11, 0xD3, 0x22, 0xC8, 0x9A, 0x12, 0x34,
                0x56, 0x78, 0x90, 0xCD, 0xAD, 0xFC, 0x1A, 0x2A,
                0x3D, 0x4D, 0x6D, 0x3C
            } } },
        { { ".", 52U, 0, 0U, true, false, false, 0, 0 } },
        true, "", "",
        { CLRX_SOURCE_DIR "/tests/amdasm/incdir0", CLRX_SOURCE_DIR "/tests/amdasm/incdir1" }
    },
    /* 71 - incbin (choose offset and size) */
    {   R"ffDXD(            .incbin "incbin1",3,4
            .incbin "incbin2",2
            .incbin "incbin3", 0, 9)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x87, 0x8C, 0xD8, 0xA8, 0x12, 0x1D, 0xCD, 0xAF,
                0x12, 0xCD, 0xCD, 0x33, 0x81, 0xA8, 0x11, 0xD3,
                0x22, 0xC8, 0x9A, 0x12, 0x34, 0x56, 0x78, 0x90,
                0xCD, 0xAD, 0xFC, 0x1A
            } } },
        { { ".", 28U, 0, 0U, true, false, false, 0, 0 } },
        true, "", "",
        { CLRX_SOURCE_DIR "/tests/amdasm/incdir0", CLRX_SOURCE_DIR "/tests/amdasm/incdir1" }
    },
    /* 72 - failed incbin */
    {   R"ffDXD(            .incbin "incdir0\\incbinx"
            .incbin "xxxx.bin"
            .incbin "xxxa.bin")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:1:21: Error: Binary file "
        "'incdir0\\incbinx' not found or unavailable in any directory\n"
        "test.s:2:21: Error: Binary file 'xxxx.bin' "
        "not found or unavailable in any directory\n"
        "test.s:3:21: Error: Binary file 'xxxa.bin' "
        "not found or unavailable in any directory\n", "",
        { CLRX_SOURCE_DIR "/tests/amdasm" }
    },
    /* 73 - absolute section and errors */
    {   R"ffDXD(        .struct 6
label1:
        .struct 7
label2:
        .org 3,44
label3:
        .offset .+20
label4:)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        {
            { ".", 23U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "label1", 6U, ASMSECT_ABS, 0U, true, true, false, 0, 0 },
            { "label2", 7U, ASMSECT_ABS, 0U, true, true, false, 0, 0 },
            { "label3", 3U, ASMSECT_ABS, 0U, true, true, false, 0, 0 },
            { "label4", 23U, ASMSECT_ABS, 0U, true, true, false, 0, 0 }
        },
        true, "test.s:5:14: Warning: Fill value is ignored inside absolute section\n", ""
    },
    /* 74 */
    {   R"ffDXD(        .struct 6
label1:
        .struct 7
label2:
        .org 3,44
label3:
        .incbin "offset.s"
        .byte 11,2,2,,44
        .hword 11,2,2,,44
        .int 11,2,2,,44
        .long 11,2,2,,44
        .half 11,2,2,,44
        .single 11,2,2,,44
        .double 11,2,2,,44
        .string "ala","ma","kota"
        .string16 "ala","ma","kota"
        .string32 "ala","ma","kota"
        .string64 "ala","ma","kota"
        .ascii "ala","ma","kota"
        .asciz "ala","ma","kota"
        .offset .+20
label4:
        .offset %%%%)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        {
            { ".", 23U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "label1", 6U, ASMSECT_ABS, 0U, true, true, false, 0, 0 },
            { "label2", 7U, ASMSECT_ABS, 0U, true, true, false, 0, 0 },
            { "label3", 3U, ASMSECT_ABS, 0U, true, true, false, 0, 0 },
            { "label4", 23U, ASMSECT_ABS, 0U, true, true, false, 0, 0 }
        },
        false, R"ffDXD(test.s:5:14: Warning: Fill value is ignored inside absolute section
test.s:7:9: Error: Writing data into non-writeable section is illegal
test.s:8:9: Error: Writing data into non-writeable section is illegal
test.s:9:9: Error: Writing data into non-writeable section is illegal
test.s:10:9: Error: Writing data into non-writeable section is illegal
test.s:11:9: Error: Writing data into non-writeable section is illegal
test.s:12:9: Error: Writing data into non-writeable section is illegal
test.s:13:9: Error: Writing data into non-writeable section is illegal
test.s:14:9: Error: Writing data into non-writeable section is illegal
test.s:15:9: Error: Writing data into non-writeable section is illegal
test.s:16:9: Error: Writing data into non-writeable section is illegal
test.s:17:9: Error: Writing data into non-writeable section is illegal
test.s:18:9: Error: Writing data into non-writeable section is illegal
test.s:19:9: Error: Writing data into non-writeable section is illegal
test.s:20:9: Error: Writing data into non-writeable section is illegal
test.s:23:17: Error: Expected primary expression before operator
test.s:23:19: Error: Expected primary expression before operator
)ffDXD", ""
    },
    /* 75 - empty lines inside macro,repeats */
    {   R"ffDXD(            .rept 1

.error "111"


  .error "222"


.error "333"
            .endr
            .irp xv,aa,bb,cc

.error "111a"


  .error "222a"


.error "333a"
            .endr
            .macro macro

.error "uurggg"

.error "uurggg"
            .endm
            macro)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        false, R"ffDXD(In repetition 1/1:
test.s:3:1: Error: 111
In repetition 1/1:
test.s:6:3: Error: 222
In repetition 1/1:
test.s:9:1: Error: 333
In repetition 1/3:
test.s:13:1: Error: 111a
In repetition 1/3:
test.s:16:3: Error: 222a
In repetition 1/3:
test.s:19:1: Error: 333a
In repetition 2/3:
test.s:13:1: Error: 111a
In repetition 2/3:
test.s:16:3: Error: 222a
In repetition 2/3:
test.s:19:1: Error: 333a
In repetition 3/3:
test.s:13:1: Error: 111a
In repetition 3/3:
test.s:16:3: Error: 222a
In repetition 3/3:
test.s:19:1: Error: 333a
In macro substituted from test.s:27:13:
test.s:23:1: Error: uurggg
In macro substituted from test.s:27:13:
test.s:25:1: Error: uurggg
)ffDXD", ""
    },
    /* 76 - IRP and IRPC */
    {   R"ffDXD(        .irp Xv, aa , cv  ,  dd,  12AA,  ff
        .string "::\Xv\()__"
        .endr
        .irp Xv, aa  cv    dd   12AA  ff
        .string "::\Xv\()__"
        .endr
        .irpc Xv, 1 5  66 [ ] aa
        .string "::\Xv\()__"
        .endr
        .irpc Xv, x , y
        .string "::\Xv\()__"
        .endr
        .irp Xv,   
        .string "::\Xv\()__"
        .endr
        .irpc Xv,    
        .string "::\Xv\()__"
        .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, {
            0x3a, 0x3a, 0x61, 0x61, 0x5f, 0x5f, 0x00, 0x3a,
            0x3a, 0x63, 0x76, 0x5f, 0x5f, 0x00, 0x3a, 0x3a,
            0x64, 0x64, 0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x31,
            0x32, 0x41, 0x41, 0x5f, 0x5f, 0x00, 0x3a, 0x3a,
            0x66, 0x66, 0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x61,
            0x61, 0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x63, 0x76,
            0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x64, 0x64, 0x5f,
            0x5f, 0x00, 0x3a, 0x3a, 0x31, 0x32, 0x41, 0x41,
            0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x66, 0x66, 0x5f,
            0x5f, 0x00, 0x3a, 0x3a, 0x31, 0x5f, 0x5f, 0x00,
            0x3a, 0x3a, 0x35, 0x5f, 0x5f, 0x00, 0x3a, 0x3a,
            0x36, 0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x36, 0x5f,
            0x5f, 0x00, 0x3a, 0x3a, 0x5b, 0x5f, 0x5f, 0x00,
            0x3a, 0x3a, 0x5d, 0x5f, 0x5f, 0x00, 0x3a, 0x3a,
            0x61, 0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x61, 0x5f,
            0x5f, 0x00, 0x3a, 0x3a, 0x78, 0x5f, 0x5f, 0x00,
            0x3a, 0x3a, 0x2c, 0x5f, 0x5f, 0x00, 0x3a, 0x3a,
            0x79, 0x5f, 0x5f, 0x00, 0x3a, 0x3a, 0x5f, 0x5f,
            0x00, 0x3a, 0x3a, 0x5f, 0x5f, 0x00
        } } },
        { { ".", 150U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 77 - section arithmetics */
    {   R"ffDXD(            .amd
            .kernel a
al:         .ascii "aaabbcc"
ae:
            .kernel b
bl:         .ascii "aaabbcc"
be:
            .int al-ae-bl+be
            .int -bl+be
            z = al*7-(ae-al)*al
            z1 = (ae*7+be*19)-3*be-(be<<4)-6*ae)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { "a", "b" },
        {
            { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } },
            { ".text", 0, AsmSectionType::CODE,
                { 0x61, 0x61, 0x61, 0x62, 0x62, 0x63, 0x63 } },
            { ".text", 1, AsmSectionType::CODE,
                {   0x61, 0x61, 0x61, 0x62, 0x62, 0x63, 0x63, 0x00,
                    0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00
                } }
        },
        {
            { ".", 15U, 2, 0U, true, false, false, 0, 0 },
            { "ae", 7U, 1, 0U, true, true, false, 0, 0 },
            { "al", 0U, 1, 0U, true, true, false, 0, 0 },
            { "be", 7U, 2, 0U, true, true, false, 0, 0 },
            { "bl", 0U, 2, 0U, true, true, false, 0, 0 },
            { "z", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "z1", 7U, 1, 0U, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    /* 78 - next test of sections' arithmetics */
    {   R"ffDXD(.amd
            .kernel a
            .ascii "xx1"
al:         .ascii "aaabbccdd"
ae:
            .kernel b
            .ascii "x1212"
bl:         .ascii "bbcc"
be:
            .kernel c
            .ascii "1234355667"
cl:         .ascii "abbcc"
ce:
            x0 = ae-al
            x1 = 5-(-al)
            x2 = al-7
            x3 = ce-al-be+ae+bl-cl+9
            x4 = (-21*al+21*bl-7*cl)  - ((ae*7 - 12*bl)*-3 - 8*be + 7*(-ce-bl))
            x5 = (ae<<5 + bl<<2) - al*32 - be*4
            y00 = al-bl==al-bl
            y01 = al+cl+bl!=cl+al+bl
            y02 = al+cl+bl<cl+al+bl
            y03 = al+cl+bl>cl+al+bl
            y04 = al+cl+bl<=cl+al+bl
            y05 = al+cl+bl>=cl+al+bl
            y06 = al+cl+bl<@cl+al+bl
            y07 = al+cl+bl>@cl+al+bl
            y08 = al+cl+bl<=@cl+al+bl
            y09 = al+cl+bl>=@cl+al+bl
            
            y12 = al+cl+bl<cl+al+bl+99
            y13 = al+cl+bl>cl+al+bl+99
            y14 = al+cl+bl<=cl+al+bl+99
            y15 = al+cl+bl>=cl+al+bl+99
            y16 = al+cl+bl<@cl+al+bl+99
            y17 = al+cl+bl>@cl+al+bl+99
            y18 = al+cl+bl<=@cl+al+bl+99
            y19 = al+cl+bl>=@cl+al+bl+99
            
            y22 = al+cl+bl<cl+al+bl-99
            y23 = al+cl+bl>cl+al+bl-99
            y24 = al+cl+bl<=cl+al+bl-99
            y25 = al+cl+bl>=cl+al+bl-99
            y26 = al+cl+bl<@cl+al+bl-99
            y27 = al+cl+bl>@cl+al+bl-99
            y28 = al+cl+bl<=@cl+al+bl-99
            y29 = al+cl+bl>=@cl+al+bl-99
            z0 = al-ae ? bl : ce)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { "a", "b", "c" },
        {
            { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } },
            { ".text", 0, AsmSectionType::CODE,
                {   0x78, 0x78, 0x31, 0x61, 0x61, 0x61, 0x62, 0x62,
                    0x63, 0x63, 0x64, 0x64 } },
            { ".text", 1, AsmSectionType::CODE,
                { 0x78, 0x31, 0x32, 0x31, 0x32, 0x62, 0x62, 0x63, 0x63 } },
            { ".text", 2, AsmSectionType::CODE,
                {   0x31, 0x32, 0x33, 0x34, 0x33, 0x35, 0x35, 0x36,
                    0x36, 0x37, 0x61, 0x62, 0x62, 0x63, 0x63 } }
        },
        {
            { ".", 15U, 3, 0U, true, false, false, 0, 0 },
            { "ae", 12U, 1, 0U, true, true, false, 0, 0 },
            { "al", 3U, 1, 0U, true, true, false, 0, 0 },
            { "be", 9U, 2, 0U, true, true, false, 0, 0 },
            { "bl", 5U, 2, 0U, true, true, false, 0, 0 },
            { "ce", 15U, 3, 0U, true, true, false, 0, 0 },
            { "cl", 10U, 3, 0U, true, true, false, 0, 0 },
            { "x0", 9U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x1", 8U, 1, 0U, true, false, false, 0, 0 },
            { "x2", 18446744073709551612U, 1, 0U, true, false, false, 0, 0 },
            { "x3", 19U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x4", 256U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x5", 272U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y00", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y01", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y02", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y03", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y04", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y05", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y06", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y07", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y08", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y09", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y12", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y13", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y14", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y15", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y16", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y17", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y18", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y19", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y22", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y23", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y24", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y25", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y26", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y27", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y28", 18446744073709551615U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y29", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "z0", 5U, 2, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 79 - error of section arithmetics */
    {   R"ffDXD(            .amd
            .kernel a
            .ascii "xx1"
al:         .ascii "aaabbccdd"
ae:
            .kernel b
            .ascii "x1212"
bl:         .ascii "bbcc"
be:
            .kernel c
            .ascii "1234355667"
cl:         .ascii "abbcc"
ce:
            e0 = al-be+be*7
            e0 = ~al
            e0 = (-23*al+21*bl-7*cl)  - ((ae*7 - 12*bl)*-3 - 8*be + 7*(-ce-bl))
            e0 = (ae<<5 + bl<<2) - al*32 - be*7
            e0 = al>>7
            e0 = al/7
            e0 = al%7
            e0 = al>>>7
            e0 = al//6
            e0 = al%%6
            e0 = al|6
            e0 = al&6
            e0 = al^6
            e0 = al&&6
            e0 = al||6
            e0 = al!6
            e0 = al==7
            e0 = al!=7
            e0 = al<7
            e0 = al>7
            e0 = al<=7
            e0 = al>=7
            e0 = al<@7
            e0 = al>@7
            e0 = al<=@7
            e0 = al>=@7
            z0 = al-bl ? bl : ce)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { "a", "b", "c" },
        {
            { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } },
            { ".text", 0, AsmSectionType::CODE,
                {   0x78, 0x78, 0x31, 0x61, 0x61, 0x61, 0x62, 0x62,
                    0x63, 0x63, 0x64, 0x64 } },
            { ".text", 1, AsmSectionType::CODE,
                { 0x78, 0x31, 0x32, 0x31, 0x32, 0x62, 0x62, 0x63, 0x63 } },
            { ".text", 2, AsmSectionType::CODE,
                {   0x31, 0x32, 0x33, 0x34, 0x33, 0x35, 0x35, 0x36,
                    0x36, 0x37, 0x61, 0x62, 0x62, 0x63, 0x63 } }
        },
        {
            { ".", 15U, 3, 0U, true, false, false, 0, 0 },
            { "ae", 12U, 1, 0U, true, true, false, 0, 0 },
            { "al", 3U, 1, 0U, true, true, false, 0, 0 },
            { "be", 9U, 2, 0U, true, true, false, 0, 0 },
            { "bl", 5U, 2, 0U, true, true, false, 0, 0 },
            { "ce", 15U, 3, 0U, true, true, false, 0, 0 },
            { "cl", 10U, 3, 0U, true, true, false, 0, 0 },
            { "e0", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "z0", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 }
        }, false, 
R"ffDXD(test.s:14:18: Error: Only one relative=1 (section) can be result of expression
test.s:15:18: Error: Only one relative=1 (section) can be result of expression
test.s:16:18: Error: Only one relative=1 (section) can be result of expression
test.s:17:18: Error: Only one relative=1 (section) can be result of expression
test.s:18:18: Error: Shift right is not allowed for any relative value
test.s:19:18: Error: Signed division is not allowed for any relative value
test.s:20:18: Error: Signed Modulo is not allowed for any relative value
test.s:21:18: Error: Signed shift right is not allowed for any relative value
test.s:22:18: Error: Division is not allowed for any relative value
test.s:23:18: Error: Modulo is not allowed for any relative value
test.s:24:18: Error: Binary OR is not allowed for any relative value
test.s:25:18: Error: Binary AND is not allowed for any relative value
test.s:26:18: Error: Binary XOR is not allowed for any relative value
test.s:27:18: Error: Logical AND is not allowed for any relative value
test.s:28:18: Error: Logical OR is not allowed for any relative value
test.s:29:18: Error: Binary ORNOT is not allowed for any relative value
test.s:30:18: Error: For comparisons two values must have this same relatives!
test.s:31:18: Error: For comparisons two values must have this same relatives!
test.s:32:18: Error: For comparisons two values must have this same relatives!
test.s:33:18: Error: For comparisons two values must have this same relatives!
test.s:34:18: Error: For comparisons two values must have this same relatives!
test.s:35:18: Error: For comparisons two values must have this same relatives!
test.s:36:18: Error: For comparisons two values must have this same relatives!
test.s:37:18: Error: For comparisons two values must have this same relatives!
test.s:38:18: Error: For comparisons two values must have this same relatives!
test.s:39:18: Error: For comparisons two values must have this same relatives!
test.s:40:18: Error: Choice is not allowed for first relative value
)ffDXD", ""
    },
    /* 80 - relatives inside '.eqv' expressions */
    {   R"ffDXD(            .amd
            .kernel a
            .ascii "xx1"
al:         .ascii "aaabbccdd"
ae:
            .kernel b
            .ascii "x1212"
bl:         .ascii "bbcc"
be:
            .kernel c
            .ascii "1234355667"
cl:         .ascii "abbcc"
ce:
            .eqv v0, cl-ce
            .eqv v1, be+v0
            .eqv v2, 5+v0
            x0 = v1-v2
            x1 = v1+7)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { "a", "b", "c" },
        {
            { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } },
            { ".text", 0, AsmSectionType::CODE,
                {   0x78, 0x78, 0x31, 0x61, 0x61, 0x61, 0x62, 0x62,
                    0x63, 0x63, 0x64, 0x64 } },
            { ".text", 1, AsmSectionType::CODE,
                { 0x78, 0x31, 0x32, 0x31, 0x32, 0x62, 0x62, 0x63, 0x63 } },
            { ".text", 2, AsmSectionType::CODE,
                {   0x31, 0x32, 0x33, 0x34, 0x33, 0x35, 0x35, 0x36,
                    0x36, 0x37, 0x61, 0x62, 0x62, 0x63, 0x63 } }
        },
        {
            { ".", 15U, 3, 0U, true, false, false, 0, 0 },
            { "ae", 12U, 1, 0U, true, true, false, 0, 0 },
            { "al", 3U, 1, 0U, true, true, false, 0, 0 },
            { "be", 9U, 2, 0U, true, true, false, 0, 0 },
            { "bl", 5U, 2, 0U, true, true, false, 0, 0 },
            { "ce", 15U, 3, 0U, true, true, false, 0, 0 },
            { "cl", 10U, 3, 0U, true, true, false, 0, 0 },
            { "v0", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "v1", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "v2", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x0", 4U, 2, 0U, true, false, false, 0, 0 },
            { "x1", 11U, 2, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 81 - raw code section tests */
    {   R"ffDXD(            .rawcode
            .text
            .byte 1,2,2,3,4
            .main
            .byte 0xf,0xf0,0xdd
            .section .text
            .byte 12,14)ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 0x01, 0x02, 0x02, 0x03, 0x04, 0x0f, 0xf0, 0xdd, 0x0c, 0x0e } } },
        { { ".", 10U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 82 - raw code section tests (errors) */
    {   R"ffDXD(            .rawcode
            .data
            .rodata
            .section .notes
            .kernel test1
            .main)ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE, { } } },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:2:13: Error: Only section '.text' can be in raw code\n"
        "test.s:3:13: Error: Only section '.text' can be in raw code\n"
        "test.s:4:13: Error: Only section '.text' can be in raw code\n"
        "test.s:5:13: Error: In rawcode defining kernels is not allowed\n", ""
    },
    /* 83 - Gallium format (sections) */
    {   R"ffDXD(            .gallium
            .ascii "some text"
            .main
            .rodata
            .ascii "some data"
            .text
aa1: bb2:   # kernel labels
            .byte 220
            .rodata
            .byte 211
            .section .comment
            .string "this is comment"
            .kernel aa1
            .kernel bb2
            .main
            .string "next comment"
            .kernel aa1
            .kernel bb2
            .text
            .ascii "next text"
            .kernel bb2
            .globaldata
            .ascii "endofdata"
            .section .test1
            .string "this is test"
            .section .test1
            .string "aa")ffDXD",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false, { "aa1", "bb2" },
        {
            { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
                {
                    0x73, 0x6f, 0x6d, 0x65, 0x20, 0x74, 0x65, 0x78,
                    0x74, 0xdc, 0x6e, 0x65, 0x78, 0x74, 0x20, 0x74,
                    0x65, 0x78, 0x74
                } },
            { ".rodata", ASMKERN_GLOBAL, AsmSectionType::DATA,
                {
                    0x73, 0x6f, 0x6d, 0x65, 0x20, 0x64, 0x61, 0x74,
                    0x61, 0xd3, 0x65, 0x6e, 0x64, 0x6f, 0x66, 0x64,
                    0x61, 0x74, 0x61
                } },
            { ".comment", ASMKERN_GLOBAL, AsmSectionType::GALLIUM_COMMENT,
                {
                    0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
                    0x63, 0x6f, 0x6d, 0x6d, 0x65, 0x6e, 0x74, 0x00,
                    0x6e, 0x65, 0x78, 0x74, 0x20, 0x63, 0x6f, 0x6d,
                    0x6d, 0x65, 0x6e, 0x74, 0x00
                } },
            { nullptr, 0, AsmSectionType::CONFIG, { } },
            { nullptr, 1, AsmSectionType::CONFIG, { } },
            { ".test1", ASMKERN_GLOBAL, AsmSectionType::EXTRA_SECTION,
                { 0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
                  0x74, 0x65, 0x73, 0x74, 0x00, 0x61, 0x61, 0x00 } }
        },
        {
            { ".", 16U, 5, 0U, true, false, false, 0, 0 },
            { "aa1", 9U, 0, 0U, true, true, false, 0, 0 },
            { "bb2", 9U, 0, 0U, true, true, false, 0, 0 }
        },
        true, "", ""
    },
    /* 84 - Gallium format (sections, errors) */
    {   R"ffDXD(            .gallium
            .kernel aa22
            .int 24,5,6
            .fill 10,4,0xff
            .p2align 4)ffDXD",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false, { "aa22" },
        {
            { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE, { } },
            { nullptr, 0, AsmSectionType::CONFIG, { } }
        },
        { { ".", 0U, 1, 0U, true, false, false, 0, 0 } },
        false, "test.s:3:13: Error: Writing data into non-writeable section is illegal\n"
        "test.s:4:13: Error: Writing data into non-writeable section is illegal\n"
        "test.s:5:13: Error: Change output counter inside non-addressable "
        "section is illegal\n", ""
    },
    /* AmdFormat (sections) */
    {   R"ffDXD(            .amd
            .globaldata
            .ascii "aaabbb"
            .section .ulu
            .ascii "server"
            .kernel aaa1
            .section .ulu2
            .ascii "linux"
            .main
            .section .ulu
            .ascii "xserver"
            .kernel aaa1
            .ascii "oops"
            .dATa
            .string "dd777dd"
            .section .ulu
            .ascii "uline"
            .main
            .ascii "..xXx.."
            .kernel aaa1
            .ascii "nfx"
            
            .kernel bxv
            .asciz "zeroOne"
            .roDATA
            .asciz "zeroTwo"
            .kernel aaa1
            .ascii "33"
            .mAIn
            .asciz "top1"
            .globaldata
            .ascii "nextType"
            .kernel aaa1
            .tEXt
            .main
            .ascii "yetAnother"
            .kernel aaa1
            .asciz "burger"
            .main
            .kernel aaa1
            .asciz "radeon"
            .kernel bxv
            .asciz "fury")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { "aaa1", "bxv" },
        {
            { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
                {   0x61, 0x61, 0x61, 0x62, 0x62, 0x62, 0x6e, 0x65,
                    0x78, 0x74, 0x54, 0x79, 0x70, 0x65, 0x79, 0x65,
                    0x74, 0x41, 0x6e, 0x6f, 0x74, 0x68, 0x65, 0x72 } },
            { ".ulu", ASMKERN_GLOBAL, AsmSectionType::EXTRA_SECTION,
                {   0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x78, 0x73,
                    0x65, 0x72, 0x76, 0x65, 0x72, 0x2e, 0x2e, 0x78,
                    0x58, 0x78, 0x2e, 0x2e, 0x74, 0x6f, 0x70, 0x31, 0x00 } },
            { ".text", 0, AsmSectionType::CODE,
                {   0x62, 0x75, 0x72, 0x67, 0x65, 0x72, 0x00, 0x72,
                    0x61, 0x64, 0x65, 0x6f, 0x6e, 0x00 } },
            { ".ulu2", 0, AsmSectionType::EXTRA_SECTION,
                {   0x6c, 0x69, 0x6e, 0x75, 0x78, 0x6f, 0x6f, 0x70, 0x73 } },
            { ".data", 0, AsmSectionType::DATA,
                {   0x64, 0x64, 0x37, 0x37, 0x37, 0x64, 0x64, 0x00 } },
            { ".ulu", 0, AsmSectionType::EXTRA_SECTION,
                { 0x75, 0x6c, 0x69, 0x6e, 0x65, 0x6e, 0x66, 0x78, 0x33, 0x33 } },
            { ".text", 1, AsmSectionType::CODE,
                { 0x7a, 0x65, 0x72, 0x6f, 0x4f, 0x6e, 0x65, 0x00 } },
            { ".rodata", 1, AsmSectionType::EXTRA_SECTION,
                {   0x7a, 0x65, 0x72, 0x6f, 0x54, 0x77, 0x6f, 0x00,
                    0x66, 0x75, 0x72, 0x79, 0x00 } }
        },
        { { ".", 13U, 7, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* .gpu/.arch tests */
    {   ".amd\n.gpu Tahiti\n",
        BinaryFormat::AMD, GPUDeviceType::TAHITI, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.gpu CapeVerde\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.gpu Bonaire\n",
        BinaryFormat::AMD, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.gpu Mullins\n",
        BinaryFormat::AMD, GPUDeviceType::MULLINS, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.gpu Hainan\n",
        BinaryFormat::AMD, GPUDeviceType::HAINAN, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.gpu Iceland\n",
        BinaryFormat::AMD, GPUDeviceType::ICELAND, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.arch GCN1.0\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.arch GCN1.1\n",
        BinaryFormat::AMD, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.arch GCN1.2\n",
        BinaryFormat::AMD, GPUDeviceType::TONGA, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* arch gpu conditionals */
    {   ".amd\n.gpu Tahiti\n.ifarch GCN1.0\n.print \"GCN1.0\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::TAHITI, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "GCN1.0\n",
    },
    {   ".amd\n.gpu Hawaii\n.ifarch GCN1.0\n.print \"GCN1.0\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::HAWAII, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.gpu Tonga\n.ifarch GCN1.2\n.print \"GCN1.2\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::TONGA, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "GCN1.2\n",
    },
    {   ".amd\n.gpu Bonaire\n.ifnarch GCN1.2\n.print \"notGCN1.2\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "notGCN1.2\n",
    },
    /* gpu conditionals */
    {   ".amd\n.gpu Pitcairn\n.ifgpu Pitcairn\n.print \"pitcairn\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::PITCAIRN, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "pitcairn\n",
    },
    {   ".amd\n.gpu Iceland\n.ifgpu Pitcairn\n.print \"pitcairn\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::ICELAND, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.gpu Iceland\n.ifngpu Pitcairn\n.print \"notpitcairn\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::ICELAND, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "notpitcairn\n",
    },
    /* formats */
    {   ".amd\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "catalystfmt\n",
    },
    {   ".format amd\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "catalystfmt\n",
    },
    {   ".gallium\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".format gallium\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".amd\n.ifnfmt catalyst\n.print \"notcatalystfmt\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    {   ".gallium\n.ifnfmt catalyst\n.print \"notcatalystfmt\"\n.endif\n",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "notcatalystfmt\n",
    },
    /* bitness conditional */
    {   ".amd\n.32bit\n.if32\n.print \"is32bit\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "is32bit\n",
    },
    {   ".amd\n.64bit\n.if32\n.print \"is32bit\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, true,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* unresolved expression and resolved expression */
    {   R"ffDXD(sym3 = sym7*sym4
        sym3 = 17
        sym7 = 1
        sym4 = 3)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "sym3", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym4", 3, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym7", 1, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* register ranges assignment */
    {   R"ffDXD(sym1 = 123
        sym1 = %v[12:15]
        sym2 = x*21
        sym2 = %v[120:125]
        sym3 = %sym2[1:3]
        sym4 = %v[120:125]
        sym4 = 12
        .set sym5, %v[120:125]
        .equiv sym6, %v[120:125]
        x=43)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA } },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "sym1", (256+12) | ((256+16ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "sym2", (256+120) | ((256+126ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "sym3", (256+121) | ((256+124ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "sym4", 12, ASMSECT_ABS, 0, true, false, 0, 0 },
            { "sym5", (256+120) | ((256+126ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "sym6", (256+120) | ((256+126ULL)<<32), ASMSECT_ABS, 0,
                true, true, false, 0, 0, true },
            { "x", 43, ASMSECT_ABS, 0, true, false, 0, 0 }
        }, true, "", ""
    },
    {   R"ffDXD(. = %s3 # error
        .equiv sym6, %v[120:125]
        .equiv sym6, %v[120:125] # error
        .eqv sym7, %v[120:126]
        .eqv sym7, %v[120:127] # error
        .size sym6 , 4
        .global sym6, sym7
        .local sym6, sym7
        .weak sym6, sym7
        )ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA } },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "sym6", (256+120) | ((256+126ULL)<<32), ASMSECT_ABS, 0,
                true, true, false, 0, 0, true },
            { "sym7", (256+120) | ((256+127ULL)<<32), ASMSECT_ABS, 0,
                true, true, false, 0, 0, true },
        }, false, R"ffDXD(test.s:1:1: Error: Symbol '.' requires a resolved expression
test.s:3:16: Error: Symbol 'sym6' is already defined
test.s:5:14: Error: Symbol 'sym7' is already defined
test.s:6:15: Error: Symbol must not be register symbol
test.s:7:17: Error: Symbol must not be register symbol
test.s:7:23: Error: Symbol must not be register symbol
test.s:8:16: Error: Symbol must not be register symbol
test.s:8:22: Error: Symbol must not be register symbol
test.s:9:15: Error: Symbol must not be register symbol
test.s:9:21: Error: Symbol must not be register symbol
)ffDXD", ""
    },
    /* altmacro */
    {   R"ffDXD(.altmacro
        .macro test a,b,c
        .byte a,b,c,a+b
        .endm
        test 2,4,7
        test a,b,c
        a=10
        b=54
        c=26
        )ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 2, 4, 7, 6, 10, 54, 26, 64} } },
        {
            { ".", 8U, 0, 0U, true, false, false, 0, 0 },
            { "a", 10U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "b", 54U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "c", 26U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    {   R"ffDXD(.altmacro
        .macro test a,b,c
        local al,bl,cl,dl
al:     .byte a
bl:     .byte b
cl:     .byte c
dl:     .byte a+b
        .endm
        test 2,4,7
        test a,b,c
        a=10
        b=54
        c=26
        )ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 2, 4, 7, 6, 10, 54, 26, 64} } },
        {
            { ".", 8U, 0, 0U, true, false, false, 0, 0 },
            { ".LL0", 0U, 0, 0U, true, true, false, 0, 0 },
            { ".LL1", 1U, 0, 0U, true, true, false, 0, 0 },
            { ".LL2", 2U, 0, 0U, true, true, false, 0, 0 },
            { ".LL3", 3U, 0, 0U, true, true, false, 0, 0 },
            { ".LL4", 4U, 0, 0U, true, true, false, 0, 0 },
            { ".LL5", 5U, 0, 0U, true, true, false, 0, 0 },
            { ".LL6", 6U, 0, 0U, true, true, false, 0, 0 },
            { ".LL7", 7U, 0, 0U, true, true, false, 0, 0 },
            { "a", 10U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "b", 54U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "c", 26U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    {
        R"ffDXD(.altmacro
.macro test1 v1 v2
xx: yyx:    local t1 , t6
    .string "v1 v2 vxx t1  \t6 t9 "
.endm
someval = 221
        test1 %-(12+someval*2),  <aaa!!>
     )ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 
                0x2d, 0x34, 0x35, 0x34, 0x20, 0x61, 0x61, 0x61,
                0x21, 0x20, 0x76, 0x78, 0x78, 0x20, 0x2e, 0x4c,
                0x4c, 0x30, 0x20, 0x20, 0x2e, 0x4c, 0x4c, 0x31,
                0x20, 0x74, 0x39, 0x20, 0x00
            } } },
        {
            { ".", 29U, 0, 0U, true, false, false, 0, 0 },
            { "someval", 221U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "xx", 0U, 0, 0U, true, true, false, 0, 0 },
            { "yyx", 0U, 0, 0U, true, true, false, 0, 0 }
        },
        true, "", ""
    },
    {   R"ffDXD(.altmacro
        .macro test a$xx,_b.,$$c
        .byte a$xx,_b.,$\
$c,a$x\
x+_b.
        .endm
        test 2,4,7
        test a,b,c
        a=10
        b=54
        c=26
        )ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 2, 4, 7, 6, 10, 54, 26, 64} } },
        {
            { ".", 8U, 0, 0U, true, false, false, 0, 0 },
            { "a", 10U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "b", 54U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "c", 26U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    {
        R"ffDXD(.altmacro
        .macro test v1
a: b: _c:    local a a .\
. ..\
 d a
        .endm
        test 44)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "_c", 0U, 0, 0U, true, true, false, 0, 0 },
            { "a", 0U, 0, 0U, true, true, false, 0, 0 },
            { "b", 0U, 0, 0U, true, true, false, 0, 0 }
        },
        false, R"ffDXD(In macro substituted from test.s:7:9:
test.s:3:22: Error: Name 'a' was already used by local or macro argument
In macro substituted from test.s:7:9:
test.s:4:3: Error: Name '..' was already used by local or macro argument
In macro substituted from test.s:7:9:
test.s:5:4: Error: Name 'a' was already used by local or macro argument
)ffDXD", ""
    },
    {
        R"ffDXD(.altmacro
.macro testw vx
local sx2
        .string "<vx>sx2"
.endm

.macro testx vx
        #.noaltmacro
        testw aaarrggg
x\():    local ad
        .string "\vx (ad)"
Local sx
        .string "sx\()___"
.endm

        testx "aaaax!! bbb!!")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x3c, 0x61, 0x61, 0x61, 0x72, 0x72, 0x67, 0x67,
                0x67, 0x3e, 0x2e, 0x4c, 0x4c, 0x30, 0x00, 0x61,
                0x61, 0x61, 0x61, 0x78, 0x21, 0x20, 0x62, 0x62,
                0x62, 0x21, 0x20, 0x28, 0x2e, 0x4c, 0x4c, 0x31,
                0x29, 0x00, 0x2e, 0x4c, 0x4c, 0x32, 0x5f, 0x5f,
                0x5f, 0x00,
            } } },
        {
            { ".", 42U, 0, 0U, true, false, false, 0, 0 },
            { "x", 15U, 0, 0U, true, true, false, 0, 0 }
        },
        true, "", ""
    },
    {   R"ffDXD(.rept 1
        .int 0xaaddd
loop:   .rept 10
        .int 0x10
        .endr
        .int 0xaaaa

        .endr            
            .int 12343)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 
                0xdd, 0xad, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0xaa, 0xaa, 0x00, 0x00,
                0x37, 0x30, 0x00, 0x00
            } } },
        {
            { ".", 52U, 0, 0U, true, false, false, 0, 0 },
            { "loop", 4U, 0, 0U, true, true, false, 0, 0 }
        },
        true, "", ""
    },
    {   R"ffDXD(.irpc x, "abc"
        .int 0xaaddd
\x\()loop\x: .rept 3
        .int 0x10
        .endr
        .int 0xaaaa

        .endr            
            .int 12343)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 
                0xdd, 0xad, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                0xaa, 0xaa, 0x00, 0x00, 0xdd, 0xad, 0x0a, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0xaa, 0xaa, 0x00, 0x00,
                0xdd, 0xad, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                0xaa, 0xaa, 0x00, 0x00, 0x37, 0x30, 0x00, 0x00
            } } },
        {
            { ".", 64U, 0, 0U, true, false, false, 0, 0 },
            { "aloopa", 4U, 0, 0U, true, true, false, 0, 0 },
            { "bloopb", 24U, 0, 0U, true, true, false, 0, 0 },
            { "cloopc", 44U, 0, 0U, true, true, false, 0, 0 }
        },
        true, "", ""
    },
    /* macro name case sensitives */
    {   R"ffDXD(.macrocase
            .macro one1
            .byte 2
            .endm
            one1
            ONE1
            .nomacrocase
            .macro one2
            .byte 4
            .endm
            .macro ONE2
            .byte 5
            .endm
            one2
            ONE2
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                2, 2, 4, 5
            } } },
        { { ".", 4U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /*
     * Scopes tests
     */
    /* 1 standard symbol assignment */
    {   R"ffDXD(sym1 = 7
        .scope ala
        sym2 = 81
        sym3 = ::beata::sym7*sym4
        sym4 = ::beata::sym5*::beata::sym6+::beata::sym7 - sym1
        .ends
        .scope beata
        sym5 = 17
        sym6 = 43
        sym7 = 91
        .ends)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "ala::sym2", 81, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::sym3", 91*(17*43+91-7), ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::sym4", 17*43+91-7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beata::sym5", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beata::sym6", 43, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beata::sym7", 91, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym1", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 1 standard symbol assignment 2 */
    {   R"ffDXD(sym1 = 7
        .scope beata
        sym5 = 17
        sym6 = 43
        sym7 = 91
        .ends
        .scope ala
        sym2 = 81
        sym3 = beata::sym7*sym4
        sym4 = beata::sym5*beata::sym6+beata::sym7 - sym1
        .ends
        )ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "ala::sym2", 81, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::sym3", 91*(17*43+91-7), ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::sym4", 17*43+91-7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beata::sym5", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beata::sym6", 43, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beata::sym7", 91, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym1", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 2 - visibility */
    {
        R"ffDXD(.rawcode
            .byte 0
            sym1 = 5
            .byte sym1
            .scope
                sym1 = 7
                .byte sym1
                .scope
                    sym1 = 9
                    .byte sym1
                .ends
                .byte sym1
            .ends
            .byte sym1
            
            sym1 = 15
            .byte sym1
            .scope ala
                sym1 = 17
                .byte sym1
                .scope beta
                    sym1 = 19
                    .byte sym1
                .ends
                .byte sym1
            .ends
            .byte sym1
            .byte ala::beta::sym1
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 0, 5, 7, 9, 7, 5, 15, 17, 19, 17, 15, 19 } } },
        {
            { ".", 12, 0, 0, true, false, false, 0, 0 },
            { "ala::beta::sym1", 19, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::sym1", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym1", 15, ASMSECT_ABS, 0, true, false, false, 0, 0 },
        }, true, "", ""
    },
    { nullptr }
};
