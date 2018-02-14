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

const AsmTestCase asmTestCases1Tbl[] =
{
    /* 0 empty */
    { "", BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false,
        { }, { }, { { ".", 0, 0, 0, true, false, false, 0, 0 } }, true, "", "" },
    /* 1 standard symbol assignment */
    {   R"ffDXD(sym1 = 7
        sym2 = 81
        sym3 = sym7*sym4
        sym4 = sym5*sym6+sym7 - sym1
        sym5 = 17
        sym6 = 43
        sym7 = 91)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
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
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
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
        valx = 122
        valx = x+y+z
        .equiv valx,f+f
        .eqv valx,f+f
labelx:
        .equiv labelx,f+f
        .eqv labelx,f+f
        )ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 0xfa, 0xfd, 0xfb, 0xda, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0b, 0x64, 0x64,
              0x78, 0x82, 0x82, 0x82 } } },
        {
            { ".", 16, 0, 0, true, false, false, 0, 0 },
            { "1b", 6, 0, 0, true, false, false, 0, 0 },
            { "1f", 6, 0, 0, false, false, false, 0, 0 },
            { "f", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "lab1", 16, 0, 0, true, true, false, 0, 0 },
            { "lab2", 16, 0, 0, true, true, false, 0, 0 },
            { "lab3", 16, 0, 0, true, true, false, 0, 0 },
            { "labelx", 16U, 0, 0U, true, true, false, 0, 0 },
            { "myval", 9, ASMSECT_ABS, 0, true, false, false, 0, 0, },
            { "start", 0, 0, 0, true, true, false, 0, 0 },
            { "testx", 130, ASMSECT_ABS, 0, true, true, false, 0, 0 },
            { "valx", 122U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "y", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "z", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "zx", 10, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "zy", 11, ASMSECT_ABS, 0, true, false, false, 0, 0 },
            { "zz", 120, ASMSECT_ABS, 0, true, false, false, 0, 0 },
        }, false,
        "test.s:3:1: Error: Symbol 'start' is already defined\n"
        "test.s:4:9: Error: Symbol 'start' is already defined\n"
        "test.s:10:9: Error: Illegal number at statement begin\n"
        "test.s:27:16: Error: Symbol 'testx' is already defined\n"
        "test.s:30:16: Error: Symbol 'myval' is already defined\n"
        "test.s:31:16: Error: Symbol 'myval' is already defined\n"
        "test.s:35:9: Error: Symbol 'testx' is already defined\n"
        "test.s:36:14: Error: Symbol 'testx' is already defined\n"
        "test.s:37:1: Error: Symbol 'testx' is already defined\n"
        "test.s:44:16: Error: Symbol 'valx' is already defined\n"
        "test.s:45:14: Error: Symbol 'valx' is already defined\n"
        "test.s:47:16: Error: Symbol 'labelx' is already defined\n"
        "test.s:48:14: Error: Symbol 'labelx' is already defined\n", ""
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x94, 0, 0, 0, 0xe, 0, 0, 0 } } },
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 0, 0, 0, 0, 0, 0, 0, 0 } } },
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
        true, R"ffDXD(Expression evaluation from test.s:3:23:
                      from test.s:4:18:
test.s:1:24: Warning: Shift count out of range (between 0 and 63)
Expression evaluation from test.s:6:22:
                      from test.s:9:23:
                      from test.s:10:18:
test.s:7:24: Warning: Shift count out of range (between 0 and 63)
Expression evaluation from test.s:9:23:
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 0, 0, 0, 0, 0, 0, 0, 0 } } },
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
            uuxu = 34 ````
            .set udu, 3445   ``````)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "aaa", 0U, 0, 0U, true, true, false, 0, 0 },
        },
        false, R"ffDXD(test.s:1:19: Error: Expected assignment expression
test.s:2:17: Error: Expected symbol
test.s:2:17: Error: Expected ',' before argument
test.s:3:21: Error: Expected ',' before argument
test.s:4:23: Error: Expected assignment expression
test.s:5:1: Error: Illegal number at statement begin
test.s:6:6: Error: Symbol 'aaa' is already defined
test.s:7:13: Error: Unknown instruction
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
            .string64 "alad",,"ma1","6kota"
            .string "\x9b23;" # changed hex character parsing
            .short 0x10000)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
                0x00, 0x00, 0x00, 0x23, 0x3b, 0x00, 0x00, 0x00
            } } },
        { { ".", 528U, 0, 0U, true, false, false, 0, 0 } },
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
test.s:20:20: Warning: Value 0x10000 truncated to 0x0
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
    /* 19 - .org and '.' */
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
            .org 80,0xfa
            .org .+4,0xcdaaa
            .eqv .,.+6)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
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
                0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa,
                0xaa, 0xaa, 0xaa, 0xaa, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00
            } } },
        { { ".", 90U, 0, 0U, true, false, false, 0, 0 } },
        true, "test.s:14:22: Warning: Value 0xcdaaa truncated to 0xaa\n", ""
    },
    /* 20 - .org and '.' (rawcode) */
    {   R"ffDXD(.rawcode
            . = 1
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
            .org 80,0xfa
            .org .+4,0xcdaaa
            .eqv .,.+6)ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
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
                0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa, 0xfa,
                0xaa, 0xaa, 0xaa, 0xaa, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00
            } } },
        { { ".", 90U, 0, 0U, true, false, false, 0, 0 } },
        true, "test.s:15:22: Warning: Value 0xcdaaa truncated to 0xaa\n", ""
    },
    /* 21 - globals, locals and sizes */
    {   R"ffDXD(            .global x1, xx2,ddx
            v0 = 10
            v1 = 11
            v2 = 33
vl:
            .global v0,v1,v2,vl
            .local lx1, lxx2,lddx
            lv0 = 10
            lv1 = 11
            lv2 = 33
lvl:
            .local lv0,lv1,lv2,lvl
            .weak wx1, wxx2,wddx
            wv0 = 10
            wv1 = 11
            wv2 = 33
wvl:
            .weak wv0,wv1,wv2,wvl
            .size wv0,5
            .size v1,7
            .eqv ddvt,77
            .global ddvt,.,ddvt
            .size .,55
            # extern should ignored
            .extern ala,ma,kota)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "ddvt", 77U, ASMSECT_ABS, 0U, true, true, false, 16, 0 },
            { "ddx", 0U, ASMSECT_ABS, 0U, false, false, false, 16, 0 },
            { "lddx", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "lv0", 10U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "lv1", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "lv2", 33U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "lvl", 0U, 0, 0U, true, true, false, 0, 0 },
            { "lx1", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "lxx2", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "v0", 10U, ASMSECT_ABS, 0U, true, false, false, 16, 0 },
            { "v1", 11U, ASMSECT_ABS, 7U, true, false, false, 16, 0 },
            { "v2", 33U, ASMSECT_ABS, 0U, true, false, false, 16, 0 },
            { "vl", 0U, 0, 0U, true, true, false, 16, 0 },
            { "wddx", 0U, ASMSECT_ABS, 0U, false, false, false, 32, 0 },
            { "wv0", 10U, ASMSECT_ABS, 5U, true, false, false, 32, 0 },
            { "wv1", 11U, ASMSECT_ABS, 0U, true, false, false, 32, 0 },
            { "wv2", 33U, ASMSECT_ABS, 0U, true, false, false, 32, 0 },
            { "wvl", 0U, 0, 0U, true, true, false, 32, 0 },
            { "wx1", 0U, ASMSECT_ABS, 0U, false, false, false, 32, 0 },
            { "wxx2", 0U, ASMSECT_ABS, 0U, false, false, false, 32, 0 },
            { "x1", 0U, ASMSECT_ABS, 0U, false, false, false, 16, 0 },
            { "xx2", 0U, ASMSECT_ABS, 0U, false, false, false, 16, 0 }
        }, true, "test.s:22:26: Warning: Symbol '.' is ignored\n"
            "test.s:23:19: Warning: Symbol '.' is ignored\n", ""
    },
    /* 22 - messages */
    {   R"ffDXD(            .print "to jest test"
            .warning "o rety"
            .error "aaarrrgggg!!!!"
            .fail 510
            .fail 110
            .fail -1
            .fail
            .err
            .abort)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        false, R"ffDXD(test.s:2:13: Warning: o rety
