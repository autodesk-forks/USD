set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdStorm)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

if(EMSCRIPTEN)
    set(PXR_CMD "pxr_library")
else()
    # At the moment, the emscripten setup ins configured to build only static libraries
    set(PXR_CMD "pxr_plugin")
endif()

cmake_language(CALL ${PXR_CMD} hdStorm
    LIBRARIES
        plug
        tf
        trace
        vt
        work
        hd
        hdSt
        ${OPENSUBDIV_LIBRARIES}

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}

    PUBLIC_CLASSES
        rendererPlugin

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)
