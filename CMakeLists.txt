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

PROJECT(CLRadeonExtender)

SET(CLRX_VERSION 0.1.0)
SET(CLRX_MAJOR_VERSION 0)
SET(CLRX_MINOR_VERSION 1)
SET(CLRX_MICRO_VERSION 0)

ENABLE_LANGUAGE(CXX)

#####
# packacing
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Unofficial OpenCL Radeon extensions")
SET(CPACK_PACKAGE_VENDOR "matszpk")
SET(CPACK_PACKAGE_MAINTAINER "matszpk")
SET(CPACK_PACKAGE_VERSION_MAJOR ${CLRX_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${CLRX_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${CLRX_MICRO_VERSION})

INCLUDE(CPack)

######
## end packaging

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=gnu++11" HAVE_GNU11_FLAG)

# workaround for 
SET(CMAKE_REQUIRED_FLAGS "-std=gnu++11")
CHECK_INCLUDE_FILE_CXX(iostream HAVE_GNU11_IOSTREAM)
UNSET(CMAKE_REQUIRED_FLAGS)

IF(HAVE_GNU11_FLAG AND HAVE_GNU11_IOSTREAM)
    SET(CPP_STANDARD "-std=gnu++11")
    SET(HAVE_GNU11 1)
ELSE(HAVE_GNU11_FLAG AND HAVE_GNU11_IOSTREAM)
    SET(CPP_STANDARD "-std=c++11")
ENDIF(HAVE_GNU11_FLAG AND HAVE_GNU11_IOSTREAM)

MESSAGE(STATUS "Use standard ${CPP_STANDARD} for ${CMAKE_CXX_COMPILER}")

### 32-bit compilation option
OPTION(BUILD_32BIT "Build 32-bit version" OFF)

IF(NOT LIB_INSTALL_DIR)
    IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND
        CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT BUILD_32BIT)
        SET(LIB_INSTALL_DIR "lib64")
    ELSE(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND
        CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT BUILD_32BIT)
        SET(LIB_INSTALL_DIR "lib")
    ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND
        CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT BUILD_32BIT)
    MESSAGE("Using library install dirname ${LIB_INSTALL_DIR}")
ENDIF(NOT LIB_INSTALL_DIR)

IF(BUILD_32BIT)
    SET(CMAKE_CXX_FLAGS "-Wall -m32 ${CPP_STANDARD}")
    SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -m32 ${CPP_STANDARD} -g -O0 -DDEBUG=1")
    SET(CMAKE_CXX_FLAGS_DEBUGCOV "-Wall -m32 ${CPP_STANDARD} -g -O0 -DDEBUG=1 --coverage")
    SET(CMAKE_CXX_FLAGS_GCCSAN
    "-Wall -m32 ${CPP_STANDARD} -g -O0 -DDEBUG=1 -fstack-protector-all -fsanitize=address")
    SET(CMAKE_CXX_FLAGS_GCCSSP
        "-Wall -m32 ${CPP_STANDARD} -g -O0 -DDEBUG=1 -fstack-protector-all")
    SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -m32 ${CPP_STANDARD} -O2 -fomit-frame-pointer")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "-Wall -m32 ${CPP_STANDARD} -g -O2 -funroll-loops -DDEBUG=1")
    SET(CMAKE_CXX_FLAGS_RELWITHPROFILE
        "-Wall -m32 ${CPP_STANDARD} -g -O2 -pg -funroll-loops")
ELSE(BUILD_32BIT)
    SET(CMAKE_CXX_FLAGS "-Wall ${CPP_STANDARD}")
    SET(CMAKE_CXX_FLAGS_DEBUG "-Wall ${CPP_STANDARD} -g -O0 -DDEBUG=1")
    SET(CMAKE_CXX_FLAGS_DEBUGCOV "-Wall ${CPP_STANDARD} -g -O0 -DDEBUG=1 --coverage")
    SET(CMAKE_CXX_FLAGS_GCCSAN
        "-Wall ${CPP_STANDARD} -g -O0 -DDEBUG=1 -fstack-protector-all -fsanitize=address")
    SET(CMAKE_CXX_FLAGS_GCCSSP
        "-Wall ${CPP_STANDARD} -g -O0 -DDEBUG=1 -fstack-protector-all")
    SET(CMAKE_CXX_FLAGS_RELEASE "-Wall ${CPP_STANDARD} -O2 -fomit-frame-pointer")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "-Wall ${CPP_STANDARD} -g -O2 -funroll-loops -DDEBUG=1")
    SET(CMAKE_CXX_FLAGS_RELWITHPROFILE "-Wall ${CPP_STANDARD} -g -O2 -pg -funroll-loops")
