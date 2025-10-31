#pragma once

#include "LibScreenshots/internals/export.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"

using LibScreenshots::ScreenshotResult;

namespace LibScreenshots {
    ScreenshotResult LIBSCREENSHOTS_EXPORT TakeScreenshot();
    ScreenshotResult LIBSCREENSHOTS_EXPORT TakeScreenshot(int x, int y, int width, int height);
}