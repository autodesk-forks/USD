//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/mesh.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdx/pickBuffers.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hio/image.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (pickables)
);

class Hdx_TestDriver : public HdSt_TestDriverBase<Hdx_UnitTestDelegate>
{
public:
    Hdx_TestDriver();

    std::shared_ptr<HdxPickBuffers> PickWithBuffers(int width, int height,
        GfFrustum const &frustum, GfMatrix4d const &viewMatrix,
        bool doOcclusion = false);

    void SetSceneColReprSelector(HdReprSelector const &reprSelector) {
        _pickablesCol.SetReprSelector(reprSelector);
    }

protected:
    using HdSt_TestDriverBase::_Init;
    void _Init(HdReprSelector const &reprSelector) override;

private:
    HdRprimCollection _pickablesCol;
};

Hdx_TestDriver::Hdx_TestDriver()
{
    _Init(HdReprSelector(HdReprTokens->hull));
}

void
Hdx_TestDriver::_Init(HdReprSelector const &reprSelector)
{   
    _SetupSceneDelegate();
    
    Hdx_UnitTestDelegate &delegate = GetDelegate();

    // prepare pick task
    SdfPath pickTask("/pickTask");
    delegate.AddPickTask(pickTask);

    // picking
    _pickablesCol = HdRprimCollection(_tokens->pickables, 
        HdReprSelector(HdReprTokens->refined));
    // We have to unfortunately explicitly add collections besides 'geometry'
    // See HdRenderIndex constructor.
    delegate.GetRenderIndex().GetChangeTracker().AddCollection(
        _tokens->pickables);
}

std::shared_ptr<HdxPickBuffers>
Hdx_TestDriver::PickWithBuffers(int width, int height,
    GfFrustum const &frustum,
    GfMatrix4d const &viewMatrix,
    bool doOcclusion) {
    HdxPickHitVector allHits;
    auto pickBuffers = std::make_shared<HdxPickBuffers>();
    
    HdxPickTaskContextParams p;
    // Use full framebuffer resolution for pick buffers
    p.resolution = GfVec2i(width, height);
    p.subRect = GfVec4i(0, 0, width, height);
    p.resolveMode = HdxPickTokens->resolveNone;
    p.viewMatrix = viewMatrix;
    p.projectionMatrix = frustum.ComputeProjectionMatrix();
    p.collection = _pickablesCol;
    if (doOcclusion) {
        HdRprimCollection occluderCol = HdRprimCollection(_tokens->pickables,
            HdReprSelector(HdReprTokens->smoothHull));
        p.occluderCollection = occluderCol;
        p.doUnpickablesOcclude = true;
    }
    p.outHits = &allHits;
    p.pickBuffers = pickBuffers;

    HdTaskSharedPtrVector tasks;
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(
        SdfPath("/pickTask")));
    VtValue pickParams(p);
    _GetEngine()->SetTaskContextData(HdxPickTokens->pickParams, pickParams);
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);

    return pickBuffers;
}

// Helper function to generate unique RGB color from integer ID (hash-based)
// https://gist.github.com/badboy/6267743
std::tuple<unsigned char, unsigned char, unsigned char> IdToColor(int id) {
    unsigned int hash = static_cast<unsigned int>(id);
    hash = (hash ^ 61) ^ (hash >> 16);
    hash = hash + (hash << 3);
    hash = hash ^ (hash >> 4);
    hash = hash * 0x27d4eb2d;
    hash = hash ^ (hash >> 15);

    unsigned char r = static_cast<unsigned char>((hash & 0xFF0000) >> 16);
    unsigned char g = static_cast<unsigned char>((hash & 0x00FF00) >> 8);
    unsigned char b = static_cast<unsigned char>(hash & 0x0000FF);

    return std::make_tuple(r, g, b);
};

