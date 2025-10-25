#include "ScreenshotX11.hpp"


#include <opencv2/imgproc.hpp>
#include <opencv2/core/mat.hpp>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>

#include <X11/Xutil.h>
#include <stdexcept>
#include <iostream>
#include <utility>

using namespace LibScreenshots;
using LibGraphics::Image;

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
    int x, y = 0;

    // Display *display = XOpenDisplay(nullptr);
    //
    // if (!display) {
    //     fprintf(stderr, "Unable to open display\n");
    //     exit(EXIT_FAILURE);
    // }
    //
    // Window root = DefaultRootWindow(display);
    // XImage *image = XGetImage(display, root, x, y, width, height, AllPlanes, ZPixmap);
    //
    // if (!image) {
    //     XCloseDisplay(display);
    //     throw std::runtime_error("Failed to capture image");
    // }

    printf("Screen resolution %dx%d\n", width, height);






    //
    // cv::Mat mat = cv::Mat(height, width, CV_8UC4, image->data);
    // cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
    //
    // XDestroyImage(image);
    // XCloseDisplay(display);

    return ScreenshotResult();
}

ScreenshotResult ScreenshotX11::captureRegion(const int x, const int y, const int width, const int height) {
    // Display *display = XOpenDisplay(nullptr);
    // if (display == nullptr) {
    //     fprintf(stderr, "Unable to open display\n");
    //     exit(EXIT_FAILURE);
    // }
    // Window root = DefaultRootWindow(display);
    // XImage *image = XGetImage(display, root, x, y, width, height, AllPlanes, ZPixmap);
    //
    // if (image == nullptr) {
    //     XCloseDisplay(display);
    //     throw std::runtime_error("Failed to capture image");
    // }
    //
    // cv::Mat mat = cv::Mat(height, width, CV_8UC4, image->data);
    // cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
    //
    // XDestroyImage(image);

    captureScreen();
    return ScreenshotResult();

}
