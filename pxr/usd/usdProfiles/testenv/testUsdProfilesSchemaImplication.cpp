//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/claimsAPI.h"
#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/usd/usdProfiles/tokens.h"
#include "pxr/usd/usd/colorSpaceAPI.h"
#include "pxr/usd/usd/colorSpaceDefinitionAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/dictionary.h"

#include <cstdio>
#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------
// TestSeededCapabilityGraph
//
// Verifies that pxr/usd/usd/plugInfo.json seeds the capability graph with the
// three foundation capabilities introduced by this change.
//----------------------------------------------------------------------------

static void
TestSeededCapabilityGraph()
{
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd.core")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd.core.colorspace")));

    // usd.core.colorspace transitively depends on usd
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd.core.colorspace"), TfToken("usd"))
        == UsdProfileRegistry::QueryStatus::ValidPath);

    printf("  TestSeededCapabilityGraph passed\n");
}

//----------------------------------------------------------------------------
// TestDirectRegistryQuery
//
// Verifies GetSchemaImpliedCapabilities returns the correct capability set
// for each annotated color-space schema, and returns empty for unannotated
// schemas.
//----------------------------------------------------------------------------

static void
TestDirectRegistryQuery()
{
    // UsdColorSpaceAPI (single-apply)
    {
        TfType t = UsdSchemaRegistry::GetTypeFromName(TfToken("ColorSpaceAPI"));
        TF_AXIOM(!t.IsUnknown());
        std::set<TfToken> caps =
            UsdProfileRegistry::GetSchemaImpliedCapabilities(t);
        TF_AXIOM(caps.count(TfToken("usd.core.colorspace")) == 1);
    }

    // UsdColorSpaceDefinitionAPI (multi-apply)
    {
        TfType t = UsdSchemaRegistry::GetTypeFromName(
            TfToken("ColorSpaceDefinitionAPI"));
        TF_AXIOM(!t.IsUnknown());
        std::set<TfToken> caps =
            UsdProfileRegistry::GetSchemaImpliedCapabilities(t);
        TF_AXIOM(caps.count(TfToken("usd.core.colorspace")) == 1);
    }

    // Unknown TfType → empty
    {
        std::set<TfToken> caps =
            UsdProfileRegistry::GetSchemaImpliedCapabilities(TfType());
        TF_AXIOM(caps.empty());
    }

    // Unannotated schema (ClipsAPI) → usd.core.colorspace not present
    {
        TfType t = UsdSchemaRegistry::GetTypeFromName(TfToken("ClipsAPI"));
        TF_AXIOM(!t.IsUnknown());
        std::set<TfToken> caps =
            UsdProfileRegistry::GetSchemaImpliedCapabilities(t);
        TF_AXIOM(caps.count(TfToken("usd.core.colorspace")) == 0);
    }

    // Calling twice returns identical results (cache path)
    {
        TfType t = UsdSchemaRegistry::GetTypeFromName(TfToken("ColorSpaceAPI"));
        std::set<TfToken> caps1 =
            UsdProfileRegistry::GetSchemaImpliedCapabilities(t);
        std::set<TfToken> caps2 =
            UsdProfileRegistry::GetSchemaImpliedCapabilities(t);
        TF_AXIOM(caps1 == caps2);
    }

    printf("  TestDirectRegistryQuery passed\n");
}

//----------------------------------------------------------------------------
// TestMultiApplyDedupe
//
// Applying the same multi-apply schema N times must produce each capability
// exactly once in the aggregated output, defaulted to "hard".
//----------------------------------------------------------------------------

static void
TestMultiApplyDedupe()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
    UsdPrim leaf = stage->DefinePrim(SdfPath("/Root/Leaf"));

    TF_AXIOM(UsdColorSpaceDefinitionAPI::Apply(leaf, TfToken("foo")));
    TF_AXIOM(UsdColorSpaceDefinitionAPI::Apply(leaf, TfToken("bar")));

    UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
    VtDictionary merged = claims.PopulateCapabilityUsages();

    TF_AXIOM(merged.count("usd.core.colorspace") == 1);
    TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
             == UsdProfilesTokens->hard);

    // No duplicate keys — dictionary entries are unique by definition, but
    // verify the total entry count is exactly what's expected.
    size_t colorspaceEntries = 0;
    for (const auto& entry : merged) {
        if (entry.first.find("colorspace") != std::string::npos) {
            ++colorspaceEntries;
        }
    }
    TF_AXIOM(colorspaceEntries == 1);

    printf("  TestMultiApplyDedupe passed\n");
}

//----------------------------------------------------------------------------
// TestDefaultStrengthIsHard
//
// An implied capability that the user has not declared a strength for should
// default to "hard" in the merged dictionary.
//----------------------------------------------------------------------------

