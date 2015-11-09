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

ADD_LIBRARY(SamplesCLUtils STATIC CLUtils.cpp)

SET(LINK_LIBRARIES SamplesCLUtils OpenCL CLRXAmdAsm CLRXAmdBin CLRXUtils)

ADD_EXECUTABLE(VectorAdd VectorAdd.cpp)
TARGET_LINK_LIBRARIES(VectorAdd ${LINK_LIBRARIES})

IF(PNG_FOUND)
    ADD_EXECUTABLE(ImageMix ImageMix.cpp)
    TARGET_LINK_LIBRARIES(ImageMix ${LINK_LIBRARIES} ${PNG_LIBRARIES})
ENDIF(PNG_FOUND)
