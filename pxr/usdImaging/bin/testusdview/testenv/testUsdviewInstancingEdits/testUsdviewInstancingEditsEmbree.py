#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.common import SelectionHighlightModes
from pxr import UsdGeom

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.selHighlightMode = (
        SelectionHighlightModes.NEVER)

#
# Tests whether visibility authored on a PointInstancer is respected.
#
def _testInstancerVisibilityEdits(appController):
    from pxr import Sdf

    testLayer = Sdf.Layer.FindOrOpen("usd-11149/instancerVisibility.usda")
    appController._dataModel.stage.GetRootLayer().TransferContent(testLayer)
    appController._takeShot("instancerInvisibleEmbree.png", waitForConvergence=True)

    vis = appController._dataModel.stage.GetPropertyAtPath("/PointInstancer.visibility")
    vis.Set(UsdGeom.Tokens.inherited)
    appController._takeShot("instancerVisibleEmbree.png", waitForConvergence=True)

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testInstancerVisibilityEdits(appController)
