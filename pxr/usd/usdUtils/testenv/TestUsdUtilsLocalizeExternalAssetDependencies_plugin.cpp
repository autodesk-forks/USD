//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/usdaFileFormat.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <fstream>
#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define EXTERNAL_ASSET_FILE_FORMAT_TOKENS \
    ((Id,      "testformat"))             \
    ((Version, "1.0"))                    \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(ExternalAssetFileFormatTokens, 
    EXTERNAL_ASSET_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(ExternalAssetFileFormat);

// This is a simple file format which is used to test External Asset
// Dependencies.  An example of such files would be .mtl files that accompany
// an OBJ file or a .glb file which accompanies a glTF file.
class ExternalAssetFileFormat : public SdfFileFormat {
public:

    // SdfFileFormat overrides.
    virtual bool CanRead(const std::string &file) const override;
    virtual bool Read(SdfLayer* layer,
                      const std::string& resolvedPath,
                      bool metadataOnly) const override;

    virtual std::set<std::string> GetExternalAssetDependencies(
        const SdfLayer& layer) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~ExternalAssetFileFormat() = default;
    ExternalAssetFileFormat();
};

TF_DEFINE_PUBLIC_TOKENS(
    ExternalAssetFileFormatTokens, 
    EXTERNAL_ASSET_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(ExternalAssetFileFormat, SdfFileFormat);
}

ExternalAssetFileFormat::ExternalAssetFileFormat()
    : SdfFileFormat(
        ExternalAssetFileFormatTokens->Id,
        ExternalAssetFileFormatTokens->Version,
        ExternalAssetFileFormatTokens->Target,
        ExternalAssetFileFormatTokens->Id)
{
}

bool
ExternalAssetFileFormat::CanRead(const std::string& filePath) const
{
    return TfGetExtension(filePath) == "testformat";
}

bool
ExternalAssetFileFormat::Read(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    return true;
}

// Simple approach to generating a list of external asset dependencies.
// Return path to all files which have the `.external` extension in the same,
// child directory or the parent layer as this layer.
std::set<std::string> ExternalAssetFileFormat::GetExternalAssetDependencies(
        const SdfLayer& layer) const
{
    const std::string layerDir = TfGetPathName(layer.GetResolvedPath());
    const std::string parentDir = TfGetPathName(layerDir.substr(0, layerDir.size() - 1));

    std::set<std::string> results;

    std::vector<std::string> files = TfGlob({
        TfStringCatPaths(parentDir, "*.external"),
        TfStringCatPaths(layerDir, "*.external"),
        TfStringCatPaths(layerDir, "**/*.external")
    }, ARCH_GLOB_MARK);
    
    for (const auto& file : files) {
        auto temp = TfNormPath(file);
        results.insert(TfNormPath(file));
    }

    // this is a special testing case for external assets with a name the 
    // same as the layer but without an extension.  It is meant to exercise
    // downstream packaging of such files.
    const std::string specialAssetName = TfStringCatPaths(layerDir, 
        TfStringGetBeforeSuffix(TfGetBaseName(layer.GetResolvedPath())));

    if (TfPathExists(specialAssetName)) {
        results.insert(TfNormPath(specialAssetName));
    }

    return results;
}

PXR_NAMESPACE_CLOSE_SCOPE

