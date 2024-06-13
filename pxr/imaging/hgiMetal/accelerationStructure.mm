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
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/accelerationStructure.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalBuildableAccelerationStructure::HgiMetalBuildableAccelerationStructure(
    Hgi* pHgi, HgiAccelerationStructureTriangleGeometryDesc const& _hgiDesc)
    : _type(Type::TriangleGeom), _isBuilt(false), _isPassthrough(false)
{
    _triangleGeomDesc = [MTLAccelerationStructureTriangleGeometryDescriptor new];
    
    _triangleGeomDesc.label = [NSString stringWithUTF8String:_hgiDesc.debugName.c_str()];
    _triangleGeomDesc.indexType = HgiMetalConversions::GetIndexType(_hgiDesc.indexType);
    
    HgiMetalBuffer* indexBuffer = (HgiMetalBuffer*)_hgiDesc.indexData.Get();
    HgiMetalBuffer* vertexBuffer = (HgiMetalBuffer*)_hgiDesc.vertexData.Get();
    
    _triangleGeomDesc.indexBuffer = indexBuffer->GetBufferId();
    _triangleGeomDesc.indexBufferOffset = 0;

    _triangleGeomDesc.vertexBuffer = vertexBuffer->GetBufferId();
    _triangleGeomDesc.vertexFormat = HgiMetalConversions::GetAttributeFormat(_hgiDesc.vertexFormat);
    _triangleGeomDesc.vertexStride = _hgiDesc.vertexStride;
    _triangleGeomDesc.vertexBufferOffset = 0;

    _triangleGeomDesc.triangleCount = _hgiDesc.count;
    
    _triangleGeomDesc.primitiveDataBuffer = nil;
    _triangleGeomDesc.primitiveDataBufferOffset = 0;
    _triangleGeomDesc.primitiveDataStride = 0;
    _triangleGeomDesc.primitiveDataElementSize = 0;
    
    //_triangleGeomDesc.intersectionFunctionTableOffset = 0;
    
    _entries = _hgiDesc.count;
    
    MTLPrimitiveAccelerationStructureDescriptor *accelDescriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
    accelDescriptor.geometryDescriptors = @[ _triangleGeomDesc ];
    accelDescriptor.usage = MTLAccelerationStructureUsageNone;
    
    id<MTLDevice> _device = ((HgiMetal*)pHgi)->GetPrimaryDevice();
    _accelerationStructureDesc = accelDescriptor;
    _accelerationStructureSizes = [_device accelerationStructureSizesWithDescriptor:accelDescriptor];
    _accelerationStructure = [_device newAccelerationStructureWithSize:_accelerationStructureSizes.accelerationStructureSize];
}

HgiMetalBuildableAccelerationStructure::HgiMetalBuildableAccelerationStructure(
    Hgi* pHgi, HgiAccelerationStructureInstanceGeometryDesc const& _hgiDesc)
    : _type(Type::InstancedGeom), _isBuilt(false), _isPassthrough(false)
{
    id<MTLDevice> device = ((HgiMetal*)pHgi)->GetPrimaryDevice();
    
    NSUInteger instanceBufferSize = sizeof(MTLAccelerationStructureInstanceDescriptor) * _hgiDesc.instances.size();
    _instancesBuffer = [device newBufferWithLength:instanceBufferSize options:MTLResourceStorageModeShared];
    
    NSMutableArray* accelerationStructures = [[NSMutableArray alloc] init];
    
    MTLAccelerationStructureInstanceDescriptor* instanceDescriptors = (MTLAccelerationStructureInstanceDescriptor*)_instancesBuffer.contents;
    for(int i = 0; i < _hgiDesc.instances.size(); i++)
    {
        const HgiAccelerationStructureInstanceDesc& instance = _hgiDesc.instances[i];
        
        HgiMetalAccelerationStructure* blas = (HgiMetalAccelerationStructure*)instance.blas.Get();
        [accelerationStructures addObject:blas->_accelStructure._accelerationStructure];
        _subStructures.push_back(&blas->_accelStructure);
        
        MTLAccelerationStructureInstanceDescriptor& instanceDesc = instanceDescriptors[i];
        instanceDesc.options = MTLAccelerationStructureInstanceOptionOpaque;
        instanceDesc.mask = 2; //instance.mask;
        instanceDesc.accelerationStructureIndex = (uint32)i;
        instanceDesc.intersectionFunctionTableOffset = 0;
        
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 4; k++) {
                instanceDesc.transformationMatrix[k][j] = instance.transform[j][k];
            }
        }
    }
    
    _entries = (uint32_t)_hgiDesc.instances.size();

    MTLInstanceAccelerationStructureDescriptor *accelDescriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];
    
    accelDescriptor.instancedAccelerationStructures = accelerationStructures;
    accelDescriptor.instanceCount = _hgiDesc.instances.size();
    accelDescriptor.instanceDescriptorBuffer = _instancesBuffer;
    accelDescriptor.instanceDescriptorBufferOffset = 0;
    accelDescriptor.instanceDescriptorStride = sizeof(MTLAccelerationStructureInstanceDescriptor);
    accelDescriptor.usage = MTLAccelerationStructureUsageNone;
    
    _accelerationStructureDesc = accelDescriptor;
    _accelerationStructureSizes = [device accelerationStructureSizesWithDescriptor:accelDescriptor];
    _accelerationStructure = [device newAccelerationStructureWithSize:_accelerationStructureSizes.accelerationStructureSize];
}

