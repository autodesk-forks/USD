#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/sdf/wrapPathJs.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdGeomSphere) {
    class_<pxr::UsdGeomSphere, base<pxr::UsdGeomGprim>>("UsdGeomSphere")
        .constructor<const pxr::UsdPrim &>()
        .class_function("Define", &pxr::UsdGeomSphere::Define)
        .function("GetRadiusAttr", &pxr::UsdGeomSphere::GetRadiusAttr)
        .function("GetExtentAttr", &pxr::UsdGeomSphere::GetExtentAttr)
    ;
}