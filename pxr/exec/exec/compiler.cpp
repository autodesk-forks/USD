//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compiler.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/leafCompilationTask.h"
#include "pxr/exec/exec/outputKey.h"
#include "pxr/exec/exec/valueKey.h"

#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/exec/vdf/maskedOutput.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_Compiler::Exec_Compiler(
    const EsfStage &stage,
    Exec_Program *program) :
    _stage(stage),
    _program(program),
    _rootTask(nullptr),
    _taskGroupContext(
        tbb::task_group_context::isolated,
        tbb::task_group_context::concurrent_wait |
        tbb::task_group_context::default_traits)
{
    _rootTask = new (tbb::task::allocate_root(_taskGroupContext))
        tbb::empty_task();
    _rootTask->set_ref_count(1);
}

Exec_Compiler::~Exec_Compiler()
{
    tbb::task::destroy(*_rootTask);
}

std::vector<VdfMaskedOutput>
Exec_Compiler::Compile(TfSpan<const ExecValueKey> valueKeys)
{
    TRACE_FUNCTION();

    // Note that the returned vector should always have the same size as
    // valueKeys.  Any key that failed to compile should yield a null masked
    // output at the corresponding index in the result.
    std::vector<VdfMaskedOutput> leafOutputs(valueKeys.size());

    // Compiler state shared between all compilation tasks.
    Exec_CompilationState state(_stage, _program);

    // Process requested value keys in parallel and spawn compilation tasks.
    WorkWithScopedParallelism(
        [rootTask = _rootTask, &state, valueKeys, &leafOutputs]() {

        WorkParallelForN(valueKeys.size(),
            [rootTask, &state, valueKeys, &leafOutputs](size_t b, size_t e) {
            for (size_t i = b; i != e; ++i) {
                tbb::task *requestedOutputTask =
                    new (tbb::task::allocate_additional_child_of(*rootTask))
                        Exec_LeafCompilationTask(
                            state,
                            Exec_OutputKey(valueKeys[i]),
                            &leafOutputs[i]);
                tbb::task::spawn(*requestedOutputTask);
            }
        });

        {
            TRACE_FUNCTION_SCOPE("waiting for tasks");
            rootTask->wait_for_all();
        }
    });

    return leafOutputs;
}

PXR_NAMESPACE_CLOSE_SCOPE
