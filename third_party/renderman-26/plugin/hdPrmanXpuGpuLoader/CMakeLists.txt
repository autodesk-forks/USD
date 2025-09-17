set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrmanXpuGpuLoader)

pxr_plugin(hdPrmanXpuGpuLoader
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

if(TARGET hdPrmanXpuGpuLoader)
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${PROJECT_BINARY_DIR}/include/pxr/imaging"
    )
endif()
