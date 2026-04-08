#pragma once

#include "LibScreenshots/ScreenshotResult.hpp"

namespace LibScreenshots {
    class IScreenshotBackend {
    public:
        virtual ~IScreenshotBackend() = default;

        virtual ScreenshotResult captureScreen() = 0;
        virtual ScreenshotResult captureRegion(int x, int y, int width, int height) = 0;
    };
};
