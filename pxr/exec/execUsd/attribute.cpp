//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/attribute.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

// EsfAttribute should not reserve more space than necessary.
static_assert(sizeof(ExecUsd_Attribute) == sizeof(EsfAttribute));

ExecUsd_Attribute::~ExecUsd_Attribute() = default;

SdfValueTypeName ExecUsd_Attribute::_GetValueTypeName() const
{
    return _GetWrapped().GetTypeName();
}

bool ExecUsd_Attribute::_Get(VtValue *value, UsdTimeCode time) const
{
    return _GetWrapped().Get(value, time);
}

PXR_NAMESPACE_CLOSE_SCOPE