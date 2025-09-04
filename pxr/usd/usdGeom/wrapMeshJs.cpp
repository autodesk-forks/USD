#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/usd/sdf/emscriptenSdfToVtValue.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_ENABLE_WEAK_PTR_CAST(UsdStage)

EMSCRIPTEN_BINDINGS(UsdGeomMesh) {
    class_<pxr::UsdGeomMesh, base<pxr::UsdGeomPointBased>>("UsdGeomMesh")
        .constructor<const pxr::UsdPrim &>()
        .class_function("Define", &pxr::UsdGeomMesh::Define)
        .function("GetFaceVertexCountsAttr", &pxr::UsdGeomMesh::GetFaceVertexCountsAttr)
        .function("CreateFaceVertexCountsAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomMesh, &pxr::UsdGeomMesh::CreateFaceVertexCountsAttr>)
        .function("GetFaceVertexIndicesAttr", &pxr::UsdGeomMesh::GetFaceVertexIndicesAttr)
        .function("CreateFaceVertexIndicesAttr",
                  &SetCustomAttributeFromEmscriptenVal<pxr::UsdGeomMesh,
                      &pxr::UsdGeomMesh::CreateFaceVertexIndicesAttr>)
        ;
}