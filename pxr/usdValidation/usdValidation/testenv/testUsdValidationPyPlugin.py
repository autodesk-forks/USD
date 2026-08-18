#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

"""End-to-end test for a Python-type validator plugin.

This test exercises the full lazy-load path:

1. plugInfo.json (Type: "python") declares validator metadata.
2. Plug.Registry().RegisterPlugins() discovers the plugin; the
   ValidationRegistry parses its Validators metadata.
3. GetOrLoadValidatorByName() triggers plugin->Load(), which does
   ``import testPyValidatorPlugin``.
4. The module's top-level code calls RegisterPluginStageValidator
   and RegisterPluginLayerValidator.
5. The validators are returned and can be invoked.
"""

import os
import sys
import unittest

from pxr import Plug, Sdf, Usd, UsdValidation

class TestUsdValidationPyPlugin(unittest.TestCase):

    PLUGIN_NAME = "testPyValidatorPlugin"

    @classmethod
    def setUpClass(cls):
        # The plugin directory must be discoverable by PlugRegistry
        # BEFORE the ValidationRegistry singleton is created, so that
        # plugInfo.json metadata is parsed during registry initialization.
        #
        # The test working directory is set to the TESTENV location by
        # pxr_register_test, so the plugin package is a subdirectory.
        pluginDir = os.path.join(os.getcwd(), cls.PLUGIN_NAME)
        if not os.path.isdir(pluginDir):
            # Fallback for manual invocation outside ctest.
            pluginDir = os.path.join(os.path.dirname(__file__),
                                     "testUsdValidationPyPlugin",
                                     cls.PLUGIN_NAME)
        # Add the parent of the plugin package to sys.path so that
        # ``import testPyValidatorPlugin`` resolves correctly when the
        # Plug system does TfPyRunSimpleString("import ...").
        parentDir = os.path.dirname(pluginDir)
        if parentDir not in sys.path:
            sys.path.insert(0, parentDir)
        Plug.Registry().RegisterPlugins(pluginDir + "/")
        assert Plug.Registry().GetPluginWithName(cls.PLUGIN_NAME), (
            f"Plugin {cls.PLUGIN_NAME} not found after registering {pluginDir}")

    def test_MetadataDiscoverableBeforeLoad(self):
        """Validator metadata from plugInfo.json is available before the
        plugin module is imported."""
        registry = UsdValidation.ValidationRegistry()
        meta = registry.GetValidatorMetadata(
            self.PLUGIN_NAME + ":PyStageValidator")
        self.assertIsNotNone(meta)
        self.assertEqual(
            meta.doc,
            "Stage validator implemented in a Python plugin.")
        self.assertIn("testPyValidatorPlugin", meta.GetKeywords())

    def test_LazyLoadStageValidator(self):
        """GetOrLoadValidatorByName triggers plugin load and registration."""
        registry = UsdValidation.ValidationRegistry()
        # This call triggers plugin->Load() -> import testPyValidatorPlugin
        # -> __init__.py calls RegisterPluginStageValidator.
        validator = registry.GetOrLoadValidatorByName(
            self.PLUGIN_NAME + ":PyStageValidator")
        self.assertIsNotNone(validator)

        stage = Usd.Stage.CreateInMemory()
        errors = validator.Validate(stage)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "PyPluginStageError")

    def test_LazyLoadLayerValidator(self):
        """Layer validator from the same plugin is also available."""
        registry = UsdValidation.ValidationRegistry()
        # __init__.py registers both validators, so the layer validator is
        # available even though this test runs before test_LazyLoadStageValidator.
        validator = registry.GetOrLoadValidatorByName(
            self.PLUGIN_NAME + ":PyLayerValidator")
        self.assertIsNotNone(validator)

        layer = Sdf.Layer.CreateAnonymous(".usda")
        errors = validator.Validate(layer)
        self.assertEqual(len(errors), 1)
        self.assertEqual(errors[0].GetName(), "PyPluginLayerError")

    def test_DiscoverableByKeyword(self):
        """Plugin validators appear in keyword queries."""
        registry = UsdValidation.ValidationRegistry()
        metadatas = registry.GetValidatorMetadataForKeyword(
            "testPyValidatorPlugin")
        names = [m.name for m in metadatas]
        self.assertIn(self.PLUGIN_NAME + ":PyStageValidator", names)
        self.assertIn(self.PLUGIN_NAME + ":PyLayerValidator", names)


if __name__ == "__main__":
    unittest.main()
