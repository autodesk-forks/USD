//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HGI_WEBGPU_DIAGNOSTIC_H
#define PXR_IMAGING_HGI_WEBGPU_DIAGNOSTIC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"

PXR_NAMESPACE_OPEN_SCOPE

HGIWEBGPU_API
bool
HgiWebGPUIsDebugEnabled();

template<typename Encoder>
constexpr bool isCommandEncoder =
    std::is_same_v<Encoder, wgpu::CommandEncoder> ||
    std::is_same_v<Encoder, wgpu::RenderPassEncoder> ||
    std::is_same_v<Encoder, wgpu::ComputePassEncoder> ||
    std::is_same_v<Encoder, wgpu::RenderBundleEncoder>;

/// Begin a label in a webgpu command encoder
template<typename Encoder>
HGIWEBGPU_API
std::enable_if_t<isCommandEncoder<Encoder>>
HgiWebGPUBeginLabel(Encoder const &encoder, const char* label)
{
    if (!HgiWebGPUIsDebugEnabled() || !label) {
        return;
    }
    encoder.PushDebugGroup(label);
}

/// End the last pushed label in a webgpu command encoder
template<typename Encoder>
HGIWEBGPU_API
std::enable_if_t<isCommandEncoder<Encoder>>
HgiWebGPUEndLabel(Encoder const &encoder)
{
    if (!HgiWebGPUIsDebugEnabled()) {
        return;
    }
    encoder.PopDebugGroup();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HGI_WEBGPU_DIAGNOSTIC_H
