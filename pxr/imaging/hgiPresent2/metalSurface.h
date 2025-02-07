//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_METALSURFACE_H
#define PXR_IMAGING_HGIPRESENT2_METALSURFACE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent2/aovSet.h"
#include "pxr/imaging/hgiPresent2/api.h"
#include "pxr/imaging/hgiPresent2/present.h"

#if __OBJC__
@protocol CAMetalDrawable;
#endif


PXR_NAMESPACE_OPEN_SCOPE


class HgiMetal;

/// \class HgiPresent2MetalSurface
///
/// Present to a CAMetalLayer using HgiMetal.
///
class HgiPresent2MetalSurface final : public HgiPresent2AovSet
{
private:
    /// The C++ and Objective-C++ files see two different types for
    /// CAMetalLayerPtr, due to it being an Objective-C class pointer.
    /// By hiding the type in a struct, we can ensure that declarations and
    /// definitions have the same signature regardless of the source file.
    /// Since we allow implicit conversions we can keep this implementation
    /// detail private.
    struct _HideCAMetalLayerPtr
    {
        _HideCAMetalLayerPtr(CAMetalLayerPtr layer)
            : layer{layer}
        {
        }

        CAMetalLayerPtr layer;
    };

public:
    /// Create an HgiPresent2AovSet which sources AOVs from a CAMetalLayer
    /// drawable pool.
    HGIPRESENT2_API
    explicit HgiPresent2MetalSurface(HgiMetal* hgi, _HideCAMetalLayerPtr layer);

    HGIPRESENT2_API
    ~HgiPresent2MetalSurface() override;

    HGIPRESENT2_API
    bool IsColorFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    bool IsDepthFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    std::optional<bool> IsValid() const override;

    HGIPRESENT2_API
    void UpdateParams(const HgiPresent2Params& params) override;

    HGIPRESENT2_API
    RgbaSwizzle GetRgbaSwizzle() const override;

    HGIPRESENT2_API
    HgiTextureHandle Acquire(
        HgiCmds* blitCmds, uint32_t width, uint32_t height) override;

    HGIPRESENT2_API
    void SubmitAndPresent(std::unique_ptr<HgiCmds> commands) override;

private:
    void _ApplyParamsToLayer();

#if __OBJC__
    using CAMetalDrawablePtr = id<CAMetalDrawable>;
    static_assert(sizeof(CAMetalDrawablePtr) == sizeof(void*));
#else
    using CAMetalDrawablePtr = struct _pxr__CAMetalDrawable*;
#endif

    HgiMetal* _hgiMetal{};
    HgiPresent2SurfaceParams _params{};
    CAMetalLayerPtr _metalLayer{};
    CAMetalDrawablePtr _currentDrawable{};
    bool _layerReady{false};
};

/// The HgiMetal type definition requires Objective-C++. This function can be
/// used to perform dynamic_cast<HgiMetal*> from regular C++.
HGIPRESENT2_API
HgiMetal*
HgiPresent2DynamicCastHgiMetal(Hgi* hgi);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
