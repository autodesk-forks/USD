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
#include "pxr/imaging/hgiVulkan/accelerationStructureCmds.h"
#include "pxr/imaging/hgiVulkan/accelerationStructure.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanAccelerationStructureCmds::HgiVulkanAccelerationStructureCmds(HgiVulkan* hgi)
    : HgiAccelerationStructureCmds()
    , _hgi(hgi)
    , _commandBuffer(nullptr)
{
}

HgiVulkanAccelerationStructureCmds::~HgiVulkanAccelerationStructureCmds()
{
}

void
HgiVulkanAccelerationStructureCmds::PushDebugGroup(const char* label)
{
    _CreateCommandBuffer();
    HgiVulkanBeginLabel(_hgi->GetPrimaryDevice(), _commandBuffer, label);
}

void
HgiVulkanAccelerationStructureCmds::PopDebugGroup()
{
    _CreateCommandBuffer();
    HgiVulkanEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
}


void
HgiVulkanAccelerationStructureCmds::Build(HgiAccelerationStructureHandleVector accelStructures, const std::vector<HgiAccelerationStructureBuildRange>& ranges)
{
    _CreateCommandBuffer();

    std::vector< VkAccelerationStructureBuildRangeInfoKHR> rangeInfo;
    std::vector< VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPointers;
    std::vector< VkAccelerationStructureBuildGeometryInfoKHR> buildInfo;

    if(ranges.size() != accelStructures.size())
        TF_FATAL_ERROR("Mismatched number of ranges and acceleration structures");

    for (int i = 0; i < ranges.size(); i++) {
        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
        accelerationStructureBuildRangeInfo.primitiveCount = ranges[i].primitiveCount;
        accelerationStructureBuildRangeInfo.primitiveOffset = ranges[i].primitiveOffset;
        accelerationStructureBuildRangeInfo.firstVertex = ranges[i].firstVertex;
        accelerationStructureBuildRangeInfo.transformOffset = ranges[i].transformOffset;
        rangeInfo.push_back(accelerationStructureBuildRangeInfo);
        rangeInfoPointers.push_back(&rangeInfo[i]);

        HgiVulkanAccelerationStructure* pVkAccelStruct = (HgiVulkanAccelerationStructure*)accelStructures[i].Get();
        buildInfo.push_back(pVkAccelStruct->GetBuildGeometryInfo());
            
    }

    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    VkCommandBuffer vkCommandBuffer = _commandBuffer->GetVulkanCommandBuffer();
    device->vkCmdBuildAccelerationStructuresKHR(
        vkCommandBuffer,
        buildInfo.size(),
        buildInfo.data(),
        rangeInfoPointers.data());

}

bool
HgiVulkanAccelerationStructureCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
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
HgiVulkanAccelerationStructureCmds::_CreateCommandBuffer()
{
    if (!_commandBuffer) {
        HgiVulkanDevice* device = _hgi->GetPrimaryDevice();
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        _commandBuffer = queue->AcquireCommandBuffer();
        TF_VERIFY(_commandBuffer);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
