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
#ifndef PXR_IMAGING_HGI_WEBGPU_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGI_WEBGPU_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiWebGPU/api.h"
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
class HgiWebGPUGraphicsCmds final : public HgiWebGPUBaseGraphicsCmds<wgpu::RenderPassEncoder>
{
public:
    HGIWEBGPU_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override {}

    HGIWEBGPU_API
    void SetViewport(GfVec4i const& vp) override;

    HGIWEBGPU_API
    void SetScissor(GfVec4i const& sc) override;

    HGIWEBGPU_API
    void ExecutePrerecordedCmds(HgiCmds* cmds) override;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUGraphicsCmds(
        HgiWebGPU* hgi,
        HgiGraphicsCmdsDesc const& desc);

    HGIWEBGPU_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiWebGPUGraphicsCmds() = delete;
    HgiWebGPUGraphicsCmds & operator=(const HgiWebGPUGraphicsCmds&) = delete;
    HgiWebGPUGraphicsCmds(const HgiWebGPUGraphicsCmds&) = delete;

    void _EndRenderPass() override;
    bool _IsTimestampsEnabled();

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
