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
#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/base/tf/emscriptenTypeRegistration.h"

#include <emscripten/bind.h>
using namespace emscripten;

std::vector<std::string> wrap_UsdUtilsExtractExternalReferences(const std::string &filePath)
{
    std::vector<std::string> subLayers, references, payloads;
    pxr::UsdUtilsExtractExternalReferences(
    	filePath,
        &subLayers, 
        &references, 
        &payloads);

    std::vector<std::string> combinedList;
    size_t totalSize = subLayers.size() + references.size() + payloads.size();
    combinedList.reserve(totalSize);

    combinedList.insert(combinedList.end(), subLayers.begin() , subLayers.end());
    combinedList.insert(combinedList.end(), references.begin(), references.end());
    combinedList.insert(combinedList.end(), payloads.begin()  , payloads.end());

    return combinedList;
}

EMSCRIPTEN_REGISTER_VECTOR_TO_ARRAY_CONVERSION(std::string)
EMSCRIPTEN_REGISTER_TYPE(std::vector< std::string >)

EMSCRIPTEN_BINDINGS(UsdUtils) {
    emscripten::function("UsdUtilsExtractExternalReferences" , &wrap_UsdUtilsExtractExternalReferences);
}
