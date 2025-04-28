#include "pxr/usd/usdGeom/xformOp.h"
#include "pxr/usd/sdf/emscriptenSdfToVtValue.h"
#include "pxr/usd/usd/wrapTimeCodeJs.h"
#include <emscripten/bind.h>

using namespace emscripten;


EMSCRIPTEN_BINDINGS(UsdGeomXformOp) {
    class_<pxr::UsdGeomXformOp>("UsdGeomXformOp")
        .function("Get", &::GetAndReturnEmscriptenValFromVtValue<pxr::UsdGeomXformOp>)
        .function("Get", &::GetAndReturnEmscriptenValFromVtValue_TimeCode<pxr::UsdGeomXformOp>)
        .function("Set", &::SetVtValueFromEmscriptenVal<pxr::UsdGeomXformOp>)
        .function("Set", &::SetVtValueFromEmscriptenVal_TimeCode<pxr::UsdGeomXformOp>)
        .function("GetOpName", select_overload<pxr::TfToken()const>(&pxr::UsdGeomXformOp::GetOpName))
    ;
}