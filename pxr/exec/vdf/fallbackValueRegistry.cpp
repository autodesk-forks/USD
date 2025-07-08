//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/fallbackValueRegistry.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Vdf_FallbackValueRegistry);

Vdf_FallbackValueRegistry &
Vdf_FallbackValueRegistry::GetInstance()
{
    return TfSingleton<Vdf_FallbackValueRegistry>::GetInstance();
}

Vdf_FallbackValueRegistry::Vdf_FallbackValueRegistry()
{
    TfSingleton<Vdf_FallbackValueRegistry>::SetInstanceConstructed(*this);
}

TfType
Vdf_FallbackValueRegistry::CheckForRegistration(const std::type_info &ti) const
{
    const TfType type = TfType::Find(ti);
    if (ARCH_UNLIKELY(type.IsUnknown())) {
        TF_FATAL_ERROR("Type '%s' not registered with TfType",
                       ArchGetDemangled(ti).c_str());
    }

    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ false);
    if (_fallbackMap.find(type) == _fallbackMap.end()) {
        TF_FATAL_ERROR("No fallback value registered for \"%s\"",
            type.GetTypeName().c_str());
    }

    return type;
}

std::pair<Vdf_FallbackValueRegistry::_FallbackMap::iterator, bool>
Vdf_FallbackValueRegistry::_InsertRegistration(TfType type, _Value &&fallback)
{
    if (ARCH_UNLIKELY(type.IsUnknown())) {
        TF_FATAL_ERROR("Attempted to register fallback value with "
                       "unknown type");
    }

    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ true);
    return _fallbackMap.emplace(type, std::move(fallback));
}

const Vdf_FallbackValueRegistry::_Value &
Vdf_FallbackValueRegistry::_GetFallback(TfType type) const
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ false);
    _FallbackMap::const_iterator iter = _fallbackMap.find(type);

    if (iter == _fallbackMap.end()) {
        TF_FATAL_ERROR("No fallback value registered for \"%s\"",
            type.GetTypeName().c_str());
    }
    return iter->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
