#pragma once

#include "LibScreenshots/internals/platform_dispatch.hpp"
#include "LibScreenshots/internals/export.hpp"
#include "LibScreenshots/ScreenshotResult.hpp"


using LibScreenshots::ScreenshotResult;

namespace LibScreenshots {
    ScreenshotResult LIBGRAPHICS_API takes_screenshot();
    ScreenshotResult LIBGRAPHICS_API takes_screenshot(int x, int y, int width, int height);
}