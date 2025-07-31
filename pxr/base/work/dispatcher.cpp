//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/dispatcher.h"

PXR_NAMESPACE_OPEN_SCOPE

WorkDispatcher::WorkDispatcher()
    : _dispatcher(WorkImpl_Dispatcher())
      , _isCancelled(false)
{
    _waitCleanupFlag.clear();
}

WorkDispatcher::~WorkDispatcher() noexcept
{
    Wait();
}

void
WorkDispatcher::Wait()
{
    // Wait for tasks to complete.
    _dispatcher.Wait();

    // If we take the flag from false -> true, we do the cleanup.
    if (_waitCleanupFlag.test_and_set() == false) {
        _dispatcher.Reset();

        // Post all diagnostics to this thread's list.
        for (auto &et: _errors) {
            et.Post();
        }
        _errors.clear();
        _waitCleanupFlag.clear();
        _isCancelled = false;
    }
}

bool
WorkDispatcher::IsCancelled() const
{
    return _isCancelled;
}

void
WorkDispatcher::Cancel()
{
    _isCancelled = true;
    _dispatcher.Cancel();
}

/* static */
void
WorkDispatcher::_TransportErrors(const TfErrorMark &mark,
                                 _ErrorTransports *errors)
{
    TfErrorTransport transport = mark.Transport();
    errors->grow_by(1)->swap(transport);
}

PXR_NAMESPACE_CLOSE_SCOPE
