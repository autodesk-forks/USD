//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent2/glInterop.h"
#include "pxr/imaging/hgiPresent2/present.h"

#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiPresent2GLInterop::HgiPresent2GLInterop(Hgi* hgi, uint32_t fboName)
    : HgiPresent2Impl(hgi)
{
    _fboName = fboName;
}

bool
HgiPresent2GLInterop::IsColorFormatSupported(HgiFormat format) const
{
    // Vulkan and WebGPU interop limitations.
    switch (format) {
    case HgiFormatUNorm8Vec4:
    case HgiFormatFloat16Vec4:
    case HgiFormatFloat32Vec4:
        return true;
    default:
        return false;
    }
}

bool
HgiPresent2GLInterop::IsDepthFormatSupported(HgiFormat format) const
{
    // Vulkan and WebGPU interop limitations.
    return format == HgiFormatFloat32;
}

std::optional<bool>
HgiPresent2GLInterop::IsValid() const
{
    // HgiInterop doesn't have a way to check.
    return std::nullopt;
}

void
HgiPresent2GLInterop::UpdateParams(const HgiPresent2Params& params)
{
    _params = params.composition;
}

void
HgiPresent2GLInterop::Present(
    HgiTextureHandle const& srcColor, HgiTextureHandle const& srcDepth)
{
    _interop.TransferToApp(_hgi, srcColor, srcDepth, _params, _fboName);
}

PXR_NAMESPACE_CLOSE_SCOPE
