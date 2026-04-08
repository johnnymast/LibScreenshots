#pragma once
#include <LibGraphics/Image.hpp>

namespace LibScreenshots {
    struct ScreenshotResult {
        LibGraphics::Image image;
        int width{};
        int height{};
        int channels{};

        [[nodiscard]] bool isValid() const {
            return image.isValid();
        }
    };
}
