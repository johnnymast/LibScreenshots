#include "ScreenshotPipeWire.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

#ifdef HAVE_PIPEWIRE

#include <gio/gio.h>
#include <glib.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <thread>
#include <unistd.h>

using namespace LibScreenshots;
using LibGraphics::Image;

static void onPortalResponse(GDBusConnection *conn, const gchar *sender_name,
                             const gchar *object_path, const gchar *interface_name,
                             const gchar *signal_name, GVariant *parameters, gpointer user_data) {
    guint32 response;
    GVariant *results;

    g_variant_get(parameters, "(u@a{sv})", &response, &results);
    if (response == 0) {
        std::cout << "[Portal] User approved screencast request\n";
    } else {
        std::cerr << "[Portal] User denied or error occurred\n";
    }

    g_variant_unref(results);
}



ScreenshotPipeWire::ScreenshotPipeWire() {
    pw_init(nullptr, nullptr);
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

    stream_ = nullptr;
    frameBuffer_.clear();
    frameWidth_ = 0;
    frameHeight_ = 0;
    frameStride_ = 0;
    frameFormat_ = 0;
    frameReady_ = false;
    streamActive_ = false;
    pipewireFd_ = -1;
    pipewireNode_ = 0;
    sessionHandle_.clear();
    initialized_ = false;
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
    if (!requestScreenCast()) {
        std::cerr << "[PipeWire] Failed to request screencast session\n";
        return false;
    }

    if (!loop_) {
        loop_ = pw_thread_loop_new("screenshot-loop", nullptr);
        if (!loop_) return false;

        context_ = pw_context_new(pw_thread_loop_get_loop(loop_), nullptr, 0);
        if (!context_) return false;

        if (pw_thread_loop_start(loop_) < 0) return false;

        pw_thread_loop_lock(loop_);
        core_ = pw_context_connect(context_, nullptr, 0);
        if (!core_) {
            pw_thread_loop_unlock(loop_);
            return false;
        }
        pw_thread_loop_unlock(loop_);
    }

    initialized_ = true;
    return true;
}

bool ScreenshotPipeWire::requestScreenCast() {
    GError *error = nullptr;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error || !connection) {
        std::cerr << "[PipeWire] ❌ Failed to get DBus connection: " << (error ? error->message : "unknown") << "\n";
        if (error) g_error_free(error);
        return false;
    }

    // Genereer geldige token
    std::string token = "libscreenshots_" + std::to_string(rand() % 100000);
    std::cout << "[PipeWire] 🧷 Using handle_token: " << token << "\n";

    // CreateSession
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string(token.c_str()));
    g_variant_builder_add(&options, "{sv}", "interactive", g_variant_new_boolean(TRUE));  // ✅ Belangrijk
    std::cout << "[PipeWire] before g_variant_new" << "\n";

    GVariant *params = g_variant_new("(a{sv})", g_variant_builder_end(&options));

    std::cout << "[PipeWire] after g_variant_new" << "\n";


    std::cout << "[PipeWire] 📤 Calling CreateSession...\n";
    GVariant *result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        "CreateSession",
        params,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error || !result) {
        std::cerr << "[PipeWire] ❌ CreateSession failed: " << (error ? error->message : "unknown") << "\n";
        if (error) g_error_free(error);
        g_object_unref(connection);
        return false;
    }

    // Extract request path
    GVariant *child = nullptr;
    g_variant_get(result, "(@o)", &child);
    const gchar *request_path = g_variant_get_string(child, nullptr);
    std::cout << "[PipeWire] 📡 Listening for Response on: " << request_path << "\n";
    g_variant_unref(child);
    g_variant_unref(result);

    // Setup main loop and context
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    struct Context {
        bool approved = false;
        GMainLoop *loop;
    };
    auto *context = new Context{false, loop};

    // Listen for Response signal
    g_dbus_connection_signal_subscribe(
        connection,
        "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request",
        "Response",
        request_path,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *, GVariant *parameters, gpointer user_data) {
            auto *ctx = static_cast<Context *>(user_data);
            guint32 response_code;
            GVariant *results = nullptr;
            g_variant_get(parameters, "(u@a{sv})", &response_code, &results);
            ctx->approved = (response_code == 0);
            std::cout << "[PipeWire] 📥 Portal responded with code: " << response_code << "\n";
            g_variant_unref(results);
            g_main_loop_quit(ctx->loop);
        },
        context,
        [](gpointer data) { delete static_cast<Context *>(data); }
    );

    // Wait for user response
    std::cout << "[PipeWire] ⏳ Waiting for user approval...\n";
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    if (!context->approved) {
        std::cerr << "[PipeWire] ❌ User denied screencast request or error occurred\n";
        g_object_unref(connection);
        return false;
    }

    // SelectSources
    GVariantBuilder sourceOptions;
    g_variant_builder_init(&sourceOptions, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&sourceOptions, "{sv}", "types", g_variant_new_uint32(1)); // 1 = monitor
    g_variant_builder_add(&sourceOptions, "{sv}", "multiple", g_variant_new_boolean(FALSE));

    std::cout << "[PipeWire] 📤 Calling SelectSources...\n";
    result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        "SelectSources",
        g_variant_new("(oa{sv})", request_path, g_variant_builder_end(&sourceOptions)),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error || !result) {
        std::cerr << "[PipeWire] ❌ SelectSources failed: " << (error ? error->message : "unknown") << "\n";
        if (error) g_error_free(error);
        g_object_unref(connection);
        return false;
    }
    g_variant_unref(result);
    std::cout << "[PipeWire] ✅ SelectSources succeeded\n";

    // Start
    GVariantBuilder startOptions;
    g_variant_builder_init(&startOptions, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&startOptions, "{sv}", "handle_token", g_variant_new_string(token.c_str()));

    std::cout << "[PipeWire] 📤 Calling Start...\n";
    GUnixFDList *fd_list = nullptr;
    result = g_dbus_connection_call_with_unix_fd_list_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast",
        "Start",
        g_variant_new("(osa{sv})", request_path, "", g_variant_builder_end(&startOptions)),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &fd_list,
        nullptr,
        &error
    );

    if (error || !result || !fd_list) {
        std::cerr << "[PipeWire] ❌ Start failed: " << (error ? error->message : "unknown") << "\n";
        if (error) g_error_free(error);
        g_object_unref(connection);
        return false;
    }

    pipewireFd_ = g_unix_fd_list_get(fd_list, 0, &error);
    if (pipewireFd_ < 0 || error) {
        std::cerr << "[PipeWire] ❌ Failed to extract FD: " << (error ? error->message : "unknown") << "\n";
        if (error) g_error_free(error);
        g_variant_unref(result);
        g_object_unref(connection);
        return false;
    }

    std::cout << "[PipeWire] ✅ PipeWire FD acquired: " << pipewireFd_ << "\n";

    g_variant_unref(result);
    g_object_unref(connection);
    return true;
}

