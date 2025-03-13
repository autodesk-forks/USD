//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/materialXSyncDispatcher.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/materialXFilter.h"

#include "pxr/usd/sdf/path.h"

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

HdSt_MaterialXSyncDispatcher::HdSt_MaterialXSyncDispatcher()
{
}

HdSt_MaterialXSyncDispatcher::~HdSt_MaterialXSyncDispatcher()
{
}

void
HdSt_MaterialXSyncDispatcher::AddGeneratorTask(
    std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
    HdStResourceRegistrySharedPtr& resourceRegistry)
{
    HD_TRACE_FUNCTION();


    if ( resourceRegistry->ContainsMaterialXShader(
        generatorTask->GetShaderHash() )) {
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
        _RunGeneratorTask(
            std::move(generatorTask), resourceRegistry);
    }
}

void
HdSt_MaterialXSyncDispatcher::_RunGeneratorTask(
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
HdSt_MaterialXSyncDispatcher::Wait()
{
	HD_TRACE_FUNCTION();

    _dispatcher.Wait();
	_generatorTaskSet.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
