//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(PRESTO_MULTITHREADED_MUNGING, true,
    "Enables single-frame parallel evaluation.");

bool
VdfIsParallelEvaluationEnabled() 
{
    return TfGetEnvSetting(PRESTO_MULTITHREADED_MUNGING);
}

PXR_NAMESPACE_CLOSE_SCOPE
