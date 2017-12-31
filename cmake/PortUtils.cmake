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

# set output name for static library (if windows then this macor doesn't not set)
MACRO(SET_TARGET_OUTNAME TARGET OUTNAME)
  IF(NOT WIN32)
  SET_TARGET_PROPERTIES(${TARGET} PROPERTIES VERSION ${CLRX_VERSION}
          OUTPUT_NAME ${OUTNAME})
  ENDIF(NOT WIN32)
ENDMACRO(SET_TARGET_OUTNAME TARGET VERSION OUTNAME)

# this macro set libraries for test (for windows sets static libraries)
MACRO(TEST_LINK_LIBRARIES)
    SET(PROG ${ARGV0})
    SET(MYLIBS "${ARGN}")
    LIST(REMOVE_AT MYLIBS 0)
    IF(WIN32)
        SET(TESTLIBS "")
        FOREACH(LIB IN LISTS MYLIBS)
            SET(LIB "${LIB}Static")
            LIST(APPEND TESTLIBS "${LIB}")
        ENDFOREACH(LIB IN LISTS MYLIBS)
    ELSE(WIN32)
        SET(TESTLIBS "${MYLIBS}")
    ENDIF(WIN32)
    TARGET_LINK_LIBRARIES(${PROG} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${TESTLIBS})
ENDMACRO(TEST_LINK_LIBRARIES PROG LIBS)
