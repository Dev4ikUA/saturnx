# SaturnX

*A Native Sega Saturn Emulator Frontend with Vulkan Rendering*

## Overview

**SaturnX** is an experimental Sega Saturn emulator frontend built around the Beetle Saturn Libretro core.

The project combines a native Win32 application frontend, a custom Libretro host implementation, and a Vulkan-based rendering backend to provide an independent environment for running Sega Saturn software.

The repository contains the frontend source code, Libretro integration layer, Vulkan rendering implementation, and build configuration.

SaturnX is an independent software development project focused on emulator frontend architecture, low-level graphics programming, hardware emulation integration, and the Sega Saturn platform.

## Key Features

- **Native Win32 Frontend:** Windows-native application interface built using the Win32 API.
- **Libretro Core Integration:** Dynamic loading of the Beetle Saturn core as an external DLL.
- **Custom Libretro Host:** Native implementation of Libretro callbacks, environment interfaces, and core communication.
- **Vulkan Rendering:** Hardware-accelerated video presentation through a Vulkan rendering pipeline.
- **Disc Image Support:** Loading Sega Saturn games through CUE/BIN disc image formats.
- **BIOS Integration:** External Sega Saturn BIOS configuration and system directory management.
- **Virtual File System:** Custom Libretro VFS backend for disc image and file access.
- **Controller Input:** Experimental Saturn controller input handling through the Libretro Joypad interface.

## Technical Stack

| Component | Technology |
|---|---|
| Primary language | C++ |
| Graphics API | Vulkan |
| Application interface | Win32 API |
| Emulation backend | Beetle Saturn |
| Core interface | Libretro API |
| Dynamic loading | Windows DLL API |
| Build system | CMake |
| Compiler | MSVC |
| Target platform | Microsoft Windows |

## Platform Compatibility

SaturnX is currently developed and tested on Microsoft Windows.

The application uses native Windows APIs and Vulkan for graphics presentation.

Linux and macOS support are not currently provided.

## Prerequisites

A Windows-compatible C/C++ development environment is required.

Recommended tools:

- Microsoft Visual Studio 2022.
- MSVC C++ compiler.
- CMake 3.25 or newer.
- Vulkan SDK.
- Beetle Saturn Libretro core build environment.

A compatible Vulkan-capable graphics device and driver are required for rendering.

## Runtime Requirements

SaturnX requires an external Beetle Saturn Libretro core and a compatible Sega Saturn BIOS.

Expected runtime directory structure:

```text
runtime/
├── cores/
│   └── beetle-saturn.dll
│
├── system/
│   └── mpr-17933.bin
│
└── saves/
```

BIOS files are not distributed with SaturnX. Users must provide their own legally obtained BIOS dumps.

## Building

Configure the project using the provided CMake presets.

**Debug configuration:**

```bash
cmake --preset windows-debug
cmake --build --preset windows-debug
```

**Release configuration:**

```bash
cmake --preset windows-release
cmake --build --preset windows-release
```

## Architecture

SaturnX separates the application frontend, emulation host, and graphics backend into independent components.

The Beetle Saturn core is compiled separately and loaded dynamically at runtime through the Windows DLL loading mechanism.

This architecture allows the frontend and emulation backend to remain independent while communicating through the standardized Libretro API.

### Video Pipeline

```text
Beetle Saturn Core
        |
        v
Libretro Video Callback
        |
        v
SaturnX Frame Interface
        |
        v
Vulkan Rendering Backend
        |
        v
Vulkan Swapchain
        |
        v
Win32 Application Window
```

## Current Development Status

SaturnX is in an experimental development stage.

The current implementation focuses on establishing communication between the Libretro core and the native frontend, loading Sega Saturn disc images, and presenting emulated video output through Vulkan.

**Sonic R** is currently used as a test title during development.

Audio output, complete controller support, save states, memory card emulation, and additional frontend functionality remain under development.

## License

SaturnX source code is distributed under the license specified in this repository.

Third-party components, including Beetle Saturn and Libretro, retain their respective licenses.

## Disclaimer

SaturnX is an independent project and is not affiliated with, endorsed by, or associated with SEGA.

Sega Saturn is a trademark of SEGA CORPORATION.
