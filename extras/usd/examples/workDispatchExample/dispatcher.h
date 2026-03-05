//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_DISPATCHER_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_DISPATCHER_H

#include <dispatch/dispatch.h>
#include "workDispatchExample/api.h"

#include <functional>
#include <type_traits>
#include <utility>

class WorkImpl_Dispatcher
{
public:

    /// Construct a new dispatcher.
    WORK_DISPATCH_EXAMPLE_API
    WorkImpl_Dispatcher();

    /// Wait() for any pending tasks to complete, then destroy the dispatcher.
    WORK_DISPATCH_EXAMPLE_API
    ~WorkImpl_Dispatcher() noexcept;

    WorkImpl_Dispatcher(WorkImpl_Dispatcher const &) = delete;
    WorkImpl_Dispatcher &operator=(WorkImpl_Dispatcher const &) = delete;

    template <class Callable>
    inline void Run(Callable &&c) {
        __block _InvokerTaskWrapper<
                    typename std::remove_reference<Callable>::type> task = 
        _InvokerTaskWrapper<typename std::remove_reference<Callable>::type>
                    (std::forward<Callable>(c));
        
        dispatch_group_async(_group, _queue, ^{
            task();});
    }

    WORK_DISPATCH_EXAMPLE_API
    void Reset();

    /// Block until the work started by Run() completes.
    WORK_DISPATCH_EXAMPLE_API
    void Wait();

    /// Cancel remaining work and return immediately.
    ///
    /// Calling this function affects task that are being run directly
    /// by this dispatcher. If any of these tasks are using their own
    /// dispatchers to run tasks, these dispatchers will not be affected
    /// and these tasks will run to completion, unless they are also
    /// explicitly cancelled.
    ///
    /// This call does not block.  Call Wait() after Cancel() to wait for
    /// pending tasks to complete.
    WORK_DISPATCH_EXAMPLE_API
    void Cancel();

    template <class Fn>
    struct _InvokerTaskWrapper {
        explicit _InvokerTaskWrapper(Fn &&fn) 
            : _fn(std::make_shared<Fn>(std::move(fn))) {};
        explicit _InvokerTaskWrapper(Fn const &fn) 
            : _fn(std::make_shared<Fn>(fn)) {};

        _InvokerTaskWrapper(_InvokerTaskWrapper &&other) = default;
        _InvokerTaskWrapper(const _InvokerTaskWrapper &other) = default;

        void operator()() const {
            (*_fn)(); 
        }
    private:
        std::shared_ptr<Fn> _fn;
    };
    dispatch_group_t _group;
    dispatch_queue_t _queue;
    
};

///////////////////////////////////////////////////////////////////////////////

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_DISPATCHER_H