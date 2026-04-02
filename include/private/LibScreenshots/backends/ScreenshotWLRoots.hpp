#pragma once

#if HAVE_WLROOTS_SCREEN_COPY

#include <vector>
#include <cstddef>
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
        ScreenshotWLRoots();

        ~ScreenshotWLRoots();

        bool captureInternal(wl_output *output);

        wl_display *m_display = nullptr;
        wl_registry *m_registry = nullptr;

        zwlr_screencopy_manager_v1 *m_manager = nullptr;
        zwlr_screencopy_frame_v1 *m_frame = nullptr;

        wl_shm *m_shm = nullptr;
        wl_shm_pool *m_shm_pool = nullptr;
        wl_buffer *m_wl_buffer = nullptr;
        void *m_shm_data = nullptr;
        std::size_t m_shm_size = 0;

        std::vector<wl_output *> m_outputs;

        std::vector<uint8_t> m_buffer;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_stride = 0;

        bool m_frame_done = false;
    };
}
#endif