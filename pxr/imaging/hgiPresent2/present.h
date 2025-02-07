//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_PRESENT_H
#define PXR_IMAGING_HGIPRESENT2_PRESENT_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiPresent2/aovSet.h"
#include "pxr/imaging/hgiPresent2/api.h"
#include "pxr/imaging/hgiPresent2/presentImpl.h"

#include "pxr/imaging/hgiInterop/hgiInterop.h"

#include <optional>
#include <variant>

#if defined(ARCH_OS_DARWIN)
#if __OBJC__
@class CAMetalLayer;
using CAMetalLayerPtr = const CAMetalLayer*;
static_assert(sizeof(CAMetalLayerPtr) == sizeof(void*));
#else
/// A type-safe C++ forward declaration of the CAMetalLayer Objective-C class.
/// You may cast to this type if necessary.
using CAMetalLayerPtr = const struct _pxr__CAMetalLayer*;
#endif
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
#include "pxr/imaging/hgiVulkan/vulkan.h"
#endif

#if defined(PXR_WEBGPU_SUPPORT_ENABLED)
#include <webgpu/webgpu_cpp.h>
#endif


PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HgiTexture;

/// Represents all supported handle types. The actual available types in the
/// union will depend on the USD features enabled at build time.
using HgiPresent2SurfaceHandle = std::variant<
    std::monostate
#if defined(ARCH_OS_DARWIN)
    , CAMetalLayerPtr
#endif
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    , VkSurfaceKHR
#endif
#if defined(PXR_WEBGPU_SUPPORT_ENABLED)
    , wgpu::Surface
#endif
    >;

/// This is a common set of parameters for "surface based" presentation
/// implementations. They allow an application to provide "opinions" to the
/// presentation implementation.
struct HgiPresent2SurfaceParams
{
    /// Source texture color space. Supported values are system dependent. Only
    /// LinearRec709 and SRGBRec709 are guaranteed.
    TfToken srcColorSpace{GfColorSpaceNames->LinearRec709};
    /// If true, then source may contain colors outside the primaries of its
    /// color space, and we wish to preserve these in the presented output.
    /// Otherwise the out-of-bounds colors will be clamped to the space.
    bool extendedColor{false};
    /// The preferred destination format. Not all values are supported. If no
    /// exact match is possible, then a "best fit" is performed. This fit will
    /// try to maximize coverage of the original color range, minimize
    /// quantization losses, and minimize unused storage bits.
    HgiFormat preferredDstFormat{HgiFormatUNorm8Vec4};
    /// Must be the same as srcColorSpace, except when srcColorSpace is
    /// LinearRec709 and extendedColor is false, then it can be SRGBRec709, in
    /// which case the sRGB transfer function is applied in hardware before
    /// presentation.
    TfToken preferredDstColorSpace{GfColorSpaceNames->SRGBRec709};
    /// Try to enable display refresh rate synchronization (aka v-sync).
    bool wantVsync{true};

    bool operator==(const HgiPresent2SurfaceParams& other) const
    {
        return srcColorSpace == other.srcColorSpace &&
            extendedColor == other.extendedColor &&
            preferredDstFormat == other.preferredDstFormat &&
            preferredDstColorSpace == other.preferredDstColorSpace &&
            wantVsync == other.wantVsync;
    }

    bool operator!=(const HgiPresent2SurfaceParams& other) const
    {
        return !(*this == other);
    }
};

/// The composition parameters will simply reuse the structure defined by the
/// HgiInterop library (see below). There are no additional fields, but some
/// could be added in the future if required.
struct HgiPresent2CompositionParams :
    HgiInteropCompositionParams
{
};

/// A generic set of parameters for presentation. Which parameters are used
/// depends on the presentation implementation.
struct HgiPresent2Params
{
    /// Parameter for configuring a presentation surface.
    /// See HgiPresent2SurfaceParams.
    HgiPresent2SurfaceParams surface;
    /// Parameter for compositing into an existing frambuffer.
    /// See HgiPresent2CompositionParams.
    HgiPresent2CompositionParams composition;

    bool operator==(const HgiPresent2Params& other) const
    {
        return surface == other.surface &&
            composition == other.composition;
    }

    bool operator!=(const HgiPresent2Params& other) const
    {
        return !(*this == other);
    }
};

///
/// \class HgiPresent2
///
/// Enables submitting a color AOV, and optionally a depth AOV, to an externally
/// managed presentation implementation, or one of the builtin supported
/// surfaces.
///
class HgiPresent2 final
{
public:
    HGIPRESENT2_API
    HgiPresent2(HgiPresent2&&) noexcept;

