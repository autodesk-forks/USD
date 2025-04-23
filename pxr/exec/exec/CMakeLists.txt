set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE exec)

pxr_library(exec
    LIBRARIES
        ef
        esf
        tf
        trace
        sdf
        usd
        vdf
        vt
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
        
    PUBLIC_HEADERS
        api.h
        providerResolution.h
        registerSchema.h
        types.h

    PUBLIC_CLASSES
        computationBuilders
        request
        system
        tokens
        typeRegistry
        valueKey

    PRIVATE_HEADERS
        compilationState.h
        inputKey.h

    PRIVATE_CLASSES
        callbackNode
        compilationTask
        compiledOutputCache
        compiler
        compilerTaskSync
        computationDefinition
        definitionRegistry
        inputResolvingCompilationTask
        leafCompilationTask
        outputKey
        outputProvidingCompilationTask
        program
        requestImpl
        uncompilationRuleSet
        uncompilationTable

    RESOURCE_FILES
        plugInfo.json
)

pxr_build_test(testExecComputationRegistration
    LIBRARIES
        exec
    CPPFILES
        testenv/testExecComputationRegistration.cpp
)

pxr_register_test(testExecComputationRegistration
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecComputationRegistration"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUncompilationTable
    LIBRARIES
        exec
        work
    CPPFILES
        testenv/testExecUncompilationTable.cpp
)

pxr_register_test(testExecUncompilationTable
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUncompilationTable"
    EXPECTED_RETURN_CODE 0
)
