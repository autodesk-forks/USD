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
#include "pxr/imaging/hgiVulkan/rayTracingCmds.h"
#include "pxr/imaging/hgiVulkan/accelerationStructure.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanRayTracingCmds::HgiVulkanRayTracingCmds(HgiVulkan* hgi)
    : HgiRayTracingCmds()
    , _hgi(hgi)
    , _commandBuffer(nullptr)
{
}

HgiVulkanRayTracingCmds::~HgiVulkanRayTracingCmds()
{
}

void
HgiVulkanRayTracingCmds::PushDebugGroup(const char* label)
{
    _CreateCommandBuffer();
    HgiVulkanBeginLabel(_hgi->GetPrimaryDevice(), _commandBuffer, label, GfVec4f(1.0f));
}

void
HgiVulkanRayTracingCmds::PopDebugGroup()
{
    _CreateCommandBuffer();
    HgiVulkanEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
}



void  HgiVulkanRayTracingCmds::BindResources(HgiResourceBindingsHandle resources) {
    _CreateCommandBuffer();

    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    VkCommandBuffer vkCommandBuffer = _commandBuffer->GetVulkanCommandBuffer();
    HgiVulkanResourceBindings* pVkResources = (HgiVulkanResourceBindings*)resources.Get();
    HgiVulkanRayTracingPipeline* pVkPipeline = (HgiVulkanRayTracingPipeline*)_pipeline.Get();
    VkDescriptorSet descriptorSet = pVkResources->GetDescriptorSet();

    vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pVkPipeline->GetVulkanPipelineLayout(), 0, 1, &descriptorSet, 0, 0);

}
void  HgiVulkanRayTracingCmds::HgiVulkanRayTracingCmds::BindPipeline(HgiRayTracingPipelineHandle pipeline)
{
    _CreateCommandBuffer();

    _pipeline = pipeline;
    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    VkCommandBuffer vkCommandBuffer = _commandBuffer->GetVulkanCommandBuffer();
    HgiVulkanRayTracingPipeline* pVkRayTracingPipeline = (HgiVulkanRayTracingPipeline*)pipeline.Get();
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pVkRayTracingPipeline->GetVulkanPipeline());

}

void
    HgiVulkanRayTracingCmds::TraceRays(uint32_t width, uint32_t height, uint32_t depth)
{
    _CreateCommandBuffer();

    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    VkCommandBuffer vkCommandBuffer = _commandBuffer->GetVulkanCommandBuffer();
 
    HgiVulkanRayTracingPipeline* pVkPipeline = (HgiVulkanRayTracingPipeline*)_pipeline.Get();
    auto &shaderBindingTable = pVkPipeline->GetShaderBindingTable();


    HgiVulkanBuffer* pRaygenShaderBindingTableBufferVk = (HgiVulkanBuffer*)shaderBindingTable.raygenShaderBindingTable.Get();
    HgiVulkanBuffer* pMissShaderBindingTableBufferVk = (HgiVulkanBuffer*)shaderBindingTable.missShaderBindingTable.Get();
    HgiVulkanBuffer* pHitShaderBindingTableBufferVk = (HgiVulkanBuffer*)shaderBindingTable.hitShaderBindingTable.Get();

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = pRaygenShaderBindingTableBufferVk->GetDeviceAddress();
    raygenShaderSbtEntry.stride = shaderBindingTable.raygenShaderBindingTableStride;
    raygenShaderSbtEntry.size = pRaygenShaderBindingTableBufferVk->GetDescriptor().byteSize;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = pMissShaderBindingTableBufferVk->GetDeviceAddress();
    missShaderSbtEntry.stride = shaderBindingTable.missShaderBindingTableStride;
    missShaderSbtEntry.size = pMissShaderBindingTableBufferVk->GetDescriptor().byteSize;

    // A pipeline for a scene with no geometry has no hit groups, and gets a placeholder buffer
    // because Hgi cannot create a zero sized one. Vulkan wants an all-zero region in that case.
    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    const uint32_t hitStride = shaderBindingTable.hitShaderBindingTableStride;
    const size_t hitTableSize = pHitShaderBindingTableBufferVk->GetDescriptor().byteSize;
    if (hitStride > 0 && hitTableSize >= hitStride) {
        hitShaderSbtEntry.deviceAddress = pHitShaderBindingTableBufferVk->GetDeviceAddress();
        hitShaderSbtEntry.stride = hitStride;
        hitShaderSbtEntry.size = hitTableSize;
    }

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    /*
        Dispatch the ray tracing commands
    */
    //vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline.pipeline);
    //vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline.pipelineLayout, 0, 1, &descriptorSets.descriptorSet, 0, 0);



    device->vkCmdTraceRaysKHR(
        vkCommandBuffer,
        &raygenShaderSbtEntry,
        &missShaderSbtEntry,
        &hitShaderSbtEntry,
        &callableShaderSbtEntry,
        width,
        height,
        1);
}

bool
HgiVulkanRayTracingCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (!_commandBuffer) {
        return false;
    }

    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();

    // Submit the GPU work and optionally do CPU - GPU synchronization.
    queue->SubmitToQueue(_commandBuffer, wait);

    return true;
}

void
HgiVulkanRayTracingCmds::_CreateCommandBuffer()
{
    if (!_commandBuffer) {
        HgiVulkanDevice* device = _hgi->GetPrimaryDevice();
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        _commandBuffer = queue->AcquireCommandBuffer();
        TF_VERIFY(_commandBuffer);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
