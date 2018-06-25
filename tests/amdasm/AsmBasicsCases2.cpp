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
#include <sstream>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "../TestUtils.h"
#include "AsmBasics.h"

using namespace CLRX;

const AsmTestCase asmTestCases2Tbl[] =
{
    /* 0 - undef test */
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
    /* 1 - include test 1 */
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
    /* 2 - include test 2 */
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
    /* 3 - failed include */
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
    /* 4 - incbin */
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
    /* 5 - incbin (choose offset and size) */
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
    /* 6 - failed incbin */
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
    /* 7 - absolute section and errors */
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
    /* 8 */
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
    /* 9 - empty lines inside macro,repeats */
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
    /* 10 - IRP and IRPC */
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
    /* 11 - section arithmetics */
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
    /* 12 - next test of sections' arithmetics */
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
            
            y30 = al&0
            y31 = al&-1
            y32 = 0&al
            y33 = -1&al
            y34 = al|0
            y35 = al|-1
            y36 = 0|al
            y37 = -1|al
            y38 = al^0
            y39 = 0^al
            y40 = al!0
            y41 = al!-1
            y42 = al&&0
            y43 = 0&&al
            y44 = al||1
            y45 = 2||al
            y46 = 0!-al
            y47 = -1!al
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
            { "y30", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y31", 3U, 1, 0U, true, false, false, 0, 0 },
            { "y32", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y33", 3U, 1, 0U, true, false, false, 0, 0 },
            { "y34", 3U, 1, 0U, true, false, false, 0, 0 },
            { "y35", 0xffffffffffffffffULL, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y36", 3U, 1, 0U, true, false, false, 0, 0 },
            { "y37", 0xffffffffffffffffULL, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y38", 3U, 1, 0U, true, false, false, 0, 0 },
            { "y39", 3U, 1, 0U, true, false, false, 0, 0 },
            { "y40", 0xffffffffffffffffULL, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y41", 3U, 1, 0U, true, false, false, 0, 0 },
            { "y42", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y43", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y44", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y45", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y46", 2U, 1, 0U, true, false, false, 0, 0 },
            { "y47", 0xffffffffffffffffULL, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "z0", 5U, 2, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 13 - error of section arithmetics */
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
            e0 = al||0
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
            e0 = 2!al
            e0 = 6|al
            e0 = 6&al
            e0 = 6^al
            e0 = 6&&al
            e0 = 0||al
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
test.s:24:18: Error: Binary OR is not allowed for any relative value except special cases
test.s:25:18: Error: Binary AND is not allowed for any relative value except special cases
test.s:26:18: Error: Binary XOR is not allowed for any relative value except special cases
test.s:27:18: Error: Logical AND is not allowed for any relative value except special cases
test.s:28:18: Error: Logical OR is not allowed for any relative value except special cases
test.s:29:18: Error: Binary ORNOT is not allowed for any relative value except special cases
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
test.s:40:18: Error: Binary ORNOT is not allowed for any relative value except special cases
test.s:41:18: Error: Binary OR is not allowed for any relative value except special cases
test.s:42:18: Error: Binary AND is not allowed for any relative value except special cases
test.s:43:18: Error: Binary XOR is not allowed for any relative value except special cases
test.s:44:18: Error: Logical AND is not allowed for any relative value except special cases
test.s:45:18: Error: Logical OR is not allowed for any relative value except special cases
test.s:46:18: Error: Choice is not allowed for first relative value
)ffDXD", ""
    },
    /* 14 - relatives inside '.eqv' expressions */
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
    /* 15 - raw code section tests */
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
    /* 16 - raw code section tests (errors) */
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
    /* 17 - Gallium format (sections) */
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
    /* 18 - Gallium format (sections, errors) */
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
    /* 19 - AmdFormat (sections) */
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
    /* 20 - .gpu/.arch tests */
    {   ".amd\n.gpu Tahiti\n",
        BinaryFormat::AMD, GPUDeviceType::TAHITI, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 21 - .gpu/.arch tests */
    {   ".amd\n.gpu CapeVerde\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 22 - .gpu/.arch tests */
    {   ".amd\n.gpu Bonaire\n",
        BinaryFormat::AMD, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 23 - .gpu/.arch tests */
    {   ".amd\n.gpu Mullins\n",
        BinaryFormat::AMD, GPUDeviceType::MULLINS, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 24 - .gpu/.arch tests */
    {   ".amd\n.gpu Hainan\n",
        BinaryFormat::AMD, GPUDeviceType::HAINAN, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 25 - .gpu/.arch tests */
    {   ".amd\n.gpu Iceland\n",
        BinaryFormat::AMD, GPUDeviceType::ICELAND, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 26 - .gpu/.arch tests */
    {   ".amd\n.arch GCN1.0\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 27 - .gpu/.arch tests */
    {   ".amd\n.arch GCN1.1\n",
        BinaryFormat::AMD, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 28 - .gpu/.arch tests */
    {   ".amd\n.arch GCN1.2\n",
        BinaryFormat::AMD, GPUDeviceType::ICELAND, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 29 - arch gpu conditionals */
    {   ".amd\n.gpu Tahiti\n.ifarch GCN1.0\n.print \"GCN1.0\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::TAHITI, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "GCN1.0\n",
    },
    /* 30 - arch gpu conditionals */
    {   ".amd\n.gpu Hawaii\n.ifarch GCN1.0\n.print \"GCN1.0\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::HAWAII, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 31 - arch gpu conditionals */
    {   ".amd\n.gpu Tonga\n.ifarch GCN1.2\n.print \"GCN1.2\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::TONGA, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "GCN1.2\n",
    },
    /* 32 - arch gpu conditionals */
    {   ".amd\n.gpu Bonaire\n.ifnarch GCN1.2\n.print \"notGCN1.2\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "notGCN1.2\n",
    },
    /* 33 - gpu conditionals */
    {   ".amd\n.gpu Pitcairn\n.ifgpu Pitcairn\n.print \"pitcairn\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::PITCAIRN, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "pitcairn\n",
    },
    /* 34 - gpu conditionals */
    {   ".amd\n.gpu Iceland\n.ifgpu Pitcairn\n.print \"pitcairn\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::ICELAND, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 35 - gpu conditionals */
    {   ".amd\n.gpu Iceland\n.ifngpu Pitcairn\n.print \"notpitcairn\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::ICELAND, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "notpitcairn\n",
    },
    /* 36 - formats */
    {   ".amd\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "catalystfmt\n",
    },
    /* 37 - formats */
    {   ".format amd\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "catalystfmt\n",
    },
    /* 38 - formats */
    {   ".gallium\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 39 - formats */
    {   ".format gallium\n.iffmt catalyst\n.print \"catalystfmt\"\n.endif\n",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 40 - formats */
    {   ".amd\n.ifnfmt catalyst\n.print \"notcatalystfmt\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 41 - formats */
    {   ".gallium\n.ifnfmt catalyst\n.print \"notcatalystfmt\"\n.endif\n",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "notcatalystfmt\n",
    },
    /* 42 - bitness conditional */
    {   ".amd\n.32bit\n.if32\n.print \"is32bit\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } },
        true, "", "is32bit\n",
    },
    /* 43 - bitness conditional */
    {   ".amd\n.64bit\n.if32\n.print \"is32bit\"\n.endif\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, true,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "",
    },
    /* 44 - unresolved expression and resolved expression */
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
    /* 45 - register ranges assignment */
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
    /* 46 */
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
    /* 47 - altmacro */
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
    /* 48 - altmacro */
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
    /* 49 - altmacro */
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
    /* 50 - altmacro */
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
    /* 51 - altmacro */
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
    /* 52 - altmacro */
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
    /* 53 - altmacro */
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
    /* 54 - altmacro */
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
    /* 55 - macro name case sensitives */
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
    /* 56 - standard symbol assignment */
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
    /* 57 - standard symbol assignment 2 */
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
    /* 58 - visibility */
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
    /* 59 - visibility 2 */
    {   R"ffDXD(.rawcode
            .byte 0
            sym1 = 1
            sym2 = 2
            sym3 = 3
            sym4 = 4
            .scope ala
                .scope x1
                    sym1 = 9
                    sym4 = 10
                    sym8 = 25
                    sym9 = 30
                .ends
                .scope x2
                    sym2 = 8
                    sym3 = 11
                    sym8 = 26
                    sym9 = 31
                .ends
                sym1 = 6
                sym3 = 7
            .ends
            .scope beta
                sym2 = 12
                .scope x1
                    sym1 = 14
                    sym4 = 15
                    sym8 = 27
                .ends
                .scope x2
                    sym2 = 16
                    sym3 = 17
                .ends
                .scope y2
                    sym2 = 19
                    sym3 = 20
                    sym4 = 22
                .ends
                sym3 = 13
            .ends
            .byte sym1,sym2,sym3,sym4
            .scope ala
                .byte sym1,sym2,sym3,sym4
                .scope x1
                    .byte sym1,sym2,sym3,sym4
                .ends
                .scope x2
                    .byte sym1,sym2,sym3,sym4
                .ends
            .ends
            .scope beta
                .byte sym1,sym2,sym3,sym4
                .scope x1
                    .byte sym1,sym2,sym3,sym4
                .ends
                .scope x2
                    .byte sym1,sym2,sym3,sym4
                .ends
                .scope y2
                    .byte sym1,sym2,sym3,sym4
                .ends
            .ends
            # using
            .using ::ala::x1
            .byte sym1,sym2,sym3,sym4,sym8
            .using ::ala::x2
            .byte sym1,sym2,sym3,sym4,sym8
            .unusing ::ala::x2
            .byte sym1,sym2,sym3,sym4,sym8
            .unusing ::ala::x1
            .using ::ala::x2
            .byte sym1,sym2,sym3,sym4,sym8
            .using ::ala::x1
            .byte sym1,sym2,sym3,sym4,sym8
            .using ::beta::x1
            .scope ala
                .using x2
                .byte sym1,sym2,sym3,sym4,sym8
                .using x1
                .byte sym1,sym2,sym3,sym4,sym8
                .unusing
            .ends
            .byte sym1,sym2,sym3,sym4,sym8
            .byte sym1,sym2,sym3,sym4,sym8,sym9
            .unusing
            .byte sym1,sym2,sym3,sym4,sym8,sym9
            
            # recursive finding by usings
            .scope ela
                .using ala::x1
                .using ala::x2
            .ends
            .scope newscope
                .using ela
            .ends
            .byte newscope::sym8, newscope::sym9
            .scope newscope
                .byte sym8, sym9
            .ends
            .byte ::newscope::sym8, ::newscope::sym9
            # recursive finding scopes through using
            .scope jula
                .using ::ala
            .ends
            .scope xela
                .using ::jula
            .ends
            .unusing
            .scope
                .using jula::x1
                .byte sym8, sym9
            .ends
            .scope
                .using xela::x2
                .byte sym8, sym9
            .ends
            .scope looper
                .using ::looper # !!!
                .byte somebody
            .ends
            .scope looper3
                .using ::looper2
            .ends
            .scope looper2
                .using ::looper3 # !!!
                .byte somebody2
            .ends
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 0, 1, 2, 3, 4,
                6, 2, 7, 4,
                9, 2, 7, 10,
                6, 8, 11, 4,
                1, 12, 13, 4,
                14, 12, 13, 15,
                1, 16, 17, 4,
                1, 19, 20, 22,
                // using
                1, 2, 3, 4, 25,
                1, 2, 3, 4, 26,
                1, 2, 3, 4, 25,
                1, 2, 3, 4, 26,
                1, 2, 3, 4, 25,
                6, 8, 7, 4, 26,
                6, 8, 7, 10, 25,
                1, 2, 3, 4, 27,
                1, 2, 3, 4, 27, 30,
                1, 2, 3, 4, 0, 0,
                // recursive using
                26, 31, 26, 31, 26, 31,
                25, 30, 26, 31, 0, 0
            } } },
        {
            { ".", 97, 0, 0, true, false, false, 0, 0 },
            { "ala::sym1", 6, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::sym3", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x1::sym1", 9, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x1::sym4", 10, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x1::sym8", 25, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x1::sym9", 30, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x2::sym2", 8, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x2::sym3", 11, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x2::sym8", 26, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::x2::sym9", 31, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::sym2", 12, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::sym3", 13, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::x1::sym1", 14, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::x1::sym4", 15, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::x1::sym8", 27, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::x2::sym2", 16, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::x2::sym3", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::y2::sym2", 19, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::y2::sym3", 20, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "beta::y2::sym4", 22, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "looper2::somebody2", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "looper::somebody", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "sym1", 1, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym2", 2, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym3", 3, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym4", 4, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym8", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "sym9", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 60 - scope finding */
    {   R"ffDXD(.rawcode
        .byte 0
        obj = 1
        .scope ala
            obj = 2
            .scope beta
                obj = 3
                obj4 = 28
                .scope ala
                    obj = 4
                .ends
                .scope ceta
                    obj = 5
                    obj4 = 26
                .ends
            .ends
            .scope ceta
                obj = 6
                obj2 = 23
                .scope beta
                    obj = 7
                    obj4 = 27
                .ends
                .scope ceta
                    obj = 8
                    obj2 = 24
                .ends
                .scope buru
                    obj = 9
                .ends
            .ends
            .scope linux
                obj = 10
                .scope wifi
                    obj = 11
                    obj1 = 22
                    obj5 = 30
                .ends
                .scope ala
                    obj3 = 25
                    obj = 12
                .ends
            .ends
        .ends
        .byte obj, ala::obj, ala::beta::obj, ala::beta::ala::obj, ala::beta::ceta::obj
        .byte ala::ceta::obj, ala::ceta::beta::obj, ala::ceta::ceta::obj
        .byte ala::ceta::buru::obj, ala::linux::obj, ala::linux::wifi::obj
        .byte ala::linux::ala::obj
        # inside scopes
        .scope ala
            .scope beta
                # linux::beta::obj created because not scope found by first scope name
                .byte ala::obj, beta::obj, ceta::obj, ceta::beta::obj, linux::ala::obj
                .byte ::ala::obj, ::obj
            .ends
            .scope ceta
                # linux::beta::obj created because not scope found by first scope name
                # from current scope
                .byte ala::obj, beta::obj, ceta::obj, ceta::beta::obj, ceta::ceta::obj
            .ends
            .scope new
                .byte ala::obj, beta::obj, ceta::obj, ceta::beta::obj, ceta::ceta::obj
            .ends
            .scope
                .byte ala::obj, beta::obj, ceta::obj, ceta::beta::obj, ceta::ceta::obj
                .byte ::ala::obj, linux::wifi::obj
                .scope ala
                    obj = 66
                    .byte obj
                .ends
            .ends
            .scope
                .scope ala
                    .byte obj
                .ends
            .ends
        .ends
        # with using
        .using ala::linux::wifi
        .byte obj1
        .scope new
            .using ala::ceta
            .byte obj2
            .using ala::ceta::ceta
            .byte obj2
            .scope
                .using ala::ceta::ceta
                .using ala::ceta
                .byte obj2
            .ends
            .scope
                .using ala::ceta::beta
                .using ala::beta::ceta
                .using ala::linux::ala
                .byte obj3, obj2, obj4, obj5
            .ends
        .ends
        .scope ala
            .using beta
            .byte obj4
            .scope ceta
                .using beta
                .byte obj4
                .unusing
                .byte obj4
            .ends
        .ends
        .unusing
        .using ::ala::beta::ceta
        .using ::ala::ceta::beta
        .using ::ala::beta::ceta
        .byte obj4
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
              4, 3, 5, 0, 12, 2, 1, 2, 7, 8, 0, 0,
              2, 3, 6, 7, 8, 2, 3, 6, 7, 8, 2, 11, 66, 2,
              // using
              22, 23, 24, 23, 25, 24, 26, 30,
              28, 27, 28, 26
            } } },
        {
            { ".", 51, 0, 0, true, false, false, 0, 0 },
            { "ala::beta::ala::obj", 4, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::beta::ceta::beta::obj", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "ala::beta::ceta::obj", 5, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::beta::ceta::obj4", 26, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::beta::obj", 3, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::beta::obj4", 28, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::ceta::beta::obj", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::ceta::beta::obj4", 27, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::ceta::buru::obj", 9, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::ceta::ceta::beta::obj", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "ala::ceta::ceta::ceta::obj", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "ala::ceta::ceta::obj", 8, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::ceta::ceta::obj2", 24, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::ceta::obj", 6, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::ceta::obj2", 23, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::linux::ala::obj", 12, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::linux::ala::obj3", 25, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::linux::obj", 10, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::linux::wifi::obj", 11, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::linux::wifi::obj1", 22, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::linux::wifi::obj5", 30, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "ala::obj", 2, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "obj", 1, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 61 - creating symbols, labels */
    {   R"ffDXD(.rawcode
        .byte 0
        sym1 = 25
        .set sym2, 27
        .equ sym3, 28
        .equiv sym4, 29
        .eqv sym5, 31
        label1:
        .scope creta
            .byte sym1, sym2, sym3, sym4, sym5, sym6
            sym1 = 15
            .set sym2, 17
            .equ sym3, 18
            .equiv sym4, 19
            .eqv sym5, 21
            sym6 = 16
            label1:
            .scope ula
                .byte sym1, sym2, sym3, sym4, sym5, sym6, sym7
                sym1 = 5
                .set sym2, 7
                .equ sym3, 8
                .equiv sym4, 9
                .eqv sym5, 11
                sym6 = 6
                sym7 = 106
                label1:
            .ends
        .ends
        .byte creta::sym2, creta::ula::sym7
        creta::ula::label2:
        .scope creta
            .scope ula
                .byte label2-label1
            .ends
        .ends
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 0, 25, 27, 28, 29, 31, 16, 15, 17, 18, 19, 21, 16, 106, 17, 106, 2 } } },
        {
            { ".", 17, 0, 0, true, false, false, 0, 0 },
            { "creta::label1", 7, 0, 0, true, true, false, 0, 0 },
            { "creta::sym1", 15, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::sym2", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::sym3", 18, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::sym4", 19, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "creta::sym5", 21, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "creta::sym6", 16, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::ula::label1", 14, 0, 0, true, true, false, 0, 0 },
            { "creta::ula::label2", 16, 0, 0, true, true, false, 0, 0 },
            { "creta::ula::sym1", 5, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::ula::sym2", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::ula::sym3", 8, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::ula::sym4", 9, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "creta::ula::sym5", 11, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "creta::ula::sym6", 6, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "creta::ula::sym7", 106, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "label1", 1, 0, 0, true, true, false, 0, 0 },
            { "sym1", 25, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym2", 27, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym3", 28, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym4", 29, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "sym5", 31, ASMSECT_ABS, 0, true, true, false, 0, 0 }
        }, true, "", ""
    },
    /* 62 - local labels and counter */
    {   R"ffDXD(.rawcode
            start: .int 3,5,6
label1: vx0 = start
        vx2 = label1+6
        vx3 = label2+8
        .int 1,2,3,4
label2: .int 3,6,7
        vx4 = 2f
.scope ala
2:      .int 11
.ends
        vx5 = 2b
        vx6 = 2f
        vx7 = 3f
.scope ela
2:      .int 12
.ends
.scope cola
3:
.ends
        vx8 = 3b
        # program counter
.scope aaa
        .int .-start
.ends
.scope bbb
        . = 60
.ends
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 3, 0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0,
              3, 0, 0 ,0, 4, 0, 0, 0, 3, 0, 0, 0, 6, 0, 0, 0, 7, 0, 0, 0,
              11, 0, 0, 0, 12, 0, 0, 0, 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } },
        {
            { ".", 60, 0, 0, true, false, false, 0, 0 },
            { "2b", 44, 0, 0, true, false, false, 0, 0 },
            { "2f", 44, 0, 0, false, false, false, 0, 0 },
            { "3b", 48, 0, 0, true, false, false, 0, 0 },
            { "3f", 48, 0, 0, false, false, false, 0, 0 },
            { "label1", 12, 0, 0, true, true, false, 0, 0 },
            { "label2", 28, 0, 0, true, true, false, 0, 0 },
            { "start", 0, 0, 0, true, true, false, 0, 0 },
            { "vx0", 0, 0, 0, true, false, false, 0, 0 },
            { "vx2", 18, 0, 0, true, false, false, 0, 0 },
            { "vx3", 36, 0, 0, true, false, false, 0, 0 },
            { "vx4", 40, 0, 0, true, false, false, 0, 0 },
            { "vx5", 40, 0, 0, true, false, false, 0, 0 },
            { "vx6", 44, 0, 0, true, false, false, 0, 0 },
            { "vx7", 48, 0, 0, true, false, false, 0, 0 },
            { "vx8", 48, 0, 0, true, false, false, 0, 0 },
        }, true, "", ""
    },
    /* 63 - get_version (AMD) */
    {   R"ffDXD(            .amd
            .kernel a
            .driver_version 200206
            .get_driver_version alaver
.scope Vx
            .get_driver_version alaver
.ends
.scope tx
            totver = 65
            blaver = 112
            .get_driver_version totver
.ends
            .get_driver_version tx::blaver
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { "a" },
        {
            { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } },
            { ".text", 0, AsmSectionType::CODE,
                {  } }
        },
        {
            { ".", 0U, 1, 0U, true, false, false, 0, 0 },
            { "Vx::alaver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "alaver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "tx::blaver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "tx::totver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    /* 64 - get_version (AMDCL2) */
    {   R"ffDXD(            .amdcl2
            .kernel a
            .driver_version 200206
            .get_driver_version alaver
.scope Vx
            .get_driver_version alaver
.ends
.scope tx
            totver = 65
            blaver = 112
            .get_driver_version totver
.ends
            .get_driver_version tx::blaver
)ffDXD",
        BinaryFormat::AMDCL2, GPUDeviceType::CAPE_VERDE, false, { "a" },
        {
            { ".rodata", ASMKERN_GLOBAL, AsmSectionType::DATA, { } },
            { ".text", 0, AsmSectionType::CODE,
                {  } }
        },
        {
            { ".", 0U, 1, 0U, true, false, false, 0, 0 },
            { "Vx::alaver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "alaver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "tx::blaver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "tx::totver", 200206U, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    /* 65 - register ranges */
    {   R"ffDXD(
        .scope ala
            sym1 = 123
            sym1 = %v[12:15]
            .scope bela
                sym2 = ::x*21
                sym2 = %v[120:125]
                sym3 = %sym2[1:3]
            .ends
            sym4 = %v[120:125]
            sym4 = 12
        .ends
        .scope black
            .set sym5, %v[120:125]
            .equiv sym6, %v[120:125]
        .ends
        sym8 = %v[ala::sym4:ala::sym4+4]
        sym9 = %::black::sym5[1:2]
        x=43)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA } },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "ala::bela::sym2", (256+120) | ((256+126ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "ala::bela::sym3", (256+121) | ((256+124ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "ala::sym1", (256+12) | ((256+16ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "ala::sym4", 12, ASMSECT_ABS, 0, true, false, 0, 0 },
            { "black::sym5", (256+120) | ((256+126ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "black::sym6", (256+120) | ((256+126ULL)<<32), ASMSECT_ABS, 0,
                true, true, false, 0, 0, true },
            { "sym8", (256+12) | ((256+17ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "sym9", (256+121) | ((256+123ULL)<<32), ASMSECT_ABS, 0,
                true, false, false, 0, 0, true },
            { "x", 43, ASMSECT_ABS, 0, true, false, 0, 0 }
        }, true, "", ""
    },
    /* 66 - scope errors */
    {   R"ffDXD(.rawcode
            ala::bx::. = 7
            .byte 1
            .int tlx::blx::.
            .using  
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE, { 1 } } },
        {
            { ".", 1, 0, 0, true, false, false, 0, 0 }
        }, false, "test.s:2:13: Error: Symbol '.' can be only in global scope\n"
        "test.s:4:18: Error: Symbol '.' can be only in global scope\n"
        "test.s:4:18: Error: Expression have unresolved symbol 'tlx::blx::.'\n"
        "test.s:5:21: Error: Expected scope path\n", ""
    },
    /* 67 - get_* */
    {   R"ffDXD(.amdcl2
            .gpu Mullins
            .get_gpu GPUX
            .get_arch ARCHY
            .get_64bit _64bitZZ
            .get_format FORMATWWW
)ffDXD",
        BinaryFormat::AMDCL2, GPUDeviceType::MULLINS, false, { },
        {
            { ".rodata", ASMKERN_GLOBAL, AsmSectionType::DATA, { } }
        },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "ARCHY", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "FORMATWWW", 3U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "GPUX", 12U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "_64bitZZ", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
        }, true, "", ""
    },
    /* 68 - get_* */
    {   R"ffDXD(.amdcl2
            .gpu Mullins
            .64bit
            .get_gpu GPUX
            .get_arch ARCHY
            .get_64bit _64bitZZ
            .get_format FORMATWWW
)ffDXD",
        BinaryFormat::AMDCL2, GPUDeviceType::MULLINS, true, { },
        {
            { ".rodata", ASMKERN_GLOBAL, AsmSectionType::DATA, { } }
        },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "ARCHY", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "FORMATWWW", 3U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "GPUX", 12U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "_64bitZZ", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
        }, true, "", ""
    },
    /* 69 - get_version */
    {   R"ffDXD(.amdcl2
            .get_version CLRX_VERSION
)ffDXD",
        BinaryFormat::AMDCL2, GPUDeviceType::CAPE_VERDE, false, { },
        {
            { ".rodata", ASMKERN_GLOBAL, AsmSectionType::DATA, { } }
        },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "CLRX_VERSION", CLRX_VERSION_NUMBER, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
        }, true, "", ""
    },
    /* 70 - '.for' repetition */
    {
        R"ffDXD(
            .for  x = 1  ,  x<  16,  x+x
                .int x
            .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 16U, 0, 0U, true, false, false, 0, 0 },
            { "x", 16U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 71 - '.for' repetition (error if x is onceDefined) */
    {
        R"ffDXD(
            .equiv x, 0
            .for  x = 1  ,  x<  16,  x+x
                .int x
            .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x00, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "x", 0U, ASMSECT_ABS, 0U, true, true, false, 0, 0 }
        }, false, "test.s:3:19: Error: Symbol 'x' is already defined\n"
            "test.s:5:13: Error: No '.rept' before '.endr'\n", ""
    },
    /* 72 - '.for' repetition (error due to undefined symbols) */
    {
        R"ffDXD(
            .for  x = 1  ,  x<  16,  x+x+vv
                .int x
            .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "vv", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, false, "test.s:2:36: Error: Expression have unresolved symbol 'vv'\n", ""
    },
    /* 73 - '.for' repetition (error due to undefined symbols) */
    {
        R"ffDXD(
            .for  x = 1  ,  x<  16+vv ,  x+x
                .int x
            .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "vv", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x", 2U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, false, "test.s:2:27: Error: Expression have unresolved symbol 'vv'\n", ""
    },
    /* 74 - '.for' repetition - with expr symbol */
    {   R"ffDXD(
        .eqv c,11+d
        d=7
        .for x =  1,x<16+c,x+x
            .int x
        .endr
        .int c
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
                0x12, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 28U, 0, 0U, true, false, false, 0, 0 },
            { "c", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "d", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x", 64U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 75 - '.while' repetition */
    {
        R"ffDXD(
            x=1
            .while  x  <  16
                .int x
                x=x+x
            .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 16U, 0, 0U, true, false, false, 0, 0 },
            { "x", 16U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 76 - '.for' repetition (error due to division by zero) */
    {
        R"ffDXD(
            .for  x = 1  ,  x<  16,  x+x+(x/0)
                .int x
            .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "x", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, false, "test.s:2:44: Error: Division by zero\n", ""
    },
    /* 77 - '.for' repetition (error due to division by zero) */
    {
        R"ffDXD(
            .for  x = 1  ,  x<  16+(x/0),  x+x
                .int x
            .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "x", 2U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, false, "test.s:2:38: Error: Division by zero\n", ""
    },
    /* 78 - '.for' - error: conditional is not absolute */
    {
        R"ffDXD(.gallium
            .for  x = 1  ,  x + 16+.,  x+x
                .int x
            .endr
)ffDXD",
        BinaryFormat::GALLIUM, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            {
                0x01, 0x00, 0x00, 0x00
            } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "x", 2U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, false,
        "test.s:2:27: Error: Value of conditional expression is not absolute\n", ""
    },
    {   /* 79 - '.for' and nested '.while' */
        R"ffDXD(
    .for x =  1,x<16,x+x
        y = 1
        .while y<7
            .byte x,y
            y=y+1
        .endr
    .endr
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x01, 0x01, 0x02, 0x01, 0x03, 0x01, 0x04,
                0x01, 0x05, 0x01, 0x06, 0x02, 0x01, 0x02, 0x02,
                0x02, 0x03, 0x02, 0x04, 0x02, 0x05, 0x02, 0x06,
                0x04, 0x01, 0x04, 0x02, 0x04, 0x03, 0x04, 0x04,
                0x04, 0x05, 0x04, 0x06, 0x08, 0x01, 0x08, 0x02,
                0x08, 0x03, 0x08, 0x04, 0x08, 0x05, 0x08, 0x06
            } } },
        {
            { ".", 48U, 0, 0U, true, false, false, 0, 0 },
            { "x", 16U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "y", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    // ROCm section diffs
    {   // 80 - ROCm section differences
        R"ffDXD(.rocm
.gpu Iceland
.arch_minor 0
.arch_stepping 0
.eflags 2
.newbinfmt
.target "amdgcn-amd-amdhsa-amdgizcl-gfx800"
.md_version 1, 0
.globaldata
gdata1:
.global gdata1
.int 1,2,3,4,6
gdata2:
.int somesym
.int vectorAdd-gdata2+2
.byte (vectorAdd+0x100)*4==gdata2+gdata2+gdata2+(0x650+0x100)*4+gdata2
someval1=somesym+10000
someval2=someval1*2
.eqv someval3, somesym1*4+c
c=1
someval4=someval3+1000000
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
somesym=vectorAdd-gdata1
somesym1=vectorAdd-gdata1
        s_branch vectorAdd + (vectorAdd-gdata2)
        s_branch vectorAdd + ((vectorAdd-gdata2)&0xffffe0)
        .int (vectorAdd+0x100)*4==gdata2+gdata2+gdata2+(0x650+0x100)*4+gdata2
        s_endpgm
c=somesym1
someval5=someval3+3000000
c=2
someval6=someval3+2000000

somesym1=1
aa1=aa2
aa0=aa1
aa1=3

aa2=6
)ffDXD",
        BinaryFormat::ROCM, GPUDeviceType::ICELAND, false,
        { "vectorAdd" },
        {   // sections
            { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
                {
                    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                    0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x40, 0x00, 0x0c, 0x00, 0x90, 0x00, 0x00, 0x00,
                    0x0b, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x01, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x06,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0xff, 0x00, 0x81, 0xbe, 0x50, 0x07, 0x00, 0x00,
                    0x51, 0x01, 0x82, 0xbf, 0x4c, 0x01, 0x82, 0xbf,
                    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x81, 0xbf
                } },
            { ".rodata", ASMKERN_GLOBAL, AsmSectionType::DATA,
                {
                    0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                    0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
                    0x06, 0x00, 0x00, 0x00, 0x64, 0x06, 0x00, 0x00,
                    0x52, 0x06, 0x00, 0x00, 0xff
                } },
            { nullptr, 0, AsmSectionType::CONFIG, { } },
        },
        {
            { ".", 280U, 0, 0U, true, false, false, 0, 0 },
            { "aa0", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "aa1", 3U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "aa2", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "c", 2U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "gdata1", 0U, 1, 0U, true, true, false, 16, 0 },
            { "gdata2", 20U, 1, 0U, true, true, false, 0, 0 },
            { "somesym", 1636U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "somesym1", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "someval1", 11636U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "someval2", 23272U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "someval3", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "someval4", 1006545U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "someval5", 3008180U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "someval6", 2006546U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "vectorAdd", 0U, 0, 0U, true, true, false, 0, 0 }
        }, true, "", ""
    },
    {   // 81 - use enums
        R"ffDXD(.enum sym1,sym2,sym6,xxx
        .enum >1?100:0 ,  ala,joan,beta
        
        .scope Error
            .enum OK
            .enum BADFD
            .enum BADIP
        .ends
        .scope Result
            .enum NONE
            .enum INCOMPLETE
            .enum FULL
        .ends
        .enum otherEnum, blabla
        .enum >, myzero
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "Error::BADFD", 1, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "Error::BADIP", 2, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "Error::OK", 0, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "Result::FULL", 2, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "Result::INCOMPLETE", 1, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "Result::NONE", 0, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "ala", 100, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "beta", 102, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "blabla", 104, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "joan", 101, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "myzero", 0, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "otherEnum", 103, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "sym1", 0, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "sym2", 1, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "sym6", 2, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "xxx", 3, ASMSECT_ABS, 0, true, true, false, 0, 0 }
        }, true, "", ""
    },
    {   // 82 - use enums - errors
        R"ffDXD(mydef = 1
        .enum mydef,mydef2
        .enum >xxx, doOne, doSecond
)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "mydef", 1, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "mydef2", 0, ASMSECT_ABS, 0, true, true, false, 0, 0 }
        }, false,
        "test.s:2:15: Error: Symbol 'mydef' is already defined\n"
        "test.s:3:16: Error: Expression have unresolved symbol 'xxx'\n",
        ""
    },
    /* 83 - policy version */
    {   R"ffDXD(.amdcl2
            .get_policy POLICY0
            .policy 12
            .get_policy POLICY1
            .policy 1112222233333
)ffDXD",
        BinaryFormat::AMDCL2, GPUDeviceType::CAPE_VERDE, false, { },
        {
            { ".rodata", ASMKERN_GLOBAL, AsmSectionType::DATA, { } }
        },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "POLICY0", CLRX_VERSION_NUMBER, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "POLICY1", 12, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, true,
        "test.s:5:21: Warning: Value 0x102f59c72f5 truncated to 0xf59c72f5\n", ""
    },
    /* 84 - GPU arch check (ifarch) */
    {   ".amd\n.gpu Tahiti\n"
        ".ifarch GCN1.1\n"
        ".print \"isGCN1.1\"\n"
        ".endif\n"
        ".ifnarch GCN1.1\n"
        ".print \"isNotGCN1.1\"\n"
        ".endif\n",
        BinaryFormat::AMD, GPUDeviceType::TAHITI, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isNotGCN1.1\n",
    },
    /* 85 - GPU arch check (ifarch) */
    {   ".amd\n.gpu Bonaire\n"
        ".ifarch GCN1.1\n"
        ".print \"isGCN1.1\"\n"
        ".endif\n"
        ".ifnarch GCN1.1\n"
        ".print \"isNotGCN1.1\"\n"
        ".endif\n",
        BinaryFormat::AMD, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isGCN1.1\n",
    },
    /* 86 - GPU arch check (ifarch) */
    {   ".amdcl2\n.gpu Bonaire\n"
        ".ifarch GCN1.4\n"
        ".print \"isGCN1.4\"\n"
        ".endif\n"
        ".ifnarch GCN1.4\n"
        ".print \"isNotGCN1.4\"\n"
        ".endif\n",
        BinaryFormat::AMDCL2, GPUDeviceType::BONAIRE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isNotGCN1.4\n",
    },
    /* 87 - GPU arch check (ifarch) */
    {   ".amdcl2\n.gpu vega10\n"
        ".ifarch GCN1.4\n"
        ".print \"isGCN1.4\"\n"
        ".endif\n"
        ".ifnarch GCN1.4\n"
        ".print \"isNotGCN1.4\"\n"
        ".endif\n",
        BinaryFormat::AMDCL2, GPUDeviceType::GFX900, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isGCN1.4\n",
    },
    /* 88 - GPU arch check (ifarch) */
    {   ".amdcl2\n.gpu vega20\n"
        ".ifarch GCN1.4\n"
        ".print \"isGCN1.4\"\n"
        ".endif\n"
        ".ifnarch GCN1.4\n"
        ".print \"isNotGCN1.4\"\n"
        ".endif\n",
        BinaryFormat::AMDCL2, GPUDeviceType::GFX906, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isGCN1.4\n",
    },
    /* 89 - GPU arch check (ifarch) */
    {   ".amdcl2\n.gpu vega10\n"
        ".ifarch GCN1.4.1\n"
        ".print \"isGCN1.4.1\"\n"
        ".endif\n"
        ".ifnarch GCN1.4.1\n"
        ".print \"isNotGCN1.4.1\"\n"
        ".endif\n",
        BinaryFormat::AMDCL2, GPUDeviceType::GFX900, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isNotGCN1.4.1\n",
    },
    /* 90 - GPU arch check (ifarch) */
    {   ".amdcl2\n.gpu vega20\n"
        ".ifarch GCN1.4.1\n"
        ".print \"isGCN1.4.1\"\n"
        ".endif\n"
        ".ifnarch GCN1.4.1\n"
        ".print \"isNotGCN1.4.1\"\n"
        ".endif\n",
        BinaryFormat::AMDCL2, GPUDeviceType::GFX906, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isGCN1.4.1\n",
    },
    /* 89 - GPU arch check (ifarch) */
    {   ".amdcl2\n.gpu fiji\n"
        ".ifarch GCN1.4.1\n"
        ".print \"isGCN1.4.1\"\n"
        ".endif\n"
        ".ifnarch GCN1.4.1\n"
        ".print \"isNotGCN1.4.1\"\n"
        ".endif\n",
        BinaryFormat::AMDCL2, GPUDeviceType::FIJI, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true,
        "", "isNotGCN1.4.1\n",
    },
    { nullptr }
};
