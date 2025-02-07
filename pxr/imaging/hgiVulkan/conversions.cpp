//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/imaging/hgiVulkan/conversions.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

#include <vulkan/vk_enum_string_helper.h>

PXR_NAMESPACE_OPEN_SCOPE


static constexpr std::pair<HgiAttachmentLoadOp, VkAttachmentLoadOp>
_LoadOpTable[] =
{
    {HgiAttachmentLoadOpDontCare, VK_ATTACHMENT_LOAD_OP_DONT_CARE},
    {HgiAttachmentLoadOpClear,    VK_ATTACHMENT_LOAD_OP_CLEAR},
    {HgiAttachmentLoadOpLoad,     VK_ATTACHMENT_LOAD_OP_LOAD}
};
static_assert(std::size(_LoadOpTable) == HgiAttachmentLoadOpCount);

static constexpr std::pair<HgiAttachmentStoreOp, VkAttachmentStoreOp>
_StoreOpTable[] =
{
    {HgiAttachmentStoreOpDontCare, VK_ATTACHMENT_STORE_OP_DONT_CARE},
    {HgiAttachmentStoreOpStore,    VK_ATTACHMENT_STORE_OP_STORE}
};
static_assert(std::size(_StoreOpTable) == HgiAttachmentStoreOpCount);

