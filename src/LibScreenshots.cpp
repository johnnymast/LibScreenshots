#include <LibScreenshots/BackendSelector.hpp>
#include <LibGraphics/Image.hpp>
#include <LibScreenshots/export.hpp>

namespace LibScreenshots {

    LIBSCREENSHOTS_API LibGraphics::Image TakeScreenshot() {
        return Backend().captureScreen().image;
    }

    LIBSCREENSHOTS_API LibGraphics::Image TakeScreenshot(int x, int y, int w, int h) {
        return Backend().captureRegion(x, y, w, h).image;
    }
}
