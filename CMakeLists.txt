cmake_minimum_required(VERSION 3.16)
project(LibScreenshots LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Platform detection
if(WIN32)
    set(IS_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(IS_LINUX TRUE)
endif()

# Optional Wayland toggle
option(WAYLAND "Enable Wayland support via DBus portal" ON)

# Core library
add_library(${PROJECT_NAME} SHARED
        src/main.cpp

        include/LibScreenshots/IScreenshotBackend.hpp
        include/LibScreenshots/ScreenshotResult.hpp

        src/wayland/ScreenshotWayland.hpp
        src/wayland/ScreenshotWayland.cpp
        include/LibScreenshots.hpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Platform-specific dependencies
if(IS_WINDOWS)
    message(STATUS "🖥️ Configuring Windows backend")
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE user32 gdi32)

elseif(IS_LINUX)
    if(WAYLAND)
        message(STATUS "✅ Configuring Wayland backend via DBus portal")

        target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WAYLAND)

        find_package(PkgConfig REQUIRED)
        pkg_check_modules(DBUS REQUIRED dbus-1)
        pkg_check_modules(GIO REQUIRED gio-2.0)
        pkg_check_modules(GLIB REQUIRED glib-2.0)

        target_include_directories(${PROJECT_NAME} PRIVATE
                ${DBUS_INCLUDE_DIRS}
                ${GIO_INCLUDE_DIRS}
                ${GLIB_INCLUDE_DIRS}
        )

        target_link_libraries(${PROJECT_NAME} PRIVATE
                ${DBUS_LIBRARIES}
                ${GIO_LIBRARIES}
                ${GLIB_LIBRARIES}
        )

        target_compile_options(${PROJECT_NAME} PRIVATE
                ${DBUS_CFLAGS_OTHER}
                ${GIO_CFLAGS_OTHER}
                ${GLIB_CFLAGS_OTHER}
        )
    else()
        message(STATUS "🖥️ Configuring X11 backend")
        target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_X11)
        find_package(X11 REQUIRED)
        target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
    endif()
endif()
