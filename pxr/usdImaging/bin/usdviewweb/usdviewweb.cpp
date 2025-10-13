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

#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>
#include <webgpu/webgpu_cpp.h>
#include <pxr/imaging/hgiWebGPU/texture.h>
#include <pxr/imaging/hgiWebGPU/hgi.h>

#include <functional>
#include <vector>
#include <fstream>

#include <GLFW/glfw3.h>
#include <cmath>

#include <pxr/pxr.h>
#include <pxr/base/tf/token.h>
#include <pxr/usdImaging/usdImagingGL/renderParams.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/imaging/hgi/hgi.h>
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"

#include "camera.h"
#include "window_state.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
        INFO
);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(INFO, "UsdViewWeb info");
}

namespace {

    std::function<void()> loop;
    pxr::UsdImagingGLRenderParams renderParams;
    std::unique_ptr<pxr::UsdImagingGLEngine> glEngine;
    pxr::UsdStageRefPtr stage;
    pxr::GlfSimpleMaterial defaultMaterial;
    pxr::GlfSimpleLight light;
    pxr::GlfSimpleLightVector defaultLighting;
    pxr::GfVec4f defaultAmbient = pxr::GfVec4f(0.01f, 0.01f, 0.01f, 1.0f);
    std::string filePath;

    void main_loop() {
        loop();
    }

    wgpu::RenderPipeline createBlitPipeline(wgpu::Device const &device, wgpu::TextureFormat const &format) {
        wgpu::ShaderSourceWGSL wgslDesc = {};
        wgslDesc.code = R"(
var<private> pos : array<vec2<f32>, 3> = array<vec2<f32>, 3>(
              vec2<f32>(-1.0, -1.0), vec2<f32>(-1.0, 3.0), vec2<f32>(3.0, -1.0));
struct VertexOutput {
    @builtin(position) position : vec4<f32>,
                                  @location(0) texCoord : vec2<f32>,
};
@vertex
    fn vertexMain(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
    var output : VertexOutput;
    output.texCoord = pos[vertexIndex] * vec2<f32>(-0.5, -0.5) + vec2<f32>(0.5);
    output.position = vec4<f32>(pos[vertexIndex], 0.0, 1.0);
    return output;
}
@group(0) @binding(0) var imgSampler : sampler;
@group(0) @binding(1) var img : texture_2d<f32>;
@fragment
    fn fragmentMain(@location(0) texCoord : vec2<f32>) -> @location(0) vec4<f32> {
    return textureSample(img, imgSampler, vec2<f32>(1.0, 1.0) - texCoord);
})";
        wgpu::ShaderModuleDescriptor mipmapShaderModuleDsc = {};
        mipmapShaderModuleDsc.nextInChain = &wgslDesc;
        wgpu::ShaderModule blitShaderModule = device.CreateShaderModule(&mipmapShaderModuleDsc);

        wgpu::RenderPipelineDescriptor pipelineDsc = {};
        wgpu::VertexState vertexState = {};
        vertexState.module = blitShaderModule;
        vertexState.entryPoint = "vertexMain";
        pipelineDsc.vertex = vertexState;
        wgpu::FragmentState fragmentState = {};
        fragmentState.module = blitShaderModule;
        fragmentState.entryPoint = "fragmentMain";
        wgpu::ColorTargetState colorDesc = {};
        colorDesc.format = format;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorDesc;

        wgpu::BindGroupLayoutEntry samplerBGL;
        samplerBGL.visibility = wgpu::ShaderStage::Fragment;
        samplerBGL.binding = 0;
        samplerBGL.sampler.type = wgpu::SamplerBindingType::Filtering;
        wgpu::BindGroupLayoutEntry textureBGL;
        textureBGL.visibility = wgpu::ShaderStage::Fragment;
        textureBGL.texture.sampleType = wgpu::TextureSampleType::Float;
        textureBGL.binding = 1;

        std::vector<wgpu::BindGroupLayoutEntry> entries {
                samplerBGL,
                textureBGL
        };
        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor;
        bindGroupLayoutDescriptor.label = "mipmapGeneratorBGL";
        bindGroupLayoutDescriptor.entryCount = 2;
        bindGroupLayoutDescriptor.entries = entries.data();

