//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent2/aovBlit.h"

#include "pxr/imaging/hgi/hgi.h"


PXR_NAMESPACE_OPEN_SCOPE


HgiPresent2AovBlit::HgiPresent2AovBlit(
    Hgi* hgi, std::unique_ptr<HgiPresent2AovSet> aovSet)
    : HgiPresent2Impl(hgi)
    , _aovSet(std::move(aovSet))
{
}

bool
HgiPresent2AovBlit::IsColorFormatSupported(HgiFormat format) const
{
    return _aovSet->IsColorFormatSupported(format);
}

bool
HgiPresent2AovBlit::IsDepthFormatSupported(HgiFormat format) const
{
    return _aovSet->IsDepthFormatSupported(format);
}

std::optional<bool>
HgiPresent2AovBlit::IsValid() const
{
    return _aovSet->IsValid();
}

void
HgiPresent2AovBlit::UpdateParams(const HgiPresent2Params& params)
{
    _aovSet->UpdateParams(params);
}

void
HgiPresent2AovBlit::Present(
    HgiTextureHandle const& srcColor, HgiTextureHandle const&)
{
    HgiBlitCmdsUniquePtr blitCmds = _hgi->CreateBlitCmds();

    const auto srcSize = srcColor->GetDescriptor().dimensions;
    const HgiTextureHandle dstColor =
        _aovSet->Acquire(blitCmds.get(), srcSize[0], srcSize[1]);
    if (!dstColor) {
        return;
    }

    // Y source range is swapped to vertically flip the image.
    blitCmds->BlitTexture(srcColor, {{0, srcSize[1]}, {srcSize[0] - 1, -1}},
        dstColor, {}, HgiSamplerFilterLinear);

    _aovSet->SubmitAndPresent(std::move(blitCmds));
}


PXR_NAMESPACE_CLOSE_SCOPE
