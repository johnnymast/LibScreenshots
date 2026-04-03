cmake_minimum_required(VERSION 3.16)
project(LibScreenshots)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT (WINDOWS_SCREENSHOTS OR X11_SCREENSHOTS OR WLROOTS_SCREEN_COPY_SCREENSHOTS))
    message(FATAL_ERROR
            "LibScreenshots: No backend selected! Enable at least one of: "
            "WINDOWS_SCREENSHOTS, X11_SCREENSHOTS, WLROOTS_SCREEN_COPY_SCREENSHOTS."
    )
endif()

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

if (WINDOWS_SCREENSHOTS)
    message(STATUS "🪟  Enabling Windows backend")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WINDOWS)

    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotWindows.hpp
            src/backends/ScreenshotWindows.cpp
    )

    include(Windows OPTIONAL)
endif()

if (X11_SCREENSHOTS)
    message(STATUS "🖥️  Enabling X11 backend")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_X11)

    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotX11.hpp
            src/backends/ScreenshotX11.cpp
    )

    include(X11 OPTIONAL)
endif()

if (WLROOTS_SCREEN_COPY_SCREENSHOTS)
    message(STATUS "📡  Enabling WLRoots Screen copy protocol")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WLROOTS_SCREEN_COPY)

    target_sources(${PROJECT_NAME} PRIVATE
            include/private/LibScreenshots/backends/ScreenshotWLRoots.hpp
            src/backends/ScreenshotWLRoots.cpp
    )

    include(WLRoots OPTIONAL)

endif()

include(Graphics)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/public
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/private
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        LibGraphics
)


# ============================================
# Install LibGraphics
# ============================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 1. Install the library
install(TARGETS LibGraphics
        EXPORT LibGraphicsTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 2. Install public headers
install(DIRECTORY include/LibGraphics
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 3. Export targets
install(EXPORT LibGraphicsTargets
        FILE LibGraphicsTargets.cmake
        NAMESPACE LibGraphics::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibGraphics
)

# 4. Generate version file
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/LibGraphicsConfigVersion.cmake"
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

# 5. Config file
configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LibGraphicsConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LibGraphicsConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibGraphics
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/LibGraphicsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/LibGraphicsConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibGraphics
)