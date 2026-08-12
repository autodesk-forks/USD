//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Verifies that Storm consumes an externally-owned GPU buffer published on a
// primvar via HdExtGpuBufferSchema (direct-bind / zero-copy) and renders it
// identically to the ordinary CPU-primvar path.
//
// Two workflows, selected by --instancing:
//   * basic (default): a single non-instanced cube whose *points* primvar is
//     backed by an external GPU buffer.
//   * --instancing: a grid of instanced cubes where BOTH the prototype *points*
//     and the instancer's per-instance *transforms* are external GPU buffers.
//     This is the two-axis instancing case (prototype primvar + instancer
//     primvar).
//
// The geometry is published through a HdRetainedSceneIndex inserted into the
// render index, because the schema lives as a data-source *child* of the
// primvar -- the legacy HdUnitTestDelegate emulation path would not carry it.
//
// Correctness check (self-comparing, no baseline image required): the test
// renders the SAME scene twice in one run -- once with ordinary CPU primvars,
// once with the external GPU buffers -- reads back both color images, and
// asserts they are pixel-identical. Because both images come from the same GPU
// in the same run, a correct GPU path is bit-for-bit equal to the CPU path; a
// wrong or silently-falling-back GPU path diverges and fails the test. This
// avoids committing a driver-specific baseline PNG.

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/extGpuBufferSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/errorMark.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Cube prototype: 8 corners, 6 quad faces (matches the flow-viewport example).
const VtIntArray _faceVertexCounts  = {4, 4, 4, 4, 4, 4};
const VtIntArray _faceVertexIndices = {0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6,
                                       6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4};

VtVec3fArray
_CubePoints(float h)
{
    return VtVec3fArray{
        {-h, -h,  h}, { h, -h,  h}, {-h,  h,  h}, { h,  h,  h},
        {-h,  h, -h}, { h,  h, -h}, {-h, -h, -h}, { h, -h, -h}};
}

using _PointArrayDs = HdRetainedTypedSampledDataSource<VtVec3fArray>;
using _IntArrayDs   = HdRetainedTypedSampledDataSource<VtIntArray>;

// The AOV path the driver uses for color (see HdSt_TestDriverBase::_GetAovPath).
const SdfPath _colorAovId("/testDriver/aov_color");

} // anonymous namespace

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(30.0f, 30.0f);
    }

    void InitTest() override {}   // all work happens in OffscreenTest / DrawTest
    void UninitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    // Build the scene into `scene`. When `gpuShare` is true the cube points
    // (and, for the instancing workflow, the instance transforms) are published
    // as external GPU buffers created from `driver`'s Hgi and appended to
    // `buffers` (so the caller can free them); otherwise they are CPU primvars.
    void _BuildScene(HdSt_TestDriver *driver,
                     HdRetainedSceneIndexRefPtr &scene,
                     std::vector<HgiBufferHandle> &buffers,
                     bool gpuShare) const;

    // Build the cube primvars container (points + constant displayColor).
    HdContainerDataSourceHandle _BuildCubePrimvars(
        HdSt_TestDriver *driver,
        std::vector<HgiBufferHandle> &buffers,
        bool gpuShare) const;

    uint64_t _MakeGpuBuffer(HdSt_TestDriver *driver,
                            std::vector<HgiBufferHandle> &buffers,
                            const void *data, size_t byteSize,
                            uint32_t stride) const;

    HdContainerDataSourceHandle _WithExtGpuBuffer(
        HdSt_TestDriver *driver,
        const HdContainerDataSourceHandle &primvar,
        uint64_t rawHandle, size_t byteSize,
        HdTupleType elementType, size_t numElements) const;

    // Render the scene once (fresh driver) and read the color AOV back into
    // `out`. Optionally also writes the image to `writePath`.
    void _RenderToPixels(bool gpuShare, std::vector<uint8_t> &out,
                         int &width, int &height,
                         const std::string &writePath) const;

    GfVec3f _CameraTranslate() const {
        // Frame the single cube up close, the grid pulled back.
        return _instancing ? GfVec3f(0.0f, 0.0f, -30.0f)
                           : GfVec3f(0.0f, 0.0f, -8.0f);
    }

    // Interactive-mode driver (created lazily; not used by --offscreen).
    std::unique_ptr<HdSt_TestDriver> _driver;
    HdRetainedSceneIndexRefPtr _driverScene;
    std::vector<HgiBufferHandle> _driverBuffers;

    // Geometry parameters.
    bool _instancing = false;   // --instancing: grid of instanced cubes
    int _div = 3;               // grid is _div x _div cubes (instancing only)
    float _halfSize = 1.0f;
    float _spacing = 3.0f;

    std::string _outputFilePath;  // --write: writes the GPU-shared image
    bool _writeCpu = false;       // --writeCpu: --write writes the CPU image
};

