//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/vt/array.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f));
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    void RunIndexedPrimvarsTests(int argc, char **argv);

private:
    HdSt_TestDriverUniquePtr _driver;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()" << std::endl;

    _driver = std::make_unique<HdSt_TestDriver>(HdReprTokens->smoothHull);
    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
{
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCullStyle(HdCullStyleNothing);

    _driver->SetCamera(viewMatrix, projMatrix,
        CameraUtilFraming(GfRect2i(GfVec2i(0, 0), width, height)));

    _driver->UpdateAovDimensions(width, height);

    _driver->Draw();
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();

    if (!_outputFilePath.empty()) {
        _driver->WriteToFile("color", _outputFilePath);
    }
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::RunIndexedPrimvarsTests(int argc, char **argv)
{
    // Begin Test
    RunTest(argc, argv);

    HdUnitTestDelegate &delegate = _driver->GetDelegate();

    // Create a simple box
    const SdfPath cube1{"/cube1"};
    const VtVec3fArray points = {GfVec3f(-2, -2, 2), GfVec3f(2, -2, 2),
        GfVec3f(-2, 2, 2), GfVec3f(2, 2, 2), GfVec3f(-2, 2, -2),
        GfVec3f(2, 2, -2), GfVec3f(-2, -2, -2), GfVec3f(2, -2, -2)};
    const VtIntArray numVerts = {4, 4, 4, 4, 4, 4};
    const VtIntArray verts = {
        0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4};
    delegate.AddMesh(cube1,
        GfMatrix4f(1.0f).SetRotate(GfRotation(GfVec3f(0, 0, 1), 45)), points,
        numVerts, verts, false, SdfPath(), PxOsdOpenSubdivTokens->none);
    delegate.AddPrimvar(cube1, HdTokens->normals,
        VtValue(
            VtVec3fArray({GfVec3f(0, 0, 1), GfVec3f(0, 1, 0), GfVec3f(0, 0, -1),
                GfVec3f(0, -1, 0), GfVec3f(1, 0, 0), GfVec3f(-1, 0, 0)})),
        HdInterpolationUniform, HdPrimvarRoleTokens->none,
        VtIntArray({0, 1, 2, 3, 4, 5}));

    delegate.RemovePrimvar(cube1, HdTokens->displayColor);
    delegate.AddPrimvar(cube1, HdTokens->displayColor,
        VtValue(VtVec3fArray(6, GfVec3f(1, 1, 0))), HdInterpolationUniform,
        HdPrimvarRoleTokens->none);
    delegate.RemovePrimvar(cube1, HdTokens->displayOpacity);
    delegate.AddPrimvar(cube1, HdTokens->displayOpacity,
        VtValue(VtFloatArray(1, 0.5f)), HdInterpolationUniform,
        HdPrimvarRoleTokens->none, VtIntArray({0, 0, 0, 0, 0, 0}));

    _outputFilePath = "testHdStIndexedPrimvars_before.png";
    RunOffscreenTest();

    delegate.RemovePrimvar(cube1, HdTokens->displayColor);
    delegate.AddPrimvar(cube1, HdTokens->displayColor,
        VtValue(VtVec3fArray({GfVec3f(0, 1, 1), GfVec3f(1, 0, 1)})),
        HdInterpolationUniform, HdPrimvarRoleTokens->none,
        VtIntArray({1, 0, 1, 0, 1, 0}));
    _outputFilePath = "testHdStIndexedPrimvars_replaceColor.png";
    RunOffscreenTest();

    delegate.RemovePrimvar(cube1, HdTokens->displayOpacity);
    delegate.AddPrimvar(cube1, HdTokens->displayOpacity, VtValue(1.0f),
        HdInterpolationConstant, HdPrimvarRoleTokens->none);
    _outputFilePath = "testHdStIndexedPrimvars_replaceOpacity.png";
    RunOffscreenTest();

    delegate.GetRenderIndex().GetChangeTracker().MarkRprimClean(
        cube1, HdChangeTracker::DirtyPrimvar);
    _outputFilePath = "testHdStIndexedPrimvars_partialDirty.png";
    RunOffscreenTest();
}

////////////////////////////////////////////////////////////

void
BasicTest(int argc, char **argv)
{
    My_TestGLDrawing driver;
    driver.RunIndexedPrimvarsTests(argc, argv);
}

int
main(int argc, char **argv)
{
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
