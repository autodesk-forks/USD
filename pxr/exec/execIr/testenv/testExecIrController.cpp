//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/controllerBuilder.h"
#include "pxr/exec/execIr/types.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/execUsd/cacheView.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"
#include "pxr/exec/vdf/context.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_EQ(expr, expected)                                              \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (expr_ != expected) {                                               \
            std::cout << std::flush;                                           \
            std::cerr << std::flush;                                           \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (input)
    (output)
);

static ExecIrResult _ForwardCompute(const VdfContext & ctx);

static ExecIrResult _InverseCompute(const VdfContext & ctx);

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecIrControllerAddOneController)
{
    ExecIrControllerBuilder builder(self, &_ForwardCompute, &_InverseCompute);

    builder.InvertibleInputAttribute<double>(_tokens->input);
    builder.InvertibleOutputAttribute<double>(_tokens->output);
}

static ExecIrResult
_ForwardCompute(const VdfContext & ctx)
{
    // Extract the input value.
    const double input = ctx.GetInputValue<double>(_tokens->input);

    // Create a map to store the results.
    ExecIrResult result;

    // Compute and store the output value.
    result[_tokens->output] = input + 1.0;

    return result;
}

// The inverse compute callback function.
//
// The context provides desired values for all invertible outputs. The
// function is responsible for computing the invertible input values that
// satisfy the desired output values, returning the values in a map from
// invertible input name to VtValue.
//
static ExecIrResult
_InverseCompute(const VdfContext & ctx)
{
    // Extract the output value.
    const double output = ctx.GetInputValue<double>(_tokens->output);

    // Create a map to store the results
    ExecIrResult result;

    // Compute and store the input value
    result[_tokens->input] = output - 1.0;

    return result;
}

static void
Test_ForwardCompute()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddOneController "PlusOne" {
            double input = 10.0
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/PlusOne"));
    TF_AXIOM(prim);
    const UsdAttribute output = prim.GetAttribute(_tokens->output);
    TF_AXIOM(output);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{output, ExecBuiltinComputations->computeValue}
    });
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        TfErrorMark mark;

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());

        ASSERT_EQ(value.Get<double>(), 11.0);

        TF_AXIOM(mark.IsClean());
    }

    // Now set the input and compute again.
    {
        const UsdAttribute input = prim.GetAttribute(_tokens->input);
        TF_AXIOM(input);
        input.Set(2.0);

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        ASSERT_EQ(value.Get<double>(), 3.0);
    }
}

static void
Test_InverseCompute()
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(
        R"usda(
        #usda 1.0

        def AddOneController "PlusOne" {
            double output = 10.0
        }
        )usda");
    const UsdStageConstRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    const UsdPrim prim = usdStage->GetPrimAtPath(SdfPath("/PlusOne"));
    TF_AXIOM(prim);
    const UsdAttribute output = prim.GetAttribute(_tokens->output);
    TF_AXIOM(output);

    ExecUsdSystem execSystem(usdStage);
    const ExecUsdRequest request = execSystem.BuildRequest({
        ExecUsdValueKey{prim, ExecIrTokens->inverseCompute}
    });
    TF_AXIOM(request.IsValid());

    execSystem.PrepareRequest(request);
    TF_AXIOM(request.IsValid());

    {
        TfErrorMark mark;

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        const ExecIrResult valueMap =
            value.Get<ExecIrResult>();

        const std::vector<std::pair<const char *, double>> expected{{
            {"input", 9.0},
        }};
        for (const auto &entry : expected) {
            const auto it = valueMap.find(TfToken(entry.first));
            TF_AXIOM(it != valueMap.end());
            ASSERT_EQ(it->second.Get<double>(), entry.second);
        }

        TF_AXIOM(mark.IsClean());
    }

    // Now set the parent space and compute again.
    {
        const UsdAttribute output = prim.GetAttribute(_tokens->output);
        TF_AXIOM(output);
        output.Set(3.0);

        ExecUsdCacheView cache = execSystem.Compute(request);
        const VtValue value = cache.Get(0);
        TF_AXIOM(!value.IsEmpty());
        const ExecIrResult valueMap =
            value.Get<ExecIrResult>();

        const std::vector<std::pair<const char *, double>> expected{{
            {"input", 2.0},
        }};
        for (const auto &entry : expected) {
            const auto it = valueMap.find(TfToken(entry.first));
            TF_AXIOM(it != valueMap.end());
            ASSERT_EQ(it->second.Get<double>(), entry.second);
        }
    }
}

int main(int argc, char **argv)
{
    // Load the custom schema.
    const PlugPluginPtrVector testPlugins =
        PlugRegistry::GetInstance().RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecIrController");

    Test_ForwardCompute();
    Test_InverseCompute();
}
