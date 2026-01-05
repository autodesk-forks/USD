//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_EMSCRIPTEN_SDF_TO_VTVALUE_H
#define PXR_USD_SDF_EMSCRIPTEN_SDF_TO_VTVALUE_H

#include <emscripten/val.h>

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include <functional>
#include <iostream>

using SdfToVtValueFunc =
    std::function<pxr::VtValue(const emscripten::val& jsVal)>;

SdfToVtValueFunc* UsdJsToSdfType(pxr::SdfValueTypeName const& targetType);

pxr::VtValue GetVtValueFromEmscriptenVal(const emscripten::val& value,
    pxr::SdfValueTypeName const& targetType, bool* const success = nullptr);

template<typename T>
bool
SetVtValueFromEmscriptenVal(T& self, const emscripten::val& value)
{
    SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(self.GetTypeName());
    bool result = false;
    if (sdfToValue != NULL) {
        pxr::VtValue vtValue = (*sdfToValue)(value);
        result = self.Set(vtValue);
    } else {
        std::cerr << "Couldn't find a VtValue mapping for "
                  << self.GetTypeName() << std::endl;
    }
    return result;
}

#endif // PXR_USD_SDF_EMSCRIPTEN_SDF_TO_VTVALUE_H
