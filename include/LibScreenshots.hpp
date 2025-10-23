#pragma once

#include "LibScreenshots/internals/export.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

namespace LibScreenshots {
    ScreenshotResult LIBGRAPHICS_API takes_screenshot(int x, int y, int width, int height);
}