test.s:3:13: Error: aaarrrgggg!!!!
test.s:4:13: Warning: .fail 510 encountered
test.s:5:13: Error: .fail 110 encountered
test.s:6:13: Error: .fail -1 encountered
test.s:7:18: Error: Expected expression
test.s:8:13: Error: .err encountered
test.s:9:13: Error: Aborted!
)ffDXD", "to jest test\n"
    },
    /* 23 - pseudo-ops errors */
    {   R"ffDXD(            .byte 22,12+,  55,1*, 7, `
            .hword 22,12+,  55,1*, 7, `
            .short 22,12+,  55,1*, 7, `
            .word 22,12+,  55,1*, 7, `
            .int 22,12+,  55,1*, 7, `
            .long 22,12+,  55,1*, 7, `
            .long 22,12+,  55,1*, 7, `
            .long 111111111111111111111,333333,1111111111111111111111111111
            .half 1.3544, 1.341e3, 1e10000, 76.233e, %
            .float 1.3544, 1.341e3, 1e10000, 76.233e, %
            .double 1.3544, 1.341e3, 1e10000, 76.233e, %
            .octa 1233, `
            .ascii "aaa", "aaax", ::, 2344, '34'
            .asciz "aaa", "aaax", ::, 2344, '34'
            .string "aaa", "aaax", ::, 2344, '34'
            .string16 "aaa", "aaax", ::, 2344, '34'
            .string32 "aaa", "aaax", ::, 2344, '34'
            .string64 "aaa", "aaax", ::, 2344, '34'
            .fill ,,
            .fillq ,,
            .skip ,
            .space ,,
            .align 3,5,1
            .balign 1,2,2
            .p2align 634
            .global ,,,
            .local ,,,
            .weak ,,,
            .size 3343,aa
            .size a1221, 
            .size a1221, ```
            .extern ,,,,
            .print 23233
            .error xxx
            .extern 65,88,,
            .global
            .fill -1,0`
            .fill 0,-1`)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x16, 0x37, 0x07, 0x00, 0x16, 0x00, 0x37, 0x00,
                0x07, 0x00, 0x00, 0x00, 0x16, 0x00, 0x37, 0x00,
                0x07, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
                0x37, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
                0x37, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
                0x37, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
                0x37, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x6b, 0x3d, 0x3d, 0x65,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb, 0x5c,
                0xad, 0x3f, 0x00, 0xa0, 0xa7, 0x44, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x08, 0x3d, 0x9b, 0x55, 0x9f, 0xab,
                0xf5, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4,
                0x94, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0xd1, 0x04, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
                0x78, 0x61, 0x61, 0x61, 0x00, 0x61, 0x61, 0x61,
                0x78, 0x00, 0x61, 0x61, 0x61, 0x00, 0x61, 0x61,
                0x61, 0x78, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61,
                0x00, 0x00, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61,
                0x00, 0x78, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00,
                0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00
            } } },
        {
            { ".", 338U, 0, 0U, true, false, false, 0, 0 },
            { "a1221", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 }
        },
        false, R"ffDXD(test.s:1:25: Error: Unterminated expression
