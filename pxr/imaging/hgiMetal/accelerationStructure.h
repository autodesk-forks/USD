#line 1 "/Volumes/shared_data/usd/pxr/imaging/hgiMetal/accelerationStructure.h"
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
#ifndef PXR_IMAGING_HGIMETAL_HGI_ACCELERATION_STRUCTURE_H
#define PXR_IMAGING_HGIMETAL_HGI_ACCELERATION_STRUCTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgi/accelerationStructure.h"

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

class HgiMetalAccelerationStructureCmds;

struct HgiAccelerationStructureTriangleGeometryDesc;
struct HgiAccelerationStructureInstanceGeometryDesc;
struct HgiAccelerationStructureDesc;

class HgiMetalBuildableAccelerationStructure
{
public:
    HgiMetalBuildableAccelerationStructure(Hgi* pHgi,
                                           HgiAccelerationStructureTriangleGeometryDesc const& _triangleGeomStructure);
    HgiMetalBuildableAccelerationStructure(Hgi* pHgi,
                                           HgiAccelerationStructureInstanceGeometryDesc const& _instanceGeomStructure);
    HgiMetalBuildableAccelerationStructure(Hgi* pHgi,
                                           HgiAccelerationStructureDesc const& _structure);
    ~HgiMetalBuildableAccelerationStructure() {};
    
    id<MTLBuffer>                                               GetInstanceBuffer() const { return _instancesBuffer; }
    id<MTLAccelerationStructure>                                GetAccelerationStructure() const { return _accelerationStructure; }
    const std::vector<HgiMetalBuildableAccelerationStructure*>& GetSubStructures() const { return _subStructures; }
protected:
    enum class Type
    {
        TriangleGeom,
        InstancedGeom,  //Some pseudo type due to the way things are current setup.
        Instanced,
    };
    
    Type                                _type;
    
    //TriangleGeom ONLY
    MTLAccelerationStructureTriangleGeometryDescriptor* _triangleGeomDesc;
    
    uint32_t                                             _entries;
    std::vector<HgiMetalBuildableAccelerationStructure*> _subStructures;
    
    id<MTLBuffer>                                        _instancesBuffer;
    MTLAccelerationStructureDescriptor*                  _accelerationStructureDesc;
    MTLAccelerationStructureSizes                        _accelerationStructureSizes;
    id<MTLAccelerationStructure>                         _accelerationStructure;
    bool                                                 _isBuilt;
    bool                                                 _isPassthrough;
    
    friend class HgiMetalAccelerationStructureCmds;
};

///
/// \class HgiMetalAccelerationStructureGeometry
///
/// Represents GPU acceleration structure for ray tracing.
/// AccelerationStructureGeometrys should be created via Hgi::CreateAccelerationStructureGeometry.
///
class HgiMetalAccelerationStructureGeometry : public HgiAccelerationStructureGeometry
{
public:
    HGIMETAL_API
        virtual ~HgiMetalAccelerationStructureGeometry();

//    MTLAccelerationStructureTriangleGeometryDescriptor* GetMetalGeometry() {
//        return _triangleGeomDesc;
//    }

//    uint32_t GetPrimitiveCount() { return _primitiveCount;  }

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIMETAL_API
        uint64_t& GetInflightBits() {
        return _inflightBits;
    }

protected:
    friend class HgiMetal;

    HgiMetalAccelerationStructureGeometry(
        Hgi* pHgi,
        HgiAccelerationStructureTriangleGeometryDesc const& desc);
    HgiMetalAccelerationStructureGeometry(Hgi* pHgi,
            HgiAccelerationStructureInstanceGeometryDesc const& desc);


private:
    // HgiMetalDevice* _device;
    uint64_t _inflightBits;

    HgiMetalAccelerationStructureGeometry() = delete;
    HgiMetalAccelerationStructureGeometry& operator=(const HgiMetalAccelerationStructureGeometry&) = delete;
    HgiMetalAccelerationStructureGeometry(const HgiMetalAccelerationStructureGeometry&) = delete;

//    uint32_t                            _primitiveCount;
//    uint32_t                            _instanceCount;
    
    
//    NSMutableArray*                     _accelerationStructures;
//    std::vector<HgiMetalAccelerationStructureGeometry*> _geomSubAccelerationStructures;
//    HgiBufferHandle                     _instancesBuffer;
//
//    MTLAccelerationStructureDescriptor* _accelerationStructureDesc;
//    id<MTLAccelerationStructure>        _accelerationStructure;
//    MTLAccelerationStructureSizes       _accelerationStructureSizes;
//    bool                                _isBuilt;
    
    HgiMetalBuildableAccelerationStructure _accelStructure;
    
//    friend class HgiMetalAccelerationStructure;
    friend class HgiMetalBuildableAccelerationStructure;
};


///
/// \class HgiMetalAccelerationStructure
///
/// Represents GPU acceleration structure for ray tracing.
/// AccelerationStructures should be created via Hgi::CreateAccelerationStructure.
///
class HgiMetalAccelerationStructure : public HgiAccelerationStructure
{
public:
    HGIMETAL_API
        virtual ~HgiMetalAccelerationStructure();

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
    HGIMETAL_API
        uint64_t GetRawResource() const override;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIMETAL_API
        uint64_t& GetInflightBits() {
        return _inflightBits;
    }

//    HGIMETAL_API
//        HgiBufferHandle GetScratchBuffer() {
//        return _scratchBuffer;
//    }
    
    HGIMETAL_API
    id<MTLAccelerationStructure> GetAccelerationStructure() {
        return _accelStructure.GetAccelerationStructure();
    }
    
    HGIMETAL_API
    HgiMetalBuildableAccelerationStructure& GetBuildableAccelerationStructure() {
        return _accelStructure;
    }

protected:
    friend class HgiMetal;
    friend class HgiMetalAccelerationStructureCmds;
    friend class HgiMetalAccelerationStructureGeometry;

    HGIMETAL_API
        HgiMetalAccelerationStructure(
            Hgi* pHgi,
            HgiAccelerationStructureDesc const& desc);

//    MTLAccelerationStructureDescriptor* GetAccelerationStructureDescriptor() {
//        return _accelerationStructureDesc;
//    }
private:
    HgiMetalAccelerationStructure() = delete;
    HgiMetalAccelerationStructure& operator=(const HgiMetalAccelerationStructure&) = delete;
    HgiMetalAccelerationStructure(const HgiMetalAccelerationStructure&) = delete;

    uint64_t _inflightBits;
    
//    std::vector<HgiMetalAccelerationStructureGeometry*> _geomSubAccelerationStructures;
//    HgiBufferHandle                                     _instancesBuffer;
//
//    MTLAccelerationStructureDescriptor*         _accelerationStructureDesc;
//    id<MTLAccelerationStructure>                _accelerationStructure;
//    MTLAccelerationStructureSizes               _accelerationStructureSizes;
//    bool                                        _isBuilt;
    
//    std::vector<uint32_t> _primitiveCounts;
    
//    HgiBufferHandle _scratchBuffer;
    
    HgiMetalBuildableAccelerationStructure _accelStructure;
    
    friend class HgiMetalBuildableAccelerationStructure;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
