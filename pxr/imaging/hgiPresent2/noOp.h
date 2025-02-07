//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_NOOP_H
#define PXR_IMAGING_HGIPRESENT2_NOOP_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent2/api.h"
#include "pxr/imaging/hgiPresent2/presentImpl.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiPresent2NoOp
///
/// Does nothing. Provides a fallback when no presentation
/// is configured.
///
class HgiPresent2NoOp final : public HgiPresent2Impl
{
public:
    HGIPRESENT2_API
    explicit HgiPresent2NoOp();

    HGIPRESENT2_API
    ~HgiPresent2NoOp() override = default;

    HGIPRESENT2_API
    bool IsColorFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    bool IsDepthFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    std::optional<bool> IsValid() const override;

    HGIPRESENT2_API
    void UpdateParams(const HgiPresent2Params& params) override;

    HGIPRESENT2_API
    void Present(
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth) override;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
