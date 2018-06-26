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
#include <cstring>
#include <cstdio>
#include <utility>
#include <initializer_list>
#include <CLRX/utils/GPUId.h>
#include "../TestUtils.h"

using namespace CLRX;

// table with accepted GPU device names (with mixed lower/upper case)
static const std::pair<const char*, GPUDeviceType>
gpuDeviceEntryTable[] =
{
    { "baffin", GPUDeviceType::BAFFIN },
    { "bonaire", GPUDeviceType::BONAIRE },
    { "capeverde", GPUDeviceType::CAPE_VERDE },
    { "carrizo", GPUDeviceType::CARRIZO },
    { "dummy", GPUDeviceType::DUMMY },
    { "ellesmere", GPUDeviceType::ELLESMERE },
    { "fiji", GPUDeviceType::FIJI },
    { "gfx700", GPUDeviceType::SPECTRE },
    { "gfx701", GPUDeviceType::HAWAII },
    { "gfx801", GPUDeviceType::CARRIZO },
    { "gfx802", GPUDeviceType::TONGA },
    { "gfx803", GPUDeviceType::FIJI },
    { "gfx804", GPUDeviceType::GFX804 },
    { "gfx900", GPUDeviceType::GFX900 },
    { "gfx901", GPUDeviceType::GFX901 },
    { "gfx902", GPUDeviceType::GFX902 },
    { "gfx903", GPUDeviceType::GFX903 },
    { "gfx904", GPUDeviceType::GFX904 },
    { "gfx905", GPUDeviceType::GFX905 },
    { "gfx906", GPUDeviceType::GFX906 },
    { "gfx907", GPUDeviceType::GFX907 },
    { "goose", GPUDeviceType::GOOSE },
    { "hainan", GPUDeviceType::HAINAN },
    { "hawaii", GPUDeviceType::HAWAII },
    { "horse", GPUDeviceType::HORSE },
    { "iceland", GPUDeviceType::ICELAND },
    { "kalindi", GPUDeviceType::KALINDI },
    { "mullins", GPUDeviceType::MULLINS },
    { "oland", GPUDeviceType::OLAND },
    { "pitcairn", GPUDeviceType::PITCAIRN },
    { "polaris10", GPUDeviceType::ELLESMERE },
    { "polaris11", GPUDeviceType::BAFFIN },
    { "polaris12", GPUDeviceType::GFX804 },
    { "polaris20", GPUDeviceType::ELLESMERE },
    { "polaris21", GPUDeviceType::BAFFIN },
    { "polaris22", GPUDeviceType::GFX804 },
    { "raven", GPUDeviceType::GFX903 },
    { "spectre", GPUDeviceType::SPECTRE },
    { "spooky", GPUDeviceType::SPOOKY },
    { "stoney", GPUDeviceType::STONEY },
    { "tahiti", GPUDeviceType::TAHITI },
    { "tonga", GPUDeviceType::TONGA },
    { "topaz", GPUDeviceType::ICELAND },
    { "vega10", GPUDeviceType::GFX900 },
    { "vega11", GPUDeviceType::GFX902 },
    { "vega12", GPUDeviceType::GFX904 },
    { "vega20", GPUDeviceType::GFX906 },
    
    { "bAffin", GPUDeviceType::BAFFIN },
    { "boNAIre", GPUDeviceType::BONAIRE },
    { "cAPEverde", GPUDeviceType::CAPE_VERDE },
    { "carRIZo", GPUDeviceType::CARRIZO },
    { "duMMy", GPUDeviceType::DUMMY },
    { "elLESmere", GPUDeviceType::ELLESMERE },
    { "fiJI", GPUDeviceType::FIJI },
    { "gfX700", GPUDeviceType::SPECTRE },
    { "gFx701", GPUDeviceType::HAWAII },
    { "Gfx801", GPUDeviceType::CARRIZO },
    { "gFx802", GPUDeviceType::TONGA },
    { "gfX803", GPUDeviceType::FIJI },
    { "gFX804", GPUDeviceType::GFX804 },
    { "gfX900", GPUDeviceType::GFX900 },
    { "GFx901", GPUDeviceType::GFX901 },
    { "gFx902", GPUDeviceType::GFX902 },
    { "gfX903", GPUDeviceType::GFX903 },
    { "gFX904", GPUDeviceType::GFX904 },
    { "Gfx905", GPUDeviceType::GFX905 },
    { "Gfx906", GPUDeviceType::GFX906 },
    { "Gfx907", GPUDeviceType::GFX907 },
    { "goOSe", GPUDeviceType::GOOSE },
    { "hAINAn", GPUDeviceType::HAINAN },
    { "hAWaII", GPUDeviceType::HAWAII },
    { "hoRSe", GPUDeviceType::HORSE },
    { "iCELaNd", GPUDeviceType::ICELAND },
    { "KaliNDi", GPUDeviceType::KALINDI },
    { "mULlINs", GPUDeviceType::MULLINS },
    { "oLAnd", GPUDeviceType::OLAND },
    { "piTCairn", GPUDeviceType::PITCAIRN },
    { "polARis10", GPUDeviceType::ELLESMERE },
    { "pOLaris11", GPUDeviceType::BAFFIN },
    { "poLARis12", GPUDeviceType::GFX804 },
    { "polaRIS20", GPUDeviceType::ELLESMERE },
    { "pOLAris21", GPUDeviceType::BAFFIN },
    { "poLARis22", GPUDeviceType::GFX804 },
    { "raVEn", GPUDeviceType::GFX903 },
    { "spECTre", GPUDeviceType::SPECTRE },
    { "sPOoKy", GPUDeviceType::SPOOKY },
    { "stONeY", GPUDeviceType::STONEY },
    { "TAhITI", GPUDeviceType::TAHITI },
    { "toNGa", GPUDeviceType::TONGA },
    { "tOPAz", GPUDeviceType::ICELAND },
    { "VEga10", GPUDeviceType::GFX900 },
    { "veGA11", GPUDeviceType::GFX902 },
    { "VeGa12", GPUDeviceType::GFX904 },
    { "VeGa20", GPUDeviceType::GFX906 }
};

