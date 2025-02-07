//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiPresent2/testenv/testWindow.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

PXR_NAMESPACE_OPEN_SCOPE

class HgiPresent2TestWindowX11 final : public HgiPresent2TestWindow
{
public:
    HgiPresent2TestWindowX11(const GfVec2i &size)
    {
        _display = XOpenDisplay(nullptr);
        if (!_display) {
            TF_RUNTIME_ERROR("Failed to open X11 display");
            return;
        }

        _window = XCreateSimpleWindow(_display, XDefaultRootWindow(_display), 0,
            0, size[0], size[1], 0, 0, 0);
        if (!_window) {
            TF_RUNTIME_ERROR("Failed to create X11 window");
            return;
        }

        XStoreName(_display, _window, "testHgiPresent2 X11");

        _deleteWindowMessage = XInternAtom(_display, "WM_DELETE_WINDOW", false);
        XSetWMProtocols(_display, _window, &_deleteWindowMessage, 1);

        XMapWindow(_display, _window);
        XFlush(_display);
    }

    ~HgiPresent2TestWindowX11() override
    {
        if (_display) {
            if (_window) {
                XDestroyWindow(_display, _window);
            }
            XCloseDisplay(_display);
        }
    }

    HgiPresent2TestWindowHandle GetHandle() const override
    {
        if (!_display || !_window) {
            return {};
        }

        return HgiPresent2TestXlibWindowHandle{_display, _window};
    }

    GfVec2i GetSize() const override
    {
        XWindowAttributes attributes{};
        if (XGetWindowAttributes(_display, _window, &attributes)) {
            return {attributes.width, attributes.height};
        }

        return {};
    }

    void SetSize(const GfVec2i &size) override
    {
        XResizeWindow(_display, _window, size[0], size[1]);
    }

    bool Update() override
    {
        if (!_display || !_window) {
            return false;
        }

        while (XPending(_display)) {
            XEvent event;
            XNextEvent(_display, &event);
            if (event.type == ClientMessage &&
                static_cast<Atom>(event.xclient.data.l[0]) ==
                    _deleteWindowMessage) {
                return false;
            }
        }

        return true;
    }

private:
    Display *_display{};
    Window _window{};
    Atom _deleteWindowMessage;
};

std::unique_ptr<HgiPresent2TestWindow>
HgiPresent2TestCreateX11Window(const GfVec2i &size)
{
    return std::make_unique<HgiPresent2TestWindowX11>(size);
}

PXR_NAMESPACE_CLOSE_SCOPE
