#pragma once

#if HAVE_WINDOWS

#include <LibScreenshots/IScreenshotBackend.hpp>

namespace LibScreenshots {
    class ScreenshotWindows : public IScreenshotBackend {
    public:
        static ScreenshotWindows &getInstance();

        ScreenshotResult captureScreen() override;

        ScreenshotResult captureRegion(int x, int y, int width, int height) override;

    private:
        ScreenshotWindows() = default;
    };
}

#endif
