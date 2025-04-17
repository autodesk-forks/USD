//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_POOL_CHAIN_INDEX_H
#define PXR_EXEC_VDF_POOL_CHAIN_INDEX_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/output.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// XXX: DO NOT TOUCH OR USE!
///      This duplicates the .pool token defined in EfTokens. Thus, it is
///      crucial that the tokens be kept in sync. Although, these tokens are
///      declared as "public", they are really only to be used internally by
///      the VdfPoolChainIndexer.
///      Note, that the fact that we have this token in Vdf, at all, is very
///      unfortunate. In the future, we would like to clean this up!
#define VDF_POOLCHAININDEX_TOKENS \
    ((pool, ".pool"))

TF_DECLARE_PUBLIC_TOKENS(
    _VdfPoolChainIndexTokens, VDF_API, VDF_POOLCHAININDEX_TOKENS);


/// Opaque pool chain index type.
///
/// Clients may compare indicies to determine the pool chain ordering.
class VdfPoolChainIndex
{
public:
    bool operator<(VdfPoolChainIndex rhs) const
    {
        return _index < rhs._index;
    }

    bool operator<=(VdfPoolChainIndex rhs) const
    {
        return !(rhs < *this);
    }

    bool operator>(VdfPoolChainIndex rhs) const
    {
        return rhs < *this;
    }

    bool operator>=(VdfPoolChainIndex rhs) const
    {
        return !(*this < rhs);
    }

    bool operator==(VdfPoolChainIndex rhs) const
    {
        return _index == rhs._index;
    }

    bool operator!=(VdfPoolChainIndex rhs) const
    {
        return !(*this == rhs);
    }

private:
    friend class VdfPoolChainIndexer;
    VdfPoolChainIndex(int poolChainIndex, uint32_t outputIndex)
        // Combine the pool chain and output indices into a single field.
        //
        // The combined index should be sorted primarily in pool chain order.
        // The invalid pool chain index is -1 so we increment the pool chain
        // index to ensure that the previous implementation's order, which put
        // invalid entries first, is maintained.
        : _index(((static_cast<size_t>(poolChainIndex)+1) << 32ull) |
                 outputIndex)
    {}

private:
    size_t _index;
};


/// Returns \c true if \p output is a pool output.
///
inline bool
Vdf_IsPoolOutput(const VdfOutput &output)
{
    return 
        output.GetAssociatedInput() && 
        output.GetName() == _VdfPoolChainIndexTokens->pool;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
