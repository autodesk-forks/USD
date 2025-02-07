//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_GLINTEROP_H
#define PXR_IMAGING_HGIPRESENT2_GLINTEROP_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent2/api.h"
#include "pxr/imaging/hgiPresent2/presentImpl.h"

#include "pxr/imaging/hgiInterop/hgiInterop.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiInteropPresentInteropGL
///
/// An HgiPresent2Impl which uses the existing HgiInterop library to present to
/// an external OpenGL framebuffer.
///
class HgiPresent2GLInterop final : public HgiPresent2Impl
{
public:
    /// Create an HgiPresent2Impl which composes the AOV into the given OpenGL
    /// framebuffer. If fboName is 0, then this uses the currently bound
    /// framebuffer.
    HGIPRESENT2_API
    explicit HgiPresent2GLInterop(Hgi* hgi, uint32_t fboName);

    HGIPRESENT2_API
    ~HgiPresent2GLInterop() override = default;

    HGIPRESENT2_API
    bool IsColorFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    bool IsDepthFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    std::optional<bool> IsValid() const override;

    HGIPRESENT2_API
    void UpdateParams(const HgiPresent2Params& params) override;

    HGIPRESENT2_API
    void Present(HgiTextureHandle const& srcColor,
        HgiTextureHandle const& srcDepth) override;

private:
    HgiInterop _interop;
    uint32_t _fboName;
    HgiInteropCompositionParams _params;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
