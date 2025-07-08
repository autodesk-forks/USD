//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_TYPED_VECTOR_H
#define PXR_EXEC_VDF_TYPED_VECTOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/vector.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A VdfTypedVector implements a VdfVector with a specific type. 
///
template<typename TYPE>
class VdfTypedVector : public VdfVector
{
public:
    // Note the sole purpose of this type is to allow default construction
    // of empty VdfVectors.  VdfVector is not polymorphic, this type should
    // not hold any state at all.  We rely that objects of this type can
    // be sliced into VdfVectors with no adverse effect.

    /// Constructs an empty vector
    ///
    VdfTypedVector() 
    {
        static_assert(sizeof(VdfTypedVector) == sizeof(VdfVector),
                      "VdfTypedVector must have same size as VdfVector");

        _data.New< Vdf_VectorImplEmpty<TYPE> >(0);
    }

    /// Construct a new vector and initialize it with a specific value
    ///
    VdfTypedVector(const TYPE &value)
    {
        static_assert(sizeof(VdfTypedVector) == sizeof(VdfVector),
                      "VdfTypedVector must have same size as VdfVector");

        _data.New< Vdf_VectorImplSingle<TYPE> >(value);
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

