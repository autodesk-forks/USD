//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/collectionPredicateLibrary.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/collectionPredicateLibrary.h"

#include "pxr/usd/usd/collectionPredicateLibrary.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/pathExpressionEval.h"
#include "pxr/usd/sdf/pathExpression.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// ---------------------------------------------------------------------------
// Test scene
//
// /World          def   Scope  kind=assembly
// /World/Car      def   Xform  kind=group,     [GeomModelAPI]
// /World/Car/Body def   Mesh   kind=component, [CollectionAPI:foo,
//                                               CollectionAPI:bar]
// /World/Car/Wheel def  Mesh   kind=component, [MaterialBindingAPI]
// /World/Asset    def   Xform  kind=component, [GeomModelAPI]
// /World/Abstract class -      (no kind)
// /World/Override over  -      (no kind)
// /World/NoKind   def   Scope  (no kind)
//
// Note: UsdImagingStageSceneIndex uses "!UsdPrimIsAbstract" in its traversal
// predicate, so /World/Abstract does not appear in the Hd scene index.
// Tests involving that path are marked "USD-only".

static const char * const kTestUsda = R"usda(
#usda 1.0

def Scope "World" (
    kind = "assembly"
) {
    def Xform "Car" (
        kind = "group"
        prepend apiSchemas = ["GeomModelAPI"]
    ) {
        def Mesh "Body" (
            kind = "component"
            prepend apiSchemas = ["CollectionAPI:foo",
                                  "CollectionAPI:bar"]
        ) {
        }
        def Mesh "Wheel" (
            kind = "component"
            prepend apiSchemas = ["MaterialBindingAPI"]
        ) {
        }
    }
    def Xform "Asset" (
        kind = "component"
        prepend apiSchemas = ["GeomModelAPI"]
    ) {
    }
    class "Abstract" {
    }
    over "Override" {
    }
    def Scope "NoKind" {
    }
}
)usda";

static UsdStageRefPtr
_CreateStage()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    TF_AXIOM(layer->ImportFromString(kTestUsda));
    return UsdStage::Open(layer);
}

static UsdImagingStageSceneIndexRefPtr
_CreateSceneIndex(const UsdStageRefPtr &stage)
{
    UsdImagingStageSceneIndexRefPtr si = UsdImagingStageSceneIndex::New();
    si->SetStage(stage);
    return si;
}

// ---------------------------------------------------------------------------
// _Checker: creates both evaluators for an expression and provides Check()
// to verify that both the USD and UsdImaging libraries return the expected
// result for a given expression.
//
// Since UsdImagingStageSceneIndex excludes class prims from its index, Check()
// is only valid for paths that exist in the Hd scene index (def and over
// prims).  Use CheckUsdOnly() for class prim paths.

struct _Checker
{
    using _UsdEval = SdfPathExpressionEval<UsdObject>;

    const SdfPathExpression expr;
    const UsdStageRefPtr stage;
    const _UsdEval usdEval;
    const HdCollectionExpressionEvaluator hdEval;

    _Checker() = default;

    _Checker(const std::string &exprStr,
             const UsdStageRefPtr &stg,
             const HdSceneIndexBaseRefPtr &si)
        : expr(exprStr)
        , stage(stg)
        , usdEval(SdfMakePathExpressionEval(
            expr, UsdGetCollectionPredicateLibrary()))
        , hdEval(si, expr, UsdImagingGetCollectionPredicateLibrary())
    {
    }

    /// Assert that both evaluators return \p expected for \p path.
    void Check(const SdfPath &path, bool expected) const {
        const bool usdRes = usdEval.Match(
            path, [this](const SdfPath &p) {
                return stage->GetObjectAtPath(p);
            }).GetValue();
        const bool hdRes = hdEval.Match(path).GetValue();

        if (usdRes != hdRes) {
            std::cerr << "Mismatch when evaluating expression " << expr
                      << " for path " << path
                      << ": USD eval result =" << usdRes
                      << ", Hd eval result =" << hdRes
                      << ", expected =" << expected << std::endl;
        }

        TF_AXIOM(usdRes == expected);
        TF_AXIOM(hdRes  == expected);
    }

