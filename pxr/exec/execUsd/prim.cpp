//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/prim.h"

#include "pxr/exec/execUsd/attribute.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// EsfPrim should not reserve more space than necessary.
static_assert(sizeof(ExecUsd_Prim) == sizeof(EsfPrim));

ExecUsd_Prim::~ExecUsd_Prim() = default;

TfTokenVector ExecUsd_Prim::_GetAppliedSchemas() const
{
    return _GetWrapped().GetAppliedSchemas();
}

EsfAttribute ExecUsd_Prim::_GetAttribute(const TfToken &attrName) const
{
    return {
        std::in_place_type<ExecUsd_Attribute>,
        _GetWrapped().GetAttribute(attrName)
    };
}

EsfPrim ExecUsd_Prim::_GetParent() const
{
    return {std::in_place_type<ExecUsd_Prim>, _GetWrapped().GetParent()};
}

TfType ExecUsd_Prim::_GetType() const
{
    return _GetWrapped().GetPrimTypeInfo().GetSchemaType();
}

PXR_NAMESPACE_CLOSE_SCOPE