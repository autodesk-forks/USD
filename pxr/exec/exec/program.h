//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_PROGRAM_H
#define PXR_EXEC_EXEC_PROGRAM_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/compiledOutputCache.h"
#include "pxr/exec/exec/uncompilationTable.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/network.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class EfTimeInputNode;
class EsfJournal;
class VdfNode;

/// Owns a VdfNetwork and related data structures to access and modify the
/// network.
///
/// The VdfNetwork describes the toplogical structure of nodes and connections,
/// but does not prescribe any meaning to the organization of the network. In
/// order to efficiently compile, update, and evaluate the network, Exec
/// requires additional metadata to facilitate common access patterns.
///
/// Generally, the data structures contained by this class are those that must
/// have exactly one instance per-network. The responsibilities of these data
/// structures include:
///
///   - Tracking which VdfOutput provides the value of a given Exec_OutputKey.
///   - Tracking the conditions when specific nodes and connections should be
///     deleted from the network.
///
/// Some of these data structures must be modified when the network is modified.
/// Therefore, compilation never directly accesses the VdfNetwork, but does so
/// through an Exec_Program.
///
class Exec_Program
{
public:
    Exec_Program();
    ~Exec_Program();

    /// Non-copyable and non-movable.
    Exec_Program(const Exec_Program &) = delete;
    Exec_Program& operator=(const Exec_Program &) = delete;

    /// Adds a new node in the VdfNetwork.
    ///
    /// Constructs a node of type \p NodeType. The first argument of the node
    /// constructor is a pointer to the VdfNetwork maintained by this
    /// Exec_Program, and the remaining arguments are forwarded from
    /// \p nodeCtorArgs.
    ///
    /// Uncompilation rules for the new node are added from the \p journal
    /// entries.
    ///
    /// \return a pointer to the newly constructed node. This pointer is owned
    /// by the network.
    ///
    template <class NodeType, class... NodeCtorArgs>
    NodeType *CreateNode(
        const EsfJournal &journal,
        NodeCtorArgs... nodeCtorArgs);

    /// Collection of VdfMaskedOutputs that feed into a VdfInput.
    ///
    /// Most inputs only source from one output, hence the choice of
    /// TfSmallVector.
    //
    using SourceOutputs = TfSmallVector<VdfMaskedOutput, 1>;

    /// Makes connections between nodes in the VdfNetwork.
    ///
    /// All non-null VdfMaskedOutputs in \p outputs are connected to the input
    /// named \p inputName on \p inputNode. Null outputs are skipped.
    ///
    /// Even if \p outputs is empty or lacks non-null outputs, this method
    /// should still be called in order to properly add uncompilation rules from
    /// the \p journal.
    ///
    void Connect(
        const EsfJournal &journal,
        const SourceOutputs &outputs,
        VdfNode *inputNode,
        const TfToken &inputName);

    /// Returns the mapping from Exec_OutputKey to VdfMaskedOutputs for the
    /// network.
    ///
    Exec_CompiledOutputCache *GetCompiledOutputCache()
    {
        return &_compiledOutputCache;
    }

    /// Gets the underlying VdfNetwork.
    VdfNetwork &GetNetwork() { return _network; }
    const VdfNetwork &GetNetwork() const { return _network; }

private:
    // Updates data strucutres for a newly-added node.
    void _AddNode(const EsfJournal &journal, const VdfNode *node);

    VdfNetwork _network;
    EfTimeInputNode *_timeInputNode;
    Exec_CompiledOutputCache _compiledOutputCache;
    Exec_UncompilationTable _uncompilationTable;
};


template <class NodeType, class... NodeCtorArgs>
NodeType *Exec_Program::CreateNode(
    const EsfJournal &journal,
    NodeCtorArgs... nodeCtorArgs)
{
    static_assert(std::is_base_of_v<VdfNode, NodeType>);
    NodeType *node = new NodeType(
        &_network, std::forward<NodeCtorArgs>(nodeCtorArgs)...);
    _AddNode(journal, node);
    return node;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif