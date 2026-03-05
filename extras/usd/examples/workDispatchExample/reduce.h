//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_REDUCE_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_REDUCE_H

#include <dispatch/dispatch.h>

inline size_t ceilDivide(size_t a, size_t b) {
    return (a/b) + (a % b != 0);
}

/// Implements WorkParallelReduce
template <typename Fn, typename Rn, typename V>
V
WorkImpl_ParallelReduceN(
    const V &identity,
    size_t n,
    Fn &&loopCallback,
    Rn &&reductionCallback,
    size_t grainSize)
{
    dispatch_queue_t queue = 
        dispatch_queue_create("ReduceQueue",  DISPATCH_QUEUE_CONCURRENT);
    // If N is less than grainsize, it's not worth incurring the overhead of
    // parallel work so run serially instead.
    if (n > grainSize) {
        // Process the underlying data in grainsized chunks
        size_t numIters = ceilDivide(n, grainSize); 
        __block std::vector<V> vals(numIters);
        dispatch_apply(numIters, queue, ^(size_t i) {
            size_t startIdx = i*grainSize;
            size_t endIdx = startIdx+grainSize >= n ? n : startIdx+grainSize;
            V val = loopCallback(startIdx,endIdx, identity);
            vals[i] = val;
        });
        // Merge each chunk with the chunk in the corresponding half i.e. if you
        // have a vector of {a, b, c, d} you would call the reductionCallback on 
        // {a,c} and {b,d} in one iteration. Repeat until only one value remains.
        size_t size = vals.size();
        size_t step = ceilDivide(size,2);
        while (step > 1) {
            dispatch_apply(step, queue, ^(size_t i) {
                if (i+step < size) {
                    V val = reductionCallback(vals[i], vals[i+step]);
                    vals[i] = val;
                }
            });
            size = step;
            step = ceilDivide(step,2);
        }
        return reductionCallback(vals[0],vals[1]);
    }
    return loopCallback(0, n, identity);
}

#endif // PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_REDUCE_H
