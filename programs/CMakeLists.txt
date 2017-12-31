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

SET(LINK_LIBRARIES CLRXAmdAsm${PROGRAM_LIB_SUFFIX} CLRXAmdBin${PROGRAM_LIB_SUFFIX}
        CLRXUtils${PROGRAM_LIB_SUFFIX}
        ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

ADD_EXECUTABLE(clrxdisasm clrxdisasm.cpp)

TARGET_LINK_LIBRARIES(clrxdisasm ${LINK_LIBRARIES})

INSTALL(TARGETS clrxdisasm RUNTIME DESTINATION bin)

ADD_EXECUTABLE(clrxasm clrxasm.cpp)

TARGET_LINK_LIBRARIES(clrxasm ${LINK_LIBRARIES})

INSTALL(TARGETS clrxasm RUNTIME DESTINATION bin)

IF(BUILD_MANUAL)
    POD2MAN("${PROJECT_SOURCE_DIR}/programs/clrxdisasm.pod" clrxdisasm 1)
    POD2MAN("${PROJECT_SOURCE_DIR}/programs/clrxasm.pod" clrxasm 1)
ENDIF(BUILD_MANUAL)
