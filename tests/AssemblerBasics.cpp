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
#include <cstdio>
#include <sstream>
#include <map>
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdasm/Assembler.h>
#include "TestUtils.h"

using namespace CLRX;

struct Section
{
    const char* kernel;
    AsmSectionType type;
    Array<cxbyte> content;
};

struct SymEntry
{
    const char* name;
    uint64_t value;
    cxuint sectionId;
    uint64_t size;
    bool isDefined;
    bool onceDefined;
    bool base;
    cxbyte info;
    cxbyte other;
};

struct AsmTestCase
{
    const char* input;
    BinaryFormat format;
    GPUDeviceType deviceType;
    bool is64Bit;
    const Array<Section> sections;
    const Array<SymEntry> symbols;
    bool good;
    const char* errorMessages;
    const char* printMessages;
};

static AsmTestCase asmTestCases1Tbl[] =
{
    /* 0 empty */
    { "", BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
      {  }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "" },
    /* 1 standard symbol assignment */
    {   R"ffDXD(sym1 = 7
        sym2 = 81
        sym3 = sym7*sym4
        sym4 = sym5*sym6+sym7 - sym1
        sym5 = 17
        sym6 = 43
        sym7 = 91)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "sym1", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym2", 81, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym3", 91*(17*43+91-7), ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym4", 17*43+91-7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym5", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym6", 43, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym7", 91, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 2 undefined symbols and self defined and redefinitions */
    {   R"ffDXD(sym1 = 7
        sym2 = 81
        sym3 = sym7*sym4
        sym4 = sym5*sym6+sym7 - sym1
        sym5 = 17
        sym6 = 43
        sym9 = sym9
        sym10 = sym10
        sym10 = sym2+7)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "sym1", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym10", 88, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym2", 81, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym3", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "sym4", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "sym5", 17, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym6", 43, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "sym7", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
            { "sym9", 0, ASMSECT_ABS, 0, false, false, false, 0, 0 },
        }, true, "", ""
    },
    // 3 labels and local labels
    {   R"ffDXD(.rawcode
start: .int 3,5,6
label1: vx0 = start
        vx2 = label1+6
        vx3 = label2+8
        .int 1,2,3,4
label2: .int 3,6,7
        vx4 = 2f
2:      .int 11
        vx5 = 2b
        vx6 = 2f
        vx7 = 3f
2:      .int 12
3:      vx8 = 3b
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::RAWCODE_CODE,
            { 3, 0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0,
              3, 0, 0 ,0, 4, 0, 0, 0, 3, 0, 0, 0, 6, 0, 0, 0, 7, 0, 0, 0,
              11, 0, 0, 0, 12, 0, 0, 0 } } },
        {
            { ".", 48, 0, 0, true, false, false, 0, 0 },
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
    /* 4 labels on absolute section type (likes global data) */
    {   R"ffDXD(label1:
3:      v1 = label1
        v2 = 3b)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA, { } } },
        {
            { ".", 0, 0, 0, true, false, false, 0, 0 },
            { "3b", 0, 0, 0, true, false, false, 0, 0 },
            { "3f", 0, 0, 0, false, false, false, 0, 0 },
            { "label1", 0, 0, 0, true, true, false, 0, 0, },
            { "v1", 0, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "v2", 0, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 5 assignments, assignment of labels and symbols */
    {   R"ffDXD(.rawcode
start: .byte 0xfa, 0xfd, 0xfb, 0xda
start:  # try define again this same label
        start = 132 # try define by assignment
        .byte zx
        zx = 9
        .byte zx
        zx = 10
1:      .byte zx
        1 = 6       # illegal asssignemt of local label
        # by .set
        .byte zy
        .set zy, 10
        .byte zy
        .set zy, 11
        .byte zy
        # by .equ
        .byte zz
        .equ zz, 100
        .byte zz
        .equ zz, 120
        .byte zz
        # by equiv
        .byte testx
        .equiv testx, 130   # illegal by equiv
        .byte testx
        .equiv testx, 150
        .byte testx
        myval = 0x12
        .equiv myval,0x15   # illegal by equiv
        .equiv myval,0x15   # illegal by equiv
        myval = 6       # legal by normal assignment
        .set myval,8    # legal
        .equ myval,9    # legal
        testx = 566
        .set testx,55
testx:
        lab1 = 656
        .set lab2, 594
        .set lab3, 551
lab1: lab2: lab3: # reassign by labels is legal
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::RAWCODE_CODE,
            { 0xfa, 0xfd, 0xfb, 0xda, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0b, 0x64, 0x64,
              0x78, 0x82, 0x82, 0x82 } } },
        {
            { ".", 16, 0, 0, true, false, false, 0, 0 },
            { "1b", 6, 0, 0, true, false, false, 0, 0 },
            { "1f", 6, 0, 0, false, false, false, 0, 0 },
            { "lab1", 16, 0, 0, true, true, false, 0, 0 },
            { "lab2", 16, 0, 0, true, true, false, 0, 0 },
            { "lab3", 16, 0, 0, true, true, false, 0, 0 },
            { "myval", 9, ASMSECT_ABS, 0, true, false, false, 0, 0, },
            { "start", 0, 0, 0, true, true, false, 0, 0 },
            { "testx", 130, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "zx", 10, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "zy", 11, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "zz", 120, ASMSECT_ABS, 0, true, false, false, 0, 0 },
        }, false,
        "test.s:3:1: Error: Symbol 'start' is already defined\n"
        "test.s:4:9: Error: Symbol 'start' is already defined\n"
        "test.s:10:9: Error: Illegal number at statement begin\n"
        "test.s:10:11: Error: Garbages at end of line with pseudo-op\n"
        "test.s:27:16: Error: Symbol 'testx' is already defined\n"
        "test.s:30:16: Error: Symbol 'myval' is already defined\n"
        "test.s:31:16: Error: Symbol 'myval' is already defined\n"
        "test.s:35:9: Error: Symbol 'testx' is already defined\n"
        "test.s:36:14: Error: Symbol 'testx' is already defined\n"
        "test.s:37:1: Error: Symbol 'testx' is already defined\n", ""
    },
    /* 6 .eqv test 1 */
    {   R"ffDXD(        z=5
        .eqv v1,v+t
        .eqv v,z*y
        .int v1
        .int v+v
        z=8
        .int v+v
        z=9
        y=3
        t=7
        .int v1
        t=8
        y=2
        .int v1+v)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
          { 0x16, 0, 0, 0, 0x1e, 0, 0, 0, 0x30, 0, 0, 0, 0x22, 0, 0, 0,
            0x2c, 0, 0, 0 } } },
        {
            { ".", 20, 0, 0, true, false, false, 0, 0 },
            { "t", 8, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "v", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "v1", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "y", 2, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z", 9, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 7 .eqv test 2 */
    {   R"ffDXD(.int y+7
        t=8
        tx=3
        .eqv y,t*tx+2
        
        .int y2+7
        t2=8
        .eqv y2,t2*tx2+3
        tx2=5
        
        n1=7
        n2=6
        .eqv out0,n1*n2+2
        .int out0
        n2=5
        .int out0
        
        t2=3
        t3=4
        .eqv x0,2*t2*t3
        .eqv out1,x0*2
        .int out1
        
        .eqv x1,2
        .eqv out2,x1*2
        .int out2)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
          { 0x21, 0, 0, 0, 0x32, 0, 0, 0, 0x2c, 0, 0, 0, 0x25, 0, 0, 0,
            0x30, 0, 0, 0, 0x04, 0, 0, 0 } } },
        {
            { ".", 24, 0, 0, true, false, false, 0, 0 },
            { "n1", 7, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "n2", 5, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "out0", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "out1", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "out2", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "t", 8, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "t2", 3, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "t3", 4, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "tx", 3, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "tx2", 5, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x0", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x1", 2, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "y", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "y2", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 }
        }, true, "", ""
    },
    /* 8 .eqv test3 - various order of .eqv */
    {   R"ffDXD(x00t = 6
            x00u = x00t+9
            .eqv x03,6
            .eqv x02,x03+2*x03+x00u
            .eqv x01,x02*x02+x00t
            x00 = x01+x02*x03
            .int x00
            z00 = x00
            
            x10u = x10t+11
            x10t = 8
            .eqv x12,x13+2*x13+x10u
            .eqv x13,14
            .eqv x11,x12*x12+x10t
            x10 = x11+x12*x13
            .int x10
            z10 = x10
            
            x20u = x20t+3
            x20t = 11
            .eqv x21,x22*x22+x20t
            .eqv x22,x23+2*x23+x20u
            .eqv x23,78
            x20 = x21+x22*x23
            .int x20
            z20 = x20
            
            x30u = x30t+21
            x30t = 31
            x30 = x31+x32*x33
            .eqv x31,x32*x32+x30t
            .eqv x32,x33+2*x33+x30u
            .eqv x33,5
            .int x30
            z30 = x30
            
            z40 = x40
            .int x40
            x40u = x40t+71
            x40t = 22
            x40 = x41+x42*x43
            .eqv x41,x42*x42+x40t
            .eqv x42,x43+2*x43+x40u
            .eqv x43,12
            
            z50 = x50
            .int x50
            x50t = 15
            x50 = x51+x52*x53
            .eqv x51,x52*x52+x50t
            .eqv x52,x53+2*x53+x50u
            .eqv x53,23
            x50u = x50t+19
            )ffDXD", /* TODO: GNU as incorrectly calculates x40 and x50 symbols */
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
          { 0x0d, 0x5, 0, 0, 0xe7, 0x11, 0, 0, 0xdb, 0x3b, 1, 0, 0xf7, 0x12, 0, 0,
            0x23, 0x47, 0, 0, 0xc1, 0x32, 0, 0 } } },
        {
            { ".", 24, 0, 0, true, false, false, 0, 0 },
            { "x00", 1293, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x00t", 6, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x00u", 15, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x01", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x02", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x03", 6, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "x10", 4583, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x10t", 8, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x10u", 19, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x11", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x12", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x13", 14, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "x20", 80859, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x20t", 11, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x20u", 14, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x21", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x22", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x23", 78, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "x30", 4855, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x30t", 31, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x30u", 52, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x31", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x32", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x33", 5, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "x40", 18211, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x40t", 22, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x40u", 93, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x41", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x42", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x43", 12, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "x50", 12993, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x50t", 15, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x50u", 34, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x51", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x52", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x53", 23, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "z00", 1293, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z10", 4583, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z20", 80859, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z30", 4855, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z40", 18211, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z50", 12993, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 9 .eqv test3 - various order of .eqv */
    {   R"ffDXD(x00t = 6
            x00u = x00t+9
            .eqv x03,x00u*x00t+6
            .eqv x02,x03+2*x03+x00u
            .eqv x01,x02*x02+x00t
            x00 = x01+x02*x03
            .int x00
            z00 = x00
            
            x10u = x10t+11
            x10t = 8
            .eqv x12,x13+2*x13+x10u
            .eqv x13,x10u*x10t+14
            .eqv x11,x12*x12+x10t
            x10 = x11+x12*x13
            .int x10
            z10 = x10
            
            x20u = x20t+3
            x20t = 11
            .eqv x21,x22*x22+x20t
            .eqv x22,x23+2*x23+x20u
            .eqv x23,x20u*x20t+78
            x20 = x21+x22*x23
            .int x20
            z20 = x20
            
            x30u = x30t+21
            x30t = 31
            x30 = x31+x32*x33
            .eqv x31,x32*x32+x30t
            .eqv x32,x33+2*x33+x30u
            .eqv x33,x30u*x30t+5
            .int x30
            z30 = x30
            
            z40 = x40
            .int x40
            x40u = x40t+71
            x40t = 22
            x40 = x41+x42*x43
            .eqv x41,x42*x42+x40t
            .eqv x42,x43+2*x43+x40u
            .eqv x43,x40u*x40t+12
            
            z50 = x50
            .int x50
            x50t = 15
            x50 = x51+x52*x53
            .eqv x51,x52*x52+x50t
            .eqv x52,x53+2*x53+x50u
            .eqv x53,x50u*x50t+23
            x50u = x50t+19
            )ffDXD", /* TODO: GNU as incorrectly calculates x40 and x50 symbols */
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
          { 0x47, 0xd8, 0x01, 0x00, 0x5f, 0x63, 0x05, 0x00, 0x9f, 0x34, 0x0a, 0x00,
            0x67, 0xc9, 0xe7, 0x01, 0xfd, 0x17, 0x1c, 0x03, 0xc5, 0xf8, 0x35, 0x00 } } },
        {
            { ".", 24, 0, 0, true, false, false, 0, 0 },
            { "x00", 120903U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x00t", 6, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x00u", 15, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x01", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x02", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x03", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x10", 353119U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x10t", 8, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x10u", 19, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x11", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x12", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x13", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x20", 668831U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x20t", 11, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x20u", 14, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x21", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x22", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x23", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x30", 31967591U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x30t", 31, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x30u", 52, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x31", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x32", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x33", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x40", 52172797U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x40t", 22, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x40u", 93, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x41", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x42", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x43", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x50", 3537093U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x50t", 15, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x50u", 34, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "x51", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x52", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "x53", 0, ASMSECT_ABS, 0, false, true, true, 0, 0 },
            { "z00", 120903U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z10", 353119U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z20", 668831U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z30", 31967591U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z40", 52172797U, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "z50", 3537093U, ASMSECT_ABS, 0, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 10 .eqv - undefined snapshots */
    {   R"ffDXD(            x00u = x00t+9
            .eqv x02,x03+2*x03+x00u
            .eqv x01,x02*x02+x00t
            x00 = x01+x02*x03
            .int x00+x00*x00
            z00 = x00+x00*x00
            
            x10u = x10t+11
            x10t = 8
            .eqv x12,x13+2*x13+x10u
            .eqv x11,x12*x12+x10t
            x10 = x11+x12*x13
            .int x10+x10+x10
            
            .int x20+x20+x20
            x20u = x20t+3
            .eqv x21,x22*x22+x20t
            .eqv x22,x23+2*x23+x20u
            .eqv x23,x20t+78
            x20 = x21+x22*x23
            
            x30u = x30t+21
            x30t = 31
            x30 = x31+x32*x33
            .eqv x31,x32*x32+x30t
            .eqv x32,x33+2*x33+x30u
            .int x10+x20+x30*x30
            .int x30
            z30 = x10+x20+x30*x30
        )ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } },
        {
            { ".", 20U, 0, 0, true, false, false, 0, 0 },
            { "x00", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x00t", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x00u", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x01", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x02", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x03", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x10", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x10t", 8U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x10u", 19U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x11", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x12", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x13", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x20", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x20t", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x20u", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x21", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x22", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x23", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x30", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x30t", 31U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x30u", 52U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x31", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x32", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x33", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "z00", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "z30", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 11 .eqv - reassign trials */
    {   R"ffDXD(            .eqv x0, 5
            .set x0, 56
            .equ x0, 53
            x0 = 51
x0:
            .eqv x1, x1_1<<1
            .set x1, 56
            .equ x1, 53
            x1 = 51
x1:
            x1_1 = 74
            .int x1
            
            .int x2
            .eqv x2, x2_1<<1
            .set x2, 56
            .equ x2, 53
            x2 = 51
x2:
            x2_1 = 7)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA, { 0x94, 0, 0, 0, 0xe, 0, 0, 0 } } },
        {
            { ".", 8U, 0, 0U, true, false, false, 0, 0 },
            { "x0", 5U, ASMSECT_ABS, 0U, true, true, false, 0, 0 },
            { "x1", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x1_1", 74U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x2", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x2_1", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        false, R"ffDXD(test.s:2:18: Error: Symbol 'x0' is already defined
test.s:3:18: Error: Symbol 'x0' is already defined
test.s:4:13: Error: Symbol 'x0' is already defined
test.s:5:1: Error: Symbol 'x0' is already defined
test.s:7:18: Error: Symbol 'x1' is already defined
test.s:8:18: Error: Symbol 'x1' is already defined
test.s:9:13: Error: Symbol 'x1' is already defined
test.s:10:1: Error: Symbol 'x1' is already defined
test.s:16:18: Error: Symbol 'x2' is already defined
test.s:17:18: Error: Symbol 'x2' is already defined
test.s:18:13: Error: Symbol 'x2' is already defined
test.s:19:1: Error: Symbol 'x2' is already defined
)ffDXD", ""
    },
    /* 12 - .eqv evaluation */
    {   R"ffDXD(            .eqv t0, x0<<65
            x0 = 6
            .eqv t00, t0+t0
            .int t00
            
            .eqv t1, t2<<65
            .eqv t2, x1<<67
            x1 = 5
            .eqv t20, t1+t1
            .int t20)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA, { 0, 0, 0, 0, 0, 0, 0, 0 } } },
        {
            { ".", 8U, 0, 0U, true, false, false, 0, 0 },
            { "t0", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "t00", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "t1", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "t2", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "t20", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x0", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x1", 5U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
        },
        true, R"ffDXD(Expression evaluation from test.s:3:22:
                      from test.s:4:18:
test.s:1:24: Warning: Shift count out of range (between 0 and 63)
Expression evaluation from test.s:6:21:
                      from test.s:9:22:
                      from test.s:10:18:
test.s:7:24: Warning: Shift count out of range (between 0 and 63)
Expression evaluation from test.s:9:22:
                      from test.s:10:18:
test.s:6:24: Warning: Shift count out of range (between 0 and 63)
)ffDXD", ""
    },
    /* 13 - .eqv */
    {   R"ffDXD(            .int x00+x01+x00
            a00 = 3
            a01 = 7
            a02 = 0
            .eqv x00,a00*a01
            .eqv x01,a00/a02
            
            .int x10+x11+x10
            .eqv x10,a10*a11
            .eqv x11,a10/a12
            a10 = 3
            a11 = 7
            a12 = 0)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA, { 0, 0, 0, 0, 0, 0, 0, 0 } } },
        {
            { ".", 8U, 0, 0U, true, false, false, 0, 0 },
            { "a00", 3U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "a01", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "a02", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "a10", 3U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "a11", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "a12", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x00", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x01", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x10", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "x11", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 }
        },
        false, "Expression evaluation from test.s:1:18:\n"
        "test.s:6:25: Error: Division by zero\n"
        "Expression evaluation from test.s:8:18:\n"
        "test.s:10:25: Error: Division by zero\n", ""
    },
    /* 14 - assignment syntax errors */
    {   R"ffDXD(            uu  = 
            .set
            .set uu 
            .set uu , 
1x:
aaa: aaa:
            uu<> = t
            uuxu = 34 ;;;;
            .set udu, 3445   ;;;;;;;)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA, { } } },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "aaa", 0U, 0, 0U, true, true, false, 0, 0 },
        },
        false, R"ffDXD(test.s:1:19: Error: Expected assignment expression
