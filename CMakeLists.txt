cmake_minimum_required(VERSION 3.16)
project(LibScreenshots)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(X11_SCREENSHOTS      "Build for X11" ON)

if(NOT (WINDOWS_SCREENSHOTS OR X11_SCREENSHOTS OR WAYLAND_SCREENSHOTS OR PIPEWIRE_SCREENSHOTS))
    message(FATAL_ERROR
            "LibScreenshots: No backend selected! Enable at least one of: "
            "WINDOWS_SCREENSHOTS, X11_SCREENSHOTS, WAYLAND_SCREENSHOTS, PIPEWIRE_SCREENSHOTS."
    )
endif()


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

        src/pipewire/ScreenshotPipeWire.hpp
        src/pipewire/ScreenshotPipeWire.cpp

        src/x11/ScreenshotX11.hpp
        src/x11/ScreenshotX11.cpp

        include/LibScreenshots.hpp
)

# Platform detection
if (X11_SCREENSHOTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_X11)
endif()

if (WAYLAND_SCREENSHOTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WAYLAND)
endif()

if (PIPEWIRE_SCREENSHOTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_PIPEWIRE)
endif()

if (WINDOWS_SCREENSHOTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WINDOW)
endif()

include(Graphics)
#include(Windows)
#include(Wayland)
#include(PipeWire)
#include(X11)

set(LIBGRAPHICS_INCLUDE_DIRS "/home/johnny/CLionProjects/shot/build/_deps/libgraphics-src/include")
message(STATUS "🍺 LIBGRAPHICS_INCLUDE_DIRS = ${LIBGRAPHICS_INCLUDE_DIRS}")

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LIBGRAPHICS_INCLUDE_DIRS}
)