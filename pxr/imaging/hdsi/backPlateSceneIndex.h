//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_BACK_PLATE_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_BACK_PLATE_SCENE_INDEX_H
 
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE
 
TF_DECLARE_WEAK_AND_REF_PTRS(HdsiBackPlateSceneIndex);

/// \class HdsiBackPlateSceneIndex
///
/// The back plate scene index converts back plate prims into mesh and material
/// prims that align with the specifications outlined in:
/// https://github.com/PixarAnimationStudios/OpenUSD-proposals/tree/main/proposals/back-plates
/// 
/// TODO: Add support for opacity, depth maps, color-grading, and animated
/// materials.
/// 
class HdsiBackPlateSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiBackPlateSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);
 
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override final;
 
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override final;
 
protected:
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override final;
 
    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override final;
 
    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override final;
 
    HDSI_API
    HdsiBackPlateSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

private:
    void
    _RemoveBackPlateChildren(
        const SdfPath &primPath,
        SdfPathSet * const removedBackPlatePrims);
    void
    _AddBackPlateChildren(
        const SdfPath &primPath,
        SdfPathSet * const addedBackPlatePrims);
    void
    _DirtyBackPlateChildren(
        const SdfPath &primPath,
        HdSceneIndexObserver::DirtiedPrimEntries * const 
            dirtiedBackPlatePrims);

    HdSceneIndexPrim
    _GetMeshOrMaterialSiPrim(
        const SdfPath &plateInstancePath,
        const TfToken &plateChildType) const;
    
    bool
    _IsBackPlateInMap(
        const SdfPath &platePath) const;

    using _BackPlates = std::unordered_set<TfToken,TfToken::HashFunctor>;
    using _CameraToBackPlatesMap = std::map<SdfPath, _BackPlates>;

    // Maps the camera prim path to all the names of the back plate instances 
    // associated with it. 
    _CameraToBackPlatesMap _cameraToBackPlates;
};
 
PXR_NAMESPACE_CLOSE_SCOPE
 
#endif //PXR_IMAGING_HDSI_BACK_PLATE_SCENE_INDEX_H