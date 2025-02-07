//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent2/vulkanSurface.h"

#include "pxr/imaging/hgiVulkan/blitCmds.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include "pxr/imaging/hgiPresent2/colorVolume.h"
#include "pxr/imaging/hgiPresent2/debugCodes.h"

#include <vulkan/vk_enum_string_helper.h>

#include <array>
#include <iostream>

// This code uses the Vulkan APIs directly since it only works with HgiVulkan.
// It doesn't really makes much sense to use the Hgi abstraction if it's always
// HgiVulkan: it just adds overhead. Unless we go the full mile abstracting
// over surfaces, swapchains and other presentation concerns, using the Hgi
// layer doesn't have any real benefits. Implementing it would be a lot of
// additional work to replace not that many lines of API specific code.

namespace {
PXR_NAMESPACE_USING_DIRECTIVE

std::optional<VkColorSpaceKHR>
_VkColorSpaceFromName(TfToken const& name, bool extended)
{
    // First name is clamped space (if it exists), second is extended (if it exists).
    static const std::unordered_map<TfToken, TfSmallVector<VkColorSpaceKHR, 2>, TfToken::HashFunctor> namedColorSpaces = {
        {GfColorSpaceNames->LinearAP1, {}},
        {GfColorSpaceNames->LinearAP0, {}},
        {GfColorSpaceNames->LinearRec709, {VK_COLOR_SPACE_BT709_LINEAR_EXT, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT}},
        {GfColorSpaceNames->LinearP3D65, {VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT}},
        {GfColorSpaceNames->LinearRec2020, {VK_COLOR_SPACE_BT2020_LINEAR_EXT}},
        {GfColorSpaceNames->LinearAdobeRGB, {VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT}},
        {GfColorSpaceNames->LinearCIEXYZD65, {}},
        {GfColorSpaceNames->SRGBRec709, {VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT}},
        // Neither a gamma of 1.8 nor 2.2 are good approximations
        // for the true BT.709 transfer functions. Actually the
        // in between value of 2.0 is a much better approximation,
        // see: https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html#TRANSFER_ITU_INVOETF
        // We'll just allow both.
        {GfColorSpaceNames->G22Rec709, {VK_COLOR_SPACE_BT709_NONLINEAR_EXT}},
        {GfColorSpaceNames->G18Rec709, {VK_COLOR_SPACE_BT709_NONLINEAR_EXT}},
        {GfColorSpaceNames->SRGBAP1, {}},
        {GfColorSpaceNames->G22AP1, {}},
        {GfColorSpaceNames->SRGBP3D65, {VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT}},
        {GfColorSpaceNames->G22AdobeRGB, {VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT}},
        {GfColorSpaceNames->Identity, {VK_COLOR_SPACE_PASS_THROUGH_EXT}},
        {GfColorSpaceNames->Data, {VK_COLOR_SPACE_PASS_THROUGH_EXT}},
        {GfColorSpaceNames->Raw, {VK_COLOR_SPACE_PASS_THROUGH_EXT}},
        {GfColorSpaceNames->Unknown, {VK_COLOR_SPACE_PASS_THROUGH_EXT}},
        {GfColorSpaceNames->CIEXYZ, {}},
        {GfColorSpaceNames->LinearDisplayP3, {VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT}},
    };

    if (const auto iter = namedColorSpaces.find(name);
        iter != namedColorSpaces.end()) {
        const auto& spaces = iter->second;
        if (spaces.empty()) {
            return std::nullopt;
        }
        if (extended && spaces.size() > 1) {
            return spaces[1];
        }
        return spaces[0];
    }

    TF_CODING_ERROR("Unrecognized GfColorSpaceNames: update _VkColorSpaceFromName");
    return std::nullopt;
}

bool
_FormatConvertsColorSpace(VkFormat format, const TfToken& srcColorSpace,
    const TfToken& dstColorSpace, bool extendedColor)
{
    switch (format) {
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return !extendedColor &&
            srcColorSpace == GfColorSpaceNames->LinearRec709 &&
            dstColorSpace == GfColorSpaceNames->SRGBRec709;
    default:
        return srcColorSpace == dstColorSpace;
    }
}

/// Get the color volume that is defined by a format.
ColorVolume
_GetColorVolume(VkFormat format)
{
    // The Vulkan format list is very long, but most formats don't make sense
    // for presentation. While the API says almost nothing about what can be
    // supported, we can make some reasonable assumptions and apply some of our
    // own criteria. We ignore:
    //   - Compressed formats
    //   - Extension formats
    //   - Integer and scaled format
    //   - Any format that isn't RGB or RGBA
    //   - Formats with less than 8 bits per component
    //   - Formats with more than 32 bits per component
    // We could probably also ignore signed normalized formats, but color
    // spaces with negative values are a thing, so we opt to keep them.
    // Additionally the type must be listed in HgiFormat (either exactly or
    // compatible), which rules out quite a few useful presentation types. We've
    // commented them out, if HgiFormat gains support feel free to uncomment.
    // (This is a holdover from the original HgiPresent2 prototype which didn't
    // have this HgiFormat compatiblity requirement.)
    switch (format) {
    // case VK_FORMAT_R8G8B8_UNORM:
    // case VK_FORMAT_R8G8B8_SRGB:
    // case VK_FORMAT_B8G8R8_UNORM:
    // case VK_FORMAT_B8G8R8_SRGB:
    //     return ColorVolume::ForNormInt(false, 8, 3);
    // case VK_FORMAT_R8G8B8_SNORM:
    // case VK_FORMAT_B8G8R8_SNORM:
    //     return ColorVolume::ForNormInt(true, 8, 3);
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
    // case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
    // case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return ColorVolume::ForNormInt(false, 8, 4);
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    // case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        return ColorVolume::ForNormInt(true, 8, 4);
    // case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    // case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    //     return ColorVolume::ForNormInt(false, 10, 4);
    // case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        return ColorVolume::ForNormInt(true, 10, 4);
    // case VK_FORMAT_R16G16B16_UNORM:
    //     return ColorVolume::ForNormInt(false, 16, 3);
    // case VK_FORMAT_R16G16B16_SNORM:
    //     return ColorVolume::ForNormInt(true, 16, 3);
    case VK_FORMAT_R16G16B16_SFLOAT:
        return ColorVolume::ForHalf(3);
    // case VK_FORMAT_R16G16B16A16_UNORM:
    //     return ColorVolume::ForNormInt(false, 16, 4);
    // case VK_FORMAT_R16G16B16A16_SNORM:
    //     return ColorVolume::ForNormInt(true, 16, 4);
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return ColorVolume::ForHalf(4);
    case VK_FORMAT_R32G32B32_SFLOAT:
        return ColorVolume::ForSingle(3);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return ColorVolume::ForSingle(4);
    // case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    //     return ColorVolume::ForFloat(false, 5, 5, 3);
    // case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    //     return ColorVolume::ForFloat(false, 5, 9, 3);
    default:
        return {};
    }
}
} // namespace


