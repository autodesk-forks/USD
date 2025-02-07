//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/types.h"
#include "pxr/imaging/hgiPresent2/present.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "testenv/testAovSetWithCapture.h"
#include "testenv/testWindow.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

struct _Capture
{
    uint32_t start{0};
    uint32_t stop{std::numeric_limits<uint32_t>::max()};
    uint32_t skip{0};
};

struct _Resize
{
    float factor{1};
    uint32_t frame{0};
};

struct _Args
{
    std::string windowApi;
    std::string filePath;
    _Capture capture;
    _Resize resize;
    bool vsync{false};
    bool colorCorrection{false};
};

static void
_ParseArgs(int argc, char** argv, _Args& args)
{
    const auto checkForMissingArgument = [argc, argv](int i, int n = 1) {
        if (i + n >= argc) {
            std::cout << TfGetBaseName(argv[0]) << ": "
                      << "missing parameter(s) for '" << argv[i] << "'"
                      << std::endl;
            return false;
        }

        return true;
    };

    for (int i = 1; i != argc; ++i) {
        if (strcmp(argv[i], "-") == 0) {
            std::cout
                << TfGetBaseName(argv[0]) << ": "
                << "-stage filePath\n\n"
                << "HgiPresent2 tests\n\n"
                << "-stage filePath           name of the USD stage to open\n"
                << "-window apiName           name of the window API to use\n"
                << "-size width height        initial window size (default "
                   "640x480)\n"
                << "-vsync                    enable v-sync\n"
                << "-capture start stop skip  capture frame between start and "
                   "stop (exclusive), skipping the given number of frame in "
                   "between captures\n"
                << "-colorCorrection          enable sRGB color correction\n"
                << "-resize factor frame      at the given frame, resize by "
                   "the given factor\n"
                << std::endl;
            exit(0);
        } else if (strcmp(argv[i], "-window") == 0) {
            if (!checkForMissingArgument(i)) {
                exit(1);
            }
            args.windowApi = argv[++i];
        } else if (strcmp(argv[i], "-stage") == 0) {
            if (!checkForMissingArgument(i)) {
                exit(1);
            }
            args.filePath = argv[++i];
        } else if (strcmp(argv[i], "-capture") == 0) {
            if (!checkForMissingArgument(i, 3)) {
                exit(1);
            }
            try {
                args.capture.start = std::stoul(argv[++i]);
                args.capture.stop = std::stoul(argv[++i]);
                args.capture.skip = std::stoul(argv[++i]);
            } catch (const std::exception&) {
                std::cout << TfGetBaseName(argv[0]) << ": "
                          << "not a 32bit unsigned integer '" << argv[i] << "'"
                          << std::endl;
                exit(1);
            }
        } else if (strcmp(argv[i], "-vsync") == 0) {
            args.vsync = true;
        } else if (strcmp(argv[i], "-colorCorrection") == 0) {
            args.colorCorrection = true;
        }  else if (strcmp(argv[i], "-resize") == 0) {
            if (!checkForMissingArgument(i, 2)) {
                exit(1);
            }
            try {
                args.resize.factor = std::stof(argv[++i]);
            } catch (const std::exception&) {
                std::cout << TfGetBaseName(argv[0]) << ": "
                          << "not a float '" << argv[i] << "'"
                          << std::endl;
                exit(1);
            }
            try {
                args.resize.frame = std::stoul(argv[++i]);
            } catch (const std::exception&) {
                std::cout << TfGetBaseName(argv[0]) << ": "
                          << "not a 32bit unsigned integer '" << argv[i] << "'"
                          << std::endl;
                exit(1);
            }
        } else {
            std::cout << TfGetBaseName(argv[0]) << ": "
                      << "unknown argument '" << argv[i] << "'" << std::endl;
            exit(1);
        }
    }

    if (args.filePath.empty()) {
        std::cout << TfGetBaseName(argv[0]) << ": "
                  << "missing argument '-stage'" << std::endl;
        exit(1);
    }

    if (args.windowApi.empty()) {
#if defined(ARCH_OS_DARWIN)
        args.windowApi = "Metal";
#elif defined(ARCH_OS_LINUX)
        args.windowApi = "X11";
#elif defined(ARCH_OS_WINDOWS)
        args.windowApi = "WIN32";
#endif
    }
}

