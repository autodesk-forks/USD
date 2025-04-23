//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_ATTRIBUTE_H
#define PXR_EXEC_ESF_ATTRIBUTE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"
#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"
#include "pxr/exec/esf/property.h"

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfJournal;

/// Attribute abstraction for scene adapter implementations.
///
/// The attribute abstraction closely resembles the read-only interface of
/// UsdAttribute. 
///
/// The public methods of this class are called by the exec network compiler.
/// Each method takes an argument of type EsfJournal* which captures the
/// conditions for recompilation.
///
class ESF_API_TYPE EsfAttributeInterface : public EsfPropertyInterface
{
public:
    ESF_API ~EsfAttributeInterface() override;

    /// \see UsdAttribute::GetValueTypeName
    ESF_API SdfValueTypeName GetValueTypeName(EsfJournal *journal) const;


    /// Gets the resolved value of the attribute at a given time.
    ///
    /// This method is not called by exec compilation, and therefore does not
    /// accept an EsfJournal argument.
    ///
    /// \see UsdAttribute::Get
    ///
    bool Get(VtValue *value, UsdTimeCode time) const
    {
        return _Get(value, time);
    }

protected:
    /// This constructor may only be called by the scene adapter implementation.
    EsfAttributeInterface(const SdfPath &path) : EsfPropertyInterface(path) {}

private:
    // These methods must be implemented by the scene adapter implementation.
    virtual SdfValueTypeName _GetValueTypeName() const = 0;
    virtual bool _Get(VtValue *value, UsdTimeCode time) const = 0;
};

/// Holds an implementation of EsfAttributeInterface in a fixed-size buffer.
///
/// The buffer is large enough to fit an implementation that wraps a
/// UsdAttribute. The size is specified as an integer literal to prevent
/// introducing Usd as a dependency.
///
class EsfAttribute
    : public EsfFixedSizePolymorphicHolder<EsfAttributeInterface, 48>
{
public:
    using EsfFixedSizePolymorphicHolder::EsfFixedSizePolymorphicHolder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
