//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_VALUE_KEY_H
#define PXR_EXEC_EXEC_VALUE_KEY_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Specifies a computed value.
///
/// Clients identify computations to evaluate using a scene description
/// object that provides computations and the name of the computation.
/// 
class ExecValueKey
{
public:
    ExecValueKey(const SdfPath& provider, const TfToken& computation) :
        _provider(provider),
        _computation(computation)
    {}

    const SdfPath& GetProviderPath() const {
        return _provider;
    }

    const TfToken& GetComputationToken() const {
        return _computation;
    }

    bool operator==(const ExecValueKey& rhs) const {
        return _provider == rhs._provider &&
            _computation == rhs._computation;
    }

    bool operator!=(const ExecValueKey& rhs) const {
        return !(*this == rhs);
    }

    template <typename HashState>
    friend void TfHashAppend(HashState& h, const ExecValueKey& key) {
        h.Append(key._provider);
        h.Append(key._computation);
    }

    /// Return a human-readable description of this value key for diagnostic
    /// purposes.
    /// 
    EXEC_API
    std::string GetDebugName() const;

private:
    SdfPath _provider;
    TfToken _computation;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
