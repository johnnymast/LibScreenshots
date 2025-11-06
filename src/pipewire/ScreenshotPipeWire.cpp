#include "ScreenshotPipeWire.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

#ifdef HAVE_PIPEWIRE

#include <gio/gio.h>
#include <glib.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

using namespace LibScreenshots;
using LibGraphics::Image;

ScreenshotPipeWire::ScreenshotPipeWire() {
    pw_init(nullptr, nullptr);
}

ScreenshotPipeWire::~ScreenshotPipeWire() {
    cleanup();
    pw_deinit();
}

ScreenshotPipeWire &ScreenshotPipeWire::getInstance() {
    static ScreenshotPipeWire instance;
    return instance;
}

bool ScreenshotPipeWire::initializePipeWire() {
    if (initialized_) return true;

    // Request screen cast session from portal
    if (!requestScreenCast()) {
        std::cerr << "[PipeWire] Failed to request screencast session\n";
        return false;
    }

    // Create PipeWire main loop
    loop_ = pw_thread_loop_new("screenshot-loop", nullptr);
    if (!loop_) {
        std::cerr << "[PipeWire] Failed to create thread loop\n";
        return false;
    }

    context_ = pw_context_new(pw_thread_loop_get_loop(loop_), nullptr, 0);
    if (!context_) {
        std::cerr << "[PipeWire] Failed to create context\n";
        cleanup();
        return false;
    }

    if (pw_thread_loop_start(loop_) < 0) {
        std::cerr << "[PipeWire] Failed to start thread loop\n";
        cleanup();
        return false;
    }

    pw_thread_loop_lock(loop_);

    core_ = pw_context_connect(context_, nullptr, 0);
    if (!core_) {
        std::cerr << "[PipeWire] Failed to connect to PipeWire\n";
        pw_thread_loop_unlock(loop_);
        cleanup();
        return false;
    }

    pw_thread_loop_unlock(loop_);

    initialized_ = true;
    std::cout << "[PipeWire] ✅ Initialized successfully\n";
    return true;
}

bool ScreenshotPipeWire::requestScreenCast() {
    GError *error = nullptr;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error) {
        std::cerr << "[PipeWire] Failed to get DBus connection: " << error->message << "\n";
        g_error_free(error);
        return false;
    }

    // Create a session
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string("libscreenshots"));
    g_variant_builder_add(&options, "{sv}", "session_handle_token", g_variant_new_string("libscreenshots_session"));

    GVariant *result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        "CreateSession",
        g_variant_new("(a{sv})", &options),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "[PipeWire] Failed to create session: " << error->message << "\n";
        g_error_free(error);
        g_object_unref(connection);
        return false;
    }

    // Extract session handle
    GVariant *child = nullptr;
    g_variant_get(result, "(@o)", &child);
    const gchar *session_path = g_variant_get_string(child, nullptr);
    sessionHandle_ = session_path;

    g_variant_unref(child);
    g_variant_unref(result);

    // Select sources (entire screen)
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "types", g_variant_new_uint32(1 | 2)); // Monitor + Window
    g_variant_builder_add(&options, "{sv}", "multiple", g_variant_new_boolean(FALSE));

    result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        "SelectSources",
        g_variant_new("(oa{sv})", sessionHandle_.c_str(), &options),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "[PipeWire] Failed to select sources: " << error->message << "\n";
        g_error_free(error);
        g_object_unref(connection);
        return false;
    }

    g_variant_unref(result);

    // Start the session
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string("libscreenshots_start"));

    result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        "Start",
        g_variant_new("(osa{sv})", sessionHandle_.c_str(), "", &options),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "[PipeWire] Failed to start session: " << error->message << "\n";
        g_error_free(error);
        g_object_unref(connection);
        return false;
    }

    // Parse result to get PipeWire node ID
    GVariant *response_data = nullptr;
    g_variant_get(result, "(u@a{sv})", nullptr, &response_data);

    if (response_data) {
        GVariant *streams_variant = g_variant_lookup_value(response_data, "streams", G_VARIANT_TYPE("a(ua{sv})"));

        if (streams_variant) {
            GVariantIter iter;
            g_variant_iter_init(&iter, streams_variant);

            GVariant *stream_props;
            if (g_variant_iter_next(&iter, "@(ua{sv})", &stream_props)) {
                guint32 node_id;
                GVariant *props_dict;
                g_variant_get(stream_props, "(u@a{sv})", &node_id, &props_dict);

                pipewireNode_ = node_id;

                g_variant_unref(props_dict);
                g_variant_unref(stream_props);
            }

            g_variant_unref(streams_variant);
        }

        g_variant_unref(response_data);
    }

    g_variant_unref(result);
    g_object_unref(connection);

    std::cout << "[PipeWire] Session created, node ID: " << pipewireNode_ << "\n";
    return pipewireNode_ > 0;
}

