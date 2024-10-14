//
// Copyright 2022 Pixar
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
#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/blitCmds.h"
#include "pxr/imaging/hgiWebGPU/computeCmds.h"
#include "pxr/imaging/hgiWebGPU/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/graphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/sampler.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/shaderProgram.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include <algorithm>

#if defined EMSCRIPTEN
#include <emscripten/html5_webgpu.h>
#else
#if defined _WIN32 && !defined WIN32_VULKAN
#define DAWN_ENABLE_BACKEND_D3D12
#elif defined(ARCH_OS_DARWIN)
#define DAWN_ENABLE_BACKEND_METAL
#else
#define DAWN_ENABLE_BACKEND_VULKAN
#endif

#include <dawn/dawn_proc.h>
#include <dawn/webgpu_cpp.h>
#include <dawn/native/NullBackend.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<HgiWebGPU, TfType::Bases<Hgi> >();
    t.SetFactory<HgiFactory<HgiWebGPU>>();
}

// GetDevice code based on https://github.com/kainino0x/webgpu-cross-platform-demo/blob/main/main.cpp
#if defined EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>

wgpu::Device GetDevice() {
    WGPUDevice deviceImp = emscripten_webgpu_get_device();
    return wgpu::Device::Acquire(deviceImp);
}
#else
#include <dawn/native/DawnNative.h>

    void PrintDeviceError(WGPUErrorType errorType, const char* message, void*) {
        std::string errorTypeName = "";
        switch (errorType) {
            case WGPUErrorType_Validation:
                errorTypeName = "Validation";
                break;
            case WGPUErrorType_OutOfMemory:
                errorTypeName = "Out of memory";
                break;
            case WGPUErrorType_Unknown:
                errorTypeName = "Unknown";
                break;
            case WGPUErrorType_DeviceLost:
                errorTypeName = "Device lost";
                break;
            default:
                return;
        }
        TF_CODING_ERROR(errorTypeName + " error: " + message);
    }
    static std::unique_ptr<dawn::native::Instance> instance;

    wgpu::Device GetDevice() {
        if (!instance) {
            instance = std::make_unique<dawn::native::Instance>();
        }

        // Simply pick the first adapter in the sorted list.
        dawn::native::Adapter backendAdapter = instance->EnumerateAdapters()[0];

        wgpu::DeviceDescriptor descriptor;
        std::vector<wgpu::FeatureName> requiredFeatures = {
                wgpu::FeatureName::Depth32FloatStencil8,
                wgpu::FeatureName::Float32Filterable
        };

        if (TfDebug::IsEnabled(HGIWEBGPU_DEBUG_TIMESTAMPS)) {
            requiredFeatures.push_back(wgpu::FeatureName::TimestampQuery);
        }

        #ifndef EMSCRIPTEN
            // toggles are handled by chrome itself, so we only enable it for the desktop version where we have direct
            // control
            wgpu::DawnTogglesDescriptor deviceTogglesDesc;
            // Toggle for debugging shader
            std::vector<const char *> enabledToggles = {};
            if (TfDebug::IsEnabled(HGIWEBGPU_DEBUG_SHADER_CODE)) {
                enabledToggles.push_back("dump_shaders");
                enabledToggles.push_back("disable_symbol_renaming");
            }
            deviceTogglesDesc.enabledToggles = enabledToggles.data();
            deviceTogglesDesc.enabledToggleCount = enabledToggles.size();
            descriptor.nextInChain = &deviceTogglesDesc;

        #endif

        WGPUSupportedLimits supportedLimits = {};
        backendAdapter.GetLimits(&supportedLimits);
        backendAdapter.SetUseTieredLimits(true);

        // If the requirements are not met, dawn will throw a warning
        wgpu::RequiredLimits limits = {};
        limits.limits.maxStorageBuffersPerShaderStage = 10;
        limits.limits.maxColorAttachmentBytesPerSample = 64;
        limits.limits.maxBufferSize = 0x40000000;
        descriptor.requiredLimits = &limits;

        descriptor.requiredFeatures = requiredFeatures.data();
        descriptor.requiredFeatureCount = requiredFeatures.size();

        WGPUDevice cDevice = backendAdapter.CreateDevice(&descriptor);
        wgpu::Device device = wgpu::Device::Acquire(cDevice);
        DawnProcTable procs = dawn::native::GetProcs();

        dawnProcSetProcs(&procs);
        procs.deviceSetUncapturedErrorCallback(cDevice, PrintDeviceError, nullptr);
        return device;
    }
