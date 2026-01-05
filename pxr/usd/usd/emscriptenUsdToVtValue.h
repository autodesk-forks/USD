//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_EMSCRIPTEN_USD_TO_VTVALUE_H
#define PXR_USD_USD_EMSCRIPTEN_USD_TO_VTVALUE_H

#include <emscripten/val.h>

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"

#include <iostream>

template<typename T,
    pxr::UsdAttribute (T::*setter)(
        pxr::VtValue const& defaultValue, bool writeSparsely) const>
pxr::UsdAttribute
SetCustomAttributeFromEmscriptenVal(T& self, const emscripten::val& value)
{
    bool result = false;
    const pxr::SdfValueTypeName& typeName =
        (&self->*setter)(pxr::VtValue(), false).GetTypeName();
    pxr::VtValue vtValue =
        GetVtValueFromEmscriptenVal(value, typeName, &result);
    if (result) {
        return (&self->*setter)(vtValue, false);
    } else {
        std::cerr << "Couldn't find a VtValue mapping for " << typeName
                  << std::endl;
    }
    return pxr::UsdAttribute();
}

#endif // PXR_USD_USD_EMSCRIPTEN_USD_TO_VTVALUE_H