void ScreenshotPipeWire::onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *param) {
    auto *self = static_cast<ScreenshotPipeWire*>(data);

    if (!param || id != SPA_PARAM_Format) return;

    struct spa_video_info_raw format;
    if (spa_format_video_raw_parse(param, &format) < 0) return;

    self->frameWidth_ = format.size.width;
    self->frameHeight_ = format.size.height;
    self->frameFormat_ = format.format;

    std::cout << "[PipeWire] Format: " << self->frameWidth_ << "x" << self->frameHeight_ << "\n";
}

void ScreenshotPipeWire::onStreamProcess(void *data) {
    auto *self = static_cast<ScreenshotPipeWire*>(data);

    struct pw_buffer *buf = pw_stream_dequeue_buffer(self->stream_);
    if (!buf) return;

    struct spa_buffer *spa_buf = buf->buffer;
    if (!spa_buf || spa_buf->n_datas == 0) {
        pw_stream_queue_buffer(self->stream_, buf);
        return;
    }

    struct spa_data *data_buf = &spa_buf->datas[0];
    if (!data_buf->data) {
        pw_stream_queue_buffer(self->stream_, buf);
        return;
    }

    std::lock_guard<std::mutex> lock(self->frameMutex_);

    size_t size = data_buf->chunk->size;
    self->frameBuffer_.assign(static_cast<uint8_t*>(data_buf->data),
                               static_cast<uint8_t*>(data_buf->data) + size);
    self->frameStride_ = data_buf->chunk->stride;
    self->frameReady_ = true;

    pw_stream_queue_buffer(self->stream_, buf);

    self->frameCv_.notify_one();
}

void ScreenshotPipeWire::onStreamStateChanged(void *data, enum pw_stream_state old,
                                               enum pw_stream_state state, const char *error) {
    auto *self = static_cast<ScreenshotPipeWire*>(data);

    std::cout << "[PipeWire] Stream state: " << pw_stream_state_as_string(state) << "\n";

    if (state == PW_STREAM_STATE_STREAMING) {
        self->streamActive_ = true;
    } else if (state == PW_STREAM_STATE_ERROR) {
        std::cerr << "[PipeWire] Stream error: " << (error ? error : "unknown") << "\n";
        self->streamActive_ = false;
    }
}

void ScreenshotPipeWire::startStream() {
    if (streamActive_) return;

    if (!initializePipeWire()) {
        throw std::runtime_error("[PipeWire] Failed to initialize");
    }

    pw_thread_loop_lock(loop_);

    static const struct pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .param_changed = onStreamParamChanged,
        .process = onStreamProcess,
        .state_changed = onStreamStateChanged,
    };

    struct pw_properties *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Screen",
        nullptr
    );

    stream_ = pw_stream_new(core_, "screenshot-stream", props);

    auto *hook = static_cast<spa_hook*>(calloc(1, sizeof(spa_hook)));
    pw_stream_add_listener(stream_, hook, &stream_events, this);

    uint8_t buffer[1024];
    struct spa_pod_builder pod_builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const struct spa_pod *params[1];
    params[0] = static_cast<const struct spa_pod*>(spa_pod_builder_add_object(&pod_builder,
        SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(3,
            SPA_VIDEO_FORMAT_BGRx,
            SPA_VIDEO_FORMAT_RGBx,
            SPA_VIDEO_FORMAT_RGB)));

    pw_stream_connect(stream_,
                      PW_DIRECTION_INPUT,
                      pipewireNode_,
                      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
                      params, 1);

    pw_thread_loop_unlock(loop_);

    // Wait for stream to become active
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void ScreenshotPipeWire::stopStream() {
    if (!streamActive_) return;

    pw_thread_loop_lock(loop_);
    if (stream_) {
        pw_stream_disconnect(stream_);
        pw_stream_destroy(stream_);
        stream_ = nullptr;
    }
    pw_thread_loop_unlock(loop_);

    streamActive_ = false;
}

