#include "pxr/usd/usdGeom/xformable.h"
#include <emscripten/bind.h>

#include <vector>

using namespace emscripten;


EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(pxr::UsdGeomXformOp)
EMSCRIPTEN_REGISTER_TYPE(std::vector<pxr::UsdGeomXformOp>)

EMSCRIPTEN_BINDINGS(UsdGeomXformable) {
    class_<pxr::UsdGeomXformable, base<pxr::UsdGeomImageable>>("UsdGeomXformable")
        .constructor<const pxr::UsdPrim &>()
        .function("GetXformOpOrderAttr", &pxr::UsdGeomXformable::GetXformOpOrderAttr)
        .function("GetTranslateOp" , &pxr::UsdGeomXformable::GetTranslateOp)
        .function("GetRotateXYZOp" , &pxr::UsdGeomXformable::GetRotateXYZOp)
        .function("GetScaleOp"     , &pxr::UsdGeomXformable::GetScaleOp)
        .function("AddTranslateOp" , &pxr::UsdGeomXformable::AddTranslateOp)
        .function("AddRotateXYZOp" , &pxr::UsdGeomXformable::AddRotateXYZOp)
        .function("AddScaleOp"     , &pxr::UsdGeomXformable::AddScaleOp)
        .function("SetXformOpOrder", &pxr::UsdGeomXformable::SetXformOpOrder)
    ;
}