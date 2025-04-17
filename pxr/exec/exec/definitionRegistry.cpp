//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/definitionRegistry.h"

#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/exec/types.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Exec_DefinitionRegistry);

Exec_DefinitionRegistry::Exec_DefinitionRegistry()
{
    // Ensure the type registry is initialized before the definition registry so
    // that computation registrations will be able to look up value types.
    ExecTypeRegistry::GetInstance();

    // Calling SetInstanceConstructed() makes it possible to call GetInstance()
    // before this constructor has finished.
    //
    // This is neccessary because the following call to SubscribeTo() will
    // _immediately_ invoke all registry functions which will, in turn, most
    // likely call GetInstance().
    TfSingleton<Exec_DefinitionRegistry>::SetInstanceConstructed(*this);

    // Now initialize the registry.
    //
    // We use ExecDefinitionRegistryTag to identify registry functions, rather
    // than definition registry type, so Exec_DefinitionRegistry can remain
    // private.
    TfRegistryManager::GetInstance().SubscribeTo<ExecDefinitionRegistryTag>();
}

// This must be defined in the cpp file, or we get undefined symbols when
// linking.
Exec_DefinitionRegistry&
Exec_DefinitionRegistry::GetInstance() {
    return TfSingleton<Exec_DefinitionRegistry>::GetInstance();
}

const Exec_ComputationDefinition *
Exec_DefinitionRegistry::GetPrimComputationDefinition(
    TfType schemaType,
    const TfToken &computationName) const
{
    const auto it = _primComputationDefinitions.find(
        {schemaType, computationName});
    return it == _primComputationDefinitions.end() ? nullptr : &it->second;
}

void
Exec_DefinitionRegistry::_RegisterPrimComputation(
    TfType schemaType,
    const TfToken &computationName,
    TfType resultType,
    ExecCallbackFn &&callback,
    Exec_InputKeyVector &&inputKeys)
{
    if (schemaType.IsUnknown()) {
        TF_CODING_ERROR(
            "Attempt to register a computation using an unknown type.");
        return;
    }

    const bool emplaced =
        _primComputationDefinitions.emplace(
            std::piecewise_construct, 
            std::forward_as_tuple(schemaType, computationName),
            std::forward_as_tuple(
                resultType,
                std::move(callback),
                std::move(inputKeys))).second;

    if (!emplaced) {
        TF_CODING_ERROR(
            "Duplicate prim computation registration for computation named "
            "'%s' on schema %s",
            computationName.GetText(),
            schemaType.GetTypeName().c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