uint64_t
My_TestGLDrawing::_MakeGpuBuffer(HdSt_TestDriver *driver,
                                 std::vector<HgiBufferHandle> &buffers,
                                 const void *data, size_t byteSize,
                                 uint32_t stride) const
{
    HgiBufferDesc desc;
    desc.usage = HgiBufferUsageVertex;
    desc.byteSize = byteSize;
    desc.vertexStride = stride;   // Hgi requires this for vertex buffers
    desc.initialData = data;

    HgiBufferHandle buffer = driver->GetHgi()->CreateBuffer(desc);
    const uint64_t rawHandle = buffer->GetRawResource();
    buffers.push_back(std::move(buffer));
    return rawHandle;
}

HdContainerDataSourceHandle
My_TestGLDrawing::_WithExtGpuBuffer(
    HdSt_TestDriver *driver,
    const HdContainerDataSourceHandle &primvar,
    uint64_t rawHandle, size_t byteSize,
    HdTupleType elementType, size_t numElements) const
{
    // backendApi must equal the consumer's Hgi->GetAPIName() (HgiTokens->OpenGL
    // for HgiGL) -- that is exactly what a real producer's _GetCurrentBackendApi
    // publishes.
    HdContainerDataSourceHandle extGpuBuffer =
        HdExtGpuBufferSchema::Builder()
            .SetBackendApi(HdRetainedTypedSampledDataSource<TfToken>::New(
                driver->GetHgi()->GetAPIName()))
            .SetRawHandle(
                HdRetainedTypedSampledDataSource<uint64_t>::New(rawHandle))
            .SetRawHandleByteSize(
                HdRetainedTypedSampledDataSource<size_t>::New(byteSize))
            .SetNumElements(
                HdRetainedTypedSampledDataSource<size_t>::New(numElements))
            .SetElementType(
                HdRetainedTypedSampledDataSource<HdTupleType>::New(elementType))
            .SetByteOffset(HdRetainedTypedSampledDataSource<size_t>::New(0))
            .SetByteStride(HdRetainedTypedSampledDataSource<size_t>::New(0))
            .SetDirectBindable(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .Build();

    return HdOverlayContainerDataSource::New(
        primvar,
        HdRetainedContainerDataSource::New(
            HdExtGpuBufferSchema::GetSchemaToken(), extGpuBuffer));
}

HdContainerDataSourceHandle
My_TestGLDrawing::_BuildCubePrimvars(HdSt_TestDriver *driver,
                                     std::vector<HgiBufferHandle> &buffers,
                                     bool gpuShare) const
{
    const VtVec3fArray points = _CubePoints(_halfSize);

    HdContainerDataSourceHandle pointsPrimvar;
    if (gpuShare) {
        // GPU-only: empty CPU value + extGpuBuffer child pointing at a real
        // GL buffer holding the 8 cube corners.
        const size_t byteSize = points.size() * sizeof(GfVec3f);
        const uint64_t rawHandle =
            _MakeGpuBuffer(driver, buffers, points.cdata(), byteSize,
                           sizeof(GfVec3f));
        HdContainerDataSourceHandle emptyValue =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(_PointArrayDs::New(VtVec3fArray()))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point))
                .Build();
        pointsPrimvar = _WithExtGpuBuffer(
            driver, emptyValue, rawHandle, byteSize,
            HdTupleType{HdTypeFloatVec3, 1}, points.size());
    } else {
        pointsPrimvar =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(_PointArrayDs::New(points))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point))
                .Build();
    }

    HdContainerDataSourceHandle colorPrimvar =
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                VtVec3fArray{GfVec3f(0.2f, 0.7f, 0.9f)}))
            .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->constant))
            .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                HdPrimvarSchemaTokens->color))
            .Build();

    return HdRetainedContainerDataSource::New(
        HdPrimvarsSchemaTokens->points, pointsPrimvar,
        HdTokens->displayColor,         colorPrimvar);
}

