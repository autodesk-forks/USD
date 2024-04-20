set(PXR_PREFIX pxr/usd/plugin)
set(PXR_PACKAGE httpResolver)

pxr_library(${PXR_PACKAGE}
    LIBRARIES
        ar
        arch
    PUBLIC_CLASSES
        resolver
    CPPFILES
        resolver.cpp
    RESOURCE_FILES
        plugInfo.json
)