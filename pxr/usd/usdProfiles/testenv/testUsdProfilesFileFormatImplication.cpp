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
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/dictionary.h"

#include <cstdio>
#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using QS = UsdProfileRegistry::QueryStatus;

//----------------------------------------------------------------------------
// SetupPlugins
//
// Registers the side-installed TestUsdProfilesPhonyFileFormat plugin. The
// build system installs it to UsdProfilesPlugins/lib/... next to this test.
//----------------------------------------------------------------------------

static void
SetupPlugins()
{
    const std::string pluginPath = TfStringCatPaths(
        TfGetPathName(ArchGetExecutablePath()),
        "UsdProfilesPlugins/lib/TestUsdProfilesPhonyFileFormat*/Resources/") + "/";

    PlugPluginPtrVector plugins =
        PlugRegistry::GetInstance().RegisterPlugins(pluginPath);

    TF_AXIOM(plugins.size() == 1);
    TF_AXIOM(plugins[0]->GetName() == "TestUsdProfilesPhonyFileFormat");
}

//----------------------------------------------------------------------------
// TestPhonyCapabilityGraph
//
// Verifies that TestUsdProfilesPhonyFileFormat_plugInfo.json seeds test,
// test.phony, and test.phony.fileformat in the capability graph.
//----------------------------------------------------------------------------

static void
TestPhonyCapabilityGraph()
{
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("test")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("test.phony")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(
        TfToken("test.phony.fileformat")));

    // test.phony.fileformat transitively depends on test.
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("test.phony.fileformat"), TfToken("test"))
        == QS::ValidPath);

    printf("  TestPhonyCapabilityGraph passed\n");
}

//----------------------------------------------------------------------------
// TestFileFormatRegistryQuery
//
// Verifies GetFileFormatImpliedCapabilities returns the expected capability
// set for UsdProfilesTestPhonyFileFormat, and that unknown types return empty.
//----------------------------------------------------------------------------

static void
TestFileFormatRegistryQuery()
{
    // Unknown type returns empty.
    std::set<TfToken> empty =
        UsdProfileRegistry::GetFileFormatImpliedCapabilities(
            TfType::GetUnknownType());
    TF_AXIOM(empty.empty());

    // UsdProfilesTestPhonyFileFormat should imply test.phony.fileformat.
    TfType fmt = TfType::FindByName("UsdProfilesTestPhonyFileFormat");
    TF_AXIOM(!fmt.IsUnknown());

    std::set<TfToken> caps =
        UsdProfileRegistry::GetFileFormatImpliedCapabilities(fmt);
    TF_AXIOM(caps.count(TfToken("test.phony.fileformat")));

    // Cache: second call returns identical result.
    std::set<TfToken> caps2 =
        UsdProfileRegistry::GetFileFormatImpliedCapabilities(fmt);
    TF_AXIOM(caps == caps2);

    printf("  TestFileFormatRegistryQuery passed\n");
}

//----------------------------------------------------------------------------
// TestFileFormatAggregation
//
// Verifies that PopulateCapabilityUsages() picks up
// test.phony.fileformat when the stage has a .phony sublayer, with a default
// strength of "hard".
//----------------------------------------------------------------------------

static void
TestFileFormatAggregation(const std::string& phonyPath)
{
    // Stage with a .phony sublayer: capability appears at default "hard".
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->GetRootLayer()->GetSubLayerPaths().push_back(phonyPath);
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();

        TF_AXIOM(merged.count("test.phony.fileformat") == 1);
        TF_AXIOM(merged["test.phony.fileformat"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->hard);
        TF_AXIOM(claims.GetCapabilityUsages() == merged);
    }

    // In-memory stage with no .phony sublayer: capability absent.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();

        TF_AXIOM(merged.count("test.phony.fileformat") == 0);
    }

    // Mixed: schema capability + file format capability in one traversal.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->GetRootLayer()->GetSubLayerPaths().push_back(phonyPath);
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
        UsdColorSpaceAPI::Apply(root);

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        VtDictionary merged = claims.PopulateCapabilityUsages();

        TF_AXIOM(merged.count("usd.core.colorspace") == 1);
        TF_AXIOM(merged.count("test.phony.fileformat") == 1);
        TF_AXIOM(merged["usd.core.colorspace"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->hard);
        TF_AXIOM(merged["test.phony.fileformat"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->hard);
    }

    // Authored strength wins: user declares enhancement for the file-format
    // implied capability before Populate runs.
    {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->GetRootLayer()->GetSubLayerPaths().push_back(phonyPath);
        UsdPrim root = stage->DefinePrim(SdfPath("/Root"));

        UsdProfilesClaimsAPI claims = UsdProfilesClaimsAPI::Apply(root);
        claims.SetCapabilityUsage(
            TfToken("test.phony.fileformat"), UsdProfilesTokens->enhancement);

        VtDictionary merged = claims.PopulateCapabilityUsages();
        TF_AXIOM(merged["test.phony.fileformat"].UncheckedGet<TfToken>()
                 == UsdProfilesTokens->enhancement);
    }

    printf("  TestFileFormatAggregation passed\n");
}

int
main(int argc, char* argv[])
{
    SetupPlugins();

    // The TESTENV copies minimal.phony to CWD.
    const std::string phonyPath = "minimal.phony";

    printf("Running testUsdProfilesFileFormatImplication\n");
    TestPhonyCapabilityGraph();
    TestFileFormatRegistryQuery();
    TestFileFormatAggregation(phonyPath);
    printf("All tests passed.\n");
    return 0;
}
