//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_HGIPRESENT2IMPL_H
#define PXR_IMAGING_HGIPRESENT2_HGIPRESENT2IMPL_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiPresent2/api.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE


class Hgi;
class HgiTexture;
struct HgiPresent2Params;

///
/// This is the interface for actual implementations of presentation. It may be
/// implemented by an application to provide fully custom presentation.
///
class HgiPresent2Impl
{
public:
    /// For convenience, a presentation implementation always has access to the
    /// Hgi instance.
    HGIPRESENT2_API
    explicit HgiPresent2Impl(Hgi* hgi)
        : _hgi(hgi)
    {
    }

    HGIPRESENT2_API
    virtual ~HgiPresent2Impl() = default;

    /// Check whether or not a given AOV color format is supported for
    /// presentation. It's generally expected that any 3 or 4 component format
    /// convertible to float is at least supported, but the implementation is
    /// free to impose additional requirements.
    HGIPRESENT2_API
    virtual bool IsColorFormatSupported(HgiFormat format) const = 0;

    /// Check whether or not a given AOV depth format is supported for
    /// presentation. Depth presentation is an optional feature: this may return
    /// false for all formats.
    HGIPRESENT2_API
    virtual bool IsDepthFormatSupported(HgiFormat format) const = 0;

    /// Set the surface parameters for the subsequent Acquire() calls.
    HGIPRESENT2_API
    virtual void UpdateParams(const HgiPresent2Params& params) = 0;

    /// Check if the current presentation is valid and functioning. This might
    /// return no value if the presentation is in an indeterminate state. For
    /// example initialization might not be complete until the first call to
    /// Present().
    HGIPRESENT2_API
    virtual std::optional<bool> IsValid() const = 0;

    /// Do the presentation. The depth AOV can be ignored if not required by the
    /// implementation. Any operation is valid (even doing nothing), as long as
    /// the AOVs are free to be reused once the function returns.
    HGIPRESENT2_API
    virtual void Present(
        HgiTextureHandle const& srcColor, HgiTextureHandle const& srcDepth) = 0;

protected:
    Hgi* _hgi;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
