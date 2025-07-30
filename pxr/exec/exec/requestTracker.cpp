//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/requestTracker.h"

#include "pxr/exec/exec/requestImpl.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_RequestTracker::Insert(Exec_RequestImpl *impl)
{
    bool inserted;
    {
        TfSpinMutex::ScopedLock lock{_requestsMutex};
        inserted = _requests.insert(impl).second;
    }
    TF_VERIFY(inserted);
}

void
Exec_RequestTracker::Remove(Exec_RequestImpl *impl)
{
    bool erased;
    {
        TfSpinMutex::ScopedLock lock{_requestsMutex};
        erased = _requests.erase(impl);
    }
    TF_VERIFY(erased);
}

void
Exec_RequestTracker::DidInvalidateComputedValues(
    const Exec_AuthoredValueInvalidationResult &invalidationResult)
{
    TfSpinMutex::ScopedLock lock{_requestsMutex};
    for (Exec_RequestImpl * const impl : _requests) {
        // TODO: Once we expect the system to contain more than a handful of
        // requests, we should do this in parallel. We might still want to
        // invoke the invalidation callbacks serially, though.
        impl->DidInvalidateComputedValues(invalidationResult);
    }
}

void
Exec_RequestTracker::DidInvalidateComputedValues(
    const Exec_DisconnectedInputsInvalidationResult &invalidationResult)
{
    TfSpinMutex::ScopedLock lock{_requestsMutex};
    for (Exec_RequestImpl * const impl : _requests) {
        // TODO: Once we expect the system to contain more than a handful of
        // requests, we should do this in parallel. We might still want to
        // invoke the invalidation callbacks serially, though.
        impl->DidInvalidateComputedValues(invalidationResult);
    }
}

void
Exec_RequestTracker::DidChangeTime(
    const Exec_TimeChangeInvalidationResult &invalidationResult) const
{
    TfSpinMutex::ScopedLock lock{_requestsMutex};
    for (Exec_RequestImpl * const impl : _requests) {
        // TODO: Once we expect the system to contain more than a handful of
        // requests, we should do this in parallel. We might still want to
        // invoke the invalidation callbacks serially, though.
        impl->DidChangeTime(invalidationResult);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
