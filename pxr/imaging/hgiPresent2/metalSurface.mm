//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent2/metalSurface.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/base/gf/range1d.h"

#include "pxr/imaging/hgiMetal/blitCmds.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/texture.h"

#include "pxr/imaging/hgiPresent2/debugCodes.h"

#include <array>
#include <iostream>
#include <unordered_map>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


// This code uses the Metal APIs directly since it only works with HgiMetal.
// It doesn't really makes much sense to use the Hgi abstraction if it's always
// HgiMetal: it just adds overhead. Unless we go the full mile abstracting
// over surfaces, swapchains and other presentation concerns, using the Hgi
// layer doesn't have any real benefits. Implementing it would be a lot of
// additional work to replace not that many lines of API specific code.

namespace {
PXR_NAMESPACE_USING_DIRECTIVE

std::optional<MTLPixelFormat>
_FindPreferredSurfaceFormatMatch(HgiFormat format, bool needs_sRGB_Transform)
{
    if (needs_sRGB_Transform) {
        return MTLPixelFormatBGRA8Unorm_sRGB;
    }

    switch (format) {
    case HgiFormatUNorm8Vec4:
        return MTLPixelFormatBGRA8Unorm;
    case HgiFormatUNorm8Vec4srgb:
        return MTLPixelFormatBGRA8Unorm_sRGB;
    case HgiFormatFloat16Vec4:
        return MTLPixelFormatRGBA16Float;
    case HgiFormatFloat32Vec3:
    case HgiFormatFloat16Vec3:
    case HgiFormatFloat32Vec4:
        return MTLPixelFormatRGBA16Float;
    case HgiFormatInvalid:
    case HgiFormatCount:
        TF_CODING_ERROR("Invalid Format");
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

std::optional<CGColorSpaceRef>
_CGColorSpaceFromName(TfToken const& name, bool extended)
{
    // First name is clamped space (if it exists), second is extended (if it exists).
    // Nil means no color space conversion.
    static const std::unordered_map<TfToken, TfSmallVector<CFStringRef, 2>, TfToken::HashFunctor> namedColorSpaces = {
        {GfColorSpaceNames->LinearAP1, {kCGColorSpaceACESCGLinear}},
        {GfColorSpaceNames->LinearAP0, {}},
        {GfColorSpaceNames->LinearRec709, {kCGColorSpaceLinearSRGB, kCGColorSpaceExtendedLinearSRGB}},
        {GfColorSpaceNames->LinearP3D65, {kCGColorSpaceLinearDisplayP3, kCGColorSpaceExtendedLinearDisplayP3}},
        {GfColorSpaceNames->LinearRec2020, {kCGColorSpaceLinearITUR_2020, kCGColorSpaceExtendedLinearITUR_2020}},
        {GfColorSpaceNames->LinearAdobeRGB, {}},
        {GfColorSpaceNames->LinearCIEXYZD65, {kCGColorSpaceGenericXYZ}},
        {GfColorSpaceNames->SRGBRec709, {kCGColorSpaceSRGB, kCGColorSpaceExtendedSRGB}},
        // See note about G22Rec709 and G18Rec709 in vulkanSurface.cpp
        {GfColorSpaceNames->G22Rec709, {kCGColorSpaceITUR_709}},
        {GfColorSpaceNames->G18Rec709, {kCGColorSpaceITUR_709}},
        {GfColorSpaceNames->SRGBAP1, {}},
        {GfColorSpaceNames->G22AP1, {}},
        {GfColorSpaceNames->SRGBP3D65, {kCGColorSpaceDisplayP3, kCGColorSpaceExtendedDisplayP3}},
        {GfColorSpaceNames->G22AdobeRGB, {kCGColorSpaceAdobeRGB1998}},
        {GfColorSpaceNames->Identity, {nil}},
        {GfColorSpaceNames->Data, {nil}},
        {GfColorSpaceNames->Raw, {nil}},
        {GfColorSpaceNames->Unknown, {nil}},
        {GfColorSpaceNames->CIEXYZ, {kCGColorSpaceGenericXYZ}},
        {GfColorSpaceNames->LinearDisplayP3, {kCGColorSpaceExtendedLinearDisplayP3}},
    };

    if (const auto iter = namedColorSpaces.find(name);
        iter != namedColorSpaces.end()) {
        const auto& spaces = iter->second;
        if (spaces.empty()) {
            return std::nullopt;
        }
        if (!spaces[0]) {
            return nil;
        }
        if (extended && spaces.size() > 1) {
            return CGColorSpaceCreateWithName(spaces[1]);
        }
        return CGColorSpaceCreateWithName(spaces[0]);
    }

    TF_CODING_ERROR("Unrecognized GfColorSpaceNames: update _CGColorSpaceFromName");
    return std::nullopt;
}
} // namespace

PXR_NAMESPACE_OPEN_SCOPE


HgiPresent2MetalSurface::HgiPresent2MetalSurface(
    HgiMetal* hgi, _HideCAMetalLayerPtr layer)
    : _hgiMetal{hgi}
    , _metalLayer{layer.layer}
{
    if (!_hgiMetal) {
        TF_CODING_ERROR("hgi must be a valid HgiMetal pointer");
        return;
    }

    if (!_metalLayer) {
        TF_CODING_ERROR("surface must be a valid CAMetalLayer pointer");
        return;
    }
}

HgiPresent2MetalSurface::~HgiPresent2MetalSurface()
{
    @autoreleasepool {
        if (_currentDrawable) {
            [_currentDrawable release];
        }
    }
}

bool
HgiPresent2MetalSurface::IsColorFormatSupported(HgiFormat format) const
{
    return HgiIsFloatFormat(format) && !HgiIsCompressed(format);
}

bool
HgiPresent2MetalSurface::IsDepthFormatSupported(HgiFormat) const
{
    return false;
}

std::optional<bool>
HgiPresent2MetalSurface::IsValid() const
{
    return _hgiMetal && _metalLayer;
}

void
HgiPresent2MetalSurface::UpdateParams(const HgiPresent2Params& params)
{
    if (_params == params.surface) {
        return;
    }

    _params = params.surface;
    _layerReady = false;
}

HgiPresent2AovSet::RgbaSwizzle
HgiPresent2MetalSurface::GetRgbaSwizzle() const
{
    switch (_metalLayer.pixelFormat) {
    case MTLPixelFormatBGRA8Unorm:
    case MTLPixelFormatBGRA8Unorm_sRGB:
        return {2, 1, 0, 3};
    default:
        return HgiPresent2AovSet::identityRgbaSwizzle;
    }
}

void
HgiPresent2MetalSurface::_ApplyParamsToLayer()
{
    @autoreleasepool {
        if (_layerReady) {
            return;
        }

        // Metal only supports a clamped sRGB conversion
        const bool needs_sRGB_Transform = !_params.extendedColor &&
            _params.srcColorSpace == GfColorSpaceNames->LinearRec709 &&
            _params.preferredDstColorSpace == GfColorSpaceNames->SRGBRec709;
        const auto maybePixelFormat = _FindPreferredSurfaceFormatMatch(
            _params.preferredDstFormat, needs_sRGB_Transform);
        if (!maybePixelFormat) {
            TF_WARN("No compatible Metal layer format found: presentation is "
                    "disabled");
            return;
        }

        const auto maybeColorSpace =
            _CGColorSpaceFromName(_params.preferredDstColorSpace,
            _params.extendedColor);
        if (!maybeColorSpace) {
            TF_WARN("Requested Metal layer color space not found: presentation "
                    "is disabled");
            return;
        }

        if (TfDebug::IsEnabled(HGIPRESENT2_DUMP_CANDIDATE_SURFACE_FORMATS)) {
            std::string colorSpaceNameCStr;
            colorSpaceNameCStr.resize(128);
            if (CFStringRef colorSpaceName =
                    CGColorSpaceCopyName(*maybeColorSpace)) {
                CFStringGetCString(colorSpaceName, colorSpaceNameCStr.data(),
                    colorSpaceNameCStr.length(), kCFStringEncodingUTF8);
                CFRelease(colorSpaceName);
            }
            std::cout << "Metal surface format: " << *maybePixelFormat << ", "
                      << colorSpaceNameCStr << std::endl;
        }

        if (_metalLayer.device == nil) {
            _metalLayer.device = _hgiMetal->GetPrimaryDevice();
        }

        _metalLayer.allowsNextDrawableTimeout = NO;
        // So we can write to the drawable
        _metalLayer.framebufferOnly = NO;
        _metalLayer.maximumDrawableCount = 2;
        _metalLayer.pixelFormat = *maybePixelFormat;
        _metalLayer.colorspace = *maybeColorSpace;
        // Needs release: https://github.com/KhronosGroup/MoltenVK/issues/940
        CGColorSpaceRelease(*maybeColorSpace);
#if defined(ARCH_OS_OSX)
        _metalLayer.displaySyncEnabled = _params.wantVsync;
#endif

        _layerReady = true;
    }
}

HgiTextureHandle
HgiPresent2MetalSurface::Acquire(
    [[maybe_unused]] HgiCmds* commands, uint32_t width, uint32_t height)
{
    @autoreleasepool {
        if (!_metalLayer) {
            return {};
        }
        _metalLayer.drawableSize =
            CGSize{static_cast<float>(width), static_cast<float>(height)};
        _ApplyParamsToLayer();

        id<CAMetalDrawable> drawable = [_metalLayer nextDrawable];
        if (!drawable) {
            return {};
        }

        _currentDrawable = drawable;
        [_currentDrawable retain];

        return _hgiMetal->CreateTextureFromExisting(
            drawable.texture, "Metal Drawable Texture");
    }
}

void
HgiPresent2MetalSurface::SubmitAndPresent(std::unique_ptr<HgiCmds> commands)
{
    @autoreleasepool {
        if (!_currentDrawable) {
            return;
        }

        if (const auto commandBuffer =
                _hgiMetal->GetPrimaryCommandBuffer(commands.get())) {
            [commandBuffer presentDrawable:_currentDrawable];
            _hgiMetal->SetHasWork();
        } else {
            [_hgiMetal->GetSecondaryCommandBuffer()
                presentDrawable:_currentDrawable];
        }
        [_currentDrawable release];
        _currentDrawable = nil;

        // We need to force a sync here because we don't have the
        // synchronization mechanism to prevent the AOV from being reused
        // before presentation is finished.
        _hgiMetal->SubmitCmds(
            commands.get(), HgiSubmitWaitTypeWaitUntilCompleted);
    }
}

HgiMetal*
HgiPresent2DynamicCastHgiMetal(Hgi* hgi)
{
    return dynamic_cast<HgiMetal*>(hgi);
}


PXR_NAMESPACE_CLOSE_SCOPE
