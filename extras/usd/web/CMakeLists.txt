set(PXR_PREFIX pxr/usdweb)
set(PXR_PACKAGE usdweb)

pxr_cpp_bin(${PXR_PACKAGE}
    LIBRARIES
        usdImagingGL
        hdStorm
        usdShaders
        usdUtils
        # Trial libs that may be needed for MaterialX
        hdSt         # not sure if this is needed here for MaterialX
        usdMtlx      # not sure if needed
        usdImaging
        sdrGlslfx
        usdShaders


    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
)

#set(MATERIALX_RESOURCES "item1;item2;item3")
#file(GLOB MATERIALX_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/*")
#file(GLOB_RECURSE MATERIALX_RESOURCES "${CMAKE_INSTALL_PREFIX}/libraries/*")
#list(TRANSFORM MATERIALX_RESOURCES REPLACE "${CMAKE_INSTALL_PREFIX}/" "")
#message(AUTHOR_WARNING "MATERIALX_RESOURCES: \"${MATERIALX_RESOURCES}\"")
#message(FATAL_ERROR "EXITING FOR TESTING...")


if (EMSCRIPTEN)
    list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/usd/plugInfo.json")
    list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/plugin/usd/plugInfo.json")

    # TODO: Add in MaterialX to the RESOURCE_PARAMETERS for target_link_options or target_link_libraries

    # XXX: We manually set the libraries instead of using the LIBRARIES argument pxr_cpp_bin due to
    # problems with the tbb library.
    target_link_libraries(${PXR_PACKAGE}
            ${RESOURCE_PARAMETERS}
            ${TBB_tbb_LIBRARY}
    )
    set(BUILD_FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.js
            ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.wasm
            ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.data
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/wasm-server.py
            ${CMAKE_CURRENT_SOURCE_DIR}/${PXR_PACKAGE}.html
        )

    set(CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB " --tracing ${DEBUG_OPTIONS} \
        -s ENVIRONMENT=web,worker \
        -pthread \
        -s MAXIMUM_MEMORY=4GB \
        -s PTHREAD_POOL_SIZE=8 \
        -s WASM_BIGINT \
        -s NO_EXIT_RUNTIME=1 \
        -s SHARED_MEMORY=1 \
    ")
    #-s STACK_SIZE=786432
    if(PXR_WASM_NODE)
        target_link_options(${PXR_PACKAGE} PRIVATE "SHELL:-s NODERAWFS=0")
    endif ()
    if (PXR_ENABLE_JS_BINDINGS_SUPPORT)
        set(CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB "${CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB} --bind")
    endif()
    target_link_options(${PXR_PACKAGE} PRIVATE "SHELL:${CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB} \
        -s USE_GLFW=3 \
        -s MAX_WEBGL_VERSION=2 \
        -s USE_WEBGPU=1 \
        -O0 \
        -s ALLOW_MEMORY_GROWTH \
        -s FULL_ES3 \
    ")
    # See if this is useful: -sFULL_ES3

    target_compile_options(${PXR_PACKAGE} PRIVATE "SHELL:${CMAKE_EXE_LINKER_FLAGS_USDVIEWWEB} -sMODULARIZE=0 -O0")
    target_link_options(${PXR_PACKAGE} PRIVATE "SHELL: \
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources/webgpu-env-settings@/webgpu-env-settings \
        --embed-file ${CMAKE_INSTALL_PREFIX}/libraries@/libraries/ \
        -mbulk-memory \
        -matomics \
        --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/resources/preload.js \ 
        --profiling-funcs \
        ")

    if(CMAKE_BUILD_TYPE MATCHES "(Debug|DEBUG|debug|RelWithDebInfo|RELWITHDEBINFO|relwithdebinfo)")
        message("Building with debug information")
        # See https://developer.chrome.com/blog/faster-wasm-debugging#debug_fission
        # Note: -g is set automatically so we're just adding some additional flags
        target_link_options(${PXR_PACKAGE} PRIVATE "SHELL: -gseparate-dwarf -s ERROR_ON_WASM_CHANGES_AFTER_LINK")
        target_compile_options(${PXR_PACKAGE} PRIVATE "SHELL: -gdwarf-5 -gsplit-dwarf -gpubnames")
        add_custom_command(
                TARGET ${PXR_PACKAGE}
                POST_BUILD
                COMMAND emstrip --no-strip-all --remove-section=name ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.wasm && emdwp -e ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.wasm.debug.wasm -o ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.wasm.debug.wasm.dwp
                COMMENT "Currently reduces the debug build from 332M to 85M while keeping debug information in separate files"
        )
        list(APPEND
                BUILD_FILES
                ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.wasm.debug.wasm
        )
    endif()

    install(
        FILES
        ${BUILD_FILES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    )
endif()

target_sources(${PXR_PACKAGE} PRIVATE
        freeCameraGL.cpp
)