static void
TestDefaultStrengthIsHard()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
    UsdPrim leaf = stage->DefinePrim(SdfPath("/Root/Leaf"));
    TF_AXIOM(UsdColorSpaceAPI::Apply(leaf));

    UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
    VtDictionary merged = claims.PopulateCapabilityUsages();

    TF_AXIOM(merged.count("usd.core.colorspace") == 1);
    TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
             == UsdProfilesTokens->hard);

    printf("  TestDefaultStrengthIsHard passed\n");
}

//----------------------------------------------------------------------------
// TestAuthoredStrengthsAreRespected
//
// If the user has authored a strength for a capability before Populate is
// called, that authored strength survives. Capabilities the user has not
// declared a strength for default to "hard".
//----------------------------------------------------------------------------

static void
TestAuthoredStrengthsAreRespected()
{
    // User declares enhancement for an implied capability.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdPrim leaf = stage->DefinePrim(SdfPath("/Root/Leaf"));
        TF_AXIOM(UsdColorSpaceAPI::Apply(leaf));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        // User says: "for this application, colorspace is just an enhancement."
        claims.SetCapabilityUsage(
            TfToken("usd.core.colorspace"), UsdProfilesTokens->enhancement);

        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged.count("usd.core.colorspace") == 1);
        TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
                 == UsdProfilesTokens->enhancement);
    }

    // User declares soft for an implied capability.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdPrim leaf = stage->DefinePrim(SdfPath("/Root/Leaf"));
        TF_AXIOM(UsdColorSpaceAPI::Apply(leaf));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        claims.SetCapabilityUsage(
            TfToken("usd.core.colorspace"), UsdProfilesTokens->soft);

        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
                 == UsdProfilesTokens->soft);
    }

    // User explicitly declares hard — round-trips as hard (matches default).
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdPrim leaf = stage->DefinePrim(SdfPath("/Root/Leaf"));
        TF_AXIOM(UsdColorSpaceAPI::Apply(leaf));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        claims.SetCapabilityUsage(
            TfToken("usd.core.colorspace"), UsdProfilesTokens->hard);

        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
                 == UsdProfilesTokens->hard);
    }

    printf("  TestAuthoredStrengthsAreRespected passed\n");
}

//----------------------------------------------------------------------------
// TestEndToEndAggregator
//----------------------------------------------------------------------------

static void
TestEndToEndAggregator()
{
    // Basic aggregation: schema applied to a deep descendant propagates up.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdPrim leaf = stage->DefinePrim(SdfPath("/Root/A/B"));
        TF_AXIOM(UsdColorSpaceAPI::Apply(leaf));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();

        TF_AXIOM(merged.size() == 1);
        TF_AXIOM(merged.count("usd.core.colorspace"));
        TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
                 == UsdProfilesTokens->hard);
        // Stored metadata matches the returned dictionary.
        TF_AXIOM(claims.GetCapabilityUsages() == merged);
    }

    // Empty namespace → empty result.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged.empty());
    }

    // Schema applied to the root prim itself is included.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        TF_AXIOM(UsdColorSpaceAPI::Apply(root));
        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged.count("usd.core.colorspace") == 1);
    }

    // Schemas on multiple siblings are all collected.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdPrim a    = stage->DefinePrim(SdfPath("/Root/A"));
        UsdPrim b    = stage->DefinePrim(SdfPath("/Root/B"));
        TF_AXIOM(UsdColorSpaceAPI::Apply(a));
        TF_AXIOM(UsdColorSpaceDefinitionAPI::Apply(b, TfToken("myCs")));
        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged.count("usd.core.colorspace") == 1);
        TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
                 == UsdProfilesTokens->hard);
    }

    // Pre-authored entries for capabilities still implied by the prim
    // survive verbatim through Populate.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        TF_AXIOM(UsdColorSpaceAPI::Apply(root));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        claims.SetCapabilityUsage(
            TfToken("usd.core.colorspace"), UsdProfilesTokens->soft);

        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged["usd.core.colorspace"].Get<TfToken>()
                 == UsdProfilesTokens->soft);
    }

    printf("  TestEndToEndAggregator passed\n");
}

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------

int
main(int argc, char* argv[])
{
    printf("testUsdProfilesSchemaImplication\n");

    TestSeededCapabilityGraph();
    TestDirectRegistryQuery();
    TestMultiApplyDedupe();
    TestDefaultStrengthIsHard();
    TestAuthoredStrengthsAreRespected();
    TestEndToEndAggregator();

    printf("All tests passed\n");
    return 0;
}
