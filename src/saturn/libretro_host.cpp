#include "libretro_host.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace SaturnX
{

    LibretroHost* LibretroHost::activeHost_ = nullptr;

    namespace
    {

        struct VfsFile
        {
            FILE* file = nullptr;
            std::string path;
        };

        static VfsFile* toVfsFile(
            retro_vfs_file_handle* handle)
        {
            return reinterpret_cast<VfsFile*>(handle);
        }

        static const VfsFile* toVfsFile(
            const retro_vfs_file_handle* handle)
        {
            return reinterpret_cast<const VfsFile*>(handle);
        }

        static std::string parentDirectory(
            const std::string& path)
        {
            const size_t slash =
                path.find_last_of("\\/");

            if (slash == std::string::npos)
                return ".";

            if (slash == 0)
                return path.substr(0, 1);

            return path.substr(0, slash);
        }

        static bool hasRead(unsigned mode)
        {
            return (mode & RETRO_VFS_FILE_ACCESS_READ) != 0;
        }

        static bool hasWrite(unsigned mode)
        {
            return (mode & RETRO_VFS_FILE_ACCESS_WRITE) != 0;
        }

        static bool hasUpdateExisting(unsigned mode)
        {
            return (mode &
                RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING) != 0;
        }

    } // namespace


    /*
     * IMPORTANT:
     *
     * The libretro.h used by this project exposes VFS API v1.
     * Do not add a "truncate" member here unless your local
     * libretro.h actually defines VFS API v2.
     */
    retro_vfs_interface LibretroHost::vfsInterface_ =
    {
        &LibretroHost::vfsGetPath,
        &LibretroHost::vfsOpen,
        &LibretroHost::vfsClose,
        &LibretroHost::vfsSize,
        &LibretroHost::vfsTell,
        &LibretroHost::vfsSeek,
        &LibretroHost::vfsRead,
        &LibretroHost::vfsWrite,
        &LibretroHost::vfsFlush,
        &LibretroHost::vfsRemove,
        &LibretroHost::vfsRename
    };


    LibretroHost::LibretroHost() = default;


    LibretroHost::~LibretroHost()
    {
        unload();
    }


    bool LibretroHost::load(
        const wchar_t* path)
    {
        if (!path || !*path)
            return false;

        unload();

        module_ = LoadLibraryW(path);

        if (!module_)
        {
            const DWORD error = GetLastError();

            char buffer[256]{};

            sprintf_s(
                buffer,
                "SaturnX: LoadLibraryW failed, error=%lu\n",
                static_cast<unsigned long>(error));

            OutputDebugStringA(buffer);

            return false;
        }

        activeHost_ = this;

        if (!resolveSymbols())
        {
            OutputDebugStringA(
                "SaturnX: Failed to resolve Libretro symbols.\n");

            unload();
            return false;
        }

        configureDirectories();

        /*
         * Register every callback before retro_init().
         */
        api_.setEnvironment(
            &LibretroHost::environmentCallback);

        api_.setVideoRefresh(
            &LibretroHost::videoCallback);

        api_.setAudioSample(
            &LibretroHost::audioCallback);

        api_.setAudioSampleBatch(
            &LibretroHost::audioBatchCallback);

        api_.setInputPoll(
            &LibretroHost::inputPollCallback);

        api_.setInputState(
            &LibretroHost::inputStateCallback);

        if (api_.setControllerPortDevice)
        {
            api_.setControllerPortDevice(
                0,
                RETRO_DEVICE_JOYPAD);

            api_.setControllerPortDevice(
                1,
                RETRO_DEVICE_JOYPAD);
        }

        /*
         * Ask the core for its reported API version.
         */
        {
            char buffer[128]{};

            sprintf_s(
                buffer,
                "SaturnX: Beetle Libretro API version=%u\n",
                api_.apiVersion());

            OutputDebugStringA(buffer);
        }

        api_.init();

        initialized_ = true;
        gameLoaded_ = false;

        return true;
    }


    bool LibretroHost::resolveSymbols()
    {
        api_.apiVersion =
            symbol<FunctionTable::ApiVersionFn>(
                "retro_api_version");

        api_.init =
            symbol<FunctionTable::InitFn>(
                "retro_init");

        api_.deinit =
            symbol<FunctionTable::DeinitFn>(
                "retro_deinit");

        api_.reset =
            symbol<FunctionTable::ResetFn>(
                "retro_reset");

        api_.run =
            symbol<FunctionTable::RunFn>(
                "retro_run");

        api_.loadGame =
            symbol<FunctionTable::LoadGameFn>(
                "retro_load_game");

        api_.unloadGame =
            symbol<FunctionTable::UnloadGameFn>(
                "retro_unload_game");

        api_.getSystemInfo =
            symbol<FunctionTable::GetSystemInfoFn>(
                "retro_get_system_info");

        api_.getSystemAvInfo =
            symbol<FunctionTable::GetSystemAvInfoFn>(
                "retro_get_system_av_info");

        api_.setEnvironment =
            symbol<FunctionTable::SetEnvironmentFn>(
                "retro_set_environment");

        api_.setVideoRefresh =
            symbol<FunctionTable::SetVideoRefreshFn>(
                "retro_set_video_refresh");

        api_.setAudioSample =
            symbol<FunctionTable::SetAudioSampleFn>(
                "retro_set_audio_sample");

        api_.setAudioSampleBatch =
            symbol<FunctionTable::SetAudioSampleBatchFn>(
                "retro_set_audio_sample_batch");

        api_.setInputPoll =
            symbol<FunctionTable::SetInputPollFn>(
                "retro_set_input_poll");

        api_.setInputState =
            symbol<FunctionTable::SetInputStateFn>(
                "retro_set_input_state");

        api_.setControllerPortDevice =
            symbol<FunctionTable::SetControllerPortDeviceFn>(
                "retro_set_controller_port_device");

        return
            api_.apiVersion &&
            api_.init &&
            api_.deinit &&
            api_.reset &&
            api_.run &&
            api_.loadGame &&
            api_.unloadGame &&
            api_.setEnvironment &&
            api_.setVideoRefresh &&
            api_.setAudioSample &&
            api_.setAudioSampleBatch &&
            api_.setInputPoll &&
            api_.setInputState;
    }


    bool LibretroHost::configureDirectories()
    {
        wchar_t buffer[32768]{};

        const DWORD length =
            GetModuleFileNameW(
                nullptr,
                buffer,
                static_cast<DWORD>(std::size(buffer)));

        if (length == 0 ||
            length >= std::size(buffer))
        {
            return false;
        }

        const std::filesystem::path exePath(
            std::wstring(buffer, length));

        const std::filesystem::path exeDir =
            exePath.parent_path();

        /*
         * Current layout:
         *
         * SaturnX/
         *   build/debug/bin/Debug/SaturnX.exe
         *
         * so go four levels upward:
         *
         * Debug
         * bin
         * debug
         * build
         * then SaturnX/
         */
        std::filesystem::path root =
            exeDir;

        for (int i = 0; i < 4; ++i)
            root = root.parent_path();

        const std::filesystem::path systemPath =
            root / L"runtime" / L"system";

        const std::filesystem::path savePath =
            root / L"runtime" / L"saves";

        std::error_code ec;

        std::filesystem::create_directories(
            systemPath,
            ec);

        std::filesystem::create_directories(
            savePath,
            ec);

        systemDirectory_ =
            wideToUtf8(
                systemPath.wstring());

        saveDirectory_ =
            wideToUtf8(
                savePath.wstring());

        coreAssetsDirectory_ =
            systemDirectory_;

        return true;
    }


    void LibretroHost::unload()
    {
        if (!module_)
            return;

        if (gameLoaded_ &&
            api_.unloadGame)
        {
            api_.unloadGame();
            gameLoaded_ = false;
        }

        if (initialized_ &&
            api_.deinit)
        {
            api_.deinit();
            initialized_ = false;
        }

        FreeLibrary(module_);

        module_ = nullptr;
        activeHost_ = nullptr;

        api_ = {};

        frame_ = {};

        coreOptions_.clear();

        systemDirectory_.clear();
        saveDirectory_.clear();
        coreAssetsDirectory_.clear();
        contentDirectory_.clear();
        currentGamePath_.clear();
    }


    bool LibretroHost::init()
    {
        return module_ &&
            initialized_;
    }


    void LibretroHost::deinit()
    {
        if (!module_ ||
            !initialized_)
        {
            return;
        }

        if (gameLoaded_ &&
            api_.unloadGame)
        {
            api_.unloadGame();
            gameLoaded_ = false;
        }

        api_.deinit();

        initialized_ = false;
    }


    bool LibretroHost::loadGame(
        const char* path)
    {
        if (!module_ ||
            !initialized_ ||
            !api_.loadGame ||
            !path ||
            !*path)
        {
            return false;
        }

        retro_game_info game{};

        game.path = path;
        game.data = nullptr;
        game.size = 0;
        game.meta = nullptr;

        currentGamePath_ = path;
        contentDirectory_ =
            parentDirectory(currentGamePath_);

        OutputDebugStringA(
            "SaturnX: Loading game:\n");

        OutputDebugStringA(
            currentGamePath_.c_str());

        OutputDebugStringA("\n");

        const bool result =
            api_.loadGame(&game);

        if (!result)
        {
            OutputDebugStringA(
                "SaturnX: retro_load_game() FAILED\n");
        }
        else
        {
            OutputDebugStringA(
                "SaturnX: retro_load_game() OK\n");
        }

        gameLoaded_ = result;

        return result;
    }


    void LibretroHost::unloadGame()
    {
        if (!module_ ||
            !gameLoaded_ ||
            !api_.unloadGame)
        {
            return;
        }

        api_.unloadGame();

        gameLoaded_ = false;
        frame_.valid = false;
    }


    void LibretroHost::reset()
    {
        if (module_ &&
            initialized_ &&
            api_.reset)
        {
            api_.reset();
        }
    }


    void LibretroHost::run()
    {
        if (!module_ ||
            !initialized_ ||
            !gameLoaded_ ||
            !api_.run)
        {
            return;
        }

        api_.run();
    }


    unsigned LibretroHost::apiVersion() const
    {
        if (!module_ ||
            !api_.apiVersion)
        {
            return 0;
        }

        return api_.apiVersion();
    }


    bool LibretroHost::isLoaded() const
    {
        return module_ != nullptr &&
            initialized_;
    }


    const VideoFrame&
        LibretroHost::videoFrame() const
    {
        return frame_;
    }


    void LibretroHost::setController(
        unsigned port,
        const ControllerState& state)
    {
        if (port >= 2)
            return;

        controllers_[port] = state;
    }


    /*
     * --------------------------------------------------------------------------
     * Environment callback
     * --------------------------------------------------------------------------
     */

    bool RETRO_CALLCONV
        LibretroHost::environmentCallback(
            unsigned cmd,
            void* data)
    {
        LibretroHost* host =
            activeHost_;

        if (!host)
            return false;

        host->reportEnvironmentCommand(cmd);

        switch (cmd)
        {
            /*
             * ------------------------------------------------------------------
             * Core options
             * ------------------------------------------------------------------
             */

        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        {
            if (!data)
                return false;

            unsigned* version =
                static_cast<unsigned*>(data);

            *version = 0;

            return true;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES:
        {
            if (!data)
                return false;

            const retro_variable* variables =
                static_cast<const retro_variable*>(data);

            host->coreOptions_.clear();

            for (const retro_variable* var = variables;
                var && var->key;
                ++var)
            {
                std::string value;

                if (var->value)
                {
                    /*
                     * Format:
                     *
                     * "Description; option1|option2|option3"
                     *
                     * The text after ';' contains the available
                     * options. We choose the first option as default.
                     */
                    const char* separator =
                        std::strchr(
                            var->value,
                            ';');

                    if (separator)
                    {
                        ++separator;

                        while (*separator == ' ')
                            ++separator;

                        value = separator;

                        const size_t pipe =
                            value.find('|');

                        if (pipe != std::string::npos)
                            value.resize(pipe);
                    }
                    else
                    {
                        value =
                            var->value;
                    }
                }

                host->coreOptions_.emplace(
                    var->key,
                    std::move(value));
            }

            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE:
        {
            if (!data)
                return false;

            retro_variable* variable =
                static_cast<retro_variable*>(data);

            if (!variable->key)
                return false;

            const auto it =
                host->coreOptions_.find(
                    variable->key);

            if (it == host->coreOptions_.end())
            {
                variable->value = nullptr;
                return false;
            }

            variable->value =
                it->second.c_str();

            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        {
            if (!data)
                return false;

            *static_cast<bool*>(data) = false;

            return true;
        }


        /*
         * ------------------------------------------------------------------
         * Directories
         * ------------------------------------------------------------------
         */

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        {
            if (!data)
                return false;

            *static_cast<const char**>(data) =
                host->systemDirectory_.c_str();

            return true;
        }

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        {
            if (!data)
                return false;

            *static_cast<const char**>(data) =
                host->saveDirectory_.c_str();

            return true;
        }


        /*
         * ------------------------------------------------------------------
         * Video
         * ------------------------------------------------------------------
         */

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        {
            if (!data)
                return false;

            const retro_pixel_format format =
                *static_cast<const retro_pixel_format*>(
                    data);

            return format ==
                RETRO_PIXEL_FORMAT_XRGB8888;
        }


        /*
         * ------------------------------------------------------------------
         * Logging
         * ------------------------------------------------------------------
         */

        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        {
            if (!data)
                return false;

            retro_log_callback* callback =
                static_cast<retro_log_callback*>(
                    data);

            callback->log =
                &LibretroHost::logCallback;

            return true;
        }


        /*
         * ------------------------------------------------------------------
         * Virtual File System
         * ------------------------------------------------------------------
         */

        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
        {
            if (!data)
                return false;

            retro_vfs_interface_info* info =
                static_cast<retro_vfs_interface_info*>(
                    data);

            /*
             * SaturnX implements VFS API v1.
             *
             * If the core asks for a newer ABI,
             * we correctly reject it.
             */
            if (info->required_interface_version > 1)
            {
                info->iface = nullptr;
                return false;
            }

            info->iface =
                &LibretroHost::vfsInterface_;

            return true;
        }


        /*
         * ------------------------------------------------------------------
         * Input
         * ------------------------------------------------------------------
         */

        case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES:
        {
            if (!data)
                return false;

            uint64_t* capabilities =
                static_cast<uint64_t*>(data);

            *capabilities =
                (uint64_t{ 1 } << RETRO_DEVICE_JOYPAD);

            return true;
        }

        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        {
            return true;
        }


        /*
         * ------------------------------------------------------------------
         * Misc compatibility
         * ------------------------------------------------------------------
         */

        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
        {
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
        {
            return true;
        }

        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        {
            return true;
        }

        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
        {
            return true;
        }

        case RETRO_ENVIRONMENT_SET_MESSAGE:
        {
            return true;
        }

        default:
        {
            /*
             * Unknown/unsupported environment request.
             *
             * This is correct Libretro behavior: only return true
             * when we actually implemented the request.
             */
            return false;
        }
        }
    }


    /*
     * --------------------------------------------------------------------------
     * Video
     * --------------------------------------------------------------------------
     */

    void RETRO_CALLCONV
        LibretroHost::videoCallback(
            const void* data,
            unsigned width,
            unsigned height,
            size_t pitch)
    {
        LibretroHost* host =
            activeHost_;

        if (!host)
            return;

        /*
         * NULL means "duplicate previous frame" when the frontend
         * advertises support for frame duping.
         *
         * We don't advertise frame duping, so NULL is treated as
         * an invalid frame.
         */
        if (!data ||
            width == 0 ||
            height == 0 ||
            pitch == 0)
        {
            host->frame_.valid = false;
            return;
        }

        const size_t bytes =
            pitch *
            static_cast<size_t>(height);

        host->frame_.pixels.resize(bytes);

        std::memcpy(
            host->frame_.pixels.data(),
            data,
            bytes);

        host->frame_.width = width;
        host->frame_.height = height;
        host->frame_.pitch = pitch;
        host->frame_.valid = true;
    }


    /*
     * --------------------------------------------------------------------------
     * Audio
     * --------------------------------------------------------------------------
     */

    void RETRO_CALLCONV
        LibretroHost::audioCallback(
            int16_t,
            int16_t)
    {
        /*
         * Audio backend comes later.
         */
    }


    size_t RETRO_CALLCONV
        LibretroHost::audioBatchCallback(
            const int16_t*,
            size_t frames)
    {
        /*
         * Report all frames as consumed.
         *
         * Currently SaturnX has no audio output device attached.
         */
        return frames;
    }


    /*
     * --------------------------------------------------------------------------
     * Input
     * --------------------------------------------------------------------------
     */

    void RETRO_CALLCONV
        LibretroHost::inputPollCallback()
    {
    }


    unsigned LibretroHost::controllerMask(
        const ControllerState& state)
    {
        unsigned mask = 0;

        if (state.b)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_B);

        if (state.y)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_Y);

        if (state.start)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_START);

        if (state.up)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_UP);

        if (state.down)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_DOWN);

        if (state.left)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_LEFT);

        if (state.right)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_RIGHT);

        if (state.a)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_A);

        if (state.x)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_X);

        if (state.l)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_L);

        if (state.r)
            mask |= (1u << RETRO_DEVICE_ID_JOYPAD_R);

        return mask;
    }


    int16_t RETRO_CALLCONV
        LibretroHost::inputStateCallback(
            unsigned port,
            unsigned device,
            unsigned,
            unsigned id)
    {
        LibretroHost* host =
            activeHost_;

        if (!host ||
            port >= 2)
        {
            return 0;
        }

        if ((device & RETRO_DEVICE_MASK) !=
            RETRO_DEVICE_JOYPAD)
        {
            return 0;
        }

        const ControllerState& state =
            host->controllers_[port];

        if (id == RETRO_DEVICE_ID_JOYPAD_MASK)
        {
            return static_cast<int16_t>(
                controllerMask(state));
        }

        switch (id)
        {
        case RETRO_DEVICE_ID_JOYPAD_B:
            return state.b ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_Y:
            return state.y ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_START:
            return state.start ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_UP:
            return state.up ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_DOWN:
            return state.down ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_LEFT:
            return state.left ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_RIGHT:
            return state.right ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_A:
            return state.a ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_X:
            return state.x ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_L:
            return state.l ? 1 : 0;

        case RETRO_DEVICE_ID_JOYPAD_R:
            return state.r ? 1 : 0;

        default:
            return 0;
        }
    }


    /*
     * --------------------------------------------------------------------------
     * Logging
     * --------------------------------------------------------------------------
     */

    void RETRO_CALLCONV
        LibretroHost::logCallback(
            enum retro_log_level level,
            const char* fmt,
            ...)
    {
        if (!fmt)
            return;

        char message[4096]{};

        va_list args;

        va_start(args, fmt);

        _vsnprintf_s(
            message,
            sizeof(message),
            _TRUNCATE,
            fmt,
            args);

        va_end(args);

        char output[4608]{};

        sprintf_s(
            output,
            "[Beetle][%s] %s",
            logLevelName(level),
            message);

        OutputDebugStringA(output);
    }


    const char*
        LibretroHost::logLevelName(
            enum retro_log_level level)
    {
        switch (level)
        {
        case RETRO_LOG_DEBUG:
            return "DEBUG";

        case RETRO_LOG_INFO:
            return "INFO";

        case RETRO_LOG_WARN:
            return "WARN";

        case RETRO_LOG_ERROR:
            return "ERROR";

        default:
            return "LOG";
        }
    }


    void LibretroHost::reportEnvironmentCommand(
        unsigned cmd) const
    {
        char buffer[128]{};

        sprintf_s(
            buffer,
            "[SaturnX] environment cmd=%u\n",
            cmd);

        OutputDebugStringA(buffer);
    }


    /*
     * --------------------------------------------------------------------------
     * VFS
     * --------------------------------------------------------------------------
     */

    const char* RETRO_CALLCONV
        LibretroHost::vfsGetPath(
            retro_vfs_file_handle* stream)
    {
        const VfsFile* file =
            toVfsFile(stream);

        if (!file)
            return nullptr;

        return file->path.c_str();
    }


    retro_vfs_file_handle* RETRO_CALLCONV
        LibretroHost::vfsOpen(
            const char* path,
            unsigned mode,
            unsigned)
    {
        if (!path || !*path)
            return nullptr;

        /*
         * Libretro paths are UTF-8.
         * Convert them to UTF-16 so Windows can open them correctly.
         */
        const std::wstring widePath =
            utf8ToWide(path);

        if (widePath.empty())
            return nullptr;

        const bool read =
            hasRead(mode);

        const bool write =
            hasWrite(mode);

        const bool update =
            hasUpdateExisting(mode);

        const wchar_t* fileMode = L"rb";

        if (read && write)
        {
            fileMode =
                update
                ? L"r+b"
                : L"w+b";
        }
        else if (write)
        {
            fileMode = L"wb";
        }

        FILE* file = nullptr;

        if (_wfopen_s(
            &file,
            widePath.c_str(),
            fileMode) != 0 ||
            !file)
        {
            char buffer[4096]{};

            sprintf_s(
                buffer,
                "[SaturnX][VFS] OPEN FAILED: %s\n",
                path);

            OutputDebugStringA(buffer);

            return nullptr;
        }

        VfsFile* handle =
            new VfsFile;

        handle->file = file;
        handle->path = path;

        {
            char buffer[4096]{};

            sprintf_s(
                buffer,
                "[SaturnX][VFS] OPEN: %s\n",
                path);

            OutputDebugStringA(buffer);
        }

        return reinterpret_cast<retro_vfs_file_handle*>(
            handle);
    }


    int RETRO_CALLCONV
        LibretroHost::vfsClose(
            retro_vfs_file_handle* stream)
    {
        VfsFile* file =
            toVfsFile(stream);

        if (!file)
            return -1;

        const int result =
            file->file
            ? std::fclose(file->file)
            : -1;

        delete file;

        return result == 0
            ? 0
            : -1;
    }


    int64_t RETRO_CALLCONV
        LibretroHost::vfsSize(
            retro_vfs_file_handle* stream)
    {
        VfsFile* file =
            toVfsFile(stream);

        if (!file || !file->file)
            return -1;

        const __int64 oldPosition =
            _ftelli64(file->file);

        if (oldPosition < 0)
            return -1;

        if (_fseeki64(
            file->file,
            0,
            SEEK_END) != 0)
        {
            return -1;
        }

        const __int64 size =
            _ftelli64(file->file);

        _fseeki64(
            file->file,
            oldPosition,
            SEEK_SET);

        return static_cast<int64_t>(size);
    }


    int64_t RETRO_CALLCONV
        LibretroHost::vfsTell(
            retro_vfs_file_handle* stream)
    {
        VfsFile* file =
            toVfsFile(stream);

        if (!file || !file->file)
            return -1;

        return static_cast<int64_t>(
            _ftelli64(file->file));
    }


    int64_t RETRO_CALLCONV
        LibretroHost::vfsSeek(
            retro_vfs_file_handle* stream,
            int64_t offset,
            int seekPosition)
    {
        VfsFile* file =
            toVfsFile(stream);

        if (!file || !file->file)
            return -1;

        int origin = SEEK_SET;

        switch (seekPosition)
        {
        case RETRO_VFS_SEEK_POSITION_START:
            origin = SEEK_SET;
            break;

        case RETRO_VFS_SEEK_POSITION_CURRENT:
            origin = SEEK_CUR;
            break;

        case RETRO_VFS_SEEK_POSITION_END:
            origin = SEEK_END;
            break;

        default:
            return -1;
        }

        if (_fseeki64(
            file->file,
            offset,
            origin) != 0)
        {
            return -1;
        }

        return static_cast<int64_t>(
            _ftelli64(file->file));
    }


    int64_t RETRO_CALLCONV
        LibretroHost::vfsRead(
            retro_vfs_file_handle* stream,
            void* data,
            uint64_t size)
    {
        VfsFile* file =
            toVfsFile(stream);

        if (!file ||
            !file->file ||
            (!data && size != 0))
        {
            return -1;
        }

        if (size == 0)
            return 0;

        return static_cast<int64_t>(
            std::fread(
                data,
                1,
                static_cast<size_t>(size),
                file->file));
    }


    int64_t RETRO_CALLCONV
        LibretroHost::vfsWrite(
            retro_vfs_file_handle* stream,
            const void* data,
            uint64_t size)
    {
        VfsFile* file =
            toVfsFile(stream);

        if (!file ||
            !file->file ||
            (!data && size != 0))
        {
            return -1;
        }

        if (size == 0)
            return 0;

        return static_cast<int64_t>(
            std::fwrite(
                data,
                1,
                static_cast<size_t>(size),
                file->file));
    }


    int RETRO_CALLCONV
        LibretroHost::vfsFlush(
            retro_vfs_file_handle* stream)
    {
        VfsFile* file =
            toVfsFile(stream);

        if (!file || !file->file)
            return -1;

        return std::fflush(file->file) == 0
            ? 0
            : -1;
    }


    int RETRO_CALLCONV
        LibretroHost::vfsRemove(
            const char* path)
    {
        if (!path || !*path)
            return -1;

        const std::wstring widePath =
            utf8ToWide(path);

        if (widePath.empty())
            return -1;

        return _wremove(
            widePath.c_str()) == 0
            ? 0
            : -1;
    }


    int RETRO_CALLCONV
        LibretroHost::vfsRename(
            const char* oldPath,
            const char* newPath)
    {
        if (!oldPath ||
            !newPath)
        {
            return -1;
        }

        const std::wstring oldWide =
            utf8ToWide(oldPath);

        const std::wstring newWide =
            utf8ToWide(newPath);

        if (oldWide.empty() ||
            newWide.empty())
        {
            return -1;
        }

        return _wrename(
            oldWide.c_str(),
            newWide.c_str()) == 0
            ? 0
            : -1;
    }


    /*
     * --------------------------------------------------------------------------
     * Encoding helpers
     * --------------------------------------------------------------------------
     */

    std::string LibretroHost::wideToUtf8(
        const std::wstring& value)
    {
        if (value.empty())
            return {};

        const int length =
            WideCharToMultiByte(
                CP_UTF8,
                0,
                value.c_str(),
                static_cast<int>(value.size()),
                nullptr,
                0,
                nullptr,
                nullptr);

        if (length <= 0)
            return {};

        std::string result(
            static_cast<size_t>(length),
            '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(value.size()),
            result.data(),
            length,
            nullptr,
            nullptr);

        return result;
    }


    std::wstring LibretroHost::utf8ToWide(
        const std::string& value)
    {
        if (value.empty())
            return {};

        const int length =
            MultiByteToWideChar(
                CP_UTF8,
                0,
                value.c_str(),
                static_cast<int>(value.size()),
                nullptr,
                0);

        if (length <= 0)
            return {};

        std::wstring result(
            static_cast<size_t>(length),
            L'\0');

        MultiByteToWideChar(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(value.size()),
            result.data(),
            length);

        return result;
    }

}