        wgpu::BindGroupLayout bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDescriptor);
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

        pipelineDsc.layout = pipelineLayout;

        pipelineDsc.fragment = &fragmentState;
        return device.CreateRenderPipeline(&pipelineDsc);
    }

    EM_JS(void, ems_setup, (), {
        if (_ems_main) {
        var currentFileName = "";
        function loadUsdFileFromArrayBuffer(filename, usdFile) {
            let parts = filename.split('.');
            let extension = parts[parts.length - 1];
            extension = extension.split('?')[0];
            // random filename allows to use multiple files at the same time.
            // this implementation currently clears the old file.
            let fileName = (Math.random() + 1).toString(36).substring(7);
            let inputFile = fileName + "." + extension;

            FS_createDataFile('/', inputFile, new Uint8Array(usdFile), true, true, true);

            // clear existing objects
            if (currentFileName) {
                FS_unlink(currentFileName, true);
            }
            currentFileName = inputFile;
            return inputFile;
        }

        async function loadFile(fileOrHandle) {
            let file = undefined;
            try {
                if(fileOrHandle.getFile !== undefined) {
                    file = await fileOrHandle.getFile();
                }
                else
                    file = fileOrHandle;
                var reader = new FileReader();
                reader.onload = function(event) {
                    loadUsdFileFromArrayBuffer(file.name, event.target.result);
                };
                reader.readAsArrayBuffer(file);
            }
            catch(ex) {
                console.warn("Error loading file", fileOrHandle, ex);
            }
        }

        async function loadBinaryFile(url) {
            try {
                const response = await fetch(url);
                if (!response.ok) {
                    throw new Error(`Failed to fetch ${url}: ${response.statusText}`);
                }
                const arrayBuffer = await response.arrayBuffer();
                const urlObject = new URL(url, window.location.origin);
                const fileName = urlObject.pathname.split('/').pop();
                return loadUsdFileFromArrayBuffer(fileName, arrayBuffer);
            } catch (error) {
                console.error(`Error loading binary file: ${error.message}`);
                throw error;
            }
        }

        function testAndLoadFile(file) {
            let ext = file.name.split('.').pop();
            console.log(file.name + ", " + file.size + ", " + ext);
            if(ext == 'usd' || ext == 'usdz'|| ext == 'usda') {
                loadFile(file);
            }
        }

        if (navigator["gpu"]) {
            window.loadBinaryFile = loadBinaryFile;
            (async function() {
                const adapter = await navigator["gpu"]["requestAdapter"]();
                const requestedFeatures = [
                    'depth32float-stencil8',
                    'float32-filterable',
                    'clip-distances',
                    'primitive-index',
                    'texture-formats-tier2'
                ];
                const requiredFeatures = [];
                requestedFeatures.forEach((feat) => {
                    if (adapter.features.has(feat)){
                        requiredFeatures.push(feat);
                        console.log('WebGPU adapter supports ' + feat + '.');
                    } else {
                        console.log('WebGPU adapter does not support ' + feat + '.');
                    }
                });
                const requiredLimits = {
                    maxStorageBuffersPerShaderStage: 10,
                    maxColorAttachmentBytesPerSample: 64,
                    maxBufferSize: adapter.limits.maxBufferSize,
                    maxStorageBufferBindingSize: adapter.limits.maxStorageBufferBindingSize
                };
                const device = await adapter.requestDevice({
                    requiredFeatures: requiredFeatures,
                    requiredLimits: requiredLimits
                });

                Module["preinitializedWebGPUDevice"] = device;
                const canvasContainer = document.getElementById("canvasContainer");
                const height = document.getElementById('canvasContainer').offsetHeight;
                const width = document.getElementById('canvasContainer').offsetWidth;
                const webgpuCanvas = document.createElement("canvas");
                webgpuCanvas.id = "webgpuCanvas";
                webgpuCanvas.height = height;
                webgpuCanvas.width = width;
                canvasContainer.appendChild(webgpuCanvas);
                canvasContainer.style.display = "flex";
                canvasContainer.style.justifyContent = "center";
                const mainCanvas = document.getElementById("canvas");
                mainCanvas.style.position = "absolute";
                mainCanvas.style.opacity = 0;
                const urlParams = new URLSearchParams(window.location.search);
                const perf = urlParams.has('perf');
                let modelParam = urlParams.get('model');
                let usdFilename = "";
                if (modelParam) {
                    usdFilename = await loadBinaryFile(modelParam);
                }

                var lengthBytes = lengthBytesUTF8(usdFilename) + 1;
                var fileNameOnWasmHeap = _malloc(lengthBytes);
                stringToUTF8(usdFilename, fileNameOnWasmHeap, lengthBytes);

                if(!urlParams.has('perf')) {
                    _ems_main(
                        width,
                        height,
                        -1,
                        fileNameOnWasmHeap,
                        false
                    );
                }
                _free(fileNameOnWasmHeap);

            })();
    } else {
        console.log("WebGPU not found.");
    }
    } else {
    console.log("Module entry point not found.");
    }
    });

    void initGLEngine() {
        stage = pxr::UsdStage::Open(filePath);

        // Initialize usd imaging engine
        pxr::SdfPathVector excludedPaths;
        glEngine = std::make_unique<pxr::UsdImagingGLEngine>(
                stage->GetPseudoRoot().GetPath(), excludedPaths);

        pxr::TfToken renderer = pxr::TfToken("HdStormRendererPlugin");
        if (!glEngine->SetRendererPlugin(renderer)) {
            TF_RUNTIME_ERROR("Couldn't set renderer plugin: %s", renderer.GetText());
            exit(-1);
        } else {
            TF_INFO(INFO).Msg("Renderer plugin: %s", renderer.GetText());
        }
        if (!glEngine) {
            TF_RUNTIME_ERROR("Couldn't initialize UsdImagingGLEngine");
            exit(-1);
        } else {
            TF_INFO(INFO).Msg("UsdImagingGLEngine initialized successfully");
        }

        renderParams.showRender = true;
        renderParams.enableLighting = true;
        renderParams.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
        renderParams.enableSceneMaterials = true;
        renderParams.enableUsdDrawModes = true;
        renderParams.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED;
        renderParams.colorCorrectionMode = pxr::HdxColorCorrectionTokens->sRGB;
        renderParams.highlight = true;
        renderParams.clearColor = pxr::GfVec4f(0.5f);
        glEngine->SetSelectionColor(pxr::GfVec4f(1, 1, 0, 1));
    }

    void setupDefaults(pxr::GfVec3d const &lightPosition) {
        // Set default lights and materials
        defaultMaterial.SetAmbient(pxr::GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
        defaultMaterial.SetSpecular(pxr::GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
        defaultMaterial.SetShininess(32.0f);

        light.SetPosition(
                pxr::GfVec4f((float) lightPosition[0], (float) lightPosition[1], (float) lightPosition[2], 1.f));
        light.SetAmbient(pxr::GfVec4f(0.9));
        defaultLighting.push_back(light);
    }

    pxr::GfRange3d getStageBounds() {
        pxr::TfTokenVector purposes;
        purposes.push_back(pxr::UsdGeomTokens->default_);
        purposes.push_back(pxr::UsdGeomTokens->proxy);
        bool useExtentHints = false;

        pxr::UsdGeomBBoxCache bboxCache(pxr::UsdTimeCode::Default(), purposes, useExtentHints);
        pxr::GfBBox3d bbox = bboxCache.ComputeWorldBound(stage->GetPseudoRoot());
        pxr::GfRange3d world = bbox.ComputeAlignedRange();
        return world;
    }

    extern "C" int initialize(uint32_t width, uint32_t height, int32_t numFrames, const char* fileName, bool rotate) {
        if (fileName == nullptr || fileName[0] == '\0') {
            std::cout << "Empty file name" << std::endl;
            filePath = "/" MODEL_NAME "." MODEL_EXT_NAME;
        } else {
            std::cout << "File name: " << fileName << std::endl;
            filePath = fileName;
        }

        TF_INFO(INFO).Msg("File: %s", filePath.c_str());
        TF_INFO(INFO).Msg("Starting GLEngine ");
        wgpu::Instance instance = wgpu::CreateInstance();
        wgpu::Surface surface;
        initGLEngine();
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(EXIT_FAILURE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);

        // just use multiples of 256 now until row alignment is handled in HgiWebGPU
        auto window = glfwCreateWindow(width, height, "HgiWebGPU Test", NULL, NULL);
        if (!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        // get the size of the framebuffer
        int framebufferWidth = 1, framebufferHeight = 1;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        glfwMakeContextCurrent(window);
        pxr::Hgi *hgi = glEngine->GetHgi();
        pxr::HgiWebGPU* hgiWebGPU = static_cast<pxr::HgiWebGPU*>(hgi);
        wgpu::Device device = hgiWebGPU->GetPrimaryDevice();
        wgpu::TextureFormat swapChainFormat =  wgpu::TextureFormat::BGRA8Unorm;

        {
            wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
            canvasDesc.selector = "#webgpuCanvas";

            wgpu::SurfaceDescriptor surfDesc{};
            surfDesc.nextInChain = &canvasDesc;
            surface = instance.CreateSurface(&surfDesc);

            wgpu::SurfaceConfiguration surfaceDesc{};
            surfaceDesc.device = device;
            surfaceDesc.format = swapChainFormat;
            surfaceDesc.width = framebufferWidth;
            surfaceDesc.height = framebufferHeight;
            surface.Configure(&surfaceDesc);
        }

        wgpu::RenderPipeline pipeline = createBlitPipeline(device, swapChainFormat);
        wgpu::SamplerDescriptor samplerDsc = {};
        samplerDsc.minFilter = wgpu::FilterMode::Linear;
        samplerDsc.magFilter = wgpu::FilterMode::Linear;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDsc);

        // Setup camera
        Camera camera = Camera();
        pxr::GfRange3d bounds = getStageBounds();

        // create the samer and set its state
        const auto center = bounds.GetMidpoint();

        const auto dimensions = bounds.GetSize();
        const auto diameter = std::max(dimensions[0], std::max(dimensions[1], dimensions[2]));

        camera.sphere(diameter);
        camera.setPosition(bounds.GetMax() * 2.f);
        camera.setTarget(center);
        camera.setViewport(pxr::GfVec4d(0.f, 0.f, framebufferWidth, framebufferHeight));
        camera.update();
        setupDefaults(camera.getPosition());
        // attach the camera to the window state object
        WindowState wstate;
        wstate.camera = &camera;

        // set the window state data so we can use it in glfw callbacks
        glfwSetWindowUserPointer(window, (void *) &wstate);

        // set glfw input callbacks
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSwapBuffers(window);

        const int rotationSpeed = 40;
        EM_ASM({
            const event = new CustomEvent('onplay', {});
            window.dispatchEvent(event);
        });
        int frameIndex = 0;

        SdfPathVector selection;

        auto returnHits = [&selection,&wstate](UsdImagingGLEngine::IntersectionResultVector outResults) -> void {
            selection.clear();
            if (outResults.size() > 0) {
                std::cout << "Hit "
                    << outResults[0].hitPoint << ", "
                    << outResults[0].hitNormal << ", "
                    << outResults[0].hitPrimPath << ", "
                    << outResults[0].hitInstancerPath << ", "
                    << outResults[0].hitInstanceIndex << "\n";
                selection.push_back(outResults[0].hitPrimPath);
            } else {
                std::cout << "No hit " << std::endl;
            }
            wstate.pickingState = PickingState::Ready;
        };
        loop = [&]() {

            glfwSwapInterval(1);
            glfwPollEvents();
            if (wstate.pickingState == PickingState::Requested) {
                pxr::UsdImagingGLEngine::PickParams pickParams = {
                    HdxPickTokens->resolveNearestToCenter,
                };
                UsdImagingGLRenderParams params;
                glEngine->TestIntersection(
                        pickParams,
                        camera.getViewMatrix(),
                        camera.pickingMatrix(wstate.mouseX, wstate.mouseY),
                        stage->GetPseudoRoot(),
                        params,
                        returnHits);
                wstate.pickingState = PickingState::Processing;
            }
            // update the uniforms
            // blit the texture data to the OpenGL framebuffer
            camera.setViewport(pxr::GfVec4d(0.f, 0.f, framebufferWidth, framebufferHeight));

            // glEngine update
            glEngine->SetRenderBufferSize(pxr::GfVec2i(framebufferWidth, framebufferHeight));
            glEngine->SetRendererAov(pxr::HdAovTokens->color);
            glEngine->SetRenderViewport(pxr::GfVec4d(0, 0, framebufferWidth, framebufferHeight));
            glEngine->SetWindowPolicy(pxr::CameraUtilConformWindowPolicy::CameraUtilFit);
            if (rotate) {
                camera.mouseDown(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0, framebufferWidth / 2, framebufferHeight / 2);
                camera.mouseMove(framebufferWidth / 2 + rotationSpeed, framebufferHeight / 2);
                camera.update();
                camera.mouseUp();
            }
            glEngine->SetCameraState(camera.getViewMatrix(), camera.getProjectionMatrix());
            const auto position = camera.getPosition();
            defaultLighting[0].SetPosition(
                    pxr::GfVec4f((float) position[0], (float) position[1], (float) position[2], 1.f));
            glEngine->SetLightingState(defaultLighting, defaultMaterial, defaultAmbient);

            // Render
            glEngine->SetEnablePresentation(false);

            EM_ASM({
                const event = new CustomEvent('onframechanged', {detail: {frameIndex: $0}});
                window.dispatchEvent(event);
            }, frameIndex);
            glEngine->SetSelected(selection);
            glEngine->Render(stage->GetPseudoRoot(), renderParams);
            EM_ASM({
                const frameIndex = $0;
                const numFrames = $1;
                const event = new CustomEvent('onframepresented', {
                    detail: {
                        frameIndex: frameIndex,
                        lastFrame: frameIndex === numFrames-1,
                        firstFrame: frameIndex === 0
                    }
                });
                window.dispatchEvent(event);
            }, frameIndex, numFrames);


            pxr::HgiTextureHandle colorTarget = glEngine->GetAovTexture(pxr::HdAovTokens->color);

            wgpu::SurfaceTexture surfaceTexture;
            surface.GetCurrentTexture(&surfaceTexture);
            wgpu::TextureView backbuffer = surfaceTexture.texture.CreateView();
            auto srcTexture =static_cast<pxr::HgiWebGPUTexture*>(colorTarget.Get());
            wgpu::Texture colorTexture = srcTexture->GetTextureHandle();
            wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();

            wgpu::RenderPassColorAttachment attachment{};
            attachment.view = backbuffer;
            attachment.loadOp = wgpu::LoadOp::Clear;
            attachment.storeOp = wgpu::StoreOp::Store;
            attachment.clearValue = {0, 0, 0, 1};

            wgpu::RenderPassDescriptor renderpass{};
            renderpass.colorAttachmentCount = 1;
            renderpass.colorAttachments = &attachment;

            renderpass.depthStencilAttachment = nullptr;

            wgpu::BindGroup bindGroup;
            {
                wgpu::TextureViewDescriptor textureViewDesc = {};
                wgpu::TextureView srcView = colorTexture.CreateView(&textureViewDesc);

                wgpu::BindGroupEntry samplerEntry = {};
                samplerEntry.sampler = sampler;
                samplerEntry.binding = 0;

                wgpu::BindGroupEntry textureEntry = {};
                textureEntry.textureView = srcView;
                textureEntry.binding = 1;

                const std::vector<wgpu::BindGroupEntry> entries = {
                        samplerEntry,
                        textureEntry
                };

                const wgpu::BindGroupLayout bindGroupLayout = pipeline.GetBindGroupLayout(0);
                wgpu::BindGroupDescriptor bindGroupDsc = {};
                std::string bindGroupDscLabel = "Texture BindGroupDescriptor";
                bindGroupDsc.label = bindGroupDscLabel.c_str();
                bindGroupDsc.layout = bindGroupLayout;
                bindGroupDsc.entryCount = entries.size();
                bindGroupDsc.entries = entries.data();
                bindGroup = device.CreateBindGroup(&bindGroupDsc);
            }

            wgpu::CommandBuffer commands;
            {
                wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                {
                    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
                    pass.SetPipeline(pipeline);
                    pass.SetBindGroup(0, bindGroup);
                    pass.Draw(3);
                    pass.End();
                }
                commands = encoder.Finish();
            }

            device.GetQueue().Submit(1, &commands);
            if (numFrames > 0) frameIndex++;

            if (numFrames > 0 && frameIndex == numFrames) {
                emscripten_cancel_main_loop();
                glfwDestroyWindow(window);
                glfwTerminate();
                printf("Shutting down\n");
            }
            #if !defined(ARCH_OS_WASM_VM)
                surface.Present();
                instance.ProcessEvents();
            #endif
        };
        emscripten_set_main_loop(main_loop, 0, true);
        glfwDestroyWindow(window);
        glfwTerminate();

        exit(EXIT_SUCCESS);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

extern "C" int __main__(int argc, char **argv);

int main(int argc, char **argv) {
    pxr::ems_setup();
    return 0;
}

extern "C" __attribute__((used, visibility("default"))) void ems_main(
    uint32_t width, uint32_t height, int32_t numFrames, const char* fileName, bool rotate) {
    pxr::initialize(width, height, numFrames, fileName, rotate);
}

extern "C" __attribute__((used, visibility("default"))) bool isWasm64() {
#if defined(__wasm64__)
    return true;
#else
    return false;
#endif
}
