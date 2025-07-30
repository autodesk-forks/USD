//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_NODE_RECOMPILATION_INFO_H
#define PXR_EXEC_EXEC_NODE_RECOMPILATION_INFO_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/inputKey.h"

#include "pxr/exec/esf/object.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfInput;

/// Stores required information to recompile inputs of an arbitrary node.
class Exec_NodeRecompilationInfo
{
public:
    Exec_NodeRecompilationInfo(
        const EsfObject &provider,
        Exec_InputKeyVectorConstRefPtr &&inputKeys)
        : _provider(provider)
        , _inputKeys(std::move(inputKeys))
    {}

    /// Gets the provider of the node, which serves as the input resolution
    /// origin.
    ///
    const EsfObject &GetProvider() const {
        return _provider;
    }

    /// Gets the input key to re-resolve \p input on the node.
    ///
    /// Returns nullptr if an input key could not be found.
    ///
    const Exec_InputKey *GetInputKey(const VdfInput &input) const;

private:
    // The node's provider.
    // TODO: This needs to be updated in response to namespace edits.
    const EsfObject _provider;
    
    Exec_InputKeyVectorConstRefPtr _inputKeys;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif