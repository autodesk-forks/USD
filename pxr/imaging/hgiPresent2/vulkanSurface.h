//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_VULKANSURFACE_H
#define PXR_IMAGING_HGIPRESENT2_VULKANSURFACE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/smallVector.h"

#include "pxr/imaging/hgiVulkan/vulkan.h"

#include "pxr/imaging/hgiPresent2/aovSet.h"
#include "pxr/imaging/hgiPresent2/api.h"
#include "pxr/imaging/hgiPresent2/present.h"


PXR_NAMESPACE_OPEN_SCOPE


class HgiVulkan;

/// \class HgiPresent2VulkanSurface
///
/// An implementation of HgiPresent2AovSet using a VkSurfaceKHR.
///
class HgiPresent2VulkanSurface final : public HgiPresent2AovSet
{
public:
    /// Create an HgiPresent2AovSet which sources AOVs from a VkSurfaceKHR
    /// surface swap-chain.
    HGIPRESENT2_API
    explicit HgiPresent2VulkanSurface(HgiVulkan* hgi, VkSurfaceKHR surface);

    HGIPRESENT2_API
    ~HgiPresent2VulkanSurface() override;

    HGIPRESENT2_API
    bool IsColorFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    bool IsDepthFormatSupported(HgiFormat format) const override;

    HGIPRESENT2_API
    std::optional<bool> IsValid() const override;

    HGIPRESENT2_API
    void UpdateParams(const HgiPresent2Params& params) override;

    HGIPRESENT2_API
    RgbaSwizzle GetRgbaSwizzle() const override;

    HGIPRESENT2_API
    HgiTextureHandle Acquire(
        HgiCmds* blitCmds, uint32_t width, uint32_t height) override;

    HGIPRESENT2_API
    void SubmitAndPresent(std::unique_ptr<HgiCmds> commands) override;

private:
    void _DestroySurfaceResources(VkInstance instance, VkDevice device);

    void _ResolveSurfaceProperties(VkPhysicalDevice physicalDevice);

    void _CreateSwapchainResources(VkDevice device);

    void _DestroySwapchainResources(VkDevice device);

    HgiVulkan* _hgiVulkan{};
    HgiPresent2SurfaceParams _params{};

    VkSurfaceKHR _surface{};


    GfVec2i _srcTextureSize{};

    uint32_t _imageCount{};
    VkCompositeAlphaFlagBitsKHR _alphaComposition{};
    VkPresentModeKHR _presentMode{};
    VkExtent2D _swapchainExtent{};
    VkSurfaceFormatKHR _imageFormat{};
    VkImageUsageFlags _imageUsageFlags{};
    bool _validSurfaceProperties{false};

    VkSwapchainKHR _swapchain{};
    TfSmallVector<HgiTextureHandle, 4> _swapchainTextures;
    VkSemaphore _imageAcquiredSemaphore{};
    TfSmallVector<VkSemaphore, 4> _blitCompleteSemaphores{};
    bool _validSwapchainResources{false};

    uint32_t _swapchainImageIndex{};
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
