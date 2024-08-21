//
// Copyright 2023 Pixar
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

#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/path.h>

PXR_NAMESPACE_OPEN_SCOPE

extern "C" {
    void initGLEngine(const char* filePath, pxr::UsdImagingGLEngine** glEngine, pxr::UsdStageRefPtr* stage) {
    *stage = pxr::UsdStage::Open(filePath);

    // Initialize usd imaging engine
    pxr::SdfPathVector excludedPaths;
    *glEngine = new pxr::UsdImagingGLEngine((*stage)->GetPseudoRoot().GetPath(), excludedPaths);

    pxr::TfToken renderer = pxr::TfToken("HdStormRendererPlugin");
    if (!(*glEngine)->SetRendererPlugin(renderer)) {
        TF_RUNTIME_ERROR("Couldn't set renderer plugin: %s", renderer.GetText());
        exit(-1);
    } else {
        TF_INFO(INFO).Msg("Renderer plugin: %s", renderer.GetText());
    }
    if (!(*glEngine)) {
        TF_RUNTIME_ERROR("Couldn't initialize UsdImagingGLEngine");
        exit(-1);
    } else {
        TF_INFO(INFO).Msg("UsdImagingGLEngine initialized successfully");
    }
 }
}
PXR_NAMESPACE_CLOSE_SCOPE