test.s:2:17: Error: Expected symbol
test.s:2:17: Error: Expected expression
test.s:3:21: Error: Expected expression
test.s:4:22: Error: Expected assignment expression
test.s:5:1: Error: Illegal number at statement begin
test.s:5:2: Error: Garbages at end of line with pseudo-op
test.s:6:6: Error: Symbol 'aaa' is already defined
test.s:7:15: Error: Garbages at end of line with pseudo-op
test.s:8:23: Error: Garbages at end of expression
test.s:9:30: Error: Garbages at end of expression
)ffDXD", ""
    },
    /* 15 - basic pseudo-ops */
    {   R"ffDXD(            .byte 0,1,2,4,0xffda,-12,,
            .short 1234,1331,-4431,1331,0xdabdd,,
            .hword 1234,1331,-4431,1331,0xdabdd,,
            .int -221,11,533,-123, 223232323,,
            .long -221,11,533,-123, 223232323,,
            .word -221,11,533,-123, 223232323,,
            .quad 189181818918918918,89892898932893832,,
            .octa 23932800000000000000000000000829999993,, -0x10000000000000000
            .octa -89323489918918921982189212891289
            .half -1.4,2.4,,1.2
            .float -1.4,2.4,,1.2
            .double -1.4,2.4,,1.2
            .ascii "ala",,"ma6","1kota"
            .asciz "alax",,"ma5","2kota"
            .string "alay",,"ma4","3kota"
            .string16 "alaz",,"ma3","4kota"
            .string32 "alad",,"ma2","5kota"
            .string64 "alad",,"ma1","6kota")ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
            {
                0x00, 0x01, 0x02, 0x04, 0xda, 0xf4, 0x00, 0x00,
                0xd2, 0x04, 0x33, 0x05, 0xb1, 0xee, 0x33, 0x05,
                0xdd, 0xab, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x04,
                0x33, 0x05, 0xb1, 0xee, 0x33, 0x05, 0xdd, 0xab,
                0x00, 0x00, 0x00, 0x00, 0x23, 0xff, 0xff, 0xff,
                0x0b, 0x00, 0x00, 0x00, 0x15, 0x02, 0x00, 0x00,
                0x85, 0xff, 0xff, 0xff, 0x43, 0x41, 0x4e, 0x0d,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x23, 0xff, 0xff, 0xff, 0x0b, 0x00, 0x00, 0x00,
                0x15, 0x02, 0x00, 0x00, 0x85, 0xff, 0xff, 0xff,
                0x43, 0x41, 0x4e, 0x0d, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x23, 0xff, 0xff, 0xff,
                0x0b, 0x00, 0x00, 0x00, 0x15, 0x02, 0x00, 0x00,
                0x85, 0xff, 0xff, 0xff, 0x43, 0x41, 0x4e, 0x0d,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x06, 0xdb, 0x9f, 0xaa, 0xdc, 0x1b, 0xa0, 0x02,
                0x88, 0xa8, 0xb9, 0x84, 0x1d, 0x5d, 0x3f, 0x01,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x79, 0xcb, 0x78, 0x31, 0xe0, 0x36, 0x22, 0xe9,
                0xc9, 0x19, 0x0d, 0x5c, 0x24, 0x4a, 0x01, 0x12,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0x67, 0x87, 0x62, 0x5f, 0x1d, 0x35, 0x72, 0x0d,
                0xbc, 0xe4, 0x3c, 0x94, 0x98, 0xfb, 0xff, 0xff,
                0x9a, 0xbd, 0xcd, 0x40, 0x00, 0x00, 0xcd, 0x3c,
                0x33, 0x33, 0xb3, 0xbf, 0x9a, 0x99, 0x19, 0x40,
                0x00, 0x00, 0x00, 0x00, 0x9a, 0x99, 0x99, 0x3f,
                0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0xf6, 0xbf,
                0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x03, 0x40,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xf3, 0x3f,
                0x61, 0x6c, 0x61, 0x6d, 0x61, 0x36, 0x31, 0x6b,
                0x6f, 0x74, 0x61, 0x61, 0x6c, 0x61, 0x78, 0x00,
                0x6d, 0x61, 0x35, 0x00, 0x32, 0x6b, 0x6f, 0x74,
                0x61, 0x00, 0x61, 0x6c, 0x61, 0x79, 0x00, 0x6d,
                0x61, 0x34, 0x00, 0x33, 0x6b, 0x6f, 0x74, 0x61,
                0x00, 0x61, 0x00, 0x6c, 0x00, 0x61, 0x00, 0x7a,
                0x00, 0x00, 0x00, 0x6d, 0x00, 0x61, 0x00, 0x33,
                0x00, 0x00, 0x00, 0x34, 0x00, 0x6b, 0x00, 0x6f,
                0x00, 0x74, 0x00, 0x61, 0x00, 0x00, 0x00, 0x61,
                0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x61,
                0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x61,
                0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x6b,
                0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x74,
                0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00
            } } },
        { { ".", 523U, 0, 0U, true, false, false, 0, 0 } },
        true, R"ffDXD(test.s:1:27: Warning: Value 0xffda truncated to 0xda
