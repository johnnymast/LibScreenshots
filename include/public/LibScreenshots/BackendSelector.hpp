#pragma once

#include <LibScreenshots/IScreenshotBackend.hpp>

#if HAVE_X11
#include <LibScreenshots/backends/ScreenshotX11.hpp>
#endif

#if HAVE_WLROOTS_SCREEN_COPY
#include <LibScreenshots/backends/ScreenshotWLRoots.hpp>
#endif

#if HAVE_WINDOWS
#include <LibScreenshots/backends/ScreenshotWindows.hpp>
#endif

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
