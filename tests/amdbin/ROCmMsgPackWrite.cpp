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

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testMsgPackBytesWrite);
    return retVal;
}
