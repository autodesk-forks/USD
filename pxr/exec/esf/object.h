//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_OBJECT_H
#define PXR_EXEC_ESF_OBJECT_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"
#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfJournal;
class EsfPrim;
class TfToken;

/// Scene object abstraction for scene adapter implementations.
///
/// The scene object abstraction closely resembles the read-only interface of
/// UsdObject.
///
/// The public methods of this class are called by the exec network compiler.
/// Each method takes an argument of type EsfJournal* which captures the
/// conditions for recompilation.
///
/// This class and all classes derived from it are compatible with
/// EsfFixedSizePolymorphicHolder.
///
class ESF_API_TYPE EsfObjectInterface : public EsfFixedSizePolymorphicBase
{
public:
    ESF_API ~EsfObjectInterface() override;

    /// \see UsdObject::IsValid
    ESF_API bool IsValid(EsfJournal *journal) const;

    /// \see UsdObject::GetPath
    ESF_API SdfPath GetPath(EsfJournal *journal) const;

    /// \see UsdObject::GetName
    ESF_API TfToken GetName(EsfJournal *journal) const;

    /// Returns the prim that owns this EsfObject. If this EsfObject is already
    /// a prim, returns this object as a prim.
    ESF_API EsfPrim GetPrim(EsfJournal *journal) const;

protected:
    /// This constructor may only be called by the scene adapter implementation.
    EsfObjectInterface(const SdfPath &path) : _path(path) {}

    /// Gets the path to this object used for journaling.
    const SdfPath &_GetPath() const { return _path; }

private:
    // Object path that will be added to EsfJournals.
    SdfPath _path;

    // These methods must be implemented by the scene adapter implementation.
    virtual bool _IsValid() const = 0;
    virtual TfToken _GetName() const = 0;
    virtual EsfPrim _GetPrim() const = 0;
};

/// Holds an implementation of EsfObjectInterface in a fixed-size buffer.
///
/// The buffer is large enough to fit an implementation that wraps a UsdObject.
/// The size is specified as an integer literal to prevent introducing Usd as
/// a dependency.
///
class EsfObject
    : public EsfFixedSizePolymorphicHolder<EsfObjectInterface, 48>
{
public:
    using EsfFixedSizePolymorphicHolder::EsfFixedSizePolymorphicHolder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
