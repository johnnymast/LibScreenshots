#include "LibScreenshots/LibScreenshots.hpp"
#include "LibScreenshots/IScreenshotBackend.hpp"
#include "LibScreenshots/BackendSelector.hpp"

namespace LibScreenshots {
    ScreenshotResult LIBSCREENSHOTS_EXPORT TakeScreenshot() {
        auto& instance = Backend();  // Use reference, not copy
        ScreenshotResult result = instance.captureScreen();
        return result;
    }

    ScreenshotResult LIBSCREENSHOTS_EXPORT TakeScreenshot(const int x, const int y, const int width, const int height) {
        auto& instance = Backend();  // Use reference, not copy
        ScreenshotResult result = instance.captureRegion(x, y, width, height);
        return result;
    }
}