PXR_NAMESPACE_OPEN_SCOPE


HgiPresent2VulkanSurface::HgiPresent2VulkanSurface(
    HgiVulkan* hgi, VkSurfaceKHR surface)
    : _hgiVulkan{hgi}
    , _surface{surface}
{
    if (!_hgiVulkan) {
        TF_CODING_ERROR("hgi must be a valid HgiVulkan pointer");
        return;
    }

    if (!_surface) {
        TF_CODING_ERROR("surface must be a valid VkSurfaceKHR handle");
        return;
    }

    const auto hgiDevice = _hgiVulkan->GetPrimaryDevice();
    const auto device = hgiDevice->GetVulkanDevice();

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    HGIVULKAN_VERIFY_VK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo,
        HgiVulkanAllocator(), &_imageAcquiredSemaphore));
}

HgiPresent2VulkanSurface::~HgiPresent2VulkanSurface()
{
    const auto hgiDevice = _hgiVulkan->GetPrimaryDevice();
    const auto device = hgiDevice->GetVulkanDevice();

    // The Vulkan spec isn't really clear whether or not we have to wait for
    // vkQueuePresentKHR() to finish before we can safely destroy the
    // semaphores. In practice we've observed validation errors, so we need to
    // wait. Without the relatively new VK_EXT_swapchain_maintenance1, we don't
    // have any better option than calling vkQueueWaitIdle(). See this post for
    // more information: https://stackoverflow.com/questions/75437792
    HGIVULKAN_VERIFY_VK_RESULT(vkQueueWaitIdle(
        hgiDevice->GetCommandQueue()->GetVulkanGraphicsQueue()));

    _DestroySurfaceResources(
        _hgiVulkan->GetVulkanInstance()->GetVulkanInstance(), device);

    vkDestroySemaphore(device, _imageAcquiredSemaphore, HgiVulkanAllocator());

    for (const auto semaphore : _blitCompleteSemaphores) {
        vkDestroySemaphore(device, semaphore, HgiVulkanAllocator());
    }
    _blitCompleteSemaphores.clear();
}

