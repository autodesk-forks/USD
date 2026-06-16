//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/fastCompression.h"

#include "pxr/base/arch/defines.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

static char values[] = { 'a', 'b', 'c', 'd' };

static bool
testDecompressExpectFailure(
    char const *compressed, size_t compressedSize,
    char *output, size_t maxOutputSize)
{
    TfErrorMark m;
    size_t const n = TfFastCompression::DecompressFromBuffer(
        compressed, output, compressedSize, maxOutputSize);
    TF_AXIOM(n == 0);
    TF_AXIOM(!m.IsClean());
    return true;
}

static bool
testDecompressEmptyBuffer()
{
    char out[1] = {};
    return testDecompressExpectFailure("", 0, out, sizeof(out));
}

static bool
testDecompressInvalidChunkCount128()
{
    // 128 exceeds the maximum chunk count of 127.
    unsigned char const buf[] = { 128, 0 };
    char out[64] = {};
    return testDecompressExpectFailure(
        reinterpret_cast<char const *>(buf), sizeof(buf), out, sizeof(out));
}

static bool
testDecompressInvalidChunkCount255()
{
    unsigned char const buf[] = { 255, 0 };
    char out[64] = {};
    return testDecompressExpectFailure(
        reinterpret_cast<char const *>(buf), sizeof(buf), out, sizeof(out));
}

static bool
testDecompressCorruptSingleChunkPayload()
{
    // Single-chunk header with invalid LZ4 payload bytes.
    unsigned char const buf[] = { 0, 0x01, 0x02, 0x03, 0x04, 0x05 };
    char out[64] = {};
    return testDecompressExpectFailure(
        reinterpret_cast<char const *>(buf), sizeof(buf), out, sizeof(out));
}

static bool
testRoundTrip(size_t sz)
{
    // Create some data to compress.
    std::unique_ptr<char []> src(new char[sz]);
    for (size_t i = 0; i != sz; ++i) { src[i] = values[(i ^ (i >> 3)) & 3]; }

    // Make a buffer to house compressed data.
    std::unique_ptr<char []> compressed(
        new char[TfFastCompression::GetCompressedBufferSize(sz)]);

    TfErrorMark m;
    
    // Compress.
    size_t compressedSize =
        TfFastCompression::CompressToBuffer(src.get(), compressed.get(), sz);
    printf("Compressed %zu bytes to %zu\n", sz, compressedSize);

    // Decompress.
    std::unique_ptr<char []> decomp(new char[sz]);
    size_t decompressedSize = TfFastCompression::DecompressFromBuffer(
        compressed.get(), decomp.get(), compressedSize, sz);

    printf("Decompressed %zu bytes to %zu\n", compressedSize, decompressedSize);
    TF_AXIOM(sz == decompressedSize);

    // Validate equality.
    TF_AXIOM(std::equal(src.get(), src.get() + sz, decomp.get()));

    return m.IsClean();
}

static bool
testMultiChunkTruncatedFirstChunkSizePrefix()
{
    // Two chunks declared, but only three bytes follow the header: not enough
    // to read the first chunk's int32 (4 bytes) size field (no LZ4 call is made).
    // Index:     [0]  [1]  [2]  [3]
    // Byte:       2    0    0    0
    //             │    └────────────── only 3 bytes after header
    //             └── nChunksByte = 2
    // [chunk count: 1 byte][chunk 1: int32 size + LZ4 payload][chunk 2: int32 size + LZ4 payload]...
    unsigned char const buf[] = { 2, 0, 0, 0 };
    char out[64] = {};
    return testDecompressExpectFailure(
        reinterpret_cast<char const *>(buf), sizeof(buf), out, sizeof(out));
}

static bool
testMultiChunkTruncatedSecondChunkSizePrefix()
{
    // Two chunks declared. The first chunk has compressed size 0 (valid) and
    // consumes its size prefix only; the buffer ends before the second chunk's
    // int32 size field can be read.
    unsigned char const buf[] = { 2, 0, 0, 0, 0 };
    char out[64] = {};
    return testDecompressExpectFailure(
        reinterpret_cast<char const *>(buf), sizeof(buf), out, sizeof(out));
}

static bool
testMultiChunkOversizedPayloadClaim()
{
    // One chunk whose size field exceeds the remaining bytes.
    unsigned char const buf[] = { 1, 8, 0, 0, 0 };
    char out[64] = {};
    return testDecompressExpectFailure(
        reinterpret_cast<char const *>(buf), sizeof(buf), out, sizeof(out));
}

static bool
testMultiChunkNegativePayloadSize()
{
    unsigned char const buf[] = { 1, 0xff, 0xff, 0xff, 0xff };
    char out[64] = {};
    return testDecompressExpectFailure(
        reinterpret_cast<char const *>(buf), sizeof(buf), out, sizeof(out));
}

static bool
Test_TfFastCompression()
{
    if (!testDecompressEmptyBuffer()) {
        return false;
    }
    if (!testDecompressInvalidChunkCount128()) {
        return false;
    }
    if (!testDecompressInvalidChunkCount255()) {
        return false;
    }
    if (!testDecompressCorruptSingleChunkPayload()) {
        return false;
    }
    if (!testMultiChunkTruncatedFirstChunkSizePrefix()) {
        return false;
    }
    if (!testMultiChunkTruncatedSecondChunkSizePrefix()) {
        return false;
    }
    if (!testMultiChunkOversizedPayloadClaim()) {
        return false;
    }
    if (!testMultiChunkNegativePayloadSize()) {
        return false;
    }

    size_t sizes[] = {
        0,
        3,                   3 + 2,
        3*1024,              3*1024 + 2267,
        3*1024*1024,         3*1024*1024 + 514229,
        7*1024*1024,         7*1024*1024 + 514229,
        2008*1024*1024,      2008*1024*1024 + 514229,
        3*1024*1024*1024ull, 3*1024*1024*1024ull + 178656871
    };

    for (auto sz: sizes) {
        if (!testRoundTrip(sz)) {
            TF_FATAL_ERROR("Failed to (de)compress size %s\n",
                           TfStringify(sz).c_str());
        }
    }
    return true;
}

TF_ADD_REGTEST(TfFastCompression);
