//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compilerTaskSync.h"

#include "pxr/exec/exec/compiledOutputCache.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_CompilerTaskSync::ClaimResult
Exec_CompilerTaskSync::Claim(const Exec_OutputKey &key, tbb::task *successor)
{
    // Add the key to the map. If another task got to claiming it first, it's
    // expected and safe for the key to already have an entry.
    const auto &[iterator, inserted] = _claimedTasks.emplace(
        std::piecewise_construct, 
            std::forward_as_tuple(key),
            std::forward_as_tuple());
    _Entry *const entry = &iterator->second;

    // If the task associated with this output is already done, return here.
    uint8_t state = entry->state.load(std::memory_order_acquire);
    if (state == _TaskStateDone) {
        return ClaimResult::Done;
    }

    // If the task has not been claimed yet, attempt to claim it by CAS and
    // return the result.
    else if (state == _TaskStateUnclaimed &&
        entry->state.compare_exchange_strong(state, _TaskStateClaimed)) {
        return ClaimResult::Claimed;
    }

    // If we get here, the task has already been claimed, or the CAS failed and
    // another task got to claim it just before we did. In this case, wait on
    // the task completion. If we fail to wait on the task, it completed just
    // as we were about to wait and we can consider it done!
    const ClaimResult claimResult = _waitlists.WaitOn(
        &entry->waiting, successor)
        ? ClaimResult::Wait
        : ClaimResult::Done;
    return claimResult;
}

void
Exec_CompilerTaskSync::MarkDone(const Exec_OutputKey &key)
{
    // Note, some of these TF_VERIFYs can be safely relaxed if we later
    // want to mark tasks done from tasks that aren't the original claimaints.

    // We expect the publishing task to have previously claimed this key, so
    // there should already be an entry in the map.
    const auto iterator = _claimedTasks.find(key);
    if (!TF_VERIFY(iterator != _claimedTasks.end())) {
        return;
    }
    _Entry *const entry = &iterator->second;

    // Set the state to done. We expect this to transition from the claimed
    // state.
    const uint8_t previousState = entry->state.exchange(_TaskStateDone);
    TF_VERIFY(previousState == _TaskStateClaimed);

    // Close the waiting queue and notify all waiting tasks. We expect to be
    // the first to close the queue.
    const bool closed = _waitlists.CloseAndNotify(&entry->waiting);
    TF_VERIFY(closed);   
}

PXR_NAMESPACE_CLOSE_SCOPE
