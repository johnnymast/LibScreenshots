#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "modules/stb_image_write.h"
#include "modules/stb_image.h"

#if PLATFORM_WINDOWS
#   pragma message("✅ Compiling with PLATFORM_WINDOWS")
    #include "../../../src/windows/ScreenshotWin32.hpp"
    #define CREATE_SCREENSHOT_PLATFORM() new ScreenshotWin32()
#elif PLATFORM_LINUX
    #if WAYLAND
        #include "../../../src/wayland/ScreenshotWayland.hpp"
        #define CREATE_SCREENSHOT_PLATFORM() new ScreenshotWayland()
    #else
        #include "../../../src/x11/ScreenshotX11.hpp"
        #define CREATE_SCREENSHOT_PLATFORM() new ScreenshotX11()
    #endif

#else
// #   pragma message("✅ OOPS")
    #define CREATE_SCREENSHOT_PLATFORM() nullptr
#endif