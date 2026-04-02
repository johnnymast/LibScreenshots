cmake_minimum_required(VERSION 3.16)
project(LibScreenshots)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Require at least one backend
if(NOT (WINDOWS_SCREENSHOTS OR X11_SCREENSHOTS OR WLROOTS_SCREEN_COPY_SCREENSHOTS))
    message(FATAL_ERROR
            "LibScreenshots: No backend selected! Enable at least one of: "
            "WINDOWS_SCREENSHOTS, X11_SCREENSHOTS, WLROOTS_SCREEN_COPY_SCREENSHOTS."
    )
endif()

# Core library (backend‑agnostic sources only)
add_library(${PROJECT_NAME} SHARED
        src/main.cpp

        include/private/LibScreenshots/BackendSelector.hpp
        include/private/LibScreenshots/ScreenshotResult.hpp

        include/public/LibScreenshots/IScreenshotBackend.hpp
        include/public/LibScreenshots/LibScreenshots.hpp
        include/public/LibScreenshots/export.hpp
)

#
# Backend selection
#

# --- Windows backend ---
if (WINDOWS_SCREENSHOTS)
    message(STATUS "🪟  Enabling Windows backend")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WINDOWS)

    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotWindows.hpp
            src/backends/ScreenshotWindows.cpp
    )

    include(Windows OPTIONAL)
endif()

# --- X11 backend ---
if (X11_SCREENSHOTS)
    message(STATUS "🖥️  Enabling X11 backend")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_X11)

    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotX11.hpp
            src/backends/ScreenshotX11.cpp
    )

    include(X11 OPTIONAL)
endif()

# --- Wayland backend ---
if (WLROOTS_SCREEN_COPY_SCREENSHOTS)
    message(STATUS "📡  Enabling WLRoots Screen copy protocol")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WLROOTS_SCREEN_COPY)

    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotWLRoots.hpp
            src/backends/ScreenshotWLRoots.cpp
    )

    include(WLRoots OPTIONAL)

endif()

# Graphics dependency
include(Graphics)

# Public include dirs
target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/public
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LIBGRAPHICS_INCLUDE_DIRS}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/private
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
