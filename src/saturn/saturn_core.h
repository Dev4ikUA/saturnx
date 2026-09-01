#pragma once

#include "libretro_host.h"

#include <string>

namespace SaturnX
{

    class SaturnCore
    {
    public:
        bool loadCore(const std::wstring& path);
        bool loadGame(const std::string& path);

        void unloadGame();
        void reset();
        void runFrame();

        bool isLoaded() const;

        const VideoFrame& videoFrame() const;

        void setController(
            unsigned port,
            const ControllerState& state);

    private:
        LibretroHost host_;
    };

}