//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_PRIM_H
#define PXR_EXEC_EXEC_USD_PRIM_H

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/object.h"

#include "pxr/exec/esf/prim.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Implementation of EsfPrimInterface that wraps a UsdPrim.
class ExecUsd_Prim : public ExecUsd_ObjectImpl<EsfPrimInterface, UsdPrim>
{
public:
    ~ExecUsd_Prim() override;

    /// Copies the provided \p prim into this instance.
    ExecUsd_Prim(const UsdPrim &prim)
        : ExecUsd_ObjectImpl<EsfPrimInterface, UsdPrim>(prim) {}

    /// Moves the provided \p prim into this instance.
    ExecUsd_Prim(UsdPrim &&prim)
        : ExecUsd_ObjectImpl<EsfPrimInterface, UsdPrim>(std::move(prim)) {}

private:
    // EsfPrimInterface implementation.
    TfTokenVector _GetAppliedSchemas() const final;
    EsfAttribute _GetAttribute(const TfToken &attrName) const final;
    EsfPrim _GetParent() const final;
    TfType _GetType() const final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif