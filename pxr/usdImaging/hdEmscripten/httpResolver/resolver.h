#pragma once

// IMPORT THIRD-PARTY LIBRARIES
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/defineResolver.h>
#include <emscripten/fetch.h>
#include <emscripten.h>

PXR_NAMESPACE_OPEN_SCOPE

class HttpResolver : public ArDefaultResolver {
public:
    HttpResolver();
    ~HttpResolver();

    ArResolvedPath _Resolve(const std::string& path) const override;
    std::shared_ptr<ArAsset> _OpenAsset(const ArResolvedPath &resolvedPath) const override;
    ArResolvedPath _ResolveForNewAsset(const std::string &assetPath) const override;
    std::filesystem::path FetchAndSaveAsset(const std::string& baseUrl, const std::string& filePath) const;
    void saveBinaryAssetContentToFile(const char* assetContent, size_t length, const std::string& filePath) const;
    void setBaseUrl(const std::string &url) const;
    void setBaseTempDir(const std::string &tempDir) const;
private:
    mutable std::string baseUrl;
    bool verbose = false;
    mutable std::string baseTempDir;
};

PXR_NAMESPACE_CLOSE_SCOPE
