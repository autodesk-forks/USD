#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  SET(LIB_POSTFIX ${CMAKE_DEBUG_POSTFIX})
endif()

if(UNIX)
    find_path(OIIO_BASE_DIR
            include/OpenImageIO/oiioversion.h
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "/opt/oiio"
    )
    find_path(OIIO_LIBRARY_DIR
            libOpenImageIO${LIB_POSTFIX}.so
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenImageIO library path"
    )
elseif(WIN32)
    find_path(OIIO_BASE_DIR
            include/OpenImageIO/oiioversion.h
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
    )
    find_path(OIIO_LIBRARY_DIR
            OpenImageIO${LIB_POSTFIX}.lib
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OpenImageIO library path"
    )
endif()

find_path(OIIO_INCLUDE_DIR
        OpenImageIO/oiioversion.h
    HINTS
        "${OIIO_LOCATION}"
        "$ENV{OIIO_LOCATION}"
        "${OIIO_BASE_DIR}"
    PATH_SUFFIXES
        include/
    DOC
        "OpenImageIO headers path"
)

list(APPEND OIIO_INCLUDE_DIRS ${OIIO_INCLUDE_DIR})
foreach(OIIO_LIB
    OpenImageIO
    OpenImageIO_Util
    )

    find_library(OIIO_${OIIO_LIB}_LIBRARY
            ${OIIO_LIB}${LIB_POSTFIX}
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "OIIO's ${OIIO_LIB}${LIB_POSTFIX} library path"
    )

    if(OIIO_${OIIO_LIB}_LIBRARY)
        list(APPEND OIIO_LIBRARIES ${OIIO_${OIIO_LIB}_LIBRARY})
    endif()
endforeach(OIIO_LIB)

foreach(OIIO_BIN
        iconvert
        idiff
        igrep
        iinfo
        iv
        maketx
        oiiotool)

    find_program(OIIO_${OIIO_BIN}_BINARY
            ${OIIO_BIN}
        HINTS
            "${OIIO_LOCATION}"
            "$ENV{OIIO_LOCATION}"
            "${OIIO_BASE_DIR}"
        PATH_SUFFIXES
            bin/
        DOC
            "OIIO's ${OIIO_BIN} binary"
    )
    if(OIIO_${OIIO_BIN}_BINARY)
        list(APPEND OIIO_BINARIES ${OIIO_${OIIO_BIN}_BINARY})
    endif()
endforeach(OIIO_BIN)

if(OIIO_INCLUDE_DIRS AND EXISTS "${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h")
    file(STRINGS ${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h
        MAJOR
        REGEX
        "#define OIIO_VERSION_MAJOR.*$")
    file(STRINGS ${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h
        MINOR
        REGEX
        "#define OIIO_VERSION_MINOR.*$")
    file(STRINGS ${OIIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h
        PATCH
        REGEX
        "#define OIIO_VERSION_PATCH.*$")
    string(REGEX MATCHALL "[0-9]+" MAJOR ${MAJOR})
    string(REGEX MATCHALL "[0-9]+" MINOR ${MINOR})
    string(REGEX MATCHALL "[0-9]+" PATCH ${PATCH})
    set(OIIO_VERSION "${MAJOR}.${MINOR}.${PATCH}")
endif()

# handle the QUIETLY and REQUIRED arguments and set OIIO_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OpenImageIO
    REQUIRED_VARS
        OIIO_LIBRARIES
        OIIO_INCLUDE_DIRS
    VERSION_VAR
        OIIO_VERSION
)
