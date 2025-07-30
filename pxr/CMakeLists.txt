pxr_core_prologue()

add_subdirectory(external)
add_subdirectory(base)
add_subdirectory(usd)

if (${PXR_BUILD_EXEC})
    # XXX: OpenExec currently does not support oneTBB.
    if (PXR_FIND_TBB_IN_CONFIG)
        message(STATUS "Skipping OpenExec because it currently does not support oneTBB")
    else()
        add_subdirectory(exec)
    endif()
endif()

if (${PXR_BUILD_USD_VALIDATION})
    add_subdirectory(usdValidation)
endif()

if (${PXR_BUILD_IMAGING})
    add_subdirectory(imaging)
    if (${PXR_BUILD_USD_IMAGING})
        add_subdirectory(usdImaging)
    endif()
endif()

pxr_core_epilogue()

export(PACKAGE pxr)

configure_file(pxrConfig.cmake.in
  "${PROJECT_BINARY_DIR}/pxrConfig.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/pxrConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
install(FILES
  "${CMAKE_CURRENT_LIST_DIR}/../cmake/macros/pxrStaticConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

install(EXPORT pxrTargets DESTINATION "cmake")
