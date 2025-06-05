//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_EMSCRIPTEN_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_EMSCRIPTEN_H

#include "pxr/pxr.h"
#include <EGL/egl.h>

PXR_NAMESPACE_OPEN_SCOPE

class GarchEmscriptenContextState {
public:
    /// Construct with the current state.
    GarchEmscriptenContextState();

    /// Construct with the given state.
    GarchEmscriptenContextState(EGLDisplay, EGLSurface, EGLContext);

    /// Compare for equality.
    bool operator==(const GarchEmscriptenContextState& rhs) const;

    /// Returns a hash value for the state.
    size_t GetHash() const;

    /// Returns \c true if the context state is valid.
    bool IsValid() const;

    /// Make the context current.
    void MakeCurrent();

    /// Make no context current.
    static void DoneCurrent();

public:
    EGLDisplay display;
    EGLSurface draw;
    EGLContext context;

private:
    bool _defaultCtor;
};

// Hide the platform specific type name behind a common name.
typedef GarchEmscriptenContextState GarchGLPlatformContextState;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_EMSCRIPTEN_H