test.s:1:33: Error: Unterminated expression
test.s:1:38: Warning: No expression, zero has been put
test.s:1:38: Error: Expected ',' before next value
test.s:2:26: Error: Unterminated expression
test.s:2:34: Error: Unterminated expression
test.s:2:39: Warning: No expression, zero has been put
test.s:2:39: Error: Expected ',' before next value
test.s:3:26: Error: Unterminated expression
test.s:3:34: Error: Unterminated expression
test.s:3:39: Warning: No expression, zero has been put
test.s:3:39: Error: Expected ',' before next value
test.s:4:25: Error: Unterminated expression
test.s:4:33: Error: Unterminated expression
test.s:4:38: Warning: No expression, zero has been put
test.s:4:38: Error: Expected ',' before next value
test.s:5:24: Error: Unterminated expression
test.s:5:32: Error: Unterminated expression
test.s:5:37: Warning: No expression, zero has been put
test.s:5:37: Error: Expected ',' before next value
test.s:6:25: Error: Unterminated expression
test.s:6:33: Error: Unterminated expression
test.s:6:38: Warning: No expression, zero has been put
test.s:6:38: Error: Expected ',' before next value
test.s:7:25: Error: Unterminated expression
test.s:7:33: Error: Unterminated expression
test.s:7:38: Warning: No expression, zero has been put
test.s:7:38: Error: Expected ',' before next value
test.s:8:19: Error: Number out of range
test.s:8:39: Error: Expected ',' before next value
test.s:8:40: Error: Garbages at end of line
test.s:9:36: Error: Absolute value of number is too big
test.s:9:45: Error: Garbages at floating point exponent
test.s:9:54: Error: Floating point doesn't have value part!
test.s:9:54: Error: Expected ',' before next value
test.s:10:37: Error: Absolute value of number is too big
test.s:10:46: Error: Garbages at floating point exponent
test.s:10:55: Error: Floating point doesn't have value part!
test.s:10:55: Error: Expected ',' before next value
test.s:11:38: Error: Absolute value of number is too big
test.s:11:47: Error: Garbages at floating point exponent
test.s:11:56: Error: Floating point doesn't have value part!
test.s:11:56: Error: Expected ',' before next value
test.s:12:25: Error: Missing number
test.s:12:25: Error: Expected ',' before next value
test.s:13:35: Error: Expected string
test.s:13:39: Error: Expected string
test.s:13:45: Error: Expected string
test.s:14:35: Error: Expected string
test.s:14:39: Error: Expected string
test.s:14:45: Error: Expected string
test.s:15:36: Error: Expected string
test.s:15:40: Error: Expected string
test.s:15:46: Error: Expected string
test.s:16:38: Error: Expected string
test.s:16:42: Error: Expected string
test.s:16:48: Error: Expected string
test.s:17:38: Error: Expected string
test.s:17:42: Error: Expected string
test.s:17:48: Error: Expected string
test.s:18:38: Error: Expected string
test.s:18:42: Error: Expected string
test.s:18:48: Error: Expected string
test.s:19:19: Error: Expected expression
test.s:20:20: Error: Expected expression
test.s:22:21: Error: Garbages at end of line
test.s:23:20: Error: Alignment is not power of 2
test.s:25:22: Error: Power of 2 of alignment is greater than 63
test.s:26:21: Error: Expected symbol name
test.s:26:22: Error: Expected symbol name
test.s:26:23: Error: Expected symbol name
test.s:26:24: Error: Expected symbol name
test.s:27:20: Error: Expected symbol name
test.s:27:21: Error: Expected symbol name
test.s:27:22: Error: Expected symbol name
test.s:27:23: Error: Expected symbol name
test.s:28:19: Error: Expected symbol name
test.s:28:20: Error: Expected symbol name
test.s:28:21: Error: Expected symbol name
test.s:28:22: Error: Expected symbol name
test.s:29:19: Error: Expected symbol name
test.s:29:24: Error: Expression have unresolved symbol 'aa'
test.s:30:26: Error: Expected expression
test.s:31:26: Error: Expected expression
test.s:32:21: Error: Expected symbol name
test.s:32:22: Error: Expected symbol name
test.s:32:23: Error: Expected symbol name
test.s:32:24: Error: Expected symbol name
test.s:32:25: Error: Expected symbol name
test.s:33:20: Error: Expected string
test.s:34:20: Error: Expected string
test.s:35:21: Error: Expected symbol name
test.s:35:24: Error: Expected symbol name
test.s:35:27: Error: Expected symbol name
test.s:35:28: Error: Expected symbol name
test.s:36:20: Error: Expected symbol name
test.s:37:19: Warning: Negative repeat has no effect
test.s:37:23: Error: Expected ',' before argument
test.s:38:21: Warning: Negative size has no effect
test.s:38:23: Error: Expected ',' before argument
)ffDXD", ""
    },
    /* 24 - illegal output counter change 1 */
    {   R"ffDXD(            . = -6
            .org .-8
            .org .-11
            .set .,.-11
            .int 1,2,4
            . = 3
            . = 7
            . = .-6)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
              0x04, 0x00, 0x00, 0x00 } } },
        { { ".", 12U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:1:13: Error: Attempt to move backwards\n"
        "test.s:2:18: Error: Attempt to move backwards\n"
        "test.s:3:18: Error: Attempt to move backwards\n"
        "test.s:4:18: Error: Attempt to move backwards\n"
        "test.s:6:13: Error: Attempt to move backwards\n"
        "test.s:7:13: Error: Attempt to move backwards\n"
        "test.s:8:13: Error: Attempt to move backwards\n", ""
    },
    /* 25 - illegal output counter change 2 (rawcode) */
    {   R"ffDXD(.rawcode
            . = -6
            .org .-8
            .org .-11
            .set .,.-11
            .int 1,2,4
            . = 3
            . = 7
            . = .-6)ffDXD",
        BinaryFormat::RAWCODE, GPUDeviceType::CAPE_VERDE, false, { },
        { { ".text", ASMKERN_GLOBAL, AsmSectionType::CODE,
            { 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
              0x04, 0x00, 0x00, 0x00 } } },
        { { ".", 12U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:2:13: Error: Attempt to move backwards\n"
        "test.s:3:18: Error: Attempt to move backwards\n"
        "test.s:4:18: Error: Attempt to move backwards\n"
        "test.s:5:18: Error: Attempt to move backwards\n"
        "test.s:7:13: Error: Attempt to move backwards\n"
        "test.s:8:13: Error: Attempt to move backwards\n"
        "test.s:9:13: Error: Attempt to move backwards\n", ""
    },
    /* 26 - negative repeats,sizes */
    {   R"ffDXD(           .fill -7,0,45
           .fill 6,-7,3
           .fill -7,0,3
           .fill 6,-7,43
           .space -73,4
           .skip -563,14)ffDXD",
       BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, {  } } },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        true, "test.s:1:18: Warning: Negative repeat has no effect\n"
        "test.s:2:20: Warning: Negative size has no effect\n"
        "test.s:3:18: Warning: Negative repeat has no effect\n"
        "test.s:4:20: Warning: Negative size has no effect\n"
        "test.s:5:19: Warning: Negative size has no effect\n"
        "test.s:6:18: Warning: Negative size has no effect\n", ""
    },
    /* 27 */
    {   "           z =  (1 << 7\\\n7) + (343 /\\\n* */ << 64 ) + (11<\\\n\\\n\\\n<77)\n"
        "        .string \"aaa\\\nvvv\":",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x61, 0x61, 0x61, 0x76, 0x76, 0x76, 0x00
            } } },
        {
            { ".", 7U, 0, 0U, true, false, false, 0, 0 },
            { "z", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        false, "test.s:1:20: Warning: Shift count out of range (between 0 and 63)\n"
        "test.s:3:6: Warning: Shift count out of range (between 0 and 63)\n"
        "test.s:3:19: Warning: Shift count out of range (between 0 and 63)\n"
        "test.s:8:5: Error: Expected ',' before next value\n", ""
    },
    /* 28 - Upper pseudo-ops names */
    {   ".FILL 5,,15\n",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 15, 15, 15, 15, 15 } } },
        { { ".", 5U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /*  29 - */
    {
        R"ffDXD(                .eqv x2,y1+y2*z-z2
                .fill x2,5,6
                .equ x,y1+y2*z-z2
                .fill x,5,6)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "x", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "x2", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 },
            { "y1", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "y2", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "z", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "z2", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 }
        },
        false, "test.s:2:23: Error: Expression have unresolved symbol 'x2'\n"
        "test.s:4:23: Error: Expression have unresolved symbol 'x'\n", ""
    },
    /* 30 */
    {   R"ffDXD(            .eqv x2,5+vx
            .equ vx,2
            .fill x2,1,33
            .equ x,20-vy
            .equ vy,9
            .fill x,1,93)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x5d,
                0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d,
                0x5d, 0x5d
            } } },
        {
            { ".", 18U, 0, 0U, true, false, false, 0, 0 },
            { "vx", 2U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "vy", 9U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "x2", 0U, ASMSECT_ABS, 0U, false, true, true, 0, 0 }
        }, true, "", ""
    },
    /* 31 - multiple statement in single line */
    {   R"ffDXD(            .string "abc;", "ab\";","ab\\"; .byte 0xff
            .byte '\''; .byte ';'; .byte 0x8a
            .byte 1; .int 2; ;)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x61, 0x62, 0x63, 0x3b, 0x00, 0x61, 0x62, 0x22,
                0x3b, 0x00, 0x61, 0x62, 0x5c, 0x00, 0xff, 0x27,
                0x3b, 0x8a, 0x01, 0x02, 0x00, 0x00, 0x00
            } } },
        { { ".", 23U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 32 */
    {   R"ffDXD(            .string "abc;", "ab\";","ab\\"; .byte 0xff,~
            .byte '\''; .byte ';',; .byte 0x8a
            .byte 1; .int 2 x; ;)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x61, 0x62, 0x63, 0x3b, 0x00, 0x61, 0x62, 0x22,
                0x3b, 0x00, 0x61, 0x62, 0x5c, 0x00, 0xff, 0x27,
                0x3b, 0x00, 0x8a, 0x01, 0x02, 0x00, 0x00, 0x00
            } } },
        { { ".", 24U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:1:57: Error: Unterminated expression\n"
        "test.s:2:35: Warning: No expression, zero has been put\n"
        "test.s:3:29: Error: Expected ',' before next value\n", ""
    },
    /* 33 */
    {   R"ffDXD(            .string "abc;", "ab\";","ab\\"; .byte 0xff
            .byte '\''; .byte ';'; .byte 0x8a
            .byte 1; .fill uuu,; \
.int , 2 x; ; .fill xxx,; .fill yyy)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 
                0x61, 0x62, 0x63, 0x3b, 0x00, 0x61, 0x62, 0x22,
                0x3b, 0x00, 0x61, 0x62, 0x5c, 0x00, 0xff, 0x27,
                0x3b, 0x8a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02,
                0x00, 0x00, 0x00
            } } },
        { { ".", 27U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:3:28: Error: Expression have unresolved symbol 'uuu'\n"
        "test.s:4:6: Warning: No expression, zero has been put\n"
        "test.s:4:10: Error: Expected ',' before next value\n"
        "test.s:4:21: Error: Expression have unresolved symbol 'xxx'\n"
        "test.s:4:33: Error: Expression have unresolved symbol 'yyy'\n", ""
    },
    /* 34 - if statements */
    {   R"ffDXD(            .if 1
            .byte 1
            .endif
            .if 0
            .byte 2
            .endif
            
            .ifne 1
            .byte 3
            .endif
            .ifne 0
            .byte 4
            .endif
            
            .ifle 1
            .byte 5
            .endif
            .ifle 0
            .byte 6
            .endif
            .ifle -1
            .byte 7
            .endif
            
            .iflt 1
            .byte 8
            .endif
            .iflt 0
            .byte 9
            .endif
            .iflt -1
            .byte 10
            .endif
            
            .ifge 1
            .byte 11
            .endif
            .ifge 0
            .byte 12
            .endif
            .ifge -1
            .byte 13
            .endif
            
            .ifgt 1
            .byte 14
            .endif
            .ifgt 0
            .byte 15
            .endif
            .ifgt -1
            .byte 16
            .endif
            
            .ifeq 1
            .byte 17
            .endif
            .ifeq 0
            .byte 18
            .endif
            
            .ifb  
            .byte 21
            .endif
            .ifb  xx
            .byte 22
            .endif
            .ifnb  
            .byte 23
            .endif
            .ifnb  xx
            .byte 24
            .endif
            
            ab = 7
            ac = ax
            .ifdef aaa
            .byte 25
            .endif
            .ifdef ab
            .byte 26
            .endif
            .ifdef ac
            .byte 27
            .endif
            .ifndef aaa
            .byte 28
            .endif
            .ifndef ab
            .byte 29
            .endif
            .ifndef ac
            .byte 30
            .endif
            .ifnotdef aaa
            .byte 31
            .endif
            .ifnotdef ab
            .byte 32
            .endif
            .ifnotdef ac
            .byte 33
            .endif
            
            .ifeqs "ala","ala"
            .byte 41
            .endif
            .ifeqs "ala","alA"
            .byte 42
            .endif
            .ifeqs "ala","alax"
            .byte 43
            .endif
            .ifnes "ala","ala"
            .byte 44
            .endif
            .ifnes "ala","alA"
            .byte 45
            .endif
            .ifnes "ala","alax"
            .byte 46
            .endif
            
            .ifc   buru     , buru
            .byte 47
            .endif
            .ifc   bu ru     , bu  ru
            .byte 48
            .endif
            .ifc   "buru"     , "buru"
            .byte 49
            .endif
            .ifc   "bu ru"     , "bu  ru"
            .byte 50
            .endif
            .ifc   buxru     , buru
            .byte 51
            .endif
            .ifc   buRu     , buru
            .byte 52
            .endif
            .ifnc   buru     , buru
            .byte 53
            .endif
            .ifnc   bu ru     , bu  ru
            .byte 54
            .endif
            .ifnc   "buru"     , "buru"
            .byte 55
            .endif
            .ifnc   "bu ru"     , "bu  ru"
            .byte 56
            .endif
            .ifnc   buxru     , buru
            .byte 57
            .endif
            .ifnc   buRu     , buru
            .byte 58
            .endif)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 
                0x01, 0x03, 0x06, 0x07, 0x0a, 0x0b, 0x0c, 0x0e,
                0x12, 0x15, 0x18, 0x1a, 0x1b, 0x1c, 0x1f, 0x29,
                0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x38, 0x39, 0x3a
            } } },
        {
            { ".", 24U, 0, 0U, true, false, false, 0, 0 },
            { "ab", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "ac", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "ax", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 35 - nesting of ifs */
    {   R"ffDXD(            .if 4
            .byte 4
            .else # to ignore
                .if 5
                    .if 8
                    .endif
                .else
                    .if 7
                    .else
                    .endif
                .endif
            .endif)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 4 } } },
        { { ".", 1U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 36 - if.elseif.else.endif all  ways */
    {   R"ffDXD(            .if 0
            .byte 1
            .elseif 1
            .byte 2
            .else
            .byte 3
            .endif
            
            .if 1
            .byte 4
            .elseif 0
            .byte 5
            .else
            .byte 6
            .endif
            
            .if 0
            .byte 7
            .elseif 0
            .byte 8
            .else
            .byte 9
            .endif)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 2, 4, 9 } } },
        { { ".", 3U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 37 - unterminated else */
    {   R"ffDXD(            .if 123
            .byte 1
            .else
            .byte 22)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 1 } } },
        { { ".", 1U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:3:13: Error: Unterminated '.else'\n"
        "test.s:1:13: Error: here is begin of conditional clause\n", ""
    },
    /* 38 - unterminated elseif */
    {   R"ffDXD(            .if 123
            .byte 1
            .elseif 12
            .byte 22)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 1 } } },
        { { ".", 1U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:3:13: Error: Unterminated '.elseif'\n"
        "test.s:1:13: Error: here is begin of conditional clause\n", ""
    },
    /* 39 - unterminated if */
    {   R"ffDXD(            .if 123
            .byte 1)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 1 } } },
        { { ".", 1U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:1:13: Error: Unterminated '.if'\n", ""
    },
    /* 40 - nested unterminated ifs */
    {   R"ffDXD(            .if 123
            .byte 0
            .if 6
            .byte 1
            .elseif 7
            .byte 2
            .if 8
            .byte 9
            .else
            .byte 10)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 0, 1 } } },
        { { ".", 2U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:9:13: Error: Unterminated '.else'\n"
        "test.s:7:13: Error: here is begin of conditional clause\n"
        "test.s:5:13: Error: Unterminated '.elseif'\n"
        "test.s:3:13: Error: here is begin of conditional clause\n"
        "test.s:1:13: Error: Unterminated '.if'\n", ""
    },
    /* 41 - conditional pseudo-ops: syntax errors */
    {   R"ffDXD(            .if 2*aa+7
            .if 
            .ifle 2*aa+7
            .ifle 
            .iflt 2*aa+7
            .iflt 
            .ifge 2*aa+7
            .ifge 
            .ifgt 2*aa+7
            .ifgt 
            .ifeq 2*aa+7
            .ifeq
            
            .ifc aaa
            .ifeqs 55+5
            .ifeqs "aa",55
            .ifeqs 5,"bda"
            
            .ifnes 55+5
            .ifnes "aa",55
            .ifnes 5,"bda"
            
            .ifdef +++
            .ifdef
            .ifdef 0dd
            
            .if 7
            .else xx
            .endif xx)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { }, { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        false,
        R"ffDXD(test.s:1:19: Error: Expression have unresolved symbol 'aa'
test.s:2:17: Error: Expected expression
test.s:3:21: Error: Expression have unresolved symbol 'aa'
test.s:4:19: Error: Expected expression
test.s:5:21: Error: Expression have unresolved symbol 'aa'
test.s:6:19: Error: Expected expression
test.s:7:21: Error: Expression have unresolved symbol 'aa'
test.s:8:19: Error: Expected expression
test.s:9:21: Error: Expression have unresolved symbol 'aa'
test.s:10:19: Error: Expected expression
test.s:11:21: Error: Expression have unresolved symbol 'aa'
test.s:12:18: Error: Expected expression
test.s:14:21: Error: Missing second string
test.s:15:20: Error: Expected string
test.s:15:24: Error: Expected ',' before argument
test.s:16:25: Error: Expected string
test.s:17:20: Error: Expected string
test.s:19:20: Error: Expected string
test.s:19:24: Error: Expected ',' before argument
test.s:20:25: Error: Expected string
test.s:21:20: Error: Expected string
test.s:23:20: Error: Expected symbol
test.s:24:19: Error: Expected symbol
test.s:25:20: Error: Expected symbol
test.s:28:19: Error: Garbages at end of line
test.s:29:20: Error: Garbages at end of line
test.s:27:13: Error: Unterminated '.if'
)ffDXD", ""
    },
    /* 42 - ifc tests */
    {   R"ffDXD(                .ifc ala ++ ala, ala + +  ala
                .byte 11
                .endif
                .ifc ala -- ala, ala - -  ala
                .byte 12
                .endif
                .ifc ala %% ala, ala % %  ala
                .byte 13
                .endif
                .ifc ala @@ ala, ala @ @  ala
                .byte 14
                .endif
                .ifc ala (( ala, ala ( (  ala
                .byte 15
                .endif
                .ifc ala () ala, ala ( )  ala
                .byte 16
                .endif
                .ifc ala $$ ala, ala $ $  ala
                .byte 17
                .endif
                .ifc ala << ala, ala < <  ala
                .byte 18
                .endif
                .ifc ala :: ala, ala : :  ala
                .byte 19
                .endif
                .ifc ala "::" ala, ala ": :"  ala
                .byte 19
                .endif)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 0x0b, 0x0e, 0x12, 0x13 } } },
        { { ".", 4U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 43 - rept and if */
    {   R"ffDXD(            reptCnt = 1
            .rept 6
            counter = 0
                .rept reptCnt
                .if counter&1
                    .byte reptCnt,counter
                .else
                    .byte reptCnt|0x80,counter
                .endif
                counter = counter+1
                .endr
            reptCnt = reptCnt + 1
            .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x81, 0x00, 0x82, 0x00, 0x02, 0x01, 0x83, 0x00,
                0x03, 0x01, 0x83, 0x02, 0x84, 0x00, 0x04, 0x01,
                0x84, 0x02, 0x04, 0x03, 0x85, 0x00, 0x05, 0x01,
                0x85, 0x02, 0x05, 0x03, 0x85, 0x04, 0x86, 0x00,
                0x06, 0x01, 0x86, 0x02, 0x06, 0x03, 0x86, 0x04,
                0x06, 0x05
            } } },
        {
            { ".", 42U, 0, 0U, true, false, false, 0, 0 },
            { "counter", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "reptCnt", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 44 - */
    {   R"ffDXD(            reptCnt = 1
            .rept 6
            counter = 0
                .rept reptCnt
                .if counter&1
                    .byte reptCnt,counter
                .else
                    .byte reptCnt|0x80,counter
                .endif
                counter = counter+1
                .endr
            counter = 3
                .rept counter
                    .byte 0xc0|counter
                    counter = counter - 1
                .endr
            reptCnt = reptCnt + 1
            .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x81, 0x00, 0xc3, 0xc2, 0xc1, 0x82, 0x00, 0x02,
                0x01, 0xc3, 0xc2, 0xc1, 0x83, 0x00, 0x03, 0x01,
                0x83, 0x02, 0xc3, 0xc2, 0xc1, 0x84, 0x00, 0x04,
                0x01, 0x84, 0x02, 0x04, 0x03, 0xc3, 0xc2, 0xc1,
                0x85, 0x00, 0x05, 0x01, 0x85, 0x02, 0x05, 0x03,
                0x85, 0x04, 0xc3, 0xc2, 0xc1, 0x86, 0x00, 0x06,
                0x01, 0x86, 0x02, 0x06, 0x03, 0x86, 0x04, 0x06,
                0x05, 0xc3, 0xc2, 0xc1
            } } },
        {
            { ".", 60U, 0, 0U, true, false, false, 0, 0 },
            { "counter", 0U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "reptCnt", 7U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, true, "", ""
    },
    /* 45 - unterminated rept */
    {   R"ffDXD(reptCnt = 1
            .rept 6
            counter = 0)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "reptCnt", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        }, false, "test.s:2:13: Error: Unterminated repetition\n", ""
    },
    /* 46 - endr without rept */
    {   R"ffDXD(            .rept 2
            .string "ala"
            .endr
            .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x61, 0x6c, 0x61, 0x00, 0x61, 0x6c, 0x61, 0x00 } } },
        { { ".", 8U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:4:13: Error: No '.rept' before '.endr'\n", ""
    },
    /* 47 - no if on rept */
    {   R"ffDXD(            cnt  = 1
            .rept 0
            .elseif 10
            .else
             cnt = cnt + 1
            .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "cnt", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
        }, true, "", ""
    },
    /* 48 - */
    {
        R"ffDXD(            cnt  = 1
            .if 1
            .rept 0
            .endif
             cnt = cnt + 1
            .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        {
            { ".", 0U, 0, 0U, true, false, false, 0, 0 },
            { "cnt", 1U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
        }, false, "test.s:2:13: Error: Unterminated '.if'\n", ""
    },
    /* 49 - error inside repetitions */
    {   R"ffDXD(            .rept 1
            exe = 4*6; d = f; dd = ; 
            .endr
            .rept 3
            .fill ,, ; .byte 2 ; .fill 4*x ;
            .endr
            .rept 3
                .rept 2
                .byte ----
                .endr
            .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 2, 2, 2 } } },
        {
            { ".", 3U, 0, 0U, true, false, false, 0, 0 },
            { "d", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "exe", 24U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "f", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 }
        }, false, R"ffDXD(In repetition 1/1:
