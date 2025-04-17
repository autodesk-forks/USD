//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compilationTask.h"

#include "pxr/exec/exec/compilationState.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_CompilerTaskSync::ClaimResult
Exec_CompilationTask::TaskDependencies::ClaimSubtask(const Exec_OutputKey &key)
{
    const Exec_CompilerTaskSync::ClaimResult result =
        Exec_CompilationState::OutputTasksAccess::_Get(&_compilationState)
            .Claim(key, _successor);
    if (result == Exec_CompilerTaskSync::ClaimResult::Wait) {
        _hasDependencies = true;
    }
    return result;
}

tbb::task *
Exec_CompilationTask::execute()
{
    TaskStages taskStages(this, _compilationState, &_taskStage);
    _Compile(_compilationState, taskStages);
    return nullptr;
}

void
Exec_CompilationTask::_MarkDone(const Exec_OutputKey &key)
{
    Exec_CompilationState::OutputTasksAccess::_Get(&_compilationState)
        .MarkDone(key);
}

PXR_NAMESPACE_CLOSE_SCOPE