ENDIF(BUILD_32BIT)

IF(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})
    MESSAGE("Please do an out-of-tree build:")
    MESSAGE("rm -f CMakeCache.txt; mkdir -p default; cd default; cmake ..; make")
    MESSAGE(FATAL_ERROR "in-tree-build detected")
ENDIF(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR})

OPTION(NO_STATIC "Disable static during compiling" OFF)

####
# Documentation
####
OPTION(BUILD_DOCUMENTATION "Build whole documentation" OFF)
OPTION(BUILD_DOXYGEN "Use Doxygen to create the HTML based API documentation" OFF)
OPTION(BUILD_MANUAL "Use Pod2Man to create the Unix manuals" OFF)
OPTION(BUILD_CLRXDOC "Use markdown_py to create offline wiki pages" OFF)

IF(BUILD_DOCUMENTATION)
  SET(BUILD_DOXYGEN ON)
  SET(BUILD_MANUAL ON)
  SET(BUILD_CLRXDOC ON)
ENDIF(BUILD_DOCUMENTATION)

IF(BUILD_DOXYGEN)
  FIND_PACKAGE(Doxygen)
  IF(NOT DOXYGEN_FOUND)
    MESSAGE(FATAL_ERROR 
      "Doxygen is needed to build the documentation. Please install it correctly")
  ENDIF(NOT DOXYGEN_FOUND)
  #-- Configure the Template Doxyfile for our specific project
  CONFIGURE_FILE(Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile  @ONLY IMMEDIATE)
  #-- Add a custom target to run Doxygen when ever the project is built
  ADD_CUSTOM_TARGET(Docs COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
        SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
  # IF you do NOT want the documentation to be generated EVERY time you build the project
  # then leave out the 'ALL' keyword from the above command.
  INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/doc/ DESTINATION share/doc/CLRX/doxygen)
ENDIF(BUILD_DOXYGEN)

IF(BUILD_CLRXDOC)
  FIND_PROGRAM(MARKDOWN_PY markdown_py)
  IF(NOT MARKDOWN_PY)
    MESSAGE(FATAL_ERROR "markdown_py is needed to build CLRX documentation")
  ENDIF(NOT MARKDOWN_PY)
  ADD_CUSTOM_TARGET(CLRXDocs ALL COMMAND sh ${PROJECT_SOURCE_DIR}/doc/genclrxdoc.sh
          ${PROJECT_SOURCE_DIR}/doc ${PROJECT_BINARY_DIR}/clrxdoc)
  INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/clrxdoc/ DESTINATION share/doc/CLRX/clrx)
ENDIF(BUILD_CLRXDOC)

IF(BUILD_MANUAL)
    INCLUDE(Pod2Man)
    ADD_MANPAGE_TARGET()
ENDIF(BUILD_MANUAL)
####
# Documentation
####

####
# OpenCL find
####
SET(OPENCL_DIST_DIR "" CACHE PATH "OpenCL Distribution directory")

IF(NOT OPENCL_DIST_DIR)
    IF(EXISTS /opt/AMDAPP)
        SET(OPENCL_DIST_DIR "/opt/AMDAPP")
        MESSAGE(STATUS "OpenCL AMDAPP found")
    ELSEIF(EXISTS /usr/include/CL)
        SET(OPENCL_DIST_DIR "/usr")
        MESSAGE(STATUS "Standard OpenCL found.")
    ENDIF(EXISTS /opt/AMDAPP)
ENDIF(NOT OPENCL_DIST_DIR)

IF (OPENCL_DIST_DIR)
    SET(OPENCL_LIBRARY_DIRS "")
    SET(OPENCL_INCLUDE_DIRS "${OPENCL_DIST_DIR}/include")
    SET(OPENCL_LIBRARIES "OpenCL")
    SET(HAVE_OPENCL 1)