test.s:2:36: Error: Expected assignment expression
In repetition 1/3:
test.s:5:19: Error: Expected expression
In repetition 1/3:
test.s:5:42: Error: Expression have unresolved symbol 'x'
In repetition 2/3:
test.s:5:19: Error: Expected expression
In repetition 2/3:
test.s:5:42: Error: Expression have unresolved symbol 'x'
In repetition 3/3:
test.s:5:19: Error: Expected expression
In repetition 3/3:
test.s:5:42: Error: Expression have unresolved symbol 'x'
In repetition 1/2:
              1/3:
test.s:9:27: Error: Unterminated expression
In repetition 2/2:
              1/3:
test.s:9:27: Error: Unterminated expression
In repetition 1/2:
              2/3:
test.s:9:27: Error: Unterminated expression
In repetition 2/2:
              2/3:
test.s:9:27: Error: Unterminated expression
In repetition 1/2:
              3/3:
test.s:9:27: Error: Unterminated expression
In repetition 2/2:
              3/3:
test.s:9:27: Error: Unterminated expression
)ffDXD", ""
    },
    /* 50 - empty repeats */
    {   R"ffDXD(    .rept 7
        .endr
        .rept 9; .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { }, { },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 51 - basic macro calls */
    {   R"ffDXD(            .macro am1 a,b,c
            .byte \a,\b,\c
            .endm
            
            .macro am1_1 a=56,b,c
            .byte \a,\b,\c
            .endm
            
            .macro am1_2 a,b=7+1,c
            .byte \a,\b,\c
            .endm
            
            .macro am1_3 a,b=7+1,c=6
            .byte \a,\b,\c
            .endm
            
            am1 3,5,7
            am1
            am1 12,77
            
            am1_1 3,5,7
            am1_1 ,-3 , 7
            am1_1 12,77
            
            am1_2 3,5,7
            am1_2 -3,  , 7
            am1_2 12,77
            
            am1_3 3,5,7
            am1_3 -3,  , 7
            am1_3 12,77)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x03, 0x05, 0x07, 0x00, 0x00, 0x00, 0x0c, 0x4d,
                0x00, 0x03, 0x05, 0x07, 0x38, 0xfd, 0x07, 0x0c,
                0x4d, 0x00, 0x03, 0x05, 0x07, 0xfd, 0x08, 0x07,
                0x0c, 0x4d, 0x00, 0x03, 0x05, 0x07, 0xfd, 0x08,
                0x07, 0x0c, 0x4d, 0x06
            } } },
        { { ".", 36U, 0, 0U, true, false, false, 0, 0 } },
        true, R"ffDXD(In macro substituted from test.s:18:13:
