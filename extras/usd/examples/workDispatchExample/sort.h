//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
// 
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_SORT_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_SORT_H

#include <dispatch/dispatch.h>
#include <typeinfo>
#include <algorithm>
#include <iterator>

// QuickSort implementation
template <typename Iter, typename Fn>
inline void WorkLibdispatch_QuickSort(
                                   Iter low, 
                                   Iter high, 
                                   dispatch_group_t group, 
                                   dispatch_queue_t queue,
                                   const Fn& comp) 
{ 
    if (low == high) {
        return;
    }
    auto pivot = *std::next(low, std::distance(low, high)/2);
    auto mid = std::partition(low, high, 
        [pivot](const auto& elem) { 
            return elem < pivot;
        });
    auto mid2 = std::partition(mid, high,
        [pivot](const auto& elem) { 
            return elem <= pivot;
        });
 
    dispatch_group_async(group, queue, ^{ WorkLibdispatch_QuickSort(
        low, mid, group, queue, comp);});
    dispatch_group_async(group, queue, ^{ WorkLibdispatch_QuickSort(
        mid2, high, group, queue, comp);});
}

/// Implements WorkParallelSort with a custom comparison functor.
///
template <typename C, typename Compare>
inline void 
WorkImpl_ParallelSort(C* container, const Compare& comp)
{
    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = 
        dispatch_queue_create("SortQueue",  DISPATCH_QUEUE_CONCURRENT);
    dispatch_group_async(group, queue, ^{WorkLibdispatch_QuickSort(
        container->begin(), container->end(), group, queue, comp);});
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    dispatch_release(group);
}

/// Implements WorkParallelSort
///
template <typename C>
inline void 
WorkImpl_ParallelSort(C* container)
{
    WorkImpl_ParallelSort(container, std::less<>());
}

#endif