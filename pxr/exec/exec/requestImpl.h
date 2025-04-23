//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REQUEST_IMPL_H
#define PXR_EXEC_EXEC_REQUEST_IMPL_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/valueKey.h"

#include "pxr/base/tf/bits.h"
#include "pxr/exec/vdf/maskedOutput.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class ExecSystem;
class VdfRequest;
class VdfSchedule;

// Contains data structures necessary to implement ExecRequest.
//
class Exec_RequestImpl
{
    Exec_RequestImpl(const Exec_RequestImpl&) = delete;
    Exec_RequestImpl& operator=(const Exec_RequestImpl&) = delete;

public:
    explicit Exec_RequestImpl(std::vector<ExecValueKey> &&valueKeys);
    ~Exec_RequestImpl();

    void Compile(ExecSystem *system);
    void Schedule();

private:
    const std::vector<ExecValueKey> _valueKeys;

    // As an optimization, requesting values that don't actually require
    // evaluating any computations are satisfied directly from value
    // resolution.
    TfBits _requiresComputation;

    std::vector<VdfMaskedOutput> _leafOutputs;

    std::unique_ptr<VdfRequest> _computeRequest;
    std::unique_ptr<VdfSchedule> _schedule;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
