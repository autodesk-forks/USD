set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE execUsd)

pxr_library(execUsd
    LIBRARIES
        esf
        usd
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        sceneAdapter

    PRIVATE_CLASSES
        attribute
        object
        prim
        property
        stage

    RESOURCE_FILES
        plugInfo.json
)

pxr_build_test(testExecUsdBasicCompilation
    LIBRARIES
        gf
        tf
        esf
        exec
        execUsd
        vdf
        usdGeom
    CPPFILES
        testenv/testExecUsdBasicCompilation.cpp
)

pxr_register_test(testExecUsdBasicCompilation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdBasicCompilation"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdExternalRegistration
    LIBRARIES
        exec
        tf
        vdf
    CPPFILES
        testenv/testExecUsdExternalRegistration.cpp
)

pxr_register_test(testExecUsdExternalRegistration
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdExternalRegistration"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdSceneObjects
    LIBRARIES
        esf
        execUsd
        usdGeom
    CPPFILES
        testenv/testExecUsdSceneObjects.cpp
)

pxr_register_test(testExecUsdSceneObjects
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdSceneObjects"
    EXPECTED_RETURN_CODE 0
)
