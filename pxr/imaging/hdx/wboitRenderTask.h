//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_WBOIT_RENDER_TASK_H
#define PXR_IMAGING_HDX_WBOIT_RENDER_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hdx/renderTask.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdStRenderPassShaderSharedPtr = std::shared_ptr<HdStRenderPassShader>;

class HdStRenderBuffer;
class HdStRenderPassState;

/// \class HdxWbOitRenderTask
///
/// A task for rendering transparent geometry using 
/// Weighted Blended Order-Independent Transparency (https://jcgt.org/published/0002/02/09/)
/// Its companion task, HdxWbOitResolveTask, will blend the buffers to screen.
///
class HdxWbOitRenderTask : public HdxRenderTask 
{
public:
    static bool IsEnabled();

    HDX_API
    HdxWbOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxWbOitRenderTask() override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx, 
                 HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

private:
    HdxWbOitRenderTask() = delete;
    HdxWbOitRenderTask(const HdxWbOitRenderTask &) = delete;
    HdxWbOitRenderTask &operator =(const HdxWbOitRenderTask &) = delete;

    bool InitTextures(HdTaskContext* ctx, const HdRenderPassStateSharedPtr& renderPassState);

    HdStRenderPassShaderSharedPtr _wboitTranslucentRenderPassShader;
    HdRenderPassAovBindingVector _wboitAovBindings;
    std::vector<std::unique_ptr<HdStRenderBuffer>> _wboitBuffers;

    HdRenderIndex *_index;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_WBOIT_RENDER_TASK_H
