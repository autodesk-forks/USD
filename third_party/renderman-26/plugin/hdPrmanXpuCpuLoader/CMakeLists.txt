set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrmanXpuCpuLoader)

pxr_plugin(hdPrmanXpuCpuLoader
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

if(TARGET hdPrmanXpuCpuLoader)
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${PROJECT_BINARY_DIR}/include/pxr/imaging"
    )
endif()