test.s:2:19: Warning: No expression, zero has been put
In macro substituted from test.s:18:13:
test.s:2:20: Warning: No expression, zero has been put
In macro substituted from test.s:18:13:
test.s:2:21: Warning: No expression, zero has been put
In macro substituted from test.s:19:13:
test.s:2:25: Warning: No expression, zero has been put
In macro substituted from test.s:23:13:
test.s:6:25: Warning: No expression, zero has been put
In macro substituted from test.s:27:13:
test.s:10:25: Warning: No expression, zero has been put
)ffDXD", ""
    },
    /* 52 - varargs */
    {   R"ffDXD(            .macro varArgs ax,bx,dx:vararg
            .hword \ax*\bx+\bx, \dx
            .endm
            
            varArgs 12,55,4
            varArgs 7,3
            varArgs 7,3,2,4,-5,14
            varArgs 7 3 2,4,-5,14 # without ',')ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0xcb, 0x02, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00,
                0x18, 0x00, 0x02, 0x00, 0x04, 0x00, 0xfb, 0xff,
                0x0e, 0x00, 0x18, 0x00, 0x02, 0x00, 0x04, 0x00,
                0xfb, 0xff, 0x0e, 0x00
            } } },
        { { ".", 28U, 0, 0U, true, false, false, 0, 0 } },
        true, "In macro substituted from test.s:6:13:\n"
        "test.s:2:27: Warning: No expression, zero has been put\n", ""
    },
    /* 53 - required arguments */
    {   R"ffDXD(            .macro reqArg1 af,bf:req,cf
            .ascii "\af:\bf:\cf;"
            .endm
            
            .macro reqArg2 af,bf:req,cf:req
            .ascii "\af:\bf:\cf;"
            .endm
            
            reqArg1 1,32,4
            reqArg1 ,32,
            reqArg1 1,,4
            
            reqArg2 1,32,4
            reqArg2 ,32,23
            reqArg2 ,32
            reqArg2 1,,4)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x31, 0x3a, 0x33, 0x32, 0x3a, 0x34, 0x3b, 0x3a,
                0x33, 0x32, 0x3a, 0x3b, 0x31, 0x3a, 0x33, 0x32,
                0x3a, 0x34, 0x3b, 0x3a, 0x33, 0x32, 0x3a, 0x32,
                0x33, 0x3b
            } } },
        { { ".", 26U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:11:23: Error: Value required for macro argument 'bf'\n"
        "test.s:15:24: Error: Value required for macro argument 'cf'\n"
        "test.s:16:23: Error: Value required for macro argument 'bf'\n", ""
    },
    /* 54 - argument passing */
    {   R"ffDXD(            .macro passer a,b,c,d,e,f
            .asciz "\a:\b:\c:\d:\e:\f"
            .endm
            passer
            passer aa fgf a ! A 41 uu +*+ 8 33
            passer aa * aa "aa * aa" XaF %%%
            passer aa +,ff,^^,33 + 33 - -)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x3a, 0x3a, 0x3a, 0x3a, 0x3a, 0x00, 0x61, 0x61,
                0x3a, 0x66, 0x67, 0x66, 0x3a, 0x61, 0x21, 0x41,
                0x3a, 0x34, 0x31, 0x3a, 0x75, 0x75, 0x2b, 0x2a,
                0x2b, 0x38, 0x3a, 0x33, 0x33, 0x00, 0x61, 0x61,
                0x3a, 0x2a, 0x3a, 0x61, 0x61, 0x3a, 0x61, 0x61,
                0x20, 0x2a, 0x20, 0x61, 0x61, 0x3a, 0x58, 0x61,
                0x46, 0x3a, 0x25, 0x25, 0x25, 0x00, 0x61, 0x61,
                0x2b, 0x3a, 0x66, 0x66, 0x3a, 0x5e, 0x5e, 0x3a,
                0x33, 0x33, 0x2b, 0x33, 0x33, 0x3a, 0x2d, 0x3a,
                0x2d, 0x00,
            } } },
        { { ".", 74U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 55 - call macro in macro */
    {   R"ffDXD(            .macro one x
            .hword 12+\x
            .endm
            .macro two x
            one 3+\x; one 4+\x
            .endm
            .macro three x
            one 11+\x; two \x+\x; two \x*\x
            .endm
            
            three 9)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x20, 0x00, 0x21, 0x00, 0x22, 0x00, 0x60, 0x00, 0x61, 0x00 } } },
        { { ".", 10U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 56 - call with errors */
    {   R"ffDXD(            .macro one x
            .hword 12+\x
            .error "simple error"
            .endm
            .macro two x
            one 3+\x; one 4+\x
            .endm
            .macro three x
            one 11+\x; two \x+\x; two \x*\x
            .endm
            
            three 9)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x20, 0x00, 0x21, 0x00, 0x22, 0x00, 0x60, 0x00, 0x61, 0x00 } } },
        { { ".", 10U, 0, 0U, true, false, false, 0, 0 } },
        false, R"ffDXD(In macro substituted from test.s:9:13;
                     from test.s:12:13:
