cmake_minimum_required(VERSION 3.16)
project(LibScreenshots)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Optional Wayland toggle
option(WAYLAND "Enable Wayland support via DBus portal" ON)
option(PIPEWIRE "Enable PipeWire support for screen capture" OFF)

if (NOT WAYLAND)
    set(X11 ON)
else ()
    set(X11 OFF)
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
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)

    set(X11 OFF)
    set(WAYLAND OFF)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_LINUX)

    if (X11)
        target_compile_definitions(LibScreenshots PUBLIC HAVE_X11)
    endif ()

    if (WAYLAND)
        target_compile_definitions(LibScreenshots PUBLIC HAVE_WAYLAND)
    endif ()
endif ()


include(Graphics)
include(Windows)
include(Wayland)
include(PipeWire)
include(X11)

set(LIBGRAPHICS_INCLUDE_DIRS "/home/johnny/CLionProjects/shot/build/_deps/libgraphics-src/include")
message(STATUS "🍺 LIBGRAPHICS_INCLUDE_DIRS = ${LIBGRAPHICS_INCLUDE_DIRS}")

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LIBGRAPHICS_INCLUDE_DIRS}
)