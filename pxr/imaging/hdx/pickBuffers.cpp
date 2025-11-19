//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/pickBuffers.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxPickBuffers::HdxPickBuffers() = default;

HdxPickBuffers::~HdxPickBuffers()
{
    FreeBuffers();
}

HdxPickBuffers::HdxPickBuffers(HdxPickBuffers&& other) noexcept
    : _primIds(std::move(other._primIds))
    , _instanceIds(std::move(other._instanceIds))
    , _elementIds(std::move(other._elementIds))
    , _edgeIds(std::move(other._edgeIds))
    , _pointIds(std::move(other._pointIds))
    , _neyes(std::move(other._neyes))
    , _depths(std::move(other._depths))
    , _index(other._index)
    , _viewMatrix(std::move(other._viewMatrix))
    , _projectionMatrix(std::move(other._projectionMatrix))
    , _depthRange(std::move(other._depthRange))
    , _bufferSize(std::move(other._bufferSize))
{
    other._index = nullptr;
}

HdxPickBuffers& 
HdxPickBuffers::operator=(HdxPickBuffers&& other) noexcept
{
    if (this != &other) {
        FreeBuffers();

        _primIds = std::move(other._primIds);
        _instanceIds = std::move(other._instanceIds);
        _elementIds = std::move(other._elementIds);
        _edgeIds = std::move(other._edgeIds);
        _pointIds = std::move(other._pointIds);
        _neyes = std::move(other._neyes);
        _depths = std::move(other._depths);
        _index = other._index;
        _viewMatrix = std::move(other._viewMatrix);
        _projectionMatrix = std::move(other._projectionMatrix);
        _depthRange = std::move(other._depthRange);
        _bufferSize = std::move(other._bufferSize);
        other._index = nullptr;
    }
    return *this;
}

void
HdxPickBuffers::Initialize(
    HdStTextureUtils::AlignedBuffer<int>& primIds,
    HdStTextureUtils::AlignedBuffer<int>& instanceIds,
    HdStTextureUtils::AlignedBuffer<int>& elementIds,
    HdStTextureUtils::AlignedBuffer<int>& edgeIds,
    HdStTextureUtils::AlignedBuffer<int>& pointIds,
    HdStTextureUtils::AlignedBuffer<int>& neyes,
    HdStTextureUtils::AlignedBuffer<float>& depths,
    HdRenderIndex const* index,
    GfMatrix4d const viewMatrix,
    GfMatrix4d const projectionMatrix,
    GfVec2f const depthRange,
    GfVec2i const bufferSize)
{
    _primIds = std::move(primIds);
    _instanceIds = std::move(instanceIds);
    _elementIds = std::move(elementIds);
    _edgeIds = std::move(edgeIds);
    _pointIds = std::move(pointIds);
    _neyes = std::move(neyes);
    _depths = std::move(depths);
    _index = index;
    _viewMatrix = viewMatrix;
    _projectionMatrix = projectionMatrix;
    _depthRange = depthRange;
    _bufferSize = bufferSize;
}

void
HdxPickBuffers::FreeBuffers()
{
    _primIds = HdStTextureUtils::AlignedBuffer<int>();
    _instanceIds = HdStTextureUtils::AlignedBuffer<int>();
    _elementIds = HdStTextureUtils::AlignedBuffer<int>();
    _edgeIds = HdStTextureUtils::AlignedBuffer<int>();
    _pointIds = HdStTextureUtils::AlignedBuffer<int>();
    _neyes = HdStTextureUtils::AlignedBuffer<int>();
    _depths = HdStTextureUtils::AlignedBuffer<float>();
}

PXR_NAMESPACE_CLOSE_SCOPE

