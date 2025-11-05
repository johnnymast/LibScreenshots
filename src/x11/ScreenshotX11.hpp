#pragma once

#include <LibScreenshots/ScreenshotResult.hpp>
#include <LibScreenshots/IScreenshotBackend.hpp>

namespace LibScreenshots {

    class ScreenshotX11 final : public IScreenshotBackend {
    public:
        static ScreenshotX11 &getInstance();
        ScreenshotResult captureScreen() override;
        ScreenshotResult captureRegion(int x, int y, int width, int height)  override;
    };
} // namespace LibScreenshots
