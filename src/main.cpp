#include <LibScreenshots/BackendSelector.hpp>
#include <LibGraphics/Image.hpp>
#include <LibScreenshots/export.hpp>

namespace LibScreenshots {
    LIBSCREENSHOTS_EXPORT LibGraphics::Image TakeScreenshot() {
        return Backend().captureScreen().image;
    }

    LIBSCREENSHOTS_EXPORT LibGraphics::Image TakeScreenshot(int x, int y, int width, int height) {
        return Backend().captureRegion(x, y, width, height).image;
    }
}
