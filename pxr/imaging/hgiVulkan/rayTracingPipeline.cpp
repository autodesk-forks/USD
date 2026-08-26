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
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgiVulkan/rayTracingPipeline.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/pipelineCache.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/shaderProgram.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanRayTracingPipeline::HgiVulkanRayTracingPipeline(

    Hgi* pHgi,
    HgiVulkanDevice* device,
    HgiRayTracingPipelineDesc const& desc)
    : HgiRayTracingPipeline(desc)
    , _device(device)
    , _inflightBits(0)
    , _vkPipeline(nullptr)
    , _vkPipelineLayout(nullptr)
    , _pHgi(pHgi)
{

    for (int i = 0; i < desc.descriptorSetLayouts.size(); i++) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for(int j=0;j< desc.descriptorSetLayouts[i].resourceBinding.size();j++)
        {
            HgiRayTracingPipelineResourceBindingDesc bindingDesc = desc.descriptorSetLayouts[i].resourceBinding[j];

            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = bindingDesc.bindingIndex;
            layoutBinding.descriptorType = HgiVulkanConversions::GetDescriptorType(bindingDesc.resourceType);
            layoutBinding.descriptorCount = bindingDesc.count;
            layoutBinding.stageFlags = HgiVulkanConversions::GetShaderStages(bindingDesc.stageUsage);
            bindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCI{};
        descriptorSetlayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetlayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetlayoutCI.pBindings = bindings.data();
        TF_VERIFY(vkCreateDescriptorSetLayout(_device->GetVulkanDevice(), &descriptorSetlayoutCI, nullptr, &descriptorSetLayout)== VK_SUCCESS);
        _vkDescriptorSetLayouts.push_back(descriptorSetLayout);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = _vkDescriptorSetLayouts.size();
    pipelineLayoutCI.pSetLayouts = _vkDescriptorSetLayouts.data();
    TF_VERIFY(vkCreatePipelineLayout(_device->GetVulkanDevice(), &pipelineLayoutCI, nullptr, &_vkPipelineLayout)==VK_SUCCESS);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
   
    for (int i = 0; i < desc.shaders.size(); i++) {
        HgiShaderFunctionHandle shader = desc.shaders[i].shader;
        VkPipelineShaderStageCreateInfo shaderStage;
        shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = (VkShaderStageFlagBits) HgiVulkanConversions::GetShaderStages(shader->GetDescriptor().shaderStage);;
        shaderStage.module = (VkShaderModule)shader->GetRawResource();
        shaderStage.pName = desc.shaders[i].entryPoint.c_str();
        shaderStages.push_back(shaderStage);
    }
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

    for (int i = 0; i < desc.groups.size(); i++) 
    {
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
        shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroup.type = HgiVulkanConversions::GetRayTracingShaderGroupType(desc.groups[i].type);
        shaderGroup.generalShader = desc.groups[i].generalShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].generalShader;
        shaderGroup.closestHitShader = desc.groups[i].closestHitShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].closestHitShader;
        shaderGroup.anyHitShader = desc.groups[i].anyHitShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].anyHitShader;
        shaderGroup.intersectionShader = desc.groups[i].intersectionShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].intersectionShader;
        shaderGroups.push_back(shaderGroup);
    }

    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
    rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayTracingPipelineCI.pStages = shaderStages.data();
    rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
    rayTracingPipelineCI.pGroups = shaderGroups.data();
    rayTracingPipelineCI.maxPipelineRayRecursionDepth = desc.maxRayRecursionDepth;
    rayTracingPipelineCI.layout = _vkPipelineLayout;
    TF_VERIFY(_device->vkCreateRayTracingPipelinesKHR(_device->GetVulkanDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &_vkPipeline)== VK_SUCCESS);

    BuildShaderBindingTable();
    
    // Debug label
    if (!desc.debugName.empty()) {
        std::string debugLabel = "Pipeline " + desc.debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkPipeline,
            VK_OBJECT_TYPE_PIPELINE,
            debugLabel.c_str());
    }
}

