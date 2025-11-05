#include "LibScreenshots.hpp"
#include "LibScreenshots/internals/platform_dispatch.hpp"

namespace LibScreenshots {
    ScreenshotResult LIBSCREENSHOTS_EXPORT TakeScreenshot() {
        auto instance = Screenshotinstance();
        ScreenshotResult result = instance.captureScreen();
        return result;
    }

    ScreenshotResult LIBSCREENSHOTS_EXPORT TakeScreenshot(const int x, const int y, const int width, const int height) {
        auto instance = Screenshotinstance();
        ScreenshotResult result = instance.captureRegion(x, y, width, height);
        return result;
    }
}
