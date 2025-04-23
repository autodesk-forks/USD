//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REQUEST_H
#define PXR_EXEC_EXEC_REQUEST_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_RequestImpl;

/// A batch of computations to evaluate concurrently.
class ExecRequest
{
    ExecRequest(const ExecRequest&) = delete;
    ExecRequest& operator=(const ExecRequest&) = delete;

public:
    EXEC_API
    ~ExecRequest();

    /// Returns false if the request can no longer be used due to scene
    /// changes.
    EXEC_API
    bool IsValid() const;

private:
    // Requests are issued by an ExecSystem and may only be used with that
    // system.
    friend class ExecSystem;

    EXEC_API
    ExecRequest(const std::shared_ptr<Exec_RequestImpl>&);

private:
    std::weak_ptr<Exec_RequestImpl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
