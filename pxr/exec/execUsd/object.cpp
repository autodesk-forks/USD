//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/object.h"

#include "pxr/exec/execUsd/prim.h"

#include "pxr/base/tf/token.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/property.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// EsfObject should not reserve more space than necessary.
static_assert(sizeof(ExecUsd_Object) == sizeof(EsfObject));

template <class InterfaceType, class UsdObjectType>
ExecUsd_ObjectImpl<InterfaceType, UsdObjectType>::~ExecUsd_ObjectImpl()
    = default;

template <class InterfaceType, class UsdObjectType>
bool ExecUsd_ObjectImpl<InterfaceType, UsdObjectType>::_IsValid() const
{
    return _GetWrapped().IsValid();
}

template <class InterfaceType, class UsdObjectType>
TfToken ExecUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetName() const
{
    return _GetWrapped().GetName();
}

template <class InterfaceType, class UsdObjectType>
EsfPrim ExecUsd_ObjectImpl<InterfaceType, UsdObjectType>::_GetPrim() const
{
    return {std::in_place_type<ExecUsd_Prim>, _GetWrapped().GetPrim()};
}

// Explicit template instantiations.
template class ExecUsd_ObjectImpl<EsfAttributeInterface, UsdAttribute>;
template class ExecUsd_ObjectImpl<EsfObjectInterface, UsdObject>;
template class ExecUsd_ObjectImpl<EsfPrimInterface, UsdPrim>;
template class ExecUsd_ObjectImpl<EsfPropertyInterface, UsdProperty>;

PXR_NAMESPACE_CLOSE_SCOPE