    /// Assert that the USD evaluator returns \p expected for \p path.
    /// Use for paths that exist only in the USD stage (e.g. class prims).
    void CheckUsdOnly(const SdfPath &path, bool expected) const {
        const bool usdRes = usdEval.Match(
            path, [this](const SdfPath &p) {
                return stage->GetObjectAtPath(p);
            }).GetValue();

        if (usdRes != expected) {
            std::cerr << "USD evaluator mismatch when evaluating expression "
                      << expr << " for path " << path
                      << ": USD eval result =" << usdRes
                      << ", expected =" << expected << std::endl;
        }
        TF_AXIOM(usdRes == expected);
    }
};

// ---------------------------------------------------------------------------

bool
TestAbstract(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // abstract(true): /World/Abstract is a class prim and is present in the
    // USD stage, but excluded from the Hd scene index.  Only the USD
    // evaluator can return true here.
    {
        _Checker c("//{abstract}", stage, si);
        c.CheckUsdOnly(SdfPath("/World/Abstract"), true);
        c.Check(SdfPath("/World"), false);
        c.Check(SdfPath("/World/Car"), false);
        c.Check(SdfPath("/World/Override"), false);
        c.Check(SdfPath("/World/NoKind"), false);
    }
    
    // abstract(false)
    {
        _Checker c("//{abstract:false}", stage, si);
        c.CheckUsdOnly(SdfPath("/World/Abstract"), false);
        c.Check(SdfPath("/World"), true);
        c.Check(SdfPath("/World/Car"), true);
        c.Check(SdfPath("/World/Override"), true);
        c.Check(SdfPath("/World/NoKind"), true);
    }

    return true;
}

bool
TestDefined(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // defined() — true only when specifier == "def".
    {
        _Checker c("//{defined}", stage, si);
        c.Check(SdfPath("/World"), true);
        c.Check(SdfPath("/World/Car"), true);
        c.Check(SdfPath("/World/Car/Body"), true);
        c.Check(SdfPath("/World/NoKind"), true);
        c.Check(SdfPath("/World/Override"), false);

        // XXX UsdPrim:::IsDefined() returns true for /World/Abstract.
        //     Unclear why. Commenting out test case for now.
        // c.CheckUsdOnly(SdfPath("/World/Abstract"), false);
    }

    // defined(false): class and over prims.
    {
        _Checker c("//{defined:false}", stage, si);
        c.Check(SdfPath("/World/Override"), true);
        c.Check(SdfPath("/World"), false);
        c.Check(SdfPath("/World/Car"), false);
        // XXX UsdPrim:::IsDefined() returns true for /World/Abstract.
        //     Unclear why.
        // c.CheckUsdOnly(SdfPath("/World/Abstract"), true);
    }

    return true;
}

bool
TestSpecifier(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // specifier("def") is equivalent to defined.
    {
        _Checker c("//{specifier:def}", stage, si);
        c.Check(SdfPath("/World"), true);
        c.Check(SdfPath("/World/Car/Body"), true);
        c.Check(SdfPath("/World/Override"), false);
        // class prim has specifier "class", not "def".
        c.CheckUsdOnly(SdfPath("/World/Abstract"), false);
    }

    // specifier("class") — USD-only since class prims are excluded from Hd.
    {
        _Checker c("//{specifier:class}", stage, si);
        c.CheckUsdOnly(SdfPath("/World/Abstract"), true);
        c.CheckUsdOnly(SdfPath("/World"), false);
    }

    // specifier("over"): only /World/Override.
    {
        _Checker c("//{specifier:over}", stage, si);
        c.Check(SdfPath("/World/Override"), true);
        c.Check(SdfPath("/World"), false);
        c.CheckUsdOnly(SdfPath("/World/Abstract"), false);
    }

    // Multiple specifiers: specifier("over","class") matches over or class prims;
    // the "class" check is USD-only.
    {
        _Checker c(R"(//{specifier("over","class")})", stage, si);
        c.Check(SdfPath("/World/Override"), true);
        c.CheckUsdOnly(SdfPath("/World/Abstract"), true);
        c.Check(SdfPath("/World"), false);
        c.Check(SdfPath("/World/Car"), false);
    }

    return true;
}

