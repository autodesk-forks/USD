//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/outputProvidingCompilationTask.h"

#include "pxr/exec/exec/callbackNode.h"
#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/compiledOutputCache.h"
#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/inputResolvingCompilationTask.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/tokens.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

static const Exec_ComputationDefinition *
_GetAttributeComputationDefinition(
    TfType schemaType,
    const TfToken &attributeName,
    const TfToken &computationName)
{
    // TODO: attribute computation definitions are not yet implemented
    return nullptr;
}

void
Exec_OutputProvidingCompilationTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskStages &taskStages)
{
    TRACE_FUNCTION();

    // TODO: Journaling
    EsfJournal *journal = nullptr;

    const EsfStage &stage = compilationState.GetStage();

    const SdfPath &providerObjectPath =
        _outputKey.GetValueKey().GetProviderPath();
    const EsfObject providerObject =
        stage->GetObjectAtPath(providerObjectPath, journal);

    const EsfPrim prim = providerObject->GetPrim(journal);
    const TfType primSchemaType = prim->GetType(journal);

    const EsfAttribute attribute =
        stage->GetAttributeAtPath(providerObjectPath, journal);

    const TfToken &computationName =
        _outputKey.GetValueKey().GetComputationToken();

    const Exec_DefinitionRegistry &registry =
        Exec_DefinitionRegistry::GetInstance();
    const Exec_ComputationDefinition *const definition =
        attribute->IsValid(journal)
        ? _GetAttributeComputationDefinition(
            primSchemaType,
            attribute->GetName(journal),
            computationName)
        : registry.GetPrimComputationDefinition(
            primSchemaType,
            computationName);

    // TODO: Lookup builtin computation definition

    // We were unable to find a registered or builtin computation for this
    // output key. Mark the task done, but it won't have a compiled output
    // key.
    if (!definition) {
        _MarkDone(_outputKey);
        return;
    }

    const Exec_InputKeyVector &inputKeys = definition->GetInputKeys();

    taskStages.Invoke(
    // Make sure input dependencies are fulfilled
    [this, &compilationState, &providerObjectPath, &inputKeys](
        TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("input tasks");

        _inputSources.resize(inputKeys.size());
        _SourceOutputs *sourceOutputs = _inputSources.data();
        for (const Exec_InputKey &inputKey : inputKeys) {
            deps.NewSubtask<Exec_InputResolvingCompilationTask>(
                compilationState,
                inputKey,
                providerObjectPath,
                sourceOutputs++);
        }
    },

    // Compile and connect the callback node
    [this, &compilationState, definition, &inputKeys](TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("node creation");

        VdfInputSpecs inputSpecs;
        inputSpecs.Reserve(inputKeys.size());
        for (const Exec_InputKey &inputKey : inputKeys) {
            inputSpecs.ReadConnector(inputKey.resultType, inputKey.inputName);
        }

        VdfOutputSpecs outputSpecs;
        outputSpecs.Connector(definition->GetResultType(), VdfTokens->out);

        // TODO: Journaling
        EsfJournal nodeJournal;

        VdfNode *const callbackNode =
            compilationState.GetProgram()->CreateNode<Exec_CallbackNode>(
                nodeJournal,
                inputSpecs,
                outputSpecs,
                definition->GetCallback());

        callbackNode->SetDebugNameCallback([outputKey = _outputKey]{
            return outputKey.GetDebugName();
        });

        for (size_t i = 0; i < _inputSources.size(); ++i) {
            // TODO: Journaling
            EsfJournal inputJournal;

            compilationState.GetProgram()->Connect(
                inputJournal,
                _inputSources[i],
                callbackNode,
                inputKeys[i].inputName);
        }

        // Return the compiled output to the calling task.
        VdfMaskedOutput compiledOutput(
            callbackNode->GetOutput(), VdfMask::AllOnes(1));
        *_resultOutput = compiledOutput;

        // Then publish it to the compiled outputs cache.
        Exec_CompiledOutputCache *compiledOutputs =
            compilationState.GetProgram()->GetCompiledOutputCache();
        compiledOutputs->Insert(_outputKey, compiledOutput);

        // Then indicate that the task identified by _outputKey is done. This
        // notifies all other tasks with a dependency on this _outputKey.
        _MarkDone(_outputKey);
    }
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
