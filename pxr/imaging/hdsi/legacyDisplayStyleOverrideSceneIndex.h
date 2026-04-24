//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiLegacyDisplayStyleSceneIndex_Impl
{
struct _StyleInfo;
using _StyleInfoSharedPtr = std::shared_ptr<_StyleInfo>;
}

TF_DECLARE_REF_PTRS(HdsiLegacyDisplayStyleOverrideSceneIndex);

///
/// \class HdsiLegacyDisplayStyleOverrideSceneIndex
///
/// A scene index providing override fallback values for
/// the legacy display style for each prim.
///
/// This will provide fallback values only when specific
/// fallback values have been specified.
/// Otherwise the fallback will be to the schema default values.
///
/// So far, this supports only refineLevel and cullStyle.
///
class HdsiLegacyDisplayStyleOverrideSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// Sets the refineLevelFallback
    /// (at data source locator displayStyle:refineLevel)
    /// for every prim in the input scene inedx.
    ///
    /// If an empty optional value is provided, a null data source will be
    /// returned for the data source locator.
    ///
    HDSI_API
    void SetRefineLevelFallback(const std::optional<int> &refineLevelFallback);

    /// Sets the cullStyleFallback
    /// (at data source locator displayStyle:cullStyleFallback)
    /// for every prim in the input scene index.
    ///
    /// If an empty token value is provided, a null data source will be
    /// returned for the data source locator.
    ///
    HDSI_API
    void SetCullStyleFallback(const TfToken &cullStyleFallback);

protected:
    HdsiLegacyDisplayStyleOverrideSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

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
    void _DirtyAllPrims(const HdDataSourceLocatorSet &locators);

    HdsiLegacyDisplayStyleSceneIndex_Impl::
    _StyleInfoSharedPtr const _styleInfo;

    /// Prim underlay data source.
    HdContainerDataSourceHandle const _underlayDs;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H
