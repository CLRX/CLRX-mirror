#
# Copyright (C) 2013    Emmanuel Roullit <emmanuel.roullit@gmail.com>
# Modified: Mateusz Szpakowski
#

# Generate man pages of the project by using the
# POD header written in the tool source code.
# To use it, include this file in CMakeLists.txt and
# invoke POD2MAN(<podfile> <manfile> <section>)

# original sources: https://github.com/markusa/netsniff-ng_filter/blob/master/src/cmake/modules/Pod2Man.cmake

MACRO(POD2MAN PODFILE MANFILE SECTION)
    FIND_PROGRAM(POD2MAN pod2man)
    FIND_PROGRAM(GZIP gzip)

    IF(POD2MAN AND GZIP)
        IF(NOT EXISTS "${PODFILE}")
            MESSAGE(FATAL_ERROR "Could not find pod file ${PODFILE} to generate man page")
        ENDIF(NOT EXISTS "${PODFILE}")

        ADD_CUSTOM_COMMAND(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}"
            DEPENDS "${PODFILE}"
            COMMAND "${POD2MAN}"
            ARGS --section ${SECTION} --center ${CMAKE_PROJECT_NAME} --release --stderr --name ${MANFILE}
            "${PODFILE}" > "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}"
        )

        ADD_CUSTOM_COMMAND(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}.gz"
            COMMAND "${GZIP}" --best -c "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}" > "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}.gz"
            DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}"
        )

        SET(MANPAGE_TARGET "man-${MANFILE}")

        ADD_CUSTOM_TARGET("${MANPAGE_TARGET}" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}.gz")
        ADD_DEPENDENCIES(man "${MANPAGE_TARGET}")

        INSTALL(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/${MANFILE}.${SECTION}.gz"
            DESTINATION share/man/man${SECTION}
            )
    ELSE(POD2MAN AND GZIP)
        MESSAGE(WARNING "No Pod2Man or gzip. Unix manuals will not be generated")
    ENDIF(POD2MAN AND GZIP)
ENDMACRO(POD2MAN PODFILE MANFILE SECTION)

MACRO(ADD_MANPAGE_TARGET)
    # It is not possible add a dependency to target 'install'
    # Run hard-coded 'make man' when 'make install' is invoked
    INSTALL(CODE "EXECUTE_PROCESS(COMMAND make man)")
    ADD_CUSTOM_TARGET(man ALL)
ENDMACRO(ADD_MANPAGE_TARGET)
