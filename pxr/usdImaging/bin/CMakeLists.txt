set(DIRS
    usdrecord
    usdview
    testusdview
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

if (${PXR_ENABLE_MATERIALX_SUPPORT})
    add_subdirectory(usdBakeMtlx)
endif()

if (${PXR_BUILD_JS_USDVIEWWEB})
    add_subdirectory(usdviewweb)
endif()
