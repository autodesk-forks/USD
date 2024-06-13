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
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgiMetal/rayTracingPipeline.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"
#include "pxr/imaging/hgiMetal/shaderProgram.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalRayTracingPipeline::HgiMetalRayTracingPipeline(
    HgiMetal* hgi,
    HgiRayTracingPipelineDesc const& desc)
    : HgiRayTracingPipeline(desc)
    , _inflightBits(0)
    , _pipelineState(nil)
    , _hgi((HgiMetal*)hgi)
{
    id<MTLDevice> device = _hgi->GetPrimaryDevice();
    
    id<MTLFunction> entryFunction = nil;
    NSMutableArray<id<MTLFunction>>* linkedFunctionsArray = [NSMutableArray<id<MTLFunction>> new];
    
    for(auto it = desc.shaders.begin(); it != desc.shaders.end(); it++)
    {
        HgiMetalShaderFunction* function = (HgiMetalShaderFunction*)(*it).shader.Get();
        if(function->GetDescriptor().shaderStage == HgiShaderStageRayGen)
            entryFunction = function->GetShaderId();
        else
            [linkedFunctionsArray addObject:function->GetShaderId()];
    }
    MTLLinkedFunctions *linkedFunctions = nil;
    if (linkedFunctionsArray) {
        linkedFunctions = [[MTLLinkedFunctions alloc] init];
        linkedFunctions.functions = linkedFunctionsArray;
    }

    //Create pipeline
    {
        MTLComputePipelineDescriptor *descriptor = [[MTLComputePipelineDescriptor alloc] init];
        
        descriptor.computeFunction = entryFunction;
        descriptor.linkedFunctions = linkedFunctions;
        
        //descriptor.threadGroupSizeIsMultipleOfThreadExecutionWidth = YES;
        
        NSError *error;
        
        // Create the compute pipeline state.
        _pipelineState = [device newComputePipelineStateWithDescriptor:descriptor
                                                               options:0
                                                            reflection:nil
                                                                 error:&error];
        assert(_pipelineState);
    }
    
    BuildShaderBindingTable();
    
    // for (int i = 0; i < desc.descriptorSetLayouts.size(); i++) {
    //     std::vector<VkDescriptorSetLayoutBinding> bindings;
    //     for(int j=0;j< desc.descriptorSetLayouts[i].resourceBinding.size();j++)
    //     {
    //         HgiRayTracingPipelineResourceBindingDesc bindingDesc = desc.descriptorSetLayouts[i].resourceBinding[j];

    //         VkDescriptorSetLayoutBinding layoutBinding{};
    //         layoutBinding.binding = bindingDesc.bindingIndex;
    //         layoutBinding.descriptorType = HgiMetalConversions::GetDescriptorType(bindingDesc.resourceType);
    //         layoutBinding.descriptorCount = bindingDesc.count;
    //         layoutBinding.stageFlags = HgiMetalConversions::GetShaderStages(bindingDesc.stageUsage);
    //         bindings.push_back(layoutBinding);
    //     }

    //     VkDescriptorSetLayout descriptorSetLayout;
    //     VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCI{};
    //     descriptorSetlayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //     descriptorSetlayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
    //     descriptorSetlayoutCI.pBindings = bindings.data();
    //     TF_VERIFY(vkCreateDescriptorSetLayout(_device->GetMetalDevice(), &descriptorSetlayoutCI, nullptr, &descriptorSetLayout)== VK_SUCCESS);
    //     _vkDescriptorSetLayouts.push_back(descriptorSetLayout);
    // }

    // VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    // pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineLayoutCI.setLayoutCount = _vkDescriptorSetLayouts.size();
    // pipelineLayoutCI.pSetLayouts = _vkDescriptorSetLayouts.data();
    // TF_VERIFY(vkCreatePipelineLayout(_device->GetMetalDevice(), &pipelineLayoutCI, nullptr, &_vkPipelineLayout)==VK_SUCCESS);

    // std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
   
    // for (int i = 0; i < desc.shaders.size(); i++) {
    //     HgiShaderFunctionHandle shader = desc.shaders[i].shader;
    //     VkPipelineShaderStageCreateInfo shaderStage;
    //     shaderStage = {};
    //     shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    //     shaderStage.stage = (VkShaderStageFlagBits) HgiMetalConversions::GetShaderStages(shader->GetDescriptor().shaderStage);;
    //     shaderStage.module = (VkShaderModule)shader->GetRawResource();
    //     shaderStage.pName = desc.shaders[i].entryPoint.c_str();
    //     shaderStages.push_back(shaderStage);
    // }
    // std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

    // for (int i = 0; i < desc.groups.size(); i++) 
    // {
    //     VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
    //     shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    //     shaderGroup.type = HgiMetalConversions::GetRayTracingShaderGroupType(desc.groups[i].type);
    //     shaderGroup.generalShader = desc.groups[i].generalShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].generalShader;
    //     shaderGroup.closestHitShader = desc.groups[i].closestHitShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].closestHitShader;
    //     shaderGroup.anyHitShader = desc.groups[i].anyHitShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].anyHitShader;
    //     shaderGroup.intersectionShader = desc.groups[i].intersectionShader == 0xFFFF ? VK_SHADER_UNUSED_KHR : desc.groups[i].intersectionShader;
    //     shaderGroups.push_back(shaderGroup);
    // }

    // VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
    // rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    // rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    // rayTracingPipelineCI.pStages = shaderStages.data();
    // rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
    // rayTracingPipelineCI.pGroups = shaderGroups.data();
    // rayTracingPipelineCI.maxPipelineRayRecursionDepth = desc.maxRayRecursionDepth;
    // rayTracingPipelineCI.layout = _vkPipelineLayout;
    // TF_VERIFY(_device->vkCreateRayTracingPipelinesKHR(_device->GetMetalDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &_vkPipeline)== VK_SUCCESS);

     
    
    // Debug label
    // if (!desc.debugName.empty()) {
    //     std::string debugLabel = "Pipeline " + desc.debugName;
    //     HgiMetalSetDebugName(
    //         device,
    //         (uint64_t)_vkPipeline,
    //         VK_OBJECT_TYPE_PIPELINE,
    //         debugLabel.c_str());
    // }
}

