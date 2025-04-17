//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_INPUT_KEY_H
#define PXR_EXEC_EXEC_INPUT_KEY_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/providerResolution.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Data used to specify a computation input.
///
/// Exec compilation uses input keys to compile the input connections for that
/// provide input values to computations. The input key is expressed relative to
/// the scene object that owns the computation that reads from the input.
///
struct Exec_InputKey
{
    /// The name used to uniquely address the input value.
    TfToken inputName;

    /// The requested computation name.
    TfToken computationName;

    /// The requested computation result type.
    TfType resultType;

    /// Describes how we find the provider, starting from the object that owns
    /// the computation to which this key provides an input.
    ExecProviderResolution providerResolution;

    /// Indicates whether or not the input is optional.
    bool optional;
};

/// A vector of input keys.
///
/// This is chosen for efficient storage of input keys in
/// Exec_ComputationDefinition%s.
///
using Exec_InputKeyVector = TfSmallVector<Exec_InputKey, 1>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
