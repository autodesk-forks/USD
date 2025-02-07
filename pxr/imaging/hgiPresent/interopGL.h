//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_INTEROP_H
#define PXR_IMAGING_HGIPRESENT_INTEROP_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent/presentImpl.h"

#include "pxr/imaging/hgiInterop/hgiInterop.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiPresentInteropGL
///
/// Forward the textures to \class HgiInterop.
///
class HgiPresentInteropGL final : public HgiPresentImpl
{
public:
    explicit HgiPresentInteropGL(Hgi* hgi,
        HgiGLInteropHandle const &handle,
        HgiCompositionParams const &composition);

    ~HgiPresentInteropGL() override = default;

    bool IsFormatSupported(HgiFormat colorFormat) const override;

    bool IsValid() const override;

    void Present(
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth) override;

private:
    HgiInterop _interop;
    uint32_t _fboName;
    HgiCompositionParams _composition;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
