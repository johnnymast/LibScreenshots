#include "LibScreenshots/backends/ScreenshotWindows.hpp"

namespace LibScreenshots {

    ScreenshotWindows& ScreenshotWindows::getInstance() {
        static ScreenshotWin32 instance;
        return instance;
    }
}