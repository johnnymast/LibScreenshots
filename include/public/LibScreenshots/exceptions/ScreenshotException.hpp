#pragma once
#include <stdexcept>
#include <string>

namespace LibScreenshots::Exceptions {

    class ScreenshotException : public std::runtime_error {
    public:
        explicit ScreenshotException(std::string msg, std::string module)
            : std::runtime_error(msg), mod(std::move(module)) {}

        const char* module() const noexcept {
            return mod.c_str();
        }

    private:
        std::string mod;
    };
}