static const size_t gpuDeviceEntryTableSize =
    sizeof(gpuDeviceEntryTable) / sizeof(std::pair<const char*, GPUDeviceType>);

static void testGetGPUDeviceFromName()
{
    char descBuf[60];
    // checking device type names
    for (cxuint i = 0; i < gpuDeviceEntryTableSize; i++)
    {
        snprintf(descBuf, sizeof descBuf, "Test %d %s", i,
                        gpuDeviceEntryTable[i].first);
        const GPUDeviceType result = getGPUDeviceTypeFromName(
                        gpuDeviceEntryTable[i].first);
        assertValue("testGetGPUDeviceFromName", descBuf,
                    cxuint(gpuDeviceEntryTable[i].second), cxuint(result));
    }
    // checking exceptions
    assertCLRXException("testGetGPUDeviceFromName", "TestExc fxdxcd",
            "Unknown GPU device type", getGPUDeviceTypeFromName, "fdxdS");
    assertCLRXException("testGetGPUDeviceFromName", "TestExc fxdxcd2",
            "Unknown GPU device type", getGPUDeviceTypeFromName, "Polrdas");
    assertCLRXException("testGetGPUDeviceFromName", "TestExc empty",
            "Unknown GPU device type", getGPUDeviceTypeFromName, "");
}