    HGIPRESENT2_API
    HgiPresent2& operator=(HgiPresent2&&) noexcept;

    HgiPresent2(const HgiPresent2&) = delete;
    HgiPresent2& operator=(const HgiPresent2&) = delete;

    HGIPRESENT2_API
    ~HgiPresent2();

    /// Get the actual presentation implementation, optionally downcasting it to
    /// the given HgiPresent2Impl subclass. This returns null if the
    /// implementation is not actually of the given type.
    template<typename Impl = HgiPresent2Impl>
    std::enable_if_t<std::is_base_of_v<HgiPresent2Impl, Impl>, Impl*>
    HGIPRESENT2_API GetImplementation() const
    {
        return dynamic_cast<Impl*>(_hgiPresent2Impl.get());
    }

    /// See HgiPresent2Impl::IsColorFormatSupported(HgiFormat).
    HGIPRESENT2_API
    bool IsColorFormatSupported(HgiFormat format) const;

    /// See HgiPresent2Impl::IsDepthFormatSupported(HgiFormat).
    HGIPRESENT2_API
    bool IsDepthFormatSupported(HgiFormat format) const;

    /// See HgiPresent2Impl::IsValid().
    HGIPRESENT2_API
    std::optional<bool> IsValid() const;

    /// See HgiPresent2Impl::UpdateParams(const HgiPresent2Params&).
    HGIPRESENT2_API
    void UpdateParams(const HgiPresent2Params& params);

    /// See HgiPresent2Impl::Present(HgiTextureHandle const&, HgiTextureHandle
    /// const&).
    HGIPRESENT2_API
    void Present(
        HgiTextureHandle const& srcColor, HgiTextureHandle const& srcDepth);

    /// Immediately destroy the presentation implementation. Presentation
    /// resources are released, and all future function calls on this object
    /// become invalid.
    HGIPRESENT2_API
    void Destroy();

    /// Create an HgiPresent2 instance from an existing HgiPresent2Impl instance.
    /// Ownership will be taken. This is used to create fully custom
    /// presentation solutions.
    HGIPRESENT2_API
    static HgiPresent2 Create(std::unique_ptr<HgiPresent2Impl> impl);

    /// Create a no-op presentation. Used as a "null" fallback when nothing else
    /// is available.
    HGIPRESENT2_API
    static HgiPresent2 CreateNoOp();

    /// Create an HgiPresent2 instance from a new HgiPresent2AovBlit instance,
    /// which will take ownership of the given HgiPresent2AovSet instance and
    /// use it as a source of AOVs. This provides a level of abstraction over
    /// presentation blitting, but allows control over the presentation target
    /// AOVs.
    HGIPRESENT2_API
    static HgiPresent2 CreateAovBlit(
        Hgi* hgi, std::unique_ptr<HgiPresent2AovSet> aovSet);

    /// Create an HgiPresent2 instance from a new HgiPresent2AovBlit instance,
    /// which will be initialized with a new HgiPresent2AovSet created using the
    /// given HgiPresent2SurfaceHandle. This is mostly a convenience function to
    /// simplify generic surface presentation.
    ///
    /// If the Hgi implementation doesn't support the
    /// HgiPresent2SurfaceHandle type, a coding error is generated, and the
    /// returned presentation behaves as if the implementation was
    /// HgiPresent2NoOp
    HGIPRESENT2_API
    static HgiPresent2 CreateAovBlit(
        Hgi* hgi, const HgiPresent2SurfaceHandle& surface);

    /// Create an HgiPresent2 instance from a new HgiPresent2InteropGL instance,
    /// which will be initialized using the given OpenGL framebuffer object
    /// name.
    HGIPRESENT2_API
    static HgiPresent2 CreateGLInterop(Hgi* hgi, uint32_t fboName);

private:
    explicit HgiPresent2(std::unique_ptr<HgiPresent2Impl> impl);

    std::unique_ptr<HgiPresent2Impl> _hgiPresent2Impl;
};

/// Create a new HgiPresent2AovSet instance using the given
/// HgiPresent2SurfaceHandle. If the Hgi implementation doesn't support the
/// HgiPresent2SurfaceHandle type, a coding error is generated, and null is
/// returned.
HGIPRESENT2_API
std::unique_ptr<HgiPresent2AovSet>
HgiPresent2SurfaceToAovSet(Hgi* hgi, const HgiPresent2SurfaceHandle& surface);

PXR_NAMESPACE_CLOSE_SCOPE


#endif
