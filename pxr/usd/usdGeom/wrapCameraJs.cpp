#include "pxr/usd/usdGeom/camera.h"
#include <emscripten/bind.h>

#include "pxr/usd/sdf/emscriptenSdfToVtValue.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(UsdGeomCamera) {
    class_<pxr::UsdGeomCamera, base<pxr::UsdGeomXformable>>("UsdGeomCamera")
        .constructor<const pxr::UsdPrim &>()
    ;
}
