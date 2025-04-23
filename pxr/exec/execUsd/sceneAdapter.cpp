//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/sceneAdapter.h"

#include "pxr/exec/execUsd/attribute.h"
#include "pxr/exec/execUsd/object.h"
#include "pxr/exec/execUsd/prim.h"
#include "pxr/exec/execUsd/property.h"
#include "pxr/exec/execUsd/stage.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

EsfStage ExecUsdSceneAdapter::AdaptStage(const UsdStageConstRefPtr &stage)
{
    if (!stage) {
        TF_CODING_ERROR(
            "ExecUsdSceneAdapter cannot adapt a null stage pointer");
    }
    return {std::in_place_type<ExecUsd_Stage>, stage};
}

EsfStage ExecUsdSceneAdapter::AdaptStage(UsdStageConstRefPtr &&stage)
{
    if (!stage) {
        TF_CODING_ERROR(
            "ExecUsdSceneAdapter cannot adapt a null stage pointer");
    }
    return {std::in_place_type<ExecUsd_Stage>, std::move(stage)};
}

EsfObject ExecUsdSceneAdapter::AdaptObject(const UsdObject &object)
{
    return {std::in_place_type<ExecUsd_Object>, object};
}

EsfObject ExecUsdSceneAdapter::AdaptObject(UsdObject &&object)
{
    return {std::in_place_type<ExecUsd_Object>, std::move(object)};
}

EsfPrim ExecUsdSceneAdapter::AdaptPrim(const UsdPrim &prim)
{
    return {std::in_place_type<ExecUsd_Prim>, prim};
}

EsfPrim ExecUsdSceneAdapter::AdaptPrim(UsdPrim &&prim)
{
    return {std::in_place_type<ExecUsd_Prim>, std::move(prim)};
}

EsfProperty ExecUsdSceneAdapter::AdaptProperty(
    const UsdProperty &property)
{
    return {std::in_place_type<ExecUsd_Property>, property};
}

EsfProperty ExecUsdSceneAdapter::AdaptProperty(UsdProperty &&property)
{
    return {std::in_place_type<ExecUsd_Property>, std::move(property)};
}

EsfAttribute ExecUsdSceneAdapter::AdaptAttribute(
    const UsdAttribute &attribute)
{
    return {std::in_place_type<ExecUsd_Attribute>, attribute};
}

EsfAttribute ExecUsdSceneAdapter::AdaptAttribute(UsdAttribute &&attribute)
{
    return {std::in_place_type<ExecUsd_Attribute>, std::move(attribute)};
}

PXR_NAMESPACE_CLOSE_SCOPE