ScreenshotResult ScreenshotPipeWire::captureFrame() {
    startStream();

    ScreenshotResult result;

    // Wait for a frame
    std::unique_lock<std::mutex> lock(frameMutex_);
    if (!frameCv_.wait_for(lock, std::chrono::seconds(2), [this] { return frameReady_; })) {
        std::cerr << "[PipeWire] Timeout waiting for frame\n";
        return result;
    }

    // Convert frame buffer to Image
    if (!frameBuffer_.empty() && frameWidth_ > 0 && frameHeight_ > 0) {
        try {
            // Convert BGRx/RGBx to RGB
            int channels = 3;
            std::vector<uint8_t> rgbData(frameWidth_ * frameHeight_ * channels);

            for (int y = 0; y < frameHeight_; ++y) {
                for (int x = 0; x < frameWidth_; ++x) {
                    int src_idx = y * frameStride_ + x * 4;  // 4 bytes per pixel (BGRx)
                    int dst_idx = (y * frameWidth_ + x) * 3;

                    // Assume BGRx format, convert to RGB
                    rgbData[dst_idx + 0] = frameBuffer_[src_idx + 2]; // R
                    rgbData[dst_idx + 1] = frameBuffer_[src_idx + 1]; // G
                    rgbData[dst_idx + 2] = frameBuffer_[src_idx + 0]; // B
                }
            }

            result.image = Image(frameWidth_, frameHeight_, channels, std::move(rgbData));
            result.width = frameWidth_;
            result.height = frameHeight_;
            result.channels = channels;

            frameReady_ = false;
        } catch (const std::exception &e) {
            std::cerr << "[PipeWire] Failed to create image: " << e.what() << "\n";
        }
    }

    return result;
}

ScreenshotResult ScreenshotPipeWire::captureScreen() {
    return captureFrame();
}

ScreenshotResult ScreenshotPipeWire::captureRegion(const int x, const int y, const int width, const int height) {
    const ScreenshotResult full = captureScreen();

    if (!full.image.isValid()) {
        return ScreenshotResult();
    }

    Image cropped = full.image.crop(x, y, width, height);

    ScreenshotResult region;
    region.image = std::move(cropped);
    region.width = region.image.width;
    region.height = region.image.height;
    region.channels = region.image.channels;

    return region;
}

void ScreenshotPipeWire::cleanup() {
    stopStream();

    if (loop_) {
        pw_thread_loop_stop(loop_);
    }

    if (core_) {
        pw_core_disconnect(core_);
        core_ = nullptr;
    }

    if (context_) {
        pw_context_destroy(context_);
        context_ = nullptr;
    }

    if (loop_) {
        pw_thread_loop_destroy(loop_);
        loop_ = nullptr;
    }

    initialized_ = false;
}

#else
// Stub implementation when PipeWire is not available

namespace LibScreenshots {
    ScreenshotPipeWire::ScreenshotPipeWire() {}
    ScreenshotPipeWire::~ScreenshotPipeWire() {}

    ScreenshotPipeWire &ScreenshotPipeWire::getInstance() {
        static ScreenshotPipeWire instance;
        return instance;
    }

    ScreenshotResult ScreenshotPipeWire::captureScreen() {
        throw std::runtime_error("PipeWire support not compiled");
    }

    ScreenshotResult ScreenshotPipeWire::captureRegion(int, int, int, int) {
        throw std::runtime_error("PipeWire support not compiled");
    }
}

#endif