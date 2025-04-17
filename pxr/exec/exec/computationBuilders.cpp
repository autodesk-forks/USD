//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/computationBuilders.h"

#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"

PXR_NAMESPACE_OPEN_SCOPE


//
// Exec_ComputationBuilderValueSpecifierBase
//

struct Exec_ComputationBuilderValueSpecifierBase::_Data
{
    Exec_InputKey inputKey;
};

Exec_ComputationBuilderValueSpecifierBase::
Exec_ComputationBuilderValueSpecifierBase(
    const TfToken &computationName,
    TfType resultType,
    ExecProviderResolution &&providerResolution,
    const TfToken &inputName)
    : _data(
        new _Data{
            inputName,
            computationName,
            resultType,
            std::move(providerResolution),
            /* optional */ true})
{}

Exec_ComputationBuilderValueSpecifierBase::
Exec_ComputationBuilderValueSpecifierBase(
    const Exec_ComputationBuilderValueSpecifierBase &o)
    : _data(std::make_unique<_Data>(*o._data))
{}

Exec_ComputationBuilderValueSpecifierBase::
~Exec_ComputationBuilderValueSpecifierBase()
{}

void
Exec_ComputationBuilderValueSpecifierBase::_SetInputName(
    const TfToken &inputName)
{
    _data->inputKey.inputName = inputName;
}

void
Exec_ComputationBuilderValueSpecifierBase::_SetOptional(
    const bool optional)
{
    _data->inputKey.optional = optional;
}

void
Exec_ComputationBuilderValueSpecifierBase::_GetInputKey(
    Exec_InputKey *const inputKey) const
{
    *inputKey = _data->inputKey;
}

//
// Exec_PrimComputationBuilder
//

struct Exec_PrimComputationBuilder::_Data
{
    _Data(
        const TfType schemaType_,
        const TfToken &computationName_)
    : schemaType(schemaType_)
    , computationName(computationName_)
    {
    }

    const TfType schemaType;
    const TfToken computationName;
    TfType resultType;
    ExecCallbackFn callback;
    Exec_InputKeyVector inputKeys;
};

Exec_PrimComputationBuilder::Exec_PrimComputationBuilder(
    const TfType schemaType,
    const TfToken &computationName)
    : _data(std::make_unique<_Data>(schemaType, computationName))
{
}

Exec_PrimComputationBuilder::~Exec_PrimComputationBuilder()
{
    Exec_DefinitionRegistry::RegisterComputationAccess::_RegisterPrimComputation(
        Exec_DefinitionRegistry::GetInstance(),
        _data->schemaType,
        _data->computationName,
        _data->resultType,
        std::move(_data->callback),
        std::move(_data->inputKeys));
}

void
Exec_PrimComputationBuilder::_AddCallback(
    ExecCallbackFn &&callback, TfType resultType)
{
    _data->callback = std::move(callback);
    _data->resultType = resultType;
}

void
Exec_PrimComputationBuilder::_AddInputKey(
    const Exec_ComputationBuilderValueSpecifierBase *const valueSpecifier)
{
    _data->inputKeys.push_back({});
    valueSpecifier->_GetInputKey(&_data->inputKeys.back());
}

//
// Exec_ComputationBuilder
//

Exec_ComputationBuilder::Exec_ComputationBuilder(
    const TfType schemaType)
    : _schemaType(schemaType)
{
}

Exec_PrimComputationBuilder 
Exec_ComputationBuilder::PrimComputation(
    const TfToken &computationName)
{
    return Exec_PrimComputationBuilder(_schemaType, computationName);
}

PXR_NAMESPACE_CLOSE_SCOPE
