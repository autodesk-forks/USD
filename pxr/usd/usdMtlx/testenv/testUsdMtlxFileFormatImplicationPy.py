#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import unittest
from pxr import Tf, Usd, UsdMtlx, UsdProfiles  # noqa: F401 - UsdMtlx registers the plugin

PR = UsdProfiles.ProfileRegistry
CA = UsdProfiles.ClaimsAPI


class TestMtlxCapabilityGraph(unittest.TestCase):
    """Verify that usdMtlx/plugInfo.json seeds the mtlx capability sub-graph."""

    def test_MtlxCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("usd.mtlx"))

    def test_MtlxFileFormatCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("usd.mtlx.fileformat"))

    def test_MtlxFileFormatTransitivelyReachesUsd(self):
        status = PR.HasPredecessor("usd.mtlx.fileformat", "usd")
        self.assertEqual(status, PR.QueryStatus.ValidPath)


class TestFileFormatRegistryQuery(unittest.TestCase):
    """Verify GetFileFormatImpliedCapabilities for UsdMtlxFileFormat.

    The registry returns just the set of implied capability names; strengths
    are recorded by the consumer on its ClaimsAPI.
    """

    def test_MtlxFileFormatImpliesFileFormatCapability(self):
        t = Tf.Type.FindByName("UsdMtlxFileFormat")
        self.assertFalse(t.isUnknown)
        caps = PR.GetFileFormatImpliedCapabilities(t)
        self.assertIn("usd.mtlx.fileformat", caps)

    def test_UnknownTypeReturnsEmpty(self):
        caps = PR.GetFileFormatImpliedCapabilities(Tf.Type.Unknown)
        self.assertEqual(caps, set())

    def test_ResultIsCached(self):
        t = Tf.Type.FindByName("UsdMtlxFileFormat")
        caps1 = PR.GetFileFormatImpliedCapabilities(t)
        caps2 = PR.GetFileFormatImpliedCapabilities(t)
        self.assertEqual(caps1, caps2)


class TestFileFormatAggregation(unittest.TestCase):
    """Verify that PopulateCapabilityUsages picks up file-format
    implied capabilities from the stage's used layers, with a default
    strength of 'hard' and authored-strength preservation."""

    def _mtlxPath(self):
        return os.path.join(os.getcwd(), "minimal.mtlx")

    def test_MtlxSublayerContributesMtlxCapabilityAtDefaultHard(self):
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().subLayerPaths.append(self._mtlxPath())
        root = stage.DefinePrim("/Root")

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertIn("usd.mtlx.fileformat", merged)
        self.assertEqual(merged["usd.mtlx.fileformat"], "hard")
        self.assertEqual(claims.GetCapabilityUsages(), merged)

    def test_NoMtlxSublayerNoMtlxCapability(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertNotIn("usd.mtlx.fileformat", merged)

    def test_MixedSchemaAndFileFormatCapabilities(self):
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().subLayerPaths.append(self._mtlxPath())
        root = stage.DefinePrim("/Root")
        Usd.ColorSpaceAPI.Apply(root)

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertIn("usd.core.colorspace", merged)
        self.assertIn("usd.mtlx.fileformat", merged)
        self.assertEqual(merged["usd.core.colorspace"], "hard")
        self.assertEqual(merged["usd.mtlx.fileformat"], "hard")

    def test_AuthoredEnhancementWinsOverDefault(self):
        # User declares: "for this application, the mtlx file format
        # capability is an enhancement, not a hard requirement."
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().subLayerPaths.append(self._mtlxPath())
        root = stage.DefinePrim("/Root")

        claims = CA.Apply(root)
        claims.SetCapabilityUsage("usd.mtlx.fileformat", "enhancement")

        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged["usd.mtlx.fileformat"], "enhancement")

    def test_AnonymousLayerNotCounted(self):
        # In-memory stages only have anonymous layers; none should contribute
        # usd.mtlx.fileformat even if usdMtlx is registered.
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()
        self.assertNotIn("usd.mtlx.fileformat", merged)


if __name__ == '__main__':
    unittest.main()
