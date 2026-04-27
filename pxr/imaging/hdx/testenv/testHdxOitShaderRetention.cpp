//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Regression test for shader recompilation after garbage collection in OIT.

#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdx/unitTestDelegate.h"

#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/vt/value.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char* argv[])
{
    TfErrorMark mark;

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    GarchGLDebugWindow window("Hdx OIT Shader Retention Test", 256, 256);
    window.Init();

    HgiUniquePtr hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};

    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> index(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    TF_VERIFY(index != nullptr);

    std::unique_ptr<Hdx_UnitTestDelegate> delegate(
        new Hdx_UnitTestDelegate(index.get()));
    HdEngine engine;

    const SdfPath oitRenderTaskId("/oitRenderTask");
    const SdfPath oitResolveTaskId("/oitResolveTask");
    delegate->AddOitRenderTask(oitRenderTaskId);
    delegate->AddOitResolveTask(oitResolveTaskId);
    delegate->AddGrid(SdfPath("/grid"), GfMatrix4d(1));

    HdTaskSharedPtrVector tasks = {
        index->GetTask(oitRenderTaskId),
        index->GetTask(oitResolveTaskId),
    };

    // Render once to compile all OIT GLSL programs.
    engine.Execute(index.get(), &tasks);

    // Trigger GC. With the fix each pass holds its own render pass state, so
    // both compiled programs stay referenced and survive. Without the fix the
    // shared state swaps shaders, dropping the opaque program's refcount to 1
    // and letting GC free it.
    auto resourceRegistry = std::dynamic_pointer_cast<HdStResourceRegistry>(
        renderDelegate.GetResourceRegistry());
    TF_VERIFY(resourceRegistry);
    resourceRegistry->GarbageCollect();

    // GC resets instGlslProgram to the surviving count. Zero it so any
    // increment on the next render means a freed program was recompiled.
    perfLog.SetCounter(HdPerfTokens->instGlslProgram, 0.0);

    engine.Execute(index.get(), &tasks);

    const double compilesAfterGC =
        perfLog.GetCounter(HdPerfTokens->instGlslProgram);
    TF_VERIFY(compilesAfterGC == 0.0,
        "OIT shaders were freed by GC: %g GLSL program(s) recompiled",
        compilesAfterGC);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
