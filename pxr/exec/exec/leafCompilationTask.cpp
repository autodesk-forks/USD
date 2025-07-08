//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/leafCompilationTask.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/compiledOutputCache.h"
#include "pxr/exec/exec/compilerTaskSync.h"
#include "pxr/exec/exec/outputProvidingCompilationTask.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/trace/trace.h"
#include "pxr/exec/ef/leafNode.h"
#include "pxr/exec/vdf/maskedOutput.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_LeafCompilationTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskStages &taskStages)
{
    TRACE_FUNCTION();

    // Look up the cached output.
    const auto &[output, hasOutput] =
        compilationState.GetProgram()->GetCompiledOutputCache()->Find(
            _outputKey);

    taskStages.Invoke(
    // Determine if the output dependency has been compiled, and if not
    // run the tasks to produce the output.
    [this, &compilationState, hasOutput=hasOutput]
    (TaskDependencies &deps) {
        // Nothing to do here, if we already have a cached output.
        if (hasOutput) {
            return;
        }

        TRACE_FUNCTION_SCOPE("leaf output");

        // Claim the task for producing the missing output.
        const Exec_CompilerTaskSync::ClaimResult claimResult =
            deps.ClaimSubtask(_outputKey);
        if (claimResult == Exec_CompilerTaskSync::ClaimResult::Claimed) {
            // Run the task that produces the output.
            deps.NewSubtask<Exec_OutputProvidingCompilationTask>(
                compilationState,
                _outputKey,
                _resultOutput);
        }
    },

    // Compile and register the leaf node.
    [this, &compilationState, output=output, hasOutput=hasOutput]
    (TaskDependencies &deps) {
        if (!TF_VERIFY(hasOutput)) {
            return;
        }

        TRACE_FUNCTION_SCOPE("leaf node creation");

        // TODO: Journaling
        EsfJournal nodeJournal;
        EsfJournal inputJournal;

        EfLeafNode *const leafNode =
            compilationState.GetProgram()->CreateNode<EfLeafNode>(
                nodeJournal,
                output.GetOutput()->GetSpec().GetType());

        leafNode->SetDebugNameCallback([outputKey = _outputKey]{
            return outputKey.GetDebugName();
        });

        compilationState.GetProgram()->Connect(
            inputJournal,
            Exec_Program::SourceOutputs{output},
            leafNode,
            EfLeafTokens->in);
    }
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
