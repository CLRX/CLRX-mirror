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

SET(LIBCLRWRAPPERSRC CLInternals.cpp
        CLFunctions1.cpp
        CLFunctions2.cpp
        CLFunctions3.cpp)

ADD_LIBRARY(CLRXWrapper SHARED ${LIBCLRWRAPPERSRC})

TARGET_LINK_LIBRARIES(CLRXWrapper ${CMAKE_THREAD_LIBS_INIT} CLRXUtils CLRXAmdAsm CLRXAmdBin)
SET_TARGET_PROPERTIES(CLRXWrapper PROPERTIES VERSION ${CLRX_LIB_VERSION}
        SOVERSION ${CLRX_MAJOR_VERSION} COMPILE_FLAGS "-D__CLRXWRAPPER__=1")

INSTALL(TARGETS CLRXWrapper LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR} RUNTIME DESTINATION bin)
