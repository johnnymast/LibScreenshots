#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "modules/stb_image_write.h"
#include "modules/stb_image.h"

#if PLATFORM_WINDOWS
#   pragma message("✅ Compiling with PLATFORM_WINDOWS")
    #include "../../../src/windows/ScreenshotWin32.hpp"
    #define Screenshotinstance() LibScreenshots::ScreenshotWindows::getInstance()

#elif PLATFORM_LINUX
    #if WAYLAND
    #   pragma message("✅ Using Wayland DBus Portal backend")
        #include "../../../src/wayland/ScreenshotWayland.hpp"
        #define Screenshotinstance() LibScreenshots::ScreenshotWayland::getInstance()

    #elif defined(HAVE_PIPEWIRE)
    #   pragma message("✅ Using PipeWire backend (works on both X11 and Wayland)")
        #include "../../../src/pipewire/ScreenshotPipeWire.hpp"
        #define Screenshotinstance() LibScreenshots::ScreenshotPipeWire::getInstance()

    #else
    #   pragma message("✅ Using X11 backend")
        #include "../../../src/x11/ScreenshotX11.hpp"
        #define Screenshotinstance() LibScreenshots::ScreenshotX11::getInstance()
    #endif

#else
    #define CREATE_SCREENSHOT_PLATFORM() nullptr
#endif