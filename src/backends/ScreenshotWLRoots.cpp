#include "LibScreenshots/backends/ScreenshotWLRoots.hpp"

#if HAVE_WLROOTS_SCREEN_COPY
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <climits>
#include <atomic>
#include <algorithm>
#include <sys/mman.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>

using namespace std::chrono;

namespace LibScreenshots {

static const wl_registry_listener REGISTRY_LISTENER = {
    ScreenshotWLRoots::registryAdd,
    ScreenshotWLRoots::registryRemove
};

static const zwlr_screencopy_frame_v1_listener FRAME_LISTENER = {
    ScreenshotWLRoots::frameBuffer,
    ScreenshotWLRoots::frameFlags,
    ScreenshotWLRoots::frameReady,
    ScreenshotWLRoots::frameFailed,
    ScreenshotWLRoots::frameDamage,
    ScreenshotWLRoots::frameLinuxDmabuf,
    ScreenshotWLRoots::frameBufferDone
};

/**
 * Reports whether we can convert the given wl_shm format, and how its bytes are
 * laid out in memory. The conversion loop reads four bytes per pixel, so a
 * format with fewer would overrun the last row of the mapping.
 */
static bool shmFormatChannelOrder(uint32_t format, bool& blueFirst) {
    switch (format) {
        case WL_SHM_FORMAT_ARGB8888:
        case WL_SHM_FORMAT_XRGB8888:
            blueFirst = true;   // little-endian 0xAARRGGBB -> B G R A
            return true;
        case WL_SHM_FORMAT_ABGR8888:
        case WL_SHM_FORMAT_XBGR8888:
            blueFirst = false;  // little-endian 0xAABBGGRR -> R G B A
            return true;
        default:
            return false;
    }
}

/**
 * The name has to be unique per call. The previous version handed the literal
 * "/libshots-XXXXXX" to shm_open without ever expanding the template, so two
 * concurrent captures raced on one name and a crash before shm_unlink left a
 * stale object behind that broke every later capture with EEXIST.
 */
static int create_shm_file(std::size_t size) {
    static std::atomic<unsigned> counter{0};

    for (int attempt = 0; attempt < 16; ++attempt) {
        char name[64];
        std::snprintf(name, sizeof(name), "/libshots-%d-%u",
                      static_cast<int>(getpid()), counter.fetch_add(1));

        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd < 0) {
            if (errno == EEXIST)
                continue;
            return -1;
        }

        shm_unlink(name);

        if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
            close(fd);
            return -1;
        }

        return fd;
    }

    return -1;
}

ScreenshotWLRoots& ScreenshotWLRoots::getInstance() {
    /**
     * Deliberately leaked. The IPC threads in LibCore are detached and never
     * joined, so they can still be inside a capture when static destructors
     * run; tearing the connection down underneath them is a use-after-free.
     * The OS reclaims the connection and the mapping at process exit.
     */
    static ScreenshotWLRoots* instance = new ScreenshotWLRoots();
    return *instance;
}

ScreenshotWLRoots::ScreenshotWLRoots() {
    m_display = wl_display_connect(nullptr);
    if (!m_display)
        return;

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &REGISTRY_LISTENER, this);
    wl_display_roundtrip(m_display);
}

ScreenshotWLRoots::~ScreenshotWLRoots() {
    resetFrame();
    releaseBuffer();

    for (auto& out : m_outputs)
        wl_output_destroy(out.proxy);
    m_outputs.clear();

    if (m_manager)
        zwlr_screencopy_manager_v1_destroy(m_manager);
    if (m_registry)
        wl_registry_destroy(m_registry);
    if (m_display)
        wl_display_disconnect(m_display);
}

void ScreenshotWLRoots::registryAdd(
    void* data, wl_registry* registry,
    uint32_t name, const char* interface, uint32_t version
) {
    auto self = static_cast<ScreenshotWLRoots*>(data);

    if (strcmp(interface, wl_output_interface.name) == 0) {
        wl_output* out = static_cast<wl_output*>(
            wl_registry_bind(registry, name, &wl_output_interface, 1)
        );
        self->m_outputs.push_back(Output{name, out});
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        self->m_shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1)
        );
    } else if (strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0) {
        self->m_manager_version = std::min(version, 3u);
        self->m_manager = static_cast<zwlr_screencopy_manager_v1*>(
            wl_registry_bind(registry, name,
                             &zwlr_screencopy_manager_v1_interface,
                             self->m_manager_version)
        );
    }
}

/**
 * Used to be a no-op, which left destroyed wl_output globals in m_outputs.
 * captureScreen() then handed a dead proxy to capture_output, and Hyprland
 * recycles outputs on monitor, VRR and DPMS transitions often enough for that
 * to look like a random failure.
 */
