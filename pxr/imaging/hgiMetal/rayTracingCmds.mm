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
#include "pxr/imaging/hgiMetal/rayTracingCmds.h"
#include "pxr/imaging/hgiMetal/accelerationStructure.h"
#include "pxr/imaging/hgiMetal/computePipeline.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalRayTracingCmds::HgiMetalRayTracingCmds(HgiMetal* hgi)
    : HgiRayTracingCmds()
    , _hgi(hgi)
    , _pipelineState(nullptr)
    , _commandBuffer(nil)
    , _argumentBuffer(nil)
    , _encoder(nil)
    , _secondaryCommandBuffer(false)
{
    _CreateEncoder();
}

HgiMetalRayTracingCmds::~HgiMetalRayTracingCmds()
{
    TF_VERIFY(_encoder == nil, "Encoder created, but never commited.");
}

void
HgiMetalRayTracingCmds::_CreateEncoder()
{
    if (!_encoder) {
        _commandBuffer = _hgi->GetPrimaryCommandBuffer(this);
        if (_commandBuffer == nil) {
            _commandBuffer = _hgi->GetSecondaryCommandBuffer();
            _secondaryCommandBuffer = true;
        }
        _encoder = [_commandBuffer computeCommandEncoder];
    }
}

void
HgiMetalRayTracingCmds::_CreateArgumentBuffer()
{
    if (!_argumentBuffer) {
        _argumentBuffer = _hgi->GetArgBuffer();
    }
}

void
HgiMetalRayTracingCmds::PushDebugGroup(const char* label)
{
    _CreateEncoder();
    HGIMETAL_DEBUG_PUSH_GROUP(_encoder, label)
}

void
HgiMetalRayTracingCmds::PopDebugGroup()
{
    if (_encoder) {
        HGIMETAL_DEBUG_POP_GROUP(_encoder)
    }
}

void  HgiMetalRayTracingCmds::HgiMetalRayTracingCmds::BindPipeline(HgiRayTracingPipelineHandle pipeline)
{
    _CreateEncoder();
    _pipelineState = static_cast<HgiMetalRayTracingPipeline*>(pipeline.Get());
    _pipelineState->BindPipeline(_encoder);
}

void  HgiMetalRayTracingCmds::BindResources(HgiResourceBindingsHandle r)
{
    if (HgiMetalResourceBindings* rb=
        static_cast<HgiMetalResourceBindings*>(r.Get()))
    {
        _CreateEncoder();
        _CreateArgumentBuffer();

        rb->BindResources(_hgi, _encoder, _argumentBuffer);
    }
}

void HgiMetalRayTracingCmds::TraceRays(uint32_t dimX, uint32_t dimY, uint32_t dimZ)
{
    if (dimX == 0 || dimY == 0 || dimZ == 0) {
        return;
    }

    uint32_t maxTotalThreads =
        (uint32_t)[_pipelineState->GetMetalPipelineState() maxTotalThreadsPerThreadgroup];
    uint32_t exeWidth =
        (uint32_t)[_pipelineState->GetMetalPipelineState() threadExecutionWidth];

    uint32_t thread_width, thread_height, thread_depth;
    thread_width = MIN(maxTotalThreads, exeWidth);
    if (dimY == 1 && dimZ == 1) {
        thread_height = 1;
        thread_depth = 1;
    }
    else if(dimZ == 1) {
        thread_width = exeWidth;
        thread_height = maxTotalThreads / thread_width;
        thread_depth = 1;
    }
    else {
        uint32_t dim = (uint32_t)powf((float)(maxTotalThreads / exeWidth), 1.f / 3.f);
        thread_width = dim;
        thread_height = dim;
        thread_depth = dim;
    }

    if (_argumentBuffer.storageMode != MTLStorageModeShared &&
        [_argumentBuffer respondsToSelector:@selector(didModifyRange:)]) {
        NSRange range = NSMakeRange(0, _argumentBuffer.length);

        ARCH_PRAGMA_PUSH
        ARCH_PRAGMA_INSTANCE_METHOD_NOT_FOUND
        [_argumentBuffer didModifyRange:range];
        ARCH_PRAGMA_POP
    }
    
    const HgiMetalRayTracingShaderBindingTable& table = _pipelineState->GetShaderBindingTable();
    //[_encoder setIntersectionFunctionTable:table.intersectionShaderBindingTable atBufferIndex:5];
    [_encoder setVisibleFunctionTable:table.hitShaderBindingTable atBufferIndex:6];
    [_encoder setVisibleFunctionTable:table.missShaderBindingTable atBufferIndex:7];

    [_encoder dispatchThreads:MTLSizeMake(dimX, dimY, dimZ)
        threadsPerThreadgroup:MTLSizeMake(MIN(thread_width, dimX),
                                          MIN(thread_height, dimY),
                                          MIN(thread_depth, dimZ))];

    _hasWork = true;
    _argumentBuffer = nil;

    // HgiMetalDevice* device = _commandBuffer->GetDevice();
    // VkCommandBuffer vkCommandBuffer = _commandBuffer->GetMetalCommandBuffer();
 
    // HgiMetalRayTracingPipeline* pVkPipeline = (HgiMetalRayTracingPipeline*)_pipeline.Get();
    // auto &shaderBindingTable = pVkPipeline->GetShaderBindingTable();


    // HgiMetalBuffer* pRaygenShaderBindingTableBufferVk = (HgiMetalBuffer*)shaderBindingTable.raygenShaderBindingTable.Get();
    // HgiMetalBuffer* pMissShaderBindingTableBufferVk = (HgiMetalBuffer*)shaderBindingTable.missShaderBindingTable.Get();
    // HgiMetalBuffer* pHitShaderBindingTableBufferVk = (HgiMetalBuffer*)shaderBindingTable.hitShaderBindingTable.Get();

    // VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    // raygenShaderSbtEntry.deviceAddress = pRaygenShaderBindingTableBufferVk->GetDeviceAddress();
    // raygenShaderSbtEntry.stride = shaderBindingTable.raygenShaderBindingTableStride;
    // raygenShaderSbtEntry.size = pRaygenShaderBindingTableBufferVk->GetDescriptor().byteSize;

    // VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    // missShaderSbtEntry.deviceAddress = pMissShaderBindingTableBufferVk->GetDeviceAddress();
    // missShaderSbtEntry.stride = shaderBindingTable.missShaderBindingTableStride;
    // missShaderSbtEntry.size = pMissShaderBindingTableBufferVk->GetDescriptor().byteSize;

    // VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    // hitShaderSbtEntry.deviceAddress = pHitShaderBindingTableBufferVk->GetDeviceAddress();
    // hitShaderSbtEntry.stride = shaderBindingTable.hitShaderBindingTableStride;
    // hitShaderSbtEntry.size = pHitShaderBindingTableBufferVk->GetDescriptor().byteSize;

    // VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    /*
        Dispatch the ray tracing commands
    */
    //vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline.pipeline);
    //vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline.pipelineLayout, 0, 1, &descriptorSets.descriptorSet, 0, 0);



    // device->vkCmdTraceRaysKHR(
    //     vkCommandBuffer,
    //     &raygenShaderSbtEntry,
    //     &missShaderSbtEntry,
    //     &hitShaderSbtEntry,
    //     &callableShaderSbtEntry,
    //     width,
    //     height,
    //     1);
}

bool
HgiMetalRayTracingCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
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
    _argumentBuffer = nil;

    return submittedWork;
}

PXR_NAMESPACE_CLOSE_SCOPE
