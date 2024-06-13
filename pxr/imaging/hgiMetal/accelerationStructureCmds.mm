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
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/computeCmds.h"
#include "pxr/imaging/hgiMetal/accelerationStructureCmds.h"
#include "pxr/imaging/hgiMetal/accelerationStructure.h"
#include "pxr/imaging/hgiMetal/computePipeline.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalAccelerationStructureCmds::HgiMetalAccelerationStructureCmds(HgiMetal* hgi)
    : HgiAccelerationStructureCmds()
    , _hgi(hgi)
{
//    MTLCaptureManager* cptMgr = [MTLCaptureManager sharedCaptureManager];
//    MTLCaptureDescriptor* cpt_desc = [MTLCaptureDescriptor new];
//    cpt_desc.captureObject = ((HgiMetal*)hgi)->GetPrimaryDevice();
//    NSError* error;
//    [cptMgr startCaptureWithDescriptor:cpt_desc error:&error];
    _CreateEncoder();
}

HgiMetalAccelerationStructureCmds::~HgiMetalAccelerationStructureCmds()
{
    TF_VERIFY(_encoder == nil, "Encoder created, but never commited.");
}

void
HgiMetalAccelerationStructureCmds::_CreateEncoder()
{
    if (!_encoder) {
        //_commandBuffer = _hgi->GetPrimaryCommandBuffer(this);
        if (_commandBuffer == nil) {
            _commandBuffer = _hgi->GetSecondaryCommandBuffer();
            _secondaryCommandBuffer = true;
        }
        _encoder = [_commandBuffer accelerationStructureCommandEncoder];
    }
}

void
HgiMetalAccelerationStructureCmds::PushDebugGroup(const char* label)
{
    _CreateEncoder();
    HGIMETAL_DEBUG_PUSH_GROUP(_encoder, label)
}

void
HgiMetalAccelerationStructureCmds::PopDebugGroup()
{
    if (_encoder) {
        HGIMETAL_DEBUG_POP_GROUP(_encoder)
    }
}


void
HgiMetalAccelerationStructureCmds::Build(HgiAccelerationStructureHandleVector accelStructures, const std::vector<HgiAccelerationStructureBuildRange>& ranges)
{
    assert(ranges.size() == accelStructures.size());

    id<MTLDevice> device = _hgi->GetPrimaryDevice();
    std::function<void(HgiMetalBuildableAccelerationStructure&)> buildStructure = [&](HgiMetalBuildableAccelerationStructure& structure) {
        for(auto it = structure._subStructures.begin(); it != structure._subStructures.end(); it++)
            buildStructure(*(*it));
        
        if(structure._isPassthrough)
            return;
        
        if(!structure._isBuilt)
        {
            NSUInteger scratchBufferSize = structure._accelerationStructureSizes.buildScratchBufferSize;
            
            id <MTLBuffer> scratchBuffer = [device newBufferWithLength:scratchBufferSize options:MTLResourceStorageModePrivate];
            
            [_encoder buildAccelerationStructure:structure._accelerationStructure
                                      descriptor:structure._accelerationStructureDesc
                                   scratchBuffer:scratchBuffer
                             scratchBufferOffset:0];
            
            [scratchBuffer release];
            
            structure._isBuilt = true;
        }
        
        [_encoder useResource:structure._accelerationStructure usage:MTLResourceUsageRead];
        if(structure._instancesBuffer)
            [_encoder useResource:structure._instancesBuffer usage:MTLResourceUsageRead];
    };
    
    for (int i = 0; i < ranges.size(); i++)
    {
        HgiMetalAccelerationStructure& meshStructure = *(HgiMetalAccelerationStructure*)accelStructures[i].Get();
        buildStructure(meshStructure._accelStructure);
    }
}

bool
HgiMetalAccelerationStructureCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    bool submittedWork = false;
    if (_encoder) {
        [_encoder endEncoding];
        _encoder = nil;
        submittedWork = true;

        HgiMetal::CommitCommandBufferWaitType waitType;
        switch(wait) {
            case HgiSubmitWaitTypeNoWait:
                waitType = HgiMetal::CommitCommandBuffer_NoWait;
                break;
            case HgiSubmitWaitTypeWaitUntilCompleted:
                waitType = HgiMetal::CommitCommandBuffer_WaitUntilCompleted;
                break;
        }

        if (_secondaryCommandBuffer) {
            _hgi->CommitSecondaryCommandBuffer(_commandBuffer, waitType);
        }
        else {
            _hgi->CommitPrimaryCommandBuffer(waitType);
        }
    }
    
    if (_secondaryCommandBuffer) {
        _hgi->ReleaseSecondaryCommandBuffer(_commandBuffer);
    }
    _commandBuffer = nil;
//    _argumentBuffer = nil;

    return submittedWork;

}

PXR_NAMESPACE_CLOSE_SCOPE
