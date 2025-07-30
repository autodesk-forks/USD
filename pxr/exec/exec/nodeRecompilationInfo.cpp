//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/nodeRecompilationInfo.h"

#include "pxr/exec/exec/inputKey.h"

#include "pxr/base/tf/token.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/node.h"

PXR_NAMESPACE_OPEN_SCOPE

const Exec_InputKey *
Exec_NodeRecompilationInfo::GetInputKey(const VdfInput &input) const
{
    const Exec_InputKey *const it = std::find_if(
        _inputKeys->Get().begin(),
        _inputKeys->Get().end(),
        [&inputName = input.GetName()](const Exec_InputKey &inputKey) {
            return inputKey.inputName == inputName;
        });
    
    if (!TF_VERIFY(it != _inputKeys->Get().end(),
        "Recompilation could not obtain input key for '%s' on node '%s'",
        input.GetName().GetText(),
        input.GetNode().GetDebugName().c_str())) {
        return nullptr;
    }

    return it;
}

PXR_NAMESPACE_CLOSE_SCOPE
