//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/renderBufferPool.h"

#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/base/arch/vsnprintf.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

HdStPooledRenderBuffer::HdStPooledRenderBuffer(
    std::shared_ptr<HdStRenderBuffer> buffer,
    HdStRenderBufferPool* pool,
    const SdfPath& graphPath,
    bool isDepth,
    uint16_t idx) :
    _buffer(buffer),
    _pool(pool),
    _graphPath(graphPath),
    _isDepth(isDepth),
    _idx(idx)
{}

HdStPooledRenderBuffer::~HdStPooledRenderBuffer()
{
    _pool->Free(
        _buffer->GetFormat(),
        { (int)_buffer->GetWidth(), (int)_buffer->GetHeight() },
        _buffer->IsMultiSampled(),
        _isDepth,
        _graphPath,
        _idx);
}

HdStPooledRenderBufferUniquePtr
HdStRenderBufferPool::Allocate(
    HdStResourceRegistry* registry,
    const SdfPath& graphPath,
    HdFormat fmt,
    GfVec2i dims,
    bool multiSampled,
    bool depth)
{
    _PooledRenderBufferDesc desc;
    desc.fmt = fmt;
    desc.dims = dims;
    desc.multiSampled = multiSampled;
    desc.depth = depth;
    auto iter = _pooledRenderBuffers.find(desc);
    if (iter == _pooledRenderBuffers.end()) {
        iter = _pooledRenderBuffers.insert(std::make_pair(
            desc, _HdStRenderBufferPoolEntry())).first;
    }
    
    _HdStRenderBufferVec& buffers = iter->second.buffers;
    uint16_t slot = iter->second.allocs[graphPath].Allocate();
    if (slot >= buffers.size()) {
        buffers.resize(slot + 1);
    }
    if (!buffers[slot]) {
        SdfPath path = SdfPath(ArchStringPrintf("aov_Temp_%s_%ix%i_%s%s",
            TfEnum::GetName(fmt).c_str(), dims[0], dims[1],
            multiSampled ? "multiSampled" : "", depth ? "depth" : ""));
        buffers[slot] =
            std::make_shared<HdStRenderBuffer>(registry, path);
        buffers[slot]->Allocate(
            GfVec3i(desc.dims[0], desc.dims[1], 1),
            desc.fmt,
            desc.multiSampled);
    }
    return std::make_unique<HdStPooledRenderBuffer>(
        buffers[slot], this, graphPath, depth, slot);
}

void
HdStRenderBufferPool::Free(
    HdFormat fmt,
    GfVec2i dims,
    bool multiSampled,
    bool depth,
    const SdfPath& graphPath,
    uint16_t idx)
{
    _PooledRenderBufferDesc desc;
    desc.fmt = fmt;
    desc.dims = dims;
    desc.multiSampled = multiSampled;
    desc.depth = depth;
    _pooledRenderBuffers[desc].allocs[graphPath].Free(idx);
}

void
HdStRenderBufferPool::Commit()
{
    for (auto& type : _pooledRenderBuffers) {
        for (auto& buffer : type.second.buffers) {
            if (buffer.use_count() == 1) {
                buffer = nullptr;
            }
        }
    }
}

uint16_t
HdStRenderBufferPool::_AllocationTracker::Allocate()
{
    if (freeList.empty()) {
        return max++;
    } else {
        uint16_t allocated = freeList.back();
        freeList.pop_back();
        return allocated;
    }
}

void
HdStRenderBufferPool::_AllocationTracker::Free(uint16_t index)
{
    freeList.push_back(index);
}

PXR_NAMESPACE_CLOSE_SCOPE
