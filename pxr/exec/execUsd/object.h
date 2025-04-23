//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_OBJECT_H
#define PXR_EXEC_EXEC_USD_OBJECT_H

#include "pxr/pxr.h"

#include "pxr/exec/esf/object.h"
#include "pxr/usd/usd/object.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

/// Common implementation of EsfObjectInterface.
///
/// This implementation wraps an instance of UsdObject or subclass of UsdObject.
/// The exact type is specified by the \p UsdObjectType template parameter.
///
/// The \p InterfaceType template parameter controls which interface this class
/// inherits from, which can be EsfObjectInterface or any other interface that
/// extends EsfObjectInterface.
///
/// ## ExecUsd Inheritance Structure
///
/// ExecUsd defines implementations of the Esf interface classes. Naturally,
/// ExecUsd_Object inherits from EsfObjectInterface, ExecUsd_Prim inherits from
/// EsfPrimInterface, etc.
///
/// However, while all prims are objects (i.e. EsfPrimInterface extends
/// EsfObjectInterface), ExecUsd_Prim does _not_ inherit from ExecUsd_Object.
/// This means ExecUsd_Prim needs to override and re-implement the virtual
/// methods of EsfObjectInterface in the same manner as ExecUsd_Object, but must
/// do so using a UsdPrim instead of a UsdObject.
///
/// We prevent code duplication by defining ExecUsd_XxxImpl class templates.
/// These templates provide a single implementation for virtual methods defined
/// by an Esf interface which can be "grafted" onto any subclass of that
/// interface while also operating on a generic USD object type.
///
/// For example, this is the full inheritance chain for ExecUsd_Attribute:
///
///  EsfObjectInterface
///    EsfPropertyInterface
///      EsfAttributeInterface
///        ExecUsd_ObjectImpl<EsfAttributeInterface, UsdAttribute>
///          ExecUsd_PropertyImpl<EsfAttributeInterface, UsdAttribute>
///            ExecUsd_Attribute
///
/// And here is the full inheritance chain for ExecUsd_Prim. Note that in this
/// diagram, ExecUsd_ObjectImpl inherits from EsfPrimInterface instead.
///
///  EsfObjectInterface
///    EsfPrimInterface
///      ExecUsd_ObjectImpl<EsfPrimInterface, UsdPrim>
///        ExecUsd_Prim
///
template <class InterfaceType, class UsdObjectType>
class ExecUsd_ObjectImpl : public InterfaceType
{
    static_assert(std::is_base_of_v<EsfObjectInterface, InterfaceType>);
    static_assert(std::is_base_of_v<UsdObject, UsdObjectType>);

public:
    ~ExecUsd_ObjectImpl() override;

    /// Copies the provided object into this instance.
    ExecUsd_ObjectImpl(const UsdObjectType &object)
        : InterfaceType(object.GetPath())
        , _object(object) {}

    /// Moves the provided object into this instance.
    ExecUsd_ObjectImpl(UsdObjectType &&object)
        : InterfaceType(object.GetPath())
        , _object(std::move(object)) {}

protected:
    /// Accessors to the wrapped object are made available to all derived
    /// classes.
    UsdObjectType &_GetWrapped() { return _object; }
    const UsdObjectType &_GetWrapped() const { return _object; }

private:
    /// The wrapped native usd object.
    UsdObjectType _object;

    // EsfObjectInterface implementation.
    bool _IsValid() const final;
    TfToken _GetName() const final;
    EsfPrim _GetPrim() const final;
};

/// Implementation of EsfObjectInterface that wraps a UsdObject.
using ExecUsd_Object = ExecUsd_ObjectImpl<EsfObjectInterface, UsdObject>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif