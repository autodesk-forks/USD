//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_FALLBACK_VALUE_REGISTRY_H
#define PXR_EXEC_VDF_FALLBACK_VALUE_REGISTRY_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/traits.h"
#include "pxr/exec/vdf/typeDispatchTable.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/tf/anyUniquePtr.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/type.h"

#include <tbb/spin_rw_mutex.h>

#include <map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_FallbackValueRegistry
///
/// \brief This is a private helper class that is used to hold the default
/// values for types registered with the execution system as well as provide
/// some useful utility functions.
///
/// All API is thread safe.
///
class Vdf_FallbackValueRegistry
{
public:

    // Non-copyable
    //
    Vdf_FallbackValueRegistry(const Vdf_FallbackValueRegistry &) = delete;
    Vdf_FallbackValueRegistry& operator=(
        const Vdf_FallbackValueRegistry &) = delete;

    // Non-movable
    //
    Vdf_FallbackValueRegistry(Vdf_FallbackValueRegistry &&) = delete;
    Vdf_FallbackValueRegistry& operator=(Vdf_FallbackValueRegistry &&) = delete;

    /// Returns the Vdf_FallbackValueRegistry instance.
    ///
    VDF_API
    static Vdf_FallbackValueRegistry &GetInstance();

    /// Returns the registered fallback value for \p Type from the registry.
    /// It is a fatal error to query types that are not registered. 
    ///
    template<typename Type>
    inline const Type &GetFallback() const;

    /// Checks to make sure that \p ti has been registered with a fallback
    /// value.
    ///
    /// This method will cause a fatal error if the type isn't registered.
    ///
    VDF_API
    TfType CheckForRegistration(const std::type_info &ti) const;

    /// Fills \p vector with the fallback value registered for the given type.
    ///
    void FillVector(TfType type, VdfVector *vector, size_t numElements) const {
        _fillVectorDispatchTable.Call<bool>(type, *this, vector, numElements);
    }


private:

    friend class TfSingleton<Vdf_FallbackValueRegistry>;

    // Private constructor, only called by friend TfSingleton
    //
    Vdf_FallbackValueRegistry();

    // A very simple type-erased container.  This provides only functionality
    // that is relevant to storing execution fallback values.  Other, more
    // general type-erased containers can cause substantial compile-time
    // increases because we store many types and their unused functionality
    // must still be emitted.
    //
    class _Value
    {
    public:
        template <typename T>
        explicit _Value(const T &fallback);

        // There is no checked Get.  The registry must ensure that nobody is
        // able to register values for the wrong type.
        template <typename T>
        const T & UncheckedGet() const;

        // Compares values for types that define equality comparision; returns
        // true if not equality comparable.
        template <typename T>
        bool Equals(const T &rhs) const {
            if constexpr (VdfIsEqualityComparable<T>) {
                return UncheckedGet<T>() == rhs;
            }

            return true;
        }

    private:
        TfAnyUniquePtr _ptr;
    };

    // Only the VdfExecutionTypeRegistry is allowed to call _RegisterType();
    friend class VdfExecutionTypeRegistry;

    // Registers fallback as the fallback value for type, returning false if a
    // fallback was already inserted for the given type.
    template<typename Type> 
    static bool _RegisterType(const Type &fallback, const TfType &type);

    // Inserts fallback as the value for type, returning false if a fallback was
    // already inserted for the given type.
    template<typename Type> 
    bool _InsertRegistrationTyped(TfType type, const Type &fallback) {
        const auto result = _InsertRegistration(type, _Value(fallback));
        if (!result.second) {
            TF_VERIFY(
                result.first->second.Equals(fallback),
                "Type %s registered more than once with different fallback "
                "values.",
                type.GetTypeName().c_str());
            return false;
        }

        return true;
    }

    using _FallbackMap = std::map<TfType, _Value>;

    // Inserts fallback as the value for type.
    //
    // Attempting to register a fallback with unknown type is a fatal error.
    VDF_API
    std::pair<_FallbackMap::iterator, bool> _InsertRegistration(
        TfType type, _Value &&fallback);

    // Helper method for GetFallback() to move as much in to the .cpp as
    // possible.
    VDF_API
    const _Value &_GetFallback(TfType type) const;


    // This is the callback struct for the FillVector call on this class.
    template <typename Type>
    struct _FillVector {
        static bool Call(const Vdf_FallbackValueRegistry &reg,
                         VdfVector *vector, size_t numElements);
    };

    //
    // Data Members
    //

    // This is a type dispatch table used to fill in default values in
    // an output.
    typedef VdfTypeDispatchTable<_FillVector> _FillVectorDispatchTable;
    _FillVectorDispatchTable _fillVectorDispatchTable;

    // This is the map from type to fallback value
    _FallbackMap _fallbackMap;

    // Spin mutex that protects ADT in this class, assumption is that the
    // very most access pattern here are reads.
    mutable tbb::spin_rw_mutex _mutex;
};

///////////////////////////////////////////////////////////////////////////////

template<typename Type>
inline
const Type &Vdf_FallbackValueRegistry::GetFallback() const
{
    const _Value &fallback = _GetFallback(TfType::Find<Type>());
    return fallback.UncheckedGet<Type>();
}

// static
template<typename Type>
bool
Vdf_FallbackValueRegistry::_RegisterType(
    const Type &fallback, const TfType &type)
{
    TF_AXIOM(TfType::Find<Type>() == type);

    Vdf_FallbackValueRegistry &reg = GetInstance();

    if (!reg._InsertRegistrationTyped(type, fallback)) {
        return false;
    }

    // Register this type with our dispatch table for FillVector()
    reg._fillVectorDispatchTable.RegisterType<Type>();

    return true;
}

template <typename Type>
bool
Vdf_FallbackValueRegistry::_FillVector<Type>::Call(
    const Vdf_FallbackValueRegistry &reg,
    VdfVector *vector,
    size_t numElements) 
{
    const Type &fallbackValue = reg.GetFallback<Type>();

    vector->Resize<Type>(numElements);
    VdfVector::ReadWriteAccessor<Type> accessor = 
        vector->GetReadWriteAccessor<Type>();
    for (size_t i = 0; i < numElements; ++i) {
        accessor[i] = fallbackValue;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
inline
Vdf_FallbackValueRegistry::_Value::_Value(const T &fallback)
    : _ptr(TfAnyUniquePtr::New(fallback))
{
}

template <typename T>
inline const T &
Vdf_FallbackValueRegistry::_Value::UncheckedGet() const
{
    return *static_cast<const T*>(_ptr.Get());
}

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
