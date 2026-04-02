#pragma once

#pragma once

#include <LibScreenshots/IScreenshotBackend.hpp>
#include <LibScreenshots/backends/ScreenshotX11.hpp>
#include <LibScreenshots/backends/ScreenshotWLRoots.hpp>
#include <LibScreenshots/backends/ScreenshotWindows.hpp>

namespace LibScreenshots {

    inline IScreenshotBackend& Backend()
    {
#if HAVE_WINDOWS
        return ScreenshotWindows::getInstance();
#elif HAVE_X11
        return ScreenshotX11::getInstance();
#elif HAVE_WLROOTS_SCREEN_COPY
        return ScreenshotWLRoots::getInstance();
#else
        static_assert(false, "No screenshot backend selected!");
#endif
    }

}
