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
#include "pxr/imaging/hgiVulkan/accelerationStructure.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiVulkan/device.h"

PXR_NAMESPACE_OPEN_SCOPE



HgiVulkanAccelerationStructureGeometry::HgiVulkanAccelerationStructureGeometry(
    Hgi *pHgi,
    HgiVulkanDevice* device,
    HgiAccelerationStructureTriangleGeometryDesc const& desc) : HgiAccelerationStructureGeometry(desc),_device(device) {
    _accelerationStructureGeometry = {};
    _accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    _accelerationStructureGeometry.flags = HgiVulkanConversions::GetAccelerationStructureGeometryFlags(desc.flags);
    _accelerationStructureGeometry.pNext = nullptr;
    _accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    _accelerationStructureGeometry.geometry.triangles.pNext = nullptr;
    _accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    _accelerationStructureGeometry.geometry.triangles.vertexFormat = HgiVulkanConversions::GetFormat(desc.vertexFormat);;
    _accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = desc.vertexData->GetDeviceAddress();
    _accelerationStructureGeometry.geometry.triangles.maxVertex = desc.maxVertex;
    _accelerationStructureGeometry.geometry.triangles.vertexStride = desc.vertexStride;
    _accelerationStructureGeometry.geometry.triangles.indexType = desc.indexType== HgiIndexTypeUInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32; //TODO respect format
    _accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = desc.indexData->GetDeviceAddress();
    _accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = desc.transformData->GetDeviceAddress();

    _primitiveCount = desc.count;
}

HgiVulkanAccelerationStructureGeometry::~HgiVulkanAccelerationStructureGeometry() {
    // This is called from inside garbage collector, so can just call delete on buffer pointer.
    delete _instancesBuffer.Get();
}

HgiVulkanAccelerationStructureGeometry::HgiVulkanAccelerationStructureGeometry(Hgi* pHgi,
    HgiVulkanDevice* device, HgiAccelerationStructureInstanceGeometryDesc const& desc) : HgiAccelerationStructureGeometry(desc), _device(device) {

    std::vector<VkAccelerationStructureInstanceKHR> instances; 
    instances.resize(desc.instances.size());
    for(int i=0;i< desc.instances.size();i++)
    {
        HgiAccelerationStructureHandle blas = desc.instances[i].blas;
        HgiVulkanAccelerationStructure* pVkBlas = (HgiVulkanAccelerationStructure*)blas.Get();
        HgiBufferHandle blasBuffer = pVkBlas->GetAccelerationStructureBuffer();

        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 4; k++) {
                instances[i].transform.matrix[j][k] = desc.instances[i].transform[j][k];
            }
        }
        instances[i].instanceCustomIndex = desc.instances[i].id;
        instances[i].mask = desc.instances[i].mask;
        instances[i].instanceShaderBindingTableRecordOffset = desc.instances[i].groupIndex;
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[i].accelerationStructureReference = blasBuffer->GetDeviceAddress();

    }

    HgiBufferDesc instancesBufferDesc;
    instancesBufferDesc.debugName = desc.debugName + "InstancesBuffer";
    instancesBufferDesc.usage = HgiBufferUsageAccelerationStructureBuildInputReadOnly | HgiBufferUsageRayTracingExtensions | HgiBufferUsageShaderDeviceAddress;
    // A scene can legitimately contain no instances (an empty scene, or one whose render buffer
    // was released first). Hgi rejects a zero sized buffer, and &instances[0] on an empty vector
    // is undefined behaviour, so allocate room for one unused instance in that case; the build
    // below still reports the real instance count.
    instancesBufferDesc.initialData = instances.empty() ? nullptr : instances.data();
    instancesBufferDesc.byteSize =
        (instances.empty() ? 1 : instances.size()) * sizeof(VkAccelerationStructureInstanceKHR);
    _instancesBuffer = pHgi->CreateBuffer(instancesBufferDesc);

    _accelerationStructureGeometry = {};
    _accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    _accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    _accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    _accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    _accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    _accelerationStructureGeometry.geometry.instances.data.deviceAddress = _instancesBuffer->GetDeviceAddress();

    _primitiveCount = instances.size();
}


