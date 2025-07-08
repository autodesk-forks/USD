//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/valueKey.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
ExecValueKey::GetDebugName() const
{
    std::string debugName = _provider.GetAsString();
    debugName += " [";
    debugName += _computation.GetString();
    debugName += ']';
    return debugName;
}

PXR_NAMESPACE_CLOSE_SCOPE
