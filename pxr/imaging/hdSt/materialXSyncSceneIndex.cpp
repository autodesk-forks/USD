//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/materialXSyncSceneIndex.h"
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

HdSt_MaterialXSyncSceneIndex::HdSt_MaterialXSyncSceneIndex(
    const HdSceneIndexBaseRefPtr&   inputSceneIndex,
    const HdStRenderDelegate&       renderDelegate)
    : HdSingleInputFilteringSceneIndexBase( inputSceneIndex)
    , _renderDelegate(                      renderDelegate)
{
}

HdSt_MaterialXSyncSceneIndex::~HdSt_MaterialXSyncSceneIndex()
{
}

HdSceneIndexPrim
HdSt_MaterialXSyncSceneIndex::GetPrim(const SdfPath &primPath) const
{
    // Just forward to input scene index - we don't modify prim data
    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdSt_MaterialXSyncSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    // Just forward to input scene index - we don't modify hierarchy
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdSt_MaterialXSyncSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    HD_TRACE_FUNCTION();

    for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
        if (entry.primType == HdPrimTypeTokens->material) {
            HdSceneIndexPrim materialPrim =
                _GetInputSceneIndex()->GetPrim(entry.primPath);

            VtValue vtMat = HdSceneIndexAdapterSceneDelegate::GetMaterialResourceFromSceneIndexPrim(
                materialPrim,
                _renderDelegate.GetMaterialRenderContexts());

            HdStResourceRegistrySharedPtr resourceRegistry =
                std::static_pointer_cast<HdStResourceRegistry>(
                    _renderDelegate.GetResourceRegistry());

            auto generatorTask = HdSt_CreateMaterialXGeneratorTask(
                entry.primPath,
                vtMat,
                *resourceRegistry->GetHgi());

            if (generatorTask) {
                _AddGeneratorTask(std::move(generatorTask), resourceRegistry);
            }
        }
    }
    
    // Forward the notification to observers
    _SendPrimsAdded(entries);
}

void
HdSt_MaterialXSyncSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    // Just forward the notification - we don't need to do anything special for removed prims
    _SendPrimsRemoved(entries);
}

void
HdSt_MaterialXSyncSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // Just forward the notification - we don't need to do anything special for dirtied prims
    _SendPrimsDirtied(entries);
}

void
HdSt_MaterialXSyncSceneIndex::_AddGeneratorTask(
    std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
    HdStResourceRegistrySharedPtr& resourceRegistry)
{
    HD_TRACE_FUNCTION();

    if (resourceRegistry->ContainsMaterialXShader(
        generatorTask->GetShaderHash())) {
        // We already have a shader for this topology.
        return;
    }

    // Use a separate concurrent container for tracking unique generator tasks.
    // The generated shader will be registered in the resource registry when
    // the task completes.
    auto insertResult =
        _generatorTaskSet.insert(generatorTask->GetShaderHash());

    if (insertResult.second) {
        // If no generator task with the given hash existed,
        // run the task
        _LaunchGeneratorTask(
            std::move(generatorTask), resourceRegistry);
    }
}

void
HdSt_MaterialXSyncSceneIndex::_LaunchGeneratorTask(
    std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
    HdStResourceRegistrySharedPtr& resourceRegistry)
{
    HD_TRACE_FUNCTION();

    // Just in case, pass the registry to the parallel task by a weak pointer,
    // to avoid keeping it alive if something goes wrong with synchronization.
    std::weak_ptr<HdStResourceRegistry> resourceRegistryWeakPtr
        = resourceRegistry;

    _dispatcher.Run(
        [generatorTask = std::move(generatorTask), resourceRegistryWeakPtr]
        {
            HD_TRACE_FUNCTION();

            mx::ShaderPtr mxShader;

            try {
                mxShader = generatorTask->Generate();
            }
            catch (mx::Exception& exception) {
                TF_CODING_ERROR("Unable to generate the MaterialX shader.\n"
                    "MxException: %s", exception.what());
            }

            if (HdStResourceRegistrySharedPtr resourceRegistry =
                resourceRegistryWeakPtr.lock()) {

                HdInstance<mx::ShaderPtr> mxShaderInstance =
                    resourceRegistry->RegisterMaterialXShader(
                        generatorTask->GetShaderHash());

                // Store the mx::ShaderPtr
                mxShaderInstance.SetValue(mxShader);
            }
            else {
                TF_CODING_ERROR(
                    "HdStResourceRegistry destroyed before "
                    "the async task completes");
            }
        }
    );
}

void
HdSt_MaterialXSyncSceneIndex::Wait()
{
    HD_TRACE_FUNCTION();

    _dispatcher.Wait();
    _generatorTaskSet.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
