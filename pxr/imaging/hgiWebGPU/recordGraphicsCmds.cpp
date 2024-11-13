//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"
#include "pxr/imaging/hgiWebGPU/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/recordGraphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/baseGraphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPURecordGraphicsCmds::HgiWebGPURecordGraphicsCmds(
        HgiWebGPU *hgi,
        HgiGraphicsCmdsDesc const &desc)
        : HgiWebGPUBaseGraphicsCmds<wgpu::RenderBundleEncoder>(hgi, desc) {
    wgpu::RenderBundleEncoderDescriptor renderBundle;
    std::vector<wgpu::TextureFormat> colorFormats;
    for (const auto &colorAttachmentDesc: _descriptor.colorAttachmentDescs) {
        colorFormats.push_back(HgiWebGPUConversions::GetPixelFormat(colorAttachmentDesc.format));
    }
    renderBundle.colorFormats = colorFormats.data();
    renderBundle.colorFormatCount = colorFormats.size();

    auto depthTarget = dynamic_cast<HgiWebGPUTexture *>(_descriptor.depthTexture.Get());
    if (depthTarget) {
        renderBundle.depthStencilFormat = HgiWebGPUConversions::GetDepthOrStencilTextureFormat(
                _descriptor.depthAttachmentDesc.usage, _descriptor.depthAttachmentDesc.format);
    }
    auto colorTarget = dynamic_cast<HgiWebGPUTexture *>(_descriptor.colorTextures.front().Get());
    renderBundle.sampleCount = colorTarget->GetDescriptor().sampleCount;
    wgpu::Device device = _hgi->GetPrimaryDevice();
    _passEncoder = device.CreateRenderBundleEncoder(&renderBundle);
}

HgiWebGPURecordGraphicsCmds::~HgiWebGPURecordGraphicsCmds() {
    _commandBuffer = nullptr;
}

void
HgiWebGPURecordGraphicsCmds::SetViewport(GfVec4i const &vp) {
    TF_WARN("SetViewport not implemented for secondary command buffers");
}

void
HgiWebGPURecordGraphicsCmds::SetScissor(GfVec4i const &sc) {
    TF_WARN("SetViewport not implemented for secondary command buffers");
}


void
HgiWebGPURecordGraphicsCmds::_EndRenderPass() {
    if (_renderPassStarted) {
        if (!_renderBundle) {
            // release any resources
            _lastIndexBuffer = nullptr;

            if (_hasWork) {
                auto depthTarget = dynamic_cast<HgiWebGPUTexture *>(_descriptor.depthTexture.Get());
                if (depthTarget) {
                    HgiTextureDesc depthDesc = depthTarget->GetDescriptor();
                    if (depthDesc.sampleCount > 1 && _descriptor.depthResolveTexture) {
                        auto depthResolveTarget = dynamic_cast<HgiWebGPUTexture *>(_descriptor.depthResolveTexture.Get());
                        _hgi->ResolveDepth(_commandEncoder, *depthTarget, *depthResolveTarget);
                    }
                }
            }

            _renderBundle = _passEncoder.Finish();
            _passEncoder = nullptr;
        }
    }
}

bool HgiWebGPURecordGraphicsCmds::IsReady() {
    return !!_renderBundle;
}

wgpu::RenderBundle HgiWebGPURecordGraphicsCmds::GetPreRecordedCmds() const {
    return _renderBundle;
}

PXR_NAMESPACE_CLOSE_SCOPE
