//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_BUFFER_DESC_H
#define PXR_IMAGING_HD_ST_EXT_BUFFER_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/extGpuBufferSchema.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/buffer.h"

#include "pxr/base/tf/token.h"

#include <cstddef>
#include <cstdint>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct HdStExtGpuBufferDesc
///
/// Storm-private, decoded form of an HdExtGpuBufferSchema.  This is the only
/// place the external-buffer fields are flattened into a POD; there is no
/// shared hd-level descriptor type anymore.  Constructed by Storm's enrichment
/// step during \c _PopulateVertexPrimvars via \c FromSchema and never placed
/// into \c VtValue.
///
struct HdStExtGpuBufferDesc
{
    // --- Decoded descriptor fields (formerly the hd-level POD) ---

    /// Element type and tuple count.  Named tupleType to match HdBufferSpec /
    /// HdBufferSource::GetTupleType(); sourced from the schema's elementType
    /// member.
    HdTupleType tupleType = {HdTypeInvalid, 0};
    size_t   numElements = 0;
    size_t   byteOffset = 0;
    size_t   byteStride = 0;
    TfToken  backendApi;
    uint64_t rawHandle = 0;
    size_t   rawHandleByteSize = 0;
    bool     directBindable = false;

    // Non-owning HgiBuffer wrapper around rawHandle, used as the source buffer
    // in GPU-to-GPU blit ops.  Owned by the HdStExtGpuBufferSource that
    // populates this descriptor.
    HgiBufferHandle cachedHgiHandle;

    /// Decode the renderer-agnostic HdExtGpuBufferSchema into this Storm-local
    /// POD.  Returns nullopt when the schema is undefined or incomplete
    /// (missing backendApi / rawHandle / numElements / elementType), in which
    /// case the caller falls back to the CPU primvar path.  Optional members
    /// (byteOffset / byteStride / rawHandleByteSize / directBindable) default
    /// to 0 / false when absent.
    static std::optional<HdStExtGpuBufferDesc>
    FromSchema(const HdExtGpuBufferSchema &schema)
    {
        if (!schema || !schema.IsComplete()) {
            return std::nullopt;
        }

        HdStExtGpuBufferDesc d;
        d.backendApi  = schema.GetBackendApi()->GetTypedValue(0.0f);
        d.rawHandle   = schema.GetRawHandle()->GetTypedValue(0.0f);
        d.numElements = schema.GetNumElements()->GetTypedValue(0.0f);
        d.tupleType   = schema.GetElementType()->GetTypedValue(0.0f);

        if (const HdSizetDataSourceHandle s = schema.GetRawHandleByteSize()) {
            d.rawHandleByteSize = s->GetTypedValue(0.0f);
        }
        if (const HdSizetDataSourceHandle s = schema.GetByteOffset()) {
            d.byteOffset = s->GetTypedValue(0.0f);
        }
        if (const HdSizetDataSourceHandle s = schema.GetByteStride()) {
            d.byteStride = s->GetTypedValue(0.0f);
        }
        if (const HdBoolDataSourceHandle s = schema.GetDirectBindable()) {
            d.directBindable = s->GetTypedValue(0.0f);
        }
        return d;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_BUFFER_DESC_H
