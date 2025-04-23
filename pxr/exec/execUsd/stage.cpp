//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/stage.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/exec/execUsd/attribute.h"
#include "pxr/exec/execUsd/object.h"
#include "pxr/exec/execUsd/prim.h"
#include "pxr/exec/execUsd/property.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// EsfStage should not reserve more space than necessary.
static_assert(sizeof(ExecUsd_Stage) == sizeof(EsfStage));

ExecUsd_Stage::~ExecUsd_Stage() = default;

ExecUsd_Stage::ExecUsd_Stage(const UsdStageConstRefPtr &stage)
    : _stage(stage)
{
    TF_VERIFY(_stage);
}

ExecUsd_Stage::ExecUsd_Stage(UsdStageConstRefPtr &&stage)
    : _stage(std::move(stage))
{
    TF_VERIFY(_stage);
}

EsfAttribute ExecUsd_Stage::_GetAttributeAtPath(const SdfPath &path) const
{
    return {
        std::in_place_type<ExecUsd_Attribute>,
        _stage->GetAttributeAtPath(path)
    };
}

EsfObject ExecUsd_Stage::_GetObjectAtPath(const SdfPath &path) const
{
    return {std::in_place_type<ExecUsd_Object>, _stage->GetObjectAtPath(path)};
}

EsfPrim ExecUsd_Stage::_GetPrimAtPath(const SdfPath &path) const
{
    return {std::in_place_type<ExecUsd_Prim>, _stage->GetPrimAtPath(path)};
}

EsfProperty ExecUsd_Stage::_GetPropertyAtPath(const SdfPath &path) const
{
    return {
        std::in_place_type<ExecUsd_Property>,
        _stage->GetPropertyAtPath(path)
    };
}

PXR_NAMESPACE_CLOSE_SCOPE