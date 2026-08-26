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
#ifndef PXR_IMAGING_HGI_VULKAN_RAYTRACING_PIPELINE_H
#define PXR_IMAGING_HGI_VULKAN_RAYTRACING_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/rayTracingPipeline.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

using VkDescriptorSetLayoutVector = std::vector<VkDescriptorSetLayout>;

struct HgiVulkanRayTracingShaderBindingTable {
    HgiBufferHandle raygenShaderBindingTable;
    size_t raygenShaderBindingTableStride;
    HgiBufferHandle missShaderBindingTable;
    size_t missShaderBindingTableStride;
    HgiBufferHandle hitShaderBindingTable;
    size_t hitShaderBindingTableStride;
};

/// \class HgiVulkanRayTracingPipeline
///
/// Vulkan implementation of HgiRayTracingPipeline.
///
class HgiVulkanRayTracingPipeline final : public HgiRayTracingPipeline
{
public:
    HGIVULKAN_API
        ~HgiVulkanRayTracingPipeline() override;

    /// Apply pipeline state
    HGIVULKAN_API
        void BindPipeline(VkCommandBuffer cb);

    /// Returns the vulkan pipeline layout
    HGIVULKAN_API
        VkPipelineLayout GetVulkanPipelineLayout() const;

    /// Returns the vulkan pipeline layout
    HGIVULKAN_API
        VkPipeline GetVulkanPipeline() const;

    /// Returns the vulkan pipeline layout
    HGIVULKAN_API
        const VkDescriptorSetLayoutVector& GetVulkanDescriptorSetLayouts() const {
        return _vkDescriptorSetLayouts;
    }

    HGIVULKAN_API
        const HgiVulkanRayTracingShaderBindingTable& GetShaderBindingTable() const {
        return _shaderBindingTable;
    }

    /// Returns the device used to create this object.
    HGIVULKAN_API
        HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
        uint64_t& GetInflightBits();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
        HgiVulkanRayTracingPipeline(
            Hgi* pHgi,
            HgiVulkanDevice* device,
            HgiRayTracingPipelineDesc const& desc);

private:
    HgiVulkanRayTracingPipeline() = delete;
    HgiVulkanRayTracingPipeline& operator=(const HgiVulkanRayTracingPipeline&) = delete;
    HgiVulkanRayTracingPipeline(const HgiVulkanRayTracingPipeline&) = delete;

    void BuildShaderBindingTable();
    void WriteShaderGroup(const HgiRayTracingPipelineGroupDesc& group, const std::vector<uint8_t>& shaderHandleStorage, uint32_t shaderHandleIndex, std::vector<uint8_t>& buffer, size_t &strideOut);

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;
    VkPipeline _vkPipeline;
    VkPipelineLayout _vkPipelineLayout;
    VkDescriptorSetLayoutVector _vkDescriptorSetLayouts;
    HgiVulkanRayTracingShaderBindingTable _shaderBindingTable;

    Hgi* _pHgi;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
