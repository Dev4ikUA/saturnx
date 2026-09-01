#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "libretro.h"

namespace SaturnX
{

    struct VideoFrame
    {
        std::vector<uint8_t> pixels;
        unsigned width = 0;
        unsigned height = 0;
        size_t pitch = 0;
        bool valid = false;
    };

    struct ControllerState
    {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;

        bool a = false;
        bool b = false;
        bool c = false;

        bool x = false;
        bool y = false;
        bool z = false;

        bool l = false;
        bool r = false;
        bool start = false;
    };

    class LibretroHost
    {
    public:
        LibretroHost();
        ~LibretroHost();

        LibretroHost(const LibretroHost&) = delete;
        LibretroHost& operator=(const LibretroHost&) = delete;

        bool load(const wchar_t* path);
        void unload();

        bool init();
        void deinit();

        bool loadGame(const char* path);
        void unloadGame();

        void reset();
        void run();

        unsigned apiVersion() const;
        bool isLoaded() const;

        const VideoFrame& videoFrame() const;

        void setController(
            unsigned port,
            const ControllerState& state);

    private:
        struct FunctionTable
        {
            using ApiVersionFn = unsigned (RETRO_CALLCONV*)();
            using InitFn = void (RETRO_CALLCONV*)();
            using DeinitFn = void (RETRO_CALLCONV*)();
            using ResetFn = void (RETRO_CALLCONV*)();
            using RunFn = void (RETRO_CALLCONV*)();

            using LoadGameFn =
                bool (RETRO_CALLCONV*)(const struct retro_game_info*);

            using UnloadGameFn = void (RETRO_CALLCONV*)();

            using GetSystemInfoFn =
                void (RETRO_CALLCONV*)(struct retro_system_info*);

            using GetSystemAvInfoFn =
                void (RETRO_CALLCONV*)(struct retro_system_av_info*);

            using SetEnvironmentFn =
                void (RETRO_CALLCONV*)(retro_environment_t);

            using SetVideoRefreshFn =
                void (RETRO_CALLCONV*)(retro_video_refresh_t);

            using SetAudioSampleFn =
                void (RETRO_CALLCONV*)(retro_audio_sample_t);

            using SetAudioSampleBatchFn =
                void (RETRO_CALLCONV*)(retro_audio_sample_batch_t);

            using SetInputPollFn =
                void (RETRO_CALLCONV*)(retro_input_poll_t);

            using SetInputStateFn =
                void (RETRO_CALLCONV*)(retro_input_state_t);

            using SetControllerPortDeviceFn =
                void (RETRO_CALLCONV*)(unsigned, unsigned);

            ApiVersionFn apiVersion = nullptr;
            InitFn init = nullptr;
            DeinitFn deinit = nullptr;
            ResetFn reset = nullptr;
            RunFn run = nullptr;
            LoadGameFn loadGame = nullptr;
            UnloadGameFn unloadGame = nullptr;
            GetSystemInfoFn getSystemInfo = nullptr;
            GetSystemAvInfoFn getSystemAvInfo = nullptr;
            SetEnvironmentFn setEnvironment = nullptr;
            SetVideoRefreshFn setVideoRefresh = nullptr;
            SetAudioSampleFn setAudioSample = nullptr;
            SetAudioSampleBatchFn setAudioSampleBatch = nullptr;
            SetInputPollFn setInputPoll = nullptr;
            SetInputStateFn setInputState = nullptr;
            SetControllerPortDeviceFn setControllerPortDevice = nullptr;
        };

        HMODULE module_ = nullptr;
        FunctionTable api_{};

        bool initialized_ = false;
        bool gameLoaded_ = false;

        VideoFrame frame_;
        ControllerState controllers_[2]{};

        std::unordered_map<std::string, std::string> coreOptions_;

        std::string systemDirectory_;
        std::string saveDirectory_;
        std::string coreAssetsDirectory_;
        std::string contentDirectory_;

        std::string currentGamePath_;

        static LibretroHost* activeHost_;

        /*
         * Libretro environment.
         */
        static bool RETRO_CALLCONV environmentCallback(
            unsigned cmd,
            void* data);

        /*
         * Video / audio / input.
         */
        static void RETRO_CALLCONV videoCallback(
            const void* data,
            unsigned width,
            unsigned height,
            size_t pitch);

        static void RETRO_CALLCONV audioCallback(
            int16_t left,
            int16_t right);

        static size_t RETRO_CALLCONV audioBatchCallback(
            const int16_t* data,
            size_t frames);

        static void RETRO_CALLCONV inputPollCallback();

        static int16_t RETRO_CALLCONV inputStateCallback(
            unsigned port,
            unsigned device,
            unsigned index,
            unsigned id);

        /*
         * Logging.
         */
        static void RETRO_CALLCONV logCallback(
            enum retro_log_level level,
            const char* fmt,
            ...);

        /*
         * VFS.
         *
         * The libretro header intentionally keeps these handles opaque.
         * Internally SaturnX uses our own FILE-backed handle.
         */
        static const char* RETRO_CALLCONV vfsGetPath(
            struct retro_vfs_file_handle* stream);

        static struct retro_vfs_file_handle*
            RETRO_CALLCONV vfsOpen(
                const char* path,
                unsigned mode,
                unsigned hints);

        static int RETRO_CALLCONV vfsClose(
            struct retro_vfs_file_handle* stream);

        static int64_t RETRO_CALLCONV vfsSize(
            struct retro_vfs_file_handle* stream);

        static int64_t RETRO_CALLCONV vfsTell(
            struct retro_vfs_file_handle* stream);

        static int64_t RETRO_CALLCONV vfsSeek(
            struct retro_vfs_file_handle* stream,
            int64_t offset,
            int seekPosition);

        static int64_t RETRO_CALLCONV vfsRead(
            struct retro_vfs_file_handle* stream,
            void* data,
            uint64_t size);

        static int64_t RETRO_CALLCONV vfsWrite(
            struct retro_vfs_file_handle* stream,
            const void* data,
            uint64_t size);

        static int RETRO_CALLCONV vfsFlush(
            struct retro_vfs_file_handle* stream);

        static int RETRO_CALLCONV vfsRemove(
            const char* path);

        static int RETRO_CALLCONV vfsRename(
            const char* oldPath,
            const char* newPath);

        static const char* createCString(
            const std::string& value);

        /*
         * Helpers.
         */
        bool resolveSymbols();

        bool configureDirectories();

        void reportEnvironmentCommand(unsigned cmd) const;

        template<typename T>
        T symbol(const char* name)
        {
            return reinterpret_cast<T>(
                GetProcAddress(module_, name));
        }

        static std::string wideToUtf8(
            const std::wstring& value);

        static std::wstring utf8ToWide(
            const std::string& value);

        static unsigned controllerMask(
            const ControllerState& state);

        static const char* logLevelName(
            enum retro_log_level level);

        static retro_vfs_interface vfsInterface_;
    };

}