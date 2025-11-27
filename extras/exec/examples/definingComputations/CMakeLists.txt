set(PXR_PACKAGE exec)

pxr_build_test(definingComputations
    LIBRARIES
        exec
        execUsd
        plug
        sdf
        tf
        usd
        vdf
    CPPFILES
        definingComputations.cpp
)

pxr_install_test_dir(
    SRC testenv/testDefiningComputationsExample
    DEST testDefiningComputationsExample
)

pxr_register_test(testDefiningComputationsExample
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/definingComputations"
    EXPECTED_RETURN_CODE 0
)
