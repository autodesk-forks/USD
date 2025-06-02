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

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl3.h>

#include <emscripten/em_js.h>
#include <emscripten/emscripten.h>
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#ifdef EMSCRIPTEN_USE_CONTRIB_GLFW3
#include <GLFW/emscripten_glfw3.h>
#endif

#include <functional>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdlib> // for setenv
#include <iostream>

#include <pxr/pxr.h>
#include <pxr/base/tf/token.h>
#include <pxr/usdImaging/usdImagingGL/renderParams.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/imaging/hgi/hgi.h>
#include <pxr/imaging/hgi/blitCmdsOps.h>
#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hgiWebGPU/texture.h>
#include <pxr/imaging/hgiWebGPU/hgi.h>

#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/plug/plugin.h>

#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/color.h>

#include <pxr/imaging/hdSt/debugCodes.h>
#include <pxr/imaging/hgi/debugCodes.h>

#include "freeCameraGL.h"
#include "window_state.h"
#include "debugCodes.h"


PXR_NAMESPACE_OPEN_SCOPE

namespace usdweb {
    std::function<void()> loop;
    pxr::UsdImagingGLRenderParams renderParams;
    std::unique_ptr<pxr::UsdImagingGLEngine> glEngine;
    pxr::UsdStageRefPtr stage;
    pxr::GlfSimpleMaterial defaultMaterial;
    pxr::GlfSimpleLight cameraLight;
    pxr::GlfSimpleLight domeLight;
    bool cameraLightEnabled;
    bool domeLightEnabled;
    pxr::GlfSimpleLightVector defaultLighting;
    bool _defaultLightingDirty = true;
    pxr::GfVec4f defaultAmbient = pxr::GfVec4f(0.01f, 0.01f, 0.01f, 1.0f);
    std::string filePath;
    FreeCameraGL camera;
    int framebufferWidth = 1, framebufferHeight = 1;

    void main_loop() {
        loop();
    }

