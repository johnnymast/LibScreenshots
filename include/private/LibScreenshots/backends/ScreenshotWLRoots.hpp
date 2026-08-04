#pragma once

#if HAVE_WLROOTS_SCREEN_COPY
#include <vector>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <mutex>
#include <wayland-client.h>
#include "wlr-screencopy-unstable-v1-client-protocol.h"

#include "LibScreenshots/IScreenshotBackend.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

namespace LibScreenshots {
    class ScreenshotWLRoots : public IScreenshotBackend {
    public:
        static ScreenshotWLRoots &getInstance();

        ScreenshotResult captureScreen() override;

        ScreenshotResult captureRegion(int x, int y, int width, int height) override;
        static void registryAdd(void *data, wl_registry *registry,
                                uint32_t name, const char *interface, uint32_t version);

        static void registryRemove(void *data, wl_registry *registry, uint32_t name);

        static void frameBuffer(void *data, zwlr_screencopy_frame_v1 *frame,
                                uint32_t format, uint32_t width, uint32_t height, uint32_t stride);

        static void frameFlags(void *data, zwlr_screencopy_frame_v1 *frame, uint32_t flags);

        static void frameReady(void *data, zwlr_screencopy_frame_v1 *frame,
                               uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec);

        static void frameFailed(void *data, zwlr_screencopy_frame_v1 *frame);

        static void frameDamage(void *data, zwlr_screencopy_frame_v1 *frame,
                                uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        static void frameLinuxDmabuf(void *data, zwlr_screencopy_frame_v1 *frame,
                                     uint32_t format, uint32_t width, uint32_t height);

        static void frameBufferDone(void *data, zwlr_screencopy_frame_v1 *frame);

    private:
        struct Output {
            uint32_t name = 0;
            wl_output *proxy = nullptr;
        };

        ScreenshotWLRoots();

        ~ScreenshotWLRoots();

        bool captureInternal(wl_output *output);

        /**
         * Pumps the display until the frame completes or the deadline passes.
         * Uses prepare_read/poll instead of wl_display_dispatch so the timeout
         * is actually honoured; a plain dispatch blocks indefinitely and a
         * silent compositor would hang the caller forever.
         */
        bool dispatchUntilDone(std::chrono::milliseconds timeout);

        /** Allocates the shm backing store and sends the copy request. */
        void allocateAndCopy();

        void resetFrame();

        void releaseBuffer();

        wl_display *m_display = nullptr;
        wl_registry *m_registry = nullptr;

        zwlr_screencopy_manager_v1 *m_manager = nullptr;
        zwlr_screencopy_frame_v1 *m_frame = nullptr;
        uint32_t m_manager_version = 0;

        wl_shm *m_shm = nullptr;
        wl_shm_pool *m_shm_pool = nullptr;
        wl_buffer *m_wl_buffer = nullptr;
        void *m_shm_data = nullptr;
        std::size_t m_shm_size = 0;

        std::vector<Output> m_outputs;
        wl_output *m_active_output = nullptr;

        std::vector<uint8_t> m_buffer;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_stride = 0;
        uint32_t m_format = 0;

        /** True when the shm format stores blue in the first byte (X/ARGB8888). */
        bool m_blue_first = true;

        bool m_have_buffer_params = false;
        bool m_frame_done = false;
        bool m_frame_failed = false;

        /**
         * A single wl_display connection and one set of frame/buffer members are
         * shared by every caller, but TakeScreenshot() is reached from both the
         * plugin pipeline (main thread) and the IPC read thread via
         * Chat::WipeChat(). Without this the two interleave inside frameReady
         * and frameBuffer and memcpy from a just-munmapped mapping. Recursive
         * because captureRegion() delegates to captureScreen().
         */
        std::recursive_mutex m_mutex;
    };
}
#endif
