//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBufferArrayRange.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/extGpuBuffer.h"
#include "pxr/imaging/hdSt/extGpuBufferSource.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

HdStExtGpuBufferArrayRange::HdStExtGpuBufferArrayRange(
    HdStResourceRegistry *resourceRegistry)
    : HdStBufferArrayRange(resourceRegistry)
    , _numElements(0)
    , _version(0)
    , _valid(false)
{
    HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferAliasCount);
}

HdStExtGpuBufferArrayRange::~HdStExtGpuBufferArrayRange()
{
    for (auto *buf : _ownedExternalGpuBuffers) {
        delete buf;
    }
    _ownedExternalGpuBuffers.clear();
    HD_PERF_COUNTER_DECR(HdStPerfTokens->extGpuBufferAliasCount);
}

void
HdStExtGpuBufferArrayRange::SetExternalResource(
    TfToken const &name,
    uint64_t rawHandle,
    size_t rawHandleByteSize,
    HdTupleType tupleType,
    size_t numElements,
    size_t byteOffset)
{
    int const stride = static_cast<int>(HdDataSizeOfTupleType(tupleType));

    auto *aliasBuffer = new HdStExtGpuBuffer(rawHandle, rawHandleByteSize);
    _ownedExternalGpuBuffers.push_back(aliasBuffer);

    HgiBufferHandle aliasHandle(
        aliasBuffer, HdSt_GetNextExtGpuBufferHandleId());

    auto resource = std::make_shared<HdStBufferResource>(
        HdTokens->primvar,
        tupleType,
        static_cast<int>(byteOffset),
        stride);

    size_t const bufferSize = numElements * stride;
    resource->SetAllocation(aliasHandle, bufferSize);

    _resources.push_back(std::make_pair(name, resource));
    _numElements = numElements;
    _valid = true;
}

void
HdStExtGpuBufferArrayRange::ReleaseExternalResources()
{
    _resources.clear();
    for (auto *buf : _ownedExternalGpuBuffers) {
        delete buf;
    }
    _ownedExternalGpuBuffers.clear();
    _numElements = 0;
    _valid = false;
    IncrementVersion();
}

bool
HdStExtGpuBufferArrayRange::UpdateExternalResources(
    HdBufferSourceSharedPtrVector const &sources)
{
    if (sources.size() != _resources.size()) {
        return false;
    }

    // Verify all sources are direct-bindable external GPU sources, names
    // match, and tuple types / offsets haven't changed (those are const on
    // the HdStBufferResource and can't be updated in-place).
    for (size_t i = 0; i < sources.size(); ++i) {
        if (!dynamic_cast<HdStExtGpuBufferSource const *>(sources[i].get())) {
            return false;
        }
        if (sources[i]->GetName() != _resources[i].first) {
            return false;
        }
        auto const *extSrc =
            static_cast<HdStExtGpuBufferSource const *>(
                sources[i].get());
        auto const &hdDesc = extSrc->GetDescriptor();
        auto const &resource = _resources[i].second;
        if (hdDesc.tupleType != resource->GetTupleType() ||
            static_cast<int>(hdDesc.byteOffset) != resource->GetOffset()) {
            return false;
        }
    }

    for (size_t i = 0; i < sources.size(); ++i) {
        auto const *extSrc =
            static_cast<HdStExtGpuBufferSource const *>(
                sources[i].get());
        auto const &hdDesc = extSrc->GetDescriptor();

        int const stride =
            static_cast<int>(HdDataSizeOfTupleType(hdDesc.tupleType));
        size_t byteSize = hdDesc.rawHandleByteSize > 0
            ? hdDesc.rawHandleByteSize
            : hdDesc.numElements * stride;

        _ownedExternalGpuBuffers[i]->UpdateRawHandle(hdDesc.rawHandle, byteSize);

        auto &resource = _resources[i].second;
        size_t const bufferSize = hdDesc.numElements * stride;
        resource->SetAllocation(resource->GetHandle(), bufferSize);

        _numElements = hdDesc.numElements;
    }

    IncrementVersion();
    return true;
}

bool
HdStExtGpuBufferArrayRange::MergeExternalResources(
    HdBufferSourceSharedPtrVector const &sources)
{
    for (auto const &src : sources) {
        if (!dynamic_cast<HdStExtGpuBufferSource const *>(src.get())) {
            return false;
        }
    }

    for (auto const &src : sources) {
        auto const *extSrc =
            static_cast<HdStExtGpuBufferSource const *>(src.get());
        auto const &hdDesc = extSrc->GetDescriptor();

        int const stride =
            static_cast<int>(HdDataSizeOfTupleType(hdDesc.tupleType));
        size_t byteSize = hdDesc.rawHandleByteSize > 0
            ? hdDesc.rawHandleByteSize
            : hdDesc.numElements * stride;

        // Find existing resource by name.
        bool found = false;
        for (size_t i = 0; i < _resources.size(); ++i) {
            if (_resources[i].first != src->GetName()) {
                continue;
            }
            // Update the existing alias buffer and resource in-place.
            _ownedExternalGpuBuffers[i]->UpdateRawHandle(
                hdDesc.rawHandle, byteSize);

            auto &resource = _resources[i].second;
            size_t const bufferSize = hdDesc.numElements * stride;
            resource->SetAllocation(resource->GetHandle(), bufferSize);

            found = true;
            break;
        }

        if (!found) {
            // Append as a new resource (same logic as SetExternalResource).
            auto *aliasBuffer =
                new HdStExtGpuBuffer(hdDesc.rawHandle, byteSize);
            _ownedExternalGpuBuffers.push_back(aliasBuffer);

            HgiBufferHandle aliasHandle(
                aliasBuffer, HdSt_GetNextExtGpuBufferHandleId());

            auto resource = std::make_shared<HdStBufferResource>(
                HdTokens->primvar,
                hdDesc.tupleType,
                static_cast<int>(hdDesc.byteOffset),
                stride);

            size_t const bufferSize = hdDesc.numElements * stride;
            resource->SetAllocation(aliasHandle, bufferSize);

            _resources.push_back(
                std::make_pair(src->GetName(), resource));
        }

        _numElements = hdDesc.numElements;
    }

    _valid = true;
    IncrementVersion();
    return true;
}

