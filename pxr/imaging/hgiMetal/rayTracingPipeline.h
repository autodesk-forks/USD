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
#ifndef PXR_IMAGING_HGI_METAL_RAYTRACING_PIPELINE_H
#define PXR_IMAGING_HGI_METAL_RAYTRACING_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/rayTracingPipeline.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgiMetal/hgi.h"

#include <Metal/Metal.h>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiMetal;

// using VkDescriptorSetLayoutVector = std::vector<VkDescriptorSetLayout>;

struct HgiMetalRayTracingShaderBindingTable {
    id<MTLVisibleFunctionTable> raygenShaderBindingTable;
    size_t raygenShaderBindingTableSize;
    id<MTLVisibleFunctionTable> missShaderBindingTable;
    size_t missShaderBindingTableSize;
    id<MTLVisibleFunctionTable> hitShaderBindingTable;
    size_t hitShaderBindingTableSize;
    id<MTLIntersectionFunctionTable> intersectionShaderBindingTable;
    size_t insersectionShaderBindingTableSize;
};

/// \class HgiMetalRayTracingPipeline
///
/// Metal implementation of HgiRayTracingPipeline.
///
class HgiMetalRayTracingPipeline final : public HgiRayTracingPipeline
{
public:
    HGIMETAL_API
        ~HgiMetalRayTracingPipeline() override;

    HGIMETAL_API
    void BindPipeline(id<MTLComputeCommandEncoder> computeEncoder);
    
    HGIMETAL_API
    id<MTLComputePipelineState> GetMetalPipelineState();
    

    /// Returns the metal pipeline layout
    // HGIMETAL_API
        // VkPipelineLayout GetMetalPipelineLayout() const;

    /// Returns the metal pipeline layout
    // HGIMETAL_API
        // VkPipeline GetMetalPipeline() const;

    /// Returns the metal pipeline layout
    // HGIMETAL_API
    //     const VkDescriptorSetLayoutVector& GetMetalDescriptorSetLayouts() const {
    //     return _vkDescriptorSetLayouts;
    // }

     HGIMETAL_API
         const HgiMetalRayTracingShaderBindingTable& GetShaderBindingTable() const {
         return _shaderBindingTable;
     }

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIMETAL_API
        uint64_t& GetInflightBits();

protected:
    friend class HgiMetal;

    HGIMETAL_API
        HgiMetalRayTracingPipeline(
            HgiMetal* hgi,
            HgiRayTracingPipelineDesc const& desc);

private:
    HgiMetalRayTracingPipeline() = delete;
    HgiMetalRayTracingPipeline& operator=(const HgiMetalRayTracingPipeline&) = delete;
    HgiMetalRayTracingPipeline(const HgiMetalRayTracingPipeline&) = delete;

    void BuildShaderBindingTable();

    uint64_t _inflightBits;
    id<MTLComputePipelineState> _pipelineState;
    HgiMetalRayTracingShaderBindingTable _shaderBindingTable;

    HgiMetal* _hgi;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