#endif  // __EMSCRIPTEN__

HgiWebGPU::HgiWebGPU()
: _device(GetDevice())
, _currentCmds(nullptr)
,_depthResolver(_device)
,_mipmapGenerator(_device)
, _workToFlush(false)

{
    // get the default command queue
    _commandQueue = _device.GetQueue();

    _capabilities.reset(new HgiWebGPUCapabilities(_device));
}

HgiWebGPU::~HgiWebGPU()
{
    _PerformGarbageCollection();
}

bool
HgiWebGPU::IsBackendSupported() const
{
    return true;
}

wgpu::Device
HgiWebGPU::GetPrimaryDevice() const
{
    return _device;
}

HgiGraphicsCmdsUniquePtr
HgiWebGPU::CreateGraphicsCmds(
    HgiGraphicsCmdsDesc const& desc)
{
    HgiWebGPUGraphicsCmds* gfxCmds(new HgiWebGPUGraphicsCmds(this, desc));
    return HgiGraphicsCmdsUniquePtr(gfxCmds);
}

HgiComputeCmdsUniquePtr
HgiWebGPU::CreateComputeCmds(HgiComputeCmdsDesc const& desc)
{
    HgiWebGPUComputeCmds* computeCmds = new HgiWebGPUComputeCmds(this, desc);
    if (!_currentCmds) {
        _currentCmds = computeCmds;
    }
    return HgiComputeCmdsUniquePtr(computeCmds);
}

HgiBlitCmdsUniquePtr
HgiWebGPU::CreateBlitCmds()
{
    HgiWebGPUBlitCmds* blitCmds = new HgiWebGPUBlitCmds(this);
    if (!_currentCmds) {
        _currentCmds = blitCmds;
    }
    return HgiBlitCmdsUniquePtr(blitCmds);
}