test.s:1:38: Warning: No expression, zero has been put
test.s:1:39: Warning: No expression, zero has been put
test.s:2:41: Warning: Value 0xdabdd truncated to 0xabdd
test.s:2:49: Warning: No expression, zero has been put
test.s:2:50: Warning: No expression, zero has been put
test.s:3:41: Warning: Value 0xdabdd truncated to 0xabdd
test.s:3:49: Warning: No expression, zero has been put
test.s:3:50: Warning: No expression, zero has been put
test.s:4:46: Warning: No expression, zero has been put
test.s:4:47: Warning: No expression, zero has been put
test.s:5:47: Warning: No expression, zero has been put
test.s:5:48: Warning: No expression, zero has been put
test.s:6:47: Warning: No expression, zero has been put
test.s:6:48: Warning: No expression, zero has been put
test.s:7:56: Warning: No expression, zero has been put
test.s:7:57: Warning: No expression, zero has been put
test.s:8:58: Warning: No 128-bit literal, zero has been put
test.s:10:28: Warning: No floating point literal, zero has been put
test.s:11:29: Warning: No floating point literal, zero has been put
test.s:12:30: Warning: No floating point literal, zero has been put
)ffDXD", ""
    },
    /* 16 - fillings */
    {   R"ffDXD(            .fill 5,,15
            .fill 5
            .fill 5,1
            .fill 5,0,0xdc
            .fill 5,1,0xdc
            .fill 5,2,0xbaca
            .fill 5,3,0xbaca90
            .fill 5,4,0xbaca9022
            .fill 5,5,0xbaca901155
            .fillq 5,5,0xbaca901155
            .fillq 5,8,0x907856453412cdba
            .fill 5,0,0x11dc
            .fill 5,1,0x22dc
            .fill 5,2,0x444baca
            .fill 5,3,0xfebaca90
            .fill 5,4,0x121baca9022
            .fill 5,5,0x12baca901155
            .fillq 5,5,0x12baca901155)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
            {
                0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc,
                0xdc, 0xdc, 0xdc, 0xdc, 0xca, 0xba, 0xca, 0xba,
                0xca, 0xba, 0xca, 0xba, 0xca, 0xba, 0x90, 0xca,
                0xba, 0x90, 0xca, 0xba, 0x90, 0xca, 0xba, 0x90,
                0xca, 0xba, 0x90, 0xca, 0xba, 0x22, 0x90, 0xca,
                0xba, 0x22, 0x90, 0xca, 0xba, 0x22, 0x90, 0xca,
                0xba, 0x22, 0x90, 0xca, 0xba, 0x22, 0x90, 0xca,
                0xba, 0x55, 0x11, 0x90, 0xca, 0x00, 0x55, 0x11,
                0x90, 0xca, 0x00, 0x55, 0x11, 0x90, 0xca, 0x00,
                0x55, 0x11, 0x90, 0xca, 0x00, 0x55, 0x11, 0x90,
                0xca, 0x00, 0x55, 0x11, 0x90, 0xca, 0xba, 0x55,
                0x11, 0x90, 0xca, 0xba, 0x55, 0x11, 0x90, 0xca,
                0xba, 0x55, 0x11, 0x90, 0xca, 0xba, 0x55, 0x11,
                0x90, 0xca, 0xba, 0xba, 0xcd, 0x12, 0x34, 0x45,
                0x56, 0x78, 0x90, 0xba, 0xcd, 0x12, 0x34, 0x45,
                0x56, 0x78, 0x90, 0xba, 0xcd, 0x12, 0x34, 0x45,
                0x56, 0x78, 0x90, 0xba, 0xcd, 0x12, 0x34, 0x45,
                0x56, 0x78, 0x90, 0xba, 0xcd, 0x12, 0x34, 0x45,
                0x56, 0x78, 0x90, 0xdc, 0xdc, 0xdc, 0xdc, 0xdc,
                0xca, 0xba, 0xca, 0xba, 0xca, 0xba, 0xca, 0xba,
                0xca, 0xba, 0x90, 0xca, 0xba, 0x90, 0xca, 0xba,
                0x90, 0xca, 0xba, 0x90, 0xca, 0xba, 0x90, 0xca,
                0xba, 0x22, 0x90, 0xca, 0xba, 0x22, 0x90, 0xca,
                0xba, 0x22, 0x90, 0xca, 0xba, 0x22, 0x90, 0xca,
                0xba, 0x22, 0x90, 0xca, 0xba, 0x55, 0x11, 0x90,
                0xca, 0x00, 0x55, 0x11, 0x90, 0xca, 0x00, 0x55,
                0x11, 0x90, 0xca, 0x00, 0x55, 0x11, 0x90, 0xca,
                0x00, 0x55, 0x11, 0x90, 0xca, 0x00, 0x55, 0x11,
                0x90, 0xca, 0xba, 0x55, 0x11, 0x90, 0xca, 0xba,
                0x55, 0x11, 0x90, 0xca, 0xba, 0x55, 0x11, 0x90,
                0xca, 0xba, 0x55, 0x11, 0x90, 0xca, 0xba
            } } },
        { { ".", 255U, 0, 0U, true, false, false, 0, 0 } },
        true, "test.s:9:23: Warning: Value 0xbaca901155 truncated to 0xca901155\n"
            "test.s:13:23: Warning: Value 0x22dc truncated to 0xdc\n"
            "test.s:14:23: Warning: Value 0x444baca truncated to 0xbaca\n"
            "test.s:15:23: Warning: Value 0xfebaca90 truncated to 0xbaca90\n"
            "test.s:16:23: Warning: Value 0x121baca9022 truncated to 0xbaca9022\n"
            "test.s:17:23: Warning: Value 0x12baca901155 truncated to 0xca901155\n"
            "test.s:18:24: Warning: Value 0x12baca901155 truncated to 0xbaca901155\n", ""
    },
    /* 17 - alignment pseudo-ops */
    {   R"ffDXD(        .byte 1,2,3,4,5
        .align 4
        .byte 1,2,3,4,5
        .align 4,0xfa
        .byte 1,2,3,4,5
        .align 4,0xfa,3
        .byte 1,2,3,4,5
        .align 4,0xfa,2
        .byte 1,1,1
        
        .byte 1,2,3,4,5
        .balign 4
        .byte 1,2,3,4,5
        .balign 4,0xfb
        .byte 1,2,3,4,5
        .balign 4,0xfb,3
        .byte 1,2,3,4,5
        .balign 4,0xfb,2
        .byte 1,1,1
        
        .byte 1,2,3,4,5
        .p2align 2
        .byte 1,2,3,4,5
        .p2align 2,0xeb
        .byte 1,2,3,4,5
        .p2align 2,0xeb,3
        .byte 1,2,3,4,5
        .p2align 2,0xeb,2
        .byte 1,1,1
        
        .byte 3,2
        .balignw 8
        .byte 3,2
        .balignw 8,0x21fb
        .byte 3,2
        .balignw 8,0x21fb,6
        .byte 3,2
        .balignw 8,0x21fb,5
        .byte 1,1,1,1,1,1
        .align 8
        .align 8,0xfff
        .p2align 3,0xfff
        .byte 1
        .balign 8,0xfff
        .short 1
        .balignw 8,0xfecda
        .long 1
        .balignl 8,0x1313321fecda)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
            {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00, 0x00,
                0x01, 0x02, 0x03, 0x04, 0x05, 0xfa, 0xfa, 0xfa,
                0x01, 0x02, 0x03, 0x04, 0x05, 0xfa, 0xfa, 0xfa,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x01, 0x01,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00, 0x00,
                0x01, 0x02, 0x03, 0x04, 0x05, 0xfb, 0xfb, 0xfb,
                0x01, 0x02, 0x03, 0x04, 0x05, 0xfb, 0xfb, 0xfb,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x01, 0x01,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00, 0x00,
                0x01, 0x02, 0x03, 0x04, 0x05, 0xeb, 0xeb, 0xeb,
                0x01, 0x02, 0x03, 0x04, 0x05, 0xeb, 0xeb, 0xeb,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x01, 0x01,
                0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x03, 0x02, 0xfb, 0x21, 0xfb, 0x21, 0xfb, 0x21,
                0x03, 0x02, 0xfb, 0x21, 0xfb, 0x21, 0xfb, 0x21,
                0x03, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                0x01, 0x00, 0xda, 0xec, 0xda, 0xec, 0xda, 0xec,
                0x01, 0x00, 0x00, 0x00, 0xda, 0xec, 0x1f, 0x32
            } } },
        { { ".", 152U, 0, 0U, true, false, false, 0, 0 } },
        true, "test.s:41:18: Warning: Value 0xfff truncated to 0xff\n"
        "test.s:42:20: Warning: Value 0xfff truncated to 0xff\n"
        "test.s:44:19: Warning: Value 0xfff truncated to 0xff\n"
        "test.s:46:20: Warning: Value 0xfecda truncated to 0xecda\n"
        "test.s:48:20: Warning: Value 0x1313321fecda truncated to 0x321fecda\n", ""
    },
    /* 18 - space, skip */
    {   R"ffDXD(            .skip 12,0xfd
            .skip 7
            .space 12,0xfd
            .space 7
            .skip 12,-0xfd
            .skip 7
            .space 12,-0xfd
            .space 7)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
            {
                0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd,
                0xfd, 0xfd, 0xfd, 0xfd, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd,
                0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03,
                0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
                0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00
            } } },
        { { ".", 76U, 0, 0U, true, false, false, 0, 0 } },
        true, "test.s:5:22: Warning: Value 0xffffffffffffff03 truncated to 0x3\n"
        "test.s:7:23: Warning: Value 0xffffffffffffff03 truncated to 0x3\n", ""
    },
    {   R"ffDXD(. = 1
            .int 2,3
            .org .+6
            .int 12
            . = .+6
            .int 23
            .set .,.+4
            .int 65
            .equ .,.+4
            .int 77
            .equiv .,.+6
            .int 21
            .org 80,0xfa)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { { nullptr, AsmSectionType::AMD_GLOBAL_DATA,
            {
                0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xfa,
                0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa,
                0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa,
                0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa
            } } },
        { { ".", 80U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    }
};

