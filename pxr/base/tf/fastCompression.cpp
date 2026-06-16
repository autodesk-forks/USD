//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fastCompression.h"

// XXX: Need to isolate symbols here?
#include "pxrLZ4/lz4.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_lz4;

size_t
TfFastCompression::GetMaxInputSize()
{
    return 127 * static_cast<size_t>(LZ4_MAX_INPUT_SIZE);
}

size_t
TfFastCompression::GetCompressedBufferSize(size_t inputSize)
{
    if (inputSize > GetMaxInputSize())
        return 0;
    
    // If it fits in one chunk then it's just the compress bound plus 1.
    if (inputSize <= LZ4_MAX_INPUT_SIZE) {
        return LZ4_compressBound(inputSize) + 1;
    }
    size_t nWholeChunks = inputSize / LZ4_MAX_INPUT_SIZE;
    size_t partChunkSz = inputSize % LZ4_MAX_INPUT_SIZE;
    size_t sz = 1 + nWholeChunks *
        (LZ4_compressBound(LZ4_MAX_INPUT_SIZE) + sizeof(int32_t));
    if (partChunkSz)
        sz += LZ4_compressBound(partChunkSz) + sizeof(int32_t);
    return sz;
}

size_t
TfFastCompression::CompressToBuffer(
    char const *input, char *compressed, size_t inputSize)
{
    if (inputSize > GetMaxInputSize()) {
        TF_CODING_ERROR("Attempted to compress a buffer of %zu bytes, "
                        "more than the maximum supported %zu",
                        inputSize, GetMaxInputSize());
        return 0;
    }
    
    // If it fits in one chunk, just do it.
    char const * const origCompressed = compressed;
    if (inputSize <= LZ4_MAX_INPUT_SIZE) {
        compressed[0] = 0; // < zero byte means one chunk.
        compressed += 1 + LZ4_compress_default(
            input, compressed + 1, inputSize,
            LZ4_compressBound(inputSize));
    } else {
        size_t nWholeChunks = inputSize / LZ4_MAX_INPUT_SIZE;
        size_t partChunkSz = inputSize % LZ4_MAX_INPUT_SIZE;
        *compressed++ = nWholeChunks + (partChunkSz ? 1 : 0);
        auto writeChunk = [](char const *&input, char *&output, size_t size) {
            char *o = output;
            output += sizeof(int32_t);
            int32_t n = LZ4_compress_default(
                input, output, size, LZ4_compressBound(size));
            std::memcpy(o, &n, sizeof(n));
            output += n;
            input += size;
        };
        for (size_t chunk = 0; chunk != nWholeChunks; ++chunk) {
            writeChunk(input, compressed, LZ4_MAX_INPUT_SIZE);
        }
        if (partChunkSz) {
            writeChunk(input, compressed, partChunkSz);
        }
    }

    return compressed - origCompressed;
}    

size_t
TfFastCompression::DecompressFromBuffer(
    char const *compressed, char *output,
    size_t compressedSize, size_t maxOutputSize)
{
    if (compressedSize == 0) {
        TF_RUNTIME_ERROR("Cannot decompress an empty compressed buffer");
        return 0;
    }

    size_t const maxMultiChunkPayloadChunks =
        GetMaxInputSize() / static_cast<size_t>(LZ4_MAX_INPUT_SIZE);

    // Leading byte: 0 selects one LZ4 block payload (below); otherwise it is
    // the number of LZ4 chunks following (see CompressToBuffer). At most one
    // chunk per LZ4_MAX_INPUT_SIZE-byte logical slice of GetMaxInputSize().
    // Interpret this octet as unsigned so values 128-255 are not negative when
    // plain char is signed.
    unsigned char const nChunksByte =
        static_cast<unsigned char>(*compressed++);

    if (nChunksByte != 0 &&
        static_cast<size_t>(nChunksByte) > maxMultiChunkPayloadChunks) {
        TF_RUNTIME_ERROR(
            "Failed to decompress multi-chunk data: invalid chunk count "
            "%u (maximum supported is %zu)",
            static_cast<unsigned int>(nChunksByte),
            maxMultiChunkPayloadChunks);
        return 0;
    }

    if (nChunksByte == 0) {
        // Just one.
        int nDecompressed = LZ4_decompress_safe(
            compressed, output, compressedSize-1, maxOutputSize);
        if (nDecompressed < 0) {
            TF_RUNTIME_ERROR("Failed to decompress data, possibly corrupt? "
                             "LZ4 error code: %d", nDecompressed);
            return 0;
        }
        return nDecompressed;
    } else {
        // -1 because the leading nChunksByte was already consumed above.
        char const * const compressedEnd = compressed + (compressedSize - 1);
        unsigned int const nChunks = nChunksByte;
        size_t totalDecompressed = 0;
        for (unsigned int i = 0; i != nChunks; ++i) {
            ptrdiff_t const remaining = compressedEnd - compressed;
            if (remaining < static_cast<ptrdiff_t>(sizeof(int32_t /* chunk size */))) {
                TF_RUNTIME_ERROR(
                    "Failed to decompress multi-chunk data: truncated "
                    "chunk size at chunk %u of %u",
                    i + 1, nChunks);
                return 0;
            }
            int32_t chunkSize = 0;
            std::memcpy(&chunkSize, compressed, sizeof(chunkSize));
            compressed += sizeof(chunkSize);
            if (chunkSize < 0) {
                TF_RUNTIME_ERROR(
                    "Failed to decompress multi-chunk data: invalid "
                    "negative compressed chunk size %d at chunk %u of %u",
                    chunkSize, i + 1, nChunks);
                return 0;
            }
            size_t const chunkPayloadSize = static_cast<size_t>(chunkSize);
            if (chunkPayloadSize > static_cast<size_t>(remaining - sizeof(int32_t))) {
                TF_RUNTIME_ERROR(
                    "Failed to decompress multi-chunk data: chunk %u of %u "
                    "claims %zu compressed bytes but only %td remain",
                    i + 1, nChunks, chunkPayloadSize,
                    remaining - static_cast<ptrdiff_t>(sizeof(int32_t)));
                return 0;
            }
            int nDecompressed = LZ4_decompress_safe(
                compressed, output, chunkSize,
                std::min<size_t>(LZ4_MAX_INPUT_SIZE, maxOutputSize));
            if (nDecompressed < 0) {
                TF_RUNTIME_ERROR("Failed to decompress data, possibly corrupt? "
                                 "LZ4 error code: %d", nDecompressed);
                return 0;
            }
            compressed += chunkPayloadSize;
            output += nDecompressed;
            maxOutputSize -= nDecompressed;
            totalDecompressed += nDecompressed;
        }
        return totalDecompressed;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