// table with accepted GPU device names (with mixed lower/upper case)
static const std::pair<const char*, GPUArchitecture>
gpuArchitectureEntryTable[] =
{
    { "gcn1.0", GPUArchitecture::GCN1_0 },
    { "gcn1.1", GPUArchitecture::GCN1_1 },
    { "gcn1.2", GPUArchitecture::GCN1_2 },
    { "gcn1.4", GPUArchitecture::GCN1_4 },
    { "gcn1.4.1", GPUArchitecture::GCN1_4_1 },
    { "gfx6", GPUArchitecture::GCN1_0 },
    { "gfx7", GPUArchitecture::GCN1_1 },
    { "gfx8", GPUArchitecture::GCN1_2 },
    { "gfx9", GPUArchitecture::GCN1_4 },
    { "gfx906", GPUArchitecture::GCN1_4_1 },
    { "si", GPUArchitecture::GCN1_0 },
    { "ci", GPUArchitecture::GCN1_1 },
    { "vi", GPUArchitecture::GCN1_2 },
    { "vega", GPUArchitecture::GCN1_4 },
    { "vega20", GPUArchitecture::GCN1_4_1 },
    { "gCn1.0", GPUArchitecture::GCN1_0 },
    { "Gcn1.1", GPUArchitecture::GCN1_1 },
    { "gCn1.2", GPUArchitecture::GCN1_2 },
    { "GCn1.4", GPUArchitecture::GCN1_4 },
    { "Gfx6", GPUArchitecture::GCN1_0 },
    { "gFX7", GPUArchitecture::GCN1_1 },
    { "GfX8", GPUArchitecture::GCN1_2 },
    { "gFX9", GPUArchitecture::GCN1_4 },
    { "gFX906", GPUArchitecture::GCN1_4_1 },
    { "Si", GPUArchitecture::GCN1_0 },
    { "CI", GPUArchitecture::GCN1_1 },
    { "vI", GPUArchitecture::GCN1_2 },
    { "vEGa", GPUArchitecture::GCN1_4 },
    { "vEGa20", GPUArchitecture::GCN1_4_1 }
};

static const size_t gpuArchitectureEntryTableSize =
    sizeof(gpuArchitectureEntryTable) / sizeof(std::pair<const char*, GPUArchitecture>);

static void testGetGPUArchitectureFromName()
{
    char descBuf[60];
    // checking architecture names
    for (cxuint i = 0; i < gpuArchitectureEntryTableSize; i++)
    {
        snprintf(descBuf, sizeof descBuf, "Test %d %s", i,
                        gpuArchitectureEntryTable[i].first);
        const GPUArchitecture result = getGPUArchitectureFromName(
                        gpuArchitectureEntryTable[i].first);
        assertValue("testGetGPUArchitectureFromName", descBuf,
                    cxuint(gpuArchitectureEntryTable[i].second), cxuint(result));
    }
    // checking exceptions
    assertCLRXException("testGetGPUArchitectureFromName", "TestExc fxdxcd",
            "Unknown GPU architecture", getGPUArchitectureFromName, "fdxdS");
    assertCLRXException("testGetGPUArchitectureFromName", "TestExc fxdxcd2",
            "Unknown GPU architecture", getGPUArchitectureFromName, "Polrdas");
    assertCLRXException("testGetGPUArchitectureFromName", "TestExc empty",
            "Unknown GPU architecture", getGPUArchitectureFromName, "");
}

struct GPUMaxRegTestCase
{
    GPUArchitecture arch;
    cxuint regType;
    Flags flags;
    cxuint regsNum;
};

// getGPUMaxRegistersNum testcase table
static const GPUMaxRegTestCase gpuMaxRegTestTable[] =
{
    { GPUArchitecture::GCN1_0, REGTYPE_VGPR, 0, 256 },
    { GPUArchitecture::GCN1_1, REGTYPE_VGPR, 0, 256 },
    { GPUArchitecture::GCN1_2, REGTYPE_VGPR, 0, 256 },
    { GPUArchitecture::GCN1_4, REGTYPE_VGPR, 0, 256 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_VGPR, 0, 256 },
    { GPUArchitecture::GCN1_0, REGTYPE_SGPR, 0, 104 },
    { GPUArchitecture::GCN1_1, REGTYPE_SGPR, 0, 104 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, 0, 102 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, 0, 102 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, 0, 102 },
    { GPUArchitecture::GCN1_0, REGTYPE_SGPR, REGCOUNT_NO_VCC, 102 },
    { GPUArchitecture::GCN1_1, REGTYPE_SGPR, REGCOUNT_NO_VCC, 102 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, REGCOUNT_NO_VCC, 100 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, REGCOUNT_NO_VCC, 100 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, REGCOUNT_NO_VCC, 100 },
    { GPUArchitecture::GCN1_1, REGTYPE_SGPR, REGCOUNT_NO_FLAT, 100 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, REGCOUNT_NO_FLAT, 96 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, REGCOUNT_NO_FLAT, 96 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, REGCOUNT_NO_FLAT, 96 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, REGCOUNT_NO_XNACK, 98 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, REGCOUNT_NO_XNACK, 98 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, REGCOUNT_NO_XNACK, 98 }
};

