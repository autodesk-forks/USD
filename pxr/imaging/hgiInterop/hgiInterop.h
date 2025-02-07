//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIINTEROP_HGIINTEROP_H
#define PXR_IMAGING_HGIINTEROP_HGIINTEROP_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/gf/rect2i.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiInterop/api.h"
#include "pxr/imaging/hgi/texture.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE
class Hgi;
class VtValue;

struct HgiInteropImpl;

/// \class HgiInterop
///
/// Hydra Graphics Interface Interop.
///
/// HgiInterop provides functionality to transfer render targets between
/// supported APIs as efficiently as possible.
///
class HgiInterop final
{
public:
    HGIINTEROP_API
    HgiInterop();

    HGIINTEROP_API
    ~HgiInterop();

    /// Composite the provided textures over the application / viewer's
    /// framebuffer contents.
    /// `srcHgi`: 
    ///     Determines the source format/platform of the textures.
    ///     Eg. if hgi is of type HgiMetal, the textures are HgiMetalTexture.
    /// `srcColor`: is the source color aov texture to composite to screen.
    /// `srcDepth`: (optional) is the depth aov texture to composite to screen.
    /// `dstFramebuffer`:
    ///     The framebuffer that the source textures are presented into.
    ///     An uint32_t (aka GLuint) FBO name. For backwards compatibility,
    ///     the currently bound framebuffer is used when the value is 0.
    ///
    /// `dstRegion`:
    ///     Subrect region of the framebuffer over which to composite.
    ///     Coordinates are (left, BOTTOM, width, height) which is the same
    ///     convention as OpenGL viewport coordinates.
    ///
    /// Note:
    /// To composite correctly, blending is enabled. 
    /// If `srcDepth` is provided, depth testing is enabled.
    /// As a result, the contents of the application framebuffer matter.
    /// In order to use the contents of `srcColor` and `srcDepth` as-is
    /// (i.e., blit), the color attachment should be cleared to (0,0,0,0) and
    /// the depth attachment needs to be cleared to 1.
    /// 
    HGIINTEROP_API
    void TransferToApp(
        Hgi *srcHgi,
        HgiTextureHandle const &srcColor,
        HgiBlendFactor colorSrcBlendFactor,
        HgiBlendFactor colorDstBlendFactor,
        HgiBlendOp colorBlendOp,
        HgiBlendFactor alphaSrcBlendFactor,
        HgiBlendFactor alphaDstBlendFactor,
        HgiBlendOp alphaBlendOp,
        HgiTextureHandle const &srcDepth,
        HgiCompareFunction depthFunc,
        uint32_t dstFramebuffer,
        GfRect2i const &dstRegion);

private:
    HgiInterop & operator=(const HgiInterop&) = delete;
    HgiInterop(const HgiInterop&) = delete;

    std::unique_ptr<HgiInteropImpl> _hgiInteropImpl;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
