//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBufferConsumer.h"

#include "pxr/imaging/hdSt/extBufferDesc.h"
#include "pxr/imaging/hdSt/extGpuBufferArrayRange.h"
#include "pxr/imaging/hdSt/extGpuBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hgi/hgi.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

HdExtGpuBufferSchema
HdSt_GetExtGpuBufferSchema(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    TfToken const &name)
{
    HdSceneIndexBaseRefPtr si =
        sceneDelegate->GetRenderIndex().GetTerminalSceneIndex();
    if (!si) {
        return HdExtGpuBufferSchema(nullptr);
    }
    const HdContainerDataSourceHandle primDs = si->GetPrim(id).dataSource;
    const HdPrimvarSchema pv =
        HdPrimvarsSchema::GetFromParent(primDs).GetPrimvar(name);
    return HdExtGpuBufferSchema::GetFromParent(pv.GetContainer());
}

HdBufferSourceSharedPtr
HdSt_TryCreateExtGpuBufferSource(
    TfToken const &name,
    HdExtGpuBufferSchema const &schema,
    HdStResourceRegistry *registry)
{
    if (!schema || !schema.IsComplete()) {
        return nullptr;
    }

    std::optional<HdStExtGpuBufferDesc> hdDescOpt =
        HdStExtGpuBufferDesc::FromSchema(schema);
    if (!hdDescOpt) {
        return nullptr;
    }
    HdStExtGpuBufferDesc const &hdDesc = *hdDescOpt;

    // Validate the buffer's backend matches the active Hgi. backendApi carries
    // the same token Hgi reports (HgiTokens->OpenGL / Vulkan / Metal), so we
    // compare directly against GetAPIName() -- no local token mapping needed.
    Hgi *hgi = registry->GetHgi();
    if (!hgi || hdDesc.backendApi != hgi->GetAPIName()) {
        HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferFallbackCount);
        return nullptr;
    }

    // Bounds check (if size known).  Required fields (numElements, rawHandle,
    // tupleType) are already guaranteed non-degenerate by IsComplete/FromSchema.
    if (hdDesc.rawHandleByteSize > 0) {
        size_t elemSize = HdDataSizeOfTupleType(hdDesc.tupleType);
        size_t stride = (hdDesc.byteStride > 0)
            ? hdDesc.byteStride : elemSize;
        size_t requiredEnd =
            hdDesc.byteOffset + hdDesc.numElements * stride;
        if (requiredEnd > hdDesc.rawHandleByteSize) {
            HD_PERF_COUNTER_INCR(
                HdStPerfTokens->extGpuBufferFallbackCount);
            return nullptr;
        }
    }

    // The source owns its non-owning HgiBuffer wrapper internally.
    return std::make_shared<HdStExtGpuBufferSource>(name, hdDesc);
}

HdBufferArrayRangeSharedPtr
HdSt_TryCreateExtGpuBufferAliasBAR(
    HdBufferSourceSharedPtrVector const &sources,
    HdStResourceRegistry *registry,
    HdBufferArrayRangeSharedPtr const &existingBar)
{
    if (sources.empty()) {
        return nullptr;
    }

    // All sources must be direct-bindable external GPU sources.
    for (auto const &src : sources) {
        auto const *extSrc =
            dynamic_cast<HdStExtGpuBufferSource const *>(src.get());
        if (!extSrc) {
            return nullptr;
        }
        if (!extSrc->GetDescriptor().directBindable) {
            return nullptr;
        }
    }

    // Fast path: reuse existing alias BAR in-place.
    if (existingBar) {
        auto *aliasRange = dynamic_cast<HdStExtGpuBufferArrayRange *>(
            existingBar.get());
        if (aliasRange) {
            if (aliasRange->UpdateExternalResources(sources)) {
                return existingBar;
            }
            if (aliasRange->MergeExternalResources(sources)) {
                return existingBar;
            }
        }
    }

    // Slow path: create a new alias BAR.
    auto aliasBAR = std::make_shared<HdStExtGpuBufferArrayRange>(
        registry);

    for (auto const &src : sources) {
        auto const *extSrc =
            static_cast<HdStExtGpuBufferSource const *>(src.get());
        auto const &hdDesc = extSrc->GetDescriptor();

        size_t elemSize = HdDataSizeOfTupleType(hdDesc.tupleType);
        size_t byteSize = hdDesc.rawHandleByteSize > 0
            ? hdDesc.rawHandleByteSize
            : hdDesc.numElements * elemSize;

        aliasBAR->SetExternalResource(
            src->GetName(),
            hdDesc.rawHandle,
            byteSize,
            hdDesc.tupleType,
            hdDesc.numElements,
            hdDesc.byteOffset);
    }

    return aliasBAR;
}

PXR_NAMESPACE_CLOSE_SCOPE
