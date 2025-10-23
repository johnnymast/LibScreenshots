#pragma once

#if defined(WIN32)
#include "windows/ScreenshotWin32.hpp"
#elif defined(WAYLAND)
#include "wayland/ScreenshotWayland.hpp"
#else
#include "x11/ScreenshotX11.hpp"
#endif

