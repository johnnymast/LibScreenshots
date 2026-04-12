#pragma once

#include <windows.h>
#include <vector>

#include <LibScreenshots/IScreenshotBackend.hpp>
#include <LibGraphics/Image.hpp>

namespace LibScreenshots {

    class ScreenshotWindows : public IScreenshotBackend {
    public:
        static IScreenshotBackend& getInstance();

        ScreenshotResult captureScreen() override;
        ScreenshotResult captureRegion(int x, int y, int width, int height) override;

    private:
        ScreenshotWindows() = default;
        ~ScreenshotWindows() = default;

        ScreenshotWindows(const ScreenshotWindows&) = delete;
        ScreenshotWindows& operator=(const ScreenshotWindows&) = delete;

        ScreenshotResult captureDIB(int x, int y, int width, int height);
    };

} // namespace LibScreenshots
