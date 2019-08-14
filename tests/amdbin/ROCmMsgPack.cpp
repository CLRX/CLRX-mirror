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
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testMsgPackBytes);
    return retVal;
}