class TestHgiPresent2
{
public:
    void RunTest(int argc, char** argv);
    void InitTest(const std::string& windowApi, const std::string& filePath);
    void DrawTest(const _Capture& capture, const _Resize& resize,
        bool vsync, bool colorCorrection);

private:
    std::unique_ptr<HgiPresent2TestWindow> _window;
    HgiPresent2SurfaceHandle _surface;
    UsdStageRefPtr _stage;
    std::unique_ptr<UsdImagingGLEngine> _engine;
    GlfSimpleLightingContextRefPtr _lightingContext;

    GfVec2i _windowSize{640, 480};
    std::array<float, 3> _translate{};
};

void
TestHgiPresent2::RunTest(int argc, char** argv)
{
    _Args args;
    _ParseArgs(argc, argv, args);

    InitTest(args.windowApi, args.filePath);
    DrawTest(args.capture, args.resize, args.vsync, args.colorCorrection);
}

void
TestHgiPresent2::InitTest(
    const std::string& windowApi, const std::string& filePath)
{
    _window = HgiPresent2TestWindow::Create(windowApi, _windowSize);
    if (!_window) {
        TF_RUNTIME_ERROR("Failed to create window");
        return;
    }

    _stage = UsdStage::Open(filePath);

    UsdImagingGLEngine::Parameters parameters;
    parameters.rootPath = _stage->GetPseudoRoot().GetPath();

    _engine = std::make_unique<UsdImagingGLEngine>(parameters);

    _surface = HgiPresent2TestWindowHandleToSurfaceHandle(
        _engine->GetHgi(), _window->GetHandle());
    if (std::holds_alternative<std::monostate>(_surface)) {
        TF_RUNTIME_ERROR("Failed to create surface");
        return;
    }

    // Extent hints are sometimes authored as an optimization to avoid
    // computing bounds, they are particularly useful for some tests where
    // there is no bound on the first frame.
    const TfTokenVector purposes{UsdGeomTokens->default_};
    UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), purposes, true);

    GfBBox3d bbox = bboxCache.ComputeWorldBound(_stage->GetPseudoRoot());
    GfRange3d world = bbox.ComputeAlignedRange();

    GfVec3d worldCenter = (world.GetMin() + world.GetMax()) / 2.0;
    double worldSize = world.GetSize().GetLength();

    if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
        // transpose y and z centering translation
        _translate[0] = -worldCenter[0];
        _translate[1] = -worldCenter[2];
        _translate[2] = -worldCenter[1] - worldSize;
    } else {
        _translate[0] = -worldCenter[0];
        _translate[1] = -worldCenter[1];
        _translate[2] = -worldCenter[2] - worldSize;
    }

    _lightingContext = GlfSimpleLightingContext::New();
    GlfSimpleLight light;
    light.SetPosition(GfVec4f(0, -.5, .5, 0));
    light.SetDiffuse(GfVec4f(1, 1, 1, 1));
    light.SetAmbient(GfVec4f(0, 0, 0, 1));
    light.SetSpecular(GfVec4f(1, 1, 1, 1));
    GlfSimpleLightVector lights;
    lights.push_back(light);
    _lightingContext->SetLights(lights);

    GlfSimpleMaterial material;
    material.SetAmbient(GfVec4f(0.2, 0.2, 0.2, 1.0));
    material.SetDiffuse(GfVec4f(0.8, 0.8, 0.8, 1.0));
    material.SetSpecular(GfVec4f(0, 0, 0, 1));
    material.SetShininess(0.0001f);
    _lightingContext->SetMaterial(material);
    _lightingContext->SetSceneAmbient(GfVec4f(0.2, 0.2, 0.2, 1.0));
}

