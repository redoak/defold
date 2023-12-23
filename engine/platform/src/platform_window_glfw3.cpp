// Copyright 2020-2023 The Defold Foundation
// Copyright 2014-2020 King
// Copyright 2009-2014 Ragnar Svensson, Christian Murray
// Licensed under the Defold License version 1.0 (the "License"); you may not use
// this file except in compliance with the License.
//
// You may obtain a copy of the License, together with FAQs at
// https://www.defold.com/license
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <assert.h>

#include "platform_window.h"
#include "platform_window_constants.h"

#include <glfw/glfw3.h>

#include <dlib/platform.h>
#include <dlib/log.h>
#include <dlib/math.h>

namespace dmPlatform
{
    struct Window
    {
        GLFWwindow* m_Window;
        WindowResizeCallback          m_ResizeCallback;
        void*                         m_ResizeCallbackUserData;
        WindowCloseCallback           m_CloseCallback;
        void*                         m_CloseCallbackUserData;
        WindowFocusCallback           m_FocusCallback;
        void*                         m_FocusCallbackUserData;
        WindowIconifyCallback         m_IconifyCallback;
        void*                         m_IconifyCallbackUserData;
        WindowAddKeyboardCharCallback m_AddKeyboarCharCallBack;
        void*                         m_AddKeyboarCharCallBackUserData;
        WindowSetMarkedTextCallback   m_SetMarkedTextCallback;
        void*                         m_SetMarkedTextCallbackUserData;
        WindowDeviceChangedCallback   m_DeviceChangedCallback;
        void*                         m_DeviceChangedCallbackUserData;
        WindowGamepadEventCallback    m_GamepadEventCallback;
        void*                         m_GamepadEventCallbackUserData;

        int32_t                       m_Width;
        int32_t                       m_Height;
        /*
        uint32_t                      m_Samples               : 8;
        uint32_t                      m_HighDPI               : 1;
        */

        uint32_t                      m_SwapIntervalSupported : 1;
        uint32_t                      m_WindowOpened          : 1;
    };

    static void glfw_error_callback(int error, const char* description)
    {
        dmLogError("GLFW Error: %s\n", description);
    }

    HWindow NewWindow()
    {
        if (glfwInit() == GL_FALSE)
        {
            dmLogError("Could not initialize glfw.");
            return 0;
        }

        Window* wnd = new Window;
        memset(wnd, 0, sizeof(Window));

        glfwSetErrorCallback(glfw_error_callback);

        return wnd;
    }

    void DeleteWindow(HWindow window)
    {
        delete window;
    }

    PlatformResult OpenWindowOpenGL(Window* wnd, const WindowParams& params)
    {
        // osx
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        wnd->m_Window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);

        if (!wnd->m_Window)
        {
            return PLATFORM_RESULT_WINDOW_OPEN_ERROR;
        }

        wnd->m_SwapIntervalSupported = 1;

        glfwMakeContextCurrent(wnd->m_Window);

