#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/sdf/wrapPathJs.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdGeomXform) {
    class_<pxr::UsdGeomXform, base<pxr::UsdGeomXformable>>("UsdGeomXform")
        .constructor<const pxr::UsdPrim &>()
        .class_function("Define", &pxr::UsdGeomXform::Define)
    ;
}