HgiVulkanRayTracingPipeline::~HgiVulkanRayTracingPipeline()
{
    vkDestroyPipelineLayout(
        _device->GetVulkanDevice(),
        _vkPipelineLayout,
        HgiVulkanAllocator());

    vkDestroyPipeline(
        _device->GetVulkanDevice(),
        _vkPipeline,
        HgiVulkanAllocator());

    for (VkDescriptorSetLayout layout : _vkDescriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(
            _device->GetVulkanDevice(),
            layout,
            HgiVulkanAllocator());
    }

    // This is called from inside garbage collector, so can just call delete on buffer pointer.
    delete _shaderBindingTable.hitShaderBindingTable.Get();
    delete _shaderBindingTable.missShaderBindingTable.Get();
    delete _shaderBindingTable.raygenShaderBindingTable.Get();
}

void
HgiVulkanRayTracingPipeline::BindPipeline(VkCommandBuffer cb)
{
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _vkPipeline);
}

VkPipelineLayout
HgiVulkanRayTracingPipeline::GetVulkanPipelineLayout() const
{
    return _vkPipelineLayout;
}

VkPipeline
HgiVulkanRayTracingPipeline::GetVulkanPipeline() const
{
    return _vkPipeline;
}

HgiVulkanDevice*
HgiVulkanRayTracingPipeline::GetDevice() const
{
    return _device;
}

uint64_t &
HgiVulkanRayTracingPipeline::GetInflightBits()
{
    return _inflightBits;
}


uint32_t alignedSize(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);

}

void alignBuffer(std::vector<uint8_t>& dst, uint32_t index,  uint32_t stride) {
    if ((index + 1) * stride > dst.size())
        dst.resize((index + 1) * stride);

}

void copyAlignedHandle(std::vector<uint8_t>& dst, uint32_t dstIdx, const std::vector<uint8_t>& src, uint32_t srcIdx, const uint32_t stride) {
    alignBuffer(dst, dstIdx,stride);

    memcpy(&dst[dstIdx * stride], &src[srcIdx * stride], stride);
}
VkDeviceOrHostAddressConstKHR GetBufferAddress(HgiBufferHandle buffer);


void HgiVulkanRayTracingPipeline::WriteShaderGroup(const HgiRayTracingPipelineGroupDesc &group , const std::vector<uint8_t>& shaderHandleStorage, uint32_t shaderHandleIndex, std::vector<uint8_t>& buffer, size_t &strideOut) {
    auto rayTracingPipelineProperties = _device->GetRayTracingPipelineProperties();
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
    uint32_t handleIndex = (buffer.size()/handleSizeAligned);
    size_t startSize = buffer.size();
    copyAlignedHandle(buffer, handleIndex++, shaderHandleStorage, shaderHandleIndex, handleSizeAligned);
    
    // Copy shader record buffer after the shader handle.
    if (group.shaderRecordLength) {
        size_t bufferIndex = buffer.size();
        buffer.resize(bufferIndex + group.shaderRecordLength);
        memcpy((void*)&buffer[bufferIndex], group.pShaderRecord, group.shaderRecordLength);
    }

    // Pad buffer to ensure correct alignment.
    while (buffer.size() % handleSizeAligned) {
        buffer.push_back(0xff);
    }

    size_t endSize = buffer.size();
    size_t stride = endSize - startSize;
    if (!strideOut) {
        strideOut = stride;
    }
    else {
        if (strideOut != stride) {
            TF_CODING_ERROR("Shader group stride mismatch %d!=%d",
                stride,
                strideOut);
        }
    }
}

