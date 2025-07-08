//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILATION_TASK_H
#define PXR_EXEC_EXEC_COMPILATION_TASK_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/compilerTaskSync.h"

#include <tbb/task.h>

#include <cstdint>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationState;
class Exec_OutputKey;

/// Base class for parallel compilation tasks.
///
class Exec_CompilationTask : public tbb::task
{
public:
    explicit Exec_CompilationTask(Exec_CompilationState &compilationState) :
        _compilationState(compilationState),
        _taskStage(0)
    {}

    tbb::task *execute() final;

protected:
    class TaskStages;
    class TaskDependencies;

    /// Main entry point of a compilation task to be implemented in the
    /// derived class.
    /// 
    /// Note, we deliberately chose mutable reference types for these
    /// parameters, as opposed to pointers as required for output parameters per
    /// our coding conventions. We attempt to optimize for readability in the
    /// overrides (of which there will be many) compared to the very short (and
    /// likely not frequently changing) call site in execute(). The spirit
    /// behind the convention is to bring clarity to what is an output parameter
    /// at the call site, which is often more important, but irrelevant in this
    /// particular case.
    virtual void _Compile(Exec_CompilationState &, TaskStages &) = 0;

    /// Called from the _Compile method in the derived class to indicate that
    /// the task identified by \c key has been completed. This must be called
    /// *after* the task published its results.
    void _MarkDone(const Exec_OutputKey &key);

private:
    // State persistent to one round of compilation
    Exec_CompilationState &_compilationState;

    // Task stage
    uint32_t _taskStage;
};

/// Manages the task dependencies established during task stages.
///
class Exec_CompilationTask::TaskDependencies {
public:
    /// Constructs and runs a new subtask and establishes the subtask as a
    /// dependency of the calling task. The calling task's _Compile method will
    /// automatically be re-executed once all dependencies have been fulfilled.
    template<class TaskType, class ... Args>
    void NewSubtask(Args&&... args);

    /// Claims a subtask identified by the provided \p key as a dependency. If
    /// the claimed subtask has already been claimed by another task, the
    /// calling task will establish a dependency on the subtask and the _Compile
    /// method will automatically be re-executed once all dependencies have been
    /// fulfilled.
    Exec_CompilerTaskSync::ClaimResult ClaimSubtask(const Exec_OutputKey &key);

private:
    friend class Exec_CompilationTask::TaskStages;

    TaskDependencies(
        tbb::task *successor,
        Exec_CompilationState &compilationState) :
        _successor(successor),
        _compilationState(compilationState),
        _hasDependencies(false)
    {}

    bool _HasDependencies() const {
        return _hasDependencies;
    }

    tbb::task *const _successor;
    Exec_CompilationState &_compilationState;
    bool _hasDependencies;
};

template<class TaskType, class ... Args>
void Exec_CompilationTask::TaskDependencies::NewSubtask(Args&&... args)
{
    _hasDependencies = true;
    tbb::task *task =
        new (tbb::task::allocate_additional_child_of(*_successor))
            TaskType(std::forward<Args>(args)...);
    tbb::task::spawn(*task);
}

/// Manages the callables associated with task stages.
/// 
/// Sequentially advances through stages, putting the task to sleep between
/// stages while there are unfulfilled dependencies, and then automatically
/// re-executing the _Compile method with the next stage once all dependencies
/// have been fulfilled.
/// 
class Exec_CompilationTask::TaskStages {
public:
    /// Invokes the callables in order, each denoting a task stage.
    template<typename... Callables>
    void Invoke(Callables&&... callables) {
        _InvokeOne(0, std::forward<Callables>(callables)...);
    }

private:
    friend class Exec_CompilationTask;

    TaskStages(
        tbb::task *successor,
        Exec_CompilationState &compilationState,
        uint32_t *taskStage) :
        _successor(successor),
        _compilationState(compilationState),
        _taskStage(taskStage)
    {}

    void _InvokeOne(uint32_t) {}

    template<typename Callable, typename... Tail>
    void _InvokeOne(
        uint32_t i,
        Callable&& callable,
        Tail&&... tail);

    tbb::task *const _successor;
    Exec_CompilationState &_compilationState;
    uint32_t *const _taskStage;
};

template<typename Callable, typename... Tail>
void Exec_CompilationTask::TaskStages::_InvokeOne(
    uint32_t i,
    Callable&& callable,
    Tail&&... tail)
{
    // If this is the active stage, invoke the callable.
    if (i >= *_taskStage) {
        // Increment the task reference count to make sure it does not
        // prematurely reach 0 as dependencies are fulfilled concurrently.
        _successor->increment_ref_count();

        // Construct the TaskDependencies instance and invoke the callable
        TaskDependencies taskDependencies(_successor, _compilationState);
        std::forward<Callable>(callable)(taskDependencies);

        // Advance to the next stage.
        ++(*_taskStage);

        // If dependencies were established, recycle the calling task. This will
        // ensure that the last dependency task will respawn this task when the
        // ref count reaches 0.
        // This implicitly decrements the ref count after returning.
        if (taskDependencies._HasDependencies()) {
            _successor->recycle_as_safe_continuation();
            return;
        }

        // If no dependencies were established, we need to decrement the ref
        // count to undo the increment at the top of this method.
        _successor->decrement_ref_count();
    }

    // Invoke the next stage if we haven't returned yet.
    _InvokeOne(i + 1, std::forward<Tail>(tail)...);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif