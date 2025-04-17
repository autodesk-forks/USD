//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_ATTRIBUTE_H
#define PXR_EXEC_EXEC_USD_ATTRIBUTE_H

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/property.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Implementation of EsfAttributeInterface that wraps a UsdAttribute.
class ExecUsd_Attribute
    : public ExecUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>
{
public:
    ~ExecUsd_Attribute() override;

    /// Copies the provided \p attribute into this instance.
    ExecUsd_Attribute(const UsdAttribute &attribute)
        : ExecUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>(attribute) {}

    /// Moves the provided \p attribute into this instance.
    ExecUsd_Attribute(UsdAttribute &&attribute)
        : ExecUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>(std::move(attribute)) {}

private:
    // EsfAttributeInterface implementation.
    SdfValueTypeName _GetValueTypeName() const final;
    bool _Get(VtValue *value, UsdTimeCode time) const final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