test.s:3:13: Error: simple error
In macro substituted from test.s:6:13;
                     from test.s:9:23;
                     from test.s:12:13:
test.s:3:13: Error: simple error
In macro substituted from test.s:6:24;
                     from test.s:9:23;
                     from test.s:12:13:
test.s:3:13: Error: simple error
In macro substituted from test.s:6:13;
                     from test.s:9:32;
                     from test.s:12:13:
test.s:3:13: Error: simple error
In macro substituted from test.s:6:24;
                     from test.s:9:32;
                     from test.s:12:13:
test.s:3:13: Error: simple error
)ffDXD", ""
    },
    /* 57 - macro counter */
    {   R"ffDXD(        .macro t3
        .endm
        .macro t2; t3; t3; .endm
        .macro t1; t2; t2; .int \@, \@; .endm
        .rept 5; t1; .endr)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
                0x0e, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
                0x15, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
                0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00
            } } },
        { { ".", 40U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 58 - macro in macro */
    {   R"ffDXD(.macro supply x
        .byte \x*\x+\x
        .endm
        .macro creator name, t, s
        .macro \name x,y
            .int \x+\t,\y*\s
            supply 3
        .endm
        .endm
        creator mymac,9,12
        creator mymax,41,18
        mymac 3,7
        mymax 3,7)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x0c, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
                0x0c, 0x2c, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x0c,
            } } },
        { { ".", 18U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 59 - macro defined in macro */
    {   R"ffDXD(            .macro tx0 name0,name1,t0,t1,t2,t3
            .macro \name0 u0,u1
            .macro \name0\()\name1 x0,x1
            .int \t0+\u0*\x0, \t0+\u1*\x1
            .int \t2+\u0*\x0, \t3+\u1*\x1
            .endm
            .endm
            .endm
            
            tx0 next1 next2 2 3 4 5
            next1 -3 -5
            next1next2 13 37)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0xdb, 0xff, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff,
                0xdd, 0xff, 0xff, 0xff, 0x4c, 0xff, 0xff, 0xff
            } } },
        { { ".", 16U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 60 - macro defined in macro (error) */
    {   R"ffDXD(.macro tx0 name0,name1,t0,t1,t2,t3
            .macro \name0 u0,u1
            .macro \name0\()\name1 x0,x1
            .int \t0+\u0*\x0, \t0+\u1*\x1
            .int \t2+\u0*\x0, \t3+\u1*\x1
            .error "aaaaaaarrrrrgggggggghhhhhhhhh!!!"
            .endm
            .endm
            .endm
            
            tx0 next1 next2 2 3 4 5
            next1 -3 -5
            next1next2 13 37)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0xdb, 0xff, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff,
                0xdd, 0xff, 0xff, 0xff, 0x4c, 0xff, 0xff, 0xff
            } } },
        { { ".", 16U, 0, 0U, true, false, false, 0, 0 } },
        false, R"ffDXD(In macro substituted from test.s:13:13:
