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
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/dictionary.h"

// Include the file format header so TfType is registered before our queries.
#include "pxr/usd/usdMtlx/fileFormat.h"

#include <cstdio>
#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using QS = UsdProfileRegistry::QueryStatus;

//----------------------------------------------------------------------------
// TestMtlxCapabilityGraph
//
// Verifies that usdMtlx/plugInfo.json seeds usd.mtlx and
// usd.mtlx.fileformat in the capability graph.
//----------------------------------------------------------------------------

static void
TestMtlxCapabilityGraph()
{
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd.mtlx")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd.mtlx.fileformat")));

    // usd.mtlx.fileformat transitively depends on usd
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd.mtlx.fileformat"), TfToken("usd"))
        == QS::ValidPath);

    printf("  TestMtlxCapabilityGraph passed\n");
}

//----------------------------------------------------------------------------
// TestFileFormatRegistryQuery
//
// Verifies GetFileFormatImpliedCapabilities returns the expected capability
// set for UsdMtlxFileFormat, and that unknown types return empty.
//----------------------------------------------------------------------------

static void
TestFileFormatRegistryQuery()
{
    // Unknown type returns empty.
    std::set<TfToken> empty =
        UsdProfileRegistry::GetFileFormatImpliedCapabilities(
            TfType::GetUnknownType());
    TF_AXIOM(empty.empty());

    // UsdMtlxFileFormat should imply usd.mtlx.fileformat.
    TfType fmt = TfType::FindByName("UsdMtlxFileFormat");
    TF_AXIOM(!fmt.IsUnknown());

    std::set<TfToken> caps =
        UsdProfileRegistry::GetFileFormatImpliedCapabilities(fmt);
    TF_AXIOM(caps.count(TfToken("usd.mtlx.fileformat")));

    // Cache: second call returns identical result.
    std::set<TfToken> caps2 =
        UsdProfileRegistry::GetFileFormatImpliedCapabilities(fmt);
    TF_AXIOM(caps == caps2);

    printf("  TestFileFormatRegistryQuery passed\n");
}

//----------------------------------------------------------------------------
// TestFileFormatAggregation
//
// Verifies that PopulateCapabilityUsages() picks up usd.mtlx.fileformat
// when the stage has a .mtlx sublayer, at default strength "hard", and that
// authored strengths survive.
//----------------------------------------------------------------------------

static void
TestFileFormatAggregation(const std::string& mtlxPath)
{
    // Stage with a .mtlx sublayer: capability defaults to "hard".
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->GetRootLayer()->GetSubLayerPaths().push_back(mtlxPath);
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();

        TF_AXIOM(merged.count("usd.mtlx.fileformat") == 1);
        TF_AXIOM(merged["usd.mtlx.fileformat"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->hard);
        TF_AXIOM(claims.GetCapabilityUsages() == merged);
    }

    // In-memory stage with no .mtlx sublayer: capability absent.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();

        TF_AXIOM(merged.count("usd.mtlx.fileformat") == 0);
    }

    // Mixed: schema capability + file format capability in one traversal.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->GetRootLayer()->GetSubLayerPaths().push_back(mtlxPath);
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdColorSpaceAPI::Apply(root);

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();

        TF_AXIOM(merged.count("usd.core.colorspace") == 1);
        TF_AXIOM(merged.count("usd.mtlx.fileformat") == 1);
        TF_AXIOM(merged["usd.core.colorspace"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->hard);
        TF_AXIOM(merged["usd.mtlx.fileformat"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->hard);
    }

    // Authored strength wins: user declares enhancement before Populate runs.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->GetRootLayer()->GetSubLayerPaths().push_back(mtlxPath);
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        claims.SetCapabilityUsage(
            TfToken("usd.mtlx.fileformat"), UsdProfilesTokens->enhancement);

        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged["usd.mtlx.fileformat"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->enhancement);
    }

    printf("  TestFileFormatAggregation passed\n");
}

int
main(int argc, char* argv[])
{
    // The TESTENV copies minimal.mtlx to CWD.
    const std::string mtlxPath = "minimal.mtlx";

    printf("Running testUsdMtlxFileFormatImplication\n");
    TestMtlxCapabilityGraph();
    TestFileFormatRegistryQuery();
    TestFileFormatAggregation(mtlxPath);
    printf("All tests passed.\n");
    return 0;
}
