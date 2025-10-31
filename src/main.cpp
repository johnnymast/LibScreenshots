#include "LibScreenshots.hpp"
#include "LibScreenshots/internals/platform_dispatch.hpp"

namespace LibScreenshots {
    ScreenshotResult LIBGRAPHICS_API TakeScreenshot() {
        const auto platform = CREATE_SCREENSHOT_PLATFORM();
        ScreenshotResult result = platform->captureScreen();
        return result;
    }

    ScreenshotResult LIBGRAPHICS_API TakeScreenshot(const int x, const int y, const int width, const int height) {
        const auto platform = CREATE_SCREENSHOT_PLATFORM();
        ScreenshotResult result = platform->captureRegion(x, y, width, height);
        return result;
    }
}
