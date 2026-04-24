#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Tf, Ar, Sdf, UsdUtils, Plug
import os, shutil
import unittest

def _RemoveExistingDir(dir):
    if os.path.exists(dir):
        print("Removing existing localization directory: {}".format(dir))
        shutil.rmtree(dir)

class TestUsdUtilsAssetLocalization(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register test file format plugin
        # Test plugins are installed relative to this script
        testRoot = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "UsdUtilsPlugins")

        pr = Plug.Registry()

        testFileFormatPath = os.path.join(testRoot,
            "lib/TestUsdUtilsLocalizeExternalAssetDependencies*/Resources/")
        pr.RegisterPlugins(testFileFormatPath)

    def test_RemappedReferencePathsAppearInLocalizedLayers(self):
        localizationDir = "remapRef_localized"

        _RemoveExistingDir(localizationDir)
        self.assertTrue(UsdUtils.LocalizeAsset("remapRef/root.usda",  
                                      localizationDir, editLayersInPlace=True))
        
        localizedSubPath = os.path.join(localizationDir, "0", "sub.usda")
        sub = Sdf.Layer.FindOrOpen(localizedSubPath)
        self.assertIsNotNone(sub)

        world = sub.GetPrimAtPath("/World")
        self.assertIsNotNone(world)

        self.assertListEqual(
            list(world.referenceList.GetAddedOrExplicitItems()),
            [Sdf.Reference('1/ref.usda')])
        
    def test_RemappedReferencePathsAppearInUsdzPackages(self):
        assetPath = "remapRef/root.usda"
        archivePath = "remapRef.usdz"

        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)
        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath, editLayersInPlace=True))
        
        subPath = Ar.JoinPackageRelativePath([archivePath, "0/sub.usda"])
        sub = Sdf.Layer.FindOrOpen(subPath)
        self.assertIsNotNone(sub)

        world = sub.GetPrimAtPath("/World")
        self.assertIsNotNone(world)

        self.assertListEqual(
            list(world.referenceList.GetAddedOrExplicitItems()),
            [Sdf.Reference('1/ref.usda')])
        
    def test_ExternalAssetsAppearInUsdzPackage(self):
        assetPath = "externalAssets/root.usda"
        archivePath = "externalAssets.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)
        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath, editLayersInPlace=True))

        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "root.usda", 
            "asset/asset",
            "asset/file1.external", 
            "asset/subdir/file2.external", 
            "asset/asset.testformat"
        ]
        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_ExternalAssetsAppearInUsdzPackageContextDep(self):
        assetPath = "externalAssets/rootContextDep.usda"
        archivePath = "externalAssetsContextDep.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)
        with Ar.ResolverContextBinder(context):
            self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                assetPath, archivePath, editLayersInPlace=True))

        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "rootContextDep.usda", 
            "0/asset",
            "0/file1.external", 
            "0/subdir/file2.external", 
            "0/asset.testformat"
        ]

        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_ExternalAssetsAppearInLocalizedPackage(self):
        assetPath = "externalAssets/root.usda"
        localizationDir = "externalAssets_localized"
        _RemoveExistingDir(localizationDir)

        self.assertTrue(UsdUtils.LocalizeAsset(assetPath, localizationDir))

        expectedAssets = [
            "root.usda", 
            "asset/asset", 
            "asset/file1.external", 
            "asset/subdir/file2.external", 
            "asset/asset.testformat"
        ]

        for expectedAsset in expectedAssets:
            self.assertTrue(os.path.exists(
                os.path.join(localizationDir, expectedAsset)))
            
    def test_ExternalAssetsAppearInLocalizedPackageContextDep(self):
        assetPath = "externalAssets/rootContextDep.usda"
        localizationDir = "externalAssetsContextDep_localized"
        _RemoveExistingDir(localizationDir)

        self.assertTrue(UsdUtils.LocalizeAsset(assetPath, localizationDir))

        expectedAssets = [
            "rootContextDep.usda", 
            "0/asset", 
            "0/file1.external", 
            "0/subdir/file2.external", 
            "0/asset.testformat"
        ]

        for expectedAsset in expectedAssets:
            self.assertTrue(os.path.exists(
                os.path.join(localizationDir, expectedAsset)))

            
    def test_ExternalAssetsAboveRootTriggerWarningInUsdz(self):
        assetPath = "externalAssetOutsideRoot/root.usda"
        archivePath = "externalAssetOutsideRoot.usdz"
        context = Ar.GetResolver().CreateDefaultContextForAsset(assetPath)

        with Tf.DiagnosticTrap() as trap:
            with Ar.ResolverContextBinder(context):
                self.assertTrue(UsdUtils.CreateNewUsdzPackage(
                    assetPath, archivePath, editLayersInPlace=True))
            
            self.assertTrue(trap.HasWarnings())
            commentaryStr = "Could not determine package relative path."
            self.assertTrue(
                any(commentaryStr in s.commentary for s in trap.GetWarnings()))

        zf = Sdf.ZipFile.Open(archivePath)
        expectedAssets = [
            "root.usda", 
            "asset/file1.external", 
            "asset/asset.testformat"
        ]
        self.assertEqual(expectedAssets, zf.GetFileNames())

    def test_ExternalAssetsAboveRootTriggerWarningInLocalizedPackage(self):
        assetPath = "externalAssetOutsideRoot/root.usda"
        localizationDir = "externalAssetOutsideRoot_localized"
        _RemoveExistingDir(localizationDir)

        with Tf.DiagnosticTrap() as trap:
            self.assertTrue(UsdUtils.LocalizeAsset(assetPath, localizationDir))

            self.assertTrue(trap.HasWarnings())
            commentaryStr = "Could not determine package relative path."
            self.assertTrue(
                any(commentaryStr in s.commentary for s in trap.GetWarnings()))

        expectedAssets = [
            "root.usda", 
            "asset/file1.external",
            "asset/asset.testformat"
        ]

        for expectedAsset in expectedAssets:
            self.assertTrue(os.path.exists(
                os.path.join(localizationDir, expectedAsset)))

if __name__=="__main__":
    unittest.main()