uint64_t
HgiVulkanAccelerationStructure::GetRawResource() const
{
    return (uint64_t)_accelerationStructure;
}


HgiVulkanAccelerationStructure::HgiVulkanAccelerationStructure(
    Hgi* pHgi,
    HgiVulkanDevice* device,
    HgiAccelerationStructureDesc const& desc): HgiAccelerationStructure(desc), _device(device) {
    _vkGeom.clear();
    _primitiveCounts.clear();
    for (int i = 0; i < desc.geometry.size(); i++) {
        HgiVulkanAccelerationStructureGeometry* pGeom = (HgiVulkanAccelerationStructureGeometry*)desc.geometry[i].Get();
        _vkGeom.push_back(*pGeom->GetVulkanGeometry());
        _primitiveCounts.push_back(pGeom->GetPrimitiveCount());
    }

    VkAccelerationStructureTypeKHR type = HgiVulkanConversions::GetAccelerationStructureType(desc.type);;

    _buildGeomInfo = {};
    _buildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    _buildGeomInfo.type = type;
    _buildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    _buildGeomInfo.geometryCount = _vkGeom.size();
    _buildGeomInfo.pGeometries = &_vkGeom[0];

    _buildSizesInfo = {};
    _buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    _device->vkGetAccelerationStructureBuildSizesKHR(
        _device->GetVulkanDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &_buildGeomInfo,
        &_primitiveCounts[0],
        &_buildSizesInfo);

    HgiBufferDesc accelBufferDesc;
    accelBufferDesc.debugName = _descriptor.debugName + "AccelerationStructureBuffer";
    accelBufferDesc.usage = HgiBufferUsageAccelerationStructureStorage | HgiBufferUsageRayTracingExtensions | HgiBufferUsageShaderDeviceAddress;
    accelBufferDesc.initialData = nullptr;
    accelBufferDesc.byteSize = _buildSizesInfo.accelerationStructureSize;
    _accelStructureBuffer = pHgi->CreateBuffer(accelBufferDesc);

    HgiVulkanBuffer* pAccelBufferVk = (HgiVulkanBuffer*)_accelStructureBuffer.Get();

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = pAccelBufferVk->GetVulkanBuffer();
    accelerationStructureCreateInfo.size = _buildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = type;
    _device->vkCreateAccelerationStructureKHR(_device->GetVulkanDevice(), &accelerationStructureCreateInfo, nullptr, &_accelerationStructure);

    HgiBufferDesc scratchBufferDesc;
    scratchBufferDesc.debugName = _descriptor.debugName + "ScratchBuffer";
    scratchBufferDesc.usage = HgiBufferUsageStorage | HgiBufferUsageRayTracingExtensions | HgiBufferUsageShaderDeviceAddress;
    scratchBufferDesc.initialData = nullptr;
    scratchBufferDesc.byteSize = _buildSizesInfo.buildScratchSize;
    _scratchBuffer = pHgi->CreateBuffer(scratchBufferDesc);
    HgiVulkanBuffer* pScratchBufferVk = (HgiVulkanBuffer*)_scratchBuffer.Get();

    _buildGeomInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    _buildGeomInfo.dstAccelerationStructure = _accelerationStructure;
    _buildGeomInfo.scratchData.deviceAddress = pScratchBufferVk->GetDeviceAddress();
}

HgiVulkanAccelerationStructure::~HgiVulkanAccelerationStructure() {
    // This is called from inside garbage collector, so can just call delete on buffer pointer.
    delete _scratchBuffer.Get();
    delete _accelStructureBuffer.Get();

    _device->vkDestroyAccelerationStructureKHR(_device->GetVulkanDevice(), _accelerationStructure,
        HgiVulkanAllocator());
}

PXR_NAMESPACE_CLOSE_SCOPE
