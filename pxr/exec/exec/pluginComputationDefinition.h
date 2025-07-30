//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_PLUGIN_COMPUTATION_DEFINITION_H
#define PXR_EXEC_EXEC_PLUGIN_COMPUTATION_DEFINITION_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/types.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfJournal;
class EsfObjectInterface;
class Exec_Program;
class VdfNode;

/// A class that defines a plugin computation.
///
/// A plugin computation definition includes the callback that implements the
/// evaluation logic and input keys that indicate how to source the input values
/// that are provided to the callback at evaluation time.
///
class Exec_PluginComputationDefinition final
    : public Exec_ComputationDefinition
{
public:
    /// Creates a definition for a plugin computation.
    ///
    /// The computation's evaluation-time behavior is implemented by \p
    /// callback. The \p inputKeys indicate how to source the computation's
    /// input values.
    ///
    Exec_PluginComputationDefinition(
        TfType resultType,
        const TfToken &computationName,
        ExecCallbackFn &&callback,
        Exec_InputKeyVectorRefPtr &&inputKeys);

    ~Exec_PluginComputationDefinition() override;

    Exec_InputKeyVectorConstRefPtr GetInputKeys(
        const EsfObjectInterface &providerObject,
        EsfJournal *journal) const override;

    VdfNode *CompileNode(
        const EsfObjectInterface &providerObject,
        EsfJournal *nodeJournal,
        Exec_Program *program) const override;

private:
    const ExecCallbackFn _callback;
    const Exec_InputKeyVectorConstRefPtr _inputKeys;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
