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
#ifndef PXR_IMAGING_ACCELERATION_STRUCTURE_CMDS_H
#define PXR_IMAGING_ACCELERATION_STRUCTURE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/accelerationStructure.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/cmds.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HgiAccelerationStructureCmdsUniquePtr = std::unique_ptr<class HgiAccelerationStructureCmds>;


struct HgiAccelerationStructureBuildRange {
    uint32_t    primitiveCount = 0;
    uint32_t    primitiveOffset = 0;
    uint32_t    firstVertex = 0;
    uint32_t    transformOffset = 0;
};

/// \class HgiAccelerationStructureCmds
///
/// A graphics API independent abstraction of compute commands.
/// HgiAccelerationStructureCmds is a lightweight object that cannot be re-used after it has
/// been submitted. A new cmds object should be acquired for each frame.
///
class HgiAccelerationStructureCmds : public HgiCmds
{
public:
    HGI_API
    ~HgiAccelerationStructureCmds() override;

    /// Push a debug marker.
    HGI_API
    virtual void PushDebugGroup(const char* label) = 0;

    /// Pop the last debug marker.
    HGI_API
    virtual void PopDebugGroup() = 0;
    /// Execute a compute shader with provided thread group count in each
    /// dimension.
    HGI_API
    virtual void Build(HgiAccelerationStructureHandleVector accelStructures, const std::vector<HgiAccelerationStructureBuildRange> &ranges) = 0;
protected:
    HGI_API
    HgiAccelerationStructureCmds();

private:
    HgiAccelerationStructureCmds & operator=(const HgiAccelerationStructureCmds&) = delete;
    HgiAccelerationStructureCmds(const HgiAccelerationStructureCmds&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
