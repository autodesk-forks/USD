set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiWebGPU)

if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_WEBGPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT or PXR_ENABLE_WEBGPU_SUPPORT is OFF")
    return()
endif()

set(CMAKE_CXX_STANDARD 17)

message(STATUS "DAWN WebGPU support is ON")
set(PXR_ENABLE_PRECOMPILED_HEADERS OFF)
set(PXR_ENABLE_PRECOMPILED_HEADERS_PREV ${PXR_ENABLE_PRECOMPILED_HEADERS})
message("Storing PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
message(STATUS "Set to compiler flag /std:c++17")

set(HGI_WEBGPU_LIBRARIES)
set(LIB_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
if (WIN32)
    set(STATIC_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}) # .lib (static or dynamic library)
    set(SHARED_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}) # .lib (static or dynamic library)
    list(APPEND HGI_WEBGPU_LIBRARIES
        dxguid.lib
        ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_common${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/SPIRV-Tools-opt${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/SPIRV-Tools${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/absl_strings${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/absl_strings_internal${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/absl_str_format_internal${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/absl_int128${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/absl_raw_logging_internal${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/absl_throw_delegate${SHARED_LIB_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/absl_base${SHARED_LIB_SUFFIX}
    )
elseif(APPLE)
    set(STATIC_LIB_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX}) # .a (static library)
    set(SHARED_LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}) # .dylib (dynamic library)
endif()

list(APPEND HGI_WEBGPU_LIBRARIES
    # dawn
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_native${SHARED_LIB_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_platform${SHARED_LIB_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawn_proc${SHARED_LIB_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}webgpu_dawn${SHARED_LIB_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}dawncpp${STATIC_LIB_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint${STATIC_LIB_SUFFIX}
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}tint_diagnostic_utils${STATIC_LIB_SUFFIX}

    # shaderc
    ${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX}shaderc_shared${SHARED_LIB_SUFFIX}
)

pxr_library(hgiWebGPU
    LIBRARIES
        arch
        hgi
        tf
        trace
        hgiGL
        ${HGI_WEBGPU_LIBRARIES}

    PUBLIC_CLASSES
        blitCmds
        buffer
        capabilities
        computeCmds
        computePipeline
        conversions
        graphicsCmds
        graphicsPipeline
        hgi
        resourceBindings
        sampler
        shaderCompiler
        shaderFunction
        shaderGenerator
        shaderProgram
        shaderSection
        stepFunctions
        texture

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        mipmapGenerator
        debugCodes

    RESOURCE_FILES
        plugInfo.json
)

set(CMAKE_CXX_STANDARD 14)
message(STATUS "Recover compiler flag to /std:c++14")

message("Restoring PXR_ENABLE_PRECOMPILED_HEADERS:${PXR_ENABLE_PRECOMPILED_HEADERS_PREV}")
set(PXR_ENABLE_PRECOMPILED_HEADERS ${PXR_ENABLE_PRECOMPILED_HEADERS_PREV})