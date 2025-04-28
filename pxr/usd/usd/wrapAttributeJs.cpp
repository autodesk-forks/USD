#include "pxr/base/vt/value.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/usd/sdf/emscriptenSdfToVtValue.h"
#include "pxr/usd/usd/wrapTimeCodeJs.h"

#include <emscripten/bind.h>

using namespace emscripten;


std::string GetTypeName(pxr::UsdAttribute& self) {
    return self.GetTypeName().GetType().GetTypeName();
}

EMSCRIPTEN_BINDINGS(UsdAttribute) {
    register_vector<pxr::UsdAttribute>("vector<UsdAttribute>");

    class_<pxr::UsdAttribute>("UsdAttribute")
        .function("Get", &::GetAndReturnEmscriptenValFromVtValue<pxr::UsdAttribute>)
        .function("Get", &::GetAndReturnEmscriptenValFromVtValue_TimeCode<pxr::UsdAttribute>)
        .function("Set", &::SetVtValueFromEmscriptenVal<pxr::UsdAttribute>)
        .function("Set", &::SetVtValueFromEmscriptenVal_TimeCode<pxr::UsdAttribute>)
        .function("GetTypeName", GetTypeName)
        ;
}
