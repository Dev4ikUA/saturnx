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

```text
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