    wgpu::RenderPipeline createBlitPipeline(wgpu::Device const &device, wgpu::TextureFormat const &format) {
        wgpu::ShaderModuleWGSLDescriptor wgslDesc = {};
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

    // 
    // JavaScript Function Generator from C++: 
    //   This gets embedded in the generated usdweb.js and 
    //   can be called as a JavaScript function.
    // void ems_setup()
    EM_JS(void, ems_setup, (), {
        console.log('In ems_setup()');
        if (navigator["gpu"]) {
        console.log('Requesting adapter');
        navigator["gpu"]["requestAdapter"]().then(function (adapter) {
            console.log('Requesting adapter. Completed.');
            const requestedFeatures = [
            'depth32float-stencil8',
                    'float32-filterable'
            ];
            const requiredFeatures = [];
            requestedFeatures.forEach((feat) => {
                    if (adapter.features.has(feat))
                    {
                        requiredFeatures.push(feat);
                        console.log('WebGPU adapter supports ' + feat + '.');
                    }
                    else
                    {
                        console.log('WebGPU adapter does not support ' + feat + '.');
                    }
            });
            const requiredLimits = {
                    maxStorageBuffersPerShaderStage: 10,
                    maxColorAttachmentBytesPerSample: 64,
                    maxBufferSize: 0x40000000
            };
            console.log('Requesting device');
            adapter["requestDevice"]({requiredFeatures, requiredLimits}).then( function (device) {
                console.log('Requesting device. Completed.');
                Module["preinitializedWebGPUDevice"] = device;
                console.log('Get orig canvas:'+ document.getElementById("canvas").id);
                const canvasContainer = document.getElementById("canvasContainer");
                const height = canvasContainer.offsetHeight;
                const width  = canvasContainer.offsetWidth;
                const webgpuCanvas = document.createElement("canvas");
                webgpuCanvas.id = "webgpuCanvas";
                webgpuCanvas.height = height;
                webgpuCanvas.width = width;
                canvasContainer.appendChild(webgpuCanvas);
                console.log('Get webgpu canvas:'+ webgpuCanvas.id);
                canvasContainer.style.display = "flex";
                canvasContainer.style.justifyContent = "center";
                // Make sure canvas (created in usdweb.html) has opacity=0;
                const mainCanvas = document.getElementById("canvas");
                console.log('Get new canvas:'+ mainCanvas.id);
                mainCanvas.style.position = "absolute";
                mainCanvas.style.opacity = 0;
                console.log("Module.myInitialize(width, height);.");
                Module.myInitialize(width, height);
            }).catch((res) => { console.log(res); });
        }, function () {
            console.log("WebGPU adapter not found.");
        });
    } else {
        console.log("WebGPU not found.");
    }
    });


    pxr::GfBBox3d getStageBBox(const pxr::UsdPrim &prim) {
        pxr::TfTokenVector purposes;
        purposes.push_back(pxr::UsdGeomTokens->default_);
        purposes.push_back(pxr::UsdGeomTokens->proxy);
        bool useExtentHints = false;

        pxr::UsdGeomBBoxCache bboxCache(pxr::UsdTimeCode::Default(), purposes, useExtentHints);
        pxr::GfBBox3d bbox = bboxCache.ComputeWorldBound(prim);
        return bbox;
    }


    void fit_camera(const pxr::UsdPrim &prim)
    {
        pxr::GfBBox3d bbox = prim.IsValid() ? getStageBBox(prim) : getStageBBox(stage->GetPseudoRoot());
        float frameFit = 1.2f;
        camera.FrameSelection(bbox, frameFit);
        camera.update();
    }


    void LookThroughUsdCamera(const pxr::UsdPrim &prim)
    {
        camera.LookThroughUsdCamera(prim, pxr::usdweb::renderParams.frame);
        camera.update();
    }


    void initParamsForWebStage()
    {
        // Set timecode
        pxr::UsdTimeCode new_timecode( pxr::usdweb::stage->GetStartTimeCode() );
        pxr::usdweb::renderParams.frame = new_timecode;
        // Set Camera
        pxr::usdweb::camera.SetIsZUp( (UsdGeomGetStageUpAxis(pxr::usdweb::stage) == pxr::UsdGeomTokens->z) );
        pxr::usdweb::fit_camera(pxr::usdweb::stage->GetPseudoRoot());
        // Adjust Dome Light
        if (pxr::usdweb::camera.IsZUp()) {
            pxr::usdweb::domeLight.SetTransform(pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d::XAxis(), 90.0)));
        }
    }

    // To be called right before Render() is called
    //
    void UpdateDefaultLightingBeforeRender()
    {
        // Make sure defaultLighting vector is current
        if (_defaultLightingDirty) {
            TF_DEBUG_MSG(USDWEB, "Updating lights since _defaultLightingDirty.");
            defaultLighting.clear();
            if (cameraLightEnabled) {
                defaultLighting.push_back(cameraLight); 
            }
            if (domeLightEnabled) {
                defaultLighting.push_back(domeLight);
            }
            _defaultLightingDirty = false;

            pxr::usdweb::glEngine->SetRendererSetting(
                pxr::HdRenderSettingsTokens->domeLightCameraVisibility,
                pxr::VtValue(domeLightEnabled));
        }
    }
 

    // Initialize params used in SetLightingState()
    void initUsdRenderParams()
    {
        // Set defaultAmbient
        //   initialized when variable instanciated

        // Set default material
        defaultMaterial.SetAmbient(pxr::GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
        defaultMaterial.SetSpecular(pxr::GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
        defaultMaterial.SetShininess(32.0f);   

        // Set default lights
        cameraLightEnabled = true;
        domeLightEnabled   = false;
        cameraLight.SetIsCameraSpaceLight(true); // makes position camera relative
        cameraLight.SetPosition(pxr::GfVec4f(0.f, 0.f, 0.f, 1.f));
        cameraLight.SetAmbient(pxr::GfVec4f(0.0f)); //(pxr::GfVec4f(0.9));
        domeLight.SetIsDomeLight(true); // makes a dome light
        domeLight.SetPosition(pxr::GfVec4f(0.f, 0.f, 0.f, 1.f));
        domeLight.SetAmbient(pxr::GfVec4f(0.0f)); //(pxr::GfVec4f(0.9));
    }

    void initGLEngine() {
        TfDebug::Enable(HDST_DUMP_FAILING_SHADER_SOURCEFILE);
        //TfDebug::Enable(HGI_DEBUG_DEVICE_CAPABILITIES);
        //TfDebug::Enable(HDST_DUMP_SHADER_SOURCE);
        //TfDebug::Enable(HDST_DUMP_SHADER_SOURCEFILE);
        setenv("HGIGL_ENABLE_BINDLESS_TEXTURE", "0", 1);
        setenv("HGIGL_ENABLE_BINDLESS_BUFFER", "0", 1);
        setenv("HGIGL_ENABLE_BUILTIN_BARYCENTRICS", "0", 1);
        setenv("HGIGL_ENABLE_MULTI_DRAW_INDIRECT", "0", 1);
        setenv("HGIGL_ENABLE_SHADER_DRAW_PARAMETERS", "0", 1);

        // Init USD render params
        initUsdRenderParams();

        // Make sure there is a default stage
        if (!stage) {
            //stage = pxr::UsdStage::Open(filePath);
            stage = pxr::UsdStage::CreateInMemory();
            UsdGeomSetStageUpAxis(stage, UsdGeomTokens->y);
            stage->DefinePrim(pxr::SdfPath("/world"), pxr::TfToken("Xform"));
            pxr::UsdPrim prim = stage->DefinePrim(pxr::SdfPath("/world/sphere"), pxr::TfToken("Sphere"));
            // apply a material
            pxr::UsdShadeMaterial material = pxr::UsdShadeMaterial::Define(stage, pxr::SdfPath("/world/MaterialBlue"));
            pxr::UsdShadeShader shader = pxr::UsdShadeShader::Define(stage, pxr::SdfPath("/world/MaterialBlue/Shader"));
            shader.CreateIdAttr().Set(pxr::TfToken("UsdPreviewSurface"));
            shader.CreateInput(pxr::TfToken("diffuseColor"), pxr::SdfValueTypeNames->Color3f).Set(pxr::GfVec3f(0.1, 0.1, 0.8));
            material.CreateSurfaceOutput().ConnectToSource(shader.ConnectableAPI(), pxr::TfToken("surface"));
            pxr::UsdShadeMaterialBindingAPI(prim).Apply(prim).Bind(material);
            initParamsForWebStage();
        }

        // Initialize usd imaging engine
        pxr::SdfPathVector excludedPaths;
        glEngine = std::make_unique<pxr::UsdImagingGLEngine>(
                stage->GetPseudoRoot().GetPath(), excludedPaths);

        pxr::TfToken renderer = pxr::TfToken("HdStormRendererPlugin");
        if (!glEngine->SetRendererPlugin(renderer)) {
            TF_RUNTIME_ERROR("Couldn't set renderer plugin: %s", renderer.GetText());
            exit(-1);
        } else {
            TF_INFO(USDWEB).Msg("Renderer plugin: %s", renderer.GetText());
        }
        if (!glEngine) {
            TF_RUNTIME_ERROR("Couldn't initialize UsdImagingGLEngine");
            exit(-1);
        } else {
            TF_INFO(USDWEB).Msg("UsdImagingGLEngine initialized successfully");
        }

        renderParams.showRender = true;
        renderParams.enableLighting = true;
        renderParams.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
        renderParams.enableSceneMaterials = true;
        renderParams.enableSceneLights = true;
        renderParams.enableUsdDrawModes = true;
        renderParams.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED;
        renderParams.colorCorrectionMode = pxr::HdxColorCorrectionTokens->sRGB;
        renderParams.highlight = true;
        renderParams.clearColor = pxr::GfVec4f(0.5f);
    }


    // NOTES
    //  REFERENCE:
    //      https://github.com/pongasoft/emscripten-glfw/blob/master/docs/Usage.md
    //
    //  Look into emscripten::glfw3::SetNextWindowCanvasSelector("#canvas1")
    //  Look at emscripten::glfw3 "--use-port=contrib.glfw3" as more modern version, multiple windows, faster
    //
    //  BUG:
    //    If initialize() uses std::string as a parameter, then the
    //    function _wgpuDeviceCreateCommandEncoder(deviceId, descriptor) will have a garbage deviceId
    //    passed into it and fail.
    // 
    int initialize(uint32_t width, uint32_t height)
    {
        std::cout << "In initialize(" << width << ", " << height << ")" << std::endl;
        TF_INFO(USDWEB).Msg("Starting GLEngine ");
        initGLEngine();
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(EXIT_FAILURE);

#ifdef EMSCRIPTEN_USE_CONTRIB_GLFW3
        // Need to call this before glfwCreateWindow()
        emscripten::glfw3::SetNextWindowCanvasSelector("#webgpuCanvas");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // without this set, it fails in CreateSurface
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);        
#endif        

        // just use multiples of 256 now until row alignment is handled in HgiWebGPU
        std::cout << "glfwCreateWindow" << std::endl;
        auto window = glfwCreateWindow(width, height, "USD Web", NULL, NULL);
        if (!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

#ifdef EMSCRIPTEN_USE_CONTRIB_GLFW3
        //emscripten::glfw3::MakeCanvasResizable(window, "window"); // fill entire window
        //emscripten::glfw3::MakeCanvasResizable(window, "canvas1-container"); // inside a <div/> 
#endif

        // get the size of the framebuffer
        int framebufferWidth = 1, framebufferHeight = 1;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        std::cout << "glfwMakeContextCurrent" << std::endl;
        glfwMakeContextCurrent(window);
        wgpu::SwapChain swapChain;
        pxr::Hgi *hgi = glEngine->GetHgi();
        pxr::HgiWebGPU* hgiWebGPU = static_cast<pxr::HgiWebGPU*>(hgi);
        wgpu::Device device = hgiWebGPU->GetPrimaryDevice();
        if (!device) {
            std::cout << "ERROR: Invalid wgpu::Device retrieved from hgiWebGPU->GetPrimaryDevice()" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);            
        }
        wgpu::TextureFormat swapChainFormat =  wgpu::TextureFormat::BGRA8Unorm;

        {
            wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc{};
            canvasDesc.selector = "#webgpuCanvas";

            wgpu::SurfaceDescriptor surfDesc{};
            surfDesc.nextInChain = &canvasDesc;
            std::cout << "wgpu::CreateInstance()" << std::endl;
            wgpu::Instance instance = wgpu::CreateInstance();
            std::cout << "instance.CreateSurface(&surfDesc)" << std::endl;
            wgpu::Surface surface = instance.CreateSurface(&surfDesc);

            wgpu::SwapChainDescriptor scDesc{};
            scDesc.usage = wgpu::TextureUsage::RenderAttachment;
            scDesc.format = wgpu::TextureFormat::BGRA8Unorm;
            scDesc.width = framebufferWidth;
            scDesc.height = framebufferHeight;
            scDesc.presentMode = wgpu::PresentMode::Fifo;
            swapChain = device.CreateSwapChain(surface, &scDesc);
        }

        wgpu::Texture testTexture;
        {
            wgpu::TextureDescriptor textureDescriptor;
            textureDescriptor.dimension = wgpu::TextureDimension::e2D;
            textureDescriptor.size.width = width;
            textureDescriptor.size.height = height;
            textureDescriptor.size.depthOrArrayLayers = 1;
            textureDescriptor.sampleCount = 1;
            textureDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
            textureDescriptor.mipLevelCount = 1;
            textureDescriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
            std::cout << "device.CreateTexture(&textureDescriptor)" << std::endl;
            testTexture = device.CreateTexture(&textureDescriptor);

            // Initialize the texture with arbitrary data until we can load images
            std::vector<uint8_t> data(2 * 4 * width * height, 0);
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] = static_cast<uint8_t>(i % 253);
            }

            wgpu::BufferDescriptor stgDescriptor;
            stgDescriptor.size = static_cast<uint32_t>(data.size());
            stgDescriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
            std::cout << "device.CreateBuffer(&stgDescriptor);" << std::endl;
            wgpu::Buffer stagingBuffer = device.CreateBuffer(&stgDescriptor);

            std::cout << "device.GetQueue().WriteBuffer" << std::endl;
            device.GetQueue().WriteBuffer(stagingBuffer, 0, data.data(), static_cast<uint32_t>(data.size()));

            wgpu::ImageCopyBuffer imageCopyBuffer = {};
            imageCopyBuffer.buffer = stagingBuffer;
             wgpu::TextureDataLayout textureDataLayout;
            textureDataLayout.offset = 0;
            textureDataLayout.bytesPerRow = 4 * width;

            imageCopyBuffer.layout = textureDataLayout;

            wgpu::ImageCopyTexture imageCopyTexture;
            imageCopyTexture.texture = testTexture;
            imageCopyTexture.mipLevel = 0;
            imageCopyTexture.origin = {0, 0, 0};
            wgpu::Extent3D copySize = {width, height, 1};

            std::cout << "About to device.CreateCommandEncoder()" << std::endl;
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            std::cout << "encoder.CopyBufferToTexture()" << std::endl;
            encoder.CopyBufferToTexture(&imageCopyBuffer, &imageCopyTexture, &copySize);

            wgpu::CommandBuffer copy = encoder.Finish();
            device.GetQueue().Submit(1, &copy);
        }

        std::cout << "createBlitPipeline" << std::endl;
        wgpu::RenderPipeline pipeline = createBlitPipeline(device, swapChainFormat);
        wgpu::SamplerDescriptor samplerDsc = {};
        samplerDsc.minFilter = wgpu::FilterMode::Linear;
        samplerDsc.magFilter = wgpu::FilterMode::Linear;
        std::cout << "CreateSampler" << std::endl;
        wgpu::Sampler sampler = device.CreateSampler(&samplerDsc);

        // gl framebuffer for blitting
        // we render the frame into a webgpu texture then read it back, upload it as a GL texture and do a framebuffer blit
        // suboptimal but this is to get things working
        GLuint frameBufferTexture;
        std::cout << "glGenTextures" << std::endl;
        glGenTextures(1, &frameBufferTexture);
        std::cout << "glBindTexture" << std::endl;
        glBindTexture(GL_TEXTURE_2D, frameBufferTexture);
        char imageData[4 * 4 * 4];
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGBA, GL_HALF_FLOAT, imageData);

        GLuint frameBufferObject;
        std::cout << "glGenFramebuffers" << std::endl;
        glGenFramebuffers(1, &frameBufferObject);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferObject);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBufferTexture, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // for holding the data read back from the WebGPU render target
        std::vector<uint8_t> colorData;

        // Setup camera
        fit_camera(pxr::UsdPrim());

        // attach the camera to the window state object
        WindowState wstate;
        wstate.camera = &camera;

        // set the window state data so we can use it in glfw callbacks
        std::cout << "glfwSetWindowUserPointer" << std::endl;
        glfwSetWindowUserPointer(window, (void *) &wstate);

        // set glfw input callbacks
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSwapBuffers(window);

        loop = [&]() {

            glfwSwapInterval(1);
            // update the uniforms
            // blit the texture data to the OpenGL framebuffer
            camera.setViewportDimensions(pxr::GfVec2f(framebufferWidth, framebufferHeight));

            UpdateDefaultLightingBeforeRender();

            // glEngine update
            glEngine->SetRenderBufferSize(pxr::GfVec2i(framebufferWidth, framebufferHeight));
            glEngine->SetRendererAov(pxr::HdAovTokens->color);
            glEngine->SetRenderViewport(pxr::GfVec4d(0, 0, framebufferWidth, framebufferHeight));
            glEngine->SetWindowPolicy(pxr::CameraUtilConformWindowPolicy::CameraUtilFit);
            glEngine->SetCameraState(camera.getViewMatrix(), camera.getProjectionMatrix());
            for (GlfSimpleLight& lt : defaultLighting) {
                if (lt.IsCameraSpaceLight()) {
                    lt.SetTransform( camera.getViewInverse() );
                }
            }
            glEngine->SetLightingState(defaultLighting, defaultMaterial, defaultAmbient);

            // Render
            glEngine->SetEnablePresentation(false);

            glEngine->Render(stage->GetPseudoRoot(), renderParams);

            pxr::HgiTextureHandle colorTarget = glEngine->GetAovTexture(pxr::HdAovTokens->color);

            wgpu::TextureView backbuffer = swapChain.GetCurrentTextureView();
            pxr::HgiWebGPUTexture* srcTexture =static_cast<pxr::HgiWebGPUTexture*>(colorTarget.Get());
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

            glfwPollEvents();
        };
        emscripten_set_main_loop(main_loop, 0, true);
        glfwDestroyWindow(window);
        glfwTerminate();

        exit(EXIT_SUCCESS);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
