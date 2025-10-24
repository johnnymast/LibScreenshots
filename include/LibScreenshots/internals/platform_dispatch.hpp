#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "modules/stb_image_write.h"
#include "modules/stb_image.h"

#if defined(PLATFORM_WINDOWS)
    #include "windows/ScreenshotWin32.hpp"
    #define CREATE_SCREENSHOT_PLATFORM() new ScreenshotWin32()
#elif defined(PLATFORM_WAYLAND)
    #include "../../../src//wayland/ScreenshotWayland.hpp"
    #define CREATE_SCREENSHOT_PLATFORM() new ScreenshotWayland()
#elif defined(PLATFORM_X11)
    #include "x11/ScreenshotX11.hpp"
    #define CREATE_SCREENSHOT_PLATFORM() new ScreenshotX11()
#else
    #define CREATE_SCREENSHOT_PLATFORM() nullptr
#endif