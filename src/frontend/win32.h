#pragma once

#include <Windows.h>

namespace SaturnX
{
    class Window
    {
    public:
        bool create(HINSTANCE instance);
        bool processMessages();
        HWND handle() const;

    private:
        static LRESULT CALLBACK windowProc(
            HWND hwnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam);

        HWND hwnd_ = nullptr;
    };
}