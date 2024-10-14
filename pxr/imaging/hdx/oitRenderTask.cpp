//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/oitRenderTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/oitBufferAccessor.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"

#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

static const HioGlslfxSharedPtr &
_GetRenderPassOitGlslfx()
{
    static const HioGlslfxSharedPtr glslfx =
        std::make_shared<HioGlslfx>(HdxPackageRenderPassOitShader());
    return glslfx;
}

static const HioGlslfxSharedPtr &
_GetRenderPassOitOpaqueGlslfx()
{
    static const HioGlslfxSharedPtr glslfx =
        std::make_shared<HioGlslfx>(HdxPackageRenderPassOitOpaqueShader());
    return glslfx;
}

HdxOitRenderTask::HdxOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitTranslucentRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            _GetRenderPassOitGlslfx()))
    , _oitOpaqueRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            _GetRenderPassOitOpaqueGlslfx()))
    , _isOitEnabled(HdxOitBufferAccessor::IsOitEnabled())
{
}

HdxOitRenderTask::~HdxOitRenderTask() = default;

void
HdxOitRenderTask::_Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_isOitEnabled) {
        if (!_translucentRenderPassState) {
            _translucentRenderPassState = std::make_shared<HdStRenderPassState>(
                    _oitTranslucentRenderPassShader);
        }

        if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
            HdxRenderTaskParams params;

            VtValue valueVt = delegate->Get(GetId(), HdTokens->params);
            if (valueVt.IsHolding<HdxRenderTaskParams>()) {
                params = valueVt.UncheckedGet<HdxRenderTaskParams>();
                HdxRenderSetupTask::SyncParamsHelper(_translucentRenderPassState, params);
            }

            HdxRenderTask::_Sync(delegate, ctx, dirtyBits);
            HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
            if (!TF_VERIFY(renderPassState)) return;

            auto extendedState =
                    dynamic_cast<HdStRenderPassState*>(renderPassState.get());
            if (!TF_VERIFY(extendedState, "OIT only works with HdSt")) {
                return;
            }

            {
                // blending is relevant only for the oitResolve task.
                extendedState->SetBlendEnabled(false);
                extendedState->SetAlphaToCoverageEnabled(false);
                extendedState->SetAlphaThreshold(0.f);
            }
            // We render into an SSBO -- not MSAA compatible
            renderPassState->SetMultiSampleEnabled(false);

            renderPassState->SetEnableDepthMask(true);
            renderPassState->SetColorMaskUseDefault(false);
            renderPassState->SetColorMasks({HdRenderPassState::ColorMaskRGBA});

            // Render pass state overrides
            _translucentRenderPassState->SetBlendEnabled(false);
            _translucentRenderPassState->SetAlphaToCoverageEnabled(false);
            _translucentRenderPassState->SetAlphaThreshold(renderPassState->GetAlphaThreshold());
            _translucentRenderPassState->SetMultiSampleEnabled(renderPassState->GetMultiSampleEnabled());
            _translucentRenderPassState->SetEnableDepthMask(false);
            _translucentRenderPassState->SetColorMaskUseDefault(false);
            _translucentRenderPassState->SetColorMasks({HdRenderPassState::ColorMaskNone});
        }

        if (!_translucentPass) {
            HdRenderIndex &renderIndex = delegate->GetRenderIndex();
            HdRenderDelegate *renderDelegate = renderIndex.GetRenderDelegate();
            VtValue val = delegate->Get(GetId(), HdTokens->collection);
            HdRprimCollection collection = val.Get<HdRprimCollection>();
            _translucentPass = renderDelegate->CreateRenderPass(&renderIndex, collection);
        }

        // sync render pass
        _translucentPass->Sync();
    }
}

