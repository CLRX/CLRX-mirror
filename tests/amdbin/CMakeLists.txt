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

ADD_EXECUTABLE(GalliumBinGen GalliumBinGen.cpp)
TEST_LINK_LIBRARIES(GalliumBinGen CLRXAmdBin CLRXUtils)
ADD_TEST(GalliumBinGen GalliumBinGen)

ADD_EXECUTABLE(AmdBinGen AmdBinGen.cpp)
TEST_LINK_LIBRARIES(AmdBinGen CLRXAmdBin CLRXUtils)
ADD_TEST(AmdBinGen AmdBinGen)

ADD_EXECUTABLE(AmdBinLoading AmdBinLoading.cpp)
TEST_LINK_LIBRARIES(AmdBinLoading CLRXAmdBin CLRXUtils)
ADD_TEST(AmdBinLoading AmdBinLoading)

ADD_EXECUTABLE(AmdCL2BinGen AmdCL2BinGen.cpp)
TEST_LINK_LIBRARIES(AmdCL2BinGen CLRXAmdBin CLRXUtils)
ADD_TEST(AmdCL2BinGen AmdCL2BinGen)

ADD_EXECUTABLE(ROCmBinGen ROCmBinGen.cpp)
TEST_LINK_LIBRARIES(ROCmBinGen CLRXAmdBin CLRXUtils)
ADD_TEST(ROCmBinGen ROCmBinGen)

ADD_EXECUTABLE(ROCmMetadata ROCmMetadata.cpp)
TEST_LINK_LIBRARIES(ROCmMetadata CLRXAmdBin CLRXUtils)
ADD_TEST(ROCmMetadata ROCmMetadata)
