//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_PLUGIN_RENDERER_UNIQUE_HANDLE_H
#define PXR_IMAGING_HD_PLUGIN_RENDERER_UNIQUE_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/rendererPluginHandle.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderer;

///
/// A handle for a renderer that was created by a renderer plugin.
/// Its semantics are similar to std::unique_ptr.
///
/// The handle owns the renderer and ensures that the plugin is kept alive
/// until the renderer is destroyed. The handle can also be queried for the
/// id of the plugin used to create the renderer.
///
class HdPluginRendererUniqueHandle final
{
public:
    HD_API
    HdPluginRendererUniqueHandle();

    HD_API
    HdPluginRendererUniqueHandle(HdPluginRendererUniqueHandle &&);

    HD_API
    HdPluginRendererUniqueHandle(const std::nullptr_t &);

    HD_API
    ~HdPluginRendererUniqueHandle();

    HD_API
    HdPluginRendererUniqueHandle &operator=(HdPluginRendererUniqueHandle &&);

    HD_API
    HdPluginRendererUniqueHandle &operator=(const std::nullptr_t &);

    HdRenderer *Get() const { return _renderer.get(); }

    HdRenderer *operator->() const { return _renderer.get(); }
    HdRenderer &operator*() const { return *_renderer; }

    /// Is the wrapped HdRenderer valid?
    explicit operator bool() const { return bool(_renderer); }

    /// Id of the plugin used to create the renderer.
    HD_API
    TfToken GetPluginId() const;

private:
    friend class HdRendererPlugin;

    HdPluginRendererUniqueHandle(
        HdRendererPluginHandle plugin,
        std::unique_ptr<HdRenderer> renderer);

    HdRendererPluginHandle _plugin;
    std::unique_ptr<HdRenderer> _renderer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
