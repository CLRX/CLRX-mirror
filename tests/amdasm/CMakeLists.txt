####
#  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
#  Copyright (C) 2014-2018 Mateusz Szpakowski
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
####

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.1)

ADD_EXECUTABLE(GCNDisasmOpcodes
        GCNDisasmOpcodes.cpp
        GCNDisasmOpc11.cpp
        GCNDisasmOpc12.cpp
        GCNDisasmOpc14.cpp)
TEST_LINK_LIBRARIES(GCNDisasmOpcodes CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(GCNDisasmOpcodes GCNDisasmOpcodes)

ADD_EXECUTABLE(GCNDisasmLabels GCNDisasmLabels.cpp)
TEST_LINK_LIBRARIES(GCNDisasmLabels CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(GCNDisasmLabels GCNDisasmLabels)

ADD_EXECUTABLE(DisasmDataTest DisasmDataTest.cpp)
TEST_LINK_LIBRARIES(DisasmDataTest CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(DisasmDataTest DisasmDataTest)

ADD_EXECUTABLE(AsmExprParse AsmExprParse.cpp)
TEST_LINK_LIBRARIES(AsmExprParse CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmExprParse AsmExprParse)

ADD_EXECUTABLE(AssemblerBasics AssemblerBasics.cpp
        AsmBasicsCases1.cpp
        AsmBasicsCases2.cpp)
TEST_LINK_LIBRARIES(AssemblerBasics CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AssemblerBasics AssemblerBasics)

ADD_EXECUTABLE(AsmAmdFormat AsmAmdFormat.cpp)
TEST_LINK_LIBRARIES(AsmAmdFormat CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmAmdFormat AsmAmdFormat)

ADD_EXECUTABLE(AsmAmdCL2Format AsmAmdCL2Format.cpp)
TEST_LINK_LIBRARIES(AsmAmdCL2Format CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmAmdCL2Format AsmAmdCL2Format)

ADD_EXECUTABLE(AsmGalliumFormat AsmGalliumFormat.cpp)
TEST_LINK_LIBRARIES(AsmGalliumFormat CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmGalliumFormat AsmGalliumFormat)

ADD_EXECUTABLE(AsmROCmFormat AsmROCmFormat.cpp)
TEST_LINK_LIBRARIES(AsmROCmFormat CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmROCmFormat AsmROCmFormat)

ADD_EXECUTABLE(GCNAsmOpcodes
        GCNAsmOpcodes.cpp
        GCNAsmOpc11.cpp
        GCNAsmOpc12.cpp
        GCNAsmOpc14.cpp)
TEST_LINK_LIBRARIES(GCNAsmOpcodes CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(GCNAsmOpcodes GCNAsmOpcodes)

ADD_EXECUTABLE(AsmRegPool AsmRegPool.cpp)
TEST_LINK_LIBRARIES(AsmRegPool CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmRegPool AsmRegPool)

ADD_EXECUTABLE(GCNRegVarUsage GCNRegVarUsage.cpp)
TEST_LINK_LIBRARIES(GCNRegVarUsage CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(GCNRegVarUsage GCNRegVarUsage)

ADD_EXECUTABLE(AsmCodeFlow AsmCodeFlow.cpp)
TEST_LINK_LIBRARIES(AsmCodeFlow CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmCodeFlow AsmCodeFlow)

ADD_EXECUTABLE(AsmRegAlloc
        AsmRegAlloc.cpp
        AsmRegAllocCase1.cpp
        AsmRegAllocCase2.cpp
        AsmRegAllocCase3.cpp)
TEST_LINK_LIBRARIES(AsmRegAlloc CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmRegAlloc AsmRegAlloc)

ADD_EXECUTABLE(AsmRegAlloc2 AsmRegAlloc2.cpp)
TEST_LINK_LIBRARIES(AsmRegAlloc2 CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmRegAlloc2 AsmRegAlloc2)

ADD_EXECUTABLE(AsmRegAlloc3 AsmRegAlloc3.cpp)
TEST_LINK_LIBRARIES(AsmRegAlloc3 CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmRegAlloc3 AsmRegAlloc3)

ADD_EXECUTABLE(AsmSourcePosHandler AsmSourcePosHandler.cpp)
TEST_LINK_LIBRARIES(AsmSourcePosHandler CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(AsmSourcePosHandler AsmSourcePosHandler)

ADD_EXECUTABLE(GCNWaitHandle GCNWaitHandle.cpp)
TEST_LINK_LIBRARIES(GCNWaitHandle CLRXAmdAsm CLRXAmdBin CLRXUtils)
ADD_TEST(GCNWaitHandle GCNWaitHandle)
