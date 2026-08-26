//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"

#include <algorithm>
#include <unordered_map>
#include "pxr/imaging/hgiVulkan/sampler.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/shaderProgram.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

std::vector<HgiVulkanBufferVector*>
    HgiVulkanGarbageCollector::_bufferList;
std::vector<HgiVulkanTextureVector*>
    HgiVulkanGarbageCollector::_textureList;
std::vector<HgiVulkanSamplerVector*>
    HgiVulkanGarbageCollector::_samplerList;
std::vector<HgiVulkanShaderFunctionVector*>
    HgiVulkanGarbageCollector::_shaderFunctionList;
std::vector<HgiVulkanShaderProgramVector*>
    HgiVulkanGarbageCollector::_shaderProgramList;
std::vector<HgiVulkanResourceBindingsVector*>
    HgiVulkanGarbageCollector::_resourceBindingsList;
std::vector<HgiVulkanGraphicsPipelineVector*>
    HgiVulkanGarbageCollector::_graphicsPipelineList;
std::vector<HgiVulkanComputePipelineVector*>
    HgiVulkanGarbageCollector::_computePipelineList;
std::vector<HgiVulkanRayTracingPipelineVector*>
    HgiVulkanGarbageCollector::_rayTracingPipelineList;
std::vector<HgiVulkanAccelerationStructureVector*>
    HgiVulkanGarbageCollector::_accelerationStructureList;
std::vector<HgiVulkanAccelerationStructureGeometryVector*>
    HgiVulkanGarbageCollector::_accelerationStructureGeometryList;


template<class T>
static void _EmptyTrash(
    std::vector<std::vector<T*>*>* list,
    HgiVulkanDevice* device,
    uint64_t queueInflightBits)
{
    // Loop the garbage vectors of each thread. Indexed rather than range-for: deleting an
    // object below can register a new per-thread vector in *list, which would invalidate a
    // range-for iterator mid-loop.
    for (size_t listIdx = 0; listIdx < list->size(); ++listIdx) {
        std::vector<T*>* vec = (*list)[listIdx];
        for (size_t i=vec->size(); i-- > 0;) {
            T* object = (*vec)[i];

            // Each device has its own queue, so its own set of inflight bits.
            // We must only destroy objects that belong to this device & queue.
            // (The garbage collector collects objects from all devices.)
            //
            // Compare the device *pointers* rather than the VkDevice handles. The thread local
            // trash vectors are shared by every Hgi in the process, so this list can hold
            // objects belonging to a device that has already been destroyed; reaching through
            // such an object to call GetVulkanDevice() on it is a use-after-free, and it
            // crashed whenever a second Hgi was created (as the unit tests do per test case).
            if (device != object->GetDevice()) {
                continue;
            }

            // See comments in PerformGarbageCollection.
            if ((queueInflightBits & object->GetInflightBits()) == 0) {
                delete object;
                std::iter_swap(vec->begin() + i, vec->end() - 1);
                vec->pop_back();
            }
        }
    }
}

HgiVulkanGarbageCollector::HgiVulkanGarbageCollector(HgiVulkan* hgi)
    : _hgi(hgi)
    , _isDestroying(false)
{
}

HgiVulkanGarbageCollector::~HgiVulkanGarbageCollector()
{
    // Free the per-thread vectors this collector owns. Their contents were emptied by the final
    // PerformGarbageCollection; what is released here is just the vectors themselves, which were
    // previously leaked. Other threads may still hold a cached pointer to one of these, which is
    // why _GetThreadLocalStorageList re-checks that a cached list is still registered with the
    // collector it is asking about before returning it -- that check only compares pointers.
    auto deleteLists = [](auto& lists) {
        for (auto* list : lists) {
            delete list;
        }
        lists.clear();
    };
    deleteLists(_bufferList);
    deleteLists(_textureList);
    deleteLists(_samplerList);
    deleteLists(_shaderFunctionList);
    deleteLists(_shaderProgramList);
    deleteLists(_resourceBindingsList);
    deleteLists(_graphicsPipelineList);
    deleteLists(_computePipelineList);
    deleteLists(_accelerationStructureList);
    deleteLists(_accelerationStructureGeometryList);
    deleteLists(_rayTracingPipelineList);
}

/* Multi threaded */
HgiVulkanBufferVector*
HgiVulkanGarbageCollector::GetBufferList()
{
    return _GetThreadLocalStorageList(&_bufferList);
}

/* Multi threaded */
HgiVulkanTextureVector*
HgiVulkanGarbageCollector::GetTextureList()
{
    return _GetThreadLocalStorageList(&_textureList);
}

/* Multi threaded */
HgiVulkanSamplerVector*
HgiVulkanGarbageCollector::GetSamplerList()
{
    return _GetThreadLocalStorageList(&_samplerList);
}

/* Multi threaded */
HgiVulkanShaderFunctionVector*
HgiVulkanGarbageCollector::GetShaderFunctionList()
{
    return _GetThreadLocalStorageList(&_shaderFunctionList);
}

/* Multi threaded */
HgiVulkanShaderProgramVector*
HgiVulkanGarbageCollector::GetShaderProgramList()
{
    return _GetThreadLocalStorageList(&_shaderProgramList);
}

/* Multi threaded */
HgiVulkanResourceBindingsVector*
HgiVulkanGarbageCollector::GetResourceBindingsList()
{
    return _GetThreadLocalStorageList(&_resourceBindingsList);
}

