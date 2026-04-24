set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE execIr)

pxr_library(execIr
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        gf
        tf
        execUsd
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        controllerBuilder
        tokens
        types

    PRIVATE_HEADERS

    PRIVATE_CLASSES
        utils

    CPPFILES
        fkController.cpp

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        README.md
)

#
# Tests
#

pxr_build_test(testExecIrController
    LIBRARIES
        exec
        execIr
        execUsd
        plug
        sdf
        tf
        usd
        vdf
    CPPFILES
        testenv/testExecIrController.cpp
)

pxr_register_test(testExecIrController
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrController"
    TESTENV testenv/testExecIrController
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecIrFkController
    LIBRARIES
        exec
        execIr
        execUsd
        gf
        sdf
        tf
        usd
    CPPFILES
        testenv/testExecIrFkController.cpp
)

pxr_register_test(testExecIrFkController
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecIrFkController"
    EXPECTED_RETURN_CODE 0
)
