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

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/exec/systemChangeProcessor.h"
#include "pxr/exec/esfUsd/sceneAdapter.h"
#include "pxr/usd/usd/notice.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdStage);

// TfNotice requires that notice listeners implement TfWeakPtrFacace.
class ExecUsdSystem::_NoticeListener : public TfWeakBase
{
public:
    // Subscribe to notices in the constructor.
    _NoticeListener(
        ExecUsdSystem *system,
        const UsdStageConstRefPtr &stage);

    // Revoke notice subscriptions in the destructor.
    ~_NoticeListener();

private:
    // Delivers UsdNotice::ObjectsChanged notices to the ExecSystem.
    void _DidObjectsChanged(
        const UsdNotice::ObjectsChanged &objectsChanged);

    ExecUsdSystem *const _system;
    TfNotice::Key _objectsChangedNoticeKey;
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
        TF_CODING_ERROR("Cannot cache an expired request");
        return ExecUsdCacheView();
    }

    ExecUsd_RequestImpl &requestImpl = request._GetImpl();

    // Before caching values, make sure that the request has been prepared.
    requestImpl.Compile();
    requestImpl.Schedule();

    return requestImpl.Compute();
}

ExecUsdSystem::_NoticeListener::_NoticeListener(
    ExecUsdSystem *const system,
    const UsdStageConstRefPtr &stage)
    : _system(system)
    , _objectsChangedNoticeKey(
        TfNotice::Register(
            TfCreateWeakPtr(this),
            &ExecUsdSystem::_NoticeListener::_DidObjectsChanged,
            UsdStageConstPtr(stage)))
{
}

ExecUsdSystem::_NoticeListener::~_NoticeListener()
{
    TfNotice::Revoke(_objectsChangedNoticeKey);
}

void
ExecUsdSystem::_NoticeListener::_DidObjectsChanged(
    const UsdNotice::ObjectsChanged &objectsChanged)
{
    TRACE_FUNCTION();

    ExecSystem::_ChangeProcessor changeProcessor(_system);

    for (const SdfPath &path : objectsChanged.GetResyncedPaths()) {
        changeProcessor.DidResync(path);
    }

    for (const SdfPath &path :
        objectsChanged.GetResolvedAssetPathsResyncedPaths()) {
        changeProcessor.DidResync(path);
    }

    for (const SdfPath &path : objectsChanged.GetChangedInfoOnlyPaths()) {
        changeProcessor.DidChangeInfoOnly(
            path,
            objectsChanged.GetChangedFields(path));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
