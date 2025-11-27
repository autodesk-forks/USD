//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/vt/value.h"
#include "pxr/exec/execGeom/tokens.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"

#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE;

void Example()
{
    // Open the layer that contains our scene on a UsdStage.
    const UsdStageRefPtr stage = UsdStage::Open("xformPrims.usda");

    // Create an ExecUsdSystem, which we will use to evaluate computations on
    // the stage.
    ExecUsdSystem execSystem(stage);

    // Create a vector of value keys that indicate which computed values we are
    // requesting for evaluation.
    std::vector<ExecUsdValueKey> valueKeys {
        {stage->GetPrimAtPath(SdfPath("/Root/A1")),
         ExecGeomXformableTokens->computeLocalToWorldTransform},
        {stage->GetPrimAtPath(SdfPath("/Root/A2")),
         ExecGeomXformableTokens->computeLocalToWorldTransform},
    };

    // Build the request.
    const ExecUsdRequest request =
        execSystem.BuildRequest(std::move(valueKeys));

    // Prepare the request, ensuring the data flow graph is compiled and the
    // schedule is created.
    execSystem.PrepareRequest(request);

    // Evaluate the data flow graph according to the schedule, to yield the
    // requested computed values.
    ExecUsdCacheView cache = execSystem.Compute(request);

    // Extract the values.
    VtValue value;
    value = cache.Get(0);
    const GfMatrix4d a1LocalToWorld = value.Get<GfMatrix4d>();
    value = cache.Get(1);
    const GfMatrix4d a2LocalToWorld = value.Get<GfMatrix4d>();

    // The resulting matrices are the concatenation of the transforms
    // authored on A1 and Root and A2 and Root, respectively. Here, we
    // extract the translations from the resulting matrices, demonstrating
    // that we end up with the expected net translations necessary to
    // translate points in each local space into world space.
    TF_AXIOM(GfIsClose(
        a1LocalToWorld.ExtractTranslation(), GfVec3d(1, 2, 0), 1e-6));
    TF_AXIOM(GfIsClose(
        a2LocalToWorld.ExtractTranslation(), GfVec3d(1, 0, 3), 1e-6));
}

int main()
{
    Example();
    
    return 0;
}
