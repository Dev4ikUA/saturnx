#include "win32.h"

namespace SaturnX
{
    HWND Window::handle() const
    {
        return hwnd_;
    }

    LRESULT CALLBACK Window::windowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        switch (message)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcA(
                hwnd,
                message,
                wParam,
                lParam);
        }
    }

    bool Window::create(HINSTANCE instance)
    {
        const char* className = "SaturnXWindow";

        WNDCLASSA wc{};
        wc.lpfnWndProc = windowProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorA(
            nullptr,
            IDC_ARROW);

        if (!RegisterClassA(&wc))
            return false;

        hwnd_ = CreateWindowExA(
            0,
            className,
            "SaturnX",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            960,
            720,
            nullptr,
            nullptr,
            instance,
            nullptr);

        if (!hwnd_)
            return false;

        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);

        return true;
    }

    bool Window::processMessages()
    {
        MSG message{};

        while (PeekMessageA(
            &message,
            nullptr,
            0,
            0,
            PM_REMOVE))
        {
            if (message.message == WM_QUIT)
                return false;

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        return true;
    }
}