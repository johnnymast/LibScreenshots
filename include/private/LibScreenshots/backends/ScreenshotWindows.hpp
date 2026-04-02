#pragma once

#if HAVE_WINDOWS

namespace LibScreenshots {
    class ScreenshotWindows {
    public:
        static ScreenshotWindows &getInstance();
    };
}

#endif