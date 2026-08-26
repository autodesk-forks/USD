//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HGIMETAL_RAYTRACING_CMDS_H
#define PXR_IMAGING_HGIMETAL_RAYTRACING_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/rayTracingCmds.h"
#include "pxr/imaging/hgi/accelerationStructure.h"
#include "pxr/imaging/hgiMetal/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiMetal;
class HgiMetalCommandBuffer;
class HgiMetalRayTracingPipeline;


/// \class HgiMetalRayTracingCmds
///
/// OpenGL implementation of HgiRayTracingCmds.
///
class HgiMetalRayTracingCmds final : public HgiRayTracingCmds
{
public:
    HGIMETAL_API
    ~HgiMetalRayTracingCmds() override;

    HGIMETAL_API
    void PushDebugGroup(const char* label) override;

    HGIMETAL_API
    void PopDebugGroup() override;

    HGIMETAL_API
    void BindPipeline(HgiRayTracingPipelineHandle pipeline) override;

    HGIMETAL_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    /// Execute a compute shader with provided thread group count in each
    /// dimension.
    HGIMETAL_API
        void TraceRays(uint32_t width, uint32_t height, uint32_t depth) override;

protected:
    friend class HgiMetal;

    HGIMETAL_API
    HgiMetalRayTracingCmds(HgiMetal* hgi);

    HGIMETAL_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiMetalRayTracingCmds() = delete;
    HgiMetalRayTracingCmds & operator=(const HgiMetalRayTracingCmds&) = delete;
    HgiMetalRayTracingCmds(const HgiMetalRayTracingCmds&) = delete;
    
    void _CreateEncoder();
    void _CreateArgumentBuffer();
    
    HgiMetal* _hgi;
    HgiMetalRayTracingPipeline* _pipelineState;
    id<MTLCommandBuffer> _commandBuffer;
    id<MTLBuffer> _argumentBuffer;
    id<MTLComputeCommandEncoder> _encoder;
    bool _secondaryCommandBuffer;
    bool _hasWork;


};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
