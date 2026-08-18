//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h"

#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiLegacyDisplayStyleSceneIndex_Impl
{

struct _StyleInfo
{
    std::optional<int> refineLevelFallback;
    /// Retained data source storing refineLevelFallback (or null ptr if
    /// empty optional value) to avoid allocating a data source for every prim.
    HdDataSourceBaseHandle refineLevelFallbackDs;

    TfToken cullStyleFallback;
    /// Retained data source storing cullStyleFallback (or null ptr if
    /// empty optional value) to avoid allocating a data source for every prim.
    HdDataSourceBaseHandle cullStyleFallbackDS;
};

/// Data source for locator displayStyle.
class _LegacyDisplayStyleFallbackDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LegacyDisplayStyleFallbackDataSource);

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdLegacyDisplayStyleSchemaTokens->refineLevel) {
            return _styleInfo->refineLevelFallbackDs;
        }
        if (name == HdLegacyDisplayStyleSchemaTokens->cullStyle) {
            return _styleInfo->cullStyleFallbackDS;
        }
        return nullptr;
    }

    TfTokenVector GetNames() override
    {
        static const TfTokenVector names = {
            HdLegacyDisplayStyleSchemaTokens->refineLevel,
            HdLegacyDisplayStyleSchemaTokens->cullStyle
        };

        return names;
    }

private:
    _LegacyDisplayStyleFallbackDataSource(_StyleInfoSharedPtr const &styleInfo)
      : _styleInfo(styleInfo)
    {
    }

    _StyleInfoSharedPtr _styleInfo;
};

} // namespace HdsiLegacyDisplayStyleSceneIndex_Impl

using namespace HdsiLegacyDisplayStyleSceneIndex_Impl;

HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr
HdsiLegacyDisplayStyleOverrideSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiLegacyDisplayStyleOverrideSceneIndex(
            inputSceneIndex));
}

HdsiLegacyDisplayStyleOverrideSceneIndex::
HdsiLegacyDisplayStyleOverrideSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _styleInfo(std::make_shared<_StyleInfo>())
  , _underlayDs(
      HdRetainedContainerDataSource::New(
          HdLegacyDisplayStyleSchemaTokens->displayStyle,
          _LegacyDisplayStyleFallbackDataSource::New(_styleInfo)))
{
}

HdSceneIndexPrim
HdsiLegacyDisplayStyleOverrideSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.dataSource) {
        prim.dataSource =
            HdOverlayContainerDataSource::New(
                prim.dataSource, _underlayDs);
    }
    return prim;
}

SdfPathVector
HdsiLegacyDisplayStyleOverrideSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}


void
HdsiLegacyDisplayStyleOverrideSceneIndex::SetCullStyleFallback(
    const TfToken &cullStyleFallback)
{
    if (cullStyleFallback == _styleInfo->cullStyleFallback) {
        return;
    }

    _styleInfo->cullStyleFallback = cullStyleFallback;
    _styleInfo->cullStyleFallbackDS =
        !cullStyleFallback.IsEmpty()
        ? HdRetainedTypedSampledDataSource<TfToken>::New(cullStyleFallback)
        : nullptr;

    static const HdDataSourceLocatorSet locators(
        HdLegacyDisplayStyleSchema::GetDefaultLocator() );
    // XXX We get insufficient invalidation if we append the
    // cullStyleFallback locator:
    // .Append(HdLegacyDisplayStyleSchemaTokens->cullStyleFallback));

    _DirtyAllPrims(locators);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::SetRefineLevelFallback(
    const std::optional<int> &refineLevelFallback)
{
    if (refineLevelFallback == _styleInfo->refineLevelFallback) {
        return;
    }

    _styleInfo->refineLevelFallback = refineLevelFallback;
    _styleInfo->refineLevelFallbackDs =
        refineLevelFallback
            ? HdRetainedTypedSampledDataSource<int>::New(*refineLevelFallback)
            : nullptr;

    static const HdDataSourceLocatorSet locators(
        HdLegacyDisplayStyleSchema::GetDefaultLocator()
            .Append(HdLegacyDisplayStyleSchemaTokens->refineLevel));

    _DirtyAllPrims(locators);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_DirtyAllPrims(
    const HdDataSourceLocatorSet &locators)
{
    if (!_IsObserved()) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries entries;
    for (const SdfPath &path : HdSceneIndexPrimView(_GetInputSceneIndex())) {
        entries.push_back({path, locators});
    }

    _SendPrimsDirtied(entries);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsAdded(entries);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);
}

void
HdsiLegacyDisplayStyleOverrideSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsDirtied(entries);
}


PXR_NAMESPACE_CLOSE_SCOPE
