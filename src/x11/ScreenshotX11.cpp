#include "ScreenshotX11.hpp"


#include <opencv2/imgproc.hpp>
#include <opencv2/core/mat.hpp>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>

#include <X11/Xutil.h>
#include <stdexcept>
#include <iostream>
#include <utility>

#include "LibGraphics/utils/Converter.hpp"

using namespace LibScreenshots;
using LibGraphics::Image;
using LibGraphics::Utils::Converter;

std::pair<int, int> GetScreenResolution() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        throw std::runtime_error("Failed to open X11 display");
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    XRRScreenResources* resources = XRRGetScreenResources(display, root);
    if (!resources) {
        XCloseDisplay(display);
        throw std::runtime_error("Failed to get screen resources");
    }

    XRRCrtcInfo* crtcInfo = nullptr;
    std::pair<int, int> resolution;

    if (resources->ncrtc > 0) {
        RRCrtc crtc = resources->crtcs[0];
        crtcInfo = XRRGetCrtcInfo(display, resources, crtc);
        if (!crtcInfo) {
            XRRFreeScreenResources(resources);
            XCloseDisplay(display);
            throw std::runtime_error("Failed to get CRTC info");
        }

        resolution = { static_cast<int>(crtcInfo->width), static_cast<int>(crtcInfo->height) };
        XRRFreeCrtcInfo(crtcInfo);
    } else {
        XRRFreeScreenResources(resources);
        XCloseDisplay(display);
        throw std::runtime_error("No CRTCs found");
    }

    XRRFreeScreenResources(resources);
    XCloseDisplay(display);
    return resolution;
}

ScreenshotResult ScreenshotX11::captureScreen() {
    auto [width, height] = GetScreenResolution();
    int x = 0, y = 0;

    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Width and height must be positive");

    if (x < 0 || y < 0)
        throw std::invalid_argument("Coordinates must be non-negative");

    Display* display = XOpenDisplay(nullptr);
    if (!display)
        throw std::runtime_error("Failed to open X11 display");

    ScreenshotResult resultImage;

    try {
        Window root = XRootWindow(display, XDefaultScreen(display));
        XImage* xImage = XGetImage(display, root, x, y, width, height, AllPlanes, ZPixmap);

        if (!xImage)
            throw std::runtime_error("Failed to capture screenshot");

        int channels = xImage->bits_per_pixel / 8;
        int expected_stride = width * channels;

        cv::Mat resultMat;

        if (xImage->bytes_per_line != expected_stride) {
            std::cerr << "Warning: unexpected stride, copying row-by-row\n";
            resultMat = cv::Mat(height, width, channels == 4 ? CV_8UC4 : CV_8UC3);
            for (int row = 0; row < height; ++row) {
                memcpy(resultMat.ptr(row), xImage->data + row * xImage->bytes_per_line, expected_stride);
            }
        } else {
            resultMat = cv::Mat(height, width, channels == 4 ? CV_8UC4 : CV_8UC3, xImage->data, xImage->bytes_per_line).clone();
        }

        cv::Mat bgrMat;
        if (channels == 4) {
            cv::cvtColor(resultMat, bgrMat, cv::COLOR_BGRA2BGR);
        } else {
            bgrMat = resultMat;
        }

        resultImage.image = Converter::MatToImage(bgrMat);
        resultImage.width = resultImage.image.width;
        resultImage.height = resultImage.image.height;
        resultImage.channels = resultImage.image.channels;

        XDestroyImage(xImage);
        XCloseDisplay(display);
    } catch (...) {
        XCloseDisplay(display);
        throw;
    }

    return resultImage;
}

ScreenshotResult ScreenshotX11::captureRegion(const int x, const int y, const int width, const int height) {
    const ScreenshotResult full = captureScreen();

    Image cropped = full.image.crop(x, y, width, height);

    ScreenshotResult region;
    region.image = std::move(cropped);
    region.width = region.image.width;
    region.height = region.image.height;
    region.channels = region.image.channels;

    return region;
}