void ScreenshotWLRoots::registryRemove(void* data, wl_registry*, uint32_t name) {
    auto self = static_cast<ScreenshotWLRoots*>(data);

    const auto it = std::find_if(
        self->m_outputs.begin(), self->m_outputs.end(),
        [name](const Output& out) { return out.name == name; }
    );

    if (it == self->m_outputs.end())
        return;

    // The output behind the in-flight frame is gone, so it can never complete.
    if (self->m_active_output == it->proxy) {
        self->m_active_output = nullptr;
        self->m_buffer.clear();
        self->m_frame_failed = true;
        self->m_frame_done = true;
    }

    wl_output_destroy(it->proxy);
    self->m_outputs.erase(it);
}

void ScreenshotWLRoots::resetFrame() {
    if (m_frame) {
        zwlr_screencopy_frame_v1_destroy(m_frame);
        m_frame = nullptr;
    }
}

void ScreenshotWLRoots::releaseBuffer() {
    /**
     * Order matters. The compositor reaches the pixels through the wl_buffer and
     * the pool, so both have to go before we drop our own mapping.
     */
    if (m_wl_buffer) {
        wl_buffer_destroy(m_wl_buffer);
        m_wl_buffer = nullptr;
    }
    if (m_shm_pool) {
        wl_shm_pool_destroy(m_shm_pool);
        m_shm_pool = nullptr;
    }
    if (m_shm_data) {
        munmap(m_shm_data, m_shm_size);
        m_shm_data = nullptr;
    }

    m_shm_size = 0;
}

void ScreenshotWLRoots::allocateAndCopy() {
    if (!m_frame || !m_shm || !m_have_buffer_params) {
        m_frame_failed = true;
        m_frame_done = true;
        return;
    }

    const std::size_t stride = m_stride;
    const std::size_t size = static_cast<std::size_t>(m_height) * stride;

    if (m_width == 0 || m_height == 0 || size == 0 || size > INT_MAX ||
        stride < static_cast<std::size_t>(m_width) * 4) {
        m_frame_failed = true;
        m_frame_done = true;
        return;
    }

    releaseBuffer();

    const int fd = create_shm_file(size);
    if (fd < 0) {
        m_frame_failed = true;
        m_frame_done = true;
        return;
    }

    void* data_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data_ptr == MAP_FAILED) {
        close(fd);
        m_frame_failed = true;
        m_frame_done = true;
        return;
    }

    m_shm_pool = wl_shm_create_pool(m_shm, fd, static_cast<int>(size));
    close(fd);

    if (!m_shm_pool) {
        munmap(data_ptr, size);
        m_frame_failed = true;
        m_frame_done = true;
        return;
    }

    m_shm_data = data_ptr;
    m_shm_size = size;

    m_wl_buffer = wl_shm_pool_create_buffer(
        m_shm_pool,
        0,
        static_cast<int>(m_width),
        static_cast<int>(m_height),
        static_cast<int>(m_stride),
        m_format
    );

    if (!m_wl_buffer) {
        m_frame_failed = true;
        m_frame_done = true;
        return;
    }

    zwlr_screencopy_frame_v1_copy(m_frame, m_wl_buffer);
}

bool ScreenshotWLRoots::dispatchUntilDone(milliseconds timeout) {
    const int fd = wl_display_get_fd(m_display);
    const auto deadline = steady_clock::now() + timeout;

    while (!m_frame_done) {
        if (wl_display_dispatch_pending(m_display) < 0)
            return false;

        if (m_frame_done)
            break;

        if (wl_display_flush(m_display) < 0 && errno != EAGAIN)
            return false;

        const auto remaining = duration_cast<milliseconds>(deadline - steady_clock::now());
        if (remaining.count() <= 0)
            return false;

        // A failed prepare_read means the queue refilled; drain it and retry.
        while (wl_display_prepare_read(m_display) != 0) {
            if (wl_display_dispatch_pending(m_display) < 0)
                return false;
            if (m_frame_done)
                return true;
        }

        pollfd pfd{fd, POLLIN, 0};
        const int ret = poll(&pfd, 1, static_cast<int>(remaining.count()));

        if (ret < 0) {
            wl_display_cancel_read(m_display);
            if (errno == EINTR)
                continue;
            return false;
        }

        if (ret == 0) {
            wl_display_cancel_read(m_display);
            return false;
        }

        if (wl_display_read_events(m_display) < 0)
            return false;
    }

    return m_frame_done;
}