void
TestHgiPresent2::DrawTest(const _Capture& capture, const _Resize& resize,
    bool vsync, bool colorCorrection)
{
    if (!_window) {
        return;
    }

    TfStopwatch renderTime;

    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = true;
    params.enableSceneMaterials = true;
    params.complexity = 1;
    params.cullStyle =
        UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED;
    params.colorCorrectionMode = colorCorrection ?
        HdxColorCorrectionTokens->sRGB :
        HdxColorCorrectionTokens->disabled;

    _engine->SetRendererAov(HdAovTokens->color);
    _engine->SetLightingState(_lightingContext);

    uint32_t frame = 0;
    const auto wantTexels = [&capture, &frame]() {
        // Don't capture if skip == 0. Used for debugging the window.
        return capture.skip != 0 && frame >= capture.start &&
            (frame - capture.start) % capture.skip == 0;
    };
    const auto getTexels = [&frame](
                                  const HgiTextureDesc& descriptor,
                                  std::vector<std::byte> texels) {
        HioImage::StorageSpec storageSpec;
        storageSpec.width = descriptor.dimensions[0];
        storageSpec.height = descriptor.dimensions[1];
        storageSpec.depth = descriptor.dimensions[2];
        storageSpec.format = HdxGetHioFormat(descriptor.format);
        storageSpec.flipped = false;
        storageSpec.data = texels.data();

        std::stringstream fileNameBuffer;
        fileNameBuffer << "testHgiPresent2" << "_frame" << frame << "_"
                       << storageSpec.width << "x" << storageSpec.height
                       << ".png";
        const auto fileName = fileNameBuffer.str();

        HioImageSharedPtr const image = HioImage::OpenForWriting(fileName);
        if (!image || !image->Write(storageSpec)) {
            TF_RUNTIME_ERROR("Failed to write image to %s", fileName.c_str());
        }
    };

    const auto hgi = _engine->GetHgi();
    auto aovSet = HgiPresent2SurfaceToAovSet(hgi, _surface);
    auto testAovSetWithCapture =
        std::make_unique<HgiPresent2TestAovSetWithCapture>(std::move(aovSet),
            HgiPresent2TestWantTexelsCallback{wantTexels},
            HgiPresent2TestGetTexelsCallback{getTexels});
    _engine->EnablePresentation(
        HgiPresent2::CreateAovBlit(hgi, std::move(testAovSetWithCapture)));
    _engine->EnableVsync(vsync);

    params.frame = UsdTimeCode::Default();

    for (; frame < capture.stop; frame++) {
        if (!_window->Update()) {
            break;
        }

        _windowSize = _window->GetSize();
        if (resize.factor != 1 && resize.frame == frame) {
            _windowSize *= resize.factor;
            _window->SetSize(_windowSize);
            if (!_window->Update()) {
                break;
            }
        }

        // Assume a constant 60fps so the test aren't frame rate dependent.
        const double t = frame / 60.;

        // Create an oscillating motion
        double rotateY{}, rotateX{};
        ArchSinCos(t * 2 * M_PI, &rotateY, &rotateX);
        rotateY *= 15;
        rotateX *= 15;

        const double aspectRatio =
            static_cast<double>(_windowSize[0]) / _windowSize[1];
        GfFrustum frustum;
        frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);
        const GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();

        GfVec4d viewport = {0, 0, static_cast<double>(_windowSize[0]),
            static_cast<double>(_windowSize[1])};
        _engine->SetRenderViewport(viewport);

        GfMatrix4d viewMatrix(1.0);
        viewMatrix *=
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(0, 1, 0), rotateY));
        viewMatrix *=
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), rotateX));
        viewMatrix *= GfMatrix4d().SetTranslate(
            GfVec3d(_translate[0], _translate[1], _translate[2]));
        GfMatrix4d modelViewMatrix = viewMatrix;
        if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
            // rotate from z-up to y-up
            modelViewMatrix = GfMatrix4d().SetRotate(
                                  GfRotation(GfVec3d(1.0, 0.0, 0.0), -90.0)) *
                modelViewMatrix;
        }
        _engine->SetCameraState(modelViewMatrix, projMatrix);

        renderTime.Start();
        do {
            _engine->Render(_stage->GetPseudoRoot(), params);
        } while (!_engine->IsConverged());
        renderTime.Stop();
    }

    _engine->DisablePresentation();

    HgiPresent2TestDestroySurfaceHandle(hgi, _surface);
}

int
main(int argc, char** argv)
{
    TfErrorMark mark;

    TestHgiPresent2 driver;
    driver.RunTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