void ScreenshotPipeWire::onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *param) {
    auto *self = static_cast<ScreenshotPipeWire*>(data);
    if (!param || id != SPA_PARAM_Format) return;

    struct spa_video_info_raw format;
    if (spa_format_video_raw_parse(param, &format) < 0) return;

    self->frameWidth_ = format.size.width;
    self->frameHeight_ = format.size.height;
    self->frameFormat_ = format.format;
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
    if (state == PW_STREAM_STATE_STREAMING) {
        self->streamActive_ = true;
    } else if (state == PW_STREAM_STATE_ERROR) {
        self->streamActive_ = false;
    }
}

void ScreenshotPipeWire::startStream() {
    if (streamActive_) return;
    if (!initializePipeWire()) throw std::runtime_error("PipeWire init failed");

    pw_thread_loop_lock(loop_);

    static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        nullptr,
        onStreamStateChanged,
        nullptr,
        nullptr,
        onStreamParamChanged,
        nullptr,
        nullptr,
        onStreamProcess,
        nullptr,
        nullptr,
        nullptr
    };

    struct pw_properties *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Screen",
        nullptr
    );

    pw_properties_set(props, "pipewire.fd", std::to_string(pipewireFd_).c_str());

    stream_ = pw_stream_new(core_, "screenshot-stream", props);
    auto *hook = static_cast<spa_hook*>(calloc(1, sizeof(spa_hook)));
    pw_stream_add_listener(stream_, hook, &stream_events, this);

    pw_stream_connect(stream_,
                      PW_DIRECTION_INPUT,
                      PW_ID_ANY,
                      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
                      nullptr, 0);

    pw_thread_loop_unlock(loop_);
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

    std::unique_lock<std::mutex> lock(frameMutex_);
    if (!frameCv_.wait_for(lock, std::chrono::seconds(2), [this] { return frameReady_; })) {
        std::cerr << "[PipeWire] Timeout waiting for frame\n";
        return result;
    }

    if (!frameBuffer_.empty() && frameWidth_ > 0 && frameHeight_ > 0) {
        try {
            int channels = 3;
            std::vector<uint8_t> rgbData(frameWidth_ * frameHeight_ * channels);

            for (int y = 0; y < frameHeight_; ++y) {
                for (int x = 0; x < frameWidth_; ++x) {
                    int src_idx = y * frameStride_ + x * 4;
                    int dst_idx = (y * frameWidth_ + x) * 3;

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
            std::cerr << "[PipeWire] Failed to convert image: " << e.what() << "\n";
        }
    }

    return result;
}


ScreenshotResult ScreenshotPipeWire::captureScreen() {
    initialized_ = false;
    sessionHandle_.clear();
    pipewireFd_ = -1;
    pipewireNode_ = 0;

    return captureFrame();
}

ScreenshotResult ScreenshotPipeWire::captureRegion(int x, int y, int width, int height) {
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
#endif