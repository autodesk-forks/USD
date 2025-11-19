//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_PICK_BUFFERS_H
#define PXR_IMAGING_HDX_PICK_BUFFERS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2f.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxPickBuffers
///
/// A utility class for transferring pick ID buffers.
class HdxPickBuffers {
public:

    HDX_API
    HdxPickBuffers();

    HDX_API
    ~HdxPickBuffers();

    HDX_API
    HdxPickBuffers(HdxPickBuffers&& other) noexcept;

    HDX_API
    HdxPickBuffers& operator=(HdxPickBuffers&& other) noexcept;

    HdxPickBuffers(const HdxPickBuffers&) = delete;
    HdxPickBuffers& operator=(const HdxPickBuffers&) = delete;

    HDX_API
    void Initialize(
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
        GfVec2i const bufferSize);

    HDX_API
    void FreeBuffers();

    HDX_API
    int const* GetPrimIds() const { return _primIds.get(); }

    HDX_API
    int const* GetInstanceIds() const { return _instanceIds.get(); }

    HDX_API
    int const* GetFaceIds() const { return _elementIds.get(); }

    HDX_API
    int const* GetEdgeIds() const { return _edgeIds.get(); }

    HDX_API
    int const* GetPointIds() const { return _pointIds.get(); }

    HDX_API
    int const* GetNormals() const { return _neyes.get(); }

    HDX_API
    float const* GetDepths() const { return _depths.get(); }

    HDX_API
    HdRenderIndex const* GetRenderIndex() const { return _index; }

    HDX_API
    GfMatrix4d GetViewMatrix() const { return _viewMatrix; }

    HDX_API
    GfMatrix4d GetProjectionMatrix() const { return _projectionMatrix; }

    HDX_API
    GfVec2f const GetDepthRange() const { return _depthRange; }

    HDX_API
    GfVec2i const GetBufferSize() const { return _bufferSize; }

private:
    HdStTextureUtils::AlignedBuffer<int> _primIds;
    HdStTextureUtils::AlignedBuffer<int> _instanceIds;
    HdStTextureUtils::AlignedBuffer<int> _elementIds;
    HdStTextureUtils::AlignedBuffer<int> _edgeIds;
    HdStTextureUtils::AlignedBuffer<int> _pointIds;
    HdStTextureUtils::AlignedBuffer<int> _neyes;
    HdStTextureUtils::AlignedBuffer<float> _depths;
    HdRenderIndex const* _index;
    GfMatrix4d _viewMatrix;
    GfMatrix4d _projectionMatrix;
    GfVec2f _depthRange;
    GfVec2i _bufferSize;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_PICK_BUFFERS_H

