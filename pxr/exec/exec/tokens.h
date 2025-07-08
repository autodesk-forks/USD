//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_TOKENS_H
#define PXR_EXEC_EXEC_TOKENS_H

/// \file
///
/// This public header declares static tokens that form part of the API for this
/// library.
///

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define EXEC_BUILTIN_COMPUTATIONS  \
    (computeValue)

/// Declares tokens used to identify builtin computations.
TF_DECLARE_PUBLIC_TOKENS(
    ExecBuiltinComputations, EXEC_API, EXEC_BUILTIN_COMPUTATIONS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