// --------------------------------------------------------------------------
// Utility function to write integer buffers as images
// Encodes 32-bit integers by packing bytes into RGBA channels
static bool
_WriteIntToColorBufferToImage(int const* buffer, GfVec2i const& size,
                       std::string const& filename)
{
    if (!buffer) {
        TF_CODING_ERROR("Null buffer provided to _WriteIntBufferToImage");
        return false;
    }

    // Pack 32-bit integers into RGBA bytes (8 bits per channel)
    std::vector<uint8_t> byteData(size[0] * size[1] * 4);
    
    for (int i = 0; i < size[0] * size[1]; ++i) {
        auto [r, g, b] = buffer[i] >= 0
            ? IdToColor(buffer[i])
            : std::tuple<unsigned char, unsigned char, unsigned char>(0, 0, 0);

        byteData[i * 4 + 0] = r;
        byteData[i * 4 + 1] = g;
        byteData[i * 4 + 2] = b;
        byteData[i * 4 + 3] = 255; // A = 255
    }

    HioImage::StorageSpec storage;
    storage.width = size[0];
    storage.height = size[1];
    storage.format = HioFormatUNorm8Vec4;
    storage.flipped = true;
    storage.data = byteData.data();

    HioImageSharedPtr const image = HioImage::OpenForWriting(filename);
    if (!image) {
        TF_RUNTIME_ERROR("Failed to open image for writing %s",
            filename.c_str());
        return false;
    }

    if (!image->Write(storage)) {
        TF_RUNTIME_ERROR("Failed to write image to %s", filename.c_str());
        return false;
    }

    return true;
}

// Utility function to write packed 2-10-10-10 normal buffers as RGB images
// The neye buffer stores normals packed using HdVec4f_2_10_10_10_REV format
static bool
_WritePackedNormalBufferToImage(int const* buffer, GfVec2i const& size,
                                std::string const& filename)
{
    if (!buffer) {
        TF_CODING_ERROR("Null buffer provided to _WritePackedNormalBufferToImage");
        return false;
    }

    std::vector<float> floatData(size[0] * size[1] * 4);
    
    for (int i = 0; i < size[0] * size[1]; ++i) {
        // Decompress 2-10-10-10 packed normals back to 4 channels
        GfVec3f neye = HdVec4f_2_10_10_10_REV(buffer[i]).GetAsVec<GfVec3f>();

        floatData[i * 4 + 0] = neye[0];
        floatData[i * 4 + 1] = neye[1];
        floatData[i * 4 + 2] = neye[2];
        floatData[i * 4 + 3] = 1.0f;
    }

    HioImage::StorageSpec storage;
    storage.width = size[0];
    storage.height = size[1];
    storage.format = HioFormatFloat32Vec4;
    storage.flipped = true;
    storage.data = floatData.data();

    HioImageSharedPtr const image = HioImage::OpenForWriting(filename);
    if (!image) {
        TF_RUNTIME_ERROR("Failed to open image for writing %s",
            filename.c_str());
        return false;
    }

    if (!image->Write(storage)) {
        TF_RUNTIME_ERROR("Failed to write image to %s", filename.c_str());
        return false;
    }

    return true;
}

// Utility function to write float buffers (like depth) as grayscale images
static bool
_WriteFloatBufferToImage(float const* buffer, GfVec2i const& size, 
                         std::string const& filename)
{
    if (!buffer) {
        TF_CODING_ERROR("Null buffer provided to _WriteFloatBufferToImage");
        return false;
    }

    std::vector<float> floatData(size[0] * size[1] * 4);
    
    for (int i = 0; i < size[0] * size[1]; ++i) {
        float val = buffer[i];
        floatData[i * 4 + 0] = val;
        floatData[i * 4 + 1] = val;
        floatData[i * 4 + 2] = val;
        floatData[i * 4 + 3] = 1.0f; // A
    }

    HioImage::StorageSpec storage;
    storage.width = size[0];
    storage.height = size[1];
    storage.format = HioFormatFloat32Vec4;
    storage.flipped = true;
    storage.data = floatData.data();

    HioImageSharedPtr const image = HioImage::OpenForWriting(filename);
    if (!image) {
        TF_RUNTIME_ERROR("Failed to open image for writing %s",
            filename.c_str());
        return false;
    }

    if (!image->Write(storage)) {
        TF_RUNTIME_ERROR("Failed to write image to %s", filename.c_str());
        return false;
    }

    return true;
}

// --------------------------------------------------------------------------

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    void InitTest() override;
    void DrawTest() override {}
    void OffscreenTest() override;

protected:
    void _InitScene();

private:
    std::unique_ptr<Hdx_TestDriver> _driver;
};

////////////////////////////////////////////////////////////

