#include <LibScreenshots/BackendSelector.hpp>
#include <LibGraphics/Image.hpp>
#include <LibScreenshots/export.hpp>

namespace LibScreenshots {

    LibGraphics::Image LIBSCREENSHOTS_EXPORT TakeScreenshot()
    {
        return Backend().captureScreen().image;
    }

    LibGraphics::Image LIBSCREENSHOTS_EXPORT TakeScreenshot(int x, int y, int width, int height)
    {
        return Backend().captureRegion(x, y, width, height).image;
    }

}