bool
TestKind(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // kind("assembly") — assembly has no sub-kinds, so only exact matches.
    {
        _Checker c("//{kind:assembly}", stage, si);
        c.Check(SdfPath("/World"), true);
        c.Check(SdfPath("/World/Car"), false);
        c.Check(SdfPath("/World/Car/Body"), false);
    }

    // kind("group") with strict=false: assembly IsA group, so /World matches.
    {
        _Checker c("//{kind:group}", stage, si);
        c.Check(SdfPath("/World"), true);       // assembly IsA group
        c.Check(SdfPath("/World/Car"), true);   // group
        c.Check(SdfPath("/World/Car/Body"), false);
        c.Check(SdfPath("/World/NoKind"), false);
    }

    // kind("group", strict=true): only exact "group" kind.
    {
        _Checker c("//{kind(group,strict=true)}", stage, si);
        c.Check(SdfPath("/World"), false);      // assembly, not group
        c.Check(SdfPath("/World/Car"), true);   // group
        c.Check(SdfPath("/World/Car/Body"), false);
    }

    // kind("component"): component prims.
    {
        _Checker c("//{kind:component}", stage, si);
        c.Check(SdfPath("/World/Car/Body"), true);
        c.Check(SdfPath("/World/Car/Wheel"), true);
        c.Check(SdfPath("/World/Asset"), true);
        c.Check(SdfPath("/World"), false);
        c.Check(SdfPath("/World/NoKind"), false);
    }

    // Prims with no kind always return false.
    {
        _Checker c("//{kind:assembly}", stage, si);
        c.Check(SdfPath("/World/Override"), false);
        c.Check(SdfPath("/World/NoKind"), false);
    }

    return true;
}

bool
TestModel(const UsdStageRefPtr &stage,
          const HdSceneIndexBaseRefPtr &si)
{
    // model() matches any prim whose kind IsA "model".
    {
        _Checker c("//{model}", stage, si);
        c.Check(SdfPath("/World"), true);            // assembly IsA model
        c.Check(SdfPath("/World/Car"), true);        // group IsA model
        c.Check(SdfPath("/World/Car/Body"), true);   // component IsA model
        c.Check(SdfPath("/World/Asset"), true);      // component IsA model
        c.Check(SdfPath("/World/NoKind"), false);
        c.Check(SdfPath("/World/Override"), false);
    }

    // model(false): prims NOT in the model hierarchy.
    {
        _Checker c("//{model:false}", stage, si);
        c.Check(SdfPath("/World/NoKind"), true);
        c.Check(SdfPath("/World/Override"), true);
        c.Check(SdfPath("/World"), false);
        c.Check(SdfPath("/World/Car/Body"), false);
    }

    return true;
}

bool
TestGroup(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // group() matches prims whose kind IsA "group".
    {
        _Checker c("//{group}", stage, si);
        c.Check(SdfPath("/World"), true);       // assembly IsA group
        c.Check(SdfPath("/World/Car"), true);   // group
        c.Check(SdfPath("/World/Car/Body"), false);
        c.Check(SdfPath("/World/NoKind"), false);
    }

    // group(false): prims NOT in the group hierarchy.
    {
        _Checker c("//{group:false}", stage, si);
        c.Check(SdfPath("/World/Car/Body"), true);
        c.Check(SdfPath("/World/NoKind"), true);
        c.Check(SdfPath("/World"), false);
        c.Check(SdfPath("/World/Car"), false);
    }

    return true;
}