extern "C" int __main__(int argc, char **argv);

// Automatically called from usdweb.js: run(args) -> callMain(args)
// This can be suppressed by setting Module["noInitialRun"] = false
int main(int argc, char **argv) {
    pxr::usdweb::ems_setup();
    return 0;
}


// NOTE: May not be needed if it is called from 
// main(int argc, char **argv) 
void wrap_ems_setup()
{
    pxr::usdweb::ems_setup();   
}

pxr::UsdStageRefPtr& wrap_getStage()
{
    return pxr::usdweb::stage;
}

void wrap_resetEngine()
{
    pxr::SdfPathVector excludedPaths;
    pxr::usdweb::glEngine.reset(
        new pxr::UsdImagingGLEngine(pxr::usdweb::stage->GetPseudoRoot().GetPath(), excludedPaths));   
}

void wrap_setStage(pxr::UsdStageRefPtr &s)
{
    pxr::usdweb::stage = s;
    // Reset Engine (with stage)
    if (pxr::usdweb::glEngine != nullptr) {
        wrap_resetEngine();
    }
    pxr::usdweb::initParamsForWebStage();
}

void wrap_setRenderParamsTime(double d)
{
    pxr::UsdTimeCode new_timecode(d);
    pxr::usdweb::renderParams.frame = new_timecode;
}