void
HgiPresent2VulkanSurface::_DestroySurfaceResources(
    VkInstance instance, VkDevice device)
{
    _DestroySwapchainResources(device);

    _validSurfaceProperties = false;
}

void
HgiPresent2VulkanSurface::_ResolveSurfaceProperties(
    VkPhysicalDevice physicalDevice)
{
    if (!_surface || _validSurfaceProperties) {
        return;
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    HGIVULKAN_VERIFY_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice, _surface, &surfaceCapabilities));

    // 0 means unlimited (until you run out of memory).
    // We don't really need more than 2.
    const auto maxImageCount = surfaceCapabilities.maxImageCount == 0 ?
        std::max(2u, surfaceCapabilities.minImageCount) :
        surfaceCapabilities.maxImageCount;
    _imageCount =
        std::clamp(2u, surfaceCapabilities.minImageCount, maxImageCount);

    // We need to be conservative about which usages we choose, because they're
    // not guaranteed to be supported for all swapchain formats, and there's no
    // good way to check what usages are format allows, only if its supports a
    // set of usages at once. Anyway according to the AovSet interface, we only
    // need to ensure the image can be blitted to (TRANSFER_DST) or used as a
    // color attachment. We also want to read the blit result for testing
    // (TRANSFER_SRC). So these are the only usages we need: COLOR_ATTACHMENT
    // is guaranteed, and TRANSFER_SRC and TRANSFER_DST are 100% supported. See:
    //     https://vulkan.gpuinfo.org/listsurfaceusageflags.php
    const VkImageUsageFlags transferFlags =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((surfaceCapabilities.supportedUsageFlags & transferFlags) !=
        transferFlags) {
        TF_WARN("Vulkan image transfer usage unsupported: presentation is "
                "disabled");
        return;
    }
    _imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (surfaceCapabilities.supportedCompositeAlpha &
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
        // Defer to the window if we can. This bit is probably only ever set
        // alone according to the spec's wording:
        //  The way in which the presentation engine treats the alpha component
        //  in the images is unknown to the Vulkan API. Instead, the application
        //  is responsible for setting the composite alpha blending mode using
        //  native window system commands.
        _alphaComposition = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    } else if (surfaceCapabilities.supportedCompositeAlpha &
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        _alphaComposition = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    } else {
        TF_WARN("Vulkan opaque or inherit alpha composition unsupported: "
                "presentation is disabled");
        return;
    }

    _presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!_params.wantVsync) {
        uint32_t presentModeCount{};
        HGIVULKAN_VERIFY_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, _surface, &presentModeCount, nullptr));
        std::vector<VkPresentModeKHR> presentModes{presentModeCount};
        HGIVULKAN_VERIFY_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, _surface, &presentModeCount, presentModes.data()));
        if (std::find(presentModes.begin(), presentModes.end(),
                VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end()) {
            _presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    if (!(surfaceCapabilities.supportedTransforms &
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)) {
        // According to the Vulkan capability database,
        // 100% of drivers support this so we should never get here.
        TF_WARN("Vulkan surface transform is unsupported: presentation is "
                "disabled");
        return;
    }

    // Note: while it's possible to get the surface size from
    // surfaceCapabilities.currentExtent, it's not guaranteed to be
    // available, so we still need to rely on the window size coming
    // from the present task.
    _swapchainExtent.width =
        std::clamp(static_cast<uint32_t>(_srcTextureSize[0]),
            surfaceCapabilities.minImageExtent.width,
            surfaceCapabilities.maxImageExtent.width);
    _swapchainExtent.height =
        std::clamp(static_cast<uint32_t>(_srcTextureSize[1]),
            surfaceCapabilities.minImageExtent.height,
            surfaceCapabilities.maxImageExtent.height);

    const auto minComponents = HgiGetComponentCount(_params.preferredDstFormat);
    const auto preferredColorVolume = _GetColorVolume(
        HgiVulkanConversions::GetFormat(_params.preferredDstFormat));

    uint32_t formatCount{};
    HGIVULKAN_VERIFY_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, _surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> formats{formatCount};
    HGIVULKAN_VERIFY_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, _surface, &formatCount, formats.data()));

    const auto maybeColorSpace = _VkColorSpaceFromName(
        _params.preferredDstColorSpace, _params.extendedColor);
    if (!maybeColorSpace) {
        TF_WARN("No compatible Vulkan surface color space found: presentation "
                "is disabled");
        return;
    }

    using CandidateFormat = std::pair<VkSurfaceFormatKHR, ColorVolume::Match>;
    std::vector<CandidateFormat> candidateFormats;
    for (const auto formatAndColorSpace : formats) {
        const auto [format, colorSpace] = formatAndColorSpace;
        if (colorSpace != maybeColorSpace) {
            continue;
        }

        const auto colorVolume = _GetColorVolume(format);
        if (!colorVolume || colorVolume.dimensions < minComponents ||
            !_FormatConvertsColorSpace(format, _params.srcColorSpace,
                _params.preferredDstColorSpace, _params.extendedColor)) {
            continue;
        }

        candidateFormats.emplace_back(formatAndColorSpace,
            preferredColorVolume.ComputeMatch(colorVolume));
    }

    if (candidateFormats.empty()) {
        TF_WARN("No compatible Vulkan surface format found: presentation is "
                "disabled");
        return;
    }

    // We'll assume that the driver will return formats in some order of
    // preference, so use stable sorting to preserve it when matches are tied.
    std::stable_sort(candidateFormats.begin(), candidateFormats.end(),
        [](const CandidateFormat& a, const CandidateFormat& b) {
            return a.second < b.second;
        });

    if (TfDebug::IsEnabled(HGIPRESENT2_DUMP_CANDIDATE_SURFACE_FORMATS)) {
        std::cout << "Candidate surface formats (first won):\n";
        for (const auto& [format, match] : candidateFormats) {
            std::cout << "    " << string_VkFormat(format.format) << ", "
                      << string_VkColorSpaceKHR(format.colorSpace) << ", "
                      << match << "\n";
        }
        std::cout << std::endl;
    }

    _imageFormat = candidateFormats.front().first;

    _validSurfaceProperties = true;
}

