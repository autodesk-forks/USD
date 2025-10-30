//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_THREAD_LIMITS_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_THREAD_LIMITS_H

#include <thread>

static int _threadLimit = 0;

inline int GetThreadLimit(){
    return _threadLimit;
}

/// Implements WorkGetPhysicalConcurrencyLimit
inline unsigned
WorkImpl_GetPhysicalConcurrencyLimit()
{
    return std::thread::hardware_concurrency();
}

/// Helps implement Work_InitializeThreading
inline void 
WorkImpl_InitializeThreading(unsigned threadLimit)
{
    return;
}

/// Implements WorkSetConcurrencyLimit
inline void
WorkImpl_SetConcurrencyLimit(unsigned threadLimit)
{
    unsigned numCores = std::thread::hardware_concurrency();
    if (threadLimit != 1) {
        _threadLimit = numCores;
    } else {
        _threadLimit = 1;
    }
}

// Return the concurrency limit.
inline int WorkImpl_GetConcurrencyLimit()
{
    return _threadLimit;
}

/// Implements WorkSupportsGranularThreadLimits
inline bool
WorkImpl_SupportsGranularThreadLimits()
{
    return false;
}

#endif