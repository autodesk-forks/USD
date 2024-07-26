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
#ifndef PXR_IMAGING_HGI_ACCELERATION_STRUCTURE_H
#define PXR_IMAGING_HGI_ACCELERATION_STRUCTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/base/gf/matrix4f.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiAccelerationStructure;
using HgiAccelerationStructureHandle = HgiHandle<HgiAccelerationStructure>;

struct HgiAccelerationStructureInstanceDesc
{
    GfMatrix4f  transform;
    uint32_t id = 0;
    uint32_t mask = 0xff;
    uint32_t groupIndex = 0;
    HgiAccelerationStructureInstanceFlags flags = HgiAccelerationStructureInstanceFlagsDisableFaceCulling;
    HgiAccelerationStructureHandle blas;
};

struct HgiAccelerationStructureInstanceGeometryDesc
{
    std::string debugName;
    std::vector<HgiAccelerationStructureInstanceDesc> instances;
};


/// \struct HgiAccelerationStructureGeometryDesc
///
/// Describes the properties needed to create a GPU acceleration structure for ray tracing.
///
struct HgiAccelerationStructureTriangleGeometryDesc
{
    HgiAccelerationStructureTriangleGeometryDesc()
    {}

    std::string debugName;
    HgiFormat vertexFormat = HgiFormatFloat32Vec3;
    HgiBufferHandle vertexData;
    size_t vertexStride = 0;
    uint32_t maxVertex = 0;
    HgiIndexType indexType = HgiIndexTypeUInt32;
    HgiBufferHandle indexData;
    HgiBufferHandle transformData;
    HgiAccelerationStructureGeometryFlags flags = HgiAccelerationStructureGeometryOpaque;
    uint32_t count = 0;
    // per primitive data
    HgiBufferHandle primitiveData;
    size_t          primitiveDataStride = 0;
    size_t          primitiveDataElementSize = 0;
};

HGI_API
bool operator==(
    const HgiAccelerationStructureTriangleGeometryDesc& lhs,
    const HgiAccelerationStructureTriangleGeometryDesc& rhs);

HGI_API
bool operator!=(
    const HgiAccelerationStructureTriangleGeometryDesc& lhs,
    const HgiAccelerationStructureTriangleGeometryDesc& rhs);


///
/// \class HgiAccelerationStructureGeometry
///
/// Represents GPU acceleration structure for ray tracing.
/// AccelerationStructureGeometrys should be created via Hgi::CreateAccelerationStructureGeometry.
///
class HgiAccelerationStructureGeometry
{
public:
    HGI_API
        virtual ~HgiAccelerationStructureGeometry() {}

protected:
    HGI_API
        HgiAccelerationStructureGeometry(HgiAccelerationStructureTriangleGeometryDesc const& /*desc*/) {}
    HGI_API
        HgiAccelerationStructureGeometry(HgiAccelerationStructureInstanceGeometryDesc const& /*desc*/ ) {}

private:
    HgiAccelerationStructureGeometry() = delete;
    HgiAccelerationStructureGeometry& operator=(const HgiAccelerationStructureGeometry&) = delete;
    HgiAccelerationStructureGeometry(const HgiAccelerationStructureGeometry&) = delete;
};

using HgiAccelerationStructureGeometryHandle = HgiHandle<HgiAccelerationStructureGeometry>;
using HgiAccelerationStructureGeometryHandleVector = std::vector<HgiAccelerationStructureGeometryHandle>;

/// \struct HgiAccelerationStructureDesc
///
/// Describes the properties needed to create a GPU acceleration structure for ray tracing.
///
struct HgiAccelerationStructureDesc
{
    HgiAccelerationStructureDesc()
    {}

    std::string debugName;
    HgiAccelerationStructureGeometryHandleVector geometry;
    HgiAccelerationStructureType type;

};

HGI_API
bool operator==(
    const HgiAccelerationStructureDesc& lhs,
    const HgiAccelerationStructureDesc& rhs);

HGI_API
bool operator!=(
    const HgiAccelerationStructureDesc& lhs,
    const HgiAccelerationStructureDesc& rhs);


///
/// \class HgiAccelerationStructure
///
/// Represents GPU acceleration structure for ray tracing.
/// AccelerationStructures should be created via Hgi::CreateAccelerationStructure.
///
class HgiAccelerationStructure
{
public:
    HGI_API
        virtual ~HgiAccelerationStructure();

    /// The descriptor describes the object.
    HGI_API
        HgiAccelerationStructureDesc const& GetDescriptor() const;

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
    HGI_API
        virtual uint64_t GetRawResource() const = 0;

protected:
    HGI_API
        HgiAccelerationStructure(HgiAccelerationStructureDesc const& desc);

    HgiAccelerationStructureDesc _descriptor;

private:
    HgiAccelerationStructure() = delete;
    HgiAccelerationStructure& operator=(const HgiAccelerationStructure&) = delete;
    HgiAccelerationStructure(const HgiAccelerationStructure&) = delete;


};

using HgiAccelerationStructureHandle = HgiHandle<HgiAccelerationStructure>;
using HgiAccelerationStructureHandleVector = std::vector<HgiAccelerationStructureHandle>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
