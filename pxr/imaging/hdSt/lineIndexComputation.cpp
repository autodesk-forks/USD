//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/lineIndexComputation.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_LineIndexComputation::HdSt_LineIndexComputation(
    HdMeshTopologySharedPtr const &topology, SdfPath const &id)
    : _id(id), _topology(topology)
{
}

void
HdSt_LineIndexComputation::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->emplace_back(HdTokens->indices, HdTupleType{HdTypeInt32Vec2, 1});
}

bool
HdSt_LineIndexComputation::Resolve()
{
    if (!_TryLock()) return false;
    HD_TRACE_FUNCTION();

    VtVec3iArray triangleIndices;
    VtIntArray primitiveParam;
    VtIntArray trianglesEdgeIndices;
    HdMeshUtil meshUtil(_topology.get(), _id);
    meshUtil.ComputeTriangleIndices(
        &triangleIndices,
        &primitiveParam,
        &trianglesEdgeIndices);

    const size_t numTriangles = triangleIndices.size();
    VtVec2iArray lineIndices(numTriangles * 3);
    for (size_t i = 0; i < numTriangles; ++i) {
        const int i0 = triangleIndices[i][0];
        const int i1 = triangleIndices[i][1];
        const int i2 = triangleIndices[i][2];
        lineIndices[i * 3 + 0].Set(i0, i1);
        lineIndices[i * 3 + 1].Set(i1, i2);
        lineIndices[i * 3 + 2].Set(i2, i0);
    }

    _SetResult(std::make_shared<HdVtBufferSource>(
        HdTokens->indices, VtValue(lineIndices)));
    _SetResolved();
    return true;
}

bool
HdSt_LineIndexComputation::_CheckValid() const
{
    return (_topology != nullptr);
}

PXR_NAMESPACE_CLOSE_SCOPE 