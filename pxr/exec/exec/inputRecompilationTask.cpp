//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/inputRecompilationTask.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/inputResolvingCompilationTask.h"
#include "pxr/exec/exec/nodeRecompilationInfo.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/trace/trace.h"
#include "pxr/exec/ef/leafNode.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_InputRecompilationTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskPhases &taskPhases)
{
    taskPhases.Invoke(
    [this, &compilationState](TaskDependencies &taskDeps) {
        TRACE_FUNCTION_SCOPE("recompile input");

        // Fetch recompilation info for the input's node.
        const Exec_NodeRecompilationInfo *const nodeRecompilationInfo =
            compilationState.GetProgram()->GetNodeRecompilationInfo(
                &_input->GetNode());
        if (!TF_VERIFY(
            nodeRecompilationInfo,
            "Unable to recompile input '%s' because no recompilation info was "
            "found for the node.",
            _input->GetDebugName().c_str())) {
            return;
        }

        // Fetch recompilation info specific to this input.
        const EsfObject &originObject = nodeRecompilationInfo->GetProvider();
        const Exec_InputKey *const inputKey =
            nodeRecompilationInfo->GetInputKey(*_input);
        if (!TF_VERIFY(
            inputKey,
            "Unable to recompile input '%s' because no input key was found.",
            _input->GetDebugName().c_str())) {
            return;
        }

        // Re-resolve and recompile the input's dependencies.
        taskDeps.NewSubtask<Exec_InputResolvingCompilationTask>(
            compilationState,
            *inputKey,
            originObject,
            &_resultOutputs,
            &_journal);
    },

    [this, &compilationState](TaskDependencies &taskDeps) {
        TRACE_FUNCTION_SCOPE("reconnect input");

        // If the input belonged to a leaf node, then we require exactly one
        // source output.
        if (!TF_VERIFY(
            _resultOutputs.size() == 1 ||
            !EfLeafNode::IsALeafNode(_input->GetNode()),
            "Recompilation of leaf node input '%s' expected exactly 1 output; "
            "got %zu.",
            _input->GetDebugName().c_str(),
            _resultOutputs.size())) {
            return;
        }

        // Connect the recompiled outputs to this input.
        compilationState.GetProgram()->Connect(
            _journal, 
            _resultOutputs, 
            &_input->GetNode(),
            _input->GetName());
    }
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
