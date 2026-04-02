#include "LibScreenshots/backends/ScreenshotWLRoots.hpp"

#include <cstring>
#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

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

static int create_shm_file(std::size_t size) {
    char name[] = "/libshots-XXXXXX";
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
        return -1;

    shm_unlink(name);

    if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

ScreenshotWLRoots& ScreenshotWLRoots::getInstance() {
    static ScreenshotWLRoots instance;
    return instance;
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
    if (m_wl_buffer)
        wl_buffer_destroy(m_wl_buffer);
    if (m_shm_pool)
        wl_shm_pool_destroy(m_shm_pool);
    if (m_shm_data)
        munmap(m_shm_data, m_shm_size);

    for (auto* out : m_outputs)
        wl_output_destroy(out);

    if (m_frame)
        zwlr_screencopy_frame_v1_destroy(m_frame);
    if (m_manager)
        zwlr_screencopy_manager_v1_destroy(m_manager);
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
        self->m_outputs.push_back(out);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        self->m_shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1)
        );
    } else if (strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0) {
        self->m_manager = static_cast<zwlr_screencopy_manager_v1*>(
            wl_registry_bind(registry, name,
                             &zwlr_screencopy_manager_v1_interface,
                             std::min(version, 3u))
        );
    }
}

void ScreenshotWLRoots::registryRemove(void*, wl_registry*, uint32_t) {}

bool ScreenshotWLRoots::captureInternal(wl_output* output) {
    m_buffer.clear();
    m_frame_done = false;

    if (!m_manager || !m_shm)
        return false;

    if (m_frame) {
        zwlr_screencopy_frame_v1_destroy(m_frame);
        m_frame = nullptr;
    }

    m_frame = zwlr_screencopy_manager_v1_capture_output(
        m_manager, 1, output
    );

    zwlr_screencopy_frame_v1_add_listener(m_frame, &FRAME_LISTENER, this);

    int spins = 0;
    while (!m_frame_done && spins++ < 50)
        wl_display_roundtrip(m_display);

    return !m_buffer.empty();
}

ScreenshotResult ScreenshotWLRoots::captureScreen() {
    ScreenshotResult result{};

    if (!m_display || !m_manager || !m_shm || m_outputs.empty())
        return result;

    wl_output* output = m_outputs[0];

    if (!captureInternal(output))
        return result;

    std::vector<uint8_t> rgb;
    rgb.resize(m_width * m_height * 3);

    for (uint32_t y = 0; y < m_height; ++y) {
        const uint8_t* src = m_buffer.data() + y * m_stride;
        uint8_t* dst = rgb.data() + y * (m_width * 3);

        for (uint32_t x = 0; x < m_width; ++x) {
            uint8_t b = src[x * 4 + 0];
            uint8_t g = src[x * 4 + 1];
            uint8_t r = src[x * 4 + 2];

            dst[x * 3 + 0] = r;
            dst[x * 3 + 1] = g;
            dst[x * 3 + 2] = b;
        }
    }

    LibGraphics::Image img(static_cast<int>(m_width),
                           static_cast<int>(m_height),
                           3,
                           std::move(rgb));

    result.image = img;
    result.width = img.width;
    result.height = img.height;
    result.channels = img.channels;

    return result;
}

ScreenshotResult ScreenshotWLRoots::captureRegion(int x, int y, int width, int height) {
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
    void* data, zwlr_screencopy_frame_v1* frame,
    uint32_t format, uint32_t width, uint32_t height, uint32_t stride
) {
    auto self = static_cast<ScreenshotWLRoots*>(data);

    self->m_width = width;
    self->m_height = height;
    self->m_stride = stride;

    std::size_t size = static_cast<std::size_t>(height) * stride;

    if (self->m_shm_data) {
        munmap(self->m_shm_data, self->m_shm_size);
        self->m_shm_data = nullptr;
        self->m_shm_size = 0;
    }
    if (self->m_wl_buffer) {
        wl_buffer_destroy(self->m_wl_buffer);
        self->m_wl_buffer = nullptr;
    }
    if (self->m_shm_pool) {
        wl_shm_pool_destroy(self->m_shm_pool);
        self->m_shm_pool = nullptr;
    }

    int fd = create_shm_file(size);
    if (fd < 0) {
        self->m_frame_done = true;
        return;
    }

    void* data_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data_ptr == MAP_FAILED) {
        close(fd);
        self->m_frame_done = true;
        return;
    }

    self->m_shm_data = data_ptr;
    self->m_shm_size = size;

    self->m_shm_pool = wl_shm_create_pool(self->m_shm, fd, static_cast<int>(size));
    self->m_wl_buffer = wl_shm_pool_create_buffer(
        self->m_shm_pool,
        0,
        static_cast<int>(width),
        static_cast<int>(height),
        static_cast<int>(stride),
        format
    );

    close(fd);

    zwlr_screencopy_frame_v1_copy(frame, self->m_wl_buffer);
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
    }

    self->m_frame_done = true;
}

void ScreenshotWLRoots::frameFailed(void* data, zwlr_screencopy_frame_v1*) {
    auto self = static_cast<ScreenshotWLRoots*>(data);
    self->m_buffer.clear();
    self->m_frame_done = true;
}

void ScreenshotWLRoots::frameDamage(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t, uint32_t) {}
void ScreenshotWLRoots::frameLinuxDmabuf(void*, zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {}
void ScreenshotWLRoots::frameBufferDone(void*, zwlr_screencopy_frame_v1*) {}

} // namespace LibScreenshots