void
HdxOitRenderTask::Prepare(HdTaskContext* ctx,
                          HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // OIT buffers take up significant GPU resources. Skip if there are no
    // oit draw items (i.e. no translucent draw items)
    if (_isOitEnabled && HdxRenderTask::_HasDrawItems()) {
        HdxRenderTask::Prepare(ctx, renderIndex);
        HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
        HdStRenderPassState* extendedState =
                dynamic_cast<HdStRenderPassState*>(renderPassState.get());
        extendedState->SetRenderPassShader(_oitOpaqueRenderPassShader);
        _translucentRenderPassState->SetViewport(extendedState->GetViewport());
        _translucentRenderPassState->SetCamera(extendedState->GetCamera());
        _translucentRenderPassState->SetOverrideWindowPolicy(extendedState->GetOverrideWindowPolicy());
        _translucentRenderPassState->SetFraming(extendedState->GetFraming());
        _translucentRenderPassState->SetAovBindings(renderPassState->GetAovBindings());
        _translucentRenderPassState->SetAovInputBindings(renderPassState->GetAovInputBindings());
        _translucentRenderPassState->Prepare(renderIndex->GetResourceRegistry());

        const HgiCapabilities* capabilities = _GetHgi()->GetCapabilities();
        if (!capabilities->IsSet(HgiDeviceCapabilitiesForceEarlyFragmentTest)) {
            // In case we don't have support for early fragment test, we need to skip the depth texture
            // for the second pass to avoid reading and writing to the same depth texture.
            auto noDepthAovs = renderPassState->GetAovBindings();
            noDepthAovs.erase(
                    std::remove_if(noDepthAovs.begin(), noDepthAovs.end(),
                                   [](const HdRenderPassAovBinding &aov) {
                                       return HdAovHasDepthSemantic(aov.aovName) ||
                                              HdAovHasDepthStencilSemantic(aov.aovName);
                                   }),
                    noDepthAovs.end());
            _translucentRenderPassState->SetAovBindings(noDepthAovs);
            // We need to bind the depth texture if the platform doesn't allow early fragment test to be forced.
            // This works in tandem with the TaskController setting the depth as input textures for the translucent task
            _oitTranslucentRenderPassShader->UpdateAovInputTextures(
                    _translucentRenderPassState->GetAovInputBindings(),
                    renderIndex);
        }

        HdxOitBufferAccessor(ctx).RequestOitBuffers();
    }
}

static
bool
_HasColorAov(HdRenderPassAovBindingVector const& aovBindings)
{
    return std::find_if(aovBindings.begin(), aovBindings.end(),
        [](HdRenderPassAovBinding const& binding){
            return binding.aovName == HdAovTokens->color; })
                != aovBindings.end();
}

void
HdxOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    GLF_GROUP_FUNCTION();

    if (!_isOitEnabled || !HdxRenderTask::_HasDrawItems()) {
        return;
    }

    if (!TF_VERIFY(_translucentRenderPassState)) return;

    HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
    if (!TF_VERIFY(renderPassState)) return;

    HdStRenderPassState* extendedState =
        dynamic_cast<HdStRenderPassState*>(renderPassState.get());
    if (!TF_VERIFY(extendedState, "OIT only works with HdSt")) {
        return;
    }
    
    // If there are aovs, but none of them are color, skip rendering for this 
    // task.
    HdRenderPassAovBindingVector const& aovBindings =
        renderPassState->GetAovBindings();
    if (!aovBindings.empty() && !_HasColorAov(aovBindings)) {
        return;
    }

    //
    // Pre Execute Setup
    //
    {
        HdxOitBufferAccessor oitBufferAccessor(ctx);

        oitBufferAccessor.RequestOitBuffers();
        oitBufferAccessor.InitializeOitBuffersIfNecessary(_GetHgi());
        if (!oitBufferAccessor.AddOitBufferBindings(
                _oitTranslucentRenderPassShader)) {
            TF_CODING_ERROR(
                "No OIT buffers allocated but needed by OIT render task");
            return;
        }
    }

    //
    // 1. Opaque pixels pass
    //
    // Fragments that are opaque (alpha > 1.0) are rendered to the active
    // framebuffer. Translucent fragments are discarded.
    // This can reduce the data written to the OIT SSBO buffers because of
    // improved depth testing.
    //
    {
        HdxRenderTask::Execute(ctx);
    }

    //
    // 2. Translucent pixels pass
    //
    // Fill OIT SSBO buffers for the translucent fragments.
    //
    {
        auto extendedState =
                dynamic_cast<HdStRenderPassState*>(_translucentRenderPassState.get());
        HdxRenderTask::_SetHdStRenderPassState(ctx, extendedState);
        _translucentPass->Execute(_translucentRenderPassState, GetRenderTags());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
