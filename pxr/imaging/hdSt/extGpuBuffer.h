//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_H
#define PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hgi/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStExtGpuBuffer
///
/// A non-owning HgiBuffer wrapper around an externally-owned GPU buffer.
/// This allows Storm's draw dispatch to bind a buffer that was allocated
/// by producer without copying.
///
/// The destructor does NOT free the underlying GPU resource.
///
class HdStExtGpuBuffer final : public HgiBuffer
{
public:
    HDST_API
    HdStExtGpuBuffer(uint64_t rawHandle, size_t byteSize);

    HDST_API
    ~HdStExtGpuBuffer() override;

    HDST_API
    size_t GetByteSizeOfResource() const override;

    HDST_API
    uint64_t GetRawResource() const override;

    HDST_API
    void *GetCPUStagingAddress() override;

    /// Update the wrapped raw handle and byte size without reallocating.
    HDST_API
    void UpdateRawHandle(uint64_t rawHandle, size_t byteSize);

private:
    uint64_t _rawHandle;
    size_t _byteSize;
};

/// Returns a process-wide unique id for constructing the HgiBufferHandle that
/// wraps an external GPU buffer.  HgiHandle equality is id-based, so every
/// alias handle -- regardless of the call site or translation unit that
/// creates it -- must draw from this single counter.  Independent per-site
/// counters would restart at 1 and hand out colliding ids, making two distinct
/// external buffers compare equal in Storm's binding/aggregation caches.
HDST_API
uint64_t HdSt_GetNextExtGpuBufferHandleId();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_H
