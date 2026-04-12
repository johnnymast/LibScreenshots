#pragma once
#include <LibGraphics/Image.hpp>
#include <LibScreenshots/exceptions/ScreenshotException.hpp>

namespace LibScreenshots {
    LibGraphics::Image TakeScreenshot();
    LibGraphics::Image TakeScreenshot(int x, int y, int width, int height);
}