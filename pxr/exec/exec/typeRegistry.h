//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_TYPE_REGISTRY_H
#define PXR_EXEC_EXEC_TYPE_REGISTRY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Singleton used to register and access value types used by exec computations.
///
/// Value types that are used for exec computation input and output values must
/// be registered with this registry.
///
/// The registry is initialized with all value types that Sdf suports for
/// attribute and metadata values.
///
class ExecTypeRegistry
{
public:

    ExecTypeRegistry(ExecTypeRegistry const&) = delete;
    ExecTypeRegistry& operator=(ExecTypeRegistry const&) = delete;

    /// Provides access to the singleton instance, first ensuring it is
    /// constructed.
    ///
    EXEC_API
    static ExecTypeRegistry &GetInstance();

    /// Registers \p ValueType as a value type that exec computations can use
    /// for input and output values, with the fallback value \p fallback.
    ///
    /// In any circumstance that requires a fallback value, i.e., when an
    /// arbitrary value of type \p ValueType must be produced, \p fallback will
    /// be used.
    ///
    /// \warning
    /// If a given \p ValueType is registered more than once, all calls must
    /// specify the same \p fallback; otherwise, which fallback value wins is
    /// indeterminate. If an equality operator is defined for \p ValueType, that
    /// operator will be used to verify that all fallback values have the same
    /// value. Otherwise, multiple registrations are allowed, with no
    /// verification that the fallback values match.
    ///
    template <typename ValueType>
    void RegisterType(ValueType const &fallback);

    /// Confirms that \p ValueType has been registered.
    ///
    /// If \p ValueType has been registered with the ExecTypeRegistry, the
    /// corresponding TfType is returned.
    ///
    /// \warning
    /// If \p ValueType has not been registerd, a fatal error is emitted.
    ///
    template <typename ValueType>
    TfType CheckForRegistration()
    {
        return VdfExecutionTypeRegistry::CheckForRegistration<ValueType>();
    }

private:

    // Only TfSingleton can create instances.
    friend class TfSingleton<ExecTypeRegistry>;

    ExecTypeRegistry();
};

template <typename ValueType>
void
ExecTypeRegistry::RegisterType(ValueType const &fallback)
{
    VdfExecutionTypeRegistry::GetInstance()
        .Define(fallback);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
