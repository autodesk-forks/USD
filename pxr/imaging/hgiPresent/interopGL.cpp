//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/interopGL.h"

#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiPresentInteropGL::HgiPresentInteropGL(Hgi* hgi,
    HgiGLInteropHandle const &handle,
    HgiCompositionParams const &composition)
    : HgiPresentImpl(hgi)
    , _composition(composition)
{
    _fboName = handle.fboName;
}

bool
HgiPresentInteropGL::IsFormatSupported(HgiFormat colorFormat) const
{
    // Integer formats are not supported (this requires the GL interop to
    // support additional sampler types), nor are compressed formats.
    return HgiIsFloatFormat(colorFormat) && !HgiIsCompressed(colorFormat);
}

bool
HgiPresentInteropGL::IsValid() const
{
    // HgiInterop doesn't have a way to check, so assume always valid.
    return true;
}

void
HgiPresentInteropGL::Present(
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth)
{
    _interop.TransferToApp(_hgi,
        srcColor,
        _composition.colorSrcBlendFactor,
        _composition.colorDstBlendFactor,
        _composition.colorBlendOp,
        _composition.alphaSrcBlendFactor,
        _composition.alphaDstBlendFactor,
        _composition.alphaBlendOp,
        srcDepth,
        _composition.depthFunc,
        _fboName,
        _composition.dstRegion
    );
}


PXR_NAMESPACE_CLOSE_SCOPE
