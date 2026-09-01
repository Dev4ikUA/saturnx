#include <Windows.h>

#include "frontend/win32.h"
#include "saturn/saturn_core.h"
#include "vulkan/vulkan_renderer.h"

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int)
{
    SaturnX::Window window;

    if (!window.create(instance))
        return 1;

    SaturnX::VulkanRenderer vulkan;

    if (!vulkan.initialize(window.handle()))
        return 1;

    SaturnX::SaturnCore saturn;

    const wchar_t* corePath =
        L"runtime\\cores\\beetle-saturn.dll";

    if (!saturn.loadCore(
        L"C:\\Users\\Sunflowerr\\source\\repos\\SaturnX\\runtime\\cores\\beetle-saturn.dll"))
    {
        MessageBoxA(nullptr,
            "Cannot load Beetle Saturn.",
            "SaturnX",
            MB_OK | MB_ICONERROR);

        return 1;
    }

    // Поки тестовий шлях:
    // поклади сюди свій CUE/ISO.
    const std::string gamePath =
        "C:\\Users\\Sunflowerr\\source\\repos\\SaturnX\\rom\\Sonic R (Prototype - Sep 10) (hidden-palace.org).cue";

    if (!saturn.loadGame(gamePath))
    {
        MessageBoxA(
            nullptr,
            "Cannot load Saturn game.",
            "SaturnX",
            MB_OK | MB_ICONERROR);

        return 1;
    }

    while (window.processMessages())
    {
        saturn.runFrame();

        const SaturnX::VideoFrame& frame =
            saturn.videoFrame();

        if (frame.valid)
        {
            vulkan.uploadFrame(
                frame.pixels.data(),
                frame.width,
                frame.height,
                frame.pitch);
        }

        vulkan.drawFrame();
    }

    vulkan.shutdown();

    return 0;
}