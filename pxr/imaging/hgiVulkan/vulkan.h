//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_VULKAN_H
#define PXR_IMAGING_HGIVULKAN_VULKAN_H

#include "pxr/base/arch/defines.h"

#if defined(ARCH_OS_OSX)
    // Needed for VK_KHR_portability_subset
    #define VK_ENABLE_BETA_EXTENSIONS 1
#endif

#include <vulkan/vulkan.h>

#if defined(ARCH_OS_WINDOWS)
    #define PXR_VK_EXTERNAL_MEMORY_HANDLE \
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#elif defined(PXR_X11_SUPPORT_ENABLED)
    #define PXR_VK_EXTERNAL_MEMORY_HANDLE \
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#else
    #define PXR_VK_EXTERNAL_MEMORY_HANDLE 0
#endif

#include "pxr/imaging/hgiVulkan/vk_mem_alloc.h"

// Use the default allocator (nullptr)
inline VkAllocationCallbacks*
HgiVulkanAllocator() {
    return nullptr;
}

#endif
