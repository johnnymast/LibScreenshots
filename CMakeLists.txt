cmake_minimum_required(VERSION 3.16)
project(LibScreenshots)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


# Optional Wayland toggle
option(WAYLAND "Enable Wayland support via DBus portal" ON)

# Core library
add_library(${PROJECT_NAME} SHARED
        src/main.cpp


        include/LibScreenshots/internals/platform_dispatch.hpp
        include/LibScreenshots/internals/export.hpp

        include/LibScreenshots/IScreenshotBackend.hpp
        include/LibScreenshots/ScreenshotResult.hpp

        src/windows/ScreenshotWin32.hpp
        src/windows/ScreenshotWin32.cpp

        src/wayland/ScreenshotWayland.hpp
        src/wayland/ScreenshotWayland.cpp

        src/x11/ScreenshotX11.hpp
        src/x11/ScreenshotX11.cpp

        include/LibScreenshots.hpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../Graphics/include
)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_LINUX)
endif()

include(Graphics)
include(Windows)
include(Wayland)
include(X11)


