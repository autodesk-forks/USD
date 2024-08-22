//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIINTEROP_HGIINTEROPCPU_H
#define PXR_IMAGING_HGIINTEROP_HGIINTEROPCPU_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiInterop/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class VtValue;

/// \class HgiInteropVulkan
///
/// Provides Some other graphic backend to copy texture to the CPU and then map it to GL.
///
class HgiInteropCpu final
{
public:
    HGIINTEROP_API
    HgiInteropCpu(Hgi* hgi);

    HGIINTEROP_API
    ~HgiInteropCpu();

    /// Composite provided color (and optional depth) textures over app's
    /// framebuffer contents.
    HGIINTEROP_API
    void CompositeToInterop(
        HgiTextureHandle const &color,
        HgiTextureHandle const &depth,
        VtValue const &framebuffer,
        GfVec4i const& viewport);

private:
    HgiInteropCpu() = delete;

    Hgi* _hgi;
    uint32_t _vs;
    uint32_t _fsNoDepth;
    uint32_t _fsDepth;
    uint32_t _prgNoDepth;
    uint32_t _prgDepth;
    uint32_t _vertexBuffer;

    // XXX We tmp copy GPU texture to CPU and then to GL texture
    // Once we share GPU memory between Vulkan and GL we can remove this.
    uint32_t _glColorTex;
    uint32_t _glDepthTex;

    std::vector<uint8_t> _colorTarget;
    std::vector<uint8_t> _depthTarget;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
