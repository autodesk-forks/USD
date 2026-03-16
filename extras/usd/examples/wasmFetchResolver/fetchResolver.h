//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/writableAsset.h"
#include "pxr/base/vt/value.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// This class provides an example implementation of how the Emscripten Fetch
// API can be used to load a stage from layers located on a web server.
class FetchResolver final
    : public PXR_NS::ArResolver
{
public:
    FetchResolver() = default;
    virtual ~FetchResolver() = default;

protected:
    std::string _CreateIdentifier(
        const std::string& assetPath,
        const PXR_NS::ArResolvedPath& anchorAssetPath) const final;

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final;

    // Attempts to resolve the path by fetching the corresponding file from
    // the server.
    ArResolvedPath _Resolve(
        const std::string& assetPath) const final;

    // Currently this resolver is read only. This method will always return an
    // empty asset path.
    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const final;

    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const final;

    // Currently this resolver is read only. This method will always
    // return nullptr.
    std::shared_ptr<ArWritableAsset>
    _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final;

private:
    mutable std::mutex _mutex;
    mutable std::set<std::string> _downloads;
    mutable std::condition_variable _condition;
};
