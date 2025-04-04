#include "pxr/usd/usdGeom/gprim.h"
#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdGeomGprim) {
    class_<pxr::UsdGeomGprim, base<pxr::UsdGeomBoundable>>("UsdGeomGprim")
        .constructor<const pxr::UsdPrim &>()
    ;
}