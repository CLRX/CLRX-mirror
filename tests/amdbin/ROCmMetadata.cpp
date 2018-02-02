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
#include <memory>
#include <CLRX/utils/Containers.h>
#include <CLRX/amdbin/ROCmBinaries.h>
#include "../TestUtils.h"

using namespace CLRX;

struct ROCmMetadataTestCase
{
    const char* input;      // input metadata string
    ROCmMetadata expected;
    bool good;
    const char* error;
};

static const ROCmMetadataTestCase rocmMetadataTestCases[] =
{
    {   // test 0
        R"ffDXD(---
Version:         [ 1, 0 ]
Printf:          
  - '1:1:4:index\72%d\n'
  - '2:4:4:4:4:4:i=%d,a=%f,b=%f,c=%f\n'
Kernels:         
  - Name:            vectorAdd
    SymbolName:      'vectorAdd@kd'
    Language:        OpenCL C
    LanguageVersion: [ 1, 2 ]
    Args:            
      - Name:            n
        TypeName:        uint
        Size:            4
        Align:           4
        ValueKind:       ByValue
        ValueType:       U32
        AccQual:         Default
      - Name:            a
        TypeName:        'float*'
        Size:            8
        Align:           8
        ValueKind:       GlobalBuffer
        ValueType:       F32
        AddrSpaceQual:   Global
        AccQual:         Default
        IsConst:         true
      - Name:            b
        TypeName:        'float*'
        Size:            8
        Align:           8
        ValueKind:       GlobalBuffer
        ValueType:       F32
        AddrSpaceQual:   Global
        AccQual:         Default
        IsConst:         true
      - Name:            c
        TypeName:        'float*'
        Size:            8
        Align:           8
        ValueKind:       GlobalBuffer
        ValueType:       F32
        AddrSpaceQual:   Global
        AccQual:         Default
      - Size:            8
        Align:           8
        ValueKind:       HiddenGlobalOffsetX
        ValueType:       I64
      - Size:            8
        Align:           8
        ValueKind:       HiddenGlobalOffsetY
        ValueType:       I64
      - Size:            8
        Align:           8
        ValueKind:       HiddenGlobalOffsetZ
        ValueType:       I64
      - Size:            8
        Align:           8
        ValueKind:       HiddenPrintfBuffer
        ValueType:       I8
        AddrSpaceQual:   Global
    CodeProps:       
      KernargSegmentSize: 64
      GroupSegmentFixedSize: 0
      PrivateSegmentFixedSize: 0
      KernargSegmentAlign: 8
      WavefrontSize:   64
      NumSGPRs:        14
      NumVGPRs:        11
      MaxFlatWorkGroupSize: 256
...
)ffDXD",
        {
            { 1, 0 }, // version
            {    // printfInfos
                { 1, { 4 }, "index:%d\n" },
                { 2, { 4, 4, 4, 4 }, "i=%d,a=%f,b=%f,c=%f\n" }
            },
            {
                {   // kernel 0
                    "vectorAdd", "vectorAdd@kd",
                    {   // arguments
                        { "n", "uint", 4, 4, 0, ROCmValueKind::BY_VALUE,
                          ROCmValueType::UINT32, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "a", "float*", 8, 8, 0, ROCmValueKind::GLOBAL_BUFFER,
                          ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          true, false, false, false },
                        { "b", "float*", 8, 8, 0, ROCmValueKind::GLOBAL_BUFFER,
                          ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          true, false, false, false },
                        { "c", "float*", 8, 8, 0, ROCmValueKind::GLOBAL_BUFFER,
                          ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X,
                          ROCmValueType::INT64, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Y,
                          ROCmValueType::INT64, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Z,
                          ROCmValueType::INT64, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_PRINTF_BUFFER,
                          ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false }
                    },
                     "OpenCL C", { 1, 2 },
                     { BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED },
                     { BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED },
                     "", "", 64, 0, 0, 8, 64,
                     14, 11, 256,
                     { BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED },
                     BINGEN_NOTSUPPLIED, BINGEN_NOTSUPPLIED
                }
            }
        },
        true, ""
    },
    {   // test 1
        R"ffDXD(---
Version:         [ 1, 0 ]
Printf:          
  - '1:1:4:index\72%d\n'
  - '2:4:4:4:4:4:i=%d,a=%f,b=%f,c=%f\n'
Kernels:         
  - Name:            vectorAdd
    SymbolName:      'vectorAdd@kd'
    Language:        OpenCL C
    LanguageVersion: [ 1, 2 ]
    Attrs:
      ReqdWorkGroupSize:
        - 7
        - 9
        - 11
      WorkGroupSizeHint:
        - 112
        - 33
        - 66
      VecTypeHint: uint8
      RuntimeHandle:  kernelRT
    Args:            
      - Name:            n
        TypeName:        uint
        Size:            4
        Align:           4
        ValueKind:       ByValue
        ValueType:       U32
        AccQual:         Default
        ActualAccQual:   Default
      - Name:            a
        TypeName:        'float*'
        Size:            8
        Align:           16
        PointeeAlign:    32
        ValueKind:       GlobalBuffer
        ValueType:       F32
        AddrSpaceQual:   Global
        AccQual:         ReadOnly
        ActualAccQual:   ReadWrite
        IsConst:         true
      - Name:            b
        TypeName:        'float*'
        Size:            8
        Align:           16
        PointeeAlign:    32
        ValueKind:       GlobalBuffer
        ValueType:       F32
        AddrSpaceQual:   Global
        AccQual:         WriteOnly
        ActualAccQual:   WriteOnly
        IsConst:         true
      - Name:            c
        TypeName:        'float*'
        Size:            8
        Align:           8
        ValueKind:       GlobalBuffer
        ValueType:       F32
        AddrSpaceQual:   Global
        AccQual:         ReadWrite
        ActualAccQual:   ReadOnly
      - Size:            8
        Align:           8
        ValueKind:       HiddenGlobalOffsetX
        ValueType:       I64
      - Size:            8
        Align:           8
        ValueKind:       HiddenGlobalOffsetY
        ValueType:       I64
      - Size:            8
        Align:           8
        ValueKind:       HiddenGlobalOffsetZ
        ValueType:       I64
      - Size:            8
        Align:           8
        ValueKind:       HiddenPrintfBuffer
        ValueType:       I8
        AddrSpaceQual:   Global
      - Size:            8
        Align:           8
        ValueKind:       HiddenCompletionAction
        ValueType:       I8
        AddrSpaceQual:   Global
      - Size:            8
        Align:           8
        ValueKind:       HiddenNone
        ValueType:       I8
        AddrSpaceQual:   Global
      - Size:            8
        Align:           8
        ValueKind:       HiddenDefaultQueue
        ValueType:       I8
        AddrSpaceQual:   Global
    CodeProps:       
      KernargSegmentSize: 64
      GroupSegmentFixedSize: 120
      PrivateSegmentFixedSize: 408
      KernargSegmentAlign: 8
      WavefrontSize:   64
      NumSGPRs:        14
      NumVGPRs:        11
      MaxFlatWorkGroupSize: 256
      FixedWorkGroupSize: [ 11, 91, 96 ]
      NumSpilledSGPRs: 12
      NumSpilledVGPRs: 37
...
)ffDXD",
        {
            { 1, 0 }, // version
            {    // printfInfos
                { 1, { 4 }, "index:%d\n" },
                { 2, { 4, 4, 4, 4 }, "i=%d,a=%f,b=%f,c=%f\n" }
            },
            {
                {   // kernel 0
                    "vectorAdd", "vectorAdd@kd",
                    {   // arguments
                        { "n", "uint", 4, 4, 0, ROCmValueKind::BY_VALUE,
                          ROCmValueType::UINT32, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "a", "float*", 8, 16, 32, ROCmValueKind::GLOBAL_BUFFER,
                          ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::READ_ONLY, ROCmAccessQual::READ_WRITE,
                          true, false, false, false },
                        { "b", "float*", 8, 16, 32, ROCmValueKind::GLOBAL_BUFFER,
                          ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::WRITE_ONLY, ROCmAccessQual::WRITE_ONLY,
                          true, false, false, false },
                        { "c", "float*", 8, 8, 0, ROCmValueKind::GLOBAL_BUFFER,
                          ROCmValueType::FLOAT32, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::READ_WRITE, ROCmAccessQual::READ_ONLY,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_X,
                          ROCmValueType::INT64, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Y,
                          ROCmValueType::INT64, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_GLOBAL_OFFSET_Z,
                          ROCmValueType::INT64, ROCmAddressSpace::NONE,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_PRINTF_BUFFER,
                          ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_COMPLETION_ACTION,
                          ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_NONE,
                          ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false },
                        { "", "", 8, 8, 0, ROCmValueKind::HIDDEN_DEFAULT_QUEUE,
                          ROCmValueType::INT8, ROCmAddressSpace::GLOBAL,
                          ROCmAccessQual::DEFAULT, ROCmAccessQual::DEFAULT,
                          false, false, false, false }
                    },
                     "OpenCL C", { 1, 2 },
                     { 7, 9, 11 },
                     { 112, 33, 66 },
                     "uint8", "kernelRT", 64, 120, 408, 8, 64,
                     14, 11, 256,
                     { 11, 91, 96 },
                     12, 37
                }
            }
        },
        true, ""
    }
};

