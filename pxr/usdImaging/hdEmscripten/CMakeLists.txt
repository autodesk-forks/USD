set(PXR_PREFIX pxr/hdEmscripten)
set(PXR_PACKAGE hdEmscripten)

if (NOT PXR_ENABLE_JS_SUPPORT)
    return()
endif()

set(BINDINGS_NAME "emHdBindings")

pxr_library(hdEmscripten
    LIBRARIES
      hd
      usd
      usdUtils
      sdf

    PUBLIC_CLASSES
        webRenderDelegate

    PUBLIC_HEADERS
        webRenderDelegate.h
)


target_link_options(hdEmscripten_internal PRIVATE "SHELL: -sEXPORT_NAME=getUsdModule -sMODULARIZE=1 -lembind -sFORCE_FILESYSTEM=1")
target_compile_options(hdEmscripten_internal PRIVATE "SHELL: -lembind")

set(RESOURCE_PARAMETERS "")

list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/usd/plugInfo.json")

set(BUILD_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.js
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.wasm
    ${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.worker.js
    ${CMAKE_CURRENT_SOURCE_DIR}/static/hdEmscripten.html
    ${CMAKE_CURRENT_SOURCE_DIR}/static/simpleShading.usda
    ${CMAKE_CURRENT_SOURCE_DIR}/js/ThreeJsRenderDelegate.js
    ${CMAKE_CURRENT_SOURCE_DIR}/js/OrbitControls.js
    ${CMAKE_CURRENT_SOURCE_DIR}/js/coi-serviceworker.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/environments/helicopter-landing-pad-vis-4K.hdr
)

if(PXR_WASM_NODE)
    list(TRANSFORM RESOURCE_PARAMETERS REPLACE "-preload" "-embed")
else()
    list(APPEND BUILD_FILES "${CMAKE_CURRENT_BINARY_DIR}/${BINDINGS_NAME}.data")
endif()

pxr_cpp_bin(${BINDINGS_NAME}
  LIBRARIES
      hdEmscripten
      usdImaging
      ${RESOURCE_PARAMETERS}
)

set_target_properties(${BINDINGS_NAME}
    PROPERTIES
        SUFFIX ".js"
)
target_link_options(${BINDINGS_NAME} PRIVATE "SHELL:-sEXPORT_NAME=getUsdModule -sMODULARIZE=1 -lembind -sFORCE_FILESYSTEM=1")
target_compile_options(${BINDINGS_NAME} PRIVATE "SHELL: -lembind")

install(
    FILES
    ${BUILD_FILES}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)