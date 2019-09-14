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
#include <cstring>
#include <cstdio>
#include <vector>
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include "../TestUtils.h"

using namespace CLRX;

static void testMsgPackBytesWrite()
{
    {
        static const cxbyte outBytes0[3] = { 0x92, 0xc3, 0xc2 };
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(2, out);
        awriter.putBool(true);
        awriter.putBool(false);
        assertArray("MsgPackWrite0", "tc0",
                    Array<cxbyte>(outBytes0, outBytes0 + sizeof(outBytes0)), out);
    }
    {
        static const cxbyte outBytes1[16] = { 0x9f, 0, 1, 2, 3, 4, 5, 6, 7,
            8, 9, 10, 11, 12, 13, 14 };
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(15, out);
        for (cxuint i = 0; i < 15; i++)
            awriter.putUInt(i);
        assertArray("MsgPackWrite0", "tc1",
                    Array<cxbyte>(outBytes1, outBytes1 + sizeof(outBytes1)), out);
    }
    {   // longer array
        static const cxbyte outBytes2[21] = { 0xdc, 0x0, 0x12, 0, 1, 2, 3, 4, 5, 6, 7,
            8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(18, out);
        for (cxuint i = 0; i < 18; i++)
            awriter.putUInt(i);
        assertArray("MsgPackWrite0", "tc2",
                    Array<cxbyte>(outBytes2, outBytes2 + sizeof(outBytes2)), out);
    }
    {   // long array (12222 elements)
        Array<cxbyte> outBytes3(12222+3);
        outBytes3[0] = 0xdc;
        outBytes3[1] = 12222>>8;
        outBytes3[2] = 12222&0xff;
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(12222, out);
        for (cxuint i = 0; i < 12222; i++)
        {
            awriter.putUInt(i&0x7f);
            outBytes3[i+3] = i&0x7f;
        }
        assertArray("MsgPackWrite0", "tc3", outBytes3, out);
    }
    {   // long array (1777777 elements)
        Array<cxbyte> outBytes4(1777777+5);
        outBytes4[0] = 0xdd;
        outBytes4[1] = 1777777>>24;
        outBytes4[2] = (1777777>>16)&0xff;
        outBytes4[3] = (1777777>>8)&0xff;
        outBytes4[4] = 1777777&0xff;
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(1777777, out);
        for (cxuint i = 0; i < 1777777; i++)
        {
            awriter.putUInt(i&0x7f);
            outBytes4[i+5] = i&0x7f;
        }
        assertArray("MsgPackWrite0", "tc4", outBytes4, out);
    }
    /* test putting values */
    {
        static const cxbyte outBytes5[21] = { 0x95, 0x12, 0xcc, 0xd1, 0xcd, 0xab, 0x73,
            0xce, 0xa1, 0x18, 0xb9, 0x31,
            0xcf, 0x11, 0x88, 0x99, 0xa1, 0xb6, 0xa1, 0x9e, 0x1a };
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(5, out);
        awriter.putUInt(0x12);
        awriter.putUInt(0xd1);
        awriter.putUInt(0xab73);
        awriter.putUInt(0xa118b931U);
        awriter.putUInt(0x118899a1b6a19e1aULL);
        assertArray("MsgPackWrite0", "tc5",
                    Array<cxbyte>(outBytes5, outBytes5 + sizeof(outBytes5)), out);
    }
    // testing string putting (short)
    {
        static const cxbyte outBytes6[7] = { 0x91, 0xa5, 0x4a, 0x5b, 0x31, 0x28, 0x44 };
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(1, out);
        awriter.putString("\x4a\x5b\x31\x28\x44");
        assertArray("MsgPackWrite0", "tc6",
                    Array<cxbyte>(outBytes6, outBytes6 + sizeof(outBytes6)), out);
    }
    {
        Array<cxbyte> outBytes7(187+3);
        outBytes7[0] = 0x91;
        outBytes7[1] = 0xd9;
        outBytes7[2] = 187;
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(1, out);
        for (cxuint i = 0; i < 187; i++)
            outBytes7[i+3] = (i&0x7f)+10;
        std::string s(reinterpret_cast<cxbyte*>(outBytes7.data())+3,
                    reinterpret_cast<cxbyte*>(outBytes7.data())+187+3);
        awriter.putString(s.c_str());
        assertArray("MsgPackWrite0", "tc7", outBytes7, out);
    }
    {
        Array<cxbyte> outBytes8(18529+4);
        outBytes8[0] = 0x91;
        outBytes8[1] = 0xda;
        outBytes8[2] = 18529>>8;
        outBytes8[3] = 18529&0xff;
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(1, out);
        for (cxuint i = 0; i < 18529; i++)
            outBytes8[i+4] = (i&0x7f)+10;
        std::string s(reinterpret_cast<cxbyte*>(outBytes8.data())+4,
                    reinterpret_cast<cxbyte*>(outBytes8.data())+18529+4);
        awriter.putString(s.c_str());
        assertArray("MsgPackWrite0", "tc8", outBytes8, out);
    }
    {
        Array<cxbyte> outBytes9(9838391+6);
        outBytes9[0] = 0x91;
        outBytes9[1] = 0xdb;
        outBytes9[2] = 9838391>>24;
        outBytes9[3] = (9838391>>16)&0xff;
        outBytes9[4] = (9838391>>8)&0xff;
        outBytes9[5] = 9838391&0xff;
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(1, out);
        for (cxuint i = 0; i < 9838391; i++)
            outBytes9[i+6] = (i&0x7f)+10;
        std::string s(reinterpret_cast<cxbyte*>(outBytes9.data())+6,
                    reinterpret_cast<cxbyte*>(outBytes9.data())+9838391+6);
        awriter.putString(s.c_str());
        assertArray("MsgPackWrite0", "tc9", outBytes9, out);
    }
    /* map writer class */
    {
        static const cxbyte outBytes10[21] = { 0x83,
            0xa4, 'a', 'b', 'a', 'b', 15,
            0xa3, 'c', 'o', 'b', 0xcd, 0xe9, 0x11,
            0xa5, 'e', 'x', 't', 'r', 'a', 11 };
        std::vector<cxbyte> out;
        MsgPackMapWriter mwriter(3, out);
        mwriter.putKeyString("abab");
        mwriter.putValueUInt(15);
        mwriter.putKeyString("cob");
        mwriter.putValueUInt(0xe911);
        mwriter.putKeyString("extra");
        mwriter.putValueUInt(11);
        assertArray("MsgPackWrite0", "tc10",
                    Array<cxbyte>(outBytes10, outBytes10 + sizeof(outBytes10)), out);
    }
    // longer map testcase
    {
        Array<cxbyte> outBytes11(14821*10+3);
        outBytes11[0] = 0xde;
        outBytes11[1] = 14821>>8;
        outBytes11[2] = 14821&0xff;
        std::vector<cxbyte> out;
        MsgPackMapWriter mwriter(14821, out);
        for (cxuint i = 0; i < 14821; i++)
        {
            char kbuf[8], vbuf[8];
            ::snprintf(kbuf, 8, "%04x", i);
            ::snprintf(vbuf, 8, "%04x", i^0xdca1);
            outBytes11[3+i*10] = 0xa4;
            std::copy(kbuf, kbuf+4, ((char*)outBytes11.data())+3 + i*10 + 1);
            outBytes11[3+i*10 + 5] = 0xa4;
            std::copy(vbuf, vbuf+4, ((char*)outBytes11.data())+3 + i*10 + 6);
            
            mwriter.putKeyString(kbuf);
            mwriter.putValueString(vbuf);
        }
        assertArray("MsgPackWrite0", "tc11", outBytes11, out);
    }
    // longer map testcase
    {
        Array<cxbyte> outBytes12(793894*14+5);
        outBytes12[0] = 0xdf;
        outBytes12[1] = 793894>>24;
        outBytes12[2] = (793894>>16)&0xff;
        outBytes12[3] = (793894>>8)&0xff;
        outBytes12[4] = 793894&0xff;
        std::vector<cxbyte> out;
        MsgPackMapWriter mwriter(793894, out);
        for (cxuint i = 0; i < 793894; i++)
        {
            char kbuf[10], vbuf[10];
            ::snprintf(kbuf, 10, "%06x", i);
            ::snprintf(vbuf, 10, "%06x", (i^0x11b3dca1U)&0xffffffU);
            outBytes12[5+i*14] = 0xa6;
            std::copy(kbuf, kbuf+6, ((char*)outBytes12.data())+5 + i*14 + 1);
            outBytes12[5+i*14 + 7] = 0xa6;
            std::copy(vbuf, vbuf+6, ((char*)outBytes12.data())+5 + i*14 + 8);
            
            mwriter.putKeyString(kbuf);
            mwriter.putValueString(vbuf);
        }
        assertArray("MsgPackWrite0", "tc12", outBytes12, out);
    }
}

struct ROCmMsgPackMDTestCase
{
    ROCmMetadata input;
    Array<ROCmKernelDescriptor> inputKDescs;
    size_t expectedSize;
    const cxbyte* expected;      // input metadata string
    bool good;
    const char* error;
};

static const cxbyte rocmMsgPackInput0[] =
{
    0x83,
    0xae, 'a', 'm', 'd', 'h', 's', 'a', '.', 'k', 'e', 'r', 'n', 'e', 'l', 's',
    0x91,
    // kernel A
    0xde, 0x00, 0x13,
    0xa5, '.', 'a', 'r', 'g', 's',
    // kernel args
    0xdc, 0x00, 0x16,
    0x88,   // 0
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa6, 'g', 'l', 'o', 'b', 'a', 'l',
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '0',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x00,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa7, 'i', 'n', 't', '8', '_', 't', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa2, 'i', '8',
    0x88,   // 1
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa8, 'c', 'o', 'n', 's', 't', 'a', 'n', 't',
        0xac, '.', 'i', 's', '_', 'v', 'o', 'l', 'a', 't', 'i', 'l', 'e', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '1',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x08,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa6, 'u', 'c', 'h', 'a', 'r', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa2, 'u', '8',
    0x88,   // 2
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa7, 'p', 'r', 'i', 'v', 'a', 't', 'e',
        0xac, '.', 'i', 's', '_', 'r', 'e', 's', 't', 'r', 'i', 'c', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '2',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x10,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa6, 'i', 'n', 't', '1', '6', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '1', '6',
    0x88,   // 3
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa5, 'l', 'o', 'c', 'a', 'l',
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '3',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x18,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa9, 'u', 'i', 'n', 't', '1', '6', '_', 't', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'u', '1', '6',
    0x89,   // 4
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa5, 'l', 'o', 'c', 'a', 'l',
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa4, 'i', 'n', '3', 'x',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x20,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xae, '.', 'p', 'o', 'i', 'n', 't', 'e', 'e', '_', 'a', 'l', 'i', 'g', 'n', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa5, 'h', 'a', 'l', 'f', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'f', '1', '6',
    0x88,   // 5
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa7, 'g', 'e', 'n', 'e', 'r', 'i', 'c',
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '4',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x28,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa6, 'i', 'n', 't', '3', '2', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '3', '2',
    0x88,   // 6
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa6, 'r', 'e', 'g', 'i', 'o', 'n',
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '5',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x30,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa7, 'u', 'i', 'n', 't', '3', '2', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'u', '3', '2',
    0x87,   // 7
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '6',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x38,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa6, 'f', 'l', 'o', 'a', 't', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xa5, 'q', 'u', 'e', 'u', 'e',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'f', '3', '2',
    0x88,   // 8
        0xa7, '.', 'a', 'c', 'c', 'e', 's', 's',
            0xa9, 'r', 'e', 'a', 'd', '_', 'o', 'n', 'l', 'y',
        0xa8, '.', 'i', 's', '_', 'p', 'i', 'p', 'e', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '7',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x40,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa5, 'l', 'o', 'n', 'g', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xa4, 'p', 'i', 'p', 'e',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x87,   // 9
        0xa7, '.', 'a', 'c', 'c', 'e', 's', 's',
            0xaa, 'r', 'e', 'a', 'd', '_', 'w', 'r', 'i', 't', 'e',
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '8',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x48,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa9, 'i', 'm', 'a', 'g', 'e', '2', 'd', '_', 't',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xa5, 'i', 'm', 'a', 'g', 'e',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'u', '6', '4',
    0x86,   // 10
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '9',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x50,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa9, 's', 'a', 'm', 'p', 'l', 'e', 'r', '_', 't',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xa7, 's', 'a', 'm', 'p', 'l', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'f', '6', '4',
    0x87,   // 11
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa6, 'g', 'l', 'o', 'b', 'a', 'l',
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa4, 'i', 'n', '1', '0',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x58,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa6, 's', 't', 'r', 'u', 'c', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb6, 'd', 'y', 'n', 'a', 'm', 'i', 'c', '_', 's', 'h', 'a', 'r', 'e', 'd', '_',
                'p', 'o', 'i', 'n', 't', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e',
            0xa6, 's', 't', 'r', 'u', 'c', 't',
    0x86,   // 12
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa1, 'n',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x60,
        0xa5, '.', 's', 'i', 'z', 'e', 0x04,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', 't',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xa8, 'b', 'y', '_', 'v', 'a', 'l', 'u', 'e',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '3', '2',
    0x84,   // 13
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x70,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb6, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'g', 'l', 'o', 'b', 'a', 'l', '_',
            'o', 'f', 'f', 's', 'e', 't', '_', 'x',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 14
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x78,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb6, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'g', 'l', 'o', 'b', 'a', 'l', '_',
            'o', 'f', 'f', 's', 'e', 't', '_', 'y',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 15
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0x80,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb6, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'g', 'l', 'o', 'b', 'a', 'l', '_',
            'o', 'f', 'f', 's', 'e', 't', '_', 'z',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 16
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0x88,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb4, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'p', 'r', 'i', 'n', 't', 'f', '_',
            'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 17
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0x90,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb4, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'd', 'e', 'f', 'a', 'u', 'l', 't',
            '_', 'q', 'u', 'e', 'u', 'e',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 18
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0x98,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb8, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'c', 'o', 'm', 'p', 'l', 'e', 't',
            'i', 'o', 'n', '_', 'a', 'c', 't', 'i', 'o', 'n',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 19
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0xa0,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb9, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'm', 'u', 'l', 't', 'i', 'g', 'r',
            'i', 'd', '_', 's', 'y', 'n', 'c', '_', 'a', 'r', 'g',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x88,   // 20
        0xae, '.', 'a', 'c', 't', 'u', 'a', 'l', '_', 'a', 'c', 'c', 'e', 's', 's',
            0xaa, 'w', 'r', 'i', 't', 'e', '_', 'o', 'n', 'l', 'y',
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa6, 'g', 'l', 'o', 'b', 'a', 'l',
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'x', '0',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0xa8,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa7, 'i', 'n', 't', '8', '_', 't', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa2, 'i', '8',
    0x88,   // 21
        0xae, '.', 'a', 'c', 't', 'u', 'a', 'l', '_', 'a', 'c', 'c', 'e', 's', 's',
            0xaa, 'r', 'e', 'a', 'd', '_', 'w', 'r', 'i', 't', 'e',
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa6, 'g', 'l', 'o', 'b', 'a', 'l',
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'x', '1',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0xb0,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa7, 'i', 'n', 't', '8', '_', 't', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa2, 'i', '8',
    0xb6, '.', 'd', 'e', 'v', 'i', 'c', 'e', '_', 'e', 'n', 'q', 'u', 'e', 'u', 'e', '_',
        's', 'y', 'm', 'b', 'o', 'l', 0xa4, 'a', 'b', 'c', 'd',
    0xb9, '.', 'g', 'r', 'o', 'u', 'p', '_', 's', 'e', 'g', 'm', 'e', 'n', 't', '_',
        'f', 'i', 'x', 'e', 'd', '_', 's', 'i', 'z', 'e', 0x48,
    0xb6, '.', 'k', 'e', 'r', 'n', 'a', 'r', 'g', '_', 's', 'e', 'g', 'm', 'e', 'n', 't',
        '_', 'a', 'l', 'i', 'g', 'n', 0x10,
    0xb5, '.', 'k', 'e', 'r', 'n', 'a', 'r', 'g', '_', 's', 'e', 'g', 'm', 'e', 'n', 't',
        '_', 's', 'i', 'z', 'e', 0xcc, 0xc0,
    0xa9, '.', 'l', 'a', 'n', 'g', 'u', 'a', 'g', 'e',
        0xa8, 'O', 'p', 'e', 'n', 'C', 'L', ' ', 'C',
    0xb1, '.', 'l', 'a', 'n', 'g', 'u', 'a', 'g', 'e', '_',
        'v', 'e', 'r', 's', 'i', 'o', 'n', 0x92, 0x03, 0x02,
    0xb8, '.', 'm', 'a', 'x', '_', 'f', 'l', 'a', 't', '_', 'w', 'o', 'r', 'k',
        'g', 'r', 'o', 'u', 'p', '_', 's', 'i', 'z', 'e', 0xcd, 0x01, 0x8c,
    0xa5, '.', 'n', 'a', 'm', 'e', 0xa8, 't', 'e', 's', 't', 'e', 'r', 'e', 'k',
    0xbb, '.', 'p', 'r', 'i', 'v', 'a', 't', 'e', '_', 's', 'e', 'g', 'm', 'e', 'n', 't',
        '_', 'f', 'i', 'x', 'e', 'd', '_', 's', 'i', 'z', 'e', 0x20,
    0xb4, '.', 'r', 'e', 'q', 'd', '_', 'w', 'o', 'r', 'k', 'g', 'r', 'o', 'u', 'p', '_',
            's', 'i', 'z', 'e', 0x93, 0x06, 0x09, 0x11,
    0xab, '.', 's', 'g', 'p', 'r', '_', 'c', 'o', 'u', 'n', 't', 0x2b,
    0xb1, '.', 's', 'g', 'p', 'r', '_', 's', 'p', 'i', 'l', 'l', '_',
            'c', 'o', 'u', 'n', 't', 0x05,
    0xa7, '.', 's', 'y', 'm', 'b', 'o', 'l',
        0xab, 't', 'e', 's', 't', 'e', 'r', 'e', 'k', '.', 'k', 'd',
    0xae, '.', 'v', 'e', 'c', '_', 't', 'y', 'p', 'e', '_', 'h', 'i', 'n', 't',
        0xa5, 'i', 'n', 't', '1', '6',
    0xab, '.', 'v', 'g', 'p', 'r', '_', 'c', 'o', 'u', 'n', 't', 0x09,
    0xb1, '.', 'v', 'g', 'p', 'r', '_', 's', 'p', 'i', 'l', 'l', '_',
            'c', 'o', 'u', 'n', 't', 0x08,
    0xaf, '.', 'w', 'a', 'v', 'e', 'f', 'r', 'o', 'n', 't', '_', 's', 'i', 'z', 'e', 0x40,
    0xb4, '.', 'w', 'o', 'r', 'k', 'g', 'r', 'o', 'u', 'p', '_', 's', 'i', 'z', 'e', '_',
            'h', 'i', 'n', 't', 0x93, 0x04, 0x0b, 0x05,
    // printf infos
    0xad, 'a', 'm', 'd', 'h', 's', 'a', '.', 'p', 'r', 'i', 'n', 't', 'f', 0x92,
        0xd9, 0x20, '2', ':', '4', ':', '4', ':', '4', ':', '4', ':', '4', ':','i', '=',
        '%', 'd', ',', 'a', '=', '%', 'f', ',', 'b', '=', '%', 'f', ',',
        'c', '=', '%', 'f', 0x0a,
        0xaf, '1', ':', '1', ':', '4', ':', 'i', 'n', 'd', 'e', 'x', 'a', '%', 'd', 0x0a,
    // version
    0xae, 'a', 'm', 'd', 'h', 's', 'a', '.', 'v', 'e', 'r', 's', 'i', 'o', 'n',
        0x92, 0x27, 0x34
};

static const cxbyte rocmMsgPackInput1[] =
{
    0x82,
    0xae, 'a', 'm', 'd', 'h', 's', 'a', '.', 'k', 'e', 'r', 'n', 'e', 'l', 's',
    0x91,
    // kernel A
    0xde, 0x00, 0x13,
    0xa5, '.', 'a', 'r', 'g', 's',
    // kernel args
    0x95,
    0x88,   // 0
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa6, 'g', 'l', 'o', 'b', 'a', 'l',
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa3, 'i', 'n', '0',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x00,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa6, 'f', 'l', 'o', 'a', 't', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'f', '3', '2',
    0x88,   // 1
        0xae, '.', 'a', 'd', 'd', 'r', 'e', 's', 's', '_', 's', 'p', 'a', 'c', 'e',
            0xa6, 'g', 'l', 'o', 'b', 'a', 'l',
        0xa9, '.', 'i', 's', '_', 'c', 'o', 'n', 's', 't', 0xc3,
        0xa5, '.', 'n', 'a', 'm', 'e', 0xa4, 'o', 'u', 't', '0',
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x00,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xaa, '.', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e',
            0xa6, 'f', 'l', 'o', 'a', 't', '*',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xad, 'g', 'l', 'o', 'b', 'a', 'l', '_', 'b', 'u', 'f', 'f', 'e', 'r',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'f', '3', '2',
    0x84,   // 2
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x70,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb6, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'g', 'l', 'o', 'b', 'a', 'l', '_',
            'o', 'f', 'f', 's', 'e', 't', '_', 'x',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 3
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0x78,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb6, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'g', 'l', 'o', 'b', 'a', 'l', '_',
            'o', 'f', 'f', 's', 'e', 't', '_', 'y',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0x84,   // 4
        0xa7, '.', 'o', 'f', 'f', 's', 'e', 't', 0xcc, 0x80,
        0xa5, '.', 's', 'i', 'z', 'e', 0x08,
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 'k', 'i', 'n', 'd',
            0xb6, 'h', 'i', 'd', 'd', 'e', 'n', '_', 'g', 'l', 'o', 'b', 'a', 'l', '_',
            'o', 'f', 'f', 's', 'e', 't', '_', 'z',
        0xab, '.', 'v', 'a', 'l', 'u', 'e', '_', 't', 'y', 'p', 'e', 0xa3, 'i', '6', '4',
    0xb6, '.', 'd', 'e', 'v', 'i', 'c', 'e', '_', 'e', 'n', 'q', 'u', 'e', 'u', 'e', '_',
        's', 'y', 'm', 'b', 'o', 'l', 0xa4, 'a', 'b', 'c', 'd',
    0xb9, '.', 'g', 'r', 'o', 'u', 'p', '_', 's', 'e', 'g', 'm', 'e', 'n', 't', '_',
        'f', 'i', 'x', 'e', 'd', '_', 's', 'i', 'z', 'e', 0xcc, 0xe7,
    0xb6, '.', 'k', 'e', 'r', 'n', 'a', 'r', 'g', '_', 's', 'e', 'g', 'm', 'e', 'n', 't',
        '_', 'a', 'l', 'i', 'g', 'n', 0x10,
    0xb5, '.', 'k', 'e', 'r', 'n', 'a', 'r', 'g', '_', 's', 'e', 'g', 'm', 'e', 'n', 't',
        '_', 's', 'i', 'z', 'e', 0xcc, 0xc0,
    0xa9, '.', 'l', 'a', 'n', 'g', 'u', 'a', 'g', 'e',
        0xa8, 'O', 'p', 'e', 'n', 'C', 'L', ' ', 'C',
    0xb1, '.', 'l', 'a', 'n', 'g', 'u', 'a', 'g', 'e', '_',
        'v', 'e', 'r', 's', 'i', 'o', 'n', 0x92, 0x03, 0x02,
    0xb8, '.', 'm', 'a', 'x', '_', 'f', 'l', 'a', 't', '_', 'w', 'o', 'r', 'k',
        'g', 'r', 'o', 'u', 'p', '_', 's', 'i', 'z', 'e', 0xcd, 0x01, 0x8c,
    0xa5, '.', 'n', 'a', 'm', 'e', 0xa7, 'v', 'e', 'c', 't', 'o', 'r', 's',
    0xbb, '.', 'p', 'r', 'i', 'v', 'a', 't', 'e', '_', 's', 'e', 'g', 'm', 'e', 'n', 't',
        '_', 'f', 'i', 'x', 'e', 'd', '_', 's', 'i', 'z', 'e', 0xcd, 0x15, 0x2e,
    0xb4, '.', 'r', 'e', 'q', 'd', '_', 'w', 'o', 'r', 'k', 'g', 'r', 'o', 'u', 'p', '_',
            's', 'i', 'z', 'e', 0x93, 0x06, 0x09, 0x11,
    0xab, '.', 's', 'g', 'p', 'r', '_', 'c', 'o', 'u', 'n', 't', 0x2b,
    0xb1, '.', 's', 'g', 'p', 'r', '_', 's', 'p', 'i', 'l', 'l', '_',
            'c', 'o', 'u', 'n', 't', 0x05,
    0xa7, '.', 's', 'y', 'm', 'b', 'o', 'l',
        0xaa, 'v', 'e', 'c', 't', 'o', 'r', 's', '.', 'k', 'd',
    0xae, '.', 'v', 'e', 'c', '_', 't', 'y', 'p', 'e', '_', 'h', 'i', 'n', 't',
        0xa5, 'i', 'n', 't', '1', '6',
    0xab, '.', 'v', 'g', 'p', 'r', '_', 'c', 'o', 'u', 'n', 't', 0x09,
    0xb1, '.', 'v', 'g', 'p', 'r', '_', 's', 'p', 'i', 'l', 'l', '_',
            'c', 'o', 'u', 'n', 't', 0x08,
    0xaf, '.', 'w', 'a', 'v', 'e', 'f', 'r', 'o', 'n', 't', '_', 's', 'i', 'z', 'e', 0x40,
    0xb4, '.', 'w', 'o', 'r', 'k', 'g', 'r', 'o', 'u', 'p', '_', 's', 'i', 'z', 'e', '_',
            'h', 'i', 'n', 't', 0x93, 0x04, 0x0b, 0x05,
    // version
    0xae, 'a', 'm', 'd', 'h', 's', 'a', '.', 'v', 'e', 'r', 's', 'i', 'o', 'n',
        0x92, 0x27, 0x34
};

static const ROCmMsgPackMDTestCase rocmMsgPackMDTestCases[] =
{
    {   // testcase 0
        {
            { 39, 52 },
            {   // printfInfos
                { 2, { 4, 4, 4, 4 }, "i=%d,a=%f,b=%f,c=%f\n" },
                { 1, { 4 }, "indexa%d\n" },
            },
            {   // kernels
                { // kernel 1
                    "testerek", "testerek.kd",
                    {   // args
                        { "in0", "int8_t*", 8, 0, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "in1", "uchar*", 8, 8, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::UINT8, ROCmAddressSpace::CONSTANT,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, true, false },
                        { "in2", "int16*", 8, 16, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::INT16, ROCmAddressSpace::PRIVATE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, true, false, false },
                        { "in3", "uint16_t*", 8, 24, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::UINT16, ROCmAddressSpace::LOCAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "in3x", "half*", 8, 32, 8, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::FLOAT16, ROCmAddressSpace::LOCAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "in4", "int32*", 8, 40, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::INT32, ROCmAddressSpace::GENERIC,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "in5", "uint32*", 8, 48, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::UINT32, ROCmAddressSpace::REGION,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "in6", "float*", 8, 56, 0, ROCmValueKind::QUEUE,
                            ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "in7", "long*", 8, 64, 0, ROCmValueKind::PIPE,
                            ROCmValueType::INT64, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::READ_ONLY, ROCmAccessQual::DEFAULT,
                            false, false, false, true },
                        { "in8", "image2d_t", 8, 72, 0, ROCmValueKind::IMAGE,
                            ROCmValueType::UINT64, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::READ_WRITE, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "in9", "sampler_t", 8, 80, 0, ROCmValueKind::SAMPLER,
                            ROCmValueType::FLOAT64, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "in10", "struc*", 8, 88, 0, ROCmValueKind::DYN_SHARED_PTR,
                            ROCmValueType::STRUCTURE, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "n", "int", 4, 96, 0, ROCmValueKind::BY_VALUE,
                            ROCmValueType::INT32, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 112, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 120, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Y,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 128, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Z,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 136, 0, ROCmValueKind::HIDDEN_PRINTF_BUFFER,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 144, 0, ROCmValueKind::HIDDEN_DEFAULT_QUEUE,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 152, 0, ROCmValueKind::HIDDEN_COMPLETION_ACTION,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 160, 0, ROCmValueKind::HIDDEN_MULTIGRID_SYNC_ARG,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "ix0", "int8_t*", 8, 168, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::READ_ONLY, ROCmAccessQual::WRITE_ONLY,
                            false, false, false, false },
                        { "ix1", "int8_t*", 8, 176, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::WRITE_ONLY, ROCmAccessQual::READ_WRITE,
                            false, false, false, false }
                    },
                    "OpenCL C", { 3, 2 }, { 6, 9, 17 }, { 4, 11, 5 }, "int16", "",
                    192, 72, 32, 16, 64, 43, 9, 396, { 0, 0, 0 }, 5, 8, "abcd"
                }
            }
        },
        {
            { BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED, 0, 0, 0, { }, 0, 0, 0, 0, { } }
        },
        sizeof(rocmMsgPackInput0), rocmMsgPackInput0,
        true, ""
    },
    {   // testcase 1
        {
            { 39, 52 },
            { },
            {   // kernels
                { // kernel 1
                    "vectors", "vectors.kd",
                    {   // args
                        { "in0", "float*", 8, 0, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "out0", "float*", 8, 0, 0, ROCmValueKind::GLOBAL_BUFFER,
                            ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            true, false, false, false },
                        { "", "", 8, 112, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 120, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Y,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                        { "", "", 8, 128, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Z,
                            ROCmValueType::INT64, ROCmAddressSpace::NONE,
                            ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                            false, false, false, false },
                    },
                    "OpenCL C", { 3, 2 }, { 6, 9, 17 }, { 4, 11, 5 }, "int16", "",
                    192, BINGEN64_DEFAULT, BINGEN64_DEFAULT,
                    16, 64, 43, 9, 396, { 0, 0, 0 }, 5, 8, "abcd"
                }
            }
        },
        {
            { 231, 5422, 0, 0, 0, { }, 0, 0, 0, 0, { } }
        },
        sizeof(rocmMsgPackInput1), rocmMsgPackInput1,
        true, ""
    }
};

static void testParseROCmMsgPackMDCase(cxuint testId, const ROCmMsgPackMDTestCase& testCase)
{
    std::vector<const ROCmKernelDescriptor*> kdescs(testCase.inputKDescs.size());
    for (size_t i = 0; i < testCase.inputKDescs.size(); i++)
        kdescs[i] = testCase.inputKDescs.data()+i;
    
    bool good = true;
    std::string error;
    std::vector<cxbyte> result;
    try
    { generateROCmMetadataMsgPack(testCase.input, kdescs.data(), result); }
    catch(const std::exception& ex)
    {
        good = false;
        error = ex.what();
    }
    
    char testName[30];
    snprintf(testName, 30, "Test #%u", testId);
    assertValue(testName, "good", testCase.good, good);
    assertString(testName, "error", testCase.error, error.c_str());
    if (!good)
        // do not check if test failed
        return;
    
    assertArray(testName, "result", Array<cxbyte>(testCase.expected,
                        testCase.expected + testCase.expectedSize), result);
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testMsgPackBytesWrite);
    for (cxuint i = 0; i < sizeof(rocmMsgPackMDTestCases)/
                            sizeof(ROCmMsgPackMDTestCase); i++)
        try
        { testParseROCmMsgPackMDCase(i, rocmMsgPackMDTestCases[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
