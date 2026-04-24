set(DIRS
    usdImaging
    usdImagingGL
    usdProcImaging
    usdSkelImaging
    usdVolImaging
    usdAppUtils
    usdviewq
    bin
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()
