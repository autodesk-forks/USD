//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/program.h"

#include "pxr/base/tf/token.h"
#include "pxr/exec/ef/timeInputNode.h"
#include "pxr/exec/vdf/node.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_Program::Exec_Program()
{
    // This is not a memory leak. The allocated node becomes owned by the
    // VdfNetwork.
    _timeInputNode = new EfTimeInputNode(&_network);
}

Exec_Program::~Exec_Program() = default;

void Exec_Program::Connect(
    const EsfJournal &journal,
    const SourceOutputs &outputs,
    VdfNode *inputNode,
    const TfToken &inputName)
{
    for (const VdfMaskedOutput &output : outputs) {
        // XXX
        // Note that it's possible for the SourceOutputs contains null outputs.
        // This can happen if the input depends on output keys that could not
        // be compiled (e.g. requesting a computation on a prim which does not
        // have a registered computation of that name). This can be re-visited
        // if output keys contain Exec_ComputationDefinition pointers, as that
        // requires we find a matching computation in order to form that output
        // key.
        if (output) {
            _network.Connect(output, inputNode, inputName);
        }
    }
    _uncompilationTable.AddRulesForInput(
        inputNode->GetId(), inputName, journal);
}

void Exec_Program::_AddNode(const EsfJournal &journal, const VdfNode *node)
{
    _uncompilationTable.AddRulesForNode(node->GetId(), journal);
}

PXR_NAMESPACE_CLOSE_SCOPE
