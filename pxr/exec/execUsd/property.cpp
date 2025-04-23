//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/property.h"

#include "pxr/exec/esf/attribute.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

// EsfProperty should not reserve more space than necessary.
static_assert(sizeof(ExecUsd_Property) == sizeof(EsfProperty));

template <class InterfaceType, class UsdPropertyType>
ExecUsd_PropertyImpl<InterfaceType, UsdPropertyType>::~ExecUsd_PropertyImpl()
    = default;

template <class InterfaceType, class UsdPropertyType>
TfToken
ExecUsd_PropertyImpl<InterfaceType, UsdPropertyType>::_GetBaseName() const
{
    return this->_GetWrapped().GetBaseName();
}

template <class InterfaceType, class UsdPropertyType>
TfToken
ExecUsd_PropertyImpl<InterfaceType, UsdPropertyType>::_GetNamespace() const
{
    return this->_GetWrapped().GetNamespace();
}

// Explicit template instantiations
template class ExecUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>;
template class ExecUsd_PropertyImpl<EsfPropertyInterface, UsdProperty>;

PXR_NAMESPACE_CLOSE_SCOPE