// side_module.cpp
#include <pxr/imaging/hgiWebGPU/texture.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/path.h>

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
