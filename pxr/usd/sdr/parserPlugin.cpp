//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/sdr/parserPlugin.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(SDR_WARN_UNIMPLEMENTED_GET_SHADING_SYSTEM, true,
    "By default, warns if SdrParserPlugin::GetShadingSystem is not "
    "implemented in SdrParserPlugin subclasses. One warning is "
    "emitted for all subclasses. When set to false, no warning is "
    "emitted.");

// Register this plugin type with Tf
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdrParserPlugin>();
}

SdrParserPlugin::SdrParserPlugin()
{
    // nothing yet
}

SdrParserPlugin::~SdrParserPlugin()
{
    // nothing yet
}

const TfToken&
SdrParserPlugin::GetShadingSystem() const
{
    static bool warned = false;
    if (!warned) {
        const bool warn =
            TfGetEnvSetting(SDR_WARN_UNIMPLEMENTED_GET_SHADING_SYSTEM);
        if (warn) {
            const TfType& derivedType = TfType::FindByTypeid(typeid(*this));
            TF_WARN("%s::GetSourceType is deprecated in favor of "
                    "GetShadingSystem. In a future release, unimplemented "
                    "GetShadingSystem will cause a build error.",
                    derivedType.GetTypeName().c_str());
        }
        warned = true;
    }
    return GetSourceType();
}

const TfToken&
SdrParserPlugin::GetSourceType() const
{
    static const TfToken empty;
    return empty;
}

SdrShaderNodeUniquePtr
SdrParserPlugin::GetInvalidShaderNode(const SdrShaderNodeDiscoveryResult& dr)
{
    // Although the discovery result's "discovery type" could be used as the
    // node's type, that would expose an internal type that is not intended to
    // be visible to the outside. Instead, just use the generic "unknown" type.
    return SdrShaderNodeUniquePtr(
        new SdrShaderNode(
            dr.identifier,
            dr.version,
            dr.name,
            dr.function,
            TfToken("unknown discovery type"),
            TfToken("unknown shading system"),
            dr.resolvedUri,
            dr.resolvedUri,
            /* properties = */ SdrShaderPropertyUniquePtrVec()
        )
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
