#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import unittest
from pxr import Plug, Tf, Usd, UsdProfiles

PR = UsdProfiles.ProfileRegistry
CA = UsdProfiles.ClaimsAPI


def _registerPhonyPlugin():
    pluginRoot = (os.path.join(os.path.dirname(__file__),
                               "UsdProfilesPlugins", "lib") +
                  "/TestUsdProfilesPhonyFileFormat*/Resources/")
    Plug.Registry().RegisterPlugins(pluginRoot)


class TestPhonyCapabilityGraph(unittest.TestCase):
    """Verify that the Phony test plugin seeds its capability sub-graph."""

    @classmethod
    def setUpClass(cls):
        _registerPhonyPlugin()

    def test_TestCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("test"))

    def test_PhonyCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("test.phony"))

    def test_PhonyFileFormatCapabilityPresent(self):
        self.assertTrue(PR.HasCapability("test.phony.fileformat"))

    def test_PhonyFileFormatTransitivelyReachesTest(self):
        status = PR.HasPredecessor("test.phony.fileformat", "test")
        self.assertEqual(status, PR.QueryStatus.ValidPath)


class TestFileFormatRegistryQuery(unittest.TestCase):
    """Verify GetFileFormatImpliedCapabilities for UsdProfilesTestPhonyFileFormat.

    The registry returns just the set of implied capability names; strengths
    are recorded by the consumer on its ClaimsAPI.
    """

    @classmethod
    def setUpClass(cls):
        _registerPhonyPlugin()

    def test_PhonyFileFormatImpliesFileFormatCapability(self):
        t = Tf.Type.FindByName("UsdProfilesTestPhonyFileFormat")
        self.assertFalse(t.isUnknown)
        caps = PR.GetFileFormatImpliedCapabilities(t)
        self.assertIn("test.phony.fileformat", caps)

    def test_UnknownTypeReturnsEmpty(self):
        caps = PR.GetFileFormatImpliedCapabilities(Tf.Type.Unknown)
        self.assertEqual(caps, set())

    def test_ResultIsCached(self):
        t = Tf.Type.FindByName("UsdProfilesTestPhonyFileFormat")
        caps1 = PR.GetFileFormatImpliedCapabilities(t)
        caps2 = PR.GetFileFormatImpliedCapabilities(t)
        self.assertEqual(caps1, caps2)


class TestFileFormatAggregation(unittest.TestCase):
    """Verify that PopulateCapabilityUsages picks up file-format
    implied capabilities from the stage's used layers, with a default
    strength of 'hard' and authored-strength preservation."""

    @classmethod
    def setUpClass(cls):
        _registerPhonyPlugin()

    def _phonyPath(self):
        return os.path.join(os.getcwd(), "minimal.phony")

    def test_PhonySublayerContributesCapabilityAtDefaultHard(self):
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().subLayerPaths.append(self._phonyPath())
        root = stage.DefinePrim("/Root")

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertIn("test.phony.fileformat", merged)
        self.assertEqual(merged["test.phony.fileformat"], "hard")
        self.assertEqual(claims.GetCapabilityUsages(), merged)

    def test_NoPhonySublayerNoCapability(self):
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertNotIn("test.phony.fileformat", merged)

    def test_MixedSchemaAndFileFormatCapabilities(self):
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().subLayerPaths.append(self._phonyPath())
        root = stage.DefinePrim("/Root")
        Usd.ColorSpaceAPI.Apply(root)

        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()

        self.assertIn("usd.core.colorspace", merged)
        self.assertIn("test.phony.fileformat", merged)
        self.assertEqual(merged["usd.core.colorspace"], "hard")
        self.assertEqual(merged["test.phony.fileformat"], "hard")

    def test_AuthoredEnhancementWinsOverDefault(self):
        # User declares: "for this application, the phony file format
        # capability is an enhancement, not a hard requirement."
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().subLayerPaths.append(self._phonyPath())
        root = stage.DefinePrim("/Root")

        claims = CA.Apply(root)
        claims.SetCapabilityUsage("test.phony.fileformat", "enhancement")

        merged = claims.PopulateCapabilityUsages()
        self.assertEqual(merged["test.phony.fileformat"], "enhancement")

    def test_AnonymousLayerNotCounted(self):
        # In-memory stages only have anonymous layers; none should contribute
        # test.phony.fileformat even if the Phony plugin is registered.
        stage = Usd.Stage.CreateInMemory()
        root = stage.DefinePrim("/Root")
        claims = CA.Apply(root)
        merged = claims.PopulateCapabilityUsages()
        self.assertNotIn("test.phony.fileformat", merged)


if __name__ == '__main__':
    unittest.main()
