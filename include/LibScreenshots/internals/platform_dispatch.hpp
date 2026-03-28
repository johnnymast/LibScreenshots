#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "modules/stb_image_write.h"
#include "modules/stb_image.h"

#if HAVE_WINDOWS
#pragma message("Compiling for Windows")
#include "../../../src/windows/ScreenshotWin32.hpp"
#define Screenshotinstance() LibScreenshots::ScreenshotWindows::getInstance()
#endif

#if HAVE_WAYLAND
#pragma message("Compiling for Wayland with Dbus")
#include "../../../src/wayland/ScreenshotWayland.hpp"
#define Screenshotinstance() LibScreenshots::ScreenshotWayland::getInstance()
#endif

#if HAVE_X11
#pragma message("Compiling for X11")
#include "../../../src/x11/ScreenshotX11.hpp"
#define Screenshotinstance() LibScreenshots::ScreenshotX11::getInstance()
#endif


#if HAVE_PIPEWIRE
#pragma message("Compiling for Pipewire with Dbus")
#include "../../../src/pipewire/ScreenshotPipeWire.hpp"
#define Screenshotinstance() LibScreenshots::ScreenshotPipeWire::getInstance()
#endif
