//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIALX_SYNC_SCENE_INDEX_H
#define PXR_IMAGING_HD_ST_MATERIALX_SYNC_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <tbb/concurrent_unordered_set.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdSt_MaterialXGeneratorTask;
class SdfPath;
class HdStRenderDelegate;

using HdStResourceRegistrySharedPtr =
    std::shared_ptr<class HdStResourceRegistry>;

/// Launches and manages parallel generator tasks, including caching them by
/// material hash, so that only one task is launched for each unique material.
class HdSt_MaterialXSyncSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HdSt_MaterialXSyncSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdStRenderDelegate& renderDelegate);

    ~HdSt_MaterialXSyncSceneIndex() override;

    /// Wait for all parallel tasks to complete
    void Wait();

    // HdSceneIndexBase overrides
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    // HdSingleInputFilteringSceneIndexBase overrides
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    // Add the generator task to the cache. If this material hash hasn't been
    // added before, the task is launched on a worker thread.
    void _AddGeneratorTask(
        std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
        HdStResourceRegistrySharedPtr& resourceRegistry);

    // Lunch the generator task
    void _LaunchGeneratorTask(
        std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
        HdStResourceRegistrySharedPtr& resourceRegistry);

    const HdStRenderDelegate& _renderDelegate;

    // Synchronizes the generator tasks
    WorkDispatcher _dispatcher;

    // Thread-safe set of material hashes for which generator tasks have been
    // launched. Used to prevent launching duplicate tasks.
    using _GeneratorTaskSet = tbb::concurrent_unordered_set<size_t>;
    _GeneratorTaskSet _generatorTaskSet;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_MATERIALX_SYNC_SCENE_INDEX_H
