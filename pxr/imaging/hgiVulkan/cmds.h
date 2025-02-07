//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_CMDS_H
#define PXR_IMAGING_HGIVULKAN_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgi/cmds.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanCommandBuffer;

/// \class HgiVulkanCmds
///
/// Vulkan subinterface of HgiCmds.
///
/// TODO: this could be refactored to contain the command code between
/// HgiVulkanGraphicsCmds, HgiVulkanBlitCmds and HgiVulkanComputeCmds.
/// Lots of duplicated code for commmand buffer management.
///
class ARCH_EXPORT_TYPE HgiVulkanCmds : public virtual HgiCmds
{
public:
    HGIVULKAN_API
    virtual ~HgiVulkanCmds() = default;

    /// Returns the command buffer used inside this cmds.
    HGIVULKAN_API
    virtual HgiVulkanCommandBuffer* GetCommandBuffer() = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
