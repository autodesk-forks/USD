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
#ifndef PXR_IMAGING_HGI_RAYTRACING_PIPELINE_H
#define PXR_IMAGING_HGI_RAYTRACING_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/attachmentDesc.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiRayTracingPipelineShaderDesc {
    HgiShaderFunctionHandle shader;
    std::string entryPoint = "main";
};

struct HgiRayTracingPipelineGroupDesc {
    HgiRayTracingShaderGroupType type;
    uint32_t                     generalShader = 0xFFFF;
    uint32_t                     closestHitShader = 0xFFFF;
    uint32_t                     anyHitShader = 0xFFFF;
    uint32_t                     intersectionShader = 0xFFFF;
    void*                        pShaderRecord;
    size_t                       shaderRecordLength;
};

struct HgiRayTracingPipelineResourceBindingDesc {
    uint32_t                bindingIndex;
    HgiBindResourceType     resourceType;
    uint32_t                count = 1;
    HgiShaderStage          stageUsage;
    HgiSamplerHandleVector  samplers;
};

struct HgiRayTracingPipelineDescriptorSetLayoutDesc {
    std::vector<HgiRayTracingPipelineResourceBindingDesc> resourceBinding;

};

/// \struct HgiRayTracingPipelineDesc
///
/// Describes the properties needed to create a GPU compute pipeline.
///
/// <ul>
/// <li>shaderProgram:
///   Shader function used in this pipeline.</li>
/// <li>shaderConstantsDesc:
///   Describes the shader uniforms.</li>
/// </ul>
///
struct HgiRayTracingPipelineDesc
{
    HGI_API
        HgiRayTracingPipelineDesc();

    std::string debugName;
    std::vector<HgiRayTracingPipelineDescriptorSetLayoutDesc> descriptorSetLayouts;
    std::vector<HgiRayTracingPipelineShaderDesc> shaders;
    std::vector<HgiRayTracingPipelineGroupDesc> groups;
    uint32_t maxRayRecursionDepth;
};


///
/// \class HgiRayTracingPipeline
///
/// Represents a graphics platform independent GPU compute pipeline resource.
///
/// Base class for Hgi compute pipelines.
/// To the client (HdSt) compute pipeline resources are referred to via
/// opaque, stateless handles (HgiRayTracingPipelineHandle).
///
class HgiRayTracingPipeline
{
public:
    HGI_API
        virtual ~HgiRayTracingPipeline();

    /// The descriptor describes the object.
    HGI_API
        HgiRayTracingPipelineDesc const& GetDescriptor() const;

protected:
    HGI_API
        HgiRayTracingPipeline(HgiRayTracingPipelineDesc const& desc);

    HgiRayTracingPipelineDesc _descriptor;

private:
    HgiRayTracingPipeline() = delete;
    HgiRayTracingPipeline& operator=(const HgiRayTracingPipeline&) = delete;
    HgiRayTracingPipeline(const HgiRayTracingPipeline&) = delete;
};

using HgiRayTracingPipelineHandle = HgiHandle<class HgiRayTracingPipeline>;
using HgiRayTracingPipelineHandleVector = std::vector<HgiRayTracingPipelineHandle>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