bool ScreenshotWLRoots::captureInternal(wl_output* output) {
    m_buffer.clear();
    m_frame_done = false;
    m_frame_failed = false;
    m_have_buffer_params = false;

    if (!m_manager || !m_shm)
        return false;

    // Nothing should be in flight, but never build on top of leftover state.
    resetFrame();
    releaseBuffer();

    m_active_output = output;

    m_frame = zwlr_screencopy_manager_v1_capture_output(m_manager, 1, output);
    if (!m_frame) {
        m_active_output = nullptr;
        return false;
    }

    zwlr_screencopy_frame_v1_add_listener(m_frame, &FRAME_LISTENER, this);

    const bool completed = dispatchUntilDone(milliseconds(500));

    /**
     * Tear the frame down before its buffer even when we time out. The previous
     * version left an abandoned frame holding a pending copy against a
     * wl_buffer that the next capture then destroyed under the compositor.
     * frameReady already copied the pixels into m_buffer, so dropping the
     * mapping here costs nothing.
     */
    resetFrame();
    releaseBuffer();
    m_active_output = nullptr;

    if (m_display)
        wl_display_flush(m_display);

    return completed && !m_frame_failed && !m_buffer.empty();
}

ScreenshotResult ScreenshotWLRoots::captureScreen() {
    const std::lock_guard<std::recursive_mutex> guard(m_mutex);

    ScreenshotResult result{};

    if (!m_display || !m_manager || !m_shm || m_outputs.empty())
        return result;

    if (!captureInternal(m_outputs.front().proxy))
        return result;

    const std::size_t width = m_width;
    const std::size_t height = m_height;
    const std::size_t stride = m_stride;

    if (stride < width * 4 || m_buffer.size() < height * stride)
        return result;

    std::vector<uint8_t> rgb(width * height * 3);

    const std::size_t r_offset = m_blue_first ? 2 : 0;
    const std::size_t b_offset = m_blue_first ? 0 : 2;

    for (std::size_t y = 0; y < height; ++y) {
        const uint8_t* src = m_buffer.data() + y * stride;
        uint8_t* dst = rgb.data() + y * width * 3;

        for (std::size_t x = 0; x < width; ++x) {
            dst[x * 3 + 0] = src[x * 4 + r_offset];
            dst[x * 3 + 1] = src[x * 4 + 1];
            dst[x * 3 + 2] = src[x * 4 + b_offset];
        }
    }

    LibGraphics::Image img(static_cast<int>(width),
                           static_cast<int>(height),
                           3,
                           std::move(rgb));

    result.image = img;
    result.width = img.width;
    result.height = img.height;
    result.channels = img.channels;

    return result;
}

ScreenshotResult ScreenshotWLRoots::captureRegion(int x, int y, int width, int height) {
    const std::lock_guard<std::recursive_mutex> guard(m_mutex);

    ScreenshotResult full = captureScreen();
    if (!full.isValid())
        return ScreenshotResult{};

    ScreenshotResult region;
    region.image = full.image.crop(x, y, width, height);
    region.width = region.image.width;
    region.height = region.image.height;
    region.channels = region.image.channels;

    return region;
}

void ScreenshotWLRoots::frameBuffer(
    void* data, zwlr_screencopy_frame_v1*,
    uint32_t format, uint32_t width, uint32_t height, uint32_t stride
) {
    auto self = static_cast<ScreenshotWLRoots*>(data);

    bool blueFirst = true;
    if (!shmFormatChannelOrder(format, blueFirst))
        return; // Unsupported; a later buffer event may offer one we can use.

    self->m_format = format;
    self->m_blue_first = blueFirst;
    self->m_width = width;
    self->m_height = height;
    self->m_stride = stride;
    self->m_have_buffer_params = true;

    /**
     * Version 3 enumerates the supported buffer types and only allows copy
     * after buffer_done. Versions below that never send buffer_done, so there
     * the copy has to go out from here.
     */
    if (self->m_manager_version < 3)
        self->allocateAndCopy();
}

void ScreenshotWLRoots::frameFlags(void*, zwlr_screencopy_frame_v1*, uint32_t) {}

void ScreenshotWLRoots::frameReady(
    void* data, zwlr_screencopy_frame_v1*,
    uint32_t, uint32_t, uint32_t
) {
    auto self = static_cast<ScreenshotWLRoots*>(data);

    if (self->m_shm_data && self->m_shm_size > 0) {
        self->m_buffer.resize(self->m_shm_size);
        std::memcpy(self->m_buffer.data(), self->m_shm_data, self->m_shm_size);
    } else {
        self->m_frame_failed = true;
    }

    self->m_frame_done = true;
}

void ScreenshotWLRoots::frameFailed(void* data, zwlr_screencopy_frame_v1*) {
    auto self = static_cast<ScreenshotWLRoots*>(data);
    self->m_buffer.clear();
    self->m_frame_failed = true;
    self->m_frame_done = true;
}

void ScreenshotWLRoots::frameDamage(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t, uint32_t) {}

void ScreenshotWLRoots::frameLinuxDmabuf(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {}

void ScreenshotWLRoots::frameBufferDone(void* data, zwlr_screencopy_frame_v1*) {
    auto self = static_cast<ScreenshotWLRoots*>(data);
    self->allocateAndCopy();
}

}
#endif
