#pragma once

#if HAVE_WINDOW
#include "windows/ScreenshotWin32.hpp"
#else

    #if HAVE_WAYLAND
        #include "wayland/ScreenshotWayland.hpp"
    #endif

    #if HAVE_X11
        #include "x11/ScreenshotX11.hpp"
    #endif

    #if HAVE_PIPEWIRE
        #include "pipewire/ScreenshotPipeWire.hpp"
    #endif

#endif