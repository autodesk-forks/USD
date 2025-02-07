//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent2/present.h"

#include "pxr/imaging/hgiPresent2/presentImpl.h"

#include "pxr/imaging/hgiPresent2/aovBlit.h"
#include "pxr/imaging/hgiPresent2/noOp.h"

#if defined(PXR_GL_SUPPORT_ENABLED)
#include "pxr/imaging/hgiPresent2/glInterop.h"
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
#include "pxr/imaging/hgiPresent2/vulkanSurface.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#endif

#if defined(PXR_METAL_SUPPORT_ENABLED)
#include "pxr/imaging/hgiPresent2/metalSurface.h"
#endif


PXR_NAMESPACE_OPEN_SCOPE


HgiPresent2::HgiPresent2(std::unique_ptr<HgiPresent2Impl> impl)
    : _hgiPresent2Impl(std::move(impl))
{
}

HgiPresent2::HgiPresent2(HgiPresent2&&) noexcept = default;

HgiPresent2& HgiPresent2::operator=(HgiPresent2&&) noexcept = default;

HgiPresent2::~HgiPresent2() = default;

bool
HgiPresent2::IsColorFormatSupported(HgiFormat format) const
{
    if (!TF_VERIFY(_hgiPresent2Impl)) {
        return false;
    }

    return _hgiPresent2Impl->IsColorFormatSupported(format);
}

bool
HgiPresent2::IsDepthFormatSupported(HgiFormat format) const
{
    if (!TF_VERIFY(_hgiPresent2Impl)) {
        return false;
    }

    return _hgiPresent2Impl->IsDepthFormatSupported(format);
}

std::optional<bool>
HgiPresent2::IsValid() const
{
    if (!TF_VERIFY(_hgiPresent2Impl)) {
        return false;
    }

    return _hgiPresent2Impl->IsValid();
}

void
HgiPresent2::UpdateParams(const HgiPresent2Params& params)
{
    if (!TF_VERIFY(_hgiPresent2Impl)) {
        return;
    }

    _hgiPresent2Impl->UpdateParams(params);
}

void
HgiPresent2::Destroy()
{
    if (!TF_VERIFY(_hgiPresent2Impl)) {
        return;
    }

    _hgiPresent2Impl = nullptr;
}

void
HgiPresent2::Present(
    HgiTextureHandle const& srcColor, HgiTextureHandle const& srcDepth)
{
    if (!TF_VERIFY(_hgiPresent2Impl)) {
        return;
    }

    _hgiPresent2Impl->Present(srcColor, srcDepth);
}

/*static*/ HgiPresent2
HgiPresent2::Create(std::unique_ptr<HgiPresent2Impl> impl)
{
    return HgiPresent2{std::move(impl)};
}

/*static*/ HgiPresent2
HgiPresent2::CreateNoOp()
{
    return HgiPresent2{std::make_unique<HgiPresent2NoOp>()};
}

/*static*/ HgiPresent2
HgiPresent2::CreateAovBlit(Hgi* hgi, std::unique_ptr<HgiPresent2AovSet> aovSet)
{
    return HgiPresent2{
        std::make_unique<HgiPresent2AovBlit>(hgi, std::move(aovSet))};
}

/*static*/ HgiPresent2
HgiPresent2::CreateAovBlit(Hgi* hgi, const HgiPresent2SurfaceHandle& surface)
{
    if (auto aovSet = HgiPresent2SurfaceToAovSet(hgi, surface)) {
        return HgiPresent2{std::make_unique<HgiPresent2AovBlit>(
            hgi, std::move(aovSet))};
    }

    return CreateNoOp();
}

/*static*/ HgiPresent2
HgiPresent2::CreateGLInterop(Hgi* hgi, uint32_t fboName)
{
    return HgiPresent2{
        std::make_unique<HgiPresent2GLInterop>(hgi, fboName)};
}

std::unique_ptr<HgiPresent2AovSet>
HgiPresent2SurfaceToAovSet(Hgi* hgi, const HgiPresent2SurfaceHandle& surface)
{
    return std::unique_ptr<HgiPresent2AovSet>(std::visit(
        [hgi](auto handle) -> HgiPresent2AovSet* {
            using Handle = std::decay_t<decltype(handle)>;
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
            if constexpr (std::is_same_v<Handle, VkSurfaceKHR>) {
                if (const auto hgiVulkan = dynamic_cast<HgiVulkan*>(hgi)) {
                    return new HgiPresent2VulkanSurface{hgiVulkan, handle};
                }
                TF_WARN("Unsupported Hgi: presentation is disabled");
                return nullptr;
            }
#endif
#if defined(PXR_METAL_SUPPORT_ENABLED)
            if constexpr (std::is_same_v<Handle, CAMetalLayerPtr>) {
                if (const auto hgiMetal = HgiPresent2DynamicCastHgiMetal(hgi)) {
                    return new HgiPresent2MetalSurface{hgiMetal, handle};
                }
                TF_WARN("Unsupported Hgi: presentation is disabled");
                return nullptr;
            }
#endif
            TF_CODING_ERROR("Invalid surface handle");
            return nullptr;
        },
        surface));
}


PXR_NAMESPACE_CLOSE_SCOPE
