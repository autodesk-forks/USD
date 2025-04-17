//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/request.h"

#include "pxr/exec/exec/requestImpl.h"

PXR_NAMESPACE_OPEN_SCOPE

ExecRequest::ExecRequest(
    const std::shared_ptr<Exec_RequestImpl>& impl)
    : _impl(impl)
{
}

ExecRequest::~ExecRequest() = default;

bool
ExecRequest::IsValid() const
{
    return !_impl.expired();
}

PXR_NAMESPACE_CLOSE_SCOPE
