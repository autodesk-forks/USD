//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILATION_STATE_H
#define PXR_EXEC_EXEC_COMPILATION_STATE_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/compilerTaskSync.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfStage;
class Exec_CompilationTask;
class Exec_Program;

/// Data shared between all compilation tasks.
/// 
/// We construct an instance of this class at the beginning of a round of
/// compilation and then pass along a reference to this instance to all
/// compilation tasks. This prevents bloating the size of every task with this
/// commonly used data.
/// 
class Exec_CompilationState
{
public:
    Exec_CompilationState(
        const EsfStage &stage,
        Exec_Program *program) :
        _stage(stage),
        _program(program)
    {
        TF_VERIFY(_program);
    }

    /// The scene adapter stage.
    const EsfStage &GetStage() const {
        return _stage;
    }

    /// The program being compiled.
    Exec_Program *GetProgram() {
        return _program;
    }

    class OutputTasksAccess {
        friend class Exec_CompilationTask;

        static Exec_CompilerTaskSync &_Get(Exec_CompilationState *state) {
            return state->_outputTasks;
        }
    };

private:

    const EsfStage &_stage;
    Exec_CompilerTaskSync _outputTasks;
    Exec_Program *_program;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