static void testGetGPUMaxRegistersNum()
{
    char descBuf[60];
    // checking architecture names
    for (cxuint i = 0; i < sizeof gpuMaxRegTestTable/ sizeof(GPUMaxRegTestCase); i++)
    {
        const GPUMaxRegTestCase testCase = gpuMaxRegTestTable[i];
        snprintf(descBuf, sizeof descBuf, "Test %d", i);
        const cxuint result = getGPUMaxRegistersNum(testCase.arch, testCase.regType,
                                testCase.flags);
        assertValue("testGetGPUMaxRegistersNum", descBuf,
                    testCase.regsNum, result);
    }
}

// getGPUExtraRegsNum testcase table
static const GPUMaxRegTestCase gpuExtraRegsTestTable[] =
{
    { GPUArchitecture::GCN1_0, REGTYPE_VGPR, 0, 0 },
    { GPUArchitecture::GCN1_1, REGTYPE_VGPR, 0, 0 },
    { GPUArchitecture::GCN1_2, REGTYPE_VGPR, 0, 0 },
    { GPUArchitecture::GCN1_4, REGTYPE_VGPR, 0, 0 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_VGPR, 0, 0 },
    { GPUArchitecture::GCN1_0, REGTYPE_SGPR, 0, 0 },
    { GPUArchitecture::GCN1_1, REGTYPE_SGPR, 0, 0 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, 0, 0 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, 0, 0 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, 0, 0 },
    { GPUArchitecture::GCN1_0, REGTYPE_SGPR, GCN_VCC, 2 },
    { GPUArchitecture::GCN1_1, REGTYPE_SGPR, GCN_VCC, 2 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, GCN_VCC, 2 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, GCN_VCC, 2 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, GCN_VCC, 2 },
    { GPUArchitecture::GCN1_1, REGTYPE_SGPR, GCN_FLAT, 4 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, GCN_FLAT, 6 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, GCN_FLAT, 6 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, GCN_FLAT, 6 },
    { GPUArchitecture::GCN1_2, REGTYPE_SGPR, GCN_XNACK, 4 },
    { GPUArchitecture::GCN1_4, REGTYPE_SGPR, GCN_XNACK, 4 },
    { GPUArchitecture::GCN1_4_1, REGTYPE_SGPR, GCN_XNACK, 4 }
};

static void testGetGPUExtraRegsNum()
{
    char descBuf[60];
    // checking architecture names
    for (cxuint i = 0; i < sizeof gpuExtraRegsTestTable/ sizeof(GPUMaxRegTestCase); i++)
    {
        const GPUMaxRegTestCase testCase = gpuExtraRegsTestTable[i];
        snprintf(descBuf, sizeof descBuf, "Test %d", i);
        const cxuint result = getGPUExtraRegsNum(testCase.arch, testCase.regType,
                                testCase.flags);
        assertValue("testGetGPUExtraRegsNum", descBuf,
                    testCase.regsNum, result);
    }
}


int main(int argc, const char** argv)
{
    int retVal = 0;
    retVal |= callTest(testGetGPUDeviceFromName);
    retVal |= callTest(testGetGPUArchitectureFromName);
    retVal |= callTest(testGetGPUMaxRegistersNum);
    retVal |= callTest(testGetGPUExtraRegsNum);
    return retVal;
}
