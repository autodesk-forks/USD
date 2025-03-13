//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIALX_SYNC_DISPATCHER_H
#define PXR_IMAGING_HD_ST_MATERIALX_SYNC_DISPATCHER_H

#include "pxr/pxr.h"
#include "pxr/base/work/dispatcher.h"

#include <tbb/concurrent_unordered_set.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdSt_MaterialXGeneratorTask;
class SdfPath;

using HdStResourceRegistrySharedPtr =
    std::shared_ptr<class HdStResourceRegistry>;

/// Manages parallel generator tasks, including caching them by material hash
/// so that only one task is launched for each unique material.
class HdSt_MaterialXSyncDispatcher
{
public:
    HdSt_MaterialXSyncDispatcher();
    ~HdSt_MaterialXSyncDispatcher();

    /// Add the generator task to the cache. If this material hash hasn't been
    /// added before, the task is launched on a worker thread.
    void AddGeneratorTask(
        std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
        HdStResourceRegistrySharedPtr& resourceRegistry);

    /// Wait for all parallel tasks to complete, after which point it's safe to
    /// call HdStMaterial::Sync() for all materials.
    void Wait();

private:
    void _RunGeneratorTask(
        std::unique_ptr<HdSt_MaterialXGeneratorTask>,
        HdStResourceRegistrySharedPtr& resourceRegistry);

    WorkDispatcher _dispatcher;

    using GeneratorTaskSet = tbb::concurrent_unordered_set<size_t>;
    GeneratorTaskSet _generatorTaskSet;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_MATERIALX_SYNC_DISPATCHER_H