/* Multi threaded */
HgiVulkanGraphicsPipelineVector*
HgiVulkanGarbageCollector::GetGraphicsPipelineList()
{
    return _GetThreadLocalStorageList(&_graphicsPipelineList);
}

/* Multi threaded */
HgiVulkanComputePipelineVector*
HgiVulkanGarbageCollector::GetComputePipelineList()
{
    return _GetThreadLocalStorageList(&_computePipelineList);
}

/* Multi threaded */
HgiVulkanRayTracingPipelineVector*
HgiVulkanGarbageCollector::GetRayTracingPipelineList()
{
    return _GetThreadLocalStorageList(&_rayTracingPipelineList);
}

/* Multi threaded */
HgiVulkanAccelerationStructureVector*
HgiVulkanGarbageCollector::GetAccelerationStructureList()
{
    return _GetThreadLocalStorageList(&_accelerationStructureList);
}

/* Multi threaded */
HgiVulkanAccelerationStructureGeometryVector*
HgiVulkanGarbageCollector::GetAccelerationStructureGeometryList()
{
    return _GetThreadLocalStorageList(&_accelerationStructureGeometryList);
}


/* Single threaded */
void
HgiVulkanGarbageCollector::PerformGarbageCollection(HgiVulkanDevice* device)
{
    // Garbage Collection notes:
    //
    // When the client requests objects to be destroyed (Eg. Hgi::DestroyBuffer)
    // we put objects into this garbage collector. At that time we also store
    // the bits of the command buffers that are 'in-flight'.
    // We have to delay destroying the vulkan resources until there are no
    // command buffers using the resource.
    // Instead of tracking complex dependencies between objects and cmd buffers
    // we simply assume that all in-flight command buffers might be using the
    // destroyed object and wait until those command buffers have been
    // consumed by the GPU.
    //
    // In _EmptyTrash we try to delete objects in the garbage collector.
    // We compare the bits of the queue and the object to decide if we can
    // delete the object. Example:
    //
    //    Each command buffer takes up one bit (where 1 means "in-flight").
    //    Queue currently in-flight cmd buf bits:   01001011101
    //    In-flight bits when obj was trashed:      00100000100
    //    Bitwise & result:                         00000000100
    //
    // Conclusion: object cannot yet be destroyed. One command buffer that was
    // in-flight during the destruction request is still in-flight and might
    // still be using the object on the GPU.

    _isDestroying = true;

    // Check what command buffers are in-flight on the device queue.
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();
    uint64_t queueBits = queue->GetInflightCommandBuffersBits();
    _EmptyTrash(&_bufferList, device, queueBits);
    _EmptyTrash(&_textureList, device, queueBits);
    _EmptyTrash(&_samplerList, device, queueBits);
    _EmptyTrash(&_shaderFunctionList, device, queueBits);
    _EmptyTrash(&_shaderProgramList, device, queueBits);
    _EmptyTrash(&_resourceBindingsList, device, queueBits);
    _EmptyTrash(&_graphicsPipelineList, device, queueBits);
    _EmptyTrash(&_computePipelineList, device, queueBits);
    _EmptyTrash(&_accelerationStructureList, device, queueBits);
    _EmptyTrash(&_accelerationStructureGeometryList, device, queueBits);
    _EmptyTrash(&_rayTracingPipelineList, device, queueBits);

    _isDestroying = false;
}

template<class T>
T* HgiVulkanGarbageCollector::_GetThreadLocalStorageList(std::vector<T*>* collector)
{
    if (ARCH_UNLIKELY(_isDestroying)) {
        // This used to spin on _isDestroying, which deadlocks outright whenever the thread
        // asking is the thread collecting -- exactly what happens when an object destroyed from
        // _EmptyTrash queues another object. Appending here is safe: the outer loop is indexed
        // and the inner loop walks backwards, so anything added now is simply collected on the
        // next pass.
        TF_CODING_ERROR("Object queued for destruction during garbage collection; it will be "
                        "collected on the next pass.");
    }

    // One vector per (collector, thread). This used to be a single function-local
    // `thread_local T*`, which is shared by *every* collector on the thread: the first Hgi to ask
    // created and registered the vector, and every later Hgi silently reused it without
    // registering it. Those later objects were therefore queued onto a list no live collector
    // ever empties -- they are never destroyed, and VMA asserts that allocations outlived the
    // allocator as soon as the second Hgi tears its device down. A unit test binary creates one
    // Hgi per test, so this fires almost immediately; usdview creates one and never sees it.
    thread_local std::unordered_map<void*, T*> _tls;
    static std::mutex garbageMutex;

    auto it = _tls.find(collector);
    if (it != _tls.end()) {
        // A collector allocated at a recycled address would otherwise be handed the previous
        // owner's freed vector, so confirm the cached list is still registered here. This only
        // ever compares pointers; it never dereferences a stale one.
        std::lock_guard<std::mutex> guard(garbageMutex);
        if (std::find(collector->begin(), collector->end(), it->second) != collector->end()) {
            return it->second;
        }
        _tls.erase(it);
    }

    T* list = new T();
    {
        std::lock_guard<std::mutex> guard(garbageMutex);
        collector->push_back(list);
    }
    _tls[collector] = list;
    return list;
}


PXR_NAMESPACE_CLOSE_SCOPE
