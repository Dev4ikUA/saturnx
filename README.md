# SaturnX

A custom Sega Saturn emulator frontend built around the Beetle Saturn Libretro core, with a native Win32 frontend and Vulkan rendering.

SaturnX is an experimental project focused on learning low-level emulator integration, Libretro frontend development, Vulkan rendering, and Sega Saturn architecture.

## Current Status

**Work in progress.**

The current prototype can:

- Load the Beetle Saturn Libretro core as a DLL
- Initialize the Libretro API
- Provide Libretro environment callbacks
- Provide a VFS backend for the core
- Load Sega Saturn CUE/BIN disc images
- Provide a Saturn BIOS
- Start real Saturn games
- Receive video frames from Beetle Saturn
- Present frames through a Vulkan renderer
- Detect Saturn controller input

The current test game is **Sonic R**.

## Architecture


                 SaturnX
                    |
        +-----------+-----------+
        |                       |
   Win32 Frontend          Vulkan Renderer
        |                       |
        +-----------+-----------+
                    |
              Libretro Host
                    |
          Beetle Saturn DLL
                    |
             Sega Saturn
                    |
          CUE / BIN + BIOS

SaturnX does not directly compile the Beetle Saturn emulator sources into the main executable.

Instead, Beetle Saturn is built as a separate Libretro core:

SaturnX.exe
    |
    +-- runtime/
        +-- cores/
            +-- beetle-saturn.dll

The core is loaded dynamically at runtime using the Windows DLL loader.

Project Structure
SaturnX/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
│
├── src/
│   ├── main.cpp
│   │
│   ├── frontend/
│   │   ├── win32.cpp
│   │   └── win32.h
│   │
│   ├── saturn/
│   │   ├── CMakeLists.txt
│   │   ├── libretro_host.cpp
│   │   ├── libretro_host.h
│   │   ├── saturn_core.cpp
│   │   └── saturn_core.h
│   │
│   └── vulkan/
│       ├── vulkan_renderer.cpp
│       └── vulkan_renderer.h
│
├── ext/
│   └── beetle-saturn/
│
└── runtime/
    ├── cores/
    │   └── beetle-saturn.dll
    │
    ├── system/
    │   └── mpr-17933.bin
    │
    └── saves/
Requirements
Build Environment
Windows 10 or newer
Visual Studio 2022
MSVC
CMake 3.25+
Vulkan SDK
A working C/C++ build environment for building the Beetle Saturn Libretro core
Runtime

A Sega Saturn BIOS is required.

For a US/EU Saturn configuration, SaturnX currently expects:

runtime/system/mpr-17933.bin

BIOS files are not included in this repository.

You must obtain BIOS dumps legally from hardware you own.

Building

Configure the project:

cmake --preset windows-debug

Build:

cmake --build --preset windows-debug

For Release:

cmake --preset windows-release
cmake --build --preset windows-release
Beetle Saturn Core

SaturnX uses the Beetle Saturn Libretro core as its Saturn emulation backend.

The core is loaded dynamically:

LoadLibraryW(L"...\\beetle-saturn.dll");

and the Libretro entry points are resolved with:

GetProcAddress();

This keeps the emulator core separate from the SaturnX frontend.

Libretro Integration

SaturnX currently implements a native Libretro host with support for:

Environment callbacks
Core options
Logging
Video callbacks
Audio callbacks
Input polling
Joypad input
Libretro VFS API
System directory
Save directory
Pixel format negotiation

The VFS implementation allows Beetle Saturn to access CUE/BIN files through the SaturnX frontend.

Disc Images

SaturnX currently uses CUE/BIN disc images.

Example:

rom/
├── Sonic R (Prototype - Sep 10) (hidden-palace.org).cue
└── SONIC R (PROTOTYPE - SEP 10) (HIDDEN-PALACE.ORG).BIN

The .cue file must reference the corresponding .bin file correctly.

Vulkan

The video output path is currently:

Beetle Saturn
      |
      v
retro_video_refresh()
      |
      v
SaturnX VideoFrame
      |
      v
Vulkan staging buffer
      |
      v
Vulkan image
      |
      v
Swapchain
      |
      v
Window

The current renderer is intentionally simple and is being developed incrementally.

Input

SaturnX uses the Libretro joypad interface for controller input.

The Saturn controller mapping is currently experimental and will be expanded as the frontend develops.

Goals

The long-term goal is to turn SaturnX into a complete native Sega Saturn emulator frontend with:

Full Vulkan rendering
Proper Saturn controller support
Audio output
Save states
Memory card / backup RAM support
Disc management
Configuration system
Shader support
Frame pacing
Fullscreen and windowed modes
Better Libretro compatibility
Native Saturn debugging tools
A proper frontend UI
Why Saturn?

The Sega Saturn is a fascinating system from a hardware and software engineering perspective.

Its architecture combines multiple CPUs, dedicated video processors, SH-2 processors, a CD subsystem, complex memory access patterns, and unusual rendering hardware.

That makes it a particularly interesting platform for experimenting with emulator architecture and low-level systems programming.

Development Philosophy

SaturnX is being developed as a learning and experimentation project.

The project prioritizes:

Understanding how emulator components communicate
Explicit low-level APIs
Native Windows development
Vulkan
C and C++
Simple, inspectable architecture
Incremental implementation over large abstractions
License

SaturnX source code is licensed under the license specified in this repository.

Third-party components, including Beetle Saturn and Libretro components, retain their respective licenses.

Disclaimer

SaturnX is an independent project and is not affiliated with Sega.

Sega Saturn is a trademark of SEGA CORPORATION.
