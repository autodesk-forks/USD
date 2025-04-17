//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_OUTPUT_KEY_H
#define PXR_EXEC_EXEC_OUTPUT_KEY_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/valueKey.h"

#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Specifies an output to compile.
///
/// Compilation uses output keys to identify outputs to compile along with
/// parameters needed for their compilation.
/// 
class Exec_OutputKey
{
public:
    explicit Exec_OutputKey(const ExecValueKey &valueKey) :
        _valueKey(valueKey)
    {}

    /// Returns the value key, which specifies the scene object provider and
    /// name of the computation to compile.
    /// 
    const ExecValueKey &GetValueKey() const {
        return _valueKey;
    }

    bool operator==(const Exec_OutputKey &rhs) const {
        return _valueKey == rhs._valueKey;
    }

    bool operator!=(const Exec_OutputKey &rhs) const {
        return !(*this == rhs);
    }

    template <typename HashState>
    friend void TfHashAppend(HashState& h, const Exec_OutputKey& key) {
        h.Append(key._valueKey);
    }

    /// Return a human-readable description of this value key for diagnostic
    /// purposes.
    /// 
    EXEC_API
    std::string GetDebugName() const;

private:
    ExecValueKey _valueKey;
};

/// A vector of output keys.
///
/// This is chosen for efficient storage of output keys generated from
/// Exec_CompilationTask%s, where often just a single output key is generated
/// per input.
///
using Exec_OutputKeyVector = TfSmallVector<Exec_OutputKey, 1>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif