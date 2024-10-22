#ifndef PXR_IMAGING_HGI_WEBGPU_SPIRVTRANSFORMS_H
#define PXR_IMAGING_HGI_WEBGPU_SPIRVTRANSFORMS_H

#include "pxr/pxr.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/imaging/hgiWebGPU/api.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    HGIWEBGPU_DEBUG_SPIRV_TRANSFORM
);

HGIWEBGPU_API
bool ApplySpirvViewportFlip(
    std::vector<uint32_t>& spirv);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
