set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiPresent)

if (NOT PXR_BUILD_GPU_SUPPORT)
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

set(optionalLibraries "")
set(optionalPrivateClasses "")
set(optionalPrivateHeaders "")
set(optionalCppFiles "")

if (PXR_ENABLE_GL_SUPPORT AND NOT PXR_APPLE_EMBEDDED)
    list(APPEND optionalLibraries hgiInterop)
    list(APPEND optionalPrivateClasses glInterop)
endif()

if (PXR_ENABLE_METAL_SUPPORT)
    list(APPEND optionalLibraries hgiMetal)
    list(APPEND optionalLibraries "-framework CoreGraphics")
    list(APPEND optionalLibraries "-framework QuartzCore")
    list(APPEND optionalPrivateHeaders metal.h)
    list(APPEND optionalCppFiles metal.mm)
endif()

if (PXR_ENABLE_VULKAN_SUPPORT)
    list(APPEND optionalLibraries hgiVulkan)
    list(APPEND optionalPrivateClasses vulkan)
endif()

pxr_library(hgiPresent
    LIBRARIES
        gf
        tf
        hgi
        vt
        ${optionalLibraries}

    PUBLIC_CLASSES
        debugCodes
        present

    PUBLIC_HEADERS
        api.h
        interopHandle.h
        windowHandle.h

    PRIVATE_CLASSES
        noOp
        ${optionalPrivateClasses}

    PRIVATE_HEADERS
        presentImpl.h
        ${optionalPrivateHeaders}

    CPPFILES
        ${optionalCppFiles}
)
