//`
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_BASE_TF_FAST_COMPRESSION_H
#define PXR_BASE_TF_FAST_COMPRESSION_H

/// \file tf/fastCompression.h
/// Simple fast data compression/decompression routines.

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

/// Fast LZ4-based compression with a small custom framing layer.
///
/// The compressed buffer begins with a leading byte:
/// - \c 0 — a single LZ4 block follows (no per-chunk size prefix).
/// - \c N (1–127) — \c N chunks; each chunk is
///   <tt>[int32_t compressedSize][LZ4 payload]</tt>.
///
/// Data produced by CompressToBuffer() can be decompressed only with
/// DecompressFromBuffer().  The maximum logical input size is
/// GetMaxInputSize().
class TfFastCompression
{
public:
    /// Return the largest input buffer size that can be compressed with these
    /// functions.  Guaranteed to be at least 200 GB.
    TF_API static size_t
    GetMaxInputSize();
    
    /// Return the largest possible compressed size for the given \p inputSize
    /// in the worst case (input is not compressible).  This is larger than
    /// \p inputSize.  Returns 0 if \p inputSize is larger than
    /// GetMaxInputSize().
    TF_API static size_t
    GetCompressedBufferSize(size_t inputSize);

    /// Compress \p inputSize bytes in \p input and store the result in
    /// \p compressed.  The \p compressed buffer must point to at least
    /// GetCompressedBufferSize(\p inputSize) bytes.  Returns the number of
    /// bytes written to \p compressed.  If \p inputSize exceeds
    /// GetMaxInputSize(), posts \c TF_CODING_ERROR and returns 0.
    TF_API static size_t
    CompressToBuffer(char const *input, char *compressed, size_t inputSize);
                           
    /// Decompress \p compressedSize bytes from \p compressed into \p output.
    ///
    /// On success, returns the number of bytes written to \p output (which may
    /// be 0 when \p maxOutputSize is 0 or the payload decompresses to an empty
    /// result).  No more than \p maxOutputSize bytes are written.
    ///
    /// On failure, posts \c TF_RUNTIME_ERROR, returns 0, and leaves \p output
    /// in an unspecified state.  Callers must not treat a return value of 0
    /// alone as failure; consult a TfErrorMark or compare the return value to
    /// a known expected decompressed size when one is available.
    ///
    /// For buffers produced by CompressToBuffer(), the decompressed size equals
    /// the original \p inputSize passed to compression.
    ///
    /// \p compressedSize must be greater than zero.
    ///
    /// \note Typical success check when the expected size is known:
    /// \code
    /// TfErrorMark m;
    /// size_t n = DecompressFromBuffer(comp, out, compSize, expectedSize);
    /// if (!m.IsClean() || n != expectedSize) { /* handle error */ }
    /// \endcode
    TF_API static size_t
    DecompressFromBuffer(char const *compressed, char *output,
                         size_t compressedSize, size_t maxOutputSize);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_FAST_COMPRESSION_H