void HgiVulkanRayTracingPipeline::BuildShaderBindingTable() {
    auto rayTracingPipelineProperties = _device->GetRayTracingPipelineProperties();
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
    const uint32_t groupCount = _descriptor.groups.size();
    const uint32_t sbtSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    TF_VERIFY(_device->vkGetRayTracingShaderGroupHandlesKHR(_device->GetVulkanDevice(), _vkPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()) == VK_SUCCESS);

    std::vector<uint8_t> rayGenShaderHandleStorage;
    std::vector<uint8_t> missShaderHandleStorage;
    std::vector<uint8_t> hitShaderHandleStorage;
    size_t rayGenStride=0;
    size_t missStride = 0;
    size_t hitStride = 0;
    for (int i = 0; i < _descriptor.groups.size(); i++) {
        if (_descriptor.groups[i].generalShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].generalShader;
            if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, rayGenShaderHandleStorage,rayGenStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, missShaderHandleStorage,missStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageClosestHit || _descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, hitShaderHandleStorage, hitStride);
            }
        }
        else if (_descriptor.groups[i].closestHitShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].closestHitShader;
            if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, rayGenShaderHandleStorage,rayGenStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, missShaderHandleStorage,missStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageClosestHit || _descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, hitShaderHandleStorage, hitStride);
            }
        }
        else if (_descriptor.groups[i].anyHitShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].anyHitShader;
            if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, rayGenShaderHandleStorage,rayGenStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, missShaderHandleStorage,missStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageClosestHit || _descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, hitShaderHandleStorage, hitStride);
            }
        }
        else if (_descriptor.groups[i].intersectionShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].intersectionShader;
            if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, rayGenShaderHandleStorage,rayGenStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, missShaderHandleStorage,missStride);
            }
            else if (_descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageClosestHit || _descriptor.shaders[idx].shader->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                WriteShaderGroup(_descriptor.groups[i], shaderHandleStorage, i, hitShaderHandleStorage, hitStride);
            }
        }

    }



    HgiBufferDesc raygenShaderBindingTableBufferDesc;
    raygenShaderBindingTableBufferDesc.debugName = _descriptor.debugName + " Raygen Shader Binding Table";
    raygenShaderBindingTableBufferDesc.usage = HgiBufferUsageShaderBindingTable | HgiBufferUsageRayTracingExtensions | HgiBufferUsageShaderDeviceAddress;
    // A pipeline can have no hit groups when the scene is empty; Hgi rejects a zero
    // sized buffer, so keep a one byte placeholder and leave it uninitialised.
    raygenShaderBindingTableBufferDesc.byteSize = rayGenShaderHandleStorage.empty() ? 1 : rayGenShaderHandleStorage.size();
    raygenShaderBindingTableBufferDesc.initialData = rayGenShaderHandleStorage.empty() ? nullptr : rayGenShaderHandleStorage.data();
    _shaderBindingTable.raygenShaderBindingTable = _pHgi->CreateBuffer(raygenShaderBindingTableBufferDesc);
    _shaderBindingTable.raygenShaderBindingTableStride = rayGenStride;

    HgiBufferDesc missShaderBindingTableBufferDesc;
    missShaderBindingTableBufferDesc.debugName = _descriptor.debugName + " Miss Shader Binding Table";
    missShaderBindingTableBufferDesc.usage = HgiBufferUsageShaderBindingTable | HgiBufferUsageRayTracingExtensions | HgiBufferUsageShaderDeviceAddress;
    missShaderBindingTableBufferDesc.byteSize = missShaderHandleStorage.empty() ? 1 : missShaderHandleStorage.size();
    missShaderBindingTableBufferDesc.initialData = missShaderHandleStorage.empty() ? nullptr : missShaderHandleStorage.data();
    _shaderBindingTable.missShaderBindingTable = _pHgi->CreateBuffer(missShaderBindingTableBufferDesc);
    _shaderBindingTable.missShaderBindingTableStride = missStride;

    HgiBufferDesc closestHitShaderBindingTableBufferDesc;
    closestHitShaderBindingTableBufferDesc.debugName = _descriptor.debugName + " Hit Shader Binding Table";
    closestHitShaderBindingTableBufferDesc.usage = HgiBufferUsageShaderBindingTable | HgiBufferUsageRayTracingExtensions | HgiBufferUsageShaderDeviceAddress;
    closestHitShaderBindingTableBufferDesc.byteSize = hitShaderHandleStorage.empty() ? 1 : hitShaderHandleStorage.size();
    closestHitShaderBindingTableBufferDesc.initialData = hitShaderHandleStorage.empty() ? nullptr : hitShaderHandleStorage.data();
    _shaderBindingTable.hitShaderBindingTable = _pHgi->CreateBuffer(closestHitShaderBindingTableBufferDesc);
    _shaderBindingTable.hitShaderBindingTableStride = hitStride;
}


PXR_NAMESPACE_CLOSE_SCOPE
