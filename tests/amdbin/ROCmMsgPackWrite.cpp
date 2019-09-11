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
        Array<cxbyte> outBytes3(1777777+5);
        outBytes3[0] = 0xdd;
        outBytes3[1] = 1777777>>24;
        outBytes3[2] = (1777777>>16)&0xff;
        outBytes3[3] = (1777777>>8)&0xff;
        outBytes3[4] = 1777777&0xff;
        std::vector<cxbyte> out;
        MsgPackArrayWriter awriter(1777777, out);
        for (cxuint i = 0; i < 1777777; i++)
        {
            awriter.putUInt(i&0x7f);
            outBytes3[i+5] = i&0x7f;
        }
        assertArray("MsgPackWrite0", "tc4", outBytes3, out);
    }
    /* test putting values */
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testMsgPackBytesWrite);
    return retVal;
}
