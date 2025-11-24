set(PXR_PREFIX jsBindings)
set(PXR_PACKAGE jsBindings)

if (NOT EMSCRIPTEN)
  return()
endif()

set(RESOURCE_PARAMETERS "")

list(APPEND RESOURCE_PARAMETERS "--preload-file ${PROJECT_BINARY_DIR}/plugins_plugInfo.json@/usd/plugInfo.json")

if(PXR_WASM_NODE)
  list(TRANSFORM RESOURCE_PARAMETERS REPLACE "-preload" "-embed")
endif()

pxr_cpp_bin(${PXR_PACKAGE}
    LIBRARIES
        usd
        usdLux
        usdGeom
        usdUtils
        sdf
        tf
        ${RESOURCE_PARAMETERS}
)

target_compile_options(${PXR_PACKAGE} PRIVATE "SHELL:-lembind")
target_link_options(${PXR_PACKAGE} PRIVATE "SHELL:-sEXPORT_NAME=getUsdModule -sMODULARIZE=1  -lembind --pre-js ${CMAKE_SOURCE_DIR}/js/prejs.js")
set_target_properties(${PXR_PACKAGE}
    PROPERTIES
        SUFFIX ".js"
)

set(BUILD_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.js
    ${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.wasm
    ${CMAKE_SOURCE_DIR}/js/test.html
)

if(NOT PXR_WASM_NODE)
    list(APPEND BUILD_FILES "${CMAKE_CURRENT_BINARY_DIR}/${PXR_PACKAGE}.data")
endif()

install(
    FILES
      ${BUILD_FILES}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
# Install files for npm package

install(
    FILES
      ${BUILD_FILES}
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../js/bindings/${CMAKE_BUILD_TYPE}
)
