#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "ScreenshotWayland.hpp"
#include "modules/stb_image_write.h"
#include "modules/stb_image.h"

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;
using namespace LibScreenshots;

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
    if (error) throw std::runtime_error(error->message);

    // Extract request path
    GVariant *child = nullptr;
    g_variant_get(result, "(@o)", &child);
    const gchar *request_path = g_variant_get_string(child, nullptr);

    // Set up result container and main loop
    ScreenshotResult resultData;
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);

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
            auto *context = static_cast<std::pair<ScreenshotResult *, GMainLoop *> *>(user_data);

            guint32 response_code;
            GVariant *results;
            g_variant_get(parameters, "(u@a{sv})", &response_code, &results);

            if (response_code != 0) {
                g_main_loop_quit(context->second);
                return;
            }

            GVariant *uri_variant = g_variant_lookup_value(results, "uri", G_VARIANT_TYPE_STRING);
            const gchar *uri = g_variant_get_string(uri_variant, nullptr);
            std::string path = uri;
            if (path.rfind("file://", 0) == 0) path = path.substr(7);

            int retries = 10;
            while (retries-- > 0 && !fs::exists(path)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (!fs::exists(path)) {
                g_main_loop_quit(context->second);
                return;
            }

            std::ifstream file(path, std::ios::binary);
            if (!file) {
                g_main_loop_quit(context->second);
                return;
            }

            std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), {});
            context->first->pixels = std::move(buffer);
            context->first->width = 0;
            context->first->height = 0;
            context->first->channels = 0;

            g_main_loop_quit(context->second);
        },
        new std::pair<ScreenshotResult *, GMainLoop *>(&resultData, loop),
        [](gpointer data) {
            delete static_cast<std::pair<ScreenshotResult *, GMainLoop *> *>(data);
        }
    );

    g_main_loop_run(loop);
    g_main_loop_unref(loop);
    g_variant_unref(child);

    return resultData;
}

ScreenshotResult ScreenshotWayland::captureRegion(int x, int y, int width, int height) {
    ScreenshotResult full = captureScreen();

    int w, h, channels;
    unsigned char *decoded = stbi_load_from_memory(
        full.pixels.data(), full.pixels.size(), &w, &h, &channels, 0
    );
    if (!decoded) throw std::runtime_error("Failed to decode PNG");

    std::vector<unsigned char> cropped;
    for (int row = y; row < y + height; ++row) {
        for (int col = x; col < x + width; ++col) {
            int src_index = (row * w + col) * channels;
            cropped.insert(cropped.end(), decoded + src_index, decoded + src_index + channels);
        }
    }
    //
    // stbi_write_png("/tmp/johnny-region.png", width, height, channels, cropped.data(), width * channels);


    stbi_image_free(decoded);

    ScreenshotResult region;
    region.pixels = std::move(cropped);
    region.width = width;
    region.height = height;
    region.channels = channels;
    return region;
}

std::string ScreenshotWayland::backendName() const {
    return "wayland";
}
