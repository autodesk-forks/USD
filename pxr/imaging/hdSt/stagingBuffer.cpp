//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/stagingBuffer.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/capabilities.h"
#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStStagingBuffer::HdStStagingBuffer(HdStResourceRegistry *resourceRegistry)
    : _resourceRegistry(resourceRegistry)
    , _head(0)
    , _capacity(0)
{
    _uniformMemoryAccess = resourceRegistry->GetHgi()->GetCapabilities()->
                          IsSet(HgiDeviceCapabilitiesBitsUnifiedMemory);
}

HdStStagingBuffer::~HdStStagingBuffer()
{
    Deallocate();
}

void
HdStStagingBuffer::Deallocate()
{
    if (_buffer) {
        _resourceRegistry->GetHgi()->DestroyBuffer(&_buffer);
        _buffer = {};
    }

    _capacity = 0;
}

void
HdStStagingBuffer::Resize(size_t capacity)
{
    // Only change the capacity if there aren't any queued copy operations.
    if (_head != 0) {
        TF_CODING_ERROR("Cannot change size of staging buffer during Commit");
        return;
    }

    _capacity = capacity;
}

void
HdStStagingBuffer::StageCopy(HgiBufferCpuToGpuOp const &copyOp)
{
    if (copyOp.byteSize == 0 ||
        !copyOp.cpuSourceBuffer ||
        !copyOp.gpuDestinationBuffer)
    {
        return;
    }

    // Skip staging buffer if device supports unified memory or when
    // the to-be-copied data is 'large'. Doing the extra memcpy into the
    // stating buffer to avoid many small GPU buffer upload can be more
    // expensive than just submitting the CPU to GPU copy operation directly.
    // The value of 'queueThreshold' is estimated (when is the extra memcpy
    // into the staging buffer slower than immediately issuing a gpu upload)
    static constexpr size_t queueThreshold = 512 * 1024;
    if (_uniformMemoryAccess || copyOp.byteSize > queueThreshold) {
        HgiBlitCmds* blitCmds = _resourceRegistry->GetGlobalBlitCmds();
        blitCmds->CopyBufferCpuToGpu(copyOp);
        return;
    }

    static constexpr size_t recoveryRatio = 4;

    // If there is no buffer or it is either too small or
    // substantially larger than the required size, recreate it.
    if (!_buffer ||
        _buffer->GetDescriptor().byteSize < _capacity ||
        _buffer->GetDescriptor().byteSize > _capacity * recoveryRatio) {
        HgiBufferDesc bufferDesc;
        bufferDesc.debugName = "HdStStagingBuffer";
        bufferDesc.byteSize = _capacity;

        Hgi* hgi = _resourceRegistry->GetHgi();

        if (_buffer) {
            hgi->DestroyBuffer(&_buffer);
        }

        _buffer = hgi->CreateBuffer(bufferDesc);
    }

    size_t capacity = _buffer->GetDescriptor().byteSize;
    uint8_t *cpuStaging = static_cast<uint8_t*>(_buffer->GetCPUStagingAddress());

    if (TF_VERIFY(_head + copyOp.byteSize <= capacity)) {
        // Copy source into the staging buffer.
        char const *sourceData =
            static_cast<char const *>(copyOp.cpuSourceBuffer);
        memcpy(cpuStaging + _head,
               sourceData + copyOp.sourceByteOffset,
               copyOp.byteSize);

        bool aggregated = false;

        // If this copy is contiguous with last staged one then aggregate them.
        if (!_gpuCopyOps.empty()) {
            HgiBufferGpuToGpuOp &lastCopy = _gpuCopyOps.back();
            size_t lastCopyEnd = lastCopy.destinationByteOffset
                               + lastCopy.byteSize;

            if (lastCopy.gpuDestinationBuffer == copyOp.gpuDestinationBuffer &&
                lastCopyEnd == copyOp.destinationByteOffset) {
                lastCopy.byteSize += copyOp.byteSize;
                aggregated = true;
            }
        }

        if (!aggregated) {
            // Create a GPU to GPU blit operation to do the final copy.
            HgiBufferGpuToGpuOp gpuCopy;

            gpuCopy.gpuSourceBuffer = _buffer;
            gpuCopy.sourceByteOffset = _head;
            gpuCopy.byteSize = copyOp.byteSize;
            gpuCopy.gpuDestinationBuffer = copyOp.gpuDestinationBuffer;
            gpuCopy.destinationByteOffset = copyOp.destinationByteOffset;

            _gpuCopyOps.push_back(gpuCopy);
        }

        _head += copyOp.byteSize;
    }
}

bool
HdStStagingBuffer::Flush()
{
    if (_head == 0) {
        // UMA case
        return false;
    }

    HgiBlitCmds* blitCmds = _resourceRegistry->GetGlobalBlitCmds();

    blitCmds->PushDebugGroup(__ARCH_PRETTY_FUNCTION__);

    HgiBufferCpuToGpuOp op;
    uint8_t* const cpuStaging = static_cast<uint8_t* const>(
        _buffer->GetCPUStagingAddress());

    op.cpuSourceBuffer = cpuStaging;
    op.sourceByteOffset = 0;
    op.gpuDestinationBuffer = _buffer;
    op.destinationByteOffset = 0;
    op.byteSize = _head;
    blitCmds->CopyBufferCpuToGpu(op);
    blitCmds->InsertMemoryBarrier(HgiMemoryBarrierAll);

    for (auto const &copyOp : _gpuCopyOps) {
        blitCmds->CopyBufferGpuToGpu(copyOp);
    }

    blitCmds->PopDebugGroup();

    _gpuCopyOps.clear();
    _head = 0;

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