bool
HdStExtGpuBufferArrayRange::IsValid() const
{
    return _valid;
}

bool
HdStExtGpuBufferArrayRange::IsAssigned() const
{
    return _valid;
}

bool
HdStExtGpuBufferArrayRange::IsImmutable() const
{
    return false;
}

bool
HdStExtGpuBufferArrayRange::RequiresStaging() const
{
    return false;
}

bool
HdStExtGpuBufferArrayRange::Resize(int numElements)
{
    // Alias ranges own no real storage: element-count and allocation-size
    // changes are driven by the producer through Update/MergeExternalResources
    // (which call HdStBufferResource::SetAllocation). We only track the count
    // for GetNumElements() and return false -- the conventional "cannot resize
    // in place" signal -- so the aggregation strategy never treats this range
    // as reusable capacity.
    _numElements = numElements;
    return false;
}

void
HdStExtGpuBufferArrayRange::CopyData(
    HdBufferSourceSharedPtr const &bufferSource)
{
    // No-op for alias ranges — the external buffer IS the data.
}

VtValue
HdStExtGpuBufferArrayRange::ReadData(TfToken const &name) const
{
    TF_CODING_ERROR("ReadData not supported on external GPU buffer array range");
    return VtValue();
}

int
HdStExtGpuBufferArrayRange::GetElementOffset() const
{
    return 0;
}

int
HdStExtGpuBufferArrayRange::GetByteOffset(
    TfToken const &resourceName) const
{
    for (auto const &pair : _resources) {
        if (pair.first == resourceName) {
            return pair.second->GetOffset();
        }
    }
    return 0;
}

size_t
HdStExtGpuBufferArrayRange::GetNumElements() const
{
    return _numElements;
}

size_t
HdStExtGpuBufferArrayRange::GetVersion() const
{
    return _version.load();
}

void
HdStExtGpuBufferArrayRange::IncrementVersion()
{
    _version.fetch_add(1);
}

size_t
HdStExtGpuBufferArrayRange::GetMaxNumElements() const
{
    return _numElements;
}

HdBufferArrayUsageHint
HdStExtGpuBufferArrayRange::GetUsageHint() const
{
    return HdBufferArrayUsageHintBitsVertex |
           HdBufferArrayUsageHintBitsStorage;
}

void
HdStExtGpuBufferArrayRange::SetBufferArray(HdBufferArray *)
{
    // Alias ranges are not associated with a real buffer array.
}

void
HdStExtGpuBufferArrayRange::DebugDump(std::ostream &out) const
{
    out << "HdStExtGpuBufferArrayRange:"
        << " valid=" << _valid
        << " numElements=" << _numElements
        << " resources=" << _resources.size()
        << "\n";
}

HdStBufferResourceSharedPtr
HdStExtGpuBufferArrayRange::GetResource() const
{
    if (_resources.size() == 1) {
        return _resources.front().second;
    }
    if (!_resources.empty()) {
        TF_CODING_ERROR("GetResource() called on alias BAR with "
                        "multiple resources; use GetResource(name).");
    }
    return nullptr;
}

HdStBufferResourceSharedPtr
HdStExtGpuBufferArrayRange::GetResource(TfToken const &name)
{
    for (auto const &pair : _resources) {
        if (pair.first == name) {
            return pair.second;
        }
    }
    return nullptr;
}

HdStBufferResourceNamedList const &
HdStExtGpuBufferArrayRange::GetResources() const
{
    return _resources;
}

const void *
HdStExtGpuBufferArrayRange::_GetAggregation() const
{
    // Aggregate two alias BARs when they wrap the same underlying GPU
    // buffer.  The first resource's raw handle is used as the key:
    // in the typical case, every alias BAR built from the producer
    // has the same ordered resource layout (positions, normals,
    // uv, ...) so first-handle equality implies full
    // bind-equivalence.  Empty BARs fall back to `this` so they remain
    // their own aggregation island.
    if (_resources.empty()) {
        return this;
    }
    auto const &res = _resources.front().second;
    auto *aliasBuffer =
        static_cast<HdStExtGpuBuffer *>(res->GetHandle().Get());
    if (!aliasBuffer) {
        return this;
    }
    uint64_t const rawHandle = aliasBuffer->GetRawResource();
    TF_DEBUG(HDST_DRAW).Msg(
        "[ExternalGpuBAR] aggregation key: rawHandle=0x%llx, "
        "resources=%zu, first=%s\n",
        static_cast<unsigned long long>(rawHandle),
        _resources.size(),
        _resources.front().first.GetText());
    // Fold the 64-bit handle into a pointer-sized key so we don't silently
    // truncate the high word on 32-bit builds (where uintptr_t is 32-bit).
    // On 64-bit the fold is a no-op and the key is the handle itself.
    uintptr_t key = static_cast<uintptr_t>(rawHandle);
    if (sizeof(uintptr_t) < sizeof(uint64_t)) {
        key ^= static_cast<uintptr_t>(rawHandle >> 32);
    }
    return reinterpret_cast<const void *>(key);
}

PXR_NAMESPACE_CLOSE_SCOPE
