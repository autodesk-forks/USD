//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/inputResolvingCompilationTask.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/compiledOutputCache.h"
#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/outputProvidingCompilationTask.h"
#include "pxr/exec/exec/program.h"

#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_OutputKeyVector
static _GenerateOutputKeys(
    const EsfStage &stage,
    const SdfPath &originObjectPath,
    const Exec_InputKey &inputKey);

Exec_OutputKeyVector
static _GenerateLocalOutputKeys(
    const EsfStage &stage,
    const SdfPath &originObjectPath,
    const Exec_InputKey &inputKey);

Exec_OutputKeyVector
static _GenerateNamespaceAncestorOutputKeys(
    const EsfStage &stage,
    const SdfPath &originObjectPath,
    const Exec_InputKey &inputKey);

void
Exec_InputResolvingCompilationTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskStages &taskStages)
{
    TRACE_FUNCTION();

    taskStages.Invoke(
    // Generate the output key (or multiple output keys) to compile from the
    // input key, and create new subtasks for any outputs that still need to be
    // compiled.
    [this, &compilationState](TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("compile sources");

        // Generate the output keys for all the inputs.
        _outputKeys = _GenerateOutputKeys(
            compilationState.GetStage(), _originObject, _inputKey);
        _resultOutputs->resize(_outputKeys.size());

        // For every output key, make sure it's either already available or
        // a task has been kicked off to produce it.
        for (size_t i = 0; i < _outputKeys.size(); ++i) {
            const Exec_OutputKey &outputKey = _outputKeys[i];
            VdfMaskedOutput *const resultOutput = &(*_resultOutputs)[i];

            const auto &[output, hasOutput] =
                compilationState.GetProgram()->GetCompiledOutputCache()->Find(
                    outputKey);
            if (hasOutput) {
                *resultOutput = output;
                continue;
            }

            // Claim the task for producing the missing output.
            const Exec_CompilerTaskSync::ClaimResult claimResult =
                deps.ClaimSubtask(outputKey);
            if (claimResult == Exec_CompilerTaskSync::ClaimResult::Claimed) {
                // Run the task that produces the output.
                deps.NewSubtask<Exec_OutputProvidingCompilationTask>(
                    compilationState,
                    outputKey,
                    resultOutput);
            }
        }
    },

    // Compiled outputs are now all available and can be retrieved from the
    // compiled outputs cache.
    [this, &compilationState](TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("populate result");

        // For every output key, check if we still don't have a result and if
        // so retrieve it from the compiled output. All the task dependencies
        // should have fulfilled at this point.
        for (size_t i = 0; i < _outputKeys.size(); ++i) {
            const Exec_OutputKey &outputKey = _outputKeys[i];
            VdfMaskedOutput *const resultOutput = &(*_resultOutputs)[i];

            if (*resultOutput) {
                continue;
            }

            const auto &[output, hasOutput] =
                compilationState.GetProgram()->GetCompiledOutputCache()->Find(
                    outputKey);
            if (!output) {
                TF_VERIFY(_inputKey.optional);
                continue;
            }

            *resultOutput = output;
        }
    }
    );
}

Exec_OutputKeyVector
static _GenerateOutputKeys(
    const EsfStage &stage,
    const SdfPath &originObjectPath,
    const Exec_InputKey &inputKey)
{
    TRACE_FUNCTION();

    switch(inputKey.providerResolution.dynamicTraversal) {
        case ExecProviderResolution::DynamicTraversal::Local:
            return _GenerateLocalOutputKeys(
                stage, originObjectPath, inputKey);

        case ExecProviderResolution::DynamicTraversal::NamespaceAncestor:
            return _GenerateNamespaceAncestorOutputKeys(
                stage, originObjectPath, inputKey);
    }

    TF_CODING_ERROR("Unhandled provider resolution mode");
    return {};
}

Exec_OutputKeyVector
static _GenerateLocalOutputKeys(
    const EsfStage &stage,
    const SdfPath &originObjectPath,
    const Exec_InputKey &inputKey)
{
    TRACE_FUNCTION();

    // TODO: Journaling
    EsfJournal *journal = nullptr;

    const SdfPath providerPath =
        inputKey.providerResolution.localTraversal.MakeAbsolutePath(
            originObjectPath);

    const EsfObject providerObject =
        stage->GetObjectAtPath(providerPath, journal);
    if (!providerObject->IsValid(journal)) {
        return {};
    }

    return { 
        Exec_OutputKey(ExecValueKey(
            providerObject->GetPath(journal), inputKey.computationName))
    };
}

Exec_OutputKeyVector
static _GenerateNamespaceAncestorOutputKeys(
    const EsfStage &stage,
    const SdfPath &originObjectPath,
    const Exec_InputKey &inputKey)
{
    TRACE_FUNCTION();

    // TODO: Journaling
    EsfJournal *journal = nullptr;

    const Exec_DefinitionRegistry &registry =
        Exec_DefinitionRegistry::GetInstance();

    const EsfObject originObject =
        stage->GetObjectAtPath(originObjectPath, journal);
    const EsfPrim originPrim = originObject->GetPrim(journal);
    EsfPrim parentPrim = originPrim->GetParent(journal);

    while (parentPrim->IsValid(journal)) {
        const Exec_ComputationDefinition *const definition =
            registry.GetPrimComputationDefinition(
                parentPrim->GetType(journal),
                inputKey.computationName);
        if (definition && definition->GetResultType() == inputKey.resultType) {
            return {
                Exec_OutputKey(ExecValueKey(
                    parentPrim->GetPath(journal),
                    inputKey.computationName))
            };
        }

        // TODO: Lookup builtin computation definitions

        parentPrim = parentPrim->GetParent(journal);
    }

    // We were unable to find the computation on any of the ancestors.
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
