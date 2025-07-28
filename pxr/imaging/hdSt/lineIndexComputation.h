//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_ST_LINE_INDEX_COMPUTATION_H
#define PXR_IMAGING_HD_ST_LINE_INDEX_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/meshTopology.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_LineIndexComputation
///
/// A computation that generates a line index buffer (vec2i) from a
/// mesh's triangle topology. This is used as a fallback for wireframe
/// rendering when there is no support for tesselation nor built-in barycentrics.
///
class HdSt_LineIndexComputation : public HdComputedBufferSource {
public:
    HDST_API
    HdSt_LineIndexComputation(
        HdMeshTopologySharedPtr const &topology,
        SdfPath const &id);

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

    HDST_API
    bool Resolve() override;

protected:
    HDST_API
    bool _CheckValid() const override;

private:
    SdfPath const _id;
    HdMeshTopologySharedPtr _topology;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_LINE_INDEX_COMPUTATION_H 