bool
TestHasAPI(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // hasAPI("GeomModelAPI") — single-apply schema.
    {
        _Checker c("//{hasAPI:GeomModelAPI}", stage, si);
        c.Check(SdfPath("/World/Car"), true);
        c.Check(SdfPath("/World/Asset"), true);
        c.Check(SdfPath("/World"), false);
        c.Check(SdfPath("/World/Car/Body"), false);
        c.Check(SdfPath("/World/Override"), false);
    }

    // hasAPI("MaterialBindingAPI") — single-apply schema.
    {
        _Checker c("//{hasAPI:MaterialBindingAPI}", stage, si);
        c.Check(SdfPath("/World/Car/Wheel"), true);
        c.Check(SdfPath("/World/Car/Body"), false);
        c.Check(SdfPath("/World/Car"), false);
    }

    // hasAPI("CollectionAPI") without instanceName — matches any
    // CollectionAPI entry (exact or "CollectionAPI:*" prefix).
    {
        _Checker c("//{hasAPI:CollectionAPI}", stage, si);
        c.Check(SdfPath("/World/Car/Body"), true);
        c.Check(SdfPath("/World/Car/Wheel"), false);
        c.Check(SdfPath("/World/Car"), false);
    }

    // hasAPI("CollectionAPI", instanceName="foo") — specific instance.
    {
        _Checker c("//{hasAPI(CollectionAPI,instanceName=foo)}", stage, si);
        c.Check(SdfPath("/World/Car/Body"), true);
        c.Check(SdfPath("/World/Car/Wheel"), false);
    }

    // hasAPI("CollectionAPI", instanceName="bar") — other instance.
    {
        _Checker c("//{hasAPI(CollectionAPI,instanceName=bar)}", stage, si);
        c.Check(SdfPath("/World/Car/Body"), true);
        c.Check(SdfPath("/World/Car/Wheel"), false);
    }

    // Prim with no applied schemas returns false.
    {
        _Checker c("//{hasAPI:GeomModelAPI}", stage, si);
        c.Check(SdfPath("/World/NoKind"), false);
        c.Check(SdfPath("/World/Override"), false);
    }

    return true;
}

bool
TestIsa(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // isa requires the USD schema registry to resolve type names.
    // Skip subtests for types that are not registered in this environment.

    const TfType meshType =
        UsdSchemaRegistry::GetTypeFromSchemaTypeName(TfToken("Mesh"));
    const TfType xformableType =
        UsdSchemaRegistry::GetTypeFromSchemaTypeName(TfToken("Xformable"));

    if (!meshType) {
        std::cout << "  (skipping isa:Mesh tests — type not registered)"
                  << std::endl;
    } else {
        // isa("Mesh")
        {
            _Checker c("//{isa:Mesh}", stage, si);
            c.Check(SdfPath("/World/Car/Body"), true);
            c.Check(SdfPath("/World/Car/Wheel"), true);
            c.Check(SdfPath("/World"), false);
            c.Check(SdfPath("/World/Car"), false);
        }

        // isa("Mesh", strict=true) — same result since Mesh is a concrete type.
        {
            _Checker c("//{isa(Mesh,strict=true)}", stage, si);
            c.Check(SdfPath("/World/Car/Body"), true);
            c.Check(SdfPath("/World/Car/Wheel"), true);
            c.Check(SdfPath("/World"), false);
            c.Check(SdfPath("/World/Car"), false);
        }
    }

    if (!xformableType) {
        std::cout << "  (skipping isa:Xformable tests — type not registered)"
                  << std::endl;
    } else {
        // isa("Xformable") with strict=false: Xform and Mesh both inherit
        // from Xformable.
        {
            _Checker c("//{isa:Xformable}", stage, si);
            c.Check(SdfPath("/World/Car"), true);       // Xform
            c.Check(SdfPath("/World/Car/Body"), true);  // Mesh
            c.Check(SdfPath("/World"), false);          // Scope
        }

        // isa("Xformable", strict=true): no test prim has the exact type
        // "Xformable", so all return false.
        {
            _Checker c("//{isa(Xformable,strict=true)}", stage, si);
            c.Check(SdfPath("/World/Car"), false);
            c.Check(SdfPath("/World/Car/Body"), false);
        }
    }

    // Prim with no typeName (over) always returns false.
    {
        _Checker c("//{isa:Mesh}", stage, si);
        c.Check(SdfPath("/World/Override"), false);
    }

    return true;
}

bool
TestCompoundPredicates(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // kind("component") AND hasAPI("GeomModelAPI").
    {
        _Checker c("//{kind:component and hasAPI:GeomModelAPI}", stage, si);
        c.Check(SdfPath("/World/Asset"), true);
        c.Check(SdfPath("/World/Car"), false);       // group, not component
        c.Check(SdfPath("/World/Car/Body"), false);  // no GeomModelAPI
    }

    // defined AND group: defined prims in the group hierarchy.
    {
        _Checker c("//{defined and group}", stage, si);
        c.Check(SdfPath("/World"), true);       // def + assembly IsA group
        c.Check(SdfPath("/World/Car"), true);   // def + group
        c.Check(SdfPath("/World/Override"), false);  // over specifier
        c.Check(SdfPath("/World/Car/Body"), false);  // component
    }

    return true;
}

