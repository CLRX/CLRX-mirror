####
#  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
#  Copyright (C) 2014-2017 Mateusz Szpakowski
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

SET(LIBAMDBINSRC
        AmdBinaries.cpp
        AmdBinGen.cpp
        AmdCL2Binaries.cpp
        AmdCL2BinGen.cpp
        ElfBinaries.cpp
        GalliumBinaries.cpp
        ROCmBinaries.cpp)

SET(LINK_LIBRARIES CLRXUtils)

ADD_LIBRARY(CLRXAmdBin SHARED ${LIBAMDBINSRC})

TARGET_LINK_LIBRARIES(CLRXAmdBin ${LINK_LIBRARIES})
SET_TARGET_PROPERTIES(CLRXAmdBin PROPERTIES VERSION ${CLRX_VERSION}
        SOVERSION ${CLRX_MAJOR_VERSION})

INSTALL(TARGETS CLRXAmdBin LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR} RUNTIME DESTINATION bin)

IF(NOT NO_STATIC)
    ADD_LIBRARY(CLRXAmdBinStatic STATIC ${LIBAMDBINSRC})
    SET_TARGET_OUTNAME(CLRXAmdBinStatic CLRXAmdBin)
    
    INSTALL(TARGETS CLRXAmdBinStatic ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
            LIBRARY DESTINATION ${LIB_INSTALL_DIR})
ENDIF(NOT NO_STATIC)
