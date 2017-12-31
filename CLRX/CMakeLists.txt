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

INSTALL(DIRECTORY "${PROJECT_SOURCE_DIR}/CLRX"
        DESTINATION include FILES_MATCHING PATTERN "*.h")

INSTALL(FILES "${PROJECT_BINARY_DIR}/CLRX/Config.h" DESTINATION include/CLRX)

IF(BUILD_DOXYGEN)
  FIND_PACKAGE(Doxygen)
  IF(DOXYGEN_FOUND)
      FILE(GLOB_RECURSE CLRX_HEADERS "${PROJECT_SOURCE_DIR}/CLRX/*")
      #-- Configure the Template Doxyfile for our specific project
      CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/Doxyfile.in" "${PROJECT_BINARY_DIR}/Doxyfile"
                  @ONLY IMMEDIATE)
      ADD_CUSTOM_TARGET(Docs ALL DEPENDS "${PROJECT_BINARY_DIR}/doc/doxygen.stamp")
      #-- Add a custom target to run Doxygen when ever the project is built
      ADD_CUSTOM_COMMAND(OUTPUT "${PROJECT_BINARY_DIR}/doc/doxygen.stamp"
            COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/Doxyfile"
            COMMAND cmake -E touch "${PROJECT_BINARY_DIR}/doc/doxygen.stamp"
            DEPENDS "${PROJECT_BINARY_DIR}/Doxyfile" ${CLRX_HEADERS})
      # IF you do NOT want the documentation to be generated EVERY time you build the project
      # then leave out the 'ALL' keyword from the above command.
      INSTALL(DIRECTORY "${PROJECT_BINARY_DIR}/doc/" DESTINATION share/doc/CLRX/doxygen
            PATTERN *.cmake EXCLUDE PATTERN CMakeFiles EXCLUDE
            PATTERN Makefile EXCLUDE PATTERN doxygen.stamp EXCLUDE)
  ELSE(DOXYGEN_FOUND)
    MESSAGE(WARNING
      "Doxygen documentation will not be built, because no correct Doxygen installation")
  ENDIF(DOXYGEN_FOUND)
ENDIF(BUILD_DOXYGEN)