double wrap_getRenderParamsTime()
{
    return pxr::usdweb::renderParams.frame.GetValue();
}


void wrap_setRenderParamsEnableSceneMaterials(bool enable)
{
    pxr::usdweb::renderParams.enableSceneMaterials = enable;
}


void wrap_setCameraLightEnabled(bool enable)
{
    pxr::usdweb::cameraLightEnabled = enable;
    pxr::usdweb::_defaultLightingDirty = true;
}

void wrap_setDomeLightEnabled(bool enable)
{
    pxr::usdweb::domeLightEnabled = enable;
    pxr::usdweb::_defaultLightingDirty = true;
}

void wrap_setDomeLightTextureFile(const std::string& path)
{
    pxr::SdfAssetPath asset_path(path, path);
    bool orig_domeLightEnabled = pxr::usdweb::domeLightEnabled;
    pxr::usdweb::domeLightEnabled = false;
    pxr::usdweb::domeLight.SetDomeLightTextureFile(asset_path);
    pxr::usdweb::domeLightEnabled = orig_domeLightEnabled;
}

void wrap_setSceneLightsEnabled(bool enable)
{
    pxr::usdweb::renderParams.enableSceneLights = enable;
}


EMSCRIPTEN_BINDINGS(Usdweb) {
    emscripten::function("myInitialize", &pxr::usdweb::initialize);
    emscripten::function("UsdwebInit" , &wrap_ems_setup); // handled automatically in main()
    emscripten::function("UsdwebGetStage", &wrap_getStage);
    emscripten::function("UsdwebSetStage", &wrap_setStage);
    emscripten::function("UsdwebFitCamera", &pxr::usdweb::fit_camera);
    emscripten::function("UsdwebLookThroughUsdCamera", &pxr::usdweb::LookThroughUsdCamera);
    emscripten::function("UsdwebSetRenderParamsTime", &wrap_setRenderParamsTime);    
    emscripten::function("UsdwebGetRenderParamsTime", &wrap_getRenderParamsTime);    
    emscripten::function("UsdwebSetRenderParamsEnableSceneMaterials", &wrap_setRenderParamsEnableSceneMaterials);
    emscripten::function("UsdwebSetCameraLightEnabled", &wrap_setCameraLightEnabled);
    emscripten::function("UsdwebSetDomeLightEnabled",   &wrap_setDomeLightEnabled);    
    emscripten::function("UsdwebSetDomeLightTextureFile", &wrap_setDomeLightTextureFile);
    emscripten::function("UsdwebSetSceneLightsEnabled", &wrap_setSceneLightsEnabled);
}
