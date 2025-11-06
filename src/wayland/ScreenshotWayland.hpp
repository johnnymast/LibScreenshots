#pragma once

#include <LibScreenshots/ScreenshotResult.hpp>
#include <LibScreenshots/IScreenshotBackend.hpp>

namespace LibScreenshots {

    class ScreenshotWayland : public IScreenshotBackend {
    public:
        static ScreenshotWayland &getInstance();

        ScreenshotResult captureScreen() override;
        ScreenshotResult captureRegion(int x, int y, int width, int height)  override;
    };
} // namespace LibScreenshots
