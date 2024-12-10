//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_WRAPTOKEN_H
#define PXR_BASE_TF_WRAPTOKEN_H

#ifdef ARCH_OS_WASM_VM

#include <emscripten/bind.h>
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/emscriptenTypeRegistration.h"
#include <iostream>

EMSCRIPTEN_REGISTER_TYPE_CONVERSION(pxr::TfToken)
    return BindingType<val>::toWireType(val(value.GetString()), rvp::default_tag{});
}
static pxr::TfToken fromWireType(WireType value) {
    return pxr::TfToken(BindingType<val>::fromWireType(value).as<std::string>());
EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(pxr::TfToken)

EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(pxr::TfToken)
EMSCRIPTEN_REGISTER_TYPE(std::vector<pxr::TfToken>)

#endif // ARCH_OS_WASM_VM

#endif // PXR_BASE_TF_WRAPTOKEN_H