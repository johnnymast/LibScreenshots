#include "ScreenshotWayland.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

#if HAVE_WAYLAND
#include <glib.h>
#include <gio/gio.h>

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdlib>   // for std::system
#include <vector>

namespace fs = std::filesystem;
using namespace LibScreenshots;
using LibGraphics::Image;

ScreenshotWayland &ScreenshotWayland::getInstance() {
    static ScreenshotWayland instance;
    return instance;
}

// Helper: run external command and return path
static std::string run_command(const std::string &cmd, const std::string &outfile) {
    int ret = std::system(cmd.c_str());
    if (ret == 0 && fs::exists(outfile)) {
        return outfile;
    }
    return {};
}

ScreenshotResult ScreenshotWayland::captureScreen() {
    ScreenshotResult resultData;

    // ============================================================
    // BEGIN portal main method (via org.freedesktop.portal.Screenshot)
    // ============================================================
    GError *error = nullptr;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (error) throw std::runtime_error(error->message);

    GDBusProxy *proxy = g_dbus_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
        nullptr,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Screenshot",
        nullptr,
        &error
    );

    if (proxy) {
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&options, "{sv}", "interactive", g_variant_new_boolean(true));
        g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string("libscreenshots"));

        GVariant *parameters = g_variant_new("(sa{sv})", "", &options);

        GVariant *result = g_dbus_connection_call_sync(
            connection,
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Screenshot",
            "Screenshot",
            parameters,
            G_VARIANT_TYPE("(o)"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            &error
        );
        if (error) throw std::runtime_error(error->message);

        const gchar *request_path;
        g_variant_get(result, "(o)", &request_path);

        GMainLoop *loop = g_main_loop_new(nullptr, FALSE);

        struct Context {
            ScreenshotResult *result;
            GMainLoop *loop;
            bool completed;
        };
        auto context = new Context{&resultData, loop, false};

        guint subscription = g_dbus_connection_signal_subscribe(
            connection,
            "org.freedesktop.portal.Desktop",
            "org.freedesktop.portal.Request",
            "Response",
            request_path,
            nullptr,
            G_DBUS_SIGNAL_FLAGS_NONE,
            [](GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *,
               GVariant *parameters, gpointer user_data) {
                auto *context = static_cast<Context *>(user_data);
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

                int retries = 50;
                while (retries-- > 0 && !fs::exists(path)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                if (fs::exists(path)) {
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
                auto *context = static_cast<Context *>(data);
                delete context;
            }
        );

        g_main_loop_run(loop);
        g_dbus_connection_signal_unsubscribe(connection, subscription);
        g_main_loop_unref(loop);
        g_variant_unref(result);
        g_object_unref(proxy);

        if (resultData.image.isValid()) {
            return resultData;
        }
    }
    // ============================================================
    // END portal main method
    // ============================================================

    // ============================================================
    // BEGIN fallback: Spectacle
    // ============================================================
    std::string tmpfile = "/tmp/libscreenshot.png";
    std::string path;

    std::cout << "Trying Spectacle: " << "spectacle -b -n -a -o " + tmpfile << std::endl;
    path = run_command("spectacle -b -n -a -o " + tmpfile, tmpfile);
    if (!path.empty()) {
        std::ifstream file(path, std::ios::binary);
        std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), {});
        auto img = Image::load_from_memory(buffer.data(), buffer.size());
        resultData.image = std::move(img);
        resultData.width = resultData.image.width;
        resultData.height = resultData.image.height;
        resultData.channels = resultData.image.channels;
        return resultData;
    }
    // ============================================================
    // END fallback: Spectacle
    // ============================================================

    throw std::runtime_error("No screenshot backend available on this system");
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
