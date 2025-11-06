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
    g_variant_builder_add(&options, "{sv}", "modal", g_variant_new_boolean(false));
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

    // Stack-allocated result that will be moved at the end
    ScreenshotResult resultData;
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);

    // Simple context with pointers - no ownership transfer
    struct Context {
        ScreenshotResult *result;
        GMainLoop *loop;
        bool completed;
    };

    auto context = new Context{&resultData, loop, false};

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

            // Mark as started to prevent double-quit
            if (context->completed) return;

            guint32 response_code;
            GVariant *results = nullptr;
            g_variant_get(parameters, "(u@a{sv})", &response_code, &results);

            if (results == nullptr || response_code != 0) {
                if (results) g_variant_unref(results);
                context->completed = true;
                g_main_loop_quit(context->loop);
                return;
            }

            GVariant *uri_variant = g_variant_lookup_value(results, "uri", G_VARIANT_TYPE_STRING);
            if (!uri_variant) {
                g_variant_unref(results);
                context->completed = true;
                g_main_loop_quit(context->loop);
                return;
            }

            const gchar *uri = g_variant_get_string(uri_variant, nullptr);
            std::string path = uri ? uri : "";
            if (path.rfind("file://", 0) == 0) path = path.substr(7);

            // Wait for file to exist
            int retries = 10;
            while (retries-- > 0 && !std::filesystem::exists(path)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (std::filesystem::exists(path)) {
                std::ifstream file(path, std::ios::binary);
                if (file) {
                    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), {});
                    try {
                        auto img = Image::load_from_memory(buffer.data(), buffer.size());
                        context->result->image = std::move(img);
                        context->result->width = context->result->image.width;
                        context->result->height = context->result->image.height;
                        context->result->channels = context->result->image.channels;
                    } catch (const std::exception &e) {
                        std::cerr << "[ScreenshotWayland] ❌ Failed to decode image: " << e.what() << "\n";
                    }
                }
            }

            g_variant_unref(uri_variant);
            g_variant_unref(results);

            context->completed = true;
            g_main_loop_quit(context->loop);
        },
        context,
        [](gpointer data) {
            // Only free the context struct itself, not the pointed-to data
            auto *context = static_cast<Context *>(data);
            delete context;
        }
    );

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    g_variant_unref(child);
    g_variant_unref(result);

    return resultData;
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