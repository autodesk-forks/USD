#include "pxr/usd/usdShade/output.h"
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

EMSCRIPTEN_BINDINGS(UsdShadeOutput) {
  class_<pxr::UsdShadeOutput>("UsdShadeOutput")
    .function("GetFullName", &pxr::UsdShadeOutput::GetFullName)
    .function("GetPrim", &pxr::UsdShadeOutput::GetPrim)    
    .function("Set", &::SetVtValueFromEmscriptenVal<pxr::UsdShadeOutput>)
    .function("Set", &::SetVtValueFromEmscriptenVal_TimeCode<pxr::UsdShadeOutput>)
    ;
}
