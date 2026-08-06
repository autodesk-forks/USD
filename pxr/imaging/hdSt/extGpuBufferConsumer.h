//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_CONSUMER_H
#define PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_CONSUMER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/extGpuBufferSchema.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdStResourceRegistry;

/// \file extGpuBufferConsumer.h
///
/// Storm-internal helpers, shared by HdStMesh / HdStPoints / HdStBasisCurves,
/// that turn a producer-published HdExtGpuBufferSchema on a primvar into a
/// Storm buffer source / zero-copy alias BAR. See external_gpu_buffer_proposal.

/// Fetch the HdExtGpuBufferSchema (if any) for primvar \p name off the prim's
/// container data source in the terminal scene index. Returns an undefined
/// schema when the prim / primvar / extGpuBuffer child is absent (e.g. a legacy
/// scene delegate that never authors it) — the caller treats that as "no
/// external buffer, use the CPU path".
HDST_API
HdExtGpuBufferSchema
HdSt_GetExtGpuBufferSchema(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    TfToken const &name);

/// Try to create an HdStExtGpuBufferSource (a buffer source with no CPU
/// payload) from \p schema. Returns nullptr if the schema is absent/incomplete
/// or enrichment fails (backend mismatch, out-of-bounds), in which case the
/// caller should fall through to its existing CPU path.
HDST_API
HdBufferSourceSharedPtr
HdSt_TryCreateExtGpuBufferSource(
    TfToken const &name,
    HdExtGpuBufferSchema const &schema,
    HdStResourceRegistry *registry);

/// If every source in \p sources is a direct-bindable external GPU source,
/// return a zero-copy alias BAR wrapping the external handles; otherwise
/// nullptr. When \p existingBar is already an HdStExtGpuBufferArrayRange it is
/// updated in place and returned (same pointer), so HdStUpdateDrawItemBAR does
/// not mark draw batches dirty.
HDST_API
HdBufferArrayRangeSharedPtr
HdSt_TryCreateExtGpuBufferAliasBAR(
    HdBufferSourceSharedPtrVector const &sources,
    HdStResourceRegistry *registry,
    HdBufferArrayRangeSharedPtr const &existingBar = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_CONSUMER_H
