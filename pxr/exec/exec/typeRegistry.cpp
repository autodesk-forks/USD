//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/typeRegistry.h"

#include "pxr/exec/ef/time.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(ExecTypeRegistry);

ExecTypeRegistry&
ExecTypeRegistry::GetInstance()
{
    return TfSingleton<ExecTypeRegistry>::GetInstance();
}

inline
ExecTypeRegistry::ExecTypeRegistry()
{
    TRACE_FUNCTION();

    SdfSchema const& schema = SdfSchema::GetInstance();

    // Ensure that USD value types are registered before subscribing to our
    // registry function so that plugin type registration cannot override the
    // schema fallback value.

#define _EXEC_REGISTER_VALUE_TYPE(unused, elem)                         \
    {                                                                   \
        using ValueType = SDF_VALUE_CPP_TYPE(elem);                     \
        TfType const type = TfType::Find<ValueType>();                  \
        SdfValueTypeName const name = schema.FindType(type);            \
        VtValue const &def = name.GetDefaultValue();                    \
        if (TF_VERIFY(def.IsHolding<ValueType>())) {                    \
            ValueType const &value = def.UncheckedGet<ValueType>();     \
            RegisterType(value);                                        \
        }                                                               \
    }

    TF_PP_SEQ_FOR_EACH(_EXEC_REGISTER_VALUE_TYPE, ~, SDF_VALUE_TYPES);
#undef _EXEC_REGISTER_VALUE_TYPE

    RegisterType(EfTime());
    RegisterType(SdfPath());

    TfSingleton<ExecTypeRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<ExecTypeRegistry>();
}

PXR_NAMESPACE_CLOSE_SCOPE
