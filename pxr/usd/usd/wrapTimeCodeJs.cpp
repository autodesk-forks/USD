#include "pxr/usd/usd/wrapTimeCodeJs.h"
#include "pxr/usd/usd/timeCode.h"
#include <emscripten/bind.h>

using namespace emscripten;


EMSCRIPTEN_BINDINGS(UsdTimeCode) {
    class_<pxr::UsdTimeCode>("UsdTimeCode")
        .constructor<double>()
        .class_function("EarliestTime", &pxr::UsdTimeCode::EarliestTime)
        .class_function("Default"     , &pxr::UsdTimeCode::Default)
        .function("IsEarliestTime", &pxr::UsdTimeCode::IsEarliestTime)
        .function("IsDefault", &pxr::UsdTimeCode::IsDefault)
        .function("IsNumeric", &pxr::UsdTimeCode::IsNumeric)
        .function("GetValue", &pxr::UsdTimeCode::GetValue)
        .function("SafeStep", &pxr::UsdTimeCode::SafeStep)
    ;
}