static GfMatrix4d
_GetTransform(GfRotation rot, GfVec3d translate)
{
    GfMatrix4d xform;
    xform.SetRotate(rot);
    xform.SetTranslateOnly(translate);

    return xform;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<Hdx_TestDriver>();
    _InitScene();
    SetCameraTranslate(GfVec3f(-2.3, -2.3999, -10));
    SetCameraRotate(-1, 13);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::_InitScene()
{
    Hdx_UnitTestDelegate &delegate = _driver->GetDelegate();

    GfRotation rot(/*axis*/GfVec3d(1,0,1), /*angle*/30);
    delegate.AddCube(SdfPath("/cube0"), _GetTransform(rot, GfVec3d(0,0,0)));
    delegate.AddCube(SdfPath("/cube1"), _GetTransform(rot, GfVec3d(5,0,0)));
    delegate.AddTet (SdfPath("/tet0"),  _GetTransform(rot, GfVec3d(0,0,5)));
    delegate.AddTet (SdfPath("/tet1"),  _GetTransform(rot, GfVec3d(5,0,5)));
}

// Helper function to write all pick buffers for a given representation
static void
_WritePickBuffersForRepr(std::shared_ptr<HdxPickBuffers> const& pickBuffers,
                         std::string const& reprSuffix)
{
    if (!pickBuffers) {
        TF_CODING_ERROR("Null pick buffers provided");
        return;
    }

    GfVec2i bufferSize = pickBuffers->GetBufferSize();

    if (int const* primIds = pickBuffers->GetPrimIds()) {
        std::string filename = "buffer_primIds_" + reprSuffix + ".png";
        _WriteIntToColorBufferToImage(primIds, bufferSize, filename);
    }

    if (int const* instanceIds = pickBuffers->GetInstanceIds()) {
        std::string filename = "buffer_instanceIds_" + reprSuffix + ".png";
        _WriteIntToColorBufferToImage(instanceIds, bufferSize, filename);
    }

    if (int const* elementIds = pickBuffers->GetFaceIds()) {
        std::string filename = "buffer_elementIds_" + reprSuffix + ".png";
        _WriteIntToColorBufferToImage(elementIds, bufferSize, filename);
    }

    if (int const* edgeIds = pickBuffers->GetEdgeIds()) {
        std::string filename = "buffer_edgeIds_" + reprSuffix + ".png";
        _WriteIntToColorBufferToImage(edgeIds, bufferSize, filename);
    }

    if (int const* pointIds = pickBuffers->GetPointIds()) {
        std::string filename = "buffer_pointIds_" + reprSuffix + ".png";
        _WriteIntToColorBufferToImage(pointIds, bufferSize, filename);
    }

    if (int const* neyes = pickBuffers->GetNormals()) {
        std::string filename = "buffer_neyes_" + reprSuffix + ".png";
        _WritePackedNormalBufferToImage(neyes, bufferSize, filename);
    }

    if (float const* depths = pickBuffers->GetDepths()) {
        std::string filename = "buffer_depths_" + reprSuffix + ".png";
        _WriteFloatBufferToImage(depths, bufferSize, filename);
    }
}

void
My_TestGLDrawing::OffscreenTest()
{
    // Test pick buffers with different representations
    // This generates 7 buffer types × 4 representations = 28 images
    
    struct ReprTest {
        const char* name;
        TfToken reprToken;
    };
    
    const ReprTest tests[] = {
        {"smooth", HdReprTokens->smoothHull},
        {"flat", HdReprTokens->hull},
        {"point", HdReprTokens->points},
        {"line", HdReprTokens->wire}
    };

    for (const auto& test : tests) {
        std::cout << "\n=== Testing " << test.name << " Representation ===" << std::endl;
        _driver->SetSceneColReprSelector(HdReprSelector(test.reprToken));
        
        std::shared_ptr<HdxPickBuffers> pickBuffers = _driver->PickWithBuffers(
            GetWidth(), GetHeight(), GetFrustum(), GetViewMatrix());
        
        if (pickBuffers) {
            _WritePickBuffersForRepr(pickBuffers, test.name);
        } else {
            TF_CODING_ERROR("Null pick buffers provided");
        }
    }

    _driver->SetSceneColReprSelector(HdReprSelector(HdReprTokens->points));
    std::shared_ptr<HdxPickBuffers> pickBuffers = _driver->PickWithBuffers(
        GetWidth(), GetHeight(), GetFrustum(), GetViewMatrix(), true);
    if (pickBuffers) {
        _WritePickBuffersForRepr(pickBuffers, "points_occluded");
    } else {
        TF_CODING_ERROR("Null pick buffers provided");
    }
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;

    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
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
