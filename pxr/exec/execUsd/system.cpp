//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/system.h"

#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/requestImpl.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/exec/execUsd/valueOverride.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/esfUsd/sceneAdapter.h"
#include "pxr/exec/esfUsd/stageData.h"
#include "pxr/exec/exec/systemChangeProcessor.h"
#include "pxr/usd/usd/notice.h"

#include <tbb/concurrent_vector.h>

#include <algorithm>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdStage);

class ExecUsdSystem::_NoticeListener : EsfUsdStageData::ListenerBase
{
public:
    _NoticeListener(
        ExecUsdSystem *system,
        const UsdStageConstRefPtr &stage)
      : _system(system)
      , _stageData(EsfUsdStageData::RegisterStage(stage, this))
    {}

    ~_NoticeListener() override {
        _stageData->Unregister(this);
    }

private:
    void _DidObjectsChanged(
        const UsdNotice::ObjectsChanged &objectsChanged,
        const EsfUsdStageData::ChangedPathSet &changedTargetPaths)
        const override;

    ExecUsdSystem *const _system;
    const std::shared_ptr<EsfUsdStageData> _stageData;
};

ExecUsdSystem::ExecUsdSystem(const UsdStageConstRefPtr &stage)
    : ExecSystem(EsfUsdSceneAdapter::AdaptStage(stage))
    , _noticeListener(std::make_unique<_NoticeListener>(this, stage))
{
}

ExecUsdSystem::~ExecUsdSystem() = default;

void
ExecUsdSystem::ChangeTime(const UsdTimeCode time)
{
    _ChangeTime(EfTime(time));
}

ExecUsdRequest
ExecUsdSystem::BuildRequest(
    std::vector<ExecUsdValueKey> &&valueKeys,
    ExecRequestComputedValueInvalidationCallback &&valueCallback,
    ExecRequestTimeChangeInvalidationCallback &&timeCallback)
{
    TRACE_FUNCTION();

    return ExecUsdRequest(
        std::make_unique<ExecUsd_RequestImpl>(
            this,
            std::move(valueKeys),
            std::move(valueCallback),
            std::move(timeCallback)));
}

void
ExecUsdSystem::PrepareRequest(const ExecUsdRequest &request)
{
    TRACE_FUNCTION();

    if (!request.IsValid()) {
        TF_CODING_ERROR("Cannot prepare an expired request");
        return;
    }

    ExecUsd_RequestImpl &requestImpl = request._GetImpl();
    requestImpl.Compile();
    requestImpl.Schedule();
}

ExecUsdCacheView
ExecUsdSystem::Compute(const ExecUsdRequest &request)
{
    TRACE_FUNCTION();

    if (!request.IsValid()) {
        TF_CODING_ERROR("Cannot compute an expired request");
        return ExecUsdCacheView();
    }

    ExecUsd_RequestImpl &requestImpl = request._GetImpl();

    // Before computing values, make sure that the request has been prepared.
    requestImpl.Compile();
    requestImpl.Schedule();

    return requestImpl.Compute();
}

ExecUsdCacheView
ExecUsdSystem::ComputeWithOverrides(
    const ExecUsdRequest &request,
    ExecUsdValueOverrideVector &&valueOverrides)
{
    TRACE_FUNCTION();

    if (!request.IsValid()) {
        TF_CODING_ERROR("Cannot compute an expired request");
        return ExecUsdCacheView();
    }

    ExecUsd_RequestImpl &requestImpl = request._GetImpl();

    // Before computing values, make sure that the request has been prepared.
    requestImpl.Compile();
    requestImpl.Schedule();

    return requestImpl.ComputeWithOverrides(std::move(valueOverrides));
}

void
ExecUsdSystem::_NoticeListener::_DidObjectsChanged(
    const UsdNotice::ObjectsChanged &objectsChanged,
    const EsfUsdStageData::ChangedPathSet &changedTargetPaths) const
{
    TRACE_FUNCTION();

    const UsdNotice::ObjectsChanged::PathRange resyncedPaths =
        objectsChanged.GetResyncedPaths();

    // If any objects were resynced, check for request expiration.
    if (!resyncedPaths.empty()) {
        TRACE_FUNCTION_SCOPE("check for expired requests");

        tbb::concurrent_vector<ExecUsd_RequestImpl*> expired;
        const auto expireRequests = [&expired] (Exec_RequestImpl &base) {
            ExecUsd_RequestImpl& impl = static_cast<ExecUsd_RequestImpl&>(base);
            impl.ExpireInvalidIndices();
            // We cannot discard requests from within this callback because the
            // request tracker is locked during its execution.
            if (impl.GetExpiredIndices().AreAllSet()) {
                expired.push_back(&impl);
            }
        };
        _system->_ParallelForEachRequest(expireRequests);

        for (ExecUsd_RequestImpl *const impl : expired) {
            impl->Discard();
        }
    }

    ExecSystem::_ChangeProcessor changeProcessor(_system);

    for (const SdfPath &path : resyncedPaths) {
        changeProcessor.DidResync(path);
    }

    for (const SdfPath &path :
             objectsChanged.GetResolvedAssetPathsResyncedPaths()) {
        changeProcessor.DidResync(path);
    }

    for (const SdfPath &path : objectsChanged.GetChangedInfoOnlyPaths()) {
        changeProcessor.DidChangeInfoOnly(
            path, objectsChanged.GetChangedFields(path));
    }

    for (const SdfPath &path : changedTargetPaths) {
        changeProcessor.DidChangeIncomingConnections(path);
    };
}

PXR_NAMESPACE_CLOSE_SCOPE
