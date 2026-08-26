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
#ifndef PXR_IMAGING_HGIVULKAN_ACCELERATION_STRUCTURE_CMDS_H
#define PXR_IMAGING_HGIVULKAN_ACCELERATION_STRUCTURE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/accelerationStructureCmds.h"
#include "pxr/imaging/hgi/accelerationStructure.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class HgiVulkanCommandBuffer;


/// \class HgiVulkanAccelerationStructureCmds
///
/// OpenGL implementation of HgiAccelerationStructureCmds.
///
class HgiVulkanAccelerationStructureCmds final : public HgiAccelerationStructureCmds
{
public:
    HGIVULKAN_API
    ~HgiVulkanAccelerationStructureCmds() override;

    HGIVULKAN_API
    void PushDebugGroup(const char* label) override;

    HGIVULKAN_API
    void PopDebugGroup() override;

    HGIVULKAN_API
        virtual void Build(HgiAccelerationStructureHandleVector accelStructures, const std::vector<HgiAccelerationStructureBuildRange>& ranges) override;


protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanAccelerationStructureCmds(HgiVulkan* hgi);

    HGIVULKAN_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiVulkanAccelerationStructureCmds() = delete;
    HgiVulkanAccelerationStructureCmds & operator=(const HgiVulkanAccelerationStructureCmds&) = delete;
    HgiVulkanAccelerationStructureCmds(const HgiVulkanAccelerationStructureCmds&) = delete;

    void _BindResources();
    void _CreateCommandBuffer();

    HgiVulkan* _hgi;
    HgiVulkanCommandBuffer* _commandBuffer;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
