#ifndef PXR_USD_USD_WRAP_TIMECODE_H
#define PXR_USD_USD_WRAP_TIMECODE_H

#ifdef ARCH_OS_WASM_VM

#include "pxr/usd/usd/timeCode.h"
#include "pxr/base/tf/emscriptenTypeRegistration.h"
#include <emscripten/bind.h>


EMSCRIPTEN_REGISTER_TYPE_CONVERSION(pxr::UsdTimeCode)
    return BindingType<val>::toWireType(val(value.GetValue()), rvp::default_tag{});
}
static pxr::UsdTimeCode fromWireType(WireType value) {
    return pxr::UsdTimeCode(BindingType<val>::fromWireType(value).as<double>());
EMSCRIPTEN_REGISTER_TYPE_CONVERSION_END(pxr::UsdTimeCode)

#endif // ARCH_OS_WASM_VM

#endif // PXR_USD_USD_WRAP_TIMECODE_H
