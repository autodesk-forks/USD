//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (computeDensity)
    (computeMomentum)
);

// Note: This code is slightly different from what's shown in the tutorial
// documentation because the test infrastructure doesn't allow us to generate
// code for a schema and include the generated tokens header here. Therefore,
// we construct the ParamsAPI property tokens manually below.
//
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(UsdSchemaExamplesParamsAPI)
{
    // Define a computation that reads the values of the attributes params:mass
    // and params:velocity and computes the momentum.
    self.PrimComputation(_tokens->computeMomentum)
        .Callback<double>(+[](const VdfContext &context) {
            const double mass =
                context.GetInputValue<double>(TfToken("params:mass"));
            const double velocity = 
                context.GetInputValue<double>(TfToken("params:velocity"));

            return mass * velocity;
        })
        .Inputs(
            AttributeValue<double>(TfToken("params:mass")),
            AttributeValue<double>(TfToken("params:velocity"))
        );

    // Define a computation that reads the values of the attributes params:mass
    // and params:volume and computes the density.
    self.PrimComputation(_tokens->computeDensity)
        .Callback<double>(+[](const VdfContext &context) {
            const double mass =
                context.GetInputValue<double>(TfToken("params:mass"));
            const double volume =
                context.GetInputValue<double>(TfToken("params:volume"));

            return mass == 0.0 ? 0.0 : volume / mass;
        })
        .Inputs(
            AttributeValue<double>(TfToken("params:mass")),
            AttributeValue<double>(TfToken("params:volume"))
        );
}

// Test the computations registered above.
//
int main()
{
    // Load the custom schema.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "definingComputations");

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(R"usda(#usda 1.0
        def Scope "Root" (
            apiSchemas = ["ParamsAPI"]
        )
        {
            double params:mass = 2.0
            double params:velocity = 5.0
            double params:volume = 10.0
        }
    )usda");

    UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim root = usdStage->GetPrimAtPath(SdfPath("/Root"));
    TF_AXIOM(root);

    ExecUsdSystem execSystem(usdStage);

    std::vector<ExecUsdValueKey> valueKeys {
        {root, _tokens->computeMomentum},
        {root, _tokens->computeDensity}
    };

    ExecUsdRequest request = execSystem.BuildRequest(std::move(valueKeys));
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    ExecUsdCacheView view = execSystem.Compute(request);
    VtValue v;

    v = view.Get(0);
    const double momentum = v.Get<double>();
    TF_AXIOM(momentum == 10.0);

    v = view.Get(1);
    const double density = v.Get<double>();
    TF_AXIOM(density == 5.0);

    return 0;
}
