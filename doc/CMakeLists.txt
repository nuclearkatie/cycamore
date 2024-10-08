# Build file for CMake in order to generate the documentation.

# ---------------------------------
# Check for Doxygen
# ---------------------------------

IF(DOXYGEN_FOUND)
    SET(DOXYGEN_SOURCE_DIR ${CYCAMORE_SOURCE_DIR})
    SET(DOXYGEN_INPUT ${DOC_INPUT_DIR}/doxygen.conf)
    SET(DOXYGEN_OUTPUT ${DOC_OUTPUT_DIR}/html/index.html)
    SET(PROJECT_LOGO ${CYCLUS_CORE_SHARE_DIR}/doc/logo2_bike.gif)
    SET(DOXYGEN_STYLESHEET_FILE ${CYCLUS_CORE_SHARE_DIR}/doc/doxygen-awesome.css)

    FIND_PACKAGE(HTMLHelp)

    IF(HTML_HELP_COMPILER)
        SET(DOXYGEN_HTMLHELP YES)
    ELSE(HTML_HELP_COMPILER)
        SET(DOXYGEN_HTMLHELP NO)
    ENDIF(HTML_HELP_COMPILER)

    # ----------------------------------------------------------------------
    # Some doxyfile settings to be configured into the template doxy.conf.in
    # ----------------------------------------------------------------------
    SET(DOXYGEN_LATEX NO)
    SET(DOXYGEN_PDFLATEX NO)
    SET(DOXYGEN_PDF_HYPERLINKS NO)
    SET(DOXYGEN_EXTRACT_PRIVATE YES)
    SET(DOXYGEN_EXTRACT_STATIC YES)
    SET(DOXYGEN_EXTRACT_LOCAL YES)
    SET(DOXYGEN_EXTRACT_INTERNAL YES)

    IF(DOT)
        SET(HAVE_DOT YES)
    ELSE(DOT)
        SET(HAVE_DOT NO)
    ENDIF(DOT)

    SET(DOXYGEN_LANGUAGE "English" CACHE STRING "Language used by doxygen")
    MARK_AS_ADVANCED(DOXYGEN_LANGUAGE)

    STRING(REGEX REPLACE ";" " " DOXYGEN_INPUT_LIST "${DOXYGEN_SOURCE_DIR}")

    CONFIGURE_FILE(${DOC_INPUT_DIR}/doxy.conf.in ${DOC_INPUT_DIR}/doxygen.conf)


    IF(WIN32)
        SET(DOXYGEN_GENERATE_MAN NO)
        FIND_PROGRAM(HHC_PROGRAM
            NAMES hhc.exe
            PATHS "c:/Program Files/HTML Help Workshop"
            DOC "HTML Help Compiler program")
    ELSE(WIN32)
        SET(DOXYGEN_GENERATE_MAN YES)
        SET(HHC_PROGRAM)
    ENDIF(WIN32)

    ADD_CUSTOM_COMMAND(
        OUTPUT ${DOXYGEN_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E echo_append "Building cycamore Documentation..."
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
        COMMAND ${CMAKE_COMMAND} -E echo "Done."
        WORKING_DIRECTORY ${DOC_OUTPUT_DIR}
        DEPENDS ${DOXYGEN_INPUT}
        )


    ADD_CUSTOM_TARGET(cycamoredoc DEPENDS ${DOXYGEN_OUTPUT})

    ADD_CUSTOM_TARGET(cycamoredoc_forced
        COMMAND ${CMAKE_COMMAND} -E echo_append "Building cycamore Documentation..."
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
        COMMAND ${CMAKE_COMMAND} -E echo "Done."
        WORKING_DIRECTORY ${DOC_OUTPUT_DIR})

ENDIF(DOXYGEN_FOUND)
