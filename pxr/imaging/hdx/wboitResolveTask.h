//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_WBOIT_RESOLVE_TASK_H
#define PXR_IMAGING_HDX_WBOIT_RESOLVE_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/fullscreenShader.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdxWbOitResolveTask : public pxr::HdxTask
{
public:
    HDX_API
    HdxWbOitResolveTask(pxr::HdSceneDelegate* delegate, pxr::SdfPath const& id);

    HDX_API
    ~HdxWbOitResolveTask() override;

    HDX_API
    void Prepare(pxr::HdTaskContext* ctx, pxr::HdRenderIndex* renderIndex) override;

    HDX_API
    void Execute(pxr::HdTaskContext* ctx) override;

protected:

    HDX_API
    void _Sync(pxr::HdSceneDelegate* delegate, pxr::HdTaskContext* ctx,
        pxr::HdDirtyBits* dirtyBits) override;

private:
    HdxWbOitResolveTask()                                   = delete;
    HdxWbOitResolveTask(const HdxWbOitResolveTask&)            = delete;
    HdxWbOitResolveTask& operator=(const HdxWbOitResolveTask&) = delete;

private: 
    std::unique_ptr<class HdxFullscreenShader> _shader;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_WBOIT_RESOLVE_TASK_H
