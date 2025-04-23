//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/system.h"

#include "pxr/exec/exec/compiledOutputCache.h"
#include "pxr/exec/exec/compiler.h"
#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/request.h"
#include "pxr/exec/exec/requestImpl.h"

#include "pxr/exec/ef/executor.h"
#include "pxr/exec/ef/leafNodeCache.h"
#include "pxr/exec/ef/timeInputNode.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/parallelDataManagerVector.h"
#include "pxr/exec/vdf/parallelExecutorEngine.h"
#include "pxr/exec/vdf/parallelSpeculationExecutorEngine.h"

#include "pxr/base/tf/span.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class ExecSystem::_EditMonitor final : public VdfNetwork::EditMonitor {
public:
    explicit _EditMonitor(EfLeafNodeCache *const leafNodeCache) :
        _leafNodeCache(leafNodeCache)
    {}

    void WillClear() override {
        _leafNodeCache->Clear();
    }

    void DidConnect(const VdfConnection *connection) override {
        _leafNodeCache->DidConnect(*connection);
    }

    void DidAddNode(const VdfNode *node) override {}

    void WillDelete(const VdfConnection *connection) override {
        _leafNodeCache->WillDeleteConnection(*connection);
    }

    void WillDelete(const VdfNode *node) override {}

private:
    EfLeafNodeCache *const _leafNodeCache;
};

ExecSystem::ExecSystem(EsfStage &&stage) :
    _stage(std::move(stage)),
    _program(std::make_unique<Exec_Program>()),
    _leafNodeCache(std::make_unique<EfLeafNodeCache>()),
    _editMonitor(std::make_unique<_EditMonitor>(_leafNodeCache.get())),
    _executor(std::make_unique<EfExecutor<
                  VdfParallelExecutorEngine, VdfParallelDataManagerVector>>())
{
    _program->GetNetwork().RegisterEditMonitor(_editMonitor.get());
}

ExecSystem::~ExecSystem()
{
    _program->GetNetwork().UnregisterEditMonitor(_editMonitor.get());
}

ExecRequest
ExecSystem::BuildRequest(std::vector<ExecValueKey> &&valueKeys)
{
    const std::shared_ptr<Exec_RequestImpl> &impl = *_requests.push_back(
        std::make_shared<Exec_RequestImpl>(std::move(valueKeys)));
    return ExecRequest(impl);
}

void
ExecSystem::PrepareRequest(const ExecRequest &request)
{
    if (std::shared_ptr<Exec_RequestImpl> impl = request._impl.lock()) {
        impl->Compile(this);
        impl->Schedule();
    }
}

void
ExecSystem::GraphNetwork(const char *filename) const
{
    VdfGrapher::GraphToFile(_program->GetNetwork(), filename);
}

std::vector<VdfMaskedOutput>
ExecSystem::_Compile(TfSpan<const ExecValueKey> valueKeys)
{
    Exec_Compiler compiler(_stage, _program.get());
    return compiler.Compile(valueKeys);
}

PXR_NAMESPACE_CLOSE_SCOPE
