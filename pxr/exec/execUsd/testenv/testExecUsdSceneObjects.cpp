//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/sceneAdapter.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/property.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/scope.h"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE;

namespace
{

struct Fixture
{
    SdfLayerRefPtr layer;
    UsdStageConstRefPtr stage;
    EsfJournal * const journal = nullptr;

    Fixture()
    {
        layer = SdfLayer::CreateAnonymous();
        const bool importedLayer = layer->ImportFromString(R"usd(
            #sdf 1.0
            def Scope "Prim1" (
                prepend apiSchemas = ["CollectionAPI:collection1"]
            ) {
                int attr1 = 1
                int ns1:ns2:attr2 = 2
            }
            )usd");
        TF_AXIOM(importedLayer);

        stage = UsdStage::Open(layer);
        TF_AXIOM(stage);
    }
};

void TestStage(Fixture &fixture)
{
    // Tests that ExecUsd_Stages behave as UsdStage.

    const EsfStage stage = ExecUsdSceneAdapter::AdaptStage(fixture.stage);

    const EsfPrim prim = stage->GetPrimAtPath(
        SdfPath("/Prim1"), fixture.journal);
    TF_AXIOM(prim->IsValid(fixture.journal));

    const EsfAttribute attr = stage->GetAttributeAtPath(
        SdfPath("/Prim1.attr1"), fixture.journal);
    TF_AXIOM(attr->IsValid(fixture.journal));

    const EsfProperty prop = stage->GetPropertyAtPath(
        SdfPath("/Prim1.ns1:ns2:attr2"), fixture.journal);
    TF_AXIOM(prop->IsValid(fixture.journal));
}

void TestObject(Fixture &fixture)
{
    // Tests that ExecUsd_Objects behave as UsdObjects.

    const EsfObject primObject = ExecUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1")));
    TF_AXIOM(primObject->IsValid(fixture.journal));

    const EsfObject attrObject = ExecUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.attr1")));
    TF_AXIOM(attrObject->IsValid(fixture.journal));

    const EsfObject invalidObject = ExecUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Does/Not/Exist")));
    TF_AXIOM(!invalidObject->IsValid(fixture.journal));
}

void TestPrim(Fixture &fixture)
{
    // Tests that ExecUsd_Prims behave as UsdPrims.

    const EsfPrim prim = ExecUsdSceneAdapter::AdaptPrim(
        fixture.stage->GetPrimAtPath(SdfPath("/Prim1")));
    TF_AXIOM(prim->IsValid(fixture.journal));

    const EsfPrim pseudoRootPrim = prim->GetParent(fixture.journal);
    TF_AXIOM(pseudoRootPrim->IsValid(fixture.journal));
    TF_AXIOM(pseudoRootPrim->GetPath(fixture.journal) == SdfPath("/"));

    const TfType expectedType = TfType::Find<UsdGeomScope>();
    TF_AXIOM(prim->GetType(fixture.journal) == expectedType);

    const TfTokenVector expectedSchemas{ TfToken("CollectionAPI:collection1") };
    TF_AXIOM(prim->GetAppliedSchemas(fixture.journal) == expectedSchemas);

    const EsfAttribute attr = prim->GetAttribute(
        TfToken("attr1"), fixture.journal);
    TF_AXIOM(attr->IsValid(fixture.journal));
    TF_AXIOM(attr->GetPath(fixture.journal) == SdfPath("/Prim1.attr1"));
}

void TestProperty(Fixture &fixture)
{
    // Tests that ExecUsd_Properties behave as UsdProperties.

    const EsfProperty prop = ExecUsdSceneAdapter::AdaptProperty(
        fixture.stage->GetPropertyAtPath(SdfPath("/Prim1.ns1:ns2:attr2")));
    TF_AXIOM(prop->IsValid(fixture.journal));

    TF_AXIOM(prop->GetBaseName(fixture.journal) == TfToken("attr2"));
    TF_AXIOM(prop->GetNamespace(fixture.journal) == TfToken("ns1:ns2"));
}

void TestAttribute(Fixture &fixture)
{
    // Tests that ExecUsd_Attributes behave as UsdAttributes.

    const EsfAttribute attr = ExecUsdSceneAdapter::AdaptAttribute(
        fixture.stage->GetAttributeAtPath(SdfPath("/Prim1.attr1")));
    TF_AXIOM(attr->IsValid(fixture.journal));

    TF_AXIOM(attr->GetValueTypeName(fixture.journal) == SdfValueTypeNames->Int);

    VtValue value;
    TF_AXIOM(attr->Get(&value, UsdTimeCode::Default()));
    TF_AXIOM(value.IsHolding<int>());
    TF_AXIOM(value.UncheckedGet<int>() == 1);
}

}

int main()
{
    std::vector tests {
        TestStage,
        TestObject,
        TestPrim,
        TestProperty,
        TestAttribute
    };
    for (auto test : tests) {
        Fixture fixture;
        test(fixture);
    }
}
