//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent2/testenv/testAovSetWithCapture.h"

#include "pxr/base/tf/smallVector.h"

#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/hgi.h"


PXR_NAMESPACE_OPEN_SCOPE


HgiPresent2TestAovSetWithCapture::HgiPresent2TestAovSetWithCapture(
    std::unique_ptr<HgiPresent2AovSet> aovSet,
        HgiPresent2TestWantTexelsCallback wantTexels,
        HgiPresent2TestGetTexelsCallback getTexels)
    : _aovSet{std::move(aovSet)}
    , _wantTexels{std::move(wantTexels)}
    , _getTexels{std::move(getTexels)}
{
}

HgiPresent2TestAovSetWithCapture::~HgiPresent2TestAovSetWithCapture() = default;

bool
HgiPresent2TestAovSetWithCapture::IsColorFormatSupported(HgiFormat format) const
{
    return _aovSet->IsColorFormatSupported(format);
}

bool
HgiPresent2TestAovSetWithCapture::IsDepthFormatSupported(HgiFormat format) const
{
    return _aovSet->IsDepthFormatSupported(format);
}

std::optional<bool>
HgiPresent2TestAovSetWithCapture::IsValid() const
{
    return _aovSet->IsValid();
}

void
HgiPresent2TestAovSetWithCapture::UpdateParams(const HgiPresent2Params& params)
{
    return _aovSet->UpdateParams(params);
}

HgiPresent2AovSet::RgbaSwizzle
HgiPresent2TestAovSetWithCapture::GetRgbaSwizzle() const
{
    return _aovSet->GetRgbaSwizzle();
}

HgiTextureHandle
HgiPresent2TestAovSetWithCapture::Acquire(
    HgiCmds* commands, uint32_t width, uint32_t height)
{
    _currentTexture = _aovSet->Acquire(commands, width, height);
    if (!_currentTexture) {
        TF_RUNTIME_ERROR("AOV set texture acquire failed");
    }
    return _currentTexture;
}

static void
_SwizzleToRgba(HgiPresent2AovSet::RgbaSwizzle swizzle, HgiFormat format,
    std::vector<std::byte>& texelsBytes)
{
    if (swizzle == HgiPresent2AovSet::identityRgbaSwizzle) {
        return;
    }

    // We make use of std::memcpy here to prevent strict aliasing violations.
    // If we had C++20 we could use std::bit_cast. But modern compilers are very
    // good at heap elision, so these memcpy calls should be optimized away.

    const size_t componentCount = HgiGetComponentCount(format);
    const HgiFormat componentFormat = HgiGetComponentBaseFormat(format);
    TF_VERIFY(componentCount == 1 || componentFormat != format);

    const size_t texelSize = HgiGetDataSizeOfFormat(format);
    const size_t componentSize = HgiGetDataSizeOfFormat(componentFormat);

    TfSmallVector<std::byte, 16> buffer;
    buffer.resize(texelSize);

    for (size_t i = 0; i < texelsBytes.size(); i += texelSize) {
        std::byte* const texelBytes = texelsBytes.data() + i;
        for (size_t c = 0; c < componentCount; c++) {
            std::memcpy(buffer.data() + c,
                texelBytes + swizzle[c] * componentSize, componentSize);
        }
        std::memcpy(texelBytes, buffer.data(), texelSize);
    }
}

void
HgiPresent2TestAovSetWithCapture::SubmitAndPresent(
    std::unique_ptr<HgiCmds> commands)
{
    if (!dynamic_cast<HgiBlitCmds*>(commands.get())) {
        TF_FATAL_ERROR("commands must be HgiBlitCmds");
    }

    std::unique_ptr<HgiBlitCmds> blitCmds{
        dynamic_cast<HgiBlitCmds*>(commands.release())};

    if (!_wantTexels()) {
        _aovSet->SubmitAndPresent(std::move(blitCmds));
        return;
    }

    const size_t byteSize = _currentTexture->GetByteSizeOfResource();
    std::vector<std::byte> texelsBytes;
    texelsBytes.resize(byteSize);

    HgiTextureGpuToCpuOp copyOp;
    copyOp.gpuSourceTexture = _currentTexture;
    copyOp.cpuDestinationBuffer = texelsBytes.data();
    copyOp.destinationBufferByteSize = byteSize;
    blitCmds->CopyTextureGpuToCpu(copyOp);

    _aovSet->SubmitAndPresent(std::move(blitCmds));

    const auto descriptor = _currentTexture->GetDescriptor();
    const auto swizzle = _aovSet->GetRgbaSwizzle();
    _SwizzleToRgba(swizzle, descriptor.format, texelsBytes);

    _getTexels(descriptor, std::move(texelsBytes));
}


PXR_NAMESPACE_CLOSE_SCOPE
