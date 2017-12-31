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

CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/pkgconfig/clrxamdasm.pc.in"
        "${PROJECT_BINARY_DIR}/pkgconfig/clrxamdasm.pc" @ONLY)
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/pkgconfig/clrxamdbin.pc.in"
        "${PROJECT_BINARY_DIR}/pkgconfig/clrxamdbin.pc" @ONLY)
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/pkgconfig/clrxutils.pc.in"
        "${PROJECT_BINARY_DIR}/pkgconfig/clrxutils.pc" @ONLY)

SET(PKGCONFIG_INSTALL_DIR ${LIB_INSTALL_DIR}/pkgconfig)
INSTALL(FILES "${PROJECT_BINARY_DIR}/pkgconfig/clrxamdasm.pc"
              "${PROJECT_BINARY_DIR}/pkgconfig/clrxamdbin.pc"
              "${PROJECT_BINARY_DIR}/pkgconfig/clrxutils.pc"
              DESTINATION "${PKGCONFIG_INSTALL_DIR}")
              
IF(HAVE_OPENCL)
    CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/pkgconfig/clrxclhelper.pc.in"
            "${PROJECT_BINARY_DIR}/pkgconfig/clrxclhelper.pc" @ONLY)
    INSTALL(FILES "${PROJECT_BINARY_DIR}/pkgconfig/clrxclhelper.pc"
              DESTINATION "${PKGCONFIG_INSTALL_DIR}")
ENDIF(HAVE_OPENCL)
