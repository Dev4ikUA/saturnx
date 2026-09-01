#include "saturn_core.h"

namespace SaturnX
{

    bool SaturnCore::loadCore(
        const std::wstring& path)
    {
        return host_.load(path.c_str());
    }

    bool SaturnCore::loadGame(
        const std::string& path)
    {
        return host_.loadGame(path.c_str());
    }

    void SaturnCore::unloadGame()
    {
        host_.unloadGame();
    }

    void SaturnCore::reset()
    {
        host_.reset();
    }

    void SaturnCore::runFrame()
    {
        host_.run();
    }

    bool SaturnCore::isLoaded() const
    {
        return host_.isLoaded();
    }

    const VideoFrame& SaturnCore::videoFrame() const
    {
        return host_.videoFrame();
    }

    void SaturnCore::setController(
        unsigned port,
        const ControllerState& state)
    {
        host_.setController(port, state);
    }

}