HgiMetalRayTracingPipeline::~HgiMetalRayTracingPipeline()
{
    assert(0);
    // vkDestroyPipelineLayout(
    //     _device->GetMetalDevice(),
    //     _vkPipelineLayout,
    //     HgiMetalAllocator());

    // vkDestroyPipeline(
    //     _device->GetMetalDevice(),
    //     _vkPipeline,
    //     HgiMetalAllocator());

    // for (VkDescriptorSetLayout layout : _vkDescriptorSetLayouts) {
    //     vkDestroyDescriptorSetLayout(
    //         _device->GetMetalDevice(),
    //         layout,
    //         HgiMetalAllocator());
    // }

    // // This is called from inside garbage collector, so can just call delete on buffer pointer.
    // delete _shaderBindingTable.hitShaderBindingTable.Get();
    // delete _shaderBindingTable.missShaderBindingTable.Get();
    // delete _shaderBindingTable.raygenShaderBindingTable.Get();
}

/// Apply pipeline state
HGIMETAL_API
void HgiMetalRayTracingPipeline::BindPipeline(id<MTLComputeCommandEncoder> computeEncoder)
{
    [computeEncoder setComputePipelineState:_pipelineState];
}

HGIMETAL_API
id<MTLComputePipelineState> HgiMetalRayTracingPipeline::GetMetalPipelineState()
{
    return _pipelineState;
}

uint64_t &
HgiMetalRayTracingPipeline::GetInflightBits()
{
    return _inflightBits;
}

