//
// Copyright 2021 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/wrapPathJs.h"
#include "pxr/base/tf/wrapTokenJs.h"
#include "pxr/usd/usd/emscriptenPtrRegistrationHelper.h"

#include <string>
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_REGISTER_SMART_PTR(pxr::UsdStage)

template<typename T>
std::string exportToString(T& self)
{
    std::string output;
    self.ExportToString(&output);
    return output;
}

pxr::UsdStageRefPtr CreateNew(const std::string& identifier)
{
    return pxr::UsdStage::CreateNew(identifier, pxr::UsdStage::InitialLoadSet::LoadAll);
}

pxr::UsdStageRefPtr Open(const std::string& identifier)
{
    return pxr::UsdStage::Open(identifier);
}

EMSCRIPTEN_BINDINGS(UsdStage) {
  class_<pxr::UsdStage>("UsdStage")
    .smart_ptr_constructor("UsdStageRefPtr", &CreateNew)
    .class_function("CreateNew", &CreateNew)
    .class_function("CreateNew", select_overload<pxr::UsdStageRefPtr(const std::string&, pxr::UsdStage::InitialLoadSet load)>(&pxr::UsdStage::CreateNew))
    .function("ExportToString", &exportToString<pxr::UsdStage>)
    .function("DefinePrim", &pxr::UsdStage::DefinePrim)
    ;
}
