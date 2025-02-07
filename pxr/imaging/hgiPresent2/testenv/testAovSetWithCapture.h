//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_TEST_TESTAOVSETWITHCAPTURE_H
#define PXR_IMAGING_HGIPRESENT2_TEST_TESTAOVSETWITHCAPTURE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent2/aovSet.h"

#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

using HgiPresent2TestWantTexelsCallback =
    std::function<bool()>;
using HgiPresent2TestGetTexelsCallback =
    std::function<void(const HgiTextureDesc&, std::vector<std::byte>)>;

class HgiPresent2TestAovSetWithCapture final : public HgiPresent2AovSet
{
public:
    explicit HgiPresent2TestAovSetWithCapture(
        std::unique_ptr<HgiPresent2AovSet> aovSet,
        HgiPresent2TestWantTexelsCallback wantTexels,
        HgiPresent2TestGetTexelsCallback getTexels);

    ~HgiPresent2TestAovSetWithCapture() override;

    bool IsColorFormatSupported(HgiFormat format) const override;

    bool IsDepthFormatSupported(HgiFormat format) const override;

    std::optional<bool> IsValid() const override;

    void UpdateParams(const HgiPresent2Params& params) override;

    RgbaSwizzle GetRgbaSwizzle() const override;

    HgiTextureHandle Acquire(
        HgiCmds* blitCmds, uint32_t width, uint32_t height) override;

    void SubmitAndPresent(std::unique_ptr<HgiCmds> commands) override;

private:
    std::unique_ptr<HgiPresent2AovSet> _aovSet;
    HgiTextureHandle _currentTexture;
    HgiPresent2TestWantTexelsCallback _wantTexels;
    HgiPresent2TestGetTexelsCallback _getTexels;
};

PXR_NAMESPACE_CLOSE_SCOPE


#endif
