#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# This is an example script from the USD tutorial,
# "Transformations, Animation, and Layer Offsets".
#
# When run, it will generate a series of usda files in the current
# directory that illustrate each of the steps in the tutorial.
#
from pxr import Usd, UsdGeom, Gf, Sdf, Ts

def MakeInitialStage(path):
    stage = Usd.Stage.CreateNew(path)
    UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.z)
    stage.SetStartTimeCode(0)
    stage.SetEndTimeCode(192)
    return stage
def Step1():
    stage = MakeInitialStage('Step1.usda')
    stage.SetMetadata('comment', 'Step 1: Start and end time codes')
    stage.Save()

def AddReferenceToGeometry(stage, path):
    geom = UsdGeom.Xform.Define(stage, path)
    geom.GetPrim().GetReferences().AddReference('./top.geom.usd')
    return geom
def Step2():
    stage = MakeInitialStage('Step2.usda')
    stage.SetMetadata('comment', 'Step 2: Geometry reference')
    top = AddReferenceToGeometry(stage, '/Top')
    stage.Save()

def AddSpin(top):
    spin = top.AddRotateZOp(opSuffix='spin')
    spin.Set(time=0, value=0)
    spin.Set(time=192, value=1440)
def Step3():
    stage = MakeInitialStage('Step3.usda')
    stage.SetMetadata('comment', 'Step 3: Adding spin animation')
    top = AddReferenceToGeometry(stage, '/Top')
    AddSpin(top)
    stage.Save()

def AddSpinSpline(top):
    spin = top.AddRotateZOp(opSuffix='spin')
    spinAttr = spin.GetAttr()
    typeName = str(spinAttr.GetTypeName())
    spline = Ts.Spline(typeName)
    spline.SetKnot(Ts.Knot(
        typeName = typeName,
        time = 0,
        value = 0,
        nextInterp = Ts.InterpLinear,
        ))
    lp = Ts.LoopParams()
    lp.protoStart = 0
    lp.protoEnd = 48
    lp.numPostLoops = 3
    lp.valueOffset = 360
    spline.SetInnerLoopParams(lp)
    spinAttr.SetSpline(spline)
def Step3A():
    stage = MakeInitialStage('Step3A.usda')
    stage.SetMetadata('comment', 'Step 3A: Using splines instead of time-samples')
    top = AddReferenceToGeometry(stage, '/Top')
    AddSpinSpline(top)
    stage.Save()


def AddTilt(top):
    tilt = top.AddRotateXOp(opSuffix='tilt')
    tilt.Set(value=12)
def Step4():
    stage = MakeInitialStage('Step4.usda')
    stage.SetMetadata('comment', 'Step 4: Adding tilt')
    top = AddReferenceToGeometry(stage, '/Top')
    AddTilt(top)
    AddSpinSpline(top)
    stage.Save()

def Step4A():
    stage = MakeInitialStage('Step4A.usda')
    stage.SetMetadata('comment', 'Step 4A: Adding spin and tilt')
    top = AddReferenceToGeometry(stage, '/Top')
    AddSpinSpline(top)
    AddTilt(top)
    stage.Save()

def AddOffset(top):
    top.AddTranslateOp(opSuffix='offset').Set(value=(0, 0.1, 0))
def AddPrecession(top):
    precess = top.AddRotateZOp(opSuffix='precess')
    precessAttr = precess.GetAttr()
    typeName = str(precessAttr.GetTypeName())
    spline = Ts.Spline(typeName)
    spline.SetKnot(Ts.Knot(
        typeName = typeName,
        time = 0,
        value = 0,
        nextInterp = Ts.InterpLinear,
        ))
    spline.SetKnot(Ts.Knot(
        typeName = typeName,
        time = 192,
        value = 360,
        nextInterp = Ts.InterpLinear,
        ))
    precessAttr.SetSpline(spline)
def Step5():
    stage = MakeInitialStage('Step5.usda')
    stage.SetMetadata('comment', 'Step 5: Adding precession and offset')
    top = AddReferenceToGeometry(stage, '/Top')
    AddPrecession(top)
    AddOffset(top)
    AddTilt(top)
    AddSpinSpline(top)
    stage.Save()

