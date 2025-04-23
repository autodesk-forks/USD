//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/tokens.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (attributeComputation)
    (attributeName)
    (emptyComputation)
    (missingComputation)
    (namespaceAncestorInput)
    (noInputsComputation)
    (primComputation)
);

// A type that is not registered with TfType.
struct TestUnknownType {};

EXEC_REGISTER_SCHEMA(TestUnknownType)
{
    self.PrimComputation(_tokens->noInputsComputation)
        .Callback<double>(+[](const VdfContext &) { return 1.0; });
}

// A "schema" type to register computations against.
struct TestSchemaType {};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TestSchemaType>();
}

EXEC_REGISTER_SCHEMA(TestSchemaType)
{
    self.PrimComputation(_tokens->emptyComputation);

    self.PrimComputation(_tokens->noInputsComputation)
        .Callback(+[](const VdfContext &) { return 1.0; });

    self.PrimComputation(_tokens->primComputation)
        .Callback<double>([](const VdfContext &ctx) { ctx.SetOutput(11.0); })
        .Inputs(
            Computation<double>(_tokens->primComputation),
            Attribute(_tokens->attributeName)
                .Computation<int>(_tokens->attributeComputation),
            AttributeValue<int>(_tokens->attributeName)
                .Required(),
            NamespaceAncestor<bool>(_tokens->primComputation)
                .InputName(_tokens->namespaceAncestorInput)
        );
}

// XXX:TODO
#if 0

// Test that clients can register schemas inside their own namespaces.

namespace client_namespace {

struct TestNamespacedSchemaType {};


EXEC_REGISTER_SCHEMA(TestNamespacedSchemaType)
{
    self.PrimComputation(_tokens->noInputsComputation)
        .Callback(+[](const VdfContext &) { return 1.0; });
}

} // namespace client_namespace

#endif

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()

static void
_PrintInputKeys(
    const TfSmallVector<Exec_InputKey, 1> &inputKeys)
{
    std::cout << "\nPrinting " << inputKeys.size() << " input keys:\n";

    for (const Exec_InputKey &key : inputKeys) {
        std::cout << "\nkey:\n";
        std::cout << "  input name: " << key.inputName << "\n";
        std::cout << "  computation name: " << key.computationName << "\n";
        std::cout << "  result type: " << key.resultType << "\n";
        std::cout << "  local traversal path: "
                  << key.providerResolution.localTraversal << "\n";
        std::cout << "  traversal: "
                  << static_cast<int>(key.providerResolution.dynamicTraversal)
                  << "\n";
        std::cout << "  optional: " << key.optional << "\n";
    }
}

static void
TestUnknownSchemaType()
{
    static const TfType schemaType = TfType::Find<TestUnknownType>();
    TF_AXIOM(schemaType.IsUnknown());

    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();

    const Exec_ComputationDefinition *const primCompDef =
        reg.GetPrimComputationDefinition(
            schemaType, _tokens->noInputsComputation);
    TF_AXIOM(!primCompDef);
}

static void
TestComputationRegistration()
{
    static const TfType schemaType = TfType::Find<TestSchemaType>();
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();

    {
        // Look up a computation that wasn't registered.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetPrimComputationDefinition(
                schemaType, _tokens->missingComputation);
        TF_AXIOM(!primCompDef);
    }

    {
        // Look up a computation with no callback or inputs.
        //
        // (Once we support composition of computation definitions, we will
        // want some kind of validation to ensure we end up with a callback.)
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetPrimComputationDefinition(
                schemaType, _tokens->emptyComputation);
        TF_AXIOM(primCompDef);

        TF_AXIOM(!primCompDef->GetCallback());
        ASSERT_EQ(primCompDef->GetInputKeys().size(), 0);
    }

    {
        // Look up a computation with no inputs.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetPrimComputationDefinition(
                schemaType, _tokens->noInputsComputation);
        TF_AXIOM(primCompDef);

        TF_AXIOM(primCompDef->GetCallback());
        ASSERT_EQ(primCompDef->GetInputKeys().size(), 0);
    }

    {
        // Look up a computation with multiple inputs.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetPrimComputationDefinition(
                schemaType, _tokens->primComputation);
        TF_AXIOM(primCompDef);

        TF_AXIOM(primCompDef->GetCallback());

        const auto inputKeys = primCompDef->GetInputKeys();
        ASSERT_EQ(inputKeys.size(), 4);

        _PrintInputKeys(inputKeys);

        size_t index = 0;
        {
            const Exec_InputKey &key = inputKeys[index++];
            ASSERT_EQ(key.inputName, _tokens->primComputation);
            ASSERT_EQ(key.computationName, _tokens->primComputation);
            ASSERT_EQ(key.resultType, TfType::Find<double>());
            ASSERT_EQ(key.providerResolution.localTraversal, SdfPath("."));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::Local);
            ASSERT_EQ(key.optional, true);
        }

        {
            const Exec_InputKey &key = inputKeys[index++];
            ASSERT_EQ(key.inputName, _tokens->attributeComputation);
            ASSERT_EQ(key.computationName, _tokens->attributeComputation);
            ASSERT_EQ(key.resultType, TfType::Find<int>());
            ASSERT_EQ(key.providerResolution.localTraversal,
                      SdfPath(".attributeName"));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::Local);
            ASSERT_EQ(key.optional, true);
        }

        {
            const Exec_InputKey &key = inputKeys[index++];
            ASSERT_EQ(key.inputName, _tokens->attributeName);
            ASSERT_EQ(
                key.computationName, ExecBuiltinComputations->computeValue);
            ASSERT_EQ(key.resultType, TfType::Find<int>());
            ASSERT_EQ(key.providerResolution.localTraversal,
                      SdfPath(".attributeName"));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::Local);
            ASSERT_EQ(key.optional, false);
        }

        {
            const Exec_InputKey &key = inputKeys[index++];
            ASSERT_EQ(key.inputName, _tokens->namespaceAncestorInput);
            ASSERT_EQ(key.computationName, _tokens->primComputation);
            ASSERT_EQ(key.resultType, TfType::Find<bool>());
            ASSERT_EQ(key.providerResolution.localTraversal, SdfPath("."));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::
                          NamespaceAncestor);
            ASSERT_EQ(key.optional, true);
        }
    }
}

int main()
{
    TestUnknownSchemaType();

    TestComputationRegistration();

    return 0;
}
