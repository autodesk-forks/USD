//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/computationDefinition.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

Exec_ComputationDefinition::Exec_ComputationDefinition(
    TfType resultType,
    ExecCallbackFn &&callback,
    Exec_InputKeyVector &&inputKeys)
    : _resultType(resultType)
    , _callback(std::move(callback))
    , _inputKeys(std::move(inputKeys))
{        
}

PXR_NAMESPACE_CLOSE_SCOPE
