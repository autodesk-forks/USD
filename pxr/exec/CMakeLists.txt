set(DIRS
    vdf
    ef
    esf
    exec
    execUsd
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()