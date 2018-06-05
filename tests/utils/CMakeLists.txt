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

ADD_EXECUTABLE(cstrtofXTest cstrtofXTest.cpp)
TEST_LINK_LIBRARIES(cstrtofXTest CLRXUtils)
ADD_TEST(cstrtofXTest cstrtofXTest)

ADD_EXECUTABLE(CLIParser CLIParser.cpp)
TEST_LINK_LIBRARIES(CLIParser CLRXUtils)
ADD_TEST(CLIParser CLIParser)

ADD_EXECUTABLE(GPUId GPUId.cpp)
TEST_LINK_LIBRARIES(GPUId CLRXUtils)
ADD_TEST(GPUId GPUId)

ADD_EXECUTABLE(DTree DTree.cpp)
TEST_LINK_LIBRARIES(DTree CLRXUtils)
ADD_TEST(DTree DTree)
