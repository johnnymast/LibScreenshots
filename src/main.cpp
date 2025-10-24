#include "LibScreenshots.hpp"

namespace LibScreenshots {
    ScreenshotResult LIBGRAPHICS_API takes_screenshot() {
        auto platform = CREATE_SCREENSHOT_PLATFORM();
        ScreenshotResult result = platform->captureScreen();
        return result;
    }

    ScreenshotResult LIBGRAPHICS_API takes_screenshot(int x, int y, int width, int height) {
        auto platform = CREATE_SCREENSHOT_PLATFORM();
        ScreenshotResult result = platform->captureRegion(x, y, width, height);
        return result;
    }
}
