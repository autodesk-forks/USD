add_executable(wasmFetchResolver
    fetchResolver.h fetchResolver.cpp
    main.cpp
)

set_target_properties(wasmFetchResolver PROPERTIES SUFFIX ".html")

target_link_options(wasmFetchResolver PRIVATE "SHELL:-sFETCH=1")
target_link_options(wasmFetchResolver PRIVATE "SHELL:-sPROXY_TO_PTHREAD=1")

# This specific version of target_link_libraries is used here because it
# ensures that whole-archive linking options are added to the linker line
# for the target. This is essential for a wasm build due to the fact that
# the compiler will aggressively strip what it deems to be dead code. This
# results in registry functions not being run.
# XXX: Would be nice to break this functionality out into a reusable public
# utility function.
_pxr_target_link_libraries(wasmFetchResolver usd ar tf arch)

target_include_directories(wasmFetchResolver PRIVATE ${CMAKE_BINARY_DIR}/include)
target_compile_options(wasmFetchResolver PRIVATE -fPIC)

# Installs the bundled express based web server. The threading model we
# use requires the addition of certain required response headers.
install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/package.json
        ${CMAKE_CURRENT_SOURCE_DIR}/server.js
    DESTINATION 
        share/usd/examples/bin/wasmFetchResolver
)

# Installs sample models
# You may place additional models in the public folder.
install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/stages
    DESTINATION
        share/usd/examples/bin/wasmFetchResolver/public)

# Installs the compiled example into the public directory which
# the bundled webserver is set to serve static files from.
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/wasmFetchResolver.html
        ${CMAKE_CURRENT_BINARY_DIR}/wasmFetchResolver.js
        ${CMAKE_CURRENT_BINARY_DIR}/wasmFetchResolver.wasm
    DESTINATION
        share/usd/examples/bin/wasmFetchResolver/public)

# Embeds the plugInfo.json file describing the fetch resolver into the
# wasm bundle.  Note that the installation directory is deliberate such
# that the plugin json is placed in plug's search path.
target_link_options(wasmFetchResolver PRIVATE "SHELL:--embed-file ${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json@/usd/wasmFetchResolver/resources/plugInfo.json")
