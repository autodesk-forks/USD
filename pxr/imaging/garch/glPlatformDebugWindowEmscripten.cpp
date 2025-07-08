//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/garch/glPlatformDebugWindowEmscripten.h"
#include "pxr/imaging/garch/glPlatformDebugContext.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

Garch_GLPlatformDebugWindow::Garch_GLPlatformDebugWindow(GarchGLDebugWindow *w)
{
}

void
Garch_GLPlatformDebugWindow::Init(const char *title,
                                  int width, int height, int nSamples)
{
}

void
Garch_GLPlatformDebugWindow::Run()
{
}

void
Garch_GLPlatformDebugWindow::ExitApp()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

