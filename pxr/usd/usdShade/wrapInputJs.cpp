#include "pxr/usd/usdShade/input.h"
#include "pxr/base/tf/wrapTokenJs.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/sdf/emscriptenSdfToVtValue.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/types.h"
#include "pxr/usd/usd/wrapTimeCodeJs.h"

#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;

bool connectToSource(pxr::UsdShadeInput &self, pxr::UsdShadeShader const &source, pxr::TfToken const &sourceName) {
  return self.ConnectToSource(source.ConnectableAPI(), sourceName, pxr::UsdShadeAttributeType::Output, pxr::SdfValueTypeName());
}

EMSCRIPTEN_BINDINGS(UsdShadeInput) {
  class_<pxr::UsdShadeInput>("UsdShadeInput")
    .function("GetFullName", &pxr::UsdShadeInput::GetFullName)
    .function("GetPrim", &pxr::UsdShadeInput::GetPrim)    
    .function("ConnectToSource", &connectToSource)
    .function("ConnectToSourceInput", select_overload<bool(const pxr::UsdShadeInput&)const>(&pxr::UsdShadeInput::ConnectToSource))
    .function("ConnectToSourcePath", select_overload<bool(const pxr::SdfPath&)const>(&pxr::UsdShadeInput::ConnectToSource))
    .function("Get", &::GetAndReturnEmscriptenValFromVtValue<pxr::UsdShadeInput>)
    .function("Get", &::GetAndReturnEmscriptenValFromVtValue_TimeCode<pxr::UsdShadeInput>)
    .function("Set", &::SetVtValueFromEmscriptenVal<pxr::UsdShadeInput>)
    .function("Set", &::SetVtValueFromEmscriptenVal_TimeCode<pxr::UsdShadeInput>)
    .function("GetAttr", &pxr::UsdShadeInput::GetAttr)
    ;
}
