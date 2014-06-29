####
#  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
#  Copyright (C) 2014 Mateusz Szpakowski
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

SET(LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

SET(LIBUTILSSRC Utilities.cpp)

ADD_LIBRARY(CLRXUtils SHARED ${LIBUTILSSRC})

TARGET_LINK_LIBRARIES(CLRXUtils ${LINK_LIBRARIES})
SET_TARGET_PROPERTIES(CLRXUtils PROPERTIES VERSION ${CLRX_VERSION}
        SOVERSION ${CLRX_MAJOR_VERSION})

INSTALL(TARGETS CLRXUtils LIBRARY DESTINATION ${LIB_INSTALL_DIR})

IF(NOT NO_STATIC)
    ADD_LIBRARY(CLRXUtilsStatic STATIC ${LIBUTILSSRC})
    SET_TARGET_PROPERTIES(CLRXUtilsStatic PROPERTIES VERSION ${CLRX_VERSION}
            OUTPUT_NAME CLRXUtils)
    
    INSTALL(TARGETS CLRXUtilsStatic ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
ENDIF(NOT NO_STATIC)
