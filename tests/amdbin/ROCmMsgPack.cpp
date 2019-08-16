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
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include "../TestUtils.h"

using namespace CLRX;

static void testMsgPackBytes()
{
    const cxbyte tc0[2] = { 0x91, 0xc0 };
    const cxbyte* dataPtr;
    // parseNil
    dataPtr = tc0;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc0));
        arrParser.parseNil();
        assertValue("MsgPack0", "tc0: DataPtr", dataPtr, tc0 + sizeof(tc0));
    }
    dataPtr = tc0;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc0)-1);
        assertCLRXException("MsgPack0", "tc0_1.Ex", "MsgPack: Can't parse nil value",
                    [&arrParser]() { arrParser.parseNil(); });
        assertValue("MsgPack0", "tc0_1.DataPtr", dataPtr, tc0 + sizeof(tc0)-1);
    }
    
    // parseBool
    const cxbyte tc1[2] = { 0x91, 0xc2 };
    dataPtr = tc1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc1));
        assertTrue("MsgPack0", "tc1.value0", !arrParser.parseBool());
        assertValue("MsgPack0", "tc1.DataPtr", dataPtr, tc1 + sizeof(tc1));
    }
    dataPtr = tc1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc1)-1);
        assertCLRXException("MsgPack0", "tc1_1.Ex", "MsgPack: Can't parse bool value",
                    [&arrParser]() { arrParser.parseBool(); });
        assertValue("MsgPack0", "tc1_1.DataPtr", dataPtr, tc1 + sizeof(tc1)-1);
    }
    
    const cxbyte tc2[2] = { 0x91, 0xc3 };
    dataPtr = tc2;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc2));
        assertTrue("MsgPack0", "tc2.value0", arrParser.parseBool());
        assertValue("MsgPack0", "tc2.DataPtr", dataPtr, tc2 + sizeof(tc2));
    }
    
    dataPtr = tc0;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc0));
        assertCLRXException("MsgPack0", "tc3.Ex", "MsgPack: Can't parse bool value",
                    [&arrParser]() { arrParser.parseBool(); });
        assertValue("MsgPack0", "tc3.DataPtr", dataPtr, tc0 + sizeof(tc0)-1);
    }
    
    // parseInteger
    const cxbyte tc4[2] = { 0x91, 0x21 };
    dataPtr = tc4;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc4));
        assertValue("MsgPack0", "tc4.value0", uint64_t(33),
                   arrParser.parseInteger(MSGPACK_WS_UNSIGNED));
        assertValue("MsgPack0", "tc4.DataPtr", dataPtr, tc4 + sizeof(tc4));
    }
    dataPtr = tc4;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc4)-1);
        assertCLRXException("MsgPack0", "tc4_1.Ex", "MsgPack: Can't parse integer value",
                    [&arrParser]() { arrParser.parseInteger(MSGPACK_WS_UNSIGNED); });
        assertValue("MsgPack0", "tc4_1.DataPtr", dataPtr, tc4 + sizeof(tc4)-1);
    }
    const cxbyte tc5[2] = { 0x91, 0x5b };
    dataPtr = tc5;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc5));
        assertValue("MsgPack0", "tc5.value0", uint64_t(91),
                   arrParser.parseInteger(MSGPACK_WS_UNSIGNED));
        assertValue("MsgPack0", "tc5.DataPtr", dataPtr, tc5 + sizeof(tc5));
    }
    const cxbyte tc6[2] = { 0x91, 0xe9 };
    dataPtr = tc6;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc6));
        assertValue("MsgPack0", "tc6.value0", uint64_t(-23),
                   arrParser.parseInteger(MSGPACK_WS_SIGNED));
        assertValue("MsgPack0", "tc6.DataPtr", dataPtr, tc6 + sizeof(tc6));
    }
    const cxbyte tc6_1[2] = { 0x91, 0xe0 };
    dataPtr = tc6_1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc6_1));
        assertValue("MsgPack0", "tc6_1.value0", uint64_t(-32),
                   arrParser.parseInteger(MSGPACK_WS_SIGNED));
        assertValue("MsgPack0", "tc6_1.DataPtr", dataPtr, tc6_1 + sizeof(tc6_1));
    }
    // longer integers
    const cxbyte tc7[3] = { 0x91, 0xcc, 0x4d };
    dataPtr = tc7;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc7));
        assertValue("MsgPack0", "tc7.value0", uint64_t(77),
                   arrParser.parseInteger(MSGPACK_WS_UNSIGNED));
        assertValue("MsgPack0", "tc7.DataPtr", dataPtr, tc7 + sizeof(tc7));
    }
    const cxbyte tc7_1[3] = { 0x91, 0xcc, 0xba };
    dataPtr = tc7_1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc7_1));
        assertValue("MsgPack0", "tc7_1.value0", uint64_t(0xba),
                   arrParser.parseInteger(MSGPACK_WS_UNSIGNED));
        assertValue("MsgPack0", "tc7_1.DataPtr", dataPtr, tc7_1 + sizeof(tc7_1));
    }
    const cxbyte tc7_2[2] = { 0x91, 0xcc };
    dataPtr = tc7_2;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc7_2));
        assertCLRXException("MsgPack0", "tc7_2.Ex", "MsgPack: Can't parse integer value",
                    [&arrParser]() { arrParser.parseInteger(MSGPACK_WS_UNSIGNED); });
        assertValue("MsgPack0", "tc7_2.DataPtr", dataPtr, tc7_2 + sizeof(tc7_2));
    }
    const cxbyte tc8[3] = { 0x91, 0xd0, 0x4d };
    dataPtr = tc8;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc8));
        assertValue("MsgPack0", "tc8.value0", uint64_t(77),
                   arrParser.parseInteger(MSGPACK_WS_SIGNED));
        assertValue("MsgPack0", "tc8.DataPtr", dataPtr, tc8 + sizeof(tc8));
    }
    const cxbyte tc8_1[3] = { 0x91, 0xd0, 0xba };
    dataPtr = tc8_1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc8_1));
        assertValue("MsgPack0", "tc8_1.value0", uint64_t(-70),
                   arrParser.parseInteger(MSGPACK_WS_SIGNED));
        assertValue("MsgPack0", "tc8_1.DataPtr", dataPtr, tc8_1 + sizeof(tc8_1));
    }
    dataPtr = tc8_1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc8_1));
        assertCLRXException("MsgPack0", "tc8_2.Ex",
                    "MsgPack: Negative value for unsigned integer",
                    [&arrParser]() { arrParser.parseInteger(MSGPACK_WS_UNSIGNED); });
        assertValue("MsgPack0", "tc8_2.DataPtr", dataPtr, tc8_1 + sizeof(tc8_1));
    }
    // 16-bit integers
    const cxbyte tc9[4] = { 0x91, 0xcd, 0x37, 0x1c };
    dataPtr = tc9;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc9));
        assertValue("MsgPack0", "tc9.value0", uint64_t(0x1c37),
                   arrParser.parseInteger(MSGPACK_WS_UNSIGNED));
        assertValue("MsgPack0", "tc9.DataPtr", dataPtr, tc9 + sizeof(tc9));
    }
    const cxbyte tc9_1[4] = { 0x91, 0xcd, 0x7e, 0xb3 };
    dataPtr = tc9_1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc9_1));
        assertValue("MsgPack0", "tc9_1.value0", uint64_t(0xb37e),
                   arrParser.parseInteger(MSGPACK_WS_UNSIGNED));
        assertValue("MsgPack0", "tc9_1.DataPtr", dataPtr, tc9_1 + sizeof(tc9_1));
    }
    const cxbyte tc9_2[3] = { 0x91, 0xcd, 0x7e };
    dataPtr = tc9_2;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc9_2));
        assertCLRXException("MsgPack0", "tc9_2.Ex", "MsgPack: Can't parse integer value",
                    [&arrParser]() { arrParser.parseInteger(MSGPACK_WS_UNSIGNED); });
        assertValue("MsgPack0", "tc9_2.DataPtr", dataPtr, tc9_2 + sizeof(tc9_2)-1);
    }
    const cxbyte tc9_3[2] = { 0x91, 0xcd };
    dataPtr = tc9_3;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc9_3));
        assertCLRXException("MsgPack0", "tc9_3.Ex", "MsgPack: Can't parse integer value",
                    [&arrParser]() { arrParser.parseInteger(MSGPACK_WS_UNSIGNED); });
        assertValue("MsgPack0", "tc9_3.DataPtr", dataPtr, tc9_3 + sizeof(tc9_3));
    }
    const cxbyte tc10[4] = { 0x91, 0xd1, 0x37, 0x1c };
    dataPtr = tc10;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc10));
        assertValue("MsgPack0", "tc10.value0", uint64_t(0x1c37),
                   arrParser.parseInteger(MSGPACK_WS_SIGNED));
        assertValue("MsgPack0", "tc10.DataPtr", dataPtr, tc10 + sizeof(tc10));
    }
    const cxbyte tc10_1[4] = { 0x91, 0xd1, 0x7e, 0xb3 };
    dataPtr = tc10_1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc10_1));
        assertValue("MsgPack0", "tc10_1.value0", uint64_t(int16_t(0xb37e)),
                   arrParser.parseInteger(MSGPACK_WS_SIGNED));
        assertValue("MsgPack0", "tc10_1.DataPtr", dataPtr, tc10_1 + sizeof(tc10_1));
    }
    dataPtr = tc10_1;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc10_1));
        assertCLRXException("MsgPack0", "tc10_2.Ex",
                    "MsgPack: Negative value for unsigned integer",
                    [&arrParser]() { arrParser.parseInteger(MSGPACK_WS_UNSIGNED); });
        assertValue("MsgPack0", "tc10_2.DataPtr", dataPtr, tc10_1 + sizeof(tc10_1));
    }
    const cxbyte tc10_3[3] = { 0x91, 0xcd, 0x7e };
    dataPtr = tc10_3;
    {
        MsgPackArrayParser arrParser(dataPtr, dataPtr + sizeof(tc10_3));
        assertCLRXException("MsgPack0", "tc10_3.Ex", "MsgPack: Can't parse integer value",
                    [&arrParser]() { arrParser.parseInteger(MSGPACK_WS_SIGNED); });
        assertValue("MsgPack0", "tc10_3.DataPtr", dataPtr, tc10_3+ sizeof(tc10_3)-1);
    }
    // 32-bit integers
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testMsgPackBytes);
    return retVal;
}
