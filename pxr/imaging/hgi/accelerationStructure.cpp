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
#include "pxr/imaging/hgi/accelerationStructure.h"

PXR_NAMESPACE_OPEN_SCOPE


bool operator==(const HgiAccelerationStructureDesc& lhs,
    const HgiAccelerationStructureDesc& rhs)
{
    return  lhs.debugName == rhs.debugName &&
        lhs.geometry == rhs.geometry &&
        lhs.type == rhs.type
        ;
}

bool operator!=(const HgiAccelerationStructureDesc& lhs,
    const HgiAccelerationStructureDesc& rhs)
{
    return !(lhs == rhs);
}

HgiAccelerationStructure::HgiAccelerationStructure(HgiAccelerationStructureDesc const& desc)
    : _descriptor(desc)
{
}

HgiAccelerationStructure::~HgiAccelerationStructure() = default;

HgiAccelerationStructureDesc const&
HgiAccelerationStructure::GetDescriptor() const
{
    return _descriptor;
}

bool operator==(const HgiAccelerationStructureTriangleGeometryDesc& lhs,
    const HgiAccelerationStructureTriangleGeometryDesc& rhs)
{
    return  lhs.debugName == rhs.debugName && 
        lhs.vertexFormat == rhs.vertexFormat && 
        lhs.vertexData==rhs.vertexData &&
        lhs.maxVertex == rhs.maxVertex &&
        lhs.indexType == rhs.indexType &&
        lhs.indexData == rhs.indexData &&
        lhs.transformData == rhs.transformData &&
        lhs.flags == rhs.flags
        ;
}

bool operator!=(const HgiAccelerationStructureTriangleGeometryDesc& lhs,
    const HgiAccelerationStructureTriangleGeometryDesc& rhs)
{
    return !(lhs == rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE
