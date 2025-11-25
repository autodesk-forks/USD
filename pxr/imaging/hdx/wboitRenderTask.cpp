//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/wboitRenderTask.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

static const HioGlslfxSharedPtr &
_GetRenderPassWbOitGlslfx()
{
    static const HioGlslfxSharedPtr glslfx =
        std::make_shared<HioGlslfx>(HdxPackageRenderPassWbOitShader());
    return glslfx;
}

TF_DEFINE_ENV_SETTING(HDX_ENABLE_WBOIT, false, 
                      "Enable weighted blended order independent transparency");

/* static */
bool
HdxWbOitRenderTask::IsEnabled()
{
    return TfGetEnvSetting(HDX_ENABLE_WBOIT);
}

HdxWbOitRenderTask::HdxWbOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _wboitTranslucentRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            _GetRenderPassWbOitGlslfx()))
{
}

HdxWbOitRenderTask::~HdxWbOitRenderTask() {
    if (_index) {
        HdRenderParam * renderParam =
                                _index->GetRenderDelegate()->GetRenderParam();
        for (auto const& aovBuffer : _wboitBuffers) {
            aovBuffer->Finalize(renderParam);
        }
    }
    _wboitBuffers.clear();
    _wboitAovBindings.clear();
};

void
HdxWbOitRenderTask::_Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxRenderTask::_Sync(delegate, ctx, dirtyBits);

        HdRenderPassStateSharedPtr renderPassState =
            _GetRenderPassState(ctx);
        if (!TF_VERIFY(renderPassState)) return;

        HdStRenderPassState* extendedState =
            dynamic_cast<HdStRenderPassState*>(renderPassState.get());
        if (!TF_VERIFY(extendedState, "WBOIT only works with HdSt")) {
            return;
        }

        // Render pass state overrides
        {   
            // Currently we only support non-multisampled buffers
            renderPassState->SetMultiSampleEnabled(false);

            // blending needs to be set
            extendedState->SetBlendEnabled(true);
            renderPassState->SetBlend(
                HdBlendOp::HdBlendOpAdd,
                HdBlendFactor::HdBlendFactorOne,
                HdBlendFactor::HdBlendFactorOne,
                HdBlendOp::HdBlendOpAdd,
                HdBlendFactor::HdBlendFactorZero,
                HdBlendFactor::HdBlendFactorOneMinusSrcAlpha
            );
            extendedState->SetAlphaToCoverageEnabled(false);
            extendedState->SetAlphaThreshold(0.f);
            renderPassState->SetEnableDepthTest(true);
            renderPassState->SetEnableDepthMask(false);
            renderPassState->SetColorMaskUseDefault(false);
            renderPassState->SetColorMasks({HdRenderPassState::ColorMaskRGBA});
        }
    }
}

void
HdxWbOitRenderTask::Prepare(HdTaskContext* ctx,
                          HdRenderIndex* renderIndex)
{
    _index = renderIndex;
    if (HdxRenderTask::_HasDrawItems()) {
        HdxRenderTask::Prepare(ctx, renderIndex);

        // Set the request flag so we know to execute the resolve task
        // The request flag is cleared in the resolve task
        (*ctx)[HdxTokens->oitRequestFlag] = VtValue(true);

        HdRenderPassStateSharedPtr renderPassState =
            _GetRenderPassState(ctx);
        if (!TF_VERIFY(renderPassState)) return;
        HdStRenderPassState* extendedState =
            dynamic_cast<HdStRenderPassState*>(renderPassState.get());

        extendedState->SetRenderPassShader(_wboitTranslucentRenderPassShader);
        // Initialize the render buffers and setup bindings
        InitTextures(ctx, renderPassState);
        renderPassState->SetAovBindings(_wboitAovBindings);
        auto width = _wboitAovBindings.front().renderBuffer->GetWidth();
        auto height = _wboitAovBindings.front().renderBuffer->GetHeight();
        renderPassState->SetViewport(GfVec4d(0, 0, width, height));
    }
}

void
HdxWbOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    GLF_GROUP_FUNCTION();
    
    // Skip if there are not oit draw items (i.e. no translucent draw items) to save resources
    if (!HdxRenderTask::_HasDrawItems()) {
        return;
    }
       
    HdxRenderTask::Execute(ctx);
}

