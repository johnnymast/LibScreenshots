#pragma once

#include "LibScreenshots/internals/export.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

using LibScreenshots::ScreenshotResult;

namespace LibScreenshots {
    ScreenshotResult LIBGRAPHICS_API TakeScreenshot();
    ScreenshotResult LIBGRAPHICS_API TakeScreenshot(int x, int y, int width, int height);
}