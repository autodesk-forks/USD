//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/requestImpl.h"

#include "pxr/exec/exec/system.h"
#include "pxr/exec/exec/typeRegistry.h"

#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"

#include "pxr/base/tf/span.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_RequestImpl::Exec_RequestImpl(
    std::vector<ExecValueKey> &&valueKeys)
    : _valueKeys(std::move(valueKeys))
{
}

Exec_RequestImpl::~Exec_RequestImpl() = default;

void
Exec_RequestImpl::Compile(ExecSystem *system)
{
    if (!system) {
        TF_CODING_ERROR("Got null system");
        return;
    }

    _leafOutputs = system->_Compile(_valueKeys);
    if (!TF_VERIFY(_leafOutputs.size() == _valueKeys.size())) {
        // If we somehow got the wrong number of outputs from compilation, we
        // have no idea if the indices correspond correctly.
        // TODO: Expire the request.
        return;
    }

    // For the moment, computation is required at all indices.  Attribute
    // value computation will support execution bypass when there is no
    // attribute expression.
    _requiresComputation.Resize(_valueKeys.size());
    _requiresComputation.SetAll();
}

void
Exec_RequestImpl::Schedule()
{
    // Because _leafOutputs's entries are one-to-one with _valueKeys, it
    // contains null masked outputs at any index that doesn't require
    // computation.
    std::vector<VdfMaskedOutput> outputs;
    outputs.reserve(_requiresComputation.GetNumSet());
    for (const VdfMaskedOutput &mo : _leafOutputs) {
        if (mo) {
            outputs.push_back(mo);
        }
    }
    TF_VERIFY(outputs.size() == _requiresComputation.GetNumSet());
    _computeRequest = std::make_unique<VdfRequest>(std::move(outputs));

    _schedule = std::make_unique<VdfSchedule>();
    VdfScheduler::Schedule(
        *_computeRequest, _schedule.get(), /* topologicallySort */ false);
}

PXR_NAMESPACE_CLOSE_SCOPE
