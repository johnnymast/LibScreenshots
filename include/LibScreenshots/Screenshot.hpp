#pragma once

#if PLATFORM_WINDOWS
#include "windows/ScreenshotWin32.hpp"
#elif PLATFORM_LINUX

    #if defined(WAYLAND)
    #include "wayland/ScreenshotWayland.hpp"
    #else
    #include "x11/ScreenshotX11.hpp"
    #endif

#endif