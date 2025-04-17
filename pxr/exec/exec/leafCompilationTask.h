//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_LEAF_COMPILATION_TASK_H
#define PXR_EXEC_EXEC_LEAF_COMPILATION_TASK_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/compilationTask.h"
#include "pxr/exec/exec/outputKey.h"

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationState;
class VdfMaskedOutput;

/// Leaf compilation task for compiling requested outputs.
///
/// This is the main entry point into the compilation task graph for outputs
/// that have been requested via an ExecRequest and therefore need leaf nodes
/// compiled and connected to them.
class Exec_LeafCompilationTask : public Exec_CompilationTask
{
public:
    Exec_LeafCompilationTask(
        Exec_CompilationState &compilationState,
        const Exec_OutputKey &outputKey,
        VdfMaskedOutput *resultOutput) :
        Exec_CompilationTask(compilationState),
        _outputKey(outputKey),
        _resultOutput(resultOutput)
    {}

private:
    void _Compile(
        Exec_CompilationState &compilationState,
        TaskStages &taskStages) override;

    // The output key for the requested output.
    const Exec_OutputKey _outputKey;

    // Pointer to the result to be populated by this task.
    VdfMaskedOutput *const _resultOutput;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif