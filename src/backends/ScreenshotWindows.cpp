#include <LibScreenshots/backends/ScreenshotWindows.hpp>

#if HAVE_WINDOWS
#include <windows.h>
#include <vector>
#include <stdexcept>

using namespace LibScreenshots;

namespace {
    void check(bool ok, const char* msg) {
        if (!ok) {
            throw std::runtime_error(msg);
        }
    }

    void getScreenResolution(int& width, int& height) {
        HDC desktopDC = GetDC(nullptr);
        if (!desktopDC)
            throw std::runtime_error("Failed to get DC for screen resolution");

        width = GetDeviceCaps(desktopDC, HORZRES);
        height = GetDeviceCaps(desktopDC, VERTRES);

        ReleaseDC(nullptr, desktopDC);

        if (width <= 0 || height <= 0)
            throw std::runtime_error("Failed to get valid screen dimensions");
    }

    LibGraphics::Image captureRegionInternal(int x, int y, int width, int height) {
        if (width <= 0 || height <= 0)
            throw std::runtime_error("Width and height must be positive");
        if (x < 0 || y < 0)
            throw std::runtime_error("Coordinates must be non-negative");

        int screenW, screenH;
        getScreenResolution(screenW, screenH);
        if (x + width > screenW || y + height > screenH)
            throw std::runtime_error("Capture region is outside screen bounds");

        HDC desktopDC = GetDC(nullptr);
        if (!desktopDC)
            throw std::runtime_error("Failed to get DC");

        HDC memDC = nullptr;
        HBITMAP hBitmap = nullptr;
        HGDIOBJ oldBitmap = nullptr;

        LibGraphics::Image result;

        try {
            memDC = CreateCompatibleDC(desktopDC);
            check(memDC != nullptr, "Failed to create compatible DC");

            hBitmap = CreateCompatibleBitmap(desktopDC, width, height);
            check(hBitmap != nullptr, "Failed to create compatible bitmap");

            oldBitmap = SelectObject(memDC, hBitmap);
            check(oldBitmap != nullptr, "Failed to select bitmap into DC");

            int rop = SRCCOPY | CAPTUREBLT;
            check(BitBlt(memDC, 0, 0, width, height, desktopDC, x, y, rop) != 0, "BitBlt failed");

            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            std::vector<uint8_t> bgra(width * height * 4);
            int scanLines = GetDIBits(
                memDC,
                hBitmap,
                0,
                height,
                bgra.data(),
                &bmi,
                DIB_RGB_COLORS
            );
            check(scanLines == height, "GetDIBits failed");

            const int channels = 3;
            std::vector<uint8_t> rgb(width * height * channels);

            for (int i = 0; i < width * height; ++i) {
                uint8_t b = bgra[i * 4 + 0];
                uint8_t g = bgra[i * 4 + 1];
                uint8_t r = bgra[i * 4 + 2];
                rgb[i * 3 + 0] = r;
                rgb[i * 3 + 1] = g;
                rgb[i * 3 + 2] = b;
            }

            result = LibGraphics::Image(width, height, channels, std::move(rgb));
        } catch (...) {
            if (oldBitmap)
                SelectObject(memDC, oldBitmap);
            if (hBitmap)
                DeleteObject(hBitmap);
            if (memDC)
                DeleteDC(memDC);
            if (desktopDC)
                ReleaseDC(nullptr, desktopDC);
            throw;
        }

        if (oldBitmap)
            SelectObject(memDC, oldBitmap);
        if (hBitmap)
            DeleteObject(hBitmap);
        if (memDC)
            DeleteDC(memDC);
        if (desktopDC)
            ReleaseDC(nullptr, desktopDC);

        return result;
    }
}

namespace LibScreenshots {

    ScreenshotWindows& ScreenshotWindows::getInstance() {
        static ScreenshotWindows instance;
        return instance;
    }

    ScreenshotResult ScreenshotWindows::captureScreen() {
        int w, h;
        getScreenResolution(w, h);

        ScreenshotResult result;
        result.image = captureRegionInternal(0, 0, w, h);
        return result;
    }

    ScreenshotResult ScreenshotWindows::captureRegion(int x, int y, int width, int height) {
        ScreenshotResult result;
        result.image = captureRegionInternal(x, y, width, height);
        return result;
    }
}
#endif