void
HgiPresent2VulkanSurface::_CreateSwapchainResources(VkDevice device)
{
    if (!_surface || !_validSurfaceProperties || _validSwapchainResources) {
        return;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = _surface;
    swapchainCreateInfo.minImageCount = _imageCount;
    swapchainCreateInfo.imageFormat = _imageFormat.format;
    swapchainCreateInfo.imageColorSpace = _imageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = _swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = _imageUsageFlags;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = _alphaComposition;
    swapchainCreateInfo.presentMode = _presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    HGIVULKAN_VERIFY_VK_RESULT(vkCreateSwapchainKHR(
        device, &swapchainCreateInfo, HgiVulkanAllocator(), &_swapchain));

    if (!_swapchain) {
        return;
    }

    uint32_t imageCount{};
    HGIVULKAN_VERIFY_VK_RESULT(
        vkGetSwapchainImagesKHR(device, _swapchain, &imageCount, nullptr));

    TfSmallVector<VkImage, 4> swapchainImages;
    swapchainImages.resize(imageCount);
    HGIVULKAN_VERIFY_VK_RESULT(vkGetSwapchainImagesKHR(
        device, _swapchain, &imageCount, swapchainImages.data()));

    _validSwapchainResources = !swapchainImages.empty() &&
        std::all_of(swapchainImages.begin(), swapchainImages.end(),
            [](VkImage image) { return image != nullptr; });

    HgiTextureDesc desc{};
    desc.usage = HgiVulkanConversions::GetTextureUsage(_imageUsageFlags);
    desc.format = HgiVulkanConversions::GetFormat(_imageFormat.format);
    desc.dimensions = {static_cast<int>(_swapchainExtent.width),
        static_cast<int>(_swapchainExtent.height), 1};

    _swapchainTextures.resize(imageCount);
    for (uint32_t imageIndex = 0; imageIndex < imageCount; imageIndex++) {
        desc.debugName = "Vukan Swapchain Image " + std::to_string(imageIndex);
        _swapchainTextures[imageIndex] = _hgiVulkan->CreateTextureFromExisting(
            desc, swapchainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
            _imageFormat.format, _imageUsageFlags);
    }

    // We use one semaphore per swapchain image (as recommended by Khronos) to
    // avoid the issue of reusing a semaphore for blit signaling while it's
    // still in use by presentation. By having one semaphore per image, we can
    // tie the semaphore signal/unsignal to the image acquire/present loop.
    // Alternatively, with the relatively new VK_EXT_swapchain_maintenance1,
    // you can use a fence to make sure the previous presentation is done before
    // trying to reuse the semaphore.
    // Note also that we only grow the list, never shrink. That's because we
    // can't destroy a semaphore while presentation is still using it. This we
    // really can't solve without a fence from VK_EXT_swapchain_maintenance1, so
    // we assume that the image count won't change (or not significantly). We'll
    // only destroy semaphores in the destructor after waiting for queue idle.
    while (_blitCompleteSemaphores.size() < imageCount) {
        _blitCompleteSemaphores.emplace_back();
        VkSemaphoreCreateInfo semaphoreCreateInfo{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        HGIVULKAN_VERIFY_VK_RESULT(
            vkCreateSemaphore(device, &semaphoreCreateInfo,
                HgiVulkanAllocator(), &_blitCompleteSemaphores.back()));
    }
}

void
HgiPresent2VulkanSurface::_DestroySwapchainResources(VkDevice device)
{
    // We're not using DestroyTexture() because we own the images,
    // know that they're safe to destroy now, and are destroying them
    // right after with vkDestroySwapchainKHR().
    for (const auto texture : _swapchainTextures) {
        delete texture.Get();
    }
    _swapchainTextures.clear();

    // Destroying the swapchain destroys its images
    vkDestroySwapchainKHR(device, _swapchain, HgiVulkanAllocator());
    _swapchain = nullptr;

    _validSwapchainResources = false;
}

bool
HgiPresent2VulkanSurface::IsColorFormatSupported(HgiFormat format) const
{
    return HgiIsFloatFormat(format) && !HgiIsCompressed(format);
}

bool
HgiPresent2VulkanSurface::IsDepthFormatSupported(HgiFormat format) const
{
    return  false;
}

std::optional<bool>
HgiPresent2VulkanSurface::IsValid() const
{
    if (!_hgiVulkan || !_surface) {
        return false;
    }

    if (!_imageCount) {
        return std::nullopt; // Not setup yet
    }

    return _validSurfaceProperties && _validSwapchainResources;
}

void
HgiPresent2VulkanSurface::UpdateParams(const HgiPresent2Params& params)
{
    if (_params == params.surface) {
        return;
    }

    _params = params.surface;
    _DestroySurfaceResources(
        _hgiVulkan->GetVulkanInstance()->GetVulkanInstance(),
        _hgiVulkan->GetPrimaryDevice()->GetVulkanDevice());
}

HgiPresent2AovSet::RgbaSwizzle
HgiPresent2VulkanSurface::GetRgbaSwizzle() const
{
    switch (_imageFormat.format) {
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SNORM:
        return {2, 1, 0, 3};
    default:
        return HgiPresent2AovSet::identityRgbaSwizzle;
    }
}

HgiTextureHandle
HgiPresent2VulkanSurface::Acquire(
    HgiCmds* hgiCommands, uint32_t width, uint32_t height)
{
    HgiVulkanCmds* commands = dynamic_cast<HgiVulkanCmds*>(hgiCommands);
    if (!commands) {
        TF_CODING_ERROR("commands must be a valid HgiVulkanCmds pointer");
        return {};
    }

    const auto hgiDevice = _hgiVulkan->GetPrimaryDevice();
    const auto device = hgiDevice->GetVulkanDevice();

    const GfVec2i srcTextureSize{
        static_cast<int>(width), static_cast<int>(height)};

    if (srcTextureSize != _srcTextureSize) {
        _srcTextureSize = srcTextureSize;
        _validSurfaceProperties = false;
        _DestroySwapchainResources(device);
    }

    _ResolveSurfaceProperties(hgiDevice->GetVulkanPhysicalDevice());
    _CreateSwapchainResources(device);
    if (!_validSurfaceProperties || !_validSwapchainResources) {
        return {};
    }

    if (const auto result = vkAcquireNextImageKHR(device, _swapchain,
            std::numeric_limits<uint64_t>::max(), _imageAcquiredSemaphore,
            nullptr, &_swapchainImageIndex);
        result == VK_ERROR_OUT_OF_DATE_KHR) {
        _validSurfaceProperties = false;
        _DestroySwapchainResources(device);
        return {};
    } else if (result == VK_ERROR_SURFACE_LOST_KHR) {
        _DestroySurfaceResources(
            _hgiVulkan->GetVulkanInstance()->GetVulkanInstance(), device);
    } else if (result != VK_SUBOPTIMAL_KHR) {
        HGIVULKAN_VERIFY_VK_RESULT(result);
    }

    const auto textureHandle = _swapchainTextures[_swapchainImageIndex];
    const auto texture = dynamic_cast<HgiVulkanTexture*>(textureHandle.Get());
    TF_VERIFY(texture);
    texture->DiscardContents(commands->GetCommandBuffer());

    return textureHandle;
}

void
HgiPresent2VulkanSurface::SubmitAndPresent(std::unique_ptr<HgiCmds> hgiCommands)
{
    std::unique_ptr<HgiVulkanCmds> commands;
    if (dynamic_cast<HgiVulkanCmds*>(hgiCommands.get())) {
        commands = std::unique_ptr<HgiVulkanCmds>{
            dynamic_cast<HgiVulkanCmds*>(hgiCommands.release())};
    } else {
        TF_CODING_ERROR("commands must be a valid HgiVulkanCmds pointer");
        return;
    }

    const auto hgiDevice = _hgiVulkan->GetPrimaryDevice();
    const auto device = hgiDevice->GetVulkanDevice();

    const auto hgiQueue = hgiDevice->GetCommandQueue();
    const auto queue = hgiQueue->GetVulkanGraphicsQueue();

    const auto texture = dynamic_cast<HgiVulkanTexture*>(
        _swapchainTextures[_swapchainImageIndex].Get());
    TF_VERIFY(texture);

    const auto commandBuffer = commands->GetCommandBuffer();

    HgiVulkanTexture::TransitionImageBarrier(commandBuffer, texture,
        texture->GetImageLayout(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, 0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    hgiQueue->SubmitToQueue(commandBuffer, HgiSubmitWaitTypeNoWait,
        _imageAcquiredSemaphore,
        _blitCompleteSemaphores[_swapchainImageIndex]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores =
        &_blitCompleteSemaphores[_swapchainImageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &_swapchainImageIndex;

    if (const auto result = vkQueuePresentKHR(queue, &presentInfo);
        result == VK_ERROR_OUT_OF_DATE_KHR) {
        _validSurfaceProperties = false;
        _DestroySwapchainResources(device);
    } else if (result == VK_ERROR_SURFACE_LOST_KHR) {
        _DestroySurfaceResources(
            _hgiVulkan->GetVulkanInstance()->GetVulkanInstance(), device);
    } else if (result != VK_SUBOPTIMAL_KHR) {
        HGIVULKAN_VERIFY_VK_RESULT(result);
    }

    // We need to force a sync here because we don't have the synchronization
    // mechanism to prevent the AOV from being reused before the blit is
    // finished. We use ResetConsumedCommandBuffers() with a wait specifically
    // so any command buffer completion handlers are called before we return.
    // This mostly matters for testing support.
    hgiQueue->ResetConsumedCommandBuffers(HgiSubmitWaitTypeWaitUntilCompleted);
}


PXR_NAMESPACE_CLOSE_SCOPE
