//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REQUEST_TRACKER_H
#define PXR_EXEC_EXEC_REQUEST_TRACKER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pxrTslRobinMap/robin_set.h"
#include "pxr/base/tf/spinMutex.h"

PXR_NAMESPACE_OPEN_SCOPE

class Exec_AuthoredValueInvalidationResult;
class Exec_DisconnectedInputsInvalidationResult;
class Exec_RequestImpl;
class Exec_TimeChangeInvalidationResult;

/// Maintains a (non-owning) set of outstanding requests.
class Exec_RequestTracker
{
public:
    /// Add \p impl to the collection of outstanding requets.
    ///
    /// The tracker does not take ownership of the request impl.  It is
    /// responsible for notifying the request of value, topological, and time
    /// changes.
    ///
    void Insert(Exec_RequestImpl *impl);

    /// Remove \p impl from the collection of outstanding requests.
    ///
    /// The request will no longer receive change notification.
    ///
    void Remove(Exec_RequestImpl *impl);

    /// Notify all requests of invalid computed values as a consequence of
    /// authored value invalidation.
    /// 
    void DidInvalidateComputedValues(
        const Exec_AuthoredValueInvalidationResult &invalidationResult);

    /// Notify all requests of invalid computed values as a consequence of
    /// uncompilation.
    /// 
    void DidInvalidateComputedValues(
        const Exec_DisconnectedInputsInvalidationResult &invalidationResult);

    /// Notify all requests of time having changed.
    void DidChangeTime(
        const Exec_TimeChangeInvalidationResult &invalidationResult) const;

private:
    mutable TfSpinMutex _requestsMutex;
    pxr_tsl::robin_set<Exec_RequestImpl *> _requests;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