static constexpr std::pair<HgiFormat, VkFormat>
_FormatTable[] =
{
    {HgiFormatUNorm8,            VK_FORMAT_R8_UNORM},
    {HgiFormatUNorm8Vec2,        VK_FORMAT_R8G8_UNORM},
    {HgiFormatUNorm8Vec4,        VK_FORMAT_R8G8B8A8_UNORM},
    {HgiFormatSNorm8,            VK_FORMAT_R8_SNORM},
    {HgiFormatSNorm8Vec2,        VK_FORMAT_R8G8_SNORM},
    {HgiFormatSNorm8Vec4,        VK_FORMAT_R8G8B8A8_SNORM},
    {HgiFormatFloat16,           VK_FORMAT_R16_SFLOAT},
    {HgiFormatFloat16Vec2,       VK_FORMAT_R16G16_SFLOAT},
    {HgiFormatFloat16Vec3,       VK_FORMAT_R16G16B16_SFLOAT},
    {HgiFormatFloat16Vec4,       VK_FORMAT_R16G16B16A16_SFLOAT},
    {HgiFormatFloat32,           VK_FORMAT_R32_SFLOAT},
    {HgiFormatFloat32Vec2,       VK_FORMAT_R32G32_SFLOAT},
    {HgiFormatFloat32Vec3,       VK_FORMAT_R32G32B32_SFLOAT},
    {HgiFormatFloat32Vec4,       VK_FORMAT_R32G32B32A32_SFLOAT},
    {HgiFormatInt16,             VK_FORMAT_R16_SINT},
    {HgiFormatInt16Vec2,         VK_FORMAT_R16G16_SINT},
    {HgiFormatInt16Vec3,         VK_FORMAT_R16G16B16_SINT},
    {HgiFormatInt16Vec4,         VK_FORMAT_R16G16B16A16_SINT},
    {HgiFormatUInt16,            VK_FORMAT_R16_UINT},
    {HgiFormatUInt16Vec2,        VK_FORMAT_R16G16_UINT},
    {HgiFormatUInt16Vec3,        VK_FORMAT_R16G16B16_UINT},
    {HgiFormatUInt16Vec4,        VK_FORMAT_R16G16B16A16_UINT},
    {HgiFormatInt32,             VK_FORMAT_R32_SINT},
    {HgiFormatInt32Vec2,         VK_FORMAT_R32G32_SINT},
    {HgiFormatInt32Vec3,         VK_FORMAT_R32G32B32_SINT},
    {HgiFormatInt32Vec4,         VK_FORMAT_R32G32B32A32_SINT},
    {HgiFormatUNorm8Vec4srgb,    VK_FORMAT_R8G8B8A8_SRGB},
    {HgiFormatBC6FloatVec3,      VK_FORMAT_BC6H_SFLOAT_BLOCK},
    {HgiFormatBC6UFloatVec3,     VK_FORMAT_BC6H_UFLOAT_BLOCK},
    {HgiFormatBC7UNorm8Vec4,     VK_FORMAT_BC7_UNORM_BLOCK},
    {HgiFormatBC7UNorm8Vec4srgb, VK_FORMAT_BC7_SRGB_BLOCK},
    {HgiFormatBC1UNorm8Vec4,     VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
    {HgiFormatBC3UNorm8Vec4,     VK_FORMAT_BC3_UNORM_BLOCK},
    {HgiFormatFloat32UInt8,      VK_FORMAT_D32_SFLOAT_S8_UINT},
    {HgiFormatPackedInt1010102,  VK_FORMAT_A2B10G10R10_SNORM_PACK32},
};
static_assert(std::size(_FormatTable) == HgiFormatCount);

// A few random format validations to make sure the table above stays in sync
// with the HgiFormat table.
static constexpr bool
_CompileTimeValidateHgiFormatTable()
{
    return HgiFormatUNorm8 == 0 &&
        HgiFormatFloat16Vec4 == 9 &&
        HgiFormatFloat32Vec4 == 13 &&
        HgiFormatUInt16Vec4 == 21 &&
        HgiFormatUNorm8Vec4srgb == 26 &&
        HgiFormatBC3UNorm8Vec4 == 32;
}

static_assert(_CompileTimeValidateHgiFormatTable(), 
    "_FormatTable array out of sync with HgiFormat enum");

static constexpr std::pair<HgiSampleCount, VkSampleCountFlagBits>
_SampleCountTable[] =
{
    {HgiSampleCount1,  VK_SAMPLE_COUNT_1_BIT},
    {HgiSampleCount2,  VK_SAMPLE_COUNT_2_BIT},
    {HgiSampleCount4,  VK_SAMPLE_COUNT_4_BIT},
    {HgiSampleCount8,  VK_SAMPLE_COUNT_8_BIT},
    {HgiSampleCount16, VK_SAMPLE_COUNT_16_BIT}
};
static_assert((std::end(_SampleCountTable) - 1)->first == HgiSampleCountEnd - 1);

static constexpr std::pair<HgiShaderStage, VkShaderStageFlagBits>
_ShaderStageTable[] =
{    
    {HgiShaderStageVertex,                  VK_SHADER_STAGE_VERTEX_BIT},
    {HgiShaderStageFragment,                VK_SHADER_STAGE_FRAGMENT_BIT},
    {HgiShaderStageCompute,                 VK_SHADER_STAGE_COMPUTE_BIT},
    {HgiShaderStageTessellationControl,     VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
    {HgiShaderStageTessellationEval,        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
    {HgiShaderStageGeometry,                VK_SHADER_STAGE_GEOMETRY_BIT},
    {HgiShaderStagePostTessellationControl, {}},
    {HgiShaderStagePostTessellationVertex,  {}},
};
static_assert(1 << std::size(_ShaderStageTable) == HgiShaderStageCustomBitsBegin);

static constexpr std::pair<HgiTextureUsageBits, VkImageUsageFlagBits>
_TextureUsageTable[] =
{
    {HgiTextureUsageBitsColorTarget,   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
    {HgiTextureUsageBitsDepthTarget,   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsStencilTarget, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsShaderRead,    VK_IMAGE_USAGE_SAMPLED_BIT},
    {HgiTextureUsageBitsShaderWrite,   VK_IMAGE_USAGE_STORAGE_BIT}
};
static_assert(1 << std::size(_TextureUsageTable) == HgiTextureUsageCustomBitsBegin);

static constexpr std::pair<HgiTextureUsageBits, VkFormatFeatureFlagBits>
_FormatFeatureTable[] =
{
    {HgiTextureUsageBitsColorTarget,   VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT},
    {HgiTextureUsageBitsDepthTarget,   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsStencilTarget, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsShaderRead,    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
    {HgiTextureUsageBitsShaderWrite,   VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT},
};
static_assert(1 << std::size(_FormatFeatureTable) == HgiTextureUsageCustomBitsBegin);

static constexpr std::pair<HgiBufferUsage, VkBufferUsageFlagBits>
_BufferUsageTable[] =
{
    {HgiBufferUsageUniform,  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
    {HgiBufferUsageIndex32,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
    {HgiBufferUsageVertex,   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
    {HgiBufferUsageStorage,  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
    {HgiBufferUsageIndirect, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT},

};
static_assert(1 << std::size(_BufferUsageTable) == HgiBufferUsageCustomBitsBegin);

static constexpr std::pair<HgiCullMode, VkCullModeFlagBits>
_CullModeTable[] =
{
    {HgiCullModeNone,         VK_CULL_MODE_NONE},
    {HgiCullModeFront,        VK_CULL_MODE_FRONT_BIT},
    {HgiCullModeBack,         VK_CULL_MODE_BACK_BIT},
    {HgiCullModeFrontAndBack, VK_CULL_MODE_FRONT_AND_BACK}
};
static_assert(std::size(_CullModeTable) == HgiCullModeCount);

static constexpr std::pair<HgiPolygonMode, VkPolygonMode>
_PolygonModeTable[] =
{
    {HgiPolygonModeFill,  VK_POLYGON_MODE_FILL},
    {HgiPolygonModeLine,  VK_POLYGON_MODE_LINE},
    {HgiPolygonModePoint, VK_POLYGON_MODE_POINT}
};
static_assert(std::size(_PolygonModeTable) == HgiPolygonModeCount);

static constexpr std::pair<HgiWinding, VkFrontFace>
_WindingTable[] =
{
    // We flip the winding order in HgiVulkan. See
    // HgiVulkanGraphicsCmds::SetViewport for details.
    {HgiWindingClockwise,        VK_FRONT_FACE_COUNTER_CLOCKWISE},
    {HgiWindingCounterClockwise, VK_FRONT_FACE_CLOCKWISE}
};
static_assert(std::size(_WindingTable) == HgiWindingCount);

static constexpr std::pair<HgiBindResourceType, VkDescriptorType>
_BindResourceTypeTable[] =
{
    {HgiBindResourceTypeSampler,              VK_DESCRIPTOR_TYPE_SAMPLER},
    {HgiBindResourceTypeSampledImage,         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
    {HgiBindResourceTypeCombinedSamplerImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    {HgiBindResourceTypeStorageImage,         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
    {HgiBindResourceTypeUniformBuffer,        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {HgiBindResourceTypeStorageBuffer,        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
    {HgiBindResourceTypeTessFactors,          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER},
};
static_assert(std::size(_BindResourceTypeTable) == HgiBindResourceTypeCount);

static constexpr std::pair<HgiBlendOp, VkBlendOp>
_blendEquationTable[] =
{
    {HgiBlendOpAdd,             VK_BLEND_OP_ADD},
    {HgiBlendOpSubtract,        VK_BLEND_OP_SUBTRACT},
    {HgiBlendOpReverseSubtract, VK_BLEND_OP_REVERSE_SUBTRACT},
    {HgiBlendOpMin,             VK_BLEND_OP_MIN},
    {HgiBlendOpMax,             VK_BLEND_OP_MAX},
};
static_assert(std::size(_blendEquationTable) == HgiBlendOpCount);

static constexpr std::pair<HgiBlendFactor, VkBlendFactor>
_blendFactorTable[] =
{
    {HgiBlendFactorZero,                  VK_BLEND_FACTOR_ZERO},
    {HgiBlendFactorOne,                   VK_BLEND_FACTOR_ONE},
    {HgiBlendFactorSrcColor,              VK_BLEND_FACTOR_SRC_COLOR},
    {HgiBlendFactorOneMinusSrcColor,      VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR},
    {HgiBlendFactorDstColor,              VK_BLEND_FACTOR_DST_COLOR},
    {HgiBlendFactorOneMinusDstColor,      VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR},
    {HgiBlendFactorSrcAlpha,              VK_BLEND_FACTOR_SRC_ALPHA},
    {HgiBlendFactorOneMinusSrcAlpha,      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
    {HgiBlendFactorDstAlpha,              VK_BLEND_FACTOR_DST_ALPHA},
    {HgiBlendFactorOneMinusDstAlpha,      VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA},
    {HgiBlendFactorConstantColor,         VK_BLEND_FACTOR_CONSTANT_COLOR},
    {HgiBlendFactorOneMinusConstantColor, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR},
    {HgiBlendFactorConstantAlpha,         VK_BLEND_FACTOR_CONSTANT_ALPHA},
    {HgiBlendFactorOneMinusConstantAlpha, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA},
    {HgiBlendFactorSrcAlphaSaturate,      VK_BLEND_FACTOR_SRC_ALPHA_SATURATE},
    {HgiBlendFactorSrc1Color,             VK_BLEND_FACTOR_SRC1_COLOR},
    {HgiBlendFactorOneMinusSrc1Color,     VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR},
    {HgiBlendFactorSrc1Alpha,             VK_BLEND_FACTOR_SRC1_ALPHA},
    {HgiBlendFactorOneMinusSrc1Alpha,     VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA},
};
static_assert(std::size(_blendFactorTable) == HgiBlendFactorCount);

static constexpr std::pair<HgiCompareFunction, VkCompareOp>
_CompareOpTable[] =
{
    {HgiCompareFunctionNever,    VK_COMPARE_OP_NEVER},
    {HgiCompareFunctionLess,     VK_COMPARE_OP_LESS},
    {HgiCompareFunctionEqual,    VK_COMPARE_OP_EQUAL},
    {HgiCompareFunctionLEqual,   VK_COMPARE_OP_LESS_OR_EQUAL},
    {HgiCompareFunctionGreater,  VK_COMPARE_OP_GREATER},
    {HgiCompareFunctionNotEqual, VK_COMPARE_OP_NOT_EQUAL},
    {HgiCompareFunctionGEqual,   VK_COMPARE_OP_GREATER_OR_EQUAL},
    {HgiCompareFunctionAlways,   VK_COMPARE_OP_ALWAYS}
};
static_assert(std::size(_CompareOpTable) == HgiCompareFunctionCount);

static constexpr std::pair<HgiTextureType, VkImageType>
_textureTypeTable[] =
{
    {HgiTextureType1D,      VK_IMAGE_TYPE_1D},
    {HgiTextureType2D,      VK_IMAGE_TYPE_2D},
    {HgiTextureType3D,      VK_IMAGE_TYPE_3D},
    {HgiTextureTypeCubemap, VK_IMAGE_TYPE_2D},
    {HgiTextureType1DArray, VK_IMAGE_TYPE_1D},
    {HgiTextureType2DArray, VK_IMAGE_TYPE_2D}
};
static_assert(std::size(_textureTypeTable) == HgiTextureTypeCount);

static constexpr std::pair<HgiTextureType, VkImageViewType>
_textureViewTypeTable[] =
{
    {HgiTextureType1D,      VK_IMAGE_VIEW_TYPE_1D},
    {HgiTextureType2D,      VK_IMAGE_VIEW_TYPE_2D},
    {HgiTextureType3D,      VK_IMAGE_VIEW_TYPE_3D},
    {HgiTextureTypeCubemap, VK_IMAGE_VIEW_TYPE_CUBE},
    {HgiTextureType1DArray, VK_IMAGE_VIEW_TYPE_1D_ARRAY},
    {HgiTextureType2DArray, VK_IMAGE_VIEW_TYPE_2D_ARRAY}
};
static_assert(std::size(_textureViewTypeTable) == HgiTextureTypeCount);

static constexpr std::pair<HgiSamplerAddressMode, VkSamplerAddressMode>
_samplerAddressModeTable[] =
{
    {HgiSamplerAddressModeClampToEdge,        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE},
    {HgiSamplerAddressModeMirrorClampToEdge,  VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE},
    {HgiSamplerAddressModeRepeat,             VK_SAMPLER_ADDRESS_MODE_REPEAT},
    {HgiSamplerAddressModeMirrorRepeat,       VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT},
    {HgiSamplerAddressModeClampToBorderColor, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER}
};
static_assert(std::size(_samplerAddressModeTable) == HgiSamplerAddressModeCount);

static constexpr std::pair<HgiSamplerFilter, VkFilter>
_samplerFilterTable[] =
{
    {HgiSamplerFilterNearest, VK_FILTER_NEAREST},
    {HgiSamplerFilterLinear,  VK_FILTER_LINEAR}
};
static_assert(std::size(_samplerFilterTable) == HgiSamplerFilterCount);

static constexpr std::pair<HgiMipFilter, VkSamplerMipmapMode>
_mipFilterTable[] =
{
    {HgiMipFilterNotMipmapped, VK_SAMPLER_MIPMAP_MODE_NEAREST /*unused*/},
    {HgiMipFilterNearest,      VK_SAMPLER_MIPMAP_MODE_NEAREST},
    {HgiMipFilterLinear,       VK_SAMPLER_MIPMAP_MODE_LINEAR}
};
static_assert(std::size(_mipFilterTable) == HgiMipFilterCount);

static constexpr std::pair<HgiBorderColor, VkBorderColor>
_borderColorTable[HgiBorderColorCount] =
{
    {HgiBorderColorTransparentBlack, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK},
    {HgiBorderColorOpaqueBlack,      VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK},
    {HgiBorderColorOpaqueWhite,      VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE}
};
static_assert(std::size(_borderColorTable) == HgiBorderColorCount);

static constexpr std::pair<HgiComponentSwizzle, VkComponentSwizzle>
_componentSwizzleTable[] =
{
    {HgiComponentSwizzleZero, VK_COMPONENT_SWIZZLE_ZERO},
    {HgiComponentSwizzleOne,  VK_COMPONENT_SWIZZLE_ONE},
    {HgiComponentSwizzleR,    VK_COMPONENT_SWIZZLE_R},
    {HgiComponentSwizzleG,    VK_COMPONENT_SWIZZLE_G},
    {HgiComponentSwizzleB,    VK_COMPONENT_SWIZZLE_B},
    {HgiComponentSwizzleA,    VK_COMPONENT_SWIZZLE_A}
};
static_assert(std::size(_componentSwizzleTable) == HgiComponentSwizzleCount);

static constexpr std::pair<HgiPrimitiveType, VkPrimitiveTopology>
_primitiveTypeTable[] =
{
    {HgiPrimitiveTypePointList,    VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
    {HgiPrimitiveTypeLineList,     VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
    {HgiPrimitiveTypeLineStrip,    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP},
    {HgiPrimitiveTypeTriangleList, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
    {HgiPrimitiveTypePatchList,    VK_PRIMITIVE_TOPOLOGY_PATCH_LIST},
    {HgiPrimitiveTypeLineListWithAdjacency,
                            VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY}
};
static_assert(std::size(_primitiveTypeTable) == HgiPrimitiveTypeCount);

static const std::pair<HgiFormat, std::string>
_imageLayoutFormatTable[] =
{ 
    {HgiFormatUNorm8,            "r8"},
    {HgiFormatUNorm8Vec2,        "rg8"},
    {HgiFormatUNorm8Vec4,        "rgba8"},
    {HgiFormatSNorm8,            "r8_snorm"},
    {HgiFormatSNorm8Vec2,        "rg8_snorm"},
    {HgiFormatSNorm8Vec4,        "rgba8_snorm"},
    {HgiFormatFloat16,           "r16f"},
    {HgiFormatFloat16Vec2,       "rg16f"},
    {HgiFormatFloat16Vec3,       ""},
    {HgiFormatFloat16Vec4,       "rgba16f"},
    {HgiFormatFloat32,           "r32f"},
    {HgiFormatFloat32Vec2,       "rg32f"},
    {HgiFormatFloat32Vec3,       ""},
    {HgiFormatFloat32Vec4,       "rgba32f" },
    {HgiFormatInt16,             "r16i"},
    {HgiFormatInt16Vec2,         "rg16i"},
    {HgiFormatInt16Vec3,         ""},
    {HgiFormatInt16Vec4,         "rgba16i"},
    {HgiFormatUInt16,            "r16ui"},
    {HgiFormatUInt16Vec2,        "rg16ui"},
    {HgiFormatUInt16Vec3,        ""},
    {HgiFormatUInt16Vec4,        "rgba16ui"},
    {HgiFormatInt32,             "r32i"},
    {HgiFormatInt32Vec2,         "rg32i"},
    {HgiFormatInt32Vec3,         ""},
    {HgiFormatInt32Vec4,         "rgba32i"},
    {HgiFormatUNorm8Vec4srgb,    ""},
    {HgiFormatBC6FloatVec3,      ""},
    {HgiFormatBC6UFloatVec3,     ""},
    {HgiFormatBC7UNorm8Vec4,     ""},
    {HgiFormatBC7UNorm8Vec4srgb, ""},
    {HgiFormatBC1UNorm8Vec4,     ""},
    {HgiFormatBC3UNorm8Vec4,     ""},
    {HgiFormatFloat32UInt8,      ""},
    {HgiFormatPackedInt1010102,  ""},
};
static_assert(std::size(_imageLayoutFormatTable) == HgiFormatCount);

VkFormat
HgiVulkanConversions::GetFormat(HgiFormat inFormat, bool depthFormat)
{
    if (!TF_VERIFY(inFormat != HgiFormatInvalid)) {
        return VK_FORMAT_UNDEFINED;
    }

    VkFormat vkFormat = _FormatTable[inFormat].second;

    // Special case for float32 depth format not properly handled by
    // _FormatTable
    if (depthFormat) {
        if (inFormat == HgiFormatFloat32) {
            vkFormat = VK_FORMAT_D32_SFLOAT;
        } else if (inFormat == HgiFormatFloat32UInt8) {
            vkFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
        }
    }

    return vkFormat;
}

HgiFormat
HgiVulkanConversions::GetFormat(VkFormat inFormat)
{
    if (!TF_VERIFY(inFormat != VK_FORMAT_UNDEFINED)) {
        return HgiFormatInvalid;
    }

    // While HdFormat/HgiFormat does not support BGRA ordering,
    // it is used for swapchain images on many platforms.
    switch (inFormat) {
    case VK_FORMAT_B8G8R8A8_UNORM:
        return HgiFormatUNorm8Vec4;
    case VK_FORMAT_B8G8R8A8_SNORM:
        return HgiFormatSNorm8Vec4;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return HgiFormatUNorm8Vec4srgb;
    default:
        for (const auto& [hgiFormat, vkFormat] : _FormatTable) {
            if (vkFormat == inFormat) {
                return hgiFormat;
            }
        }

        TF_CODING_ERROR("Missing _FormatTable entry: %s",
            string_VkFormat(inFormat));
        return HgiFormatInvalid;
    }
}

VkImageAspectFlags
HgiVulkanConversions::GetImageAspectFlag(HgiTextureUsage usage)
{
    VkImageAspectFlags result = VK_IMAGE_ASPECT_COLOR_BIT;

    if (usage & HgiTextureUsageBitsDepthTarget && 
        usage & HgiTextureUsageBitsStencilTarget) {
        result = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else if (usage & HgiTextureUsageBitsDepthTarget) {
        result = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (usage & HgiTextureUsageBitsStencilTarget) {
        result = VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    return result;
}

VkImageUsageFlags
HgiVulkanConversions::GetImageUsage(HgiTextureUsage tu)
{
    VkImageUsageFlags vkFlags = 0;
    for (const auto& [hgiFlag, vkFlag] : _TextureUsageTable) {
        if (tu & hgiFlag) {
            if (!vkFlag) {
                TF_CODING_ERROR("Unsupported HgiTextureUsage: %u",
                    static_cast<unsigned int>(hgiFlag));
                continue;
            }
            vkFlags |= vkFlag;
        }
    }

    if (!vkFlags) {
        TF_CODING_ERROR("Missing _TextureUsageTable entry: %u",
            static_cast<unsigned int>(tu));
    }
    return vkFlags;
}

HgiTextureUsage
HgiVulkanConversions::GetTextureUsage(VkImageUsageFlags iu)
{
    HgiTextureUsage hgiFlags = 0;
    for (const auto& [hgiFlag, vkFlag] : _TextureUsageTable) {
        if (iu & vkFlag) {
            hgiFlags |= hgiFlag;
        }
    }

    if (!hgiFlags) {
        TF_CODING_ERROR("Missing _TextureUsageTable entry: %s:",
            string_VkImageUsageFlags(iu).c_str());
    }
    return hgiFlags;
}

VkFormatFeatureFlags
HgiVulkanConversions::GetFormatFeature(HgiTextureUsage tu)
{
    VkFormatFeatureFlags2 vkFlags = 0;
    for (const auto& [hgiFlag, vkFlag] : _FormatFeatureTable) {
        if (tu & hgiFlag) {
            if (!vkFlag) {
                TF_CODING_ERROR("Unsupported HgiTextureUsage: %u",
                    static_cast<unsigned int>(hgiFlag));
                continue;
            }
            vkFlags |= vkFlag;
        }
    }

    if (!vkFlags) {
        TF_CODING_ERROR("Missing _FormatFeatureTable entry: %u",
            static_cast<unsigned int>(tu));
    }
    return vkFlags;
}

VkAttachmentLoadOp
HgiVulkanConversions::GetLoadOp(HgiAttachmentLoadOp op)
{
    return _LoadOpTable[op].second;
}

VkAttachmentStoreOp
HgiVulkanConversions::GetStoreOp(HgiAttachmentStoreOp op)
{
    return _StoreOpTable[op].second;
}

VkSampleCountFlagBits
HgiVulkanConversions::GetSampleCount(HgiSampleCount sc)
{
    for (const auto& [hgiSc, vkSc] : _SampleCountTable) {
        if (hgiSc == sc) {
            return vkSc;
        }
    }

    TF_CODING_ERROR("Missing _SampleCountTable entry: %u",
        static_cast<unsigned int>(sc));
    return VK_SAMPLE_COUNT_1_BIT;
}

VkShaderStageFlags
HgiVulkanConversions::GetShaderStages(HgiShaderStage ss)
{
    VkShaderStageFlags vkFlags = 0;
    for (const auto& [hgiFlag, vkFlag] : _ShaderStageTable) {
        if (ss & hgiFlag) {
            if (!vkFlag) {
                TF_CODING_ERROR("Unsupported HgiShaderStage: %u",
                    static_cast<unsigned int>(hgiFlag));
                continue;
            }
            vkFlags |= vkFlag;
        }
    }

    if (!vkFlags) {
        TF_CODING_ERROR("Missing _ShaderStageTable entry: %u",
            static_cast<unsigned int>(ss));
    }
    return vkFlags;
}

VkBufferUsageFlags
HgiVulkanConversions::GetBufferUsage(HgiBufferUsage bu)
{
    VkBufferUsageFlags vkFlags = 0;
    for (const auto& [hgiFlag, vkFlag] : _BufferUsageTable) {
        if (bu & hgiFlag) {
            if (!vkFlag) {
                TF_CODING_ERROR("Unsupported HgiBufferUsage: %u",
                    static_cast<unsigned int>(hgiFlag));
                continue;
            }
            vkFlags |= vkFlag;
        }
    }

    if (!vkFlags) {
        TF_CODING_ERROR("Missing _BufferUsageTable entry: %u",
            static_cast<unsigned int>(bu));
    }
    return vkFlags;
}

VkCullModeFlags
HgiVulkanConversions::GetCullMode(HgiCullMode cm)
{
    return _CullModeTable[cm].second;
}

VkPolygonMode
HgiVulkanConversions::GetPolygonMode(HgiPolygonMode pm)
{
    return _PolygonModeTable[pm].second;
}

VkFrontFace
HgiVulkanConversions::GetWinding(HgiWinding wd)
{
    return _WindingTable[wd].second;
}

VkDescriptorType
HgiVulkanConversions::GetDescriptorType(HgiBindResourceType rt)
{
    return _BindResourceTypeTable[rt].second;
}

VkBlendFactor
HgiVulkanConversions::GetBlendFactor(HgiBlendFactor bf)
{
    return _blendFactorTable[bf].second;
}

VkBlendOp
HgiVulkanConversions::GetBlendEquation(HgiBlendOp bo)
{
    return _blendEquationTable[bo].second;
}

VkCompareOp
HgiVulkanConversions::GetDepthCompareFunction(HgiCompareFunction cf)
{
    return _CompareOpTable[cf].second;
}

VkImageType
HgiVulkanConversions::GetTextureType(HgiTextureType tt)
{
    return _textureTypeTable[tt].second;
}

VkImageViewType
HgiVulkanConversions::GetTextureViewType(HgiTextureType tt)
{
    return _textureViewTypeTable[tt].second;
}

VkSamplerAddressMode
HgiVulkanConversions::GetSamplerAddressMode(HgiSamplerAddressMode a)
{
    return _samplerAddressModeTable[a].second;
}

VkFilter
HgiVulkanConversions::GetMinMagFilter(HgiSamplerFilter mf)
{
    return _samplerFilterTable[mf].second;
}

VkSamplerMipmapMode
HgiVulkanConversions::GetMipFilter(HgiMipFilter mf)
{
    return _mipFilterTable[mf].second;
}

VkBorderColor
HgiVulkanConversions::GetBorderColor(HgiBorderColor bc)
{
    return _borderColorTable[bc].second;
}

VkComponentSwizzle
HgiVulkanConversions::GetComponentSwizzle(HgiComponentSwizzle cs)
{
    return _componentSwizzleTable[cs].second;
}

VkPrimitiveTopology
HgiVulkanConversions::GetPrimitiveType(HgiPrimitiveType pt)
{
    return _primitiveTypeTable[pt].second;
}

const std::string&
HgiVulkanConversions::GetImageLayoutFormatQualifier(HgiFormat inFormat)
{
    const auto& layoutQualifier = _imageLayoutFormatTable[inFormat].second;
    if (layoutQualifier.empty()) {
        TF_WARN("Given HgiFormat %u is not a supported image unit format, "
                "defaulting to rgba16f", static_cast<unsigned int>(inFormat));
        return _imageLayoutFormatTable[HgiFormatFloat16Vec4].second;
    }
    return layoutQualifier;
}

PXR_NAMESPACE_CLOSE_SCOPE
