#pragma once

#include <gio/gio.h>
#include <string>
#include <LibScreenshots/ScreenshotResult.hpp>
#include <LibScreenshots/IScreenshotBackend.hpp>

namespace LibScreenshots {

    class ScreenshotWayland : public IScreenshotBackend {
    public:
        // ScreenshotWayland();
        ScreenshotResult captureScreen() override;
        ScreenshotResult captureRegion(int x, int y, int width, int height)  override;
        // std::string backendName() const override;
    };

} // namespace LibScreenshots
