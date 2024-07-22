//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/sampler.h"
#include "pxr/imaging/hgiMetal/texture.h"
#include "pxr/imaging/hgiMetal/accelerationStructure.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalResourceBindings::HgiMetalResourceBindings(
    HgiResourceBindingsDesc const& desc)
    : HgiResourceBindings(desc)
{
}

HgiMetalResourceBindings::~HgiMetalResourceBindings() = default;

void
HgiMetalResourceBindings::BindResources(
    HgiMetal *hgi,
    id<MTLRenderCommandEncoder> renderEncoder,
    id<MTLBuffer> argBuffer)
{
    id<MTLArgumentEncoder> argEncoderBuffer = hgi->GetBufferArgumentEncoder();
    id<MTLArgumentEncoder> argEncoderSampler = hgi->GetSamplerArgumentEncoder();
    id<MTLArgumentEncoder> argEncoderTexture = hgi->GetTextureArgumentEncoder();

    //
    // Bind Textures and Samplers
    //

    for (HgiTextureBindDesc const& texDesc : _descriptor.textures) {
        if (!TF_VERIFY(texDesc.textures.size() == 1)) continue;

        id<MTLTexture> metalTexture = nil;
        HgiTextureHandle const& texHandle = texDesc.textures.front();
        HgiMetalTexture* hgiMetalTexture =
            static_cast<HgiMetalTexture*>(texHandle.Get());
        if (hgiMetalTexture) {
            metalTexture = hgiMetalTexture->GetTextureId();
        }

        id<MTLSamplerState> metalSampler = nil;
        if (texDesc.samplers.size()) {
            HgiSamplerHandle const& smpHandle = texDesc.samplers.front();
            HgiMetalSampler* hgiMetalSampler =
                static_cast<HgiMetalSampler*>(smpHandle.Get());
            if (hgiMetalSampler) {
                metalSampler = hgiMetalSampler->GetSamplerId();
            }
        }
        
        if ((texDesc.stageUsage & HgiShaderStageVertex) ||
                texDesc.stageUsage & HgiShaderStagePostTessellationVertex) {
            size_t offsetSampler = HgiMetalArgumentOffsetSamplerVS
                                 + (texDesc.bindingIndex * sizeof(void*));
            [argEncoderSampler setArgumentBuffer:argBuffer
                                          offset:offsetSampler];
            [argEncoderSampler setSamplerState:metalSampler
                                       atIndex:0];

            size_t offsetTexture = HgiMetalArgumentOffsetTextureVS
                                 + (texDesc.bindingIndex * sizeof(void*));
            [argEncoderTexture setArgumentBuffer:argBuffer
                                          offset:offsetTexture];
            [argEncoderTexture setTexture:metalTexture
                                  atIndex:0];
        }

        if (texDesc.stageUsage & HgiShaderStageFragment) {
            size_t offsetSampler = HgiMetalArgumentOffsetSamplerFS
                                 + (texDesc.bindingIndex * sizeof(void*));
            [argEncoderSampler setArgumentBuffer:argBuffer
                                          offset:offsetSampler];
            [argEncoderSampler setSamplerState:metalSampler
                                       atIndex:0];

            size_t offsetTexture = HgiMetalArgumentOffsetTextureFS
                                 + (texDesc.bindingIndex * sizeof(void*));
            [argEncoderTexture setArgumentBuffer:argBuffer
                                          offset:offsetTexture];
            [argEncoderTexture setTexture:metalTexture
                                  atIndex:0];
        }
        if (metalTexture) {
            MTLResourceUsage usageFlags = MTLResourceUsageRead;
            if (metalSampler) {
                usageFlags |= MTLResourceUsageSample;
            }
            if (texDesc.writable) {
                usageFlags |= MTLResourceUsageWrite;
            }
            [renderEncoder useResource:metalTexture
                                 usage:usageFlags];
        }
    }

    [renderEncoder setVertexBuffer:argBuffer
                            offset:HgiMetalArgumentOffsetSamplerVS
                           atIndex:HgiMetalArgumentIndexSamplers];
    [renderEncoder setVertexBuffer:argBuffer
                            offset:HgiMetalArgumentOffsetTextureVS
                           atIndex:HgiMetalArgumentIndexTextures];

    [renderEncoder setFragmentBuffer:argBuffer
                              offset:HgiMetalArgumentOffsetSamplerFS
                             atIndex:HgiMetalArgumentIndexSamplers];
    [renderEncoder setFragmentBuffer:argBuffer
                              offset:HgiMetalArgumentOffsetTextureFS
                             atIndex:HgiMetalArgumentIndexTextures];

    //
    // Bind Buffers
    //

    // Note that index and vertex buffers are not bound here.
    // They are bound via the GraphicsEncoder.

    for (HgiBufferBindDesc const& bufDesc : _descriptor.buffers) {
        if (!TF_VERIFY(bufDesc.buffers.size() == 1)) continue;

        HgiBufferHandle const& bufHandle = bufDesc.buffers.front();
        HgiMetalBuffer* metalbuffer =
            static_cast<HgiMetalBuffer*>(bufHandle.Get());
        
        id<MTLBuffer> bufferId = metalbuffer->GetBufferId();
        NSUInteger offset = bufDesc.offsets.front();
        
        if (bufDesc.resourceType == HgiBindResourceTypeTessFactors) {
            [renderEncoder setTessellationFactorBuffer:bufferId
                                                offset:offset
                                        instanceStride:0];
            // Tess factors buffers need no futher binding.
            continue;
        }

        if ((bufDesc.stageUsage & HgiShaderStageVertex) ||
            (bufDesc.stageUsage & HgiShaderStagePostTessellationControl) ||
            (bufDesc.stageUsage & HgiShaderStagePostTessellationVertex)) {

            NSUInteger argBufferOffset = HgiMetalArgumentOffsetBufferVS
                                       + bufDesc.bindingIndex * sizeof(void*);
            [argEncoderBuffer setArgumentBuffer:argBuffer
                                         offset:argBufferOffset];
            [argEncoderBuffer setBuffer:bufferId offset:offset atIndex:0];
        }
        
        if (bufDesc.stageUsage & HgiShaderStageFragment) {
            NSUInteger argBufferOffset = HgiMetalArgumentOffsetBufferFS
                                       + bufDesc.bindingIndex * sizeof(void*);
            [argEncoderBuffer setArgumentBuffer:argBuffer
                                         offset:argBufferOffset];
            [argEncoderBuffer setBuffer:bufferId offset:offset atIndex:0];
        }
        MTLResourceUsage usageFlags = MTLResourceUsageRead;
        if (bufDesc.writable) {
            usageFlags |= MTLResourceUsageWrite;
        }

        [renderEncoder useResource:bufferId
                             usage:usageFlags];
    }
    

    [renderEncoder setVertexBuffer:argBuffer
                            offset:HgiMetalArgumentOffsetBufferVS
                           atIndex:HgiMetalArgumentIndexBuffers];
    [renderEncoder setFragmentBuffer:argBuffer
                              offset:HgiMetalArgumentOffsetBufferFS
                             atIndex:HgiMetalArgumentIndexBuffers];

    // Bind constants

    {
        [argEncoderBuffer setArgumentBuffer:argBuffer
                                     offset:HgiMetalArgumentOffsetConstants];
    }

    [renderEncoder setVertexBuffer:argBuffer
                            offset:HgiMetalArgumentOffsetConstants
                           atIndex:HgiMetalArgumentIndexConstants];
    [renderEncoder setFragmentBuffer:argBuffer
                              offset:HgiMetalArgumentOffsetConstants
                             atIndex:HgiMetalArgumentIndexConstants];
}