        return PLATFORM_RESULT_OK;
    }

    PlatformResult OpenWindowVulkan(Window* wnd, const WindowParams& params)
    {
        return PLATFORM_RESULT_WINDOW_OPEN_ERROR;
    }

    PlatformResult OpenWindow(HWindow window, const WindowParams& params)
    {
        if (window->m_WindowOpened)
        {
            return PLATFORM_RESULT_WINDOW_ALREADY_OPENED;
        }

        PlatformResult res = PLATFORM_RESULT_WINDOW_OPEN_ERROR;

        switch(params.m_GraphicsApi)
        {
            case PLATFORM_GRAPHICS_API_OPENGL:
                res = OpenWindowOpenGL(window, params);
                break;
            case PLATFORM_GRAPHICS_API_VULKAN:
                res = OpenWindowVulkan(window, params);
                break;
            default: assert(0);
        }

        if (res == PLATFORM_RESULT_OK)
        {
            window->m_WindowOpened = 1;

            // typedef void(* GLFWjoystickfun) (int jid, int event)
        }

        return res;
    }

    uintptr_t GetProcAddress(HWindow window, const char* proc_name)
    {
        return (uintptr_t) glfwGetProcAddress(proc_name);
    }

    void CloseWindow(HWindow window)
    {
        glfwDestroyWindow(window->m_Window);
    }

    void PollEvents(HWindow window)
    {
        glfwPollEvents();
    }

    void SwapBuffers(HWindow window)
    {
        glfwSwapBuffers(window->m_Window);
    }

    void IconifyWindow(HWindow window)
    {
        glfwIconifyWindow(window->m_Window);
    }

    uint32_t GetWindowWidth(HWindow window)
    {
        return (uint32_t) window->m_Width;
    }
    uint32_t GetWindowHeight(HWindow window)
    {
        return (uint32_t) window->m_Height;
    }

    void SetSwapInterval(HWindow window, uint32_t swap_interval)
    {
        if (window->m_SwapIntervalSupported)
        {
            glfwSwapInterval(swap_interval);
        }
    }

    void SetWindowSize(HWindow window, uint32_t width, uint32_t height)
    {
        glfwSetWindowSize(window->m_Window, (int) width, (int) height);
        int window_width, window_height;
        glfwGetWindowSize(window->m_Window, &window_width, &window_height);
        window->m_Width  = window_width;
        window->m_Height = window_height;

        // The callback is not called from glfw when the size is set manually
        if (window->m_ResizeCallback)
        {
            window->m_ResizeCallback(window->m_ResizeCallbackUserData, window_width, window_height);
        }
    }

    float GetDisplayScaleFactor(HWindow window)
    {
        return 1.0f;
    }

    void* AcquireAuxContext(HWindow window)
    {
        return 0;
    }

    void UnacquireAuxContext(HWindow window, void* aux_context)
    {
    }

    uint32_t GetWindowStateParam(HWindow window, WindowState state)
    {
        switch(state)
        {
            case WINDOW_STATE_OPENED: return window->m_WindowOpened;
        }

        return 0;
        /*
        switch(state)
        {
            case WINDOW_STATE_REFRESH_RATE: return glfwGetWindowRefreshRate();
            case WINDOW_STATE_SAMPLE_COUNT: return window->m_Samples;
            case WINDOW_STATE_HIGH_DPI:     return window->m_HighDPI;
            case WINDOW_STATE_AUX_CONTEXT:  return glfwQueryAuxContext();
            default:break;
        }

        return window->m_WindowOpened ? glfwGetWindowParam(WindowStateToGLFW(state)) : 0;
        */
    }

    uint32_t GetTouchData(HWindow window, TouchData* touch_data, uint32_t touch_data_count)
    {
        return 0;
    }

    bool GetAcceleration(HWindow window, float* x, float* y, float* z)
    {
        return false;
    }

    int32_t GetKey(HWindow window, int32_t code)
    {
         return glfwGetKey(window->m_Window, code);
    }

    int32_t GetMouseWheel(HWindow window)
    {
        return 0;
        //glfwSetScrollCallback
        // return glfwGetMouseWheel();
    }

    int32_t GetMouseButton(HWindow window, int32_t button)
    {
        return glfwGetMouseButton(window->m_Window, button);
    }

    void GetMousePosition(HWindow window, int32_t* x, int32_t* y)
    {
        double xpos, ypos;
        glfwGetCursorPos(window->m_Window, &xpos, &ypos);
        *x = (int32_t) xpos;
        *y = (int32_t) ypos;
    }

    bool GetDeviceState(HWindow window, DeviceState state, int32_t op1)
    {
        switch(state)
        {
            case DEVICE_STATE_CURSOR_LOCK:      return glfwGetInputMode(window->m_Window, GLFW_CURSOR);
            case DEVICE_STATE_JOYSTICK_PRESENT: return glfwJoystickPresent(op1);
            default:break;
        }
        assert(0 && "Not supported.");
        return false;
    }

    const char* GetJoystickDeviceName(HWindow window, uint32_t gamepad_index)
    {
        return glfwGetJoystickName((int) gamepad_index);
    }

    uint32_t GetJoystickAxes(HWindow window, uint32_t joystick_index, float* values, uint32_t values_capacity)
    {
        int32_t count = 0;
        const float* axes_values = glfwGetJoystickAxes(joystick_index, &count);
        count = dmMath::Min(count, (int32_t) values_capacity);
        if (count > 0)
        {
            memcpy(values, axes_values, sizeof(float) * count);
        }
        return count;
    }

    uint32_t GetJoystickHats(HWindow window, uint32_t joystick_index, uint8_t* values, uint32_t values_capacity)
    {
        int32_t count = 0;
        const unsigned char* hats_values = glfwGetJoystickHats(joystick_index, &count);
        count = dmMath::Min(count, (int32_t) values_capacity);
        if (count > 0)
        {
            memcpy(values, hats_values, sizeof(float) * count);
        }
        return count;
    }

    uint32_t GetJoystickButtons(HWindow window, uint32_t joystick_index, uint8_t* values, uint32_t values_capacity)
    {
        int32_t count = 0;
        const unsigned char* buttons_values = glfwGetJoystickButtons(joystick_index, &count);
        count = dmMath::Min(count, (int32_t) values_capacity);
        if (count > 0)
        {
            memcpy(values, buttons_values, sizeof(float) * count);
        }
        return count;
    }

    bool GetDeviceState(HWindow window, DeviceState state)
    {
        return GetDeviceState(window, state, 0);
    }

    void SetDeviceState(HWindow window, DeviceState state, bool op1)
    {
        SetDeviceState(window, state, op1, false);
    }

    void SetDeviceState(HWindow window, DeviceState state, bool op1, bool op2)
    {
        switch(state)
        {
            case DEVICE_STATE_CURSOR:
                glfwSetInputMode(window->m_Window, GLFW_CURSOR, op1 ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
                break;
        }

        /*
        switch(state)
        {
            case DEVICE_STATE_CURSOR:
                if (op1)
                    glfwEnable(GLFW_MOUSE_CURSOR);
                else
                    glfwDisable(GLFW_MOUSE_CURSOR);
                break;
            case DEVICE_STATE_ACCELEROMETER:
                if (op1)
                    glfwAccelerometerEnable();
                break;
            case DEVICE_STATE_KEYBOARD_DEFAULT:
                glfwShowKeyboard(op1, GLFW_KEYBOARD_DEFAULT, op2);
                break;
            case DEVICE_STATE_KEYBOARD_NUMBER_PAD:
                glfwShowKeyboard(op1, GLFW_KEYBOARD_NUMBER_PAD, op2);
                break;
            case DEVICE_STATE_KEYBOARD_EMAIL:
                glfwShowKeyboard(op1, GLFW_KEYBOARD_EMAIL, op2);
                break;
            case DEVICE_STATE_KEYBOARD_PASSWORD:
                glfwShowKeyboard(op1, GLFW_KEYBOARD_PASSWORD, op2);
                break;
            default:break;
        }
        */
    }

    void SetKeyboardCharCallback(HWindow window, WindowAddKeyboardCharCallback cb, void* user_data)
    {
        window->m_AddKeyboarCharCallBack         = cb;
        window->m_AddKeyboarCharCallBackUserData = user_data;
    }

    void SetKeyboardMarkedTextCallback(HWindow window, WindowSetMarkedTextCallback cb, void* user_data)
    {
        window->m_SetMarkedTextCallback         = cb;
        window->m_SetMarkedTextCallbackUserData = user_data;
    }

    void SetKeyboardDeviceChangedCallback(HWindow window, WindowDeviceChangedCallback cb, void* user_data)
    {
        window->m_DeviceChangedCallback         = cb;
        window->m_DeviceChangedCallbackUserData = user_data;
    }

    void SetGamepadEventCallback(HWindow window, WindowGamepadEventCallback cb, void* user_data)
    {
        window->m_GamepadEventCallback         = cb;
        window->m_GamepadEventCallbackUserData = user_data;
    }

    const int PLATFORM_JOYSTICK_LAST       = GLFW_JOYSTICK_LAST;
    const int PLATFORM_KEY_ESC             = GLFW_KEY_ESCAPE;
    const int PLATFORM_KEY_F1              = GLFW_KEY_F1;
    const int PLATFORM_KEY_F2              = GLFW_KEY_F2;
    const int PLATFORM_KEY_F3              = GLFW_KEY_F3;
    const int PLATFORM_KEY_F4              = GLFW_KEY_F4;
    const int PLATFORM_KEY_F5              = GLFW_KEY_F5;
    const int PLATFORM_KEY_F6              = GLFW_KEY_F6;
    const int PLATFORM_KEY_F7              = GLFW_KEY_F7;
    const int PLATFORM_KEY_F8              = GLFW_KEY_F8;
    const int PLATFORM_KEY_F9              = GLFW_KEY_F9;
    const int PLATFORM_KEY_F10             = GLFW_KEY_F10;
    const int PLATFORM_KEY_F11             = GLFW_KEY_F11;
    const int PLATFORM_KEY_F12             = GLFW_KEY_F12;
    const int PLATFORM_KEY_UP              = GLFW_KEY_UP;
    const int PLATFORM_KEY_DOWN            = GLFW_KEY_DOWN;
    const int PLATFORM_KEY_LEFT            = GLFW_KEY_LEFT;
    const int PLATFORM_KEY_RIGHT           = GLFW_KEY_RIGHT;
    const int PLATFORM_KEY_LSHIFT          = GLFW_KEY_LEFT_SHIFT;
    const int PLATFORM_KEY_RSHIFT          = GLFW_KEY_RIGHT_SHIFT;
    const int PLATFORM_KEY_LCTRL           = GLFW_KEY_LEFT_CONTROL;
    const int PLATFORM_KEY_RCTRL           = GLFW_KEY_RIGHT_CONTROL;
    const int PLATFORM_KEY_LALT            = GLFW_KEY_LEFT_ALT;
    const int PLATFORM_KEY_RALT            = GLFW_KEY_RIGHT_ALT;
    const int PLATFORM_KEY_TAB             = GLFW_KEY_TAB;
    const int PLATFORM_KEY_ENTER           = GLFW_KEY_ENTER;
    const int PLATFORM_KEY_BACKSPACE       = GLFW_KEY_BACKSPACE;
    const int PLATFORM_KEY_INSERT          = GLFW_KEY_INSERT;
    const int PLATFORM_KEY_DEL             = GLFW_KEY_DELETE;
    const int PLATFORM_KEY_PAGEUP          = GLFW_KEY_PAGE_UP;
    const int PLATFORM_KEY_PAGEDOWN        = GLFW_KEY_PAGE_DOWN;
    const int PLATFORM_KEY_HOME            = GLFW_KEY_HOME;
    const int PLATFORM_KEY_END             = GLFW_KEY_END;
    const int PLATFORM_KEY_KP_0            = GLFW_KEY_KP_0;
    const int PLATFORM_KEY_KP_1            = GLFW_KEY_KP_1;
    const int PLATFORM_KEY_KP_2            = GLFW_KEY_KP_2;
    const int PLATFORM_KEY_KP_3            = GLFW_KEY_KP_3;
    const int PLATFORM_KEY_KP_4            = GLFW_KEY_KP_4;
    const int PLATFORM_KEY_KP_5            = GLFW_KEY_KP_5;
    const int PLATFORM_KEY_KP_6            = GLFW_KEY_KP_6;
    const int PLATFORM_KEY_KP_7            = GLFW_KEY_KP_7;
    const int PLATFORM_KEY_KP_8            = GLFW_KEY_KP_8;
    const int PLATFORM_KEY_KP_9            = GLFW_KEY_KP_9;
    const int PLATFORM_KEY_KP_DIVIDE       = GLFW_KEY_KP_DIVIDE;
    const int PLATFORM_KEY_KP_MULTIPLY     = GLFW_KEY_KP_MULTIPLY;
    const int PLATFORM_KEY_KP_SUBTRACT     = GLFW_KEY_KP_SUBTRACT;
    const int PLATFORM_KEY_KP_ADD          = GLFW_KEY_KP_ADD;
    const int PLATFORM_KEY_KP_DECIMAL      = GLFW_KEY_KP_DECIMAL;
    const int PLATFORM_KEY_KP_EQUAL        = GLFW_KEY_KP_EQUAL;
    const int PLATFORM_KEY_KP_ENTER        = GLFW_KEY_KP_ENTER;
    const int PLATFORM_KEY_KP_NUM_LOCK     = GLFW_KEY_NUM_LOCK;
    const int PLATFORM_KEY_CAPS_LOCK       = GLFW_KEY_CAPS_LOCK;
    const int PLATFORM_KEY_SCROLL_LOCK     = GLFW_KEY_SCROLL_LOCK;
    const int PLATFORM_KEY_PAUSE           = GLFW_KEY_PAUSE;
    const int PLATFORM_KEY_LSUPER          = GLFW_KEY_LEFT_SUPER;
    const int PLATFORM_KEY_RSUPER          = GLFW_KEY_RIGHT_SUPER;
    const int PLATFORM_KEY_MENU            = GLFW_KEY_MENU;
    const int PLATFORM_KEY_BACK            = -1; // What is this used for?
    const int PLATFORM_MOUSE_BUTTON_LEFT   = GLFW_MOUSE_BUTTON_LEFT;
    const int PLATFORM_MOUSE_BUTTON_MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE;
    const int PLATFORM_MOUSE_BUTTON_RIGHT  = GLFW_MOUSE_BUTTON_RIGHT;
    const int PLATFORM_MOUSE_BUTTON_1      = GLFW_MOUSE_BUTTON_1;
    const int PLATFORM_MOUSE_BUTTON_2      = GLFW_MOUSE_BUTTON_2;
    const int PLATFORM_MOUSE_BUTTON_3      = GLFW_MOUSE_BUTTON_3;
    const int PLATFORM_MOUSE_BUTTON_4      = GLFW_MOUSE_BUTTON_4;
    const int PLATFORM_MOUSE_BUTTON_5      = GLFW_MOUSE_BUTTON_5;
    const int PLATFORM_MOUSE_BUTTON_6      = GLFW_MOUSE_BUTTON_6;
    const int PLATFORM_MOUSE_BUTTON_7      = GLFW_MOUSE_BUTTON_7;
    const int PLATFORM_MOUSE_BUTTON_8      = GLFW_MOUSE_BUTTON_8;

    const int PLATFORM_JOYSTICK_1          = GLFW_JOYSTICK_1;
    const int PLATFORM_JOYSTICK_2          = GLFW_JOYSTICK_2;
    const int PLATFORM_JOYSTICK_3          = GLFW_JOYSTICK_3;
    const int PLATFORM_JOYSTICK_4          = GLFW_JOYSTICK_4;
    const int PLATFORM_JOYSTICK_5          = GLFW_JOYSTICK_5;
    const int PLATFORM_JOYSTICK_6          = GLFW_JOYSTICK_6;
    const int PLATFORM_JOYSTICK_7          = GLFW_JOYSTICK_7;
    const int PLATFORM_JOYSTICK_8          = GLFW_JOYSTICK_8;
    const int PLATFORM_JOYSTICK_9          = GLFW_JOYSTICK_9;
    const int PLATFORM_JOYSTICK_10         = GLFW_JOYSTICK_10;
    const int PLATFORM_JOYSTICK_11         = GLFW_JOYSTICK_11;
    const int PLATFORM_JOYSTICK_12         = GLFW_JOYSTICK_12;
    const int PLATFORM_JOYSTICK_13         = GLFW_JOYSTICK_13;
    const int PLATFORM_JOYSTICK_14         = GLFW_JOYSTICK_14;
    const int PLATFORM_JOYSTICK_15         = GLFW_JOYSTICK_15;
    const int PLATFORM_JOYSTICK_16         = GLFW_JOYSTICK_16;
}
