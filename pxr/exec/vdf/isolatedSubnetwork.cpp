//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/isolatedSubnetwork.h"

#include "pxr/exec/vdf/connection.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE

VdfIsolatedSubnetworkRefPtr
VdfIsolatedSubnetwork::IsolateBranch(
    VdfConnection           *connection,
    VdfNetwork::EditFilter  *filter)
{
    VdfIsolatedSubnetwork *isolated = new VdfIsolatedSubnetwork(
        &connection->GetTargetNode().GetNetwork());

    // Collect all nodes/connections reachable.
    isolated->_TraverseBranch(connection, filter);
    isolated->_RemoveIsolatedObjectsFromNetwork();

    return TfCreateRefPtr(isolated);
}

VdfIsolatedSubnetworkRefPtr
VdfIsolatedSubnetwork::IsolateBranch(
    VdfNode                 *node,
    VdfNetwork::EditFilter  *filter)
{
    TF_VERIFY(node && !node->HasOutputConnections());

    // If we can't delete the initial node, we bail early.
    if (!filter || filter->CanDelete(node))
    {
        VdfIsolatedSubnetwork *isolated =
            new VdfIsolatedSubnetwork(&node->GetNetwork());

        // Collect all nodes/connections reachable from node.
        // Traverse up all input connections.
        for(VdfConnection *c :  node->GetInputConnections())
            isolated->_TraverseBranch(c, filter);
    
        isolated->_nodes.insert(node);
        isolated->_RemoveIsolatedObjectsFromNetwork();
    
        return TfCreateRefPtr(isolated);
    }

    return TfNullPtr;
}


// --- VdfIsolatedSubnetwork ---------------------------------------------------
// --- VdfIsolatedSubnetwork ---------------------------------------------------
// --- VdfIsolatedSubnetwork ---------------------------------------------------


VdfIsolatedSubnetwork::VdfIsolatedSubnetwork(VdfNetwork *network)
: _network(network)
{
    TF_VERIFY(_network);
}

VdfIsolatedSubnetwork::~VdfIsolatedSubnetwork()
{
    for(VdfConnection *c : _connections)
        _network->_DeleteConnection(c);

    for(VdfNode *n : _nodes)
        _network->_DeleteNode(n);
}

bool
VdfIsolatedSubnetwork::_CanTraverse(
    VdfConnection           *connection,
    VdfNetwork::EditFilter  *filter,
    const VdfConnectionSet  &visitedConnections)
{
    VdfNode &sourceNode = connection->GetSourceNode();

    // Can we delete the source node of that connection? If so,
    // recurse, else ignore this connection.
    if (filter && !filter->CanDelete(&sourceNode))
        return false;

    // If there is more than one output connection (ie. an additional one
    // besides the one we use to discover this node), stop traversing since
    // other nodes are using part of the network above this point.
    //
    // Since we don't delete connection right away (in order to get correct 
    // paths), we need to see what we've seen before in order to determine if
    // there is an additional output.

    VdfConnectionVector outputConnections = sourceNode.GetOutputConnections();

    for(size_t i=0; i<outputConnections.size(); i++)
        if (visitedConnections.count(outputConnections[i]) == 0)
            return false;

    return true;
}

void
VdfIsolatedSubnetwork::_TraverseBranch(
    VdfConnection           *connection,
    VdfNetwork::EditFilter  *filter)
{
    std::stack<VdfConnection*> stack;
    stack.push(connection);

    while (!stack.empty())
    {
        VdfConnection *currentConnection = stack.top();
        stack.pop();

        // Mark this connection as visited.
        _connections.insert(currentConnection);
    
        // We can't traverse this connection, therefore we stop.
        if (!_CanTraverse(currentConnection, filter, _connections))
            continue;
    
        VdfNode &sourceNode = currentConnection->GetSourceNode();
    
        std::pair<NodeSet::iterator, bool> res = _nodes.insert(&sourceNode);
    
        // We only traverse if the object wasn't visited before. This can 
        // happen if different inputs of the same node are connected to source 
        // outputs of the same node.
    
        if (res.second)
        {
            VdfConnectionVector l = sourceNode.GetInputConnections();

            // Push the connections onto the stack in reverse order, so that
            // on the next iteration, the first connection lives on top of the
            // stack and gets picked up first.
            TF_REVERSE_FOR_ALL(rit, l)
                stack.push(*rit);
        }
    }
}

void
VdfIsolatedSubnetwork::_RemoveIsolatedObjectsFromNetwork()
{
    // First, remove all connections. This happens before any nodes are removed
    // to match the order in which the VdfNetwork sends out deletion notices.
    for(VdfConnection *c : _connections)
        _network->_RemoveConnection(c);

    // Remove all nodes from the network so that the network is in a consistent
    // state. Note that the nodes are not deleted.
    for(VdfNode *n : _nodes)
        _network->_RemoveNode(n);
}

PXR_NAMESPACE_CLOSE_SCOPE
