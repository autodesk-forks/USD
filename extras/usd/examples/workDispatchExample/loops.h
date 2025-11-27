//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_LOOPS_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_LOOPS_H

#include <dispatch/dispatch.h> 

/// Implements WorkParallelForN
template <typename Fn>
void
WorkImpl_ParallelForN(size_t n, Fn &&callback, size_t grainSize)
{
    dispatch_queue_t queue = 
        dispatch_queue_create("ForNQueue",  DISPATCH_QUEUE_CONCURRENT);
    dispatch_apply(n, queue, ^(size_t i) {
        callback(i, i+1);
        });
}

/// Implements WorkParallelForEach
template <typename InputIterator, typename Fn>
void
WorkImpl_ParallelForEach(
    InputIterator first, InputIterator last, Fn &&fn)
{
    size_t n = std::distance(first, last);
    dispatch_queue_t queue = 
        dispatch_queue_create("ForEachQueue",  DISPATCH_QUEUE_CONCURRENT);
    dispatch_apply(n, queue, ^(size_t i) {
        fn(*(std::next(first, i)));
    });
}


#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_LOOPS_H
