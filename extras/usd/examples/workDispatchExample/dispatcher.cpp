//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "workDispatchExample/dispatcher.h"
#include "workDispatchExample/threadLimits.h"

WorkImpl_Dispatcher::WorkImpl_Dispatcher()
{
    if( WorkImpl_GetConcurrencyLimit() == 1) {
        _queue = dispatch_queue_create("DispatchQueue",  DISPATCH_QUEUE_SERIAL);
    } else {
        _queue = 
            dispatch_queue_create("DispatchQueue",  DISPATCH_QUEUE_CONCURRENT);
    }
    _group = dispatch_group_create();
}

WorkImpl_Dispatcher::~WorkImpl_Dispatcher() noexcept
{
    Wait();
    dispatch_group_wait(_group, DISPATCH_TIME_FOREVER);
}

void
WorkImpl_Dispatcher::Wait()
{
    // Wait for tasks to complete.
    dispatch_group_wait(_group, DISPATCH_TIME_FOREVER);
    
}

void
WorkImpl_Dispatcher::Reset() 
{
    return;
}

void
WorkImpl_Dispatcher::Cancel()
{
    return;
}
