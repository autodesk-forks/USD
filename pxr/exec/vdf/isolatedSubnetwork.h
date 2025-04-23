//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ISOLATED_SUBNETWORK_H
#define PXR_EXEC_VDF_ISOLATED_SUBNETWORK_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/refBase.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(VdfIsolatedSubnetwork);

class VdfNode;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfIsolatedSubnetwork
///
/// A VdfIsolatedSubnetwork is a collection of VdfNodes and their connections 
/// that are disconnected from their network.
///
/// For the purpose of execution and graphing the nodes/connections being held
/// on to appear to be deleted. WillDelete(); notices will be send out during
/// the isolation process (even though objects are only removed not deleted).
/// All notices will be sent out before nodes will be deleted, and before
/// connections will be disconnected.
///
/// However, all objects within the isolated network have kept their identity,
/// since no object has been deleted.
///
/// Ownership of the isolated nodes and connections has been transfered from
/// the VdfNetwork to this object. As a result, deletion of a
/// VdfIsolatedSubnetwork will result all nodes and connections will be deleted.
///
class VdfIsolatedSubnetwork : public TfRefBase
{
public:

    // A set of nodes to paths used during deletion of sub-networks.
    typedef TfHashSet<VdfNode *, TfHash> NodeSet;

    /// Isolates all nodes and connections reachable via input connections from
    /// \p connection that are forming a single branch (ie. that are not 
    /// connected via additional output connections to other parts of the
    /// network).
    ///
    /// Note that \p connection will be in the set of isolated objects.
    ///
    /// Optional \p filter object can be used to fine tune edit operations on
    /// the network.
    ///
    /// Returns the isolated network. As soon as there are no more references
    /// to the returned object, the actual nodes and connections will be
    /// deleted.
    ///
    /// The source node of \p connection will be considered the root node of the
    /// isolated branch.
    ///
    VDF_API static
    VdfIsolatedSubnetworkRefPtr IsolateBranch(
        VdfConnection           *connection,
        VdfNetwork::EditFilter  *filter);

    /// Isolates all nodes and connections reachable via input connections from
    /// \p node that are forming a single branch (ie. that are not connected
    /// via additional output connections to other parts of the network).
    ///
    /// Optional \p filter object can be used to fine tune edit operations on
    /// the network.
    ///
    /// Returns the isolated network. As soon as there are no more references
    /// to the returned object, the actual nodes and connections will be
    /// deleted.
    ///
    /// \p node will be considered the root node of the isolated branch.
    ///
    VDF_API static
    VdfIsolatedSubnetworkRefPtr IsolateBranch(
        VdfNode                 *node,
        VdfNetwork::EditFilter  *filter);

    /// Returns the set of isolated nodes.
    ///
    const NodeSet &GetIsolatedNodes() const
    {
        return _nodes;
    }

    /// Returns the set of isolated nodes.
    ///
    const VdfConnectionSet &GetIsolatedConnections() const
    {
        return _connections;
    }

// -----------------------------------------------------------------------------

private:

    // Ctor.
    VdfIsolatedSubnetwork(VdfNetwork *network);

    // Dtor.
    ~VdfIsolatedSubnetwork();

    // Helper that checks if we can traverse a connection.  
    static bool _CanTraverse(
        VdfConnection          *connection,
        VdfNetwork::EditFilter *filter,
        const VdfConnectionSet &visitedConnections);

    // Helper that traverses a branch.
    void _TraverseBranch(
        VdfConnection          *connection,
        VdfNetwork::EditFilter *filter);

    // Helper that removes all isolated objects from the network.
    void _RemoveIsolatedObjectsFromNetwork();

private:

    // The network
    VdfNetwork *_network;

    // The set of nodes that are isolated.
    NodeSet _nodes;
    
    // The set of connections that are isolated.
    VdfConnectionSet _connections;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