void
My_TestGLDrawing::_BuildScene(HdSt_TestDriver *driver,
                              HdRetainedSceneIndexRefPtr &scene,
                              std::vector<HgiBufferHandle> &buffers,
                              bool gpuShare) const
{
    scene = HdRetainedSceneIndex::New();

    const SdfPath cubePath("/cube");
    const SdfPath instancerPath("/instancer");

    HdContainerDataSourceHandle primvarsDs =
        _BuildCubePrimvars(driver, buffers, gpuShare);

    HdContainerDataSourceHandle meshDs =
        HdMeshSchema::Builder()
            .SetTopology(HdMeshTopologySchema::Builder()
                .SetFaceVertexCounts(_IntArrayDs::New(_faceVertexCounts))
                .SetFaceVertexIndices(_IntArrayDs::New(_faceVertexIndices))
                .Build())
            .Build();

    const GfRange3d cubeRange({-_halfSize, -_halfSize, -_halfSize},
                              { _halfSize,  _halfSize,  _halfSize});
    HdContainerDataSourceHandle extentDs =
        HdExtentSchema::Builder()
            .SetMin(HdRetainedTypedSampledDataSource<GfVec3d>::New(
                cubeRange.GetMin()))
            .SetMax(HdRetainedTypedSampledDataSource<GfVec3d>::New(
                cubeRange.GetMax()))
            .Build();

    if (!_instancing) {
        // ---- Basic: one non-instanced cube -------------------------------
        HdContainerDataSourceHandle cubeDs =
            HdRetainedContainerDataSource::New(
                HdXformSchemaTokens->xform,
                HdXformSchema::Builder()
                    .SetMatrix(HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                        GfMatrix4d(1.0)))
                    .Build(),
                HdExtentSchemaTokens->extent, extentDs,
                HdMeshSchemaTokens->mesh, meshDs,
                HdPrimvarsSchemaTokens->primvars, primvarsDs);
        scene->AddPrims({{cubePath, HdPrimTypeTokens->mesh, cubeDs}});
        return;
    }

    // ---- Instancing: prototype cube + instancer --------------------------
    HdContainerDataSourceHandle instancedByDs =
        HdInstancedBySchema::Builder()
            .SetPaths(HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                VtArray<SdfPath>({instancerPath})))
            .Build();

    HdContainerDataSourceHandle cubeDs =
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform,
            HdXformSchema::Builder()
                .SetMatrix(HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                    GfMatrix4d(1.0)))
                .SetResetXformStack(
                    HdRetainedTypedSampledDataSource<bool>::New(true))
                .Build(),
            HdExtentSchemaTokens->extent, extentDs,
            HdMeshSchemaTokens->mesh, meshDs,
            HdPrimvarsSchemaTokens->primvars, primvarsDs,
            HdInstancedBySchema::GetSchemaToken(), instancedByDs);

    // Instance transforms: a grid of translations centered on the origin.
    // GfMatrix4f (float) layout -- the GPU path skips the CPU double->float
    // conversion, so the shared buffer must already be float mat4.
    const int numInstances = _div * _div;
    VtMatrix4dArray matricesD(numInstances);          // CPU baseline path
    std::vector<GfMatrix4f> matricesF(numInstances);  // GPU buffer
    const float offset = (_div - 1) * 0.5f * _spacing;
    for (int y = 0; y < _div; ++y) {
        for (int x = 0; x < _div; ++x) {
            const int i = x + y * _div;
            const GfVec3d t(x * _spacing - offset, y * _spacing - offset, 0.0);
            GfMatrix4d m(1.0);
            m.SetTranslate(t);
            matricesD[i] = m;
            matricesF[i] = GfMatrix4f(m);
        }
    }

    VtIntArray prototypeIndices(numInstances);
    for (int i = 0; i < numInstances; ++i) {
        prototypeIndices[i] = i;
    }
    HdDataSourceBaseHandle instanceIndicesDs =
        HdRetainedTypedSampledDataSource<VtIntArray>::New(prototypeIndices);
    HdVectorDataSourceHandle instanceIndicesVec =
        HdRetainedSmallVectorDataSource::New(1, &instanceIndicesDs);

    HdContainerDataSourceHandle instancerTopologyDs =
        HdInstancerTopologySchema::Builder()
            .SetPrototypes(
                HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                    {cubePath}))
            .SetInstanceIndices(instanceIndicesVec)
            .Build();

    HdContainerDataSourceHandle xformPrimvar;
    if (gpuShare) {
        const size_t byteSize = numInstances * sizeof(GfMatrix4f);
        const uint64_t rawHandle =
            _MakeGpuBuffer(driver, buffers, matricesF.data(), byteSize,
                           sizeof(GfMatrix4f));
        HdContainerDataSourceHandle emptyValue =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtMatrix4dArray>::New(
                        VtMatrix4dArray()))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->instance))
                .Build();
        xformPrimvar = _WithExtGpuBuffer(
            driver, emptyValue, rawHandle, byteSize,
            HdTupleType{HdTypeFloatMat4, 1}, numInstances);
    } else {
        xformPrimvar =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtMatrix4dArray>::New(
                        matricesD))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->instance))
                .Build();
    }

    HdContainerDataSourceHandle instancerPrimvarsDs =
        HdRetainedContainerDataSource::New(
            HdInstancerTokens->instanceTransforms, xformPrimvar);

    HdContainerDataSourceHandle instancerDs =
        HdRetainedContainerDataSource::New(
            HdInstancerTopologySchema::GetSchemaToken(), instancerTopologyDs,
            HdPrimvarsSchema::GetSchemaToken(), instancerPrimvarsDs);

    scene->AddPrims({
        {cubePath, HdPrimTypeTokens->mesh, cubeDs},
        {instancerPath, HdInstancerTokens->instancer, instancerDs}});
}

