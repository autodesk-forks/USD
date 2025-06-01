//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef PXR_USDWEB_WINDOW_STATE_H
#define PXR_USDWEB_WINDOW_STATE_H

#include <GLFW/glfw3.h>
#include "freeCameraGL.h"
#include "debugCodes.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

static void error_callback(int error, const char *description)
{
    //TF_RUNTIME_ERROR(description); // issues a warning at compile time
}

// GLFW window state data
struct WindowState
{
    WindowState()
            : mouseX(0.0), mouseY(0.0), mouseButton(-1), mouseButtonState(-1)
    { }
    double mouseX, mouseY;
    int mouseButton;
    int mouseButtonState;
    FreeCameraGL *camera;
};

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    WindowState *windowState = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
    windowState->mouseButton = button;
    windowState->mouseButtonState = action;
    windowState->camera->mouseDown(button, action, mods, (int)windowState->mouseX, (int)windowState->mouseY);
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    WindowState *windowState = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
    double xpos,ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if( windowState->mouseButtonState == GLFW_PRESS )
    {
        windowState->camera->mouseMove(static_cast<int>(xpos), static_cast<int>(ypos));
        windowState->camera->update();
    }

    windowState->mouseX = xpos;
    windowState->mouseY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    WindowState *windowState = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
    windowState->camera->mouseWheel(xoffset, yoffset > 0 ? 1 : -1);
    windowState->camera->update();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