HgiMetalBuildableAccelerationStructure::HgiMetalBuildableAccelerationStructure(
    Hgi* pHgi, HgiAccelerationStructureDesc const& _hgiDesc)
    : _type(Type::Instanced), _isBuilt(false), _isPassthrough(false)
{
    id<MTLDevice> device = ((HgiMetal*)pHgi)->GetPrimaryDevice();

    if(_hgiDesc.geometry.size() == 1)
    {
        //Do a pass through instead
        HgiMetalAccelerationStructureGeometry* geom = (HgiMetalAccelerationStructureGeometry*)_hgiDesc.geometry[0].Get();
        _accelerationStructureDesc = geom->_accelStructure._accelerationStructureDesc;
        _accelerationStructure = geom->_accelStructure._accelerationStructure;
        _accelerationStructureSizes = geom->_accelStructure._accelerationStructureSizes;
        _isBuilt = true;
        _isPassthrough = true;
        
        _subStructures.push_back(&geom->_accelStructure);
    }
    else
    {
        NSUInteger instanceBufferSize = sizeof(MTLAccelerationStructureInstanceDescriptor) * _hgiDesc.geometry.size();
        _instancesBuffer = [device newBufferWithLength:instanceBufferSize options:MTLResourceStorageModeShared];
        
        MTLInstanceAccelerationStructureDescriptor* instAccDesc = [MTLInstanceAccelerationStructureDescriptor new];
        NSMutableArray* instAccelArray = [NSMutableArray new];
        MTLAccelerationStructureInstanceDescriptor* instDescArray = (MTLAccelerationStructureInstanceDescriptor*)_instancesBuffer.contents;
        const size_t numInstances = _hgiDesc.geometry.size();
        for(size_t i = 0; i < numInstances; i++)
        {
            HgiMetalAccelerationStructureGeometry* geom = (HgiMetalAccelerationStructureGeometry*)_hgiDesc.geometry[i].Get();
            [instAccelArray addObject:geom->_accelStructure._accelerationStructure];
            _subStructures.push_back(&geom->_accelStructure);
            
            MTLAccelerationStructureInstanceDescriptor& instDesc = instDescArray[i];
            instDesc.options = MTLAccelerationStructureInstanceOptionOpaque;
            instDesc.mask = 2; //?
            instDesc.accelerationStructureIndex = (uint32)i;
            instDesc.intersectionFunctionTableOffset = 0;
            
            for (int j = 0; j < 3; j++) {
                for (int k = 0; k < 4; k++) {
                    instDesc.transformationMatrix[k][j] = j == k ? 1.f : 0.f;
                }
            }
            
            instDesc.transformationMatrix[3][2] = -2.f;
        }
        
        instAccDesc.instanceCount = instAccelArray.count;
        instAccDesc.instancedAccelerationStructures = instAccelArray;
        instAccDesc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeDefault;
        instAccDesc.instanceDescriptorBuffer = _instancesBuffer;
        instAccDesc.instanceDescriptorBufferOffset = 0;
        instAccDesc.instanceDescriptorStride = sizeof(MTLAccelerationStructureInstanceDescriptor);
        
        _accelerationStructureDesc = instAccDesc;
        
        _accelerationStructureSizes = [device accelerationStructureSizesWithDescriptor:_accelerationStructureDesc];
        _accelerationStructure = [device newAccelerationStructureWithSize:_accelerationStructureSizes.accelerationStructureSize];
    }
}

HgiMetalAccelerationStructureGeometry::HgiMetalAccelerationStructureGeometry(
    Hgi *pHgi,
    HgiAccelerationStructureTriangleGeometryDesc const& desc) : HgiAccelerationStructureGeometry(desc), _accelStructure(pHgi, desc)
{

}

HgiMetalAccelerationStructureGeometry::~HgiMetalAccelerationStructureGeometry() {

}

HgiMetalAccelerationStructureGeometry::HgiMetalAccelerationStructureGeometry(Hgi* pHgi,
    HgiAccelerationStructureInstanceGeometryDesc const& desc) : HgiAccelerationStructureGeometry(desc), _accelStructure(pHgi, desc)
{

}


uint64_t
HgiMetalAccelerationStructure::GetRawResource() const
{
    assert(0);
    // return (uint64_t)_accelerationStructure;
}


HgiMetalAccelerationStructure::HgiMetalAccelerationStructure(
    Hgi* pHgi,
    HgiAccelerationStructureDesc const& desc): HgiAccelerationStructure(desc), _accelStructure(pHgi, desc)
{

}

HgiMetalAccelerationStructure::~HgiMetalAccelerationStructure() {
//    [_accelerationStructureDesc release];
//    assert(0);
    // This is called from inside garbage collector, so can just call delete on buffer pointer.
//    delete _scratchBuffer.Get();
//    delete _accelStructureBuffer.Get();

    // _device->vkDestroyAccelerationStructureKHR(_device->GetMetalDevice(), _accelerationStructure,
    //     HgiMetalAllocator());
}

PXR_NAMESPACE_CLOSE_SCOPE
