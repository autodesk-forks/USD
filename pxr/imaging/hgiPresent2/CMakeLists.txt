set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiPresent2)

if(NOT PXR_BUILD_GPU_SUPPORT)
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

set(optionalLibraries "")
set(optionalPublicClasses "")
set(optionalPublicHeaders "")
set(optionalCppFiles "")

if(PXR_ENABLE_GL_SUPPORT AND NOT PXR_APPLE_EMBEDDED AND NOT ANDROID)
    list(APPEND optionalLibraries hgiInterop)
    list(APPEND optionalPublicClasses glInterop)
endif()

if(PXR_ENABLE_METAL_SUPPORT)
    list(APPEND optionalLibraries hgiMetal)
    list(APPEND optionalPublicHeaders metalSurface.h)
    list(APPEND optionalCppFiles metalSurface.mm)
endif()

if(PXR_ENABLE_VULKAN_SUPPORT)
    list(APPEND optionalLibraries hgiVulkan)
    list(APPEND optionalPublicClasses vulkanSurface)
endif()

pxr_library(hgiPresent2
    LIBRARIES
        gf
        tf
        hgi
        vt
        ${optionalLibraries}

    PUBLIC_CLASSES
        aovBlit
        debugCodes
        noOp
        present
        ${optionalPublicClasses}

    PUBLIC_HEADERS
        api.h
        aovSet.h
        presentImpl.h
        ${optionalPublicHeaders}

    PRIVATE_HEADERS
        colorVolume.h

    CPPFILES
        ${optionalCppFiles}
)

if(PXR_HEADLESS_TEST_MODE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

set(optionalTestLibraries "")
set(optionalTestClasses)
set(optionalTestHeaders)
set(optionalTestCppFiles)
if(APPLE AND NOT PXR_APPLE_EMBEDDED)
    list(APPEND optionalTestLibraries "-framework AppKit")
    list(APPEND optionalTestLibraries "-framework QuartzCore")
    list(APPEND optionalTestHeaders testenv/testWindowMetal.h)
    list(APPEND optionalTestCppFiles testenv/testWindowMetal.mm)
    set(metal_window_support TRUE)
else()
    set(metal_window_support FALSE)
endif()
if(X11_FOUND)
    list(APPEND optionalTestLibraries X11::X11)
    list(APPEND optionalTestClasses testenv/testWindowX11)
endif()

pxr_build_test(testHgiPresent2
    LIBRARIES
        usdImagingGL
        hgiPresent2
        ${optionalTestLibraries}

    CLASSES
        testenv/testAovSetWithCapture
        testenv/testWindow
        testenv/testWindowHandle
        ${optionalTestClasses}

    HEADERS
        ${optionalTestHeaders}

    CPPFILES
        testenv/testHgiPresent2.cpp
        ${optionalTestCppFiles}
)
pxr_install_test_dir(
    SRC testenv/testHgiPresent2
    DEST testHgiPresent2
)

function(_register_hgipresent2_test suffix window hgi)
    cmake_parse_arguments(hp "" "" "EXTRA_ARGS;OVERRIDE_BASELINES" ${ARGN})

    if(hgi STREQUAL Metal)
        set(hgi_env HGI_ENABLE_VULKAN=0 HGI_ENABLE_WEBGPU=0)
    elseif(hgi STREQUAL Vulkan)
        set(hgi_env HGI_ENABLE_VULKAN=1 HGI_ENABLE_WEBGPU=0)
    elseif(hgi STREQUAL WebGPU)
        set(hgi_env HGI_ENABLE_VULKAN=0 HGI_ENABLE_WEBGPU=1)
    else()
        message(FATAL_ERROR "Unsupported Hgi for HgiPresent2 tests: ${hgi}")
    endif()

    set(baselines
        testHgiPresent2_frame0_640x480.png
        testHgiPresent2_frame15_640x480.png
        testHgiPresent2_frame30_640x480.png
        testHgiPresent2_frame45_640x480.png)
    foreach(override IN LISTS hp_OVERRIDE_BASELINES)
        string(REGEX MATCH "_frame[0-9]+_" frame_number ${override})
        list(FILTER baselines EXCLUDE REGEX "${frame_number}")
        list(APPEND baselines ${override})
    endforeach()

    string(JOIN " " extra_args ${hp_EXTRA_ARGS})
    pxr_register_test(testHgiPresent2${suffix}_${window}_${hgi}
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiPresent2 -stage testHgiPresent2.usda -capture 0 60 15 -window ${window} ${extra_args}"
        IMAGE_DIFF_COMPARE ${baselines}
        FAIL 0.05
        FAIL_PERCENT 0.5
        WARN 0.01
        WARN_PERCENT 0.5
        EXPECTED_RETURN_CODE 0
        TESTENV testHgiPresent2
        ENV
            ${hgi_env}
    )
endfunction()

function(_register_hgipresent2_tests window hgi)
    _register_hgipresent2_test(Linear ${window} ${hgi})
    _register_hgipresent2_test(sRGB ${window} ${hgi}
        EXTRA_ARGS -colorCorrection)
    _register_hgipresent2_test(VSync ${window} ${hgi}
        EXTRA_ARGS -vsync)
    _register_hgipresent2_test(Resize ${window} ${hgi}
        EXTRA_ARGS -resize 1.5 30
        OVERRIDE_BASELINES
            testHgiPresent2_frame30_960x720.png
            testHgiPresent2_frame45_960x720.png)
endfunction()

if(PXR_ENABLE_METAL_SUPPORT AND metal_window_support)
    _register_hgipresent2_tests(Metal Metal)
endif()
if(PXR_ENABLE_VULKAN_SUPPORT)
    if(X11_FOUND)
        _register_hgipresent2_tests(X11 Vulkan)
    endif()
    if(metal_window_support)
        _register_hgipresent2_tests(Metal Vulkan)
    endif()
endif()
