//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"

#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIWEBGPU_ENABLE_MULTI_DRAW_INDIRECT, false,
    "Use WebGPU multi draw indirect");

HgiWebGPUCapabilities::HgiWebGPUCapabilities(wgpu::Device device)
{
    _maxUniformBlockSize = 64 * 1024;
    _maxShaderStorageBlockSize = 1 * 1024 * 1024 * 1024;
    if (device.HasFeature(wgpu::FeatureName::ClipDistances)) {
        _maxClipDistances = 8;
    } else {
        _maxClipDistances = 0;
    }

#ifdef ARCH_OS_WASM_VM
    // Without this, the default value of 1 causes aligned_malloc to always
    // return 0 (null) Emscripten treats null pointers as valid, making the
    // issue VERY hard to track down
    _pageSizeAlignment = sizeof(void*);
#endif

    bool multiDrawIndirectEnabled =
        TfGetEnvSetting(HGIWEBGPU_ENABLE_MULTI_DRAW_INDIRECT);

    // https://github.com/gfx-rs/wgpu/issues/158#issuecomment-490653129
    _uniformBufferOffsetAlignment = 256;
    _SetFlag(HgiDeviceCapabilitiesBitsCppShaderPadding, false);
    _SetFlag(HgiDeviceCapabilitiesBitsGeometricStage, false);
    _SetFlag(HgiDeviceCapabilitiesBitsBuiltinBarycentrics, false);
    _SetFlag(HgiDeviceCapabilitiesBitsPushConstants, false);
    _SetFlag(HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne, false);
    _SetFlag(
        HgiDeviceCapabilitiesBitsMultiDrawIndirect, multiDrawIndirectEnabled);
    // This might be available in the future
    // https://github.com/gpuweb/gpuweb/issues/4891
    _SetFlag(HgiDeviceCapabilitiesForceEarlyFragmentTest, false);
    _SetFlag(HgiDeviceCapabilitiesBitsTimestamps,
        device.HasFeature(wgpu::FeatureName::TimestampQuery));
    _SetFlag(HgiDeviceCapabilitiesBitsBindlessTextures,
        false); // WGSL not support bindless textures as of 06/2025
    _SetFlag(HgiDeviceCapabilitiesBitsBindlessBuffers,
        false); // WGSL can use similar "storage" buffers
    // Note that HgiDeviceCapabilitiesBitsPrimitiveIdEmulation requires Metal
    // tessellation, so we never support it, regardless of the state of
    // wgpu::FeatureName::PrimitiveIndex (it's not a fallback).
}

HgiWebGPUCapabilities::~HgiWebGPUCapabilities() = default;

int
HgiWebGPUCapabilities::GetAPIVersion() const
{
    return 0;
}

int
HgiWebGPUCapabilities::GetShaderVersion() const
{
    return 460;
}

PXR_NAMESPACE_CLOSE_SCOPE
