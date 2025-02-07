//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiPresent2/testenv/testWindowHandle.h"

#if defined(PXR_METAL_SUPPORT_ENABLED)
#include "pxr/imaging/hgiPresent2/metalSurface.h"
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#if defined(ARCH_OS_OSX)
#include <vulkan/vulkan_metal.h>
#endif
#if defined(PXR_X11_SUPPORT_ENABLED)
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif
#endif

namespace {
PXR_NAMESPACE_USING_DIRECTIVE

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
bool
IsPresentationSupported(VkPhysicalDevice device, uint32_t queueFamilyIndex,
    HgiPresent2TestWindowHandle window)
{
    return std::visit([=](const auto handle) -> bool
    {
        using Handle = std::decay_t<decltype(handle)>;
#if defined(ARCH_OS_OSX)
        if constexpr (std::is_same_v<Handle, HgiPresent2TestMetalWindowHandle>) {
            return true;
        } else
#endif
#if defined(PXR_X11_SUPPORT_ENABLED)
        if constexpr (std::is_same_v<Handle, HgiPresent2TestXlibWindowHandle>) {
            XWindowAttributes windowAttributes{};
            if (!XGetWindowAttributes(
                    handle.display, handle.window, &windowAttributes)) {
                return false;
            }
            return vkGetPhysicalDeviceXlibPresentationSupportKHR(device,
                queueFamilyIndex, handle.display,
                XVisualIDFromVisual(windowAttributes.visual));
        } else
#endif
        {
            TF_CODING_ERROR("Invalid window handle");
            return false;
        }
    }, window);
}

VkSurfaceKHR
CreateVkSurface(VkInstance instance, HgiPresent2TestWindowHandle window)
{
    return std::visit([=](const auto handle)
    {
        VkSurfaceKHR surface{};
        using Handle = std::decay_t<decltype(handle)>;
#if defined(ARCH_OS_OSX)
        if constexpr (std::is_same_v<Handle, HgiPresent2TestMetalWindowHandle>) {
            TF_STATUS("Using Metal WSI");
            VkMetalSurfaceCreateInfoEXT surfaceCreateInfo{};
            surfaceCreateInfo.sType =
                VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
            surfaceCreateInfo.pLayer = handle.layer;
            HGIVULKAN_VERIFY_VK_RESULT(vkCreateMetalSurfaceEXT(
                instance, &surfaceCreateInfo, HgiVulkanAllocator(), &surface));
        } else
#endif
#if defined(PXR_X11_SUPPORT_ENABLED)
        if constexpr (std::is_same_v<Handle, HgiPresent2TestXlibWindowHandle>) {
            TF_STATUS("Using Xlib WSI");
            VkXlibSurfaceCreateInfoKHR createSurfaceInfo{};
            createSurfaceInfo.sType =
                VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createSurfaceInfo.dpy = handle.display;
            createSurfaceInfo.window = handle.window;
            HGIVULKAN_VERIFY_VK_RESULT(vkCreateXlibSurfaceKHR(
                instance, &createSurfaceInfo, nullptr, &surface));
        } else
#endif
        {
            TF_CODING_ERROR("Invalid window handle");
        }

        return surface;
    }, window);
}
#endif
}

PXR_NAMESPACE_OPEN_SCOPE

HgiPresent2SurfaceHandle
HgiPresent2TestWindowHandleToSurfaceHandle(Hgi* hgi,
    HgiPresent2TestWindowHandle window)
{
#if defined(PXR_METAL_SUPPORT_ENABLED)
    if (HgiPresent2DynamicCastHgiMetal(hgi)) {
        if (const auto metalWindow =
            std::get_if<HgiPresent2TestMetalWindowHandle>(&window)) {
            return static_cast<CAMetalLayerPtr>(metalWindow->layer);
        }
    } else
#endif
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    if (const auto hgiVulkan = dynamic_cast<HgiVulkan*>(hgi)) {
        const auto hgiDevice = hgiVulkan->GetPrimaryDevice();
        if (IsPresentationSupported(hgiDevice->GetVulkanPhysicalDevice(),
            hgiDevice->GetGfxQueueFamilyIndex(), window)) {
            return CreateVkSurface(
                hgiVulkan->GetVulkanInstance()->GetVulkanInstance(), window);
        }
    } else
#endif
    {
    }

    TF_CODING_ERROR("Invalid window handle");
    return {};
}

void
HgiPresent2TestDestroySurfaceHandle(Hgi* hgi, HgiPresent2SurfaceHandle surface)
{
#if defined(PXR_METAL_SUPPORT_ENABLED)
    if (HgiPresent2DynamicCastHgiMetal(hgi)) {
        if (std::holds_alternative<CAMetalLayerPtr>(surface)) {
            // Nothing to do: released by destroying the window
            return;
        }
    } else
#endif
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    if (const auto hgiVulkan = dynamic_cast<HgiVulkan*>(hgi)) {
        if (const auto vkSurface =  std::get_if<VkSurfaceKHR>(&surface)) {
            vkDestroySurfaceKHR(
                hgiVulkan->GetVulkanInstance()->GetVulkanInstance(),
                *vkSurface, HgiVulkanAllocator());
            return;
        }
    } else
#endif
    {
    }

    TF_CODING_ERROR("Invalid surface handle");
}

PXR_NAMESPACE_CLOSE_SCOPE
