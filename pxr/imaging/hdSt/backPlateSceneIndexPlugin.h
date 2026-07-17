//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HD_ST_BACK_PLATE_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_ST_BACK_PLATE_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_BackPlateSceneIndexPlugin
///
/// Plugin adds a back plate scene index to the Storm render to convert 
/// back plate prims into a mesh and material
///
class HdSt_BackPlateSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdSt_BackPlateSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_BACK_PLATE_SCENE_INDEX_PLUGIN_H
