//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_SYSTEM_H
#define PXR_EXEC_EXEC_SYSTEM_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/esf/stage.h"

#include <tbb/concurrent_vector.h>

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_RequestImpl;
class Exec_Program;
class ExecRequest;
class ExecValueKey;

class EfLeafNodeCache;
class EfTimeInputNode;
template <typename> class TfSpan;
class VdfExecutorInterface;
class VdfMaskedOutput;

/// A system to procedurally compute values based on scene description and
/// computation definitions.
///
/// ExecSystem owns all the structures necessary to compile, schedule and
/// evaluate requested computation values.
///
class ExecSystem
{
public:
    EXEC_API
    explicit ExecSystem(EsfStage &&stage);

    EXEC_API
    ~ExecSystem();

    EXEC_API
    ExecRequest BuildRequest(std::vector<ExecValueKey> &&valueKeys);

    EXEC_API
    void PrepareRequest(const ExecRequest &request);

    EXEC_API
    void GraphNetwork(const char *filename) const;

private:
    // Requires access to _Compile
    friend class Exec_RequestImpl;
    std::vector<VdfMaskedOutput> _Compile(TfSpan<const ExecValueKey> valueKeys);

private:
    EsfStage _stage;

    std::unique_ptr<Exec_Program> _program;
    
    std::unique_ptr<EfLeafNodeCache> _leafNodeCache;

    class _EditMonitor;
    std::unique_ptr<_EditMonitor> _editMonitor;

    std::unique_ptr<VdfExecutorInterface> _executor;
    tbb::concurrent_vector<std::shared_ptr<Exec_RequestImpl>> _requests;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
