//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_SCOPED_PARALLELISM_H
#define PXR_EXTRAS_USD_EXAMPLES_WORK_DISPATCH_EXAMPLE_SCOPED_PARALLELISM_H

template <class Fn>
auto
WorkImpl_WithScopedParallelism(Fn &&fn)
{
    // Calling wait on dispatch groups is blocking so there is no risk of the 
    // calling thread stealing work.
    return std::forward<Fn>(fn)();
}

#endif