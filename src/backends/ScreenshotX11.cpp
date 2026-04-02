#include "LibScreenshots/backends/ScreenshotX11.hpp"

#if HAVE_X11

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdexcept>
#include <vector>

using namespace LibScreenshots;
using LibGraphics::Image;

namespace {

LibGraphics::Image XImageToImage(XImage* ximg) {
    const int width = ximg->width;
    const int height = ximg->height;
    const int channels = 3;

    std::vector<uint8_t> buffer(width * height * channels);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned long pixel = XGetPixel(ximg, x, y);

            // Aanname: 24/32‑bit TrueColor, standaard masks
            uint8_t r = (pixel & ximg->red_mask)   >> 16;
            uint8_t g = (pixel & ximg->green_mask) >> 8;
            uint8_t b = (pixel & ximg->blue_mask);

            const size_t idx = (y * width + x) * channels;
            buffer[idx + 0] = r;
            buffer[idx + 1] = g;
            buffer[idx + 2] = b;
        }
    }

    return Image(width, height, channels, std::move(buffer));
}

} // namespace

ScreenshotX11& ScreenshotX11::getInstance() {
    static ScreenshotX11 instance;
    return instance;
}

ScreenshotResult ScreenshotX11::captureScreen() {
    Display* display = XOpenDisplay(nullptr);
    if (!display)
        throw std::runtime_error("Failed to open X11 display");

    ScreenshotResult result{};

    try {
        const int screen = DefaultScreen(display);
        Window root = RootWindow(display, screen);

        XWindowAttributes attrs{};
        if (!XGetWindowAttributes(display, root, &attrs))
            throw std::runtime_error("Failed to get root window attributes");

        const int x = 0;
        const int y = 0;
        const int width = attrs.width;
        const int height = attrs.height;

        XImage* ximg = XGetImage(display, root, x, y, width, height, AllPlanes, ZPixmap);
        if (!ximg)
            throw std::runtime_error("Failed to capture screenshot");

        result.image = XImageToImage(ximg);
        result.width = result.image.width;
        result.height = result.image.height;
        result.channels = result.image.channels;

        XDestroyImage(ximg);
        XCloseDisplay(display);
    } catch (...) {
        XCloseDisplay(display);
        throw;
    }

    return result;
}

ScreenshotResult ScreenshotX11::captureRegion(const int x, const int y, const int width, const int height) {
    const ScreenshotResult full = captureScreen();

    Image cropped = full.image.crop(x, y, width, height);

    ScreenshotResult region{};
    region.image = std::move(cropped);
    region.width = region.image.width;
    region.height = region.image.height;
    region.channels = region.image.channels;

    return region;
}

#endif
