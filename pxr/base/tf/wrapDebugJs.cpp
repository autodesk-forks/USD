#include "pxr/base/tf/debug.h"
#include <string>

#include "pxr/base/tf/emscriptenTypeRegistration.h"
#include <emscripten/bind.h>

using namespace emscripten;

void _SetOutputFile(const std::string &outputStream)
{
    FILE *file = nullptr;
    if (outputStream == "stdout") {
        file = stdout;
    }
    else if (outputStream == "stderr") {
        file = stderr;
    }
    pxr::TfDebug::SetOutputFile(file);
}

EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(std::string)
EMSCRIPTEN_REGISTER_TYPE(std::vector< std::string >)

EMSCRIPTEN_BINDINGS(TfDebug) {
    class_<pxr::TfDebug>("TfDebug")
        .class_function("SetDebugSymbolsByName",      &pxr::TfDebug::SetDebugSymbolsByName)
        .class_function("IsDebugSymbolNameEnabled",   &pxr::TfDebug::IsDebugSymbolNameEnabled)
        .class_function("GetDebugSymbolDescriptions", &pxr::TfDebug::GetDebugSymbolDescriptions)
        .class_function("GetDebugSymbolNames",        &pxr::TfDebug::GetDebugSymbolNames)
        .class_function("GetDebugSymbolDescription",  &pxr::TfDebug::GetDebugSymbolDescription)
        .class_function("SetOutputFile",              &_SetOutputFile)
    ;
}