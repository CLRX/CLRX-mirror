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

ADD_DEFINITIONS(-DCLRX_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")

ADD_EXECUTABLE(cstrtofXTest cstrtofXTest.cpp)
TARGET_LINK_LIBRARIES(cstrtofXTest ${CMAKE_THREAD_LIBS_INIT} CLRXUtils)
ADD_TEST(cstrtofXTest cstrtofXTest)

ADD_EXECUTABLE(GCNDisasmOpcodes
        GCNDisasmOpcodes.cpp
        GCNDisasmOpc11.cpp
        GCNDisasmOpc12.cpp)
TARGET_LINK_LIBRARIES(GCNDisasmOpcodes ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(GCNDisasmOpcodes GCNDisasmOpcodes)

ADD_EXECUTABLE(GCNDisasmLabels GCNDisasmLabels.cpp)
TARGET_LINK_LIBRARIES(GCNDisasmLabels ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(GCNDisasmLabels GCNDisasmLabels)

ADD_EXECUTABLE(AsmExprParse AsmExprParse.cpp)
TARGET_LINK_LIBRARIES(AsmExprParse ${CMAKE_THREAD_LIBS_INIT} CLRXAmdAsm CLRXUtils)
ADD_TEST(AsmExprParse AsmExprParse)

ADD_EXECUTABLE(GalliumBinGen GalliumBinGen.cpp)
TARGET_LINK_LIBRARIES(GalliumBinGen ${CMAKE_THREAD_LIBS_INIT} CLRXAmdBin CLRXUtils)
ADD_TEST(GalliumBinGen GalliumBinGen)

ADD_EXECUTABLE(AmdBinGen AmdBinGen.cpp)
TARGET_LINK_LIBRARIES(AmdBinGen ${CMAKE_THREAD_LIBS_INIT} CLRXAmdBin CLRXUtils)
ADD_TEST(AmdBinGen AmdBinGen)

ADD_EXECUTABLE(CLIParser CLIParser.cpp)
TARGET_LINK_LIBRARIES(CLIParser ${CMAKE_THREAD_LIBS_INIT} CLRXAmdBin CLRXUtils)
ADD_TEST(CLIParser CLIParser)
