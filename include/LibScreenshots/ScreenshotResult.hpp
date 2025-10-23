#pragma once

#include "LibScreenshots/internals/export.hpp"

#include <vector>

namespace LibScreenshots {
    struct LIBGRAPHICS_API ScreenshotResult {
        std::vector<unsigned char> pixels; // raw RGBA or RGB
        int width;
        int height;
        int channels;
    };
}
