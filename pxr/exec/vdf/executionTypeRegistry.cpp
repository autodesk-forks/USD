//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executionTypeRegistry.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(VdfExecutionTypeRegistry);

VdfExecutionTypeRegistry &
VdfExecutionTypeRegistry::GetInstance()
{
    return TfSingleton<VdfExecutionTypeRegistry>::GetInstance();
}

VdfExecutionTypeRegistry::VdfExecutionTypeRegistry()
{
    TfSingleton<VdfExecutionTypeRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<VdfExecutionTypeRegistry>();
}

PXR_NAMESPACE_CLOSE_SCOPE
