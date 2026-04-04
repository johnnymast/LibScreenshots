cmake_minimum_required(VERSION 3.16)
project(LibScreenshots)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT (WINDOWS_SCREENSHOTS OR X11_SCREENSHOTS OR WLROOTS_SCREEN_COPY_SCREENSHOTS))
    set(WLROOTS_SCREEN_COPY_SCREENSHOTS ON CACHE BOOL "Enable wlroots backend" FORCE)
endif()

add_library(${PROJECT_NAME} SHARED
        src/main.cpp

        include/private/LibScreenshots/BackendSelector.hpp
        include/private/LibScreenshots/ScreenshotResult.hpp

        include/public/LibScreenshots/IScreenshotBackend.hpp
        include/public/LibScreenshots/LibScreenshots.hpp
        include/public/LibScreenshots/export.hpp
)

if (WINDOWS_SCREENSHOTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WINDOWS)
    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotWindows.hpp
            src/backends/ScreenshotWindows.cpp
    )
    include(Windows OPTIONAL)
endif()

if (X11_SCREENSHOTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_X11)
    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotX11.hpp
            src/backends/ScreenshotX11.cpp
    )
    include(X11 OPTIONAL)
endif()

if (WLROOTS_SCREEN_COPY_SCREENSHOTS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WLROOTS_SCREEN_COPY)
    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotWLRoots.hpp
            src/backends/ScreenshotWLRoots.cpp
    )
    include(WLRoots OPTIONAL)
endif()

find_package(LibGraphics REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        LibGraphics::LibGraphics
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/public>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/private
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS LibScreenshots
        EXPORT LibScreenshotsTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/public/LibScreenshots
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT LibScreenshotsTargets
        FILE LibScreenshotsTargets.cmake
        NAMESPACE LibScreenshots::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibScreenshots
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/LibScreenshotsConfigVersion.cmake"
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibScreenshotsConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LibScreenshotsConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibScreenshots
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/LibScreenshotsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/LibScreenshotsConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibScreenshots
)
