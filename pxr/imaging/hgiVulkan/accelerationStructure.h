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
#ifndef PXR_IMAGING_HGIVULKAN_HGI_ACCELERATION_STRUCTURE_H
#define PXR_IMAGING_HGIVULKAN_HGI_ACCELERATION_STRUCTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgi/accelerationStructure.h"

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


///
/// \class HgiVulkanAccelerationStructureGeometry
///
/// Represents GPU acceleration structure for ray tracing.
/// AccelerationStructureGeometrys should be created via Hgi::CreateAccelerationStructureGeometry.
///
class HgiVulkanAccelerationStructureGeometry : public HgiAccelerationStructureGeometry
{
public:
    HGIVULKAN_API
        virtual ~HgiVulkanAccelerationStructureGeometry();

    VkAccelerationStructureGeometryKHR* GetVulkanGeometry() {
        return &_accelerationStructureGeometry;
    }

    uint32_t GetPrimitiveCount() { return _primitiveCount;  }

    /// Returns the device used to create this object.
    HGIVULKAN_API
        HgiVulkanDevice* GetDevice() const { return _device; }

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
        uint64_t& GetInflightBits() {
        return _inflightBits;
    }

protected:
    friend class HgiVulkan;

    HgiVulkanAccelerationStructureGeometry(
        Hgi* pHgi,
        HgiVulkanDevice* device,
        HgiAccelerationStructureTriangleGeometryDesc const& desc);
    HgiVulkanAccelerationStructureGeometry(Hgi* pHgi,
            HgiVulkanDevice* device, HgiAccelerationStructureInstanceGeometryDesc const& desc);


private:
    HgiVulkanDevice* _device;
    uint64_t _inflightBits;

    HgiVulkanAccelerationStructureGeometry() = delete;
    HgiVulkanAccelerationStructureGeometry& operator=(const HgiVulkanAccelerationStructureGeometry&) = delete;
    HgiVulkanAccelerationStructureGeometry(const HgiVulkanAccelerationStructureGeometry&) = delete;

    VkAccelerationStructureGeometryKHR _accelerationStructureGeometry;
    HgiBufferHandle _instancesBuffer;
    uint32_t _primitiveCount;
};


///
/// \class HgiVulkanAccelerationStructure
///
/// Represents GPU acceleration structure for ray tracing.
/// AccelerationStructures should be created via Hgi::CreateAccelerationStructure.
///
class HgiVulkanAccelerationStructure : public HgiAccelerationStructure
{
public:
    HGIVULKAN_API
        virtual ~HgiVulkanAccelerationStructure();

    /// This function returns the handle to the Hgi backend's gpu resource, cast
    /// to a uint64_t. Clients should avoid using this function and instead
    /// use Hgi base classes so that client code works with any Hgi platform.
    /// For transitioning code to Hgi, it can however we useful to directly
    /// access a platform's internal resource handles.
    /// There is no safety provided in using this. If you by accident pass a
    /// HgiMetal resource into an OpenGL call, bad things may happen.
    /// In OpenGL this returns the GLuint resource name.
    /// In Metal this returns the id<MTLAccelerationStructureState> as uint64_t.
    /// In Vulkan this returns the VkAccelerationStructure as uint64_t.
    HGIVULKAN_API
        uint64_t GetRawResource() const override;

    /// Returns the device used to create this object.
    HGIVULKAN_API
        HgiVulkanDevice* GetDevice() const { return _device; }

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
        uint64_t& GetInflightBits() {
        return _inflightBits;
    }

    HGIVULKAN_API
        const VkAccelerationStructureBuildSizesInfoKHR& GetBuildSizesInfo() { return _buildSizesInfo; }
    HGIVULKAN_API
        const VkAccelerationStructureBuildGeometryInfoKHR& GetBuildGeometryInfo() { return _buildGeomInfo; }

    HGIVULKAN_API
        HgiBufferHandle GetScratchBuffer() {
        return _scratchBuffer;
    }
    HGIVULKAN_API
        HgiBufferHandle GetAccelerationStructureBuffer() {
        return _accelStructureBuffer;
    }



protected:
    friend class HgiVulkan;

    HGIVULKAN_API
        HgiVulkanAccelerationStructure(
            Hgi* pHgi,
            HgiVulkanDevice* device,
            HgiAccelerationStructureDesc const& desc);

private:
    HgiVulkanAccelerationStructure() = delete;
    HgiVulkanAccelerationStructure& operator=(const HgiVulkanAccelerationStructure&) = delete;
    HgiVulkanAccelerationStructure(const HgiVulkanAccelerationStructure&) = delete;

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;
    VkAccelerationStructureKHR _accelerationStructure;
    VkAccelerationStructureBuildGeometryInfoKHR _buildGeomInfo;
    VkAccelerationStructureBuildSizesInfoKHR _buildSizesInfo;
    std::vector<VkAccelerationStructureGeometryKHR> _vkGeom;
    std::vector<uint32_t> _primitiveCounts;

    HgiBufferHandle _accelStructureBuffer;
    HgiBufferHandle _scratchBuffer;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