void HgiMetalRayTracingPipeline::BuildShaderBindingTable()
{
    {
        {
            MTLVisibleFunctionTableDescriptor* desc = [MTLVisibleFunctionTableDescriptor new];
            desc.functionCount = _descriptor.groups.size();
            _shaderBindingTable.raygenShaderBindingTable = [_pipelineState newVisibleFunctionTableWithDescriptor:desc];
            _shaderBindingTable.missShaderBindingTable = [_pipelineState newVisibleFunctionTableWithDescriptor:desc];
            _shaderBindingTable.hitShaderBindingTable = [_pipelineState newVisibleFunctionTableWithDescriptor:desc];
        }
        {
            MTLIntersectionFunctionTableDescriptor* desc = [MTLIntersectionFunctionTableDescriptor new];
            desc.functionCount = _descriptor.groups.size();
            _shaderBindingTable.intersectionShaderBindingTable = [_pipelineState newIntersectionFunctionTableWithDescriptor:desc];
        }
    }
    
    for (int i = 0; i < _descriptor.groups.size(); i++)
    {
        if (_descriptor.groups[i].generalShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].generalShader;
            HgiMetalShaderFunction* shaderFunction = (HgiMetalShaderFunction*)_descriptor.shaders[idx].shader.Get();
            
            id<MTLFunction>       function = shaderFunction->GetShaderId();
            id<MTLFunctionHandle> handle = [_pipelineState functionHandleWithFunction:function];
            if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                [_shaderBindingTable.raygenShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                [_shaderBindingTable.missShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageClosestHit || shaderFunction->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                [_shaderBindingTable.hitShaderBindingTable setFunction:handle atIndex:i];
            }
        }
        else if (_descriptor.groups[i].closestHitShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].closestHitShader;
            HgiMetalShaderFunction* shaderFunction = (HgiMetalShaderFunction*)_descriptor.shaders[idx].shader.Get();
            
            id<MTLFunction>       function = shaderFunction->GetShaderId();
            id<MTLFunctionHandle> handle = [_pipelineState functionHandleWithFunction:function];
            if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                [_shaderBindingTable.raygenShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                [_shaderBindingTable.missShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageClosestHit || shaderFunction->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                [_shaderBindingTable.hitShaderBindingTable setFunction:handle atIndex:i];
            }
        }
        else if (_descriptor.groups[i].anyHitShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].anyHitShader;
            HgiMetalShaderFunction* shaderFunction = (HgiMetalShaderFunction*)_descriptor.shaders[idx].shader.Get();
            
            id<MTLFunction>       function = shaderFunction->GetShaderId();
            id<MTLFunctionHandle> handle = [_pipelineState functionHandleWithFunction:function];
            if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                [_shaderBindingTable.raygenShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                [_shaderBindingTable.missShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageClosestHit || shaderFunction->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                [_shaderBindingTable.hitShaderBindingTable setFunction:handle atIndex:i];
            }
        }
        else if (_descriptor.groups[i].intersectionShader != 0xFFFF)
        {
            int idx = _descriptor.groups[i].intersectionShader;
            HgiMetalShaderFunction* shaderFunction = (HgiMetalShaderFunction*)_descriptor.shaders[idx].shader.Get();
            
            id<MTLFunction>       function = shaderFunction->GetShaderId();
            id<MTLFunctionHandle> handle = [_pipelineState functionHandleWithFunction:function];
            if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageRayGen) {
                [_shaderBindingTable.intersectionShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageMiss) {
                [_shaderBindingTable.intersectionShaderBindingTable setFunction:handle atIndex:i];
            }
            else if (shaderFunction->GetDescriptor().shaderStage == HgiShaderStageClosestHit || shaderFunction->GetDescriptor().shaderStage == HgiShaderStageAnyHit) {
                [_shaderBindingTable.intersectionShaderBindingTable setFunction:handle atIndex:i];
            }
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