In macro content:
    In macro substituted from test.s:12:13:
    In macro content:
        In macro substituted from test.s:11:13:
        test.s:6:13: Error: aaaaaaarrrrrgggggggghhhhhhhhh!!!
)ffDXD", ""
    },
    /* 61 - conditionals in macro */
    {   R"ffDXD(        .macro bstr str1,str2
        .ifc \str1\()xx,\str2\()x
        .ascii "\str1\str2"
        .elseifc \str1\()o,\str2\()Uo
        .ascii "oo\str1\str2"
        .endif
        .endm
        bStr ala,alax
        bStr alaU,ala)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x61, 0x6c, 0x61, 0x61, 0x6c, 0x61, 0x78, 0x6f,
                0x6f, 0x61, 0x6c, 0x61, 0x55, 0x61, 0x6c, 0x61
            } } },
        { { ".", 16U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 62 - exitm */
    {   R"ffDXD(            .macro exiter a1,b2
            .byte \a1,\b2
            .if \a1-3==\b2+5
            .hword 2221
            .exitm
            .endif
            .byte 12,\b2&0xf
            .endm
            exiter 30,70
            exiter 23,15
            .byte 155)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x1e, 0x46, 0x0c, 0x06, 0x17, 0x0f, 0xad, 0x08, 0x9b } } },
        { { ".", 9U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 63 - purgem (undefine macro) */
    {   R"ffDXD(            .macro xxx
            .purgem xXx
            .purgem xxX
            .byte 3,4,5
            .endm
            xxx
            .macro xxx
            .endm)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { 3, 4, 5 } } },
        { { ".", 3U, 0, 0U, true, false, false, 0, 0 } },
        true, "In macro substituted from test.s:6:13:\n"
        "test.s:3:21: Warning: Macro 'xxx' already doesn't exist\n", ""
    },
    /* 64 - macro def without ',' */
    {   R"ffDXD(            .macro test1 a b c d
            .hword \a, \b, \c, \d
            .endm
            .macro test2 a b = 6 c:req d
            .hword \a, \b, \c, \d
            .endm
            
            test1 1 2 3 4
            test2 1 "" 3 4)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            {
                0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00,
                0x01, 0x00, 0x06, 0x00, 0x03, 0x00, 0x04, 0x00
            } } },
        { { ".", 16U, 0, 0U, true, false, false, 0, 0 } },
        true, "", ""
    },
    /* 65 - macro error handling */
    {   R"ffDXD(            .macro test1 a b c d:vararg=xx,xx
            .macro test1 a=aa * a
            .macro test1 a,b,b,1x
            .macro test1 a1,num=5,dd:req:vararg
            .macro test1 a1,num=5,xx= xx   ""
            test1 33, 554 , aa)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        /* test1 - initializes output format, hence new section */
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA, { } } },
        { { ".", 0U, 0, 0U, true, false, false, 0, 0 } },
        false, "test.s:1:44: Error: Variadic argument must be last\n"
        "test.s:2:31: Error: Expected macro argument name\n"
        "test.s:3:30: Error: Duplicated macro argument 'b'\n"
        "test.s:3:32: Error: Expected macro argument name\n"
        "test.s:4:41: Error: Expected macro argument name\n"
        "test.s:5:44: Error: Expected macro argument name\n"
        "test.s:6:13: Error: Unknown instruction\n", ""
    },
    /* 66 - evaluate old expressions of symbols */
    {
        R"ffDXD(.int aa0
aa1=aa2
aa0=aa1
aa1=bb2
bb0=aa1
aa1=3
aa2=6
bb2=11)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x06, 0x00, 0x00, 0x00 } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "aa0", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "aa1", 3U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "aa2", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "bb0", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "bb2", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    /* 67 - evaluate old expressions of symbols (after undef symbol) */
    {
        R"ffDXD(.int aa0
aa1=aa2
aa0=aa1
aa1=bb2
bb0=aa1
.undef aa1
aa2=6
bb2=11)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x06, 0x00, 0x00, 0x00 } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "aa0", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "aa1", 0U, ASMSECT_ABS, 0U, false, false, false, 0, 0 },
            { "aa2", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "bb0", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "bb2", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    /* 68 - evaluate old expressions of symbols (replaced by regrange) */
    {
        R"ffDXD(.int aa0
aa1=aa2
aa0=aa1
aa1=bb2
bb0=aa1
aa1=%v[1:2]
aa2=6
bb2=11)ffDXD",
        BinaryFormat::AMD, GPUDeviceType::CAPE_VERDE, false, { },
        { { nullptr, ASMKERN_GLOBAL, AsmSectionType::DATA,
            { 0x06, 0x00, 0x00, 0x00 } } },
        {
            { ".", 4U, 0, 0U, true, false, false, 0, 0 },
            { "aa0", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "aa1", 1112396529921U, ASMSECT_ABS, 0U, true, false, false, 0, 0, true },
            { "aa2", 6U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "bb0", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 },
            { "bb2", 11U, ASMSECT_ABS, 0U, true, false, false, 0, 0 }
        },
        true, "", ""
    },
    { nullptr }
};
