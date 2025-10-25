#pragma once

#include "LibScreenshots/internals/export.hpp"

#include <LibGraphics/Image.hpp>

namespace LibScreenshots {
    struct LIBGRAPHICS_API ScreenshotResult {
        LibGraphics::Image image;
        int width;
        int height;
        int channels;
    };
}