static void testROCmMetadataCase(cxuint testId, const ROCmMetadataTestCase& testCase)
{
    ROCmInput rocmInput{};
    rocmInput.deviceType = GPUDeviceType::FIJI;
    rocmInput.archMinor = 0;
    rocmInput.archStepping = 3;
    rocmInput.newBinFormat = true;
    rocmInput.target = "amdgcn-amd-amdhsa-amdgizcl-gfx803";
    rocmInput.metadataSize = ::strlen(testCase.input);
    rocmInput.metadata = testCase.input;
    // generate simple binary with metadata
    Array<cxbyte> output;
    {
        ROCmBinGenerator binGen(&rocmInput);
        binGen.generate(output);
    }
    // now we load binary
    const ROCmMetadata& expected = testCase.expected;
    ROCmMetadata result;
    bool good = true;
    CString error;
    try
    {
        ROCmBinary binary(output.size(), output.data(), ROCMBIN_CREATE_METADATAINFO);
        result = binary.getMetadataInfo();
    }
    catch(const ParseException& ex)
    {
        good = false;
        error = ex.what();
    }
    
    char testName[30];
    snprintf(testName, 30, "Test #%u", testId);
    assertValue(testName, "good", testCase.good, good);
    assertString(testName, "error", testCase.error, error.c_str());
    
    assertValue(testName, "version[0]", expected.version[0], result.version[0]);
    assertValue(testName, "version[1]", expected.version[1], result.version[1]);
    assertValue(testName, "printfInfosNum", expected.printfInfos.size(),
                result.printfInfos.size());
    char buf[32];
    for (cxuint i = 0; i < expected.printfInfos.size(); i++)
    {
        snprintf(buf, 32, "Printf[%u].", i);
        std::string caseName(buf);
        const ROCmPrintfInfo& expPrintf = expected.printfInfos[i];
        const ROCmPrintfInfo& resPrintf = result.printfInfos[i];
        assertValue(testName, caseName+"id", expPrintf.id, resPrintf.id);
        assertValue(testName, caseName+"argSizesNum", expPrintf.argSizes.size(),
                    resPrintf.argSizes.size());
        char buf2[32];
        for (cxuint j = 0; j < expPrintf.argSizes.size(); j++)
        {
            snprintf(buf2, 32, "argSizes[%u]", j);
            std::string caseName2(caseName);
            caseName2 += buf2;
            assertValue(testName, caseName2, expPrintf.argSizes[j], resPrintf.argSizes[j]);
        }
        assertValue(testName, caseName+"format", expPrintf.format, resPrintf.format);
    }
    
    assertValue(testName, "kernelsNum", expected.kernels.size(), result.kernels.size());
    // kernels
    for (cxuint i = 0; i < expected.kernels.size(); i++)
    {
        snprintf(buf, 32, "Kernel[%u].", i);
        std::string caseName(buf);
        const ROCmKernelMetadata& expKernel = expected.kernels[i];
        const ROCmKernelMetadata& resKernel = result.kernels[i];
        
        assertValue(testName, caseName+"name", expKernel.name, resKernel.name);
        assertValue(testName, caseName+"symbolName",
                    expKernel.symbolName, resKernel.symbolName);
        assertValue(testName, caseName+"argsNum",
                    expKernel.argInfos.size(), resKernel.argInfos.size());
        
        char buf2[32];
        for (cxuint j = 0; j < expKernel.argInfos.size(); j++)
        {
            snprintf(buf2, 32, "args[%u].", j);
            std::string caseName2(caseName);
            caseName2 += buf2;
            const ROCmKernelArgInfo& expArgInfo = expKernel.argInfos[j];
            const ROCmKernelArgInfo& resArgInfo = resKernel.argInfos[j];
            assertValue(testName, caseName2+"name", expArgInfo.name, resArgInfo.name);
            assertValue(testName, caseName2+"typeName",
                        expArgInfo.typeName, resArgInfo.typeName);
            assertValue(testName, caseName2+"size",
                        expArgInfo.size, resArgInfo.size);
            assertValue(testName, caseName2+"align",
                        expArgInfo.align, resArgInfo.align);
            assertValue(testName, caseName2+"pointeeAlign",
                        expArgInfo.pointeeAlign, resArgInfo.pointeeAlign);
            assertValue(testName, caseName2+"valueKind",
                        cxuint(expArgInfo.valueKind), cxuint(resArgInfo.valueKind));
            assertValue(testName, caseName2+"valueType",
                        cxuint(expArgInfo.valueType), cxuint(resArgInfo.valueType));
            assertValue(testName, caseName2+"addressSpace",
                        cxuint(expArgInfo.addressSpace), cxuint(resArgInfo.addressSpace));
            assertValue(testName, caseName2+"accessQual",
                        cxuint(expArgInfo.accessQual), cxuint(resArgInfo.accessQual));
            assertValue(testName, caseName2+"actualAccessQual",
                cxuint(expArgInfo.actualAccessQual), cxuint(resArgInfo.actualAccessQual));
            assertValue(testName, caseName2+"isConst",
                        cxuint(expArgInfo.isConst), cxuint(resArgInfo.isConst));
            assertValue(testName, caseName2+"isRestrict",
                        cxuint(expArgInfo.isRestrict), cxuint(resArgInfo.isRestrict));
            assertValue(testName, caseName2+"isPipe",
                        cxuint(expArgInfo.isPipe), cxuint(resArgInfo.isPipe));
            assertValue(testName, caseName2+"isVolatile",
                        cxuint(expArgInfo.isVolatile), cxuint(resArgInfo.isVolatile));
        }
        
        assertValue(testName, caseName+"language", expKernel.language, resKernel.language);
        assertValue(testName, caseName+"langVersion[0]", expKernel.langVersion[0],
                    resKernel.langVersion[0]);
        assertValue(testName, caseName+"langVersion[1]", expKernel.langVersion[1],
                    resKernel.langVersion[1]);
        assertValue(testName, caseName+"reqdWorkGroupSize[0]", 
                    expKernel.reqdWorkGroupSize[0], resKernel.reqdWorkGroupSize[0]);
        assertValue(testName, caseName+"reqdWorkGroupSize[1]", 
                    expKernel.reqdWorkGroupSize[1], resKernel.reqdWorkGroupSize[1]);
        assertValue(testName, caseName+"reqdWorkGroupSize[2]", 
                    expKernel.reqdWorkGroupSize[2], resKernel.reqdWorkGroupSize[2]);
        assertValue(testName, caseName+"workGroupSizeHint[0]", 
                    expKernel.workGroupSizeHint[0], resKernel.workGroupSizeHint[0]);
        assertValue(testName, caseName+"workGroupSizeHint[1]", 
                    expKernel.workGroupSizeHint[1], resKernel.workGroupSizeHint[1]);
        assertValue(testName, caseName+"workGroupSizeHint[2]", 
                    expKernel.workGroupSizeHint[2], resKernel.workGroupSizeHint[2]);
        assertValue(testName, caseName+"vecTypeHint",
                    expKernel.vecTypeHint, resKernel.vecTypeHint);
        assertValue(testName, caseName+"runtimeHandle",
                    expKernel.runtimeHandle, resKernel.runtimeHandle);
        assertValue(testName, caseName+"kernargSegmentSize",
                    expKernel.kernargSegmentSize, resKernel.kernargSegmentSize);
        assertValue(testName, caseName+"groupSegmentFixedSize",
                    expKernel.groupSegmentFixedSize, resKernel.groupSegmentFixedSize);
        assertValue(testName, caseName+"privateSegmentFixedSize",
                    expKernel.privateSegmentFixedSize, resKernel.privateSegmentFixedSize);
        assertValue(testName, caseName+"kernargSegmentAlign",
                    expKernel.kernargSegmentAlign, resKernel.kernargSegmentAlign);
        assertValue(testName, caseName+"wavefrontSize",
                    expKernel.wavefrontSize, resKernel.wavefrontSize);
        assertValue(testName, caseName+"sgprsNum", expKernel.sgprsNum, resKernel.sgprsNum);
        assertValue(testName, caseName+"vgprsNum", expKernel.vgprsNum, resKernel.vgprsNum);
        assertValue(testName, caseName+"maxFlatWorkGroupSize",
                    expKernel.maxFlatWorkGroupSize, resKernel.maxFlatWorkGroupSize);
        assertValue(testName, caseName+"fixedWorkGroupSize[0]",
                    expKernel.fixedWorkGroupSize[0], resKernel.fixedWorkGroupSize[0]);
        assertValue(testName, caseName+"fixedWorkGroupSize[1]",
                    expKernel.fixedWorkGroupSize[1], resKernel.fixedWorkGroupSize[1]);
        assertValue(testName, caseName+"fixedWorkGroupSize[2]",
                    expKernel.fixedWorkGroupSize[2], resKernel.fixedWorkGroupSize[2]);
        assertValue(testName, caseName+"spilledSgprs",
                    expKernel.spilledSgprs, resKernel.spilledSgprs);
        assertValue(testName, caseName+"spilledVgprs",
                    expKernel.spilledVgprs, resKernel.spilledVgprs);
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(rocmMetadataTestCases)/sizeof(ROCmMetadataTestCase); i++)
        try
        { testROCmMetadataCase(i, rocmMetadataTestCases[i]); }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
