set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hio)

set(stbImage "stbImage.cpp")
if (EMSCRIPTEN AND PXR_WASM64)
    message(STATUS "Skipping stbImage.cpp because it is not supported on Emscripten 64")
    set(stbImage "")
endif ()

pxr_library(hio
    LIBRARIES
        arch
        js
        plug
        tf
        vt
        trace
        ar
        hf

    PUBLIC_CLASSES
        fieldTextureData
        glslfx
        glslfxConfig
        glslfxResourceLayout
        image
        imageRegistry
        types

    PRIVATE_CLASSES
        debugCodes
        dictionary

    PUBLIC_HEADERS
        api.h

    PRIVATE_HEADERS
        rankedTypeMap.h
        stb/stb_image.h
        stb/stb_image_resize2.h
        stb/stb_image_write.h

    CPPFILES
        ${stbImage}

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

set(TARGET_NAME "hio")
if(EMSCRIPTEN)
    set(TARGET_NAME "hio_internal")
endif()

target_compile_definitions("${TARGET_NAME}" PRIVATE
    ILMTHREAD_THREADING_ENABLED
    OPENEXR_C_STANDALONE
    OPENEXR_EXPORT=static)

pxr_build_test(testHioImage
    LIBRARIES
        ar
        hio
        plug
        tf

   CPPFILES
        testenv/testHioImage.cpp
)

# Tests that use plugins are not supported in static builds
if (BUILD_SHARED_LIBS)
    pxr_register_test(testHioImage
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHioImage"
        EXPECTED_RETURN_CODE 0
    )
endif()

