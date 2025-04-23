//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILER_TASK_SYNC_H
#define PXR_EXEC_EXEC_COMPILER_TASK_SYNC_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/outputKey.h"

#include "pxr/base/tf/hash.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/parallelTaskWaitlist.h"

#include <tbb/concurrent_unordered_map.h>

#include <atomic>
#include <cstdint>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompiledOutputCache;

/// Instances of this class are used to synchronize compilation task graphs.
/// 
/// Tasks can claim dependent output keys for processing, and depending on the
/// returned result are on the hook for processing the claimed output key, or
/// will be notified when a task previously claiming the same output key
/// is completed.
/// 
/// The lifetime of instances of this class is expected to be limited to one
/// round of compilation.
///
class Exec_CompilerTaskSync
{
public:
    Exec_CompilerTaskSync(const Exec_CompilerTaskSync &) = delete;
    Exec_CompilerTaskSync &operator=(const Exec_CompilerTaskSync &) =
        delete;

    Exec_CompilerTaskSync() = default;

    /// The different results claiming an output key can return.
    ///
    enum class ClaimResult {
        Done,       /// The task is already done.

        Wait,       /// Another task is currently processing the output key and
                    /// the claimant will be notified once it is done.
                    
        Claimed     /// The output key has been successfully claimed, and the
                    /// claimant is on the hook for completing the work.
    };

    /// Attempts to claim the output \p key for processing, and returns
    /// whether the attempt was successful.
    /// 
    /// This method will increment the reference count of the \p successor, if
    /// the output key has already been claimed and \p successor needs to wait
    /// for the results. Once the task is done, the \p successor will be
    /// notified by decrementing its reference count, and if it reaches zero the
    /// \p successor will automatically be spawned.
    ///
    ClaimResult Claim(const Exec_OutputKey &key, tbb::task *successor);

    /// Marks the task associated with the output \p key done.
    /// 
    /// This method will notify any tasks depending on \p key by decrementing
    /// their reference counts, and spawning them if their reference count
    /// reaches 0.
    ///
    void MarkDone(const Exec_OutputKey &key);

private:
    // The various states a task can be in.
    enum _TaskState : uint8_t {
        _TaskStateUnclaimed,
        _TaskStateClaimed,
        _TaskStateDone
    };

    // Entries in the map always begin life as unclaimed tasks with no
    // nodes on their waitlist.
    struct _Entry {
        _Entry() : state(_TaskStateUnclaimed), waiting(nullptr) {}
        std::atomic<uint8_t> state;
        VdfParallelTaskWaitlist::HeadPtr waiting;
    };

    // The map of tasks that have been claimed during this round of
    // compilation.
    using _ClaimedTasks =
        tbb::concurrent_unordered_map<Exec_OutputKey, _Entry, TfHash>;
    _ClaimedTasks _claimedTasks;

    // Manages the waitlists in the _claimedTasks entries.
    VdfParallelTaskWaitlist _waitlists;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif