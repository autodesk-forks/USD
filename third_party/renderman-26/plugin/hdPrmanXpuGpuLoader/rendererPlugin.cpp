//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrmanXpuGpuLoader/rendererPlugin.h"

#include "pxr/base/arch/library.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdPrmanXpuGpuLoaderRendererPlugin>();
}

HdPrmanXpuGpuLoaderRendererPlugin::HdPrmanXpuGpuLoaderRendererPlugin()
    : HdPrmanLoaderRendererPlugin()
{
    m_renderVariant = HdPrmanLoaderTokens->xpu;
    m_xpuCpuConfig = 1; // enable cpu
    m_xpuGpuConfig.push_back(0); // use first gpu
}

HdPrmanXpuGpuLoaderRendererPlugin::~HdPrmanXpuGpuLoaderRendererPlugin()
{
}

TfToken HdPrmanXpuGpuLoaderRendererPlugin::_GetRenderVariant()
{
    return HdPrmanLoaderTokens->xpu;
}

int HdPrmanXpuGpuLoaderRendererPlugin::_GetCpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    return 0;
}

std::vector<int> HdPrmanXpuGpuLoaderRendererPlugin::_GetGpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    std::vector<int> xpuGpuConfig 
        = HdPrmanLoaderRendererPlugin::_GetGpuConfig(settingsMap);
    return (xpuGpuConfig.empty()) ? {0} : xpuGpuConfig;
}

PXR_NAMESPACE_CLOSE_SCOPE