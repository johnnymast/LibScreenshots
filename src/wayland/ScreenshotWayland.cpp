#include "ScreenshotWayland.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

#if WAYLAND
#include <glib.h>
#include <gio/gio.h>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

#include <memory> // for std::unique_ptr if needed

namespace fs = std::filesystem;
using namespace LibScreenshots;
using LibGraphics::Image;

ScreenshotWayland &ScreenshotWayland::getInstance() {
    static ScreenshotWayland instance;
    return instance;
}

ScreenshotResult ScreenshotWayland::captureScreen() {
    GError *error = nullptr;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error) throw std::runtime_error(error->message);

    // Prepare options
    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "interactive", g_variant_new_boolean(false));
    g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string("libscreenshots"));

    GVariant *parameters = g_variant_new("(sa{sv})", "", &options);

    // Call Screenshot method
    GVariant *result = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Screenshot",
        "Screenshot",
        parameters,
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );
    if (error)
        std::cout << "ScreenshotWayland: Error detected " << "\n";


    std::cout << "ScreenshotWayland: " << g_variant_print(result, true) << "\n";
    if (error) throw std::runtime_error(error->message);

    // Extract request path
    GVariant *child = nullptr;
    g_variant_get(result, "(@o)", &child);
    const gchar *request_path = g_variant_get_string(child, nullptr);

    // ---- Lifetime-safe context-based approach ----
    // Use a heap-allocated result and a heap-allocated context to avoid
    // stack lifetime issues with callbacks.
    struct Context {
        ScreenshotResult* first;
        GMainLoop* second;
    };

    ScreenshotResult* resultData = new ScreenshotResult();
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);

    auto context = new Context{ resultData, loop };

    // Internal helper to transfer image data safely
    // This keeps the existing API where we fill context->first->image and metadata.
    g_dbus_connection_signal_subscribe(
        connection,
        "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request",
        "Response",
        request_path,
        nullptr,
        G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *, GVariant *parameters,
           gpointer user_data) {
            auto *context = static_cast<Context *>(user_data);

            guint32 response_code;
            GVariant *results;
            g_variant_get(parameters, "(u@a{sv})", &response_code, &results);

            if (response_code != 0) {
                if (context && context->second && g_main_loop_is_running(context->second)) {
                    g_main_loop_quit(context->second);
                }
                return;
            }

            GVariant *uri_variant = g_variant_lookup_value(results, "uri", G_VARIANT_TYPE_STRING);
            const gchar *uri = g_variant_get_string(uri_variant, nullptr);
            std::string path = uri;
            if (path.rfind("file://", 0) == 0) path = path.substr(7);

            int retries = 10;
            while (retries-- > 0 && !std::filesystem::exists(path)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (!std::filesystem::exists(path)) {
                if (context && context->second && g_main_loop_is_running(context->second)) {
                    g_main_loop_quit(context->second);
                }
                return;
            }

            std::ifstream file(path, std::ios::binary);
            if (!file) {
                if (context && context->second && g_main_loop_is_running(context->second)) {
                    g_main_loop_quit(context->second);
                }
                return;
            }

            std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), {});

            try {
                // Patch: load via data pointer and size
                auto img = Image::load_from_memory(buffer.data(), buffer.size());
                context->first->image = std::move(img);
                // Also update width/height/channels from loaded image
                context->first->width = context->first->image.width;
                context->first->height = context->first->image.height;
                context->first->channels = context->first->image.channels;

                // legacy form (kept for reference, not used):
                // context->first->image = Image::load_from_memory(buffer);
            } catch (const std::exception &e) {
                std::cerr << "[ScreenshotWayland] ❌ Failed to decode image: " << e.what() << "\n";
            }

            if (context && context->second && g_main_loop_is_running(context->second)) {
                g_main_loop_quit(context->second);
            }
        },
        context, // pass our context
        [](const gpointer data) {
            // destroy_notify: free the context and the result
            auto *context = static_cast<Context*>(data);
            delete context->first;
            delete context;
        }
    );

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    g_variant_unref(child);

    // Move the result out and free the heap-allocated storage
    ScreenshotResult final = std::move(*resultData);
    delete resultData; // frees the relocated data

    return final;
}

ScreenshotResult ScreenshotWayland::captureRegion(const int x, const int y, const int width, const int height) {
    const ScreenshotResult full = captureScreen();

    Image cropped = full.image.crop(x, y, width, height);

    ScreenshotResult region;
    region.image = std::move(cropped);
    region.width = region.image.width;
    region.height = region.image.height;
    region.channels = region.image.channels;

    return region;
}
#endif
