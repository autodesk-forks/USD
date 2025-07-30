set(DIRS
    vdf
    ef
    esf
    esfUsd
    exec
    execUsd
    execGeom
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()