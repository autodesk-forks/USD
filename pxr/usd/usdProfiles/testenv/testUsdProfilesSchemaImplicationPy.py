#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Tf, Usd, UsdProfiles

PR = UsdProfiles.ProfileRegistry
CA = UsdProfiles.ClaimsAPI


class TestSeededCapabilityGraph(unittest.TestCase):
    """Verify that pxr/usd/usd/plugInfo.json seeds the capability graph."""

    def test_RootCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("usd"))

    def test_UsdLibraryCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("usd.core"))

    def test_ColorspaceCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("usd.core.colorspace"))

    def test_ColorspaceTransitivelyReachesUsd(self):
        # usd.core.colorspace -> usd.core -> usd (transitive)
        status = PR.HasPredecessor("usd.core.colorspace", "usd")
        self.assertEqual(status, PR.QueryStatus.ValidPath)


class TestDirectRegistryQuery(unittest.TestCase):
    """Verify GetSchemaImpliedCapabilities for known annotated schemas.

    The registry returns just the set of implied capability names; strengths
    are recorded by the consumer on its ClaimsAPI, not on the implication.
    """

    def test_ColorSpaceAPIImpliesColorspace(self):
        t = Usd.SchemaRegistry.GetTypeFromName("ColorSpaceAPI")
        self.assertFalse(t.isUnknown)
        caps = PR.GetSchemaImpliedCapabilities(t)
        self.assertIn("usd.core.colorspace", caps)

    def test_ColorSpaceDefinitionAPIImpliesColorspace(self):
        t = Usd.SchemaRegistry.GetTypeFromName("ColorSpaceDefinitionAPI")
        self.assertFalse(t.isUnknown)
        caps = PR.GetSchemaImpliedCapabilities(t)
        self.assertIn("usd.core.colorspace", caps)

    def test_UnknownTypeReturnsEmpty(self):
        t = Tf.Type.Unknown
        caps = PR.GetSchemaImpliedCapabilities(t)
        self.assertEqual(caps, set())

    def test_UnannotatedSchemaReturnsEmpty(self):
        # ClipsAPI has no impliesCapabilities annotation.
        t = Usd.SchemaRegistry.GetTypeFromName("ClipsAPI")
        self.assertFalse(t.isUnknown)
        caps = PR.GetSchemaImpliedCapabilities(t)
        self.assertNotIn("usd.core.colorspace", caps)

    def test_ResultIsCached(self):
        # Calling twice returns identical results (exercises cache path).
        t = Usd.SchemaRegistry.GetTypeFromName("ColorSpaceAPI")
        caps1 = PR.GetSchemaImpliedCapabilities(t)
        caps2 = PR.GetSchemaImpliedCapabilities(t)
        self.assertEqual(caps1, caps2)


class TestMultiApplyDedupe(unittest.TestCase):
    """Applying the same multi-apply schema N times should imply each
    capability exactly once, defaulted to hard."""

    def test_TwoInstancesProduceSingleEntry(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        leaf = stage.DefinePrim("/Root/Leaf")

        Usd.ColorSpaceDefinitionAPI.Apply(leaf, "foo")
        Usd.ColorSpaceDefinitionAPI.Apply(leaf, "bar")

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        # Both instances map to the same TfType; result should have
        # exactly one entry for usd.core.colorspace, at default strength.
        self.assertEqual(merged.get("usd.core.colorspace"), "hard")
        colorspace_entries = [k for k in merged if "colorspace" in k]
        self.assertEqual(len(colorspace_entries), 1)


class TestDefaultStrengthIsHard(unittest.TestCase):
    """Capabilities implied by applied schemas, with no user-authored
    strength, default to 'hard'."""

    def test_ImpliedCapabilityDefaultsToHard(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        leaf = stage.DefinePrim("/Root/Leaf")
        Usd.ColorSpaceAPI.Apply(leaf)

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged.get("usd.core.colorspace"), "hard")


class TestAuthoredStrengthsAreRespected(unittest.TestCase):
    """User-authored strengths survive aggregation. Implied capabilities the
    user has not declared a strength for default to 'hard'."""

    def test_UserDeclaresEnhancement(self):
        # User says: for this application, colorspace is just an enhancement.
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        leaf = stage.DefinePrim("/Root/Leaf")
        Usd.ColorSpaceAPI.Apply(leaf)

        claims = CA.Apply(root)
        claims.SetCapabilityUsage("usd.core.colorspace", "enhancement")

        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged.get("usd.core.colorspace"), "enhancement")

    def test_UserDeclaresSoft(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        leaf = stage.DefinePrim("/Root/Leaf")
        Usd.ColorSpaceAPI.Apply(leaf)

        claims = CA.Apply(root)
        claims.SetCapabilityUsage("usd.core.colorspace", "soft")

        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged.get("usd.core.colorspace"), "soft")

    def test_UserDeclaresHardMatchesDefault(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        leaf = stage.DefinePrim("/Root/Leaf")
        Usd.ColorSpaceAPI.Apply(leaf)

        claims = CA.Apply(root)
        claims.SetCapabilityUsage("usd.core.colorspace", "hard")

        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged.get("usd.core.colorspace"), "hard")


class TestEndToEndAggregator(unittest.TestCase):
    """Verify the full aggregation flow from namespace to stored metadata."""

    def test_BasicAggregation(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        leaf = stage.DefinePrim("/Root/A/B")

        Usd.ColorSpaceAPI.Apply(leaf)

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertEqual(merged, {"usd.core.colorspace": "hard"})
        # Verify that the written metadata matches the returned value.
        self.assertEqual(claims.GetCapabilityUsages(), merged)

    def test_EmptyNamespaceProducesEmptyDict(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        # No API schemas applied anywhere.
        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged, {})

    def test_SchemaOnRootPrimItself(self):
        # The root prim itself is included in the traversal.
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        Usd.ColorSpaceAPI.Apply(root)
        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()
        self.assertIn("usd.core.colorspace", merged)

    def test_AggregationAcrossMultipleDescendants(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        a = stage.DefinePrim("/Root/A")
        b = stage.DefinePrim("/Root/B")

        Usd.ColorSpaceAPI.Apply(a)
        Usd.ColorSpaceDefinitionAPI.Apply(b, "myCs")

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertIn("usd.core.colorspace", merged)
        self.assertEqual(merged["usd.core.colorspace"], "hard")

    def test_AuthoredEntryForImpliedCapabilitySurvives(self):
        # If the user has already declared a strength for a capability that
        # is still implied by the prim's applied schemas, that authored
        # entry survives the Populate round-trip.
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        Usd.ColorSpaceAPI.Apply(root)
        claims = CA.Apply(root)

        claims.SetCapabilityUsage("usd.core.colorspace", "soft")
        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged.get("usd.core.colorspace"), "soft")
        self.assertEqual(claims.GetCapabilityUsages(), merged)


if __name__ == '__main__':
    unittest.main()
