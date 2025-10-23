#pragma once

#include "LibScreenshots/ScreenshotResult.hpp"
#include "LibScreenshots/internals/export.hpp"

#include <string>

namespace LibScreenshots {
    class LIBGRAPHICS_API IScreenshotBackend {
    public:
        virtual ~IScreenshotBackend() = default;

        virtual ScreenshotResult captureScreen() = 0;
        virtual ScreenshotResult captureRegion(int x, int y, int width, int height) = 0;
        [[nodiscard]] virtual std::string backendName() const = 0;
    };
}
