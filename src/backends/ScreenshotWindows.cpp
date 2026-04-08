#include <LibScreenshots/backends/ScreenshotWindows.hpp>

#if HAVE_WINDOWS
#include <windows.h>
#include <vector>

using namespace LibScreenshots;
using std::vector;

namespace LibScreenshots {

    ScreenshotWindows& ScreenshotWindows::getInstance() {
        static ScreenshotWindows instance;
        return instance;
    }

    static LibGraphics::Image HBitmapToImage(const HBITMAP hBitmap) {
        BITMAP bmp;
        GetObject(hBitmap, sizeof(BITMAP), &bmp);

        const int width = bmp.bmWidth;
        const int height = bmp.bmHeight;
        constexpr int channels = 3;

        vector<uint8_t> buffer(width * height * channels);

        BITMAPINFOHEADER bi{};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = -height; // top-down
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;

        HDC hdc = GetDC(nullptr);
        GetDIBits(hdc, hBitmap, 0, height, buffer.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        ReleaseDC(nullptr, hdc);

        return {width, height, channels, std::move(buffer)};
    }

    ScreenshotResult ScreenshotWindows::captureScreen() {
        RECT rc;
        GetClientRect(GetDesktopWindow(), &rc);

        return captureRegion(0, 0, rc.right, rc.bottom);
    }

    ScreenshotResult ScreenshotWindows::captureRegion(int x, int y, int width, int height) {
        ScreenshotResult result;

        HDC hScreen = GetDC(nullptr);
        HDC hMemDC = CreateCompatibleDC(hScreen);

        HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
        HGDIOBJ old = SelectObject(hMemDC, hBitmap);

        BitBlt(hMemDC, 0, 0, width, height, hScreen, x, y, SRCCOPY);

        SelectObject(hMemDC, old);
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreen);

        result.image = HBitmapToImage(hBitmap);
        DeleteObject(hBitmap);

        return result;
    }

}
#endif
