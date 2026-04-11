#pragma once

#include <LibScreenshots/IScreenshotBackend.hpp>
#include <LibScreenshots/ScreenshotResult.hpp>

namespace LibScreenshots {

    class ScreenshotWindows final : public IScreenshotBackend {
    public:
        static ScreenshotWindows& getInstance();

        ScreenshotResult captureScreen() override;
        ScreenshotResult captureRegion(int x, int y, int width, int height) override;
    };

}
