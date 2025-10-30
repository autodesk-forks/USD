//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_DETACHED_TASK_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_DETACHED_TASK_H

#include "workDispatchExample/dispatcher.h"
#include "workDispatchExample/api.h"


#include <type_traits>
#include <utility>

class WorkImpl_Dispatcher;

WORK_DISPATCH_EXAMPLE_API
WorkImpl_Dispatcher &
WorkDispatch_GetDetachedDispatcher();

WORK_DISPATCH_EXAMPLE_API
void
WorkDispatch_EnsureDetachedTaskProgress();

/// Invoke \p fn asynchronously, discard any errors it produces, and provide
/// no way to wait for it to complete.
template <class Fn>
inline void _RunDetachedTaskImpl(Fn &&fn)
{
    WorkDispatch_GetDetachedDispatcher().Run(std::move(fn));
    WorkDispatch_EnsureDetachedTaskProgress();
}

#endif //PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_DETACHED_TASK_H
