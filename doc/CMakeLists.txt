####
#  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
#  Copyright (C) 2014-2016 Mateusz Szpakowski
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

IF(BUILD_CLRXDOC)
  FIND_PROGRAM(MARKDOWN_PY markdown_py)
  IF(MARKDOWN_PY)
      SET(CLRXDOCS_SOURCES
          "${PROJECT_SOURCE_DIR}/doc/AmdAbi.md"
          "${PROJECT_SOURCE_DIR}/doc/AmdCl2Abi.md"
          "${PROJECT_SOURCE_DIR}/doc/BinaryFormats.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmAmd.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmAmdCl2.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmGallium.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmRocm.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmInvoke.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmPolicy.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmPseudoOps.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxAsmSyntax.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxDisasm.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxToc.md"
          "${PROJECT_SOURCE_DIR}/doc/ClrxWrapper.md"
          "${PROJECT_SOURCE_DIR}/doc/DocLicense.md"
          "${PROJECT_SOURCE_DIR}/doc/GalliumAbi.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsDs.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsFlat.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsMimg.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsMtbuf.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsMubuf.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsSmem.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsSmrd.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsSop1.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsSop2.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsSopc.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsSopk.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsSopp.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsVintrp.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsVop1.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsVop2.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsVop3.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsVop3p.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnInstrsVopc.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnIsa.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnMemHandling.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnOperands.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnSdwaDpp.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnState.md"
          "${PROJECT_SOURCE_DIR}/doc/GcnTimings.md")
      ADD_CUSTOM_TARGET(CLRXDocs ALL DEPENDS "${PROJECT_BINARY_DIR}/clrxdoc/doc.stamp")
      ADD_CUSTOM_COMMAND(OUTPUT "${PROJECT_BINARY_DIR}/clrxdoc/doc.stamp"
              COMMAND sh "${PROJECT_SOURCE_DIR}/doc/genclrxdoc.sh"
              "${PROJECT_SOURCE_DIR}/doc" "${PROJECT_BINARY_DIR}/clrxdoc"
              COMMAND cmake -E touch "${PROJECT_BINARY_DIR}/clrxdoc/doc.stamp"
              DEPENDS ${CLRXDOCS_SOURCES})
      INSTALL(DIRECTORY "${PROJECT_BINARY_DIR}/clrxdoc/" DESTINATION share/doc/CLRX/clrx
              FILES_MATCHING PATTERN "*.html" PATTERN "*.css")
  ELSE(MARKDOWN_PY)
    MESSAGE(WARNING "CLRX Documentation will not be built, because no markdown_py utility")
  ENDIF(MARKDOWN_PY)
ENDIF(BUILD_CLRXDOC)
