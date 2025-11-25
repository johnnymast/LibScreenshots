#pragma once

#include "LibScreenshots/internals/export.hpp"
#include <LibGraphics/Image.hpp>

namespace LibScreenshots {
    struct LIBSCREENSHOTS_EXPORT ScreenshotResult {
        LibGraphics::Image image;
        int width;
        int height;
        int channels;

        [[nodiscard]] bool isValid() const {
            return image.isValid();
        }
    };
}
