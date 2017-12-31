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

ADD_LIBRARY(SamplesCLUtils STATIC CLUtils.cpp)

SET(LINK_LIBRARIES SamplesCLUtils OpenCL CLRXCLHelper${PROGRAM_LIB_SUFFIX}
        CLRXAmdAsm${PROGRAM_LIB_SUFFIX}
        CLRXAmdBin${PROGRAM_LIB_SUFFIX} CLRXUtils${PROGRAM_LIB_SUFFIX}
        ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

SET(SAMPLESBINDIR share/CLRX/samples)

ADD_EXECUTABLE(VectorAdd VectorAdd.cpp)
TARGET_LINK_LIBRARIES(VectorAdd ${LINK_LIBRARIES} ${MATHLIB})

INSTALL(TARGETS VectorAdd RUNTIME DESTINATION ${SAMPLESBINDIR})

IF(PNG_FOUND)
    ADD_EXECUTABLE(ImageMix ImageMix.cpp)
    TARGET_LINK_LIBRARIES(ImageMix ${LINK_LIBRARIES} ${PNG_LIBRARIES})
    
    INSTALL(TARGETS ImageMix RUNTIME DESTINATION ${SAMPLESBINDIR})
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/samples/image1.png" DESTINATION ${SAMPLESBINDIR})
    INSTALL(FILES "${PROJECT_SOURCE_DIR}/samples/image2.png" DESTINATION ${SAMPLESBINDIR})
ENDIF(PNG_FOUND)

ADD_EXECUTABLE(ReverseBits ReverseBits.cpp)
TARGET_LINK_LIBRARIES(ReverseBits ${LINK_LIBRARIES})

INSTALL(TARGETS ReverseBits RUNTIME DESTINATION ${SAMPLESBINDIR})

ADD_EXECUTABLE(GenericPtr GenericPtr.cpp)
TARGET_LINK_LIBRARIES(GenericPtr ${LINK_LIBRARIES})

INSTALL(TARGETS GenericPtr RUNTIME DESTINATION ${SAMPLESBINDIR})