def Step6():
    # Use animated layer from Step5
    anim_layer_path = './Step5.usda'

    stage = MakeInitialStage('Step6.usda')
    stage.SetMetadata('comment', 'Step 6: Layer offsets and animation')

    left = UsdGeom.Xform.Define(stage, '/Left')
    left_top = UsdGeom.Xform.Define(stage, '/Left/Top')
    left_top.GetPrim().GetReferences().AddReference(
        assetPath = anim_layer_path,
        primPath = '/Top')

    middle = UsdGeom.Xform.Define(stage, '/Middle')
    middle.AddTranslateOp().Set(value=(2, 0, 0))
    middle_top = UsdGeom.Xform.Define(stage, '/Middle/Top')
    middle_top.GetPrim().GetReferences().AddReference(
        assetPath = anim_layer_path,
        primPath = '/Top',
        layerOffset = Sdf.LayerOffset(offset=96))

    right = UsdGeom.Xform.Define(stage, '/Right')
    right.AddTranslateOp().Set(value=(4, 0, 0))
    right_top = UsdGeom.Xform.Define(stage, '/Right/Top')
    right_top.GetPrim().GetReferences().AddReference(
        assetPath = anim_layer_path,
        primPath = '/Top',
        layerOffset = Sdf.LayerOffset(scale=0.25))

    stage.Save()

def AddSpinSplineWithExtrapolation(top):
    spin = top.AddRotateZOp(opSuffix='spin')
    spinAttr = spin.GetAttr()
    typeName = str(spinAttr.GetTypeName())
    spline = Ts.Spline(typeName)
    spline.SetKnot(Ts.Knot(
        typeName = typeName,
        time = 0,
        value = 0,
        nextInterp = Ts.InterpLinear,
        ))
    spline.SetKnot(Ts.Knot(
        typeName = typeName,
        time = 48,
        value = 360,
        nextInterp = Ts.InterpCurve,
        ))
    spline.SetPostExtrapolation(Ts.Extrapolation(Ts.ExtrapLoopRepeat))
    spinAttr.SetSpline(spline)
def AddPrecessionWithExtrapolation(top):
    precess = top.AddRotateZOp(opSuffix='precess')
    precessAttr = precess.GetAttr()
    typeName = str(precessAttr.GetTypeName())
    spline = Ts.Spline(typeName)
    spline.SetKnot(Ts.Knot(
        typeName = typeName,
        time = 0,
        value = 0,
        nextInterp = Ts.InterpLinear,
        ))
    spline.SetKnot(Ts.Knot(
        typeName = typeName,
        time = 192,
        value = 360,
        nextInterp = Ts.InterpLinear,
        ))
    spline.SetPostExtrapolation(Ts.Extrapolation(Ts.ExtrapLoopRepeat))    
    precessAttr.SetSpline(spline)  
def Step7():
    # Create animated layer that uses spline extrapolation
    stage = MakeInitialStage('Step7ref.usda')
    stage.SetMetadata('comment', 'Step 7 (ref): Adding spline extrapolation')
    top = AddReferenceToGeometry(stage, '/Top')
    AddPrecessionWithExtrapolation(top)
    AddOffset(top)
    AddTilt(top)
    AddSpinSplineWithExtrapolation(top)
    stage.Save()

    # Use animated layer configured to use spline extrapolation 
    anim_layer_path = './Step7ref.usda'

    stage = MakeInitialStage('Step7.usda')
    stage.SetMetadata('comment', 'Step 7: Spline extrapolation')

    left = UsdGeom.Xform.Define(stage, '/Left')
    left_top = UsdGeom.Xform.Define(stage, '/Left/Top')
    left_top.GetPrim().GetReferences().AddReference(
        assetPath = anim_layer_path,
        primPath = '/Top')

    middle = UsdGeom.Xform.Define(stage, '/Middle')
    middle.AddTranslateOp().Set(value=(2, 0, 0))
    middle_top = UsdGeom.Xform.Define(stage, '/Middle/Top')
    middle_top.GetPrim().GetReferences().AddReference(
        assetPath = anim_layer_path,
        primPath = '/Top',
        layerOffset = Sdf.LayerOffset(offset=96))

    right = UsdGeom.Xform.Define(stage, '/Right')
    right.AddTranslateOp().Set(value=(4, 0, 0))
    right_top = UsdGeom.Xform.Define(stage, '/Right/Top')
    right_top.GetPrim().GetReferences().AddReference(
        assetPath = anim_layer_path,
        primPath = '/Top',
        layerOffset = Sdf.LayerOffset(scale=0.25))

    stage.Save()

if __name__ == '__main__':
    Step1()
    Step2()
    Step3()
    Step3A()
    Step4()
    Step4A()
    Step5()
    Step6()
    Step7()
