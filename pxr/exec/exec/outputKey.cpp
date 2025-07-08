//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/outputKey.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
Exec_OutputKey::GetDebugName() const
{
    return _valueKey.GetDebugName();
}

PXR_NAMESPACE_CLOSE_SCOPE