HgiTextureHandle
HgiWebGPU::CreateTexture(HgiTextureDesc const & desc)
{
    return HgiTextureHandle(new HgiWebGPUTexture(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyTexture(HgiTextureHandle* texHandle)
{
    _TrashObject(texHandle);
}

HgiTextureViewHandle
HgiWebGPU::CreateTextureView(HgiTextureViewDesc const & desc)
{
    if (!desc.sourceTexture) {
        TF_CODING_ERROR("Source texture is null");
    }

    HgiTextureHandle src =
        HgiTextureHandle(new HgiWebGPUTexture(this, desc), GetUniqueId());
    HgiTextureView* view = new HgiTextureView(desc);
    view->SetViewTexture(src);
    return HgiTextureViewHandle(view, GetUniqueId());
}

void
HgiWebGPU::DestroyTextureView(HgiTextureViewHandle *viewHandle) {
    HgiTextureHandle texHandle = (*viewHandle)->GetViewTexture();
    if (_workToFlush) {
        _garbageCollectionHandlers.emplace_back([texHandle] {
            delete texHandle.Get();
        });
    } else {
        _TrashObject(&texHandle);
    }
    (*viewHandle)->SetViewTexture(HgiTextureHandle());
    delete viewHandle->Get();
    *viewHandle = HgiTextureViewHandle();
}

HgiSamplerHandle
HgiWebGPU::CreateSampler(HgiSamplerDesc const & desc)
{
    return HgiSamplerHandle(new HgiWebGPUSampler(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroySampler(HgiSamplerHandle* smpHandle)
{
    _TrashObject(smpHandle);
}

HgiBufferHandle
HgiWebGPU::CreateBuffer(HgiBufferDesc const & desc)
{
    return HgiBufferHandle(new HgiWebGPUBuffer(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyBuffer(HgiBufferHandle* bufHandle)
{
    _TrashObject(bufHandle);
}

HgiShaderFunctionHandle
HgiWebGPU::CreateShaderFunction(HgiShaderFunctionDesc const& desc)
{
    return HgiShaderFunctionHandle(
        new HgiWebGPUShaderFunction(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyShaderFunction(HgiShaderFunctionHandle* shaderFunctionHandle)
{
    _TrashObject(shaderFunctionHandle);
}

HgiShaderProgramHandle
HgiWebGPU::CreateShaderProgram(HgiShaderProgramDesc const& desc)
{
    return HgiShaderProgramHandle(
        new HgiWebGPUShaderProgram(desc), GetUniqueId());
}

void
HgiWebGPU::DestroyShaderProgram(HgiShaderProgramHandle* shaderProgramHandle)
{
    _TrashObject(shaderProgramHandle);
}


HgiResourceBindingsHandle
HgiWebGPU::CreateResourceBindings(HgiResourceBindingsDesc const& desc)
{
    return HgiResourceBindingsHandle(
        new HgiWebGPUResourceBindings(desc), GetUniqueId());
}

void
HgiWebGPU::DestroyResourceBindings(HgiResourceBindingsHandle* resHandle)
{
    _TrashObject(resHandle);
}

HgiGraphicsPipelineHandle
HgiWebGPU::CreateGraphicsPipeline(HgiGraphicsPipelineDesc const& desc)
{
    return HgiGraphicsPipelineHandle(
        new HgiWebGPUGraphicsPipeline(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyGraphicsPipeline(HgiGraphicsPipelineHandle* pipeHandle)
{
    _TrashObject(pipeHandle);
}

HgiComputePipelineHandle
HgiWebGPU::CreateComputePipeline(HgiComputePipelineDesc const& desc)
{
    return HgiComputePipelineHandle(
        new HgiWebGPUComputePipeline(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle)
{
    _TrashObject(pipeHandle);
}

TfToken const&
HgiWebGPU::GetAPIName() const {
    return HgiTokens->WebGPU;
}

HgiWebGPUCapabilities const*
HgiWebGPU::GetCapabilities() const
{
    return _capabilities.get();
}

HgiIndirectCommandEncoder*
HgiWebGPU::GetIndirectCommandEncoder() const
{
    return nullptr;
}

void
HgiWebGPU::StartFrame()
{

}

void
HgiWebGPU::EndFrame()
{
#ifndef EMSCRIPTEN
    dawn::native::InstanceProcessEvents(instance->Get());
#endif
}

wgpu::Queue
HgiWebGPU::GetQueue() const
{
    return _commandQueue;
}

void
HgiWebGPU::EnqueueCommandBuffer(wgpu::CommandBuffer const &commandBuffer)
{
    if (commandBuffer) {
        _commandBuffers.push_back(commandBuffer);
    }
}
#if !defined(EMSCRIPTEN)
void
HgiWebGPU::QueryValue()
{
    if (_inflightQuery->resultBuffer.GetMapState() == wgpu::BufferMapState::Unmapped) {
        std::shared_ptr<uint64_t> idPtr = std::make_shared<uint64_t>(0);
        auto future = _inflightQuery->resultBuffer.MapAsync(
                wgpu::MapMode::Read,
                0,
                _inflightQuery->resultBuffer.GetSize(),
                wgpu::CallbackMode::AllowProcessEvents,
                [this,idPtr](wgpu::MapAsyncStatus status, char const * message) {
                    uint64_t id = *idPtr;
                    if (status != wgpu::MapAsyncStatus::Success) {
                        TF_WARN("Failed to call MapAsync for query ");
                        _pendingQueries.erase(id);
                        return;
                    }


                    if (_pendingQueries.count(id) > 0) {
                        std::vector<uint64_t> times(2);
                        memcpy(times.data(),
                               _pendingQueries[id].resultBuffer.GetConstMappedRange(),
                               _pendingQueries[id].resultBuffer.GetSize());
                        uint64_t nanoseconds = (times[1] - times[0]);
                        float milliseconds = (float) nanoseconds * 1e-6;
                        TF_STATUS(_pendingQueries[id].label + " took: " + std::to_string(milliseconds) + "ms");
                        _pendingQueries[id].resultBuffer.Unmap();
                        _availableQueries.push_back(_pendingQueries[id]);
                        _pendingQueries.erase(id);
                    } else {
                        TF_RUNTIME_ERROR("Failed to find pending query");
                    }
                });
        _pendingQueries[future.id] = std::move(*_inflightQuery);
        _pendingQueries[future.id].id = idPtr;
        *idPtr = future.id;
        _inflightQuery = nullptr;
    }
}

    QueryFrame HgiWebGPU::_CreateQueryObjects() {
        QueryFrame queryFrame{};
        const int capacity = 2; // Max number of timestamps we can store

        wgpu::QuerySetDescriptor querySetDescriptor;
        querySetDescriptor.count = capacity;
        querySetDescriptor.type = wgpu::QueryType::Timestamp;
        queryFrame.querySet = _device.CreateQuerySet(&querySetDescriptor);

        {
            wgpu::BufferDescriptor bufferDescriptor;
            bufferDescriptor.size = capacity * sizeof(uint64_t);
            bufferDescriptor.label = ("queryResolve" + std::to_string(queryFrameCounter)).c_str();
            bufferDescriptor.usage =
                    wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc;
            queryFrame.resolveBuffer =
                    _device.CreateBuffer(&bufferDescriptor);
        }

        {
            wgpu::BufferDescriptor bufferDescriptor;
            bufferDescriptor.size = capacity * sizeof(uint64_t);
            bufferDescriptor.label = ("queryResult" + std::to_string(queryFrameCounter++)).c_str();
            bufferDescriptor.usage =
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
            queryFrame.resultBuffer =
                    _device.CreateBuffer(&bufferDescriptor);
        }
        return queryFrame;
    }

    void HgiWebGPU::ResolveQuery(wgpu::CommandEncoder &commandEncoder, const std::string &label) {

        commandEncoder.ResolveQuerySet(
                _inflightQuery->querySet,
                0,
                _inflightQuery->querySet.GetCount(),
                _inflightQuery->resolveBuffer,
                0);

        if (_inflightQuery->resultBuffer.GetMapState() == wgpu::BufferMapState::Unmapped) {
            commandEncoder.CopyBufferToBuffer(_inflightQuery->resolveBuffer,
                                              0,
                                              _inflightQuery->resultBuffer,
                                              0,
                                              _inflightQuery->resolveBuffer.GetSize());
        }
        _inflightQuery->label = label;
    }


    void HgiWebGPU::_ProcessNextInflightQuery() {
        // There could be an empty graphics cmds that request a query but never submits work to the queue.
        // In these cases, we want to reuse the current _inflightQuery
        if (!_inflightQuery) {
            if (!_availableQueries.empty()) {
                _inflightQuery = std::make_shared<QueryFrame>(std::move(_availableQueries.back()));
                _availableQueries.pop_back();
            } else {
                _inflightQuery = std::make_shared<QueryFrame>(_CreateQueryObjects());
            }
        }
    }

    wgpu::RenderPassTimestampWrites HgiWebGPU::GetRenderTimestampWrites() {
        _ProcessNextInflightQuery();

        wgpu::RenderPassTimestampWrites timestampWrites;
        timestampWrites.querySet = _inflightQuery->querySet;
        timestampWrites.beginningOfPassWriteIndex = 0;
        timestampWrites.endOfPassWriteIndex = 1;

        return timestampWrites;
    }
#endif

void
HgiWebGPU::QueueSubmit()
{
    if(!_commandBuffers.empty())
    {
        _commandQueue.Submit(_commandBuffers.size(), _commandBuffers.data());
        _commandBuffers.clear();
    }
}

int
HgiWebGPU::GetAPIVersion() const
{
    return GetCapabilities()->GetAPIVersion();
}

void
HgiWebGPU::_PerformGarbageCollection() {
    for (auto const& fn : _garbageCollectionHandlers)
        fn();

    _garbageCollectionHandlers.clear();
}

bool
HgiWebGPU::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
    TRACE_FUNCTION();

    if (cmds) {
        _workToFlush = Hgi::_SubmitCmds(cmds, wait);
        if (_workToFlush) {
            _PerformGarbageCollection();
        }
        if (cmds == _currentCmds) {
            _currentCmds = nullptr;
        }
    }

    return _workToFlush;
}

wgpu::Texture HgiWebGPU::GenerateMipmap(const wgpu::Texture& texture, const HgiTextureDesc& textureDescriptor) {
    return _mipmapGenerator.generateMipmap(texture, textureDescriptor);
}

void HgiWebGPU::ResolveDepth(wgpu::CommandEncoder const &commandEncoder, HgiWebGPUTexture &sourceTexture,
                  HgiWebGPUTexture &destinationTexture) {
        _depthResolver.resolveDepth(commandEncoder, sourceTexture, destinationTexture);
}

PXR_NAMESPACE_CLOSE_SCOPE
