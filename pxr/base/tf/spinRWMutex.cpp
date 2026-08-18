//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/spinRWMutex.h"
#include "pxr/base/arch/threads.h"

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

static constexpr int SpinsBeforeBackoff = 32;

template <class Fn>
static void WaitWithBackoff(Fn &&fn) {
    // Hope for the best...
    if (ARCH_LIKELY(fn())) {
        return;
    }
    // Otherwise spin for a bit...
    for (int i = 0; i != SpinsBeforeBackoff; ++i) {
        ARCH_SPIN_PAUSE();
        if (fn()) {
            return;
        }
    }
    // Keep checking but yield our thread...
    do {
        std::this_thread::yield();
    } while (!fn());
}
    

void
TfSpinRWMutex::_WaitForWriter() const
{
    // Wait until we see a cleared WriterFlag.  Relaxed ordering is correct
    // here: this is a pure poll and carries no happens-before obligation.
    // Synchronization is established by the acquire on the subsequent lock
    // operation, not by this load.
    WaitWithBackoff([this]() {
        return !(_lockState.load(std::memory_order_relaxed) & WriterFlag);
    });
}

void
TfSpinRWMutex::_WaitForReaders() const
{
    // Wait until we see zero readers.  Relaxed ordering is correct here for the
    // same reason as _WaitForWriter: we are polling for a condition, not
    // establishing synchronization.  The release on the final ReleaseRead (or
    // the undo fetch_sub in TryAcquireRead) pairs with the acquire on the write
    // lock operation that follows this wait, not with this load.
    WaitWithBackoff([this]() {
        return _lockState.load(std::memory_order_relaxed) == WriterFlag;
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
