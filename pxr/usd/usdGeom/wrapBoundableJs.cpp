#include "pxr/usd/usdGeom/boundable.h"
#include <emscripten/bind.h>

#include "pxr/usd/sdf/emscriptenSdfToVtValue.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdGeomBoundable) {
    class_<pxr::UsdGeomBoundable, base<pxr::UsdGeomXformable>>("UsdGeomBoundable")
        .constructor<const pxr::UsdPrim &>()
        .function("GetExtentAttr", &pxr::UsdGeomBoundable::GetExtentAttr)
        .function("CreateExtentAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomBoundable, &pxr::UsdGeomBoundable::CreateExtentAttr>)
    ;
}