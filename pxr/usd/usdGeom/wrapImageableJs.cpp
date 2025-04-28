#include "pxr/usd/usdGeom/imageable.h"
#include <emscripten/bind.h>

using namespace emscripten;

// Hack to get around UsdTimeCode() in signature.  
// Once fixed, then remove the helper func
//  void MakeVisible(const UsdTimeCode &time=UsdTimeCode::Default()) const;
void MakeVisible(pxr::UsdGeomImageable& self)
{
    self.MakeVisible();
}

// Hack to get around UsdTimeCode() in signature.  
// Once fixed, then remove the helper func
//  void MakeInvisible(const UsdTimeCode &time=UsdTimeCode::Default()) const;
void MakeInvisible(pxr::UsdGeomImageable& self)
{
    self.MakeInvisible();
}

// Hack to get around UsdTimeCode() in signature.  
// Once fixed, then remove the helper func
//  void MakeInvisible(const UsdTimeCode &time=UsdTimeCode::Default()) const;
emscripten::val ComputeVisibility(pxr::UsdGeomImageable& self)
{
    pxr::TfToken tok = self.ComputeVisibility();
    pxr::VtValue vt_val(tok);
    return vt_val._GetJsVal();
}


EMSCRIPTEN_BINDINGS(UsdGeomImageable) {
    class_<pxr::UsdGeomImageable>("UsdGeomImageable")
        .constructor<const pxr::UsdPrim &>()
        .function("GetVisibilityAttr", &pxr::UsdGeomImageable::GetVisibilityAttr)
        .function("MakeVisible", &MakeVisible)
        .function("MakeVisible", &pxr::UsdGeomImageable::MakeVisible)
        .function("MakeInvisible", &MakeInvisible)
        .function("MakeInvisible", &pxr::UsdGeomImageable::MakeInvisible)
        .function("ComputeVisibility", &ComputeVisibility)
        .function("ComputeVisibility", &pxr::UsdGeomImageable::ComputeVisibility)
    ;
}