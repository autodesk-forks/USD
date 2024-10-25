//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/wboitResolveTask.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/glf/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((shader, "WBOIT_Resolve::Fragment")));

HdxWbOitResolveTask::HdxWbOitResolveTask(HdSceneDelegate*, SdfPath const& id) : HdxTask(id) {}

HdxWbOitResolveTask::~HdxWbOitResolveTask() {}

void HdxWbOitResolveTask::_Sync(HdSceneDelegate* delegate, HdTaskContext* ctx, HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_shader)
    {
        _shader = std::make_unique<HdxFullscreenShader>(_GetHgi(), "WBOIT Resolve Shader");

        HgiShaderFunctionDesc shaderDesc;
        shaderDesc.debugName   = _tokens->shader.GetString();
        shaderDesc.shaderStage = HgiShaderStageFragment;
        HgiShaderFunctionAddStageInput(&shaderDesc, "uvOut", "vec2");
        HgiShaderFunctionAddTexture(&shaderDesc, "colorIn", 0);
        // accumulated color and total transmittance
        HgiShaderFunctionAddTexture(&shaderDesc, "buffer0", 1);
        // depth weights
        HgiShaderFunctionAddTexture(&shaderDesc, "buffer1", 2);
        HgiShaderFunctionAddStageOutput(&shaderDesc, "hd_FragColor", "vec4", "color");

        _shader->SetProgram(HdxPackageWbOitResolveShader(),
            _tokens->shader, shaderDesc);
    }
}

void HdxWbOitResolveTask::Prepare(HdTaskContext*, HdRenderIndex*) {}

void HdxWbOitResolveTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Check whether the request flag was set and delete it so that for the
    // next iteration the request flag is not set unless an OIT render task
    // explicitly sets it.
    // This avoids unnecessary resolve tasks to be executed if no OIT render task was executed.
    if (ctx->erase(HdxTokens->oitRequestFlag) == 0) {
        return;
    }

    HgiTextureHandle aovTexture;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);
    HgiTextureHandle aovTextureIntermediate;
    _GetTaskContextData(ctx, HdxAovTokens->colorIntermediate, &aovTextureIntermediate);

    HgiTextureHandle buffer0, buffer1;
    _GetTaskContextData(ctx, HdxTokens->hdxWboitBufferOne, &buffer0);
    _GetTaskContextData(ctx, HdxTokens->hdxWboitBufferTwo, &buffer1);

    if (!buffer0) {
        return;
    }

    aovTexture->SubmitLayoutChange(HgiTextureUsageBitsShaderRead);
    buffer0->SubmitLayoutChange(HgiTextureUsageBitsShaderRead);
    buffer1->SubmitLayoutChange(HgiTextureUsageBitsShaderRead);

    _shader->BindTextures({ aovTexture, buffer0, buffer1 });
    // TODO: Use blending to composite the transparency over the background. 
    // currently this causes issues on the WebGpu backend due to flipping problems.
    // The transparency buffer would be composited y flipped over the background.
    // _shader->SetBlendState(
    //     true,
    //     HgiBlendFactor::HgiBlendFactorOneMinusSrcAlpha,
    //     HgiBlendFactor::HgiBlendFactorSrcAlpha,
    //     HgiBlendOp::HgiBlendOpAdd,
    //     HgiBlendFactor::HgiBlendFactorOne,
    //     HgiBlendFactor::HgiBlendFactorOneMinusSrcAlpha,
    //     HgiBlendOp::HgiBlendOpAdd
    // );
    // _shader->SetAttachmentLoadStoreOp(HgiAttachmentLoadOp::HgiAttachmentLoadOpLoad, HgiAttachmentStoreOp::HgiAttachmentStoreOpStore);
    _shader->Draw(aovTextureIntermediate, HgiTextureHandle());

    aovTexture->SubmitLayoutChange(HgiTextureUsageBitsColorTarget);
    buffer0->SubmitLayoutChange(HgiTextureUsageBitsColorTarget);
    buffer1->SubmitLayoutChange(HgiTextureUsageBitsColorTarget);

    _ToggleRenderTarget(ctx);
}

PXR_NAMESPACE_CLOSE_SCOPE
