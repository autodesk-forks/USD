//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTION_TYPE_REGISTRY_H
#define PXR_EXEC_VDF_EXECUTION_TYPE_REGISTRY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/outputSpec.h"
#include "pxr/exec/vdf/fallbackValueRegistry.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class VdfExecutionTypeRegistry
///
/// \brief Class to register and manage types that are used within execution
///        via Vdf.
///
/// All API is thread safe.
///
class VdfExecutionTypeRegistry
{
public:
    // Non-copyable
    //
    VdfExecutionTypeRegistry(const VdfExecutionTypeRegistry &) = delete;
    VdfExecutionTypeRegistry& operator=(
        const VdfExecutionTypeRegistry &) = delete;

    // Non-movable
    //
    VdfExecutionTypeRegistry(VdfExecutionTypeRegistry &&) = delete;
    VdfExecutionTypeRegistry& operator=(VdfExecutionTypeRegistry &&) = delete;

    /// Returns the VdfExecutionTypeRegistry singleton instance.
    ///
    VDF_API
    static VdfExecutionTypeRegistry &GetInstance();

    /// Defines a type with the registry. Note that the caller needs to provide
    /// an explicit value. This is, so that enum types are well defined.
    ///
    /// This call will also define the type with TfType if it is not registered
    /// yet.
    ///
    template<typename TYPE>
    static
    TfType Define(const TYPE &fallback);

    /// Defines a type with the registry. Note that the caller needs to provide
    /// an explicit value. This is, so that enum types are well defined.
    ///
    /// This call will also define the type with TfType if it is not registered
    /// yet using BASE as base class.
    ///
    template<typename TYPE, typename BASE>
    static
    TfType Define(const TYPE &fallback);

    /// Returns the registered fallback value for \p TYPE from the registry.
    /// It is a fatal error to query types that are not registered. 
    ///
    template<typename TYPE>
    inline
    const TYPE &GetFallback();

    /// Checks if \p TYPE is defined. Returns the TfType of \p TYPE. If the 
    /// check fails, a fatal error will be issued. The intend is to make sure
    /// that all required types are registered at the time this method is 
    /// called.
    ///
    template<typename TYPE>
    static inline
    TfType CheckForRegistration();

    /// Create an empty VdfVector holding empty data of the given TfType.
    ///
    /// Note this creates an empty vector, not a fallback-valued vector.
    /// See also VdfTypedVector for creating empty vectors by type.
    ///
    static inline 
    VdfVector CreateEmptyVector(const TfType &type);

// -----------------------------------------------------------------------------

private :

    // Helper for Define() above.
    template<typename TYPE>
    void _Define(const TYPE &fallback, TfType scalarType);

    friend class TfSingleton<VdfExecutionTypeRegistry>;

    VdfExecutionTypeRegistry();

    template <typename T>
    struct _CreateEmptyVector {
        static VdfVector Call() {
            return VdfTypedVector<T>();
        }
    };
    
    VdfTypeDispatchTable<_CreateEmptyVector> _createEmptyVectorTable;
};

template<typename TYPE>
TfType VdfExecutionTypeRegistry::Define(const TYPE &fallback)
{
    TfAutoMallocTag mallocTag("Vdf", "VdfExecutionTypeRegistry::Define");
    // Define type with Tf if needed.
    TfType scalarType = TfType::Find<TYPE>();
    if (scalarType.IsUnknown() ||
        !TF_VERIFY(scalarType.GetTypeid() != typeid(void),
            "Type '%s' was declared but not defined",
            scalarType.GetTypeName().c_str())) {

        scalarType = TfType::Define<TYPE>();
    }

    VdfExecutionTypeRegistry::GetInstance()._Define(fallback, scalarType);
    return scalarType;
}

template<typename TYPE, typename BASE>
TfType VdfExecutionTypeRegistry::Define(const TYPE &fallback)
{
    TfAutoMallocTag mallocTag("Vdf", "VdfExecutionTypeRegistry::Define");
    // Define type with Tf if needed.
    TfType scalarType = TfType::Find<TYPE>();
    if (scalarType.IsUnknown() ||
        !TF_VERIFY(scalarType.GetTypeid() != typeid(void),
            "Type '%s' was declared but not defined",
            scalarType.GetTypeName().c_str())) {

        scalarType = TfType::Define<TYPE, BASE>();
    }

    VdfExecutionTypeRegistry::GetInstance()._Define(fallback, scalarType);
    return scalarType;
}

template<typename TYPE>
void VdfExecutionTypeRegistry::_Define(
    const TYPE &fallback, TfType scalarType)
{
    // Register additional Vdf facilities if needed.
    if (Vdf_FallbackValueRegistry::_RegisterType(fallback, scalarType)) {
        VdfOutputSpec::_RegisterType<TYPE>();
        _createEmptyVectorTable.RegisterType<TYPE>();
    }
}

template<typename TYPE>
inline
const TYPE &VdfExecutionTypeRegistry::GetFallback()
{
    return Vdf_FallbackValueRegistry::GetInstance().GetFallback<TYPE>();
}

template<typename TYPE>
inline
TfType VdfExecutionTypeRegistry::CheckForRegistration()
{
    return Vdf_FallbackValueRegistry::GetInstance()
        .CheckForRegistration(typeid(TYPE));
}

inline 
VdfVector VdfExecutionTypeRegistry::CreateEmptyVector(const TfType &type)
{
    return VdfExecutionTypeRegistry::GetInstance()
        ._createEmptyVectorTable.Call<VdfVector>(type);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
