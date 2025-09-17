//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrmanXpuCpuLoader/rendererPlugin.h"

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
    HdRendererPluginRegistry::Define<HdPrmanXpuCpuLoaderRendererPlugin>();
}

HdPrmanXpuCpuLoaderRendererPlugin::HdPrmanXpuCpuLoaderRendererPlugin()
    : HdPrmanLoaderRendererPlugin()
{
    m_renderVariant = HdPrmanLoaderTokens->xpu;
    m_xpuCpuConfig = 1; // enable cpu
    m_xpuGpuConfig.push_back(0); // use first gpu
}

HdPrmanXpuCpuLoaderRendererPlugin::~HdPrmanXpuCpuLoaderRendererPlugin()
{
}

TfToken HdPrmanXpuCpuLoaderRendererPlugin::_GetRenderVariant()
{
    return HdPrmanLoaderTokens->xpu;
}

int HdPrmanXpuCpuLoaderRendererPlugin::_GetCpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    return 1;
}

std::vector<int> HdPrmanXpuCpuLoaderRendererPlugin::_GetGpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    return std::vector<int>();
}

PXR_NAMESPACE_CLOSE_SCOPE