ENDIF(OPENCL_DIST_DIR)

# check for AMDAPP lib/x86 or lib/x86_64 libOpenCL.so file
IF(HAVE_OPENCL)
    IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT BUILD_32BIT)
        IF(EXISTS ${OPENCL_DIST_DIR}/lib/x86_64/libOpenCL.so)
            SET(OPENCL_LIBRARY_DIRS "/opt/AMDAPP/lib/x86_64")
        ENDIF(EXISTS ${OPENCL_DIST_DIR}/lib/x86_64/libOpenCL.so)
    ELSEIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" OR
            CMAKE_SYSTEM_PROCESSOR STREQUAL "i486" OR
            CMAKE_SYSTEM_PROCESSOR STREQUAL "i586" OR
            CMAKE_SYSTEM_PROCESSOR STREQUAL "i686" OR
            CMAKE_SYSTEM_PROCESSOR STREQUAL "x86" OR BUILD_32BIT)
        IF(EXISTS ${OPENCL_DIST_DIR}/lib/x86/libOpenCL.so)
            SET(OPENCL_LIBRARY_DIRS "/opt/AMDAPP/lib/x86")
        ENDIF(EXISTS ${OPENCL_DIST_DIR}/lib/x86/libOpenCL.so)
    ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND NOT BUILD_32BIT)
ENDIF(HAVE_OPENCL)

###
# end of OpenCL config
###

IF(HAVE_OPENCL)
    MESSAGE(STATUS "OpenCL found")
    MESSAGE(STATUS "OpenCL library dirs: " ${OPENCL_LIBRARY_DIRS})
    MESSAGE(STATUS "OpenCL libraries: " ${OPENCL_LIBRARIES})
    MESSAGE(STATUS "OpenCL include dirs: " ${OPENCL_INCLUDE_DIRS})
ELSE(HAVE_OPENCL)
    MESSAGE(WARNING "OpenCL not found")
ENDIF(HAVE_OPENCL)

SET(CMAKE_THREAD_PREFER_PTHREAD 1)
FIND_PACKAGE(Threads)

IF(BUILD_32BIT)
    MESSAGE(STATUS "This is 32-bit")
    TRY_COMPILE(HAVE_INT128 ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/Int128Detect.c
        COMPILE_DEFINITIONS "-m32")
ELSE(BUILD_32BIT)
    TRY_COMPILE(HAVE_INT128 ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/Int128Detect.c)
ENDIF(BUILD_32BIT)

IF(HAVE_INT128)
    MESSAGE(STATUS "128-bit integer arithmetic detected")
ELSE(HAVE_INT128)
    MESSAGE(STATUS "128-bit integer arithmetic not supported by compiler")
ENDIF(HAVE_INT128)

###
# other dependencies and checks
###

CHECK_INCLUDE_FILE_CXX(elf.h HAVE_ELF_H)
FIND_PACKAGE(PNG)

IF(PNG_FOUND)
    MESSAGE(STATUS "Png found ${PNG_INCLUDE_DIRS}")
ENDIF(PNG_FOUND)

###
# end of other dependencies and checks
###

SET(MATHLIB "m")

CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/CLRX/Config.h.in
        ${PROJECT_BINARY_DIR}/CLRX/Config.h)

OPTION(BUILD_TESTS "Compile tests" OFF)
OPTION(BUILD_SAMPLES "Compile samples" OFF)

IF(BUILD_TESTS)
    ENABLE_TESTING()
ENDIF(BUILD_TESTS)

INCLUDE_DIRECTORIES(${OPENCL_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})

LINK_DIRECTORIES(${OPENCL_LIBRARY_DIRS})

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/CLRX)
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/utils)
IF(HAVE_OPENCL)
    ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/clwrapper)
ENDIF(HAVE_OPENCL)
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/amdbin)
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/amdasm)
IF (BUILD_TESTS)
    ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/tests)
ENDIF(BUILD_TESTS)

ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/programs)
IF (BUILD_SAMPLES)
    ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/samples)
ENDIF(BUILD_SAMPLES)
