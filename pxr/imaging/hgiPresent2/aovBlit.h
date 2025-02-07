//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_AOVBLIT_H
#define PXR_IMAGING_HGIPRESENT2_AOVBLIT_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent2/api.h"
#include "pxr/imaging/hgiPresent2/presentImpl.h"
#include "pxr/imaging/hgiPresent2/aovSet.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiPresent2AovBlit
///
/// This implementation acquires an AOV from its HgiPresent2AovSet instance,
/// records all the necessary blit commands to a new HgiCmds object, then passes
/// this object to HgiPresent2AovSet. It will record any additional
/// implementation-specific commands, submit them, and present.
///
class HgiPresent2AovBlit final : public HgiPresent2Impl
{
public:
    /// Create an HgiPresent2Impl that will blit to the AOVs provided by
    /// HgiPresent2AovSet.
    HGIPRESENT2_API
    explicit HgiPresent2AovBlit(Hgi* hgi,
        std::unique_ptr<HgiPresent2AovSet> aovSet);

    HGIPRESENT2_API
    ~HgiPresent2AovBlit() override = default;

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

private:
    std::unique_ptr<HgiPresent2AovSet> _aovSet;
};

PXR_NAMESPACE_CLOSE_SCOPE


#endif
