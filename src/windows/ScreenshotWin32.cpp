#include "ScreenshotWin32.hpp"

namespace LibScreenshots {

    ScreenshotWin32& ScreenshotWin32::getInstance() {
        static ScreenshotWin32 instance;
        return instance;
    }
}