// Init the render buffer resources for WbOit
bool
HdxWbOitRenderTask::InitTextures(HdTaskContext* ctx, const HdRenderPassStateSharedPtr& renderPassState) {
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Get current renderPassState aov bindings and extract format and dimensions from the color buffer
    auto aovBindings = renderPassState->GetAovBindings();
    if (aovBindings.empty()) {
        TF_WARN("No AOV bindings found for WbOit render task");
        return false;
    }

    const bool createOitBuffers = _wboitBuffers.empty();

    // if the aov bindings haven't changed we don't need to reallocate the render buffers
    if (!createOitBuffers && (aovBindings.front() == _wboitAovBindings.front())) {
        return false;
    }

    // We assume the first aov binding is the color buffer
    auto colorRenderBuffer = static_cast<HdStRenderBuffer*>(aovBindings.front().renderBuffer);
    GfVec2i dimensions = GfVec2i(colorRenderBuffer->GetWidth(), colorRenderBuffer->GetHeight());
    bool isMultiSampled = colorRenderBuffer->IsMultiSampled();
    
    // force to false for now
    isMultiSampled = false;
    
    const static TfTokenVector aovOutputs = {
        HdxTokens->hdxWboitBufferOne,
        HdxTokens->hdxWboitBufferTwo,
    };

    if (createOitBuffers) {
        HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            _index->GetResourceRegistry());

        // Add the new render buffers.
        // We need two buffers for WbOit: one for the accumulated color and the total transmittance and the other for the weights.
        // The first buffer is a storing the accumulated colors and the total transmittance in a rgbafloat16 format.
        // The second buffer is storing the accumulated depth weights in a float16 format.
        for (size_t i = 0; i < aovOutputs.size(); ++i) {
            TfToken const & aovOutput = aovOutputs[i];
            SdfPath const aovId = SdfPath("wboitBuffer" + std::to_string(i));

            _wboitBuffers.push_back(
                std::make_unique<HdStRenderBuffer>(
                    hdStResourceRegistry.get(), aovId));

            HdFormat format = (aovOutput == HdxTokens->hdxWboitBufferOne) ? HdFormatFloat16Vec4 : HdFormatFloat16;
            HdAovDescriptor aovDesc = HdAovDescriptor(format, isMultiSampled, VtValue(GfVec4f(0, 0, 0, 1)));
            // Convert to a binding.
            HdRenderPassAovBinding binding;
            binding.aovName = aovOutput;
            binding.renderBufferId = aovId;
            binding.aovSettings = aovDesc.aovSettings;
            binding.renderBuffer = _wboitBuffers.back().get();
            binding.clearValue = VtValue(GfVec4f(0, 0, 0, 1));

            _wboitAovBindings.push_back(binding);
        }

        // find and bind the depth buffer that get's set for the render task
        auto depthBufferBinding = std::find_if(aovBindings.begin(), aovBindings.end(), 
            [](const HdRenderPassAovBinding& aovBinding) {
                return HdAovHasDepthSemantic(aovBinding.aovName) || HdAovHasDepthStencilSemantic(aovBinding.aovName);
            }
        );
        if (depthBufferBinding != aovBindings.end()) {
            _wboitAovBindings.push_back(*depthBufferBinding);
        } else {
            TF_WARN("No depth buffer found for WbOit render task");
        }
    }

    // Check if the render buffers need to be reallocated this might happen if the resolution changes
    VtValue existingResource = _wboitAovBindings.front().renderBuffer->GetResource(false);
    if (existingResource.IsHolding<HgiTextureHandle>()) {
        int32_t width = _wboitAovBindings.front().renderBuffer->GetWidth();
        int32_t height = _wboitAovBindings.front().renderBuffer->GetHeight();
        if (width == dimensions[0] && height == dimensions[1]) {
            return false;
        }
    }
    // (Re)allocate the render buffers
    for (size_t i = 0; i < aovOutputs.size(); ++i) {
        HdRenderPassAovBinding const & aovBinding = _wboitAovBindings[i];
        HdFormat format = (aovBinding.aovName == HdxTokens->hdxWboitBufferOne) ? HdFormatFloat16Vec4 : HdFormatFloat16;
        aovBinding.renderBuffer->Allocate(GfVec3i(dimensions[0], dimensions[1], 1), format, isMultiSampled);
        // Set the resource in the task context for later use
        (*ctx)[aovOutputs[i]] = aovBinding.renderBuffer->GetResource(false);;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
