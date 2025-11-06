#pragma once

#include <LibScreenshots/ScreenshotResult.hpp>
#include <LibScreenshots/IScreenshotBackend.hpp>

#ifdef HAVE_PIPEWIRE
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/props.h>
#endif

#include <vector>
#include <mutex>
#include <condition_variable>

namespace LibScreenshots {

    class ScreenshotPipeWire : public IScreenshotBackend {
    public:
        static ScreenshotPipeWire &getInstance();

        ScreenshotResult captureScreen() override;
        ScreenshotResult captureRegion(int x, int y, int width, int height) override;

        ~ScreenshotPipeWire();

    private:
        ScreenshotPipeWire();

        bool initializePipeWire();
        bool requestScreenCast();
        void cleanup();

        void startStream();
        void stopStream();
        ScreenshotResult captureFrame();

#ifdef HAVE_PIPEWIRE
        struct pw_thread_loop *loop_ = nullptr;
        struct pw_context *context_ = nullptr;
        struct pw_core *core_ = nullptr;
        struct pw_stream *stream_ = nullptr;

        std::vector<uint8_t> frameBuffer_;
        int frameWidth_ = 0;
        int frameHeight_ = 0;
        int frameStride_ = 0;
        uint32_t frameFormat_ = 0;

        std::mutex frameMutex_;
        std::condition_variable frameCv_;
        bool frameReady_ = false;
        bool streamActive_ = false;

        std::string sessionHandle_;
        int pipewireFd_ = -1;
        uint32_t pipewireNode_ = 0;


        static void onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *param);
        static void onStreamProcess(void *data);
        static void onStreamStateChanged(void *data, enum pw_stream_state old,
                                         enum pw_stream_state state, const char *error);
#endif

        bool initialized_ = false;
    };

} // namespace LibScreenshots
