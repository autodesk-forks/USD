// IMPORT THIRD-PARTY LIBRARIES
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/defineResolver.h>
#include <iostream>
#include <filesystem>
#include <fstream>

// IMPORT LOCAL LIBRARIES
#include "resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(HttpResolver, ArDefaultResolver);

HttpResolver::HttpResolver() : ArDefaultResolver() {}
HttpResolver::~HttpResolver() {}

struct AssetData {
    int ptrToContent;
    int length; // Use int for compatibility with JavaScript's setValue; adjust if necessary
};

EM_ASYNC_JS(void, fetch_asset, (const char* route, int dataPtr), {
    const routeString = UTF8ToString(route);
    const absoluteUrl = new URL(routeString);
    try {
        const response = await fetch(absoluteUrl);
        if (!response.ok) throw new Error('Fetch failed: ' + response.statusText);
        const buffer = await response.arrayBuffer();
        const length = buffer.byteLength;
        const ptr = _malloc(length);
        HEAPU8.set(new Uint8Array(buffer), ptr);

        // Correctly set the pointer and length in the AssetData structure
        // Note: Assumes dataPtr is a pointer to the structure where the first member is an int pointer
        // to the content, and the second is an int for the length.
        Module.HEAP32[dataPtr >> 2] = ptr; // Set the pointer
        Module.HEAP32[(dataPtr >> 2) + 1] = length; // Set the length
    } catch (err) {
        console.error("Error in fetch_asset: ", err);
        Module.HEAP32[dataPtr >> 2] = 0; // Indicate failure with null pointer
        Module.HEAP32[(dataPtr >> 2) + 1] = 0; // and zero length
    }
});

EM_JS(void, addToLoadedFiles, (const char* path), {
    if (typeof loadedFiles === 'undefined'){
        var loadedFiles = [];
    }
    loadedFiles.push(UTF8ToString(path));
});

std::filesystem::path HttpResolver::FetchAndSaveAsset(const std::string& route,
                                                   const std::string& filePath) const {
    try {
        std::filesystem::path dirPath = std::filesystem::path(filePath).parent_path();

        // Attempt to create the directory (and any necessary parent directories)
        if (std::filesystem::create_directories(dirPath)) {
            if (verbose){
                std::cout << "Directories created successfully: " << dirPath << std::endl;
            }
        } else {
            if (verbose){
                std::cout << "Directories already exist or cannot be created.\n";
            }
        }

        AssetData* data = new AssetData();
        fetch_asset(route.c_str(), reinterpret_cast<int>(data));
        char *assetContentCString = reinterpret_cast<char *>(data->ptrToContent);
        saveBinaryAssetContentToFile(assetContentCString, data->length, filePath);

        free(reinterpret_cast<void*>(data->ptrToContent));
        delete data;

    }
    catch (const std::exception& e){
        std::cout << "Error: " << e.what() << std::endl;
        return filePath;
    }

    addToLoadedFiles(filePath.c_str());
    return filePath;
}

void HttpResolver::saveBinaryAssetContentToFile(const char* assetContent, size_t length, const std::string& filePath) const {

    std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
    if (outFile) {
        // Write the binary content directly to the file
        outFile.write(assetContent, length);
        outFile.close();
        if (verbose) {
            std::cout << "File written successfully." << std::endl;
        }
    } else {
        if (verbose) {
            std::cout << "Failed to open file for writing." << std::endl;
        }
    }
}

void HttpResolver::setBaseUrl(const std::string& url) const {
    baseUrl = url;
}

void HttpResolver::setBaseTempDir(const std::string& tempDir) const {
    baseTempDir = tempDir;
}

