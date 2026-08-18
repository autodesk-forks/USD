//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_PARTICLE_FIELD_CONVERSION_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PARTICLE_FIELD_CONVERSION_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_PARTICLE_FIELD_CONVERSION_TOKENS                                                 \
                                                                                              \
    ((radianceSphericalHarmonicsCoefficients00, "radiance:sphericalHarmonicsCoefficients00")) \
                                                                                              \
    ((radianceSphericalHarmonicsCoefficients10, "radiance:sphericalHarmonicsCoefficients10")) \
    ((radianceSphericalHarmonicsCoefficients11, "radiance:sphericalHarmonicsCoefficients11")) \
    ((radianceSphericalHarmonicsCoefficients12, "radiance:sphericalHarmonicsCoefficients12")) \
                                                                                              \
    ((radianceSphericalHarmonicsCoefficients20, "radiance:sphericalHarmonicsCoefficients20")) \
    ((radianceSphericalHarmonicsCoefficients21, "radiance:sphericalHarmonicsCoefficients21")) \
    ((radianceSphericalHarmonicsCoefficients22, "radiance:sphericalHarmonicsCoefficients22")) \
    ((radianceSphericalHarmonicsCoefficients23, "radiance:sphericalHarmonicsCoefficients23")) \
    ((radianceSphericalHarmonicsCoefficients24, "radiance:sphericalHarmonicsCoefficients24")) \
                                                                                              \
    ((radianceSphericalHarmonicsCoefficients30, "radiance:sphericalHarmonicsCoefficients30")) \
    ((radianceSphericalHarmonicsCoefficients31, "radiance:sphericalHarmonicsCoefficients31")) \
    ((radianceSphericalHarmonicsCoefficients32, "radiance:sphericalHarmonicsCoefficients32")) \
    ((radianceSphericalHarmonicsCoefficients33, "radiance:sphericalHarmonicsCoefficients33")) \
    ((radianceSphericalHarmonicsCoefficients34, "radiance:sphericalHarmonicsCoefficients34")) \
    ((radianceSphericalHarmonicsCoefficients35, "radiance:sphericalHarmonicsCoefficients35")) \
    ((radianceSphericalHarmonicsCoefficients36, "radiance:sphericalHarmonicsCoefficients36")) \

TF_DECLARE_PUBLIC_TOKENS(HdsiParticleFieldConversionTokens, HDSI_PARTICLE_FIELD_CONVERSION_TOKENS);

TF_DECLARE_REF_PTRS(HdsiParticleFieldConversionSceneIndex);

// This scene index converts a particle field primitive to a points primitive
// for renderers which cannot natively render the particle field. It has three
// optional inputs:
// 1. constantWidth: A SampledDataSource if the renderer would like to render
// the points with a constant width. This datasource will be retuned as the
// primvar value for the width. Otherwise the plugin will use the average scales
// as the width if scales are present or 1.0 if not.
// 2. geometryOveral: Overlayed under the particle field if you want to
// provide additional render specific options when rendering these points.
// 3. materialOverlay: While a default UsdPreviewSurface approximation is
// provided you may want to use a more advanced material for your renderer. This
// will be overlayed with the default material under the render context you
// specify.
class HdsiParticleFieldConversionSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiParticleFieldConversionSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex, 
        const HdSampledDataSourceHandle constantWidth, 
        const HdContainerDataSourceHandle geometryOverlay, 
        const HdContainerDataSourceHandle materialOverlay);

    HDSI_API
    virtual HdSceneIndexPrim GetPrim(const SdfPath& primPath) const;

    HDSI_API
    virtual SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const;

protected:
    HdSampledDataSourceHandle _constantWidth;
    HdContainerDataSourceHandle _geometryOverlay, _materialOverlay;

    HdsiParticleFieldConversionSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdSampledDataSourceHandle constantWidth, 
        const HdContainerDataSourceHandle geometryOverlay, 
        const HdContainerDataSourceHandle materialOverlay);

    virtual void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries);

    virtual void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries);

    virtual void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDSI_PARTICLE_FIELD_CONVERSION_SCENE_INDEX_H