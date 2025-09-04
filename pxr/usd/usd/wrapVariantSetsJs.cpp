#include "pxr/usd/usd/variantSets.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(std::string)
EMSCRIPTEN_REGISTER_TYPE(std::vector< std::string >)

EMSCRIPTEN_BINDINGS(UsdVariantSet) {
  class_<pxr::UsdVariantSet>("UsdVariantSet")
    .function("GetVariantNames",       &pxr::UsdVariantSet::GetVariantNames)
    .function("GetVariantSelection",   &pxr::UsdVariantSet::GetVariantSelection)
    .function("SetVariantSelection",   &pxr::UsdVariantSet::SetVariantSelection)
    .function("ClearVariantSelection", &pxr::UsdVariantSet::ClearVariantSelection)
    .function("GetPrim",               &pxr::UsdVariantSet::GetPrim)
    .function("GetName",               &pxr::UsdVariantSet::GetName)
    ;
}

EMSCRIPTEN_BINDINGS(UsdVariantSets) {
  class_<pxr::UsdVariantSets>("UsdVariantSets")
    .function("GetNames",      select_overload<std::vector<std::string>()const>(&pxr::UsdVariantSets::GetNames))
    .function("GetVariantSet", &pxr::UsdVariantSets::GetVariantSet)
    .function("HasVariantSet", &pxr::UsdVariantSets::HasVariantSet)
    ;
}
