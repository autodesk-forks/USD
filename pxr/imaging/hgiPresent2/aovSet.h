//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_AOVSET_H
#define PXR_IMAGING_HGIPRESENT2_AOVSET_H

#include "pxr/pxr.h"

#include "pxr/base/gf/colorSpace.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiPresent2/api.h"

#include <memory>
#include <optional>


PXR_NAMESPACE_OPEN_SCOPE

class HgiCmds;
struct HgiPresent2Params;

/// \class HgiPresent2AovSet
///
/// This functions much like a swap-chain, but with relaxed requirements. It
/// allows a user to acquire an image, modify it, then submit it for
/// presentation. But unlike a swap-chain, there is no requirement that an image
/// be reused, that all images have the same format, or that they be acquired in
/// a specific order.
///
class HgiPresent2AovSet
{
public:
    /// Indices for swizzling a color from any order to RGBA.
    struct RgbaSwizzle
    {
        /// The index of the R component.
        uint8_t r : 2;
        /// The index of the G component.
        uint8_t g : 2;
        /// The index of the B component.
        uint8_t b : 2;
        /// The index of the A component.
        uint8_t a : 2;

        /// Query the component index by its RGBA index:
        /// R = 0, G = 1, B = 2, A = 3.
        uint8_t operator[](size_t i) const
        {
            switch (i) {
            case 0:
                return r;
            case 1:
                return g;
            case 2:
                return b;
            case 3:
                return a;
            default:
                ARCH_GUARANTEE_TO_COMPILER(false);
            }
        }

        bool operator==(RgbaSwizzle other) const
        {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }

        bool operator!=(RgbaSwizzle other) const
        {
            return !(*this == other);
        }
    };

    /// The identity RGBA swizzle.
    static constexpr RgbaSwizzle identityRgbaSwizzle = {0, 1, 2, 3};

    HGIPRESENT2_API
    virtual ~HgiPresent2AovSet() = default;

    /// Same as HgiPresent2Impl::IsColorFormatSupported(HgiFormat)
    HGIPRESENT2_API
    virtual bool IsColorFormatSupported(HgiFormat format) const = 0;

    /// Same as HgiPresent2Impl::IsDepthFormatSupported(HgiFormat)
    HGIPRESENT2_API
    virtual bool IsDepthFormatSupported(HgiFormat format) const = 0;

    /// Same as HgiPresent2Impl::IsValid()
    HGIPRESENT2_API
    virtual std::optional<bool> IsValid() const = 0;

    /// Same as HgiPresent2Impl::UpdateParams(const HgiPresent2Params&)
    HGIPRESENT2_API
    virtual void UpdateParams(const HgiPresent2Params& params) = 0;

    /// Hgi assumes that the color component order is always RGBA. This can't
    /// hold for presentation as some surfaces only support BGRA (or ABGR).
    /// Normally this is not a problem when using the surface in
    /// HgiBlitCmds::BlitTexture() or as a render attachment, since it will do
    /// the component reordering, but when using the HgiBlitCmds::Copy*()
    /// functions this is not the case. This function can be called to get the
    /// necessary swizzle to convert from the underlying presentation surface
    /// component order to RGBA.
    HGIPRESENT2_API
    virtual RgbaSwizzle GetRgbaSwizzle() const = 0;

    /// May return the next AOV to use for presentation. This function is
    /// allowed to block as long as necessary, and return nothing if no AOV is
    /// available (timeout or error). The function may add commands needed to
    /// acquire the AOV of the desired width and height to the command buffer.
    /// The resulting AOV dimensions might be different due to implementation
    /// limitations.
    ///
    /// The implementation may impose usage limits for the texture. At least the
    /// following usages must be supported:
    /// - Color target
    /// - BlitTexture() destination
    ///
    /// Every non-null returning call to Acquire() must be followed by a
    /// matching SubmitAndPresent() call.
    HGIPRESENT2_API
    virtual HgiTextureHandle Acquire(
        HgiCmds* commands, uint32_t width, uint32_t height) = 0;

    /// Finishes the command buffer, submits it, and presents the last acquired
    /// AOV. This function receives a command buffer which already records the
    /// necessary commands to assign the AOV content for presentation. It may
    /// add additional commands needed to finish presentation.
    ///
    /// Due to Hgi synchronization limitations, it's not possible for an
    /// implementation to ensure the presentation command buffer is done
    /// executing before drawing for the next frame has started, which might
    /// cause problems if dependencies of the presentation command buffer are
    /// reused for the next frame. An implementation is allowed to wait until
    /// the command buffer is done executing before returning to avoid such
    /// issues.
    ///
    /// An implementation can assume the AOV is free to be re-used once the
    /// submitted command buffer has finished executing.
    ///
    /// Every call to SubmitAndPresent() will be preceded by a matching
    /// non-null-returning Acquire() call.
    HGIPRESENT2_API
    virtual void SubmitAndPresent(std::unique_ptr<HgiCmds> commands) = 0;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
