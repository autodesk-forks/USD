set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrmanXpuLoader)

pxr_plugin(hdPrmanXpuLoader
    LIBRARIES
        arch
        plug
        tf
        hd
        hdPrmanLoader

    PUBLIC_CLASSES
        rendererPlugin

    RESOURCE_FILES
        plugInfo.json
    
    DISABLE_PRECOMPILED_HEADERS
)

if(TARGET hdPrmanXpuLoader)
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${PROJECT_BINARY_DIR}/include/pxr/imaging"
    )
endif()