void
My_TestGLDrawing::_RenderToPixels(bool gpuShare, std::vector<uint8_t> &out,
                                  int &width, int &height,
                                  const std::string &writePath) const
{
    const int w = GetWidth(), h = GetHeight();

    auto driver = std::make_unique<HdSt_TestDriver>(HdReprTokens->hull);
    HdRetainedSceneIndexRefPtr scene;
    std::vector<HgiBufferHandle> buffers;
    _BuildScene(driver.get(), scene, buffers, gpuShare);

    // Feed the geometry through a scene index so the extGpuBuffer child
    // survives to the terminal scene index the consumer reads from.
    driver->GetDelegate().GetRenderIndex().InsertSceneIndex(
        scene, SdfPath::AbsoluteRootPath());

    driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    driver->SetClearDepth(1.0f);
    driver->SetupAovs(w, h);
    driver->SetCamera(GetViewMatrix(), GetProjectionMatrix(),
                      CameraUtilFraming(GfRect2i(GfVec2i(0, 0), w, h)));
    driver->Draw();

    if (!writePath.empty()) {
        driver->WriteToFile("color", writePath);
    }

    // Read the color AOV back into `out`.
    HdRenderBuffer *rb = dynamic_cast<HdRenderBuffer *>(
        driver->GetDelegate().GetRenderIndex().GetBprim(
            HdPrimTypeTokens->renderBuffer, _colorAovId));
    if (!rb) {
        TF_RUNTIME_ERROR("No color render buffer to read back");
    } else {
        width = rb->GetWidth();
        height = rb->GetHeight();
        const size_t bpp = HdDataSizeOfFormat(rb->GetFormat());
        const uint8_t *data = static_cast<const uint8_t *>(rb->Map());
        out.assign(data, data + size_t(width) * height * bpp);
        rb->Unmap();
    }

    // The external GPU buffers are non-owning in Storm; free the real GL
    // resources we allocated (before this driver's Hgi is destroyed).
    Hgi *hgi = driver->GetHgi();
    for (HgiBufferHandle &b : buffers) {
        hgi->DestroyBuffer(&b);
    }
}

