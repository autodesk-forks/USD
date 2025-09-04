#include "pxr/usd/usdGeom/pointBased.h"
#include <emscripten/bind.h>

#include "pxr/usd/sdf/emscriptenSdfToVtValue.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdGeomPointBased) {
    class_<pxr::UsdGeomPointBased, base<pxr::UsdGeomGprim>>("UsdGeomPointBased")
        .constructor<const pxr::UsdPrim &>()
        .function("GetPointsAttr", &pxr::UsdGeomPointBased::GetPointsAttr)
        .function("CreatePointsAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomPointBased,
                      &pxr::UsdGeomPointBased::CreatePointsAttr>)
    ;
}