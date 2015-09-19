####
#  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
#  Copyright (C) 2014-2015 Mateusz Szpakowski
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_EXECUTABLE(GCNDisasmOpcodes
        GCNDisasmOpcodes.cpp
        GCNDisasmOpc11.cpp
        GCNDisasmOpc12.cpp)
TARGET_LINK_LIBRARIES(GCNDisasmOpcodes ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(GCNDisasmOpcodes GCNDisasmOpcodes)

ADD_EXECUTABLE(GCNDisasmLabels GCNDisasmLabels.cpp)
TARGET_LINK_LIBRARIES(GCNDisasmLabels ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(GCNDisasmLabels GCNDisasmLabels)

ADD_EXECUTABLE(DisasmDataTest DisasmDataTest.cpp)
TARGET_LINK_LIBRARIES(DisasmDataTest ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(DisasmDataTest DisasmDataTest)

ADD_EXECUTABLE(AsmExprParse AsmExprParse.cpp)
TARGET_LINK_LIBRARIES(AsmExprParse ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(AsmExprParse AsmExprParse)

ADD_EXECUTABLE(AssemblerBasics AssemblerBasics.cpp)
TARGET_LINK_LIBRARIES(AssemblerBasics ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(AssemblerBasics AssemblerBasics)

ADD_EXECUTABLE(AsmFormats AsmFormats.cpp)
TARGET_LINK_LIBRARIES(AsmFormats ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(AsmFormats AsmFormats)

ADD_EXECUTABLE(GCNAsmOpcodes
        GCNAsmOpcodes.cpp
        GCNAsmOpc11.cpp)
TARGET_LINK_LIBRARIES(GCNAsmOpcodes ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(GCNAsmOpcodes GCNAsmOpcodes)
