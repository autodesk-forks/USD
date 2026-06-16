//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/usd/sdf/crateInfo.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static std::vector<char>
_ReadFileBytes(std::string const &path)
{
    ArchConstFileMapping mapping = ArchMapFileReadOnly(path);
    TF_AXIOM(mapping);
    size_t const len = ArchGetFileMappingLength(mapping);
    std::vector<char> bytes(len);
    if (len != 0) {
        std::memcpy(bytes.data(), mapping.get(), len);
    }
    return bytes;
}

static void
_WriteFileBytes(std::string const &path, std::vector<char> const &bytes)
{
    FILE *file = ArchOpenFile(path.c_str(), "wb");
    TF_AXIOM(file);
    TF_AXIOM(std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size());
    std::fclose(file);
}

static bool
_FindSection(
    SdfCrateInfo const &info,
    char const *name,
    int64_t *start,
    int64_t *size)
{
    for (auto const &sec : info.GetSections()) {
        if (sec.name == name) {
            *start = sec.start;
            *size = sec.size;
            return true;
        }
    }
    return false;
}

static bool
_ExpectOpenFailure(std::string const &path)
{
    TfErrorMark m;
    SdfLayerRefPtr const layer = SdfLayer::FindOrOpen(path);
    TF_AXIOM(!layer);
    TF_AXIOM(!m.IsClean());
    return true;
}

static bool
_ExpectOpenSuccess(std::string const &path)
{
    TfErrorMark m;
    SdfLayerRefPtr const layer = SdfLayer::FindOrOpen(path);
    TF_AXIOM(layer);
    TF_AXIOM(m.IsClean());
    return true;
}

static std::string
_CreateMinimalUsdc()
{
    std::string const path =
        ArchMakeTmpFileName("testSdfCrateCorrupt_", ".usdc");
    SdfLayerRefPtr layer = SdfLayer::CreateNew(path);
    TF_AXIOM(layer);
    SdfPrimSpec::New(layer, "Root", SdfSpecifierDef);
    TF_AXIOM(layer->Save());
    layer.Reset();
    return path;
}

static bool
TestValidMinimalCrateOpens(std::string const &validPath)
{
    return _ExpectOpenSuccess(validPath);
}

static bool
TestCorruptTokensInvalidChunkCount(std::string const &validPath)
{
    SdfCrateInfo const info = SdfCrateInfo::Open(validPath);
    TF_AXIOM(info);

    int64_t secStart = 0;
    int64_t secSize = 0;
    TF_AXIOM(_FindSection(info, "TOKENS", &secStart, &secSize));

    // TOKENS (crate >= 0.4.0):
    // [numTokens u64][uncompressedSize u64][compressedSize u64][payload...]
    static size_t const kPayloadOffset = 24;
    TF_AXIOM(secSize >= static_cast<int64_t>(kPayloadOffset + 1));

    std::vector<char> bytes = _ReadFileBytes(validPath);
    TF_AXIOM(secStart + static_cast<int64_t>(kPayloadOffset) <
             static_cast<int64_t>(bytes.size()));

    // Invalid multi-chunk count for TfFastCompression.
    bytes[static_cast<size_t>(secStart + kPayloadOffset)] = 128;

    std::string const corruptPath =
        ArchMakeTmpFileName("testSdfCrateCorrupt_tokens_", ".usdc");
    _WriteFileBytes(corruptPath, bytes);

    bool const ok = _ExpectOpenFailure(corruptPath);
    ArchUnlinkFile(corruptPath.c_str());
    return ok;
}

static bool
TestCorruptTokensOversizedCompressedSize(std::string const &validPath)
{
    SdfCrateInfo const info = SdfCrateInfo::Open(validPath);
    TF_AXIOM(info);

    int64_t secStart = 0;
    int64_t secSize = 0;
    TF_AXIOM(_FindSection(info, "TOKENS", &secStart, &secSize));
    TF_AXIOM(secSize >= 24);

    std::vector<char> bytes = _ReadFileBytes(validPath);

    // Lie about compressed payload size in the TOKENS section header.
    static size_t const kCompressedSizeOffset = 16;
    uint64_t const badCompressedSize = 1ull << 40;
    std::memcpy(
        bytes.data() + static_cast<size_t>(secStart + kCompressedSizeOffset),
        &badCompressedSize,
        sizeof(badCompressedSize));

    std::string const corruptPath =
        ArchMakeTmpFileName("testSdfCrateCorrupt_tokensSize_", ".usdc");
    _WriteFileBytes(corruptPath, bytes);

    bool const ok = _ExpectOpenFailure(corruptPath);
    ArchUnlinkFile(corruptPath.c_str());
    return ok;
}

static bool
TestCorruptBootstrapTocPastEof(std::string const &validPath)
{
    std::vector<char> bytes = _ReadFileBytes(validPath);
    TF_AXIOM(bytes.size() >= 24);

    int64_t const badTocOffset =
        static_cast<int64_t>(bytes.size()) + 1000;
    std::memcpy(bytes.data() + 16, &badTocOffset, sizeof(badTocOffset));

    std::string const corruptPath =
        ArchMakeTmpFileName("testSdfCrateCorrupt_toc_", ".usdc");
    _WriteFileBytes(corruptPath, bytes);

    bool const ok = _ExpectOpenFailure(corruptPath);
    ArchUnlinkFile(corruptPath.c_str());
    return ok;
}

int
main(int argc, char** argv)
{
    std::string const validPath = _CreateMinimalUsdc();

    if (!TestValidMinimalCrateOpens(validPath)) {
        ArchUnlinkFile(validPath.c_str());
        return 1;
    }
    if (!TestCorruptTokensInvalidChunkCount(validPath)) {
        ArchUnlinkFile(validPath.c_str());
        return 1;
    }
    if (!TestCorruptTokensOversizedCompressedSize(validPath)) {
        ArchUnlinkFile(validPath.c_str());
        return 1;
    }
    if (!TestCorruptBootstrapTocPastEof(validPath)) {
        ArchUnlinkFile(validPath.c_str());
        return 1;
    }

    ArchUnlinkFile(validPath.c_str());

    printf("SUCCEEDED\n");
    return 0;
}