std::string combineUrl(const std::string& baseUrl, const std::string& relativePath) {
    // Step 1: Strip off the scheme
    auto schemeEnd = baseUrl.find(":/");
    if (schemeEnd == std::string::npos) {
        return baseUrl + relativePath;
    }
    std::string scheme = baseUrl.substr(0, schemeEnd + 3); // Include "://"
    std::string basePath = baseUrl.substr(schemeEnd + 3);

    // Extract the domain
    auto pathStart = basePath.find('/');
    std::string domain = basePath.substr(0, pathStart);
    std::string pathOnly = basePath.substr(pathStart); // Path without the domain

    // Step 2: Use filesystem::path for manipulation
    std::filesystem::path pathObj = pathOnly;
    pathObj = pathObj.remove_filename(); // Ensure we're manipulating the directory part
    pathObj /= relativePath; // Append the relative path
    pathObj = pathObj.lexically_normal(); // Normalize the path (resolve "..", ".", etc.)

    // Step 3: Recombine
    std::string combinedUrl = scheme + domain + pathObj.string();

    return combinedUrl;
}

ArResolvedPath HttpResolver::_Resolve(const std::string& assetPath) const {
    if (verbose){
        std::cout << "_Resolve: " << assetPath << std::endl;
    }
    std::string stringAssetPathCopy = assetPath;
    std::filesystem::path savedAssetFilePath = assetPath;
    if (std::filesystem::exists(assetPath)){
        if (verbose) {
            std::cout << "Already Exists: " << assetPath << std::endl;
        }
    }
    else if (assetPath.find("http") != std::string::npos){
        std::string githubName = "github.com";
        std::string rawGithubName = "raw.githubusercontent.com";
        std::string blob = "/blob";

        size_t pos = stringAssetPathCopy.find(githubName);
        if (pos!= std::string::npos) {
            stringAssetPathCopy.replace(pos, githubName.length(), rawGithubName);
        }

        size_t pos_blob = stringAssetPathCopy.find(blob);
        if (pos_blob!= std::string::npos) {
            stringAssetPathCopy.erase(pos_blob, blob.length());
        }

        std::filesystem::path fullHttpRouteAsPath = stringAssetPathCopy;
        std::filesystem::path rootHttpRouteAsPath = fullHttpRouteAsPath.parent_path();

        auto finalBaseUrl = rootHttpRouteAsPath.generic_string() + "/";
        if (verbose){
            std::cout << "http PATH: " << stringAssetPathCopy << std::endl;
            std::cout << "finalBaseUrl: " << finalBaseUrl << std::endl;
        }

        setBaseUrl(finalBaseUrl);

        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        // This path is chosen because if an asset is found with the path /../../../ it will go up the tmp directory structure
        // in the case of using /tmp/ then all relative paths greater than depth 1, will look the same. using 6 here is arbitrary,
        // is there a way to make this always work?
        setBaseTempDir(tempDir.generic_string() + "/1/1/1/1/1/1/");
        auto filePath = baseTempDir + fullHttpRouteAsPath.filename().generic_string();
        savedAssetFilePath = FetchAndSaveAsset(assetPath,
                                               filePath);
    }
    else if (!baseUrl.empty()){
        std::filesystem::path systemPath = stringAssetPathCopy;
        std::filesystem::path relativePath = std::filesystem::relative(systemPath, baseTempDir);

        std::string route = combineUrl(baseUrl, relativePath);
        if (verbose){
            std::cout << "Relative Path before: " << relativePath << std::endl;
        }

        savedAssetFilePath = FetchAndSaveAsset(route, systemPath);
        if (verbose){
            std::cout << "Assumed to exist now, trying from baseUrl: " << systemPath << std::endl;
        }
    }
    else {
        return ArDefaultResolver::_Resolve(assetPath);
    }

    if (verbose){
        std::cout << "ENDDD_Resolve: " << savedAssetFilePath << std::endl;
    }

    return ArResolvedPath(savedAssetFilePath);
}

std::shared_ptr<ArAsset> HttpResolver::_OpenAsset(const ArResolvedPath &resolvedPath) const {
    if (verbose){
        std::cout << "_OpenAsset: " << resolvedPath.GetPathString() << std::endl;
    }

    return ArDefaultResolver::_OpenAsset(resolvedPath);
}

ArResolvedPath HttpResolver::_ResolveForNewAsset(const std::string &assetPath) const {
    if (verbose){
        std::cout << "Resolve for new asset" << std::endl;
    }

    return ArDefaultResolver::_ResolveForNewAsset(assetPath);
}

PXR_NAMESPACE_CLOSE_SCOPE