static void testAssembler(cxuint testId, const AsmTestCase& testCase)
{
    std::istringstream input(testCase.input);
    std::ostringstream errorStream;
    std::ostringstream printStream;
    
    Assembler assembler("test.s", input, ASM_ALL, BinaryFormat::AMD,
            GPUDeviceType::CAPE_VERDE, errorStream, printStream);
    bool good = assembler.assemble();
    /* compare results */
    char testName[30];
    snprintf(testName, 30, "Test #%u", testId);
    
    assertValue(testName, "good", int(testCase.good), int(good));
    assertValue(testName, "format", int(testCase.format),
                int(assembler.getBinaryFormat()));
    assertValue(testName, "deviceType", int(testCase.deviceType),
                int(assembler.getDeviceType()));
    assertValue(testName, "64bit", int(testCase.is64Bit), int(assembler.is64Bit()));
    
    // check sections
    const std::vector<AsmSection*>& resSections = assembler.getSections();
    assertValue(testName, "sections.length", testCase.sections.size(), resSections.size());
    for (size_t i = 0; i < testCase.sections.size(); i++)
    {
        std::ostringstream caseOss;
        caseOss << "Section#" << i << ".";
        caseOss.flush();
        std::string caseName(caseOss.str());
        
        const AsmSection& resSection = *(resSections[i]);
        const Section& expSection = testCase.sections[i];
        assertValue(testName, caseName+"type", int(expSection.type), int(resSection.type));
        assertArray<cxbyte>(testName, caseName+"content", expSection.content,
                    resSection.content);
    }
    // check symbols
    const AsmSymbolMap& resSymbolMap = assembler.getSymbolMap();
    assertValue(testName, "symbols.length", testCase.symbols.size(), resSymbolMap.size());
    
    std::vector<const AsmSymbolEntry*> symEntries;
    for (const AsmSymbolEntry& symEntry: assembler.getSymbolMap())
        symEntries.push_back(&symEntry);
    std::sort(symEntries.begin(), symEntries.end(),
                [](const AsmSymbolEntry* s1, const AsmSymbolEntry* s2)
                { return s1->first < s2->first; });
    
    for (size_t i = 0; i < testCase.symbols.size(); i++)
    {
        std::ostringstream caseOss;
        caseOss << "Symbol#" << i << ".";
        caseOss.flush();
        std::string caseName(caseOss.str());
        
        const AsmSymbolEntry& resSymbol = *(symEntries[i]);
        const SymEntry& expSymbol = testCase.symbols[i];
        assertString(testName,caseName+"name", expSymbol.name, resSymbol.first);
        assertValue(testName,caseName+"value", expSymbol.value, resSymbol.second.value);
        assertValue(testName,caseName+"sectId", expSymbol.sectionId,
                     resSymbol.second.sectionId);
        assertValue(testName,caseName+"size", expSymbol.size, resSymbol.second.size);
        assertValue(testName,caseName+"isDefined", int(expSymbol.isDefined),
                    int(resSymbol.second.isDefined));
        assertValue(testName,caseName+"onceDefined", int(expSymbol.onceDefined),
                    int(resSymbol.second.onceDefined));
        assertValue(testName,caseName+"base", int(expSymbol.base),
                    int(resSymbol.second.base));
        assertValue(testName,caseName+"info", int(expSymbol.info),
                    int(resSymbol.second.info));
        assertValue(testName,caseName+"other", int(expSymbol.other),
                    int(resSymbol.second.other));
    }
    errorStream.flush();
    printStream.flush();
    const std::string errorMsgs = errorStream.str();
    const std::string printMsgs = printStream.str();
    assertString(testName, "errorMessages", testCase.errorMessages, errorMsgs);
    assertString(testName, "printMessages", testCase.printMessages, printMsgs);
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
