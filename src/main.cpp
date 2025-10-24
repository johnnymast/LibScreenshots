#if defined(PLATFORM_WINDOWS)
// #include "windows/ScreenshotWin32.hpp"
#elif defined(PLATFORM_WAYLAND)
#include "wayland/ScreenshotWayland.hpp"
#else
// #include "x11/ScreenshotX11.hpp"
#endif

#include <fstream>
#include <iostream>
//
// using namespace LibScreenshots;



namespace LibScreenshots {
    ScreenshotResult LIBGRAPHICS_API takes_screenshot(int x, int y, int width, int height) {
#if defined(PLATFORM_WINDOWS)
        // #include "windows/ScreenshotWin32.hpp"
#elif defined(PLATFORM_WAYLAND)
        auto platform = new ScreenshotWayland();
#else

#endif
        ScreenshotResult result = platform->captureRegion(x, y, width, height);

        // Write raw bytes to /tmp/johnny
        std::ofstream out("/tmp/johnny.png", std::ios::binary);
        out.write(reinterpret_cast<const char*>(result.pixels.data()), result.pixels.size());
        out.close();

        return result;
    }
}