void
My_TestGLDrawing::OffscreenTest()
{
    SetCameraTranslate(_CameraTranslate());

    std::vector<uint8_t> cpuPixels, gpuPixels;
    int cw = 0, ch = 0, gw = 0, gh = 0;

    _RenderToPixels(/*gpuShare*/false, cpuPixels, cw, ch,
                    _writeCpu ? _outputFilePath : std::string());
    _RenderToPixels(/*gpuShare*/true, gpuPixels, gw, gh,
                    _writeCpu ? std::string() : _outputFilePath);

    if (cpuPixels.empty() || gpuPixels.empty()) {
        TF_RUNTIME_ERROR("Readback produced no pixels (cpu=%zu gpu=%zu)",
                         cpuPixels.size(), gpuPixels.size());
        return;
    }
    if (cw != gw || ch != gh || cpuPixels.size() != gpuPixels.size()) {
        TF_RUNTIME_ERROR("CPU/GPU image dimensions differ: "
                         "%dx%d (%zu) vs %dx%d (%zu)",
                         cw, ch, cpuPixels.size(), gw, gh, gpuPixels.size());
        return;
    }

    // Both images come from the same GPU in the same run, so a correct GPU
    // path is bit-identical. Allow a tiny per-channel slack only to be safe.
    const int kTolerance = 2;
    size_t diffBytes = 0;
    int maxDiff = 0;
    for (size_t i = 0; i < gpuPixels.size(); ++i) {
        const int d = std::abs(int(gpuPixels[i]) - int(cpuPixels[i]));
        if (d > maxDiff) {
            maxDiff = d;
        }
        if (d > kTolerance) {
            ++diffBytes;
        }
    }

    const double diffFraction = double(diffBytes) / double(gpuPixels.size());
    std::cout << (_instancing ? "[instancing] " : "[basic] ")
              << "CPU vs GPU: maxDiff=" << maxDiff
              << " diffBytes=" << diffBytes
              << " (" << (diffFraction * 100.0) << "%)\n";

    // Any GPU-shared image that meaningfully diverges from the CPU render is a
    // failure (a silent fallback renders blank -> ~100% divergence).
    if (diffFraction > 0.001) {
        TF_RUNTIME_ERROR("GPU-shared render differs from CPU baseline: "
                         "%zu bytes (%.3f%%) exceed tolerance, maxDiff=%d",
                         diffBytes, diffFraction * 100.0, maxDiff);
    }
}

void
My_TestGLDrawing::DrawTest()
{
    // Interactive mode: show the GPU-shared scene.
    if (!_driver) {
        SetCameraTranslate(_CameraTranslate());
        _driver = std::make_unique<HdSt_TestDriver>(HdReprTokens->hull);
        _BuildScene(_driver.get(), _driverScene, _driverBuffers,
                    /*gpuShare*/true);
        _driver->GetDelegate().GetRenderIndex().InsertSceneIndex(
            _driverScene, SdfPath::AbsoluteRootPath());
        _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
        _driver->SetClearDepth(1.0f);
        _driver->SetupAovs(GetWidth(), GetHeight());
    }

    _driver->SetCamera(GetViewMatrix(), GetProjectionMatrix(),
                       CameraUtilFraming(
                           GfRect2i(GfVec2i(0, 0), GetWidth(), GetHeight())));
    _driver->UpdateAovDimensions(GetWidth(), GetHeight());
    _driver->Draw();
}

void
My_TestGLDrawing::UninitTest()
{
    if (_driver) {
        Hgi *hgi = _driver->GetHgi();
        for (HgiBufferHandle &b : _driverBuffers) {
            hgi->DestroyBuffer(&b);
        }
    }
    _driverBuffers.clear();
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    if (_driver) {
        _driver->Present(GetWidth(), GetHeight(), framebuffer);
    }
}

void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--write") {
            _outputFilePath = argv[++i];
        } else if (arg == "--div") {
            _div = atoi(argv[++i]);
        } else if (arg == "--instancing") {
            _instancing = true;
        } else if (arg == "--writeCpu") {
            _writeCpu = true;   // --write emits the CPU image instead of GPU
        }
    }
}

int
main(int argc, char *argv[])
{
    TfErrorMark mark;

    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }
    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