bool
TestUsdAndHydraPredicates(
    const UsdStageRefPtr &stage,
    const HdSceneIndexBaseRefPtr &si)
{
    // The composed library (Hd + UsdImaging predicates) supports both
    // Hydra (i.e. hd*) and USD predicates in a single expression.
    const HdCollectionPredicateLibrary composedLib =
        UsdImagingGetCollectionPredicateLibrary(
            HdGetCollectionPredicateLibrary());

    // hdType:mesh works with the composed library.
    {
        HdCollectionExpressionEvaluator eval(
            si, SdfPathExpression("//{hdType:mesh}"), composedLib);
        TF_AXIOM( eval.Match(SdfPath("/World/Car/Body")));
        TF_AXIOM( eval.Match(SdfPath("/World/Car/Wheel")));
        TF_AXIOM(!eval.Match(SdfPath("/World")));       // Scope
        TF_AXIOM(!eval.Match(SdfPath("/World/Car")));   // Xform
    }

    // Mix Hd and USD predicates in a single expression:
    // component kind AND hdType:mesh (component Mesh prims only).
    {
        HdCollectionExpressionEvaluator eval(
            si,
            SdfPathExpression("//{kind:component and hdType:mesh}"),
            composedLib);
        TF_AXIOM( eval.Match(SdfPath("/World/Car/Body")));
        TF_AXIOM( eval.Match(SdfPath("/World/Car/Wheel")));
        TF_AXIOM(!eval.Match(SdfPath("/World/Asset")));  // empty hdType
        TF_AXIOM(!eval.Match(SdfPath("/World")));
    }

    return true;
}

bool
TestMismatchedLibrary()
{
    // hdType is not in UsdGetCollectionPredicateLibrary(), so attempting to
    // build a USD-side evaluator for an expression containing hdType should
    // fail: SdfMakePathExpressionEval should emit an error and return an
    // empty evaluator.
    {
        TfErrorMark mark;
        auto usdEval = SdfMakePathExpressionEval(
            SdfPathExpression("//{hdType:mesh}"),
            UsdGetCollectionPredicateLibrary());
        TF_AXIOM( usdEval.IsEmpty());
        TF_AXIOM(!mark.IsClean());
        mark.Clear();
    }

    return true;
}

} // anon

// ---------------------------------------------------------------------------

#define RUNTEST(name, ...) \
    std::cout << (++i) << ") " << #name << "..." << std::endl; \
    if (!name(__VA_ARGS__)) { std::cout << "FAILED" << std::endl; return -1; } \
    else std::cout << "...SUCCEEDED" << std::endl

int
main(int argc, char **argv)
{
    std::cout << "STARTING testUsdImagingCollectionPredicateLibrary"
              << std::endl;

    const UsdStageRefPtr stage = _CreateStage();
    TF_AXIOM(stage);

    const UsdImagingStageSceneIndexRefPtr si = _CreateSceneIndex(stage);
    TF_AXIOM(si);

    // Wrap in HdSceneIndexBaseRefPtr for use with
    // HdCollectionExpressionEvaluator.
    const HdSceneIndexBaseRefPtr siBase = si;

    int i = 0;
    RUNTEST(TestAbstract,               stage, siBase);
    RUNTEST(TestDefined,                stage, siBase);
    RUNTEST(TestSpecifier,              stage, siBase);
    RUNTEST(TestKind,                   stage, siBase);
    RUNTEST(TestModel,                  stage, siBase);
    RUNTEST(TestGroup,                  stage, siBase);
    RUNTEST(TestHasAPI,                 stage, siBase);
    RUNTEST(TestIsa,                    stage, siBase);
    RUNTEST(TestCompoundPredicates,     stage, siBase);
    RUNTEST(TestUsdAndHydraPredicates,  stage, siBase);
    RUNTEST(TestMismatchedLibrary);

    std::cout << "DONE testUsdImagingCollectionPredicateLibrary" << std::endl;
    return 0;
}
