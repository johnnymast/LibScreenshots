#include <LibScreenshots/backends/ScreenshotWindows.hpp>
#include <LibScreenshots/exceptions/ScreenshotException.hpp>

using LibScreenshots::Exceptions::ScreenshotException;

namespace LibScreenshots {
    static void ThrowError(const char *msg) {
        throw ScreenshotException(msg, "ScreenshotWindows");
    }

    IScreenshotBackend &ScreenshotWindows::getInstance() {
        static ScreenshotWindows instance;
        return instance;
    }

    ScreenshotResult ScreenshotWindows::captureScreen() {
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);

        if (w <= 0 || h <= 0)
            ThrowError("GetSystemMetrics failed");

        return captureDIB(0, 0, w, h);
    }

    ScreenshotResult ScreenshotWindows::captureRegion(int x, int y, int width, int height) {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        if (screenW <= 0 || screenH <= 0)
            ThrowError("GetSystemMetrics failed");

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + width > screenW) width = screenW - x;
        if (y + height > screenH) height = screenH - y;

        if (width <= 0 || height <= 0)
            ThrowError("Requested region invalid");

        return captureDIB(x, y, width, height);
    }

    ScreenshotResult ScreenshotWindows::captureDIB(int x, int y, int width, int height) {
        HDC screenDC = GetDC(nullptr);
        if (!screenDC)
            ThrowError("GetDC failed");

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void *dibPixels = nullptr;

        HBITMAP dib = CreateDIBSection(
            screenDC,
            &bmi,
            DIB_RGB_COLORS,
            &dibPixels,
            nullptr,
            0
        );

        if (!dib || !dibPixels) {
            ReleaseDC(nullptr, screenDC);
            ThrowError("CreateDIBSection failed");
        }

        HDC memDC = CreateCompatibleDC(screenDC);
        if (!memDC) {
            DeleteObject(dib);
            ReleaseDC(nullptr, screenDC);
            ThrowError("CreateCompatibleDC failed");
        }

        HGDIOBJ old = SelectObject(memDC, dib);
        if (!old) {
            DeleteDC(memDC);
            DeleteObject(dib);
            ReleaseDC(nullptr, screenDC);
            ThrowError("SelectObject failed");
        }

        if (!BitBlt(memDC, 0, 0, width, height, screenDC, x, y, SRCCOPY | CAPTUREBLT)) {
            SelectObject(memDC, old);
            DeleteDC(memDC);
            DeleteObject(dib);
            ReleaseDC(nullptr, screenDC);
            ThrowError("BitBlt failed");
        }

        // Pixelbuffer kopiëren
        size_t totalBytes = static_cast<size_t>(width) * height * 4;
        std::vector<uint8_t> pixels(totalBytes);
        memcpy(pixels.data(), dibPixels, totalBytes);

        // Cleanup
        SelectObject(memDC, old);
        DeleteDC(memDC);
        DeleteObject(dib);
        ReleaseDC(nullptr, screenDC);

        // BGRA → RGBA (GDI levert altijd BGRA)
        for (size_t i = 0; i < totalBytes; i += 4) {
            std::swap(pixels[i + 0], pixels[i + 2]); // B ↔ R
        }

        ScreenshotResult result;
        result.width = width;
        result.height = height;
        result.channels = 4;
        result.image = LibGraphics::Image(width, height, 4, std::move(pixels));

        return result;
    }
}
