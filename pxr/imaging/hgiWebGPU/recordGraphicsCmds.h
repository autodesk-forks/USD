//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_RECORD_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGI_WEBGPU_RECORD_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/stepFunctions.h"
#include "pxr/imaging/hgiWebGPU/baseGraphicsCmds.h"
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;
class HgiWebGPUGraphicsPipeline;

/// \class HgiWebGPUGraphicsCmds
///
/// WebGPU implementation of HgiGraphicsEncoder.
///
class HgiWebGPURecordGraphicsCmds final : public HgiWebGPUBaseGraphicsCmds<wgpu::RenderBundleEncoder>
{
public:
    HGIWEBGPU_API
    ~HgiWebGPURecordGraphicsCmds() override;

    HGIWEBGPU_API
    void SetViewport(GfVec4i const& vp) override;

    HGIWEBGPU_API
    void SetScissor(GfVec4i const& sc) override;

    HGIWEBGPU_API
    bool IsReady();

    HGIWEBGPU_API
    wgpu::RenderBundle GetPreRecordedCmds() const;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPURecordGraphicsCmds(
        HgiWebGPU* hgi,
        HgiGraphicsCmdsDesc const& desc);

private:
    HgiWebGPURecordGraphicsCmds() = delete;
    HgiWebGPURecordGraphicsCmds & operator=(const HgiWebGPURecordGraphicsCmds&) = delete;
    HgiWebGPURecordGraphicsCmds(const HgiWebGPURecordGraphicsCmds&) = delete;

    void _EndRenderPass() override;

    wgpu::RenderBundle _renderBundle;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
