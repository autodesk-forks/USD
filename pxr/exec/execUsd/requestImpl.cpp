//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/requestImpl.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/exec/execUsd/visitValueKey.h"

#include "pxr/exec/esfUsd/sceneAdapter.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/valueKey.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Constructs an ExecValueKey that corresponds to the computed value specified
// by an ExecUsdValueKey.  Currently, this is very straightforward.  However,
// there is expected future complexity when dealing with attribute values that
// can be obtained without involving the underlying exec system.
// 
struct _ValueKeyVisitor
{
    ExecValueKey operator()(
        const ExecUsd_AttributeValueKey &key) const {
        return ExecValueKey(
            EsfUsdSceneAdapter::AdaptObject(key.provider),
            key.computation);
    }

    ExecValueKey operator()(
        const ExecUsd_PrimComputationValueKey &key) const {
        return ExecValueKey(
            EsfUsdSceneAdapter::AdaptObject(key.provider),
            key.computation);
    }
};

}

ExecUsd_RequestImpl::ExecUsd_RequestImpl(
    ExecUsdSystem *const system,
    std::vector<ExecUsdValueKey> &&valueKeys,
    ExecRequestComputedValueInvalidationCallback &&valueCallback,
    ExecRequestTimeChangeInvalidationCallback &&timeCallback)
    : Exec_RequestImpl(
        system, std::move(valueCallback), std::move(timeCallback))
    , _valueKeys(std::move(valueKeys))
{
}

ExecUsd_RequestImpl::~ExecUsd_RequestImpl() = default;

void
ExecUsd_RequestImpl::Compile()
{
    if (!_RequiresCompilation()) {
        return;
    }

    TRACE_FUNCTION();

    const size_t numValueKeys = _valueKeys.size();

    std::vector<ExecValueKey> valueKeys;
    valueKeys.reserve(numValueKeys);
    for (const ExecUsdValueKey &uvk : _valueKeys) {
        valueKeys.push_back(ExecUsd_VisitValueKey(_ValueKeyVisitor{}, uvk));
    }

    _Compile(valueKeys);
}

void
ExecUsd_RequestImpl::Schedule()
{
    _Schedule();
}

ExecUsdCacheView
ExecUsd_RequestImpl::Compute()
{
    return ExecUsdCacheView(_Compute());
}

PXR_NAMESPACE_CLOSE_SCOPE