void
HgiMetalResourceBindings::BindResources(
    HgiMetal *hgi,
    id<MTLComputeCommandEncoder> computeEncoder,
    id<MTLBuffer> argBuffer)
{
    id<MTLArgumentEncoder> argEncoderBuffer = hgi->GetBufferArgumentEncoder();
    id<MTLArgumentEncoder> argEncoderSampler = hgi->GetSamplerArgumentEncoder();
    id<MTLArgumentEncoder> argEncoderTexture = hgi->GetTextureArgumentEncoder();

    //
    // Bind Textures and Samplers
    //

    for (HgiTextureBindDesc const& texDesc : _descriptor.textures) {
        if (!TF_VERIFY(texDesc.textures.size() == 1)) 
            continue;

        HgiTextureHandle const& texHandle = texDesc.textures.front();
        HgiMetalTexture* metalTexture =
            static_cast<HgiMetalTexture*>(texHandle.Get());

        HgiSamplerHandle const& smpHandle = texDesc.samplers.front();
        HgiMetalSampler* metalSmp =
            static_cast<HgiMetalSampler*>(smpHandle.Get());

        if ((texDesc.stageUsage & HgiShaderStageCompute) ||
            (texDesc.stageUsage & HgiShaderStageRayGen) ||
            (texDesc.stageUsage & HgiShaderStageClosestHit) ||
            (texDesc.stageUsage & HgiShaderStageIntersection) ||
            (texDesc.stageUsage & HgiShaderStageAnyHit) ||
            (texDesc.stageUsage & HgiShaderStageCallable) ||
            (texDesc.stageUsage & HgiShaderStageMiss)) {
            size_t offsetSampler = HgiMetalArgumentOffsetSamplerCS
                                 + (texDesc.bindingIndex * sizeof(void*));
            [argEncoderSampler setArgumentBuffer:argBuffer
                                          offset:offsetSampler];
            [argEncoderSampler setSamplerState:metalSmp->GetSamplerId() atIndex:0];

            size_t offsetTexture = HgiMetalArgumentOffsetTextureCS
                                 + (texDesc.bindingIndex * sizeof(void*));
            [argEncoderTexture setArgumentBuffer:argBuffer
                                          offset:offsetTexture];
            MTLResourceUsage usage = MTLResourceUsageRead;
            if (texDesc.writable) {
                usage |= MTLResourceUsageWrite;
            }
            [argEncoderTexture setTexture:metalTexture->GetTextureId() atIndex:0];
            if (metalSmp) {
                usage |= MTLResourceUsageSample;
            }
            [computeEncoder useResource:metalTexture->GetTextureId()
                                  usage:usage];
        }
    }

    [computeEncoder setBuffer:argBuffer
                       offset:HgiMetalArgumentOffsetSamplerCS
                      atIndex:HgiMetalArgumentIndexSamplers];
    [computeEncoder setBuffer:argBuffer
                       offset:HgiMetalArgumentOffsetTextureCS
                      atIndex:HgiMetalArgumentIndexTextures];

    //
    // Bind Buffers
    //

    // Note that index and vertex buffers are not bound here.
    // They are bound via the GraphicsEncoder.

    for (HgiBufferBindDesc const& bufDesc : _descriptor.buffers) {
        if (!TF_VERIFY(bufDesc.buffers.size() == 1)) continue;
        if ((bufDesc.stageUsage & HgiShaderStageCompute) ||
            (bufDesc.stageUsage & HgiShaderStageRayGen) ||
            (bufDesc.stageUsage & HgiShaderStageClosestHit) ||
            (bufDesc.stageUsage & HgiShaderStageIntersection) ||
            (bufDesc.stageUsage & HgiShaderStageAnyHit) ||
            (bufDesc.stageUsage & HgiShaderStageCallable) ||
            (bufDesc.stageUsage & HgiShaderStageMiss))
        {
            HgiBufferHandle const& bufHandle = bufDesc.buffers.front();
            HgiMetalBuffer* metalbuffer =
            static_cast<HgiMetalBuffer*>(bufHandle.Get());
            
            id<MTLBuffer> bufferId = metalbuffer->GetBufferId();
            NSUInteger offset = bufDesc.offsets.front();
            size_t argBufferOffset = HgiMetalArgumentOffsetBufferCS
            + bufDesc.bindingIndex * sizeof(void*);
            [argEncoderBuffer setArgumentBuffer:argBuffer
                                         offset:argBufferOffset];
            [argEncoderBuffer setBuffer:bufferId offset:offset atIndex:0];
            MTLResourceUsage usage = MTLResourceUsageRead;
            if (bufDesc.writable) {
                usage |= MTLResourceUsageWrite;
            }
            [computeEncoder useResource:bufferId
                                  usage:usage];
        }
    }
    
    [computeEncoder setBuffer:argBuffer
                       offset:HgiMetalArgumentOffsetBufferCS 
                      atIndex:HgiMetalArgumentIndexBuffers];

    //
    // Bind Constants
    //

    {
        [argEncoderBuffer setArgumentBuffer:argBuffer
                                     offset:HgiMetalArgumentOffsetConstants];
    }
    
    [computeEncoder setBuffer:argBuffer
                       offset:HgiMetalArgumentOffsetConstants
                      atIndex:HgiMetalArgumentIndexConstants];
    
    //
    // Bind Acceleration Structures
    //
    
    bool set = false;
    for(auto it = _descriptor.accelerationStructures.begin(); it != _descriptor.accelerationStructures.end(); it++)
    {
        NSUInteger bufferIndex = (*it).bindingIndex;
        for(auto it_array = (*it).accelerationStructures.begin(); it_array != (*it).accelerationStructures.end(); it_array++)
        {
            HgiMetalAccelerationStructure* hgiAccelStruct = (HgiMetalAccelerationStructure*)(*it_array).Get();
            [computeEncoder setAccelerationStructure:hgiAccelStruct->GetAccelerationStructure() atBufferIndex:bufferIndex++];
            
            //Bind referenced structures
            std::function<void(HgiMetalBuildableAccelerationStructure&, uint32)> nestedRef = [&](HgiMetalBuildableAccelerationStructure& currentStruct, uint32 level) {
                [computeEncoder useResource:currentStruct.GetAccelerationStructure() usage:MTLResourceUsageRead];
                id<MTLBuffer> instanceBuffer = currentStruct.GetInstanceBuffer();
                if(instanceBuffer)
                    [computeEncoder useResource:instanceBuffer usage:MTLResourceUsageRead];
                const auto& subStructures = currentStruct.GetSubStructures();
                for(auto it = subStructures.begin(); it != subStructures.end(); it++)
                    nestedRef(*(*it), level + 1);
//                if(!set && level == 2)
//                {
//                    [computeEncoder setAccelerationStructure:currentStruct.GetAccelerationStructure() atBufferIndex:bufferIndex];
//                    set = true;
//                }
            };
            
            nestedRef(hgiAccelStruct->GetBuildableAccelerationStructure(), 0);
        }
    }
  }

void HgiMetalResourceBindings::SetConstantValues(
    id<MTLBuffer> argumentBuffer,
    HgiShaderStage stages,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    if (argumentBuffer.length - HgiMetalArgumentOffsetConstants < byteSize) {
        TF_CODING_ERROR("Not enough space reserved for constants");
        byteSize = argumentBuffer.length - HgiMetalArgumentOffsetConstants;
    }
    uint8_t* bufferContents = (uint8_t*) [argumentBuffer contents];
    memcpy(bufferContents + HgiMetalArgumentOffsetConstants, data, byteSize);
}


PXR_NAMESPACE_CLOSE_SCOPE
